#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>

#include <algorithm>
#include <set>

namespace simplesolid2::part {

// The token is intentionally incomplete to consumers: each Part create or
// independent restore owns a new control block. Copies of PartEvaluation
// keep retired tokens live, preventing pointer-reuse false acceptance.
struct PartRuntimeIncarnation final {};

PartDocument::PartDocument(
    core::DocumentId id,
    PartAuthoredState state,
    core::DocumentRevision revision)
    : id_{std::move(id)},
      state_{std::move(state)},
      revision_{revision},
      incarnation_token_{std::make_shared<const PartRuntimeIncarnation>()} {}

namespace {

struct SemanticProvenanceBounds final {
    const PartAuthoredState* state;
    FeatureId cap;
    std::optional<FeatureId> strict_before;
    std::optional<std::size_t>
        present_before_index;

    [[nodiscard]] bool accepts(
        FeatureId id) const noexcept {
        if (state == nullptr ||
            !state->body.next_feature_id
                 .containsAllocated(id) ||
            id > cap ||
            (strict_before &&
             !(id < *strict_before))) {
            return false;
        }

        if (!present_before_index) {
            return true;
        }

        const auto found =
            std::find_if(
                state->body.features.begin(),
                state->body.features.end(),
                [id](const PartFeature& item) {
                    return item.id == id;
                });
        if (found ==
            state->body.features.end()) {
            // Deleted historical provenance remains repairable durable intent.
            return true;
        }

        return static_cast<std::size_t>(
                   std::distance(
                       state->body.features.begin(),
                       found)) <
               *present_before_index;
    }

    [[nodiscard]] SemanticProvenanceBounds
    narrowedTo(FeatureId new_cap) const noexcept {
        auto result = *this;
        if (new_cap < result.cap) {
            result.cap = new_cap;
        }
        return result;
    }
};

[[nodiscard]] bool semanticSurfaceHistoryValid(
    const FeatureSurfaceAddress& surface,
    const SemanticProvenanceBounds& bounds) noexcept;

[[nodiscard]] bool semanticPointHistoryValid(
    const FeaturePointAddress& point,
    const SemanticProvenanceBounds& bounds) noexcept;

[[nodiscard]] bool semanticEdgeHistoryValid(
    const MaterialEdgeReference& edge,
    const SemanticProvenanceBounds& bounds) noexcept {
    if (!edge.valid() ||
        !edge.stage.feature_id ||
        !bounds.accepts(
            *edge.stage.feature_id) ||
        !bounds.accepts(
            edge.curve.producer_feature_id) ||
        edge.curve.producer_feature_id >
            *edge.stage.feature_id) {
        return false;
    }

    const auto edge_bounds =
        bounds.narrowedTo(
            *edge.stage.feature_id);

    if (!std::all_of(
            edge.curve.adjacent_surfaces.begin(),
            edge.curve.adjacent_surfaces.end(),
            [&edge_bounds](
                const FeatureSurfaceAddress& surface) {
                return semanticSurfaceHistoryValid(
                    surface,
                    edge_bounds);
            })) {
        return false;
    }

    const auto* endpoints =
        std::get_if<BetweenSemanticPoints>(
            &edge.branch);
    if (endpoints != nullptr) {
        return semanticPointHistoryValid(
                   endpoints->first, edge_bounds) &&
               semanticPointHistoryValid(
                   endpoints->second, edge_bounds);
    }
    const auto* single =
        std::get_if<AtSingleSemanticPoint>(
            &edge.branch);
    return single == nullptr ||
           semanticPointHistoryValid(
               single->point, edge_bounds);
}

[[nodiscard]] bool semanticPointHistoryValid(
    const FeaturePointAddress& point,
    const SemanticProvenanceBounds& bounds) noexcept {
    if (!point.valid() ||
        !bounds.accepts(
            point.producer_feature_id)) {
        return false;
    }

    const auto point_bounds =
        bounds.narrowedTo(
            point.producer_feature_id);

    return std::all_of(
        point.adjacent_surfaces.begin(),
        point.adjacent_surfaces.end(),
        [&point_bounds](
            const FeatureSurfaceAddress& surface) {
            return semanticSurfaceHistoryValid(
                surface,
                point_bounds);
        });
}

[[nodiscard]] bool semanticSurfaceHistoryValid(
    const FeatureSurfaceAddress& surface,
    const SemanticProvenanceBounds& bounds) noexcept {
    if (!surface.valid() ||
        !bounds.accepts(
            surface.producer_feature_id)) {
        return false;
    }

    const auto surface_bounds =
        bounds.narrowedTo(
            surface.producer_feature_id);

    return std::all_of(
               surface.source_edges.begin(),
               surface.source_edges.end(),
               [&surface_bounds](
                   const MaterialEdgeReference& edge) {
                   return semanticEdgeHistoryValid(
                       edge,
                       surface_bounds);
               }) &&
           std::all_of(
               surface.source_points.begin(),
               surface.source_points.end(),
               [&surface_bounds](
                   const FeaturePointAddress& point) {
                   return semanticPointHistoryValid(
                       point,
                       surface_bounds);
               });
}

} // namespace

const ThreePointDatumPlane* PartDocument::findThreePointDatumPlane(
    DatumId id) const noexcept {
    const auto found = std::find_if(
        state_.three_point_datum_planes.begin(),
        state_.three_point_datum_planes.end(),
        [id](const ThreePointDatumPlane& item) {
            return item.id == id;
        });
    return found == state_.three_point_datum_planes.end()
        ? nullptr : &*found;
}

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

const EdgePointDatumPlane* PartDocument::findEdgePointDatumPlane(
    DatumId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found = std::find_if(
        state_.edge_point_datum_planes.begin(),
        state_.edge_point_datum_planes.end(),
        [id](const EdgePointDatumPlane& item) {
            return item.id == id;
        });
    return found == state_.edge_point_datum_planes.end()
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
    // R13-A1 A1.1: authored relations are not evaluated geometry.
    // A Profile cannot consume the persisted seed as a solved snapshot.
    // The solver adapter must replace this fence with a validated,
    // same-stage evaluated Sketch before constraints become consumable.
    if (source->model.constraintCount() != 0U) {
        return std::nullopt;
    }
    // PG-01B/B1 safety fence: never use stale authored seed as current
    // geometric truth for a Profile that consumes linked source geometry.
    // B2/B3 will replace this with pure effective Sketch evaluation.
    const auto depends_on_bound_entity =
        [&source](const ProfileLoopIntent& loop) {
            for (const auto& use : loop.boundary) {
                for (const auto& binding :
                     source->projection_bindings) {
                    if (use.source_entity ==
                            binding.target_entity ||
                        (use.start_anchor &&
                         use.start_anchor->kind ==
                             ProfileBoundaryAnchorKind::intersection &&
                         use.start_anchor->other_entity ==
                             binding.target_entity) ||
                        (use.end_anchor &&
                         use.end_anchor->kind ==
                             ProfileBoundaryAnchorKind::intersection &&
                         use.end_anchor->other_entity ==
                             binding.target_entity)) {
                        return true;
                    }
                }
            }
            return false;
        };
    if (depends_on_bound_entity(profile->region_intent.outer) ||
        std::any_of(
            profile->region_intent.holes.begin(),
            profile->region_intent.holes.end(),
            depends_on_bound_entity)) {
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
        [&state](
            const SurfaceReference& surface) {
            if (!surface.valid() ||
                !surface.stage.feature_id) {
                return false;
            }

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
                        return feature.id ==
                               *surface.stage.feature_id;
                    });
            if (producer ==
                    state.body.features.end() ||
                stage ==
                    state.body.features.end() ||
                producer > stage) {
                return false;
            }

            const SemanticProvenanceBounds
                bounds{
                    &state,
                    *surface.stage.feature_id,
                    std::nullopt,
                    std::nullopt};
            return semanticSurfaceHistoryValid(
                surface.surface,
                bounds);
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

    // Versioned Edge-local Datum shares the existing global DatumId cursor.
    // Preserve an authored source even if its upstream feature was deleted:
    // missing geometry is a derived, repairable evaluation outcome.
    for (std::size_t index = 0U;
         index < state.edge_point_datum_planes.size();
         ++index) {
        const auto& datum = state.edge_point_datum_planes[index];
        if (!edgePointDatumPlaneStructurallyValid(datum) ||
            !state.next_datum_id.containsAllocated(datum.id)) {
            return false;
        }
        if (std::any_of(
                state.datum_planes.begin(),
                state.datum_planes.end(),
                [id = datum.id](const OffsetDatumPlane& offset) {
                    return offset.id == id;
                })) {
            return false;
        }
        for (std::size_t previous = 0U;
             previous < index;
             ++previous) {
            if (state.edge_point_datum_planes[previous].id ==
                datum.id) {
                return false;
            }
        }
    }

    // S4A: one global DatumId namespace and strict stage-owned semantic
    // Point provenance, including repairable deleted-source intent.
    for (std::size_t i = 0; i < state.three_point_datum_planes.size(); ++i) {
        const auto& datum = state.three_point_datum_planes[i];
        if (!threePointDatumPlaneStructurallyValid(datum) ||
            !state.next_datum_id.containsAllocated(datum.id)) return false;
        for (const auto& point : datum.points) {
            if (!point.stage.feature_id ||
                !semanticPointHistoryValid(
                    point.point,
                    SemanticProvenanceBounds{
                        &state, *point.stage.feature_id,
                        std::nullopt, std::nullopt})) return false;
        }
        if (std::any_of(state.datum_planes.begin(), state.datum_planes.end(),
                        [id = datum.id](const auto& other) {
                            return other.id == id;
                        }) ||
            std::any_of(state.edge_point_datum_planes.begin(),
                        state.edge_point_datum_planes.end(),
                        [id = datum.id](const auto& other) {
                            return other.id == id;
                        })) return false;
        for (std::size_t j = 0; j < i; ++j)
            if (state.three_point_datum_planes[j].id == datum.id)
                return false;
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
                const auto edge = std::find_if(
                    state.edge_point_datum_planes.begin(),
                    state.edge_point_datum_planes.end(),
                    [datum_id](const EdgePointDatumPlane& item) {
                        return item.id == *datum_id;
                    });
                if (edge == state.edge_point_datum_planes.end()) {
                    const auto point = std::find_if(
                        state.three_point_datum_planes.begin(),
                        state.three_point_datum_planes.end(),
                        [datum_id](const ThreePointDatumPlane& item) {
                            return item.id == *datum_id;
                        });
                    if (point == state.three_point_datum_planes.end())
                        return std::nullopt;
                    return point->points[0].stage;
                }
                return std::visit(
                    [](const auto& source)
                        -> std::optional<BodyStageRef> {
                        return source.source_edge.stage;
                    }, edge->source);
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

        // Strict Part-only binding metadata, never derived OCCT identity.
        // Missing historical Features remain valid repairable intent; the
        // source stage must still be an allocated, earlier semantic stage.
        std::optional<sketch::EntityId> last_bound;
        for (const auto& binding :
             hosted.projection_bindings) {
            if (!binding.valid() ||
                !hosted.model.contains(binding.target_entity) ||
                (last_bound &&
                 !(*last_bound < binding.target_entity)) ||
                !binding.source.stage.feature_id ||
                !semanticEdgeHistoryValid(
                    binding.source,
                    SemanticProvenanceBounds{
                        &state,
                        *binding.source.stage.feature_id,
                        std::nullopt,
                        std::nullopt})) {
                return false;
            }
            last_bound = binding.target_entity;
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
            if (datum == state.datum_planes.end() &&
                std::none_of(
                    state.edge_point_datum_planes.begin(),
                    state.edge_point_datum_planes.end(),
                    [datum_id](const EdgePointDatumPlane& item) {
                        return item.id == *datum_id;
                    }) &&
                std::none_of(
                    state.three_point_datum_planes.begin(),
                    state.three_point_datum_planes.end(),
                    [datum_id](const ThreePointDatumPlane& item) {
                        return item.id == *datum_id;
                    })) {
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
    // Feature dependencies must point to strictly earlier evaluated Body
    // stages, even when a projected target is an otherwise valid EntityId.
    // No self / forward / cyclic linked Profile may be restored.
    for (std::size_t feature_index = 0U;
         feature_index < state.body.features.size();
         ++feature_index) {
        const auto& feature =
            state.body.features[feature_index];
        const auto* extrude =
            std::get_if<ExtrudeFeature>(&feature.definition);
        const auto* revolve =
            std::get_if<RevolveFeature>(&feature.definition);
        if (!extrude && !revolve) {
            continue;
        }
        const auto profile_id = extrude
            ? extrude->profile_id
            : revolve->profile_id;
        const auto profile_it =
            std::find_if(
                state.profiles.begin(),
                state.profiles.end(),
                [profile_id](const PartProfile& item) {
                    return item.id == profile_id;
                });
        if (profile_it == state.profiles.end()) {
            continue;
        }
        const auto sketch_it =
            std::find_if(
                state.sketches.begin(),
                state.sketches.end(),
                [&profile_it](const PartSketch& item) {
                    return item.id ==
                           profile_it->source_sketch_id;
                });
        if (sketch_it == state.sketches.end()) {
            continue;
        }
        for (const auto& binding :
             sketch_it->projection_bindings) {
            const auto referenced =
                [&binding](const ProfileLoopIntent& loop) {
                    return std::any_of(
                        loop.boundary.begin(),
                        loop.boundary.end(),
                        [&binding](
                            const ProfileBoundaryUseIntent& use) {
                            return use.source_entity ==
                                       binding.target_entity ||
                                (use.start_anchor &&
                                 use.start_anchor->kind ==
                                     ProfileBoundaryAnchorKind::intersection &&
                                 use.start_anchor->other_entity ==
                                     binding.target_entity) ||
                                (use.end_anchor &&
                                 use.end_anchor->kind ==
                                     ProfileBoundaryAnchorKind::intersection &&
                                 use.end_anchor->other_entity ==
                                     binding.target_entity);
                        });
                };
            if (!referenced(profile_it->region_intent.outer) &&
                std::none_of(
                    profile_it->region_intent.holes.begin(),
                    profile_it->region_intent.holes.end(),
                    referenced)) {
                continue;
            }
            const auto& source_id =
                binding.source.stage.feature_id;
            if (!source_id) {
                return false;
            }
            const auto found =
                std::find_if(
                    state.body.features.begin(),
                    state.body.features.end(),
                    [source_id](const PartFeature& candidate) {
                        return candidate.id == *source_id;
                    });
            if (found != state.body.features.end() &&
                static_cast<std::size_t>(
                    std::distance(
                        state.body.features.begin(), found)) >=
                    feature_index) {
                return false;
            }
        }
    }

    for (std::size_t index = 0U;
         index < state.axes.size();
         ++index) {
        const auto& axis = state.axes[index];
        if (!partAxisStructurallyValid(axis) ||
            !state.next_axis_id
                 .containsAllocated(axis.id)) {
            return false;
        }

        if (const auto* face =
                std::get_if<StrictCylindricalFaceAxisSource>(
                    &axis.source)) {
            const auto& ref = face->surface;
            // A missing or deleted stage is authored but repairable only
            // when its IDs were already allocated in THIS Part. Never
            // accept a forged/cross-Part producer or stage FeatureId.
            if (!ref.stage.feature_id ||
                !state.body.next_feature_id.containsAllocated(
                    *ref.stage.feature_id) ||
                !state.body.next_feature_id.containsAllocated(
                    ref.surface.producer_feature_id)) {
                return false;
            }
            const auto producer = std::find_if(
                state.body.features.begin(),
                state.body.features.end(),
                [&ref](const PartFeature& feature) {
                    return feature.id ==
                        ref.surface.producer_feature_id;
                });
            const auto stage = std::find_if(
                state.body.features.begin(),
                state.body.features.end(),
                [&ref](const PartFeature& feature) {
                    return feature.id == *ref.stage.feature_id;
                });
            // If both survive, enforce the SAME semantic Surface
            // provenance and causal history rules already used for Body
            // Sketch/Datum support. No fallback by current Face geometry.
            if (producer != state.body.features.end() &&
                stage != state.body.features.end() &&
                !valid_surface_reference(ref)) {
                return false;
            }
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

            const SemanticProvenanceBounds
                provenance_bounds{
                    &state,
                    *stage_feature_id,
                    feature.id,
                    index};

            for (const auto& edge :
                 *material_edges) {
                // PG-01D v16 authoring is projection-only. Do not widen
                // existing Fillet/Chamfer direct input semantics.
                if (std::holds_alternative<AtSingleSemanticPoint>(
                        edge.branch) ||
                    edge.stage !=
                        material_edges->front().stage ||
                    !semanticEdgeHistoryValid(
                        edge,
                        provenance_bounds)) {
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
            const auto* sketch_axis =
                std::get_if<SketchLineAxisSource>(&axis.source);
            if (sketch_axis == nullptr ||
                sketch_axis->sketch_id != hosted.id) {
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

    // RG-02C/B1: the strict Body Edge producer stage must be strictly
    // upstream of any Revolve consuming its authored AxisId. Retain
    // deleted/missing producer intent as repairable, not a guessed Edge.
    for (const auto& axis : state.axes) {
        const auto* edge_axis =
            std::get_if<StraightMaterialEdgeAxisSource>(
                &axis.source);
        if (edge_axis == nullptr ||
            !edge_axis->edge.stage.feature_id) {
            continue;
        }
        const auto producer = std::find_if(
            state.body.features.begin(),
            state.body.features.end(),
            [edge_axis](const PartFeature& feature) {
                return feature.id ==
                    *edge_axis->edge.stage.feature_id;
            });
        if (producer == state.body.features.end()) {
            continue; // durable repairable source, no implicit fallback
        }
        const auto producer_index =
            static_cast<std::size_t>(
                std::distance(
                    state.body.features.begin(), producer));
        for (std::size_t feature_index = 0U;
             feature_index < state.body.features.size();
             ++feature_index) {
            const auto* required_axis =
                sourceAxisReference(
                    state.body.features[feature_index]);
            if (required_axis == nullptr) {
                continue;
            }
            const auto consumed_axis =
                authoredAxisIdForAxisReference(*required_axis);
            if (consumed_axis &&
                *consumed_axis == axis.id &&
                producer_index >= feature_index) {
                return false;
            }
        }
    }

    // H1: same existing authored AxisId dependency floor for strict
    // cylindrical Face sources. An unavailable/deleted source stage remains
    // authored and repairable, but an existing downstream Revolve may not
    // consume an Axis created from its own or a future Body result.
    for (const auto& axis : state.axes) {
        const auto* face_axis =
            std::get_if<StrictCylindricalFaceAxisSource>(
                &axis.source);
        if (face_axis == nullptr ||
            !face_axis->surface.stage.feature_id) {
            continue;
        }
        const auto source_stage = std::find_if(
            state.body.features.begin(),
            state.body.features.end(),
            [face_axis](const PartFeature& feature) {
                return feature.id ==
                    *face_axis->surface.stage.feature_id;
            });
        if (source_stage == state.body.features.end()) {
            continue; // explicit repair required, never rebind by shape
        }
        const auto source_index =
            static_cast<std::size_t>(std::distance(
                state.body.features.begin(), source_stage));
        for (std::size_t feature_index = 0U;
             feature_index < state.body.features.size();
             ++feature_index) {
            const auto* reference = sourceAxisReference(
                state.body.features[feature_index]);
            if (reference == nullptr) {
                continue;
            }
            const auto consumed =
                authoredAxisIdForAxisReference(*reference);
            if (consumed && *consumed == axis.id &&
                source_index >= feature_index) {
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

std::optional<sketch::LocalSolveProblem>
prepareStageLocalSolveProblem(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    std::optional<FeatureId> consuming_feature) {
    // Do not accept a timestamp or a plausible prefix from another document.
    // Part owns both revision and source-identity evidence.
    if (!ownsCurrentPartEvaluation(document, evaluated_prefix)) {
        return std::nullopt;
    }
    if (document.findSketch(sketch_id) == nullptr) return std::nullopt;

    // Existing Part-owned PG-01 resolver rejects missing/ambiguous stages,
    // unsupported geometry and stale sources, without mutating Sketch seeds.
    const auto evaluated = evaluateEffectiveSketchProjection(
        document, sketch_id, evaluated_prefix, query, consuming_feature);
    // A provider may re-enter DocumentSession, replacing PartAuthoredState
    // entirely. Check owner/revision BEFORE dereferencing any authored
    // reference after projection; no cross-revision mixture may escape.
    if (!ownsCurrentPartEvaluation(document, evaluated_prefix)) {
        return std::nullopt;
    }
    const auto* authored = document.findSketch(sketch_id);
    if (!authored || !evaluated || !evaluated->allResolved() ||
        evaluated->outcomes.size() != authored->projection_bindings.size()) {
        return std::nullopt;
    }

    std::vector<sketch::EntityId> fixed_lines;
    fixed_lines.reserve(authored->projection_bindings.size());
    for (std::size_t i = 0; i < authored->projection_bindings.size(); ++i) {
        const auto id = authored->projection_bindings[i].target_entity;
        const auto& resolved = evaluated->outcomes[i];
        if (!resolved.resolved() ||
            resolved.target_entity != id ||
            authored->model.findLine(id) == nullptr ||
            evaluated->model.findLine(id) == nullptr) {
            // This A1 solver accepts Line-only fixed sources. A fixed Circle
            // or Arc needs a separate explicitly authorized solver mapping.
            return std::nullopt;
        }
        fixed_lines.push_back(id);
    }
    auto problem = sketch::prepareLocalSolveProblem(
        evaluated->model.state(), fixed_lines);
    if (!problem ||
        !ownsCurrentPartEvaluation(document, evaluated_prefix)) {
        return std::nullopt;
    }
    return problem;
}


std::optional<StageLocalSolveEvaluation>
evaluateStageLocalSolve(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    const sketch::LocalSolverAcceptancePolicy& policy,
    sketch::ILocalSketchSolver& backend,
    std::optional<FeatureId> consuming_feature) {
    // Fail before any provider/backend work for invalid acceptance policy.
    if (!policy.valid()) return std::nullopt;

    // This is the ONLY Part-owned source of projected fixed reference
    // geometry; a failed current stage must not fall back to Sketch seeds.
    const auto problem = prepareStageLocalSolveProblem(
        document, sketch_id, evaluated_prefix, query, consuming_feature);
    if (!problem ||
        !ownsCurrentPartEvaluation(document, evaluated_prefix)) {
        return std::nullopt;
    }

    auto solution = sketch::evaluateLocalSolve(*problem, policy, backend);

    // P0-D current-stage publication guard: PlaneGCS evaluates an entire
    // Sketch, including disconnected underconstrained components. A valid
    // residual alone cannot authorize moving an unrelated free Line or an
    // independently already-satisfied component during a read/refresh.
    // The baseline residual report uses the EXISTING caller-owned policy;
    // there is no new geometric tolerance, nearest rebind or writeback.
    if (solution.status ==
            sketch::LocalSolveGatewayStatus::candidate_accepted &&
        solution.candidate) {
        const auto baseline = sketch::reviewLocalSolveCandidate(
            *problem, problem->authored, policy);
        bool stable =
            baseline.status == sketch::LocalResidualStatus::residuals_satisfied ||
            baseline.status == sketch::LocalResidualStatus::residual_violation;
        std::set<sketch::EntityId> movable;

        // Only a connected component that contains an equation UNSATISFIED
        // by the current accepted seeds can acquire different coordinates.
        // Binary relations propagate that permission transitively; neither
        // mere proximity nor the backend's global parameter graph does.
        if (stable) {
            for (const auto& residual : baseline.constraints) {
                if (residual.accepted) continue;
                const auto relation = std::find_if(
                    problem->equations.begin(), problem->equations.end(),
                    [&residual](const sketch::SketchConstraintState& item) {
                        return item.id == residual.constraint_id;
                    });
                if (relation == problem->equations.end() ||
                    !relation->first.entity.valid()) {
                    stable = false;
                    break;
                }
                movable.insert(relation->first.entity);
                if (relation->second.entity.valid()) {
                    movable.insert(relation->second.entity);
                }
            }
        }
        if (stable) {
            bool expanded = false;
            do {
                expanded = false;
                for (const auto& relation : problem->equations) {
                    if (!movable.contains(relation.first.entity) &&
                        !movable.contains(relation.second.entity)) {
                        continue;
                    }
                    if (relation.first.entity.valid()) {
                        expanded =
                            movable.insert(relation.first.entity).second ||
                            expanded;
                    }
                    if (relation.second.entity.valid()) {
                        expanded =
                            movable.insert(relation.second.entity).second ||
                            expanded;
                    }
                }
            } while (expanded);

            const auto& accepted = solution.candidate->lines;
            const auto& original = problem->authored.lines;
            stable = accepted.size() == original.size();
            for (std::size_t i = 0; stable && i < original.size(); ++i) {
                if (accepted[i].id != original[i].id ||
                    (!movable.contains(original[i].id) &&
                     accepted[i] != original[i])) {
                    stable = false;
                }
            }
        }
        if (!stable) {
            // Keep the canonical Part stage/revision as the only authority.
            // Consumers see a structured failure, never a translated
            // SketchScene, a stale Profile or a partial numerical result.
            solution.status = sketch::LocalSolveGatewayStatus::invalid_candidate;
            solution.candidate.reset();
            solution.degrees_of_freedom.reset();
        }
    }

    // The adapter may invoke code that edits DocumentSession. Never return
    // its otherwise converged, but now stale, derived snapshot to Part.
    if (!ownsCurrentPartEvaluation(document, evaluated_prefix)) {
        return std::nullopt;
    }
    return StageLocalSolveEvaluation{
        document.documentId(),
        document.revision(),
        sketch_id,
        consuming_feature,
        std::move(solution)};
}

} // namespace simplesolid2::part
