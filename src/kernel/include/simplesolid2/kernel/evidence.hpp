#pragma once

#include <simplesolid2/kernel/profile_input.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
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

using TransientLineageToken = std::uint32_t;

struct SingularLineageSourceEvidence final {
    std::string semantic_source;
    bool aggregate_requested{false};
    bool provider_reports_deleted{false};

    // Provider candidates are diagnostics only. Tokens are request-local and
    // have no durable meaning outside this evidence evaluation.
    std::vector<TransientLineageToken>
        provider_candidates;

    // Candidates remaining after semantic producer/stage/role filtering.
    // Geometry similarity is never allowed to populate this set.
    std::vector<TransientLineageToken>
        semantic_candidates;

    friend bool operator==(
        const SingularLineageSourceEvidence&,
        const SingularLineageSourceEvidence&) = default;
};

[[nodiscard]] inline ReferenceStatus
classifySingularLineage(
    const std::vector<SingularLineageSourceEvidence>&
        sources,
    const std::string& semantic_source) noexcept {
    const SingularLineageSourceEvidence* target =
        nullptr;
    for (const auto& source : sources) {
        if (source.semantic_source ==
            semantic_source) {
            target = &source;
            break;
        }
    }

    if (!target) {
        return ReferenceStatus::missing;
    }
    if (target->aggregate_requested) {
        return ReferenceStatus::unsupported;
    }

    std::vector<TransientLineageToken> unique;
    unique.reserve(
        target->semantic_candidates.size());
    for (const auto token :
         target->semantic_candidates) {
        bool seen = false;
        for (const auto existing : unique) {
            if (existing == token) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            unique.push_back(token);
        }
    }

    if (unique.empty()) {
        return ReferenceStatus::missing;
    }
    if (unique.size() > 1U) {
        return ReferenceStatus::ambiguous;
    }

    const auto candidate = unique.front();
    std::size_t semantic_owner_count = 0U;
    for (const auto& source : sources) {
        bool owns = false;
        for (const auto token :
             source.semantic_candidates) {
            if (token == candidate) {
                owns = true;
                break;
            }
        }
        if (owns) {
            ++semantic_owner_count;
        }
    }

    return semantic_owner_count == 1U
        ? ReferenceStatus::resolved
        : ReferenceStatus::ambiguous;
}

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
