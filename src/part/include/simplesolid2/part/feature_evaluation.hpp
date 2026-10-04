#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/reference_status.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::part {

class PartDocument;

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

enum class BodyStageKind {
    empty_body,
    after_feature,
};

struct BodyStageRef final {
    BodyStageKind kind{BodyStageKind::empty_body};
    std::optional<FeatureId> feature_id;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyStageRef&,
        const BodyStageRef&) = default;
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

struct BodyFaceTopologyRecord final {
    kernel::RuntimeFaceToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    std::optional<FeatureFaceAddress>
        semantic_address;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyFaceTopologyRecord&,
        const BodyFaceTopologyRecord&) = default;
};

struct BodyEdgeTopologyRecord final {
    kernel::RuntimeEdgeToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyEdgeTopologyRecord&,
        const BodyEdgeTopologyRecord&) = default;
};

struct BodyVertexTopologyRecord final {
    kernel::RuntimeVertexToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};

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

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool complete() const noexcept;

    friend bool operator==(
        const BodyStageTopologyCatalog&,
        const BodyStageTopologyCatalog&) = default;
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
    const ExtrudeFeature& feature);

[[nodiscard]] PartEvaluation evaluatePart(
    const PartDocument& document,
    kernel::ISolidModelingKernel& modeling_kernel);

} // namespace simplesolid2::part
