#pragma once

#include <simplesolid2/viewer/math3.hpp>

#include <algorithm>
#include <vector>

namespace simplesolid2::viewer {

struct SolidTrianglePresentation final {
    Point3 first;
    Point3 second;
    Point3 third;
    Vec3 first_normal;
    Vec3 second_normal;
    Vec3 third_normal;

    [[nodiscard]] bool valid() const noexcept {
        return finite(first) &&
               finite(second) &&
               finite(third) &&
               finite(first_normal) &&
               finite(second_normal) &&
               finite(third_normal) &&
               cross(
                   second - first,
                   third - first)
                       .squaredLength() >
                   1.0e-24 &&
               first_normal.squaredLength() >
                   1.0e-24 &&
               second_normal.squaredLength() >
                   1.0e-24 &&
               third_normal.squaredLength() >
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

enum class SolidPreviewTone {
    additive,
    subtractive,
};

struct SolidPreviewScene final {
    std::vector<SolidTrianglePresentation>
        triangles;
    SolidPreviewTone tone{
        SolidPreviewTone::additive};

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
        const SolidPreviewScene&,
        const SolidPreviewScene&) = default;
};

} // namespace simplesolid2::viewer
