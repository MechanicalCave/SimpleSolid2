#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>
#include <simplesolid2/viewer/spatial_pointer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace simplesolid2::viewer {

struct SketchLinePresentation final {
    PresentationToken token;
    Point3 start{};
    Point3 end{};
    bool construction{false};

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() &&
               finite(start) &&
               finite(end) &&
               start != end;
    }
};

struct SketchCurvePresentation final {
    PresentationToken token;
    std::vector<Point3> points;
    bool construction{false};

    [[nodiscard]] bool valid() const noexcept {
        if (!token.valid() || points.size() < 2U) {
            return false;
        }

        for (std::size_t index = 0U;
             index < points.size();
             ++index) {
            if (!finite(points[index])) {
                return false;
            }
            if (index > 0U &&
                points[index] == points[index - 1U]) {
                return false;
            }
        }

        return true;
    }
};

struct SketchOriginPresentation final {
    Point3 position{};

    [[nodiscard]] bool valid() const noexcept {
        return finite(position);
    }
};

struct SketchScene final {
    std::vector<SketchLinePresentation> lines;
    std::vector<SketchCurvePresentation> curves;
    std::optional<SketchOriginPresentation> origin;

    [[nodiscard]] bool valid() const noexcept {
        if (origin && !origin->valid()) {
            return false;
        }

        std::vector<PresentationToken> tokens;
        tokens.reserve(lines.size() + curves.size());

        for (const auto& line : lines) {
            if (!line.valid()) {
                return false;
            }
            tokens.push_back(line.token);
        }

        for (const auto& curve : curves) {
            if (!curve.valid()) {
                return false;
            }
            tokens.push_back(curve.token);
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
};

struct SketchPreviewLine final {
    Point3 start{};
    Point3 end{};
    bool construction{false};

    [[nodiscard]] bool valid() const noexcept {
        return finite(start) &&
               finite(end) &&
               start != end;
    }
};

struct SketchPreviewScene final {
    std::vector<SketchPreviewLine> lines;

    [[nodiscard]] bool valid() const noexcept {
        for (const auto& line : lines) {
            if (!line.valid()) {
                return false;
            }
        }
        return true;
    }
};

enum class SketchGripRole : std::uint8_t {
    line_start,
    line_center,
    line_end,
    circle_center,
    circle_quadrant_pos_u,
    circle_quadrant_pos_v,
    circle_quadrant_neg_u,
    circle_quadrant_neg_v,
    arc_center,
    arc_start,
    arc_end,
    arc_mid,
};

struct SketchGripKey final {
    PresentationToken owner;
    SketchGripRole role{SketchGripRole::line_center};

    [[nodiscard]] bool valid() const noexcept {
        return owner.valid();
    }

    friend bool operator==(
        const SketchGripKey&,
        const SketchGripKey&) = default;
};

struct SketchGripPresentation final {
    SketchGripKey key;
    Point3 position{};

    [[nodiscard]] bool valid() const noexcept {
        return key.valid() &&
               finite(position);
    }
};

struct SketchGripScene final {
    std::vector<SketchGripPresentation> grips;

    [[nodiscard]] bool valid() const noexcept {
        for (std::size_t left = 0U;
             left < grips.size();
             ++left) {
            if (!grips[left].valid()) {
                return false;
            }
            for (std::size_t right = left + 1U;
                 right < grips.size();
                 ++right) {
                if (grips[left].key ==
                    grips[right].key) {
                    return false;
                }
            }
        }
        return true;
    }
};

struct SketchInteractionPresentation final {
    std::optional<PresentationToken> hovered_entity;
    std::optional<SketchGripKey> hovered_grip;
    std::optional<SketchGripKey> active_grip;

    [[nodiscard]] bool valid() const noexcept {
        if (hovered_entity &&
            !hovered_entity->valid()) {
            return false;
        }
        if (hovered_grip &&
            !hovered_grip->valid()) {
            return false;
        }
        if (active_grip &&
            !active_grip->valid()) {
            return false;
        }
        return !(hovered_entity &&
                 hovered_grip);
    }
};

struct SketchGripQueryResult final {
    bool completed{};
    std::optional<SketchGripKey> grip;

    [[nodiscard]] bool valid() const noexcept {
        if (!completed) {
            return !grip.has_value();
        }
        return !grip ||
               grip->valid();
    }
};

enum class SketchMeasureMarkerRole : std::uint8_t {
    line_start,
    line_midpoint,
    line_end,
    circle_center,
    circle_quadrant_pos_u,
    circle_quadrant_pos_v,
    circle_quadrant_neg_u,
    circle_quadrant_neg_v,
    arc_center,
    arc_start,
    arc_end,
    arc_midpoint,
};

struct SketchMeasureMarkerKey final {
    PresentationToken owner;
    SketchMeasureMarkerRole role{
        SketchMeasureMarkerRole::line_midpoint};

    [[nodiscard]] bool valid() const noexcept {
        return owner.valid();
    }

    friend bool operator==(
        const SketchMeasureMarkerKey&,
        const SketchMeasureMarkerKey&) = default;
};

struct SketchMeasureMarkerPresentation final {
    SketchMeasureMarkerKey key;
    Point3 position{};

    [[nodiscard]] bool valid() const noexcept {
        return key.valid() &&
               finite(position);
    }
};

struct SketchMeasureMarkerScene final {
    std::vector<SketchMeasureMarkerPresentation> markers;
    std::vector<SketchMeasureMarkerKey> selected;

    [[nodiscard]] bool empty() const noexcept {
        return markers.empty() &&
               selected.empty();
    }

    [[nodiscard]] bool valid() const noexcept {
        if (selected.size() > 2U) {
            return false;
        }

        for (std::size_t left = 0U;
             left < markers.size();
             ++left) {
            if (!markers[left].valid()) {
                return false;
            }
            for (std::size_t right = left + 1U;
                 right < markers.size();
                 ++right) {
                if (markers[left].key ==
                    markers[right].key) {
                    return false;
                }
            }
        }

        for (std::size_t left = 0U;
             left < selected.size();
             ++left) {
            if (!selected[left].valid()) {
                return false;
            }

            bool found{};
            for (const auto& marker : markers) {
                if (marker.key == selected[left]) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                return false;
            }

            for (std::size_t right = left + 1U;
                 right < selected.size();
                 ++right) {
                if (selected[left] ==
                    selected[right]) {
                    return false;
                }
            }
        }

        return true;
    }
};

struct SketchMeasureMarkerQueryResult final {
    bool completed{};
    std::vector<SketchMeasureMarkerKey> markers;

    [[nodiscard]] bool valid() const noexcept {
        if (!completed) {
            return markers.empty();
        }

        for (std::size_t left = 0U;
             left < markers.size();
             ++left) {
            if (!markers[left].valid()) {
                return false;
            }
            for (std::size_t right = left + 1U;
                 right < markers.size();
                 ++right) {
                if (markers[left] ==
                    markers[right]) {
                    return false;
                }
            }
        }
        return true;
    }
};

enum class SketchMeasureCueSegmentKind : std::uint8_t {
    relation,
    supporting_line_continuation,
};

struct SketchMeasureCueSegment final {
    Point3 start{};
    Point3 end{};
    SketchMeasureCueSegmentKind kind{
        SketchMeasureCueSegmentKind::relation};

    [[nodiscard]] bool valid() const noexcept {
        return finite(start) &&
               finite(end) &&
               start != end;
    }
};

struct SketchMeasureCueScene final {
    std::vector<PresentationToken> highlighted_entities;
    std::vector<SketchMeasureCueSegment> segments;
    std::optional<Point3> cue_point;

    [[nodiscard]] bool empty() const noexcept {
        return highlighted_entities.empty() &&
               segments.empty() &&
               !cue_point.has_value();
    }

    [[nodiscard]] bool valid() const noexcept {
        if (cue_point &&
            !finite(*cue_point)) {
            return false;
        }

        for (std::size_t left = 0U;
             left < highlighted_entities.size();
             ++left) {
            if (!highlighted_entities[left].valid()) {
                return false;
            }
            for (std::size_t right = left + 1U;
                 right < highlighted_entities.size();
                 ++right) {
                if (highlighted_entities[left] ==
                    highlighted_entities[right]) {
                    return false;
                }
            }
        }

        for (const auto& segment : segments) {
            if (!segment.valid()) {
                return false;
            }
        }
        return true;
    }
};

enum class SketchSnapMarkerKind : std::uint8_t {
    endpoint,
    midpoint,
    center,
    quadrant,
    intersection,
    origin,
    perpendicular,
    tangent,
    nearest,
};

struct SketchSnapMarkerPresentation final {
    Point3 position{};
    SketchSnapMarkerKind kind{
        SketchSnapMarkerKind::endpoint};
    std::string label;

    [[nodiscard]] bool valid() const noexcept {
        return finite(position) &&
               !label.empty();
    }

    friend bool operator==(
        const SketchSnapMarkerPresentation&,
        const SketchSnapMarkerPresentation&) = default;
};

struct SketchSnapInferenceScene final {
    std::optional<SketchSnapMarkerPresentation>
        current;
    std::vector<SketchSnapMarkerPresentation>
        acquired;

    [[nodiscard]] bool empty() const noexcept {
        return !current &&
               acquired.empty();
    }

    [[nodiscard]] bool valid() const noexcept {
        if (current && !current->valid()) {
            return false;
        }
        if (acquired.size() > 2U) {
            return false;
        }
        return std::all_of(
            acquired.begin(),
            acquired.end(),
            [](const auto& marker) {
                return marker.valid();
            });
    }

    friend bool operator==(
        const SketchSnapInferenceScene&,
        const SketchSnapInferenceScene&) = default;
};

enum class SketchDynamicInputValueState : std::uint8_t {
    free,
    assisted,
    locked,
};

struct SketchDynamicInputFieldPresentation final {
    std::string label;
    std::string display_value;
    SketchDynamicInputValueState state{
        SketchDynamicInputValueState::free};

    [[nodiscard]] bool valid() const noexcept {
        return !label.empty();
    }

    friend bool operator==(
        const SketchDynamicInputFieldPresentation&,
        const SketchDynamicInputFieldPresentation&) = default;
};

struct SketchDynamicInputOverlay final {
    ViewportPoint2 anchor;
    std::vector<SketchDynamicInputFieldPresentation> fields;
    std::size_t focused_index{};

    [[nodiscard]] bool valid() const noexcept {
        if (!anchor.valid() ||
            fields.empty() ||
            focused_index >= fields.size()) {
            return false;
        }

        for (const auto& field : fields) {
            if (!field.valid()) {
                return false;
            }
        }
        return true;
    }

    friend bool operator==(
        const SketchDynamicInputOverlay&,
        const SketchDynamicInputOverlay&) = default;
};

} // namespace simplesolid2::viewer
