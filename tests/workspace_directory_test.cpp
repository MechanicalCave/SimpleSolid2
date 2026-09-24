#include <simplesolid2/application/workspace_directory.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (value) return;

    std::cerr
        << "WB-01A WorkspaceDirectory CHECK failed at line "
        << line
        << ": "
        << expression
        << '\n';
    std::abort();
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_workspace_directory_" +
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

bool contains(
    const std::vector<std::filesystem::path>& paths,
    const std::filesystem::path& value) {
    return std::find(
               paths.begin(),
               paths.end(),
               value) != paths.end();
}

} // namespace

int main() {
    TempDirectory temp;
    const auto root = temp.path / "Project";

    std::filesystem::create_directories(
        root / std::filesystem::path{".simplesolid"} / "internal");
    std::filesystem::create_directories(
        root / "Parts" / "Sub");
    std::filesystem::create_directories(
        root / "Other");

    application::WorkspaceDirectoryService service;

    const auto listed =
        service.listDirectories(root);
    CHECK(listed.ok());
    CHECK(contains(listed.directories, {}));
    CHECK(contains(
        listed.directories,
        std::filesystem::path{"Parts"}));
    CHECK(contains(
        listed.directories,
        std::filesystem::path{"Parts"} / "Sub"));
    CHECK(contains(
        listed.directories,
        std::filesystem::path{"Other"}));
    CHECK(!contains(
        listed.directories,
        std::filesystem::path{".simplesolid"}));
    CHECK(!contains(
        listed.directories,
        std::filesystem::path{".simplesolid"} / "internal"));

    const auto created =
        service.createDirectory(
            root,
            std::filesystem::path{"Parts"},
            std::filesystem::path{"Generated"});
    CHECK(created.ok());
    CHECK(created.relative_path ==
          std::filesystem::path{"Parts"} / std::filesystem::path{"Generated"});
    CHECK(std::filesystem::is_directory(
        root / "Parts" / std::filesystem::path{"Generated"}));

    const auto duplicate =
        service.createDirectory(
            root,
            "Parts",
            std::filesystem::path{"Generated"});
    CHECK(!duplicate.ok());
    CHECK(duplicate.diagnostic.code ==
          application::WorkspaceDirectoryErrorCode::
              already_exists);

    const auto reserved =
        service.createDirectory(
            root,
            {},
            std::filesystem::path{".simplesolid"});
    CHECK(!reserved.ok());
    CHECK(reserved.diagnostic.code ==
          application::WorkspaceDirectoryErrorCode::
              invalid_name);

    const auto escape_name =
        service.createDirectory(
            root,
            {},
            std::filesystem::path{"../Outside"});
    CHECK(!escape_name.ok());
    CHECK(escape_name.diagnostic.code ==
          application::WorkspaceDirectoryErrorCode::
              invalid_name);

    const auto escape_parent =
        service.createDirectory(
            root,
            std::filesystem::path{"../Outside"},
            std::filesystem::path{"Child"});
    CHECK(!escape_parent.ok());
    CHECK(escape_parent.diagnostic.code ==
          application::WorkspaceDirectoryErrorCode::
              invalid_relative_path);

    const auto reserved_parent =
        service.createDirectory(
            root,
            std::filesystem::path{".simplesolid"},
            std::filesystem::path{"Child"});
    CHECK(!reserved_parent.ok());
    CHECK(reserved_parent.diagnostic.code ==
          application::WorkspaceDirectoryErrorCode::
              invalid_relative_path);

    const auto missing_parent =
        service.createDirectory(
            root,
            std::filesystem::path{"Missing"},
            std::filesystem::path{"Child"});
    CHECK(!missing_parent.ok());
    CHECK(missing_parent.diagnostic.code ==
          application::WorkspaceDirectoryErrorCode::
              parent_missing);

    return EXIT_SUCCESS;
}
