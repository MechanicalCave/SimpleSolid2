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

    // BRepFilletAPI contour inspection after explicit Add() calls and before
    // Build(). This is evidence for/against silent tangent-chain expansion.
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

} // namespace simplesolid2::kernel
