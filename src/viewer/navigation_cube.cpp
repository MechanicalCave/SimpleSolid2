#include <simplesolid2/viewer/navigation_cube.hpp>
#include <simplesolid2/viewer/navigation.hpp>

#include <cmath>

namespace simplesolid2::viewer {
namespace {

[[nodiscard]] double distance(
    const Point3& a,
    const Point3& b) noexcept {
    const auto delta = a - b;
    return std::sqrt(delta.squaredLength());
}

[[nodiscard]] std::optional<Vec3> snappedAxis(
    const Vec3& value) noexcept {
    const auto unit = normalized(value);
    if (!unit) return std::nullopt;

    constexpr double aligned = 1.0 - 1.0e-8;
    constexpr double residual = 1.0e-8;

    if (std::abs(unit->x) >= aligned &&
        std::abs(unit->y) <= residual &&
        std::abs(unit->z) <= residual) {
        return Vec3{unit->x > 0.0 ? 1.0 : -1.0, 0.0, 0.0};
    }
    if (std::abs(unit->y) >= aligned &&
        std::abs(unit->x) <= residual &&
        std::abs(unit->z) <= residual) {
        return Vec3{0.0, unit->y > 0.0 ? 1.0 : -1.0, 0.0};
    }
    if (std::abs(unit->z) >= aligned &&
        std::abs(unit->x) <= residual &&
        std::abs(unit->y) <= residual) {
        return Vec3{0.0, 0.0, unit->z > 0.0 ? 1.0 : -1.0};
    }

    return std::nullopt;
}

[[nodiscard]] ViewOrientation targetOrientation(
    NavigationCubeTarget target) noexcept {
    switch (target) {
    case NavigationCubeTarget::front:
        return {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::back:
        return {{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::left:
        return {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::right:
        return {{-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top:
        return {{0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}};
    case NavigationCubeTarget::bottom:
        return {{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};

    case NavigationCubeTarget::top_front:
        return {{0.0, 1.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top_back:
        return {{0.0, -1.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top_left:
        return {{1.0, 0.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top_right:
        return {{-1.0, 0.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_front:
        return {{0.0, 1.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_back:
        return {{0.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_left:
        return {{1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_right:
        return {{-1.0, 0.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::front_left:
        return {{1.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::front_right:
        return {{-1.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::back_left:
        return {{1.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::back_right:
        return {{-1.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};

    case NavigationCubeTarget::top_front_left:
        return {{1.0, 1.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top_front_right:
        return {{-1.0, 1.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top_back_left:
        return {{1.0, -1.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::top_back_right:
        return {{-1.0, -1.0, -1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_front_left:
        return {{1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_front_right:
        return {{-1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_back_left:
        return {{1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
    case NavigationCubeTarget::bottom_back_right:
        return {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
    }

    return {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
}

[[nodiscard]] std::optional<CameraState>
cameraForOrientation(
    const CameraState& current,
    ViewOrientation orientation) noexcept {
    if (!validateCameraState(current).valid) {
        return std::nullopt;
    }

    const auto direction =
        normalized(orientation.view_direction);
    const auto up = normalized(orientation.up);
    if (!direction || !up ||
        cross(*direction, *up).squaredLength() <= 1.0e-24) {
        return std::nullopt;
    }

    const auto camera_distance =
        distance(current.eye, current.target);
    if (!std::isfinite(camera_distance) ||
        camera_distance <= 1.0e-12) {
        return std::nullopt;
    }

    auto result = current;
    result.eye =
        current.target -
        (*direction * camera_distance);
    result.up = *up;
    result.projection =
        CameraProjection::orthographic;

    if (!validateCameraState(result).valid) {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] std::optional<CameraState>
cameraForAdjacent(
    const CameraState& current,
    NavigationCubeActionKind kind) noexcept {
    if (!validateCameraState(current).valid) {
        return std::nullopt;
    }

    const auto direction =
        snappedAxis(current.target - current.eye);
    const auto up = snappedAxis(current.up);
    if (!direction || !up ||
        std::abs(dot(*direction, *up)) > 1.0e-12) {
        return std::nullopt;
    }

    const auto right =
        snappedAxis(cross(*direction, *up));
    if (!right) return std::nullopt;

    ViewOrientation next;
    switch (kind) {
    case NavigationCubeActionKind::adjacent_left:
        next = {*right, *up};
        break;
    case NavigationCubeActionKind::adjacent_right:
        next = {*right * -1.0, *up};
        break;
    case NavigationCubeActionKind::adjacent_up:
        next = {*up * -1.0, *direction};
        break;
    case NavigationCubeActionKind::adjacent_down:
        next = {*up, *direction};
        break;
    default:
        return std::nullopt;
    }

    return cameraForOrientation(current, next);
}

[[nodiscard]] std::optional<CameraState>
cameraForRoll(
    const CameraState& current,
    bool clockwise) noexcept {
    if (!validateCameraState(current).valid) {
        return std::nullopt;
    }

    const auto direction =
        snappedAxis(current.target - current.eye);
    const auto up = snappedAxis(current.up);
    if (!direction || !up ||
        std::abs(dot(*direction, *up)) > 1.0e-12) {
        return std::nullopt;
    }

    const auto right =
        snappedAxis(cross(*direction, *up));
    if (!right) return std::nullopt;

    const auto next_up =
        clockwise
            ? (*right * -1.0)
            : *right;

    return cameraForOrientation(
        current,
        {*direction, next_up});
}

} // namespace

std::optional<CameraState>
cameraForNavigationCubeTarget(
    const CameraState& current,
    NavigationCubeTarget target) noexcept {
    return cameraForOrientation(
        current,
        targetOrientation(target));
}

std::optional<NavigationCubeNavigation>
navigationForCubeAction(
    const CameraState& current,
    const NavigationCubeAction& action) noexcept {
    std::optional<CameraState> camera;
    bool fit_all = false;

    switch (action.kind) {
    case NavigationCubeActionKind::orient:
        camera =
            cameraForNavigationCubeTarget(
                current,
                action.target);
        break;

    case NavigationCubeActionKind::adjacent_left:
    case NavigationCubeActionKind::adjacent_right:
    case NavigationCubeActionKind::adjacent_up:
    case NavigationCubeActionKind::adjacent_down:
        camera =
            cameraForAdjacent(
                current,
                action.kind);
        break;

    case NavigationCubeActionKind::roll_clockwise:
        camera =
            cameraForRoll(
                current,
                true);
        break;

    case NavigationCubeActionKind::roll_counterclockwise:
        camera =
            cameraForRoll(
                current,
                false);
        break;

    case NavigationCubeActionKind::home:
        camera =
            cameraForNavigationCubeTarget(
                current,
                NavigationCubeTarget::
                    top_front_right);
        fit_all = true;
        break;
    }

    if (!camera) return std::nullopt;

    camera->projection =
        CameraProjection::orthographic;

    return NavigationCubeNavigation{
        *camera,
        fit_all};
}

bool navigationCubeFaceAligned(
    const CameraState& camera) noexcept {
    if (!validateCameraState(camera).valid) {
        return false;
    }

    const auto direction =
        snappedAxis(camera.target - camera.eye);
    const auto up = snappedAxis(camera.up);
    return direction &&
           up &&
           std::abs(dot(*direction, *up)) <= 1.0e-12;
}

} // namespace simplesolid2::viewer
