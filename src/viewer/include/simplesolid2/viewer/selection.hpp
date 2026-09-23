#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

namespace simplesolid2::viewer {

enum class SelectionIntentMode : std::uint8_t {
    replace,
    toggle,
};

struct SelectionIntent final {
    PresentationToken token;
    SelectionIntentMode mode{SelectionIntentMode::replace};

    [[nodiscard]] bool valid() const noexcept {
        return token.valid();
    }
};

struct PresentationSelection final {
    std::vector<PresentationToken> selected;
    std::optional<PresentationToken> primary;

    [[nodiscard]] bool valid() const noexcept {
        for (std::size_t left = 0; left < selected.size(); ++left) {
            if (!selected[left].valid()) {
                return false;
            }

            for (std::size_t right = left + 1U;
                 right < selected.size();
                 ++right) {
                if (selected[left] == selected[right]) {
                    return false;
                }
            }
        }

        if (!primary) {
            return true;
        }

        return primary->valid() &&
               std::find(
                   selected.begin(),
                   selected.end(),
                   *primary) != selected.end();
    }
};

using SelectionIntentHandler =
    std::function<void(const SelectionIntent&)>;

} // namespace simplesolid2::viewer
