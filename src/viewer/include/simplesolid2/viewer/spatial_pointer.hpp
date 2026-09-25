#pragma once

#include <simplesolid2/viewer/math3.hpp>

#include <cmath>
#include <cstdint>
#include <functional>

namespace simplesolid2::viewer {

struct ViewportPoint2 final {
    double x{};
    double y{};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(x) &&
               std::isfinite(y);
    }

    friend bool operator==(
        const ViewportPoint2&,
        const ViewportPoint2&) = default;
};

struct Ray3 final {
    Point3 origin{};
    Vec3 direction{};

    [[nodiscard]] bool valid() const noexcept {
        if (!finite(origin) ||
            !finite(direction)) {
            return false;
        }

        const auto length_squared =
            direction.squaredLength();

        return std::isfinite(length_squared) &&
               length_squared > 0.0;
    }
};

enum class SpatialPointerPhase : std::uint8_t {
    move,
    primary_press,
    primary_release,
};

struct SpatialPointerEvent final {
    SpatialPointerPhase phase{
        SpatialPointerPhase::move};
    ViewportPoint2 position;
    Ray3 ray;

    [[nodiscard]] bool valid() const noexcept {
        return position.valid() &&
               ray.valid();
    }
};

using SpatialPointerHandler =
    std::function<void(const SpatialPointerEvent&)>;

enum class PrimaryPointerRouting : std::uint8_t {
    presentation_selection,
    spatial_tool_input,
};

enum class ViewportCursorMode : std::uint8_t {
    system_default,
    select_pick_box,
    create_edit_crosshair,
};

} // namespace simplesolid2::viewer
