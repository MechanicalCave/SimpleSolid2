#include <simplesolid2/part/part_sketch.hpp>

#include <cmath>

namespace simplesolid2::part {
namespace {

bool finiteVector(
    const std::array<double, 3>& value) noexcept {
    return std::isfinite(value[0]) &&
           std::isfinite(value[1]) &&
           std::isfinite(value[2]);
}

double squaredLength(
    const std::array<double, 3>& value) noexcept {
    return value[0] * value[0] +
           value[1] * value[1] +
           value[2] * value[2];
}

std::array<double, 3> cross(
    const std::array<double, 3>& a,
    const std::array<double, 3>& b) noexcept {
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}

} // namespace

bool SketchPlacement::valid() const noexcept {
    return finiteVector(origin) &&
           finiteVector(u_axis) &&
           finiteVector(v_axis) &&
           squaredLength(u_axis) > 1.0e-24 &&
           squaredLength(v_axis) > 1.0e-24 &&
           squaredLength(cross(u_axis, v_axis)) > 1.0e-24;
}

bool PartSketchSupport::valid() const noexcept {
    return isSketchOriginPlane(builtin_plane);
}

bool isSketchOriginPlane(
    core::BuiltinReferenceRole role) noexcept {
    return role == core::BuiltinReferenceRole::xy_plane ||
           role == core::BuiltinReferenceRole::xz_plane ||
           role == core::BuiltinReferenceRole::yz_plane;
}

std::optional<PartSketchSupport>
partSketchSupportForBuiltinPlane(
    core::BuiltinReferenceRole role) noexcept {
    if (!isSketchOriginPlane(role)) {
        return std::nullopt;
    }

    return PartSketchSupport{role};
}

std::optional<SketchPlacement>
sketchPlacementForSupport(
    const PartSketchSupport& support) noexcept {
    if (!support.valid()) {
        return std::nullopt;
    }

    SketchPlacement placement;

    switch (support.builtin_plane) {
    case core::BuiltinReferenceRole::xy_plane:
        placement.u_axis = {1.0, 0.0, 0.0};
        placement.v_axis = {0.0, 1.0, 0.0};
        return placement;

    case core::BuiltinReferenceRole::xz_plane:
        placement.u_axis = {1.0, 0.0, 0.0};
        placement.v_axis = {0.0, 0.0, 1.0};
        return placement;

    case core::BuiltinReferenceRole::yz_plane:
        placement.u_axis = {0.0, 1.0, 0.0};
        placement.v_axis = {0.0, 0.0, 1.0};
        return placement;

    default:
        return std::nullopt;
    }
}

bool sketchPlacementMatchesSupport(
    const SketchPlacement& placement,
    const PartSketchSupport& support) noexcept {
    const auto expected =
        sketchPlacementForSupport(support);

    return expected.has_value() &&
           placement.valid() &&
           placement == *expected;
}

} // namespace simplesolid2::part
