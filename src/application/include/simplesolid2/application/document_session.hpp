#pragma once

#include <simplesolid2/core/units.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/structural_edit.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <cstddef>
#include <filesystem>
#include <map>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace simplesolid2::application {

struct SetDocumentPropertiesCommand final {
    core::DocumentProperties properties;
};

struct SetPartLengthUnitCommand final {
    core::LengthUnit unit{
        core::LengthUnit::millimetre};
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
    sketch::EntityRole role{sketch::EntityRole::regular};
};

struct AddSketchCircleCommand final {
    sketch::SketchId sketch_id;
    sketch::Point2 center;
    double radius{};
    sketch::EntityRole role{sketch::EntityRole::regular};
};

struct AddSketchArcCommand final {
    sketch::SketchId sketch_id;
    sketch::Point2 center;
    double radius{};
    double start_angle{};
    double sweep_angle{};
    sketch::EntityRole role{sketch::EntityRole::regular};
};

struct AddSketchRectangleCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    sketch::Point2 first_corner;
    sketch::Point2 opposite_corner;
    sketch::EntityRole perimeter_role{
        sketch::EntityRole::regular};
    bool draw_diagonals{};
};

struct EraseSketchEntityCommand final {
    sketch::SketchId sketch_id;
    sketch::EntityId entity_id;
};

struct EraseSketchEntitiesCommand final {
    sketch::SketchId sketch_id;
    std::vector<sketch::EntityId> entity_ids;
};

struct SetSketchEntityRoleCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    std::vector<sketch::EntityId> entity_ids;
    sketch::EntityRole role{sketch::EntityRole::regular};
};

struct SketchLineGeometryUpdate final {
    sketch::EntityId entity_id;
    sketch::Point2 start;
    sketch::Point2 end;
};

struct SketchCircleGeometryUpdate final {
    sketch::EntityId entity_id;
    sketch::Point2 center;
    double radius{};
};

struct SketchArcGeometryUpdate final {
    sketch::EntityId entity_id;
    sketch::Point2 center;
    double radius{};
    double start_angle{};
    double sweep_angle{};
};

struct UpdateSketchLinesCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    std::vector<SketchLineGeometryUpdate> lines;
};

struct UpdateSketchGeometryCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    std::vector<SketchLineGeometryUpdate> lines;
    std::vector<SketchCircleGeometryUpdate> circles;
    std::vector<SketchArcGeometryUpdate> arcs;
};

struct TrimSketchCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    sketch::EntityId target;
    std::vector<sketch::EntityId> boundaries;
    sketch::Point2 pick;
};

struct ExtendSketchCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    sketch::EntityId target;
    std::vector<sketch::EntityId> boundaries;
    sketch::StructuralEndpointRole endpoint{
        sketch::StructuralEndpointRole::end};
};

struct ExtendBothSketchLinesCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    sketch::EntityId first_line;
    sketch::EntityId second_line;
};

struct DuplicateSketchGeometryCommand final {
    sketch::SketchId sketch_id;
    core::DocumentRevision expected_revision;
    sketch::SketchTransformGeometry geometry;
};

struct CreateProfileCommand final {
    sketch::SketchId source_sketch_id;
    core::DocumentRevision expected_revision;
    part::ProfileRegionIntent region_intent;
};

struct ReplaceProfileRegionIntentCommand final {
    part::ProfileId profile_id;
    core::DocumentRevision expected_revision;
    part::ProfileRegionIntent region_intent;
};

struct SetProfilePropertiesCommand final {
    part::ProfileId profile_id;
    core::DocumentRevision expected_revision;
    std::string name;
    bool visible{true};
};

struct DeleteProfileCommand final {
    part::ProfileId profile_id;
    core::DocumentRevision expected_revision;
};

enum class DocumentSessionErrorCode {
    none,
    invalid_command,
    revision_diverged,
    history_diverged,
    transaction_failure,
    persistence_failure,
    save_conflict,
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

struct AddSketchCircleResult final {
    bool changed{false};
    std::optional<sketch::EntityId> entity_id;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentSessionErrorCode::none;
    }
};

struct AddSketchArcResult final {
    bool changed{false};
    std::optional<sketch::EntityId> entity_id;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentSessionErrorCode::none;
    }
};

struct AddSketchRectangleResult final {
    bool changed{false};
    std::vector<sketch::EntityId> entity_ids;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentSessionErrorCode::none;
    }
};

struct SketchStructuralEditCommandResult final {
    bool changed{false};
    sketch::StructuralEditStatus edit_status{
        sketch::StructuralEditStatus::invalid_request};
    std::optional<sketch::EntityId> result_entity;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code ==
               DocumentSessionErrorCode::none;
    }
};

struct DuplicateSketchGeometryResult final {
    bool changed{false};
    std::vector<sketch::EntityId> entity_ids;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code == DocumentSessionErrorCode::none;
    }
};

struct CreateProfileResult final {
    bool changed{false};
    std::optional<part::ProfileId> profile_id;
    DocumentSessionDiagnostic diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return diagnostic.code ==
               DocumentSessionErrorCode::none;
    }
};

class DocumentSession final {
public:
    DocumentSession(
        std::filesystem::path path,
        part::PartDocument document);
    DocumentSession(
        std::filesystem::path path,
        part::PartDocument document,
        part::PartFileCheckpoint file_checkpoint);

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
    [[nodiscard]] const std::optional<
        part::PartFileCheckpoint>&
    fileCheckpoint() const noexcept {
        return file_checkpoint_;
    }

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
        const SetPartLengthUnitCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const SetBuiltinReferenceVisibilityCommand& command);
    [[nodiscard]] CreatePartSketchResult execute(
        const CreatePartSketchCommand& command);
    [[nodiscard]] AddSketchLineResult execute(
        const AddSketchLineCommand& command);
    [[nodiscard]] AddSketchCircleResult execute(
        const AddSketchCircleCommand& command);
    [[nodiscard]] AddSketchArcResult execute(
        const AddSketchArcCommand& command);
    [[nodiscard]] AddSketchRectangleResult execute(
        const AddSketchRectangleCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const EraseSketchEntityCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const EraseSketchEntitiesCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const SetSketchEntityRoleCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const UpdateSketchLinesCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const UpdateSketchGeometryCommand& command);
    [[nodiscard]] SketchStructuralEditCommandResult execute(
        const TrimSketchCommand& command);
    [[nodiscard]] SketchStructuralEditCommandResult execute(
        const ExtendSketchCommand& command);
    [[nodiscard]] SketchStructuralEditCommandResult execute(
        const ExtendBothSketchLinesCommand& command);
    [[nodiscard]] DuplicateSketchGeometryResult execute(
        const DuplicateSketchGeometryCommand& command);
    [[nodiscard]] CreateProfileResult execute(
        const CreateProfileCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const ReplaceProfileRegionIntentCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const SetProfilePropertiesCommand& command);
    [[nodiscard]] DocumentSessionResult execute(
        const DeleteProfileCommand& command);
    [[nodiscard]] DocumentSessionResult undo();
    [[nodiscard]] DocumentSessionResult redo();
    [[nodiscard]] DocumentSessionResult save();

private:
    using SketchEntityIdCursorMap =
        std::map<sketch::SketchId, sketch::EntityIdCursor>;

    struct HistoryEntry final {
        part::PartAuthoredState before;
        part::PartAuthoredState after;

        HistoryEntry(
            const part::PartAuthoredState& before_state,
            part::PartAuthoredState after_state)
            : before{before_state},
              after{std::move(after_state)} {}

        HistoryEntry(const HistoryEntry&) = delete;
        HistoryEntry& operator=(const HistoryEntry&) = delete;
        HistoryEntry(HistoryEntry&&) noexcept = default;
        HistoryEntry& operator=(HistoryEntry&&) noexcept = default;
    };

    static_assert(
        !std::is_copy_constructible_v<HistoryEntry>);
    static_assert(
        !std::is_copy_assignable_v<HistoryEntry>);
    static_assert(
        std::is_nothrow_move_constructible_v<
            part::PartAuthoredState>);
    static_assert(
        std::is_nothrow_move_assignable_v<
            part::PartAuthoredState>);
    static_assert(
        std::is_nothrow_move_constructible_v<
            HistoryEntry>);
    static_assert(
        std::is_nothrow_move_assignable_v<
            HistoryEntry>);
    static_assert(
        noexcept(
            std::declval<SketchEntityIdCursorMap&>().swap(
                std::declval<SketchEntityIdCursorMap&>())));

    [[nodiscard]] DocumentSessionResult verifyRevision() const;
    [[nodiscard]] DocumentSessionResult commitCommandState(
        part::PartAuthoredState after,
        const char* failure_message);
    [[nodiscard]] DocumentSessionResult applyHistoricalState(
        const part::PartAuthoredState& expected_current,
        const part::PartAuthoredState& target);

    static void absorbSketchEntityIdCursors(
        SketchEntityIdCursorMap& cursors,
        const part::PartAuthoredState& state);
    void absorbSketchEntityIdCursors(
        const part::PartAuthoredState& state);
    void applySketchEntityIdCursors(
        part::PartAuthoredState& state) const;
    void applyProfileIdCursor(
        part::PartAuthoredState& state) const noexcept;

    std::filesystem::path path_;
    part::PartDocument document_;
    part::PartAuthoredState saved_state_;
    std::optional<part::PartFileCheckpoint>
        file_checkpoint_;
    core::DocumentRevision expected_revision_;
    std::vector<HistoryEntry> history_;
    std::size_t cursor_{0};
    SketchEntityIdCursorMap sketch_entity_id_cursors_;
    part::ProfileIdCursor profile_id_cursor_;
    part::PartDocumentStore store_;
};

} // namespace simplesolid2::application
