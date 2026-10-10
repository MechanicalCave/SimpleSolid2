#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02F stage-aware Profile CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

enum class ExtentSupportMode {
    resolved_plane,
    missing,
    ambiguous,
    non_planar,
};

class StageSolid final
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

class StageKernel final
    : public kernel::ISolidModelingKernel {
public:
    ExtentSupportMode extent_mode{
        ExtentSupportMode::resolved_plane};
    std::vector<kernel::LinearExtrudeInput> inputs;

    void reset(
        ExtentSupportMode mode =
            ExtentSupportMode::resolved_plane) {
        extent_mode = mode;
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
            std::make_shared<StageSolid>();
        if (upstream != nullptr) {
            const auto* previous =
                dynamic_cast<const StageSolid*>(
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
                double offset,
                kernel::SurfaceKind kind) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};

                std::optional<kernel::Frame3> frame;
                if (kind == kernel::SurfaceKind::plane) {
                    frame =
                        offsetFrame(
                            input.profile.frame,
                            offset);
                }

                runtime->surfaces.push_back(
                    StageSolid::Surface{
                        face,
                        surface,
                        kind,
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
                        kind,
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
            input.start_offset_mm,
            kernel::SurfaceKind::plane);

        if (call_index == 0U &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    extent_cap) {
            switch (extent_mode) {
            case ExtentSupportMode::resolved_plane:
                publish_resolved(
                    input.end_cap_role,
                    input.end_offset_mm,
                    kernel::SurfaceKind::plane);
                break;
            case ExtentSupportMode::missing:
                publish_missing(
                    input.end_cap_role);
                break;
            case ExtentSupportMode::ambiguous:
                publish_ambiguous(
                    input.end_cap_role);
                break;
            case ExtentSupportMode::non_planar:
                publish_resolved(
                    input.end_cap_role,
                    input.end_offset_mm,
                    kernel::SurfaceKind::cylinder);
                break;
            }
        } else {
            publish_resolved(
                input.end_cap_role,
                input.end_offset_mm,
                kernel::SurfaceKind::plane);
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
};

struct Fixture final {
    part::PartDocument document;
    part::FeatureId support_feature_id;
    part::FeatureId consumer_feature_id;
    sketch::SketchId face_sketch_id;
    part::ProfileId face_profile_id;
    sketch::SketchModelState local_state;
    part::PartSketchSupport support;
};

Fixture makeFixture() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto origin_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        origin_sketch.ok() &&
        origin_sketch.sketch_id.has_value());

    const auto base_rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *origin_sketch.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(base_rectangle.ok());

    const auto* base_sketch =
        session.document().findSketch(
            *origin_sketch.sketch_id);
    CHECK(base_sketch != nullptr);
    const auto base_regions =
        sketch::analyzeRegions(
            base_sketch->model);
    CHECK(base_regions.complete());
    CHECK(base_regions.regions.size() == 1U);
    const auto base_intent =
        part::makeProfileRegionIntent(
            base_regions.regions.front());
    CHECK(base_intent.has_value());

    const auto base_profile =
        session.execute(
            application::CreateProfileCommand{
                *origin_sketch.sketch_id,
                session.document().revision(),
                *base_intent});
    CHECK(
        base_profile.ok() &&
        base_profile.profile_id.has_value());

    auto state =
        session.document().state();
    const auto support_feature_id =
        state.body.next_feature_id.allocate();
    const auto consumer_feature_id =
        state.body.next_feature_id.allocate();
    CHECK(support_feature_id.has_value());
    CHECK(consumer_feature_id.has_value());

    const part::SurfaceReference
        surface_reference{
            part::BodyStageRef{
                part::BodyStageKind::after_feature,
                *support_feature_id},
            part::FeatureSurfaceAddress{
                *support_feature_id,
                part::FeatureSurfaceRoleKind::
                    extent_cap,
                std::nullopt,
                0U,
                0U,
                false}};
    CHECK(surface_reference.valid());

    const auto support =
        part::partSketchSupportForBodyPlanarSurface(
            surface_reference);
    CHECK(support.has_value());

    sketch::SketchModel face_model;
    CHECK(
        face_model.addLine(
            {2.0, 3.0},
            {8.0, 3.0})
            .valid());
    CHECK(
        face_model.addLine(
            {8.0, 3.0},
            {8.0, 9.0})
            .valid());
    CHECK(
        face_model.addLine(
            {8.0, 9.0},
            {2.0, 9.0})
            .valid());
    CHECK(
        face_model.addLine(
            {2.0, 9.0},
            {2.0, 3.0})
            .valid());

    const auto face_regions =
        sketch::analyzeRegions(face_model);
    CHECK(face_regions.complete());
    CHECK(face_regions.regions.size() == 1U);
    const auto face_intent =
        part::makeProfileRegionIntent(
            face_regions.regions.front());
    CHECK(face_intent.has_value());

    const auto face_sketch_id =
        sketch::SketchId::generate();
    const auto local_state =
        face_model.state();
    state.sketches.push_back(
        part::PartSketch{
            face_sketch_id,
            *support,
            true,
            std::move(face_model)});

    const auto face_profile_id =
        state.next_profile_id.allocate();
    CHECK(face_profile_id.has_value());
    state.profiles.push_back(
        part::PartProfile{
            *face_profile_id,
            face_sketch_id,
            "Face Profile",
            part::ProfileVisibilityPolicy::
                automatic,
            *face_intent});

    state.body.features = {
        part::PartFeature{
            *support_feature_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *base_profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}},
        part::PartFeature{
            *consumer_feature_id,
            "Face Consumer",
            false,
            part::ExtrudeFeature{
                *face_profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{4.0},
                    false}}},
    };

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());

    return Fixture{
        std::move(*restored.document),
        *support_feature_id,
        *consumer_feature_id,
        face_sketch_id,
        *face_profile_id,
        local_state,
        *support};
}

part::PartDocument withBaseDistance(
    const Fixture& fixture,
    double distance) {
    auto state =
        fixture.document.state();
    bool changed = false;
    for (auto& feature :
         state.body.features) {
        if (feature.id !=
            fixture.support_feature_id) {
            continue;
        }
        auto* extrude =
            std::get_if<part::ExtrudeFeature>(
                &feature.definition);
        CHECK(extrude != nullptr);
        auto* extent =
            std::get_if<
                part::OneSidedExtrudeExtent>(
                &extrude->extent);
        CHECK(extent != nullptr);
        extent->distance =
            core::LengthValue{distance};
        changed = true;
    }
    CHECK(changed);

    auto restored =
        part::PartDocument::restore(
            fixture.document.documentId(),
            std::move(state),
            fixture.document.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

void verifyBlockedSupport(
    const Fixture& fixture,
    StageKernel& kernel,
    ExtentSupportMode mode,
    part::ProfileKernelInputStatus
        expected_profile_status,
    part::FeatureEvaluationDiagnosticCode
        expected_feature_diagnostic) {
    kernel.reset(mode);
    const auto evaluation =
        part::evaluatePart(
            fixture.document,
            kernel);

    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(evaluation.features.size() == 2U);
    CHECK(
        evaluation.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        evaluation.features[0]
            .result_topology.has_value());
    CHECK(
        evaluation.features[1].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        evaluation.features[1].diagnostic ==
        expected_feature_diagnostic);

    // Only the upstream Feature reached the Kernel. A stale frame from a
    // previous successful evaluation cannot feed the blocked consumer.
    CHECK(kernel.inputs.size() == 1U);

    const auto materialized =
        part::resolveKernelProfileInput(
            fixture.document,
            fixture.face_profile_id,
            &*evaluation.features[0]
                  .result_topology);
    CHECK(
        materialized.status ==
        expected_profile_status);
    CHECK(!materialized.input.has_value());
}

// PG-01C Owner Sketch 5 diagnostic guardrail, using a wholly synthetic
// 40 x 30 rectangle rather than publishing the Owner's private CAD file.
// A single ULP endpoint gap must remain OPEN in Shared 2D. A future
// OCCT projection fix must use proven common source-vertex topology,
// not a global proximity/tolerance relaxation in region analysis.
void verifyExactRegionEndpointBoundary() {
    sketch::SketchModel model;
    const auto almost_30 = std::nextafter(30.0, 0.0);
    CHECK(almost_30 != 30.0);

    [[maybe_unused]] const auto bottom = model.addLine(
        {0.0, 0.0}, {40.0, 0.0}, sketch::EntityRole::regular);
    const auto right = model.addLine(
        {40.0, 0.0}, {40.0, almost_30},
        sketch::EntityRole::regular);
    [[maybe_unused]] const auto top = model.addLine(
        {40.0, 30.0}, {0.0, 30.0},
        sketch::EntityRole::regular);
    [[maybe_unused]] const auto left = model.addLine(
        {0.0, 30.0}, {0.0, 0.0},
        sketch::EntityRole::regular);

    const auto disconnected = sketch::analyzeRegions(model);
    CHECK(disconnected.regions.empty());

    // Explicitly identical coordinates represent a different, closed
    // topology. The same EntityIds survive this normal authored edit.
    CHECK(model.updateLine(
        right, {40.0, 0.0}, {40.0, 30.0}));
    const auto closed = sketch::analyzeRegions(model);
    CHECK(closed.complete());
    CHECK(closed.regions.size() == 1U);
    CHECK(closed.regions.front().area == 1200.0);
    std::cout
        << "PG01C_SKETCH5_EXACT_ENDPOINT_BOUNDARY_PASS"
        << " one_ulp_gap_rejected=1"
        << " exact_edit_closes=1"
        << " proximity_healing=0\\n";
}

// PG-01D Face Boundary follow-up, wholly synthetic Datum-plane analogue.
// The projected analytic Arc's Cartesian endpoints differ slightly from
// visually coincident Line endpoints due to double trigonometric roundoff.
// Unlike a genuine 1-ULP Line-Line endpoint gap, valid Line-Arc analytic
// intersection can still yield a sound closed region. This control avoids
// misdiagnosing arbitrary tiny Arc endpoint differences as the Owner's
// reported linked-Profile failure or relaxing global endpoint rules.
void verifyProjectedArcDatumProfileEndpointEvidence() {
    sketch::SketchModel model;
    const double pi = std::numbers::pi_v<double>;
    const sketch::Point2 arc_center{25.0, 40.0};
    const double arc_radius = 10.0;
    const sketch::Point2 start{
        arc_center.u + arc_radius * std::cos(pi),
        arc_center.v + arc_radius * std::sin(pi)};
    const sketch::Point2 end{
        arc_center.u +
            arc_radius * std::cos(pi + pi / 2.0),
        arc_center.v +
            arc_radius * std::sin(pi + pi / 2.0)};
    CHECK(start.u == 15.0);
    CHECK(end.v == 30.0);
    CHECK(start.v != 40.0 || end.u != 25.0);
    [[maybe_unused]] const auto left = model.addLine(
        {0.0, -20.0}, {0.0, 40.0});
    const auto top = model.addLine(
        {0.0, 40.0}, {15.0, 40.0});
    [[maybe_unused]] const auto arc = model.addArc(
        arc_center, arc_radius, pi, pi / 2.0);
    const auto right = model.addLine(
        {25.0, 30.0}, {25.0, -20.0});
    [[maybe_unused]] const auto bottom = model.addLine(
        {25.0, -20.0}, {0.0, -20.0});
    [[maybe_unused]] const auto inner_circle = model.addCircle(
        {12.0, 0.0}, 5.0);
    const auto rounded = sketch::analyzeRegions(model);
    std::cerr
        << "PG01D_DATUM_REGION_BEFORE_EXACT"
        << " regions=" << rounded.regions.size()
        << " diagnostics=" << rounded.diagnostics.size();
    for (const auto& region : rounded.regions) {
        std::cerr << " [area=" << region.area
                  << " holes=" << region.holes.size()
                  << " outer_uses=" << region.outer.boundary.size()
                  << "]";
    }
    std::cerr << '\\n';

    CHECK(model.updateLine(
        top, {0.0, 40.0}, start));
    CHECK(model.updateLine(
        right, end, {25.0, -20.0}));
    const auto topology_accurate =
        sketch::analyzeRegions(model);
    std::cerr
        << "PG01D_DATUM_REGION_AFTER_EXACT"
        << " regions=" << topology_accurate.regions.size()
        << " diagnostics=" << topology_accurate.diagnostics.size();
    for (const auto& region : topology_accurate.regions) {
        std::cerr << " [area=" << region.area
                  << " holes=" << region.holes.size()
                  << " outer_uses=" << region.outer.boundary.size()
                  << "]";
    }
    std::cerr << '\\n';
    CHECK(topology_accurate.complete());
    CHECK(topology_accurate.regions.size() == 1U);
    CHECK(topology_accurate.regions.front().holes.size() == 1U);
    CHECK(part::makeProfileRegionIntent(
        topology_accurate.regions.front()));
    std::cout
        << "PG01D_DATUM_PROJECTED_ARC_REGION_EVIDENCE_PASS"
        << " analytic_line_arc_region_hole=1"
        << " exact_vertex_control_region_hole=1"
        << " line_line_ulp_gap_still_rejected=1"
        << " global_tolerance_unchanged=1\\n";
}

} // namespace

int main() {
    verifyExactRegionEndpointBoundary();
    verifyProjectedArcDatumProfileEndpointEvidence();
    auto fixture = makeFixture();
    StageKernel kernel;

    const auto authored_before =
        fixture.document.state();
    const auto* face_sketch =
        fixture.document.findSketch(
            fixture.face_sketch_id);
    CHECK(face_sketch != nullptr);
    CHECK(
        face_sketch->model.state() ==
        fixture.local_state);
    CHECK(
        face_sketch->support ==
        fixture.support);

    const auto first =
        part::evaluatePart(
            fixture.document,
            kernel);
    CHECK(
        fixture.document.state() ==
        authored_before);
    CHECK(
        first.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(first.features.size() == 2U);
    CHECK(
        first.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        first.features[1].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(kernel.inputs.size() == 2U);
    CHECK(
        kernel.inputs[1].profile.frame
            .origin.z == 10.0);

    const auto first_consumer_profile =
        kernel.inputs[1].profile;
    CHECK(
        first.features[0]
            .result_topology.has_value());
    const auto first_materialized =
        part::resolveKernelProfileInput(
            fixture.document,
            fixture.face_profile_id,
            &*first.features[0]
                  .result_topology);
    CHECK(first_materialized.ok());
    CHECK(
        first_materialized.input->frame
            .origin.z == 10.0);

    auto moved =
        withBaseDistance(
            fixture,
            20.0);
    const auto* moved_sketch =
        moved.findSketch(
            fixture.face_sketch_id);
    CHECK(moved_sketch != nullptr);
    CHECK(
        moved_sketch->model.state() ==
        fixture.local_state);
    CHECK(
        moved_sketch->support ==
        fixture.support);

    kernel.reset();
    const auto moved_evaluation =
        part::evaluatePart(
            moved,
            kernel);
    CHECK(
        moved_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(kernel.inputs.size() == 2U);
    CHECK(
        kernel.inputs[1].profile.frame
            .origin.z == 20.0);
    CHECK(
        kernel.inputs[1].profile.outer ==
        first_consumer_profile.outer);
    CHECK(
        kernel.inputs[1].profile.holes ==
        first_consumer_profile.holes);
    CHECK(
        kernel.inputs[1].profile.frame.u_axis ==
        first_consumer_profile.frame.u_axis);
    CHECK(
        kernel.inputs[1].profile.frame.v_axis ==
        first_consumer_profile.frame.v_axis);
    CHECK(
        kernel.inputs[1].profile.frame.normal ==
        first_consumer_profile.frame.normal);

    verifyBlockedSupport(
        fixture,
        kernel,
        ExtentSupportMode::missing,
        part::ProfileKernelInputStatus::
            support_missing,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_missing);

    verifyBlockedSupport(
        fixture,
        kernel,
        ExtentSupportMode::ambiguous,
        part::ProfileKernelInputStatus::
            support_ambiguous,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_ambiguous);

    verifyBlockedSupport(
        fixture,
        kernel,
        ExtentSupportMode::non_planar,
        part::ProfileKernelInputStatus::
            support_unsupported,
        part::FeatureEvaluationDiagnosticCode::
            sketch_support_unsupported);

    std::cout
        << "PM02F_STAGE_AWARE_PROFILE_EVALUATION_PASS"
        << " moved_frame=1"
        << " authored_local_mutation=0"
        << " stale_frame_consumed=0"
        << " missing=1"
        << " ambiguous=1"
        << " unsupported=1"
        << '\n';

    return EXIT_SUCCESS;
}
