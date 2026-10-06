#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::part {

class AxisId final {
public:
    constexpr AxisId() noexcept = default;

    [[nodiscard]] static std::optional<AxisId> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0U;
    }

    friend bool operator==(const AxisId&, const AxisId&) = default;
    friend auto operator<=>(const AxisId&, const AxisId&) = default;

private:
    explicit constexpr AxisId(std::uint64_t value) noexcept
        : value_{value} {}

    std::uint64_t value_{0U};

    friend class AxisIdCursor;
};

class AxisIdCursor final {
public:
    constexpr AxisIdCursor() noexcept = default;

    [[nodiscard]] static std::optional<AxisIdCursor> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    [[nodiscard]] std::optional<AxisId> allocate() noexcept;

    void preserve(AxisIdCursor observed) noexcept;

    [[nodiscard]] constexpr bool containsAllocated(
        AxisId id) const noexcept {
        return id.valid() && id.value_ < next_value_;
    }

    friend bool operator==(const AxisIdCursor&, const AxisIdCursor&) = default;
    friend auto operator<=>(const AxisIdCursor&, const AxisIdCursor&) = default;

private:
    explicit constexpr AxisIdCursor(std::uint64_t next_value) noexcept
        : next_value_{next_value} {}

    std::uint64_t next_value_{1U};
};

} // namespace simplesolid2::part
