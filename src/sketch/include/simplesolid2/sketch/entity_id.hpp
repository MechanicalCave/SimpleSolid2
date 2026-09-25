#pragma once

#include <compare>
#include <cstdint>

namespace simplesolid2::sketch {

class SketchModel;

class EntityId final {
public:
    constexpr EntityId() noexcept = default;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0U;
    }

    friend bool operator==(const EntityId&, const EntityId&) = default;
    friend auto operator<=>(const EntityId&, const EntityId&) = default;

private:
    explicit constexpr EntityId(std::uint64_t value) noexcept
        : value_{value} {}

    std::uint64_t value_{0U};

    friend class SketchModel;
};

} // namespace simplesolid2::sketch
