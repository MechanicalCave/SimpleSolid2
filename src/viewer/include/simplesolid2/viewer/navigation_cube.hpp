#pragma once

#include <simplesolid2/viewer/camera_state.hpp>

#include <functional>
#include <optional>

namespace simplesolid2::viewer {

enum class NavigationCubeTarget {
    front,
    back,
    left,
    right,
    top,
    bottom,

    top_front,
    top_back,
    top_left,
    top_right,
    bottom_front,
    bottom_back,
    bottom_left,
    bottom_right,
    front_left,
    front_right,
    back_left,
    back_right,

    top_front_left,
    top_front_right,
    top_back_left,
    top_back_right,
    bottom_front_left,
    bottom_front_right,
    bottom_back_left,
    bottom_back_right,
};

enum class NavigationCubeActionKind {
    orient,
    adjacent_left,
    adjacent_right,
    adjacent_up,
    adjacent_down,
    roll_clockwise,
    roll_counterclockwise,
    home,
    toggle_projection,
};

struct NavigationCubeAction final {
    NavigationCubeActionKind kind{
        NavigationCubeActionKind::orient};
    NavigationCubeTarget target{
        NavigationCubeTarget::front};

    [[nodiscard]] static constexpr NavigationCubeAction
    orientTo(NavigationCubeTarget value) noexcept {
        return {
            NavigationCubeActionKind::orient,
            value};
    }

    friend constexpr bool operator==(
        const NavigationCubeAction&,
        const NavigationCubeAction&) = default;
};

struct NavigationCubeNavigation final {
    CameraState camera;
    bool fit_all{};

    friend bool operator==(
        const NavigationCubeNavigation&,
        const NavigationCubeNavigation&) = default;
};

using NavigationCubeActionHandler =
    std::function<void(const NavigationCubeAction&)>;

[[nodiscard]] std::optional<CameraState>
cameraForNavigationCubeTarget(
    const CameraState& current,
    NavigationCubeTarget target) noexcept;

[[nodiscard]] std::optional<NavigationCubeNavigation>
navigationForCubeAction(
    const CameraState& current,
    const NavigationCubeAction& action) noexcept;

[[nodiscard]] bool navigationCubeFaceAligned(
    const CameraState& camera) noexcept;

} // namespace simplesolid2::viewer
