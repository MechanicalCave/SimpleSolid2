#pragma once

#include <simplesolid2/viewer/math3.hpp>
#include <simplesolid2/viewer/reference_presentation.hpp>

#include <algorithm>
#include <cstddef>
#include <cmath>
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
    tool_stage,
};

enum class BodyTopologyPresentationKind : std::uint8_t {
    face,
    edge,
    vertex,
};

enum class ViewStyle : std::uint8_t {
    shaded,
    shaded_with_edges,
    shaded_with_hidden_edges,
};

struct BodyTopologyPickFilter final {
    bool faces{true};
    bool edges{true};
    bool vertices{true};

    [[nodiscard]] bool allows(
        BodyTopologyPresentationKind kind) const noexcept {
        switch (kind) {
        case BodyTopologyPresentationKind::face:
            return faces;
        case BodyTopologyPresentationKind::edge:
            return edges;
        case BodyTopologyPresentationKind::vertex:
            return vertices;
        }
        return false;
    }

    [[nodiscard]] bool any() const noexcept {
        return faces || edges || vertices;
    }

    friend bool operator==(
        const BodyTopologyPickFilter&,
        const BodyTopologyPickFilter&) = default;
};

struct BodyTopologyPickCandidate final {
    PresentationToken token;
    BodyTopologyPresentationKind kind{
        BodyTopologyPresentationKind::face};
    double screen_distance{};
    double depth{};

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() &&
               std::isfinite(screen_distance) &&
               screen_distance >= 0.0 &&
               std::isfinite(depth) &&
               depth >= 0.0;
    }

    friend bool operator==(
        const BodyTopologyPickCandidate&,
        const BodyTopologyPickCandidate&) = default;
};

struct BodyTopologyPickQueryResult final {
    bool completed{};
    BodyPresentationGeneration generation;
    std::vector<BodyTopologyPickCandidate>
        candidates;

    [[nodiscard]] bool valid() const noexcept {
        if (!completed) {
            return !generation.valid() &&
                   candidates.empty();
        }
        if (!generation.valid()) {
            return candidates.empty();
        }
        return std::all_of(
            candidates.begin(),
            candidates.end(),
            [](const BodyTopologyPickCandidate& item) {
                return item.valid();
            });
    }

    friend bool operator==(
        const BodyTopologyPickQueryResult&,
        const BodyTopologyPickQueryResult&) = default;
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
    bool material{true};
    bool ordinary_pickable{true};

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() &&
               points.size() >= 2U &&
               std::all_of(
                   points.begin(),
                   points.end(),
                   [](const Point3& point) {
                       return finite(point);
                   }) &&
               (!ordinary_pickable || material);
    }

    friend bool operator==(
        const BodyEdgePresentation&,
        const BodyEdgePresentation&) = default;
};

struct BodyVertexPresentation final {
    PresentationToken token;
    Point3 point;
    bool ordinary_pickable{true};

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

enum class BodyTopologyOverlayRole : std::uint8_t {
    feature_contribution_selected,
    feature_contribution_hover,
};

struct BodyTopologyOverlayGroup final {
    BodyTopologyOverlayRole role{
        BodyTopologyOverlayRole::
            feature_contribution_selected};
    std::vector<PresentationToken> tokens;

    [[nodiscard]] bool empty() const noexcept {
        return tokens.empty();
    }

    [[nodiscard]] bool valid() const noexcept {
        if (tokens.empty()) {
            return false;
        }
        for (std::size_t index = 0U;
             index < tokens.size();
             ++index) {
            if (!tokens[index].valid()) {
                return false;
            }
            for (std::size_t other = index + 1U;
                 other < tokens.size();
                 ++other) {
                if (tokens[index] ==
                    tokens[other]) {
                    return false;
                }
            }
        }
        return true;
    }

    friend bool operator==(
        const BodyTopologyOverlayGroup&,
        const BodyTopologyOverlayGroup&) = default;
};

struct BodyTopologyOverlayScene final {
    BodyPresentationGeneration generation;
    std::vector<BodyTopologyOverlayGroup>
        groups;

    [[nodiscard]] bool empty() const noexcept {
        return groups.empty();
    }

    [[nodiscard]] bool valid() const noexcept {
        if (groups.empty()) {
            return !generation.valid();
        }
        if (!generation.valid()) {
            return false;
        }
        for (std::size_t index = 0U;
             index < groups.size();
             ++index) {
            if (!groups[index].valid()) {
                return false;
            }
            for (std::size_t other = index + 1U;
                 other < groups.size();
                 ++other) {
                if (groups[index].role ==
                    groups[other].role) {
                    return false;
                }
            }
        }
        return true;
    }

    friend bool operator==(
        const BodyTopologyOverlayScene&,
        const BodyTopologyOverlayScene&) = default;
};

enum class SolidPreviewTone {
    additive,
    subtractive,
};

struct SolidPreviewScene final {
    // Existing one-color Extrude/Revolve preview. In material_delta mode,
    // these triangles contain ONLY the removed volume (orange).
    std::vector<SolidTrianglePresentation>
        triangles;
    SolidPreviewTone tone{
        SolidPreviewTone::additive};
    // PM-05F R2-D atomic two-color extension: added volume (blue).
    std::vector<SolidTrianglePresentation>
        added_triangles;
    BodyPresentationGeneration generation;
    bool material_delta{};

    [[nodiscard]] bool valid() const noexcept {
        const auto all_valid =
            [](const auto& mesh) {
                return std::all_of(
                    mesh.begin(), mesh.end(),
                    [](const SolidTrianglePresentation& triangle) {
                        return triangle.valid();
                    });
            };
        if (!all_valid(triangles) ||
            !all_valid(added_triangles)) {
            return false;
        }
        if (!material_delta) {
            return added_triangles.empty() &&
                   !generation.valid();
        }
        return generation.valid() &&
               tone == SolidPreviewTone::subtractive &&
               (!triangles.empty() ||
                !added_triangles.empty());
    }

    [[nodiscard]] bool empty() const noexcept {
        return triangles.empty() &&
               added_triangles.empty();
    }

    friend bool operator==(
        const SolidPreviewScene&,
        const SolidPreviewScene&) = default;
};

} // namespace simplesolid2::viewer
