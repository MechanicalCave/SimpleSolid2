#include <simplesolid2/core/document.hpp>

#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>

namespace simplesolid2::core {
namespace {

bool isHexDigit(char ch) noexcept {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
}

bool isUuidV4(std::string_view value) noexcept {
    if (value.size() != 36U) return false;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (i == 8U || i == 13U || i == 18U || i == 23U) {
            if (value[i] != '-') return false;
        } else if (!isHexDigit(value[i])) {
            return false;
        }
    }
    if (value[14] != '4') return false;
    const char variant = value[19];
    return variant == '8' || variant == '9' ||
           variant == 'a' || variant == 'A' ||
           variant == 'b' || variant == 'B';
}

std::string makeUuidV4() {
    std::random_device rd;
    std::seed_seq seed{
        rd(), rd(), rd(), rd(),
        static_cast<unsigned>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count()),
    };
    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<unsigned> dist{0U, 255U};

    std::array<std::uint8_t, 16> bytes{};
    for (auto& byte : bytes) {
        byte = static_cast<std::uint8_t>(dist(engine));
    }
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0FU) | 0x40U);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3FU) | 0x80U);

    std::ostringstream out;
    out << std::hex << std::nouppercase << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        out << std::setw(2) << static_cast<unsigned>(bytes[index]);
        if (index == 3U || index == 5U || index == 7U || index == 9U) {
            out << '-';
        }
    }
    return out.str();
}

} // namespace

DocumentId DocumentId::generate() {
    return DocumentId{makeUuidV4()};
}

std::optional<DocumentId> DocumentId::parse(std::string_view serialized) {
    if (!isUuidV4(serialized)) return std::nullopt;
    return DocumentId{std::string{serialized}};
}

std::optional<DocumentRevision> DocumentRevision::next() const noexcept {
    if (value_ == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }
    return DocumentRevision{value_ + 1U};
}

} // namespace simplesolid2::core
