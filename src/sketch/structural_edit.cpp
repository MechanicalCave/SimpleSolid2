#include <simplesolid2/sketch/structural_edit.hpp>

#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <set>
#include <utility>
#include <vector>

namespace simplesolid2::sketch {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;

struct CutLocation final {
    double parameter{};
    Point2 point;
};

enum class SupportRelationStatus {
    valid,
    disjoint,
    overlap,
    invalid,
};

struct SupportIntersection final {
    Point2 point;
    double line_parameter{};
};

struct SupportIntersections final {
    SupportRelationStatus status{
        SupportRelationStatus::invalid};
    std::vector<SupportIntersection> intersections;
};

[[nodiscard]] double normalizedAngle(
    double angle) noexcept {
    if (!std::isfinite(angle)) {
        return angle;
    }
    double result = std::fmod(angle, full_turn);
    if (result < 0.0) {
        result += full_turn;
    }
    if (result == full_turn) {
        result = 0.0;
    }
    return result;
}

[[nodiscard]] double cross(
    double au,
    double av,
    double bu,
    double bv) noexcept {
    // Compensated difference of products. This mirrors the Shared-2D
    // relation analyzer and preserves exact algebraic zero for bit-identical
    // parallel vectors without introducing a Product tolerance.
    const double second_product = av * bu;
    const double second_error =
        std::fma(-av, bu, second_product);
    const double difference =
        std::fma(au, bv, -second_product);
    return difference + second_error;
}

[[nodiscard]] std::optional<Point2> linePoint(
    const Line& line,
    double parameter) noexcept {
    if (!std::isfinite(parameter)) {
        return std::nullopt;
    }
    if (parameter == 0.0) {
        return line.start();
    }
    if (parameter == 1.0) {
        return line.end();
    }

    const double du =
        line.end().u - line.start().u;
    const double dv =
        line.end().v - line.start().v;
    const Point2 result{
        std::fma(parameter, du, line.start().u),
        std::fma(parameter, dv, line.start().v)};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<double> lineParameter(
    const Line& line,
    Point2 point) noexcept {
    if (!point.finite()) {
        return std::nullopt;
    }
    if (point == line.start()) {
        return 0.0;
    }
    if (point == line.end()) {
        return 1.0;
    }

    const double du =
        line.end().u - line.start().u;
    const double dv =
        line.end().v - line.start().v;
    double result{};
    if (std::abs(du) >= std::abs(dv)) {
        if (du == 0.0) {
            return std::nullopt;
        }
        result =
            (point.u - line.start().u) / du;
    } else {
        if (dv == 0.0) {
            return std::nullopt;
        }
        result =
            (point.v - line.start().v) / dv;
    }
    return std::isfinite(result)
        ? std::optional<double>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<double> circleParameter(
    Point2 center,
    Point2 point) noexcept {
    if (!center.finite() || !point.finite() ||
        point == center) {
        return std::nullopt;
    }
    const double angle = normalizedAngle(
        std::atan2(
            point.v - center.v,
            point.u - center.u));
    if (!std::isfinite(angle)) {
        return std::nullopt;
    }
    return angle / full_turn;
}

[[nodiscard]] std::optional<double> arcParameter(
    const Arc& arc,
    Point2 point) noexcept {
    const auto parameter =
        circleParameter(arc.center(), point);
    if (!parameter) {
        return std::nullopt;
    }

    const double angle =
        *parameter * full_turn;
    const double start =
        normalizedAngle(arc.startAngle());
    if (!std::isfinite(start)) {
        return std::nullopt;
    }

    if (arc.sweepAngle() > 0.0) {
        const double delta =
            normalizedAngle(angle - start);
        if (!std::isfinite(delta) ||
            delta > arc.sweepAngle()) {
            return std::nullopt;
        }
        return delta / arc.sweepAngle();
    }

    const double delta =
        normalizedAngle(start - angle);
    const double magnitude =
        -arc.sweepAngle();
    if (!std::isfinite(delta) ||
        delta > magnitude) {
        return std::nullopt;
    }
    return delta / magnitude;
}

[[nodiscard]] bool modelContainsSupported(
    const SketchModel& model,
    EntityId id) noexcept {
    return model.findLine(id) != nullptr ||
           model.findCircle(id) != nullptr ||
           model.findArc(id) != nullptr;
}

[[nodiscard]] bool validBoundaries(
    const SketchModel& model,
    EntityId target,
    const std::vector<EntityId>& boundaries) {
    if (!target.valid() ||
        boundaries.empty() ||
        !modelContainsSupported(model, target)) {
        return false;
    }

    for (const auto boundary : boundaries) {
        if (!boundary.valid() ||
            boundary == target ||
            !modelContainsSupported(model, boundary)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::vector<EntityId>
uniqueBoundaries(
    const std::vector<EntityId>& boundaries) {
    std::vector<EntityId> result;
    result.reserve(boundaries.size());
    std::set<EntityId> seen;
    for (const auto id : boundaries) {
        if (seen.insert(id).second) {
            result.push_back(id);
        }
    }
    return result;
}

[[nodiscard]] StructuralEditStatus collectCuts(
    const SketchModel& model,
    EntityId target,
    const std::vector<EntityId>& boundaries,
    std::vector<CutLocation>& cuts) {
    for (const auto boundary : uniqueBoundaries(boundaries)) {
        const auto relation =
            analyzeCurveRelation(
                model,
                target,
                boundary);
        if (relation.status ==
            CurveRelationStatus::invalid) {
            return StructuralEditStatus::invalid_request;
        }
        if (relation.status ==
            CurveRelationStatus::overlap) {
            return StructuralEditStatus::ambiguous_topology;
        }
        if (relation.status !=
            CurveRelationStatus::discrete) {
            continue;
        }

        for (const auto& intersection :
             relation.intersections) {
            const double parameter =
                relation.first_entity == target
                    ? intersection.first_parameter
                    : intersection.second_parameter;
            if (!std::isfinite(parameter)) {
                return StructuralEditStatus::invalid_request;
            }

            const auto duplicate =
                std::find_if(
                    cuts.begin(),
                    cuts.end(),
                    [&](const CutLocation& item) {
                        return item.parameter == parameter &&
                               item.point == intersection.point;
                    });
            if (duplicate == cuts.end()) {
                cuts.push_back(
                    CutLocation{
                        parameter,
                        intersection.point});
            }
        }
    }

    std::sort(
        cuts.begin(),
        cuts.end(),
        [](const CutLocation& lhs,
           const CutLocation& rhs) {
            if (lhs.parameter != rhs.parameter) {
                return lhs.parameter < rhs.parameter;
            }
            if (lhs.point.u != rhs.point.u) {
                return lhs.point.u < rhs.point.u;
            }
            return lhs.point.v < rhs.point.v;
        });

    return cuts.empty()
        ? StructuralEditStatus::no_intersection
        : StructuralEditStatus::ready;
}

[[nodiscard]] std::optional<SketchModel> copyModel(
    const SketchModel& model) {
    return SketchModel::restore(model.state());
}

[[nodiscard]] StructuralEditResult resultFromModel(
    SketchModel model,
    EntityId result_entity) {
    return StructuralEditResult{
        StructuralEditStatus::ready,
        model.state(),
        result_entity.valid()
            ? std::optional<EntityId>{result_entity}
            : std::nullopt};
}

[[nodiscard]] StructuralEditResult trimLine(
    const SketchModel& model,
    const Line& target,
    const std::vector<CutLocation>& all_cuts,
    Point2 pick) {
    auto pick_parameter =
        lineParameter(target, pick);
    if (!pick_parameter ||
        *pick_parameter < 0.0 ||
        *pick_parameter > 1.0) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    std::vector<CutLocation> cuts;
    for (const auto& cut : all_cuts) {
        if (cut.parameter > 0.0 &&
            cut.parameter < 1.0) {
            cuts.push_back(cut);
        }
    }
    if (cuts.empty()) {
        return {
            StructuralEditStatus::no_intersection,
            std::nullopt,
            std::nullopt};
    }

    for (const auto& cut : cuts) {
        if (*pick_parameter == cut.parameter) {
            return {
                StructuralEditStatus::not_applicable,
                std::nullopt,
                std::nullopt};
        }
    }

    auto edited = copyModel(model);
    if (!edited) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    if (*pick_parameter < cuts.front().parameter) {
        if (!edited->updateLine(
                target.id(),
                cuts.front().point,
                target.end())) {
            return {
                StructuralEditStatus::invalid_request,
                std::nullopt,
                std::nullopt};
        }
        return resultFromModel(
            std::move(*edited),
            target.id());
    }

    if (*pick_parameter > cuts.back().parameter) {
        if (!edited->updateLine(
                target.id(),
                target.start(),
                cuts.back().point)) {
            return {
                StructuralEditStatus::invalid_request,
                std::nullopt,
                std::nullopt};
        }
        return resultFromModel(
            std::move(*edited),
            target.id());
    }

    return {
        StructuralEditStatus::not_applicable,
        std::nullopt,
        std::nullopt};
}

[[nodiscard]] StructuralEditResult trimArc(
    const SketchModel& model,
    const Arc& target,
    const std::vector<CutLocation>& all_cuts,
    Point2 pick) {
    const auto pick_parameter =
        arcParameter(target, pick);
    if (!pick_parameter) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    std::vector<CutLocation> cuts;
    for (const auto& cut : all_cuts) {
        if (cut.parameter > 0.0 &&
            cut.parameter < 1.0) {
            cuts.push_back(cut);
        }
    }
    if (cuts.empty()) {
        return {
            StructuralEditStatus::no_intersection,
            std::nullopt,
            std::nullopt};
    }

    for (const auto& cut : cuts) {
        if (*pick_parameter == cut.parameter) {
            return {
                StructuralEditStatus::not_applicable,
                std::nullopt,
                std::nullopt};
        }
    }

    auto edited = copyModel(model);
    if (!edited) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    if (*pick_parameter < cuts.front().parameter) {
        const double start_angle =
            target.startAngle() +
            cuts.front().parameter *
                target.sweepAngle();
        const double sweep =
            (1.0 - cuts.front().parameter) *
            target.sweepAngle();
        if (!edited->updateArc(
                target.id(),
                target.center(),
                target.radius(),
                start_angle,
                sweep)) {
            return {
                StructuralEditStatus::invalid_request,
                std::nullopt,
                std::nullopt};
        }
        return resultFromModel(
            std::move(*edited),
            target.id());
    }

    if (*pick_parameter > cuts.back().parameter) {
        const double sweep =
            cuts.back().parameter *
            target.sweepAngle();
        if (!edited->updateArc(
                target.id(),
                target.center(),
                target.radius(),
                target.startAngle(),
                sweep)) {
            return {
                StructuralEditStatus::invalid_request,
                std::nullopt,
                std::nullopt};
        }
        return resultFromModel(
            std::move(*edited),
            target.id());
    }

    return {
        StructuralEditStatus::not_applicable,
        std::nullopt,
        std::nullopt};
}

[[nodiscard]] StructuralEditResult trimCircle(
    const SketchModel& model,
    const Circle& target,
    const std::vector<CutLocation>& all_cuts,
    Point2 pick) {
    const auto pick_parameter =
        circleParameter(target.center(), pick);
    if (!pick_parameter) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    std::vector<CutLocation> cuts;
    for (const auto& cut : all_cuts) {
        if (cut.parameter >= 0.0 &&
            cut.parameter < 1.0) {
            const auto duplicate =
                std::find_if(
                    cuts.begin(),
                    cuts.end(),
                    [&](const CutLocation& item) {
                        return item.parameter == cut.parameter &&
                               item.point == cut.point;
                    });
            if (duplicate == cuts.end()) {
                cuts.push_back(cut);
            }
        }
    }
    std::sort(
        cuts.begin(),
        cuts.end(),
        [](const CutLocation& lhs,
           const CutLocation& rhs) {
            return lhs.parameter < rhs.parameter;
        });

    if (cuts.size() < 2U) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    for (const auto& cut : cuts) {
        if (*pick_parameter == cut.parameter) {
            return {
                StructuralEditStatus::not_applicable,
                std::nullopt,
                std::nullopt};
        }
    }

    std::optional<std::size_t> interval;
    for (std::size_t index = 0U;
         index < cuts.size();
         ++index) {
        const double first =
            cuts[index].parameter;
        const double second =
            index + 1U < cuts.size()
                ? cuts[index + 1U].parameter
                : cuts.front().parameter + 1.0;
        double pick_value = *pick_parameter;
        if (index + 1U == cuts.size() &&
            pick_value < first) {
            pick_value += 1.0;
        }

        if (pick_value > first &&
            pick_value < second) {
            interval = index;
            break;
        }
    }

    if (!interval) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    const std::size_t first_index = *interval;
    const std::size_t next_index =
        (first_index + 1U) % cuts.size();
    const double removed_start =
        cuts[first_index].parameter;
    double removed_end =
        cuts[next_index].parameter;
    if (next_index == 0U) {
        removed_end += 1.0;
    }
    const double removed_span =
        removed_end - removed_start;
    const double surviving_fraction =
        1.0 - removed_span;
    const double sweep =
        surviving_fraction * full_turn;
    if (!std::isfinite(sweep) ||
        sweep <= 0.0 ||
        sweep >= full_turn) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    const double start_angle =
        cuts[next_index].parameter *
        full_turn;

    auto edited = copyModel(model);
    if (!edited || !edited->erase(target.id())) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    EntityId replacement;
    try {
        replacement =
            edited->addArc(
                target.center(),
                target.radius(),
                start_angle,
                sweep,
                target.role());
    } catch (const std::overflow_error&) {
        return {
            StructuralEditStatus::identity_exhausted,
            std::nullopt,
            std::nullopt};
    } catch (...) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    return resultFromModel(
        std::move(*edited),
        replacement);
}

[[nodiscard]] SupportIntersections lineSupportLine(
    const Line& support,
    const Line& boundary) {
    const double ru =
        support.end().u - support.start().u;
    const double rv =
        support.end().v - support.start().v;
    const double su =
        boundary.end().u - boundary.start().u;
    const double sv =
        boundary.end().v - boundary.start().v;
    const double qu =
        boundary.start().u - support.start().u;
    const double qv =
        boundary.start().v - support.start().v;

    const double scale = std::max(
        {std::abs(ru), std::abs(rv),
         std::abs(su), std::abs(sv),
         std::abs(qu), std::abs(qv)});
    if (!std::isfinite(scale) ||
        scale == 0.0) {
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
    if (!std::isfinite(denominator) ||
        !std::isfinite(collinearity)) {
        return {};
    }

    if (denominator == 0.0) {
        return SupportIntersections{
            collinearity == 0.0
                ? SupportRelationStatus::overlap
                : SupportRelationStatus::disjoint,
            {}};
    }

    const double target_parameter =
        cross(qn_u, qn_v, sn_u, sn_v) /
        denominator;
    const double boundary_parameter =
        cross(qn_u, qn_v, rn_u, rn_v) /
        denominator;
    if (!std::isfinite(target_parameter) ||
        !std::isfinite(boundary_parameter)) {
        return {};
    }
    if (boundary_parameter < 0.0 ||
        boundary_parameter > 1.0) {
        return {
            SupportRelationStatus::disjoint,
            {}};
    }

    const auto point =
        linePoint(support, target_parameter);
    if (!point) {
        return {};
    }
    return SupportIntersections{
        SupportRelationStatus::valid,
        {{*point, target_parameter}}};
}

[[nodiscard]] SupportIntersections lineSupportCircle(
    const Line& support,
    Point2 center,
    double radius) {
    const double du =
        support.end().u - support.start().u;
    const double dv =
        support.end().v - support.start().v;
    const double rel_u =
        center.u - support.start().u;
    const double rel_v =
        center.v - support.start().v;
    const double length =
        std::hypot(du, dv);
    if (!std::isfinite(length) ||
        length == 0.0 ||
        !center.finite() ||
        !std::isfinite(radius) ||
        radius <= 0.0) {
        return {};
    }

    const double unit_u = du / length;
    const double unit_v = dv / length;
    const double projection =
        std::fma(rel_u, unit_u, rel_v * unit_v);
    const double signed_perpendicular =
        cross(rel_u, rel_v, unit_u, unit_v);
    if (!std::isfinite(projection) ||
        !std::isfinite(signed_perpendicular)) {
        return {};
    }

    const double perpendicular =
        std::abs(signed_perpendicular);
    if (perpendicular > radius) {
        return {
            SupportRelationStatus::disjoint,
            {}};
    }

    const bool tangent =
        perpendicular == radius;
    double offset = 0.0;
    if (!tangent) {
        const double ratio =
            perpendicular / radius;
        const double remainder =
            std::fma(-ratio, ratio, 1.0);
        if (!std::isfinite(remainder) ||
            remainder < 0.0) {
            return {};
        }
        offset =
            radius * std::sqrt(remainder);
        if (!std::isfinite(offset)) {
            return {};
        }
    }

    const double base_parameter =
        projection / length;
    const double delta_parameter =
        offset / length;
    if (!std::isfinite(base_parameter) ||
        !std::isfinite(delta_parameter)) {
        return {};
    }

    SupportIntersections result{
        SupportRelationStatus::valid,
        {}};
    const auto append =
        [&](double parameter) {
            const auto point =
                linePoint(support, parameter);
            if (!point) {
                return false;
            }
            result.intersections.push_back(
                SupportIntersection{
                    *point,
                    parameter});
            return true;
        };

    if (!append(
            base_parameter -
            delta_parameter)) {
        return {};
    }
    if (!tangent &&
        !append(
            base_parameter +
            delta_parameter)) {
        return {};
    }
    return result;
}

[[nodiscard]] bool pointOnArc(
    const Arc& arc,
    Point2 point) noexcept {
    const auto parameter =
        arcParameter(arc, point);
    return parameter.has_value() &&
           *parameter >= 0.0 &&
           *parameter <= 1.0;
}

[[nodiscard]] SupportIntersections
lineSupportBoundary(
    const SketchModel& model,
    const Line& support,
    EntityId boundary_id) {
    if (const auto* line =
            model.findLine(boundary_id)) {
        return lineSupportLine(
            support,
            *line);
    }
    if (const auto* circle =
            model.findCircle(boundary_id)) {
        return lineSupportCircle(
            support,
            circle->center(),
            circle->radius());
    }
    if (const auto* arc =
            model.findArc(boundary_id)) {
        auto result =
            lineSupportCircle(
                support,
                arc->center(),
                arc->radius());
        if (result.status !=
            SupportRelationStatus::valid) {
            return result;
        }
        result.intersections.erase(
            std::remove_if(
                result.intersections.begin(),
                result.intersections.end(),
                [&](const SupportIntersection& item) {
                    return !pointOnArc(
                        *arc,
                        item.point);
                }),
            result.intersections.end());
        if (result.intersections.empty()) {
            result.status =
                SupportRelationStatus::disjoint;
        }
        return result;
    }
    return {};
}

struct CircleSupportIntersection final {
    Point2 point;
};

struct CircleSupportIntersections final {
    SupportRelationStatus status{
        SupportRelationStatus::invalid};
    std::vector<CircleSupportIntersection>
        intersections;
};

[[nodiscard]] CircleSupportIntersections
circleSupportLine(
    Point2 center,
    double radius,
    const Line& boundary) {
    // Reuse the conditioned line/circle construction with the finite boundary
    // as the line, then keep only roots on that authored segment.
    auto roots =
        lineSupportCircle(
            boundary,
            center,
            radius);
    if (roots.status !=
        SupportRelationStatus::valid) {
        return {
            roots.status,
            {}};
    }

    CircleSupportIntersections result{
        SupportRelationStatus::valid,
        {}};
    for (const auto& root : roots.intersections) {
        if (root.line_parameter >= 0.0 &&
            root.line_parameter <= 1.0) {
            result.intersections.push_back(
                {root.point});
        }
    }
    if (result.intersections.empty()) {
        result.status =
            SupportRelationStatus::disjoint;
    }
    return result;
}

[[nodiscard]] CircleSupportIntersections
circleSupportCircle(
    Point2 first_center,
    double first_radius,
    Point2 second_center,
    double second_radius) {
    if (first_center == second_center &&
        first_radius == second_radius) {
        return {
            SupportRelationStatus::overlap,
            {}};
    }

    const double dx =
        second_center.u - first_center.u;
    const double dy =
        second_center.v - first_center.v;
    const double distance =
        std::hypot(dx, dy);
    if (!std::isfinite(distance) ||
        !std::isfinite(first_radius) ||
        !std::isfinite(second_radius) ||
        first_radius <= 0.0 ||
        second_radius <= 0.0) {
        return {};
    }
    if (distance == 0.0) {
        return {
            SupportRelationStatus::disjoint,
            {}};
    }

    const double scale = std::max(
        {distance,
         first_radius,
         second_radius});
    if (!std::isfinite(scale) ||
        scale <= 0.0) {
        return {};
    }

    const double d = distance / scale;
    const double r1 = first_radius / scale;
    const double r2 = second_radius / scale;
    const double sum = r1 + r2;
    const double difference =
        std::abs(r1 - r2);
    if (d > sum || d < difference) {
        return {
            SupportRelationStatus::disjoint,
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
    if (!std::isfinite(along_normalized)) {
        return {};
    }

    double height_normalized = 0.0;
    if (!tangent) {
        const double height_squared =
            std::fma(
                -along_normalized,
                along_normalized,
                r1 * r1);
        if (!std::isfinite(height_squared) ||
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
    const Point2 base{
        std::fma(
            along,
            unit_u,
            first_center.u),
        std::fma(
            along,
            unit_v,
            first_center.v)};
    if (!base.finite() ||
        !std::isfinite(height)) {
        return {};
    }

    CircleSupportIntersections result{
        SupportRelationStatus::valid,
        {}};
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
            if (!point.finite()) {
                return false;
            }
            result.intersections.push_back(
                {point});
            return true;
        };

    if (!append(1.0)) {
        return {};
    }
    if (!tangent && !append(-1.0)) {
        return {};
    }
    return result;
}

[[nodiscard]] CircleSupportIntersections
circleSupportBoundary(
    const SketchModel& model,
    Point2 center,
    double radius,
    EntityId boundary_id) {
    if (const auto* line =
            model.findLine(boundary_id)) {
        return circleSupportLine(
            center,
            radius,
            *line);
    }
    if (const auto* circle =
            model.findCircle(boundary_id)) {
        return circleSupportCircle(
            center,
            radius,
            circle->center(),
            circle->radius());
    }
    if (const auto* arc =
            model.findArc(boundary_id)) {
        auto result =
            circleSupportCircle(
                center,
                radius,
                arc->center(),
                arc->radius());
        if (result.status !=
            SupportRelationStatus::valid) {
            return result;
        }
        result.intersections.erase(
            std::remove_if(
                result.intersections.begin(),
                result.intersections.end(),
                [&](const CircleSupportIntersection& item) {
                    return !pointOnArc(
                        *arc,
                        item.point);
                }),
            result.intersections.end());
        if (result.intersections.empty()) {
            result.status =
                SupportRelationStatus::disjoint;
        }
        return result;
    }
    return {};
}

[[nodiscard]] double orientedDelta(
    double from,
    double to,
    double orientation) noexcept {
    return orientation > 0.0
        ? normalizedAngle(to - from)
        : normalizedAngle(from - to);
}

struct ExtendCandidate final {
    Point2 point;
    double travel{};
};

[[nodiscard]] bool appendUniqueCandidate(
    std::vector<ExtendCandidate>& candidates,
    ExtendCandidate candidate) {
    if (!candidate.point.finite() ||
        !std::isfinite(candidate.travel) ||
        candidate.travel <= 0.0) {
        return false;
    }
    const auto duplicate =
        std::find_if(
            candidates.begin(),
            candidates.end(),
            [&](const ExtendCandidate& item) {
                return item.point == candidate.point;
            });
    if (duplicate == candidates.end()) {
        candidates.push_back(candidate);
    }
    return true;
}

[[nodiscard]] StructuralEditResult extendLine(
    const SketchModel& model,
    const Line& target,
    const ExtendSketchRequest& request) {
    std::vector<ExtendCandidate> candidates;

    for (const auto boundary :
         uniqueBoundaries(request.boundaries)) {
        const auto relation =
            lineSupportBoundary(
                model,
                target,
                boundary);
        if (relation.status ==
            SupportRelationStatus::invalid) {
            return {
                StructuralEditStatus::invalid_request,
                std::nullopt,
                std::nullopt};
        }
        if (relation.status ==
            SupportRelationStatus::overlap) {
            return {
                StructuralEditStatus::ambiguous_topology,
                std::nullopt,
                std::nullopt};
        }
        if (relation.status !=
            SupportRelationStatus::valid) {
            continue;
        }

        for (const auto& intersection :
             relation.intersections) {
            double travel{};
            if (request.endpoint ==
                StructuralEndpointRole::start) {
                if (intersection.line_parameter >= 0.0) {
                    continue;
                }
                travel =
                    -intersection.line_parameter;
            } else {
                if (intersection.line_parameter <= 1.0) {
                    continue;
                }
                travel =
                    intersection.line_parameter - 1.0;
            }
            static_cast<void>(
                appendUniqueCandidate(
                    candidates,
                    ExtendCandidate{
                        intersection.point,
                        travel}));
        }
    }

    if (candidates.empty()) {
        return {
            StructuralEditStatus::no_intersection,
            std::nullopt,
            std::nullopt};
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const ExtendCandidate& lhs,
           const ExtendCandidate& rhs) {
            if (lhs.travel != rhs.travel) {
                return lhs.travel < rhs.travel;
            }
            if (lhs.point.u != rhs.point.u) {
                return lhs.point.u < rhs.point.u;
            }
            return lhs.point.v < rhs.point.v;
        });

    if (candidates.size() > 1U &&
        candidates[0].travel ==
            candidates[1].travel &&
        candidates[0].point !=
            candidates[1].point) {
        return {
            StructuralEditStatus::ambiguous_topology,
            std::nullopt,
            std::nullopt};
    }

    auto edited = copyModel(model);
    if (!edited) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    const auto& point = candidates.front().point;
    const bool changed =
        request.endpoint ==
            StructuralEndpointRole::start
            ? edited->updateLine(
                  target.id(),
                  point,
                  target.end())
            : edited->updateLine(
                  target.id(),
                  target.start(),
                  point);
    if (!changed) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    return resultFromModel(
        std::move(*edited),
        target.id());
}

[[nodiscard]] StructuralEditResult extendArc(
    const SketchModel& model,
    const Arc& target,
    const ExtendSketchRequest& request) {
    std::vector<ExtendCandidate> candidates;
    const double orientation =
        target.sweepAngle() > 0.0
            ? 1.0
            : -1.0;
    const double start_angle =
        normalizedAngle(target.startAngle());
    const double end_angle =
        normalizedAngle(
            target.startAngle() +
            target.sweepAngle());
    if (!std::isfinite(start_angle) ||
        !std::isfinite(end_angle)) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    for (const auto boundary :
         uniqueBoundaries(request.boundaries)) {
        const auto relation =
            circleSupportBoundary(
                model,
                target.center(),
                target.radius(),
                boundary);
        if (relation.status ==
            SupportRelationStatus::invalid) {
            return {
                StructuralEditStatus::invalid_request,
                std::nullopt,
                std::nullopt};
        }
        if (relation.status ==
            SupportRelationStatus::overlap) {
            return {
                StructuralEditStatus::ambiguous_topology,
                std::nullopt,
                std::nullopt};
        }
        if (relation.status !=
            SupportRelationStatus::valid) {
            continue;
        }

        for (const auto& intersection :
             relation.intersections) {
            const auto parameter =
                circleParameter(
                    target.center(),
                    intersection.point);
            if (!parameter) {
                return {
                    StructuralEditStatus::invalid_request,
                    std::nullopt,
                    std::nullopt};
            }
            const double angle =
                *parameter * full_turn;
            const double travel =
                request.endpoint ==
                    StructuralEndpointRole::end
                    ? orientedDelta(
                          end_angle,
                          angle,
                          orientation)
                    : orientedDelta(
                          start_angle,
                          angle,
                          -orientation);
            if (!std::isfinite(travel) ||
                travel <= 0.0) {
                continue;
            }

            const double new_magnitude =
                std::abs(target.sweepAngle()) +
                travel;
            if (!std::isfinite(new_magnitude) ||
                new_magnitude >= full_turn) {
                continue;
            }

            static_cast<void>(
                appendUniqueCandidate(
                    candidates,
                    ExtendCandidate{
                        intersection.point,
                        travel}));
        }
    }

    if (candidates.empty()) {
        return {
            StructuralEditStatus::no_intersection,
            std::nullopt,
            std::nullopt};
    }

    std::sort(
        candidates.begin(),
        candidates.end(),
        [](const ExtendCandidate& lhs,
           const ExtendCandidate& rhs) {
            if (lhs.travel != rhs.travel) {
                return lhs.travel < rhs.travel;
            }
            if (lhs.point.u != rhs.point.u) {
                return lhs.point.u < rhs.point.u;
            }
            return lhs.point.v < rhs.point.v;
        });

    if (candidates.size() > 1U &&
        candidates[0].travel ==
            candidates[1].travel &&
        candidates[0].point !=
            candidates[1].point) {
        return {
            StructuralEditStatus::ambiguous_topology,
            std::nullopt,
            std::nullopt};
    }

    const auto chosen_parameter =
        circleParameter(
            target.center(),
            candidates.front().point);
    if (!chosen_parameter) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }
    const double chosen_angle =
        *chosen_parameter * full_turn;
    const double travel =
        candidates.front().travel;
    const double new_sweep =
        target.sweepAngle() +
        orientation * travel;
    const double new_start =
        request.endpoint ==
            StructuralEndpointRole::start
            ? chosen_angle
            : target.startAngle();

    auto edited = copyModel(model);
    if (!edited ||
        !edited->updateArc(
            target.id(),
            target.center(),
            target.radius(),
            new_start,
            new_sweep)) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    return resultFromModel(
        std::move(*edited),
        target.id());
}

} // namespace

StructuralEditResult evaluateTrim(
    const SketchModel& model,
    const TrimSketchRequest& request) {
    if (!request.pick.finite() ||
        !validBoundaries(
            model,
            request.target,
            request.boundaries)) {
        return {
            !modelContainsSupported(
                 model,
                 request.target)
                ? StructuralEditStatus::missing_entity
                : StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    std::vector<CutLocation> cuts;
    const auto cuts_status =
        collectCuts(
            model,
            request.target,
            request.boundaries,
            cuts);
    if (cuts_status !=
        StructuralEditStatus::ready) {
        return {
            cuts_status,
            std::nullopt,
            std::nullopt};
    }

    if (const auto* line =
            model.findLine(request.target)) {
        return trimLine(
            model,
            *line,
            cuts,
            request.pick);
    }
    if (const auto* arc =
            model.findArc(request.target)) {
        return trimArc(
            model,
            *arc,
            cuts,
            request.pick);
    }
    if (const auto* circle =
            model.findCircle(request.target)) {
        return trimCircle(
            model,
            *circle,
            cuts,
            request.pick);
    }

    return {
        StructuralEditStatus::unsupported,
        std::nullopt,
        std::nullopt};
}

StructuralEditResult evaluateExtend(
    const SketchModel& model,
    const ExtendSketchRequest& request) {
    if (!validBoundaries(
            model,
            request.target,
            request.boundaries)) {
        return {
            !modelContainsSupported(
                 model,
                 request.target)
                ? StructuralEditStatus::missing_entity
                : StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    if (const auto* line =
            model.findLine(request.target)) {
        return extendLine(
            model,
            *line,
            request);
    }
    if (const auto* arc =
            model.findArc(request.target)) {
        return extendArc(
            model,
            *arc,
            request);
    }
    if (model.findCircle(request.target)) {
        return {
            StructuralEditStatus::unsupported,
            std::nullopt,
            std::nullopt};
    }

    return {
        StructuralEditStatus::missing_entity,
        std::nullopt,
        std::nullopt};
}

StructuralEditResult evaluateExtendBoth(
    const SketchModel& model,
    const ExtendBothLinesRequest& request) {
    if (!request.first_line.valid() ||
        !request.second_line.valid() ||
        request.first_line ==
            request.second_line) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    const auto* first =
        model.findLine(request.first_line);
    const auto* second =
        model.findLine(request.second_line);
    if (!first || !second) {
        return {
            StructuralEditStatus::missing_entity,
            std::nullopt,
            std::nullopt};
    }

    const auto relation =
        lineSupportLine(
            *first,
            *second);
    if (relation.status ==
        SupportRelationStatus::overlap) {
        return {
            StructuralEditStatus::ambiguous_topology,
            std::nullopt,
            std::nullopt};
    }
    if (relation.status !=
            SupportRelationStatus::valid ||
        relation.intersections.size() != 1U) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    const auto& intersection =
        relation.intersections.front();
    const auto second_parameter =
        lineParameter(
            *second,
            intersection.point);
    if (!second_parameter) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    const double first_parameter =
        intersection.line_parameter;
    if (first_parameter >= 0.0 &&
        first_parameter <= 1.0) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }
    if (*second_parameter >= 0.0 &&
        *second_parameter <= 1.0) {
        return {
            StructuralEditStatus::not_applicable,
            std::nullopt,
            std::nullopt};
    }

    auto edited = copyModel(model);
    if (!edited) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    const Point2 x = intersection.point;
    const bool first_changed =
        first_parameter < 0.0
            ? edited->updateLine(
                  first->id(),
                  x,
                  first->end())
            : edited->updateLine(
                  first->id(),
                  first->start(),
                  x);
    const bool second_changed =
        *second_parameter < 0.0
            ? edited->updateLine(
                  second->id(),
                  x,
                  second->end())
            : edited->updateLine(
                  second->id(),
                  second->start(),
                  x);

    if (!first_changed || !second_changed) {
        return {
            StructuralEditStatus::invalid_request,
            std::nullopt,
            std::nullopt};
    }

    return StructuralEditResult{
        StructuralEditStatus::ready,
        edited->state(),
        std::nullopt};
}

} // namespace simplesolid2::sketch
