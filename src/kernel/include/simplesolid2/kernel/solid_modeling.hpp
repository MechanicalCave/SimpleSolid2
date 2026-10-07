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


// PM-02D presentation-only topology payload extracted from the same
// RuntimeSolid generation as the committed Body. Runtime tokens remain
// provider realization identifiers only and are never persisted.
struct BodyFacePresentationRange final {
    RuntimeFaceToken runtime_token;
    std::size_t first_triangle{};
    std::size_t triangle_count{};

    [[nodiscard]] bool valid(
        std::size_t total_triangles) const noexcept;

    friend bool operator==(
        const BodyFacePresentationRange&,
        const BodyFacePresentationRange&) = default;
};

struct BodyEdgePresentationPath final {
    RuntimeEdgeToken runtime_token;
    std::vector<Point3> points;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyEdgePresentationPath&,
        const BodyEdgePresentationPath&) = default;
};

struct BodyVertexPresentationPoint final {
    RuntimeVertexToken runtime_token;
    Point3 point;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyVertexPresentationPoint&,
        const BodyVertexPresentationPoint&) = default;
};

struct BodyPresentation final {
    SolidPresentationMesh mesh;
    std::vector<BodyFacePresentationRange> faces;
    std::vector<BodyEdgePresentationPath> edges;
    std::vector<BodyVertexPresentationPoint> vertices;

    [[nodiscard]] bool valid() const noexcept;
};

struct BodyPresentationResult final {
    SolidPresentationStatus status{
        SolidPresentationStatus::unsupported};
    BodyPresentation body;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
                   SolidPresentationStatus::ok &&
               body.valid();
    }
};


// Complete current-stage provider observations used by Part to construct
// semantic Curve/Point meaning. Runtime Surface tokens are adjacency evidence
// only; provider topology order and geometry diagnostics never become durable
// identity.
struct CurrentEdgeSemanticObservation final {
    RuntimeEdgeToken runtime_token;
    CurveKind provider_curve_kind{
        CurveKind::other};
    bool periodic_seam{false};
    std::vector<RuntimeSurfaceToken>
        adjacent_surfaces;
    // True when this provider Edge only partitions two bounded Face
    // realizations of one tracked semantic Surface carrier. Runtime-derived
    // representation evidence only; never durable identity.
    bool same_surface_partition{false};

    friend bool operator==(
        const CurrentEdgeSemanticObservation&,
        const CurrentEdgeSemanticObservation&) = default;
};

struct CurrentVertexSemanticObservation final {
    RuntimeVertexToken runtime_token;
    std::vector<RuntimeSurfaceToken>
        adjacent_surfaces;
    std::vector<RuntimeEdgeToken>
        incident_material_edges;
    // Diagnostic only. XYZ is never identity.
    std::optional<Point3> provider_point;

    friend bool operator==(
        const CurrentVertexSemanticObservation&,
        const CurrentVertexSemanticObservation&) = default;
};

struct InheritedEdgeRealizationLineage final {
    RuntimeEdgeToken source_token;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    std::vector<RuntimeEdgeToken>
        current_edges;

    friend bool operator==(
        const InheritedEdgeRealizationLineage&,
        const InheritedEdgeRealizationLineage&) = default;
};

struct InheritedVertexRealizationLineage final {
    RuntimeVertexToken source_token;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    std::vector<RuntimeVertexToken>
        current_vertices;

    friend bool operator==(
        const InheritedVertexRealizationLineage&,
        const InheritedVertexRealizationLineage&) = default;
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

enum class GeneratedFaceRoleKind {
    cap,
    side,
    revolve_start_cap,
    revolve_end_cap,
    revolve_side,
};

// Compatibility alias: existing Extrude code/tests keep the accepted PM-01
// vocabulary while the underlying provider-neutral role family now also
// carries PM-04 Revolve topology provenance.
using ExtrudeGeneratedFaceRoleKind =
    GeneratedFaceRoleKind;

struct GeneratedFaceRole final {
    GeneratedFaceRoleKind kind{
        GeneratedFaceRoleKind::side};
    std::optional<ExtrudeCapRole> cap_role;
    std::optional<BoundaryUseProvenance>
        side_provenance;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const GeneratedFaceRole&,
        const GeneratedFaceRole&) = default;
};

using ExtrudeFaceRole = GeneratedFaceRole;

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

struct Axis3 final {
    Point3 origin;
    Point3 direction{1.0, 0.0, 0.0};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const Axis3&,
        const Axis3&) = default;
};

struct AngularRevolveInput final {
    PlanarProfileInput profile;
    Axis3 axis;
    double start_angle_radians{};
    double end_angle_radians{};
    SolidBooleanOperation operation{
        SolidBooleanOperation::add};

    // PM-04 accepts one authored sweep interval with magnitude
    // 0 < |end-start| <= 2*pi. OneSide Reverse is represented by a negative
    // interval; Midplane is represented symmetrically around zero.
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool fullTurn() const noexcept;

    friend bool operator==(
        const AngularRevolveInput&,
        const AngularRevolveInput&) = default;
};

enum class EdgeFeatureOperation {
    fillet,
    chamfer,
};

struct EdgeFeatureInput final {
    EdgeFeatureOperation operation{
        EdgeFeatureOperation::fillet};
    // Runtime tokens remain ordered by the canonical durable semantic Edge
    // set. Numeric provider token order has no authored meaning.
    std::vector<RuntimeEdgeToken> edges;
    double parameter_mm{};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const EdgeFeatureInput&,
        const EdgeFeatureInput&) = default;
};

// Transient T1 evidence. Production providers must inspect the native
// contour/membership induced by the explicit registration before accepting a
// successful edge operation. Traversal order is irrelevant; set equality is
// mandatory. This evidence is runtime-only and never persisted.
struct EdgeFeatureInputMembership final {
    std::vector<RuntimeEdgeToken>
        provider_contour_edges;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool exactFor(
        const std::vector<RuntimeEdgeToken>&
            requested_edges) const noexcept;

    friend bool operator==(
        const EdgeFeatureInputMembership&,
        const EdgeFeatureInputMembership&) = default;
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
    // ADR-0017 runtime-only evidence. A created Add Surface may be absorbed
    // into exactly one inherited carrier when Boolean lineage proves unique
    // continuation. The created role then remains contribution evidence only.
    std::optional<RuntimeSurfaceToken>
        continued_into;
    std::vector<RuntimeFaceToken>
        contribution_faces;

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

    // PM-02C complete current-stage Edge/Vertex semantic observations. There
    // is exactly one observation per current runtime Edge/Vertex token.
    std::vector<CurrentEdgeSemanticObservation>
        current_edge_semantics;
    std::vector<CurrentVertexSemanticObservation>
        current_vertex_semantics;

    // Transient provider-history evidence across one adjacent Body stage.
    // Part may use this only to corroborate semantic lifecycle. It is never
    // persisted and never chooses among semantic candidates by provider order.
    std::vector<InheritedEdgeRealizationLineage>
        inherited_edge_realizations;
    std::vector<InheritedVertexRealizationLineage>
        inherited_vertex_realizations;

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

    // Present only for a PM-05 edge operation. Appended to preserve source
    // meaning of existing positional aggregate initializers.
    std::optional<EdgeFeatureInputMembership>
        edge_feature_input_membership;

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

    // Provider-neutral PM-04 rotational solid operation. The default keeps
    // bounded test providers source-compatible and fails closed for a valid
    // request until that provider explicitly implements Revolve.
    [[nodiscard]] virtual SolidModelingResult
    revolve(
        const AngularRevolveInput& input,
        RuntimeSolidHandle upstream = {}) noexcept;

    // Provider-neutral PM-05 explicit multi-Edge operation. The default keeps
    // bounded/fake providers source-compatible and fails closed until a
    // provider implements Fillet/Chamfer. PM-05C2 owns production OCCT.
    [[nodiscard]] virtual SolidModelingResult
    edgeFeature(
        const EdgeFeatureInput& input,
        RuntimeSolidHandle upstream = {}) noexcept;

    // Presentation-only exact operation delta for Extrude preview:
    // Add => tool - upstream Body, Cut => tool ∩ upstream Body.
    // The returned mesh is derived runtime data only and carries no CAD
    // identity. Providers that cannot supply exact delta preview must return
    // unsupported rather than substitute the raw tool.
    [[nodiscard]] virtual SolidPresentationResult
    extrudePreviewMesh(
        const LinearExtrudeInput& input,
        RuntimeSolidHandle upstream = {}) noexcept;

    // Presentation-only exact operation delta for Revolve. Same semantics as
    // Extrude preview: Add => tool - upstream Body, Cut => tool ∩ upstream
    // Body. Never returns the raw tool as a substitute for an exact delta.
    [[nodiscard]] virtual SolidPresentationResult
    revolvePreviewMesh(
        const AngularRevolveInput& input,
        RuntimeSolidHandle upstream = {}) noexcept;

    // Display-only tessellation. This must never influence authored CAD
    // state, modeling semantics or reference resolution.
    [[nodiscard]] virtual SolidPresentationResult
    presentationMesh(
        RuntimeSolidHandle solid) noexcept;

    // Atomic committed-Body presentation. Production providers must derive
    // mesh and topology presentation from the same RuntimeSolid generation.
    // Default compatibility falls back to mesh-only presentation so bounded
    // test providers remain valid while PM-02D migrates the production path.
    [[nodiscard]] virtual BodyPresentationResult
    bodyPresentation(
        RuntimeSolidHandle solid) noexcept;
};

} // namespace simplesolid2::kernel
