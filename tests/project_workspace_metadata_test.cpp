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
        std::cerr << "PH-01 metadata CHECK failed at line " << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph01_metadata_" +
                std::to_string(std::filesystem::file_time_type::clock::now().time_since_epoch().count()));
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

void verifyCreateLoadCopyAndMove(const std::filesystem::path& root) {
    ProjectWorkspaceMetadataService service;
    const auto workspace = root / "Project A";
    std::filesystem::create_directories(workspace);
    writeText(workspace / "user-file.txt", "preserve me");

    CHECK(!service.isInitialized(workspace));
    const auto created = service.initialize(workspace, "Hydraulic Press");
    CHECK(created.ok());
    CHECK(created.metadata.has_value());
    CHECK(ProjectWorkspaceMetadataService::isValidProjectId(created.metadata->project_id));
    CHECK(created.metadata->display_name == "Hydraulic Press");
    CHECK(created.metadata->schema_version == ProjectWorkspaceMetadataService::current_schema_version);
    CHECK(!created.metadata->created_at.empty());
    CHECK(service.isInitialized(workspace));
    CHECK(std::filesystem::is_regular_file(workspace / ".simplesolid" / "project.json"));
    CHECK(std::filesystem::is_regular_file(workspace / "user-file.txt"));

    const auto loaded = service.load(workspace);
    CHECK(loaded.ok());
    CHECK(loaded.metadata->project_id == created.metadata->project_id);
    CHECK(loaded.metadata->display_name == created.metadata->display_name);
    CHECK(loaded.metadata->created_at == created.metadata->created_at);

    const auto duplicate = service.initialize(workspace, "Different Name");
    CHECK(!duplicate.ok());
    CHECK(duplicate.diagnostic.code == ProjectMetadataErrorCode::already_initialized);
    const auto after_duplicate = service.load(workspace);
    CHECK(after_duplicate.ok());
    CHECK(after_duplicate.metadata->project_id == created.metadata->project_id);
    CHECK(after_duplicate.metadata->display_name == "Hydraulic Press");

    const auto copy = root / "Project A Copy";
    std::filesystem::copy(workspace, copy, std::filesystem::copy_options::recursive);
    const auto copied = service.load(copy);
    CHECK(copied.ok());
    CHECK(copied.metadata->project_id == created.metadata->project_id);

    const auto moved = root / "Project A Moved";
    std::filesystem::rename(workspace, moved);
    const auto moved_loaded = service.load(moved);
    CHECK(moved_loaded.ok());
    CHECK(moved_loaded.metadata->project_id == created.metadata->project_id);
}

void verifyPlainFolderFailsClosed(const std::filesystem::path& root) {
    ProjectWorkspaceMetadataService service;
    const auto plain = root / "Plain";
    std::filesystem::create_directories(plain);

    const auto result = service.load(plain);
    CHECK(!result.ok());
    CHECK(result.diagnostic.code == ProjectMetadataErrorCode::not_initialized);
    CHECK(!std::filesystem::exists(plain / ".simplesolid"));
}

void verifyMalformedAndMissingMetadataFailClosed(const std::filesystem::path& root) {
    ProjectWorkspaceMetadataService service;

    const auto malformed = root / "Malformed";
    std::filesystem::create_directories(malformed / ".simplesolid");
    writeText(malformed / ".simplesolid" / "project.json", "{ not-json }");
    const auto malformed_result = service.load(malformed);
    CHECK(!malformed_result.ok());
    CHECK(malformed_result.diagnostic.code == ProjectMetadataErrorCode::malformed_metadata);

    const auto missing = root / "Missing";
    std::filesystem::create_directories(missing / ".simplesolid");
    writeText(missing / ".simplesolid" / "project.json",
              "{\n"
              "  \"project_id\": \"8db04ea4-3576-4d12-9ea2-d4cb0e4af128\",\n"
              "  \"display_name\": \"Missing CreatedAt\",\n"
              "  \"schema_version\": 1\n"
              "}\n");
    const auto missing_result = service.load(missing);
    CHECK(!missing_result.ok());
    CHECK(missing_result.diagnostic.code == ProjectMetadataErrorCode::missing_field);
}

void verifySchemaAndIdentityValidation(const std::filesystem::path& root) {
    ProjectWorkspaceMetadataService service;

    const auto unsupported = root / "Unsupported";
    std::filesystem::create_directories(unsupported / ".simplesolid");
    writeText(unsupported / ".simplesolid" / "project.json",
              "{\n"
              "  \"project_id\": \"8db04ea4-3576-4d12-9ea2-d4cb0e4af128\",\n"
              "  \"display_name\": \"Unsupported\",\n"
              "  \"schema_version\": 2,\n"
              "  \"created_at\": \"2026-09-23T05:00:00Z\"\n"
              "}\n");
    const auto unsupported_result = service.load(unsupported);
    CHECK(!unsupported_result.ok());
    CHECK(unsupported_result.diagnostic.code == ProjectMetadataErrorCode::unsupported_schema);

    const auto invalid_id = root / "InvalidId";
    std::filesystem::create_directories(invalid_id / ".simplesolid");
    writeText(invalid_id / ".simplesolid" / "project.json",
              "{\n"
              "  \"project_id\": \"not-a-project-id\",\n"
              "  \"display_name\": \"Invalid ID\",\n"
              "  \"schema_version\": 1,\n"
              "  \"created_at\": \"2026-09-23T05:00:00Z\"\n"
              "}\n");
    const auto invalid_id_result = service.load(invalid_id);
    CHECK(!invalid_id_result.ok());
    CHECK(invalid_id_result.diagnostic.code == ProjectMetadataErrorCode::invalid_project_id);

    CHECK(ProjectWorkspaceMetadataService::isValidProjectId(
        "8db04ea4-3576-4d12-9ea2-d4cb0e4af128"));
    CHECK(!ProjectWorkspaceMetadataService::isValidProjectId(
        "8db04ea4-3576-1d12-9ea2-d4cb0e4af128"));
}

} // namespace

int main() {
    TempDirectory temp;
    verifyCreateLoadCopyAndMove(temp.path);
    verifyPlainFolderFailsClosed(temp.path);
    verifyMalformedAndMissingMetadataFailClosed(temp.path);
    verifySchemaAndIdentityValidation(temp.path);
    return EXIT_SUCCESS;
}
