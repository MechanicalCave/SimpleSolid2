#include <simplesolid2/sketch/transform.hpp>

#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <set>
#include <utility>

namespace simplesolid2::sketch {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;

[[nodiscard]] std::optional<Point2> transformedArcEndpoint(
    const SketchArcState& arc,
    SketchTransformEndpointRole endpoint) noexcept {
    const double angle =
        endpoint == SketchTransformEndpointRole::start
            ? arc.start_angle
            : arc.start_angle + arc.sweep_angle;
    if (!std::isfinite(angle) ||
        !arc.center.finite() ||
        !std::isfinite(arc.radius) ||
        arc.radius <= 0.0) {
        return std::nullopt;
    }

    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    if (!std::isfinite(cosine) ||
        !std::isfinite(sine)) {
        return std::nullopt;
    }

    const Point2 result{
        std::fma(arc.radius, cosine, arc.center.u),
        std::fma(arc.radius, sine, arc.center.v)};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

struct LineEndpointCandidate final {
    EntityId id;
    Point2 point;
};

[[nodiscard]] double endpointCandidateWindow(
    const SketchArcState& arc,
    Point2 endpoint) noexcept {
    const double scale =
        std::max(
            {1.0,
             std::abs(arc.center.u),
             std::abs(arc.center.v),
             std::abs(endpoint.u),
             std::abs(endpoint.v),
             arc.radius});
    return 128.0 *
           std::numeric_limits<double>::epsilon() *
           scale;
}

void captureLineArcEndpointContacts(
    const SketchModel& model,
    SketchTransformGeometry& geometry) {
    if (geometry.lines.empty() ||
        geometry.arcs.empty()) {
        return;
    }

    std::vector<LineEndpointCandidate> line_endpoints;
    line_endpoints.reserve(geometry.lines.size() * 2U);
    for (const auto& line : geometry.lines) {
        line_endpoints.push_back({line.id, line.start});
        line_endpoints.push_back({line.id, line.end});
    }
    std::sort(
        line_endpoints.begin(),
        line_endpoints.end(),
        [](const LineEndpointCandidate& lhs,
           const LineEndpointCandidate& rhs) {
            if (lhs.point.u != rhs.point.u) {
                return lhs.point.u < rhs.point.u;
            }
            if (lhs.point.v != rhs.point.v) {
                return lhs.point.v < rhs.point.v;
            }
            return lhs.id < rhs.id;
        });

    std::set<std::pair<EntityId, EntityId>> checked;
    for (const auto& arc : geometry.arcs) {
        for (const auto endpoint_role :
             {SketchTransformEndpointRole::start,
              SketchTransformEndpointRole::end}) {
            const auto endpoint =
                transformedArcEndpoint(
                    arc,
                    endpoint_role);
            if (!endpoint) continue;

            const double window =
                endpointCandidateWindow(
                    arc,
                    *endpoint);
            const auto lower =
                std::lower_bound(
                    line_endpoints.begin(),
                    line_endpoints.end(),
                    endpoint->u - window,
                    [](const LineEndpointCandidate& candidate,
                       double u) {
                        return candidate.point.u < u;
                    });

            for (auto it = lower;
                 it != line_endpoints.end() &&
                 it->point.u <= endpoint->u + window;
                 ++it) {
                if (std::abs(it->point.v - endpoint->v) >
                    window) {
                    continue;
                }

                const auto key =
                    std::pair<EntityId, EntityId>{
                        it->id,
                        arc.id};
                if (!checked.insert(key).second) {
                    continue;
                }

                const auto relation =
                    analyzeCurveRelation(
                        model,
                        it->id,
                        arc.id);
                if (relation.status !=
                    CurveRelationStatus::discrete) {
                    continue;
                }

                const bool line_is_first =
                    relation.first_entity == it->id;
                for (const auto& intersection :
                     relation.intersections) {
                    if (!intersection.first_endpoint ||
                        !intersection.second_endpoint) {
                        continue;
                    }

                    const double line_parameter =
                        line_is_first
                            ? intersection.first_parameter
                            : intersection.second_parameter;
                    const double arc_parameter =
                        line_is_first
                            ? intersection.second_parameter
                            : intersection.first_parameter;
                    if ((line_parameter != 0.0 &&
                         line_parameter != 1.0) ||
                        (arc_parameter != 0.0 &&
                         arc_parameter != 1.0)) {
                        continue;
                    }

                    geometry.line_arc_contacts.push_back(
                        SketchTransformLineArcContact{
                            it->id,
                            line_parameter == 0.0
                                ? SketchTransformEndpointRole::start
                                : SketchTransformEndpointRole::end,
                            arc.id,
                            arc_parameter == 0.0
                                ? SketchTransformEndpointRole::start
                                : SketchTransformEndpointRole::end});
                }
            }
        }
    }

    std::sort(
        geometry.line_arc_contacts.begin(),
        geometry.line_arc_contacts.end(),
        [](const SketchTransformLineArcContact& lhs,
           const SketchTransformLineArcContact& rhs) {
            if (lhs.line_id != rhs.line_id) {
                return lhs.line_id < rhs.line_id;
            }
            if (lhs.line_endpoint != rhs.line_endpoint) {
                return lhs.line_endpoint < rhs.line_endpoint;
            }
            if (lhs.arc_id != rhs.arc_id) {
                return lhs.arc_id < rhs.arc_id;
            }
            return lhs.arc_endpoint < rhs.arc_endpoint;
        });
    geometry.line_arc_contacts.erase(
        std::unique(
            geometry.line_arc_contacts.begin(),
            geometry.line_arc_contacts.end()),
        geometry.line_arc_contacts.end());
}

[[nodiscard]] bool preserveLineArcEndpointContacts(
    SketchTransformGeometry& geometry) noexcept {
    for (const auto& contact :
         geometry.line_arc_contacts) {
        const auto line =
            std::find_if(
                geometry.lines.begin(),
                geometry.lines.end(),
                [&contact](const SketchLineState& value) {
                    return value.id == contact.line_id;
                });
        const auto arc =
            std::find_if(
                geometry.arcs.begin(),
                geometry.arcs.end(),
                [&contact](const SketchArcState& value) {
                    return value.id == contact.arc_id;
                });
        if (line == geometry.lines.end() ||
            arc == geometry.arcs.end()) {
            return false;
        }

        const auto point =
            transformedArcEndpoint(
                *arc,
                contact.arc_endpoint);
        if (!point) {
            return false;
        }

        if (contact.line_endpoint ==
            SketchTransformEndpointRole::start) {
            line->start = *point;
        } else {
            line->end = *point;
        }
    }
    return true;
}

[[nodiscard]] bool validGeometry(
    const SketchTransformGeometry& geometry) noexcept {
    if (geometry.empty()) {
        return false;
    }

    for (const auto& line : geometry.lines) {
        if (!line.id.valid() ||
            !line.start.finite() ||
            !line.end.finite() ||
            line.start == line.end) {
            return false;
        }
    }

    for (const auto& circle : geometry.circles) {
        if (!circle.id.valid() ||
            !circle.center.finite() ||
            !std::isfinite(circle.radius) ||
            circle.radius <= 0.0) {
            return false;
        }
    }

    for (const auto& arc : geometry.arcs) {
        if (!arc.id.valid() ||
            !arc.center.finite() ||
            !std::isfinite(arc.radius) ||
            arc.radius <= 0.0 ||
            !std::isfinite(arc.start_angle) ||
            !std::isfinite(arc.sweep_angle) ||
            arc.sweep_angle == 0.0 ||
            std::abs(arc.sweep_angle) >= full_turn) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] std::optional<Point2> rotatePoint(
    Point2 point,
    Point2 base,
    double cosine,
    double sine) noexcept {
    const double u = point.u - base.u;
    const double v = point.v - base.v;
    Point2 result{
        base.u + cosine * u - sine * v,
        base.v + sine * u + cosine * v};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<Point2> scalePoint(
    Point2 point,
    Point2 base,
    double factor) noexcept {
    Point2 result{
        base.u + (point.u - base.u) * factor,
        base.v + (point.v - base.v) * factor};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<Point2> mirrorPoint(
    Point2 point,
    Point2 axis_start,
    double unit_u,
    double unit_v) noexcept {
    const double rel_u = point.u - axis_start.u;
    const double rel_v = point.v - axis_start.v;
    const double projection =
        rel_u * unit_u +
        rel_v * unit_v;

    const Point2 projected{
        axis_start.u + projection * unit_u,
        axis_start.v + projection * unit_v};
    const Point2 result{
        2.0 * projected.u - point.u,
        2.0 * projected.v - point.v};

    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

} // namespace

std::optional<SketchTransformGeometry>
captureSketchTransformGeometry(
    const SketchModel& model,
    const std::vector<EntityId>& entity_ids) {
    if (entity_ids.empty()) {
        return std::nullopt;
    }

    SketchTransformGeometry result;
    result.lines.reserve(entity_ids.size());
    result.circles.reserve(entity_ids.size());
    result.arcs.reserve(entity_ids.size());

    std::set<EntityId> seen;

    for (const auto id : entity_ids) {
        if (!id.valid() ||
            !seen.insert(id).second) {
            return std::nullopt;
        }

        if (const auto* line = model.findLine(id)) {
            result.lines.push_back(
                SketchLineState{
                    line->id(),
                    line->start(),
                    line->end(),
                    line->role()});
            continue;
        }

        if (const auto* circle = model.findCircle(id)) {
            result.circles.push_back(
                SketchCircleState{
                    circle->id(),
                    circle->center(),
                    circle->radius(),
                    circle->role()});
            continue;
        }

        if (const auto* arc = model.findArc(id)) {
            result.arcs.push_back(
                SketchArcState{
                    arc->id(),
                    arc->center(),
                    arc->radius(),
                    arc->startAngle(),
                    arc->sweepAngle(),
                    arc->role()});
            continue;
        }

        return std::nullopt;
    }

    if (!result.empty()) {
        captureLineArcEndpointContacts(
            model,
            result);
    }

    return result.empty()
        ? std::nullopt
        : std::optional<SketchTransformGeometry>{
              std::move(result)};
}

std::optional<SketchTransformGeometry>
translateSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 delta) {
    if (geometry.empty() ||
        !delta.finite()) {
        return std::nullopt;
    }

    auto result = geometry;

    for (auto& line : result.lines) {
        line.start.u += delta.u;
        line.start.v += delta.v;
        line.end.u += delta.u;
        line.end.v += delta.v;

        if (!line.id.valid() ||
            !line.start.finite() ||
            !line.end.finite() ||
            line.start == line.end) {
            return std::nullopt;
        }
    }

    for (auto& circle : result.circles) {
        circle.center.u += delta.u;
        circle.center.v += delta.v;

        if (!circle.id.valid() ||
            !circle.center.finite()) {
            return std::nullopt;
        }
    }

    for (auto& arc : result.arcs) {
        arc.center.u += delta.u;
        arc.center.v += delta.v;

        if (!arc.id.valid() ||
            !arc.center.finite()) {
            return std::nullopt;
        }
    }

    if (!preserveLineArcEndpointContacts(result) ||
        !validGeometry(result)) {
        return std::nullopt;
    }

    return result;
}

std::optional<SketchTransformGeometry>
rotateSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 base_point,
    double angle_radians) {
    if (!validGeometry(geometry) ||
        !base_point.finite() ||
        !std::isfinite(angle_radians)) {
        return std::nullopt;
    }

    if (angle_radians == 0.0) {
        return geometry;
    }

    const double cosine = std::cos(angle_radians);
    const double sine = std::sin(angle_radians);
    if (!std::isfinite(cosine) ||
        !std::isfinite(sine)) {
        return std::nullopt;
    }

    auto result = geometry;

    for (auto& line : result.lines) {
        const auto start =
            rotatePoint(
                line.start,
                base_point,
                cosine,
                sine);
        const auto end =
            rotatePoint(
                line.end,
                base_point,
                cosine,
                sine);
        if (!start || !end) {
            return std::nullopt;
        }
        line.start = *start;
        line.end = *end;
    }

    for (auto& circle : result.circles) {
        const auto center =
            rotatePoint(
                circle.center,
                base_point,
                cosine,
                sine);
        if (!center) {
            return std::nullopt;
        }
        circle.center = *center;
    }

    for (auto& arc : result.arcs) {
        const auto center =
            rotatePoint(
                arc.center,
                base_point,
                cosine,
                sine);
        if (!center) {
            return std::nullopt;
        }
        arc.center = *center;
        arc.start_angle += angle_radians;
    }

    if (!preserveLineArcEndpointContacts(result)) {
        return std::nullopt;
    }

    return validGeometry(result)
        ? std::optional<SketchTransformGeometry>{
              std::move(result)}
        : std::nullopt;
}

std::optional<SketchTransformGeometry>
scaleSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 base_point,
    double factor) {
    if (!validGeometry(geometry) ||
        !base_point.finite() ||
        !std::isfinite(factor) ||
        factor <= 0.0) {
        return std::nullopt;
    }

    if (factor == 1.0) {
        return geometry;
    }

    auto result = geometry;

    for (auto& line : result.lines) {
        const auto start =
            scalePoint(
                line.start,
                base_point,
                factor);
        const auto end =
            scalePoint(
                line.end,
                base_point,
                factor);
        if (!start || !end) {
            return std::nullopt;
        }
        line.start = *start;
        line.end = *end;
    }

    for (auto& circle : result.circles) {
        const auto center =
            scalePoint(
                circle.center,
                base_point,
                factor);
        const double radius =
            circle.radius * factor;
        if (!center ||
            !std::isfinite(radius) ||
            radius <= 0.0) {
            return std::nullopt;
        }
        circle.center = *center;
        circle.radius = radius;
    }

    for (auto& arc : result.arcs) {
        const auto center =
            scalePoint(
                arc.center,
                base_point,
                factor);
        const double radius =
            arc.radius * factor;
        if (!center ||
            !std::isfinite(radius) ||
            radius <= 0.0) {
            return std::nullopt;
        }
        arc.center = *center;
        arc.radius = radius;
    }

    if (!preserveLineArcEndpointContacts(result)) {
        return std::nullopt;
    }

    return validGeometry(result)
        ? std::optional<SketchTransformGeometry>{
              std::move(result)}
        : std::nullopt;
}

std::optional<SketchTransformGeometry>
mirrorSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 axis_start,
    Point2 axis_end) {
    if (!validGeometry(geometry) ||
        !axis_start.finite() ||
        !axis_end.finite() ||
        axis_start == axis_end) {
        return std::nullopt;
    }

    const double axis_u =
        axis_end.u - axis_start.u;
    const double axis_v =
        axis_end.v - axis_start.v;
    const double length =
        std::hypot(axis_u, axis_v);
    if (!std::isfinite(length) ||
        length <= 0.0) {
        return std::nullopt;
    }

    const double unit_u = axis_u / length;
    const double unit_v = axis_v / length;
    if (!std::isfinite(unit_u) ||
        !std::isfinite(unit_v)) {
        return std::nullopt;
    }

    auto result = geometry;

    for (auto& line : result.lines) {
        const auto start =
            mirrorPoint(
                line.start,
                axis_start,
                unit_u,
                unit_v);
        const auto end =
            mirrorPoint(
                line.end,
                axis_start,
                unit_u,
                unit_v);
        if (!start || !end) {
            return std::nullopt;
        }
        line.start = *start;
        line.end = *end;
    }

    for (auto& circle : result.circles) {
        const auto center =
            mirrorPoint(
                circle.center,
                axis_start,
                unit_u,
                unit_v);
        if (!center) {
            return std::nullopt;
        }
        circle.center = *center;
    }

    const double axis_angle =
        std::atan2(unit_v, unit_u);
    if (!std::isfinite(axis_angle)) {
        return std::nullopt;
    }

    for (auto& arc : result.arcs) {
        const auto center =
            mirrorPoint(
                arc.center,
                axis_start,
                unit_u,
                unit_v);
        if (!center) {
            return std::nullopt;
        }

        arc.center = *center;
        arc.start_angle =
            2.0 * axis_angle -
            arc.start_angle;
        arc.sweep_angle =
            -arc.sweep_angle;
    }

    if (!preserveLineArcEndpointContacts(result)) {
        return std::nullopt;
    }

    return validGeometry(result)
        ? std::optional<SketchTransformGeometry>{
              std::move(result)}
        : std::nullopt;
}

} // namespace simplesolid2::sketch
