#pragma once

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace simplesolid2::application {

struct ProjectSessionOpenResult;

enum class ProjectSessionErrorCode {
    none,
    project_validation_failed,
    workspace_resolution_failed,
};

struct ProjectSessionDiagnostic final {
    ProjectSessionErrorCode code{ProjectSessionErrorCode::none};
    ProjectMetadataErrorCode metadata_code{ProjectMetadataErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

class ProjectSession final {
public:
    ProjectSession(const ProjectSession&) = delete;
    ProjectSession& operator=(const ProjectSession&) = delete;
    ProjectSession(ProjectSession&&) noexcept = default;
    ProjectSession& operator=(ProjectSession&&) noexcept = default;
    ~ProjectSession() = default;

    [[nodiscard]] static ProjectSessionOpenResult open(
        const std::filesystem::path& workspace_root);

    [[nodiscard]] const std::filesystem::path& workspaceRoot() const noexcept {
        return workspace_root_;
    }

    [[nodiscard]] const ProjectWorkspaceMetadata& metadata() const noexcept {
        return metadata_;
    }

    [[nodiscard]] std::string_view projectId() const noexcept {
        return metadata_.project_id;
    }

    [[nodiscard]] std::string_view displayName() const noexcept {
        return metadata_.display_name;
    }

private:
    ProjectSession(std::filesystem::path workspace_root, ProjectWorkspaceMetadata metadata)
        : workspace_root_{std::move(workspace_root)},
          metadata_{std::move(metadata)} {}

    std::filesystem::path workspace_root_;
    ProjectWorkspaceMetadata metadata_;
};

struct ProjectSessionOpenResult final {
    std::optional<ProjectSession> session;
    ProjectSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept { return session.has_value(); }
};

} // namespace simplesolid2::application
