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


enum class EdgeSplitProbeScenario {
    middle_notch,
    remove_target,
};

enum class FaceMergeProbeScenario {
    overlapping_coplanar,
    absorbed_inner,
};

// PM-00A E03/E04 raw provider-history evidence. These values are transient
// observations only; they are not a persistent selector or identity format.
struct BooleanSubshapeHistoryEvidence final {
    std::size_t modified_count{};
    std::size_t generated_count{};
    bool deleted{false};
    bool unchanged_present{false};
    std::size_t unique_descendant_count{};

    friend bool operator==(
        const BooleanSubshapeHistoryEvidence&,
        const BooleanSubshapeHistoryEvidence&) = default;
};

struct EdgeSplitHistoryEvidence final {
    ShapeEvidence shape;
    BooleanSubshapeHistoryEvidence target;

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U;
    }

    friend bool operator==(
        const EdgeSplitHistoryEvidence&,
        const EdgeSplitHistoryEvidence&) = default;
};

struct FaceMergeHistoryEvidence final {
    ShapeEvidence shape;
    BooleanSubshapeHistoryEvidence first;
    BooleanSubshapeHistoryEvidence second;
    std::size_t shared_descendant_count{};

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U;
    }

    friend bool operator==(
        const FaceMergeHistoryEvidence&,
        const FaceMergeHistoryEvidence&) = default;
};

} // namespace simplesolid2::kernel
