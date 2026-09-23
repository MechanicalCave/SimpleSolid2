#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace simplesolid2::core {

enum class BuiltinReferenceRole : std::uint8_t {
    origin_point,
    x_axis,
    y_axis,
    z_axis,
    xy_plane,
    xz_plane,
    yz_plane,
};

inline constexpr std::array<BuiltinReferenceRole, 7> builtin_reference_roles{
    BuiltinReferenceRole::origin_point,
    BuiltinReferenceRole::x_axis,
    BuiltinReferenceRole::y_axis,
    BuiltinReferenceRole::z_axis,
    BuiltinReferenceRole::xy_plane,
    BuiltinReferenceRole::xz_plane,
    BuiltinReferenceRole::yz_plane,
};

[[nodiscard]] constexpr std::uint8_t builtinReferenceBit(
    BuiltinReferenceRole role) noexcept {
    switch (role) {
    case BuiltinReferenceRole::origin_point: return 0x01U;
    case BuiltinReferenceRole::x_axis: return 0x02U;
    case BuiltinReferenceRole::y_axis: return 0x04U;
    case BuiltinReferenceRole::z_axis: return 0x08U;
    case BuiltinReferenceRole::xy_plane: return 0x10U;
    case BuiltinReferenceRole::xz_plane: return 0x20U;
    case BuiltinReferenceRole::yz_plane: return 0x40U;
    }
    return 0U;
}

[[nodiscard]] constexpr bool isBuiltinReferenceRole(
    BuiltinReferenceRole role) noexcept {
    return builtinReferenceBit(role) != 0U;
}

class BuiltinReferenceVisibility final {
public:
    static constexpr std::uint8_t supported_mask = 0x7FU;
    static constexpr std::uint8_t default_mask = 0x0FU;

    constexpr BuiltinReferenceVisibility() noexcept = default;

    [[nodiscard]] static constexpr std::optional<BuiltinReferenceVisibility>
    fromMask(std::uint8_t mask) noexcept {
        if ((mask & static_cast<std::uint8_t>(~supported_mask)) != 0U) {
            return std::nullopt;
        }
        return BuiltinReferenceVisibility{mask};
    }

    [[nodiscard]] constexpr std::uint8_t mask() const noexcept {
        return mask_;
    }

    [[nodiscard]] constexpr bool visible(
        BuiltinReferenceRole role) const noexcept {
        const auto bit = builtinReferenceBit(role);
        return bit != 0U && (mask_ & bit) != 0U;
    }

    [[nodiscard]] constexpr bool setVisible(
        BuiltinReferenceRole role,
        bool visible) noexcept {
        const auto bit = builtinReferenceBit(role);
        if (bit == 0U) return false;

        const auto before = mask_;
        if (visible) {
            mask_ = static_cast<std::uint8_t>(mask_ | bit);
        } else {
            mask_ = static_cast<std::uint8_t>(mask_ & static_cast<std::uint8_t>(~bit));
        }
        return before != mask_;
    }

    friend constexpr bool operator==(
        const BuiltinReferenceVisibility&,
        const BuiltinReferenceVisibility&) = default;

private:
    explicit constexpr BuiltinReferenceVisibility(std::uint8_t mask) noexcept
        : mask_{mask} {}

    std::uint8_t mask_{default_mask};
};

} // namespace simplesolid2::core
