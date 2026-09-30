#include <simplesolid2/application/precision_input.hpp>

#include <charconv>
#include <cmath>
#include <cctype>
#include <numbers>
#include <string>
#include <system_error>
#include <utility>

namespace simplesolid2::application {
namespace {

std::string_view trimAscii(std::string_view text) noexcept {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1U);
    }
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1U);
    }
    return text;
}

bool asciiCaseEqual(
    std::string_view left,
    std::string_view right) noexcept {
    if (left.size() != right.size()) return false;
    for (std::size_t i = 0U; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

std::optional<double>
parseFiniteNumber(std::string_view text) noexcept {
    text = trimAscii(text);
    if (text.empty()) return std::nullopt;

    std::string normalized;
    normalized.reserve(text.size());
    bool saw_dot = false;
    bool saw_comma = false;
    for (const char ch : text) {
        if (ch == '.') {
            if (saw_dot || saw_comma) return std::nullopt;
            saw_dot = true;
            normalized.push_back('.');
        } else if (ch == ',') {
            if (saw_dot || saw_comma) return std::nullopt;
            saw_comma = true;
            normalized.push_back('.');
        } else {
            normalized.push_back(ch);
        }
    }

    double value{};
    const char* const first = normalized.data();
    const char* const last = first + normalized.size();
    const auto parsed =
        std::from_chars(first, last, value, std::chars_format::general);
    if (parsed.ec != std::errc{} ||
        parsed.ptr != last ||
        !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<CadQuantity>
coerceScalar(
    CadQuantity value,
    CadQuantityDimension dimension,
    core::LengthUnit length_unit) noexcept {
    if (!value.finite()) return std::nullopt;
    if (value.dimension == dimension) return value;
    if (value.dimension != CadQuantityDimension::scalar) {
        return std::nullopt;
    }

    switch (dimension) {
    case CadQuantityDimension::scalar:
        return value;
    case CadQuantityDimension::length:
        return CadQuantity{
            CadQuantityDimension::length,
            core::toCanonicalLength(value.canonical_value, length_unit)
                .millimetres};
    case CadQuantityDimension::angle:
        return CadQuantity{
            CadQuantityDimension::angle,
            value.canonical_value * std::numbers::pi_v<double> / 180.0};
    }
    return std::nullopt;
}

class QuantityParser final {
public:
    QuantityParser(
        std::string_view text,
        CadQuantityParseContext context) noexcept
        : text_{text},
          context_{context} {}

    [[nodiscard]] std::optional<CadQuantity> parse() noexcept {
        const auto result = parseAdditive();
        skipSpaces();
        if (!result || position_ != text_.size() || !result->finite()) {
            return std::nullopt;
        }
        return coerceScalar(
            *result,
            context_.expected_dimension,
            context_.length_unit);
    }

private:
    [[nodiscard]] std::optional<CadQuantity>
    parseAdditive() noexcept {
        auto left = parseMultiplicative();
        if (!left) return std::nullopt;

        for (;;) {
            skipSpaces();
            if (!consume('+') && !consume('-')) return left;
            const char operation = text_[position_ - 1U];
            auto right = parseMultiplicative();
            if (!right) return std::nullopt;

            if (left->dimension != right->dimension) {
                if (left->dimension == CadQuantityDimension::scalar &&
                    right->dimension == context_.expected_dimension) {
                    left = coerceScalar(
                        *left,
                        context_.expected_dimension,
                        context_.length_unit);
                } else if (
                    right->dimension == CadQuantityDimension::scalar &&
                    left->dimension == context_.expected_dimension) {
                    right = coerceScalar(
                        *right,
                        context_.expected_dimension,
                        context_.length_unit);
                }
            }
            if (!left || !right || left->dimension != right->dimension) {
                return std::nullopt;
            }

            left->canonical_value =
                operation == '+'
                    ? left->canonical_value + right->canonical_value
                    : left->canonical_value - right->canonical_value;
            if (!left->finite()) return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<CadQuantity>
    parseMultiplicative() noexcept {
        auto left = parseUnary();
        if (!left) return std::nullopt;

        for (;;) {
            skipSpaces();
            if (!consume('*') && !consume('/')) return left;
            const char operation = text_[position_ - 1U];
            const auto right = parseUnary();
            if (!right) return std::nullopt;

            left = operation == '*'
                       ? multiply(*left, *right)
                       : divide(*left, *right);
            if (!left) return std::nullopt;
        }
    }

    [[nodiscard]] std::optional<CadQuantity>
    parseUnary() noexcept {
        skipSpaces();
        double sign = 1.0;
        for (;;) {
            if (consume('+')) {
                skipSpaces();
                continue;
            }
            if (consume('-')) {
                sign = -sign;
                skipSpaces();
                continue;
            }
            break;
        }

        auto value = parsePrimary();
        if (!value) return std::nullopt;
        value->canonical_value *= sign;
        return value->finite() ? value : std::nullopt;
    }

    [[nodiscard]] std::optional<CadQuantity>
    parsePrimary() noexcept {
        skipSpaces();

        std::optional<CadQuantity> value;
        if (consume('(')) {
            value = parseAdditive();
            skipSpaces();
            if (!value || !consume(')')) return std::nullopt;
        } else {
            value = parseNumber();
            if (!value) return std::nullopt;
        }

        skipSpaces();
        const auto suffix = parseSuffix();
        if (suffix.empty()) return value;
        if (value->dimension != CadQuantityDimension::scalar) {
            return std::nullopt;
        }
        return applySuffix(*value, suffix);
    }

    [[nodiscard]] std::optional<CadQuantity>
    parseNumber() noexcept {
        skipSpaces();
        const std::size_t start = position_;

        bool saw_digit = false;
        bool saw_decimal = false;
        while (position_ < text_.size()) {
            const char ch = text_[position_];
            if (ch >= '0' && ch <= '9') {
                saw_digit = true;
                ++position_;
                continue;
            }
            if ((ch == '.' || ch == ',') && !saw_decimal) {
                saw_decimal = true;
                ++position_;
                continue;
            }
            break;
        }
        if (!saw_digit) return std::nullopt;

        if (position_ < text_.size() &&
            (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() &&
                (text_[position_] == '+' || text_[position_] == '-')) {
                ++position_;
            }
            const std::size_t exponent_digits = position_;
            while (position_ < text_.size() &&
                   text_[position_] >= '0' &&
                   text_[position_] <= '9') {
                ++position_;
            }
            if (position_ == exponent_digits) return std::nullopt;
        }

        const auto number =
            parseFiniteNumber(text_.substr(start, position_ - start));
        if (!number) return std::nullopt;
        return CadQuantity{
            CadQuantityDimension::scalar,
            *number};
    }

    [[nodiscard]] std::string_view parseSuffix() noexcept {
        const std::size_t start = position_;
        while (position_ < text_.size() &&
               std::isalpha(
                   static_cast<unsigned char>(text_[position_])) != 0) {
            ++position_;
        }
        return text_.substr(start, position_ - start);
    }

    [[nodiscard]] std::optional<CadQuantity>
    applySuffix(
        CadQuantity scalar,
        std::string_view suffix) const noexcept {
        if (!scalar.finite()) return std::nullopt;

        core::LengthUnit unit{};
        bool is_length = true;
        if (asciiCaseEqual(suffix, "mm")) {
            unit = core::LengthUnit::millimetre;
        } else if (asciiCaseEqual(suffix, "cm")) {
            unit = core::LengthUnit::centimetre;
        } else if (asciiCaseEqual(suffix, "m")) {
            unit = core::LengthUnit::metre;
        } else if (asciiCaseEqual(suffix, "in")) {
            unit = core::LengthUnit::inch;
        } else if (asciiCaseEqual(suffix, "ft")) {
            unit = core::LengthUnit::foot;
        } else {
            is_length = false;
        }

        if (is_length) {
            const auto converted =
                core::toCanonicalLength(scalar.canonical_value, unit);
            if (!converted.finite()) return std::nullopt;
            return CadQuantity{
                CadQuantityDimension::length,
                converted.millimetres};
        }

        if (asciiCaseEqual(suffix, "deg")) {
            const double radians =
                scalar.canonical_value *
                std::numbers::pi_v<double> / 180.0;
            if (!std::isfinite(radians)) return std::nullopt;
            return CadQuantity{CadQuantityDimension::angle, radians};
        }
        if (asciiCaseEqual(suffix, "rad")) {
            return CadQuantity{
                CadQuantityDimension::angle,
                scalar.canonical_value};
        }
        return std::nullopt;
    }

    [[nodiscard]] static std::optional<CadQuantity>
    multiply(CadQuantity left, CadQuantity right) noexcept {
        if (!left.finite() || !right.finite()) return std::nullopt;

        if (left.dimension == CadQuantityDimension::scalar) {
            left.canonical_value *= right.canonical_value;
            left.dimension = right.dimension;
        } else if (right.dimension == CadQuantityDimension::scalar) {
            left.canonical_value *= right.canonical_value;
        } else {
            return std::nullopt;
        }

        return left.finite()
                   ? std::optional<CadQuantity>{left}
                   : std::nullopt;
    }

    [[nodiscard]] static std::optional<CadQuantity>
    divide(CadQuantity left, CadQuantity right) noexcept {
        if (!left.finite() ||
            !right.finite() ||
            right.canonical_value == 0.0 ||
            right.dimension != CadQuantityDimension::scalar) {
            return std::nullopt;
        }

        left.canonical_value /= right.canonical_value;
        return left.finite()
                   ? std::optional<CadQuantity>{left}
                   : std::nullopt;
    }

    void skipSpaces() noexcept {
        while (position_ < text_.size() &&
               std::isspace(
                   static_cast<unsigned char>(text_[position_])) != 0) {
            ++position_;
        }
    }

    [[nodiscard]] bool consume(char expected) noexcept {
        if (position_ >= text_.size() || text_[position_] != expected) {
            return false;
        }
        ++position_;
        return true;
    }

    std::string_view text_;
    CadQuantityParseContext context_;
    std::size_t position_{};
};

} // namespace

bool CadQuantity::finite() const noexcept {
    return std::isfinite(canonical_value);
}

bool CadPointToken::finite() const noexcept {
    return std::isfinite(first) && std::isfinite(second);
}

std::optional<CadQuantity>
parseCadQuantity(
    std::string_view text,
    CadQuantityParseContext context) noexcept {
    text = trimAscii(text);
    if (text.empty()) return std::nullopt;
    if (context.expected_dimension ==
            CadQuantityDimension::length &&
        !core::isLengthUnit(
            context.length_unit)) {
        return std::nullopt;
    }
    QuantityParser parser{text, context};
    return parser.parse();
}

std::optional<CadPointToken>
parseCadPointToken(
    std::string_view text,
    core::LengthUnit length_unit) noexcept {
    text = trimAscii(text);
    if (text.empty()) return std::nullopt;

    const bool relative = text.front() == '@';
    if (relative) {
        text.remove_prefix(1U);
        text = trimAscii(text);
        if (text.empty()) return std::nullopt;
    }

    if (relative) {
        const auto polar_separator = text.find('<');
        if (polar_separator != std::string_view::npos) {
            if (text.find('<', polar_separator + 1U) !=
                std::string_view::npos) {
                return std::nullopt;
            }

            const auto distance = parseCadQuantity(
                text.substr(0U, polar_separator),
                {CadQuantityDimension::length, length_unit});
            const auto angle = parseCadQuantity(
                text.substr(polar_separator + 1U),
                {CadQuantityDimension::angle, length_unit});
            if (!distance ||
                !angle ||
                distance->canonical_value < 0.0) {
                return std::nullopt;
            }
            CadPointToken result{
                CadPointTokenKind::relative_polar,
                distance->canonical_value,
                angle->canonical_value};
            return result.finite()
                       ? std::optional<CadPointToken>{result}
                       : std::nullopt;
        }
    }

    const auto separator = text.find(';');
    if (separator == std::string_view::npos ||
        text.find(';', separator + 1U) != std::string_view::npos) {
        return std::nullopt;
    }

    const auto first = parseCadQuantity(
        text.substr(0U, separator),
        {CadQuantityDimension::length, length_unit});
    const auto second = parseCadQuantity(
        text.substr(separator + 1U),
        {CadQuantityDimension::length, length_unit});
    if (!first || !second) return std::nullopt;

    CadPointToken result{
        relative
            ? CadPointTokenKind::relative_cartesian
            : CadPointTokenKind::absolute_cartesian,
        first->canonical_value,
        second->canonical_value};
    return result.finite()
               ? std::optional<CadPointToken>{result}
               : std::nullopt;
}

} // namespace simplesolid2::application
