#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::sketch {

class SketchModel;

class EntityId final {
public:
    constexpr EntityId() noexcept = default;

    [[nodiscard]] static std::optional<EntityId> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

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

class EntityIdCursor final {
public:
    constexpr EntityIdCursor() noexcept = default;

    [[nodiscard]] static std::optional<EntityIdCursor> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    friend bool operator==(
        const EntityIdCursor&,
        const EntityIdCursor&) = default;
    friend auto operator<=>(
        const EntityIdCursor&,
        const EntityIdCursor&) = default;

private:
    explicit constexpr EntityIdCursor(
        std::uint64_t next_value) noexcept
        : next_value_{next_value} {}

    std::uint64_t next_value_{1U};

    friend class SketchModel;
};

} // namespace simplesolid2::sketch
