#include <simplesolid2/application/recent_project_store.hpp>

#include <algorithm>
#include <chrono>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#include <utility>

namespace simplesolid2::application {
namespace {

constexpr std::size_t maximum_catalog_bytes = 256U * 1024U;
constexpr std::string_view catalog_magic = "SIMPLESOLID2_RECENT_PROJECTS";

RecentProjectDiagnostic diagnostic(
    RecentProjectErrorCode code,
    std::string message,
    std::filesystem::path path = {},
    ProjectMetadataErrorCode metadata_code = ProjectMetadataErrorCode::none) {
    return RecentProjectDiagnostic{
        code,
        metadata_code,
        std::move(message),
        std::move(path),
    };
}

RecentProjectMutationResult mutationFailure(RecentProjectDiagnostic value) {
    return RecentProjectMutationResult{std::nullopt, std::move(value)};
}

RecentProjectListResult listFailure(RecentProjectDiagnostic value) {
    return RecentProjectListResult{{}, std::move(value)};
}

bool normalizeCatalogPath(
    const std::filesystem::path& input,
    std::filesystem::path& output,
    RecentProjectDiagnostic& error) {
    if (input.empty()) {
        error = diagnostic(
            RecentProjectErrorCode::invalid_catalog_path,
            "Recent Projects catalog path must not be empty");
        return false;
    }

    std::error_code ec;
    auto absolute = std::filesystem::absolute(input, ec);
    if (ec) {
        error = diagnostic(
            RecentProjectErrorCode::invalid_catalog_path,
            "Unable to make Recent Projects catalog path absolute",
            input);
        return false;
    }

    output = absolute.lexically_normal();
    return true;
}

std::string pathToUtf8(const std::filesystem::path& path) {
    const auto value = path.generic_u8string();
    return std::string{
        reinterpret_cast<const char*>(value.data()),
        value.size(),
    };
}

std::filesystem::path pathFromUtf8(std::string_view value) {
    std::u8string utf8;
    utf8.reserve(value.size());
    for (const unsigned char ch : value) {
        utf8.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path{utf8};
}

std::string locationKey(const std::filesystem::path& path) {
    std::error_code ec;
    auto normalized = std::filesystem::absolute(path, ec);
    if (ec) {
        ec.clear();
        normalized = path;
    }

    auto canonical = std::filesystem::weakly_canonical(normalized, ec);
    if (!ec && !canonical.empty()) {
        normalized = std::move(canonical);
    }

    auto key = pathToUtf8(normalized.lexically_normal());
#if defined(_WIN32)
    std::transform(key.begin(), key.end(), key.begin(), [](unsigned char ch) {
        if (ch >= static_cast<unsigned char>('A') &&
            ch <= static_cast<unsigned char>('Z')) {
            return static_cast<char>(ch - static_cast<unsigned char>('A') +
                                     static_cast<unsigned char>('a'));
        }
        return static_cast<char>(ch);
    });
#endif
    return key;
}

bool sameLocation(
    const std::filesystem::path& lhs,
    const std::filesystem::path& rhs) {
    std::error_code ec;
    if (std::filesystem::exists(lhs, ec) && !ec) {
        ec.clear();
        if (std::filesystem::exists(rhs, ec) && !ec) {
            ec.clear();
            const bool equivalent = std::filesystem::equivalent(lhs, rhs, ec);
            if (!ec) return equivalent;
        }
    }
    return locationKey(lhs) == locationKey(rhs);
}

bool canonicalExistingDirectory(
    const std::filesystem::path& input,
    std::filesystem::path& output,
    RecentProjectDiagnostic& error) {
    if (input.empty()) {
        error = diagnostic(
            RecentProjectErrorCode::workspace_resolution_failed,
            "Recent Project workspace path must not be empty",
            input);
        return false;
    }

    std::error_code ec;
    auto absolute = std::filesystem::absolute(input, ec);
    if (ec) {
        error = diagnostic(
            RecentProjectErrorCode::workspace_resolution_failed,
            "Unable to make Recent Project workspace path absolute",
            input);
        return false;
    }

    auto canonical = std::filesystem::weakly_canonical(absolute, ec);
    if (ec || !std::filesystem::is_directory(canonical, ec) || ec) {
        error = diagnostic(
            RecentProjectErrorCode::workspace_resolution_failed,
            "Recent Project workspace must be an existing directory",
            input);
        return false;
    }

    output = canonical.lexically_normal();
    return true;
}

bool validateCatalogEntry(
    RecentProjectEntry& entry,
    const std::filesystem::path& catalog_path,
    RecentProjectDiagnostic& error) {
    if (!ProjectWorkspaceMetadataService::isValidProjectId(entry.project_id)) {
        error = diagnostic(
            RecentProjectErrorCode::invalid_entry,
            "Recent Projects catalog contains an invalid ProjectId",
            catalog_path);
        return false;
    }
    if (entry.display_name.empty()) {
        error = diagnostic(
            RecentProjectErrorCode::invalid_entry,
            "Recent Projects catalog contains an empty DisplayName",
            catalog_path);
        return false;
    }
    if (entry.workspace_root.empty() || !entry.workspace_root.is_absolute()) {
        error = diagnostic(
            RecentProjectErrorCode::invalid_entry,
            "Recent Projects catalog contains a non-absolute Workspace path",
            catalog_path);
        return false;
    }
    entry.workspace_root = entry.workspace_root.lexically_normal();
    return true;
}

bool writeCatalog(
    const std::filesystem::path& catalog_path,
    const std::vector<RecentProjectEntry>& entries,
    RecentProjectDiagnostic& error) {
    const auto parent = catalog_path.parent_path();
    if (parent.empty()) {
        error = diagnostic(
            RecentProjectErrorCode::invalid_catalog_path,
            "Recent Projects catalog must have a parent directory",
            catalog_path);
        return false;
    }

    std::error_code ec;
    if (!std::filesystem::exists(parent, ec)) {
        if (ec) {
            error = diagnostic(
                RecentProjectErrorCode::io_failure,
                "Unable to inspect Recent Projects state directory",
                parent);
            return false;
        }
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            error = diagnostic(
                RecentProjectErrorCode::io_failure,
                "Unable to create Recent Projects state directory",
                parent);
            return false;
        }
    } else if (ec || !std::filesystem::is_directory(parent, ec) || ec) {
        error = diagnostic(
            RecentProjectErrorCode::io_failure,
            "Recent Projects state path is not a directory",
            parent);
        return false;
    }

    const auto unique = std::chrono::high_resolution_clock::now()
                            .time_since_epoch()
                            .count();
    auto temporary = catalog_path;
    temporary += ".tmp-" + std::to_string(unique);
    auto backup = catalog_path;
    backup += ".replace-backup";

    {
        std::ofstream out{temporary, std::ios::binary | std::ios::trunc};
        if (!out) {
            error = diagnostic(
                RecentProjectErrorCode::io_failure,
                "Unable to create temporary Recent Projects catalog",
                temporary);
            return false;
        }

        out << catalog_magic << ' ' << RecentProjectStore::current_schema_version << '\n';
        out << "entry_count " << entries.size() << '\n';
        for (const auto& entry : entries) {
            out << "entry "
                << std::quoted(entry.project_id) << ' '
                << std::quoted(entry.display_name) << ' '
                << std::quoted(pathToUtf8(entry.workspace_root)) << '\n';
        }
        out << "end\n";
        out.flush();
        if (!out) {
            std::error_code cleanup_ec;
            std::filesystem::remove(temporary, cleanup_ec);
            error = diagnostic(
                RecentProjectErrorCode::io_failure,
                "Unable to write Recent Projects catalog",
                temporary);
            return false;
        }
    }

    ec.clear();
    std::filesystem::remove(backup, ec);

    ec.clear();
    const bool had_previous = std::filesystem::exists(catalog_path, ec) && !ec;
    if (ec) {
        std::filesystem::remove(temporary, ec);
        error = diagnostic(
            RecentProjectErrorCode::io_failure,
            "Unable to inspect existing Recent Projects catalog",
            catalog_path);
        return false;
    }

    if (had_previous) {
        ec.clear();
        std::filesystem::rename(catalog_path, backup, ec);
        if (ec) {
            std::filesystem::remove(temporary, ec);
            error = diagnostic(
                RecentProjectErrorCode::io_failure,
                "Unable to stage previous Recent Projects catalog",
                catalog_path);
            return false;
        }
    }

    ec.clear();
    std::filesystem::rename(temporary, catalog_path, ec);
    if (ec) {
        if (had_previous) {
            std::error_code rollback_ec;
            std::filesystem::rename(backup, catalog_path, rollback_ec);
        }
        std::error_code cleanup_ec;
        std::filesystem::remove(temporary, cleanup_ec);
        error = diagnostic(
            RecentProjectErrorCode::io_failure,
            "Unable to publish Recent Projects catalog",
            catalog_path);
        return false;
    }

    if (had_previous) {
        std::filesystem::remove(backup, ec);
    }
    return true;
}

RecentProjectMutationResult propagateListFailure(
    const RecentProjectListResult& loaded) {
    return mutationFailure(loaded.diagnostic);
}

} // namespace

RecentProjectListResult RecentProjectStore::list() const {
    std::filesystem::path catalog_path;
    RecentProjectDiagnostic error;
    if (!normalizeCatalogPath(catalog_path_, catalog_path, error)) {
        return listFailure(std::move(error));
    }

    std::error_code ec;
    const bool exists = std::filesystem::exists(catalog_path, ec);
    if (ec) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::io_failure,
            "Unable to inspect Recent Projects catalog",
            catalog_path));
    }
    if (!exists) {
        return RecentProjectListResult{};
    }
    if (!std::filesystem::is_regular_file(catalog_path, ec) || ec) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::malformed_catalog,
            "Recent Projects catalog is not a regular file",
            catalog_path));
    }

    const auto file_size = std::filesystem::file_size(catalog_path, ec);
    if (ec) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::io_failure,
            "Unable to inspect Recent Projects catalog size",
            catalog_path));
    }
    if (file_size > maximum_catalog_bytes) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::malformed_catalog,
            "Recent Projects catalog exceeds the supported size",
            catalog_path));
    }

    std::ifstream in{catalog_path, std::ios::binary};
    if (!in) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::io_failure,
            "Unable to open Recent Projects catalog",
            catalog_path));
    }

    std::string magic;
    int version{};
    if (!(in >> magic >> version) || magic != catalog_magic) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::malformed_catalog,
            "Invalid Recent Projects catalog header",
            catalog_path));
    }
    if (version != current_schema_version) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::unsupported_schema,
            "Unsupported Recent Projects catalog schema version",
            catalog_path));
    }

    std::string key;
    std::size_t count{};
    if (!(in >> key >> count) || key != "entry_count" || count > max_entries) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::malformed_catalog,
            "Invalid Recent Projects entry count",
            catalog_path));
    }

    std::vector<RecentProjectEntry> entries;
    entries.reserve(count);

    for (std::size_t index = 0; index < count; ++index) {
        RecentProjectEntry entry;
        std::string workspace_utf8;
        if (!(in >> key
              >> std::quoted(entry.project_id)
              >> std::quoted(entry.display_name)
              >> std::quoted(workspace_utf8)) ||
            key != "entry") {
            return listFailure(diagnostic(
                RecentProjectErrorCode::malformed_catalog,
                "Invalid Recent Projects catalog entry",
                catalog_path));
        }

        try {
            entry.workspace_root = pathFromUtf8(workspace_utf8);
        } catch (const std::filesystem::filesystem_error&) {
            return listFailure(diagnostic(
                RecentProjectErrorCode::invalid_entry,
                "Recent Projects catalog contains an invalid Workspace path",
                catalog_path));
        }

        if (!validateCatalogEntry(entry, catalog_path, error)) {
            return listFailure(std::move(error));
        }

        const auto duplicate = std::find_if(
            entries.begin(),
            entries.end(),
            [&](const RecentProjectEntry& candidate) {
                return candidate.project_id == entry.project_id;
            });
        if (duplicate != entries.end()) {
            return listFailure(diagnostic(
                RecentProjectErrorCode::invalid_entry,
                "Recent Projects catalog contains duplicate ProjectId entries",
                catalog_path));
        }

        entries.push_back(std::move(entry));
    }

    if (!(in >> key) || key != "end") {
        return listFailure(diagnostic(
            RecentProjectErrorCode::malformed_catalog,
            "Recent Projects catalog terminator is missing",
            catalog_path));
    }

    std::string trailing;
    if (in >> trailing) {
        return listFailure(diagnostic(
            RecentProjectErrorCode::malformed_catalog,
            "Recent Projects catalog contains trailing data",
            catalog_path));
    }

    return RecentProjectListResult{std::move(entries), RecentProjectDiagnostic{}};
}

RecentProjectMutationResult RecentProjectStore::recordOpened(
    const ProjectSession& session) const {
    auto loaded = list();
    if (!loaded.ok()) return propagateListFailure(loaded);

    const std::string project_id{session.projectId()};
    const auto found = std::find_if(
        loaded.entries.begin(),
        loaded.entries.end(),
        [&](const RecentProjectEntry& entry) {
            return entry.project_id == project_id;
        });

    RecentProjectEntry current{
        project_id,
        std::string{session.displayName()},
        session.workspaceRoot(),
    };

    if (found != loaded.entries.end()) {
        if (!sameLocation(found->workspace_root, current.workspace_root)) {
            return mutationFailure(diagnostic(
                RecentProjectErrorCode::identity_conflict,
                "ProjectId is already registered at a different Workspace location",
                current.workspace_root));
        }
        loaded.entries.erase(found);
    }

    loaded.entries.insert(loaded.entries.begin(), current);
    if (loaded.entries.size() > max_entries) {
        loaded.entries.resize(max_entries);
    }

    std::filesystem::path catalog_path;
    RecentProjectDiagnostic error;
    if (!normalizeCatalogPath(catalog_path_, catalog_path, error)) {
        return mutationFailure(std::move(error));
    }
    if (!writeCatalog(catalog_path, loaded.entries, error)) {
        return mutationFailure(std::move(error));
    }

    return RecentProjectMutationResult{std::move(current), RecentProjectDiagnostic{}};
}

RecentProjectMutationResult RecentProjectStore::remove(
    std::string_view project_id) const {
    if (!ProjectWorkspaceMetadataService::isValidProjectId(project_id)) {
        return mutationFailure(diagnostic(
            RecentProjectErrorCode::invalid_entry,
            "Recent Projects remove requires a valid ProjectId"));
    }

    auto loaded = list();
    if (!loaded.ok()) return propagateListFailure(loaded);

    const auto found = std::find_if(
        loaded.entries.begin(),
        loaded.entries.end(),
        [&](const RecentProjectEntry& entry) {
            return entry.project_id == project_id;
        });
    if (found == loaded.entries.end()) {
        return RecentProjectMutationResult{};
    }

    RecentProjectEntry removed = *found;
    loaded.entries.erase(found);

    std::filesystem::path catalog_path;
    RecentProjectDiagnostic error;
    if (!normalizeCatalogPath(catalog_path_, catalog_path, error)) {
        return mutationFailure(std::move(error));
    }
    if (!writeCatalog(catalog_path, loaded.entries, error)) {
        return mutationFailure(std::move(error));
    }

    return RecentProjectMutationResult{std::move(removed), RecentProjectDiagnostic{}};
}

RecentProjectMutationResult RecentProjectStore::relocate(
    std::string_view expected_project_id,
    const std::filesystem::path& replacement_root) const {
    if (!ProjectWorkspaceMetadataService::isValidProjectId(expected_project_id)) {
        return mutationFailure(diagnostic(
            RecentProjectErrorCode::invalid_entry,
            "Recent Projects relocation requires a valid expected ProjectId"));
    }

    auto loaded = list();
    if (!loaded.ok()) return propagateListFailure(loaded);

    const auto found = std::find_if(
        loaded.entries.begin(),
        loaded.entries.end(),
        [&](const RecentProjectEntry& entry) {
            return entry.project_id == expected_project_id;
        });
    if (found == loaded.entries.end()) {
        return mutationFailure(diagnostic(
            RecentProjectErrorCode::entry_not_found,
            "Recent Project entry no longer exists"));
    }

    std::filesystem::path replacement;
    RecentProjectDiagnostic error;
    if (!canonicalExistingDirectory(replacement_root, replacement, error)) {
        return mutationFailure(std::move(error));
    }

    ProjectWorkspaceMetadataService metadata_service;
    auto metadata = metadata_service.load(replacement);
    if (!metadata.ok()) {
        return mutationFailure(diagnostic(
            RecentProjectErrorCode::project_validation_failed,
            std::move(metadata.diagnostic.message),
            std::move(metadata.diagnostic.path),
            metadata.diagnostic.code));
    }

    if (metadata.metadata->project_id != expected_project_id) {
        return mutationFailure(diagnostic(
            RecentProjectErrorCode::project_id_mismatch,
            "Located Project Workspace has a different ProjectId",
            replacement));
    }

    RecentProjectEntry relocated{
        metadata.metadata->project_id,
        metadata.metadata->display_name,
        replacement,
    };
    *found = relocated;

    std::filesystem::path catalog_path;
    if (!normalizeCatalogPath(catalog_path_, catalog_path, error)) {
        return mutationFailure(std::move(error));
    }
    if (!writeCatalog(catalog_path, loaded.entries, error)) {
        return mutationFailure(std::move(error));
    }

    return RecentProjectMutationResult{std::move(relocated), RecentProjectDiagnostic{}};
}

} // namespace simplesolid2::application
