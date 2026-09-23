#include <simplesolid2/application/document_workspace.hpp>

#include <algorithm>
#include <map>
#include <system_error>
#include <utility>

namespace simplesolid2::application {
namespace {

struct ValidDiscovered final {
    core::DocumentId id;
    std::string title;
    std::filesystem::path relative_path;
};

DocumentDiscoveryResult discoveryFailure(
    DocumentDiscoveryErrorCode code,
    std::string message,
    std::filesystem::path path) {
    return DocumentDiscoveryResult{
        DocumentDiscoveryDiagnostic{code, std::move(message), std::move(path)},
    };
}

} // namespace

DocumentDiscoveryResult DocumentWorkspaceIndex::refresh(
    const std::filesystem::path& workspace_root) {
    entries_.clear();
    last_diagnostic_ = {};

    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(workspace_root, ec);
    if (ec || !std::filesystem::is_directory(canonical, ec) || ec) {
        last_diagnostic_ = DocumentDiscoveryDiagnostic{
            DocumentDiscoveryErrorCode::invalid_workspace,
            "Unable to resolve Project Workspace for document discovery",
            workspace_root,
        };
        return DocumentDiscoveryResult{last_diagnostic_};
    }

    workspace_root_ = canonical.lexically_normal();

    std::vector<ValidDiscovered> valid;
    part::PartDocumentStore store;

    std::filesystem::recursive_directory_iterator iterator{
        workspace_root_,
        std::filesystem::directory_options::skip_permission_denied,
        ec};
    const std::filesystem::recursive_directory_iterator end;
    if (ec) {
        last_diagnostic_ = DocumentDiscoveryDiagnostic{
            DocumentDiscoveryErrorCode::filesystem_failure,
            "Unable to begin native document discovery",
            workspace_root_,
        };
        return DocumentDiscoveryResult{last_diagnostic_};
    }

    for (; iterator != end; iterator.increment(ec)) {
        if (ec) {
            last_diagnostic_ = DocumentDiscoveryDiagnostic{
                DocumentDiscoveryErrorCode::filesystem_failure,
                "Document discovery encountered an inaccessible filesystem entry",
                workspace_root_,
            };
            ec.clear();
            continue;
        }

        const auto& entry = *iterator;
        if (entry.is_directory(ec) && !ec &&
            entry.path().filename() == ".simplesolid") {
            iterator.disable_recursion_pending();
            continue;
        }
        ec.clear();

        if (!entry.is_regular_file(ec) || ec) {
            ec.clear();
            continue;
        }
        if (!part::PartDocumentStore::hasNativeExtension(entry.path())) {
            continue;
        }

        auto relative = std::filesystem::relative(entry.path(), workspace_root_, ec);
        if (ec) {
            relative = entry.path().filename();
            ec.clear();
        }
        relative = relative.lexically_normal();

        auto loaded = store.load(entry.path());
        if (!loaded.ok()) {
            DocumentIndexEntry invalid;
            invalid.state = DocumentIndexState::invalid;
            invalid.relative_paths.push_back(relative);
            invalid.store_code = loaded.diagnostic.code;
            invalid.diagnostic = loaded.diagnostic.message;
            entries_.push_back(std::move(invalid));
            continue;
        }

        valid.push_back(ValidDiscovered{
            loaded.document->documentId(),
            loaded.document->properties().title,
            std::move(relative),
        });
    }

    std::map<std::string, std::vector<ValidDiscovered>> grouped;
    for (auto& item : valid) {
        grouped[std::string{item.id.value()}].push_back(std::move(item));
    }

    for (auto& [key, group] : grouped) {
        std::sort(
            group.begin(),
            group.end(),
            [](const ValidDiscovered& a, const ValidDiscovered& b) {
                return a.relative_path.generic_string() <
                       b.relative_path.generic_string();
            });

        DocumentIndexEntry indexed;
        indexed.document_id = group.front().id;
        indexed.title = group.front().title;
        for (const auto& item : group) {
            indexed.relative_paths.push_back(item.relative_path);
        }

        if (group.size() == 1U) {
            indexed.state = DocumentIndexState::resolved;
        } else {
            indexed.state = DocumentIndexState::identity_conflict;
            indexed.diagnostic =
                "Multiple native Documents in this Workspace declare the same DocumentId";
        }
        entries_.push_back(std::move(indexed));
    }

    std::sort(
        entries_.begin(),
        entries_.end(),
        [](const DocumentIndexEntry& a, const DocumentIndexEntry& b) {
            const auto a_path = a.relative_paths.empty()
                                    ? std::string{}
                                    : a.relative_paths.front().generic_string();
            const auto b_path = b.relative_paths.empty()
                                    ? std::string{}
                                    : b.relative_paths.front().generic_string();
            return a_path < b_path;
        });

    return DocumentDiscoveryResult{last_diagnostic_};
}

DocumentResolution DocumentWorkspaceIndex::resolve(
    const core::DocumentId& document_id) const {
    for (const auto& entry : entries_) {
        if (!entry.document_id || *entry.document_id != document_id) continue;

        if (entry.state == DocumentIndexState::identity_conflict) {
            return DocumentResolution{
                DocumentResolutionState::identity_conflict,
                {},
                entry.relative_paths,
                entry.diagnostic,
            };
        }
        if (entry.state == DocumentIndexState::resolved &&
            entry.relative_paths.size() == 1U) {
            return DocumentResolution{
                DocumentResolutionState::resolved,
                workspace_root_ / entry.relative_paths.front(),
                entry.relative_paths,
                {},
            };
        }
    }

    return DocumentResolution{
        DocumentResolutionState::missing,
        {},
        {},
        "DocumentId is not present in the current Workspace",
    };
}

} // namespace simplesolid2::application
