#pragma once
#include <simplesolid2/viewer/math3.hpp>
#include <string>
namespace simplesolid2::viewer {
enum class CameraProjection { perspective, orthographic };
struct CameraState final { Point3 eye{8,-8,8}; Point3 target{0,0,0}; Vec3 up{0,0,1}; CameraProjection projection{CameraProjection::orthographic}; double scale{100.0}; friend bool operator==(const CameraState&,const CameraState&)=default; };
struct CameraValidationResult final { bool valid{}; std::string diagnostic; };
[[nodiscard]] CameraValidationResult validateCameraState(const CameraState& state) noexcept;
}
