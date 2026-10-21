#pragma once
#include <simplesolid2/sketch/entity_id.hpp>
#include <charconv>
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::sketch {
class SketchModel;

class ConstraintId final {
public:
    constexpr ConstraintId() noexcept = default;
    [[nodiscard]] static std::optional<ConstraintId> parse(std::string_view text) noexcept {
        if (text.empty() || text.front() == '0') return std::nullopt;
        std::uint64_t value{};
        const auto result = std::from_chars(text.data(), text.data()+text.size(), value);
        if (result.ec != std::errc{} || result.ptr != text.data()+text.size() || value == 0) return std::nullopt;
        return ConstraintId{value};
    }
    [[nodiscard]] std::string serialized() const {
        return valid() ? std::to_string(value_) : std::string{};
    }
    [[nodiscard]] constexpr bool valid() const noexcept { return value_ != 0; }
    friend bool operator==(const ConstraintId&, const ConstraintId&) = default;
    friend auto operator<=>(const ConstraintId&, const ConstraintId&) = default;
private:
    explicit constexpr ConstraintId(std::uint64_t value) noexcept : value_{value} {}
    std::uint64_t value_{};
    friend class SketchModel;
};

class ConstraintIdCursor final {
public:
    constexpr ConstraintIdCursor() noexcept = default;
    [[nodiscard]] static std::optional<ConstraintIdCursor> parse(std::string_view text) noexcept {
        if (!ConstraintId::parse(text)) return std::nullopt;
        std::uint64_t value{};
        const auto result = std::from_chars(text.data(), text.data()+text.size(), value);
        if (result.ec != std::errc{}) return std::nullopt;
        return ConstraintIdCursor{value};
    }
    [[nodiscard]] std::string serialized() const { return std::to_string(next_value_); }
    friend bool operator==(const ConstraintIdCursor&, const ConstraintIdCursor&) = default;
    friend auto operator<=>(const ConstraintIdCursor&, const ConstraintIdCursor&) = default;
private:
    explicit constexpr ConstraintIdCursor(std::uint64_t next) noexcept : next_value_{next} {}
    std::uint64_t next_value_{1};
    friend class SketchModel;
};

// Append-only enum: preserve the identity of the five previously authored kinds.
enum class ConstraintKind { horizontal, vertical, coincident, parallel, perpendicular, fix };
enum class ConstraintElement { none, line, line_start, line_end };

struct ConstraintReference final {
    EntityId entity;
    ConstraintElement element{ConstraintElement::none};
    friend bool operator==(const ConstraintReference&, const ConstraintReference&) = default;
};

struct SketchConstraintState final {
    ConstraintId id;
    ConstraintKind kind{ConstraintKind::horizontal};
    ConstraintReference first;
    ConstraintReference second;
    friend bool operator==(const SketchConstraintState&, const SketchConstraintState&) = default;
};
} // namespace simplesolid2::sketch
