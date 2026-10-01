#include <simplesolid2/sketch/snap.hpp>

#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <numeric>
#include <utility>

namespace simplesolid2::sketch {
namespace {

constexpr double tau =
    2.0 * std::numbers::pi_v<double>;

[[nodiscard]] double normalizeAngle(
    double angle) noexcept {
    if (!std::isfinite(angle)) {
        return angle;
    }
    double value = std::fmod(angle, tau);
    if (value < 0.0) {
        value += tau;
    }
    return value;
}

[[nodiscard]] bool angleOnArc(
    double start,
    double sweep,
    double angle) noexcept {
    if (!std::isfinite(start) ||
        !std::isfinite(sweep) ||
        !std::isfinite(angle) ||
        sweep == 0.0) {
        return false;
    }

    if (std::abs(sweep) >= tau) {
        return true;
    }

    if (sweep > 0.0) {
        const double delta =
            normalizeAngle(angle - start);
        return std::isfinite(delta) &&
               delta <= sweep;
    }

    const double delta =
        normalizeAngle(start - angle);
    return std::isfinite(delta) &&
           delta <= -sweep;
}

[[nodiscard]] Point2 circlePoint(
    Point2 center,
    double radius,
    double angle) noexcept {
    return {
        std::fma(radius, std::cos(angle), center.u),
        std::fma(radius, std::sin(angle), center.v)};
}

[[nodiscard]] SnapSourceRef entityPointSource(
    EntityId id,
    SnapSemanticRole role) {
    return {
        SnapSourceKind::entity_point,
        id,
        std::nullopt,
        role,
        0U};
}

[[nodiscard]] SnapSourceRef entityCurveSource(
    EntityId id,
    SnapSemanticRole role) {
    return {
        SnapSourceKind::entity_curve,
        id,
        std::nullopt,
        role,
        0U};
}

void appendCandidate(
    std::vector<SnapCandidate>& result,
    Point2 point,
    SnapKind kind,
    SnapSourceRef source) {
    SnapCandidate candidate{
        point,
        kind,
        std::move(source)};
    if (candidate.valid()) {
        result.push_back(std::move(candidate));
    }
}

[[nodiscard]] std::optional<Point2>
lineProjection(
    Point2 start,
    Point2 end,
    Point2 point,
    bool clamp_to_segment) noexcept {
    const double du = end.u - start.u;
    const double dv = end.v - start.v;
    const double length = std::hypot(du, dv);
    if (!std::isfinite(length) ||
        !(length > 0.0)) {
        return std::nullopt;
    }

    const double unit_u = du / length;
    const double unit_v = dv / length;
    const double relative_u = point.u - start.u;
    const double relative_v = point.v - start.v;
    double along =
        relative_u * unit_u +
        relative_v * unit_v;
    if (!std::isfinite(along)) {
        return std::nullopt;
    }

    if (clamp_to_segment) {
        along = std::clamp(along, 0.0, length);
    } else if (along < 0.0 || along > length) {
        return std::nullopt;
    }

    Point2 projected{
        std::fma(along, unit_u, start.u),
        std::fma(along, unit_v, start.v)};
    if (!projected.finite()) {
        return std::nullopt;
    }
    return projected;
}

[[nodiscard]] std::optional<Point2>
radialPoint(
    Point2 center,
    double radius,
    Point2 toward,
    double sign = 1.0) noexcept {
    const double du = toward.u - center.u;
    const double dv = toward.v - center.v;
    const double distance = std::hypot(du, dv);
    if (!std::isfinite(distance) ||
        !(distance > 0.0) ||
        !std::isfinite(radius) ||
        !(radius > 0.0)) {
        return std::nullopt;
    }

    const Point2 point{
        std::fma(
            sign * radius,
            du / distance,
            center.u),
        std::fma(
            sign * radius,
            dv / distance,
            center.v)};
    if (!point.finite()) {
        return std::nullopt;
    }
    return point;
}

[[nodiscard]] double pointDistance(
    Point2 first,
    Point2 second) noexcept {
    return std::hypot(
        first.u - second.u,
        first.v - second.v);
}

[[nodiscard]] std::optional<SnapCandidate>
nearestArcCandidate(
    const Arc& arc,
    Point2 pointer) noexcept {
    const Point2 start =
        circlePoint(
            arc.center(),
            arc.radius(),
            arc.startAngle());
    const Point2 end =
        circlePoint(
            arc.center(),
            arc.radius(),
            arc.startAngle() + arc.sweepAngle());
    if (!start.finite() || !end.finite()) {
        return std::nullopt;
    }

    Point2 best = start;
    double best_distance =
        pointDistance(pointer, start);
    const double end_distance =
        pointDistance(pointer, end);
    if (!std::isfinite(best_distance) ||
        !std::isfinite(end_distance)) {
        return std::nullopt;
    }
    if (end_distance < best_distance) {
        best = end;
        best_distance = end_distance;
    }

    if (const auto radial =
            radialPoint(
                arc.center(),
                arc.radius(),
                pointer)) {
        const double angle =
            std::atan2(
                radial->v - arc.center().v,
                radial->u - arc.center().u);
        if (angleOnArc(
                arc.startAngle(),
                arc.sweepAngle(),
                angle)) {
            const double radial_distance =
                pointDistance(pointer, *radial);
            if (std::isfinite(radial_distance) &&
                radial_distance < best_distance) {
                best = *radial;
            }
        }
    }

    SnapCandidate result{
        best,
        SnapKind::nearest,
        entityCurveSource(
            arc.id(),
            SnapSemanticRole::curve_nearest)};
    return result.valid()
        ? std::optional<SnapCandidate>{result}
        : std::nullopt;
}

[[nodiscard]] bool arcContainsPointDirection(
    const Arc& arc,
    Point2 point) noexcept {
    const double angle =
        std::atan2(
            point.v - arc.center().v,
            point.u - arc.center().u);
    return angleOnArc(
        arc.startAngle(),
        arc.sweepAngle(),
        angle);
}

void appendRadialPerpendicular(
    std::vector<SnapCandidate>& result,
    EntityId id,
    Point2 center,
    double radius,
    Point2 base,
    const Arc* arc) {
    for (const double sign : {1.0, -1.0}) {
        const auto point =
            radialPoint(
                center,
                radius,
                base,
                sign);
        if (!point) {
            continue;
        }
        if (arc != nullptr &&
            !arcContainsPointDirection(
                *arc,
                *point)) {
            continue;
        }
        appendCandidate(
            result,
            *point,
            SnapKind::perpendicular,
            entityCurveSource(
                id,
                SnapSemanticRole::
                    curve_perpendicular));
    }
}

void appendTangents(
    std::vector<SnapCandidate>& result,
    EntityId id,
    Point2 center,
    double radius,
    Point2 base,
    const Arc* arc) {
    const double du = base.u - center.u;
    const double dv = base.v - center.v;
    const double distance = std::hypot(du, dv);
    if (!std::isfinite(distance) ||
        !std::isfinite(radius) ||
        !(radius > 0.0) ||
        distance < radius ||
        !(distance > 0.0)) {
        return;
    }

    if (distance == radius) {
        if (arc == nullptr ||
            arcContainsPointDirection(
                *arc,
                base)) {
            appendCandidate(
                result,
                base,
                SnapKind::tangent,
                entityCurveSource(
                    id,
                    SnapSemanticRole::
                        curve_tangent));
        }
        return;
    }

    const double unit_u = du / distance;
    const double unit_v = dv / distance;
    const double cosine =
        std::clamp(
            radius / distance,
            0.0,
            1.0);
    const double sine =
        std::sqrt(
            std::max(
                0.0,
                1.0 - cosine * cosine));

    for (const double sign : {1.0, -1.0}) {
        const double tangent_u =
            cosine * unit_u -
            sign * sine * unit_v;
        const double tangent_v =
            cosine * unit_v +
            sign * sine * unit_u;
        const Point2 point{
            std::fma(
                radius,
                tangent_u,
                center.u),
            std::fma(
                radius,
                tangent_v,
                center.v)};
        if (!point.finite()) {
            continue;
        }
        if (arc != nullptr &&
            !arcContainsPointDirection(
                *arc,
                point)) {
            continue;
        }
        appendCandidate(
            result,
            point,
            SnapKind::tangent,
            entityCurveSource(
                id,
                SnapSemanticRole::curve_tangent));
    }
}

[[nodiscard]] std::uint8_t kindRank(
    SnapKind kind) noexcept {
    switch (kind) {
    case SnapKind::endpoint:
        return 0U;
    case SnapKind::midpoint:
        return 1U;
    case SnapKind::center:
        return 2U;
    case SnapKind::quadrant:
        return 3U;
    case SnapKind::intersection:
        return 4U;
    case SnapKind::origin:
        return 5U;
    case SnapKind::perpendicular:
        return 6U;
    case SnapKind::tangent:
        return 7U;
    case SnapKind::nearest:
        return 8U;
    }
    return 255U;
}

} // namespace

bool SnapEligibility::enabled(
    SnapKind kind) const noexcept {
    switch (kind) {
    case SnapKind::endpoint:
        return endpoint;
    case SnapKind::midpoint:
        return midpoint;
    case SnapKind::center:
        return center;
    case SnapKind::quadrant:
        return quadrant;
    case SnapKind::intersection:
        return intersection;
    case SnapKind::origin:
        return origin;
    case SnapKind::perpendicular:
        return perpendicular;
    case SnapKind::tangent:
        return tangent;
    case SnapKind::nearest:
        return nearest;
    }
    return false;
}

SnapEligibility resolveSnapEligibility(
    const ObjectSnapPreferences& preferences,
    std::optional<TemporarySnapOverrideKind>
        temporary_override) noexcept {
    if (temporary_override) {
        SnapEligibility result;
        switch (*temporary_override) {
        case TemporarySnapOverrideKind::endpoint:
            result.endpoint = true;
            break;
        case TemporarySnapOverrideKind::midpoint:
            result.midpoint = true;
            break;
        case TemporarySnapOverrideKind::center:
            result.center = true;
            break;
        case TemporarySnapOverrideKind::quadrant:
            result.quadrant = true;
            break;
        case TemporarySnapOverrideKind::intersection:
            result.intersection = true;
            break;
        case TemporarySnapOverrideKind::perpendicular:
            result.perpendicular = true;
            break;
        case TemporarySnapOverrideKind::tangent:
            result.tangent = true;
            break;
        case TemporarySnapOverrideKind::nearest:
            result.nearest = true;
            break;
        case TemporarySnapOverrideKind::origin:
            result.origin = true;
            break;
        case TemporarySnapOverrideKind::extension:
            result.extension = true;
            break;
        case TemporarySnapOverrideKind::none:
            result.suppress_object_assistance = true;
            break;
        }

        // A temporary override is a restricted one-point family, not a
        // preference mutation and not a request for unrelated OTRACK
        // fallback. NONE is the fully object-derived suppression family.
        return result;
    }

    SnapEligibility result;
    if (preferences.master_enabled) {
        result.endpoint =
            preferences.modes.endpoint;
        result.midpoint =
            preferences.modes.midpoint;
        result.center =
            preferences.modes.center;
        result.quadrant =
            preferences.modes.quadrant;
        result.intersection =
            preferences.modes.intersection;
        result.origin =
            preferences.modes.origin;
        result.perpendicular =
            preferences.modes.perpendicular;
        result.tangent =
            preferences.modes.tangent;
        result.nearest =
            preferences.modes.nearest;
        result.extension =
            preferences.modes.extension;
    }

    // OTRACK has a separate master. Acquisition still needs an eligible
    // semantic anchor at the integration layer; keeping this flag separate
    // prevents the OSNAP master from silently mutating the user OTRACK
    // preference.
    result.object_tracking =
        preferences.object_tracking_enabled;
    return result;
}

bool SnapSourceRef::valid() const noexcept {
    switch (kind) {
    case SnapSourceKind::intrinsic_origin:
        return !first_entity &&
               !second_entity &&
               role ==
                   SnapSemanticRole::
                       intrinsic_origin;
    case SnapSourceKind::entity_point:
    case SnapSourceKind::entity_curve:
        return first_entity &&
               first_entity->valid() &&
               !second_entity;
    case SnapSourceKind::intersection:
        return first_entity &&
               second_entity &&
               first_entity->valid() &&
               second_entity->valid() &&
               *first_entity != *second_entity &&
               role ==
                   SnapSemanticRole::intersection;
    }
    return false;
}

SnapStableKey snapStableKey(
    const SnapCandidate& candidate) noexcept {
    return {
        kindRank(candidate.kind),
        candidate.source.kind,
        candidate.source.first_entity,
        candidate.source.second_entity,
        candidate.source.role,
        candidate.source.canonical_branch};
}


bool SnapScreenCandidate::valid() const noexcept {
    return candidate.valid() &&
           std::isfinite(screen_distance) &&
           screen_distance >= 0.0;
}

bool SnapResolutionPolicy::valid() const noexcept {
    return std::isfinite(capture_distance) &&
           std::isfinite(release_distance) &&
           capture_distance >= 0.0 &&
           release_distance >= capture_distance;
}

std::optional<SnapResolution>
resolveScreenSnap(
    SnapCaptureState& state,
    const std::vector<SnapScreenCandidate>& candidates,
    SnapResolutionPolicy policy) noexcept {
    if (!policy.valid()) {
        state.clear();
        return std::nullopt;
    }

    const auto is_nearest =
        [](const SnapScreenCandidate& item) {
            return item.candidate.kind ==
                   SnapKind::nearest;
        };
    const auto key =
        [](const SnapScreenCandidate& item) {
            return snapStableKey(item.candidate);
        };
    const auto better =
        [&is_nearest, &key](
            const SnapScreenCandidate& first,
            const SnapScreenCandidate& second) {
            const bool first_nearest =
                is_nearest(first);
            const bool second_nearest =
                is_nearest(second);
            if (first_nearest != second_nearest) {
                return !first_nearest;
            }
            if (first.screen_distance !=
                second.screen_distance) {
                return first.screen_distance <
                       second.screen_distance;
            }
            return key(first) < key(second);
        };

    const SnapScreenCandidate* best{};
    bool specific_in_capture{};
    for (const auto& item : candidates) {
        if (!item.valid() ||
            item.screen_distance >
                policy.capture_distance) {
            continue;
        }
        if (!is_nearest(item)) {
            specific_in_capture = true;
        }
        if (best == nullptr ||
            better(item, *best)) {
            best = &item;
        }
    }

    const SnapScreenCandidate* retained{};
    if (state.captured) {
        for (const auto& item : candidates) {
            if (!item.valid() ||
                item.screen_distance >
                    policy.release_distance ||
                snapStableKey(item.candidate) !=
                    *state.captured) {
                continue;
            }

            // A captured Nearest never blocks a newly eligible
            // specific snap. Specific capture hysteresis may hold
            // against other specific candidates until release.
            if (is_nearest(item) &&
                specific_in_capture) {
                break;
            }
            retained = &item;
            break;
        }
    }

    const SnapScreenCandidate* selected =
        retained != nullptr ? retained : best;
    if (selected == nullptr) {
        state.clear();
        return std::nullopt;
    }

    SnapResolution resolution;
    resolution.primary =
        selected->candidate;

    for (const auto& item : candidates) {
        if (!item.valid() ||
            item.candidate.point !=
                resolution.primary.point ||
            item.screen_distance >
                policy.release_distance) {
            continue;
        }
        resolution.coincident_candidates.push_back(
            item.candidate);
    }

    std::sort(
        resolution.coincident_candidates.begin(),
        resolution.coincident_candidates.end(),
        [](const SnapCandidate& first,
           const SnapCandidate& second) {
            return snapStableKey(first) <
                   snapStableKey(second);
        });
    resolution.coincident_candidates.erase(
        std::unique(
            resolution.coincident_candidates.begin(),
            resolution.coincident_candidates.end(),
            [](const SnapCandidate& first,
               const SnapCandidate& second) {
                return snapStableKey(first) ==
                       snapStableKey(second);
            }),
        resolution.coincident_candidates.end());

    if (resolution.coincident_candidates.empty()) {
        resolution.coincident_candidates.push_back(
            resolution.primary);
    }

    state.captured =
        snapStableKey(resolution.primary);
    return resolution;
}

std::vector<SnapCandidate>
staticSnapCandidates(
    const SketchModel& model,
    const SnapModeSet& modes) {
    std::vector<SnapCandidate> result;
    const auto state = model.state();

    for (const auto& line : state.lines) {
        if (modes.endpoint) {
            appendCandidate(
                result,
                line.start,
                SnapKind::endpoint,
                entityPointSource(
                    line.id,
                    SnapSemanticRole::line_start));
            appendCandidate(
                result,
                line.end,
                SnapKind::endpoint,
                entityPointSource(
                    line.id,
                    SnapSemanticRole::line_end));
        }
        if (modes.midpoint) {
            appendCandidate(
                result,
                {
                    std::midpoint(
                        line.start.u,
                        line.end.u),
                    std::midpoint(
                        line.start.v,
                        line.end.v)},
                SnapKind::midpoint,
                entityPointSource(
                    line.id,
                    SnapSemanticRole::
                        line_midpoint));
        }
    }

    for (const auto& circle : state.circles) {
        if (modes.center) {
            appendCandidate(
                result,
                circle.center,
                SnapKind::center,
                entityPointSource(
                    circle.id,
                    SnapSemanticRole::
                        circle_center));
        }
        if (modes.quadrant) {
            const std::array<
                std::pair<double, SnapSemanticRole>,
                4U>
                quadrants{{
                    {0.0,
                     SnapSemanticRole::
                         circle_quadrant_pos_u},
                    {std::numbers::pi_v<double> / 2.0,
                     SnapSemanticRole::
                         circle_quadrant_pos_v},
                    {std::numbers::pi_v<double>,
                     SnapSemanticRole::
                         circle_quadrant_neg_u},
                    {3.0 * std::numbers::pi_v<double> / 2.0,
                     SnapSemanticRole::
                         circle_quadrant_neg_v},
                }};
            for (const auto& [angle, role] :
                 quadrants) {
                appendCandidate(
                    result,
                    circlePoint(
                        circle.center,
                        circle.radius,
                        angle),
                    SnapKind::quadrant,
                    entityPointSource(
                        circle.id,
                        role));
            }
        }
    }

    for (const auto& arc : state.arcs) {
        if (modes.center) {
            appendCandidate(
                result,
                arc.center,
                SnapKind::center,
                entityPointSource(
                    arc.id,
                    SnapSemanticRole::arc_center));
        }
        if (modes.endpoint) {
            appendCandidate(
                result,
                circlePoint(
                    arc.center,
                    arc.radius,
                    arc.start_angle),
                SnapKind::endpoint,
                entityPointSource(
                    arc.id,
                    SnapSemanticRole::arc_start));
            appendCandidate(
                result,
                circlePoint(
                    arc.center,
                    arc.radius,
                    arc.start_angle +
                        arc.sweep_angle),
                SnapKind::endpoint,
                entityPointSource(
                    arc.id,
                    SnapSemanticRole::arc_end));
        }
        if (modes.midpoint) {
            appendCandidate(
                result,
                circlePoint(
                    arc.center,
                    arc.radius,
                    arc.start_angle +
                        arc.sweep_angle * 0.5),
                SnapKind::midpoint,
                entityPointSource(
                    arc.id,
                    SnapSemanticRole::
                        arc_midpoint));
        }
        if (modes.quadrant) {
            const std::array<
                std::pair<double, SnapSemanticRole>,
                4U>
                quadrants{{
                    {0.0,
                     SnapSemanticRole::
                         arc_quadrant_pos_u},
                    {std::numbers::pi_v<double> / 2.0,
                     SnapSemanticRole::
                         arc_quadrant_pos_v},
                    {std::numbers::pi_v<double>,
                     SnapSemanticRole::
                         arc_quadrant_neg_u},
                    {3.0 * std::numbers::pi_v<double> / 2.0,
                     SnapSemanticRole::
                         arc_quadrant_neg_v},
                }};
            for (const auto& [angle, role] :
                 quadrants) {
                if (!angleOnArc(
                        arc.start_angle,
                        arc.sweep_angle,
                        angle)) {
                    continue;
                }
                appendCandidate(
                    result,
                    circlePoint(
                        arc.center,
                        arc.radius,
                        angle),
                    SnapKind::quadrant,
                    entityPointSource(
                        arc.id,
                        role));
            }
        }
    }

    if (modes.origin) {
        appendCandidate(
            result,
            {0.0, 0.0},
            SnapKind::origin,
            {
                SnapSourceKind::intrinsic_origin,
                std::nullopt,
                std::nullopt,
                SnapSemanticRole::intrinsic_origin,
                0U});
    }

    return result;
}

std::vector<SnapCandidate>
intersectionSnapCandidates(
    const SketchModel& model,
    EntityId first,
    EntityId second) {
    if (!first.valid() ||
        !second.valid() ||
        first == second) {
        return {};
    }

    const auto relation =
        analyzeCurveRelation(
            model,
            first,
            second);
    if (relation.status !=
        CurveRelationStatus::discrete) {
        return {};
    }

    std::vector<SnapCandidate> result;
    result.reserve(
        relation.intersections.size());
    for (const auto& intersection :
         relation.intersections) {
        appendCandidate(
            result,
            intersection.point,
            SnapKind::intersection,
            {
                SnapSourceKind::intersection,
                relation.first_entity,
                relation.second_entity,
                SnapSemanticRole::intersection,
                intersection.canonical_branch});
    }
    return result;
}

std::optional<SnapCandidate>
nearestSnapCandidate(
    const SketchModel& model,
    EntityId entity,
    Point2 pointer) noexcept {
    if (!entity.valid() ||
        !pointer.finite()) {
        return std::nullopt;
    }

    if (const auto* line =
            model.findLine(entity)) {
        const auto point =
            lineProjection(
                line->start(),
                line->end(),
                pointer,
                true);
        if (!point) {
            return std::nullopt;
        }
        SnapCandidate result{
            *point,
            SnapKind::nearest,
            entityCurveSource(
                entity,
                SnapSemanticRole::curve_nearest)};
        return result.valid()
            ? std::optional<SnapCandidate>{result}
            : std::nullopt;
    }

    if (const auto* circle =
            model.findCircle(entity)) {
        const auto point =
            radialPoint(
                circle->center(),
                circle->radius(),
                pointer);
        if (!point) {
            return std::nullopt;
        }
        SnapCandidate result{
            *point,
            SnapKind::nearest,
            entityCurveSource(
                entity,
                SnapSemanticRole::curve_nearest)};
        return result.valid()
            ? std::optional<SnapCandidate>{result}
            : std::nullopt;
    }

    if (const auto* arc =
            model.findArc(entity)) {
        return nearestArcCandidate(
            *arc,
            pointer);
    }

    return std::nullopt;
}

std::vector<SnapCandidate>
perpendicularSnapCandidates(
    const SketchModel& model,
    EntityId entity,
    Point2 base) noexcept {
    if (!entity.valid() || !base.finite()) {
        return {};
    }

    std::vector<SnapCandidate> result;
    if (const auto* line =
            model.findLine(entity)) {
        const auto foot =
            lineProjection(
                line->start(),
                line->end(),
                base,
                false);
        if (foot) {
            appendCandidate(
                result,
                *foot,
                SnapKind::perpendicular,
                entityCurveSource(
                    entity,
                    SnapSemanticRole::
                        curve_perpendicular));
        }
        return result;
    }

    if (const auto* circle =
            model.findCircle(entity)) {
        appendRadialPerpendicular(
            result,
            entity,
            circle->center(),
            circle->radius(),
            base,
            nullptr);
        return result;
    }

    if (const auto* arc =
            model.findArc(entity)) {
        appendRadialPerpendicular(
            result,
            entity,
            arc->center(),
            arc->radius(),
            base,
            arc);
    }
    return result;
}

std::vector<SnapCandidate>
tangentSnapCandidates(
    const SketchModel& model,
    EntityId entity,
    Point2 base) noexcept {
    if (!entity.valid() || !base.finite()) {
        return {};
    }

    std::vector<SnapCandidate> result;
    if (const auto* circle =
            model.findCircle(entity)) {
        appendTangents(
            result,
            entity,
            circle->center(),
            circle->radius(),
            base,
            nullptr);
        return result;
    }

    if (const auto* arc =
            model.findArc(entity)) {
        appendTangents(
            result,
            entity,
            arc->center(),
            arc->radius(),
            base,
            arc);
    }
    return result;
}

} // namespace simplesolid2::sketch
