#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>

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

struct SketchOriginPresentation final {
    Point3 position{};

    [[nodiscard]] bool valid() const noexcept {
        return finite(position);
    }
};

struct SketchScene final {
    std::vector<SketchLinePresentation> lines;
    std::optional<SketchOriginPresentation> origin;

    [[nodiscard]] bool valid() const noexcept {
        if (origin && !origin->valid()) {
            return false;
        }

        for (const auto& line : lines) {
            if (!line.valid()) {
                return false;
            }
        }

        for (std::size_t left = 0U;
             left < lines.size();
             ++left) {
            for (std::size_t right = left + 1U;
                 right < lines.size();
                 ++right) {
                if (lines[left].token ==
                    lines[right].token) {
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
