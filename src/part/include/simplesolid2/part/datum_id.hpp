#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::part {

class DatumId final {
public:
    constexpr DatumId() noexcept = default;

    [[nodiscard]] static std::optional<DatumId> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0U;
    }

    friend bool operator==(const DatumId&, const DatumId&) = default;
    friend auto operator<=>(const DatumId&, const DatumId&) = default;

private:
    explicit constexpr DatumId(std::uint64_t value) noexcept
        : value_{value} {}

    std::uint64_t value_{0U};

    friend class DatumIdCursor;
};

class DatumIdCursor final {
public:
    constexpr DatumIdCursor() noexcept = default;

    [[nodiscard]] static std::optional<DatumIdCursor> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    [[nodiscard]] std::optional<DatumId> allocate() noexcept;

    void preserve(DatumIdCursor observed) noexcept;

    [[nodiscard]] constexpr bool containsAllocated(
        DatumId id) const noexcept {
        return id.valid() && id.value_ < next_value_;
    }

    friend bool operator==(const DatumIdCursor&, const DatumIdCursor&) = default;
    friend auto operator<=>(const DatumIdCursor&, const DatumIdCursor&) = default;

private:
    explicit constexpr DatumIdCursor(std::uint64_t next_value) noexcept
        : next_value_{next_value} {}

    std::uint64_t next_value_{1U};
};

} // namespace simplesolid2::part
