#include "project_hub_controller.hpp"

#include <algorithm>
#include <utility>

namespace simplesolid2::application::internal {
namespace {

ProjectHubResult failure(
    ProjectHubErrorCode code,
    std::string message,
    std::filesystem::path path = {},
    ProjectMetadataErrorCode metadata_code = ProjectMetadataErrorCode::none,
    ProjectSessionErrorCode session_code = ProjectSessionErrorCode::none,
    RecentProjectErrorCode recent_code = RecentProjectErrorCode::none) {
    return ProjectHubResult{
        ProjectHubDiagnostic{
            code,
            metadata_code,
            session_code,
            recent_code,
            std::move(message),
            std::move(path),
        },
    };
}

ProjectHubResult success() {
    return ProjectHubResult{};
}

ProjectHubResult activeProjectFailure() {
    return failure(
        ProjectHubErrorCode::active_project_exists,
        "Close the active Project before opening or creating another Project");
}

ProjectHubResult recentFailure(const RecentProjectDiagnostic& diagnostic) {
    return failure(
        ProjectHubErrorCode::recent_failure,
        diagnostic.message,
        diagnostic.path,
        diagnostic.metadata_code,
        ProjectSessionErrorCode::none,
        diagnostic.code);
}

ProjectHubResult sessionFailure(const ProjectSessionDiagnostic& diagnostic) {
    return failure(
        ProjectHubErrorCode::session_failure,
        diagnostic.message,
        diagnostic.path,
        diagnostic.metadata_code,
        diagnostic.code);
}

} // namespace

RecentProjectHubListResult ProjectHubController::recentProjectHubEntries() const {
    auto listed = recent_.list();
    if (!listed.ok()) {
        return RecentProjectHubListResult{{}, std::move(listed.diagnostic)};
    }

    std::vector<RecentProjectHubEntry> result;
    result.reserve(listed.entries.size());

    for (const auto& entry : listed.entries) {
        RecentProjectHubEntry view;
        view.recent = entry;

        std::error_code ec;
        const bool exists = std::filesystem::exists(entry.workspace_root, ec);
        if (ec) {
            view.availability = RecentProjectAvailability::project_invalid;
            view.diagnostic =
                "Unable to inspect the remembered Project Workspace location";
            result.push_back(std::move(view));
            continue;
        }

        if (!exists) {
            view.availability = RecentProjectAvailability::workspace_missing;
            view.diagnostic =
                "Workspace not found. Use Locate to find the moved Project.";
            result.push_back(std::move(view));
            continue;
        }

        const bool is_directory =
            std::filesystem::is_directory(entry.workspace_root, ec);
        if (ec || !is_directory) {
            view.availability = RecentProjectAvailability::project_invalid;
            view.diagnostic =
                "Remembered Project Workspace location is not a directory";
            result.push_back(std::move(view));
            continue;
        }

        auto loaded = metadata_service_.load(entry.workspace_root);
        if (!loaded.ok()) {
            view.availability = RecentProjectAvailability::project_invalid;
            view.metadata_code = loaded.diagnostic.code;
            view.diagnostic = std::move(loaded.diagnostic.message);
            result.push_back(std::move(view));
            continue;
        }

        if (loaded.metadata->project_id != entry.project_id) {
            view.availability = RecentProjectAvailability::identity_mismatch;
            view.diagnostic =
                "Remembered Workspace contains a different ProjectId";
            result.push_back(std::move(view));
            continue;
        }

        view.availability = RecentProjectAvailability::available;
        result.push_back(std::move(view));
    }

    return RecentProjectHubListResult{
        std::move(result),
        RecentProjectDiagnostic{},
    };
}

ProjectHubResult ProjectHubController::createProject(
    const std::filesystem::path& workspace_root,
    std::string display_name) {
    if (hasActiveProject()) return activeProjectFailure();

    auto initialized = metadata_service_.initialize(
        workspace_root,
        std::move(display_name));
    if (!initialized.ok()) {
        return failure(
            ProjectHubErrorCode::metadata_failure,
            std::move(initialized.diagnostic.message),
            std::move(initialized.diagnostic.path),
            initialized.diagnostic.code);
    }

    return adoptOpened(ProjectSession::open(workspace_root));
}

ProjectHubResult ProjectHubController::openProject(
    const std::filesystem::path& workspace_root) {
    if (hasActiveProject()) return activeProjectFailure();
    return adoptOpened(ProjectSession::open(workspace_root));
}

ProjectHubResult ProjectHubController::openRecent(
    std::string_view project_id) {
    if (hasActiveProject()) return activeProjectFailure();

    auto listed = recent_.list();
    if (!listed.ok()) return recentFailure(listed.diagnostic);

    const auto found = std::find_if(
        listed.entries.begin(),
        listed.entries.end(),
        [&](const RecentProjectEntry& entry) {
            return entry.project_id == project_id;
        });
    if (found == listed.entries.end()) {
        return failure(
            ProjectHubErrorCode::recent_entry_not_found,
            "Recent Project entry no longer exists");
    }

    auto opened = ProjectSession::open(found->workspace_root);
    if (!opened.ok()) return sessionFailure(opened.diagnostic);

    if (opened.session->projectId() != found->project_id) {
        return failure(
            ProjectHubErrorCode::recent_identity_mismatch,
            "Recent Project location now contains a different ProjectId",
            found->workspace_root);
    }

    return adoptOpened(std::move(opened));
}

ProjectHubResult ProjectHubController::relocateAndOpenRecent(
    std::string_view project_id,
    const std::filesystem::path& replacement_root) {
    if (hasActiveProject()) return activeProjectFailure();

    const auto relocated = recent_.relocate(project_id, replacement_root);
    if (!relocated.ok()) return recentFailure(relocated.diagnostic);

    return openRecent(project_id);
}

ProjectHubResult ProjectHubController::removeRecent(
    std::string_view project_id) {
    const auto removed = recent_.remove(project_id);
    if (!removed.ok()) return recentFailure(removed.diagnostic);
    return success();
}

ProjectHubResult ProjectHubController::adoptOpened(
    ProjectSessionOpenResult opened) {
    if (!opened.ok()) return sessionFailure(opened.diagnostic);

    const auto recorded = recent_.recordOpened(*opened.session);
    if (!recorded.ok()) return recentFailure(recorded.diagnostic);

    active_session_.emplace(std::move(*opened.session));
    return success();
}

} // namespace simplesolid2::application::internal
