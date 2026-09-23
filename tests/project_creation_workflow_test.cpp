#include "project_hub_controller.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

using namespace simplesolid2::application;
using namespace simplesolid2::application::internal;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-02C Project Creation CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph02c_creation_" +
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

void writeText(const std::filesystem::path& path, std::string_view text) {
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out << text;
    CHECK(static_cast<bool>(out));
}

bool containsCreationStaging(const std::filesystem::path& parent) {
    std::error_code ec;
    if (!std::filesystem::is_directory(parent, ec) || ec) return false;

    for (const auto& entry : std::filesystem::directory_iterator(parent)) {
        const auto name = entry.path().filename().generic_u8string();
        if (name.starts_with(u8".simplesolid-create-")) return true;
    }
    return false;
}

void verifySuccessfulCreation(const std::filesystem::path& root) {
    const auto parent = root / "Projects";
    const auto catalog = root / "state" / "recent-projects-v1.txt";
    std::filesystem::create_directories(parent);

    ProjectHubController hub{catalog};
    const auto created = hub.createProjectInLocation(
        parent,
        "PressWorkspace",
        "Hydraulic Press");

    CHECK(created.ok());
    CHECK(hub.hasActiveProject());
    CHECK(hub.activeSession() != nullptr);

    const auto target = parent / "PressWorkspace";
    const auto canonical_target = std::filesystem::weakly_canonical(target);

    CHECK(std::filesystem::is_directory(target));
    CHECK(std::filesystem::is_regular_file(
        target / ".simplesolid" / "project.json"));
    CHECK(!containsCreationStaging(parent));

    ProjectWorkspaceMetadataService metadata_service;
    const auto metadata = metadata_service.load(target);
    CHECK(metadata.ok());
    CHECK(metadata.metadata->display_name == "Hydraulic Press");
    CHECK(metadata.metadata->project_id == hub.activeSession()->projectId());
    CHECK(hub.activeSession()->displayName() == "Hydraulic Press");
    CHECK(hub.activeSession()->workspaceRoot() == canonical_target);

    const auto recent = hub.recentProjects();
    CHECK(recent.ok());
    CHECK(recent.entries.size() == 1U);
    CHECK(recent.entries[0].project_id == metadata.metadata->project_id);
    CHECK(recent.entries[0].display_name == "Hydraulic Press");
    CHECK(recent.entries[0].workspace_root == canonical_target);

    hub.closeProject();
}

void verifyExistingTargetIsPreserved(const std::filesystem::path& root) {
    const auto parent = root / "Existing Target Parent";
    const auto target = parent / "Existing";
    const auto sentinel = target / "do-not-touch.txt";
    std::filesystem::create_directories(target);
    writeText(sentinel, "owner data");

    ProjectHubController hub{
        root / "existing-state" / "recent-projects-v1.txt"};
    const auto result = hub.createProjectInLocation(
        parent,
        "Existing",
        "Should Not Exist");

    CHECK(!result.ok());
    CHECK(result.diagnostic.code == ProjectHubErrorCode::project_folder_exists);
    CHECK(!hub.hasActiveProject());
    CHECK(std::filesystem::is_regular_file(sentinel));
    CHECK(!std::filesystem::exists(target / ".simplesolid"));
    CHECK(!containsCreationStaging(parent));
}

void verifyInvalidFolderInputsFailClosed(const std::filesystem::path& root) {
    const auto parent = root / "Invalid Folder Parent";
    std::filesystem::create_directories(parent);

    ProjectHubController hub{
        root / "invalid-state" / "recent-projects-v1.txt"};

    const std::filesystem::path invalid_folders[] = {
        {},
        ".",
        "..",
        std::filesystem::path{"nested"} / "child",
        root / "AbsoluteOutside",
    };

    for (const auto& folder : invalid_folders) {
        const auto result =
            hub.createProjectInLocation(parent, folder, "Invalid Folder");
        CHECK(!result.ok());
        CHECK(result.diagnostic.code ==
              ProjectHubErrorCode::invalid_project_folder);
        CHECK(!hub.hasActiveProject());
        CHECK(!containsCreationStaging(parent));
    }

    CHECK(!std::filesystem::exists(parent / "nested"));
    CHECK(!std::filesystem::exists(root / "AbsoluteOutside"));
}

void verifyInvalidParentIsNotCreated(const std::filesystem::path& root) {
    const auto missing_parent = root / "Missing Parent";

    ProjectHubController hub{
        root / "parent-state" / "recent-projects-v1.txt"};
    const auto missing = hub.createProjectInLocation(
        missing_parent,
        "NewProject",
        "New Project");

    CHECK(!missing.ok());
    CHECK(missing.diagnostic.code ==
          ProjectHubErrorCode::invalid_project_location);
    CHECK(!std::filesystem::exists(missing_parent));

    const auto file_parent = root / "Parent Is File";
    writeText(file_parent, "not a directory");

    const auto not_directory = hub.createProjectInLocation(
        file_parent,
        "NewProject",
        "New Project");

    CHECK(!not_directory.ok());
    CHECK(not_directory.diagnostic.code ==
          ProjectHubErrorCode::invalid_project_location);
    CHECK(std::filesystem::is_regular_file(file_parent));
}

void verifyRollbackWhenRecentCannotBeRecorded(
    const std::filesystem::path& root) {
    const auto parent = root / "Rollback Projects";
    std::filesystem::create_directories(parent);

    const auto bad_state_parent = root / "State Is File";
    writeText(bad_state_parent, "not a directory");

    ProjectHubController hub{
        bad_state_parent / "recent-projects-v1.txt"};

    const auto target = parent / "RollbackProject";
    const auto result = hub.createProjectInLocation(
        parent,
        "RollbackProject",
        "Rollback Project");

    CHECK(!result.ok());
    CHECK(result.diagnostic.code == ProjectHubErrorCode::recent_failure);
    CHECK(!hub.hasActiveProject());
    CHECK(!std::filesystem::exists(target));
    CHECK(!containsCreationStaging(parent));
    CHECK(std::filesystem::is_regular_file(bad_state_parent));
}

} // namespace

int main() {
    TempDirectory temp;
    verifySuccessfulCreation(temp.path);
    verifyExistingTargetIsPreserved(temp.path);
    verifyInvalidFolderInputsFailClosed(temp.path);
    verifyInvalidParentIsNotCreated(temp.path);
    verifyRollbackWhenRecentCannotBeRecorded(temp.path);
    return EXIT_SUCCESS;
}
