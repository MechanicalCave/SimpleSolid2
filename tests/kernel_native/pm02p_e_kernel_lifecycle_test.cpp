#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <memory>
#include <variant>
#include <numbers>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.E kernel lifecycle CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

bool near(double first, double second, double tolerance = 1.0e-9) {
    const double scale =
        std::max({
            1.0,
            std::abs(first),
            std::abs(second)});
    return std::abs(first - second) <=
           tolerance * scale;
}

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    const std::string& prefix = "cold") {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        lineUse(
            {0.0, 0.0},
            {40.0, 0.0},
            prefix + "-bottom",
            0U),
        lineUse(
            {40.0, 0.0},
            {40.0, 20.0},
            prefix + "-right",
            1U),
        lineUse(
            {40.0, 20.0},
            {0.0, 20.0},
            prefix + "-top",
            2U),
        lineUse(
            {0.0, 20.0},
            {0.0, 0.0},
            prefix + "-left",
            3U),
    };
    CHECK(input.valid());
    return input;
}

kernel::PlanarProfileInput circle(
    const std::string& source = "cold-circle") {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        kernel::BoundaryUse2D{
            kernel::Circle2{
                {0.0, 0.0},
                10.0},
            0.0,
            0.0,
            true,
            false,
            true,
            kernel::BoundaryUseProvenance{
                source,
                0U,
                0U,
                false},
        },
    };
    CHECK(input.valid());
    return input;
}

void verifyStageAccounting() {
    const auto evidence =
        kernel_occt::buildMultiStageTopologyAccountingEvidence();

    CHECK(evidence.complete());
    CHECK(evidence.stages.size() == 4U);

    const kernel::EvidenceStageOperation expected[] = {
        kernel::EvidenceStageOperation::add,
        kernel::EvidenceStageOperation::add,
        kernel::EvidenceStageOperation::cut,
        kernel::EvidenceStageOperation::cut,
    };

    for (std::size_t index = 0U;
         index < evidence.stages.size();
         ++index) {
        const auto& stage =
            evidence.stages[index];
        CHECK(stage.operation == expected[index]);
        CHECK(stage.completeClassification());
        CHECK(stage.shape.ok());
        CHECK(stage.shape.solid_count == 1U);
        CHECK(stage.topology.complete());

        CHECK(
            stage.plane_face_count ==
            stage.topology.faces.provider_unique_count);
        CHECK(stage.cylinder_face_count == 0U);
        CHECK(stage.other_face_count == 0U);

        CHECK(
            stage.line_edge_count ==
            stage.topology.edges.provider_unique_count);
        CHECK(stage.circle_edge_count == 0U);
        CHECK(stage.other_edge_count == 0U);

        CHECK(
            stage.finite_vertex_count ==
            stage.topology.vertices.provider_unique_count);

        CHECK(
            stage.topology.faces.catalog.size() ==
            stage.topology.faces.provider_unique_count);
        CHECK(
            stage.topology.edges.catalog.size() ==
            stage.topology.edges.provider_unique_count);
        CHECK(
            stage.topology.vertices.catalog.size() ==
            stage.topology.vertices.provider_unique_count);
    }
}

void verifyGeometrySimilarityTrap() {
    const auto decoy =
        kernel_occt::buildGeometrySimilarityDecoyEvidence();

    // Same provider Line geometry and length, different semantic provenance.
    CHECK(
        decoy.old_edge_in_old_shape ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        decoy.old_edge_in_replacement_shape ==
        kernel::ReferenceStatus::missing);
    CHECK(
        decoy.replacement_edge_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(decoy.edge_geometry_matches);
    CHECK(
        decoy.old_edge_curve_kind ==
        kernel::EvidenceCurveKind::line);
    CHECK(
        decoy.replacement_edge_curve_kind ==
        kernel::EvidenceCurveKind::line);
    CHECK(decoy.old_edge_length > 0.0);
    CHECK(
        near(
            decoy.old_edge_length,
            decoy.replacement_edge_length));

    // Same cylindrical class and radius, different semantic provenance.
    CHECK(
        decoy.old_cylinder_in_old_shape ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        decoy.old_cylinder_in_replacement_shape ==
        kernel::ReferenceStatus::missing);
    CHECK(
        decoy.replacement_cylinder_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(decoy.cylinder_geometry_matches);
    CHECK(
        decoy.old_cylinder_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(
        decoy.replacement_cylinder_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(near(decoy.old_cylinder_radius, 10.0));
    CHECK(
        near(
            decoy.old_cylinder_radius,
            decoy.replacement_cylinder_radius));

    // B2 covers same plane / area / shape class and canonical frame.
    const auto surface =
        kernel_occt::buildSurfaceDeleteRecreateEvidence();
    CHECK(surface.ok());
    CHECK(surface.replacement_geometry_matches_old);
    CHECK(
        surface.old_surface_after_recreate ==
        kernel::ReferenceStatus::missing);
    CHECK(
        surface.replacement_surface_status ==
        kernel::ReferenceStatus::resolved);

    // D covers exact same XYZ with unrelated semantic Point provenance.
    const auto point =
        kernel_occt::buildVertexSamePointReplacementEvidence();
    CHECK(point.ok());
    CHECK(point.provider_points_equal);
    CHECK(
        point.old_key_in_replacement_shape ==
        kernel::ReferenceStatus::missing);
    CHECK(
        point.replacement_key_status ==
        kernel::ReferenceStatus::resolved);
}

void verifyProspectiveDynamicSketchSupport() {
    const auto evidence =
        kernel_occt::buildProspectiveSketchSupportEvidence();

    CHECK(evidence.ok());
    CHECK(
        evidence.support_key.role ==
        kernel::EvidenceSurfaceCarrierRoleKind::end_cap);
    CHECK(
        evidence.before_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.after_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.before_frame.has_value());
    CHECK(evidence.after_frame.has_value());

    CHECK(near(evidence.before_frame->origin.z, 10.0));
    CHECK(near(evidence.after_frame->origin.z, 20.0));
    CHECK(
        near(
            evidence.before_frame->u_axis.x,
            evidence.after_frame->u_axis.x));
    CHECK(
        near(
            evidence.before_frame->v_axis.y,
            evidence.after_frame->v_axis.y));

    CHECK(evidence.authored_intent_unchanged);
    CHECK(evidence.world_geometry_moved);
    CHECK(
        evidence.before_world_geometry.size() ==
        evidence.local_geometry.size());
    CHECK(
        evidence.after_world_geometry.size() ==
        evidence.local_geometry.size());

    for (std::size_t index = 0U;
         index < evidence.local_geometry.size();
         ++index) {
        const auto& before =
            evidence.before_world_geometry[index];
        const auto& after =
            evidence.after_world_geometry[index];
        CHECK(near(before.x, after.x));
        CHECK(near(before.y, after.y));
        CHECK(near(after.z - before.z, 10.0));
    }

    CHECK(
        evidence.missing_status ==
        kernel::ReferenceStatus::missing);
    CHECK(evidence.missing_has_no_current_frame);
    CHECK(
        evidence.ambiguous_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(evidence.ambiguous_has_no_current_frame);
}

struct ColdEvidenceSet final {
    kernel::MultiStageTopologyAccountingEvidence stages;
    kernel::ExtrudeSurfaceCarrierEvidence stable_surface;
    kernel::SurfaceBooleanLineageEvidence split_surface;
    kernel::SurfaceDeleteRecreateEvidence deleted_surface;
    kernel::ExtrudeEdgeOntologyEvidence seam_edges;
    kernel::SurfacePairBranchEvidence ambiguous_branch;
    kernel::VertexSamePointReplacementEvidence point_decoy;
    kernel::GeometrySimilarityDecoyEvidence geometry_decoys;
    kernel::ProspectiveSketchSupportEvidence dynamic_support;

    friend bool operator==(
        const ColdEvidenceSet&,
        const ColdEvidenceSet&) = default;
};

ColdEvidenceSet evaluateColdSet() {
    return {
        kernel_occt::buildMultiStageTopologyAccountingEvidence(),
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangle(),
            10.0),
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::cut_split),
        kernel_occt::buildSurfaceDeleteRecreateEvidence(),
        kernel_occt::buildExtrudeEdgeOntologyEvidence(
            circle(),
            10.0),
        kernel_occt::buildSurfacePairBranchEvidence(),
        kernel_occt::buildVertexSamePointReplacementEvidence(),
        kernel_occt::buildGeometrySimilarityDecoyEvidence(),
        kernel_occt::buildProspectiveSketchSupportEvidence(),
    };
}

void verifyColdRebuild() {
    const auto first = evaluateColdSet();
    const auto cold = evaluateColdSet();

    CHECK(cold == first);

    CHECK(first.stages.complete());
    CHECK(first.stable_surface.completeFaceClaims());
    CHECK(
        first.split_surface.surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        first.split_surface.strict_face_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        first.deleted_surface.old_surface_after_recreate ==
        kernel::ReferenceStatus::missing);
    CHECK(
        first.seam_edges.representation_artifact_count >= 1U);
    CHECK(
        first.ambiguous_branch.pair_only_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(first.point_decoy.provider_points_equal);
    CHECK(first.geometry_decoys.edge_geometry_matches);
    CHECK(first.geometry_decoys.cylinder_geometry_matches);
    CHECK(first.dynamic_support.ok());
}


kernel::PlanarProfileInput circularSegmentProfile(
    double start_angle,
    double sweep_angle) {
    const double end_angle =
        start_angle + sweep_angle;
    const kernel::Point2 from{
        10.0 * std::cos(start_angle),
        10.0 * std::sin(start_angle)};
    const kernel::Point2 to{
        10.0 * std::cos(end_angle),
        10.0 * std::sin(end_angle)};
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        kernel::BoundaryUse2D{
            kernel::Arc2{
                {0.0, 0.0}, 10.0,
                start_angle, sweep_angle},
            0.0, 1.0, true, false, false,
            kernel::BoundaryUseProvenance{
                "pg01a-arc", 0U, 0U, false}},
        lineUse(
            to, from, "pg01a-chord", 1U)};
    CHECK(input.valid());
    return input;
}

void verifyPg01aArcProjection() {
    using Status = kernel::EdgeProjectionStatus;
    kernel_occt::OcctSolidModelingKernel query;
    const kernel::Frame3 xy{};
    const kernel::Frame3 yz{
        {0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
        {1.0, 0.0, 0.0}};
    const double pi = std::numbers::pi;
    const struct {
        double start;
        double sweep;
    } cases[] = {
        {0.0, pi},
        {pi, -pi},
        {5.0 * pi / 3.0, pi / 2.0}
    };
    std::size_t verified_arcs = 0U;
    for (const auto& arc : cases) {
        const auto body = query.extrude(
            kernel::LinearExtrudeInput{
                circularSegmentProfile(
                    arc.start, arc.sweep),
                0.0, 10.0,
                kernel::ExtrudeCapRole::profile_cap,
                kernel::ExtrudeCapRole::extent_cap,
                kernel::SolidBooleanOperation::add});
        CHECK(body.ok());
        std::size_t arcs_in_body = 0U;
        for (const auto edge : body.current_edges) {
            const auto projection =
                projectCurrentEdge(query,
                    body.solid, edge, xy);
            if (!projection.ok() ||
                !std::holds_alternative<kernel::Arc2>(
                    *projection.curve)) {
                continue;
            }
            const auto& result =
                std::get<kernel::Arc2>(
                    *projection.curve);
            CHECK(near(result.radius, 10.0));
            CHECK(near(result.center.u, 0.0));
            CHECK(near(result.center.v, 0.0));
            CHECK(near(
                std::abs(result.sweep_angle),
                std::abs(arc.sweep)));
            const auto angled =
                projectCurrentEdge(query,
                    body.solid, edge, yz);
            CHECK(angled.status ==
                  Status::unsupported_curve);
            CHECK(!angled.curve);
            const auto repeated =
                projectCurrentEdge(query,
                    body.solid, edge, xy);
            CHECK(repeated.ok());
            CHECK(repeated.curve == projection.curve);
            ++arcs_in_body;
        }
        CHECK(arcs_in_body >= 2U);
        verified_arcs += arcs_in_body;
    }
    std::cout
        << "PG01A_EXACT_ARC_PROJECTION_PASS"
        << " native_arcs=" << verified_arcs
        << " tested_orientations=3\n";
}

kernel::EdgeProjectionResult projectCurrentEdge(
    kernel_occt::OcctSolidModelingKernel& query,
    kernel::RuntimeSolidHandle body,
    kernel::RuntimeEdgeToken current_edge,
    const kernel::Frame3& frame) {
    using Status = kernel::EdgeProjectionStatus;
    if (!body || !current_edge.valid()) {
        return {Status::invalid_input, std::nullopt};
    }
    const auto bound =
        query.bindEdgeToBody(body, current_edge);
    if (!bound) {
        return {Status::edge_unavailable, std::nullopt};
    }
    return query.projectEdgeToPlane(
        body, *bound, frame);
}

// The two bodies deliberately restart the same numeric token inventory.
// Once bound to the OLD runtime solid, the token cannot be redirected to
// NEW runtime solid despite an equal integer token value.
void verifyPg01aGenerationScope() {
    using Status = kernel::EdgeProjectionStatus;
    kernel_occt::OcctSolidModelingKernel query;
    const auto old_body = query.extrude(
        kernel::LinearExtrudeInput{
            rectangle("pg01a-old"), 0.0, 10.0,
            kernel::ExtrudeCapRole::profile_cap,
            kernel::ExtrudeCapRole::extent_cap,
            kernel::SolidBooleanOperation::add});
    const auto new_body = query.extrude(
        kernel::LinearExtrudeInput{
            circle("pg01a-new"), 0.0, 10.0,
            kernel::ExtrudeCapRole::profile_cap,
            kernel::ExtrudeCapRole::extent_cap,
            kernel::SolidBooleanOperation::add});
    CHECK(old_body.ok() && new_body.ok());
    CHECK(old_body.current_edges.front().value ==
          new_body.current_edges.front().value);
    const auto old_scoped = query.bindEdgeToBody(
        old_body.solid, old_body.current_edges.front());
    CHECK(old_scoped && old_scoped->valid());
    const kernel::Frame3 xy{};
    CHECK(query.projectEdgeToPlane(
              new_body.solid, *old_scoped, xy).status ==
          Status::provider_mismatch);
    CHECK(!query.bindEdgeToBody(
              old_body.solid, kernel::RuntimeEdgeToken{}));
    CHECK(!query.bindEdgeToBody(
              new_body.solid,
              kernel::RuntimeEdgeToken{999999999U}));
    struct ForeignSolid final : kernel::RuntimeSolid {};
    CHECK(!query.bindEdgeToBody(
              std::make_shared<ForeignSolid>(),
              old_body.current_edges.front()));
    const auto now = query.bindEdgeToBody(
        new_body.solid, new_body.current_edges.front());
    CHECK(now && now->valid());
    CHECK(query.projectEdgeToPlane(
              new_body.solid, *now, xy).status !=
          Status::provider_mismatch);
    std::cout << "PG01A_SCOPED_GENERATION_PASS"
              << " colliding_token_retarget_rejected=1\n";
}

void verifyPg01aOnCurrentPartOperations() {
    using Status = kernel::EdgeProjectionStatus;
    kernel_occt::OcctSolidModelingKernel query;
    const auto box = query.extrude(
        kernel::LinearExtrudeInput{
            rectangle("pg01a-part"), 0.0, 10.0,
            kernel::ExtrudeCapRole::profile_cap,
            kernel::ExtrudeCapRole::extent_cap,
            kernel::SolidBooleanOperation::add});
    CHECK(box.ok());
    kernel::AngularRevolveInput revolve;
    revolve.profile = rectangle("pg01a-revolve");
    revolve.axis = {
        {-20.0, 0.0, 0.0},
        {0.0, 1.0, 0.0}};
    revolve.start_angle_radians = 0.0;
    revolve.end_angle_radians =
        std::numbers::pi / 2.0;
    revolve.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(revolve.valid());
    const auto revolved = query.revolve(revolve);
    CHECK(revolved.ok());
    const auto filleted = query.edgeFeature(
        kernel::EdgeFeatureInput{
            kernel::EdgeFeatureOperation::fillet,
            {box.current_edges.front()}, 1.0},
        box.solid);
    CHECK(filleted.ok());
    const auto chamfered = query.edgeFeature(
        kernel::EdgeFeatureInput{
            kernel::EdgeFeatureOperation::chamfer,
            {box.current_edges.front()}, 1.0},
        box.solid);
    CHECK(chamfered.ok());
    const kernel::Frame3 xy{};
    const kernel::Frame3 xz{
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0},
        {0.0, -1.0, 0.0}};
    std::size_t recognized = 0U;
    std::size_t unsupported = 0U;
    for (const auto& result :
         {box, revolved, filleted, chamfered}) {
        CHECK(result.ok());
        CHECK(!result.current_edges.empty());
        std::size_t supported_stage = 0U;
        for (const auto token : result.current_edges) {
            for (const auto& frame : {xy, xz}) {
                const auto projected =
                    projectCurrentEdge(query,
                        result.solid, token, frame);
                if (projected.ok()) {
                    CHECK(projected.curve);
                    ++supported_stage;
                    ++recognized;
                } else {
                    CHECK(!projected.curve);
                    CHECK(projected.status ==
                              Status::unsupported_curve ||
                          projected.status ==
                              Status::degenerate_projection);
                    if (projected.status ==
                        Status::unsupported_curve) {
                        ++unsupported;
                    }
                }
            }
        }
        CHECK(supported_stage > 0U);
    }
    std::cout
        << "PG01A_PART_FEATURE_PROJECTION_PASS"
        << " features=4"
        << " exact_curves=" << recognized
        << " unsupported=" << unsupported << '\\n';
}

void verifyPg01aExactProjection() {
    using Status = kernel::EdgeProjectionStatus;
    kernel_occt::OcctSolidModelingKernel query;
    const auto box = query.extrude(
        kernel::LinearExtrudeInput{
            rectangle("pg01a"), 0.0, 10.0,
            kernel::ExtrudeCapRole::profile_cap,
            kernel::ExtrudeCapRole::extent_cap,
            kernel::SolidBooleanOperation::add});
    CHECK(box.ok());
    CHECK(box.current_edges.size() >= 12U);
    const kernel::Frame3 xy{};
    const kernel::Frame3 xz{
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0},
        {0.0, -1.0, 0.0}};
    const kernel::Frame3 yz{
        {0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
        {1.0, 0.0, 0.0}};
    const kernel::Frame3 displaced{
        {25.0, 5.0, 0.0},
        {0.0, 1.0, 0.0},
        {-1.0, 0.0, 0.0},
        {0.0, 0.0, 1.0}};
    for (const auto& frame : {xy, xz, yz, displaced}) {
        CHECK(frame.valid());
        std::size_t lines = 0U;
        std::size_t degenerate = 0U;
        for (const auto edge : box.current_edges) {
            const auto result =
                projectCurrentEdge(query,
                    box.solid, edge, frame);
            CHECK(result.ok() ||
                  result.status == Status::degenerate_projection);
            if (result.status == Status::degenerate_projection) {
                ++degenerate;
            }
            if (result.ok()) {
                CHECK(std::holds_alternative<kernel::Line2>(
                    *result.curve));
                ++lines;
                const auto repeated =
                    projectCurrentEdge(query,
                        box.solid, edge, frame);
                CHECK(repeated.ok());
                CHECK(repeated.curve == result.curve);
            }
        }
        CHECK(lines >= 4U);
        CHECK(degenerate > 0U);
    }
    // One exactly horizontal Edge must yield an exact Sketch Line in XY.
    bool found_bottom = false;
    for (const auto edge : box.current_edges) {
        const auto result =
            projectCurrentEdge(query,
                box.solid, edge, xy);
        if (!result.ok() ||
            !std::holds_alternative<kernel::Line2>(
                *result.curve)) {
            continue;
        }
        const auto& line =
            std::get<kernel::Line2>(*result.curve);
        if ((near(line.start.u, 0.0) &&
             near(line.end.u, 40.0) ||
             near(line.start.u, 40.0) &&
             near(line.end.u, 0.0)) &&
             near(line.start.v, 0.0) &&
             near(line.end.v, 0.0)) {
            found_bottom = true;
        }
    }
    CHECK(found_bottom);
    CHECK(projectCurrentEdge(query,
              box.solid, {}, xy).status ==
          Status::invalid_input);
    auto bad = xy;
    bad.v_axis = {2.0, 0.0, 0.0};
    CHECK(projectCurrentEdge(query,
              box.solid, box.current_edges.front(),
              bad).status ==
          Status::invalid_input);
    struct ForeignSolid final : kernel::RuntimeSolid {};
    const auto first_scope = query.bindEdgeToBody(
        box.solid, box.current_edges.front());
    CHECK(first_scope && first_scope->valid());
    CHECK(query.projectEdgeToPlane(
              std::make_shared<ForeignSolid>(),
              *first_scope, xy).status ==
          Status::provider_mismatch);
    CHECK(projectCurrentEdge(query,
              box.solid,
              kernel::RuntimeEdgeToken{999999999U},
              xy).status ==
          Status::edge_unavailable);

    const auto cylinder = query.extrude(
        kernel::LinearExtrudeInput{
            circle("pg01a-circle"), 0.0, 10.0,
            kernel::ExtrudeCapRole::profile_cap,
            kernel::ExtrudeCapRole::extent_cap,
            kernel::SolidBooleanOperation::add});
    CHECK(cylinder.ok());
    std::size_t projected_circles = 0U;
    std::size_t tilted_rejections = 0U;
    for (const auto edge : cylinder.current_edges) {
        const auto result =
            projectCurrentEdge(query,
                cylinder.solid, edge, xy);
        if (result.ok() &&
            std::holds_alternative<kernel::Circle2>(
                *result.curve)) {
            const auto& c =
                std::get<kernel::Circle2>(*result.curve);
            CHECK(near(c.radius, 10.0));
            CHECK(near(c.center.u, 0.0));
            CHECK(near(c.center.v, 0.0));
            ++projected_circles;
            const auto oblique =
                projectCurrentEdge(query,
                    cylinder.solid, edge, yz);
            CHECK(oblique.status ==
                  Status::unsupported_curve);
            CHECK(!oblique.curve);
            ++tilted_rejections;
        }
    }
    CHECK(projected_circles >= 2U);
    CHECK(tilted_rejections == projected_circles);
    std::cout << "PG01A_EXACT_EDGE_PROJECTION"
              << " box_lines=4+"
              << " circles=" << projected_circles
              << " tilted_unsupported="
              << tilted_rejections << '\\n';
}

} // namespace

int main() {
    verifyStageAccounting();
    verifyGeometrySimilarityTrap();
    verifyProspectiveDynamicSketchSupport();
    verifyColdRebuild();
    verifyPg01aExactProjection();
    verifyPg01aArcProjection();
    verifyPg01aOnCurrentPartOperations();
    verifyPg01aGenerationScope();

    std::cout
        << "PM02P_E_KERNEL_LIFECYCLE_PASS"
        << " stages=4"
        << " cold_mismatches=0"
        << " geometry_similarity_rebind=0"
        << " dynamic_support_authored_mutation=0"
        << " stale_support_frame=0\n";

    return EXIT_SUCCESS;
}
