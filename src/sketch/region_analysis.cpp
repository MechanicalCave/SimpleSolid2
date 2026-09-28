#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
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
    const double du =
        line.second.u - line.first.u;
    const double dv =
        line.second.v - line.first.v;
    if (!finiteValue(du) ||
        !finiteValue(dv) ||
        !finiteValue(parameter)) {
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
    return std::fma(au, bv, -av * bu);
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
        return filterSecondArc(
            lineCircle(first, support),
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

} // namespace simplesolid2::sketch
