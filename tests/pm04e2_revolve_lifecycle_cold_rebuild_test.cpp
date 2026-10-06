#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04E2 Revolve lifecycle/cold-rebuild CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double pi =
    std::numbers::pi_v<double>;

bool near(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1.0e-12;
}

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm04e2_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

part::AxisReference authoredAxis(
    part::AxisId id) {
    return part::AxisReference{
        part::AuthoredAxisReference{id}};
}

class ColdRevolveSolid final
    : public kernel::RuntimeSolid {
public:
    explicit ColdRevolveSolid(
        std::uint64_t generation_value)
        : generation{generation_value} {}

    std::uint64_t generation{};
};

class ColdRevolveKernel final
    : public kernel::ISolidModelingKernel {
public:
    explicit ColdRevolveKernel(
        std::uint64_t generation_value)
        : generation{generation_value} {}

    std::uint64_t generation{};
    std::size_t calls{};
    std::vector<kernel::AngularRevolveInput>
        inputs;

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        kernel::SolidModelingResult result;
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    kernel::SolidModelingResult revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        ++calls;
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }
        if (upstream != nullptr &&
            dynamic_cast<const ColdRevolveSolid*>(
                upstream.get()) == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_mismatch;
            return result;
        }

        inputs.push_back(input);
        result.solid =
            std::make_shared<ColdRevolveSolid>(
                generation);

        std::uint64_t next_face = 1U;
        std::uint64_t next_surface = 1U;
        auto publish =
            [&](kernel::GeneratedFaceRole role,
                kernel::SurfaceKind kind,
                std::optional<kernel::Frame3> frame =
                    std::nullopt) {
                const kernel::RuntimeFaceToken face{
                    next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    next_surface++};
                result.current_faces.push_back(face);
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        face,
                    });
                result.new_surfaces.push_back(
                    {
                        std::move(role),
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kind,
                        std::move(frame),
                        surface,
                        {face},
                    });
            };

        if (!input.fullTurn()) {
            publish(
                {
                    kernel::GeneratedFaceRoleKind::
                        revolve_start_cap,
                    std::nullopt,
                    std::nullopt,
                },
                kernel::SurfaceKind::plane,
                input.profile.frame);
            publish(
                {
                    kernel::GeneratedFaceRoleKind::
                        revolve_end_cap,
                    std::nullopt,
                    std::nullopt,
                },
                kernel::SurfaceKind::plane,
                input.profile.frame);
        }

        const auto publish_loop =
            [&](const kernel::ProfileLoopInput& loop) {
                for (const auto& use :
                     loop.boundary) {
                    publish(
                        {
                            kernel::GeneratedFaceRoleKind::
                                revolve_side,
                            std::nullopt,
                            use.provenance,
                        },
                        kernel::SurfaceKind::other);
                }
            };
        publish_loop(input.profile.outer);
        for (const auto& hole : input.profile.holes) {
            publish_loop(hole);
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count = 0U;
        result.vertex_count = 0U;
        return result;
    }
};

struct AuthoredIds final {
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> profile_edges;
    sketch::EntityId axis_line_id;
    part::ProfileId profile_id;
    part::AxisId axis_id;
    part::FeatureId feature_id;
};

AuthoredIds authorRevolve(
    application::DocumentSession& session,
    ColdRevolveKernel& kernel) {
    const auto sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch.sketch_id,
                session.document().revision(),
                {10.0, 10.0},
                {20.0, 20.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto axis_line =
        session.execute(
            application::AddSketchLineCommand{
                *sketch.sketch_id,
                {0.0, 0.0},
                {30.0, 0.0},
                sketch::EntityRole::
                    construction});
    CHECK(
        axis_line.ok() &&
        axis_line.entity_id);

    const auto* source =
        session.document().findSketch(
            *sketch.sketch_id);
    CHECK(source != nullptr);
    const auto regions =
        sketch::analyzeRegions(
            source->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id);

    const auto axis =
        session.execute(
            application::CreateAxisCommand{
                {
                    *sketch.sketch_id,
                    *axis_line.entity_id,
                },
                session.document().revision(),
                "Axis001",
                false},
            kernel);
    CHECK(axis.ok() && axis.axis_id);

    const auto revolve =
        session.execute(
            application::CreateRevolveFeatureCommand{
                *profile.profile_id,
                authoredAxis(*axis.axis_id),
                session.document().revision(),
                part::RevolveOperation::add,
                part::OneSidedRevolveExtent{
                    core::AngleValue{pi / 2.0},
                    false},
                "Revolve001"},
            kernel);
    CHECK(
        revolve.ok() &&
        revolve.feature_id);

    return {
        *sketch.sketch_id,
        rectangle.entity_ids,
        *axis_line.entity_id,
        *profile.profile_id,
        *axis.axis_id,
        *revolve.feature_id,
    };
}

part::PartEvaluation evaluate(
    const part::PartDocument& document,
    ColdRevolveKernel& kernel) {
    return part::evaluatePart(
        document,
        kernel);
}

void checkUpToDate(
    const part::PartEvaluation& evaluation,
    part::FeatureId feature_id,
    std::uint64_t provider_generation) {
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    const auto* feature =
        evaluation.findFeature(feature_id);
    CHECK(feature != nullptr);
    CHECK(
        feature->status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        feature->diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            none);
    const auto* solid =
        dynamic_cast<const ColdRevolveSolid*>(
            evaluation.body_solid.get());
    CHECK(solid != nullptr);
    CHECK(
        solid->generation ==
        provider_generation);
}

void checkBlocked(
    const part::PartEvaluation& evaluation,
    part::FeatureId feature_id,
    part::FeatureEvaluationDiagnosticCode diagnostic) {
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(evaluation.body_solid == nullptr);
    const auto* feature =
        evaluation.findFeature(feature_id);
    CHECK(feature != nullptr);
    CHECK(
        feature->status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(feature->diagnostic == diagnostic);
    CHECK(feature->result_solid == nullptr);
    CHECK(!feature->result_topology.has_value());
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path /
        "RevolveLifecycle.ss2part";
    part::PartDocumentStore store;
    const auto document_id =
        core::DocumentId::generate();

    const auto ids = [&] {
        auto document =
            part::PartDocument::create(
                document_id);
        const auto created =
            store.createNew(
                path,
                document);
        CHECK(created.ok());

        application::DocumentSession session{
            path,
            std::move(document),
            *created.checkpoint};

        ColdRevolveKernel authoring_kernel{1U};
        const auto authored =
            authorRevolve(
                session,
                authoring_kernel);

        CHECK(
            session.document()
                .findAxis(authored.axis_id) !=
            nullptr);
        CHECK(
            session.document()
                .findFeature(authored.feature_id) !=
            nullptr);

        ColdRevolveKernel initial_kernel{2U};
        const auto initial =
            evaluate(
                session.document(),
                initial_kernel);
        checkUpToDate(
            initial,
            authored.feature_id,
            2U);
        CHECK(initial_kernel.calls == 1U);
        CHECK(initial_kernel.inputs.size() == 1U);
        CHECK(
            near(
                initial_kernel.inputs.front()
                    .axis.direction.x,
                1.0));
        CHECK(
            near(
                initial_kernel.inputs.front()
                    .axis.direction.y,
                0.0));

        CHECK(session.save().ok());
        CHECK(!session.needsSave());
        return authored;
    }();

    // True cold rebuild: no prior DocumentSession, evaluation result, runtime
    // solid or provider generation survives the load.
    auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->documentId() ==
        document_id);
    CHECK(
        loaded.document->findAxis(
            ids.axis_id) != nullptr);
    CHECK(
        loaded.document->findFeature(
            ids.feature_id) != nullptr);

    ColdRevolveKernel cold_kernel{100U};
    const auto cold =
        evaluate(
            *loaded.document,
            cold_kernel);
    checkUpToDate(
        cold,
        ids.feature_id,
        100U);
    CHECK(cold_kernel.calls == 1U);
    CHECK(cold_kernel.inputs.size() == 1U);

    application::DocumentSession reopened{
        path,
        std::move(*loaded.document),
        *loaded.checkpoint};

    // Axis source geometry edit recomputes downstream from current semantic
    // source. The provider receives the new direction, not last-good geometry.
    CHECK(
        reopened.execute(
            application::UpdateSketchLinesCommand{
                ids.sketch_id,
                reopened.document().revision(),
                {{
                    ids.axis_line_id,
                    {0.0, 0.0},
                    {0.0, 30.0},
                }}})
            .ok());

    ColdRevolveKernel moved_kernel{101U};
    auto moved =
        evaluate(
            reopened.document(),
            moved_kernel);
    checkUpToDate(
        moved,
        ids.feature_id,
        101U);
    CHECK(moved_kernel.calls == 1U);
    CHECK(
        near(
            moved_kernel.inputs.front()
                .axis.direction.x,
            0.0));
    CHECK(
        near(
            moved_kernel.inputs.front()
                .axis.direction.y,
            1.0));

    // Deleting the source Line keeps Axis intent but makes the Axis
    // unavailable. No provider call and no stale last-good Body are allowed.
    CHECK(
        reopened.execute(
            application::EraseSketchEntityCommand{
                ids.sketch_id,
                ids.axis_line_id})
            .ok());
    CHECK(
        reopened.document().findAxis(
            ids.axis_id) != nullptr);

    ColdRevolveKernel unavailable_kernel{102U};
    auto unavailable =
        evaluate(
            reopened.document(),
            unavailable_kernel);
    CHECK(unavailable_kernel.calls == 0U);
    checkBlocked(
        unavailable,
        ids.feature_id,
        part::FeatureEvaluationDiagnosticCode::
            axis_unavailable);

    const auto replacement =
        reopened.execute(
            application::AddSketchLineCommand{
                ids.sketch_id,
                {-30.0, 0.0},
                {30.0, 0.0},
                sketch::EntityRole::
                    construction});
    CHECK(
        replacement.ok() &&
        replacement.entity_id);

    CHECK(
        reopened.execute(
            application::EditAxisCommand{
                ids.axis_id,
                {
                    ids.sketch_id,
                    *replacement.entity_id,
                },
                reopened.document().revision()},
            unavailable_kernel)
            .ok());
    CHECK(
        reopened.document()
            .findAxis(ids.axis_id)
            ->id == ids.axis_id);

    ColdRevolveKernel repaired_kernel{103U};
    auto repaired =
        evaluate(
            reopened.document(),
            repaired_kernel);
    checkUpToDate(
        repaired,
        ids.feature_id,
        103U);
    CHECK(repaired_kernel.calls == 1U);

    // Undo/Redo around explicit Axis re-source toggles between the repairable
    // blocked intent and the repaired same-AxisId state.
    CHECK(reopened.undo().ok());
    ColdRevolveKernel undo_repair_kernel{104U};
    auto undo_repair =
        evaluate(
            reopened.document(),
            undo_repair_kernel);
    CHECK(undo_repair_kernel.calls == 0U);
    checkBlocked(
        undo_repair,
        ids.feature_id,
        part::FeatureEvaluationDiagnosticCode::
            axis_unavailable);

    CHECK(reopened.redo().ok());
    ColdRevolveKernel redo_repair_kernel{105U};
    auto redo_repair =
        evaluate(
            reopened.document(),
            redo_repair_kernel);
    checkUpToDate(
        redo_repair,
        ids.feature_id,
        105U);

    // Deleting the Axis object is a distinct MissingAxis state. Authored
    // Revolve intent retains the same AxisId and Undo restores that exact ID.
    CHECK(
        reopened.execute(
            application::DeleteAxisCommand{
                ids.axis_id,
                reopened.document().revision()})
            .ok());
    CHECK(
        reopened.document().findAxis(
            ids.axis_id) == nullptr);
    const auto* missing_feature =
        reopened.document().findFeature(
            ids.feature_id);
    CHECK(missing_feature != nullptr);
    const auto* missing_revolve =
        std::get_if<part::RevolveFeature>(
            &missing_feature->definition);
    CHECK(missing_revolve != nullptr);
    CHECK(
        part::authoredAxisIdForAxisReference(
            missing_revolve->axis) ==
        ids.axis_id);

    ColdRevolveKernel missing_kernel{106U};
    auto missing =
        evaluate(
            reopened.document(),
            missing_kernel);
    CHECK(missing_kernel.calls == 0U);
    checkBlocked(
        missing,
        ids.feature_id,
        part::FeatureEvaluationDiagnosticCode::
            missing_axis);

    CHECK(reopened.undo().ok());
    CHECK(
        reopened.document().findAxis(
            ids.axis_id) != nullptr);
    CHECK(
        reopened.document()
            .findAxis(ids.axis_id)
            ->id == ids.axis_id);
    ColdRevolveKernel restored_axis_kernel{107U};
    auto restored_axis =
        evaluate(
            reopened.document(),
            restored_axis_kernel);
    checkUpToDate(
        restored_axis,
        ids.feature_id,
        107U);

    CHECK(reopened.redo().ok());
    ColdRevolveKernel redone_delete_kernel{108U};
    auto redone_delete =
        evaluate(
            reopened.document(),
            redone_delete_kernel);
    CHECK(redone_delete_kernel.calls == 0U);
    checkBlocked(
        redone_delete,
        ids.feature_id,
        part::FeatureEvaluationDiagnosticCode::
            missing_axis);
    CHECK(reopened.undo().ok());

    // Missing Profile is also durable blocked intent; Undo restores the same
    // ProfileId and the Revolve evaluates normally again.
    CHECK(
        reopened.execute(
            application::DeleteProfileCommand{
                ids.profile_id,
                reopened.document().revision()})
            .ok());
    CHECK(
        reopened.document().findProfile(
            ids.profile_id) == nullptr);
    ColdRevolveKernel missing_profile_kernel{109U};
    auto missing_profile =
        evaluate(
            reopened.document(),
            missing_profile_kernel);
    CHECK(missing_profile_kernel.calls == 0U);
    checkBlocked(
        missing_profile,
        ids.feature_id,
        part::FeatureEvaluationDiagnosticCode::
            missing_profile);

    CHECK(reopened.undo().ok());
    CHECK(
        reopened.document().findProfile(
            ids.profile_id) != nullptr);
    ColdRevolveKernel restored_profile_kernel{110U};
    auto restored_profile =
        evaluate(
            reopened.document(),
            restored_profile_kernel);
    checkUpToDate(
        restored_profile,
        ids.feature_id,
        110U);

    // Generic Feature lifecycle must apply to Revolve without a parallel API.
    CHECK(
        reopened.execute(
            application::SetFeatureSuppressedCommand{
                ids.feature_id,
                reopened.document().revision(),
                true})
            .ok());
    ColdRevolveKernel suppressed_kernel{111U};
    const auto suppressed =
        evaluate(
            reopened.document(),
            suppressed_kernel);
    CHECK(suppressed_kernel.calls == 0U);
    const auto* suppressed_feature =
        suppressed.findFeature(
            ids.feature_id);
    CHECK(suppressed_feature != nullptr);
    CHECK(
        suppressed_feature->status ==
        part::FeatureEvaluationStatus::
            suppressed);

    CHECK(reopened.undo().ok());
    ColdRevolveKernel unsuppressed_kernel{112U};
    auto unsuppressed =
        evaluate(
            reopened.document(),
            unsuppressed_kernel);
    checkUpToDate(
        unsuppressed,
        ids.feature_id,
        112U);

    CHECK(
        reopened.execute(
            application::DeleteFeatureCommand{
                ids.feature_id,
                reopened.document().revision()})
            .ok());
    CHECK(
        reopened.document().findFeature(
            ids.feature_id) == nullptr);
    CHECK(
        reopened.document().body()
            .features.empty());

    CHECK(reopened.undo().ok());
    CHECK(
        reopened.document().findFeature(
            ids.feature_id) != nullptr);
    CHECK(
        reopened.document()
            .findFeature(ids.feature_id)
            ->id == ids.feature_id);
    CHECK(reopened.redo().ok());
    CHECK(
        reopened.document().findFeature(
            ids.feature_id) == nullptr);
    CHECK(reopened.undo().ok());

    // Persist the repaired active state and cold-load again. All authored IDs
    // survive while the provider generation is freshly reconstructed.
    CHECK(reopened.save().ok());
    CHECK(!reopened.needsSave());

    auto final_load =
        store.load(path);
    CHECK(final_load.ok());
    CHECK(
        final_load.document->documentId() ==
        document_id);
    CHECK(
        final_load.document->findAxis(
            ids.axis_id) != nullptr);
    CHECK(
        final_load.document->findProfile(
            ids.profile_id) != nullptr);
    CHECK(
        final_load.document->findFeature(
            ids.feature_id) != nullptr);

    ColdRevolveKernel final_kernel{1000U};
    const auto final_evaluation =
        evaluate(
            *final_load.document,
            final_kernel);
    checkUpToDate(
        final_evaluation,
        ids.feature_id,
        1000U);
    CHECK(final_kernel.calls == 1U);
    CHECK(
        near(
            final_kernel.inputs.front()
                .axis.direction.x,
            1.0));
    CHECK(
        near(
            final_kernel.inputs.front()
                .axis.direction.y,
            0.0));

    std::cout
        << "PM-04E2 Revolve lifecycle/cold-rebuild tests passed\n";
    return EXIT_SUCCESS;
}
