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

const PartFeature* PartDocument::findFeature(
    FeatureId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found = std::find_if(
        state_.body.features.begin(),
        state_.body.features.end(),
        [id](const PartFeature& item) {
            return item.id == id;
        });
    return found == state_.body.features.end()
        ? nullptr
        : &*found;
}

bool PartDocument::profilePresentationVisible(
    ProfileId id) const noexcept {
    const auto* profile = findProfile(id);
    if (profile == nullptr) {
        return false;
    }

    switch (profile->visibility) {
    case ProfileVisibilityPolicy::force_shown:
        return true;
    case ProfileVisibilityPolicy::force_hidden:
        return false;
    case ProfileVisibilityPolicy::automatic:
        break;
    }

    for (const auto& feature : state_.body.features) {
        if (feature.suppressed) continue;
        const auto source = sourceProfileId(feature);
        if (source && *source == id) {
            return false;
        }
    }
    return true;
}

const PartProfile* PartDocument::findProfile(
    ProfileId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found = std::find_if(
        state_.profiles.begin(),
        state_.profiles.end(),
        [id](const PartProfile& item) {
            return item.id == id;
        });
    return found == state_.profiles.end()
        ? nullptr
        : &*found;
}

std::optional<ResolvedProfileRegion>
PartDocument::evaluateProfile(
    ProfileId id) const {
    const auto* profile = findProfile(id);
    if (profile == nullptr) {
        return std::nullopt;
    }
    const auto* source =
        findSketch(profile->source_sketch_id);
    if (source == nullptr) {
        return std::nullopt;
    }
    return resolveProfileRegionIntent(
        source->model,
        profile->region_intent);
}

bool PartDocument::validAuthoredState(
    const PartAuthoredState& state) noexcept {
    if (!core::isLengthUnit(
            state.length_unit) ||
        !state.modeling_semantics_version.valid() ||
        state.modeling_semantics_version !=
            current_modeling_semantics_version ||
        !state.body.id.valid() ||
        !state.next_body_id.containsAllocated(
            state.body.id)) {
        return false;
    }

    for (std::size_t index = 0;
         index < state.sketches.size();
         ++index) {
        const auto& hosted = state.sketches[index];
        if (!hosted.id.valid() ||
            !hosted.support.valid()) {
            return false;
        }

        if (const auto* surface =
                bodyPlanarSurfaceReference(
                    hosted.support)) {
            const auto producer =
                std::find_if(
                    state.body.features.begin(),
                    state.body.features.end(),
                    [surface](const PartFeature& feature) {
                        return feature.id ==
                               surface->surface
                                   .producer_feature_id;
                    });
            const auto stage =
                std::find_if(
                    state.body.features.begin(),
                    state.body.features.end(),
                    [surface](const PartFeature& feature) {
                        return surface->stage
                                   .feature_id &&
                               feature.id ==
                                   *surface->stage
                                        .feature_id;
                    });
            if (producer ==
                    state.body.features.end() ||
                stage ==
                    state.body.features.end() ||
                producer > stage) {
                return false;
            }
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

    for (std::size_t index = 0U;
         index < state.profiles.size();
         ++index) {
        const auto& profile =
            state.profiles[index];

        if (!profile.id.valid() ||
            !state.next_profile_id
                 .containsAllocated(profile.id) ||
            !isProfileVisibilityPolicy(
                profile.visibility) ||
            !profileRegionIntentStructurallyValid(
                profile.region_intent)) {
            return false;
        }

        const auto source =
            std::find_if(
                state.sketches.begin(),
                state.sketches.end(),
                [&profile](const PartSketch& sketch) {
                    return sketch.id ==
                           profile.source_sketch_id;
                });
        if (source == state.sketches.end()) {
            return false;
        }

        for (std::size_t previous = 0U;
             previous < index;
             ++previous) {
            if (state.profiles[previous].id ==
                profile.id) {
                return false;
            }
        }
    }

    for (std::size_t index = 0U;
         index < state.body.features.size();
         ++index) {
        const auto& feature =
            state.body.features[index];
        if (!feature.id.valid() ||
            !state.body.next_feature_id
                 .containsAllocated(feature.id) ||
            !partFeatureDefinitionStructurallyValid(
                feature.definition)) {
            return false;
        }

        const auto source = sourceProfileId(feature);
        if (!source ||
            !state.next_profile_id
                 .containsAllocated(*source)) {
            return false;
        }

        for (std::size_t previous = 0U;
             previous < index;
             ++previous) {
            if (state.body.features[previous].id ==
                feature.id) {
                return false;
            }
        }
    }

    return true;
}

// AUDIT-01 B2 authority boundary: base revision freshness and complete
// Part authored-state validity are checked here before any state replacement.
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
