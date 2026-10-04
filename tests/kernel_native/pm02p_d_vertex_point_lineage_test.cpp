#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <algorithm>
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
            << "PM-02P.D Vertex/Point lineage CHECK failed at line "
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

kernel::PlanarProfileInput rectangle(
    double width = 40.0,
    double height = 20.0,
    const std::string& prefix = "base") {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        lineUse(
            {0.0, 0.0},
            {width, 0.0},
            prefix + "-bottom",
            0U),
        lineUse(
            {width, 0.0},
            {width, height},
            prefix + "-right",
            1U),
        lineUse(
            {width, height},
            {0.0, height},
            prefix + "-top",
            2U),
        lineUse(
            {0.0, height},
            {0.0, 0.0},
            prefix + "-left",
            3U),
    };
    CHECK(input.valid());
    return input;
}

kernel::PlanarProfileInput chamferedRectangle() {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        lineUse(
            {5.0, 0.0},
            {40.0, 0.0},
            "base-bottom",
            0U),
        lineUse(
            {40.0, 0.0},
            {40.0, 20.0},
            "base-right",
            1U),
        lineUse(
            {40.0, 20.0},
            {0.0, 20.0},
            "base-top",
            2U),
        lineUse(
            {0.0, 20.0},
            {0.0, 5.0},
            "base-left",
            3U),
        lineUse(
            {0.0, 5.0},
            {5.0, 0.0},
            "base-chamfer",
            4U),
    };
    CHECK(input.valid());
    return input;
}

std::size_t countCapRole(
    const kernel::ExtrudeVertexOntologyEvidence& evidence,
    kernel::EvidenceSurfaceCarrierRoleKind role) {
    std::size_t count = 0U;
    for (const auto& vertex : evidence.vertices) {
        const auto found =
            std::find_if(
                vertex.semantic_key.adjacent_surfaces.begin(),
                vertex.semantic_key.adjacent_surfaces.end(),
                [role](
                    const kernel::EvidenceSurfaceCarrierKey& key) {
                    return key.role == role;
                });
        if (found !=
            vertex.semantic_key.adjacent_surfaces.end()) {
            ++count;
        }
    }
    return count;
}

void verifyPristineVertexOntology() {
    const auto evidence =
        kernel_occt::buildExtrudeVertexOntologyEvidence(
            rectangle(),
            10.0);

    CHECK(evidence.completeVertexAccounting());
    CHECK(evidence.topology.complete());
    CHECK(
        evidence.topology.vertices.provider_unique_count ==
        8U);
    CHECK(evidence.vertices.size() == 8U);
    CHECK(evidence.referenceable_vertex_count == 8U);
    CHECK(evidence.unsupported_vertex_count == 0U);
    CHECK(evidence.integrity_failure_count == 0U);

    CHECK(
        countCapRole(
            evidence,
            kernel::EvidenceSurfaceCarrierRoleKind::start_cap) ==
        4U);
    CHECK(
        countCapRole(
            evidence,
            kernel::EvidenceSurfaceCarrierRoleKind::end_cap) ==
        4U);

    for (const auto& vertex : evidence.vertices) {
        CHECK(
            vertex.accounting_class ==
            kernel::EvidenceTopologyAccountingClass::referenceable);
        CHECK(
            vertex.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            vertex.semantic_key.adjacent_surfaces.size() ==
            3U);
        CHECK(vertex.incident_material_edge_count == 3U);
        CHECK(vertex.provider_point.has_value());

        const auto side_count =
            static_cast<std::size_t>(
                std::count_if(
                    vertex.semantic_key.adjacent_surfaces.begin(),
                    vertex.semantic_key.adjacent_surfaces.end(),
                    [](const kernel::EvidenceSurfaceCarrierKey& key) {
                        return key.role ==
                               kernel::
                                   EvidenceSurfaceCarrierRoleKind::
                                       side;
                    }));
        CHECK(side_count == 2U);
    }
}

void verifyDimensionEdit() {
    const auto evidence =
        kernel_occt::buildVertexDimensionEditEvidence();

    CHECK(evidence.ok());
    CHECK(
        evidence.before_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.after_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.semantic_key.adjacent_surfaces.size() == 3U);
    CHECK(evidence.before_point.has_value());
    CHECK(evidence.after_point.has_value());
    CHECK(evidence.point_moved);

    // Geometry changed, semantic key did not. Coordinates are diagnostics,
    // not the resolution authority.
    CHECK(evidence.before_point->x == 40.0);
    CHECK(evidence.after_point->x == 55.0);
    CHECK(evidence.before_point->y == 0.0);
    CHECK(evidence.after_point->y == 0.0);
}

void verifyDeletionAndGeneratedVertices() {
    const auto evidence =
        kernel_occt::buildVertexDeletionGenerationEvidence();

    CHECK(evidence.ok());
    CHECK(
        evidence.before_deleted_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.after_deleted_status ==
        kernel::ReferenceStatus::missing);

    CHECK(evidence.generated_keys.size() == 2U);
    CHECK(evidence.generated_statuses.size() == 2U);
    CHECK(evidence.generated_vertex_count == 2U);
    for (const auto status :
         evidence.generated_statuses) {
        CHECK(
            status ==
            kernel::ReferenceStatus::resolved);
    }

    CHECK(
        evidence.before_topology.vertices.provider_unique_count ==
        8U);
    CHECK(
        evidence.after_topology.vertices.provider_unique_count ==
        10U);

    // The edited profile itself must also have complete, semantic Vertex
    // accounting: the two new Point meanings are not hidden provider detail.
    const auto chamfer_ontology =
        kernel_occt::buildExtrudeVertexOntologyEvidence(
            chamferedRectangle(),
            10.0);
    CHECK(chamfer_ontology.completeVertexAccounting());
    CHECK(chamfer_ontology.referenceable_vertex_count == 10U);
    CHECK(chamfer_ontology.unsupported_vertex_count == 0U);
    CHECK(chamfer_ontology.integrity_failure_count == 0U);
}

void verifySamePointReplacement() {
    const auto evidence =
        kernel_occt::buildVertexSamePointReplacementEvidence();

    CHECK(evidence.ok());
    CHECK(
        evidence.old_key_in_old_shape ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.old_key_in_replacement_shape ==
        kernel::ReferenceStatus::missing);
    CHECK(
        evidence.replacement_key_status ==
        kernel::ReferenceStatus::resolved);

    CHECK(evidence.old_point.has_value());
    CHECK(evidence.replacement_point.has_value());
    CHECK(evidence.provider_points_equal);

    // Exact same provider coordinate is deliberately insufficient to preserve
    // the old semantic Point meaning.
    CHECK(evidence.old_point->x == evidence.replacement_point->x);
    CHECK(evidence.old_point->y == evidence.replacement_point->y);
    CHECK(evidence.old_point->z == evidence.replacement_point->z);

    const auto replacement_ontology =
        kernel_occt::buildExtrudeVertexOntologyEvidence(
            rectangle(
                40.0,
                20.0,
                "replacement"),
            10.0);
    CHECK(replacement_ontology.completeVertexAccounting());
    CHECK(replacement_ontology.referenceable_vertex_count == 8U);
}

struct EvidenceSet final {
    kernel::ExtrudeVertexOntologyEvidence pristine;
    kernel::VertexDimensionEditEvidence dimension_edit;
    kernel::VertexDeletionGenerationEvidence delete_generate;
    kernel::VertexSamePointReplacementEvidence replacement;

    friend bool operator==(
        const EvidenceSet&,
        const EvidenceSet&) = default;
};

EvidenceSet evaluate() {
    return {
        kernel_occt::buildExtrudeVertexOntologyEvidence(
            rectangle(),
            10.0),
        kernel_occt::buildVertexDimensionEditEvidence(),
        kernel_occt::buildVertexDeletionGenerationEvidence(),
        kernel_occt::buildVertexSamePointReplacementEvidence(),
    };
}

} // namespace

int main() {
    verifyPristineVertexOntology();
    verifyDimensionEdit();
    verifyDeletionAndGeneratedVertices();
    verifySamePointReplacement();

    // Cold reconstruction from declared semantic inputs only.
    const auto first = evaluate();
    const auto cold = evaluate();
    CHECK(cold == first);

    std::cout
        << "PM02P_D_VERTEX_POINT_PASS"
        << " false_resolved=0"
        << " pristine_vertices=8"
        << " moved_vertex=resolved"
        << " deleted_vertex=missing"
        << " generated_vertices=2"
        << " same_xyz_replacement=distinct\n";

    return EXIT_SUCCESS;
}
