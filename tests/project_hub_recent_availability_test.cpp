#include "project_hub_controller.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2::application;
using namespace simplesolid2::application::internal;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-02B availability CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph02b_availability_" +
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

void verifyAvailabilityIsDerivedAndNonPersistent(
    const std::filesystem::path& root) {
    const auto catalog = root / "state" / "recent-projects-v1.txt";
    const auto workspace = root / "Remembered Project";
    std::filesystem::create_directories(workspace);

    ProjectHubController hub{catalog};
    const auto created = hub.createProject(workspace, "Remembered Project");
    CHECK(created.ok());
    CHECK(hub.activeSession() != nullptr);
    const std::string expected_id{hub.activeSession()->projectId()};
    hub.closeProject();

    const auto assert_recent_unchanged = [&] {
        const auto recent = hub.recentProjects();
        CHECK(recent.ok());
        CHECK(recent.entries.size() == 1U);
        CHECK(recent.entries[0].project_id == expected_id);
        CHECK(recent.entries[0].workspace_root ==
              std::filesystem::weakly_canonical(workspace));
    };

    {
        const auto views = hub.recentProjectHubEntries();
        CHECK(views.ok());
        CHECK(views.entries.size() == 1U);
        CHECK(views.entries[0].recent.project_id == expected_id);
        CHECK(views.entries[0].availability ==
              RecentProjectAvailability::available);
        CHECK(views.entries[0].openable());
        assert_recent_unchanged();
    }

    const auto moved = root / "Moved Project";
    std::filesystem::rename(workspace, moved);

    {
        const auto views = hub.recentProjectHubEntries();
        CHECK(views.ok());
        CHECK(views.entries.size() == 1U);
        CHECK(views.entries[0].availability ==
              RecentProjectAvailability::workspace_missing);
        CHECK(!views.entries[0].openable());
        CHECK(!views.entries[0].diagnostic.empty());
        assert_recent_unchanged();
    }

    std::filesystem::create_directories(workspace);

    {
        const auto views = hub.recentProjectHubEntries();
        CHECK(views.ok());
        CHECK(views.entries.size() == 1U);
        CHECK(views.entries[0].availability ==
              RecentProjectAvailability::project_invalid);
        CHECK(views.entries[0].metadata_code ==
              ProjectMetadataErrorCode::not_initialized);
        CHECK(!views.entries[0].openable());
        assert_recent_unchanged();
    }

    std::filesystem::remove_all(workspace);

    const auto other = root / "Different Project";
    std::filesystem::create_directories(other);
    ProjectWorkspaceMetadataService metadata_service;
    const auto other_created =
        metadata_service.initialize(other, "Different Project");
    CHECK(other_created.ok());
    CHECK(other_created.metadata->project_id != expected_id);
    std::filesystem::rename(other, workspace);

    {
        const auto views = hub.recentProjectHubEntries();
        CHECK(views.ok());
        CHECK(views.entries.size() == 1U);
        CHECK(views.entries[0].availability ==
              RecentProjectAvailability::identity_mismatch);
        CHECK(!views.entries[0].openable());
        CHECK(!views.entries[0].diagnostic.empty());
        assert_recent_unchanged();
    }

    const auto moved_metadata = metadata_service.load(moved);
    CHECK(moved_metadata.ok());
    CHECK(moved_metadata.metadata->project_id == expected_id);
}

} // namespace

int main() {
    TempDirectory temp;
    verifyAvailabilityIsDerivedAndNonPersistent(temp.path);
    return EXIT_SUCCESS;
}
