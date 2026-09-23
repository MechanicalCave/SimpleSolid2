#pragma once
namespace simplesolid2::viewer_qt_occt::detail {
struct OrbitScreenAngles final { double x{}; double y{}; double z{}; };
[[nodiscard]] OrbitScreenAngles orbitScreenAnglesFromMouseDelta(int dx,int dy,double radians_per_pixel) noexcept;
[[nodiscard]] double wheelZoomDragFraction(int angle_delta_y) noexcept;
}
