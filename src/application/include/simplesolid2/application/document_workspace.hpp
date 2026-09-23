#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace simplesolid2::application {

enum class DocumentIndexState {
    resolved,
    identity_conflict,
    invalid,
};

struct DocumentIndexEntry final {
    DocumentIndexState state{DocumentIndexState::invalid};
    std::optional<core::DocumentId> document_id;
    std::string title;
    std::vector<std::filesystem::path> relative_paths;
    part::PartStoreErrorCode store_code{part::PartStoreErrorCode::none};
    std::string diagnostic;
};

enum class DocumentDiscoveryErrorCode {
    none,
    invalid_workspace,
    filesystem_failure,
};

struct DocumentDiscoveryDiagnostic final {
    DocumentDiscoveryErrorCode code{DocumentDiscoveryErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct DocumentDiscoveryResult final {
    DocumentDiscoveryDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentDiscoveryErrorCode::none;
    }
};

enum class DocumentResolutionState {
    missing,
    resolved,
    identity_conflict,
};

struct DocumentResolution final {
    DocumentResolutionState state{DocumentResolutionState::missing};
    std::filesystem::path absolute_path;
    std::vector<std::filesystem::path> relative_paths;
    std::string diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return state == DocumentResolutionState::resolved;
    }
};

class DocumentWorkspaceIndex final {
public:
    [[nodiscard]] DocumentDiscoveryResult refresh(
        const std::filesystem::path& workspace_root);

    [[nodiscard]] const std::filesystem::path& workspaceRoot() const noexcept {
        return workspace_root_;
    }
    [[nodiscard]] const std::vector<DocumentIndexEntry>& entries() const noexcept {
        return entries_;
    }
    [[nodiscard]] const DocumentDiscoveryDiagnostic& lastDiagnostic() const noexcept {
        return last_diagnostic_;
    }

    [[nodiscard]] DocumentResolution resolve(
        const core::DocumentId& document_id) const;

private:
    std::filesystem::path workspace_root_;
    std::vector<DocumentIndexEntry> entries_;
    DocumentDiscoveryDiagnostic last_diagnostic_;
};

} // namespace simplesolid2::application
