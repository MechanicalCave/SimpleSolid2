#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::part {

class FeatureId final {
public:
    constexpr FeatureId() noexcept = default;
    [[nodiscard]] static std::optional<FeatureId> parse(
        std::string_view serialized) noexcept;
    [[nodiscard]] std::string serialized() const;
    [[nodiscard]] constexpr bool valid() const noexcept {
        return value_ != 0U;
    }

    friend bool operator==(const FeatureId&, const FeatureId&) = default;
    friend auto operator<=>(const FeatureId&, const FeatureId&) = default;

private:
    explicit constexpr FeatureId(std::uint64_t value) noexcept
        : value_{value} {}
    std::uint64_t value_{0U};
    friend class FeatureIdCursor;
};

class FeatureIdCursor final {
public:
    constexpr FeatureIdCursor() noexcept = default;
    [[nodiscard]] static std::optional<FeatureIdCursor> parse(
        std::string_view serialized) noexcept;
    [[nodiscard]] std::string serialized() const;
    [[nodiscard]] std::optional<FeatureId> allocate() noexcept;
    void preserve(FeatureIdCursor observed) noexcept;
    [[nodiscard]] constexpr bool containsAllocated(FeatureId id) const noexcept {
        return id.valid() && id.value_ < next_value_;
    }

    friend bool operator==(const FeatureIdCursor&, const FeatureIdCursor&) = default;
    friend auto operator<=>(const FeatureIdCursor&, const FeatureIdCursor&) = default;

private:
    explicit constexpr FeatureIdCursor(std::uint64_t next_value) noexcept
        : next_value_{next_value} {}
    std::uint64_t next_value_{1U};
};

} // namespace simplesolid2::part
