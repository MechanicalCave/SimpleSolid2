#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::viewer {

struct SketchLinePresentation final {
    PresentationToken token;
    Point3 start{};
    Point3 end{};

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

} // namespace simplesolid2::viewer
