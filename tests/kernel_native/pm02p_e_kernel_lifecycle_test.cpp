#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
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

} // namespace

int main() {
    verifyStageAccounting();
    verifyGeometrySimilarityTrap();
    verifyProspectiveDynamicSketchSupport();
    verifyColdRebuild();

    std::cout
        << "PM02P_E_KERNEL_LIFECYCLE_PASS"
        << " stages=4"
        << " cold_mismatches=0"
        << " geometry_similarity_rebind=0"
        << " dynamic_support_authored_mutation=0"
        << " stale_support_frame=0\n";

    return EXIT_SUCCESS;
}
