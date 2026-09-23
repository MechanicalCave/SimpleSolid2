#pragma once

#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace simplesolid2::core {

enum class DocumentKind {
    part,
};

class DocumentId final {
public:
    [[nodiscard]] static DocumentId generate();
    [[nodiscard]] static std::optional<DocumentId> parse(std::string_view serialized);

    [[nodiscard]] std::string_view value() const noexcept { return value_; }

    friend bool operator==(const DocumentId&, const DocumentId&) = default;
    friend auto operator<=>(const DocumentId&, const DocumentId&) = default;

private:
    explicit DocumentId(std::string value) : value_{std::move(value)} {}

    std::string value_;
};

class DocumentRevision final {
public:
    constexpr DocumentRevision() noexcept = default;
    explicit constexpr DocumentRevision(std::uint64_t value) noexcept : value_{value} {}

    [[nodiscard]] constexpr std::uint64_t value() const noexcept { return value_; }
    [[nodiscard]] std::optional<DocumentRevision> next() const noexcept;

    friend bool operator==(const DocumentRevision&, const DocumentRevision&) = default;
    friend auto operator<=>(const DocumentRevision&, const DocumentRevision&) = default;

private:
    std::uint64_t value_{0};
};

struct DocumentProperties final {
    std::string number;
    std::string title;
    std::string description;
    std::string engineering_revision;

    friend bool operator==(const DocumentProperties&, const DocumentProperties&) = default;
};

} // namespace simplesolid2::core
