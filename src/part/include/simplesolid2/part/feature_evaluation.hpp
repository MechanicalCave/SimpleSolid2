#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/reference_status.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>
#include <simplesolid2/sketch/entity_id.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::part {

class PartDocument;
struct DatumEvaluation;

enum class FeatureEvaluationStatus {
    up_to_date,
    failed,
    blocked,
    suppressed,
};

enum class FeatureEvaluationDiagnosticCode {
    none,
    missing_profile,
    unresolved_profile,
    sketch_support_missing,
    sketch_support_ambiguous,
    sketch_support_unsupported,
    missing_upstream_body,
    upstream_unavailable,
    kernel_invalid_input,
    kernel_provider_mismatch,
    kernel_provider_failure,
    invalid_brep,
    detached_add,
    no_effect,
    empty_result,
    multi_solid,
    topology_integrity_failure,
};

enum class BodyEvaluationStatus {
    empty,
    up_to_date,
    unavailable,
};

enum class TopologyAccountingClass {
    referenceable,
    known_representation_artifact,
    semantically_unsupported,
    integrity_failure,
};

enum class FeatureFaceRoleKind {
    profile_cap,
    extent_cap,
    negative_cap,
    positive_cap,
    side,
};

struct FeatureFaceAddress final {
    FeatureId producer_feature_id;
    FeatureFaceRoleKind role{
        FeatureFaceRoleKind::side};
    std::optional<sketch::EntityId>
        source_entity;
    std::uint32_t loop_index{};
    std::uint32_t use_index{};
    bool hole{false};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureFaceAddress&,
        const FeatureFaceAddress&) = default;
};

struct FeatureFaceResolution final {
    FeatureFaceAddress address;
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    // Runtime-only bridge to the current provider result. Never serialized.
    std::optional<kernel::RuntimeFaceToken>
        runtime_token;

    friend bool operator==(
        const FeatureFaceResolution&,
        const FeatureFaceResolution&) = default;
};

struct FeatureSurfaceResolution final {
    FeatureSurfaceAddress address;
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    kernel::ReferenceStatus strict_face_status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    kernel::SurfaceKind surface_kind{
        kernel::SurfaceKind::other};
    std::optional<kernel::Frame3>
        canonical_frame;
    // Runtime-only semantic-carrier bridge. Present only while Surface is
    // Resolved in the current provider generation; never serialized.
    std::optional<kernel::RuntimeSurfaceToken>
        runtime_token;
    // Current bounded Face realizations of this carrier. May contain >1 token
    // while Surface remains Resolved and strict Face becomes Ambiguous.
    std::vector<kernel::RuntimeFaceToken>
        current_faces;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureSurfaceResolution&,
        const FeatureSurfaceResolution&) = default;
};

enum class FeatureCurveRoleKind {
    cap_side,
    side_side,
    boolean_intersection,
};

struct FeatureCurveAddress final {
    FeatureId producer_feature_id;
    FeatureCurveRoleKind role{
        FeatureCurveRoleKind::boolean_intersection};
    // Exactly two distinct Surface addresses in canonical semantic order.
    std::vector<FeatureSurfaceAddress>
        adjacent_surfaces;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureCurveAddress&,
        const FeatureCurveAddress&) = default;
    friend auto operator<=>(
        const FeatureCurveAddress&,
        const FeatureCurveAddress&) = default;
};

struct FeatureCurveResolution final {
    FeatureCurveAddress address;
    // Semantic carrier status. An inherited Curve may remain Resolved while
    // its bounded Edge realization splits into several current Edges.
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    // Strict bounded Edge selector status for the current stage.
    kernel::ReferenceStatus strict_edge_status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_edge_count{};
    kernel::CurveKind curve_kind{
        kernel::CurveKind::other};
    std::vector<kernel::RuntimeEdgeToken>
        current_edges;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureCurveResolution&,
        const FeatureCurveResolution&) = default;
};

struct FeaturePointAddress final {
    FeatureId producer_feature_id;
    // Current PM-02C supported Point meaning is an intersection of exactly
    // three distinct semantic Surfaces, kept in canonical semantic order.
    std::vector<FeatureSurfaceAddress>
        adjacent_surfaces;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeaturePointAddress&,
        const FeaturePointAddress&) = default;
    friend auto operator<=>(
        const FeaturePointAddress&,
        const FeaturePointAddress&) = default;
};

struct FeaturePointResolution final {
    FeaturePointAddress address;
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_vertex_count{};
    std::vector<kernel::RuntimeVertexToken>
        current_vertices;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeaturePointResolution&,
        const FeaturePointResolution&) = default;
};

struct BodyFaceTopologyRecord final {
    kernel::RuntimeFaceToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    // Strict bounded Face meaning is present only when that semantic Face is
    // singularly Resolved at this stage.
    std::optional<FeatureFaceAddress>
        semantic_address;
    // Carrier candidates are distinct from strict Face identity. A single
    // Face normally maps to one semantic Surface; >1 is an explicit
    // Ambiguous carrier situation, never provider-order winner selection.
    std::vector<FeatureSurfaceAddress>
        surface_candidates;
    // Runtime-only Feature contribution membership. This is deliberately
    // separate from semantic Surface ownership so an Add may continue an
    // inherited carrier without losing truthful Current Feature Contribution.
    std::vector<FeatureId>
        contributing_features;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyFaceTopologyRecord&,
        const BodyFaceTopologyRecord&) = default;
};

struct BodyEdgeTopologyRecord final {
    kernel::RuntimeEdgeToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    kernel::ReferenceStatus referenceability{
        kernel::ReferenceStatus::unsupported};
    kernel::CurveKind curve_kind{
        kernel::CurveKind::other};
    bool periodic_seam{false};
    bool representation_partition{false};
    std::vector<FeatureCurveAddress>
        curve_candidates;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyEdgeTopologyRecord&,
        const BodyEdgeTopologyRecord&) = default;
};

struct BodyVertexTopologyRecord final {
    kernel::RuntimeVertexToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    kernel::ReferenceStatus referenceability{
        kernel::ReferenceStatus::unsupported};
    std::vector<FeaturePointAddress>
        point_candidates;
    std::size_t incident_material_edge_count{};
    // Current provider XYZ is diagnostic only, never identity.
    std::optional<kernel::Point3>
        provider_point;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyVertexTopologyRecord&,
        const BodyVertexTopologyRecord&) = default;
};

struct BodyStageTopologyCatalog final {
    BodyStageRef stage;
    std::vector<BodyFaceTopologyRecord> faces;
    std::vector<BodyEdgeTopologyRecord> edges;
    std::vector<BodyVertexTopologyRecord> vertices;
    std::vector<FeatureSurfaceResolution>
        surfaces;
    std::vector<FeatureCurveResolution>
        curves;
    std::vector<FeaturePointResolution>
        points;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool complete() const noexcept;

    friend bool operator==(
        const BodyStageTopologyCatalog&,
        const BodyStageTopologyCatalog&) = default;
};

struct FeatureContribution final {
    // Set-valued current contribution query. These runtime tokens identify
    // current Body realizations only; no member is persisted.
    std::vector<kernel::RuntimeFaceToken> faces;
    std::vector<kernel::RuntimeEdgeToken> direct_edges;
    std::vector<kernel::RuntimeVertexToken> direct_vertices;
    // Presentation envelope around carrier contribution. Boundary members are
    // not re-owned by the Feature merely because they bound its Faces.
    std::vector<kernel::RuntimeEdgeToken> boundary_edges;
    std::vector<kernel::RuntimeVertexToken> boundary_vertices;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureContribution&,
        const FeatureContribution&) = default;
};

struct FeatureEvaluation final {
    FeatureId feature_id;
    FeatureEvaluationStatus status{
        FeatureEvaluationStatus::blocked};
    FeatureEvaluationDiagnosticCode diagnostic{
        FeatureEvaluationDiagnosticCode::none};
    std::optional<kernel::SolidModelingStatus>
        kernel_status;
    std::vector<FeatureFaceResolution>
        produced_faces;
    std::vector<FeatureSurfaceResolution>
        produced_surfaces;
    std::vector<FeatureCurveResolution>
        produced_curves;
    std::vector<FeaturePointResolution>
        produced_points;
    // Same-revision runtime result for this exact successful Feature stage.
    // Retained only so stage-scoped topology tokens remain resolvable during
    // the current evaluation; never serialized or treated as final Body truth.
    kernel::RuntimeSolidHandle result_solid;
    // Complete runtime-only topology for this successful Feature stage.
    // Empty for Failed/Blocked/Suppressed Features. Never serialized.
    std::optional<BodyStageTopologyCatalog>
        result_topology;
};

struct PartEvaluation final {
    core::DocumentRevision source_revision;
    BodyEvaluationStatus body_status{
        BodyEvaluationStatus::empty};
    // Final current Body truth. Null whenever body_status is unavailable.
    kernel::RuntimeSolidHandle body_solid;
    // Runtime-only current-revision result immediately before the first
    // Failed/Blocked active Feature. Presentation may use this prefix while
    // final Body truth remains unavailable. Never persisted, published as
    // semantic identity, or consumed by downstream evaluation.
    kernel::RuntimeSolidHandle resolved_prefix_solid;
    std::vector<FeatureEvaluation> features;
    // Complete final Body-stage topology truth. Empty when Body is unavailable
    // or Empty. This is derived runtime state and is never serialized.
    std::optional<BodyStageTopologyCatalog>
        current_topology;
    // Same-revision topology immediately before the first Failed/Blocked
    // active Feature. Diagnostic/presentation only, never final Body truth.
    std::optional<BodyStageTopologyCatalog>
        resolved_prefix_topology;
    // Semantic references resolved at the current final Body stage only.
    // Empty when Body is unavailable.
    std::vector<FeatureFaceResolution>
        current_face_references;
    // Semantic Surface carriers resolved at the current final Body stage.
    // Empty when Body is unavailable.
    std::vector<FeatureSurfaceResolution>
        current_surface_references;
    std::vector<FeatureCurveResolution>
        current_curve_references;
    std::vector<FeaturePointResolution>
        current_point_references;

    [[nodiscard]] const FeatureEvaluation*
    findFeature(FeatureId id) const noexcept;
};

// Builds the exact provider-neutral modeling input for one authored
// Extrude definition. This is transient derived data shared by evaluation
// and runtime preview; it is never persisted.
[[nodiscard]] std::optional<
    kernel::LinearExtrudeInput>
makeKernelExtrudeInput(
    const PartDocument& document,
    const ExtrudeFeature& feature,
    const BodyStageTopologyCatalog*
        support_topology = nullptr,
    const DatumEvaluation*
        datum_evaluation = nullptr);

[[nodiscard]] FeatureContribution currentFeatureContribution(
    const BodyStageTopologyCatalog& catalog,
    FeatureId feature_id);

[[nodiscard]] PartEvaluation evaluatePart(
    const PartDocument& document,
    kernel::ISolidModelingKernel& modeling_kernel);

} // namespace simplesolid2::part
