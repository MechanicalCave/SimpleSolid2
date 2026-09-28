#include <simplesolid2/part/profile.hpp>

#include <algorithm>
#include <cstddef>
#include <set>
#include <utility>

namespace simplesolid2::part {
namespace {

[[nodiscard]] std::optional<ProfileBoundaryAnchor>
convertAnchor(
    const sketch::RegionBoundaryAnchor2D& anchor) {
    ProfileBoundaryAnchor result;
    switch (anchor.kind) {
    case sketch::RegionBoundaryAnchorKind::
        endpoint_start:
        result.kind =
            ProfileBoundaryAnchorKind::endpoint_start;
        break;
    case sketch::RegionBoundaryAnchorKind::
        endpoint_end:
        result.kind =
            ProfileBoundaryAnchorKind::endpoint_end;
        break;
    case sketch::RegionBoundaryAnchorKind::
        intersection:
        if (!anchor.other_entity.valid()) {
            return std::nullopt;
        }
        result.kind =
            ProfileBoundaryAnchorKind::intersection;
        result.other_entity =
            anchor.other_entity;
        result.canonical_branch =
            anchor.canonical_branch;
        break;
    }
    return result;
}

[[nodiscard]] bool anchorLess(
    const ProfileBoundaryAnchor& first,
    const ProfileBoundaryAnchor& second) noexcept {
    if (first.kind != second.kind) {
        return first.kind < second.kind;
    }
    if (first.other_entity != second.other_entity) {
        return first.other_entity <
               second.other_entity;
    }
    return first.canonical_branch <
           second.canonical_branch;
}

[[nodiscard]] bool optionalAnchorLess(
    const std::optional<ProfileBoundaryAnchor>& first,
    const std::optional<ProfileBoundaryAnchor>& second)
    noexcept {
    if (first.has_value() != second.has_value()) {
        return !first.has_value();
    }
    if (!first) {
        return false;
    }
    if (*first == *second) {
        return false;
    }
    return anchorLess(*first, *second);
}

[[nodiscard]] bool useLess(
    const ProfileBoundaryUseIntent& first,
    const ProfileBoundaryUseIntent& second) noexcept {
    if (first.source_entity != second.source_entity) {
        return first.source_entity <
               second.source_entity;
    }
    if (first.whole_closed_curve !=
        second.whole_closed_curve) {
        return first.whole_closed_curve <
               second.whole_closed_curve;
    }
    if (first.start_anchor != second.start_anchor) {
        return optionalAnchorLess(
            first.start_anchor,
            second.start_anchor);
    }
    if (first.end_anchor != second.end_anchor) {
        return optionalAnchorLess(
            first.end_anchor,
            second.end_anchor);
    }
    return first.follows_source_direction <
           second.follows_source_direction;
}

[[nodiscard]] bool rotationLess(
    const std::vector<ProfileBoundaryUseIntent>& values,
    std::size_t first,
    std::size_t second) noexcept {
    for (std::size_t offset = 0U;
         offset < values.size();
         ++offset) {
        const auto& lhs =
            values[(first + offset) %
                   values.size()];
        const auto& rhs =
            values[(second + offset) %
                   values.size()];
        if (lhs == rhs) {
            continue;
        }
        return useLess(lhs, rhs);
    }
    return false;
}

void canonicalizeLoop(
    ProfileLoopIntent& loop) {
    if (loop.boundary.size() < 2U) {
        return;
    }

    std::size_t best = 0U;
    for (std::size_t candidate = 1U;
         candidate < loop.boundary.size();
         ++candidate) {
        if (rotationLess(
                loop.boundary,
                candidate,
                best)) {
            best = candidate;
        }
    }

    std::rotate(
        loop.boundary.begin(),
        loop.boundary.begin() +
            static_cast<std::ptrdiff_t>(best),
        loop.boundary.end());
}

[[nodiscard]] bool loopLess(
    const ProfileLoopIntent& first,
    const ProfileLoopIntent& second) noexcept {
    return std::lexicographical_compare(
        first.boundary.begin(),
        first.boundary.end(),
        second.boundary.begin(),
        second.boundary.end(),
        useLess);
}

[[nodiscard]] bool validAnchor(
    const ProfileBoundaryAnchor& anchor,
    sketch::EntityId source) noexcept {
    switch (anchor.kind) {
    case ProfileBoundaryAnchorKind::endpoint_start:
    case ProfileBoundaryAnchorKind::endpoint_end:
        return !anchor.other_entity.valid() &&
               anchor.canonical_branch == 0U;
    case ProfileBoundaryAnchorKind::intersection:
        return anchor.other_entity.valid() &&
               anchor.other_entity != source;
    }
    return false;
}

[[nodiscard]] bool validUse(
    const ProfileBoundaryUseIntent& use) noexcept {
    if (!use.source_entity.valid()) {
        return false;
    }

    if (use.whole_closed_curve) {
        return !use.start_anchor &&
               !use.end_anchor;
    }

    return use.start_anchor &&
           use.end_anchor &&
           validAnchor(
               *use.start_anchor,
               use.source_entity) &&
           validAnchor(
               *use.end_anchor,
               use.source_entity);
}

[[nodiscard]] bool validLoop(
    const ProfileLoopIntent& loop) noexcept {
    if (loop.boundary.empty()) {
        return false;
    }
    if (loop.boundary.size() > 1U &&
        std::any_of(
            loop.boundary.begin(),
            loop.boundary.end(),
            [](const ProfileBoundaryUseIntent& use) {
                return use.whole_closed_curve;
            })) {
        return false;
    }
    return std::all_of(
        loop.boundary.begin(),
        loop.boundary.end(),
        validUse);
}

[[nodiscard]] bool validIntent(
    const ProfileRegionIntent& intent) noexcept {
    if (!validLoop(intent.outer)) {
        return false;
    }
    return std::all_of(
        intent.holes.begin(),
        intent.holes.end(),
        validLoop);
}

[[nodiscard]] bool mergeableUses(
    const sketch::RegionBoundaryUse2D& first,
    const sketch::RegionBoundaryUse2D& second) noexcept {
    return !first.whole_closed_curve &&
           !second.whole_closed_curve &&
           first.source_entity ==
               second.source_entity &&
           first.follows_source_direction ==
               second.follows_source_direction &&
           first.end_parameter ==
               second.start_parameter &&
           first.end_anchor &&
           second.start_anchor &&
           *first.end_anchor ==
               *second.start_anchor;
}

[[nodiscard]] std::vector<sketch::RegionBoundaryUse2D>
coalescedUses(
    const sketch::RegionLoop2D& source) {
    auto uses = source.boundary;
    if (uses.size() < 2U) {
        return uses;
    }

    bool closed_single_source = true;
    for (std::size_t index = 0U;
         index < uses.size();
         ++index) {
        if (!mergeableUses(
                uses[index],
                uses[(index + 1U) %
                     uses.size()])) {
            closed_single_source = false;
            break;
        }
    }
    if (closed_single_source) {
        return {
            sketch::RegionBoundaryUse2D{
                uses.front().source_entity,
                0.0,
                0.0,
                std::nullopt,
                std::nullopt,
                uses.front()
                    .follows_source_direction,
                true,
                true}};
    }

    std::size_t first_after_break = 0U;
    for (std::size_t index = 0U;
         index < uses.size();
         ++index) {
        const auto previous =
            (index + uses.size() - 1U) %
            uses.size();
        if (!mergeableUses(
                uses[previous],
                uses[index])) {
            first_after_break = index;
            break;
        }
    }
    std::rotate(
        uses.begin(),
        uses.begin() +
            static_cast<std::ptrdiff_t>(
                first_after_break),
        uses.end());

    std::vector<sketch::RegionBoundaryUse2D>
        result;
    result.reserve(uses.size());
    for (const auto& use : uses) {
        if (result.empty() ||
            !mergeableUses(
                result.back(),
                use)) {
            result.push_back(use);
            continue;
        }

        auto& previous = result.back();
        previous.end_parameter =
            use.end_parameter;
        previous.end_anchor =
            use.end_anchor;
        previous.crosses_closed_seam =
            previous.crosses_closed_seam ||
            use.crosses_closed_seam;
    }
    return result;
}

[[nodiscard]] std::optional<ProfileLoopIntent>
convertLoop(
    const sketch::RegionLoop2D& source) {
    ProfileLoopIntent result;
    const auto uses =
        coalescedUses(source);
    result.boundary.reserve(
        uses.size());

    for (const auto& use : uses) {
        if (!use.source_entity.valid()) {
            return std::nullopt;
        }

        ProfileBoundaryUseIntent converted;
        converted.source_entity =
            use.source_entity;
        converted.follows_source_direction =
            use.follows_source_direction;
        converted.whole_closed_curve =
            use.whole_closed_curve;

        if (use.whole_closed_curve) {
            if (use.start_anchor ||
                use.end_anchor) {
                return std::nullopt;
            }
        } else {
            if (!use.start_anchor ||
                !use.end_anchor) {
                return std::nullopt;
            }
            converted.start_anchor =
                convertAnchor(*use.start_anchor);
            converted.end_anchor =
                convertAnchor(*use.end_anchor);
            if (!converted.start_anchor ||
                !converted.end_anchor) {
                return std::nullopt;
            }
        }

        if (!validUse(converted)) {
            return std::nullopt;
        }
        result.boundary.push_back(
            std::move(converted));
    }

    canonicalizeLoop(result);
    return result;
}

void collectAnchorReference(
    const std::optional<ProfileBoundaryAnchor>& anchor,
    std::set<sketch::EntityId>& ids) {
    if (anchor &&
        anchor->kind ==
            ProfileBoundaryAnchorKind::intersection) {
        ids.insert(anchor->other_entity);
    }
}

[[nodiscard]] std::set<sketch::EntityId>
referencedEntities(
    const ProfileRegionIntent& intent) {
    std::set<sketch::EntityId> ids;
    const auto collect_loop =
        [&ids](const ProfileLoopIntent& loop) {
            for (const auto& use :
                 loop.boundary) {
                ids.insert(use.source_entity);
                collectAnchorReference(
                    use.start_anchor,
                    ids);
                collectAnchorReference(
                    use.end_anchor,
                    ids);
            }
        };

    collect_loop(intent.outer);
    for (const auto& hole : intent.holes) {
        collect_loop(hole);
    }
    return ids;
}

[[nodiscard]] bool diagnosticTouches(
    const sketch::RegionAnalysisDiagnostic2D& diagnostic,
    const std::set<sketch::EntityId>& ids) {
    if (diagnostic.entities.empty()) {
        return true;
    }
    return std::any_of(
        diagnostic.entities.begin(),
        diagnostic.entities.end(),
        [&ids](sketch::EntityId id) {
            return ids.contains(id);
        });
}

} // namespace

bool profileRegionIntentStructurallyValid(
    const ProfileRegionIntent& intent) noexcept {
    return validIntent(intent);
}

std::optional<ProfileRegionIntent>
makeProfileRegionIntent(
    const sketch::RegionCandidate2D& region) {
    auto outer =
        convertLoop(region.outer);
    if (!outer) {
        return std::nullopt;
    }

    ProfileRegionIntent result;
    result.outer = std::move(*outer);
    result.holes.reserve(
        region.holes.size());

    for (const auto& hole :
         region.holes) {
        auto converted =
            convertLoop(hole);
        if (!converted) {
            return std::nullopt;
        }
        result.holes.push_back(
            std::move(*converted));
    }

    std::sort(
        result.holes.begin(),
        result.holes.end(),
        loopLess);

    return validIntent(result)
        ? std::optional<ProfileRegionIntent>{
              std::move(result)}
        : std::nullopt;
}

ResolvedProfileRegion resolveProfileRegionIntent(
    const sketch::SketchModel& model,
    const ProfileRegionIntent& intent) {
    if (!validIntent(intent)) {
        return {
            ProfileIntentResolutionStatus::
                invalid_intent,
            std::nullopt};
    }

    const auto references =
        referencedEntities(intent);
    for (const auto id : references) {
        if (!model.contains(id)) {
            return {
                ProfileIntentResolutionStatus::
                    missing_source_entity,
                std::nullopt};
        }
    }

    const auto regular_source =
        [&model](sketch::EntityId id) {
            if (const auto* line =
                    model.findLine(id)) {
                return line->role() ==
                       sketch::EntityRole::regular;
            }
            if (const auto* circle =
                    model.findCircle(id)) {
                return circle->role() ==
                       sketch::EntityRole::regular;
            }
            if (const auto* arc =
                    model.findArc(id)) {
                return arc->role() ==
                       sketch::EntityRole::regular;
            }
            return false;
        };

    const auto runtime_anchor =
        [](const ProfileBoundaryAnchor& anchor)
            -> sketch::RegionBoundaryAnchor2D {
            switch (anchor.kind) {
            case ProfileBoundaryAnchorKind::
                endpoint_start:
                return {
                    sketch::RegionBoundaryAnchorKind::
                        endpoint_start,
                    {},
                    0U};
            case ProfileBoundaryAnchorKind::
                endpoint_end:
                return {
                    sketch::RegionBoundaryAnchorKind::
                        endpoint_end,
                    {},
                    0U};
            case ProfileBoundaryAnchorKind::
                intersection:
                return {
                    sketch::RegionBoundaryAnchorKind::
                        intersection,
                    anchor.other_entity,
                    anchor.canonical_branch};
            }
            return {};
        };

    const auto resolve_anchor =
        [&model, &regular_source](
            sketch::EntityId source,
            const ProfileBoundaryAnchor& anchor)
            -> std::optional<double> {
            if (anchor.kind ==
                ProfileBoundaryAnchorKind::
                    endpoint_start) {
                if (model.findLine(source) ||
                    model.findArc(source)) {
                    return 0.0;
                }
                return std::nullopt;
            }
            if (anchor.kind ==
                ProfileBoundaryAnchorKind::
                    endpoint_end) {
                if (model.findLine(source) ||
                    model.findArc(source)) {
                    return 1.0;
                }
                return std::nullopt;
            }

            if (!regular_source(
                    anchor.other_entity)) {
                return std::nullopt;
            }

            const auto relation =
                sketch::analyzeCurveRelation(
                    model,
                    source,
                    anchor.other_entity);
            if (relation.status !=
                sketch::CurveRelationStatus::
                    discrete) {
                return std::nullopt;
            }

            const auto found =
                std::find_if(
                    relation.intersections.begin(),
                    relation.intersections.end(),
                    [&anchor](
                        const sketch::
                            CurveIntersection2D& item) {
                        return item.canonical_branch ==
                               anchor.canonical_branch;
                    });
            if (found ==
                relation.intersections.end()) {
                return std::nullopt;
            }

            if (found->contact ==
                    sketch::CurveContactKind::tangent &&
                !found->first_endpoint &&
                !found->second_endpoint) {
                return std::nullopt;
            }

            return relation.first_entity == source
                ? std::optional<double>{
                      found->first_parameter}
                : std::optional<double>{
                      found->second_parameter};
        };

    const auto resolve_loop =
        [&model,
         &regular_source,
         &runtime_anchor,
         &resolve_anchor](
            const ProfileLoopIntent& loop)
            -> std::optional<
                sketch::RegionLoop2D> {
            sketch::RegionLoop2D result;
            result.boundary.reserve(
                loop.boundary.size());

            for (const auto& use :
                 loop.boundary) {
                if (!regular_source(
                        use.source_entity)) {
                    return std::nullopt;
                }

                if (use.whole_closed_curve) {
                    if (!model.findCircle(
                            use.source_entity)) {
                        return std::nullopt;
                    }
                    result.boundary.push_back(
                        sketch::RegionBoundaryUse2D{
                            use.source_entity,
                            0.0,
                            0.0,
                            std::nullopt,
                            std::nullopt,
                            use.follows_source_direction,
                            true,
                            true});
                    continue;
                }

                const auto start =
                    resolve_anchor(
                        use.source_entity,
                        *use.start_anchor);
                const auto end =
                    resolve_anchor(
                        use.source_entity,
                        *use.end_anchor);
                if (!start || !end) {
                    return std::nullopt;
                }

                const bool circle =
                    model.findCircle(
                        use.source_entity) !=
                    nullptr;
                const bool crosses_seam =
                    circle &&
                    (use.follows_source_direction
                         ? *start > *end
                         : *start < *end);

                result.boundary.push_back(
                    sketch::RegionBoundaryUse2D{
                        use.source_entity,
                        *start,
                        *end,
                        runtime_anchor(
                            *use.start_anchor),
                        runtime_anchor(
                            *use.end_anchor),
                        use.follows_source_direction,
                        crosses_seam,
                        false});
            }
            return result;
        };

    auto outer =
        resolve_loop(intent.outer);
    if (!outer) {
        return {
            ProfileIntentResolutionStatus::
                unresolved_intent,
            std::nullopt};
    }

    std::vector<sketch::RegionLoop2D>
        holes;
    holes.reserve(intent.holes.size());
    for (const auto& hole :
         intent.holes) {
        auto resolved =
            resolve_loop(hole);
        if (!resolved) {
            return {
                ProfileIntentResolutionStatus::
                    unresolved_intent,
                std::nullopt};
        }
        holes.push_back(
            std::move(*resolved));
    }

    auto region =
        sketch::validateRegionBoundary(
            model,
            std::move(*outer),
            std::move(holes));
    if (!region) {
        const auto analysis =
            sketch::analyzeRegions(model);
        if (std::any_of(
                analysis.diagnostics.begin(),
                analysis.diagnostics.end(),
                [&references](
                    const sketch::
                        RegionAnalysisDiagnostic2D&
                            diagnostic) {
                    return diagnosticTouches(
                        diagnostic,
                        references);
                })) {
            return {
                ProfileIntentResolutionStatus::
                    ambiguous_topology,
                std::nullopt};
        }

        return {
            ProfileIntentResolutionStatus::
                unresolved_intent,
            std::nullopt};
    }

    return {
        ProfileIntentResolutionStatus::valid,
        std::move(region)};

}

ProfileAreaEditResult applyProfileAreaEdit(
    const sketch::SketchModel& model,
    const ProfileRegionIntent& draft,
    std::uint32_t region_index,
    ProfileAreaEditMode mode) {
    const auto resolved =
        resolveProfileRegionIntent(
            model,
            draft);
    if (!resolved.valid()) {
        return {
            ProfileAreaEditStatus::invalid_draft,
            std::nullopt,
            std::nullopt,
            resolved.status};
    }

    const auto analysis =
        sketch::analyzeRegions(model);
    const auto target =
        std::find_if(
            analysis.regions.begin(),
            analysis.regions.end(),
            [region_index](
                const sketch::RegionCandidate2D&
                    region) {
                return region.region_index ==
                       region_index;
            });
    if (target == analysis.regions.end()) {
        return {
            ProfileAreaEditStatus::
                invalid_selection,
            std::nullopt,
            std::nullopt};
    }

    sketch::RegionAnalysis2D draft_analysis;
    auto draft_region =
        *resolved.region;
    draft_region.region_index = 0U;
    draft_analysis.regions.push_back(
        draft_region);

    std::vector<bool> selected(
        analysis.regions.size(),
        false);
    std::optional<std::size_t>
        target_position;

    for (std::size_t index = 0U;
         index < analysis.regions.size();
         ++index) {
        const auto& candidate =
            analysis.regions[index];
        if (candidate.region_index ==
            region_index) {
            target_position = index;
        }

        const auto sample =
            sketch::regionInteriorPoint(
                model,
                candidate);
        if (!sample) {
            return {
                ProfileAreaEditStatus::
                    ambiguous_topology,
                std::nullopt,
                std::nullopt};
        }

        const auto membership =
            sketch::pickRegion(
                model,
                draft_analysis,
                *sample);
        if (membership.location ==
                sketch::RegionPointLocation::
                    boundary ||
            membership.location ==
                sketch::RegionPointLocation::
                    ambiguous) {
            return {
                ProfileAreaEditStatus::
                    ambiguous_topology,
                std::nullopt,
                std::nullopt};
        }

        selected[index] =
            membership.location ==
            sketch::RegionPointLocation::inside;
    }

    if (!target_position) {
        return {
            ProfileAreaEditStatus::
                invalid_selection,
            std::nullopt,
            std::nullopt};
    }

    const bool target_selected =
        selected[*target_position];
    if ((mode ==
             ProfileAreaEditMode::add_area &&
         target_selected) ||
        (mode ==
             ProfileAreaEditMode::subtract_area &&
         !target_selected)) {
        return {
            ProfileAreaEditStatus::no_change,
            draft,
            *resolved.region};
    }

    selected[*target_position] =
        mode ==
        ProfileAreaEditMode::add_area;

    std::vector<sketch::RegionCandidate2D>
        material_cells;
    for (std::size_t index = 0U;
         index < analysis.regions.size();
         ++index) {
        if (selected[index]) {
            material_cells.push_back(
                analysis.regions[index]);
        }
    }

    const auto composition =
        sketch::composeRegionCells(
            model,
            material_cells);
    if (composition.status ==
            sketch::RegionCompositionStatus::
                disconnected ||
        composition.status ==
            sketch::RegionCompositionStatus::empty) {
        return {
            ProfileAreaEditStatus::
                disconnected_result,
            std::nullopt,
            std::nullopt};
    }
    if (!composition.valid()) {
        return {
            ProfileAreaEditStatus::
                ambiguous_topology,
            std::nullopt,
            std::nullopt};
    }

    const auto intent =
        makeProfileRegionIntent(
            *composition.region);
    if (!intent) {
        return {
            ProfileAreaEditStatus::
                ambiguous_topology,
            std::nullopt,
            std::nullopt};
    }

    if (*intent == draft) {
        return {
            ProfileAreaEditStatus::no_change,
            draft,
            *composition.region};
    }

    return {
        ProfileAreaEditStatus::changed,
        *intent,
        *composition.region};
}

} // namespace simplesolid2::part
