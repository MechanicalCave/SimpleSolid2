#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::part {

class BodyId final {
public:
    constexpr BodyId() noexcept = default;
    [[nodiscard]] static std::optional<BodyId> parse(
        std::string_view serialized) noexcept;
    [[nodiscard]] std::string serialized() const;
    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0U;
    }

    friend bool operator==(const BodyId&, const BodyId&) = default;
    friend auto operator<=>(const BodyId&, const BodyId&) = default;

private:
    explicit constexpr BodyId(std::uint64_t value) noexcept
        : value_{value} {}
    std::uint64_t value_{0U};
    friend class BodyIdCursor;
};

class BodyIdCursor final {
public:
    constexpr BodyIdCursor() noexcept = default;
    [[nodiscard]] static std::optional<BodyIdCursor> parse(
        std::string_view serialized) noexcept;
    [[nodiscard]] std::string serialized() const;
    [[nodiscard]] std::optional<BodyId> allocate() noexcept;
    void preserve(BodyIdCursor observed) noexcept;
    [[nodiscard]] constexpr bool containsAllocated(BodyId id) const noexcept {
        return id.valid() && id.value_ < next_value_;
    }

    friend bool operator==(const BodyIdCursor&, const BodyIdCursor&) = default;
    friend auto operator<=>(const BodyIdCursor&, const BodyIdCursor&) = default;

private:
    explicit constexpr BodyIdCursor(std::uint64_t next_value) noexcept
        : next_value_{next_value} {}
    std::uint64_t next_value_{1U};
};

} // namespace simplesolid2::part
