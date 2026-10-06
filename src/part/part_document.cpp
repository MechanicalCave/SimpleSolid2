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

const OffsetDatumPlane* PartDocument::findDatumPlane(
    DatumId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found = std::find_if(
        state_.datum_planes.begin(),
        state_.datum_planes.end(),
        [id](const OffsetDatumPlane& item) {
            return item.id == id;
        });
    return found == state_.datum_planes.end()
        ? nullptr
        : &*found;
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

const PartAxis* PartDocument::findAxis(
    AxisId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found = std::find_if(
        state_.axes.begin(),
        state_.axes.end(),
        [id](const PartAxis& item) {
            return item.id == id;
        });
    return found == state_.axes.end()
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

    const auto valid_surface_reference =
        [&state](const SurfaceReference& surface) {
            const auto producer =
                std::find_if(
                    state.body.features.begin(),
                    state.body.features.end(),
                    [&surface](const PartFeature& feature) {
                        return feature.id ==
                               surface.surface
                                   .producer_feature_id;
                    });
            const auto stage =
                std::find_if(
                    state.body.features.begin(),
                    state.body.features.end(),
                    [&surface](const PartFeature& feature) {
                        return surface.stage.feature_id &&
                               feature.id ==
                                   *surface.stage.feature_id;
                    });
            return producer !=
                       state.body.features.end() &&
                   stage !=
                       state.body.features.end() &&
                   producer <= stage;
        };

    for (std::size_t index = 0U;
         index < state.datum_planes.size();
         ++index) {
        const auto& datum =
            state.datum_planes[index];
        if (!offsetDatumPlaneStructurallyValid(
                datum) ||
            !state.next_datum_id
                 .containsAllocated(datum.id)) {
            return false;
        }

        if (const auto* surface =
                bodyPlanarSurfaceForPlaneReference(
                    datum.source);
            surface != nullptr &&
            !valid_surface_reference(*surface)) {
            return false;
        }

        if (const auto source_datum =
                datumPlaneIdForPlaneReference(
                    datum.source)) {
            const auto source =
                std::find_if(
                    state.datum_planes.begin(),
                    state.datum_planes.end(),
                    [source_datum](
                        const OffsetDatumPlane& item) {
                        return item.id ==
                               *source_datum;
                    });
            if (source ==
                state.datum_planes.end()) {
                return false;
            }
        }

        for (std::size_t previous = 0U;
             previous < index;
             ++previous) {
            if (state.datum_planes[previous].id ==
                datum.id) {
                return false;
            }
        }
    }

    // PM-03A bounded Datum dependency invariant. Datum references form a
    // single-parent local graph in this package; cycles fail closed without
    // introducing a universal Part dependency graph.
    for (const auto& root : state.datum_planes) {
        std::vector<DatumId> path;
        const OffsetDatumPlane* current = &root;

        while (current != nullptr) {
            if (std::find(
                    path.begin(),
                    path.end(),
                    current->id) != path.end()) {
                return false;
            }
            path.push_back(current->id);

            const auto source_id =
                datumPlaneIdForPlaneReference(
                    current->source);
            if (!source_id) {
                break;
            }

            const auto source =
                std::find_if(
                    state.datum_planes.begin(),
                    state.datum_planes.end(),
                    [source_id](
                        const OffsetDatumPlane& item) {
                        return item.id ==
                               *source_id;
                    });
            if (source ==
                state.datum_planes.end()) {
                return false;
            }
            current = &*source;
        }
    }

    const auto required_body_stage_for_support =
        [&state](
            const PartSketchSupport& support)
            -> std::optional<BodyStageRef> {
        if (const auto* surface =
                bodyPlanarSurfaceReference(
                    support)) {
            return surface->stage;
        }

        auto datum_id =
            datumPlaneIdForSketchSupport(
                support);
        std::vector<DatumId> visited;
        while (datum_id) {
            if (std::find(
                    visited.begin(),
                    visited.end(),
                    *datum_id) !=
                visited.end()) {
                return std::nullopt;
            }
            visited.push_back(*datum_id);

            const auto datum =
                std::find_if(
                    state.datum_planes.begin(),
                    state.datum_planes.end(),
                    [datum_id](
                        const OffsetDatumPlane& item) {
                        return item.id == *datum_id;
                    });
            if (datum ==
                state.datum_planes.end()) {
                return std::nullopt;
            }

            if (const auto* surface =
                    bodyPlanarSurfaceForPlaneReference(
                        datum->source)) {
                return surface->stage;
            }

            datum_id =
                datumPlaneIdForPlaneReference(
                    datum->source);
        }

        return std::nullopt;
    };

    for (std::size_t index = 0;
         index < state.sketches.size();
         ++index) {
        const auto& hosted = state.sketches[index];
        if (!hosted.support.valid()) {
            return false;
        }

        if (const auto* surface =
                bodyPlanarSurfaceReference(
                    hosted.support);
            surface != nullptr &&
            !valid_surface_reference(*surface)) {
            return false;
        }

        if (const auto datum_id =
                datumPlaneIdForSketchSupport(
                    hosted.support)) {
            const auto datum =
                std::find_if(
                    state.datum_planes.begin(),
                    state.datum_planes.end(),
                    [datum_id](
                        const OffsetDatumPlane& item) {
                        return item.id == *datum_id;
                    });
            if (datum ==
                state.datum_planes.end()) {
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

    // PM-04A authored Axis identity is durable even when its source Sketch
    // or Line later disappears. Only structural identity/high-water validity
    // and duplicate AxisId are reconstruction invariants here; source
    // availability is derived repairable Axis evaluation state.
    for (std::size_t index = 0U;
         index < state.axes.size();
         ++index) {
        const auto& axis = state.axes[index];
        if (!partAxisStructurallyValid(axis) ||
            !state.next_axis_id
                 .containsAllocated(axis.id)) {
            return false;
        }

        for (std::size_t previous = 0U;
             previous < index;
             ++previous) {
            if (state.axes[previous].id ==
                axis.id) {
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
        if (source &&
            !state.next_profile_id
                 .containsAllocated(*source)) {
            return false;
        }

        if (const auto* material_edges =
                sourceMaterialEdges(feature)) {
            const auto stage_feature_id =
                material_edges->front().stage
                    .feature_id;
            if (!stage_feature_id ||
                !state.body.next_feature_id
                     .containsAllocated(
                         *stage_feature_id) ||
                !(*stage_feature_id <
                  feature.id)) {
                return false;
            }

            const auto present_before =
                [&state, index](FeatureId id) {
                    const auto found =
                        std::find_if(
                            state.body.features.begin(),
                            state.body.features.end(),
                            [id](
                                const PartFeature& item) {
                                return item.id == id;
                            });
                    if (found ==
                        state.body.features.end()) {
                        return true;
                    }
                    return static_cast<std::size_t>(
                               std::distance(
                                   state.body.features.begin(),
                                   found)) <
                           index;
                };

            // A deleted consumed stage remains durable repairable intent.
            // When the stage still exists, it must be the exact immediately
            // preceding Body stage. Availability itself is B2 resolver state.
            const auto current_stage =
                std::find_if(
                    state.body.features.begin(),
                    state.body.features.end(),
                    [stage_feature_id](
                        const PartFeature& item) {
                        return item.id ==
                               *stage_feature_id;
                    });
            if (current_stage !=
                    state.body.features.end() &&
                (index == 0U ||
                 static_cast<std::size_t>(
                     std::distance(
                         state.body.features.begin(),
                         current_stage)) +
                         1U !=
                     index)) {
                return false;
            }

            const auto provenance_is_historical =
                [&state,
                 &feature,
                 &stage_feature_id,
                 &present_before](
                    FeatureId producer) {
                    return state.body.next_feature_id
                               .containsAllocated(
                                   producer) &&
                           producer < feature.id &&
                           producer <=
                               *stage_feature_id &&
                           present_before(producer);
                };

            const auto surface_is_historical =
                [&provenance_is_historical](
                    const FeatureSurfaceAddress& surface) {
                    return surface.valid() &&
                           provenance_is_historical(
                               surface.producer_feature_id);
                };

            const auto point_is_historical =
                [&provenance_is_historical,
                 &surface_is_historical](
                    const FeaturePointAddress& point) {
                    if (!point.valid() ||
                        !provenance_is_historical(
                            point.producer_feature_id)) {
                        return false;
                    }
                    return std::all_of(
                        point.adjacent_surfaces.begin(),
                        point.adjacent_surfaces.end(),
                        surface_is_historical);
                };

            for (const auto& edge :
                 *material_edges) {
                if (edge.stage !=
                        material_edges->front().stage ||
                    !provenance_is_historical(
                        edge.curve
                            .producer_feature_id) ||
                    !std::all_of(
                        edge.curve
                            .adjacent_surfaces.begin(),
                        edge.curve
                            .adjacent_surfaces.end(),
                        surface_is_historical)) {
                    return false;
                }

                if (const auto* endpoints =
                        std::get_if<
                            BetweenSemanticPoints>(
                            &edge.branch);
                    endpoints != nullptr &&
                    (!point_is_historical(
                         endpoints->first) ||
                     !point_is_historical(
                         endpoints->second))) {
                    return false;
                }
            }
        }

        if (const auto* axis_reference =
                sourceAxisReference(feature)) {
            const auto axis_id =
                authoredAxisIdForAxisReference(
                    *axis_reference);
            if (axis_id &&
                !state.next_axis_id
                     .containsAllocated(*axis_id)) {
                return false;
            }
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

    // PM-02G / PM-03E bounded dependency invariant: a Body-Surface-
    // or Datum-backed Sketch may only feed Features strictly downstream of
    // the support's transitive Body-stage floor. This remains ordered
    // single-Body history validation, not a universal dependency graph.
    for (const auto& hosted : state.sketches) {
        const auto support_stage =
            required_body_stage_for_support(
                hosted.support);
        if (!support_stage) {
            continue;
        }

        const auto stage =
            std::find_if(
                state.body.features.begin(),
                state.body.features.end(),
                [support_stage](
                    const PartFeature& feature) {
                    return support_stage->feature_id &&
                           feature.id ==
                               *support_stage->feature_id;
                });
        if (stage == state.body.features.end()) {
            return false;
        }
        const auto stage_index =
            static_cast<std::size_t>(
                std::distance(
                    state.body.features.begin(),
                    stage));

        for (const auto& profile : state.profiles) {
            if (profile.source_sketch_id !=
                hosted.id) {
                continue;
            }
            for (std::size_t feature_index = 0U;
                 feature_index <
                     state.body.features.size();
                 ++feature_index) {
                const auto source =
                    sourceProfileId(
                        state.body.features[
                            feature_index]);
                if (source &&
                    *source == profile.id &&
                    stage_index >=
                        feature_index) {
                    return false;
                }
            }
        }

        // PM-04C bounded Axis dependency floor: an authored Axis inherits the
        // Body-stage floor of its source Sketch. A Revolve consuming that Axis
        // must be strictly downstream. Missing/deleted Axis remains repairable
        // consumer intent and therefore has no source floor to inspect here.
        for (const auto& axis : state.axes) {
            if (axis.source.sketch_id !=
                hosted.id) {
                continue;
            }
            for (std::size_t feature_index = 0U;
                 feature_index <
                     state.body.features.size();
                 ++feature_index) {
                const auto* axis_reference =
                    sourceAxisReference(
                        state.body.features[
                            feature_index]);
                if (axis_reference == nullptr) {
                    continue;
                }
                const auto consumed_axis =
                    authoredAxisIdForAxisReference(
                        *axis_reference);
                if (consumed_axis &&
                    *consumed_axis == axis.id &&
                    stage_index >=
                        feature_index) {
                    return false;
                }
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
