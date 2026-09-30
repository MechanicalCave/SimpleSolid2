#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <optional>
#include <utility>

namespace simplesolid2::sketch {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;

enum class CurveKind {
    line,
    circle,
    arc,
};

struct CurveView final {
    CurveKind kind{CurveKind::line};
    EntityId id;
    Point2 first;
    Point2 second;
    Point2 center;
    double radius{};
    double start_angle{};
    double sweep_angle{};
};

struct RawIntersection final {
    Point2 point;
    double first_parameter{};
    double second_parameter{};
    bool tangent{false};
};

struct RawRelation final {
    CurveRelationStatus status{CurveRelationStatus::invalid};
    std::vector<RawIntersection> intersections;
};

[[nodiscard]] bool finiteValue(double value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool finitePoint(Point2 point) noexcept {
    return point.finite();
}

[[nodiscard]] std::optional<CurveView> curveView(
    const SketchModel& model,
    EntityId id) {
    if (!id.valid()) {
        return std::nullopt;
    }

    if (const auto* line = model.findLine(id)) {
        return CurveView{
            CurveKind::line,
            id,
            line->start(),
            line->end(),
            {},
            0.0,
            0.0,
            0.0};
    }
    if (const auto* circle = model.findCircle(id)) {
        return CurveView{
            CurveKind::circle,
            id,
            {},
            {},
            circle->center(),
            circle->radius(),
            0.0,
            full_turn};
    }
    if (const auto* arc = model.findArc(id)) {
        return CurveView{
            CurveKind::arc,
            id,
            {},
            {},
            arc->center(),
            arc->radius(),
            arc->startAngle(),
            arc->sweepAngle()};
    }
    return std::nullopt;
}

[[nodiscard]] double normalizedAngle(
    double angle) noexcept {
    if (!finiteValue(angle)) {
        return angle;
    }
    double result =
        std::fmod(angle, full_turn);
    if (result < 0.0) {
        result += full_turn;
    }
    if (result == full_turn) {
        result = 0.0;
    }
    return result;
}

[[nodiscard]] std::optional<Point2> circlePoint(
    const CurveView& curve,
    double angle) noexcept {
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    if (!finiteValue(cosine) ||
        !finiteValue(sine)) {
        return std::nullopt;
    }

    const Point2 result{
        std::fma(curve.radius, cosine, curve.center.u),
        std::fma(curve.radius, sine, curve.center.v)};
    return finitePoint(result)
        ? std::optional<Point2>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<Point2> arcEndpoint(
    const CurveView& arc,
    bool end) noexcept {
    return circlePoint(
        arc,
        end
            ? arc.start_angle + arc.sweep_angle
            : arc.start_angle);
}

[[nodiscard]] std::optional<double> lineParameter(
    const CurveView& line,
    Point2 point) noexcept {
    if (point == line.first) {
        return 0.0;
    }
    if (point == line.second) {
        return 1.0;
    }

    const double du =
        line.second.u - line.first.u;
    const double dv =
        line.second.v - line.first.v;
    if (!finiteValue(du) ||
        !finiteValue(dv)) {
        return std::nullopt;
    }

    double result{};
    if (std::abs(du) >= std::abs(dv)) {
        if (du == 0.0) {
            return std::nullopt;
        }
        result = (point.u - line.first.u) / du;
    } else {
        if (dv == 0.0) {
            return std::nullopt;
        }
        result = (point.v - line.first.v) / dv;
    }

    return finiteValue(result)
        ? std::optional<double>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<double> circleParameter(
    const CurveView& circle,
    Point2 point) noexcept {
    const double du = point.u - circle.center.u;
    const double dv = point.v - circle.center.v;
    if (!finiteValue(du) ||
        !finiteValue(dv)) {
        return std::nullopt;
    }

    const double angle =
        normalizedAngle(std::atan2(dv, du));
    if (!finiteValue(angle)) {
        return std::nullopt;
    }
    const double parameter = angle / full_turn;
    return finiteValue(parameter)
        ? std::optional<double>{parameter}
        : std::nullopt;
}

[[nodiscard]] std::optional<double> arcParameter(
    const CurveView& arc,
    Point2 point) noexcept {
    const auto start_point =
        arcEndpoint(arc, false);
    const auto end_point =
        arcEndpoint(arc, true);
    if (!start_point || !end_point) {
        return std::nullopt;
    }
    if (point == *start_point) {
        return 0.0;
    }
    if (point == *end_point) {
        return 1.0;
    }

    const double du = point.u - arc.center.u;
    const double dv = point.v - arc.center.v;
    if (!finiteValue(du) ||
        !finiteValue(dv)) {
        return std::nullopt;
    }

    const double angle =
        normalizedAngle(std::atan2(dv, du));
    const double start =
        normalizedAngle(arc.start_angle);
    if (!finiteValue(angle) ||
        !finiteValue(start)) {
        return std::nullopt;
    }

    if (arc.sweep_angle > 0.0) {
        const double delta =
            normalizedAngle(angle - start);
        if (!finiteValue(delta) ||
            delta > arc.sweep_angle) {
            return std::nullopt;
        }
        return delta / arc.sweep_angle;
    }

    const double delta =
        normalizedAngle(start - angle);
    const double magnitude =
        -arc.sweep_angle;
    if (!finiteValue(delta) ||
        delta > magnitude) {
        return std::nullopt;
    }
    return delta / magnitude;
}

[[nodiscard]] std::optional<double> curveParameter(
    const CurveView& curve,
    Point2 point) noexcept {
    switch (curve.kind) {
    case CurveKind::line:
        return lineParameter(curve, point);
    case CurveKind::circle:
        return circleParameter(curve, point);
    case CurveKind::arc:
        return arcParameter(curve, point);
    }
    return std::nullopt;
}

[[nodiscard]] bool endpointParameter(
    const CurveView& curve,
    double parameter) noexcept {
    if (curve.kind == CurveKind::circle) {
        return false;
    }
    return parameter == 0.0 ||
           parameter == 1.0;
}

[[nodiscard]] std::optional<Point2> linePoint(
    const CurveView& line,
    double parameter) noexcept {
    if (!finiteValue(parameter)) {
        return std::nullopt;
    }
    if (parameter == 0.0) {
        return line.first;
    }
    if (parameter == 1.0) {
        return line.second;
    }

    const double du =
        line.second.u - line.first.u;
    const double dv =
        line.second.v - line.first.v;
    if (!finiteValue(du) ||
        !finiteValue(dv)) {
        return std::nullopt;
    }

    const Point2 point{
        std::fma(parameter, du, line.first.u),
        std::fma(parameter, dv, line.first.v)};
    return finitePoint(point)
        ? std::optional<Point2>{point}
        : std::nullopt;
}

[[nodiscard]] double cross(
    double au,
    double av,
    double bu,
    double bv) noexcept {
    // Compensated difference of products. Besides reducing cancellation
    // error, this preserves exact algebraic zero for bit-identical parallel
    // vectors (for example cross(v, v)) without introducing a tolerance.
    const double second_product = av * bu;
    const double second_error =
        std::fma(-av, bu, second_product);
    const double difference =
        std::fma(au, bv, -second_product);
    return difference + second_error;
}

[[nodiscard]] RawRelation lineLine(
    const CurveView& first,
    const CurveView& second) {
    const double ru =
        first.second.u - first.first.u;
    const double rv =
        first.second.v - first.first.v;
    const double su =
        second.second.u - second.first.u;
    const double sv =
        second.second.v - second.first.v;
    const double qu =
        second.first.u - first.first.u;
    const double qv =
        second.first.v - first.first.v;

    if (!finiteValue(ru) || !finiteValue(rv) ||
        !finiteValue(su) || !finiteValue(sv) ||
        !finiteValue(qu) || !finiteValue(qv)) {
        return {};
    }

    const double scale = std::max(
        {std::abs(ru), std::abs(rv),
         std::abs(su), std::abs(sv),
         std::abs(qu), std::abs(qv)});
    if (!finiteValue(scale) || scale == 0.0) {
        return {};
    }

    const double rn_u = ru / scale;
    const double rn_v = rv / scale;
    const double sn_u = su / scale;
    const double sn_v = sv / scale;
    const double qn_u = qu / scale;
    const double qn_v = qv / scale;

    const double denominator =
        cross(rn_u, rn_v, sn_u, sn_v);
    const double collinearity =
        cross(qn_u, qn_v, rn_u, rn_v);

    if (!finiteValue(denominator) ||
        !finiteValue(collinearity)) {
        return {};
    }

    if (denominator != 0.0) {
        const auto exact_shared_endpoint =
            [&]() -> std::optional<RawIntersection> {
            if (first.first == second.first) {
                return RawIntersection{
                    first.first, 0.0, 0.0, false};
            }
            if (first.first == second.second) {
                return RawIntersection{
                    first.first, 0.0, 1.0, false};
            }
            if (first.second == second.first) {
                return RawIntersection{
                    first.second, 1.0, 0.0, false};
            }
            if (first.second == second.second) {
                return RawIntersection{
                    first.second, 1.0, 1.0, false};
            }
            return std::nullopt;
        }();

        if (exact_shared_endpoint) {
            return RawRelation{
                CurveRelationStatus::discrete,
                {*exact_shared_endpoint}};
        }
    }

    if (denominator == 0.0) {
        if (collinearity != 0.0) {
            return RawRelation{
                CurveRelationStatus::disjoint,
                {}};
        }

        const bool use_u =
            std::abs(rn_u) >= std::abs(rn_v);
        const double divisor =
            use_u ? rn_u : rn_v;
        if (divisor == 0.0) {
            return {};
        }

        const double t0 =
            (use_u ? qn_u : qn_v) / divisor;
        const double t1 =
            t0 +
            (use_u ? sn_u : sn_v) / divisor;
        if (!finiteValue(t0) ||
            !finiteValue(t1)) {
            return {};
        }

        const double low =
            std::max(0.0, std::min(t0, t1));
        const double high =
            std::min(1.0, std::max(t0, t1));

        if (high < low) {
            return RawRelation{
                CurveRelationStatus::disjoint,
                {}};
        }
        if (high > low) {
            return RawRelation{
                CurveRelationStatus::overlap,
                {}};
        }

        const auto point =
            linePoint(first, low);
        if (!point) {
            return {};
        }
        const auto second_parameter =
            lineParameter(second, *point);
        if (!second_parameter ||
            *second_parameter < 0.0 ||
            *second_parameter > 1.0) {
            return {};
        }

        return RawRelation{
            CurveRelationStatus::discrete,
            {
                RawIntersection{
                    *point,
                    low,
                    *second_parameter,
                    false},
            }};
    }

    const double first_parameter =
        cross(qn_u, qn_v, sn_u, sn_v) /
        denominator;
    const double second_parameter =
        cross(qn_u, qn_v, rn_u, rn_v) /
        denominator;
    if (!finiteValue(first_parameter) ||
        !finiteValue(second_parameter)) {
        return {};
    }

    if (first_parameter < 0.0 ||
        first_parameter > 1.0 ||
        second_parameter < 0.0 ||
        second_parameter > 1.0) {
        return RawRelation{
            CurveRelationStatus::disjoint,
            {}};
    }

    const auto point =
        linePoint(first, first_parameter);
    if (!point) {
        return {};
    }

    return RawRelation{
        CurveRelationStatus::discrete,
        {
            RawIntersection{
                *point,
                first_parameter,
                second_parameter,
                false},
        }};
}

[[nodiscard]] RawRelation lineCircle(
    const CurveView& line,
    const CurveView& circle) {
    const double du =
        line.second.u - line.first.u;
    const double dv =
        line.second.v - line.first.v;
    const double rel_u =
        circle.center.u - line.first.u;
    const double rel_v =
        circle.center.v - line.first.v;

    if (!finiteValue(du) || !finiteValue(dv) ||
        !finiteValue(rel_u) || !finiteValue(rel_v) ||
        !finiteValue(circle.radius) ||
        circle.radius <= 0.0) {
        return {};
    }

    const double length =
        std::hypot(du, dv);
    if (!finiteValue(length) ||
        length == 0.0) {
        return {};
    }

    const double unit_u = du / length;
    const double unit_v = dv / length;
    const double projection =
        std::fma(rel_u, unit_u, rel_v * unit_v);
    const double signed_perpendicular =
        cross(rel_u, rel_v, unit_u, unit_v);

    if (!finiteValue(projection) ||
        !finiteValue(signed_perpendicular)) {
        return {};
    }

    const double perpendicular =
        std::abs(signed_perpendicular);
    if (perpendicular > circle.radius) {
        return RawRelation{
            CurveRelationStatus::disjoint,
            {}};
    }

    const bool tangent =
        perpendicular == circle.radius;
    double offset = 0.0;
    if (!tangent) {
        const double ratio =
            perpendicular / circle.radius;
        const double remainder =
            std::fma(-ratio, ratio, 1.0);
        if (!finiteValue(remainder) ||
            remainder < 0.0) {
            return {};
        }
        offset =
            circle.radius * std::sqrt(remainder);
        if (!finiteValue(offset)) {
            return {};
        }
    }

    const double base_parameter =
        projection / length;
    const double delta_parameter =
        offset / length;
    if (!finiteValue(base_parameter) ||
        !finiteValue(delta_parameter)) {
        return {};
    }

    std::vector<RawIntersection> intersections;
    const auto append =
        [&](double parameter) {
            if (parameter < 0.0 ||
                parameter > 1.0) {
                return true;
            }
            const auto point =
                linePoint(line, parameter);
            if (!point) {
                return false;
            }
            const auto circle_parameter =
                circleParameter(circle, *point);
            if (!circle_parameter) {
                return false;
            }
            intersections.push_back(
                RawIntersection{
                    *point,
                    parameter,
                    *circle_parameter,
                    tangent});
            return true;
        };

    if (!append(base_parameter - delta_parameter)) {
        return {};
    }
    if (!tangent &&
        !append(base_parameter + delta_parameter)) {
        return {};
    }

    if (intersections.empty()) {
        return RawRelation{
            CurveRelationStatus::disjoint,
            {}};
    }

    return RawRelation{
        CurveRelationStatus::discrete,
        std::move(intersections)};
}

[[nodiscard]] bool sameSupportCircle(
    const CurveView& first,
    const CurveView& second) noexcept {
    return first.center == second.center &&
           first.radius == second.radius;
}

[[nodiscard]] RawRelation circleCircle(
    const CurveView& first,
    const CurveView& second) {
    if (sameSupportCircle(first, second)) {
        return RawRelation{
            CurveRelationStatus::overlap,
            {}};
    }

    const double dx =
        second.center.u - first.center.u;
    const double dy =
        second.center.v - first.center.v;
    if (!finiteValue(dx) ||
        !finiteValue(dy)) {
        return {};
    }

    const double distance =
        std::hypot(dx, dy);
    if (!finiteValue(distance)) {
        return {};
    }
    if (distance == 0.0) {
        return RawRelation{
            CurveRelationStatus::disjoint,
            {}};
    }

    const double scale =
        std::max(
            {distance,
             first.radius,
             second.radius});
    if (!finiteValue(scale) ||
        scale <= 0.0) {
        return {};
    }

    const double d = distance / scale;
    const double r1 = first.radius / scale;
    const double r2 = second.radius / scale;
    const double sum = r1 + r2;
    const double difference =
        std::abs(r1 - r2);

    if (d > sum || d < difference) {
        return RawRelation{
            CurveRelationStatus::disjoint,
            {}};
    }

    const bool tangent =
        d == sum ||
        d == difference;
    const double denominator =
        2.0 * d;
    if (denominator == 0.0) {
        return {};
    }

    const double along_normalized =
        (r1 * r1 - r2 * r2 + d * d) /
        denominator;
    if (!finiteValue(along_normalized)) {
        return {};
    }

    double height_normalized = 0.0;
    if (!tangent) {
        const double height_squared =
            std::fma(
                -along_normalized,
                along_normalized,
                r1 * r1);
        if (!finiteValue(height_squared) ||
            height_squared < 0.0) {
            return {};
        }
        height_normalized =
            std::sqrt(height_squared);
    }

    const double unit_u = dx / distance;
    const double unit_v = dy / distance;
    const double along =
        along_normalized * scale;
    const double height =
        height_normalized * scale;
    if (!finiteValue(unit_u) ||
        !finiteValue(unit_v) ||
        !finiteValue(along) ||
        !finiteValue(height)) {
        return {};
    }

    const Point2 base{
        std::fma(along, unit_u, first.center.u),
        std::fma(along, unit_v, first.center.v)};
    if (!finitePoint(base)) {
        return {};
    }

    std::vector<RawIntersection> intersections;
    const auto append =
        [&](double sign) {
            const Point2 point{
                std::fma(
                    sign * height,
                    -unit_v,
                    base.u),
                std::fma(
                    sign * height,
                    unit_u,
                    base.v)};
            if (!finitePoint(point)) {
                return false;
            }

            const auto first_parameter =
                circleParameter(first, point);
            const auto second_parameter =
                circleParameter(second, point);
            if (!first_parameter ||
                !second_parameter) {
                return false;
            }

            intersections.push_back(
                RawIntersection{
                    point,
                    *first_parameter,
                    *second_parameter,
                    tangent});
            return true;
        };

    if (!append(1.0)) {
        return {};
    }
    if (!tangent && !append(-1.0)) {
        return {};
    }

    return RawRelation{
        CurveRelationStatus::discrete,
        std::move(intersections)};
}

[[nodiscard]] bool arcContainsAngleStrict(
    const CurveView& arc,
    double angle) noexcept {
    const double start =
        normalizedAngle(arc.start_angle);
    const double value =
        normalizedAngle(angle);
    if (!finiteValue(start) ||
        !finiteValue(value)) {
        return false;
    }

    if (arc.sweep_angle > 0.0) {
        const double delta =
            normalizedAngle(value - start);
        return delta > 0.0 &&
               delta < arc.sweep_angle;
    }

    const double delta =
        normalizedAngle(start - value);
    return delta > 0.0 &&
           delta < -arc.sweep_angle;
}

[[nodiscard]] bool sameSupportArcsOverlap(
    const CurveView& first,
    const CurveView& second) noexcept {
    const double first_end =
        first.start_angle +
        first.sweep_angle;
    const double second_end =
        second.start_angle +
        second.sweep_angle;

    // Positive-length overlap exists when an endpoint of either finite arc
    // lies strictly inside the other. The midpoint fallback also covers
    // identical arcs, whose endpoints are shared boundaries rather than
    // strict interior points.
    if (arcContainsAngleStrict(
            second,
            first.start_angle) ||
        arcContainsAngleStrict(
            second,
            first_end) ||
        arcContainsAngleStrict(
            first,
            second.start_angle) ||
        arcContainsAngleStrict(
            first,
            second_end)) {
        return true;
    }

    const double first_mid =
        first.start_angle +
        first.sweep_angle * 0.5;
    const double second_mid =
        second.start_angle +
        second.sweep_angle * 0.5;

    return arcContainsAngleStrict(
               second,
               first_mid) ||
           arcContainsAngleStrict(
               first,
               second_mid);
}

[[nodiscard]] RawRelation sameSupportArcArc(
    const CurveView& first,
    const CurveView& second) {
    if (sameSupportArcsOverlap(first, second)) {
        return RawRelation{
            CurveRelationStatus::overlap,
            {}};
    }

    const double first_start =
        normalizedAngle(first.start_angle);
    const double first_end =
        normalizedAngle(
            first.start_angle +
            first.sweep_angle);
    const double second_start =
        normalizedAngle(second.start_angle);
    const double second_end =
        normalizedAngle(
            second.start_angle +
            second.sweep_angle);
    if (!finiteValue(first_start) ||
        !finiteValue(first_end) ||
        !finiteValue(second_start) ||
        !finiteValue(second_end)) {
        return {};
    }

    std::vector<double> shared_angles;
    const auto add_shared =
        [&](double first_angle,
            double second_angle) {
            if (first_angle == second_angle &&
                std::find(
                    shared_angles.begin(),
                    shared_angles.end(),
                    first_angle) ==
                    shared_angles.end()) {
                shared_angles.push_back(first_angle);
            }
        };

    add_shared(first_start, second_start);
    add_shared(first_start, second_end);
    add_shared(first_end, second_start);
    add_shared(first_end, second_end);

    if (shared_angles.empty()) {
        return RawRelation{
            CurveRelationStatus::disjoint,
            {}};
    }

    std::vector<RawIntersection> intersections;
    for (const double angle : shared_angles) {
        const auto point =
            circlePoint(first, angle);
        if (!point) {
            return {};
        }
        const auto first_parameter =
            arcParameter(first, *point);
        const auto second_parameter =
            arcParameter(second, *point);
        if (!first_parameter ||
            !second_parameter) {
            return {};
        }
        intersections.push_back(
            RawIntersection{
                *point,
                *first_parameter,
                *second_parameter,
                false});
    }

    return RawRelation{
        CurveRelationStatus::discrete,
        std::move(intersections)};
}

[[nodiscard]] std::vector<RawIntersection>
exactLineArcEndpointContacts(
    const CurveView& line,
    const CurveView& arc) {
    std::vector<RawIntersection> intersections;
    const auto arc_start = arcEndpoint(arc, false);
    const auto arc_end = arcEndpoint(arc, true);
    if (!arc_start || !arc_end) {
        return intersections;
    }

    const auto append =
        [&intersections](
            Point2 line_point,
            double line_parameter,
            Point2 arc_point,
            double arc_parameter) {
            if (line_point == arc_point) {
                intersections.push_back(
                    RawIntersection{
                        line_point,
                        line_parameter,
                        arc_parameter,
                        false});
            }
        };

    append(line.first, 0.0, *arc_start, 0.0);
    append(line.first, 0.0, *arc_end, 1.0);
    append(line.second, 1.0, *arc_start, 0.0);
    append(line.second, 1.0, *arc_end, 1.0);
    return intersections;
}

[[nodiscard]] bool sameNumericalEndpointRoot(
    const RawIntersection& computed,
    const RawIntersection& exact) noexcept {
    // This bound is dimensionless and is used only after exact evaluated
    // endpoint equality has already proven the semantic contact. It does not
    // create or heal a geometric contact; it only removes a second numerical
    // image of that already-known quadratic root.
    constexpr double parameter_bound =
        256.0 *
        std::numeric_limits<double>::epsilon();
    return std::abs(
               computed.first_parameter -
               exact.first_parameter) <=
               parameter_bound &&
           std::abs(
               computed.second_parameter -
               exact.second_parameter) <=
               parameter_bound;
}

[[nodiscard]] RawRelation mergeExactLineArcEndpoints(
    RawRelation relation,
    const CurveView& line,
    const CurveView& arc) {
    const auto exact =
        exactLineArcEndpointContacts(
            line,
            arc);
    if (exact.empty()) {
        return relation;
    }

    if (relation.status ==
        CurveRelationStatus::invalid) {
        return relation;
    }

    if (relation.status ==
        CurveRelationStatus::disjoint) {
        return RawRelation{
            CurveRelationStatus::discrete,
            exact};
    }

    if (relation.status !=
        CurveRelationStatus::discrete) {
        return relation;
    }

    for (const auto& endpoint : exact) {
        relation.intersections.erase(
            std::remove_if(
                relation.intersections.begin(),
                relation.intersections.end(),
                [&endpoint](const RawIntersection& item) {
                    return sameNumericalEndpointRoot(
                        item,
                        endpoint);
                }),
            relation.intersections.end());
        relation.intersections.push_back(
            endpoint);
    }

    return relation;
}

[[nodiscard]] RawRelation filterSecondArc(
    RawRelation relation,
    const CurveView& arc) {
    if (relation.status !=
        CurveRelationStatus::discrete) {
        return relation;
    }

    std::vector<RawIntersection> filtered;
    for (auto item : relation.intersections) {
        const auto parameter =
            arcParameter(arc, item.point);
        if (!parameter) {
            continue;
        }
        item.second_parameter = *parameter;
        filtered.push_back(item);
    }

    return RawRelation{
        filtered.empty()
            ? CurveRelationStatus::disjoint
            : CurveRelationStatus::discrete,
        std::move(filtered)};
}

[[nodiscard]] RawRelation filterBothArcs(
    RawRelation relation,
    const CurveView& first,
    const CurveView& second) {
    if (relation.status !=
        CurveRelationStatus::discrete) {
        return relation;
    }

    std::vector<RawIntersection> filtered;
    for (auto item : relation.intersections) {
        const auto first_parameter =
            arcParameter(first, item.point);
        const auto second_parameter =
            arcParameter(second, item.point);
        if (!first_parameter ||
            !second_parameter) {
            continue;
        }
        item.first_parameter = *first_parameter;
        item.second_parameter = *second_parameter;
        filtered.push_back(item);
    }

    return RawRelation{
        filtered.empty()
            ? CurveRelationStatus::disjoint
            : CurveRelationStatus::discrete,
        std::move(filtered)};
}

[[nodiscard]] RawRelation analyzeOrdered(
    const CurveView& first,
    const CurveView& second) {
    if (first.kind == CurveKind::line &&
        second.kind == CurveKind::line) {
        return lineLine(first, second);
    }

    if (first.kind == CurveKind::line &&
        second.kind == CurveKind::circle) {
        return lineCircle(first, second);
    }

    if (first.kind == CurveKind::circle &&
        second.kind == CurveKind::line) {
        auto result =
            lineCircle(second, first);
        for (auto& item : result.intersections) {
            std::swap(
                item.first_parameter,
                item.second_parameter);
        }
        return result;
    }

    if (first.kind == CurveKind::circle &&
        second.kind == CurveKind::circle) {
        return circleCircle(first, second);
    }

    if (first.kind == CurveKind::line &&
        second.kind == CurveKind::arc) {
        CurveView support = second;
        support.kind = CurveKind::circle;
        auto support_relation =
            lineCircle(first, support);
        if (support_relation.status ==
            CurveRelationStatus::discrete) {
            support_relation =
                filterSecondArc(
                    std::move(support_relation),
                    second);
        }
        return mergeExactLineArcEndpoints(
            std::move(support_relation),
            first,
            second);
    }

    if (first.kind == CurveKind::arc &&
        second.kind == CurveKind::line) {
        auto result =
            analyzeOrdered(second, first);
        for (auto& item : result.intersections) {
            std::swap(
                item.first_parameter,
                item.second_parameter);
        }
        return result;
    }

    if (first.kind == CurveKind::circle &&
        second.kind == CurveKind::arc) {
        if (sameSupportCircle(first, second)) {
            return RawRelation{
                CurveRelationStatus::overlap,
                {}};
        }
        CurveView second_support = second;
        second_support.kind = CurveKind::circle;
        return filterSecondArc(
            circleCircle(first, second_support),
            second);
    }

    if (first.kind == CurveKind::arc &&
        second.kind == CurveKind::circle) {
        auto result =
            analyzeOrdered(second, first);
        for (auto& item : result.intersections) {
            std::swap(
                item.first_parameter,
                item.second_parameter);
        }
        return result;
    }

    if (first.kind == CurveKind::arc &&
        second.kind == CurveKind::arc) {
        if (sameSupportCircle(first, second)) {
            return sameSupportArcArc(first, second);
        }
        CurveView first_support = first;
        CurveView second_support = second;
        first_support.kind = CurveKind::circle;
        second_support.kind = CurveKind::circle;
        return filterBothArcs(
            circleCircle(
                first_support,
                second_support),
            first,
            second);
    }

    return {};
}

} // namespace

CurveRelation2D analyzeCurveRelation(
    const SketchModel& model,
    EntityId first,
    EntityId second) {
    CurveRelation2D result;

    if (!first.valid() ||
        !second.valid() ||
        first == second) {
        return result;
    }

    if (second < first) {
        std::swap(first, second);
    }

    result.first_entity = first;
    result.second_entity = second;

    const auto first_curve =
        curveView(model, first);
    const auto second_curve =
        curveView(model, second);
    if (!first_curve || !second_curve) {
        return result;
    }

    auto raw =
        analyzeOrdered(
            *first_curve,
            *second_curve);
    result.status = raw.status;
    if (raw.status !=
        CurveRelationStatus::discrete) {
        return result;
    }

    std::sort(
        raw.intersections.begin(),
        raw.intersections.end(),
        [](const RawIntersection& lhs,
           const RawIntersection& rhs) {
            if (lhs.first_parameter !=
                rhs.first_parameter) {
                return lhs.first_parameter <
                       rhs.first_parameter;
            }
            if (lhs.second_parameter !=
                rhs.second_parameter) {
                return lhs.second_parameter <
                       rhs.second_parameter;
            }
            if (lhs.point.u != rhs.point.u) {
                return lhs.point.u < rhs.point.u;
            }
            return lhs.point.v < rhs.point.v;
        });

    raw.intersections.erase(
        std::unique(
            raw.intersections.begin(),
            raw.intersections.end(),
            [](const RawIntersection& lhs,
               const RawIntersection& rhs) {
                return lhs.point == rhs.point &&
                       lhs.first_parameter ==
                           rhs.first_parameter &&
                       lhs.second_parameter ==
                           rhs.second_parameter;
            }),
        raw.intersections.end());

    if (raw.intersections.empty()) {
        result.status =
            CurveRelationStatus::disjoint;
        return result;
    }

    result.intersections.reserve(
        raw.intersections.size());
    for (std::size_t index = 0;
         index < raw.intersections.size();
         ++index) {
        const auto& item =
            raw.intersections[index];
        const bool first_endpoint =
            endpointParameter(
                *first_curve,
                item.first_parameter);
        const bool second_endpoint =
            endpointParameter(
                *second_curve,
                item.second_parameter);

        CurveContactKind contact =
            CurveContactKind::proper_crossing;
        if (item.tangent) {
            contact =
                CurveContactKind::tangent;
        } else if (first_endpoint ||
                   second_endpoint) {
            contact =
                CurveContactKind::
                    endpoint_intersection;
        }

        result.intersections.push_back(
            CurveIntersection2D{
                item.point,
                item.first_parameter,
                item.second_parameter,
                static_cast<std::uint32_t>(
                    index),
                first_endpoint,
                second_endpoint,
                contact});
    }

    return result;
}



namespace {

struct DisjointSet final {
    explicit DisjointSet(std::size_t size)
        : parent(size),
          rank(size, 0U) {
        for (std::size_t i = 0; i < size; ++i) {
            parent[i] = i;
        }
    }

    [[nodiscard]] std::size_t find(
        std::size_t value) {
        if (parent[value] != value) {
            parent[value] = find(parent[value]);
        }
        return parent[value];
    }

    void unite(
        std::size_t first,
        std::size_t second) {
        first = find(first);
        second = find(second);
        if (first == second) {
            return;
        }
        if (rank[first] < rank[second]) {
            std::swap(first, second);
        }
        parent[second] = first;
        if (rank[first] == rank[second]) {
            ++rank[first];
        }
    }

    std::vector<std::size_t> parent;
    std::vector<unsigned int> rank;
};

struct TopologyIntersection final {
    EntityId first;
    EntityId second;
    Point2 point;
    double first_parameter{};
    double second_parameter{};
    std::uint32_t canonical_branch{};
    bool first_endpoint{false};
    bool second_endpoint{false};
};

struct Vertex final {
    Point2 point;
    std::vector<std::size_t> outgoing;
};

struct Cut final {
    double parameter{};
    std::size_t vertex{};
    std::optional<RegionBoundaryAnchor2D> anchor;
};

struct DerivedEdge final {
    EntityId source;
    CurveKind kind{CurveKind::line};
    std::size_t first_vertex{};
    std::size_t second_vertex{};
    double first_parameter{};
    double second_parameter{};
    std::optional<RegionBoundaryAnchor2D> first_anchor;
    std::optional<RegionBoundaryAnchor2D> second_anchor;
    bool crosses_closed_seam{false};
};

struct HalfEdge final {
    std::size_t edge{};
    bool forward{true};
    std::size_t origin{};
    std::size_t destination{};
    double tangent_angle{};
};

struct BuiltLoop final {
    RegionLoop2D loop;
    Point2 interior_point;
};

[[nodiscard]] std::optional<std::size_t> entityIndex(
    const std::vector<EntityId>& ids,
    EntityId id) {
    const auto found =
        std::lower_bound(
            ids.begin(),
            ids.end(),
            id);
    if (found == ids.end() ||
        *found != id) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(
        found - ids.begin());
}

[[nodiscard]] std::vector<EntityId> regularEntities(
    const SketchModel& model) {
    std::vector<EntityId> result;
    const auto state = model.state();
    result.reserve(
        state.lines.size() +
        state.circles.size() +
        state.arcs.size());

    for (const auto& line : state.lines) {
        if (line.role == EntityRole::regular) {
            result.push_back(line.id);
        }
    }
    for (const auto& circle : state.circles) {
        if (circle.role == EntityRole::regular) {
            result.push_back(circle.id);
        }
    }
    for (const auto& arc : state.arcs) {
        if (arc.role == EntityRole::regular) {
            result.push_back(arc.id);
        }
    }

    std::sort(result.begin(), result.end());
    return result;
}

[[nodiscard]] std::size_t findOrAddVertex(
    std::vector<Vertex>& vertices,
    Point2 point) {
    const auto found =
        std::find_if(
            vertices.begin(),
            vertices.end(),
            [point](const Vertex& vertex) {
                return vertex.point == point;
            });
    if (found != vertices.end()) {
        return static_cast<std::size_t>(
            found - vertices.begin());
    }

    vertices.push_back(
        Vertex{point, {}});
    return vertices.size() - 1U;
}

[[nodiscard]] std::optional<Point2> pointAtParameter(
    const CurveView& curve,
    double parameter) noexcept {
    if (!finiteValue(parameter)) {
        return std::nullopt;
    }

    switch (curve.kind) {
    case CurveKind::line:
        return linePoint(curve, parameter);
    case CurveKind::circle:
        return circlePoint(
            curve,
            full_turn * parameter);
    case CurveKind::arc:
        return circlePoint(
            curve,
            curve.start_angle +
                curve.sweep_angle * parameter);
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<Point2> sourceDerivative(
    const CurveView& curve,
    double parameter) noexcept {
    if (!finiteValue(parameter)) {
        return std::nullopt;
    }

    if (curve.kind == CurveKind::line) {
        const Point2 result{
            curve.second.u - curve.first.u,
            curve.second.v - curve.first.v};
        return result.finite()
            ? std::optional<Point2>{result}
            : std::nullopt;
    }

    const double angle =
        curve.kind == CurveKind::circle
            ? full_turn * parameter
            : curve.start_angle +
                  curve.sweep_angle * parameter;
    const double angular_scale =
        curve.kind == CurveKind::circle
            ? full_turn
            : curve.sweep_angle;
    const Point2 result{
        -curve.radius *
            std::sin(angle) *
            angular_scale,
        curve.radius *
            std::cos(angle) *
            angular_scale};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

[[nodiscard]] double sourceParameterDelta(
    const DerivedEdge& edge) noexcept {
    if (edge.kind == CurveKind::circle &&
        edge.crosses_closed_seam) {
        return (1.0 - edge.first_parameter) +
               edge.second_parameter;
    }
    return edge.second_parameter -
           edge.first_parameter;
}

[[nodiscard]] std::optional<double> halfEdgeAngle(
    const CurveView& curve,
    const DerivedEdge& edge,
    bool forward) noexcept {
    const double parameter =
        forward
            ? edge.first_parameter
            : edge.second_parameter;
    const auto derivative =
        sourceDerivative(curve, parameter);
    if (!derivative) {
        return std::nullopt;
    }

    const double du =
        forward ? derivative->u : -derivative->u;
    const double dv =
        forward ? derivative->v : -derivative->v;
    if (!finiteValue(du) ||
        !finiteValue(dv) ||
        (du == 0.0 && dv == 0.0)) {
        return std::nullopt;
    }

    const double angle =
        std::atan2(dv, du);
    return finiteValue(angle)
        ? std::optional<double>{angle}
        : std::nullopt;
}

[[nodiscard]] RegionBoundaryUse2D boundaryUse(
    const DerivedEdge& edge,
    bool forward) {
    return RegionBoundaryUse2D{
        edge.source,
        forward
            ? edge.first_parameter
            : edge.second_parameter,
        forward
            ? edge.second_parameter
            : edge.first_parameter,
        forward
            ? edge.first_anchor
            : edge.second_anchor,
        forward
            ? edge.second_anchor
            : edge.first_anchor,
        forward,
        edge.crosses_closed_seam,
        false};
}

[[nodiscard]] std::optional<double> circularDeltaAngle(
    const CurveView& curve,
    const RegionBoundaryUse2D& use) noexcept {
    if (use.whole_closed_curve) {
        return use.follows_source_direction
            ? full_turn
            : -full_turn;
    }

    double source_delta{};
    if (curve.kind == CurveKind::circle) {
        const double from =
            use.follows_source_direction
                ? use.start_parameter
                : use.end_parameter;
        const double to =
            use.follows_source_direction
                ? use.end_parameter
                : use.start_parameter;
        source_delta =
            use.crosses_closed_seam
                ? (1.0 - from) + to
                : to - from;
        source_delta *= full_turn;
    } else if (curve.kind == CurveKind::arc) {
        const double from =
            use.follows_source_direction
                ? use.start_parameter
                : use.end_parameter;
        const double to =
            use.follows_source_direction
                ? use.end_parameter
                : use.start_parameter;
        source_delta =
            curve.sweep_angle *
            (to - from);
    } else {
        return std::nullopt;
    }

    if (!use.follows_source_direction) {
        source_delta = -source_delta;
    }
    return finiteValue(source_delta)
        ? std::optional<double>{source_delta}
        : std::nullopt;
}

[[nodiscard]] std::optional<Point2> useStartPoint(
    const CurveView& curve,
    const RegionBoundaryUse2D& use) noexcept {
    return pointAtParameter(
        curve,
        use.start_parameter);
}

[[nodiscard]] std::optional<Point2> useEndPoint(
    const CurveView& curve,
    const RegionBoundaryUse2D& use) noexcept {
    return pointAtParameter(
        curve,
        use.end_parameter);
}

[[nodiscard]] std::optional<double> useAreaContribution(
    const CurveView& curve,
    const RegionBoundaryUse2D& use) noexcept {
    if (curve.kind == CurveKind::line) {
        const auto first =
            useStartPoint(curve, use);
        const auto second =
            useEndPoint(curve, use);
        if (!first || !second) {
            return std::nullopt;
        }
        const double value =
            0.5 *
            std::fma(
                first->u,
                second->v,
                -second->u * first->v);
        return finiteValue(value)
            ? std::optional<double>{value}
            : std::nullopt;
    }

    const auto delta =
        circularDeltaAngle(curve, use);
    if (!delta) {
        return std::nullopt;
    }

    const double start_angle =
        curve.kind == CurveKind::circle
            ? full_turn * use.start_parameter
            : curve.start_angle +
                  curve.sweep_angle *
                      use.start_parameter;
    const double end_angle =
        start_angle + *delta;
    const double value =
        0.5 *
        (curve.radius * curve.center.u *
             (std::sin(end_angle) -
              std::sin(start_angle)) -
         curve.radius * curve.center.v *
             (std::cos(end_angle) -
              std::cos(start_angle)) +
         curve.radius * curve.radius *
             *delta);
    return finiteValue(value)
        ? std::optional<double>{value}
        : std::nullopt;
}

[[nodiscard]] std::optional<double> usePerimeter(
    const CurveView& curve,
    const RegionBoundaryUse2D& use) noexcept {
    if (curve.kind == CurveKind::line) {
        const auto first =
            useStartPoint(curve, use);
        const auto second =
            useEndPoint(curve, use);
        if (!first || !second) {
            return std::nullopt;
        }
        const double value =
            std::hypot(
                second->u - first->u,
                second->v - first->v);
        return finiteValue(value)
            ? std::optional<double>{value}
            : std::nullopt;
    }

    const auto delta =
        circularDeltaAngle(curve, use);
    if (!delta) {
        return std::nullopt;
    }
    const double value =
        curve.radius * std::abs(*delta);
    return finiteValue(value)
        ? std::optional<double>{value}
        : std::nullopt;
}

[[nodiscard]] std::optional<RegionLoop2D> makeLoop(
    const SketchModel& model,
    const std::vector<DerivedEdge>& edges,
    const std::vector<HalfEdge>& half_edges,
    const std::vector<std::size_t>& cycle) {
    RegionLoop2D result;
    result.boundary.reserve(cycle.size());

    double area = 0.0;
    double perimeter = 0.0;
    for (const auto half_index : cycle) {
        const auto& half =
            half_edges[half_index];
        const auto& edge =
            edges[half.edge];
        const auto curve =
            curveView(model, edge.source);
        if (!curve) {
            return std::nullopt;
        }

        const auto use =
            boundaryUse(edge, half.forward);
        const auto contribution =
            useAreaContribution(*curve, use);
        const auto length =
            usePerimeter(*curve, use);
        if (!contribution || !length) {
            return std::nullopt;
        }

        area += *contribution;
        perimeter += *length;
        result.boundary.push_back(use);
    }

    if (!finiteValue(area) ||
        !finiteValue(perimeter)) {
        return std::nullopt;
    }

    result.signed_area = area;
    result.perimeter = perimeter;
    return result;
}

[[nodiscard]] bool inverseHalfEdges(
    const HalfEdge& first,
    const HalfEdge& second) noexcept {
    return first.edge == second.edge &&
           first.forward != second.forward;
}

[[nodiscard]] std::vector<std::size_t> cancelBridges(
    const std::vector<HalfEdge>& half_edges,
    const std::vector<std::size_t>& cycle) {
    std::vector<std::size_t> result;
    result.reserve(cycle.size());

    for (const auto half : cycle) {
        if (!result.empty() &&
            inverseHalfEdges(
                half_edges[result.back()],
                half_edges[half])) {
            result.pop_back();
        } else {
            result.push_back(half);
        }
    }

    bool changed = true;
    while (changed &&
           result.size() >= 2U) {
        changed = false;
        if (inverseHalfEdges(
                half_edges[result.front()],
                half_edges[result.back()])) {
            result.erase(result.begin());
            result.pop_back();
            changed = true;
        }
    }

    return result;
}

[[nodiscard]] bool parameterInUse(
    const CurveView& curve,
    const RegionBoundaryUse2D& use,
    double parameter) noexcept {
    if (use.whole_closed_curve) {
        return curve.kind == CurveKind::circle;
    }

    const double source_from =
        use.follows_source_direction
            ? use.start_parameter
            : use.end_parameter;
    const double source_to =
        use.follows_source_direction
            ? use.end_parameter
            : use.start_parameter;

    if (curve.kind == CurveKind::circle &&
        use.crosses_closed_seam) {
        return parameter >= source_from ||
               parameter <= source_to;
    }

    const double low =
        std::min(source_from, source_to);
    const double high =
        std::max(source_from, source_to);
    return parameter >= low &&
           parameter <= high;
}

[[nodiscard]] bool pointOnUse(
    const CurveView& curve,
    const RegionBoundaryUse2D& use,
    Point2 point) noexcept {
    if (!point.finite()) {
        return false;
    }

    if (curve.kind == CurveKind::line) {
        const auto parameter =
            lineParameter(curve, point);
        if (!parameter ||
            !parameterInUse(
                curve,
                use,
                *parameter)) {
            return false;
        }
        const auto reconstructed =
            linePoint(curve, *parameter);
        return reconstructed &&
               *reconstructed == point;
    }

    const double du =
        point.u - curve.center.u;
    const double dv =
        point.v - curve.center.v;
    const double squared =
        std::fma(du, du, dv * dv);
    const double radius_squared =
        curve.radius * curve.radius;
    if (!finiteValue(squared) ||
        !finiteValue(radius_squared) ||
        squared != radius_squared) {
        return false;
    }

    const auto parameter =
        curveParameter(curve, point);
    return parameter &&
           parameterInUse(
               curve,
               use,
               *parameter);
}

[[nodiscard]] std::optional<int> horizontalCrossings(
    const SketchModel& model,
    const RegionLoop2D& loop,
    Point2 point) {
    int crossings = 0;

    for (const auto& use : loop.boundary) {
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return std::nullopt;
        }

        const auto start =
            useStartPoint(*curve, use);
        const auto end =
            useEndPoint(*curve, use);
        if (!start || !end) {
            return std::nullopt;
        }

        if (curve->kind == CurveKind::line) {
            if (start->v == point.v ||
                end->v == point.v) {
                return std::nullopt;
            }
            const bool straddles =
                (start->v > point.v) !=
                (end->v > point.v);
            if (!straddles) {
                continue;
            }
            const double parameter =
                (point.v - start->v) /
                (end->v - start->v);
            const double x =
                std::fma(
                    parameter,
                    end->u - start->u,
                    start->u);
            if (!finiteValue(x)) {
                return std::nullopt;
            }
            if (x > point.u) {
                ++crossings;
            }
            continue;
        }

        if (start->v == point.v ||
            end->v == point.v) {
            return std::nullopt;
        }

        const double ratio =
            (point.v - curve->center.v) /
            curve->radius;
        if (!finiteValue(ratio)) {
            return std::nullopt;
        }
        if (ratio < -1.0 ||
            ratio > 1.0) {
            continue;
        }
        if (ratio == -1.0 ||
            ratio == 1.0) {
            // A ray tangent to a circular boundary touches but does not
            // cross it, so it must not toggle parity. Boundary membership
            // of the query point itself was already checked exactly above.
            continue;
        }

        const double base =
            std::asin(ratio);
        const double angles[2] = {
            normalizedAngle(base),
            normalizedAngle(
                std::numbers::pi_v<double> -
                base),
        };

        for (const double angle : angles) {
            const Point2 crossing{
                std::fma(
                    curve->radius,
                    std::cos(angle),
                    curve->center.u),
                point.v};
            const auto parameter =
                curveParameter(
                    *curve,
                    crossing);
            if (!parameter ||
                !parameterInUse(
                    *curve,
                    use,
                    *parameter)) {
                continue;
            }
            if (crossing.u > point.u) {
                ++crossings;
            }
        }
    }

    return crossings;
}

[[nodiscard]] std::optional<int> verticalCrossings(
    const SketchModel& model,
    const RegionLoop2D& loop,
    Point2 point) {
    int crossings = 0;

    for (const auto& use : loop.boundary) {
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return std::nullopt;
        }

        const auto start =
            useStartPoint(*curve, use);
        const auto end =
            useEndPoint(*curve, use);
        if (!start || !end) {
            return std::nullopt;
        }

        if (curve->kind == CurveKind::line) {
            if (start->u == point.u ||
                end->u == point.u) {
                return std::nullopt;
            }
            const bool straddles =
                (start->u > point.u) !=
                (end->u > point.u);
            if (!straddles) {
                continue;
            }
            const double parameter =
                (point.u - start->u) /
                (end->u - start->u);
            const double y =
                std::fma(
                    parameter,
                    end->v - start->v,
                    start->v);
            if (!finiteValue(y)) {
                return std::nullopt;
            }
            if (y > point.v) {
                ++crossings;
            }
            continue;
        }

        if (start->u == point.u ||
            end->u == point.u) {
            return std::nullopt;
        }

        const double ratio =
            (point.u - curve->center.u) /
            curve->radius;
        if (!finiteValue(ratio)) {
            return std::nullopt;
        }
        if (ratio < -1.0 ||
            ratio > 1.0) {
            continue;
        }
        if (ratio == -1.0 ||
            ratio == 1.0) {
            // A ray tangent to a circular boundary touches but does not
            // cross it, so it must not toggle parity. Boundary membership
            // of the query point itself was already checked exactly above.
            continue;
        }

        const double base =
            std::acos(ratio);
        const double angles[2] = {
            normalizedAngle(base),
            normalizedAngle(-base),
        };

        for (const double angle : angles) {
            const Point2 crossing{
                point.u,
                std::fma(
                    curve->radius,
                    std::sin(angle),
                    curve->center.v)};
            const auto parameter =
                curveParameter(
                    *curve,
                    crossing);
            if (!parameter ||
                !parameterInUse(
                    *curve,
                    use,
                    *parameter)) {
                continue;
            }
            if (crossing.v > point.v) {
                ++crossings;
            }
        }
    }

    return crossings;
}

enum class LoopPointState {
    outside,
    inside,
    boundary,
    ambiguous,
};

[[nodiscard]] LoopPointState pointInLoop(
    const SketchModel& model,
    const RegionLoop2D& loop,
    Point2 point) {
    for (const auto& use : loop.boundary) {
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return LoopPointState::ambiguous;
        }
        if (pointOnUse(*curve, use, point)) {
            return LoopPointState::boundary;
        }
    }

    const auto horizontal =
        horizontalCrossings(
            model,
            loop,
            point);
    if (horizontal) {
        return (*horizontal % 2) != 0
            ? LoopPointState::inside
            : LoopPointState::outside;
    }

    const auto vertical =
        verticalCrossings(
            model,
            loop,
            point);
    if (vertical) {
        return (*vertical % 2) != 0
            ? LoopPointState::inside
            : LoopPointState::outside;
    }

    return LoopPointState::ambiguous;
}

void appendUseVerticalBounds(
    const CurveView& curve,
    const RegionBoundaryUse2D& use,
    std::vector<double>& values) {
    const auto start =
        useStartPoint(curve, use);
    const auto end =
        useEndPoint(curve, use);
    if (start) values.push_back(start->v);
    if (end) values.push_back(end->v);

    if (curve.kind == CurveKind::line) {
        return;
    }

    const double angles[2] = {
        std::numbers::pi_v<double> * 0.5,
        std::numbers::pi_v<double> * 1.5,
    };
    for (const double angle : angles) {
        const auto point =
            circlePoint(curve, angle);
        if (!point) {
            continue;
        }
        const auto parameter =
            curveParameter(curve, *point);
        if (parameter &&
            parameterInUse(
                curve,
                use,
                *parameter)) {
            values.push_back(point->v);
        }
    }
}

[[nodiscard]] std::optional<std::vector<double>>
scanlineIntersections(
    const SketchModel& model,
    const RegionLoop2D& loop,
    double y) {
    std::vector<double> result;

    for (const auto& use : loop.boundary) {
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return std::nullopt;
        }

        const auto start =
            useStartPoint(*curve, use);
        const auto end =
            useEndPoint(*curve, use);
        if (!start || !end) {
            return std::nullopt;
        }
        if (start->v == y ||
            end->v == y) {
            return std::nullopt;
        }

        if (curve->kind == CurveKind::line) {
            if ((start->v > y) ==
                (end->v > y)) {
                continue;
            }
            const double parameter =
                (y - start->v) /
                (end->v - start->v);
            const double x =
                std::fma(
                    parameter,
                    end->u - start->u,
                    start->u);
            if (!finiteValue(x)) {
                return std::nullopt;
            }
            result.push_back(x);
            continue;
        }

        const double ratio =
            (y - curve->center.v) /
            curve->radius;
        if (!finiteValue(ratio)) {
            return std::nullopt;
        }
        if (ratio < -1.0 ||
            ratio > 1.0) {
            continue;
        }
        if (ratio == -1.0 ||
            ratio == 1.0) {
            // A ray tangent to a circular boundary touches but does not
            // cross it, so it must not toggle parity. Boundary membership
            // of the query point itself was already checked exactly above.
            continue;
        }

        const double base =
            std::asin(ratio);
        const double angles[2] = {
            normalizedAngle(base),
            normalizedAngle(
                std::numbers::pi_v<double> -
                base),
        };
        for (const double angle : angles) {
            const Point2 point{
                std::fma(
                    curve->radius,
                    std::cos(angle),
                    curve->center.u),
                y};
            const auto parameter =
                curveParameter(
                    *curve,
                    point);
            if (parameter &&
                parameterInUse(
                    *curve,
                    use,
                    *parameter)) {
                result.push_back(point.u);
            }
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(
        std::unique(
            result.begin(),
            result.end()),
        result.end());
    return result;
}

[[nodiscard]] std::optional<Point2> interiorPoint(
    const SketchModel& model,
    const RegionLoop2D& loop) {
    std::vector<double> vertical_values;
    for (const auto& use : loop.boundary) {
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return std::nullopt;
        }
        appendUseVerticalBounds(
            *curve,
            use,
            vertical_values);
    }

    if (vertical_values.empty()) {
        return std::nullopt;
    }

    const auto [minimum_it, maximum_it] =
        std::minmax_element(
            vertical_values.begin(),
            vertical_values.end());
    const double minimum = *minimum_it;
    const double maximum = *maximum_it;
    if (!finiteValue(minimum) ||
        !finiteValue(maximum) ||
        !(maximum > minimum)) {
        return std::nullopt;
    }

    const double fractions[] = {
        0.5,
        1.0 / 3.0,
        2.0 / 3.0,
        0.25,
        0.75,
        0.2,
        0.8,
    };

    for (const double fraction : fractions) {
        const double y =
            std::fma(
                fraction,
                maximum - minimum,
                minimum);
        const auto xs =
            scanlineIntersections(
                model,
                loop,
                y);
        if (!xs ||
            xs->size() < 2U ||
            (xs->size() % 2U) != 0U) {
            continue;
        }

        for (std::size_t index = 0U;
             index + 1U < xs->size();
             index += 2U) {
            if ((*xs)[index + 1U] <=
                (*xs)[index]) {
                continue;
            }
            const Point2 candidate{
                ((*xs)[index] +
                 (*xs)[index + 1U]) *
                    0.5,
                y};
            if (!candidate.finite()) {
                continue;
            }
            if (pointInLoop(
                    model,
                    loop,
                    candidate) ==
                LoopPointState::inside) {
                return candidate;
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<RegionLoop2D>
wholeCircleLoop(
    const SketchModel& model,
    EntityId id) {
    const auto curve =
        curveView(model, id);
    if (!curve ||
        curve->kind != CurveKind::circle) {
        return std::nullopt;
    }

    RegionLoop2D loop;
    loop.boundary.push_back(
        RegionBoundaryUse2D{
            id,
            0.0,
            0.0,
            std::nullopt,
            std::nullopt,
            true,
            true,
            true});

    const auto area =
        useAreaContribution(
            *curve,
            loop.boundary.front());
    const auto perimeter =
        usePerimeter(
            *curve,
            loop.boundary.front());
    if (!area || !perimeter ||
        !(*area > 0.0)) {
        return std::nullopt;
    }
    loop.signed_area = *area;
    loop.perimeter = *perimeter;
    return loop;
}

[[nodiscard]] std::optional<BuiltLoop> buildWholeCircle(
    const SketchModel& model,
    EntityId id) {
    auto loop =
        wholeCircleLoop(model, id);
    if (!loop) {
        return std::nullopt;
    }
    const auto sample =
        interiorPoint(model, *loop);
    if (!sample) {
        return std::nullopt;
    }
    return BuiltLoop{
        std::move(*loop),
        *sample};
}

[[nodiscard]] bool anchorLess(
    const RegionBoundaryAnchor2D& first,
    const RegionBoundaryAnchor2D& second) noexcept {
    if (first.kind != second.kind) {
        return first.kind < second.kind;
    }
    if (first.other_entity != second.other_entity) {
        return first.other_entity < second.other_entity;
    }
    return first.canonical_branch <
           second.canonical_branch;
}

[[nodiscard]] bool addCut(
    std::vector<Cut>& cuts,
    double parameter,
    std::size_t vertex,
    std::optional<RegionBoundaryAnchor2D> anchor) {
    if (!finiteValue(parameter) ||
        parameter < 0.0 ||
        parameter > 1.0) {
        return false;
    }
    for (auto& existing : cuts) {
        if (existing.parameter != parameter) {
            continue;
        }
        if (existing.vertex != vertex) {
            return false;
        }
        if (anchor &&
            (!existing.anchor ||
             anchorLess(*anchor, *existing.anchor))) {
            existing.anchor = *anchor;
        }
        return true;
    }
    cuts.push_back(
        Cut{parameter, vertex, std::move(anchor)});
    return true;
}

[[nodiscard]] std::optional<RegionBoundaryAnchor2D>
intersectionAnchor(
    EntityId other,
    std::uint32_t branch,
    bool endpoint,
    double parameter) {
    if (endpoint) {
        if (parameter == 0.0) {
            return RegionBoundaryAnchor2D{
                RegionBoundaryAnchorKind::
                    endpoint_start,
                {},
                0U};
        }
        if (parameter == 1.0) {
            return RegionBoundaryAnchor2D{
                RegionBoundaryAnchorKind::
                    endpoint_end,
                {},
                0U};
        }
        return std::nullopt;
    }

    return RegionBoundaryAnchor2D{
        RegionBoundaryAnchorKind::intersection,
        other,
        branch};
}

[[nodiscard]] std::optional<std::vector<BuiltLoop>>
buildComponentLoops(
    const SketchModel& model,
    const std::vector<EntityId>& entities,
    const std::vector<TopologyIntersection>& intersections) {
    std::vector<Vertex> vertices;
    std::vector<std::vector<Cut>> cuts(
        entities.size());

    // Intersection anchors are the canonical topology vertices. Process
    // them before authored open-curve endpoints so an exact endpoint
    // parameter (0/1) uses the relation point rather than a separately
    // re-evaluated trigonometric image of the same semantic endpoint.
    for (const auto& intersection : intersections) {
        const auto first =
            entityIndex(
                entities,
                intersection.first);
        const auto second =
            entityIndex(
                entities,
                intersection.second);
        if (!first || !second) {
            continue;
        }
        const auto vertex =
            findOrAddVertex(
                vertices,
                intersection.point);
        const auto first_anchor =
            intersectionAnchor(
                intersection.second,
                intersection.canonical_branch,
                intersection.first_endpoint,
                intersection.first_parameter);
        const auto second_anchor =
            intersectionAnchor(
                intersection.first,
                intersection.canonical_branch,
                intersection.second_endpoint,
                intersection.second_parameter);
        if (!first_anchor ||
            !second_anchor ||
            !addCut(
                cuts[*first],
                intersection.first_parameter,
                vertex,
                *first_anchor) ||
            !addCut(
                cuts[*second],
                intersection.second_parameter,
                vertex,
                *second_anchor)) {
            return std::nullopt;
        }
    }

    const auto ensure_endpoint =
        [&vertices, &cuts](
            std::size_t entity_index,
            double parameter,
            Point2 evaluated_point) {
            const auto existing =
                std::find_if(
                    cuts[entity_index].begin(),
                    cuts[entity_index].end(),
                    [parameter](const Cut& cut) {
                        return cut.parameter ==
                               parameter;
                    });
            if (existing !=
                cuts[entity_index].end()) {
                return true;
            }

            const auto vertex =
                findOrAddVertex(
                    vertices,
                    evaluated_point);
            return addCut(
                cuts[entity_index],
                parameter,
                vertex,
                RegionBoundaryAnchor2D{
                    parameter == 0.0
                        ? RegionBoundaryAnchorKind::
                              endpoint_start
                        : RegionBoundaryAnchorKind::
                              endpoint_end,
                    {},
                    0U});
        };

    for (std::size_t index = 0U;
         index < entities.size();
         ++index) {
        const auto curve =
            curveView(
                model,
                entities[index]);
        if (!curve) {
            return std::nullopt;
        }

        if (curve->kind == CurveKind::line) {
            if (!ensure_endpoint(
                    index,
                    0.0,
                    curve->first) ||
                !ensure_endpoint(
                    index,
                    1.0,
                    curve->second)) {
                return std::nullopt;
            }
        } else if (curve->kind == CurveKind::arc) {
            const auto first_point =
                arcEndpoint(*curve, false);
            const auto second_point =
                arcEndpoint(*curve, true);
            if (!first_point ||
                !second_point ||
                !ensure_endpoint(
                    index,
                    0.0,
                    *first_point) ||
                !ensure_endpoint(
                    index,
                    1.0,
                    *second_point)) {
                return std::nullopt;
            }
        }
    }

    std::vector<BuiltLoop> loops;
    std::vector<DerivedEdge> edges;

    for (std::size_t index = 0U;
         index < entities.size();
         ++index) {
        const auto curve =
            curveView(
                model,
                entities[index]);
        if (!curve) {
            return std::nullopt;
        }

        auto& entity_cuts = cuts[index];
        std::sort(
            entity_cuts.begin(),
            entity_cuts.end(),
            [](const Cut& lhs,
               const Cut& rhs) {
                if (lhs.parameter !=
                    rhs.parameter) {
                    return lhs.parameter <
                           rhs.parameter;
                }
                return lhs.vertex < rhs.vertex;
            });

        if (curve->kind == CurveKind::circle &&
            entity_cuts.empty()) {
            const auto circle =
                buildWholeCircle(
                    model,
                    entities[index]);
            if (!circle) {
                return std::nullopt;
            }
            loops.push_back(*circle);
            continue;
        }

        if (curve->kind == CurveKind::circle) {
            for (std::size_t cut = 0U;
                 cut < entity_cuts.size();
                 ++cut) {
                const auto next =
                    (cut + 1U) %
                    entity_cuts.size();
                const bool seam =
                    next == 0U;
                if (!seam &&
                    entity_cuts[cut].parameter ==
                        entity_cuts[next].parameter) {
                    continue;
                }

                edges.push_back(
                    DerivedEdge{
                        entities[index],
                        curve->kind,
                        entity_cuts[cut].vertex,
                        entity_cuts[next].vertex,
                        entity_cuts[cut].parameter,
                        entity_cuts[next].parameter,
                        entity_cuts[cut].anchor,
                        entity_cuts[next].anchor,
                        seam});
            }
            continue;
        }

        if (entity_cuts.size() < 2U) {
            return std::nullopt;
        }
        for (std::size_t cut = 0U;
             cut + 1U < entity_cuts.size();
             ++cut) {
            if (entity_cuts[cut].parameter ==
                entity_cuts[cut + 1U].parameter) {
                continue;
            }
            edges.push_back(
                DerivedEdge{
                    entities[index],
                    curve->kind,
                    entity_cuts[cut].vertex,
                    entity_cuts[cut + 1U].vertex,
                    entity_cuts[cut].parameter,
                    entity_cuts[cut + 1U].parameter,
                    entity_cuts[cut].anchor,
                    entity_cuts[cut + 1U].anchor,
                    false});
        }
    }

    std::vector<HalfEdge> half_edges;
    half_edges.reserve(edges.size() * 2U);
    for (std::size_t edge_index = 0U;
         edge_index < edges.size();
         ++edge_index) {
        const auto& edge =
            edges[edge_index];
        const auto curve =
            curveView(model, edge.source);
        if (!curve) {
            return std::nullopt;
        }
        const auto forward_angle =
            halfEdgeAngle(
                *curve,
                edge,
                true);
        const auto reverse_angle =
            halfEdgeAngle(
                *curve,
                edge,
                false);
        if (!forward_angle ||
            !reverse_angle) {
            return std::nullopt;
        }

        const auto forward_index =
            half_edges.size();
        half_edges.push_back(
            HalfEdge{
                edge_index,
                true,
                edge.first_vertex,
                edge.second_vertex,
                *forward_angle});
        const auto reverse_index =
            half_edges.size();
        half_edges.push_back(
            HalfEdge{
                edge_index,
                false,
                edge.second_vertex,
                edge.first_vertex,
                *reverse_angle});

        vertices[edge.first_vertex]
            .outgoing.push_back(
                forward_index);
        vertices[edge.second_vertex]
            .outgoing.push_back(
                reverse_index);
    }

    for (auto& vertex : vertices) {
        std::sort(
            vertex.outgoing.begin(),
            vertex.outgoing.end(),
            [&half_edges](
                std::size_t lhs,
                std::size_t rhs) {
                const auto& first =
                    half_edges[lhs];
                const auto& second =
                    half_edges[rhs];
                if (first.tangent_angle !=
                    second.tangent_angle) {
                    return first.tangent_angle <
                           second.tangent_angle;
                }
                if (first.edge != second.edge) {
                    return first.edge <
                           second.edge;
                }
                return first.forward &&
                       !second.forward;
            });
    }

    std::vector<bool> visited(
        half_edges.size(),
        false);
    for (std::size_t start = 0U;
         start < half_edges.size();
         ++start) {
        if (visited[start]) {
            continue;
        }

        std::vector<std::size_t> cycle;
        std::size_t current = start;
        bool closed = false;

        for (std::size_t guard = 0U;
             guard <= half_edges.size();
             ++guard) {
            if (visited[current]) {
                closed = current == start;
                break;
            }
            visited[current] = true;
            cycle.push_back(current);

            const auto& half =
                half_edges[current];
            const auto twin =
                current ^ 1U;
            const auto& outgoing =
                vertices[half.destination]
                    .outgoing;
            const auto found =
                std::find(
                    outgoing.begin(),
                    outgoing.end(),
                    twin);
            if (found == outgoing.end() ||
                outgoing.empty()) {
                return std::nullopt;
            }

            const auto position =
                static_cast<std::size_t>(
                    found - outgoing.begin());
            const auto previous =
                (position + outgoing.size() - 1U) %
                outgoing.size();
            current = outgoing[previous];
        }

        if (!closed) {
            return std::nullopt;
        }

        cycle =
            cancelBridges(
                half_edges,
                cycle);
        if (cycle.empty()) {
            continue;
        }

        auto loop =
            makeLoop(
                model,
                edges,
                half_edges,
                cycle);
        if (!loop) {
            return std::nullopt;
        }

        if (!(loop->signed_area > 0.0)) {
            continue;
        }

        const auto sample =
            interiorPoint(
                model,
                *loop);
        if (!sample) {
            return std::nullopt;
        }

        loops.push_back(
            BuiltLoop{
                std::move(*loop),
                *sample});
    }

    return loops;
}

[[nodiscard]] std::vector<EntityId> componentEntities(
    const std::vector<EntityId>& ids,
    DisjointSet& sets,
    std::size_t root) {
    std::vector<EntityId> result;
    for (std::size_t index = 0U;
         index < ids.size();
         ++index) {
        if (sets.find(index) == root) {
            result.push_back(ids[index]);
        }
    }
    return result;
}

[[nodiscard]] bool regularCurve(
    const SketchModel& model,
    EntityId id) noexcept {
    if (const auto* line = model.findLine(id)) {
        return line->role() == EntityRole::regular;
    }
    if (const auto* circle = model.findCircle(id)) {
        return circle->role() == EntityRole::regular;
    }
    if (const auto* arc = model.findArc(id)) {
        return arc->role() == EntityRole::regular;
    }
    return false;
}

[[nodiscard]] std::optional<double> useMidParameter(
    const CurveView& curve,
    const RegionBoundaryUse2D& use) noexcept {
    if (use.whole_closed_curve) {
        return 0.5;
    }

    double from =
        use.follows_source_direction
            ? use.start_parameter
            : use.end_parameter;
    double to =
        use.follows_source_direction
            ? use.end_parameter
            : use.start_parameter;

    if (curve.kind == CurveKind::circle &&
        use.crosses_closed_seam) {
        if (to <= from) {
            to += 1.0;
        }
        double value = (from + to) * 0.5;
        if (value >= 1.0) {
            value -= 1.0;
        }
        return finiteValue(value)
            ? std::optional<double>{value}
            : std::nullopt;
    }

    const double value =
        (from + to) * 0.5;
    return finiteValue(value)
        ? std::optional<double>{value}
        : std::nullopt;
}

[[nodiscard]] bool samePoint(
    const std::optional<Point2>& first,
    const std::optional<Point2>& second) noexcept {
    return first && second &&
           *first == *second;
}

[[nodiscard]] bool relationParametersMatch(
    const CurveRelation2D& relation,
    const CurveIntersection2D& item,
    const RegionBoundaryUse2D& first,
    double first_parameter,
    const RegionBoundaryUse2D& second,
    double second_parameter) noexcept {
    const double relation_first =
        relation.first_entity ==
                first.source_entity
            ? item.first_parameter
            : item.second_parameter;
    const double relation_second =
        relation.first_entity ==
                second.source_entity
            ? item.first_parameter
            : item.second_parameter;
    return relation_first == first_parameter &&
           relation_second == second_parameter;
}

[[nodiscard]] bool usesJoin(
    const SketchModel& model,
    const RegionBoundaryUse2D& first,
    const RegionBoundaryUse2D& second) {
    if (first.source_entity ==
        second.source_entity) {
        return first.end_parameter ==
                   second.start_parameter &&
               first.end_anchor ==
                   second.start_anchor;
    }

    const auto relation =
        analyzeCurveRelation(
            model,
            first.source_entity,
            second.source_entity);
    if (relation.status !=
        CurveRelationStatus::discrete) {
        return false;
    }

    return std::any_of(
        relation.intersections.begin(),
        relation.intersections.end(),
        [&relation, &first, &second](
            const CurveIntersection2D& item) {
            return relationParametersMatch(
                relation,
                item,
                first,
                first.end_parameter,
                second,
                second.start_parameter);
        });
}

[[nodiscard]] bool usesHaveUnexpectedContact(
    const SketchModel& model,
    const RegionBoundaryUse2D& first,
    const RegionBoundaryUse2D& second,
    bool adjacent_first_to_second) {
    const auto first_curve =
        curveView(model, first.source_entity);
    const auto second_curve =
        curveView(model, second.source_entity);
    if (!first_curve || !second_curve) {
        return true;
    }

    if (first.source_entity ==
        second.source_entity) {
        const auto first_start =
            useStartPoint(*first_curve, first);
        const auto first_end =
            useEndPoint(*first_curve, first);
        const auto second_start =
            useStartPoint(*second_curve, second);
        const auto second_end =
            useEndPoint(*second_curve, second);
        if (!first_start || !first_end ||
            !second_start || !second_end) {
            return true;
        }

        const auto expected =
            adjacent_first_to_second
                ? std::optional<Point2>{*first_end}
                : std::nullopt;

        const Point2 points[] = {
            *first_start,
            *first_end,
            *second_start,
            *second_end,
        };
        for (const auto point : points) {
            const bool on_first =
                pointOnUse(
                    *first_curve,
                    first,
                    point);
            const bool on_second =
                pointOnUse(
                    *second_curve,
                    second,
                    point);
            if (!on_first || !on_second) {
                continue;
            }
            if (!expected ||
                point != *expected) {
                return true;
            }
        }

        const auto first_mid =
            useMidParameter(
                *first_curve,
                first);
        const auto second_mid =
            useMidParameter(
                *second_curve,
                second);
        if (!first_mid || !second_mid) {
            return true;
        }
        if (parameterInUse(
                *second_curve,
                second,
                *first_mid) ||
            parameterInUse(
                *first_curve,
                first,
                *second_mid)) {
            return true;
        }
        return false;
    }

    const auto relation =
        analyzeCurveRelation(
            model,
            first.source_entity,
            second.source_entity);
    if (relation.status ==
            CurveRelationStatus::invalid ||
        relation.status ==
            CurveRelationStatus::overlap) {
        return true;
    }
    if (relation.status !=
        CurveRelationStatus::discrete) {
        return false;
    }

    for (const auto& item :
         relation.intersections) {
        const double first_parameter =
            relation.first_entity ==
                    first.source_entity
                ? item.first_parameter
                : item.second_parameter;
        const double second_parameter =
            relation.first_entity ==
                    second.source_entity
                ? item.first_parameter
                : item.second_parameter;
        if (!parameterInUse(
                *first_curve,
                first,
                first_parameter) ||
            !parameterInUse(
                *second_curve,
                second,
                second_parameter)) {
            continue;
        }

        if (!adjacent_first_to_second ||
            !relationParametersMatch(
                relation,
                item,
                first,
                first.end_parameter,
                second,
                second.start_parameter)) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] bool twoUseLoopHasUnexpectedContact(
    const SketchModel& model,
    const RegionBoundaryUse2D& first,
    const RegionBoundaryUse2D& second) {
    const auto first_curve =
        curveView(model, first.source_entity);
    const auto second_curve =
        curveView(model, second.source_entity);
    if (!first_curve || !second_curve) {
        return true;
    }

    if (!usesJoin(
            model,
            first,
            second) ||
        !usesJoin(
            model,
            second,
            first)) {
        return true;
    }

    if (first.source_entity ==
        second.source_entity) {
        const auto first_mid =
            useMidParameter(
                *first_curve,
                first);
        const auto second_mid =
            useMidParameter(
                *second_curve,
                second);
        if (!first_mid || !second_mid) {
            return true;
        }
        return parameterInUse(
                   *second_curve,
                   second,
                   *first_mid) ||
               parameterInUse(
                   *first_curve,
                   first,
                   *second_mid);
    }

    const auto relation =
        analyzeCurveRelation(
            model,
            first.source_entity,
            second.source_entity);
    if (relation.status !=
        CurveRelationStatus::discrete) {
        return true;
    }

    for (const auto& item :
         relation.intersections) {
        const double first_parameter =
            relation.first_entity ==
                    first.source_entity
                ? item.first_parameter
                : item.second_parameter;
        const double second_parameter =
            relation.first_entity ==
                    second.source_entity
                ? item.first_parameter
                : item.second_parameter;
        if (!parameterInUse(
                *first_curve,
                first,
                first_parameter) ||
            !parameterInUse(
                *second_curve,
                second,
                second_parameter)) {
            continue;
        }

        const bool forward_join =
            relationParametersMatch(
                relation,
                item,
                first,
                first.end_parameter,
                second,
                second.start_parameter);
        const bool reverse_join =
            relationParametersMatch(
                relation,
                item,
                first,
                first.start_parameter,
                second,
                second.end_parameter);
        if (!forward_join &&
            !reverse_join) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] std::optional<RegionLoop2D>
validatedLoop(
    const SketchModel& model,
    RegionLoop2D loop) {
    if (loop.boundary.empty()) {
        return std::nullopt;
    }

    if (loop.boundary.size() == 1U &&
        loop.boundary.front()
            .whole_closed_curve) {
        const auto& use =
            loop.boundary.front();
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve ||
            curve->kind != CurveKind::circle ||
            !regularCurve(
                model,
                use.source_entity) ||
            use.start_anchor ||
            use.end_anchor) {
            return std::nullopt;
        }
    } else {
        for (const auto& use :
             loop.boundary) {
            if (use.whole_closed_curve ||
                !use.start_anchor ||
                !use.end_anchor ||
                !regularCurve(
                    model,
                    use.source_entity)) {
                return std::nullopt;
            }
        }
    }

    double area = 0.0;
    double perimeter = 0.0;
    for (std::size_t index = 0U;
         index < loop.boundary.size();
         ++index) {
        const auto& use =
            loop.boundary[index];
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return std::nullopt;
        }

        const auto contribution =
            useAreaContribution(
                *curve,
                use);
        const auto length =
            usePerimeter(
                *curve,
                use);
        if (!contribution ||
            !length) {
            return std::nullopt;
        }
        area += *contribution;
        perimeter += *length;

        if (loop.boundary.size() > 1U) {
            const auto& next =
                loop.boundary[
                    (index + 1U) %
                    loop.boundary.size()];
            if (!usesJoin(
                    model,
                    use,
                    next)) {
                return std::nullopt;
            }
        }
    }

    if (!finiteValue(area) ||
        !finiteValue(perimeter) ||
        !(area > 0.0) ||
        !(perimeter > 0.0)) {
        return std::nullopt;
    }

    for (std::size_t first = 0U;
         first < loop.boundary.size();
         ++first) {
        for (std::size_t second =
                 first + 1U;
             second < loop.boundary.size();
             ++second) {
            if (loop.boundary.size() == 2U) {
                if (twoUseLoopHasUnexpectedContact(
                        model,
                        loop.boundary[first],
                        loop.boundary[second])) {
                    return std::nullopt;
                }
                continue;
            }

            const bool adjacent =
                second == first + 1U;
            const bool wrap_adjacent =
                first == 0U &&
                second + 1U ==
                    loop.boundary.size();

            if (wrap_adjacent) {
                if (usesHaveUnexpectedContact(
                        model,
                        loop.boundary[second],
                        loop.boundary[first],
                        true)) {
                    return std::nullopt;
                }
                continue;
            }

            if (usesHaveUnexpectedContact(
                    model,
                    loop.boundary[first],
                    loop.boundary[second],
                    adjacent)) {
                return std::nullopt;
            }
        }
    }

    loop.signed_area = area;
    loop.perimeter = perimeter;
    return loop;
}

[[nodiscard]] bool loopsTouchOrCross(
    const SketchModel& model,
    const RegionLoop2D& first,
    const RegionLoop2D& second) {
    for (const auto& first_use :
         first.boundary) {
        for (const auto& second_use :
             second.boundary) {
            if (usesHaveUnexpectedContact(
                    model,
                    first_use,
                    second_use,
                    false)) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] RegionBoundaryUse2D
reversedUse(
    const RegionBoundaryUse2D& use) {
    return RegionBoundaryUse2D{
        use.source_entity,
        use.end_parameter,
        use.start_parameter,
        use.end_anchor,
        use.start_anchor,
        !use.follows_source_direction,
        use.crosses_closed_seam,
        use.whole_closed_curve};
}

[[nodiscard]] bool inverseUses(
    const RegionBoundaryUse2D& first,
    const RegionBoundaryUse2D& second) {
    return first == reversedUse(second);
}

void appendMaterialLoop(
    std::vector<RegionBoundaryUse2D>& boundary,
    const RegionLoop2D& loop,
    bool material_on_left) {
    if (material_on_left) {
        boundary.insert(
            boundary.end(),
            loop.boundary.begin(),
            loop.boundary.end());
        return;
    }

    for (auto it = loop.boundary.rbegin();
         it != loop.boundary.rend();
         ++it) {
        boundary.push_back(
            reversedUse(*it));
    }
}

[[nodiscard]] std::optional<std::pair<double, double>>
loopMetrics(
    const SketchModel& model,
    const std::vector<RegionBoundaryUse2D>& boundary) {
    double area = 0.0;
    double perimeter = 0.0;
    for (const auto& use : boundary) {
        const auto curve =
            curveView(
                model,
                use.source_entity);
        if (!curve) {
            return std::nullopt;
        }
        const auto contribution =
            useAreaContribution(
                *curve,
                use);
        const auto length =
            usePerimeter(
                *curve,
                use);
        if (!contribution || !length) {
            return std::nullopt;
        }
        area += *contribution;
        perimeter += *length;
    }
    if (!finiteValue(area) ||
        !finiteValue(perimeter)) {
        return std::nullopt;
    }
    return std::pair<double, double>{
        area,
        perimeter};
}

[[nodiscard]] RegionLoop2D reversedLoop(
    const RegionLoop2D& loop) {
    RegionLoop2D result;
    result.boundary.reserve(
        loop.boundary.size());
    for (auto it = loop.boundary.rbegin();
         it != loop.boundary.rend();
         ++it) {
        result.boundary.push_back(
            reversedUse(*it));
    }
    result.signed_area =
        -loop.signed_area;
    result.perimeter =
        loop.perimeter;
    return result;
}

[[nodiscard]] bool pointInRegionMaterial(
    const SketchModel& model,
    const RegionCandidate2D& region,
    Point2 point) {
    if (pointInLoop(
            model,
            region.outer,
            point) !=
        LoopPointState::inside) {
        return false;
    }

    for (const auto& hole :
         region.holes) {
        if (pointInLoop(
                model,
                hole,
                point) !=
            LoopPointState::outside) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::optional<Point2>
regionInteriorPointImpl(
    const SketchModel& model,
    const RegionCandidate2D& region) {
    const auto simple =
        interiorPoint(
            model,
            region.outer);
    if (simple &&
        pointInRegionMaterial(
            model,
            region,
            *simple)) {
        return simple;
    }

    std::vector<double> vertical_values;
    const auto append_loop_bounds =
        [&model, &vertical_values](
            const RegionLoop2D& loop) {
            for (const auto& use :
                 loop.boundary) {
                const auto curve =
                    curveView(
                        model,
                        use.source_entity);
                if (!curve) {
                    return false;
                }
                appendUseVerticalBounds(
                    *curve,
                    use,
                    vertical_values);
            }
            return true;
        };

    if (!append_loop_bounds(region.outer)) {
        return std::nullopt;
    }
    for (const auto& hole :
         region.holes) {
        if (!append_loop_bounds(hole)) {
            return std::nullopt;
        }
    }

    std::sort(
        vertical_values.begin(),
        vertical_values.end());
    vertical_values.erase(
        std::unique(
            vertical_values.begin(),
            vertical_values.end()),
        vertical_values.end());

    for (std::size_t y_index = 0U;
         y_index + 1U <
             vertical_values.size();
         ++y_index) {
        if (!(vertical_values[y_index + 1U] >
              vertical_values[y_index])) {
            continue;
        }
        const double y =
            (vertical_values[y_index] +
             vertical_values[y_index + 1U]) *
            0.5;
        if (!finiteValue(y)) {
            continue;
        }

        std::vector<double> xs;
        const auto append_scan =
            [&model, y, &xs](
                const RegionLoop2D& loop) {
                const auto values =
                    scanlineIntersections(
                        model,
                        loop,
                        y);
                if (!values) {
                    return false;
                }
                xs.insert(
                    xs.end(),
                    values->begin(),
                    values->end());
                return true;
            };

        if (!append_scan(region.outer)) {
            continue;
        }
        bool holes_ok = true;
        for (const auto& hole :
             region.holes) {
            if (!append_scan(hole)) {
                holes_ok = false;
                break;
            }
        }
        if (!holes_ok) {
            continue;
        }

        std::sort(xs.begin(), xs.end());
        xs.erase(
            std::unique(
                xs.begin(),
                xs.end()),
            xs.end());

        for (std::size_t x_index = 0U;
             x_index + 1U < xs.size();
             ++x_index) {
            if (!(xs[x_index + 1U] >
                  xs[x_index])) {
                continue;
            }

            const Point2 candidate{
                (xs[x_index] +
                 xs[x_index + 1U]) *
                    0.5,
                y};
            if (candidate.finite() &&
                pointInRegionMaterial(
                    model,
                    region,
                    candidate)) {
                return candidate;
            }
        }
    }

    return std::nullopt;
}

[[nodiscard]] RegionComposition2D
composeRegionCellsImpl(
    const SketchModel& model,
    const std::vector<RegionCandidate2D>& cells) {
    if (cells.empty()) {
        return {
            RegionCompositionStatus::empty,
            std::nullopt};
    }

    std::vector<RegionBoundaryUse2D>
        boundary;
    for (const auto& cell : cells) {
        appendMaterialLoop(
            boundary,
            cell.outer,
            true);
        for (const auto& hole :
             cell.holes) {
            appendMaterialLoop(
                boundary,
                hole,
                false);
        }
    }

    std::vector<RegionBoundaryUse2D>
        exposed;
    exposed.reserve(boundary.size());
    for (const auto& use : boundary) {
        const auto inverse =
            std::find_if(
                exposed.begin(),
                exposed.end(),
                [&use](
                    const RegionBoundaryUse2D&
                        existing) {
                    return inverseUses(
                        existing,
                        use);
                });
        if (inverse != exposed.end()) {
            exposed.erase(inverse);
        } else {
            exposed.push_back(use);
        }
    }

    if (exposed.empty()) {
        return {
            RegionCompositionStatus::empty,
            std::nullopt};
    }

    std::vector<bool> used(
        exposed.size(),
        false);
    std::vector<RegionLoop2D>
        positive_loops;
    std::vector<RegionLoop2D>
        negative_loops;

    for (std::size_t start = 0U;
         start < exposed.size();
         ++start) {
        if (used[start]) {
            continue;
        }

        RegionLoop2D loop;
        std::size_t current = start;
        for (std::size_t guard = 0U;
             guard <= exposed.size();
             ++guard) {
            if (used[current]) {
                return {
                    RegionCompositionStatus::
                        invalid_topology,
                    std::nullopt};
            }

            used[current] = true;
            loop.boundary.push_back(
                exposed[current]);

            if (usesJoin(
                    model,
                    loop.boundary.back(),
                    loop.boundary.front())) {
                break;
            }

            std::optional<std::size_t> next;
            for (std::size_t candidate = 0U;
                 candidate < exposed.size();
                 ++candidate) {
                if (used[candidate] ||
                    !usesJoin(
                        model,
                        exposed[current],
                        exposed[candidate])) {
                    continue;
                }
                if (next) {
                    return {
                        RegionCompositionStatus::
                            invalid_topology,
                        std::nullopt};
                }
                next = candidate;
            }

            if (!next) {
                return {
                    RegionCompositionStatus::
                        invalid_topology,
                    std::nullopt};
            }
            current = *next;
        }

        if (loop.boundary.empty() ||
            !usesJoin(
                model,
                loop.boundary.back(),
                loop.boundary.front())) {
            return {
                RegionCompositionStatus::
                    invalid_topology,
                std::nullopt};
        }

        const auto metrics =
            loopMetrics(
                model,
                loop.boundary);
        if (!metrics ||
            metrics->first == 0.0 ||
            !(metrics->second > 0.0)) {
            return {
                RegionCompositionStatus::
                    invalid_topology,
                std::nullopt};
        }

        loop.signed_area =
            metrics->first;
        loop.perimeter =
            metrics->second;

        if (loop.signed_area > 0.0) {
            positive_loops.push_back(
                std::move(loop));
        } else {
            negative_loops.push_back(
                reversedLoop(loop));
        }
    }

    if (positive_loops.size() != 1U) {
        return {
            RegionCompositionStatus::disconnected,
            std::nullopt};
    }

    auto result =
        validateRegionBoundary(
            model,
            std::move(positive_loops.front()),
            std::move(negative_loops));
    if (!result) {
        return {
            RegionCompositionStatus::
                invalid_topology,
            std::nullopt};
    }

    return {
        RegionCompositionStatus::valid,
        std::move(result)};
}

} // namespace

RegionAnalysis2D analyzeRegions(
    const SketchModel& model) {
    RegionAnalysis2D result;
    const auto ids =
        regularEntities(model);
    if (ids.empty()) {
        return result;
    }

    DisjointSet sets(ids.size());
    std::vector<bool> overlap_entity(
        ids.size(),
        false);
    std::vector<bool> invalid_entity(
        ids.size(),
        false);
    std::vector<TopologyIntersection>
        intersections;

    for (std::size_t first = 0U;
         first < ids.size();
         ++first) {
        for (std::size_t second = first + 1U;
             second < ids.size();
             ++second) {
            const auto relation =
                analyzeCurveRelation(
                    model,
                    ids[first],
                    ids[second]);

            if (relation.status ==
                CurveRelationStatus::invalid) {
                sets.unite(first, second);
                invalid_entity[first] = true;
                invalid_entity[second] = true;
                continue;
            }

            if (relation.status ==
                CurveRelationStatus::overlap) {
                sets.unite(first, second);
                overlap_entity[first] = true;
                overlap_entity[second] = true;
                continue;
            }

            if (relation.status !=
                CurveRelationStatus::discrete) {
                continue;
            }

            for (const auto& item :
                 relation.intersections) {
                const bool topological =
                    item.contact !=
                        CurveContactKind::tangent ||
                    item.first_endpoint ||
                    item.second_endpoint;
                if (!topological) {
                    continue;
                }

                sets.unite(first, second);
                intersections.push_back(
                    TopologyIntersection{
                        relation.first_entity,
                        relation.second_entity,
                        item.point,
                        item.first_parameter,
                        item.second_parameter,
                        item.canonical_branch,
                        item.first_endpoint,
                        item.second_endpoint});
            }
        }
    }

    std::vector<std::size_t> roots;
    for (std::size_t index = 0U;
         index < ids.size();
         ++index) {
        const auto root =
            sets.find(index);
        if (std::find(
                roots.begin(),
                roots.end(),
                root) == roots.end()) {
            roots.push_back(root);
        }
    }

    std::vector<BuiltLoop> loops;
    for (const auto root : roots) {
        const auto entities =
            componentEntities(
                ids,
                sets,
                root);

        bool has_overlap = false;
        bool has_invalid = false;
        for (std::size_t index = 0U;
             index < ids.size();
             ++index) {
            if (sets.find(index) != root) {
                continue;
            }
            has_overlap =
                has_overlap ||
                overlap_entity[index];
            has_invalid =
                has_invalid ||
                invalid_entity[index];
        }

        if (has_overlap || has_invalid) {
            result.diagnostics.push_back(
                RegionAnalysisDiagnostic2D{
                    has_overlap
                        ? RegionAnalysisDiagnosticKind::
                              ambiguous_overlap
                        : RegionAnalysisDiagnosticKind::
                              invalid_topology,
                    entities});
            continue;
        }

        std::vector<TopologyIntersection>
            component_intersections;
        for (const auto& item : intersections) {
            if (std::binary_search(
                    entities.begin(),
                    entities.end(),
                    item.first) &&
                std::binary_search(
                    entities.begin(),
                    entities.end(),
                    item.second)) {
                component_intersections.push_back(
                    item);
            }
        }

        const auto built =
            buildComponentLoops(
                model,
                entities,
                component_intersections);
        if (!built) {
            result.diagnostics.push_back(
                RegionAnalysisDiagnostic2D{
                    RegionAnalysisDiagnosticKind::
                        invalid_topology,
                    entities});
            continue;
        }

        if (built->empty()) {
            result.diagnostics.push_back(
                RegionAnalysisDiagnostic2D{
                    RegionAnalysisDiagnosticKind::
                        open_boundary,
                    entities});
            continue;
        }

        loops.insert(
            loops.end(),
            built->begin(),
            built->end());
    }

    if (loops.empty()) {
        return result;
    }

    std::vector<std::optional<std::size_t>>
        parent(loops.size());
    for (std::size_t child = 0U;
         child < loops.size();
         ++child) {
        double parent_area =
            std::numeric_limits<double>::infinity();

        for (std::size_t candidate = 0U;
             candidate < loops.size();
             ++candidate) {
            if (candidate == child ||
                !(loops[candidate]
                      .loop.signed_area >
                  loops[child]
                      .loop.signed_area)) {
                continue;
            }

            const auto state =
                pointInLoop(
                    model,
                    loops[candidate].loop,
                    loops[child]
                        .interior_point);
            if (state ==
                    LoopPointState::boundary ||
                state ==
                    LoopPointState::ambiguous) {
                result.diagnostics.push_back(
                    RegionAnalysisDiagnostic2D{
                        RegionAnalysisDiagnosticKind::
                            invalid_topology,
                        {}});
                return result;
            }
            if (state !=
                LoopPointState::inside) {
                continue;
            }

            if (loops[candidate]
                    .loop.signed_area <
                parent_area) {
                parent_area =
                    loops[candidate]
                        .loop.signed_area;
                parent[child] = candidate;
            }
        }
    }

    for (std::size_t index = 0U;
         index < loops.size();
         ++index) {
        RegionCandidate2D candidate;
        candidate.region_index =
            static_cast<std::uint32_t>(
                result.regions.size());
        candidate.outer =
            loops[index].loop;
        candidate.area =
            candidate.outer.signed_area;
        candidate.perimeter =
            candidate.outer.perimeter;

        for (std::size_t child = 0U;
             child < loops.size();
             ++child) {
            if (parent[child] &&
                *parent[child] == index) {
                candidate.holes.push_back(
                    loops[child].loop);
                candidate.area -=
                    loops[child]
                        .loop.signed_area;
                candidate.perimeter +=
                    loops[child]
                        .loop.perimeter;
            }
        }

        if (!(candidate.area > 0.0) ||
            !finiteValue(candidate.area) ||
            !finiteValue(candidate.perimeter)) {
            result.diagnostics.push_back(
                RegionAnalysisDiagnostic2D{
                    RegionAnalysisDiagnosticKind::
                        invalid_topology,
                    {}});
            continue;
        }

        result.regions.push_back(
            std::move(candidate));
    }

    return result;
}

std::optional<RegionCandidate2D>
validateRegionBoundary(
    const SketchModel& model,
    RegionLoop2D outer,
    std::vector<RegionLoop2D> holes) {
    auto valid_outer =
        validatedLoop(
            model,
            std::move(outer));
    if (!valid_outer) {
        return std::nullopt;
    }

    std::vector<RegionLoop2D>
        valid_holes;
    valid_holes.reserve(
        holes.size());
    for (auto& hole : holes) {
        auto valid_hole =
            validatedLoop(
                model,
                std::move(hole));
        if (!valid_hole) {
            return std::nullopt;
        }

        if (loopsTouchOrCross(
                model,
                *valid_outer,
                *valid_hole)) {
            return std::nullopt;
        }

        const auto sample =
            interiorPoint(
                model,
                *valid_hole);
        if (!sample ||
            pointInLoop(
                model,
                *valid_outer,
                *sample) !=
                LoopPointState::inside) {
            return std::nullopt;
        }

        for (const auto& existing :
             valid_holes) {
            if (loopsTouchOrCross(
                    model,
                    existing,
                    *valid_hole)) {
                return std::nullopt;
            }
            const auto existing_sample =
                interiorPoint(
                    model,
                    existing);
            if (!existing_sample) {
                return std::nullopt;
            }
            if (pointInLoop(
                    model,
                    existing,
                    *sample) ==
                    LoopPointState::inside ||
                pointInLoop(
                    model,
                    *valid_hole,
                    *existing_sample) ==
                    LoopPointState::inside) {
                return std::nullopt;
            }
        }

        valid_holes.push_back(
            std::move(*valid_hole));
    }

    double area =
        valid_outer->signed_area;
    double perimeter =
        valid_outer->perimeter;
    for (const auto& hole :
         valid_holes) {
        area -= hole.signed_area;
        perimeter += hole.perimeter;
    }
    if (!finiteValue(area) ||
        !finiteValue(perimeter) ||
        !(area > 0.0) ||
        !(perimeter > 0.0)) {
        return std::nullopt;
    }

    RegionCandidate2D result;
    result.outer =
        std::move(*valid_outer);
    result.holes =
        std::move(valid_holes);
    result.area = area;
    result.perimeter = perimeter;
    return result;
}

std::optional<Point2>
regionInteriorPoint(
    const SketchModel& model,
    const RegionCandidate2D& region) {
    return regionInteriorPointImpl(
        model,
        region);
}

RegionComposition2D composeRegionCells(
    const SketchModel& model,
    const std::vector<RegionCandidate2D>& cells) {
    return composeRegionCellsImpl(
        model,
        cells);
}

std::vector<std::uint32_t>
nestedIslandRegions(
    const SketchModel& model,
    const RegionAnalysis2D& analysis,
    const RegionCandidate2D& current) {
    std::vector<std::uint32_t> result;
    if (current.holes.empty()) {
        return result;
    }

    for (const auto& hole : current.holes) {
        struct NestedCandidate final {
            const RegionCandidate2D* region{};
            Point2 sample;
        };
        std::vector<NestedCandidate> nested;

        for (const auto& candidate : analysis.regions) {
            if (candidate.region_index ==
                current.region_index) {
                continue;
            }
            const auto sample =
                regionInteriorPointImpl(
                    model,
                    candidate);
            if (!sample) {
                continue;
            }
            const auto in_hole =
                pointInLoop(
                    model,
                    hole,
                    *sample);
            if (in_hole ==
                LoopPointState::inside) {
                nested.push_back(
                    NestedCandidate{
                        &candidate,
                        *sample});
            }
        }

        for (const auto& target : nested) {
            std::size_t depth = 0U;
            bool ambiguous = false;
            for (const auto& container : nested) {
                const auto state =
                    pointInLoop(
                        model,
                        container.region->outer,
                        target.sample);
                if (state ==
                        LoopPointState::boundary ||
                    state ==
                        LoopPointState::ambiguous) {
                    ambiguous = true;
                    break;
                }
                if (state ==
                    LoopPointState::inside) {
                    ++depth;
                }
            }

            if (!ambiguous &&
                depth > 0U &&
                depth % 2U == 0U) {
                result.push_back(
                    target.region->region_index);
            }
        }
    }

    std::sort(
        result.begin(),
        result.end());
    result.erase(
        std::unique(
            result.begin(),
            result.end()),
        result.end());
    return result;
}

RegionPick2D pickRegion(
    const SketchModel& model,
    const RegionAnalysis2D& analysis,
    Point2 point) {
    if (!point.finite()) {
        return RegionPick2D{
            RegionPointLocation::ambiguous,
            std::nullopt};
    }

    std::optional<std::uint32_t> found;

    for (const auto& region :
         analysis.regions) {
        const auto outer =
            pointInLoop(
                model,
                region.outer,
                point);
        if (outer ==
            LoopPointState::boundary) {
            return RegionPick2D{
                RegionPointLocation::boundary,
                std::nullopt};
        }
        if (outer ==
            LoopPointState::ambiguous) {
            return RegionPick2D{
                RegionPointLocation::ambiguous,
                std::nullopt};
        }
        if (outer !=
            LoopPointState::inside) {
            continue;
        }

        bool excluded_by_hole = false;
        for (const auto& hole :
             region.holes) {
            const auto state =
                pointInLoop(
                    model,
                    hole,
                    point);
            if (state ==
                LoopPointState::boundary) {
                return RegionPick2D{
                    RegionPointLocation::boundary,
                    std::nullopt};
            }
            if (state ==
                LoopPointState::ambiguous) {
                return RegionPick2D{
                    RegionPointLocation::ambiguous,
                    std::nullopt};
            }
            if (state ==
                LoopPointState::inside) {
                excluded_by_hole = true;
                break;
            }
        }
        if (excluded_by_hole) {
            continue;
        }

        if (found) {
            return RegionPick2D{
                RegionPointLocation::ambiguous,
                std::nullopt};
        }
        found = region.region_index;
    }

    if (!found) {
        return RegionPick2D{
            RegionPointLocation::outside,
            std::nullopt};
    }

    return RegionPick2D{
        RegionPointLocation::inside,
        found};
}

} // namespace simplesolid2::sketch
