#include <simplesolid2/part/body_id.hpp>

#include <limits>

namespace simplesolid2::part {
namespace {

[[nodiscard]] std::optional<std::uint64_t>
parseCanonicalPositiveDecimal(std::string_view serialized) noexcept {
    if (serialized.empty() || serialized.front() == '0') {
        return std::nullopt;
    }

    std::uint64_t value{0U};
    for (const char ch : serialized) {
        if (ch < '0' || ch > '9') return std::nullopt;
        const auto digit = static_cast<std::uint64_t>(ch - '0');
        if (value >
            (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
            return std::nullopt;
        }
        value = value * 10U + digit;
    }
    return value == 0U ? std::nullopt
                       : std::optional<std::uint64_t>{value};
}

} // namespace

std::optional<BodyId> BodyId::parse(
    std::string_view serialized) noexcept {
    const auto value = parseCanonicalPositiveDecimal(serialized);
    return value ? std::optional<BodyId>{BodyId{*value}} : std::nullopt;
}

std::string BodyId::serialized() const {
    return valid() ? std::to_string(value_) : std::string{};
}

std::optional<BodyIdCursor> BodyIdCursor::parse(
    std::string_view serialized) noexcept {
    const auto value = parseCanonicalPositiveDecimal(serialized);
    return value
        ? std::optional<BodyIdCursor>{BodyIdCursor{*value}}
        : std::nullopt;
}

std::string BodyIdCursor::serialized() const {
    return std::to_string(next_value_);
}

std::optional<BodyId> BodyIdCursor::allocate() noexcept {
    if (next_value_ == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }
    const BodyId result{next_value_};
    ++next_value_;
    return result;
}

void BodyIdCursor::preserve(BodyIdCursor observed) noexcept {
    if (observed.next_value_ > next_value_) {
        next_value_ = observed.next_value_;
    }
}

} // namespace simplesolid2::part
