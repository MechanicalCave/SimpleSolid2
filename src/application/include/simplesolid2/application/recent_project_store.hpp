#pragma once

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace simplesolid2::application {

struct RecentProjectEntry final {
    std::string project_id;
    std::string display_name;
    std::filesystem::path workspace_root;
};

enum class RecentProjectErrorCode {
    none,
    invalid_catalog_path,
    io_failure,
    malformed_catalog,
    unsupported_schema,
    invalid_entry,
    project_validation_failed,
    identity_conflict,
    entry_not_found,
    project_id_mismatch,
    workspace_resolution_failed,
};

struct RecentProjectDiagnostic final {
    RecentProjectErrorCode code{RecentProjectErrorCode::none};
    ProjectMetadataErrorCode metadata_code{ProjectMetadataErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct RecentProjectListResult final {
    std::vector<RecentProjectEntry> entries;
    RecentProjectDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == RecentProjectErrorCode::none;
    }
};

struct RecentProjectMutationResult final {
    std::optional<RecentProjectEntry> entry;
    RecentProjectDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == RecentProjectErrorCode::none;
    }
};

class RecentProjectStore final {
public:
    static constexpr int current_schema_version = 1;
    static constexpr std::size_t max_entries = 20U;

    explicit RecentProjectStore(std::filesystem::path catalog_path)
        : catalog_path_{std::move(catalog_path)} {}

    [[nodiscard]] const std::filesystem::path& catalogPath() const noexcept {
        return catalog_path_;
    }

    [[nodiscard]] RecentProjectListResult list() const;

    [[nodiscard]] RecentProjectMutationResult recordOpened(
        const ProjectSession& session) const;

    [[nodiscard]] RecentProjectMutationResult remove(
        std::string_view project_id) const;

    [[nodiscard]] RecentProjectMutationResult relocate(
        std::string_view expected_project_id,
        const std::filesystem::path& replacement_root) const;

private:
    std::filesystem::path catalog_path_;
};

} // namespace simplesolid2::application
