#include <simplesolid2/part/part_document.hpp>

#include <algorithm>

namespace simplesolid2::part {

PartDocument PartDocument::create(core::DocumentId id) {
    return PartDocument{std::move(id), PartAuthoredState{}, core::DocumentRevision{}};
}

PartReconstructResult PartDocument::restore(
    core::DocumentId id,
    PartAuthoredState state,
    core::DocumentRevision revision) {
    if (!validAuthoredState(state)) {
        return PartReconstructResult{
            std::nullopt,
            PartReconstructErrorCode::invalid_state};
    }

    return PartReconstructResult{
        std::optional<PartDocument>{
            PartDocument{
                std::move(id),
                std::move(state),
                revision}},
        PartReconstructErrorCode::none};
}

const PartSketch* PartDocument::findSketch(
    const sketch::SketchId& id) const noexcept {
    const auto found = std::find_if(
        state_.sketches.begin(),
        state_.sketches.end(),
        [&id](const PartSketch& item) {
            return item.id == id;
        });

    return found == state_.sketches.end()
        ? nullptr
        : &*found;
}

bool PartDocument::validAuthoredState(
    const PartAuthoredState& state) noexcept {
    for (std::size_t index = 0;
         index < state.sketches.size();
         ++index) {
        const auto& hosted = state.sketches[index];
        if (!hosted.support.valid() ||
            !hosted.placement.valid() ||
            !sketchPlacementMatchesSupport(
                hosted.placement,
                hosted.support)) {
            return false;
        }

        for (std::size_t previous = 0;
             previous < index;
             ++previous) {
            if (state.sketches[previous].id ==
                hosted.id) {
                return false;
            }
        }
    }

    return true;
}

PartCommitResult PartDocument::commitState(
    core::DocumentRevision expected_revision,
    PartAuthoredState state) {
    if (expected_revision != revision_) {
        return PartCommitResult{
            PartCommitErrorCode::stale_transaction,
            false};
    }

    if (!validAuthoredState(state)) {
        return PartCommitResult{
            PartCommitErrorCode::invalid_state,
            false};
    }

    if (state == state_) {
        return PartCommitResult{
            PartCommitErrorCode::none,
            false};
    }

    const auto next = revision_.next();
    if (!next) {
        return PartCommitResult{
            PartCommitErrorCode::revision_exhausted,
            false};
    }

    state_ = std::move(state);
    revision_ = *next;
    return PartCommitResult{
        PartCommitErrorCode::none,
        true};
}

PartCommitResult PartDocumentTransaction::commit() {
    if (!active_ || document_ == nullptr) {
        return PartCommitResult{
            PartCommitErrorCode::inactive_transaction,
            false};
    }

    active_ = false;
    return document_->commitState(
        base_revision_,
        std::move(staged_));
}

} // namespace simplesolid2::part
