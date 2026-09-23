#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::application {

struct ProjectWorkspaceMetadata final {
    std::string project_id;
    std::string display_name;
    int schema_version{1};
    std::string created_at;
};

enum class ProjectMetadataErrorCode {
    none,
    invalid_workspace,
    already_initialized,
    not_initialized,
    io_failure,
    malformed_metadata,
    missing_field,
    unsupported_schema,
    invalid_project_id,
    invalid_display_name,
    invalid_created_at,
};

struct ProjectMetadataDiagnostic final {
    ProjectMetadataErrorCode code{ProjectMetadataErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct ProjectMetadataResult final {
    std::optional<ProjectWorkspaceMetadata> metadata;
    ProjectMetadataDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept { return metadata.has_value(); }
};

class ProjectWorkspaceMetadataService final {
public:
    static constexpr int current_schema_version = 1;

    [[nodiscard]] static std::filesystem::path metadataDirectory(
        const std::filesystem::path& workspace_root);
    [[nodiscard]] static std::filesystem::path metadataPath(
        const std::filesystem::path& workspace_root);

    [[nodiscard]] static bool isValidProjectId(std::string_view project_id) noexcept;

    [[nodiscard]] bool isInitialized(const std::filesystem::path& workspace_root) const noexcept;

    [[nodiscard]] ProjectMetadataResult initialize(
        const std::filesystem::path& workspace_root,
        std::string display_name) const;

    [[nodiscard]] ProjectMetadataResult load(
        const std::filesystem::path& workspace_root) const;
};

} // namespace simplesolid2::application
