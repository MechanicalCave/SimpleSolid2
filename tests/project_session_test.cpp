#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace simplesolid2::application;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-01 ProjectSession CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph01_session_" +
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

void verifyOpenCloseReopenAndMove(const std::filesystem::path& root) {
    ProjectWorkspaceMetadataService metadata_service;

    const auto workspace = root / "Project A";
    std::filesystem::create_directories(workspace);

    const auto initialized = metadata_service.initialize(workspace, "Hydraulic Press");
    CHECK(initialized.ok());
    const std::string project_id = initialized.metadata->project_id;

    auto first = ProjectSession::open(workspace);
    CHECK(first.ok());
    CHECK(first.session.has_value());
    CHECK(first.session->projectId() == project_id);
    CHECK(first.session->displayName() == "Hydraulic Press");
    CHECK(first.session->metadata().schema_version ==
          ProjectWorkspaceMetadataService::current_schema_version);
    CHECK(first.session->workspaceRoot() == std::filesystem::weakly_canonical(workspace));

    first.session.reset();
    CHECK(!first.session.has_value());

    auto reopened = ProjectSession::open(workspace);
    CHECK(reopened.ok());
    CHECK(reopened.session->projectId() == project_id);
    CHECK(reopened.session->workspaceRoot() == std::filesystem::weakly_canonical(workspace));

    reopened.session.reset();

    const auto moved = root / "Project A Moved";
    std::filesystem::rename(workspace, moved);

    auto after_move = ProjectSession::open(moved);
    CHECK(after_move.ok());
    CHECK(after_move.session->projectId() == project_id);
    CHECK(after_move.session->workspaceRoot() == std::filesystem::weakly_canonical(moved));
}

void verifyPlainFolderFailsClosed(const std::filesystem::path& root) {
    const auto plain = root / "Plain";
    std::filesystem::create_directories(plain);

    const auto result = ProjectSession::open(plain);
    CHECK(!result.ok());
    CHECK(result.diagnostic.code == ProjectSessionErrorCode::project_validation_failed);
    CHECK(result.diagnostic.metadata_code == ProjectMetadataErrorCode::not_initialized);
    CHECK(!std::filesystem::exists(plain / ".simplesolid"));
}

void verifyInvalidMetadataFailsBeforeSession(const std::filesystem::path& root) {
    const auto invalid = root / "Invalid";
    std::filesystem::create_directories(invalid / ".simplesolid");
    writeText(
        invalid / ".simplesolid" / "project.json",
        "{\n"
        "  \"project_id\": \"not-a-project-id\",\n"
        "  \"display_name\": \"Invalid\",\n"
        "  \"schema_version\": 1,\n"
        "  \"created_at\": \"2026-09-23T05:00:00Z\"\n"
        "}\n");

    const auto result = ProjectSession::open(invalid);
    CHECK(!result.ok());
    CHECK(result.diagnostic.code == ProjectSessionErrorCode::project_validation_failed);
    CHECK(result.diagnostic.metadata_code == ProjectMetadataErrorCode::invalid_project_id);
}

} // namespace

int main() {
    TempDirectory temp;
    verifyOpenCloseReopenAndMove(temp.path);
    verifyPlainFolderFailsClosed(temp.path);
    verifyInvalidMetadataFailsBeforeSession(temp.path);
    return EXIT_SUCCESS;
}
