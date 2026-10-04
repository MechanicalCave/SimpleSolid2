#pragma once

#include <simplesolid2/viewer/math3.hpp>
#include <simplesolid2/viewer/reference_presentation.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
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

// PM-02D committed Body presentation authority. A non-empty scene is always
// generation-scoped. The generation is presentation runtime state only and is
// never authored/persisted.
struct BodyPresentationGeneration final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend bool operator==(
        const BodyPresentationGeneration&,
        const BodyPresentationGeneration&) = default;
};

enum class BodyScenePurpose : std::uint8_t {
    current_body,
    diagnostic_prefix,
};

enum class BodyTopologyPresentationKind : std::uint8_t {
    face,
    edge,
    vertex,
};

struct BodyFacePresentation final {
    PresentationToken token;
    std::size_t first_triangle{};
    std::size_t triangle_count{};

    [[nodiscard]] bool valid(
        std::size_t total_triangles) const noexcept {
        return token.valid() &&
               triangle_count > 0U &&
               first_triangle < total_triangles &&
               triangle_count <=
                   total_triangles - first_triangle;
    }

    friend bool operator==(
        const BodyFacePresentation&,
        const BodyFacePresentation&) = default;
};

struct BodyEdgePresentation final {
    PresentationToken token;
    std::vector<Point3> points;

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() &&
               points.size() >= 2U &&
               std::all_of(
                   points.begin(),
                   points.end(),
                   [](const Point3& point) {
                       return finite(point);
                   });
    }

    friend bool operator==(
        const BodyEdgePresentation&,
        const BodyEdgePresentation&) = default;
};

struct BodyVertexPresentation final {
    PresentationToken token;
    Point3 point;

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() &&
               finite(point);
    }

    friend bool operator==(
        const BodyVertexPresentation&,
        const BodyVertexPresentation&) = default;
};

struct BodyScene final {
    BodyPresentationGeneration generation;
    BodyScenePurpose purpose{
        BodyScenePurpose::current_body};
    std::vector<SolidTrianglePresentation>
        triangles;
    std::vector<BodyFacePresentation> faces;
    std::vector<BodyEdgePresentation> edges;
    std::vector<BodyVertexPresentation> vertices;

    [[nodiscard]] bool empty() const noexcept {
        return triangles.empty() &&
               faces.empty() &&
               edges.empty() &&
               vertices.empty();
    }

    [[nodiscard]] bool valid() const noexcept {
        if (empty()) {
            return !generation.valid();
        }
        if (!generation.valid() ||
            triangles.empty()) {
            return false;
        }
        if (!std::all_of(
                triangles.begin(),
                triangles.end(),
                [](const SolidTrianglePresentation& triangle) {
                    return triangle.valid();
                })) {
            return false;
        }

        std::vector<PresentationToken> tokens;
        tokens.reserve(
            faces.size() +
            edges.size() +
            vertices.size());

        for (const auto& face : faces) {
            if (!face.valid(triangles.size())) {
                return false;
            }
            tokens.push_back(face.token);
        }
        for (const auto& edge : edges) {
            if (!edge.valid()) {
                return false;
            }
            tokens.push_back(edge.token);
        }
        for (const auto& vertex : vertices) {
            if (!vertex.valid()) {
                return false;
            }
            tokens.push_back(vertex.token);
        }

        for (std::size_t left = 0U;
             left < tokens.size();
             ++left) {
            for (std::size_t right = left + 1U;
                 right < tokens.size();
                 ++right) {
                if (tokens[left] == tokens[right]) {
                    return false;
                }
            }
        }
        return true;
    }

    friend bool operator==(
        const BodyScene&,
        const BodyScene&) = default;
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
