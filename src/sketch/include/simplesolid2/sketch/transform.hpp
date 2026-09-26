#pragma once

#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::sketch {

struct SketchTransformGeometry final {
    std::vector<SketchLineState> lines;
    std::vector<SketchCircleState> circles;
    std::vector<SketchArcState> arcs;

    [[nodiscard]] bool empty() const noexcept {
        return lines.empty() &&
               circles.empty() &&
               arcs.empty();
    }

    friend bool operator==(
        const SketchTransformGeometry&,
        const SketchTransformGeometry&) = default;
};

[[nodiscard]] std::optional<SketchTransformGeometry>
captureSketchTransformGeometry(
    const SketchModel& model,
    const std::vector<EntityId>& entity_ids);

[[nodiscard]] std::optional<SketchTransformGeometry>
translateSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 delta);

} // namespace simplesolid2::sketch
