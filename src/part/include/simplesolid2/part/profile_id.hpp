#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::part {

class ProfileId final {
public:
    constexpr ProfileId() noexcept = default;

    [[nodiscard]] static std::optional<ProfileId> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0U;
    }

    friend bool operator==(
        const ProfileId&,
        const ProfileId&) = default;
    friend auto operator<=>(
        const ProfileId&,
        const ProfileId&) = default;

private:
    explicit constexpr ProfileId(
        std::uint64_t value) noexcept
        : value_{value} {}

    std::uint64_t value_{0U};

    friend class ProfileIdCursor;
};

class ProfileIdCursor final {
public:
    constexpr ProfileIdCursor() noexcept = default;

    [[nodiscard]] static std::optional<ProfileIdCursor> parse(
        std::string_view serialized) noexcept;

    [[nodiscard]] std::string serialized() const;

    [[nodiscard]] std::optional<ProfileId>
    allocate() noexcept;

    void preserve(
        ProfileIdCursor observed) noexcept;

    friend bool operator==(
        const ProfileIdCursor&,
        const ProfileIdCursor&) = default;
    friend auto operator<=>(
        const ProfileIdCursor&,
        const ProfileIdCursor&) = default;

private:
    explicit constexpr ProfileIdCursor(
        std::uint64_t next_value) noexcept
        : next_value_{next_value} {}

    std::uint64_t next_value_{1U};
};

} // namespace simplesolid2::part
