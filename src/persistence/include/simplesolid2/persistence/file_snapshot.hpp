#pragma once

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::persistence {

using Sha256Digest = std::array<std::uint8_t, 32>;

[[nodiscard]] Sha256Digest sha256(
    std::string_view bytes) noexcept;

[[nodiscard]] std::string sha256Hex(
    const Sha256Digest& digest);

struct FileIdentity final {
    std::uint64_t volume{};
    std::array<std::uint8_t, 16> object{};

    friend bool operator==(
        const FileIdentity&,
        const FileIdentity&) = default;
};

enum class FileSnapshotErrorCode {
    none,
    not_found,
    not_regular,
    too_large,
    io_failure,
};

struct FileSnapshotDiagnostic final {
    FileSnapshotErrorCode code{
        FileSnapshotErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct FileSnapshot final {
    std::string bytes;
    FileIdentity identity;
    Sha256Digest digest{};
};

struct FileSnapshotResult final {
    std::optional<FileSnapshot> snapshot;
    FileSnapshotDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return snapshot.has_value();
    }
};

[[nodiscard]] FileSnapshotResult readFileSnapshot(
    const std::filesystem::path& path,
    std::uintmax_t maximum_bytes);

} // namespace simplesolid2::persistence
