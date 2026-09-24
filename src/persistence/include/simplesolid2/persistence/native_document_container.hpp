#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::persistence {

inline constexpr int native_document_container_version = 1;
inline constexpr std::string_view native_document_format =
    "simplesolid.native-document";

struct NativeDocumentDescriptor final {
    std::string document_kind;
    std::string document_id;
    int domain_schema_version{};
};

struct NativeDocumentPackage final {
    NativeDocumentDescriptor descriptor;
    std::string authored_json;
};

enum class NativeContainerErrorCode {
    none,
    not_found,
    io_failure,
    too_large,
    malformed_container,
    unsupported_zip_feature,
    unsafe_entry,
    duplicate_entry,
    missing_manifest,
    invalid_manifest,
    unsupported_container_version,
    missing_authored_payload,
    invalid_authored_json,
};

struct NativeContainerDiagnostic final {
    NativeContainerErrorCode code{NativeContainerErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct NativeContainerReadResult final {
    std::optional<NativeDocumentPackage> package;
    NativeContainerDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return package.has_value();
    }
};

struct NativeContainerBuildResult final {
    std::optional<std::string> bytes;
    NativeContainerDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return bytes.has_value();
    }
};

[[nodiscard]] NativeContainerReadResult readNativeDocumentContainer(
    const std::filesystem::path& path);

[[nodiscard]] NativeContainerBuildResult buildNativeDocumentContainer(
    const NativeDocumentDescriptor& descriptor,
    std::string_view authored_json);

} // namespace simplesolid2::persistence
