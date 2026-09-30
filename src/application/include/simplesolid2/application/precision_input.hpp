#pragma once

#include <simplesolid2/core/units.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace simplesolid2::application {

enum class CadQuantityDimension : std::uint8_t {
    scalar,
    length,
    angle,
};

struct CadQuantity final {
    CadQuantityDimension dimension{CadQuantityDimension::scalar};
    double canonical_value{};

    [[nodiscard]] bool finite() const noexcept;

    friend bool operator==(const CadQuantity&, const CadQuantity&) = default;
};

struct CadQuantityParseContext final {
    CadQuantityDimension expected_dimension{CadQuantityDimension::scalar};
    core::LengthUnit length_unit{core::LengthUnit::millimetre};
};

[[nodiscard]] std::optional<CadQuantity>
parseCadQuantity(
    std::string_view text,
    CadQuantityParseContext context) noexcept;

enum class CadPointTokenKind : std::uint8_t {
    absolute_cartesian,
    relative_cartesian,
    relative_polar,
};

struct CadPointToken final {
    CadPointTokenKind kind{CadPointTokenKind::absolute_cartesian};

    // Cartesian: first/second are canonical millimetres.
    // Relative polar: first is canonical millimetres and second is
    // canonical radians measured from Sketch +U.
    double first{};
    double second{};

    [[nodiscard]] bool finite() const noexcept;

    friend bool operator==(const CadPointToken&, const CadPointToken&) = default;
};

[[nodiscard]] std::optional<CadPointToken>
parseCadPointToken(
    std::string_view text,
    core::LengthUnit length_unit) noexcept;

} // namespace simplesolid2::application
