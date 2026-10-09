#pragma once

#include <simplesolid2/kernel/edge_projection.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_sketch.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::part {

// A source-controlled target that cannot be reprojected is deliberately
// removed only from the temporary evaluated Sketch, never from authored state.
enum class ProjectedSketchSourceStatus {
    resolved,
    missing_stage,
    ambiguous_source,
    unsupported_source,
    unsupported_projection,
    degenerate_projection,
    provider_failure,
    invalid_support,
    invalid_dependency,
    changed_curve_kind,
};

struct ProjectedSketchOutcome final {
    sketch::EntityId target_entity;
    ProjectedSketchSourceStatus status{
        ProjectedSketchSourceStatus::missing_stage};

    [[nodiscard]] bool resolved() const noexcept {
        return status == ProjectedSketchSourceStatus::resolved;
    }
};

struct EffectiveSketchProjection final {
    // A private, disposable snapshot. Original EntityId and role are kept.
    sketch::SketchModel model;
    std::vector<ProjectedSketchOutcome> outcomes;

    [[nodiscard]] bool allResolved() const noexcept {
        for (const auto& item : outcomes) {
            if (!item.resolved()) {
                return false;
            }
        }
        return true;
    }
};

// Pure projection: no PartDocument mutation, authored seed updates, runtime
// token persistence, global graph or implicit source rebinding. Caller supplies
// the exact same-revision evaluated Feature prefix. If consumer is present,
// sources must strictly precede the consuming Feature in authored order.
// B3 must consume this derived model instead of raw Sketch authored seeds.
[[nodiscard]] std::optional<EffectiveSketchProjection>
evaluateEffectiveSketchProjection(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    std::optional<FeatureId> consuming_feature = std::nullopt);

} // namespace simplesolid2::part
