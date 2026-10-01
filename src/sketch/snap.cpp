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

bool DeferredSnapReference::valid() const noexcept {
    if (!source.valid()) {
        return false;
    }

    switch (kind) {
    case DeferredSnapReferenceKind::line_extension:
        return source.kind ==
                   SnapSourceKind::entity_point &&
               (source.role ==
                    SnapSemanticRole::line_start ||
                source.role ==
                    SnapSemanticRole::line_end) &&
               source.first_entity.has_value() &&
               !source.second_entity.has_value();

    case DeferredSnapReferenceKind::tangent_curve:
        return source.kind ==
                   SnapSourceKind::entity_curve &&
               source.role ==
                   SnapSemanticRole::curve_tangent &&
               source.first_entity.has_value() &&
               !source.second_entity.has_value();
    }
    return false;
}

bool LineExtensionRay::valid() const noexcept {
    if (!reference.valid() ||
        reference.kind !=
            DeferredSnapReferenceKind::
                line_extension ||
        !origin.finite() ||
        !direction.finite()) {
        return false;
    }

    const double length =
        std::hypot(
            direction.u,
            direction.v);
    return std::isfinite(length) &&
           length > 0.0;
}

std::optional<DeferredSnapReference>
makeLineExtensionReference(
    const SketchModel& model,
    EntityId line,
    SnapSemanticRole endpoint_role) noexcept {
    if (!line.valid() ||
        (endpoint_role !=
             SnapSemanticRole::line_start &&
         endpoint_role !=
             SnapSemanticRole::line_end) ||
        model.findLine(line) == nullptr) {
        return std::nullopt;
    }

    DeferredSnapReference result{
        DeferredSnapReferenceKind::
            line_extension,
        {
            SnapSourceKind::entity_point,
            line,
            std::nullopt,
            endpoint_role,
            0U}};
    return result.valid()
        ? std::optional<DeferredSnapReference>{
              result}
        : std::nullopt;
}

std::optional<DeferredSnapReference>
makeTangentCurveReference(
    const SketchModel& model,
    EntityId entity) noexcept {
    if (!entity.valid() ||
        (model.findCircle(entity) == nullptr &&
         model.findArc(entity) == nullptr)) {
        return std::nullopt;
    }

    DeferredSnapReference result{
        DeferredSnapReferenceKind::
            tangent_curve,
        {
            SnapSourceKind::entity_curve,
            entity,
            std::nullopt,
            SnapSemanticRole::curve_tangent,
            0U}};
    return result.valid()
        ? std::optional<DeferredSnapReference>{
              result}
        : std::nullopt;
}

std::optional<LineExtensionRay>
lineExtensionRay(
    const SketchModel& model,
    const DeferredSnapReference& reference) noexcept {
    if (!reference.valid() ||
        reference.kind !=
            DeferredSnapReferenceKind::
                line_extension ||
        !reference.source.first_entity) {
        return std::nullopt;
    }

    const auto* line =
        model.findLine(
            *reference.source.first_entity);
    if (line == nullptr) {
        return std::nullopt;
    }

    LineExtensionRay result;
    result.reference = reference;
    if (reference.source.role ==
        SnapSemanticRole::line_start) {
        result.origin = line->start();
        result.direction = {
            line->start().u - line->end().u,
            line->start().v - line->end().v};
    } else if (
        reference.source.role ==
        SnapSemanticRole::line_end) {
        result.origin = line->end();
        result.direction = {
            line->end().u - line->start().u,
            line->end().v - line->start().v};
    } else {
        return std::nullopt;
    }

    return result.valid()
        ? std::optional<LineExtensionRay>{
              result}
        : std::nullopt;
}

namespace {

[[nodiscard]] std::optional<Point2>
projectPointToPositiveRay(
    const LineExtensionRay& ray,
    Point2 point) noexcept {
    if (!ray.valid() || !point.finite()) {
        return std::nullopt;
    }

    const double length_squared =
        ray.direction.u *
            ray.direction.u +
        ray.direction.v *
            ray.direction.v;
    if (!std::isfinite(length_squared) ||
        !(length_squared > 0.0)) {
        return std::nullopt;
    }

    const double relative_u =
        point.u - ray.origin.u;
    const double relative_v =
        point.v - ray.origin.v;
    const double parameter =
        (relative_u * ray.direction.u +
         relative_v * ray.direction.v) /
        length_squared;
    if (!std::isfinite(parameter) ||
        !(parameter > 0.0)) {
        return std::nullopt;
    }

    Point2 result{
        std::fma(
            parameter,
            ray.direction.u,
            ray.origin.u),
        std::fma(
            parameter,
            ray.direction.v,
            ray.origin.v)};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

} // namespace

std::optional<Point2>
projectPointToLineExtension(
    const SketchModel& model,
    const DeferredSnapReference& reference,
    Point2 pointer) noexcept {
    const auto ray =
        lineExtensionRay(
            model,
            reference);
    return ray
        ? projectPointToPositiveRay(
              *ray,
              pointer)
        : std::nullopt;
}

std::optional<Point2>
perpendicularPointOnLineExtension(
    const SketchModel& model,
    const DeferredSnapReference& reference,
    Point2 base) noexcept {
    const auto ray =
        lineExtensionRay(
            model,
            reference);
    return ray
        ? projectPointToPositiveRay(
              *ray,
              base)
        : std::nullopt;
}

namespace {

struct TangentCurveGeometry final {
    EntityId id;
    Point2 center;
    double radius{};
    const Arc* arc{};

    [[nodiscard]] bool valid() const noexcept {
        return id.valid() &&
               center.finite() &&
               std::isfinite(radius) &&
               radius > 0.0;
    }
};

[[nodiscard]] std::optional<TangentCurveGeometry>
tangentCurveGeometry(
    const SketchModel& model,
    const DeferredSnapReference& reference) noexcept {
    if (!reference.valid() ||
        reference.kind !=
            DeferredSnapReferenceKind::
                tangent_curve ||
        !reference.source.first_entity) {
        return std::nullopt;
    }

    const EntityId id =
        *reference.source.first_entity;
    if (const auto* circle =
            model.findCircle(id)) {
        TangentCurveGeometry result{
            id,
            circle->center(),
            circle->radius(),
            nullptr};
        return result.valid()
            ? std::optional<TangentCurveGeometry>{
                  result}
            : std::nullopt;
    }

    if (const auto* arc =
            model.findArc(id)) {
        TangentCurveGeometry result{
            id,
            arc->center(),
            arc->radius(),
            arc};
        return result.valid()
            ? std::optional<TangentCurveGeometry>{
                  result}
            : std::nullopt;
    }

    return std::nullopt;
}

[[nodiscard]] bool tangentContactAllowed(
    const TangentCurveGeometry& geometry,
    Point2 point) noexcept {
    return geometry.arc == nullptr ||
           arcContainsPointDirection(
               *geometry.arc,
               point);
}

} // namespace

bool CommonTangentCandidate::valid() const noexcept {
    return first_reference.valid() &&
           second_reference.valid() &&
           first_reference.kind ==
               DeferredSnapReferenceKind::
                   tangent_curve &&
           second_reference.kind ==
               DeferredSnapReferenceKind::
                   tangent_curve &&
           first_reference.source.first_entity &&
           second_reference.source.first_entity &&
           *first_reference.source.first_entity !=
               *second_reference.source.first_entity &&
           first_point.finite() &&
           second_point.finite() &&
           first_point != second_point &&
           canonical_branch < 4U;
}

SnapCandidate CommonTangentCandidate::
firstSnapCandidate() const noexcept {
    auto source = first_reference.source;
    source.canonical_branch = canonical_branch;
    return {
        first_point,
        SnapKind::tangent,
        source};
}

SnapCandidate CommonTangentCandidate::
secondSnapCandidate() const noexcept {
    auto source = second_reference.source;
    source.canonical_branch = canonical_branch;
    return {
        second_point,
        SnapKind::tangent,
        source};
}

std::vector<CommonTangentCandidate>
commonTangentCandidates(
    const SketchModel& model,
    const DeferredSnapReference& first,
    const DeferredSnapReference& second) {
    const auto first_geometry =
        tangentCurveGeometry(model, first);
    const auto second_geometry =
        tangentCurveGeometry(model, second);
    if (!first_geometry ||
        !second_geometry ||
        first_geometry->id ==
            second_geometry->id) {
        return {};
    }

    const double dx =
        second_geometry->center.u -
        first_geometry->center.u;
    const double dy =
        second_geometry->center.v -
        first_geometry->center.v;
    const double distance_squared =
        dx * dx + dy * dy;
    if (!std::isfinite(distance_squared) ||
        !(distance_squared > 0.0)) {
        return {};
    }

    std::vector<CommonTangentCandidate> result;
    result.reserve(4U);

    // Signed-radius formulation. second_sign=+1 yields the two external
    // tangents, second_sign=-1 yields the two internal tangents.
    for (const double second_sign :
         {1.0, -1.0}) {
        const CommonTangentFamily family =
            second_sign > 0.0
                ? CommonTangentFamily::external
                : CommonTangentFamily::internal;
        const double radius_delta =
            first_geometry->radius -
            second_sign *
                second_geometry->radius;
        const double height_squared =
            distance_squared -
            radius_delta * radius_delta;

        if (!std::isfinite(height_squared) ||
            height_squared < 0.0) {
            continue;
        }

        const double height =
            std::sqrt(height_squared);
        const std::array<double, 2U> sides{
            -1.0,
            1.0};
        const std::size_t side_count =
            height == 0.0 ? 1U : 2U;

        for (std::size_t side_index = 0U;
             side_index < side_count;
             ++side_index) {
            const double side =
                sides[side_index];

            const double normal_u =
                (dx * radius_delta -
                 dy * height * side) /
                distance_squared;
            const double normal_v =
                (dy * radius_delta +
                 dx * height * side) /
                distance_squared;

            const Point2 first_point{
                std::fma(
                    first_geometry->radius,
                    normal_u,
                    first_geometry->center.u),
                std::fma(
                    first_geometry->radius,
                    normal_v,
                    first_geometry->center.v)};
            const Point2 second_point{
                std::fma(
                    second_sign *
                        second_geometry->radius,
                    normal_u,
                    second_geometry->center.u),
                std::fma(
                    second_sign *
                        second_geometry->radius,
                    normal_v,
                    second_geometry->center.v)};

            if (!first_point.finite() ||
                !second_point.finite() ||
                first_point == second_point ||
                !tangentContactAllowed(
                    *first_geometry,
                    first_point) ||
                !tangentContactAllowed(
                    *second_geometry,
                    second_point)) {
                continue;
            }

            const CommonTangentSide tangent_side =
                side < 0.0
                    ? CommonTangentSide::negative
                    : CommonTangentSide::positive;
            const std::uint32_t family_index =
                family ==
                        CommonTangentFamily::external
                    ? 0U
                    : 1U;
            const std::uint32_t side_bit =
                tangent_side ==
                        CommonTangentSide::positive
                    ? 1U
                    : 0U;

            CommonTangentCandidate candidate{
                first,
                second,
                first_point,
                second_point,
                family,
                tangent_side,
                family_index * 2U +
                    side_bit};
            if (candidate.valid()) {
                result.push_back(
                    std::move(candidate));
            }
        }
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const CommonTangentCandidate& first,
           const CommonTangentCandidate& second) {
            return first.canonical_branch <
                   second.canonical_branch;
        });
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

bool sameSnapContact(
    const SketchModel& model,
    const SnapCandidate& first,
    const SnapCandidate& second) {
    if (!first.valid() ||
        !second.valid() ||
        first.point != second.point) {
        return false;
    }

    if (snapStableKey(first) ==
        snapStableKey(second)) {
        return true;
    }

    const bool first_origin =
        first.source.kind ==
        SnapSourceKind::intrinsic_origin;
    const bool second_origin =
        second.source.kind ==
        SnapSourceKind::intrinsic_origin;
    if (first_origin || second_origin) {
        return first.point == Point2{0.0, 0.0};
    }

    const auto source_entities =
        [](const SnapSourceRef& source) {
            std::array<std::optional<EntityId>, 2U>
                ids{
                    source.first_entity,
                    source.second_entity};
            return ids;
        };

    const auto first_entities =
        source_entities(first.source);
    const auto second_entities =
        source_entities(second.source);

    for (const auto& first_id :
         first_entities) {
        if (!first_id) {
            continue;
        }
        for (const auto& second_id :
             second_entities) {
            if (!second_id) {
                continue;
            }

            if (*first_id == *second_id) {
                return true;
            }

            const auto relation =
                analyzeCurveRelation(
                    model,
                    *first_id,
                    *second_id);
            if (relation.status !=
                CurveRelationStatus::discrete) {
                continue;
            }

            const auto exact_contact =
                std::find_if(
                    relation.intersections.begin(),
                    relation.intersections.end(),
                    [&first](
                        const CurveIntersection2D&
                            intersection) {
                        return intersection.point ==
                               first.point;
                    });
            if (exact_contact !=
                relation.intersections.end()) {
                return true;
            }
        }
    }

    return false;
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
    const SketchModel& model,
    SnapCaptureState& state,
    const std::vector<SnapScreenCandidate>& candidates,
    SnapResolutionPolicy policy) {
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
            item.screen_distance >
                policy.release_distance ||
            !sameSnapContact(
                model,
                resolution.primary,
                item.candidate)) {
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


bool trackingAnchorEligible(
    SnapKind kind) noexcept {
    switch (kind) {
    case SnapKind::endpoint:
    case SnapKind::midpoint:
    case SnapKind::center:
    case SnapKind::quadrant:
    case SnapKind::intersection:
    case SnapKind::origin:
        return true;
    case SnapKind::perpendicular:
    case SnapKind::tangent:
    case SnapKind::nearest:
        return false;
    }
    return false;
}

TrackingAcquireResult TrackingAnchorState::acquire(
    TrackingAnchor anchor) {
    if (!anchor.valid() || !valid()) {
        return TrackingAcquireResult::invalid;
    }

    const auto key =
        snapStableKey(anchor.snap);
    const auto duplicate =
        std::find_if(
            anchors.begin(),
            anchors.end(),
            [&key](const TrackingAnchor& item) {
                return snapStableKey(item.snap) ==
                       key;
            });
    if (duplicate != anchors.end()) {
        return TrackingAcquireResult::
            already_acquired;
    }

    if (anchors.size() >= 2U) {
        return TrackingAcquireResult::full;
    }

    anchors.push_back(std::move(anchor));
    return TrackingAcquireResult::acquired;
}

bool TrackingAnchorState::remove(
    const SnapStableKey& key) {
    const auto found =
        std::find_if(
            anchors.begin(),
            anchors.end(),
            [&key](const TrackingAnchor& item) {
                return snapStableKey(item.snap) ==
                       key;
            });
    if (found == anchors.end()) {
        return false;
    }
    anchors.erase(found);
    return true;
}

bool TrackingAnchorState::valid() const noexcept {
    if (anchors.size() > 2U) {
        return false;
    }
    for (std::size_t i = 0U;
         i < anchors.size();
         ++i) {
        if (!anchors[i].valid()) {
            return false;
        }
        for (std::size_t j = i + 1U;
             j < anchors.size();
             ++j) {
            if (snapStableKey(anchors[i].snap) ==
                snapStableKey(anchors[j].snap)) {
                return false;
            }
        }
    }
    return true;
}

bool InferenceGuide::valid() const noexcept {
    if (!anchor.finite() ||
        !direction.finite()) {
        return false;
    }
    const double length =
        std::hypot(
            direction.u,
            direction.v);
    return std::isfinite(length) &&
           length > 0.0;
}

namespace {

[[nodiscard]] std::optional<Point2>
canonicalGuideDirection(
    Point2 direction) noexcept {
    if (!direction.finite()) {
        return std::nullopt;
    }

    const double length =
        std::hypot(
            direction.u,
            direction.v);
    if (!std::isfinite(length) ||
        !(length > 0.0)) {
        return std::nullopt;
    }

    Point2 unit{
        direction.u / length,
        direction.v / length};

    // A guide is an undirected line. Canonical sign avoids duplicate +d/-d
    // directions without a geometric tolerance.
    if (unit.u < 0.0 ||
        (unit.u == 0.0 &&
         unit.v < 0.0)) {
        unit.u = -unit.u;
        unit.v = -unit.v;
    }
    return unit.finite()
        ? std::optional<Point2>{unit}
        : std::nullopt;
}

} // namespace

std::vector<InferenceGuide>
trackingGuides(
    const TrackingAnchorState& state,
    const std::vector<Point2>&
        additional_directions) {
    if (!state.valid()) {
        return {};
    }

    struct DirectionSpec final {
        Point2 direction;
        InferenceGuideKind kind;
    };

    std::vector<DirectionSpec> directions{
        {{1.0, 0.0},
         InferenceGuideKind::sketch_u},
        {{0.0, 1.0},
         InferenceGuideKind::sketch_v},
    };

    for (const auto direction :
         additional_directions) {
        const auto canonical =
            canonicalGuideDirection(direction);
        if (!canonical) {
            continue;
        }

        const auto duplicate =
            std::find_if(
                directions.begin(),
                directions.end(),
                [&canonical](
                    const DirectionSpec& item) {
                    return item.direction ==
                           *canonical;
                });
        if (duplicate != directions.end()) {
            continue;
        }

        directions.push_back(
            {*canonical,
             InferenceGuideKind::
                 additional_direction});
    }

    std::vector<InferenceGuide> result;
    result.reserve(
        state.anchors.size() *
        directions.size());

    for (const auto& anchor :
         state.anchors) {
        const auto key =
            snapStableKey(anchor.snap);
        for (const auto& direction :
             directions) {
            InferenceGuide guide{
                key,
                anchor.snap.point,
                direction.direction,
                direction.kind};
            if (guide.valid()) {
                result.push_back(
                    std::move(guide));
            }
        }
    }

    return result;
}

std::optional<Point2>
guideIntersection(
    const InferenceGuide& first,
    const InferenceGuide& second) noexcept {
    if (!first.valid() ||
        !second.valid()) {
        return std::nullopt;
    }

    const double determinant =
        first.direction.u *
            second.direction.v -
        first.direction.v *
            second.direction.u;
    if (!std::isfinite(determinant) ||
        determinant == 0.0) {
        return std::nullopt;
    }

    const double delta_u =
        second.anchor.u - first.anchor.u;
    const double delta_v =
        second.anchor.v - first.anchor.v;
    const double parameter =
        (delta_u * second.direction.v -
         delta_v * second.direction.u) /
        determinant;
    if (!std::isfinite(parameter)) {
        return std::nullopt;
    }

    Point2 result{
        std::fma(
            parameter,
            first.direction.u,
            first.anchor.u),
        std::fma(
            parameter,
            first.direction.v,
            first.anchor.v)};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

std::optional<Point2>
projectPointToGuide(
    const InferenceGuide& guide,
    Point2 point) noexcept {
    if (!guide.valid() ||
        !point.finite()) {
        return std::nullopt;
    }

    const double length_squared =
        guide.direction.u *
            guide.direction.u +
        guide.direction.v *
            guide.direction.v;
    if (!std::isfinite(length_squared) ||
        !(length_squared > 0.0)) {
        return std::nullopt;
    }

    const double relative_u =
        point.u - guide.anchor.u;
    const double relative_v =
        point.v - guide.anchor.v;
    const double parameter =
        (relative_u * guide.direction.u +
         relative_v * guide.direction.v) /
        length_squared;
    if (!std::isfinite(parameter)) {
        return std::nullopt;
    }

    Point2 result{
        std::fma(
            parameter,
            guide.direction.u,
            guide.anchor.u),
        std::fma(
            parameter,
            guide.direction.v,
            guide.anchor.v)};
    return result.finite()
        ? std::optional<Point2>{result}
        : std::nullopt;
}

} // namespace simplesolid2::sketch
