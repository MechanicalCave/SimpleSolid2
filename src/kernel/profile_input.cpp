#include <simplesolid2/kernel/profile_input.hpp>

#include <cmath>
#include <type_traits>

namespace simplesolid2::kernel {
namespace {

[[nodiscard]] bool finite(double value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finite(const Point2& value) noexcept {
    return finite(value.u) && finite(value.v);
}

[[nodiscard]] bool finite(const Point3& value) noexcept {
    return finite(value.x) && finite(value.y) && finite(value.z);
}

[[nodiscard]] Point3 cross(
    const Point3& first,
    const Point3& second) noexcept {
    return {
        first.y * second.z - first.z * second.y,
        first.z * second.x - first.x * second.z,
        first.x * second.y - first.y * second.x,
    };
}

[[nodiscard]] double dot(
    const Point3& first,
    const Point3& second) noexcept {
    return first.x * second.x +
           first.y * second.y +
           first.z * second.z;
}

[[nodiscard]] double squaredLength(
    const Point3& value) noexcept {
    return dot(value, value);
}

[[nodiscard]] bool parameter(double value) noexcept {
    return finite(value) && value >= 0.0 && value <= 1.0;
}

[[nodiscard]] bool curveValid(const Curve2& curve) noexcept {
    return std::visit(
        [](const auto& value) noexcept {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, Line2>) {
                return finite(value.start) &&
                       finite(value.end) &&
                       value.start != value.end;
            } else if constexpr (std::is_same_v<T, Circle2>) {
                return finite(value.center) &&
                       finite(value.radius) &&
                       value.radius > 0.0;
            } else {
                return finite(value.center) &&
                       finite(value.radius) &&
                       finite(value.start_angle) &&
                       finite(value.sweep_angle) &&
                       value.radius > 0.0 &&
                       value.sweep_angle != 0.0;
            }
        },
        curve);
}

} // namespace

bool Frame3::valid() const noexcept {
    if (!finite(origin) ||
        !finite(u_axis) ||
        !finite(v_axis) ||
        !finite(normal)) {
        return false;
    }

    const auto uxv = cross(u_axis, v_axis);
    return squaredLength(u_axis) > 0.0 &&
           squaredLength(v_axis) > 0.0 &&
           squaredLength(normal) > 0.0 &&
           squaredLength(uxv) > 0.0 &&
           dot(uxv, normal) > 0.0;
}

bool BoundaryUse2D::valid() const noexcept {
    if (!curveValid(curve) ||
        provenance.source_entity.empty()) {
        return false;
    }

    if (whole_closed_curve) {
        return std::holds_alternative<Circle2>(curve) &&
               !crosses_closed_seam;
    }

    return parameter(start_parameter) &&
           parameter(end_parameter) &&
           start_parameter != end_parameter;
}

bool ProfileLoopInput::valid() const noexcept {
    if (boundary.empty()) {
        return false;
    }

    for (const auto& use : boundary) {
        if (!use.valid()) {
            return false;
        }
    }

    if (boundary.size() == 1U) {
        return boundary.front().whole_closed_curve;
    }

    for (const auto& use : boundary) {
        if (use.whole_closed_curve) {
            return false;
        }
    }
    return true;
}

bool PlanarProfileInput::valid() const noexcept {
    if (!frame.valid() || !outer.valid()) {
        return false;
    }
    for (const auto& hole : holes) {
        if (!hole.valid()) {
            return false;
        }
    }
    return true;
}

} // namespace simplesolid2::kernel
