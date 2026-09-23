#include <simplesolid2/viewer/navigation.hpp>

#include <cmath>

namespace simplesolid2::viewer {
namespace {

double distance(
    const Point3& a,
    const Point3& b) noexcept {
    const auto delta = a - b;
    return std::sqrt(delta.squaredLength());
}

} // namespace

ViewOrientation standardViewOrientation(
    StandardView view) noexcept {
    switch (view) {
    case StandardView::front:
        return {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::back:
        return {{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::left:
        return {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::right:
        return {{-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::top:
        return {{0.0, 0.0, -1.0}, {0.0, 1.0, 0.0}};
    case StandardView::bottom:
        return {{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
    case StandardView::isometric:
    case StandardView::top_front_right:
        return {{-1.0, 1.0, -1.0}, {0.0, 0.0, 1.0}};
    case StandardView::top_front_left:
        return {{1.0, 1.0, -1.0}, {0.0, 0.0, 1.0}};
    case StandardView::top_back_left:
        return {{1.0, -1.0, -1.0}, {0.0, 0.0, 1.0}};
    case StandardView::top_back_right:
        return {{-1.0, -1.0, -1.0}, {0.0, 0.0, 1.0}};
    case StandardView::bottom_front_left:
        return {{1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}};
    case StandardView::bottom_front_right:
        return {{-1.0, 1.0, 1.0}, {0.0, 0.0, 1.0}};
    case StandardView::bottom_back_left:
        return {{1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
    case StandardView::bottom_back_right:
        return {{-1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
    }

    return {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
}

std::optional<CameraState> cameraForStandardView(
    const CameraState& current,
    StandardView view) noexcept {
    if (!validateCameraState(current).valid) {
        return std::nullopt;
    }

    const auto orientation =
        standardViewOrientation(view);
    const auto direction =
        normalized(orientation.view_direction);
    if (!direction) {
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
    result.up = orientation.up;

    if (!validateCameraState(result).valid) {
        return std::nullopt;
    }

    return result;
}

std::optional<CameraState> cameraWithProjection(
    const CameraState& current,
    CameraProjection projection) noexcept {
    if (!validateCameraState(current).valid) {
        return std::nullopt;
    }

    auto result = current;
    result.projection = projection;
    return result;
}

} // namespace simplesolid2::viewer
