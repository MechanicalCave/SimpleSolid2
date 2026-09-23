#include "project_hub_controller.hpp"

#include <algorithm>
#include <chrono>
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

bool resolveExistingParentLocation(
    const std::filesystem::path& input,
    std::filesystem::path& output) {
    if (input.empty()) return false;

    std::error_code ec;
    auto absolute = std::filesystem::absolute(input, ec);
    if (ec) return false;

    auto canonical = std::filesystem::weakly_canonical(absolute, ec);
    if (ec) return false;

    if (!std::filesystem::exists(canonical, ec) || ec) return false;
    if (!std::filesystem::is_directory(canonical, ec) || ec) return false;

    output = canonical.lexically_normal();
    return true;
}

bool isSingleProjectFolderName(const std::filesystem::path& folder) {
    if (folder.empty() || folder.is_absolute() || folder.has_root_path()) {
        return false;
    }

    if (!folder.parent_path().empty() || folder.filename() != folder) {
        return false;
    }

    const auto generic = folder.generic_u8string();
    if (generic == u8"." || generic == u8"..") return false;
    if (generic.find(u8'/') != std::u8string::npos ||
        generic.find(u8'\\') != std::u8string::npos) {
        return false;
    }

    return true;
}

void removeOwnedTree(const std::filesystem::path& path) noexcept {
    std::error_code ec;
    std::filesystem::remove_all(path, ec);
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

ProjectHubResult ProjectHubController::createProjectInLocation(
    const std::filesystem::path& parent_location,
    const std::filesystem::path& project_folder,
    std::string display_name) {
    if (hasActiveProject()) return activeProjectFailure();

    if (display_name.empty()) {
        return failure(
            ProjectHubErrorCode::metadata_failure,
            "Project name must not be empty",
            parent_location,
            ProjectMetadataErrorCode::invalid_display_name);
    }

    std::filesystem::path parent;
    if (!resolveExistingParentLocation(parent_location, parent)) {
        return failure(
            ProjectHubErrorCode::invalid_project_location,
            "Project Location must be an existing directory",
            parent_location);
    }

    if (!isSingleProjectFolderName(project_folder)) {
        return failure(
            ProjectHubErrorCode::invalid_project_folder,
            "Project folder must be one child folder name",
            project_folder);
    }

    const auto target = parent / project_folder;
    std::error_code ec;
    const bool target_exists = std::filesystem::exists(target, ec);
    if (ec) {
        return failure(
            ProjectHubErrorCode::project_creation_failure,
            "Unable to inspect requested Project folder",
            target);
    }
    if (target_exists) {
        return failure(
            ProjectHubErrorCode::project_folder_exists,
            "Requested Project folder already exists",
            target);
    }

    std::filesystem::path staging;
    const auto seed = std::chrono::high_resolution_clock::now()
                          .time_since_epoch()
                          .count();

    bool staging_created = false;
    for (unsigned attempt = 0; attempt < 64U; ++attempt) {
        staging = parent /
                  (".simplesolid-create-" +
                   std::to_string(seed) + "-" +
                   std::to_string(attempt));

        ec.clear();
        if (std::filesystem::create_directory(staging, ec)) {
            staging_created = true;
            break;
        }
        if (ec) {
            return failure(
                ProjectHubErrorCode::project_creation_failure,
                "Unable to create temporary Project Workspace",
                staging);
        }
    }

    if (!staging_created) {
        return failure(
            ProjectHubErrorCode::project_creation_failure,
            "Unable to reserve a temporary Project Workspace",
            parent);
    }

    auto initialized =
        metadata_service_.initialize(staging, std::move(display_name));
    if (!initialized.ok()) {
        removeOwnedTree(staging);
        return failure(
            ProjectHubErrorCode::metadata_failure,
            std::move(initialized.diagnostic.message),
            target,
            initialized.diagnostic.code);
    }

    ec.clear();
    if (std::filesystem::exists(target, ec)) {
        removeOwnedTree(staging);
        if (ec) {
            return failure(
                ProjectHubErrorCode::project_creation_failure,
                "Unable to recheck requested Project folder",
                target);
        }
        return failure(
            ProjectHubErrorCode::project_folder_exists,
            "Requested Project folder appeared during Project creation",
            target);
    }
    if (ec) {
        removeOwnedTree(staging);
        return failure(
            ProjectHubErrorCode::project_creation_failure,
            "Unable to recheck requested Project folder",
            target);
    }

    ec.clear();
    std::filesystem::rename(staging, target, ec);
    if (ec) {
        removeOwnedTree(staging);
        return failure(
            ProjectHubErrorCode::project_creation_failure,
            "Unable to publish the new Project Workspace",
            target);
    }

    auto adopted = adoptOpened(ProjectSession::open(target));
    if (!adopted.ok()) {
        removeOwnedTree(target);
        return adopted;
    }

    return success();
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
