#pragma once

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/application/recent_project_store.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::application::internal {

enum class ProjectHubErrorCode {
    none,
    active_project_exists,
    metadata_failure,
    session_failure,
    recent_failure,
    recent_entry_not_found,
    recent_identity_mismatch,
};

struct ProjectHubDiagnostic final {
    ProjectHubErrorCode code{ProjectHubErrorCode::none};
    ProjectMetadataErrorCode metadata_code{ProjectMetadataErrorCode::none};
    ProjectSessionErrorCode session_code{ProjectSessionErrorCode::none};
    RecentProjectErrorCode recent_code{RecentProjectErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct ProjectHubResult final {
    ProjectHubDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == ProjectHubErrorCode::none;
    }
};

class ProjectHubController final {
public:
    explicit ProjectHubController(std::filesystem::path recent_catalog_path)
        : recent_{std::move(recent_catalog_path)} {}

    [[nodiscard]] bool hasActiveProject() const noexcept {
        return active_session_.has_value();
    }

    [[nodiscard]] const ProjectSession* activeSession() const noexcept {
        return active_session_ ? &*active_session_ : nullptr;
    }

    [[nodiscard]] RecentProjectListResult recentProjects() const {
        return recent_.list();
    }

    [[nodiscard]] ProjectHubResult createProject(
        const std::filesystem::path& workspace_root,
        std::string display_name);

    [[nodiscard]] ProjectHubResult openProject(
        const std::filesystem::path& workspace_root);

    [[nodiscard]] ProjectHubResult openRecent(
        std::string_view project_id);

    [[nodiscard]] ProjectHubResult relocateAndOpenRecent(
        std::string_view project_id,
        const std::filesystem::path& replacement_root);

    [[nodiscard]] ProjectHubResult removeRecent(
        std::string_view project_id);

    void closeProject() noexcept {
        active_session_.reset();
    }

private:
    [[nodiscard]] ProjectHubResult adoptOpened(ProjectSessionOpenResult opened);

    ProjectWorkspaceMetadataService metadata_service_{};
    RecentProjectStore recent_;
    std::optional<ProjectSession> active_session_;
};

} // namespace simplesolid2::application::internal
