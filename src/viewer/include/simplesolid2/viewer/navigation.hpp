#pragma once

#include <simplesolid2/viewer/camera_state.hpp>

#include <optional>

namespace simplesolid2::viewer {

enum class StandardView {
    front,
    back,
    left,
    right,
    top,
    bottom,
    isometric,
    top_front_left,
    top_front_right,
    top_back_left,
    top_back_right,
    bottom_front_left,
    bottom_front_right,
    bottom_back_left,
    bottom_back_right,
};

struct ViewOrientation final {
    Vec3 view_direction;
    Vec3 up;
};

[[nodiscard]] ViewOrientation standardViewOrientation(
    StandardView view) noexcept;

[[nodiscard]] std::optional<CameraState> cameraForStandardView(
    const CameraState& current,
    StandardView view) noexcept;

[[nodiscard]] std::optional<CameraState> cameraWithProjection(
    const CameraState& current,
    CameraProjection projection) noexcept;

} // namespace simplesolid2::viewer
