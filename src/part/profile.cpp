#include <simplesolid2/part/profile.hpp>

#include <algorithm>
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

[[nodiscard]] std::optional<ProfileLoopIntent>
convertLoop(
    const sketch::RegionLoop2D& source) {
    ProfileLoopIntent result;
    result.boundary.reserve(
        source.boundary.size());

    for (const auto& use :
         source.boundary) {
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

    const auto analysis =
        sketch::analyzeRegions(model);

    std::optional<sketch::RegionCandidate2D>
        matched;
    for (const auto& candidate :
         analysis.regions) {
        const auto candidate_intent =
            makeProfileRegionIntent(candidate);
        if (!candidate_intent ||
            *candidate_intent != intent) {
            continue;
        }
        if (matched) {
            return {
                ProfileIntentResolutionStatus::
                    ambiguous_topology,
                std::nullopt};
        }
        matched = candidate;
    }

    if (matched) {
        return {
            ProfileIntentResolutionStatus::valid,
            std::move(matched)};
    }

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

} // namespace simplesolid2::part
