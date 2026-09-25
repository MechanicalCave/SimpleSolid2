#pragma once

#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/viewer/spatial_pointer.hpp>

#include <cmath>
#include <optional>

namespace simplesolid2::ui::detail {

struct SketchFrame3 final {
    viewer::Point3 origin;
    viewer::Vec3 u_axis;
    viewer::Vec3 v_axis;
    viewer::Vec3 normal;
};

[[nodiscard]] inline std::optional<SketchFrame3>
validatedSketchFrame(
    const part::SketchPlacement& placement) noexcept {
    if (!placement.valid()) {
        return std::nullopt;
    }

    const viewer::Point3 origin{
        placement.origin[0],
        placement.origin[1],
        placement.origin[2]};
    const viewer::Vec3 u_axis{
        placement.u_axis[0],
        placement.u_axis[1],
        placement.u_axis[2]};
    const viewer::Vec3 v_axis{
        placement.v_axis[0],
        placement.v_axis[1],
        placement.v_axis[2]};

    if (!viewer::finite(origin) ||
        !viewer::finite(u_axis) ||
        !viewer::finite(v_axis)) {
        return std::nullopt;
    }

    const auto u_length_squared =
        u_axis.squaredLength();
    const auto v_length_squared =
        v_axis.squaredLength();
    const auto uv_dot =
        viewer::dot(u_axis, v_axis);

    constexpr double metric_epsilon = 1.0e-12;
    if (!std::isfinite(u_length_squared) ||
        !std::isfinite(v_length_squared) ||
        !std::isfinite(uv_dot) ||
        std::abs(u_length_squared - 1.0) >
            metric_epsilon ||
        std::abs(v_length_squared - 1.0) >
            metric_epsilon ||
        std::abs(uv_dot) > metric_epsilon) {
        return std::nullopt;
    }

    const auto normal =
        viewer::cross(u_axis, v_axis);
    const auto normal_length_squared =
        normal.squaredLength();
    if (!viewer::finite(normal) ||
        !std::isfinite(normal_length_squared) ||
        std::abs(normal_length_squared - 1.0) >
            metric_epsilon) {
        return std::nullopt;
    }

    return SketchFrame3{
        origin,
        u_axis,
        v_axis,
        normal};
}

[[nodiscard]] inline std::optional<viewer::Point3>
sketchPointToWorld(
    const part::SketchPlacement& placement,
    const sketch::Point2& point) noexcept {
    if (!point.finite()) {
        return std::nullopt;
    }

    const auto frame =
        validatedSketchFrame(placement);
    if (!frame) {
        return std::nullopt;
    }

    const auto world =
        frame->origin +
        frame->u_axis * point.u +
        frame->v_axis * point.v;

    return viewer::finite(world)
        ? std::optional<viewer::Point3>{world}
        : std::nullopt;
}

[[nodiscard]] inline std::optional<sketch::Point2>
sketchPointFromRay(
    const part::SketchPlacement& placement,
    const viewer::Ray3& ray) noexcept {
    if (!ray.valid()) {
        return std::nullopt;
    }

    const auto frame =
        validatedSketchFrame(placement);
    if (!frame) {
        return std::nullopt;
    }

    const auto direction_length =
        std::sqrt(
            ray.direction.squaredLength());
    if (!std::isfinite(direction_length) ||
        direction_length <= 0.0) {
        return std::nullopt;
    }

    const auto denominator =
        viewer::dot(
            ray.direction,
            frame->normal);
    if (!std::isfinite(denominator)) {
        return std::nullopt;
    }

    constexpr double parallel_cosine_epsilon =
        1.0e-12;
    if (std::abs(denominator) <=
        parallel_cosine_epsilon *
            direction_length) {
        return std::nullopt;
    }

    const auto to_plane =
        frame->origin - ray.origin;
    const auto numerator =
        viewer::dot(
            to_plane,
            frame->normal);
    const auto parameter =
        numerator / denominator;

    if (!std::isfinite(parameter) ||
        parameter < 0.0) {
        return std::nullopt;
    }

    const auto world =
        ray.origin +
        ray.direction * parameter;
    if (!viewer::finite(world)) {
        return std::nullopt;
    }

    const auto delta =
        world - frame->origin;
    const sketch::Point2 result{
        viewer::dot(delta, frame->u_axis),
        viewer::dot(delta, frame->v_axis)};

    return result.finite()
        ? std::optional<sketch::Point2>{result}
        : std::nullopt;
}

} // namespace simplesolid2::ui::detail
