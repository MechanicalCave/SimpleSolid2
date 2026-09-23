#pragma once
#include <cmath>
#include <optional>
namespace simplesolid2::viewer {
struct Vec3 final { double x{}; double y{}; double z{}; [[nodiscard]] constexpr double squaredLength() const noexcept { return x*x+y*y+z*z; } friend constexpr bool operator==(const Vec3&, const Vec3&) = default; };
struct Point3 final { double x{}; double y{}; double z{}; friend constexpr bool operator==(const Point3&, const Point3&) = default; };
[[nodiscard]] constexpr Vec3 operator-(const Point3&a,const Point3&b) noexcept { return {a.x-b.x,a.y-b.y,a.z-b.z}; }
[[nodiscard]] constexpr Point3 operator+(const Point3&p,const Vec3&v) noexcept { return {p.x+v.x,p.y+v.y,p.z+v.z}; }
[[nodiscard]] constexpr Point3 operator-(const Point3&p,const Vec3&v) noexcept { return {p.x-v.x,p.y-v.y,p.z-v.z}; }
[[nodiscard]] constexpr Vec3 operator*(const Vec3&v,double s) noexcept { return {v.x*s,v.y*s,v.z*s}; }
[[nodiscard]] constexpr double dot(const Vec3&a,const Vec3&b) noexcept { return a.x*b.x+a.y*b.y+a.z*b.z; }
[[nodiscard]] constexpr Vec3 cross(const Vec3&a,const Vec3&b) noexcept { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
[[nodiscard]] inline bool finite(const Vec3&v) noexcept { return std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z); }
[[nodiscard]] inline bool finite(const Point3&p) noexcept { return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z); }
[[nodiscard]] inline std::optional<Vec3> normalized(const Vec3&v) noexcept { if(!finite(v)) return std::nullopt; const auto l2=v.squaredLength(); if(!std::isfinite(l2)||l2<=1.0e-24) return std::nullopt; return v*(1.0/std::sqrt(l2)); }
struct Transform3 final { Vec3 x_axis{1,0,0}; Vec3 y_axis{0,1,0}; Vec3 z_axis{0,0,1}; Point3 translation{}; [[nodiscard]] static constexpr Transform3 identity() noexcept { return {}; } friend constexpr bool operator==(const Transform3&,const Transform3&)=default; };
}
