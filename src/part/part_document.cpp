#include <simplesolid2/part/part_document.hpp>

namespace simplesolid2::part {

PartDocument PartDocument::create(core::DocumentId id) {
    return PartDocument{std::move(id), PartAuthoredState{}, core::DocumentRevision{}};
}

PartDocument PartDocument::restore(
    core::DocumentId id,
    PartAuthoredState state,
    core::DocumentRevision revision) {
    return PartDocument{std::move(id), std::move(state), revision};
}

PartCommitResult PartDocument::commitState(PartAuthoredState state) {
    if (state == state_) {
        return PartCommitResult{PartCommitErrorCode::none, false};
    }

    const auto next = revision_.next();
    if (!next) {
        return PartCommitResult{PartCommitErrorCode::revision_exhausted, false};
    }

    state_ = std::move(state);
    revision_ = *next;
    return PartCommitResult{PartCommitErrorCode::none, true};
}

PartCommitResult PartDocumentTransaction::commit() {
    if (!active_ || document_ == nullptr) {
        return PartCommitResult{PartCommitErrorCode::inactive_transaction, false};
    }

    const auto result = document_->commitState(std::move(staged_));
    if (result.ok()) {
        active_ = false;
    }
    return result;
}

} // namespace simplesolid2::part
