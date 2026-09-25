#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>
#include <simplesolid2/viewer/spatial_pointer.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::viewer {

struct ViewportRect2 final {
    ViewportPoint2 minimum;
    ViewportPoint2 maximum;

    [[nodiscard]] bool valid() const noexcept {
        return minimum.finite() &&
               maximum.finite() &&
               maximum.x > minimum.x &&
               maximum.y > minimum.y;
    }

    friend bool operator==(
        const ViewportRect2&,
        const ViewportRect2&) = default;
};

[[nodiscard]] inline std::optional<ViewportRect2>
normalizedViewportRect(
    ViewportPoint2 first,
    ViewportPoint2 second) noexcept {
    if (!first.finite() ||
        !second.finite() ||
        first.x == second.x ||
        first.y == second.y) {
        return std::nullopt;
    }

    return ViewportRect2{
        {
            std::min(first.x, second.x),
            std::min(first.y, second.y),
        },
        {
            std::max(first.x, second.x),
            std::max(first.y, second.y),
        }};
}

enum class SketchRectangleSelectionRule
    : std::uint8_t {
    window,
    crossing,
};

struct SketchPointQueryResult final {
    bool completed{};
    std::optional<PresentationToken> token;

    [[nodiscard]] bool valid() const noexcept {
        if (!completed) {
            return !token.has_value();
        }

        return !token.has_value() ||
               token->valid();
    }
};

struct SketchRectangleQueryResult final {
    bool completed{};
    std::vector<PresentationToken> tokens;

    [[nodiscard]] bool valid() const noexcept {
        if (!completed && !tokens.empty()) {
            return false;
        }

        for (std::size_t left = 0;
             left < tokens.size();
             ++left) {
            if (!tokens[left].valid()) {
                return false;
            }

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

struct SketchSelectionBoxOverlay final {
    ViewportPoint2 anchor;
    ViewportPoint2 current;
    SketchRectangleSelectionRule rule{
        SketchRectangleSelectionRule::window};

    [[nodiscard]] bool valid() const noexcept {
        return anchor.finite() &&
               current.finite();
    }

    friend bool operator==(
        const SketchSelectionBoxOverlay&,
        const SketchSelectionBoxOverlay&) = default;
};

} // namespace simplesolid2::viewer
