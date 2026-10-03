#pragma once

#include <simplesolid2/viewer/math3.hpp>

#include <algorithm>
#include <vector>

namespace simplesolid2::viewer {

struct SolidTrianglePresentation final {
    Point3 first;
    Point3 second;
    Point3 third;
    Vec3 normal;

    [[nodiscard]] bool valid() const noexcept {
        return finite(first) &&
               finite(second) &&
               finite(third) &&
               finite(normal) &&
               cross(
                   second - first,
                   third - first)
                       .squaredLength() >
                   1.0e-24 &&
               normal.squaredLength() >
                   1.0e-24;
    }

    friend bool operator==(
        const SolidTrianglePresentation&,
        const SolidTrianglePresentation&) = default;
};

struct SolidScene final {
    std::vector<SolidTrianglePresentation>
        triangles;

    [[nodiscard]] bool valid() const noexcept {
        return std::all_of(
            triangles.begin(),
            triangles.end(),
            [](const SolidTrianglePresentation&
                   triangle) {
                return triangle.valid();
            });
    }

    [[nodiscard]] bool empty() const noexcept {
        return triangles.empty();
    }

    friend bool operator==(
        const SolidScene&,
        const SolidScene&) = default;
};

} // namespace simplesolid2::viewer
