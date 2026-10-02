#pragma once

#include <simplesolid2/kernel/profile_input.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace simplesolid2::kernel {

enum class EvidenceStatus {
    ok,
    invalid_input,
    provider_failure,
    invalid_brep,
};

struct BoundaryLineageEvidence final {
    BoundaryUseProvenance provenance;
    std::size_t generated_edge_count{};

    friend bool operator==(
        const BoundaryLineageEvidence&,
        const BoundaryLineageEvidence&) = default;
};

struct ShapeEvidence final {
    EvidenceStatus status{EvidenceStatus::provider_failure};
    bool brep_valid{false};
    std::size_t solid_count{};
    std::size_t face_count{};
    std::size_t wire_count{};
    std::size_t edge_count{};
    std::vector<BoundaryLineageEvidence> boundary_lineage;

    [[nodiscard]] bool ok() const noexcept {
        return status == EvidenceStatus::ok && brep_valid;
    }

    friend bool operator==(
        const ShapeEvidence&,
        const ShapeEvidence&) = default;
};

enum class ReferenceStatus {
    resolved,
    missing,
    ambiguous,
    unsupported,
};

enum class ExtrudeFaceRoleKind {
    start_cap,
    end_cap,
    side,
};

enum class FaceSurfaceKind {
    plane,
    cylinder,
    cone,
    sphere,
    torus,
    other,
};

struct FaceGeometryDiagnostics final {
    FaceSurfaceKind surface_kind{
        FaceSurfaceKind::other};
    double area{};
    Point3 centroid;
    Point3 surface_axis;

    friend bool operator==(
        const FaceGeometryDiagnostics&,
        const FaceGeometryDiagnostics&) = default;
};

struct ExtrudeFaceEvidence final {
    ExtrudeFaceRoleKind role{ExtrudeFaceRoleKind::side};
    ReferenceStatus status{ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    std::optional<BoundaryUseProvenance> provenance;

    // Evidence diagnostics only. These counts expose where provider lineage
    // failed without becoming semantic identity or persistence.
    std::size_t basis_edge_match_count{};
    std::size_t generated_shape_count{};
    std::optional<FaceGeometryDiagnostics>
        geometry_diagnostics;

    friend bool operator==(
        const ExtrudeFaceEvidence&,
        const ExtrudeFaceEvidence&) = default;
};

enum class ProviderLineageObservation {
    unchanged,
    modified,
    generated,
    deleted,
};

struct ProviderSourceLineageEvidence final {
    BoundaryUseProvenance provenance;
    ProviderLineageObservation observation{
        ProviderLineageObservation::unchanged};

    friend bool operator==(
        const ProviderSourceLineageEvidence&,
        const ProviderSourceLineageEvidence&) = default;
};

struct CardinalityCandidateEvidence final {
    std::vector<BoundaryUseProvenance>
        source_provenance;
    bool semantic_role_match{true};
    ProviderLineageObservation observation{
        ProviderLineageObservation::modified};

    friend bool operator==(
        const CardinalityCandidateEvidence&,
        const CardinalityCandidateEvidence&) = default;
};

struct CardinalityFixtureEvidence final {
    std::size_t source_edge_count{};
    std::size_t physical_candidate_count{};
    bool provider_geometry_valid{false};
    std::vector<CardinalityCandidateEvidence>
        candidates;
    std::vector<ProviderSourceLineageEvidence>
        source_history;

    friend bool operator==(
        const CardinalityFixtureEvidence&,
        const CardinalityFixtureEvidence&) = default;
};

struct SingularReferenceCardinalityEvidence final {
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t physical_candidate_count{};
    std::size_t semantic_candidate_count{};
    std::size_t merged_source_count{};
    bool aggregate_requested{false};

    friend bool operator==(
        const SingularReferenceCardinalityEvidence&,
        const SingularReferenceCardinalityEvidence&) = default;
};

[[nodiscard]] inline bool
sameBoundaryProvenance(
    const BoundaryUseProvenance& first,
    const BoundaryUseProvenance& second) noexcept {
    return first == second;
}

[[nodiscard]] inline SingularReferenceCardinalityEvidence
classifySingularReferenceCardinality(
    const BoundaryUseProvenance& target,
    const std::vector<CardinalityCandidateEvidence>&
        candidates,
    bool aggregate_requested = false) {
    SingularReferenceCardinalityEvidence result;
    result.physical_candidate_count =
        candidates.size();
    result.aggregate_requested =
        aggregate_requested;

    for (const auto& candidate : candidates) {
        if (!candidate.semantic_role_match) {
            continue;
        }

        bool contains_target = false;
        for (const auto& provenance :
             candidate.source_provenance) {
            if (sameBoundaryProvenance(
                    provenance,
                    target)) {
                contains_target = true;
                break;
            }
        }
        if (!contains_target) {
            continue;
        }

        ++result.semantic_candidate_count;
        if (candidate.source_provenance.size() >
            result.merged_source_count) {
            result.merged_source_count =
                candidate.source_provenance.size();
        }
    }

    if (aggregate_requested) {
        result.status =
            ReferenceStatus::unsupported;
    } else if (
        result.semantic_candidate_count == 0U) {
        result.status =
            ReferenceStatus::missing;
    } else if (
        result.semantic_candidate_count != 1U ||
        result.merged_source_count != 1U) {
        result.status =
            ReferenceStatus::ambiguous;
    } else {
        result.status =
            ReferenceStatus::resolved;
    }

    return result;
}

struct ExtrudeEvidence final {
    ShapeEvidence shape;
    ExtrudeFaceEvidence start_cap{
        ExtrudeFaceRoleKind::start_cap,
        ReferenceStatus::unsupported,
        0U,
        std::nullopt};
    ExtrudeFaceEvidence end_cap{
        ExtrudeFaceRoleKind::end_cap,
        ReferenceStatus::unsupported,
        0U,
        std::nullopt};
    std::vector<ExtrudeFaceEvidence> sides;

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U &&
               start_cap.status ==
                   ReferenceStatus::resolved &&
               end_cap.status ==
                   ReferenceStatus::resolved;
    }

    friend bool operator==(
        const ExtrudeEvidence&,
        const ExtrudeEvidence&) = default;
};

} // namespace simplesolid2::kernel
