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
};

enum class BodyEvaluationStatus {
    empty,
    up_to_date,
    unavailable,
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
};

struct PartEvaluation final {
    core::DocumentRevision source_revision;
    BodyEvaluationStatus body_status{
        BodyEvaluationStatus::empty};
    kernel::RuntimeSolidHandle body_solid;
    std::vector<FeatureEvaluation> features;
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
