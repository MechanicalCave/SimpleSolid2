#include <simplesolid2/application/workspace_directory.hpp>

#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace simplesolid2::application {
namespace {

WorkspaceDirectoryDiagnostic failure(
    WorkspaceDirectoryErrorCode code,
    std::string message,
    std::filesystem::path path = {}) {
    return WorkspaceDirectoryDiagnostic{
        code,
        std::move(message),
        std::move(path)};
}

bool pathIsInside(
    const std::filesystem::path& root,
    const std::filesystem::path& candidate) {
    auto root_it = root.begin();
    auto candidate_it = candidate.begin();

    for (; root_it != root.end();
         ++root_it, ++candidate_it) {
        if (candidate_it == candidate.end() ||
            *root_it != *candidate_it) {
            return false;
        }
    }

    return true;
}

std::string asciiLower(
    std::string value) {
    std::transform(
        value.begin(),
        value.end(),
        value.begin(),
        [](unsigned char value) {
            return static_cast<char>(
                std::tolower(value));
        });
    return value;
}

bool isReservedComponent(
    const std::filesystem::path& component) {
    return asciiLower(
               component.generic_string()) ==
           ".simplesolid";
}

bool validRelativeDirectoryPath(
    const std::filesystem::path& relative) {
    if (relative.empty() || relative == ".") {
        return true;
    }

    if (relative.is_absolute() ||
        relative.has_root_path()) {
        return false;
    }

    const auto normalized =
        relative.lexically_normal();
    for (const auto& component : normalized) {
        if (component == ".." ||
            isReservedComponent(component)) {
            return false;
        }
    }

    return true;
}

bool validFolderName(
    std::string_view name) {
    if (name.empty() ||
        name == "." ||
        name == "..") {
        return false;
    }

    const std::filesystem::path path{
        std::string{name}};
    if (path.is_absolute() ||
        path.has_root_path() ||
        path.has_parent_path() ||
        path.filename() != path ||
        isReservedComponent(path)) {
        return false;
    }

    return true;
}

struct ResolvedDirectory final {
    std::filesystem::path root;
    std::filesystem::path absolute;
    std::filesystem::path relative;
    WorkspaceDirectoryDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code ==
               WorkspaceDirectoryErrorCode::none;
    }
};

ResolvedDirectory resolveExistingDirectory(
    const std::filesystem::path& workspace_root,
    const std::filesystem::path& relative) {
    std::error_code ec;
    auto root =
        std::filesystem::weakly_canonical(
            workspace_root,
            ec);
    if (ec ||
        !std::filesystem::is_directory(root, ec) ||
        ec) {
        return {
            {},
            {},
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    invalid_workspace,
                "Unable to resolve Project Workspace",
                workspace_root)};
    }
    root = root.lexically_normal();

    if (!validRelativeDirectoryPath(relative)) {
        return {
            root,
            {},
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    invalid_relative_path,
                "Workspace directory path must remain inside the Workspace and may not use .simplesolid",
                relative)};
    }

    const auto normalized =
        relative.empty() || relative == "."
            ? std::filesystem::path{}
            : relative.lexically_normal();

    auto absolute =
        std::filesystem::weakly_canonical(
            root / normalized,
            ec);
    if (ec ||
        !std::filesystem::is_directory(
            absolute,
            ec) ||
        ec) {
        return {
            root,
            {},
            normalized,
            failure(
                WorkspaceDirectoryErrorCode::
                    parent_missing,
                "Workspace directory does not exist",
                root / normalized)};
    }

    absolute = absolute.lexically_normal();
    if (!pathIsInside(root, absolute)) {
        return {
            root,
            {},
            normalized,
            failure(
                WorkspaceDirectoryErrorCode::
                    invalid_relative_path,
                "Workspace directory resolves outside the Project Workspace",
                absolute)};
    }

    return {
        root,
        absolute,
        normalized,
        WorkspaceDirectoryDiagnostic{}};
}

} // namespace

WorkspaceDirectoryListing
WorkspaceDirectoryService::listDirectories(
    const std::filesystem::path& workspace_root) const {
    const auto resolved =
        resolveExistingDirectory(
            workspace_root,
            {});
    if (!resolved.ok()) {
        return {
            {},
            resolved.diagnostic};
    }

    WorkspaceDirectoryListing result;
    result.directories.push_back({});

    std::error_code ec;
    std::filesystem::recursive_directory_iterator iterator{
        resolved.root,
        std::filesystem::directory_options::
            skip_permission_denied,
        ec};
    const std::filesystem::recursive_directory_iterator end;

    if (ec) {
        result.diagnostic =
            failure(
                WorkspaceDirectoryErrorCode::
                    filesystem_failure,
                "Unable to enumerate Workspace directories",
                resolved.root);
        return result;
    }

    for (; iterator != end;
         iterator.increment(ec)) {
        if (ec) {
            result.diagnostic =
                failure(
                    WorkspaceDirectoryErrorCode::
                        filesystem_failure,
                    "Workspace directory enumeration failed",
                    iterator->path());
            return result;
        }

        const auto& entry = *iterator;

        if (entry.is_symlink(ec) && !ec) {
            if (entry.is_directory(ec) && !ec) {
                iterator.disable_recursion_pending();
            }
            ec.clear();
            continue;
        }
        ec.clear();

        if (!entry.is_directory(ec) || ec) {
            ec.clear();
            continue;
        }

        if (isReservedComponent(
                entry.path().filename())) {
            iterator.disable_recursion_pending();
            continue;
        }

        auto relative =
            std::filesystem::relative(
                entry.path(),
                resolved.root,
                ec);
        if (ec) {
            result.diagnostic =
                failure(
                    WorkspaceDirectoryErrorCode::
                        filesystem_failure,
                    "Unable to derive Workspace-relative directory path",
                    entry.path());
            return result;
        }

        relative = relative.lexically_normal();
        if (!validRelativeDirectoryPath(relative)) {
            iterator.disable_recursion_pending();
            continue;
        }

        result.directories.push_back(
            std::move(relative));
    }

    std::sort(
        result.directories.begin(),
        result.directories.end(),
        [](const auto& left, const auto& right) {
            return left.generic_string() <
                   right.generic_string();
        });

    return result;
}

WorkspaceDirectoryResult
WorkspaceDirectoryService::createDirectory(
    const std::filesystem::path& workspace_root,
    const std::filesystem::path& parent_relative_path,
    std::string_view folder_name) const {
    if (!validFolderName(folder_name)) {
        return {
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    invalid_name,
                "Folder name must be one ordinary Workspace directory name and may not be .simplesolid",
                std::filesystem::path{
                    std::string{folder_name}})};
    }

    const auto parent =
        resolveExistingDirectory(
            workspace_root,
            parent_relative_path);
    if (!parent.ok()) {
        return {
            {},
            parent.diagnostic};
    }

    const auto target =
        parent.absolute /
        std::filesystem::path{
            std::string{folder_name}};

    std::error_code ec;
    if (std::filesystem::exists(target, ec)) {
        return {
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    already_exists,
                "Workspace directory already exists",
                target)};
    }
    if (ec) {
        return {
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    filesystem_failure,
                "Unable to inspect Workspace directory target",
                target)};
    }

    if (!std::filesystem::create_directory(
            target,
            ec) ||
        ec) {
        return {
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    filesystem_failure,
                "Unable to create Workspace directory",
                target)};
    }

    auto created =
        std::filesystem::weakly_canonical(
            target,
            ec);
    if (ec ||
        !std::filesystem::is_directory(
            created,
            ec) ||
        ec ||
        !pathIsInside(
            parent.root,
            created.lexically_normal())) {
        std::error_code cleanup_ec;
        std::filesystem::remove(
            target,
            cleanup_ec);
        return {
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    filesystem_failure,
                "Created Workspace directory could not be verified",
                target)};
    }

    auto relative =
        std::filesystem::relative(
            created,
            parent.root,
            ec);
    if (ec) {
        return {
            {},
            failure(
                WorkspaceDirectoryErrorCode::
                    filesystem_failure,
                "Unable to derive created Workspace-relative directory path",
                created)};
    }

    return {
        relative.lexically_normal(),
        WorkspaceDirectoryDiagnostic{}};
}

} // namespace simplesolid2::application
