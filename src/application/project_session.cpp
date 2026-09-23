#include <simplesolid2/application/project_session.hpp>

#include <algorithm>
#include <system_error>
#include <utility>

namespace simplesolid2::application {
namespace {

ProjectDocumentResult documentFailure(
    ProjectDocumentErrorCode code,
    std::string message,
    std::filesystem::path path = {},
    part::PartStoreErrorCode store_code = part::PartStoreErrorCode::none,
    std::vector<std::filesystem::path> candidates = {}) {
    return ProjectDocumentResult{
        nullptr,
        false,
        ProjectDocumentDiagnostic{
            code,
            store_code,
            std::move(message),
            std::move(path),
            std::move(candidates),
        },
    };
}

bool pathIsInside(
    const std::filesystem::path& root,
    const std::filesystem::path& candidate) {
    auto root_it = root.begin();
    auto candidate_it = candidate.begin();
    for (; root_it != root.end(); ++root_it, ++candidate_it) {
        if (candidate_it == candidate.end() || *root_it != *candidate_it) {
            return false;
        }
    }
    return true;
}

bool resolvePartTarget(
    const std::filesystem::path& workspace_root,
    const std::filesystem::path& requested,
    std::filesystem::path& target) {
    if (requested.empty() || requested.is_absolute() || requested.has_root_path()) {
        return false;
    }

    const auto relative = requested.lexically_normal();
    if (relative.empty() || relative == "." || relative.filename().empty()) {
        return false;
    }
    for (const auto& component : relative) {
        if (component == "..") return false;
    }
    if (!part::PartDocumentStore::hasNativeExtension(relative)) {
        return false;
    }

    auto parent = workspace_root / relative.parent_path();
    std::error_code ec;
    auto canonical_parent = std::filesystem::weakly_canonical(parent, ec);
    if (ec || !std::filesystem::is_directory(canonical_parent, ec) || ec) {
        return false;
    }
    canonical_parent = canonical_parent.lexically_normal();
    if (!pathIsInside(workspace_root, canonical_parent)) {
        return false;
    }

    target = canonical_parent / relative.filename();
    return true;
}

} // namespace

ProjectSessionOpenResult ProjectSession::open(
    const std::filesystem::path& workspace_root) {
    ProjectWorkspaceMetadataService metadata_service;
    auto loaded = metadata_service.load(workspace_root);
    if (!loaded.ok()) {
        return {
            std::nullopt,
            ProjectSessionDiagnostic{
                ProjectSessionErrorCode::project_validation_failed,
                loaded.diagnostic.code,
                std::move(loaded.diagnostic.message),
                std::move(loaded.diagnostic.path),
            },
        };
    }

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(workspace_root, ec);
    if (ec || !std::filesystem::is_directory(canonical, ec) || ec) {
        return {
            std::nullopt,
            ProjectSessionDiagnostic{
                ProjectSessionErrorCode::workspace_resolution_failed,
                ProjectMetadataErrorCode::none,
                "Unable to resolve validated Project workspace root",
                workspace_root,
            },
        };
    }

    ProjectSession session{
        canonical.lexically_normal(),
        std::move(*loaded.metadata),
    };
    static_cast<void>(session.refreshDocuments());

    return {
        std::optional<ProjectSession>{std::move(session)},
        ProjectSessionDiagnostic{},
    };
}

DocumentDiscoveryResult ProjectSession::refreshDocuments() {
    return document_index_.refresh(workspace_root_);
}

ProjectDocumentResult ProjectSession::createPart(
    const std::filesystem::path& workspace_relative_path) {
    std::filesystem::path target;
    if (!resolvePartTarget(workspace_root_, workspace_relative_path, target)) {
        return documentFailure(
            ProjectDocumentErrorCode::invalid_path,
            "Part path must be a .ss2part file inside an existing Workspace directory",
            workspace_relative_path);
    }

    auto document = part::PartDocument::create(core::DocumentId::generate());
    const std::string key{document.documentId().value()};

    part::PartDocumentStore store;
    const auto created = store.createNew(target, document);
    if (!created.ok()) {
        return documentFailure(
            ProjectDocumentErrorCode::persistence_failure,
            created.diagnostic.message,
            created.diagnostic.path,
            created.diagnostic.code);
    }

    const auto refreshed = refreshDocuments();
    if (!refreshed.ok()) {
        return documentFailure(
            ProjectDocumentErrorCode::discovery_failure,
            refreshed.diagnostic.message,
            refreshed.diagnostic.path);
    }

    const auto resolution = document_index_.resolve(document.documentId());
    if (!resolution.ok() ||
        std::filesystem::weakly_canonical(resolution.absolute_path) !=
            std::filesystem::weakly_canonical(target)) {
        return documentFailure(
            resolution.state == DocumentResolutionState::identity_conflict
                ? ProjectDocumentErrorCode::identity_conflict
                : ProjectDocumentErrorCode::discovery_failure,
            resolution.diagnostic.empty()
                ? "New Part could not be resolved to its published path"
                : resolution.diagnostic,
            target,
            part::PartStoreErrorCode::none,
            resolution.relative_paths);
    }

    auto session =
        std::make_unique<DocumentSession>(target, std::move(document));
    auto* session_ptr = session.get();
    open_documents_.emplace(key, std::move(session));
    return ProjectDocumentResult{session_ptr, false, ProjectDocumentDiagnostic{}};
}

ProjectDocumentResult ProjectSession::openDocument(
    const core::DocumentId& document_id) {
    const std::string key{document_id.value()};
    if (const auto existing = open_documents_.find(key);
        existing != open_documents_.end()) {
        return ProjectDocumentResult{
            existing->second.get(),
            true,
            ProjectDocumentDiagnostic{},
        };
    }

    const auto resolution = document_index_.resolve(document_id);
    if (resolution.state == DocumentResolutionState::missing) {
        return documentFailure(
            ProjectDocumentErrorCode::document_missing,
            resolution.diagnostic);
    }
    if (resolution.state == DocumentResolutionState::identity_conflict) {
        return documentFailure(
            ProjectDocumentErrorCode::identity_conflict,
            resolution.diagnostic,
            {},
            part::PartStoreErrorCode::none,
            resolution.relative_paths);
    }

    part::PartDocumentStore store;
    auto loaded = store.load(resolution.absolute_path);
    if (!loaded.ok()) {
        return documentFailure(
            ProjectDocumentErrorCode::persistence_failure,
            loaded.diagnostic.message,
            loaded.diagnostic.path,
            loaded.diagnostic.code);
    }
    if (loaded.document->documentId() != document_id) {
        static_cast<void>(refreshDocuments());
        return documentFailure(
            ProjectDocumentErrorCode::identity_changed,
            "Native Part identity changed after discovery; refresh and retry",
            resolution.absolute_path);
    }

    auto session = std::make_unique<DocumentSession>(
        resolution.absolute_path,
        std::move(*loaded.document));
    auto* session_ptr = session.get();
    open_documents_.emplace(key, std::move(session));
    return ProjectDocumentResult{session_ptr, false, ProjectDocumentDiagnostic{}};
}

DocumentSession* ProjectSession::documentSession(
    const core::DocumentId& document_id) noexcept {
    const auto found = open_documents_.find(std::string{document_id.value()});
    return found == open_documents_.end() ? nullptr : found->second.get();
}

const DocumentSession* ProjectSession::documentSession(
    const core::DocumentId& document_id) const noexcept {
    const auto found = open_documents_.find(std::string{document_id.value()});
    return found == open_documents_.end() ? nullptr : found->second.get();
}

std::vector<core::DocumentId> ProjectSession::openDocumentIds() const {
    std::vector<core::DocumentId> ids;
    ids.reserve(open_documents_.size());
    for (const auto& [key, session] : open_documents_) {
        static_cast<void>(key);
        ids.push_back(session->documentId());
    }
    return ids;
}

bool ProjectSession::closeDocument(
    const core::DocumentId& document_id,
    bool discard_unsaved) noexcept {
    const auto found = open_documents_.find(std::string{document_id.value()});
    if (found == open_documents_.end()) return true;
    if (found->second->needsSave() && !discard_unsaved) return false;
    open_documents_.erase(found);
    return true;
}

bool ProjectSession::hasDirtyDocuments() const noexcept {
    return std::any_of(
        open_documents_.begin(),
        open_documents_.end(),
        [](const auto& entry) { return entry.second->needsSave(); });
}

DocumentSessionResult ProjectSession::saveAllDirtyDocuments() {
    for (auto& [id, session] : open_documents_) {
        static_cast<void>(id);
        if (!session->needsSave()) continue;
        const auto saved = session->save();
        if (!saved.ok()) return saved;
    }
    return DocumentSessionResult{};
}

} // namespace simplesolid2::application
