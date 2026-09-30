#pragma once

#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::sketch {

enum class SketchTransformEndpointRole {
    start,
    end,
};

struct SketchTransformLineArcContact final {
    EntityId line_id;
    SketchTransformEndpointRole line_endpoint{
        SketchTransformEndpointRole::start};
    EntityId arc_id;
    SketchTransformEndpointRole arc_endpoint{
        SketchTransformEndpointRole::start};

    friend bool operator==(
        const SketchTransformLineArcContact&,
        const SketchTransformLineArcContact&) = default;
};

struct SketchTransformGeometry final {
    std::vector<SketchLineState> lines;
    std::vector<SketchCircleState> circles;
    std::vector<SketchArcState> arcs;

    // Runtime transform provenance only. These contacts are inferred from
    // the accepted source geometry when a common transform begins. They are
    // not authored Sketch relations and are never persisted. Their sole
    // purpose is to prevent one affine transform from destroying an existing
    // exact Line<->Arc endpoint contact through independent double rounding.
    std::vector<SketchTransformLineArcContact>
        line_arc_contacts;

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

// Provider-independent semantic transform core shared by Sketch Modify commands;
// UI/provider adapters must not duplicate transform geometry outside this layer.
[[nodiscard]] std::optional<SketchTransformGeometry>
translateSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 delta);

[[nodiscard]] std::optional<SketchTransformGeometry>
rotateSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 base_point,
    double angle_radians);

[[nodiscard]] std::optional<SketchTransformGeometry>
scaleSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 base_point,
    double factor);

[[nodiscard]] std::optional<SketchTransformGeometry>
mirrorSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 axis_start,
    Point2 axis_end);

} // namespace simplesolid2::sketch
