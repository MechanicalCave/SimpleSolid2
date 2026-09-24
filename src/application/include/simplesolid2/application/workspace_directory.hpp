#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace simplesolid2::application {

enum class WorkspaceDirectoryErrorCode {
    none,
    invalid_workspace,
    invalid_relative_path,
    reserved_path,
    invalid_name,
    parent_missing,
    already_exists,
    filesystem_failure,
};

struct WorkspaceDirectoryDiagnostic final {
    WorkspaceDirectoryErrorCode code{
        WorkspaceDirectoryErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct WorkspaceDirectoryListing final {
    std::vector<std::filesystem::path> directories;
    WorkspaceDirectoryDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code ==
               WorkspaceDirectoryErrorCode::none;
    }
};

struct WorkspaceDirectoryResult final {
    std::filesystem::path relative_path;
    WorkspaceDirectoryDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code ==
               WorkspaceDirectoryErrorCode::none;
    }
};

class WorkspaceDirectoryService final {
public:
    [[nodiscard]] WorkspaceDirectoryListing listDirectories(
        const std::filesystem::path& workspace_root) const;

    [[nodiscard]] WorkspaceDirectoryResult createDirectory(
        const std::filesystem::path& workspace_root,
        const std::filesystem::path& parent_relative_path,
        const std::filesystem::path& folder_name) const;
};

} // namespace simplesolid2::application
