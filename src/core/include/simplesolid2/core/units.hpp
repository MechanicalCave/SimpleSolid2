#pragma once

#include <cmath>
#include <cstdint>
#include <string_view>

namespace simplesolid2::core {

enum class LengthUnit : std::uint8_t {
    millimetre,
    centimetre,
    metre,
    inch,
    foot,
};

[[nodiscard]] constexpr bool
isLengthUnit(LengthUnit unit) noexcept {
    switch (unit) {
    case LengthUnit::millimetre:
    case LengthUnit::centimetre:
    case LengthUnit::metre:
    case LengthUnit::inch:
    case LengthUnit::foot:
        return true;
    }
    return false;
}

struct LengthValue final {
    double millimetres{};

    [[nodiscard]] bool finite() const noexcept {
        return std::isfinite(millimetres);
    }

    friend bool operator==(const LengthValue&, const LengthValue&) = default;
};

struct AngleValue final {
    double radians{};

    [[nodiscard]] bool finite() const noexcept {
        return std::isfinite(radians);
    }

    friend bool operator==(const AngleValue&, const AngleValue&) = default;
};

struct ScalarValue final {
    double value{};

    [[nodiscard]] bool finite() const noexcept {
        return std::isfinite(value);
    }

    friend bool operator==(const ScalarValue&, const ScalarValue&) = default;
};

[[nodiscard]] constexpr double
millimetresPerUnit(LengthUnit unit) noexcept {
    switch (unit) {
    case LengthUnit::millimetre:
        return 1.0;
    case LengthUnit::centimetre:
        return 10.0;
    case LengthUnit::metre:
        return 1000.0;
    case LengthUnit::inch:
        return 25.4;
    case LengthUnit::foot:
        return 304.8;
    }
    return 1.0;
}

[[nodiscard]] constexpr LengthValue
toCanonicalLength(double value, LengthUnit unit) noexcept {
    return LengthValue{value * millimetresPerUnit(unit)};
}

[[nodiscard]] constexpr double
fromCanonicalLength(LengthValue value, LengthUnit unit) noexcept {
    return value.millimetres / millimetresPerUnit(unit);
}

[[nodiscard]] constexpr std::string_view
lengthUnitSuffix(LengthUnit unit) noexcept {
    switch (unit) {
    case LengthUnit::millimetre:
        return "mm";
    case LengthUnit::centimetre:
        return "cm";
    case LengthUnit::metre:
        return "m";
    case LengthUnit::inch:
        return "in";
    case LengthUnit::foot:
        return "ft";
    }
    return "mm";
}

} // namespace simplesolid2::core
