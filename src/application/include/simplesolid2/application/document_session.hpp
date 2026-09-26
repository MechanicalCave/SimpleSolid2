#pragma once

#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace simplesolid2::application {

struct SetDocumentPropertiesCommand final {
    core::DocumentProperties properties;
};

struct SetBuiltinReferenceVisibilityCommand final {
    std::vector<core::BuiltinReferenceRole> targets;
    bool visible{true};
};

struct CreatePartSketchCommand final {
    core::BuiltinReferenceRole support{
        core::BuiltinReferenceRole::xy_plane};
};

struct AddSketchLineCommand final {
    sketch::SketchId sketch_id;
    sketch::Point2 start;
    sketch::Point2 end;
};

struct EraseSketchEntityCommand final {
    sketch::SketchId sketch_id;
    sketch::EntityId entity_id;
};

struct EraseSketchEntitiesCommand final {
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> entity_ids;
};

struct SketchLineGeometryUpdate final {
    sketch::EntityId entity_id;
    sketch::Point2 start;
    sketch::Point2 end;
};

struct UpdateSketchLinesCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    std::vector<SketchLineGeometryUpdate> lines;
};

enum class DocumentSessionErrorCode {
    none,
    invalid_command,
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

struct CreatePartSketchResult final {
    bool changed{false};
    std::optional<sketch::SketchId> sketch_id;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentSessionErrorCode::none;
    }
};

struct AddSketchLineResult final {
    bool changed{false};
    std::optional<sketch::EntityId> entity_id;
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
    [[nodiscard]] DocumentSessionResult execute(
        const SetBuiltinReferenceVisibilityCommand& command);
    [[nodiscard]] CreatePartSketchResult execute(
        const CreatePartSketchCommand& command);
    [[nodiscard]] AddSketchLineResult execute(
        const AddSketchLineCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const EraseSketchEntityCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const EraseSketchEntitiesCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const UpdateSketchLinesCommand& command);
    [[nodiscard]] DocumentSessionResult undo();
    [[nodiscard]] DocumentSessionResult redo();
    [[nodiscard]] DocumentSessionResult save();

private:
    struct HistoryEntry final {
        part::PartAuthoredState before;
        part::PartAuthoredState after;
    };

    [[nodiscard]] DocumentSessionResult verifyRevision() const;
    [[nodiscard]] DocumentSessionResult commitCommandState(
        part::PartAuthoredState after,
        const char* failure_message);
    [[nodiscard]] DocumentSessionResult applyHistoricalState(
        const part::PartAuthoredState& expected_current,
        const part::PartAuthoredState& target);

    void absorbSketchEntityIdCursors(
        const part::PartAuthoredState& state);
    void applySketchEntityIdCursors(
        part::PartAuthoredState& state) const;

    std::filesystem::path path_;
    part::PartDocument document_;
    part::PartAuthoredState saved_state_;
    core::DocumentRevision expected_revision_;
    std::vector<HistoryEntry> history_;
    std::size_t cursor_{0};
    std::map<sketch::SketchId, sketch::EntityIdCursor>
        sketch_entity_id_cursors_;
    part::PartDocumentStore store_;
};

} // namespace simplesolid2::application
