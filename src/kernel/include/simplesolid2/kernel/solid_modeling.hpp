#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel/reference_status.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace simplesolid2::kernel {

// Runtime-only provider object. It is intentionally opaque to Part/UI and is
// never authored or serialized.
class RuntimeSolid {
public:
    RuntimeSolid() = default;
    RuntimeSolid(const RuntimeSolid&) = delete;
    RuntimeSolid& operator=(const RuntimeSolid&) = delete;
    virtual ~RuntimeSolid() = default;
};

using RuntimeSolidHandle = std::shared_ptr<const RuntimeSolid>;

struct SolidMeshTriangle final {
    Point3 first;
    Point3 second;
    Point3 third;
    // Presentation-only per-vertex normals. They preserve smooth shading
    // within one provider surface while allowing sharp normals at B-Rep
    // face boundaries. They carry no CAD identity.
    Point3 first_normal;
    Point3 second_normal;
    Point3 third_normal;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const SolidMeshTriangle&,
        const SolidMeshTriangle&) = default;
};

struct SolidPresentationMesh final {
    std::vector<SolidMeshTriangle> triangles;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const SolidPresentationMesh&,
        const SolidPresentationMesh&) = default;
};

enum class SolidPresentationStatus {
    ok,
    invalid_input,
    provider_mismatch,
    provider_failure,
    unsupported,
};

struct SolidPresentationResult final {
    SolidPresentationStatus status{
        SolidPresentationStatus::unsupported};
    SolidPresentationMesh mesh;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
                   SolidPresentationStatus::ok &&
               mesh.valid();
    }
};

struct RuntimeFaceToken final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend bool operator==(
        const RuntimeFaceToken&,
        const RuntimeFaceToken&) = default;
};

struct RuntimeEdgeToken final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend bool operator==(
        const RuntimeEdgeToken&,
        const RuntimeEdgeToken&) = default;
};

struct RuntimeVertexToken final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend bool operator==(
        const RuntimeVertexToken&,
        const RuntimeVertexToken&) = default;
};

struct RuntimeSurfaceToken final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend bool operator==(
        const RuntimeSurfaceToken&,
        const RuntimeSurfaceToken&) = default;
};

enum class SurfaceKind {
    plane,
    cylinder,
    cone,
    sphere,
    torus,
    other,
};

enum class CurveKind {
    line,
    circle,
    other,
};

enum class EdgeSemanticRoleKind {
    cap_side,
    side_side,
    boolean_intersection,
    periodic_seam,
    unsupported,
};

struct CurrentEdgeSemanticRecord final {
    RuntimeEdgeToken token;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    EdgeSemanticRoleKind role{
        EdgeSemanticRoleKind::unsupported};
    CurveKind curve_kind{CurveKind::other};
    std::vector<RuntimeSurfaceToken>
        adjacent_surfaces;
    bool representation_artifact{false};
    bool produced_by_current_operation{false};
    bool integrity_failure{false};

    friend bool operator==(
        const CurrentEdgeSemanticRecord&,
        const CurrentEdgeSemanticRecord&) = default;
};

struct CurrentVertexSemanticRecord final {
    RuntimeVertexToken token;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    std::vector<RuntimeSurfaceToken>
        adjacent_surfaces;
    std::size_t incident_material_edge_count{};
    std::optional<Point3> provider_point;
    bool produced_by_current_operation{false};
    bool integrity_failure{false};

    friend bool operator==(
        const CurrentVertexSemanticRecord&,
        const CurrentVertexSemanticRecord&) = default;
};

enum class SolidBooleanOperation {
    add,
    cut,
};

enum class ExtrudeCapRole {
    profile_cap,
    extent_cap,
    negative_cap,
    positive_cap,
};

enum class ExtrudeGeneratedFaceRoleKind {
    cap,
    side,
};

struct ExtrudeFaceRole final {
    ExtrudeGeneratedFaceRoleKind kind{
        ExtrudeGeneratedFaceRoleKind::side};
    std::optional<ExtrudeCapRole> cap_role;
    std::optional<BoundaryUseProvenance>
        side_provenance;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const ExtrudeFaceRole&,
        const ExtrudeFaceRole&) = default;
};

struct LinearExtrudeInput final {
    PlanarProfileInput profile;
    double start_offset_mm{};
    double end_offset_mm{};
    ExtrudeCapRole start_cap_role{
        ExtrudeCapRole::profile_cap};
    ExtrudeCapRole end_cap_role{
        ExtrudeCapRole::extent_cap};
    SolidBooleanOperation operation{
        SolidBooleanOperation::add};

    // PM-01 supports exactly OneSide or Midplane semantics.
    [[nodiscard]] bool valid() const noexcept;
};

enum class SolidModelingStatus {
    ok,
    invalid_input,
    missing_upstream,
    provider_mismatch,
    provider_failure,
    invalid_brep,
    detached_add,
    no_effect,
    empty_result,
    multi_solid,
};

struct NewFaceLineage final {
    ExtrudeFaceRole role;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    std::optional<RuntimeFaceToken>
        resolved_token;

    friend bool operator==(
        const NewFaceLineage&,
        const NewFaceLineage&) = default;
};

struct InheritedFaceLineage final {
    RuntimeFaceToken token;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};

    friend bool operator==(
        const InheritedFaceLineage&,
        const InheritedFaceLineage&) = default;
};

struct NewSurfaceLineage final {
    ExtrudeFaceRole role;
    ReferenceStatus surface_status{
        ReferenceStatus::unsupported};
    ReferenceStatus strict_face_status{
        ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    SurfaceKind surface_kind{SurfaceKind::other};
    std::optional<Frame3> canonical_frame;
    std::optional<RuntimeSurfaceToken>
        resolved_token;
    std::vector<RuntimeFaceToken>
        current_faces;

    friend bool operator==(
        const NewSurfaceLineage&,
        const NewSurfaceLineage&) = default;
};

struct InheritedSurfaceLineage final {
    RuntimeSurfaceToken token;
    ReferenceStatus surface_status{
        ReferenceStatus::unsupported};
    ReferenceStatus strict_face_status{
        ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    SurfaceKind surface_kind{SurfaceKind::other};
    std::optional<Frame3> canonical_frame;
    std::vector<RuntimeFaceToken>
        current_faces;

    friend bool operator==(
        const InheritedSurfaceLineage&,
        const InheritedSurfaceLineage&) = default;
};

struct SolidModelingResult final {
    SolidModelingStatus status{
        SolidModelingStatus::provider_failure};
    RuntimeSolidHandle solid;
    bool brep_valid{false};
    std::size_t solid_count{};
    std::size_t face_count{};
    std::size_t edge_count{};
    std::size_t vertex_count{};

    // PM-02 runtime-only complete current-stage topology inventory.
    // These typed tokens identify provider realizations only within the
    // current runtime/evaluation authority. They are never serialized and do
    // not define durable semantic identity.
    std::vector<RuntimeFaceToken> current_faces;
    std::vector<RuntimeEdgeToken> current_edges;
    std::vector<RuntimeVertexToken> current_vertices;

    // PM-02C evaluated semantic meaning for current bounded Edge/Vertex
    // realizations. These records are derived runtime state only. Curve/Point
    // meaning is carried by semantic relationships, not by provider ordinals
    // or geometry similarity.
    std::vector<CurrentEdgeSemanticRecord>
        current_edge_semantics;
    std::vector<CurrentVertexSemanticRecord>
        current_vertex_semantics;

    // PM-01 semantic Face lineage remains separate from complete inventory.
    std::vector<InheritedFaceLineage>
        inherited_faces;
    std::vector<NewFaceLineage> new_faces;

    // PM-02B semantic Surface lineage is distinct from strict bounded Face
    // lineage. One Surface may have multiple current Face realizations after
    // trimming/splitting while remaining one semantic carrier.
    std::vector<InheritedSurfaceLineage>
        inherited_surfaces;
    std::vector<NewSurfaceLineage>
        new_surfaces;

    [[nodiscard]] bool ok() const noexcept {
        return status == SolidModelingStatus::ok &&
               solid != nullptr &&
               brep_valid &&
               solid_count == 1U;
    }
};

class ISolidModelingKernel {
public:
    ISolidModelingKernel() = default;
    ISolidModelingKernel(
        const ISolidModelingKernel&) = delete;
    ISolidModelingKernel& operator=(
        const ISolidModelingKernel&) = delete;
    virtual ~ISolidModelingKernel() = default;

    [[nodiscard]] virtual SolidModelingResult
    extrude(
        const LinearExtrudeInput& input,
        RuntimeSolidHandle upstream = {}) noexcept = 0;

    // Presentation-only exact operation delta for Extrude preview:
    // Add => tool - upstream Body, Cut => tool ∩ upstream Body.
    // The returned mesh is derived runtime data only and carries no CAD
    // identity. Providers that cannot supply exact delta preview must return
    // unsupported rather than substitute the raw tool.
    [[nodiscard]] virtual SolidPresentationResult
    extrudePreviewMesh(
        const LinearExtrudeInput& input,
        RuntimeSolidHandle upstream = {}) noexcept;

    // Display-only tessellation. This must never influence authored CAD
    // state, modeling semantics or reference resolution.
    [[nodiscard]] virtual SolidPresentationResult
    presentationMesh(
        RuntimeSolidHandle solid) noexcept;
};

} // namespace simplesolid2::kernel
