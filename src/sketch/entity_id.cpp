#include <simplesolid2/sketch/entity_id.hpp>

#include <limits>

namespace simplesolid2::sketch {
namespace {

std::optional<std::uint64_t>
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
            static_cast<std::uint64_t>(
                ch - '0');

        if (value >
            (std::numeric_limits<std::uint64_t>::max() -
             digit) /
                10U) {
            return std::nullopt;
        }

        value = value * 10U + digit;
    }

    if (value == 0U) {
        return std::nullopt;
    }

    return value;
}

} // namespace

std::optional<EntityId> EntityId::parse(
    std::string_view serialized) noexcept {
    const auto value =
        parseCanonicalPositiveDecimal(serialized);
    if (!value) {
        return std::nullopt;
    }
    return EntityId{*value};
}

std::string EntityId::serialized() const {
    if (!valid()) {
        return {};
    }
    return std::to_string(value_);
}

std::optional<EntityIdCursor>
EntityIdCursor::parse(
    std::string_view serialized) noexcept {
    const auto value =
        parseCanonicalPositiveDecimal(serialized);
    if (!value) {
        return std::nullopt;
    }
    return EntityIdCursor{*value};
}

std::string EntityIdCursor::serialized() const {
    return std::to_string(next_value_);
}

} // namespace simplesolid2::sketch
