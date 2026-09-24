#pragma once

#include <compare>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace simplesolid2::sketch {

class SketchId final {
public:
    [[nodiscard]] static SketchId generate();
    [[nodiscard]] static std::optional<SketchId> parse(
        std::string_view serialized);

    [[nodiscard]] std::string_view value() const noexcept {
        return value_;
    }

    friend bool operator==(const SketchId&, const SketchId&) = default;
    friend auto operator<=>(const SketchId&, const SketchId&) = default;

private:
    explicit SketchId(std::string value)
        : value_{std::move(value)} {}

    std::string value_;
};

} // namespace simplesolid2::sketch
