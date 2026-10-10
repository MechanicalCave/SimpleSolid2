#pragma once

#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/persistence/atomic_file.hpp>
#include <simplesolid2/persistence/file_guard.hpp>
#include <simplesolid2/persistence/file_snapshot.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace simplesolid2::part {

enum class PartStoreErrorCode {
    none,
    invalid_path,
    wrong_extension,
    target_exists,
    not_found,
    io_failure,
    malformed_document,
    missing_field,
    unsupported_schema,
    wrong_document_kind,
    invalid_document_id,
    container_failure,
    save_conflict_busy,
    save_conflict_target_missing,
    save_conflict_document_identity_changed,
    save_conflict_file_replaced,
    save_conflict_content_changed,
};

struct PartStoreDiagnostic final {
    PartStoreErrorCode code{PartStoreErrorCode::none};
    persistence::AtomicWriteErrorCode atomic_code{
        persistence::AtomicWriteErrorCode::none};
    persistence::NativeContainerErrorCode container_code{
        persistence::NativeContainerErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct PartFileCheckpoint final {
    core::DocumentId document_id;
    std::uint64_t byte_length{};
    persistence::Sha256Digest digest{};
    persistence::FileIdentity file_identity;

    friend bool operator==(
        const PartFileCheckpoint&,
        const PartFileCheckpoint&) = default;
};

struct PartLoadResult final {
    std::optional<PartDocument> document;
    std::optional<PartFileCheckpoint> checkpoint;
    PartStoreDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return document.has_value() &&
               checkpoint.has_value() &&
               diagnostic.code ==
                   PartStoreErrorCode::none;
    }
};

struct PartSaveResult final {
    std::optional<PartFileCheckpoint> checkpoint;
    PartStoreDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return checkpoint.has_value() &&
               diagnostic.code ==
                   PartStoreErrorCode::none;
    }
};

[[nodiscard]] bool isSaveConflict(
    PartStoreErrorCode code) noexcept;

class PartDocumentStore final {
public:
    static constexpr int current_schema_version = 16;

    [[nodiscard]] static bool hasNativeExtension(
        const std::filesystem::path& path) noexcept;

    [[nodiscard]] PartLoadResult load(
        const std::filesystem::path& path) const;

    [[nodiscard]] PartSaveResult createNew(
        const std::filesystem::path& path,
        const PartDocument& document) const;

    [[nodiscard]] PartSaveResult save(
        const std::filesystem::path& path,
        const PartDocument& document,
        const PartFileCheckpoint& expected_checkpoint) const;
};

} // namespace simplesolid2::part
