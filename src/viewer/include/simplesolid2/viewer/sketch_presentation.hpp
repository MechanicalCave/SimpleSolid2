#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>

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

} // namespace simplesolid2::viewer
