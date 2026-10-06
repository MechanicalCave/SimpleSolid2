#pragma once

#include <simplesolid2/kernel/evidence.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace simplesolid2::kernel {

// PM-05A evidence-only vocabulary. These records describe provider behavior
// observed on synthetic solids. They are not production Feature inputs,
// durable CAD identity, or persistence schema.
enum class EdgeFeatureEvidenceOperation {
    fillet,
    chamfer,
};

enum class EdgeFeatureProbeScenario {
    single_edge,
    disconnected_pair,
    adjacent_pair,
    trihedral_corner,
    closed_loop,
    mixed_connected_disconnected,
    excessive_parameter,
};

enum class EdgeFeatureUpstreamScenario {
    dimension_change,
    unchanged,
    trim,
    split,
    remove,
};

struct EdgeFeatureTopologySignature final {
    std::size_t face_count{};
    std::size_t edge_count{};
    std::size_t vertex_count{};
    double volume{};

    friend bool operator==(
        const EdgeFeatureTopologySignature&,
        const EdgeFeatureTopologySignature&) = default;
};

struct EdgeFeatureProviderEvidence final {
    EdgeFeatureEvidenceOperation operation{
        EdgeFeatureEvidenceOperation::fillet};
    EdgeFeatureProbeScenario scenario{
        EdgeFeatureProbeScenario::single_edge};
    double parameter{};

    ShapeEvidence source_shape;
    std::size_t requested_edge_count{};
    std::size_t resolved_input_edge_count{};

    // Native-provider contour inspection after explicit Edge registration
    // and before execution. Evidence for/against silent tangent-chain expansion.
    std::size_t provider_contour_count{};
    std::size_t provider_contour_edge_count{};
    bool exact_provider_input_membership{false};

    bool build_attempted{false};
    bool build_succeeded{false};
    std::optional<ShapeEvidence> result_shape;
    std::optional<EdgeFeatureTopologySignature>
        result_signature;

    // Repeat with the same explicit semantic set inserted in reverse order.
    // The production authored set will be canonicalized independently of
    // provider traversal order; this probe tells us whether provider output
    // invariants are insertion-order-sensitive.
    bool reverse_build_succeeded{false};
    bool reverse_same_topology_and_volume{false};

    // Provider history/provenance observations for successful results.
    std::size_t new_face_count{};
    std::size_t modified_inherited_face_count{};
    std::size_t generated_from_selected_edges_face_count{};
    std::size_t generated_from_shared_vertices_face_count{};
    std::size_t unclaimed_new_face_count{};
    std::vector<std::size_t>
        generated_faces_per_selected_edge;

    [[nodiscard]] bool sourceReady() const noexcept {
        return source_shape.ok() &&
               source_shape.solid_count == 1U &&
               requested_edge_count > 0U &&
               resolved_input_edge_count ==
                   requested_edge_count;
    }

    friend bool operator==(
        const EdgeFeatureProviderEvidence&,
        const EdgeFeatureProviderEvidence&) = default;
};

struct EdgeFeatureProviderMatrixEvidence final {
    std::vector<EdgeFeatureProviderEvidence> probes;

    [[nodiscard]] bool complete() const noexcept {
        return probes.size() == 14U;
    }

    friend bool operator==(
        const EdgeFeatureProviderMatrixEvidence&,
        const EdgeFeatureProviderMatrixEvidence&) = default;
};

struct EdgeFeatureTangentChainEvidence final {
    EdgeFeatureEvidenceOperation operation{
        EdgeFeatureEvidenceOperation::fillet};
    ShapeEvidence source_shape;
    std::size_t requested_edge_count{};
    std::size_t provider_contour_count{};
    std::size_t provider_contour_edge_count{};
    bool exact_provider_input_membership{false};
    bool build_succeeded{false};

    // Re-run after explicitly authoring every Edge the provider placed in
    // the tangent contour discovered from the one-Edge request.
    std::size_t explicit_contour_requested_edge_count{};
    std::size_t explicit_contour_provider_edge_count{};
    bool explicit_contour_exact_membership{false};
    bool explicit_contour_build_succeeded{false};

    // Same geometric tangent chain with both bounded Edges explicitly
    // registered. This proves whether exact-input enforcement can accept the
    // full authored contour while rejecting a silently grown partial set.
    std::size_t full_chain_requested_edge_count{};
    std::size_t full_chain_provider_contour_edge_count{};
    bool full_chain_exact_provider_input_membership{false};
    bool full_chain_build_succeeded{false};
    bool single_and_full_same_topology_and_volume{false};

    friend bool operator==(
        const EdgeFeatureTangentChainEvidence&,
        const EdgeFeatureTangentChainEvidence&) = default;
};

struct EdgeFeatureUpstreamEvidence final {
    EdgeFeatureEvidenceOperation operation{
        EdgeFeatureEvidenceOperation::fillet};
    EdgeFeatureUpstreamScenario scenario{
        EdgeFeatureUpstreamScenario::unchanged};
    ShapeEvidence source_shape;
    ShapeEvidence edited_shape;
    ReferenceStatus reference_status{
        ReferenceStatus::unsupported};
    std::size_t current_edge_candidate_count{};
    bool downstream_attempted{false};
    bool downstream_succeeded{false};
    bool exact_provider_input_membership{false};

    friend bool operator==(
        const EdgeFeatureUpstreamEvidence&,
        const EdgeFeatureUpstreamEvidence&) = default;
};

struct EdgeFeatureChainingEvidence final {
    EdgeFeatureEvidenceOperation first_operation{
        EdgeFeatureEvidenceOperation::fillet};
    EdgeFeatureEvidenceOperation second_operation{
        EdgeFeatureEvidenceOperation::chamfer};
    ShapeEvidence source_shape;
    ShapeEvidence first_result_shape;
    std::size_t first_generated_face_count{};
    std::size_t generated_boundary_edge_count{};
    std::size_t second_operation_attempt_count{};
    std::size_t second_operation_success_count{};

    [[nodiscard]] bool chainable() const noexcept {
        return source_shape.ok() &&
               first_result_shape.ok() &&
               first_generated_face_count > 0U &&
               generated_boundary_edge_count > 0U &&
               second_operation_success_count > 0U;
    }

    friend bool operator==(
        const EdgeFeatureChainingEvidence&,
        const EdgeFeatureChainingEvidence&) = default;
};

struct EdgeFeatureLifecycleEvidence final {
    std::vector<EdgeFeatureTangentChainEvidence>
        tangent_chain;
    std::vector<EdgeFeatureUpstreamEvidence>
        upstream;
    std::vector<EdgeFeatureChainingEvidence>
        chaining;

    [[nodiscard]] bool complete() const noexcept {
        return tangent_chain.size() == 2U &&
               upstream.size() == 10U &&
               chaining.size() == 2U;
    }

    friend bool operator==(
        const EdgeFeatureLifecycleEvidence&,
        const EdgeFeatureLifecycleEvidence&) = default;
};

} // namespace simplesolid2::kernel
