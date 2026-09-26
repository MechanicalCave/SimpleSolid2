#include <simplesolid2/application/document_session.hpp>

#include <algorithm>
#include <set>
#include <stdexcept>
#include <utility>

namespace simplesolid2::application {
namespace {

DocumentSessionResult failure(
    DocumentSessionErrorCode code,
    std::string message,
    std::filesystem::path path = {},
    part::PartCommitErrorCode commit_code = part::PartCommitErrorCode::none,
    part::PartStoreErrorCode store_code = part::PartStoreErrorCode::none) {
    return DocumentSessionResult{
        false,
        DocumentSessionDiagnostic{
            code,
            commit_code,
            store_code,
            std::move(message),
            std::move(path),
        },
    };
}

DocumentSessionResult success(bool changed = false) {
    return DocumentSessionResult{changed, DocumentSessionDiagnostic{}};
}

part::PartSketch* findSketch(
    part::PartAuthoredState& state,
    const sketch::SketchId& id) noexcept {
    const auto found = std::find_if(
        state.sketches.begin(),
        state.sketches.end(),
        [&id](const part::PartSketch& item) {
            return item.id == id;
        });

    return found == state.sketches.end()
        ? nullptr
        : &*found;
}

} // namespace

DocumentSession::DocumentSession(
    std::filesystem::path path,
    part::PartDocument document)
    : path_{std::move(path)},
      document_{std::move(document)},
      saved_state_{document_.state()},
      expected_revision_{document_.revision()} {
    absorbSketchEntityIdCursors(document_.state());
}

DocumentSessionResult DocumentSession::verifyRevision() const {
    if (document_.revision() != expected_revision_) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Document revision diverged from the active command/history context",
            path_);
    }
    return success();
}

DocumentSessionResult DocumentSession::commitCommandState(
    part::PartAuthoredState after,
    const char* failure_message) {
    if (const auto verified = verifyRevision(); !verified.ok()) {
        return verified;
    }

    applySketchEntityIdCursors(after);

    if (after == document_.state()) {
        return success(false);
    }

    std::vector<HistoryEntry> prepared = history_;
    prepared.resize(cursor_);
    prepared.push_back(HistoryEntry{document_.state(), after});

    part::PartDocumentTransaction transaction{document_};
    transaction.replaceState(std::move(after));
    const auto committed = transaction.commit();
    if (!committed.ok()) {
        return failure(
            DocumentSessionErrorCode::transaction_failure,
            failure_message,
            path_,
            committed.code);
    }
    if (!committed.changed) {
        return success(false);
    }

    history_.swap(prepared);
    cursor_ = history_.size();
    expected_revision_ = document_.revision();
    absorbSketchEntityIdCursors(document_.state());
    return success(true);
}

DocumentSessionResult DocumentSession::execute(
    const SetDocumentPropertiesCommand& command) {
    auto after = document_.state();
    after.properties = command.properties;
    return commitCommandState(
        std::move(after),
        "Part transaction failed while executing document properties command");
}

DocumentSessionResult DocumentSession::execute(
    const SetBuiltinReferenceVisibilityCommand& command) {
    for (const auto role : command.targets) {
        if (!core::isBuiltinReferenceRole(role)) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Visibility command contains an invalid built-in reference role",
                path_);
        }
    }

    auto after = document_.state();
    for (const auto role : command.targets) {
        static_cast<void>(
            after.presentation.builtin_references.setVisible(
                role,
                command.visible));
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while executing built-in reference visibility command");
}

CreatePartSketchResult DocumentSession::execute(
    const CreatePartSketchCommand& command) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            command.support);
    if (!support) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Sketch creation requires XY, XZ or YZ built-in Origin plane support",
            path_);
        return CreatePartSketchResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto placement =
        part::sketchPlacementForSupport(*support);
    if (!placement) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Unable to derive a valid Sketch placement from the selected support",
            path_);
        return CreatePartSketchResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    std::optional<sketch::SketchId> id;
    for (unsigned attempt = 0U;
         attempt < 16U;
         ++attempt) {
        auto candidate =
            sketch::SketchId::generate();
        if (document_.findSketch(candidate) == nullptr) {
            id = std::move(candidate);
            break;
        }
    }

    if (!id) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Unable to allocate a unique SketchId",
            path_);
        return CreatePartSketchResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    after.sketches.push_back(
        part::PartSketch{
            *id,
            *support,
            *placement,
            true});

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating Sketch");
    if (!committed.ok() || !committed.changed) {
        return CreatePartSketchResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return CreatePartSketchResult{
        true,
        *id,
        DocumentSessionDiagnostic{}};
}

AddSketchLineResult DocumentSession::execute(
    const AddSketchLineCommand& command) {
    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Line target SketchId does not exist",
            path_);
        return AddSketchLineResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    std::optional<sketch::EntityId> entity_id;
    try {
        entity_id =
            target->model.addLine(
                command.start,
                command.end);
    } catch (const std::invalid_argument&) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Line contains invalid authored geometry",
            path_);
        return AddSketchLineResult{
            false,
            std::nullopt,
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Sketch EntityId allocation space is exhausted",
            path_);
        return AddSketchLineResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while adding Sketch Line");
    if (!committed.ok() || !committed.changed) {
        return AddSketchLineResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return AddSketchLineResult{
        true,
        *entity_id,
        DocumentSessionDiagnostic{}};
}

DocumentSessionResult DocumentSession::execute(
    const EraseSketchEntityCommand& command) {
    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entity target SketchId does not exist",
            path_);
    }

    if (!target->model.erase(command.entity_id)) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entity target EntityId does not exist",
            path_);
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while erasing Sketch entity");
}

DocumentSessionResult DocumentSession::execute(
    const EraseSketchEntitiesCommand& command) {
    if (command.entity_ids.empty()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entities requires at least one EntityId",
            path_);
    }

    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entities target SketchId does not exist",
            path_);
    }

    std::set<sketch::EntityId> unique;
    for (const auto id : command.entity_ids) {
        if (!id.valid()) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Erase Sketch Entities contains an invalid EntityId",
                path_);
        }

        if (!unique.insert(id).second) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Erase Sketch Entities contains duplicate EntityIds",
                path_);
        }

        if (target->model.findLine(id) == nullptr) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Erase Sketch Entities target EntityId does not exist",
                path_);
        }
    }

    for (const auto id : command.entity_ids) {
        if (!target->model.erase(id)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Erase Sketch Entities validation diverged before commit",
                path_);
        }
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while erasing Sketch entities");
}

DocumentSessionResult DocumentSession::execute(
    const UpdateSketchLinesCommand& command) {
    if (command.lines.empty()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Update Sketch Lines requires at least one Line",
            path_);
    }

    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Update Sketch Lines was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Update Sketch Lines target SketchId does not exist",
            path_);
    }

    std::set<sketch::EntityId> unique;
    for (const auto& line : command.lines) {
        if (!line.entity_id.valid() ||
            !line.start.finite() ||
            !line.end.finite() ||
            line.start == line.end) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Update Sketch Lines contains invalid authored geometry",
                path_);
        }

        if (!unique.insert(line.entity_id).second) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Update Sketch Lines contains duplicate EntityIds",
                path_);
        }

        if (target->model.findLine(
                line.entity_id) == nullptr) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Update Sketch Lines target EntityId does not exist",
                path_);
        }
    }

    for (const auto& line : command.lines) {
        if (!target->model.updateLine(
                line.entity_id,
                line.start,
                line.end)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Update Sketch Lines validation diverged before commit",
                path_);
        }
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while updating Sketch Lines");
}

DocumentSessionResult DocumentSession::applyHistoricalState(
    const part::PartAuthoredState& expected_current,
    const part::PartAuthoredState& target) {
    if (const auto verified = verifyRevision(); !verified.ok()) {
        return verified;
    }
    if (document_.state() != expected_current) {
        return failure(
            DocumentSessionErrorCode::history_diverged,
            "Authored state no longer matches the Undo/Redo history cursor",
            path_);
    }

    auto adjusted_target = target;
    applySketchEntityIdCursors(adjusted_target);

    part::PartDocumentTransaction transaction{document_};
    transaction.replaceState(std::move(adjusted_target));
    const auto committed = transaction.commit();
    if (!committed.ok() || !committed.changed) {
        return failure(
            DocumentSessionErrorCode::transaction_failure,
            "Part transaction failed while applying Undo/Redo",
            path_,
            committed.code);
    }

    expected_revision_ = document_.revision();
    absorbSketchEntityIdCursors(document_.state());
    return success(true);
}

void DocumentSession::absorbSketchEntityIdCursors(
    const part::PartAuthoredState& state) {
    for (const auto& hosted : state.sketches) {
        const auto observed =
            hosted.model.entityIdCursor();

        const auto found =
            sketch_entity_id_cursors_.find(
                hosted.id);
        if (found ==
            sketch_entity_id_cursors_.end()) {
            sketch_entity_id_cursors_.emplace(
                hosted.id,
                observed);
            continue;
        }

        if (observed > found->second) {
            found->second = observed;
        }
    }
}

void DocumentSession::applySketchEntityIdCursors(
    part::PartAuthoredState& state) const {
    for (auto& hosted : state.sketches) {
        const auto found =
            sketch_entity_id_cursors_.find(
                hosted.id);
        if (found ==
            sketch_entity_id_cursors_.end()) {
            continue;
        }

        hosted.model.preserveEntityIdCursor(
            found->second);
    }
}

DocumentSessionResult DocumentSession::undo() {
    if (!canUndo()) return success(false);

    const auto& entry = history_[cursor_ - 1U];
    auto applied = applyHistoricalState(entry.after, entry.before);
    if (!applied.ok()) return applied;
    --cursor_;
    return applied;
}

DocumentSessionResult DocumentSession::redo() {
    if (!canRedo()) return success(false);

    const auto& entry = history_[cursor_];
    auto applied = applyHistoricalState(entry.before, entry.after);
    if (!applied.ok()) return applied;
    ++cursor_;
    return applied;
}

DocumentSessionResult DocumentSession::save() {
    if (const auto verified = verifyRevision(); !verified.ok()) {
        return verified;
    }

    const auto saved = store_.save(path_, document_);
    if (!saved.ok()) {
        return failure(
            DocumentSessionErrorCode::persistence_failure,
            saved.diagnostic.message,
            saved.diagnostic.path,
            part::PartCommitErrorCode::none,
            saved.diagnostic.code);
    }

    saved_state_ = document_.state();
    return success(false);
}

} // namespace simplesolid2::application
