#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/application/recent_project_store.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace simplesolid2::application;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-01 Recent Projects CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph01_recent_" +
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
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out{path, std::ios::binary | std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out << text;
    CHECK(static_cast<bool>(out));
}

std::string initializeProject(
    const std::filesystem::path& workspace,
    const std::string& display_name) {
    std::filesystem::create_directories(workspace);
    ProjectWorkspaceMetadataService metadata_service;
    const auto initialized = metadata_service.initialize(workspace, display_name);
    CHECK(initialized.ok());
    return initialized.metadata->project_id;
}

ProjectSession openProject(const std::filesystem::path& workspace) {
    auto opened = ProjectSession::open(workspace);
    CHECK(opened.ok());
    CHECK(opened.session.has_value());
    return std::move(*opened.session);
}

void verifyPersistenceRestartAndReopen(const std::filesystem::path& root) {
    const auto workspace_a = root / "Project A";
    const auto workspace_b = root / "Project B";
    const auto project_a_id = initializeProject(workspace_a, "Hydraulic Press");
    const auto project_b_id = initializeProject(workspace_b, "Fixture");

    const auto catalog = root / "user-state" / "recent-projects-v1.txt";
    RecentProjectStore store{catalog};

    auto session_a = openProject(workspace_a);
    const auto first_record = store.recordOpened(session_a);
    CHECK(first_record.ok());
    CHECK(first_record.entry.has_value());
    CHECK(first_record.entry->project_id == project_a_id);

    auto session_b = openProject(workspace_b);
    const auto second_record = store.recordOpened(session_b);
    CHECK(second_record.ok());
    CHECK(second_record.entry->project_id == project_b_id);

    const auto rerecord_a = store.recordOpened(session_a);
    CHECK(rerecord_a.ok());

    const auto listed = store.list();
    CHECK(listed.ok());
    CHECK(listed.entries.size() == 2U);
    CHECK(listed.entries[0].project_id == project_a_id);
    CHECK(listed.entries[1].project_id == project_b_id);

    RecentProjectStore after_restart{catalog};
    const auto restarted = after_restart.list();
    CHECK(restarted.ok());
    CHECK(restarted.entries.size() == 2U);
    CHECK(restarted.entries[0].project_id == project_a_id);
    CHECK(restarted.entries[0].workspace_root ==
          std::filesystem::weakly_canonical(workspace_a));

    auto reopened = ProjectSession::open(restarted.entries[0].workspace_root);
    CHECK(reopened.ok());
    CHECK(reopened.session->projectId() == restarted.entries[0].project_id);
}

void verifyRemoveDoesNotModifyProject(const std::filesystem::path& root) {
    const auto workspace = root / "Remove Project";
    const auto project_id = initializeProject(workspace, "Keep Me");
    const auto metadata_path =
        ProjectWorkspaceMetadataService::metadataPath(workspace);

    const auto catalog = root / "remove-state" / "recent-projects-v1.txt";
    RecentProjectStore store{catalog};
    auto session = openProject(workspace);
    CHECK(store.recordOpened(session).ok());

    const auto removed = store.remove(project_id);
    CHECK(removed.ok());
    CHECK(removed.entry.has_value());
    CHECK(removed.entry->project_id == project_id);

    const auto listed = store.list();
    CHECK(listed.ok());
    CHECK(listed.entries.empty());
    CHECK(std::filesystem::is_regular_file(metadata_path));

    ProjectWorkspaceMetadataService metadata_service;
    const auto still_project = metadata_service.load(workspace);
    CHECK(still_project.ok());
    CHECK(still_project.metadata->project_id == project_id);

    const auto remove_again = store.remove(project_id);
    CHECK(remove_again.ok());
    CHECK(!remove_again.entry.has_value());
}

void verifyRelocationRequiresMatchingIdentity(const std::filesystem::path& root) {
    const auto workspace = root / "Relocate Source";
    const auto project_id = initializeProject(workspace, "Relocate Me");
    const auto catalog = root / "relocate-state" / "recent-projects-v1.txt";

    RecentProjectStore store{catalog};
    auto session = openProject(workspace);
    CHECK(store.recordOpened(session).ok());

    session = openProject(workspace);
    const auto moved = root / "Relocated";
    std::filesystem::rename(workspace, moved);

    const auto relocated = store.relocate(project_id, moved);
    CHECK(relocated.ok());
    CHECK(relocated.entry.has_value());
    CHECK(relocated.entry->project_id == project_id);
    CHECK(relocated.entry->workspace_root ==
          std::filesystem::weakly_canonical(moved));

    const auto other = root / "Other Project";
    const auto other_id = initializeProject(other, "Other");
    CHECK(other_id != project_id);

    const auto wrong = store.relocate(project_id, other);
    CHECK(!wrong.ok());
    CHECK(wrong.diagnostic.code == RecentProjectErrorCode::project_id_mismatch);

    const auto after_wrong = store.list();
    CHECK(after_wrong.ok());
    CHECK(after_wrong.entries.size() == 1U);
    CHECK(after_wrong.entries[0].project_id == project_id);
    CHECK(after_wrong.entries[0].workspace_root ==
          std::filesystem::weakly_canonical(moved));

    const auto plain = root / "Plain Folder";
    std::filesystem::create_directories(plain);
    const auto invalid = store.relocate(project_id, plain);
    CHECK(!invalid.ok());
    CHECK(invalid.diagnostic.code ==
          RecentProjectErrorCode::project_validation_failed);
    CHECK(invalid.diagnostic.metadata_code ==
          ProjectMetadataErrorCode::not_initialized);
    CHECK(!std::filesystem::exists(plain / ".simplesolid"));
}

void verifyCopyConflictsInsteadOfBecomingNewProject(
    const std::filesystem::path& root) {
    const auto workspace = root / "Original";
    const auto project_id = initializeProject(workspace, "Original");
    const auto catalog = root / "copy-state" / "recent-projects-v1.txt";

    RecentProjectStore store{catalog};
    auto original_session = openProject(workspace);
    CHECK(store.recordOpened(original_session).ok());

    const auto copy = root / "Filesystem Copy";
    std::filesystem::copy(
        workspace,
        copy,
        std::filesystem::copy_options::recursive);

    ProjectWorkspaceMetadataService metadata_service;
    const auto copied_metadata = metadata_service.load(copy);
    CHECK(copied_metadata.ok());
    CHECK(copied_metadata.metadata->project_id == project_id);

    auto copied_session = openProject(copy);
    CHECK(copied_session.projectId() == project_id);

    const auto duplicate_location = store.recordOpened(copied_session);
    CHECK(!duplicate_location.ok());
    CHECK(duplicate_location.diagnostic.code ==
          RecentProjectErrorCode::identity_conflict);

    const auto listed = store.list();
    CHECK(listed.ok());
    CHECK(listed.entries.size() == 1U);
    CHECK(listed.entries[0].project_id == project_id);
    CHECK(listed.entries[0].workspace_root ==
          std::filesystem::weakly_canonical(workspace));
}

void verifyCatalogFailsClosed(const std::filesystem::path& root) {
    const auto catalog = root / "broken-state" / "recent-projects-v1.txt";
    writeText(
        catalog,
        "SIMPLESOLID2_RECENT_PROJECTS 99\n"
        "entry_count 0\n"
        "end\n");

    RecentProjectStore store{catalog};
    const auto unsupported = store.list();
    CHECK(!unsupported.ok());
    CHECK(unsupported.diagnostic.code ==
          RecentProjectErrorCode::unsupported_schema);

    const auto workspace = root / "Project Against Broken Catalog";
    initializeProject(workspace, "Do Not Rewrite");
    auto session = openProject(workspace);
    const auto record = store.recordOpened(session);
    CHECK(!record.ok());
    CHECK(record.diagnostic.code ==
          RecentProjectErrorCode::unsupported_schema);
}

} // namespace

int main() {
    TempDirectory temp;
    verifyPersistenceRestartAndReopen(temp.path);
    verifyRemoveDoesNotModifyProject(temp.path);
    verifyRelocationRequiresMatchingIdentity(temp.path);
    verifyCopyConflictsInsteadOfBecomingNewProject(temp.path);
    verifyCatalogFailsClosed(temp.path);
    return EXIT_SUCCESS;
}
