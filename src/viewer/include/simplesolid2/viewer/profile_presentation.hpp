#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>

#include <algorithm>
#include <optional>
#include <vector>

namespace simplesolid2::viewer {

struct ProfileRegionPresentation final {
    std::vector<Point3> outer;
    std::vector<std::vector<Point3>> holes;

    [[nodiscard]] bool valid() const noexcept {
        const auto loop_valid =
            [](const std::vector<Point3>& loop) {
                return loop.size() >= 3U &&
                       std::all_of(
                           loop.begin(),
                           loop.end(),
                           [](const Point3& point) {
                               return finite(point);
                           });
            };
        return loop_valid(outer) &&
               std::all_of(
                   holes.begin(),
                   holes.end(),
                   loop_valid);
    }
};

struct ProfilePresentation final {
    PresentationToken token;
    ProfileRegionPresentation region;

    [[nodiscard]] bool valid() const noexcept {
        return token.valid() && region.valid();
    }
};

struct ProfileScene final {
    std::vector<ProfilePresentation> profiles;

    [[nodiscard]] bool valid() const noexcept {
        for (std::size_t i = 0U; i < profiles.size(); ++i) {
            if (!profiles[i].valid()) return false;
            for (std::size_t j = 0U; j < i; ++j) {
                if (profiles[i].token == profiles[j].token) {
                    return false;
                }
            }
        }
        return true;
    }
};

enum class ProfilePreviewTone {
    additive,
    subtractive,
};

struct ProfilePreviewScene final {
    std::optional<ProfileRegionPresentation> region;
    bool show_boundary{false};
    ProfilePreviewTone tone{ProfilePreviewTone::additive};

    [[nodiscard]] bool valid() const noexcept {
        return !region || region->valid();
    }
};

} // namespace simplesolid2::viewer
