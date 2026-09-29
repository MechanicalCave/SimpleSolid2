#pragma once

#include <simplesolid2/sketch/sketch_model.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <variant>
#include <vector>

namespace simplesolid2::sketch {

enum class MeasurePointRole : std::uint8_t {
    line_start,
    line_midpoint,
    line_end,
    circle_center,
    circle_quadrant_pos_u,
    circle_quadrant_pos_v,
    circle_quadrant_neg_u,
    circle_quadrant_neg_v,
    arc_center,
    arc_start,
    arc_end,
    arc_midpoint,
};

struct MeasurePointRef final {
    EntityId entity_id;
    MeasurePointRole role{MeasurePointRole::line_midpoint};

    [[nodiscard]] bool valid() const noexcept {
        return entity_id.valid();
    }

    friend bool operator==(
        const MeasurePointRef&,
        const MeasurePointRef&) = default;
};

struct ResolvedMeasurePoint final {
    MeasurePointRef ref;
    Point2 point;
    EntityRole role{EntityRole::regular};

    friend bool operator==(
        const ResolvedMeasurePoint&,
        const ResolvedMeasurePoint&) = default;
};

struct MeasureLineRef final {
    EntityId entity_id;

    [[nodiscard]] bool valid() const noexcept {
        return entity_id.valid();
    }

    friend bool operator==(
        const MeasureLineRef&,
        const MeasureLineRef&) = default;
};

using MeasureRelationTarget =
    std::variant<MeasurePointRef, MeasureLineRef>;

struct PointPointMeasurement final {
    MeasurePointRef first;
    MeasurePointRef second;
    Point2 first_point;
    Point2 second_point;
    double distance{};
    double delta_u{};
    double delta_v{};
    double angle_from_positive_u{};

    friend bool operator==(
        const PointPointMeasurement&,
        const PointPointMeasurement&) = default;
};

struct PointLineMeasurement final {
    MeasurePointRef point;
    MeasureLineRef line;
    Point2 point_position;
    Point2 line_start;
    Point2 line_end;
    Point2 perpendicular_foot;
    bool foot_on_segment{};
    double distance{};

    friend bool operator==(
        const PointLineMeasurement&,
        const PointLineMeasurement&) = default;
};

struct LineLineAngleMeasurement final {
    MeasureLineRef first;
    MeasureLineRef second;
    double smaller_undirected_angle{};

    friend bool operator==(
        const LineLineAngleMeasurement&,
        const LineLineAngleMeasurement&) = default;
};

using RelationalMeasurement =
    std::variant<
        PointPointMeasurement,
        PointLineMeasurement,
        LineLineAngleMeasurement>;

[[nodiscard]] inline std::optional<ResolvedMeasurePoint>
resolveMeasurePoint(
    const SketchModel& model,
    MeasurePointRef ref) noexcept {
    if (!ref.valid()) {
        return std::nullopt;
    }

    if (const auto* line = model.findLine(ref.entity_id)) {
        Point2 point{};
        switch (ref.role) {
        case MeasurePointRole::line_start:
            point = line->start();
            break;
        case MeasurePointRole::line_midpoint:
            point = {
                (line->start().u + line->end().u) * 0.5,
                (line->start().v + line->end().v) * 0.5};
            break;
        case MeasurePointRole::line_end:
            point = line->end();
            break;
        default:
            return std::nullopt;
        }
        if (!point.finite()) return std::nullopt;
        return ResolvedMeasurePoint{ref, point, line->role()};
    }

    if (const auto* circle = model.findCircle(ref.entity_id)) {
        const auto c = circle->center();
        const auto r = circle->radius();
        Point2 point{};
        switch (ref.role) {
        case MeasurePointRole::circle_center:
            point = c;
            break;
        case MeasurePointRole::circle_quadrant_pos_u:
            point = {c.u + r, c.v};
            break;
        case MeasurePointRole::circle_quadrant_pos_v:
            point = {c.u, c.v + r};
            break;
        case MeasurePointRole::circle_quadrant_neg_u:
            point = {c.u - r, c.v};
            break;
        case MeasurePointRole::circle_quadrant_neg_v:
            point = {c.u, c.v - r};
            break;
        default:
            return std::nullopt;
        }
        if (!point.finite()) return std::nullopt;
        return ResolvedMeasurePoint{ref, point, circle->role()};
    }

    if (const auto* arc = model.findArc(ref.entity_id)) {
        const auto c = arc->center();
        const auto r = arc->radius();
        double angle{};
        switch (ref.role) {
        case MeasurePointRole::arc_center:
            if (!c.finite()) return std::nullopt;
            return ResolvedMeasurePoint{ref, c, arc->role()};
        case MeasurePointRole::arc_start:
            angle = arc->startAngle();
            break;
        case MeasurePointRole::arc_end:
            angle = arc->startAngle() + arc->sweepAngle();
            break;
        case MeasurePointRole::arc_midpoint:
            angle =
                arc->startAngle() +
                arc->sweepAngle() * 0.5;
            break;
        default:
            return std::nullopt;
        }
        const Point2 point{
            c.u + r * std::cos(angle),
            c.v + r * std::sin(angle)};
        if (!point.finite()) return std::nullopt;
        return ResolvedMeasurePoint{ref, point, arc->role()};
    }

    return std::nullopt;
}

[[nodiscard]] inline std::vector<ResolvedMeasurePoint>
measurePointCatalog(
    const SketchModel& model) {
    std::vector<ResolvedMeasurePoint> result;
    const auto state = model.state();
    result.reserve(
        state.lines.size() * 3U +
        state.circles.size() * 5U +
        state.arcs.size() * 4U);

    const auto append = [&model, &result](
                            EntityId id,
                            MeasurePointRole role) {
        if (const auto point =
                resolveMeasurePoint(
                    model,
                    MeasurePointRef{id, role})) {
            result.push_back(*point);
        }
    };

    for (const auto& line : state.lines) {
        append(line.id, MeasurePointRole::line_start);
        append(line.id, MeasurePointRole::line_midpoint);
        append(line.id, MeasurePointRole::line_end);
    }
    for (const auto& circle : state.circles) {
        append(circle.id, MeasurePointRole::circle_center);
        append(circle.id, MeasurePointRole::circle_quadrant_pos_u);
        append(circle.id, MeasurePointRole::circle_quadrant_pos_v);
        append(circle.id, MeasurePointRole::circle_quadrant_neg_u);
        append(circle.id, MeasurePointRole::circle_quadrant_neg_v);
    }
    for (const auto& arc : state.arcs) {
        append(arc.id, MeasurePointRole::arc_center);
        append(arc.id, MeasurePointRole::arc_start);
        append(arc.id, MeasurePointRole::arc_end);
        append(arc.id, MeasurePointRole::arc_midpoint);
    }
    return result;
}

[[nodiscard]] inline std::optional<RelationalMeasurement>
measureRelation(
    const SketchModel& model,
    const MeasureRelationTarget& first,
    const MeasureRelationTarget& second) noexcept {
    const auto* first_point_ref =
        std::get_if<MeasurePointRef>(&first);
    const auto* second_point_ref =
        std::get_if<MeasurePointRef>(&second);
    const auto* first_line_ref =
        std::get_if<MeasureLineRef>(&first);
    const auto* second_line_ref =
        std::get_if<MeasureLineRef>(&second);

    if (first_point_ref && second_point_ref) {
        const auto a =
            resolveMeasurePoint(model, *first_point_ref);
        const auto b =
            resolveMeasurePoint(model, *second_point_ref);
        if (!a || !b) return std::nullopt;

        const double du = b->point.u - a->point.u;
        const double dv = b->point.v - a->point.v;
        const double distance = std::hypot(du, dv);
        const double angle = std::atan2(dv, du);
        if (!std::isfinite(du) ||
            !std::isfinite(dv) ||
            !std::isfinite(distance) ||
            !std::isfinite(angle)) {
            return std::nullopt;
        }
        return RelationalMeasurement{
            PointPointMeasurement{
                *first_point_ref,
                *second_point_ref,
                a->point,
                b->point,
                distance,
                du,
                dv,
                angle}};
    }

    const auto point_line =
        [&model](
            MeasurePointRef point_ref,
            MeasureLineRef line_ref)
            -> std::optional<RelationalMeasurement> {
        const auto point =
            resolveMeasurePoint(model, point_ref);
        const auto* line =
            model.findLine(line_ref.entity_id);
        if (!point || line == nullptr) {
            return std::nullopt;
        }

        const double vu =
            line->end().u - line->start().u;
        const double vv =
            line->end().v - line->start().v;
        const double length_sq =
            vu * vu + vv * vv;
        if (!std::isfinite(length_sq) ||
            length_sq <= 0.0) {
            return std::nullopt;
        }

        const double pu =
            point->point.u - line->start().u;
        const double pv =
            point->point.v - line->start().v;
        const double t =
            (pu * vu + pv * vv) / length_sq;
        const Point2 foot{
            line->start().u + t * vu,
            line->start().v + t * vv};
        const double distance =
            std::hypot(
                point->point.u - foot.u,
                point->point.v - foot.v);
        if (!std::isfinite(t) ||
            !foot.finite() ||
            !std::isfinite(distance)) {
            return std::nullopt;
        }

        return RelationalMeasurement{
            PointLineMeasurement{
                point_ref,
                line_ref,
                point->point,
                line->start(),
                line->end(),
                foot,
                t >= 0.0 && t <= 1.0,
                distance}};
    };

    if (first_point_ref && second_line_ref) {
        return point_line(
            *first_point_ref,
            *second_line_ref);
    }
    if (first_line_ref && second_point_ref) {
        return point_line(
            *second_point_ref,
            *first_line_ref);
    }

    if (first_line_ref && second_line_ref) {
        const auto* a =
            model.findLine(first_line_ref->entity_id);
        const auto* b =
            model.findLine(second_line_ref->entity_id);
        if (a == nullptr || b == nullptr) {
            return std::nullopt;
        }

        const double au = a->end().u - a->start().u;
        const double av = a->end().v - a->start().v;
        const double bu = b->end().u - b->start().u;
        const double bv = b->end().v - b->start().v;
        const double al = std::hypot(au, av);
        const double bl = std::hypot(bu, bv);
        if (!std::isfinite(al) ||
            !std::isfinite(bl) ||
            al <= 0.0 ||
            bl <= 0.0) {
            return std::nullopt;
        }

        double cosine =
            std::abs((au * bu + av * bv) / (al * bl));
        if (!std::isfinite(cosine)) {
            return std::nullopt;
        }
        cosine = std::clamp(cosine, 0.0, 1.0);
        const double angle = std::acos(cosine);
        if (!std::isfinite(angle)) {
            return std::nullopt;
        }
        return RelationalMeasurement{
            LineLineAngleMeasurement{
                *first_line_ref,
                *second_line_ref,
                angle}};
    }

    return std::nullopt;
}

enum class MeasurementEntityKind : std::uint8_t {
    line,
    circle,
    arc,
};

struct LineMeasurement final {
    EntityId entity_id;
    EntityRole role{EntityRole::regular};
    double length{};
    double delta_u{};
    double delta_v{};
    double angle_from_positive_u{};

    friend bool operator==(
        const LineMeasurement&,
        const LineMeasurement&) = default;
};

struct CircleMeasurement final {
    EntityId entity_id;
    EntityRole role{EntityRole::regular};
    double radius{};
    double diameter{};
    double circumference{};
    double area{};

    friend bool operator==(
        const CircleMeasurement&,
        const CircleMeasurement&) = default;
};

struct ArcMeasurement final {
    EntityId entity_id;
    EntityRole role{EntityRole::regular};
    double radius{};
    double start_angle{};
    double end_angle{};
    double signed_sweep_angle{};
    double arc_length{};

    friend bool operator==(
        const ArcMeasurement&,
        const ArcMeasurement&) = default;
};

using EntityMeasurement =
    std::variant<
        LineMeasurement,
        CircleMeasurement,
        ArcMeasurement>;

[[nodiscard]] inline std::optional<EntityMeasurement>
measureEntity(
    const SketchModel& model,
    EntityId entity_id) noexcept {
    if (!entity_id.valid()) {
        return std::nullopt;
    }

    if (const auto* line = model.findLine(entity_id)) {
        const double delta_u =
            line->end().u - line->start().u;
        const double delta_v =
            line->end().v - line->start().v;
        const double length =
            std::hypot(delta_u, delta_v);
        const double angle =
            std::atan2(delta_v, delta_u);

        if (!std::isfinite(delta_u) ||
            !std::isfinite(delta_v) ||
            !std::isfinite(length) ||
            !std::isfinite(angle)) {
            return std::nullopt;
        }

        return EntityMeasurement{
            LineMeasurement{
                entity_id,
                line->role(),
                length,
                delta_u,
                delta_v,
                angle}};
    }

    if (const auto* circle = model.findCircle(entity_id)) {
        const double radius = circle->radius();
        const double diameter = 2.0 * radius;
        const double circumference =
            2.0 * std::numbers::pi_v<double> * radius;
        const double area =
            std::numbers::pi_v<double> * radius * radius;

        if (!std::isfinite(radius) ||
            !std::isfinite(diameter) ||
            !std::isfinite(circumference) ||
            !std::isfinite(area)) {
            return std::nullopt;
        }

        return EntityMeasurement{
            CircleMeasurement{
                entity_id,
                circle->role(),
                radius,
                diameter,
                circumference,
                area}};
    }

    if (const auto* arc = model.findArc(entity_id)) {
        const double radius = arc->radius();
        const double start = arc->startAngle();
        const double sweep = arc->sweepAngle();
        const double end = start + sweep;
        const double length =
            radius * std::abs(sweep);

        if (!std::isfinite(radius) ||
            !std::isfinite(start) ||
            !std::isfinite(sweep) ||
            !std::isfinite(end) ||
            !std::isfinite(length)) {
            return std::nullopt;
        }

        return EntityMeasurement{
            ArcMeasurement{
                entity_id,
                arc->role(),
                radius,
                start,
                end,
                sweep,
                length}};
    }

    return std::nullopt;
}

} // namespace simplesolid2::sketch
