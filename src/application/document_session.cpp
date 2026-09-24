#include <simplesolid2/application/document_session.hpp>

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

} // namespace

DocumentSession::DocumentSession(
    std::filesystem::path path,
    part::PartDocument document)
    : path_{std::move(path)},
      document_{std::move(document)},
      saved_state_{document_.state()},
      expected_revision_{document_.revision()} {}

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

    part::PartDocumentTransaction transaction{document_};
    transaction.replaceState(target);
    const auto committed = transaction.commit();
    if (!committed.ok() || !committed.changed) {
        return failure(
            DocumentSessionErrorCode::transaction_failure,
            "Part transaction failed while applying Undo/Redo",
            path_,
            committed.code);
    }

    expected_revision_ = document_.revision();
    return success(true);
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
