#include <simplesolid2/sketch/snap.hpp>

#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <numeric>

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
    const SketchArcState& arc,
    Point2 pointer) noexcept {
    const Point2 start =
        circlePoint(
            arc.center,
            arc.radius,
            arc.start_angle);
    const Point2 end =
        circlePoint(
            arc.center,
            arc.radius,
            arc.start_angle + arc.sweep_angle);
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
                arc.center,
                arc.radius,
                pointer)) {
        const double angle =
            std::atan2(
                radial->v - arc.center.v,
                radial->u - arc.center.u);
        if (angleOnArc(
                arc.start_angle,
                arc.sweep_angle,
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
            arc.id,
            SnapSemanticRole::curve_nearest)};
    return result.valid()
        ? std::optional<SnapCandidate>{result}
        : std::nullopt;
}

[[nodiscard]] bool arcContainsPointDirection(
    const SketchArcState& arc,
    Point2 point) noexcept {
    const double angle =
        std::atan2(
            point.v - arc.center.v,
            point.u - arc.center.u);
    return angleOnArc(
        arc.start_angle,
        arc.sweep_angle,
        angle);
}

void appendRadialPerpendicular(
    std::vector<SnapCandidate>& result,
    EntityId id,
    Point2 center,
    double radius,
    Point2 base,
    const SketchArcState* arc) {
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
    const SketchArcState* arc) {
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

    const auto state = model.state();
    const auto found =
        std::find_if(
            state.arcs.begin(),
            state.arcs.end(),
            [entity](const SketchArcState& arc) {
                return arc.id == entity;
            });
    if (found != state.arcs.end()) {
        return nearestArcCandidate(
            *found,
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

    const auto state = model.state();
    const auto found =
        std::find_if(
            state.arcs.begin(),
            state.arcs.end(),
            [entity](const SketchArcState& arc) {
                return arc.id == entity;
            });
    if (found != state.arcs.end()) {
        appendRadialPerpendicular(
            result,
            entity,
            found->center,
            found->radius,
            base,
            &*found);
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

    const auto state = model.state();
    const auto found =
        std::find_if(
            state.arcs.begin(),
            state.arcs.end(),
            [entity](const SketchArcState& arc) {
                return arc.id == entity;
            });
    if (found != state.arcs.end()) {
        appendTangents(
            result,
            entity,
            found->center,
            found->radius,
            base,
            &*found);
    }
    return result;
}

} // namespace simplesolid2::sketch
