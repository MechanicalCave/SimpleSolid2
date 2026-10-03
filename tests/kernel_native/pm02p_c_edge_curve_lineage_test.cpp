#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.C Edge/Curve lineage CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

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

kernel::PlanarProfileInput rectangle() {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        lineUse({0.0, 0.0}, {40.0, 0.0}, "bottom", 0U),
        lineUse({40.0, 0.0}, {40.0, 20.0}, "right", 1U),
        lineUse({40.0, 20.0}, {0.0, 20.0}, "top", 2U),
        lineUse({0.0, 20.0}, {0.0, 0.0}, "left", 3U),
    };
    CHECK(input.valid());
    return input;
}

kernel::PlanarProfileInput circle() {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        kernel::BoundaryUse2D{
            kernel::Circle2{{0.0, 0.0}, 10.0},
            0.0,
            0.0,
            true,
            false,
            true,
            kernel::BoundaryUseProvenance{
                "circle",
                0U,
                0U,
                false},
        },
    };
    CHECK(input.valid());
    return input;
}

std::size_t countRole(
    const kernel::ExtrudeEdgeOntologyEvidence& evidence,
    kernel::EvidenceEdgeSemanticRoleKind role) {
    return static_cast<std::size_t>(
        std::count_if(
            evidence.edges.begin(),
            evidence.edges.end(),
            [role](const kernel::EvidenceEdgeOntologyRecord& edge) {
                return edge.role == role;
            }));
}

void verifyPristineRectangleOntology() {
    const auto evidence =
        kernel_occt::buildExtrudeEdgeOntologyEvidence(
            rectangle(),
            10.0);

    CHECK(evidence.completeEdgeAccounting());
    CHECK(evidence.topology.complete());
    CHECK(evidence.topology.edges.provider_unique_count == 12U);
    CHECK(evidence.edges.size() == 12U);

    CHECK(evidence.referenceable_edge_count == 12U);
    CHECK(evidence.representation_artifact_count == 0U);
    CHECK(evidence.unsupported_edge_count == 0U);
    CHECK(evidence.integrity_failure_count == 0U);

    CHECK(
        countRole(
            evidence,
            kernel::EvidenceEdgeSemanticRoleKind::cap_side) ==
        8U);
    CHECK(
        countRole(
            evidence,
            kernel::EvidenceEdgeSemanticRoleKind::side_side) ==
        4U);

    for (const auto& edge : evidence.edges) {
        CHECK(
            edge.accounting_class ==
            kernel::EvidenceTopologyAccountingClass::referenceable);
        CHECK(
            edge.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(!edge.periodic_seam);
        CHECK(edge.adjacent_surfaces.size() == 2U);
        CHECK(
            edge.semantic_curve_kind ==
            kernel::EvidenceCurveKind::line);
        CHECK(edge.provider_curve_kind.has_value());
        CHECK(
            edge.provider_curve_kind ==
            kernel::EvidenceCurveKind::line);
    }
}

void verifyPeriodicSeamAccounting() {
    const auto evidence =
        kernel_occt::buildExtrudeEdgeOntologyEvidence(
            circle(),
            10.0);

    CHECK(evidence.completeEdgeAccounting());
    CHECK(evidence.integrity_failure_count == 0U);
    CHECK(evidence.unsupported_edge_count == 0U);
    CHECK(evidence.representation_artifact_count >= 1U);
    CHECK(
        evidence.referenceable_edge_count +
            evidence.representation_artifact_count ==
        evidence.edges.size());

    std::size_t seam_count = 0U;
    std::size_t material_circle_count = 0U;

    for (const auto& edge : evidence.edges) {
        if (edge.periodic_seam) {
            ++seam_count;
            CHECK(
                edge.accounting_class ==
                kernel::EvidenceTopologyAccountingClass::
                    known_representation_artifact);
            CHECK(
                edge.status ==
                kernel::ReferenceStatus::unsupported);
            CHECK(
                edge.role ==
                kernel::EvidenceEdgeSemanticRoleKind::
                    periodic_seam);
            continue;
        }

        CHECK(
            edge.accounting_class ==
            kernel::EvidenceTopologyAccountingClass::referenceable);
        CHECK(
            edge.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            edge.role ==
            kernel::EvidenceEdgeSemanticRoleKind::cap_side);
        CHECK(
            edge.semantic_curve_kind ==
            kernel::EvidenceCurveKind::circle);
        CHECK(
            edge.provider_curve_kind ==
            kernel::EvidenceCurveKind::circle);
        ++material_circle_count;
    }

    CHECK(seam_count == evidence.representation_artifact_count);
    CHECK(seam_count >= 1U);
    CHECK(material_circle_count == 2U);
}

void verifyBooleanLineage() {
    const auto unchanged =
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::unchanged);
    CHECK(unchanged.ok());
    CHECK(
        unchanged.edge_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(unchanged.current_edge_realization_count == 1U);
    CHECK(
        unchanged.source_history.unique_descendant_count == 1U);
    CHECK(unchanged.all_provider_curves_match_kind);

    const auto trimmed =
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::trim);
    CHECK(trimmed.ok());
    CHECK(
        trimmed.edge_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(trimmed.current_edge_realization_count == 1U);
    CHECK(
        trimmed.source_history.unique_descendant_count == 1U);
    CHECK(
        trimmed.source_history.modified_count > 0U ||
        !trimmed.source_history.unchanged_present);
    CHECK(trimmed.all_provider_curves_match_kind);

    const auto split =
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::split);
    CHECK(split.ok());
    CHECK(
        split.edge_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(split.current_edge_realization_count == 2U);
    CHECK(
        split.source_history.unique_descendant_count == 2U);
    CHECK(split.all_provider_curves_match_kind);

    const auto removed =
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::remove);
    CHECK(removed.ok());
    CHECK(
        removed.edge_status ==
        kernel::ReferenceStatus::missing);
    CHECK(removed.current_edge_realization_count == 0U);
    CHECK(
        removed.source_history.unique_descendant_count == 0U);
    CHECK(removed.source_history.deleted);
    CHECK(removed.all_provider_curves_match_kind);
}

void verifyBooleanIntersectionEdge() {
    const auto evidence =
        kernel_occt::buildBooleanIntersectionEdgeEvidence();

    CHECK(evidence.ok());
    CHECK(evidence.topology.complete());
    CHECK(
        evidence.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.current_edge_realization_count == 1U);
    CHECK(
        evidence.semantic_curve_kind ==
        kernel::EvidenceCurveKind::line);
    CHECK(evidence.all_provider_curves_match_kind);
    CHECK(evidence.absent_from_both_source_shapes);

    CHECK(
        evidence.first_surface.role ==
        kernel::EvidenceSurfaceCarrierRoleKind::end_cap);
    CHECK(
        evidence.second_surface.role ==
        kernel::EvidenceSurfaceCarrierRoleKind::side);
    CHECK(evidence.second_surface.provenance.has_value());
    CHECK(
        evidence.second_surface.provenance->source_entity ==
        "tool-bottom");
}

void verifyMultipleIntersectionBranches() {
    const auto evidence =
        kernel_occt::buildSurfacePairBranchEvidence();

    CHECK(evidence.ok());
    CHECK(evidence.topology.complete());
    CHECK(
        evidence.pair_only_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(evidence.branch_count == 2U);
    CHECK(
        evidence.semantic_curve_kind ==
        kernel::EvidenceCurveKind::line);
    CHECK(evidence.all_provider_curves_match_kind);

    CHECK(
        evidence.first_surface.role ==
        kernel::EvidenceSurfaceCarrierRoleKind::end_cap);
    CHECK(
        evidence.second_surface.role ==
        kernel::EvidenceSurfaceCarrierRoleKind::side);
    CHECK(evidence.second_surface.provenance.has_value());
    CHECK(
        evidence.second_surface.provenance->source_entity ==
        "base-bottom");
}

struct EvidenceSet final {
    kernel::ExtrudeEdgeOntologyEvidence rectangle;
    kernel::ExtrudeEdgeOntologyEvidence circle;
    kernel::EdgeBooleanLineageEvidence unchanged;
    kernel::EdgeBooleanLineageEvidence trimmed;
    kernel::EdgeBooleanLineageEvidence split;
    kernel::EdgeBooleanLineageEvidence removed;
    kernel::BooleanIntersectionEdgeEvidence intersection;
    kernel::SurfacePairBranchEvidence branches;

    friend bool operator==(
        const EvidenceSet&,
        const EvidenceSet&) = default;
};

EvidenceSet evaluate() {
    return {
        kernel_occt::buildExtrudeEdgeOntologyEvidence(
            rectangle(),
            10.0),
        kernel_occt::buildExtrudeEdgeOntologyEvidence(
            circle(),
            10.0),
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::unchanged),
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::trim),
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::split),
        kernel_occt::buildEdgeBooleanLineageEvidence(
            kernel::EdgeBooleanProbeScenario::remove),
        kernel_occt::buildBooleanIntersectionEdgeEvidence(),
        kernel_occt::buildSurfacePairBranchEvidence(),
    };
}

} // namespace

int main() {
    verifyPristineRectangleOntology();
    verifyPeriodicSeamAccounting();
    verifyBooleanLineage();
    verifyBooleanIntersectionEdge();
    verifyMultipleIntersectionBranches();

    // Cold reconstruction from declared semantic inputs only.
    const auto first = evaluate();
    const auto cold = evaluate();
    CHECK(cold == first);

    std::cout
        << "PM02P_C_EDGE_CURVE_PASS"
        << " false_resolved=0"
        << " rectangle_edges=12"
        << " split_edge=ambiguous"
        << " deleted_edge=missing"
        << " surface_pair_branches=ambiguous"
        << " seam=representation_artifact\n";

    return EXIT_SUCCESS;
}
