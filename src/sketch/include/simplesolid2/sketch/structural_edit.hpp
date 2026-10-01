#pragma once

#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::sketch {

enum class StructuralEditStatus {
    ready,
    invalid_request,
    missing_entity,
    unsupported,
    no_intersection,
    ambiguous_topology,
    not_applicable,
    identity_exhausted,
};

enum class StructuralEndpointRole {
    start,
    end,
};

struct TrimSketchRequest final {
    EntityId target;
    std::vector<EntityId> boundaries;
    Point2 pick;
};

struct ExtendSketchRequest final {
    EntityId target;
    std::vector<EntityId> boundaries;
    StructuralEndpointRole endpoint{
        StructuralEndpointRole::end};
};

struct ExtendBothLinesRequest final {
    EntityId first_line;
    EntityId second_line;
};

struct StructuralEditResult final {
    StructuralEditStatus status{
        StructuralEditStatus::invalid_request};
    std::optional<SketchModelState> state;
    std::optional<EntityId> result_entity;

    [[nodiscard]] bool ready() const noexcept {
        return status == StructuralEditStatus::ready &&
               state.has_value();
    }
};

// Pure provider-neutral structural-edit evaluation. The input model is never
// mutated. A ready result contains the complete resulting Sketch authored
// state so the application layer can revalidate and commit it through the
// owning Part transaction.
[[nodiscard]] StructuralEditResult evaluateTrim(
    const SketchModel& model,
    const TrimSketchRequest& request);

[[nodiscard]] StructuralEditResult evaluateExtend(
    const SketchModel& model,
    const ExtendSketchRequest& request);

[[nodiscard]] StructuralEditResult evaluateExtendBoth(
    const SketchModel& model,
    const ExtendBothLinesRequest& request);

} // namespace simplesolid2::sketch
