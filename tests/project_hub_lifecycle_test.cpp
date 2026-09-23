#include "project_hub_controller.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace simplesolid2::application;
using namespace simplesolid2::application::internal;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-01 Hub lifecycle CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph01_hub_" +
                std::to_string(
                    std::filesystem::file_time_type::clock::now()
                        .time_since_epoch()
                        .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out << text;
    CHECK(static_cast<bool>(out));
}

void verifyCompleteProjectHubLifecycle(const std::filesystem::path& root) {
    const auto catalog = root / "user-state" / "recent-projects-v1.txt";
    const auto workspace = root / "Project A";
    std::filesystem::create_directories(workspace);
    writeText(workspace / "user-file.txt", "preserve me");

    std::string project_id;

    {
        ProjectHubController hub{catalog};
        CHECK(!hub.hasActiveProject());
        CHECK(hub.recentProjects().ok());
        CHECK(hub.recentProjects().entries.empty());

        const auto created = hub.createProject(workspace, "Hydraulic Press");
        CHECK(created.ok());
        CHECK(hub.hasActiveProject());
        CHECK(hub.activeSession() != nullptr);
        project_id = std::string{hub.activeSession()->projectId()};
        CHECK(ProjectWorkspaceMetadataService::isValidProjectId(project_id));
        CHECK(hub.activeSession()->displayName() == "Hydraulic Press");
        CHECK(std::filesystem::is_regular_file(workspace / "user-file.txt"));

        const auto recent = hub.recentProjects();
        CHECK(recent.ok());
        CHECK(recent.entries.size() == 1U);
        CHECK(recent.entries[0].project_id == project_id);

        hub.closeProject();
        CHECK(!hub.hasActiveProject());
    }

    ProjectHubController restarted{catalog};
    const auto after_restart = restarted.recentProjects();
    CHECK(after_restart.ok());
    CHECK(after_restart.entries.size() == 1U);
    CHECK(after_restart.entries[0].project_id == project_id);

    const auto reopened = restarted.openRecent(project_id);
    CHECK(reopened.ok());
    CHECK(restarted.hasActiveProject());
    CHECK(restarted.activeSession()->projectId() == project_id);
    restarted.closeProject();

    const auto moved = root / "Project A Moved";
    std::filesystem::rename(workspace, moved);

    const auto stale_recent = restarted.openRecent(project_id);
    CHECK(!stale_recent.ok());
    CHECK(stale_recent.diagnostic.code == ProjectHubErrorCode::session_failure);
    CHECK(!restarted.hasActiveProject());

    const auto other = root / "Other Project";
    std::filesystem::create_directories(other);
    ProjectWorkspaceMetadataService metadata_service;
    const auto other_metadata = metadata_service.initialize(other, "Other");
    CHECK(other_metadata.ok());
    CHECK(other_metadata.metadata->project_id != project_id);

    const auto wrong_relocation =
        restarted.relocateAndOpenRecent(project_id, other);
    CHECK(!wrong_relocation.ok());
    CHECK(wrong_relocation.diagnostic.code ==
          ProjectHubErrorCode::recent_failure);
    CHECK(wrong_relocation.diagnostic.recent_code ==
          RecentProjectErrorCode::project_id_mismatch);
    CHECK(!restarted.hasActiveProject());

    const auto relocated =
        restarted.relocateAndOpenRecent(project_id, moved);
    CHECK(relocated.ok());
    CHECK(restarted.hasActiveProject());
    CHECK(restarted.activeSession()->projectId() == project_id);
    CHECK(restarted.activeSession()->workspaceRoot() ==
          std::filesystem::weakly_canonical(moved));
    restarted.closeProject();

    const auto copy = root / "Filesystem Copy";
    std::filesystem::copy(
        moved,
        copy,
        std::filesystem::copy_options::recursive);

    const auto copied_metadata = metadata_service.load(copy);
    CHECK(copied_metadata.ok());
    CHECK(copied_metadata.metadata->project_id == project_id);

    const auto conflict = restarted.openProject(copy);
    CHECK(!conflict.ok());
    CHECK(conflict.diagnostic.code == ProjectHubErrorCode::recent_failure);
    CHECK(conflict.diagnostic.recent_code ==
          RecentProjectErrorCode::identity_conflict);
    CHECK(!restarted.hasActiveProject());

    const auto removed = restarted.removeRecent(project_id);
    CHECK(removed.ok());
    CHECK(restarted.recentProjects().ok());
    CHECK(restarted.recentProjects().entries.empty());

    const auto project_still_exists = metadata_service.load(moved);
    CHECK(project_still_exists.ok());
    CHECK(project_still_exists.metadata->project_id == project_id);

    const auto plain = root / "Plain Folder";
    std::filesystem::create_directories(plain);
    const auto plain_open = restarted.openProject(plain);
    CHECK(!plain_open.ok());
    CHECK(plain_open.diagnostic.code == ProjectHubErrorCode::session_failure);
    CHECK(plain_open.diagnostic.metadata_code ==
          ProjectMetadataErrorCode::not_initialized);
    CHECK(!std::filesystem::exists(plain / ".simplesolid"));
}

} // namespace

int main() {
    TempDirectory temp;
    verifyCompleteProjectHubLifecycle(temp.path);
    return EXIT_SUCCESS;
}
