#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02I lifecycle survival CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm02i_" +
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

enum class SupportMode {
    resolved,
    split_face,
    missing_with_same_geometry_decoy,
    alias_ambiguous,
};

class LifecycleSolid final
    : public kernel::RuntimeSolid {
public:
    struct Surface final {
        kernel::RuntimeSurfaceToken surface;
        kernel::SurfaceKind kind{
            kernel::SurfaceKind::plane};
        std::optional<kernel::Frame3> frame;
        std::vector<kernel::RuntimeFaceToken> faces;
    };

    std::vector<Surface> surfaces;
    std::uint64_t next_face{};
    std::uint64_t next_surface{};
};

kernel::Frame3 offsetFrame(
    const kernel::Frame3& source,
    double offset) {
    auto result = source;
    result.origin.x += result.normal.x * offset;
    result.origin.y += result.normal.y * offset;
    result.origin.z += result.normal.z * offset;
    return result;
}

class LifecycleKernel final
    : public kernel::ISolidModelingKernel {
public:
    explicit LifecycleKernel(
        std::uint64_t token_seed)
        : token_seed_{token_seed} {}

    SupportMode support_mode{
        SupportMode::resolved};
    std::vector<kernel::LinearExtrudeInput> inputs;

    void reset(
        SupportMode mode =
            SupportMode::resolved) {
        support_mode = mode;
        inputs.clear();
    }

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
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

        const std::size_t call_index =
            inputs.size();
        inputs.push_back(input);

        auto runtime =
            std::make_shared<LifecycleSolid>();
        if (upstream != nullptr) {
            const auto* previous =
                dynamic_cast<const LifecycleSolid*>(
                    upstream.get());
            if (previous == nullptr) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }

            runtime->surfaces =
                previous->surfaces;
            runtime->next_face =
                previous->next_face;
            runtime->next_surface =
                previous->next_surface;

            for (const auto& inherited :
                 previous->surfaces) {
                for (const auto face :
                     inherited.faces) {
                    result.current_faces.push_back(
                        face);
                    result.inherited_faces.push_back(
                        {
                            face,
                            kernel::ReferenceStatus::
                                resolved,
                            1U,
                        });
                }
                result.inherited_surfaces.push_back(
                    {
                        inherited.surface,
                        kernel::ReferenceStatus::
                            resolved,
                        inherited.faces.size() == 1U
                            ? kernel::ReferenceStatus::
                                  resolved
                            : kernel::ReferenceStatus::
                                  ambiguous,
                        inherited.faces.size(),
                        inherited.kind,
                        inherited.frame,
                        inherited.faces,
                    });
            }
        } else {
            runtime->next_face =
                token_seed_;
            runtime->next_surface =
                token_seed_ * 10U;
        }

        const auto add_resolved_surface =
            [&result, &runtime](
                const kernel::ExtrudeFaceRole& role,
                const kernel::Frame3& frame,
                std::size_t face_count) {
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};
                std::vector<kernel::RuntimeFaceToken>
                    faces;
                for (std::size_t index = 0U;
                     index < face_count;
                     ++index) {
                    const kernel::RuntimeFaceToken face{
                        runtime->next_face++};
                    faces.push_back(face);
                    result.current_faces.push_back(face);
                }

                runtime->surfaces.push_back(
                    LifecycleSolid::Surface{
                        surface,
                        kernel::SurfaceKind::plane,
                        frame,
                        faces});

                result.new_faces.push_back(
                    {
                        role,
                        face_count == 1U
                            ? kernel::ReferenceStatus::
                                  resolved
                            : kernel::ReferenceStatus::
                                  ambiguous,
                        face_count,
                        face_count == 1U
                            ? std::optional<
                                  kernel::RuntimeFaceToken>{
                                  faces.front()}
                            : std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        face_count == 1U
                            ? kernel::ReferenceStatus::
                                  resolved
                            : kernel::ReferenceStatus::
                                  ambiguous,
                        face_count,
                        kernel::SurfaceKind::plane,
                        frame,
                        surface,
                        faces,
                    });
            };

        const auto add_missing_surface =
            [&result](
                const kernel::ExtrudeFaceRole& role) {
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            missing,
                        0U,
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            missing,
                        kernel::ReferenceStatus::
                            missing,
                        0U,
                        kernel::SurfaceKind::plane,
                        std::nullopt,
                        std::nullopt,
                        {},
                    });
            };

        const auto add_ambiguous_surface =
            [&result, &runtime](
                const kernel::ExtrudeFaceRole& role,
                const kernel::Frame3& frame) {
                std::vector<kernel::RuntimeFaceToken>
                    faces{
                        kernel::RuntimeFaceToken{
                            runtime->next_face++},
                        kernel::RuntimeFaceToken{
                            runtime->next_face++}};
                for (const auto face : faces) {
                    result.current_faces.push_back(face);
                }
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            ambiguous,
                        faces.size(),
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            ambiguous,
                        kernel::ReferenceStatus::
                            ambiguous,
                        faces.size(),
                        kernel::SurfaceKind::plane,
                        std::nullopt,
                        std::nullopt,
                        faces,
                    });
            };

        const kernel::ExtrudeFaceRole start_role{
            kernel::ExtrudeGeneratedFaceRoleKind::
                cap,
            input.start_cap_role,
            std::nullopt};
        const kernel::ExtrudeFaceRole end_role{
            kernel::ExtrudeGeneratedFaceRoleKind::
                cap,
            input.end_cap_role,
            std::nullopt};

        const auto start_frame =
            offsetFrame(
                input.profile.frame,
                (call_index == 0U &&
                 support_mode ==
                     SupportMode::
                         missing_with_same_geometry_decoy)
                    ? input.end_offset_mm
                    : input.start_offset_mm);
        add_resolved_surface(
            start_role,
            start_frame,
            1U);

        if (call_index == 0U &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    extent_cap) {
            const auto target_frame =
                offsetFrame(
                    input.profile.frame,
                    input.end_offset_mm);
            switch (support_mode) {
            case SupportMode::resolved:
                add_resolved_surface(
                    end_role,
                    target_frame,
                    1U);
                break;
            case SupportMode::split_face:
                // The semantic Surface survives while its strict bounded Face
                // realization splits. Sketch support must remain Resolved.
                add_resolved_surface(
                    end_role,
                    target_frame,
                    2U);
                break;
            case SupportMode::
                missing_with_same_geometry_decoy:
                // The start/profile cap above is deliberately placed on the
                // exact target plane. Geometry equality must not revive the
                // Missing extent-cap meaning.
                add_missing_surface(
                    end_role);
                break;
            case SupportMode::alias_ambiguous:
                add_ambiguous_surface(
                    end_role,
                    target_frame);
                break;
            }
        } else {
            add_resolved_surface(
                end_role,
                offsetFrame(
                    input.profile.frame,
                    input.end_offset_mm),
                1U);
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid = std::move(runtime);
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count = 0U;
        result.vertex_count = 0U;
        return result;
    }

private:
    std::uint64_t token_seed_{1U};
};

part::ProfileId createRectangleProfile(
    application::DocumentSession& session,
    sketch::SketchId sketch_id,
    sketch::Point2 first,
    sketch::Point2 opposite) {
    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                first,
                opposite,
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto regions =
        sketch::analyzeRegions(hosted->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok());
    CHECK(profile.profile_id.has_value());
    return *profile.profile_id;
}

const part::FeatureSurfaceResolution*
findSupportSurface(
    const part::PartEvaluation& evaluation,
    const part::SurfaceReference& reference) {
    const auto* feature =
        evaluation.findFeature(
            *reference.stage.feature_id);
    if (feature == nullptr ||
        !feature->result_topology) {
        return nullptr;
    }
    const auto found =
        std::find_if(
            feature->result_topology->surfaces.begin(),
            feature->result_topology->surfaces.end(),
            [&reference](const auto& surface) {
                return surface.address ==
                       reference.surface;
            });
    return found ==
               feature->result_topology->surfaces.end()
        ? nullptr
        : &*found;
}

struct Fixture final {
    std::filesystem::path path;
    core::DocumentId document_id;
    part::BodyId body_id;
    sketch::SketchId face_sketch_id;
    part::ProfileId base_profile_id;
    part::ProfileId face_profile_id;
    part::FeatureId base_feature_id;
    part::FeatureId add_feature_id;
    part::FeatureId cut_feature_id;
    part::PartSketchSupport face_support;
    part::SurfaceReference surface_reference;
    sketch::SketchModelState local_state;
    std::optional<kernel::RuntimeSurfaceToken>
        initial_runtime_surface;
    application::DocumentSession session;
};

Fixture makeFixture(
    const std::filesystem::path& path,
    LifecycleKernel& provider) {
    part::PartDocumentStore store;
    const auto document_id =
        core::DocumentId::generate();
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

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(base_sketch.ok());
    CHECK(base_sketch.sketch_id.has_value());

    const auto base_profile_id =
        createRectangleProfile(
            session,
            *base_sketch.sketch_id,
            {0.0, 0.0},
            {40.0, 30.0});

    provider.reset();
    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                base_profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            provider);
    CHECK(base.ok());
    CHECK(base.feature_id.has_value());

    provider.reset();
    const auto base_evaluation =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        base_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(base_evaluation.features.size() == 1U);
    CHECK(
        base_evaluation.features[0]
            .result_topology.has_value());

    const auto& base_topology =
        *base_evaluation.features[0]
             .result_topology;
    const auto surface =
        std::find_if(
            base_topology.surfaces.begin(),
            base_topology.surfaces.end(),
            [feature_id = *base.feature_id](
                const auto& candidate) {
                return candidate.status ==
                           kernel::ReferenceStatus::
                               resolved &&
                       candidate.address
                               .producer_feature_id ==
                           feature_id &&
                       candidate.address.role ==
                           part::FeatureSurfaceRoleKind::
                               extent_cap;
            });
    CHECK(surface !=
          base_topology.surfaces.end());
    CHECK(surface->runtime_token.has_value());

    const part::SurfaceReference
        surface_reference{
            base_topology.stage,
            surface->address};
    CHECK(surface_reference.valid());

    const auto support =
        part::partSketchSupportForBodyPlanarSurface(
            surface_reference);
    CHECK(support.has_value());

    const auto face_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *support,
                session.document().revision()},
            &provider);
    CHECK(face_sketch.ok());
    CHECK(face_sketch.sketch_id.has_value());

    const auto face_profile_id =
        createRectangleProfile(
            session,
            *face_sketch.sketch_id,
            {2.0, 3.0},
            {8.0, 9.0});

    provider.reset();
    const auto add =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                face_profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{4.0},
                    false},
                "Face Add"},
            provider);
    CHECK(add.ok());
    CHECK(add.feature_id.has_value());

    provider.reset();
    const auto cut =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                face_profile_id,
                session.document().revision(),
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{2.0},
                    false},
                "Face Cut"},
            provider);
    CHECK(cut.ok());
    CHECK(cut.feature_id.has_value());

    const auto* hosted =
        session.document().findSketch(
            *face_sketch.sketch_id);
    CHECK(hosted != nullptr);

    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    return {
        path,
        document_id,
        session.document().body().id,
        *face_sketch.sketch_id,
        base_profile_id,
        face_profile_id,
        *base.feature_id,
        *add.feature_id,
        *cut.feature_id,
        *support,
        surface_reference,
        hosted->model.state(),
        surface->runtime_token,
        std::move(session)};
}

void verifyBodyUpToDate(
    const part::PartDocument& document,
    LifecycleKernel& provider) {
    const auto evaluation =
        part::evaluatePart(
            document,
            provider);
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(evaluation.features.size() == 3U);
    CHECK(provider.inputs.size() == 3U);
    for (const auto& feature :
         evaluation.features) {
        CHECK(
            feature.status ==
            part::FeatureEvaluationStatus::
                up_to_date);
    }
}

void verifyFailedSupport(
    const part::PartDocument& document,
    LifecycleKernel& provider,
    const part::PartSketchSupport& support,
    part::SketchSupportResolutionStatus
        expected_support_status,
    part::SketchSupportResolutionDiagnostic
        expected_support_diagnostic,
    part::FeatureEvaluationDiagnosticCode
        expected_feature_diagnostic) {
    const auto evaluation =
        part::evaluatePart(
            document,
            provider);
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(evaluation.features.size() == 3U);
    CHECK(
        evaluation.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        evaluation.features[1].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        evaluation.features[1].diagnostic ==
        expected_feature_diagnostic);
    CHECK(
        evaluation.features[2].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        evaluation.features[2].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            upstream_unavailable);
    CHECK(provider.inputs.size() == 1U);
    CHECK(
        evaluation.features[0]
            .result_topology.has_value());

    const auto resolved =
        part::resolveSketchSupport(
            support,
            &*evaluation.features[0]
                  .result_topology);
    CHECK(
        resolved.status ==
        expected_support_status);
    CHECK(
        resolved.diagnostic ==
        expected_support_diagnostic);
    CHECK(!resolved.frame.has_value());
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path /
        "FaceSupportedLifecycle.ss2part";

    LifecycleKernel authoring_provider{100U};
    auto fixture =
        makeFixture(
            path,
            authoring_provider);

    const auto* authored_sketch =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(authored_sketch != nullptr);
    CHECK(
        authored_sketch->support ==
        fixture.face_support);
    CHECK(
        authored_sketch->model.state() ==
        fixture.local_state);

    // Delete interactions: an upstream producer or a Profile still consumed
    // by downstream Features cannot be removed through a partial transaction.
    {
        const auto state_before =
            fixture.session.document().state();
        const auto revision_before =
            fixture.session.document().revision();
        const auto deleted =
            fixture.session.execute(
                application::DeleteFeatureCommand{
                    fixture.base_feature_id,
                    revision_before});
        CHECK(!deleted.ok());
        CHECK(!deleted.changed);
        CHECK(
            fixture.session.document().revision() ==
            revision_before);
        CHECK(
            fixture.session.document().state() ==
            state_before);
        CHECK(
            fixture.session.document().findFeature(
                fixture.base_feature_id) != nullptr);
    }
    {
        const auto state_before =
            fixture.session.document().state();
        const auto revision_before =
            fixture.session.document().revision();
        const auto deleted =
            fixture.session.execute(
                application::DeleteProfileCommand{
                    fixture.face_profile_id,
                    revision_before});
        CHECK(!deleted.ok());
        CHECK(!deleted.changed);
        CHECK(
            fixture.session.document().revision() ==
            revision_before);
        CHECK(
            fixture.session.document().state() ==
            state_before);
        CHECK(
            fixture.session.document().findProfile(
                fixture.face_profile_id) != nullptr);
    }

    // A stale semantic support command is rejected before provider work and
    // cannot partially mutate authored state.
    const auto stale_revision =
        fixture.session.document().revision();
    auto properties =
        fixture.session.document().properties();
    properties.title =
        "PM-02I revision advance";
    const auto changed_properties =
        fixture.session.execute(
            application::SetDocumentPropertiesCommand{
                std::move(properties)});
    CHECK(
        changed_properties.ok() &&
        changed_properties.changed);

    const auto yz_support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::
                yz_plane);
    CHECK(yz_support.has_value());

    const auto state_before_stale =
        fixture.session.document().state();
    authoring_provider.reset();
    const auto stale =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                fixture.face_sketch_id,
                *yz_support,
                stale_revision},
            &authoring_provider);
    CHECK(!stale.ok());
    CHECK(!stale.changed);
    CHECK(
        stale.status ==
        application::SketchSupportMutationStatus::
            stale_revision);
    CHECK(authoring_provider.inputs.empty());
    CHECK(
        fixture.session.document().state() ==
        state_before_stale);

    // A split bounded Face does not break the semantic planar Surface.
    authoring_provider.reset(
        SupportMode::split_face);
    const auto split =
        part::evaluatePart(
            fixture.session.document(),
            authoring_provider);
    CHECK(
        split.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(authoring_provider.inputs.size() == 3U);
    const auto* split_surface =
        findSupportSurface(
            split,
            fixture.surface_reference);
    CHECK(split_surface != nullptr);
    CHECK(
        split_surface->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        split_surface->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        split_surface->candidate_face_count == 2U);
    CHECK(
        split.features[0]
            .result_topology.has_value());
    const auto split_support =
        part::resolveSketchSupport(
            fixture.face_support,
            &*split.features[0]
                  .result_topology);
    CHECK(split_support.valid());
    CHECK(
        split_support.status ==
        part::SketchSupportResolutionStatus::
            resolved);
    CHECK(split_support.frame.has_value());

    // Deletion with an exact-geometry decoy remains Missing: geometry equality
    // cannot revive the old semantic extent-cap support.
    authoring_provider.reset(
        SupportMode::
            missing_with_same_geometry_decoy);
    verifyFailedSupport(
        fixture.session.document(),
        authoring_provider,
        fixture.face_support,
        part::SketchSupportResolutionStatus::
            missing,
        part::SketchSupportResolutionDiagnostic::
            missing_surface,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_missing);

    // Repair is the already-accepted re-support command. It preserves IDs and
    // local U/V geometry, and Undo/Redo restore the exact semantic intent.
    const auto origin_support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::
                xy_plane);
    CHECK(origin_support.has_value());
    const auto local_before_repair =
        fixture.session.document()
            .findSketch(
                fixture.face_sketch_id)
            ->model.state();

    const auto repair =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                fixture.face_sketch_id,
                *origin_support,
                fixture.session.document()
                    .revision()});
    CHECK(repair.ok());
    CHECK(repair.changed);
    CHECK(
        repair.status ==
        application::SketchSupportMutationStatus::
            applied);
    const auto* repaired =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(repaired != nullptr);
    CHECK(repaired->id ==
          fixture.face_sketch_id);
    CHECK(repaired->support ==
          *origin_support);
    CHECK(
        repaired->model.state() ==
        local_before_repair);

    authoring_provider.reset(
        SupportMode::
            missing_with_same_geometry_decoy);
    verifyBodyUpToDate(
        fixture.session.document(),
        authoring_provider);

    const auto undo_repair =
        fixture.session.undo();
    CHECK(
        undo_repair.ok() &&
        undo_repair.changed);
    const auto* undo_sketch =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(undo_sketch != nullptr);
    CHECK(
        undo_sketch->support ==
        fixture.face_support);
    CHECK(
        undo_sketch->model.state() ==
        local_before_repair);

    authoring_provider.reset(
        SupportMode::
            missing_with_same_geometry_decoy);
    verifyFailedSupport(
        fixture.session.document(),
        authoring_provider,
        fixture.face_support,
        part::SketchSupportResolutionStatus::
            missing,
        part::SketchSupportResolutionDiagnostic::
            missing_surface,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_missing);

    const auto redo_repair =
        fixture.session.redo();
    CHECK(
        redo_repair.ok() &&
        redo_repair.changed);
    const auto* redo_sketch =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(redo_sketch != nullptr);
    CHECK(
        redo_sketch->support ==
        *origin_support);
    CHECK(
        redo_sketch->model.state() ==
        local_before_repair);

    // Re-support back to the original semantic Surface when it is current.
    authoring_provider.reset();
    const auto restore_face_support =
        fixture.session.execute(
            application::SetPartSketchSupportCommand{
                fixture.face_sketch_id,
                fixture.face_support,
                fixture.session.document()
                    .revision()},
            &authoring_provider);
    CHECK(restore_face_support.ok());
    CHECK(restore_face_support.changed);
    const auto* face_again =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(face_again != nullptr);
    CHECK(
        face_again->support ==
        fixture.face_support);
    CHECK(
        face_again->model.state() ==
        local_before_repair);

    // Alias/collapse ambiguity is structurally Ambiguous, not a first-winner
    // or geometry fallback.
    authoring_provider.reset(
        SupportMode::alias_ambiguous);
    verifyFailedSupport(
        fixture.session.document(),
        authoring_provider,
        fixture.face_support,
        part::SketchSupportResolutionStatus::
            ambiguous,
        part::SketchSupportResolutionDiagnostic::
            ambiguous_surface,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_ambiguous);

    // Save the repaired face-backed state, then destroy this session/provider.
    CHECK(fixture.session.save().ok());
    CHECK(!fixture.session.needsSave());

    const auto final_state =
        fixture.session.document().state();
    const auto final_revision =
        fixture.session.document().revision();

    part::PartDocumentStore store;
    auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->documentId() ==
        fixture.document_id);
    CHECK(
        loaded.document->body().id ==
        fixture.body_id);
    CHECK(
        loaded.document->revision() ==
        final_revision);
    CHECK(
        loaded.document->state() ==
        final_state);

    const auto* loaded_sketch =
        loaded.document->findSketch(
            fixture.face_sketch_id);
    CHECK(loaded_sketch != nullptr);
    CHECK(
        loaded_sketch->support ==
        fixture.face_support);
    CHECK(
        loaded_sketch->model.state() ==
        fixture.local_state);
    CHECK(
        loaded.document->findProfile(
            fixture.face_profile_id) != nullptr);
    CHECK(
        loaded.document->findFeature(
            fixture.base_feature_id) != nullptr);
    CHECK(
        loaded.document->findFeature(
            fixture.add_feature_id) != nullptr);
    CHECK(
        loaded.document->findFeature(
            fixture.cut_feature_id) != nullptr);

    // True cold rebuild: a fresh provider generation uses deliberately
    // different runtime token numbers. Durable semantic meaning and IDs must
    // remain unchanged.
    LifecycleKernel cold_provider{9000U};
    cold_provider.reset();
    const auto cold =
        part::evaluatePart(
            *loaded.document,
            cold_provider);
    CHECK(
        cold.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(cold.features.size() == 3U);
    CHECK(cold_provider.inputs.size() == 3U);

    const auto* cold_surface =
        findSupportSurface(
            cold,
            fixture.surface_reference);
    CHECK(cold_surface != nullptr);
    CHECK(cold_surface->runtime_token.has_value());
    CHECK(
        fixture.initial_runtime_surface
            .has_value());
    CHECK(
        cold_surface->runtime_token !=
        fixture.initial_runtime_surface);

    CHECK(
        cold.features[0]
            .result_topology.has_value());
    const auto cold_support =
        part::resolveSketchSupport(
            fixture.face_support,
            &*cold.features[0]
                  .result_topology);
    CHECK(cold_support.valid());
    CHECK(
        cold_support.status ==
        part::SketchSupportResolutionStatus::
            resolved);
    CHECK(
        cold_support.diagnostic ==
        part::SketchSupportResolutionDiagnostic::
            none);
    CHECK(cold_support.frame.has_value());
    CHECK(
        cold_support.frame->origin[2] ==
        10.0);

    std::cout
        << "PM02I_FACE_SUPPORTED_LIFECYCLE_SURVIVAL_PASS"
        << " undo_redo=1"
        << " delete_rejection=1"
        << " cold_reopen=1"
        << " runtime_token_reuse_irrelevant=1"
        << " stale_selection_rejected=1"
        << " split_surface_resolved=1"
        << " delete_with_geometry_decoy_missing=1"
        << " alias_ambiguous=1"
        << " repair_resupport=1"
        << " false_resolved=0"
        << " id_corruption=0"
        << '\n';

    return EXIT_SUCCESS;
}
