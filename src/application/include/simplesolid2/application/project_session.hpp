#pragma once

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/document_workspace.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace simplesolid2::application {

struct ProjectSessionOpenResult;

enum class ProjectSessionErrorCode {
    none,
    project_validation_failed,
    workspace_resolution_failed,
};

struct ProjectSessionDiagnostic final {
    ProjectSessionErrorCode code{ProjectSessionErrorCode::none};
    ProjectMetadataErrorCode metadata_code{ProjectMetadataErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

enum class ProjectDocumentErrorCode {
    none,
    invalid_path,
    persistence_failure,
    discovery_failure,
    document_missing,
    identity_conflict,
    identity_changed,
    dirty_document,
};

struct ProjectDocumentDiagnostic final {
    ProjectDocumentErrorCode code{ProjectDocumentErrorCode::none};
    part::PartStoreErrorCode store_code{part::PartStoreErrorCode::none};
    std::string message;
    std::filesystem::path path;
    std::vector<std::filesystem::path> candidates;
};

struct ProjectDocumentResult final {
    DocumentSession* session{};
    bool reused_session{false};
    ProjectDocumentDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept { return session != nullptr; }
};

class ProjectSession final {
public:
    ProjectSession(const ProjectSession&) = delete;
    ProjectSession& operator=(const ProjectSession&) = delete;
    ProjectSession(ProjectSession&&) noexcept = default;
    ProjectSession& operator=(ProjectSession&&) noexcept = default;
    ~ProjectSession() = default;

    [[nodiscard]] static ProjectSessionOpenResult open(
        const std::filesystem::path& workspace_root);

    [[nodiscard]] const std::filesystem::path& workspaceRoot() const noexcept {
        return workspace_root_;
    }

    [[nodiscard]] const ProjectWorkspaceMetadata& metadata() const noexcept {
        return metadata_;
    }

    [[nodiscard]] std::string_view projectId() const noexcept {
        return metadata_.project_id;
    }

    [[nodiscard]] std::string_view displayName() const noexcept {
        return metadata_.display_name;
    }

    [[nodiscard]] const DocumentWorkspaceIndex& documentIndex() const noexcept {
        return document_index_;
    }

    [[nodiscard]] DocumentDiscoveryResult refreshDocuments();

    [[nodiscard]] ProjectDocumentResult createPart(
        const std::filesystem::path& workspace_relative_path);

    [[nodiscard]] ProjectDocumentResult openDocument(
        const core::DocumentId& document_id);

    [[nodiscard]] DocumentSession* documentSession(
        const core::DocumentId& document_id) noexcept;
    [[nodiscard]] const DocumentSession* documentSession(
        const core::DocumentId& document_id) const noexcept;

    [[nodiscard]] std::vector<core::DocumentId> openDocumentIds() const;

    [[nodiscard]] bool closeDocument(
        const core::DocumentId& document_id,
        bool discard_unsaved = false) noexcept;

    [[nodiscard]] bool hasDirtyDocuments() const noexcept;
    [[nodiscard]] DocumentSessionResult saveAllDirtyDocuments();

private:
    ProjectSession(std::filesystem::path workspace_root, ProjectWorkspaceMetadata metadata)
        : workspace_root_{std::move(workspace_root)},
          metadata_{std::move(metadata)} {}

    std::filesystem::path workspace_root_;
    ProjectWorkspaceMetadata metadata_;
    DocumentWorkspaceIndex document_index_;
    std::map<std::string, std::unique_ptr<DocumentSession>> open_documents_;
};

struct ProjectSessionOpenResult final {
    std::optional<ProjectSession> session;
    ProjectSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept { return session.has_value(); }
};

} // namespace simplesolid2::application
