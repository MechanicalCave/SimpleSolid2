#pragma once

#include <simplesolid2/viewer/math3.hpp>
#include <simplesolid2/viewer/reference_presentation.hpp>

#include <algorithm>
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

enum class ViewStyle : std::uint8_t {
    shaded,
    shaded_with_edges,
    shaded_with_hidden_edges,
};

struct BodyPresentationGeneration final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend constexpr bool operator==(
        const BodyPresentationGeneration&,
        const BodyPresentationGeneration&) = default;
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
    bool pickable{true};

    friend bool operator==(
        const BodyFacePresentation&,
        const BodyFacePresentation&) = default;
};

struct BodyEdgePresentation final {
    PresentationToken token;
    std::vector<Point3> points;
    // Material/design Edge presentation is ordinarily visible in edge styles.
    // Representation artifacts remain in the complete scene but set this false.
    bool ordinary_visible{true};
    bool pickable{true};

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
    Point3 position;
    bool pickable{true};

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() &&
               finite(position);
    }

    friend bool operator==(
        const BodyVertexPresentation&,
        const BodyVertexPresentation&) = default;
};

struct BodyScene final {
    BodyPresentationGeneration generation;
    bool authoritative_for_modeling{true};
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
            triangles.empty() ||
            faces.empty() ||
            edges.empty() ||
            vertices.empty() ||
            !std::all_of(
                triangles.begin(),
                triangles.end(),
                [](const auto& triangle) {
                    return triangle.valid();
                }) ||
            !std::all_of(
                edges.begin(),
                edges.end(),
                [](const auto& edge) {
                    return edge.valid();
                }) ||
            !std::all_of(
                vertices.begin(),
                vertices.end(),
                [](const auto& vertex) {
                    return vertex.valid();
                })) {
            return false;
        }

        std::vector<bool> covered(
            triangles.size(),
            false);
        std::vector<std::uint64_t> tokens;
        tokens.reserve(
            faces.size() +
            edges.size() +
            vertices.size());

        const auto add_token =
            [&tokens](PresentationToken token) {
                if (!token.valid() ||
                    std::find(
                        tokens.begin(),
                        tokens.end(),
                        token.value) !=
                        tokens.end()) {
                    return false;
                }
                tokens.push_back(token.value);
                return true;
            };

        for (const auto& face : faces) {
            if (!add_token(face.token) ||
                face.triangle_count == 0U ||
                face.first_triangle >=
                    triangles.size() ||
                face.triangle_count >
                    triangles.size() -
                        face.first_triangle) {
                return false;
            }
            for (std::size_t index =
                     face.first_triangle;
                 index <
                     face.first_triangle +
                         face.triangle_count;
                 ++index) {
                if (covered[index]) {
                    return false;
                }
                covered[index] = true;
            }
        }

        if (std::any_of(
                covered.begin(),
                covered.end(),
                [](bool value) {
                    return !value;
                })) {
            return false;
        }

        for (const auto& edge : edges) {
            if (!add_token(edge.token)) {
                return false;
            }
        }
        for (const auto& vertex : vertices) {
            if (!add_token(vertex.token)) {
                return false;
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
