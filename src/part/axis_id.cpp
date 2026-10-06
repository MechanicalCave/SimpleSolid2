#include <simplesolid2/part/axis_id.hpp>

#include <limits>

namespace simplesolid2::part {
namespace {

[[nodiscard]] std::optional<std::uint64_t>
parseCanonicalPositiveDecimal(
    std::string_view serialized) noexcept {
    if (serialized.empty() ||
        serialized.front() == '0') {
        return std::nullopt;
    }

    std::uint64_t value{0U};
    for (const char ch : serialized) {
        if (ch < '0' || ch > '9') {
            return std::nullopt;
        }

        const auto digit =
            static_cast<std::uint64_t>(ch - '0');
        if (value >
            (std::numeric_limits<std::uint64_t>::max() -
             digit) /
                10U) {
            return std::nullopt;
        }
        value = value * 10U + digit;
    }

    return value == 0U
        ? std::nullopt
        : std::optional<std::uint64_t>{value};
}

} // namespace

std::optional<AxisId> AxisId::parse(
    std::string_view serialized) noexcept {
    const auto value =
        parseCanonicalPositiveDecimal(serialized);
    return value
        ? std::optional<AxisId>{AxisId{*value}}
        : std::nullopt;
}

std::string AxisId::serialized() const {
    return valid()
        ? std::to_string(value_)
        : std::string{};
}

std::optional<AxisIdCursor> AxisIdCursor::parse(
    std::string_view serialized) noexcept {
    const auto value =
        parseCanonicalPositiveDecimal(serialized);
    return value
        ? std::optional<AxisIdCursor>{
              AxisIdCursor{*value}}
        : std::nullopt;
}

std::string AxisIdCursor::serialized() const {
    return std::to_string(next_value_);
}

std::optional<AxisId>
AxisIdCursor::allocate() noexcept {
    if (next_value_ ==
        std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }

    const AxisId result{next_value_};
    ++next_value_;
    return result;
}

void AxisIdCursor::preserve(
    AxisIdCursor observed) noexcept {
    if (observed.next_value_ > next_value_) {
        next_value_ = observed.next_value_;
    }
}

} // namespace simplesolid2::part
