#pragma once

#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/persistence/atomic_file.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>

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

struct PartLoadResult final {
    std::optional<PartDocument> document;
    PartStoreDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept { return document.has_value(); }
};

struct PartSaveResult final {
    PartStoreDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == PartStoreErrorCode::none;
    }
};

class PartDocumentStore final {
public:
    static constexpr int current_schema_version = 3;

    [[nodiscard]] static bool hasNativeExtension(
        const std::filesystem::path& path) noexcept;

    [[nodiscard]] PartLoadResult load(
        const std::filesystem::path& path) const;

    [[nodiscard]] PartSaveResult createNew(
        const std::filesystem::path& path,
        const PartDocument& document) const;

    [[nodiscard]] PartSaveResult save(
        const std::filesystem::path& path,
        const PartDocument& document) const;
};

} // namespace simplesolid2::part
