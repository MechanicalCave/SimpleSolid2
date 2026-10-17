#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <limits>
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
    CHECK(rounded.complete());
    CHECK(topology_accurate.complete());
    // The arrangement deliberately exposes both selectable material
    // cells: the outer region with one circular hole, AND the circular
    // interior as its own region. Counting only one would incorrectly
    // discard a legitimate independent Profile candidate.
    CHECK(rounded.regions.size() == 2U);
    CHECK(topology_accurate.regions.size() == 2U);
    const auto verify_outer_with_hole = [](
        const sketch::RegionAnalysis2D& regions) {
        const auto found = std::find_if(
            regions.regions.begin(), regions.regions.end(),
            [](const auto& region) {
                return region.holes.size() == 1U &&
                    region.outer.boundary.size() == 5U;
            });
        CHECK(found != regions.regions.end());
        CHECK(part::makeProfileRegionIntent(*found));
    };
    verify_outer_with_hole(rounded);
    verify_outer_with_hole(topology_accurate);
    std::cout
        << "PG01D_DATUM_PROJECTED_ARC_REGION_EVIDENCE_PASS"
        << " outer_region_with_circle_hole=1"
        << " interior_circle_own_region=1"
        << " roundoff_not_root_cause=1"
        << " line_line_ulp_gap_still_rejected=1"
        << " global_tolerance_unchanged=1\\n";
}

// S0 baseline witness for B01, enabled before changing region algorithms.
// Assertions encode corrected geometry, not the historically observed defect.
// Keep emitted counts visible in genuine Windows RED job logs.
// Owner's original .ss2part, 2026-10-17: Circle 9 with both
// Lines 26/28 extended to their exactly shared apex (case 3), versus the
// visually similar copied short finite Lines 56/58 around Circle 59 (case 4).
// These stored decimal literals are the actual authored binary64 values.
// Case 3 has TWO distinct true finite intersections on each Line,
// separated by ~5.56e-7 / 7.74e-7 mm. N1 must not conflate them with
// one synthetic tangent, nor misclassify the left pair as disjoint.
void verifyOwnerExtendedNearSecantRegions() {
    using sketch::CurveRelationStatus;
    sketch::SketchModel extended;
    const auto circle3 = extended.addCircle(
        {89.16841975020736, 67.52296383563574}, 30.0);
    extended.addLine(
        {49.16841975020738, 0.0},
        {129.16841975020736, 0.0});
    extended.addLine(
        {49.16841975020738, 0.0},
        {49.16841975020738, 60.507216219488555});
    const auto left3 = extended.addLine(
        {49.16841975020738, 60.507216219488555},
        {89.16841975020735, 123.46646180728104});
    extended.addLine(
        {129.16841975020736, 0.0},
        {129.16841975020736, 60.507216219488555});
    const auto right3 = extended.addLine(
        {129.16841975020736, 60.507216219488555},
        {89.16841975020735, 123.46646180728104});
    const auto left_relation3 =
        sketch::analyzeCurveRelation(extended, circle3, left3);
    const auto right_relation3 =
        sketch::analyzeCurveRelation(extended, circle3, right3);
    const auto regions3 = sketch::analyzeRegions(extended);

    sketch::SketchModel copied;
    const auto circle4 = copied.addCircle(
        {181.1670624652628, 67.52296383563574}, 30.0);
    copied.addLine(
        {141.1670624652628, 0.0},
        {221.1670624652628, 0.0});
    copied.addLine(
        {141.1670624652628, 0.0},
        {141.1670624652628, 60.507216219488555});
    const auto left4 = copied.addLine(
        {141.1670624652628, 60.507216219488555},
        {155.8453883282019, 83.61062429004924});
    copied.addLine(
        {221.1670624652628, 0.0},
        {221.1670624652628, 60.507216219488555});
    const auto right4 = copied.addLine(
        {221.1670624652628, 60.507216219488555},
        {206.4887366023237, 83.61062429004924});
    const auto left_relation4 =
        sketch::analyzeCurveRelation(copied, circle4, left4);
    const auto right_relation4 =
        sketch::analyzeCurveRelation(copied, circle4, right4);
    const auto regions4 = sketch::analyzeRegions(copied);

    std::cerr << "SS2_N1_OWNER_EXTENDED_SECANTS"
              << " case3_left=" << left_relation3.intersections.size()
              << " case3_left_status=" << static_cast<int>(left_relation3.status)
              << " case3_right=" << right_relation3.intersections.size()
              << " case3_right_status=" << static_cast<int>(right_relation3.status)
              << " case3_regions=" << regions3.regions.size()
              << " case3_diagnostics=" << regions3.diagnostics.size()
              << " case4_left=" << left_relation4.intersections.size()
              << " case4_right=" << right_relation4.intersections.size()
              << " case4_regions=" << regions4.regions.size()
              << " case4_diagnostics=" << regions4.diagnostics.size()
              << '\n';

    CHECK(left_relation3.status == CurveRelationStatus::discrete);
    CHECK(right_relation3.status == CurveRelationStatus::discrete);
    CHECK(left_relation3.intersections.size() == 2U);
    CHECK(right_relation3.intersections.size() == 2U);
    CHECK(!regions3.regions.empty());
    // Publishing other cells is NOT a waiver of the unresolved lens:
    // it must retain an explicit source-grounded diagnostic.
    CHECK(!regions3.complete());
    CHECK(std::any_of(
        regions3.diagnostics.begin(),
        regions3.diagnostics.end(),
        [circle3, left3, right3](const auto& diagnostic) {
            const auto has = [&diagnostic](sketch::EntityId id) {
                return std::find(
                    diagnostic.entities.begin(),
                    diagnostic.entities.end(), id) !=
                    diagnostic.entities.end();
            };
            return diagnostic.kind ==
                sketch::RegionAnalysisDiagnosticKind::invalid_topology &&
                has(circle3) && (has(left3) || has(right3));
        }));
    CHECK(left_relation3.intersections[0].first_parameter !=
          left_relation3.intersections[1].first_parameter);
    CHECK(right_relation3.intersections[0].first_parameter !=
          right_relation3.intersections[1].first_parameter);
    CHECK(!regions4.regions.empty());
    CHECK(regions4.complete());
}

// Native Owner .ss2part schema 20, exact complete 40 authored entities,
// including Regular/Construction distinction and the copied #3/#4 cases.
// Test-only reconstruction; no parsing or persistent-product source change.
// D2-A negative: a geometrically narrow but numerically resolvable
// Circle/Line crescent is a REAL material Region. Certification must
// never discard it merely because its area is small or it has two uses.
void verifyD2ARepresentableNarrowCap() {
    sketch::SketchModel model;
    model.addCircle({0.0, 0.0}, 30.0);
    model.addLine({-45.0, 30.0 - 1e-5},
                  {45.0, 30.0 - 1e-5});
    const auto regions = sketch::analyzeRegions(model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 2U);
    const auto cap = sketch::pickRegion(
        model, regions, {0.0, 30.0 - 2e-6});
    CHECK(cap.region_index.has_value());
    CHECK(cap.location == sketch::RegionPointLocation::inside);
    const auto disk = sketch::pickRegion(
        model, regions, {0.0, 0.0});
    CHECK(disk.region_index.has_value());
    CHECK(cap.region_index != disk.region_index);
}

void verifyOwnerFullNativeScenePick() {
    struct Line {
        sketch::Point2 start;
        sketch::Point2 end;
        bool construction;
    };
    struct Circle {
        sketch::Point2 center;
        double radius;
    };
    const std::array<Line, 32> lines{{
        {{-30.0, 0.0}, {30.0, 0.0}, false}, // 1
        {{-30.0, 0.0}, {-30.0, 67.52296383563574}, false}, // 3
        {{30.0, 0.0}, {30.0, 67.52296383563574}, false}, // 4
        {{59.16841975020736, 0.0}, {59.16841975020736, 67.52296383563574}, false}, // 7
        {{119.16841975020736, 0.0}, {119.16841975020736, 67.52296383563574}, true}, // 8
        {{89.16841975020736, 0.0}, {89.16841975020736, -14.407052512069733}, true}, // 10
        {{49.16841975020738, 0.0}, {129.16841975020736, 0.0}, false}, // 11
        {{-111.13570411713846, 0.0}, {-51.13570411713846, 0.0}, false}, // 14
        {{-111.13570411713846, 0.0}, {-111.13570411713846, 92.56448649641345}, false}, // 15
        {{-51.13570411713846, 0.0}, {-51.13570411713846, 106.53001893976436}, false}, // 16
        {{6.846405134007824, -83.25915596546274}, {23.83920538253282, -66.26635571693774}, false}, // 22
        {{68.92311233700123, -78.74091461840942}, {56.130014366619065, -65.94781664802726}, false}, // 23
        {{49.16841975020738, 0.0}, {49.16841975020738, 60.507216219488555}, false}, // 25
        {{49.16841975020738, 60.507216219488555}, {89.16841975020735, 123.46646180728104}, false}, // 26
        {{129.16841975020736, 0.0}, {129.16841975020736, 60.507216219488555}, false}, // 27
        {{129.16841975020736, 60.507216219488555}, {89.16841975020735, 123.46646180728104}, false}, // 28
        {{349.5249764379939, 60.507216219488555}, {309.5249764379939, 123.46646180728108}, false}, // 32
        {{269.5249764379939, 60.507216219488555}, {309.5249764379939, 123.46646180728108}, false}, // 33
        {{266.5211586963419, -26.45386716719539}, {291.24935468828653, 12.97385113268978}, false}, // 39
        {{366.93648026946914, -38.82828053277269}, {349.2998050920396, 7.297268993808445}, false}, // 40
        {{-78.65740689293105, -83.25915596546274}, {-70.86501309242436, -75.46676216495605}, false}, // 41
        {{-16.580699689937646, -78.74091461840942}, {-24.208286064424026, -71.11332824392304}, false}, // 42
        {{-61.114932567368186, 140.6338796053901}, {72.52443017728294, 140.6338796053901}, false}, // 46
        {{-61.114932567368186, 181.295796953154}, {-61.114932567368186, 140.6338796053901}, false}, // 47
        {{151.1670624652628, 0.0}, {151.1670624652628, 67.52296383563574}, false}, // 51
        {{211.1670624652628, 0.0}, {211.1670624652628, 67.52296383563574}, true}, // 52
        {{181.1670624652628, 0.0}, {181.1670624652628, -14.407052512069733}, true}, // 53
        {{141.1670624652628, 0.0}, {221.1670624652628, 0.0}, false}, // 54
        {{141.1670624652628, 0.0}, {141.1670624652628, 60.507216219488555}, false}, // 55
        {{141.1670624652628, 60.507216219488555}, {155.8453883282019, 83.61062429004924}, false}, // 56
        {{221.1670624652628, 0.0}, {221.1670624652628, 60.507216219488555}, false}, // 57
        {{221.1670624652628, 60.507216219488555}, {206.4887366023237, 83.61062429004924}, false}, // 58
    }};
    const std::array<Circle, 7> circles{{
        {{0.0, 67.52296383563574}, 30.0}, // 2
        {{89.16841975020736, 67.52296383563574}, 30.0}, // 9
        {{-81.13570411713846, 67.52296383563574}, 30.0}, // 17
        {{36.2264805817629, -54.63476751496982}, 30.0}, // 19
        {{318.85884145566877, -4.342210497557272}, 32.59033215450208}, // 38
        {{-49.277331445175975, -54.63476751496982}, 30.0}, // 43
        {{181.1670624652628, 67.52296383563574}, 30.0}, // 59
    }};
    sketch::SketchModel scene;
    for (const auto& line : lines) {
        scene.addLine(line.start, line.end,
            line.construction ? sketch::EntityRole::construction
                              : sketch::EntityRole::regular);
    }
    for (const auto& circle : circles) {
        scene.addCircle(circle.center, circle.radius);
    }
    scene.addArc(
        {309.5249764379939, 67.52296383563574},
        30.0, 2.5755983265454314, 4.2735813076785165); // 49
    const auto state = scene.state();
    CHECK(state.lines.size() == 32U);
    CHECK(state.circles.size() == 7U);
    CHECK(state.arcs.size() == 1U);
    const auto analysis = sketch::analyzeRegions(scene);
    const auto pick3 = sketch::pickRegion(
        scene, analysis, {89.16841975020736, 20.0});
    const auto pick4 = sketch::pickRegion(
        scene, analysis, {181.1670624652628, 20.0});
    std::cerr << "SS2_S1_N1_OWNER_FULL_40_SCENE"
              << " regions=" << analysis.regions.size()
              << " diagnostics=" << analysis.diagnostics.size()
              << " pick3=" << pick3.region_index.has_value()
              << " pick4=" << pick4.region_index.has_value()
              << '\n';
    // The new D2-A fix must isolate unsupported microfaces, not hide
    // independently usable macro regions in the same connected component.
    CHECK(pick4.region_index.has_value());
    CHECK(pick3.region_index.has_value());

    // A binary64 point independently certified with dyadic-rational
    // squared-circle and Line-side inequalities to be STRICTLY inside
    // the Circle9/Line26 microscopic lens. It must not pick a macroface
    // when that derived microface is unresolved for Profile consumers.
    const sketch::Point2 actual_lens_point{
        63.84674552114644, 83.61062414524297};
    const auto lens_pick = sketch::pickRegion(
        scene, analysis, actual_lens_point);
    CHECK(!lens_pick.region_index.has_value());
    CHECK(lens_pick.location ==
          sketch::RegionPointLocation::ambiguous);
    // Also test the mirrored, independently certified STRICT interior
    // binary64 point in the Circle9/Line28 right microcap.
    const auto right_lens_pick = sketch::pickRegion(
        scene, analysis,
        {114.49009370326829, 83.61062457966176});
    CHECK(!right_lens_pick.region_index.has_value());
    CHECK(right_lens_pick.location ==
          sketch::RegionPointLocation::ambiguous);

    const auto original_left = state.lines[13].id;  // Line 26
    const auto original_right = state.lines[15].id; // Line 28
    const auto circle_id = state.circles[1].id;      // Circle 9
    const auto has_micro_diagnostic =
        [&analysis, circle_id](sketch::EntityId line_id) {
            return std::any_of(
                analysis.diagnostics.begin(),
                analysis.diagnostics.end(),
                [line_id, circle_id](const auto& diagnostic) {
                    return diagnostic.kind ==
                               sketch::RegionAnalysisDiagnosticKind::
                                   invalid_topology &&
                        diagnostic.entities.size() == 2U &&
                        std::find(
                            diagnostic.entities.begin(),
                            diagnostic.entities.end(), circle_id) !=
                            diagnostic.entities.end() &&
                        std::find(
                            diagnostic.entities.begin(),
                            diagnostic.entities.end(), line_id) !=
                            diagnostic.entities.end();
                });
        };
    CHECK(has_micro_diagnostic(original_left));
    CHECK(has_micro_diagnostic(original_right));

    // Separate macro-cell publication is useful only if Part can
    // create and uniquely replay its durable source-anchored identity.
    // No speculative OCCT/fuzzy wire representation is used here.
    const auto check_durable =
        [&scene, &analysis](std::uint32_t index) {
            const auto region = std::find_if(
                analysis.regions.begin(),
                analysis.regions.end(),
                [index](const auto& cell) {
                    return cell.region_index == index;
                });
            CHECK(region != analysis.regions.end());
            const auto intent =
                part::makeProfileRegionIntent(*region);
            CHECK(intent.has_value());
            const auto replay =
                part::resolveProfileRegionIntent(scene, *intent);
            CHECK(replay.valid());
        };
    check_durable(*pick3.region_index);
    check_durable(*pick4.region_index);

    // Localized uncertain geometry must stay visible to callers. A
    // macro-profile cannot be obtained by silently erasing real faces.
    CHECK(!analysis.complete());
    const auto original_case3_circle = state.circles[1].id;
    CHECK(std::any_of(
        analysis.diagnostics.begin(),
        analysis.diagnostics.end(),
        [original_case3_circle](const auto& diagnostic) {
            return diagnostic.kind ==
                       sketch::RegionAnalysisDiagnosticKind::invalid_topology &&
                   std::find(
                       diagnostic.entities.begin(),
                       diagnostic.entities.end(),
                       original_case3_circle) !=
                       diagnostic.entities.end();
        }));
}

void verifyS0CircleTangentRegionCells() {
    const std::array<int, 4> initial{0, 1, 2, 3};
    constexpr double pi = std::numbers::pi_v<double>;
    constexpr double disk_area = 25.0 * pi;
    constexpr double pocket_area = 90.0 - 12.5 * pi;
    std::size_t endpoint_correct = 0U;
    std::size_t interior_correct = 0U;
    std::array<std::size_t, 5> endpoint_hist{};
    std::array<std::size_t, 5> interior_hist{};
    std::array<int, 4> order = initial;

    do {
        const auto evaluate = [&](bool extended) {
            sketch::SketchModel model;
            for (int kind : order) {
                if (kind == 0) {
                    [[maybe_unused]] const auto circle =
                        model.addCircle({0.0, 0.0}, 5.0);
                } else if (kind == 1) {
                    [[maybe_unused]] const auto left =
                        model.addLine({-5.0, -9.0},
                                      {-5.0, extended ? 7.0 : 0.0});
                } else if (kind == 2) {
                    [[maybe_unused]] const auto right =
                        model.addLine({5.0, -9.0},
                                      {5.0, extended ? 7.0 : 0.0});
                } else {
                    [[maybe_unused]] const auto base =
                        model.addLine({-5.0, -9.0}, {5.0, -9.0});
                }
            }
            const auto analysis = sketch::analyzeRegions(model);
            std::vector<std::pair<double, double>> geometry;
            for (const auto& region : analysis.regions) {
                geometry.emplace_back(region.area, region.perimeter);
            }
            std::sort(geometry.begin(), geometry.end());
            const bool correct =
                geometry.size() == 2U &&
                std::abs(geometry[0].first - pocket_area) < 1e-8 &&
                std::abs(geometry[0].second -
                         (28.0 + 5.0 * pi)) < 1e-8 &&
                std::abs(geometry[1].first - disk_area) < 1e-8 &&
                std::abs(geometry[1].second -
                         10.0 * pi) < 1e-8;
            return std::pair<std::size_t, bool>{
                geometry.size(), correct};
        };
        const auto [endpoint_count, endpoint_ok] = evaluate(false);
        const auto [interior_count, interior_ok] = evaluate(true);
        ++endpoint_hist[std::min(endpoint_count, std::size_t{4})];
        ++interior_hist[std::min(interior_count, std::size_t{4})];
        endpoint_correct += static_cast<std::size_t>(endpoint_ok);
        interior_correct += static_cast<std::size_t>(interior_ok);
    } while (std::next_permutation(order.begin(), order.end()));

    std::cerr
        << "SKETCH_S0_B01_ENDPOINT_24"
        << " correct=" << endpoint_correct
        << " region0=" << endpoint_hist[0]
        << " region1=" << endpoint_hist[1]
        << " region2=" << endpoint_hist[2] << '\n'
        << "SKETCH_S0_B01_INTERIOR_24"
        << " correct=" << interior_correct
        << " region0=" << interior_hist[0]
        << " region1=" << interior_hist[1]
        << " region2=" << interior_hist[2] << '\n';
    // Independent numerical witness for F3 before changing predicates.
    // R12 asserts the full Circle->Arc Trim lifecycle in its own CTest.
    sketch::SketchModel oblique;
    const auto circle = oblique.addCircle({0.0, 0.0}, 5.0);
    const auto left = oblique.addLine({-13.0, -9.0}, {-4.0, 3.0});
    const auto right = oblique.addLine({13.0, -9.0}, {4.0, 3.0});
    const auto left_contact = sketch::analyzeCurveRelation(
        oblique, circle, left);
    const auto right_contact = sketch::analyzeCurveRelation(
        oblique, circle, right);
    std::cerr
        << "SKETCH_S0_B02_F3_RELATION"
        << " left=" << left_contact.intersections.size()
        << " right=" << right_contact.intersections.size()
        << " left_status=" << static_cast<int>(left_contact.status)
        << " right_status=" << static_cast<int>(right_contact.status)
        << '\n';
    CHECK(endpoint_correct == 24U);
    CHECK(interior_correct == 24U);
    CHECK(left_contact.intersections.size() == 1U);
    CHECK(right_contact.intersections.size() == 1U);

    // F1/F2 are more than UI polygons. A current accepted Part Profile
    // RegionIntent must survive exact source-anchored replay. This test
    // intentionally exposes any pre-existing Part resolver restriction
    // after Shared-2D begins returning the missing tangent cells.
    const auto verify_durable = [](bool extended) {
        sketch::SketchModel model;
        [[maybe_unused]] const auto circle =
            model.addCircle({0.0, 0.0}, 5.0);
        [[maybe_unused]] const auto left =
            model.addLine({-5.0, -9.0},
                          {-5.0, extended ? 7.0 : 0.0});
        [[maybe_unused]] const auto right =
            model.addLine({5.0, -9.0},
                          {5.0, extended ? 7.0 : 0.0});
        [[maybe_unused]] const auto base =
            model.addLine({-5.0, -9.0}, {5.0, -9.0});
        const auto analysis = sketch::analyzeRegions(model);
        CHECK(analysis.regions.size() == 2U);
        std::size_t resolved = 0U;
        for (const auto& region : analysis.regions) {
            const auto intent = part::makeProfileRegionIntent(region);
            if (!intent) {
                continue;
            }
            const auto replay =
                part::resolveProfileRegionIntent(model, *intent);
            if (replay.valid()) {
                ++resolved;
            } else {
                std::cerr << "SKETCH_S0_PROFILE_REPLAY"
                          << " extended=" << extended
                          << " region_area=" << region.area
                          << " status=" << static_cast<int>(replay.status)
                          << '\n';
            }
        }
        std::cerr << "SKETCH_S0_PROFILE_REPLAY_SUMMARY"
                  << " extended=" << extended
                  << " resolved=" << resolved
                  << " expected=2\n";
        return resolved == 2U;
    };
    CHECK(verify_durable(false));
    CHECK(verify_durable(true));

    // S1 F2/Arc: start=-0.2, sweep=6.0 DOES NOT contain the entire
    // bottom semicircle (it ends at 5.8 < 2*pi), so it cannot enclose
    // the lower pocket. The top semicircle (0..pi) plus the side Lines
    // and bottom base instead bounds ONE upper/outer cell of area
    // 90 + 12.5*pi. CW parameter reversal and Line source reversal
    // must preserve that cell without inventing a second closed disk
    // or healing the open Arc's actual missing bottom-right segment.
    std::size_t verified_arc_variants = 0U;
    for (const bool extended : {false, true}) {
        for (const bool clockwise : {false, true}) {
            for (const bool reverse_lines : {false, true}) {
                sketch::SketchModel model;
                [[maybe_unused]] const auto arc = model.addArc(
                    {0.0, 0.0}, 5.0,
                    clockwise ? 5.8 : -0.2,
                    clockwise ? -6.0 : 6.0);
                const auto line = [&](sketch::Point2 first,
                                      sketch::Point2 second) {
                    if (reverse_lines) {
                        std::swap(first, second);
                    }
                    [[maybe_unused]] const auto id =
                        model.addLine(first, second);
                };
                line({-5.0, -9.0}, {-5.0, extended ? 7.0 : 0.0});
                line({5.0, -9.0}, {5.0, extended ? 7.0 : 0.0});
                line({-5.0, -9.0}, {5.0, -9.0});
                const auto regions = sketch::analyzeRegions(model);
                std::cerr << "SKETCH_S0_B01_ARC"
                          << " extended=" << extended
                          << " clockwise=" << clockwise
                          << " reversed=" << reverse_lines
                          << " count=" << regions.regions.size()
                          << " first_area="
                          << (regions.regions.empty()
                                  ? -1.0
                                  : regions.regions.front().area)
                          << '\n';
                CHECK(regions.regions.size() == 1U);
                CHECK(std::abs(
                    regions.regions.front().area -
                    (90.0 + 12.5 * pi)) < 1e-8);
                ++verified_arc_variants;
            }
        }
    }
    CHECK(verified_arc_variants == 8U);

    // Exact quarter-turn and positive factor-two similarity do not
    // create or destroy material cells or authored finite contacts.
    // The transform here uses exact integer-coordinate permutations
    // rather than constructing "exact" tangency from rounded trig.
    std::size_t verified_similarities = 0U;
    for (const bool extended : {false, true}) {
        for (const bool quarter_turn : {false, true}) {
            for (const double factor : {1.0, 2.0}) {
                sketch::SketchModel model;
                const auto mapped =
                    [factor, quarter_turn](sketch::Point2 point) {
                        return quarter_turn
                            ? sketch::Point2{
                                -point.v * factor,
                                point.u * factor}
                            : sketch::Point2{
                                point.u * factor,
                                point.v * factor};
                    };
                [[maybe_unused]] const auto circle =
                    model.addCircle({0.0, 0.0}, 5.0 * factor);
                [[maybe_unused]] const auto left =
                    model.addLine(
                        mapped({-5.0, -9.0}),
                        mapped({-5.0, extended ? 7.0 : 0.0}));
                [[maybe_unused]] const auto right =
                    model.addLine(
                        mapped({5.0, -9.0}),
                        mapped({5.0, extended ? 7.0 : 0.0}));
                [[maybe_unused]] const auto base =
                    model.addLine(
                        mapped({-5.0, -9.0}),
                        mapped({5.0, -9.0}));
                const auto regions = sketch::analyzeRegions(model);
                std::vector<std::pair<double, double>> metrics;
                for (const auto& region : regions.regions) {
                    metrics.emplace_back(region.area, region.perimeter);
                }
                std::sort(metrics.begin(), metrics.end());
                CHECK(metrics.size() == 2U);
                CHECK(std::abs(metrics[0].first -
                      pocket_area * factor * factor) < 1e-8);
                CHECK(std::abs(metrics[0].second -
                      (28.0 + 5.0 * pi) * factor) < 1e-8);
                CHECK(std::abs(metrics[1].first -
                      disk_area * factor * factor) < 1e-8);
                CHECK(std::abs(metrics[1].second -
                      10.0 * pi * factor) < 1e-8);
                ++verified_similarities;
            }
        }
    }
    CHECK(verified_similarities == 8U);

    // One ULP outside a Circle must produce the same semantic
    // bounded regions/Profile with contact at finite Line endpoints
    // OR inside those authored Lines. No tool-specific Trim exception.
    for (const bool contact_interior : {false, true}) {
        sketch::SketchModel model;
        const sketch::Point2 center{
            190.34517723877235, 67.52296383563574};
        const auto left_x = std::nextafter(
            center.u - 30.0, -std::numeric_limits<double>::infinity());
        const auto right_x = std::nextafter(
            center.u + 30.0, std::numeric_limits<double>::infinity());
        [[maybe_unused]] const auto circle =
            model.addCircle(center, 30.0);
        const double top =
            center.v + (contact_interior ? 20.0 : 0.0);
        [[maybe_unused]] const auto left = model.addLine(
            {left_x, center.v - 60.0}, {left_x, top});
        [[maybe_unused]] const auto right = model.addLine(
            {right_x, center.v - 60.0}, {right_x, top});
        [[maybe_unused]] const auto base = model.addLine(
            {left_x, center.v - 60.0},
            {right_x, center.v - 60.0});
        const auto cells = sketch::analyzeRegions(model);
        std::cerr << "SS2_N1_DERIVED_REGION"
                  << " interior=" << contact_interior
                  << " cells=" << cells.regions.size()
                  << " diagnostic=" << cells.diagnostics.size()
                  << '\n';
        CHECK(cells.regions.size() == 2U);
        std::size_t replayed = 0U;
        for (const auto& cell : cells.regions) {
            const auto intent = part::makeProfileRegionIntent(cell);
            CHECK(intent.has_value());
            if (part::resolveProfileRegionIntent(model, *intent).valid()) {
                ++replayed;
            }
        }
        std::cerr << "SS2_N1_DURABLE_PROFILE"
                  << " replay=" << replayed << "/2\n";
        CHECK(replayed == 2U);
    }
}

} // namespace

int main() {
    verifyD2ARepresentableNarrowCap();
    verifyOwnerFullNativeScenePick();
    verifyOwnerExtendedNearSecantRegions();
    verifyS0CircleTangentRegionCells();
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
