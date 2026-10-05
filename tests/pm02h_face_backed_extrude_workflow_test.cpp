#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/extrude_draft.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

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
            << "PM-02H face-backed Extrude CHECK failed at line "
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

class RuntimeSolid final
    : public kernel::RuntimeSolid {
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
    result.origin.x +=
        result.normal.x * offset;
    result.origin.y +=
        result.normal.y * offset;
    result.origin.z +=
        result.normal.z * offset;
    return result;
}

class WorkflowKernel final
    : public kernel::ISolidModelingKernel {
public:
    SupportMode support_mode{
        SupportMode::resolved};
    std::vector<kernel::LinearExtrudeInput>
        extrude_inputs;
    std::vector<kernel::LinearExtrudeInput>
        preview_inputs;

    void reset(
        SupportMode mode =
            SupportMode::resolved) {
        support_mode = mode;
        extrude_inputs.clear();
        preview_inputs.clear();
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
            extrude_inputs.size();
        extrude_inputs.push_back(input);

        auto runtime =
            std::make_shared<RuntimeSolid>();
        if (upstream != nullptr) {
            const auto* previous =
                dynamic_cast<const RuntimeSolid*>(
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
                result.current_faces.push_back(
                    inherited.face);
                result.inherited_faces.push_back(
                    {
                        inherited.face,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                    });
                result.inherited_surfaces.push_back(
                    {
                        inherited.surface,
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
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
                    RuntimeSolid::Surface{
                        face,
                        surface,
                        kernel::SurfaceKind::plane,
                        frame});
                result.current_faces.push_back(face);

                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        cap,
                    cap_role,
                    std::nullopt};
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
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
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
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        cap,
                    cap_role,
                    std::nullopt};
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

        const auto publish_ambiguous =
            [&result, &runtime](
                kernel::ExtrudeCapRole cap_role) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                result.current_faces.push_back(face);

                const kernel::ExtrudeFaceRole role{
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        cap,
                    cap_role,
                    std::nullopt};
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            ambiguous,
                        1U,
                        std::nullopt,
                    });
                result.new_surfaces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            ambiguous,
                        kernel::ReferenceStatus::
                            ambiguous,
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

        if (call_index == 0U &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    extent_cap) {
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

    kernel::SolidPresentationResult
    extrudePreviewMesh(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        if (!input.valid() ||
            (input.operation ==
                 kernel::SolidBooleanOperation::cut &&
             upstream == nullptr)) {
            return {
                kernel::SolidPresentationStatus::
                    invalid_input,
                {}};
        }

        preview_inputs.push_back(input);
        kernel::SolidPresentationMesh mesh;
        const double z =
            input.profile.frame.origin.z;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, z},
                {1.0, 0.0, z},
                {0.0, 1.0, z},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0}});
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
    }
};

part::ProfileId createRectProfile(
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

    const auto* source =
        session.document().findSketch(sketch_id);
    CHECK(source != nullptr);
    const auto regions =
        sketch::analyzeRegions(source->model);
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

struct Fixture final {
    application::DocumentSession session;
    part::FeatureId base_feature_id;
    part::ProfileId base_profile_id;
    sketch::SketchId face_sketch_id;
    part::ProfileId face_profile_id;
};

Fixture makeFixture(
    WorkflowKernel& kernel) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(base_sketch.ok());
    CHECK(base_sketch.sketch_id.has_value());

    const auto base_profile_id =
        createRectProfile(
            session,
            *base_sketch.sketch_id,
            {0.0, 0.0},
            {40.0, 30.0});

    kernel.reset();
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
            kernel);
    CHECK(base.ok());
    CHECK(base.feature_id.has_value());

    const part::SurfaceReference support_reference{
        part::BodyStageRef{
            part::BodyStageKind::after_feature,
            *base.feature_id},
        part::FeatureSurfaceAddress{
            *base.feature_id,
            part::FeatureSurfaceRoleKind::
                extent_cap,
            std::nullopt,
            0U,
            0U,
            false}};
    CHECK(support_reference.valid());

    const auto support =
        part::partSketchSupportForBodyPlanarSurface(
            support_reference);
    CHECK(support.has_value());

    kernel.reset();
    const auto face_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *support,
                session.document().revision()},
            &kernel);
    CHECK(face_sketch.ok());
    CHECK(face_sketch.sketch_id.has_value());

    const auto face_profile_id =
        createRectProfile(
            session,
            *face_sketch.sketch_id,
            {2.0, 3.0},
            {8.0, 9.0});

    return {
        std::move(session),
        *base.feature_id,
        base_profile_id,
        *face_sketch.sketch_id,
        face_profile_id};
}

application::ExtrudeDraftEvaluationResult
evaluateCreateDraft(
    application::DocumentSession& session,
    WorkflowKernel& kernel,
    part::ProfileId profile_id,
    part::ExtrudeOperation operation,
    double distance,
    std::optional<application::ExtrudeDraft>& draft) {
    draft =
        application::ExtrudeDraft::beginCreate(
            session,
            profile_id);
    CHECK(draft.has_value());
    CHECK(
        draft->setDistance(
            core::LengthValue{distance}));
    CHECK(
        draft->setOperation(operation));

    return session.evaluateExtrudeDraft(
        *draft,
        kernel);
}

} // namespace

int main() {
    WorkflowKernel kernel;
    auto fixture = makeFixture(kernel);

    // Lost and ambiguous support fail at the first face-backed consumer.
    // No consumer Kernel call and no preview delta may reuse stale truth.
    std::optional<application::ExtrudeDraft>
        rejected_draft;

    kernel.reset(SupportMode::missing);
    const auto missing =
        evaluateCreateDraft(
            fixture.session,
            kernel,
            fixture.face_profile_id,
            part::ExtrudeOperation::add,
            4.0,
            rejected_draft);
    CHECK(!missing.committable());
    CHECK(
        missing.status ==
        application::ExtrudeDraftEvaluationStatus::
            target_failed);
    CHECK(
        missing.evaluation_diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_missing);
    CHECK(kernel.extrude_inputs.size() == 1U);
    CHECK(kernel.preview_inputs.empty());

    kernel.reset(SupportMode::ambiguous);
    const auto ambiguous =
        evaluateCreateDraft(
            fixture.session,
            kernel,
            fixture.face_profile_id,
            part::ExtrudeOperation::add,
            4.0,
            rejected_draft);
    CHECK(!ambiguous.committable());
    CHECK(
        ambiguous.status ==
        application::ExtrudeDraftEvaluationStatus::
            target_failed);
    CHECK(
        ambiguous.evaluation_diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_ambiguous);
    CHECK(kernel.extrude_inputs.size() == 1U);
    CHECK(kernel.preview_inputs.empty());

    // Add from the face-backed Profile uses the current support stage both
    // for candidate evaluation and for the separate exact delta preview.
    std::optional<application::ExtrudeDraft>
        add_draft;
    kernel.reset();
    const auto add_preview =
        evaluateCreateDraft(
            fixture.session,
            kernel,
            fixture.face_profile_id,
            part::ExtrudeOperation::add,
            4.0,
            add_draft);
    CHECK(add_preview.committable());
    CHECK(add_preview.previewSolidAvailable());
    CHECK(!kernel.preview_inputs.empty());
    CHECK(
        kernel.preview_inputs.back()
            .operation ==
        kernel::SolidBooleanOperation::add);
    CHECK(
        kernel.preview_inputs.back()
            .profile.frame.origin.z == 10.0);

    const auto undo_before_add =
        fixture.session.undoDepth();
    kernel.reset();
    const auto add_finish =
        application::finishExtrudeDraft(
            fixture.session,
            *add_draft,
            add_preview,
            kernel);
    CHECK(add_finish.ok());
    CHECK(add_finish.changed);
    CHECK(add_finish.feature_id.has_value());
    CHECK(
        fixture.session.undoDepth() ==
        undo_before_add + 1U);

    // The same face-backed Profile follows the existing PM-01 Cut path.
    std::optional<application::ExtrudeDraft>
        cut_draft;
    kernel.reset();
    const auto cut_preview =
        evaluateCreateDraft(
            fixture.session,
            kernel,
            fixture.face_profile_id,
            part::ExtrudeOperation::cut,
            2.0,
            cut_draft);
    CHECK(cut_preview.committable());
    CHECK(cut_preview.previewSolidAvailable());
    CHECK(!kernel.preview_inputs.empty());
    CHECK(
        kernel.preview_inputs.back()
            .operation ==
        kernel::SolidBooleanOperation::cut);
    CHECK(
        kernel.preview_inputs.back()
            .profile.frame.origin.z == 10.0);

    const auto undo_before_cut =
        fixture.session.undoDepth();
    kernel.reset();
    const auto cut_finish =
        application::finishExtrudeDraft(
            fixture.session,
            *cut_draft,
            cut_preview,
            kernel);
    CHECK(cut_finish.ok());
    CHECK(cut_finish.changed);
    CHECK(cut_finish.feature_id.has_value());
    CHECK(
        fixture.session.undoDepth() ==
        undo_before_cut + 1U);
    CHECK(
        fixture.session.document()
            .body().features.size() == 3U);

    // Editing the upstream carrier re-resolves the same semantic support.
    // Authored local face Sketch/Profile intent remains unchanged, while both
    // downstream Add and Cut receive the moved world frame.
    const auto* face_sketch_before =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(face_sketch_before != nullptr);
    const auto local_before =
        face_sketch_before->model.state();
    const auto support_before =
        face_sketch_before->support;

    kernel.reset();
    const auto edited_base =
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
            kernel);
    CHECK(edited_base.ok());
    CHECK(edited_base.changed);

    const auto* face_sketch_after =
        fixture.session.document().findSketch(
            fixture.face_sketch_id);
    CHECK(face_sketch_after != nullptr);
    CHECK(
        face_sketch_after->model.state() ==
        local_before);
    CHECK(
        face_sketch_after->support ==
        support_before);

    kernel.reset();
    const auto moved =
        part::evaluatePart(
            fixture.session.document(),
            kernel);
    CHECK(
        moved.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(moved.features.size() == 3U);
    CHECK(kernel.extrude_inputs.size() == 3U);
    CHECK(
        kernel.extrude_inputs[1]
            .profile.frame.origin.z == 20.0);
    CHECK(
        kernel.extrude_inputs[2]
            .profile.frame.origin.z == 20.0);
    CHECK(
        kernel.extrude_inputs[1]
            .operation ==
        kernel::SolidBooleanOperation::add);
    CHECK(
        kernel.extrude_inputs[2]
            .operation ==
        kernel::SolidBooleanOperation::cut);

    // Once the support is lost/ambiguous, the established downstream chain
    // fails closed. Only the upstream Base reaches the Kernel; no last-good
    // frame from the successful 20 mm evaluation is consumed.
    kernel.reset(SupportMode::missing);
    const auto lost =
        part::evaluatePart(
            fixture.session.document(),
            kernel);
    CHECK(
        lost.body_status ==
        part::BodyEvaluationStatus::unavailable);
    CHECK(lost.features.size() == 3U);
    CHECK(
        lost.features[1].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_missing);
    CHECK(
        lost.features[2].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            upstream_unavailable);
    CHECK(kernel.extrude_inputs.size() == 1U);

    kernel.reset(SupportMode::ambiguous);
    const auto ambiguous_chain =
        part::evaluatePart(
            fixture.session.document(),
            kernel);
    CHECK(
        ambiguous_chain.body_status ==
        part::BodyEvaluationStatus::unavailable);
    CHECK(
        ambiguous_chain.features[1].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_ambiguous);
    CHECK(
        ambiguous_chain.features[2].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            upstream_unavailable);
    CHECK(kernel.extrude_inputs.size() == 1U);

    std::cout
        << "PM02H_FACE_BACKED_EXTRUDE_WORKFLOW_PASS"
        << " add=1"
        << " cut=1"
        << " preview_stage_aware=1"
        << " moved_support=1"
        << " stale_downstream=0"
        << '\n';
    return EXIT_SUCCESS;
}
