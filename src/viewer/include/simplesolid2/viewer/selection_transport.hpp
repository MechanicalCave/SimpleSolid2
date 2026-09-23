#pragma once

#include <simplesolid2/viewer/reference_presentation.hpp>

#include <functional>
#include <optional>

namespace simplesolid2::viewer {

enum class SelectionIntentMode {
    replace,
    toggle,
};

struct SelectionIntent final {
    std::optional<PresentationToken> token;
    SelectionIntentMode mode{SelectionIntentMode::replace};
};

using SelectionIntentHandler =
    std::function<void(const SelectionIntent&)>;

} // namespace simplesolid2::viewer
