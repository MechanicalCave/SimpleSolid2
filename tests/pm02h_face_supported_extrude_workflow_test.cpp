#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/extrude_draft.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02H face-supported Extrude CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

enum class SupportMode {
    resolved,
    missing,
    ambiguous,
};

class WorkflowSolid final : public kernel::RuntimeSolid {
public:
    struct Surface final {
        kernel::RuntimeFaceToken face;
        kernel::RuntimeSurfaceToken surface;
        kernel::SurfaceKind kind{
            kernel::SurfaceKind::plane};
        std::optional<kernel::Frame3> frame;
    };

    std::vector<Surface> surfaces;
    std::uint64_t next_face{1U};
    std::uint64_t next_surface{1U};
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

class WorkflowKernel final
    : public kernel::ISolidModelingKernel {
public:
    SupportMode support_mode{SupportMode::resolved};
    std::vector<kernel::LinearExtrudeInput> inputs;
    std::vector<kernel::LinearExtrudeInput> preview_inputs;

    void reset(
        SupportMode mode = SupportMode::resolved) {
        support_mode = mode;
        inputs.clear();
        preview_inputs.clear();
    }

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::missing_upstream;
            return result;
        }

        const std::size_t call_index = inputs.size();
        inputs.push_back(input);

        auto runtime =
            std::make_shared<WorkflowSolid>();
        if (upstream != nullptr) {
            const auto* previous =
                dynamic_cast<const WorkflowSolid*>(
                    upstream.get());
            if (previous == nullptr) {
                result.status =
                    kernel::SolidModelingStatus::provider_mismatch;
                return result;
            }

            runtime->surfaces = previous->surfaces;
            runtime->next_face = previous->next_face;
            runtime->next_surface = previous->next_surface;

            for (const auto& inherited : previous->surfaces) {
                result.current_faces.push_back(
                    inherited.face);
                result.inherited_faces.push_back(
                    {
                        inherited.face,
                        kernel::ReferenceStatus::resolved,
                        1U,
                    });
                result.inherited_surfaces.push_back(
                    {
                        inherited.surface,
                        kernel::ReferenceStatus::resolved,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        inherited.kind,
                        inherited.frame,
                        {inherited.face},
                    });
            }
        }

        const auto publish_resolved =
            [&result, &runtime, &input](
                kernel::ExtrudeCapRole cap_role,
                double offset) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};
                const auto frame =
                    offsetFrame(
                        input.profile.frame,
                        offset);
                runtime->surfaces.push_back(
                    WorkflowSolid::Surface{
                        face,
                        surface,
                        kernel::SurfaceKind::plane,
                        frame});
                result.current_faces.push_back(face);

                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::cap,
                    cap_role,
                    std::nullopt};
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        face,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::resolved,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        kernel::SurfaceKind::plane,
                        frame,
                        surface,
                        {face},
                    });
            };

        const auto publish_missing =
            [&result](
                kernel::ExtrudeCapRole cap_role) {
                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::cap,
                    cap_role,
                    std::nullopt};
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::missing,
                        0U,
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::missing,
                        kernel::ReferenceStatus::missing,
                        0U,
                        kernel::SurfaceKind::plane,
                        std::nullopt,
                        std::nullopt,
                        {},
                    });
            };

        const auto publish_ambiguous =
            [&result, &runtime](
                kernel::ExtrudeCapRole cap_role) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                result.current_faces.push_back(face);
                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::cap,
                    cap_role,
                    std::nullopt};
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::ambiguous,
                        1U,
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::ambiguous,
                        kernel::ReferenceStatus::ambiguous,
                        1U,
                        kernel::SurfaceKind::plane,
                        std::nullopt,
                        std::nullopt,
                        {face},
                    });
            };

        publish_resolved(
            input.start_cap_role,
            input.start_offset_mm);

        // The first operation in each clean evaluation is the upstream Base
        // Feature whose extent cap carries the face-backed Sketch.
        if (call_index == 0U &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::extent_cap) {
            switch (support_mode) {
            case SupportMode::resolved:
                publish_resolved(
                    input.end_cap_role,
                    input.end_offset_mm);
                break;
            case SupportMode::missing:
                publish_missing(
                    input.end_cap_role);
                break;
            case SupportMode::ambiguous:
                publish_ambiguous(
                    input.end_cap_role);
                break;
            }
        } else {
            publish_resolved(
                input.end_cap_role,
                input.end_offset_mm);
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

    kernel::SolidPresentationResult extrudePreviewMesh(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        if (!input.valid() ||
            (input.operation ==
                 kernel::SolidBooleanOperation::cut &&
             upstream == nullptr)) {
            return {
                kernel::SolidPresentationStatus::invalid_input,
                {}};
        }

        preview_inputs.push_back(input);
        kernel::SolidPresentationMesh mesh;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, 0.0},
                {1.0, 0.0, 0.0},
                {0.0, 1.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
            });
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
    }
};

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId face_sketch_id;
    part::ProfileId base_profile_id;
    part::ProfileId face_profile_id;
    part::FeatureId base_feature_id;
    part::PartSketchSupport face_support;
    sketch::SketchModelState local_state;
};

Fixture makeFixture(WorkflowKernel& provider) {
    application::DocumentSession session{
        {},
        part::PartDocument::create(
            core::DocumentId::generate())};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(base_sketch.ok() && base_sketch.sketch_id);

    const auto base_rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *base_sketch.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(base_rectangle.ok());

    const auto* base =
        session.document().findSketch(
            *base_sketch.sketch_id);
    CHECK(base != nullptr);
    const auto base_regions =
        sketch::analyzeRegions(base->model);
    CHECK(
        base_regions.complete() &&
        base_regions.regions.size() == 1U);
    const auto base_intent =
        part::makeProfileRegionIntent(
            base_regions.regions.front());
    CHECK(base_intent);

    const auto base_profile =
        session.execute(
            application::CreateProfileCommand{
                *base_sketch.sketch_id,
                session.document().revision(),
                *base_intent});
    CHECK(base_profile.ok() && base_profile.profile_id);

    const auto base_feature =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                *base_profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            provider);
    CHECK(
        base_feature.ok() &&
        base_feature.changed &&
        base_feature.feature_id);

    provider.reset();
    const auto base_evaluation =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        base_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(base_evaluation.current_topology);

    const auto found =
        std::find_if(
            base_evaluation.current_topology
                ->surfaces.begin(),
            base_evaluation.current_topology
                ->surfaces.end(),
            [feature_id =
                 *base_feature.feature_id](
                const auto& surface) {
                return surface.status ==
                           kernel::ReferenceStatus::resolved &&
                       surface.address
                               .producer_feature_id ==
                           feature_id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::
                               extent_cap;
            });
    CHECK(
        found !=
        base_evaluation.current_topology
            ->surfaces.end());

    const auto support =
        part::partSketchSupportForBodyPlanarSurface(
            part::SurfaceReference{
                base_evaluation.current_topology->stage,
                found->address});
    CHECK(support);

    const auto face_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *support,
                session.document().revision()},
            &provider);
    CHECK(
        face_sketch.ok() &&
        face_sketch.changed &&
        face_sketch.sketch_id);

    const auto face_rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *face_sketch.sketch_id,
                session.document().revision(),
                {2.0, 3.0},
                {8.0, 9.0},
                sketch::EntityRole::regular,
                false});
    CHECK(face_rectangle.ok());

    const auto* hosted =
        session.document().findSketch(
            *face_sketch.sketch_id);
    CHECK(hosted != nullptr);
    const auto local_state = hosted->model.state();
    const auto face_regions =
        sketch::analyzeRegions(hosted->model);
    CHECK(
        face_regions.complete() &&
        face_regions.regions.size() == 1U);
    const auto face_intent =
        part::makeProfileRegionIntent(
            face_regions.regions.front());
    CHECK(face_intent);

    const auto face_profile =
        session.execute(
            application::CreateProfileCommand{
                *face_sketch.sketch_id,
                session.document().revision(),
                *face_intent});
    CHECK(face_profile.ok() && face_profile.profile_id);

    return {
        std::move(session),
        *face_sketch.sketch_id,
        *base_profile.profile_id,
        *face_profile.profile_id,
        *base_feature.feature_id,
        *support,
        local_state};
}

application::ExtrudeDraftEvaluationResult
evaluateCreateDraft(
    application::DocumentSession& session,
    WorkflowKernel& provider,
    part::ProfileId profile_id,
    part::ExtrudeOperation operation,
    double distance) {
    auto draft =
        application::ExtrudeDraft::beginCreate(
            session,
            profile_id);
    CHECK(draft);
    CHECK(
        draft->setOperation(operation));
    CHECK(
        draft->setDistance(
            core::LengthValue{distance}));

    provider.reset();
    auto evaluation =
        session.evaluateExtrudeDraft(
            *draft,
            provider);
    CHECK(evaluation.committable());
    CHECK(evaluation.previewSolidAvailable());
    CHECK(evaluation.body_solid != nullptr);
    CHECK(provider.preview_inputs.size() == 1U);
    CHECK(
        provider.preview_inputs.front()
            .operation ==
        (operation == part::ExtrudeOperation::cut
             ? kernel::SolidBooleanOperation::cut
             : kernel::SolidBooleanOperation::add));

    const auto finished =
        application::finishExtrudeDraft(
            session,
            *draft,
            evaluation,
            provider);
    CHECK(finished.ok());
    CHECK(finished.changed);
    CHECK(finished.feature_id);

    return evaluation;
}

void verifySupportFailure(
    const Fixture& fixture,
    WorkflowKernel& provider,
    SupportMode mode,
    part::FeatureEvaluationDiagnosticCode expected) {
    provider.reset(mode);
    const auto evaluation =
        part::evaluatePart(
            fixture.session.document(),
            provider);

    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::unavailable);
    CHECK(evaluation.features.size() == 3U);
    CHECK(
        evaluation.features[0].status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        evaluation.features[1].status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(
        evaluation.features[1].diagnostic ==
        expected);

    // Neither the Add nor Cut consumer reaches the Kernel after current
    // support loss/ambiguity. A previously valid world frame is not reused.
    CHECK(provider.inputs.size() == 1U);
    CHECK(provider.preview_inputs.empty());
}

} // namespace

int main() {
    WorkflowKernel provider;
    auto fixture = makeFixture(provider);

    const auto* authored_face_sketch =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(authored_face_sketch != nullptr);
    CHECK(
        authored_face_sketch->support ==
        fixture.face_support);
    CHECK(
        authored_face_sketch->model.state() ==
        fixture.local_state);

    const auto add_evaluation =
        evaluateCreateDraft(
            fixture.session,
            provider,
            fixture.face_profile_id,
            part::ExtrudeOperation::add,
            4.0);
    CHECK(
        provider.preview_inputs.front()
            .profile.frame.origin.z == 10.0);
    CHECK(
        add_evaluation.evaluation_diagnostic ==
        std::optional<
            part::FeatureEvaluationDiagnosticCode>{
            part::FeatureEvaluationDiagnosticCode::
                none});

    const auto cut_evaluation =
        evaluateCreateDraft(
            fixture.session,
            provider,
            fixture.face_profile_id,
            part::ExtrudeOperation::cut,
            2.0);
    CHECK(
        provider.preview_inputs.front()
            .profile.frame.origin.z == 10.0);
    CHECK(
        cut_evaluation.evaluation_diagnostic ==
        std::optional<
            part::FeatureEvaluationDiagnosticCode>{
            part::FeatureEvaluationDiagnosticCode::
                none});

    CHECK(
        fixture.session.document()
            .body().features.size() == 3U);

    provider.reset();
    const auto upstream_edit =
        fixture.session.execute(
            application::EditExtrudeFeatureCommand{
                fixture.base_feature_id,
                fixture.session.document().revision(),
                fixture.base_profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false},
                "Base"},
            provider);
    CHECK(upstream_edit.ok());
    CHECK(upstream_edit.changed);

    const auto* moved_face_sketch =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(moved_face_sketch != nullptr);
    CHECK(
        moved_face_sketch->support ==
        fixture.face_support);
    CHECK(
        moved_face_sketch->model.state() ==
        fixture.local_state);

    provider.reset();
    const auto moved =
        part::evaluatePart(
            fixture.session.document(),
            provider);
    CHECK(
        moved.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(moved.features.size() == 3U);
    CHECK(provider.inputs.size() == 3U);
    CHECK(
        provider.inputs[1].profile.frame.origin.z ==
        20.0);
    CHECK(
        provider.inputs[2].profile.frame.origin.z ==
        20.0);
    CHECK(
        provider.inputs[1].operation ==
        kernel::SolidBooleanOperation::add);
    CHECK(
        provider.inputs[2].operation ==
        kernel::SolidBooleanOperation::cut);

    verifySupportFailure(
        fixture,
        provider,
        SupportMode::missing,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_missing);
    verifySupportFailure(
        fixture,
        provider,
        SupportMode::ambiguous,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_ambiguous);

    std::cout
        << "PM02H_FACE_SUPPORTED_EXTRUDE_WORKFLOW_PASS"
        << " add=1"
        << " cut=1"
        << " preview_stage_aware=1"
        << " upstream_move=1"
        << " local_uv_mutation=0"
        << " missing_fail_closed=1"
        << " ambiguous_fail_closed=1"
        << " stale_downstream_kernel_calls=0"
        << '\n';

    return EXIT_SUCCESS;
}
