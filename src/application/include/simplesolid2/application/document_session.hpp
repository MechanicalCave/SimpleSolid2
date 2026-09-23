#pragma once

#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cstddef>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace simplesolid2::application {

struct SetDocumentPropertiesCommand final {
    core::DocumentProperties properties;
};

enum class DocumentSessionErrorCode {
    none,
    revision_diverged,
    history_diverged,
    transaction_failure,
    persistence_failure,
};

struct DocumentSessionDiagnostic final {
    DocumentSessionErrorCode code{DocumentSessionErrorCode::none};
    part::PartCommitErrorCode commit_code{part::PartCommitErrorCode::none};
    part::PartStoreErrorCode store_code{part::PartStoreErrorCode::none};
    std::string message;
    std::filesystem::path path;
};

struct DocumentSessionResult final {
    bool changed{false};
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentSessionErrorCode::none;
    }
};

class DocumentSession final {
public:
    DocumentSession(std::filesystem::path path, part::PartDocument document);

    DocumentSession(const DocumentSession&) = delete;
    DocumentSession& operator=(const DocumentSession&) = delete;
    DocumentSession(DocumentSession&&) noexcept = default;
    DocumentSession& operator=(DocumentSession&&) noexcept = default;
    ~DocumentSession() = default;

    [[nodiscard]] const core::DocumentId& documentId() const noexcept {
        return document_.documentId();
    }
    [[nodiscard]] const std::filesystem::path& path() const noexcept { return path_; }
    [[nodiscard]] const part::PartDocument& document() const noexcept { return document_; }

    [[nodiscard]] bool needsSave() const noexcept {
        return document_.state() != saved_state_;
    }
    [[nodiscard]] bool canUndo() const noexcept { return cursor_ > 0U; }
    [[nodiscard]] bool canRedo() const noexcept { return cursor_ < history_.size(); }
    [[nodiscard]] std::size_t undoDepth() const noexcept { return cursor_; }
    [[nodiscard]] std::size_t redoDepth() const noexcept {
        return history_.size() - cursor_;
    }

    [[nodiscard]] DocumentSessionResult execute(
        const SetDocumentPropertiesCommand& command);
    [[nodiscard]] DocumentSessionResult undo();
    [[nodiscard]] DocumentSessionResult redo();
    [[nodiscard]] DocumentSessionResult save();

private:
    struct HistoryEntry final {
        part::PartAuthoredState before;
        part::PartAuthoredState after;
    };

    [[nodiscard]] DocumentSessionResult verifyRevision() const;
    [[nodiscard]] DocumentSessionResult applyHistoricalState(
        const part::PartAuthoredState& expected_current,
        const part::PartAuthoredState& target);

    std::filesystem::path path_;
    part::PartDocument document_;
    part::PartAuthoredState saved_state_;
    core::DocumentRevision expected_revision_;
    std::vector<HistoryEntry> history_;
    std::size_t cursor_{0};
    part::PartDocumentStore store_;
};

} // namespace simplesolid2::application
