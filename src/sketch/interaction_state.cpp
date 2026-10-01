#include <simplesolid2/sketch/interaction_state.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <utility>

namespace simplesolid2::sketch {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;
constexpr double half_turn =
    std::numbers::pi_v<double>;

[[nodiscard]] Point2 lineCenter(
    const Line& line) noexcept {
    return {
        (line.start().u + line.end().u) * 0.5,
        (line.start().v + line.end().v) * 0.5};
}

[[nodiscard]] double distance(
    Point2 first,
    Point2 second) noexcept {
    return std::hypot(
        second.u - first.u,
        second.v - first.v);
}

[[nodiscard]] double direction(
    Point2 center,
    Point2 point) noexcept {
    return std::atan2(
        point.v - center.v,
        point.u - center.u);
}

[[nodiscard]] double positiveTurn(
    double angle) noexcept {
    double normalized =
        std::fmod(angle, full_turn);
    if (normalized < 0.0) {
        normalized += full_turn;
    }
    return normalized;
}

[[nodiscard]] Point2 stableUnitDirection(
    double angle) noexcept {
    if (!std::isfinite(angle)) {
        return {};
    }

    constexpr double quarter_turn =
        std::numbers::pi_v<double> / 2.0;
    double normalized = positiveTurn(angle);
    int quadrant =
        static_cast<int>(normalized / quarter_turn);
    if (quadrant > 3) {
        quadrant = 0;
        normalized = 0.0;
    }

    const double local =
        normalized -
        static_cast<double>(quadrant) * quarter_turn;
    if (local == 0.0) {
        switch (quadrant) {
        case 0: return {1.0, 0.0};
        case 1: return {0.0, 1.0};
        case 2: return {-1.0, 0.0};
        case 3: return {0.0, -1.0};
        default: return {};
        }
    }

    const double cosine = std::cos(local);
    const double sine = std::sin(local);
    switch (quadrant) {
    case 0: return {cosine, sine};
    case 1: return {-sine, cosine};
    case 2: return {-cosine, -sine};
    case 3: return {sine, -cosine};
    default: return {};
    }
}

[[nodiscard]] double ccwDelta(
    double from,
    double to) noexcept {
    return positiveTurn(to - from);
}

[[nodiscard]] Point2 pointOnCircle(
    Point2 center,
    double radius,
    double angle) noexcept {
    return {
        center.u + radius * std::cos(angle),
        center.v + radius * std::sin(angle)};
}

[[nodiscard]] bool validReplacement(
    const SketchLineState& line) noexcept {
    return line.id.valid() &&
           line.start.finite() &&
           line.end.finite() &&
           line.start != line.end;
}

[[nodiscard]] bool validReplacement(
    const SketchCircleState& circle) noexcept {
    return circle.id.valid() &&
           circle.center.finite() &&
           std::isfinite(circle.radius) &&
           circle.radius > 0.0;
}

[[nodiscard]] bool validReplacement(
    const SketchArcState& arc) noexcept {
    return arc.id.valid() &&
           arc.center.finite() &&
           std::isfinite(arc.radius) &&
           arc.radius > 0.0 &&
           std::isfinite(arc.start_angle) &&
           std::isfinite(arc.sweep_angle) &&
           arc.sweep_angle != 0.0 &&
           std::abs(arc.sweep_angle) < full_turn;
}

[[nodiscard]] std::optional<ArcIntent>
arcThroughThreePoints(
    Point2 start,
    Point2 through,
    Point2 end) noexcept {
    if (!start.finite() ||
        !through.finite() ||
        !end.finite() ||
        start == through ||
        start == end ||
        through == end) {
        return std::nullopt;
    }

    // Solve the circumcenter in a local, scale-normalized frame.
    // This preserves the existing exact duplicate/collinear semantics
    // without introducing a Product tolerance while avoiding absolute
    // coordinate squares and their translation-sensitive cancellation.
    const Point2 local_through{
        through.u - start.u,
        through.v - start.v};
    const Point2 local_end{
        end.u - start.u,
        end.v - start.v};
    if (!local_through.finite() ||
        !local_end.finite()) {
        return std::nullopt;
    }

    const double scale =
        std::max({
            std::abs(local_through.u),
            std::abs(local_through.v),
            std::abs(local_end.u),
            std::abs(local_end.v)});
    if (!std::isfinite(scale) ||
        scale <= 0.0) {
        return std::nullopt;
    }

    const Point2 through_normalized{
        local_through.u / scale,
        local_through.v / scale};
    const Point2 end_normalized{
        local_end.u / scale,
        local_end.v / scale};

    const double cross =
        std::fma(
            through_normalized.u,
            end_normalized.v,
            -through_normalized.v *
                end_normalized.u);
    if (!std::isfinite(cross) ||
        cross == 0.0) {
        return std::nullopt;
    }

    const double through_squared =
        std::fma(
            through_normalized.u,
            through_normalized.u,
            through_normalized.v *
                through_normalized.v);
    const double end_squared =
        std::fma(
            end_normalized.u,
            end_normalized.u,
            end_normalized.v *
                end_normalized.v);
    const double determinant =
        2.0 * cross;

    const double center_u_normalized =
        std::fma(
            through_squared,
            end_normalized.v,
            -end_squared *
                through_normalized.v) /
        determinant;
    const double center_v_normalized =
        std::fma(
            through_normalized.u,
            end_squared,
            -end_normalized.u *
                through_squared) /
        determinant;

    const Point2 center_offset{
        center_u_normalized * scale,
        center_v_normalized * scale};
    if (!center_offset.finite()) {
        return std::nullopt;
    }

    Point2 center{
        start.u + center_offset.u,
        start.v + center_offset.v};

    const double radius =
        distance(center, start);
    if (!center.finite() ||
        !std::isfinite(radius) ||
        radius <= 0.0) {
        return std::nullopt;
    }

    const double start_angle =
        direction(center, start);
    const double through_angle =
        direction(center, through);
    const double end_angle =
        direction(center, end);

    const double to_through =
        ccwDelta(start_angle, through_angle);
    const double to_end =
        ccwDelta(start_angle, end_angle);
    if (to_through == 0.0 ||
        to_end == 0.0) {
        return std::nullopt;
    }

    const double sweep =
        to_through < to_end
            ? to_end
            : to_end - full_turn;

    ArcIntent result{
        center,
        radius,
        start_angle,
        sweep};
    return result.valid()
        ? std::optional<ArcIntent>{result}
        : std::nullopt;
}

[[nodiscard]] int directedChordSide(
    Point2 start,
    Point2 end,
    Point2 point) noexcept {
    if (!start.finite() ||
        !end.finite() ||
        !point.finite()) {
        return 0;
    }

    const double du = end.u - start.u;
    const double dv = end.v - start.v;
    const double pu = point.u - start.u;
    const double pv = point.v - start.v;
    if (!std::isfinite(du) ||
        !std::isfinite(dv) ||
        !std::isfinite(pu) ||
        !std::isfinite(pv)) {
        return 0;
    }

    const double chord_scale =
        std::max(std::abs(du), std::abs(dv));
    const double point_scale =
        std::max(std::abs(pu), std::abs(pv));
    if (chord_scale <= 0.0 ||
        point_scale <= 0.0) {
        return 0;
    }

    const double cross =
        (du / chord_scale) *
            (pv / point_scale) -
        (dv / chord_scale) *
            (pu / point_scale);
    if (!std::isfinite(cross) || cross == 0.0) {
        return 0;
    }
    return cross > 0.0 ? 1 : -1;
}

[[nodiscard]] std::optional<ArcIntent>
arcFromChordRadius(
    Point2 start,
    Point2 end,
    Point2 side_point,
    double radius) noexcept {
    if (!start.finite() ||
        !end.finite() ||
        !side_point.finite() ||
        !std::isfinite(radius) ||
        radius <= 0.0) {
        return std::nullopt;
    }

    const double du = end.u - start.u;
    const double dv = end.v - start.v;
    const double chord = std::hypot(du, dv);
    if (!std::isfinite(chord) || chord <= 0.0) {
        return std::nullopt;
    }

    const double half_chord = chord * 0.5;
    if (radius < half_chord) {
        return std::nullopt;
    }

    const int side =
        directedChordSide(start, end, side_point);
    if (side == 0) {
        return std::nullopt;
    }

    const double ratio = half_chord / radius;
    if (!std::isfinite(ratio) ||
        ratio < 0.0 || ratio > 1.0) {
        return std::nullopt;
    }

    const double offset =
        radius *
        std::sqrt(std::max(
            0.0,
            1.0 - ratio * ratio));
    if (!std::isfinite(offset)) {
        return std::nullopt;
    }

    const Point2 midpoint{
        start.u + du * 0.5,
        start.v + dv * 0.5};
    const Point2 left_normal{
        -dv / chord,
        du / chord};
    const Point2 center{
        midpoint.u -
            static_cast<double>(side) *
                offset * left_normal.u,
        midpoint.v -
            static_cast<double>(side) *
                offset * left_normal.v};
    if (!center.finite()) {
        return std::nullopt;
    }

    const double sweep_magnitude =
        2.0 * std::asin(ratio);
    if (!std::isfinite(sweep_magnitude) ||
        sweep_magnitude <= 0.0 ||
        sweep_magnitude > half_turn) {
        return std::nullopt;
    }

    ArcIntent result{
        center,
        radius,
        direction(center, start),
        side > 0
            ? -sweep_magnitude
            : sweep_magnitude};
    return result.valid()
        ? std::optional<ArcIntent>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<RectangleIntent>
rectangleFromSize(
    Point2 first,
    Point2 pointer,
    double width,
    double height) noexcept {
    if (!first.finite() ||
        !pointer.finite() ||
        !std::isfinite(width) ||
        !std::isfinite(height) ||
        width <= 0.0 ||
        height <= 0.0) {
        return std::nullopt;
    }

    const double du = pointer.u - first.u;
    const double dv = pointer.v - first.v;
    if (!std::isfinite(du) ||
        !std::isfinite(dv) ||
        du == 0.0 ||
        dv == 0.0) {
        return std::nullopt;
    }

    RectangleIntent result{
        first,
        {
            first.u + (du > 0.0 ? width : -width),
            first.v + (dv > 0.0 ? height : -height)}};
    return result.valid()
        ? std::optional<RectangleIntent>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<RectangleIntent>
rectangleFromSizeLocks(
    Point2 first,
    Point2 pointer,
    std::optional<double> width,
    std::optional<double> height) noexcept {
    if (!first.finite() ||
        !pointer.finite()) {
        return std::nullopt;
    }

    const double du = pointer.u - first.u;
    const double dv = pointer.v - first.v;
    if (!std::isfinite(du) ||
        !std::isfinite(dv) ||
        du == 0.0 ||
        dv == 0.0) {
        return std::nullopt;
    }

    return rectangleFromSize(
        first,
        pointer,
        width.value_or(std::abs(du)),
        height.value_or(std::abs(dv)));
}

[[nodiscard]] std::optional<double>
sameDirectionSweep(
    double start_angle,
    double end_angle,
    double previous_sweep) noexcept {
    const double positive =
        ccwDelta(start_angle, end_angle);
    if (positive == 0.0) {
        return std::nullopt;
    }

    const double candidate =
        previous_sweep > 0.0
            ? positive
            : positive - full_turn;

    if (candidate == 0.0 ||
        std::abs(candidate) >= full_turn ||
        std::abs(candidate) == half_turn) {
        return std::nullopt;
    }

    return candidate;
}

[[nodiscard]] std::optional<double>
signedAngle(
    Point2 base,
    Point2 reference,
    Point2 destination) noexcept {
    if (!base.finite() ||
        !reference.finite() ||
        !destination.finite() ||
        reference == base ||
        destination == base) {
        return std::nullopt;
    }

    const double ref_u = reference.u - base.u;
    const double ref_v = reference.v - base.v;
    const double dst_u = destination.u - base.u;
    const double dst_v = destination.v - base.v;

    const double cross =
        ref_u * dst_v -
        ref_v * dst_u;
    const double dot =
        ref_u * dst_u +
        ref_v * dst_v;
    const double angle =
        std::atan2(cross, dot);

    return std::isfinite(angle)
        ? std::optional<double>{angle}
        : std::nullopt;
}

} // namespace

bool CircleIntent::valid() const noexcept {
    return center.finite() &&
           std::isfinite(radius) &&
           radius > 0.0;
}

bool ArcIntent::valid() const noexcept {
    return center.finite() &&
           std::isfinite(radius) &&
           radius > 0.0 &&
           std::isfinite(start_angle) &&
           std::isfinite(sweep_angle) &&
           sweep_angle != 0.0 &&
           std::abs(sweep_angle) < full_turn;
}

bool RectangleIntent::valid() const noexcept {
    return first_corner.finite() &&
           opposite_corner.finite() &&
           first_corner.u != opposite_corner.u &&
           first_corner.v != opposite_corner.v;
}

std::array<LineSegmentIntent, 4>
RectangleIntent::perimeter() const noexcept {
    const Point2 a = first_corner;
    const Point2 c = opposite_corner;
    const Point2 b{c.u, a.v};
    const Point2 d{a.u, c.v};
    return {
        LineSegmentIntent{a, b},
        LineSegmentIntent{b, c},
        LineSegmentIntent{c, d},
        LineSegmentIntent{d, a}};
}

std::array<LineSegmentIntent, 2>
RectangleIntent::diagonals() const noexcept {
    const Point2 a = first_corner;
    const Point2 c = opposite_corner;
    const Point2 b{c.u, a.v};
    const Point2 d{a.u, c.v};
    return {
        LineSegmentIntent{a, c},
        LineSegmentIntent{b, d}};
}

std::optional<LineStage>
SketchInteractionState::lineStage() const noexcept {
    return tool_ == SketchTool::line
        ? std::optional<LineStage>{line_stage_}
        : std::nullopt;
}

std::optional<CircleStage>
SketchInteractionState::circleStage() const noexcept {
    return tool_ == SketchTool::circle
        ? std::optional<CircleStage>{circle_stage_}
        : std::nullopt;
}

std::optional<ArcStage>
SketchInteractionState::arcStage() const noexcept {
    return tool_ == SketchTool::arc
        ? std::optional<ArcStage>{arc_stage_}
        : std::nullopt;
}

std::optional<RectangleStage>
SketchInteractionState::rectangleStage() const noexcept {
    return tool_ == SketchTool::rectangle
        ? std::optional<RectangleStage>{rectangle_stage_}
        : std::nullopt;
}

std::optional<MoveStage>
SketchInteractionState::moveStage() const noexcept {
    if (tool_ != SketchTool::move ||
        !transform_session_) {
        return std::nullopt;
    }

    switch (transform_session_->stage) {
    case CommonTransformStage::select_objects:
        return MoveStage::select_objects;
    case CommonTransformStage::await_base_point:
        return MoveStage::await_base_point;
    case CommonTransformStage::await_destination:
        return MoveStage::await_destination;
    default:
        return std::nullopt;
    }
}

std::optional<CommonTransformStage>
SketchInteractionState::commonTransformStage() const noexcept {
    return commonTransformTool() &&
                   transform_session_
        ? std::optional<CommonTransformStage>{
              transform_session_->stage}
        : std::nullopt;
}

bool SketchInteractionState::commonTransformTool()
    const noexcept {
    return tool_ == SketchTool::move ||
           tool_ == SketchTool::copy ||
           tool_ == SketchTool::rotate ||
           tool_ == SketchTool::scale ||
           tool_ == SketchTool::mirror;
}

namespace {

[[nodiscard]] std::optional<PointResolution>
resolvePointRequestCandidate(
    const PointRequest& request,
    const PointFieldLocks& locks,
    PointResolutionSource pointer_source,
    const std::optional<SnapCandidate>& pointer_snap)
    noexcept {
    if (!request.pointer_candidate) {
        return std::nullopt;
    }

    const auto raw = *request.pointer_candidate;
    Point2 resolved = raw;
    const bool locked = !locks.empty();

    const auto finish =
        [&](Point2 point)
            -> std::optional<PointResolution> {
            if (!point.finite()) {
                return std::nullopt;
            }

            PointResolution result{
                point,
                locked
                    ? PointResolutionSource::numeric_lock
                    : pointer_source,
                locked
                    ? std::nullopt
                    : pointer_snap};

            return result.valid()
                ? std::optional<PointResolution>{
                      std::move(result)}
                : std::nullopt;
        };

    if (!request.base) {
        if (locks.u) {
            resolved.u = *locks.u;
        }
        if (locks.v) {
            resolved.v = *locks.v;
        }
        return finish(resolved);
    }

    const auto base = *request.base;
    const double raw_du = raw.u - base.u;
    const double raw_dv = raw.v - base.v;

    if (locks.delta_u ||
        locks.delta_v) {
        resolved = {
            base.u +
                locks.delta_u.value_or(raw_du),
            base.v +
                locks.delta_v.value_or(raw_dv)};
        return finish(resolved);
    }

    if (locks.distance ||
        locks.angle) {
        const double raw_distance =
            std::hypot(raw_du, raw_dv);

        double resolved_angle{};
        if (locks.angle) {
            resolved_angle = *locks.angle;
        } else {
            if (!std::isfinite(raw_distance) ||
                raw_distance <= 0.0) {
                return std::nullopt;
            }
            resolved_angle =
                std::atan2(raw_dv, raw_du);
        }

        const double resolved_distance =
            locks.distance.value_or(
                raw_distance);
        if (!std::isfinite(resolved_distance) ||
            resolved_distance < 0.0 ||
            !std::isfinite(resolved_angle)) {
            return std::nullopt;
        }

        resolved = {
            base.u +
                resolved_distance *
                    std::cos(resolved_angle),
            base.v +
                resolved_distance *
                    std::sin(resolved_angle)};
    }

    return finish(resolved);
}

} // namespace

std::optional<PointRequest>
SketchInteractionState::activePointRequest() const noexcept {
    const auto finalized =
        [this](PointRequest request)
            -> std::optional<PointRequest> {
            request.temporary_snap_override =
                temporary_snap_override_;
            request.resolution =
                resolvePointRequestCandidate(
                    request,
                    point_field_locks_,
                    point_pointer_source_,
                    point_pointer_snap_);
            return request.valid()
                ? std::optional<PointRequest>{
                      std::move(request)}
                : std::nullopt;
        };

    if (manipulation_) {
        const bool distance_enabled =
            manipulation_->mode ==
                DirectEditMode::reshape ||
            manipulation_->mode ==
                DirectEditMode::move;
        PointRequest request{
            manipulation_->pivot,
            point_pointer_candidate_,
            distance_enabled,
            true,
            true,
            true};
        if (manipulation_->mode ==
                DirectEditMode::rotate &&
            manipulation_->rotate_reference_point) {
            request.polar_relative_reference =
                direction(
                    manipulation_->pivot,
                    *manipulation_->
                        rotate_reference_point);
        }
        return finalized(std::move(request));
    }

    if (tool_ == SketchTool::line &&
        !pending_line_request_) {
        const bool based =
            line_stage_ == LineStage::await_next_point &&
            line_anchor_.has_value();
        PointRequest request{
            based ? line_anchor_ : std::nullopt,
            point_pointer_candidate_,
            based,
            true,
            based,
            based};
        if (based && line_relative_reference_) {
            request.polar_relative_reference =
                line_relative_reference_;
        }
        return finalized(std::move(request));
    }

    if (tool_ == SketchTool::circle &&
        circle_stage_ == CircleStage::await_center &&
        !pending_circle_request_) {
        PointRequest request{
            std::nullopt,
            point_pointer_candidate_,
            false,
            true,
            false,
            false};
        return finalized(std::move(request));
    }

    if (tool_ == SketchTool::arc &&
        !pending_arc_request_) {
        const bool end_stage =
            arc_stage_ == ArcStage::await_end &&
            arc_start_.has_value();
        const bool arc_point_stage =
            arc_stage_ == ArcStage::await_arc_point &&
            arc_start_.has_value() &&
            arc_end_.has_value();

        if (arc_stage_ == ArcStage::await_start ||
            end_stage ||
            arc_point_stage) {
            PointRequest request{
                end_stage ? arc_start_ : std::nullopt,
                point_pointer_candidate_,
                end_stage,
                true,
                end_stage,
                end_stage};
            return finalized(std::move(request));
        }
    }

    if (tool_ == SketchTool::rectangle &&
        !pending_rectangle_request_) {
        const bool first_corner =
            rectangle_stage_ ==
            RectangleStage::await_first_corner;
        PointRequest request{
            first_corner
                ? std::nullopt
                : rectangle_first_corner_,
            point_pointer_candidate_,
            false,
            first_corner,
            false,
            false};
        return finalized(std::move(request));
    }

    if (!commonTransformTool() ||
        !transform_session_ ||
        transform_session_->stage ==
            CommonTransformStage::select_objects) {
        return std::nullopt;
    }

    PointRequest request{
        transform_session_->base_point,
        point_pointer_candidate_,
        false,
        true,
        false,
        false};

    switch (transform_session_->stage) {
    case CommonTransformStage::await_base_point:
    case CommonTransformStage::await_axis_start:
        request.base.reset();
        break;
    case CommonTransformStage::await_destination:
        request.direct_distance_enabled =
            (tool_ == SketchTool::move ||
             tool_ == SketchTool::copy) &&
            request.base.has_value();
        request.relative_cartesian_enabled =
            request.base.has_value();
        request.relative_polar_enabled =
            request.base.has_value();
        break;
    case CommonTransformStage::await_reference_point:
    case CommonTransformStage::await_axis_end:
        request.relative_cartesian_enabled =
            request.base.has_value();
        request.relative_polar_enabled =
            request.base.has_value();
        break;
    case CommonTransformStage::select_objects:
        return std::nullopt;
    }

    if (transform_session_->stage ==
            CommonTransformStage::await_destination &&
        (tool_ == SketchTool::rotate ||
         tool_ == SketchTool::scale) &&
        transform_session_->base_point &&
        transform_session_->reference_point) {
        request.polar_relative_reference =
            direction(
                *transform_session_->base_point,
                *transform_session_->
                    reference_point);
    }

    return finalized(std::move(request));
}

bool SketchInteractionState::setTemporarySnapOverride(
    TemporarySnapOverrideKind value) noexcept {
    if (!activePointRequest()) {
        return false;
    }
    temporary_snap_override_ = value;
    return true;
}

bool SketchInteractionState::clearTemporarySnapOverride()
    noexcept {
    if (!temporary_snap_override_) {
        return false;
    }
    temporary_snap_override_.reset();
    return true;
}

std::optional<ResolvedSketchInput>
SketchInteractionState::resolveExplicitPoint(
    ExplicitPointInput input) const noexcept {
    if (!input.valid()) {
        return std::nullopt;
    }

    const auto request = activePointRequest();
    if (!request) {
        return std::nullopt;
    }

    Point2 resolved;
    switch (input.kind) {
    case ExplicitPointInputKind::absolute_cartesian:
        if (!request->absolute_cartesian_enabled) {
            return std::nullopt;
        }
        resolved = {input.first, input.second};
        break;

    case ExplicitPointInputKind::relative_cartesian:
        if (!request->relative_cartesian_enabled ||
            !request->base) {
            return std::nullopt;
        }
        resolved = {
            request->base->u + input.first,
            request->base->v + input.second};
        break;

    case ExplicitPointInputKind::relative_polar:
        if (!request->relative_polar_enabled ||
            !request->base) {
            return std::nullopt;
        }
        {
            const auto direction =
                stableUnitDirection(input.second);
            resolved = {
                request->base->u +
                    input.first * direction.u,
                request->base->v +
                    input.first * direction.v};
        }
        break;
    }

    PointResolution result{
        resolved,
        PointResolutionSource::explicit_numeric,
        std::nullopt};
    return result.valid()
        ? std::optional<ResolvedSketchInput>{
              result}
        : std::nullopt;
}

std::optional<ResolvedSketchInput>
SketchInteractionState::resolvePointerInput(
    Point2 raw) noexcept {
    return resolvePointerInput(
        raw,
        PointResolutionSource::raw_pointer);
}

std::optional<ResolvedSketchInput>
SketchInteractionState::resolvePointerInput(
    Point2 raw,
    PointResolutionSource source,
    std::optional<SnapCandidate> object_snap) noexcept {
    if (!raw.finite() || !activePointRequest()) {
        return std::nullopt;
    }

    const bool pointer_source =
        source ==
            PointResolutionSource::raw_pointer ||
        source ==
            PointResolutionSource::polar ||
        source ==
            PointResolutionSource::object_snap ||
        source ==
            PointResolutionSource::tracking_inference;
    if (!pointer_source) {
        return std::nullopt;
    }

    if (source ==
        PointResolutionSource::object_snap) {
        if (!object_snap ||
            !object_snap->valid() ||
            object_snap->point != raw) {
            return std::nullopt;
        }
    } else if (object_snap) {
        return std::nullopt;
    }

    point_pointer_candidate_ = raw;
    point_pointer_source_ = source;
    point_pointer_snap_ =
        std::move(object_snap);
    return resolvedPointRequestCandidate();
}

std::optional<ResolvedSketchInput>
SketchInteractionState::
resolvedPointRequestCandidate() const noexcept {
    const auto request = activePointRequest();
    return request
        ? request->resolution
        : std::nullopt;
}

bool SketchInteractionState::lockPointField(
    PointFieldLockSemantic semantic,
    double value) noexcept {
    const auto request = activePointRequest();
    if (!request || !std::isfinite(value)) {
        return false;
    }

    const bool based = request->base.has_value();
    const bool polar_family_locked =
        point_field_locks_.distance.has_value() ||
        point_field_locks_.angle.has_value();
    const bool cartesian_family_locked =
        point_field_locks_.delta_u.has_value() ||
        point_field_locks_.delta_v.has_value();

    switch (semantic) {
    case PointFieldLockSemantic::u:
        if (based ||
            !request->absolute_cartesian_enabled) {
            return false;
        }
        point_field_locks_.u = value;
        return true;

    case PointFieldLockSemantic::v:
        if (based ||
            !request->absolute_cartesian_enabled) {
            return false;
        }
        point_field_locks_.v = value;
        return true;

    case PointFieldLockSemantic::distance:
        if (!based ||
            (!request->relative_polar_enabled &&
             !request->direct_distance_enabled) ||
            value < 0.0 ||
            cartesian_family_locked) {
            return false;
        }
        point_field_locks_.distance = value;
        return true;

    case PointFieldLockSemantic::angle:
        if (!based ||
            !request->relative_polar_enabled ||
            cartesian_family_locked) {
            return false;
        }
        point_field_locks_.angle = value;
        return true;

    case PointFieldLockSemantic::delta_u:
        if (!based ||
            !request->relative_cartesian_enabled ||
            polar_family_locked) {
            return false;
        }
        point_field_locks_.delta_u = value;
        return true;

    case PointFieldLockSemantic::delta_v:
        if (!based ||
            !request->relative_cartesian_enabled ||
            polar_family_locked) {
            return false;
        }
        point_field_locks_.delta_v = value;
        return true;
    }

    return false;
}

std::optional<ResolvedSketchInput>
SketchInteractionState::resolveDirectDistance(
    double requested_distance) const noexcept {
    if (!std::isfinite(requested_distance) ||
        requested_distance < 0.0) {
        return std::nullopt;
    }

    const auto request = activePointRequest();
    if (!request ||
        !request->direct_distance_enabled ||
        !request->base ||
        !request->pointer_candidate) {
        return std::nullopt;
    }

    const auto base = *request->base;
    const auto pointer = *request->pointer_candidate;
    const double du = pointer.u - base.u;
    const double dv = pointer.v - base.v;
    const double length = std::hypot(du, dv);
    if (!std::isfinite(length) || length <= 0.0) {
        return std::nullopt;
    }

    const Point2 resolved{
        base.u + (du / length) * requested_distance,
        base.v + (dv / length) * requested_distance};
    PointResolution result{
        resolved,
        PointResolutionSource::explicit_numeric,
        std::nullopt};
    return result.valid()
        ? std::optional<ResolvedSketchInput>{
              result}
        : std::nullopt;
}

void SketchInteractionState::activateLine() noexcept {
    resetMeasure();
    manipulation_.reset();
    point_pointer_candidate_.reset();
    resetCommonTransform();
    clearHover();
    tool_ = SketchTool::line;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();
}

void SketchInteractionState::activateCircle() noexcept {
    resetMeasure();
    manipulation_.reset();
    point_pointer_candidate_.reset();
    resetCommonTransform();
    clearHover();
    tool_ = SketchTool::circle;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();
}

void SketchInteractionState::activateArc() noexcept {
    resetMeasure();
    manipulation_.reset();
    point_pointer_candidate_.reset();
    resetCommonTransform();
    clearHover();
    tool_ = SketchTool::arc;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();
}

void SketchInteractionState::activateRectangle() noexcept {
    resetMeasure();
    manipulation_.reset();
    point_pointer_candidate_.reset();
    resetCommonTransform();
    clearHover();
    tool_ = SketchTool::rectangle;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();
}

void SketchInteractionState::activateMeasure(
    const SketchModel& model) noexcept {
    manipulation_.reset();
    point_pointer_candidate_.reset();
    resetCommonTransform();
    clearHover();
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();
    tool_ = SketchTool::measure;
    resetMeasure();

    if (selected_.size() == 1U &&
        model.contains(selected_.front())) {
        measure_target_ = selected_.front();
    }
}

bool SketchInteractionState::setMeasureTarget(
    const SketchModel& model,
    std::optional<EntityId> target) noexcept {
    if (tool_ != SketchTool::measure ||
        measure_between_active_) {
        return false;
    }
    if (target &&
        (!target->valid() || !model.contains(*target))) {
        return false;
    }
    measure_target_ = target;
    clearHover();
    return true;
}

bool SketchInteractionState::enterMeasureBetween() noexcept {
    if (tool_ != SketchTool::measure) {
        return false;
    }
    if (measure_between_active_) {
        return true;
    }

    measure_target_.reset();
    measure_between_active_ = true;
    measure_first_target_.reset();
    measure_second_target_.reset();
    clearHover();
    return true;
}

void SketchInteractionState::leaveMeasureBetween() noexcept {
    measure_between_active_ = false;
    measure_first_target_.reset();
    measure_second_target_.reset();
    clearHover();
}

MeasureRelationAcceptOutcome
SketchInteractionState::acceptMeasureRelationTarget(
    const SketchModel& model,
    MeasureRelationTarget target) noexcept {
    if (tool_ != SketchTool::measure ||
        !measure_between_active_) {
        return MeasureRelationAcceptOutcome::inactive;
    }

    const bool valid_target =
        std::visit(
            [&model](const auto& value) {
                using Value =
                    std::decay_t<decltype(value)>;
                if constexpr (
                    std::is_same_v<
                        Value,
                        MeasurePointRef>) {
                    return resolveMeasurePoint(
                               model,
                               value)
                        .has_value();
                } else {
                    return value.valid() &&
                           model.findLine(
                               value.entity_id) !=
                               nullptr;
                }
            },
            target);
    if (!valid_target) {
        return MeasureRelationAcceptOutcome::
            invalid_target;
    }

    if (measure_second_target_) {
        measure_first_target_ = std::move(target);
        measure_second_target_.reset();
        return MeasureRelationAcceptOutcome::
            first_target_accepted;
    }

    if (!measure_first_target_) {
        measure_first_target_ = std::move(target);
        return MeasureRelationAcceptOutcome::
            first_target_accepted;
    }

    if (!measureRelation(
            model,
            *measure_first_target_,
            target)) {
        return MeasureRelationAcceptOutcome::
            relation_rejected;
    }

    measure_second_target_ = std::move(target);
    return MeasureRelationAcceptOutcome::
        relation_accepted;
}

bool SketchInteractionState::clearMeasureRelation() noexcept {
    if (!measure_between_active_) {
        return false;
    }
    const bool changed =
        measure_first_target_.has_value() ||
        measure_second_target_.has_value();
    measure_first_target_.reset();
    measure_second_target_.reset();
    return changed;
}

std::optional<RelationalMeasurement>
SketchInteractionState::measureRelationalResult(
    const SketchModel& model) const noexcept {
    if (tool_ != SketchTool::measure ||
        !measure_between_active_ ||
        !measure_first_target_ ||
        !measure_second_target_) {
        return std::nullopt;
    }
    return measureRelation(
        model,
        *measure_first_target_,
        *measure_second_target_);
}

bool SketchInteractionState::activateCommonTransform(
    SketchTool tool,
    const SketchModel& model) {
    if (tool != SketchTool::move &&
        tool != SketchTool::copy &&
        tool != SketchTool::rotate &&
        tool != SketchTool::scale &&
        tool != SketchTool::mirror) {
        return false;
    }

    resetMeasure();
    manipulation_.reset();
    point_pointer_candidate_.reset();
    resetCommonTransform();
    clearHover();
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();

    CommonTransformSession session;
    if (selected_.empty()) {
        session.stage =
            CommonTransformStage::select_objects;
        tool_ = tool;
        transform_session_ = std::move(session);
        return true;
    }

    const auto geometry =
        captureSketchTransformGeometry(
            model,
            selected_);
    if (!geometry) {
        resetToSelect();
        return false;
    }

    session.selection_snapshot = selected_;
    session.initial_geometry = *geometry;
    session.stage =
        tool == SketchTool::mirror
            ? CommonTransformStage::await_axis_start
            : CommonTransformStage::await_base_point;

    tool_ = tool;
    transform_session_ = std::move(session);
    return true;
}

bool SketchInteractionState::activateMove(
    const SketchModel& model) {
    return activateCommonTransform(
        SketchTool::move,
        model);
}

bool SketchInteractionState::activateCopy(
    const SketchModel& model) {
    return activateCommonTransform(
        SketchTool::copy,
        model);
}

bool SketchInteractionState::activateRotate(
    const SketchModel& model) {
    return activateCommonTransform(
        SketchTool::rotate,
        model);
}

bool SketchInteractionState::activateScale(
    const SketchModel& model) {
    return activateCommonTransform(
        SketchTool::scale,
        model);
}

bool SketchInteractionState::activateMirror(
    const SketchModel& model) {
    return activateCommonTransform(
        SketchTool::mirror,
        model);
}

bool SketchInteractionState::completeTransformSelection(
    const SketchModel& model) {
    if (!commonTransformTool() ||
        !transform_session_ ||
        transform_session_->stage !=
            CommonTransformStage::select_objects ||
        selected_.empty()) {
        return false;
    }

    const auto geometry =
        captureSketchTransformGeometry(
            model,
            selected_);
    if (!geometry) {
        return false;
    }

    transform_session_->selection_snapshot = selected_;
    transform_session_->initial_geometry = *geometry;
    transform_session_->base_point.reset();
    transform_session_->reference_point.reset();
    transform_session_->current_preview.reset();
    transform_session_->explicit_value.reset();
    point_pointer_candidate_.reset();
    transform_session_->stage =
        tool_ == SketchTool::mirror
            ? CommonTransformStage::await_axis_start
            : CommonTransformStage::await_base_point;
    clearHover();
    return true;
}

bool SketchInteractionState::acceptTransformPoint(
    ResolvedSketchInput input) noexcept {
    if (!commonTransformTool() ||
        !transform_session_ ||
        !input.valid() ||
        transform_session_->selection_snapshot.empty() ||
        transform_session_->initial_geometry.empty()) {
        return false;
    }

    switch (transform_session_->stage) {
    case CommonTransformStage::await_base_point:
        if (tool_ != SketchTool::move &&
            tool_ != SketchTool::copy &&
            tool_ != SketchTool::rotate &&
            tool_ != SketchTool::scale) {
            return false;
        }
        transform_session_->base_point = input.position;
        point_pointer_candidate_ = input.position;
        transform_session_->reference_point.reset();
        transform_session_->current_preview.reset();
        transform_session_->explicit_value.reset();
        point_field_locks_ = {};
        transform_session_->stage =
            (tool_ == SketchTool::move ||
             tool_ == SketchTool::copy)
                ? CommonTransformStage::await_destination
                : CommonTransformStage::await_reference_point;
        if (tool_ == SketchTool::move ||
            tool_ == SketchTool::copy) {
            transform_session_->current_preview = input;
        }
        clearHover();
        consumeTemporarySnapOverride();
        return true;

    case CommonTransformStage::await_reference_point:
        if ((tool_ != SketchTool::rotate &&
             tool_ != SketchTool::scale) ||
            !transform_session_->base_point ||
            input.position ==
                *transform_session_->base_point) {
            return false;
        }
        transform_session_->reference_point =
            input.position;
        point_pointer_candidate_ = input.position;
        transform_session_->current_preview = input;
        transform_session_->explicit_value.reset();
        point_field_locks_ = {};
        transform_session_->stage =
            CommonTransformStage::await_destination;
        clearHover();
        consumeTemporarySnapOverride();
        return true;

    case CommonTransformStage::await_axis_start:
        if (tool_ != SketchTool::mirror) {
            return false;
        }
        transform_session_->base_point = input.position;
        point_pointer_candidate_ = input.position;
        transform_session_->reference_point.reset();
        transform_session_->current_preview = input;
        point_field_locks_ = {};
        transform_session_->stage =
            CommonTransformStage::await_axis_end;
        clearHover();
        consumeTemporarySnapOverride();
        return true;

    default:
        return false;
    }
}

bool SketchInteractionState::updateTransformPreview(
    ResolvedSketchInput input) noexcept {
    if (!commonTransformTool() ||
        !transform_session_ ||
        !input.valid()) {
        return false;
    }

    const bool destination =
        transform_session_->stage ==
            CommonTransformStage::await_destination &&
        (tool_ == SketchTool::move ||
         tool_ == SketchTool::copy ||
         tool_ == SketchTool::rotate ||
         tool_ == SketchTool::scale);
    const bool axis_end =
        transform_session_->stage ==
            CommonTransformStage::await_axis_end &&
        tool_ == SketchTool::mirror;

    if (!destination && !axis_end) {
        return false;
    }

    transform_session_->current_preview = input;
    return true;
}

bool SketchInteractionState::acceptTransformValue(
    double value) noexcept {
    if (!transform_session_ ||
        transform_session_->stage !=
            CommonTransformStage::await_destination ||
        !transform_session_->base_point ||
        !transform_session_->reference_point ||
        !std::isfinite(value)) {
        return false;
    }

    if (tool_ == SketchTool::rotate) {
        transform_session_->explicit_value = value;
        return true;
    }

    if (tool_ == SketchTool::scale &&
        value > 0.0) {
        transform_session_->explicit_value = value;
        return true;
    }

    return false;
}

std::optional<SketchTransformGeometry>
SketchInteractionState::transformGeometryState() const {
    if (!commonTransformTool() ||
        !transform_session_ ||
        !transform_session_->base_point ||
        !transform_session_->current_preview) {
        return std::nullopt;
    }

    const auto base =
        *transform_session_->base_point;
    const auto current =
        transform_session_->current_preview->position;

    if ((tool_ == SketchTool::move ||
         tool_ == SketchTool::copy) &&
        transform_session_->stage ==
            CommonTransformStage::await_destination) {
        return translateSketchGeometry(
            transform_session_->initial_geometry,
            {
                current.u - base.u,
                current.v - base.v});
    }

    if ((tool_ == SketchTool::rotate ||
         tool_ == SketchTool::scale) &&
        transform_session_->stage ==
            CommonTransformStage::await_destination &&
        transform_session_->reference_point) {
        const auto reference =
            *transform_session_->reference_point;

        if (tool_ == SketchTool::rotate) {
            if (transform_session_->explicit_value) {
                return rotateSketchGeometry(
                    transform_session_->initial_geometry,
                    base,
                    *transform_session_->explicit_value);
            }

            const auto angle =
                signedAngle(
                    base,
                    reference,
                    current);
            return angle
                ? rotateSketchGeometry(
                      transform_session_->
                          initial_geometry,
                      base,
                      *angle)
                : std::nullopt;
        }

        if (transform_session_->explicit_value) {
            return *transform_session_->explicit_value > 0.0
                ? scaleSketchGeometry(
                      transform_session_->initial_geometry,
                      base,
                      *transform_session_->explicit_value)
                : std::nullopt;
        }

        const double reference_distance =
            distance(base, reference);
        const double current_distance =
            distance(base, current);
        if (!std::isfinite(reference_distance) ||
            reference_distance <= 0.0 ||
            !std::isfinite(current_distance) ||
            current_distance <= 0.0) {
            return std::nullopt;
        }

        const double factor =
            current_distance /
            reference_distance;
        return scaleSketchGeometry(
            transform_session_->initial_geometry,
            base,
            factor);
    }

    if (tool_ == SketchTool::mirror &&
        transform_session_->stage ==
            CommonTransformStage::await_axis_end) {
        return mirrorSketchGeometry(
            transform_session_->initial_geometry,
            base,
            current);
    }

    return std::nullopt;
}

bool SketchInteractionState::continueCopyPlacement() noexcept {
    if (tool_ != SketchTool::copy ||
        !transform_session_ ||
        transform_session_->stage !=
            CommonTransformStage::await_destination ||
        !transform_session_->base_point ||
        transform_session_->selection_snapshot.empty() ||
        transform_session_->initial_geometry.empty()) {
        return false;
    }

    transform_session_->current_preview.reset();
    point_pointer_candidate_.reset();
    consumeTemporarySnapOverride();
    clearHover();
    return true;
}

void SketchInteractionState::finishTransform() noexcept {
    if (!commonTransformTool()) {
        return;
    }
    resetToSelect();
    clearHover();
}

bool SketchInteractionState::completeMoveSelection(
    const SketchModel& model) {
    return tool_ == SketchTool::move &&
           completeTransformSelection(model);
}

bool SketchInteractionState::acceptMoveBasePoint(
    ResolvedSketchInput input) noexcept {
    return tool_ == SketchTool::move &&
           transform_session_ &&
           transform_session_->stage ==
               CommonTransformStage::await_base_point &&
           acceptTransformPoint(input);
}

bool SketchInteractionState::updateMoveDestination(
    ResolvedSketchInput input) noexcept {
    return tool_ == SketchTool::move &&
           updateTransformPreview(input);
}

std::optional<SketchTransformGeometry>
SketchInteractionState::moveGeometryState() const {
    return tool_ == SketchTool::move
        ? transformGeometryState()
        : std::nullopt;
}

void SketchInteractionState::finishMove() noexcept {
    if (tool_ == SketchTool::move) {
        finishTransform();
    }
}

LinePointResult
SketchInteractionState::acceptLinePoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::line) {
        return {
            LinePointOutcome::inactive_tool,
            std::nullopt};
    }

    if (!point.finite()) {
        return {
            LinePointOutcome::invalid_point,
            std::nullopt};
    }

    if (line_stage_ ==
        LineStage::await_first_point) {
        line_anchor_ = point;
        point_pointer_candidate_ = point;
        point_field_locks_ = {};
        consumeTemporarySnapOverride();
        line_stage_ =
            LineStage::await_next_point;
        return {
            LinePointOutcome::first_point_accepted,
            std::nullopt};
    }

    if (pending_line_request_) {
        return {
            LinePointOutcome::request_pending,
            std::nullopt};
    }

    if (!line_anchor_) {
        resetLineStage();
        return {
            LinePointOutcome::invalid_point,
            std::nullopt};
    }

    if (point == *line_anchor_) {
        return {
            LinePointOutcome::zero_length_ignored,
            std::nullopt};
    }

    LineSegmentIntent request{
        *line_anchor_,
        point};
    if (!request.valid()) {
        return {
            LinePointOutcome::invalid_point,
            std::nullopt};
    }

    pending_line_request_ = request;
    consumeTemporarySnapOverride();
    return {
        LinePointOutcome::segment_requested,
        request};
}

CirclePointResult
SketchInteractionState::acceptCirclePoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::circle) {
        return {
            CirclePointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!point.finite()) {
        return {
            CirclePointOutcome::invalid_point,
            std::nullopt};
    }

    if (circle_stage_ ==
        CircleStage::await_center) {
        circle_center_ = point;
        circle_radius_lock_.reset();
        point_field_locks_ = {};
        consumeTemporarySnapOverride();
        circle_stage_ =
            CircleStage::await_radius;
        return {
            CirclePointOutcome::center_accepted,
            std::nullopt};
    }

    if (pending_circle_request_) {
        return {
            CirclePointOutcome::request_pending,
            std::nullopt};
    }
    if (!circle_center_) {
        resetCircleStage();
        return {
            CirclePointOutcome::invalid_point,
            std::nullopt};
    }

    CircleIntent request{
        *circle_center_,
        circle_radius_lock_.value_or(
            distance(*circle_center_, point))};
    if (!request.valid()) {
        return {
            CirclePointOutcome::zero_radius_ignored,
            std::nullopt};
    }

    pending_circle_request_ = request;
    return {
        CirclePointOutcome::circle_requested,
        request};
}

bool SketchInteractionState::lockCircleRadius(
    double radius) noexcept {
    if (tool_ != SketchTool::circle ||
        circle_stage_ != CircleStage::await_radius ||
        pending_circle_request_ ||
        !circle_center_ ||
        !std::isfinite(radius) ||
        radius <= 0.0) {
        return false;
    }

    circle_radius_lock_ = radius;
    return true;
}

CirclePointResult
SketchInteractionState::acceptCircleRadius(
    double radius) noexcept {
    if (tool_ != SketchTool::circle) {
        return {
            CirclePointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!lockCircleRadius(radius)) {
        return {
            CirclePointOutcome::invalid_radius,
            std::nullopt};
    }

    CircleIntent request{
        *circle_center_,
        radius};
    if (!request.valid()) {
        return {
            CirclePointOutcome::invalid_radius,
            std::nullopt};
    }

    pending_circle_request_ = request;
    return {
        CirclePointOutcome::circle_requested,
        request};
}

ArcPointResult
SketchInteractionState::acceptArcPoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::arc) {
        return {
            ArcPointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!point.finite()) {
        return {
            ArcPointOutcome::invalid_point,
            std::nullopt};
    }

    if (arc_stage_ == ArcStage::await_start) {
        arc_start_ = point;
        arc_end_.reset();
        arc_radius_lock_.reset();
        point_pointer_candidate_ = point;
        point_field_locks_ = {};
        consumeTemporarySnapOverride();
        arc_stage_ = ArcStage::await_end;
        return {
            ArcPointOutcome::start_accepted,
            std::nullopt};
    }

    if (arc_stage_ == ArcStage::await_end) {
        if (!arc_start_ || point == *arc_start_) {
            return {
                ArcPointOutcome::degenerate_ignored,
                std::nullopt};
        }
        arc_end_ = point;
        arc_radius_lock_.reset();
        point_pointer_candidate_ = point;
        point_field_locks_ = {};
        consumeTemporarySnapOverride();
        arc_stage_ = ArcStage::await_arc_point;
        return {
            ArcPointOutcome::end_accepted,
            std::nullopt};
    }

    if (pending_arc_request_) {
        return {
            ArcPointOutcome::request_pending,
            std::nullopt};
    }
    if (!arc_start_ || !arc_end_) {
        resetArcStage();
        return {
            ArcPointOutcome::invalid_point,
            std::nullopt};
    }

    // A complete explicit Arc Point outranks any runtime Radius lock.
    arc_radius_lock_.reset();
    const auto request =
        arcThroughThreePoints(
            *arc_start_,
            point,
            *arc_end_);
    if (!request) {
        return {
            ArcPointOutcome::degenerate_ignored,
            std::nullopt};
    }

    pending_arc_request_ = *request;
    consumeTemporarySnapOverride();
    return {
        ArcPointOutcome::arc_requested,
        request};
}

ArcPointResult
SketchInteractionState::acceptArcPointer(
    Point2 point) noexcept {
    if (tool_ != SketchTool::arc ||
        arc_stage_ != ArcStage::await_arc_point ||
        !arc_radius_lock_) {
        return acceptArcPoint(point);
    }
    if (!point.finite()) {
        return {
            ArcPointOutcome::invalid_point,
            std::nullopt};
    }
    if (pending_arc_request_) {
        return {
            ArcPointOutcome::request_pending,
            std::nullopt};
    }
    if (!arc_start_ || !arc_end_) {
        resetArcStage();
        return {
            ArcPointOutcome::invalid_point,
            std::nullopt};
    }

    point_pointer_candidate_ = point;
    const auto request =
        arcFromChordRadius(
            *arc_start_,
            *arc_end_,
            point,
            *arc_radius_lock_);
    if (!request) {
        return {
            ArcPointOutcome::radius_locked,
            std::nullopt};
    }

    pending_arc_request_ = *request;
    consumeTemporarySnapOverride();
    return {
        ArcPointOutcome::arc_requested,
        request};
}

bool SketchInteractionState::lockArcRadius(
    double radius) noexcept {
    if (tool_ != SketchTool::arc ||
        arc_stage_ != ArcStage::await_arc_point ||
        pending_arc_request_ ||
        !arc_start_ ||
        !arc_end_ ||
        !std::isfinite(radius) ||
        radius <= 0.0) {
        return false;
    }

    const double chord =
        distance(*arc_start_, *arc_end_);
    if (!std::isfinite(chord) ||
        chord <= 0.0 ||
        radius < chord * 0.5) {
        return false;
    }

    arc_radius_lock_ = radius;
    return true;
}

ArcPointResult
SketchInteractionState::acceptArcRadius(
    double radius) noexcept {
    if (tool_ != SketchTool::arc) {
        return {
            ArcPointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!lockArcRadius(radius)) {
        return {
            ArcPointOutcome::invalid_radius,
            std::nullopt};
    }
    if (!point_pointer_candidate_) {
        return {
            ArcPointOutcome::radius_locked,
            std::nullopt};
    }

    const auto request =
        arcFromChordRadius(
            *arc_start_,
            *arc_end_,
            *point_pointer_candidate_,
            radius);
    if (!request) {
        return {
            ArcPointOutcome::radius_locked,
            std::nullopt};
    }

    pending_arc_request_ = *request;
    consumeTemporarySnapOverride();
    return {
        ArcPointOutcome::arc_requested,
        request};
}

RectanglePointResult
SketchInteractionState::acceptRectanglePoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::rectangle) {
        return {
            RectanglePointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!point.finite()) {
        return {
            RectanglePointOutcome::invalid_point,
            std::nullopt};
    }

    if (rectangle_stage_ ==
        RectangleStage::await_first_corner) {
        rectangle_first_corner_ = point;
        rectangle_width_lock_.reset();
        rectangle_height_lock_.reset();
        point_pointer_candidate_ = point;
        point_field_locks_ = {};
        consumeTemporarySnapOverride();
        rectangle_stage_ =
            RectangleStage::await_opposite_corner;
        return {
            RectanglePointOutcome::first_corner_accepted,
            std::nullopt};
    }

    if (pending_rectangle_request_) {
        return {
            RectanglePointOutcome::request_pending,
            std::nullopt};
    }
    if (!rectangle_first_corner_) {
        resetRectangleStage();
        return {
            RectanglePointOutcome::invalid_point,
            std::nullopt};
    }

    std::optional<RectangleIntent> request;
    if (rectangle_width_lock_ ||
        rectangle_height_lock_) {
        request =
            rectangleFromSizeLocks(
                *rectangle_first_corner_,
                point,
                rectangle_width_lock_,
                rectangle_height_lock_);
    } else {
        RectangleIntent candidate{
            *rectangle_first_corner_,
            point};
        if (candidate.valid()) {
            request = candidate;
        }
    }
    if (!request) {
        return {
            RectanglePointOutcome::degenerate_ignored,
            std::nullopt};
    }

    pending_rectangle_request_ = *request;
    consumeTemporarySnapOverride();
    return {
        RectanglePointOutcome::rectangle_requested,
        request};
}

bool SketchInteractionState::lockRectangleWidth(
    double width) noexcept {
    if (tool_ != SketchTool::rectangle ||
        rectangle_stage_ !=
            RectangleStage::await_opposite_corner ||
        pending_rectangle_request_ ||
        !rectangle_first_corner_ ||
        !std::isfinite(width) ||
        width <= 0.0) {
        return false;
    }

    rectangle_width_lock_ = width;
    return true;
}

bool SketchInteractionState::lockRectangleHeight(
    double height) noexcept {
    if (tool_ != SketchTool::rectangle ||
        rectangle_stage_ !=
            RectangleStage::await_opposite_corner ||
        pending_rectangle_request_ ||
        !rectangle_first_corner_ ||
        !std::isfinite(height) ||
        height <= 0.0) {
        return false;
    }

    rectangle_height_lock_ = height;
    return true;
}

RectanglePointResult
SketchInteractionState::acceptRectangleSize(
    double width,
    double height) noexcept {
    if (tool_ != SketchTool::rectangle) {
        return {
            RectanglePointOutcome::inactive_tool,
            std::nullopt};
    }
    if (rectangle_stage_ !=
            RectangleStage::await_opposite_corner ||
        pending_rectangle_request_ ||
        !rectangle_first_corner_ ||
        !std::isfinite(width) ||
        !std::isfinite(height) ||
        width <= 0.0 ||
        height <= 0.0) {
        return {
            RectanglePointOutcome::invalid_size,
            std::nullopt};
    }

    rectangle_width_lock_ = width;
    rectangle_height_lock_ = height;

    if (!point_pointer_candidate_) {
        return {
            RectanglePointOutcome::size_locked,
            std::nullopt};
    }

    const auto request =
        rectangleFromSize(
            *rectangle_first_corner_,
            *point_pointer_candidate_,
            width,
            height);
    if (!request) {
        return {
            RectanglePointOutcome::size_locked,
            std::nullopt};
    }

    pending_rectangle_request_ = *request;
    consumeTemporarySnapOverride();
    return {
        RectanglePointOutcome::rectangle_requested,
        request};
}

bool SketchInteractionState::resolveLineRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::line ||
        line_stage_ !=
            LineStage::await_next_point ||
        !pending_line_request_) {
        return false;
    }

    const auto resolved =
        *pending_line_request_;
    pending_line_request_.reset();

    if (committed) {
        line_relative_reference_ =
            direction(
                resolved.start,
                resolved.end);
        line_anchor_ = resolved.end;
        point_pointer_candidate_ = resolved.end;
        point_field_locks_ = {};
    }

    return true;
}

bool SketchInteractionState::resolveCircleRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::circle ||
        circle_stage_ !=
            CircleStage::await_radius ||
        !pending_circle_request_) {
        return false;
    }

    pending_circle_request_.reset();
    if (committed) {
        resetCircleStage();
    }
    return true;
}

bool SketchInteractionState::resolveArcRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::arc ||
        arc_stage_ != ArcStage::await_arc_point ||
        !pending_arc_request_) {
        return false;
    }

    pending_arc_request_.reset();
    if (committed) {
        resetArcStage();
    }
    return true;
}

bool SketchInteractionState::resolveRectangleRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::rectangle ||
        rectangle_stage_ !=
            RectangleStage::await_opposite_corner ||
        !pending_rectangle_request_) {
        return false;
    }

    pending_rectangle_request_.reset();
    if (committed) {
        resetRectangleStage();
    }
    return true;
}

std::optional<LineSegmentIntent>
SketchInteractionState::previewLine(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::line ||
        line_stage_ !=
            LineStage::await_next_point ||
        !line_anchor_ ||
        pending_line_request_ ||
        !current.finite() ||
        current == *line_anchor_) {
        return std::nullopt;
    }

    LineSegmentIntent preview{
        *line_anchor_,
        current};

    return preview.valid()
        ? std::optional<LineSegmentIntent>{
              preview}
        : std::nullopt;
}

std::optional<CircleIntent>
SketchInteractionState::previewCircle(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::circle ||
        circle_stage_ !=
            CircleStage::await_radius ||
        !circle_center_ ||
        pending_circle_request_ ||
        !current.finite()) {
        return std::nullopt;
    }

    CircleIntent preview{
        *circle_center_,
        circle_radius_lock_.value_or(
            distance(*circle_center_, current))};
    return preview.valid()
        ? std::optional<CircleIntent>{preview}
        : std::nullopt;
}

std::optional<ArcIntent>
SketchInteractionState::previewArc(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::arc ||
        arc_stage_ != ArcStage::await_arc_point ||
        !arc_start_ ||
        !arc_end_ ||
        pending_arc_request_ ||
        !current.finite()) {
        return std::nullopt;
    }

    if (arc_radius_lock_) {
        return arcFromChordRadius(
            *arc_start_,
            *arc_end_,
            current,
            *arc_radius_lock_);
    }

    return arcThroughThreePoints(
        *arc_start_,
        current,
        *arc_end_);
}

std::optional<RectangleIntent>
SketchInteractionState::previewRectangle(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::rectangle ||
        rectangle_stage_ !=
            RectangleStage::await_opposite_corner ||
        !rectangle_first_corner_ ||
        pending_rectangle_request_ ||
        !current.finite()) {
        return std::nullopt;
    }

    if (rectangle_width_lock_ ||
        rectangle_height_lock_) {
        return rectangleFromSizeLocks(
            *rectangle_first_corner_,
            current,
            rectangle_width_lock_,
            rectangle_height_lock_);
    }

    RectangleIntent preview{
        *rectangle_first_corner_,
        current};
    return preview.valid()
        ? std::optional<RectangleIntent>{preview}
        : std::nullopt;
}

void SketchInteractionState::finishTool() noexcept {
    manipulation_.reset();
    clearHover();
    resetToSelect();
}

void SketchInteractionState::cancelTool() noexcept {
    manipulation_.reset();
    clearHover();
    resetToSelect();
}

bool SketchInteractionState::
clearRequestLocalNumericLocks() noexcept {
    bool cleared = false;

    if (!point_field_locks_.empty()) {
        point_field_locks_ = {};
        cleared = true;
    }

    if (circle_radius_lock_) {
        circle_radius_lock_.reset();
        cleared = true;
    }

    if (arc_radius_lock_) {
        arc_radius_lock_.reset();
        cleared = true;
    }

    if (rectangle_width_lock_ ||
        rectangle_height_lock_) {
        rectangle_width_lock_.reset();
        rectangle_height_lock_.reset();
        cleared = true;
    }

    if (transform_session_ &&
        transform_session_->explicit_value) {
        transform_session_->explicit_value.reset();
        cleared = true;
    }

    if (manipulation_ &&
        manipulation_->explicit_value) {
        manipulation_->explicit_value.reset();
        cleared = true;
    }

    return cleared;
}

bool SketchInteractionState::escape() noexcept {
    if (clearTemporarySnapOverride()) {
        return true;
    }
    if (clearRequestLocalNumericLocks()) {
        return true;
    }

    if (manipulation_) {
        cancelDirectManipulation();
        return true;
    }

    if (tool_ == SketchTool::select) {
        if (selected_.empty()) {
            return false;
        }
        clearSelection();
        clearHover();
        return true;
    }

    if (tool_ == SketchTool::line) {
        if (line_stage_ ==
            LineStage::await_next_point) {
            resetLineStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (tool_ == SketchTool::circle) {
        if (circle_stage_ ==
            CircleStage::await_radius) {
            resetCircleStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (tool_ == SketchTool::arc) {
        if (arc_stage_ != ArcStage::await_start) {
            resetArcStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (tool_ == SketchTool::rectangle) {
        if (rectangle_stage_ ==
            RectangleStage::await_opposite_corner) {
            resetRectangleStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (commonTransformTool()) {
        resetCommonTransform();
        resetToSelect();
        clearHover();
        return true;
    }

    if (tool_ == SketchTool::measure) {
        if (measure_between_active_) {
            leaveMeasureBetween();
            return true;
        }
        resetToSelect();
        clearHover();
        return true;
    }

    return false;
}

void SketchInteractionState::cancelForHistory() noexcept {
    manipulation_.reset();
    resetCommonTransform();
    clearHover();
    resetToSelect();
}

bool SketchInteractionState::addSelection(
    EntityId id) {
    if (!selectionMutable() || !id.valid()) {
        return false;
    }

    if (std::find(
            selected_.begin(),
            selected_.end(),
            id) == selected_.end()) {
        selected_.push_back(id);
    }
    primary_ = id;
    return true;
}

bool SketchInteractionState::addSelection(
    std::vector<EntityId> ids) {
    if (!selectionMutable()) {
        return false;
    }

    std::set<EntityId> incoming;
    for (const auto id : ids) {
        if (!id.valid() ||
            !incoming.insert(id).second) {
            return false;
        }
    }

    for (const auto id : incoming) {
        if (std::find(
                selected_.begin(),
                selected_.end(),
                id) == selected_.end()) {
            selected_.push_back(id);
        }
    }
    return true;
}

bool SketchInteractionState::replaceSelection(
    EntityId id) {
    if (!selectionMutable() || !id.valid()) {
        return false;
    }

    selected_ = {id};
    primary_ = id;
    return true;
}

bool SketchInteractionState::toggleSelection(
    EntityId id) {
    if (!selectionMutable() || !id.valid()) {
        return false;
    }

    const auto found =
        std::find(
            selected_.begin(),
            selected_.end(),
            id);

    if (found == selected_.end()) {
        selected_.push_back(id);
        primary_ = id;
        return true;
    }

    const bool removed_primary =
        primary_ && *primary_ == id;
    selected_.erase(found);

    if (selected_.empty()) {
        primary_.reset();
    } else if (removed_primary) {
        primary_ = deterministicPrimary();
    }

    return true;
}

bool SketchInteractionState::toggleSelection(
    std::vector<EntityId> ids) {
    if (!selectionMutable()) {
        return false;
    }

    std::set<EntityId> incoming;
    for (const auto id : ids) {
        if (!id.valid() ||
            !incoming.insert(id).second) {
            return false;
        }
    }

    bool removed_primary = false;
    for (const auto id : incoming) {
        const auto found =
            std::find(
                selected_.begin(),
                selected_.end(),
                id);
        if (found == selected_.end()) {
            selected_.push_back(id);
        } else {
            removed_primary =
                removed_primary ||
                (primary_ && *primary_ == id);
            selected_.erase(found);
        }
    }

    if (selected_.empty()) {
        primary_.reset();
    } else if (removed_primary) {
        primary_ = deterministicPrimary();
    }

    return true;
}

void SketchInteractionState::clearSelection() noexcept {
    if (!selectionMutable()) {
        return;
    }
    selected_.clear();
    primary_.reset();
}

bool SketchInteractionState::replaceSelection(
    std::vector<EntityId> ids,
    std::optional<EntityId> primary) {
    if (!selectionMutable()) {
        return false;
    }

    std::set<EntityId> unique;

    for (const auto id : ids) {
        if (!id.valid() ||
            !unique.insert(id).second) {
            return false;
        }
    }

    if (primary) {
        if (!primary->valid() ||
            unique.find(*primary) ==
                unique.end()) {
            return false;
        }
    }

    selected_ = std::move(ids);
    primary_ = primary;
    return true;
}

void SketchInteractionState::reconcileSelection(
    const SketchModel& model) {
    if (!selectionMutable()) {
        return;
    }

    selected_.erase(
        std::remove_if(
            selected_.begin(),
            selected_.end(),
            [&model](EntityId id) {
                return !model.contains(id);
            }),
        selected_.end());

    if (primary_ &&
        std::find(
            selected_.begin(),
            selected_.end(),
            *primary_) ==
            selected_.end()) {
        primary_ = deterministicPrimary();
    }
}

bool SketchInteractionState::setHoveredEntity(
    std::optional<EntityId> entity) noexcept {
    const bool hover_allowed =
        tool_ == SketchTool::select ||
        (commonTransformTool() &&
         transform_session_ &&
         transform_session_->stage ==
             CommonTransformStage::select_objects);

    if (!hover_allowed || manipulation_) {
        return false;
    }
    if (entity && !entity->valid()) {
        return false;
    }

    const bool changed =
        hovered_entity_ != entity ||
        hovered_grip_.has_value();
    hovered_entity_ = entity;
    hovered_grip_.reset();
    return changed;
}

bool SketchInteractionState::setHoveredGrip(
    std::optional<SketchGripRef> grip) noexcept {
    if (tool_ != SketchTool::select ||
        manipulation_) {
        return false;
    }
    if (grip && !grip->valid()) {
        return false;
    }

    const bool changed =
        hovered_grip_ != grip ||
        hovered_entity_.has_value();
    hovered_grip_ = grip;
    hovered_entity_.reset();
    return changed;
}

void SketchInteractionState::clearHover() noexcept {
    hovered_entity_.reset();
    hovered_grip_.reset();
}

std::optional<SketchGripRef>
SketchInteractionState::activeGrip() const noexcept {
    if (!manipulation_) {
        return std::nullopt;
    }
    return manipulation_->active_grip;
}

std::optional<DirectEditMode>
SketchInteractionState::directEditMode() const noexcept {
    if (!manipulation_) {
        return std::nullopt;
    }
    return manipulation_->mode;
}

bool SketchInteractionState::directManipulationCopyEnabled()
    const noexcept {
    return manipulation_.has_value() &&
           manipulation_->copy_enabled;
}

bool SketchInteractionState::enableDirectManipulationCopy()
    noexcept {
    if (!manipulation_ ||
        (manipulation_->mode != DirectEditMode::reshape &&
         manipulation_->mode != DirectEditMode::move)) {
        return false;
    }
    manipulation_->copy_enabled = true;
    return true;
}

bool SketchInteractionState::cycleDirectEditMode() noexcept {
    if (!manipulation_) {
        return false;
    }

    const bool center_grip =
        manipulation_->active_grip.role ==
            SketchGripRole::line_center ||
        manipulation_->active_grip.role ==
            SketchGripRole::circle_center ||
        manipulation_->active_grip.role ==
            SketchGripRole::arc_center;

    DirectEditMode next = manipulation_->mode;
    if (center_grip) {
        switch (manipulation_->mode) {
        case DirectEditMode::reshape:
        case DirectEditMode::mirror:
            next = DirectEditMode::move;
            break;
        case DirectEditMode::move:
            next = DirectEditMode::rotate;
            break;
        case DirectEditMode::rotate:
            next = DirectEditMode::scale;
            break;
        case DirectEditMode::scale:
            next = DirectEditMode::mirror;
            break;
        }
    } else {
        switch (manipulation_->mode) {
        case DirectEditMode::reshape:
            next = DirectEditMode::move;
            break;
        case DirectEditMode::move:
            next = DirectEditMode::rotate;
            break;
        case DirectEditMode::rotate:
            next = DirectEditMode::scale;
            break;
        case DirectEditMode::scale:
            next = DirectEditMode::mirror;
            break;
        case DirectEditMode::mirror:
            next = DirectEditMode::reshape;
            break;
        }
    }

    manipulation_->mode = next;
    manipulation_->copy_enabled = false;
    manipulation_->rotate_reference_point.reset();
    manipulation_->scale_reference_radius.reset();
    manipulation_->explicit_value.reset();
    point_field_locks_ = {};

    const auto current =
        manipulation_->current_input.position;
    if (next == DirectEditMode::rotate &&
        current.finite() &&
        current != manipulation_->pivot) {
        manipulation_->rotate_reference_point =
            current;
    } else if (next == DirectEditMode::scale &&
               current.finite()) {
        const double radius =
            distance(
                manipulation_->pivot,
                current);
        if (std::isfinite(radius) &&
            radius > 0.0) {
            manipulation_->scale_reference_radius =
                radius;
        }
    }

    return true;
}

bool SketchInteractionState::beginDirectManipulation(
    const SketchModel& model,
    SketchGripRef grip) {
    if (tool_ != SketchTool::select ||
        manipulation_ ||
        !grip.valid() ||
        std::find(
            selected_.begin(),
            selected_.end(),
            grip.entity_id) == selected_.end()) {
        return false;
    }

    DirectManipulationSession session;
    session.active_grip = grip;
    session.selection_snapshot = selected_;

    const auto captured_selection =
        captureSketchTransformGeometry(
            model,
            session.selection_snapshot);
    if (!captured_selection) {
        return false;
    }
    session.selection_geometry = *captured_selection;

    switch (grip.role) {
    case SketchGripRole::line_start: {
        const auto* owner =
            model.findLine(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.pivot = owner->start();
        session.owner_geometry.lines.push_back(
            {
                owner->id(),
                owner->start(),
                owner->end(),
                owner->role()});
        break;
    }
    case SketchGripRole::line_end: {
        const auto* owner =
            model.findLine(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.pivot = owner->end();
        session.owner_geometry.lines.push_back(
            {owner->id(), owner->start(), owner->end()});
        break;
    }
    case SketchGripRole::line_center: {
        const auto* owner =
            model.findLine(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::move;
        session.pivot = lineCenter(*owner);
        break;
    }
    case SketchGripRole::circle_center: {
        const auto* owner =
            model.findCircle(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::move;
        session.pivot = owner->center();
        break;
    }
    case SketchGripRole::circle_quadrant_pos_u:
    case SketchGripRole::circle_quadrant_pos_v:
    case SketchGripRole::circle_quadrant_neg_u:
    case SketchGripRole::circle_quadrant_neg_v: {
        const auto* owner =
            model.findCircle(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.owner_geometry.circles.push_back(
            {
                owner->id(),
                owner->center(),
                owner->radius(),
                owner->role()});

        Point2 direction_vector{};
        if (grip.role ==
            SketchGripRole::circle_quadrant_pos_u) {
            direction_vector = {owner->radius(), 0.0};
        } else if (grip.role ==
                   SketchGripRole::circle_quadrant_pos_v) {
            direction_vector = {0.0, owner->radius()};
        } else if (grip.role ==
                   SketchGripRole::circle_quadrant_neg_u) {
            direction_vector = {-owner->radius(), 0.0};
        } else {
            direction_vector = {0.0, -owner->radius()};
        }
        session.pivot = {
            owner->center().u + direction_vector.u,
            owner->center().v + direction_vector.v};
        break;
    }
    case SketchGripRole::arc_center: {
        const auto* owner =
            model.findArc(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::move;
        session.pivot = owner->center();
        break;
    }
    case SketchGripRole::arc_start:
    case SketchGripRole::arc_end:
    case SketchGripRole::arc_mid: {
        const auto* owner =
            model.findArc(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.owner_geometry.arcs.push_back(
            {
                owner->id(),
                owner->center(),
                owner->radius(),
                owner->startAngle(),
                owner->sweepAngle(),
                owner->role()});

        double angle = owner->startAngle();
        if (grip.role == SketchGripRole::arc_end) {
            angle += owner->sweepAngle();
        } else if (grip.role ==
                   SketchGripRole::arc_mid) {
            angle += owner->sweepAngle() * 0.5;
        }
        session.pivot =
            pointOnCircle(
                owner->center(),
                owner->radius(),
                angle);
        break;
    }
    }

    session.current_input =
        ResolvedSketchInput{session.pivot};
    point_pointer_candidate_ = session.pivot;
    manipulation_ = std::move(session);
    clearHover();
    return true;
}

bool SketchInteractionState::updateDirectManipulation(
    ResolvedSketchInput input) noexcept {
    if (!manipulation_ || !input.valid()) {
        return false;
    }

    if (manipulation_->mode ==
            DirectEditMode::rotate &&
        !manipulation_->rotate_reference_point &&
        input.position != manipulation_->pivot) {
        manipulation_->rotate_reference_point =
            input.position;
    }

    if (manipulation_->mode ==
            DirectEditMode::scale &&
        !manipulation_->scale_reference_radius) {
        const double radius =
            distance(
                manipulation_->pivot,
                input.position);
        if (std::isfinite(radius) &&
            radius > 0.0) {
            manipulation_->scale_reference_radius =
                radius;
        }
    }

    manipulation_->current_input = input;
    return true;
}

bool SketchInteractionState::acceptDirectManipulationValue(
    double value) noexcept {
    if (!manipulation_ ||
        !std::isfinite(value)) {
        return false;
    }

    switch (manipulation_->mode) {
    case DirectEditMode::rotate:
    case DirectEditMode::mirror:
        manipulation_->explicit_value = value;
        return true;

    case DirectEditMode::scale:
        if (value <= 0.0) {
            return false;
        }
        manipulation_->explicit_value = value;
        return true;

    case DirectEditMode::reshape:
    case DirectEditMode::move:
        return false;
    }

    return false;
}

std::optional<DirectManipulationGeometry>
SketchInteractionState::directManipulationGeometryState()
    const {
    if (!manipulation_ ||
        !manipulation_->current_input.valid()) {
        return std::nullopt;
    }

    const auto current =
        manipulation_->current_input.position;

    switch (manipulation_->mode) {
    case DirectEditMode::move:
        return translateSketchGeometry(
            manipulation_->selection_geometry,
            Point2{
                current.u - manipulation_->pivot.u,
                current.v - manipulation_->pivot.v});

    case DirectEditMode::rotate:
        if (manipulation_->explicit_value) {
            return rotateSketchGeometry(
                manipulation_->selection_geometry,
                manipulation_->pivot,
                *manipulation_->explicit_value);
        }
        if (!manipulation_->rotate_reference_point) {
            return std::nullopt;
        }
        if (const auto angle =
                signedAngle(
                    manipulation_->pivot,
                    *manipulation_->
                        rotate_reference_point,
                    current)) {
            return rotateSketchGeometry(
                manipulation_->selection_geometry,
                manipulation_->pivot,
                *angle);
        }
        return std::nullopt;

    case DirectEditMode::scale:
        if (manipulation_->explicit_value) {
            return *manipulation_->explicit_value > 0.0
                ? scaleSketchGeometry(
                      manipulation_->selection_geometry,
                      manipulation_->pivot,
                      *manipulation_->explicit_value)
                : std::nullopt;
        }
        if (!manipulation_->scale_reference_radius) {
            return std::nullopt;
        } else {
            const double current_radius =
                distance(
                    manipulation_->pivot,
                    current);
            if (!std::isfinite(current_radius) ||
                current_radius <= 0.0) {
                return std::nullopt;
            }
            const double factor =
                current_radius /
                *manipulation_->
                    scale_reference_radius;
            return std::isfinite(factor) &&
                           factor > 0.0
                ? scaleSketchGeometry(
                      manipulation_->selection_geometry,
                      manipulation_->pivot,
                      factor)
                : std::nullopt;
        }

    case DirectEditMode::mirror:
        if (manipulation_->explicit_value) {
            return mirrorSketchGeometry(
                manipulation_->selection_geometry,
                manipulation_->pivot,
                pointOnCircle(
                    manipulation_->pivot,
                    1.0,
                    *manipulation_->explicit_value));
        }
        return mirrorSketchGeometry(
            manipulation_->selection_geometry,
            manipulation_->pivot,
            current);

    case DirectEditMode::reshape:
        break;
    }

    auto result =
        manipulation_->owner_geometry;

    switch (manipulation_->active_grip.role) {
    case SketchGripRole::line_start:
    case SketchGripRole::line_end:
        if (result.lines.size() != 1U ||
            !result.circles.empty() ||
            !result.arcs.empty()) {
            return std::nullopt;
        }
        if (manipulation_->active_grip.role ==
            SketchGripRole::line_start) {
            result.lines.front().start = current;
        } else {
            result.lines.front().end = current;
        }
        return validReplacement(result.lines.front())
            ? std::optional<DirectManipulationGeometry>{
                  std::move(result)}
            : std::nullopt;

    case SketchGripRole::circle_quadrant_pos_u:
    case SketchGripRole::circle_quadrant_pos_v:
    case SketchGripRole::circle_quadrant_neg_u:
    case SketchGripRole::circle_quadrant_neg_v:
        if (result.circles.size() != 1U ||
            !result.lines.empty() ||
            !result.arcs.empty()) {
            return std::nullopt;
        }
        result.circles.front().radius =
            distance(
                result.circles.front().center,
                current);
        return validReplacement(
                   result.circles.front())
            ? std::optional<DirectManipulationGeometry>{
                  std::move(result)}
            : std::nullopt;

    case SketchGripRole::arc_mid:
        if (result.arcs.size() != 1U ||
            !result.lines.empty() ||
            !result.circles.empty()) {
            return std::nullopt;
        }
        result.arcs.front().radius =
            distance(
                result.arcs.front().center,
                current);
        return validReplacement(result.arcs.front())
            ? std::optional<DirectManipulationGeometry>{
                  std::move(result)}
            : std::nullopt;

    case SketchGripRole::arc_start:
        if (result.arcs.size() != 1U ||
            !result.lines.empty() ||
            !result.circles.empty()) {
            return std::nullopt;
        } else {
            auto& arc = result.arcs.front();
            if (current == arc.center) {
                return std::nullopt;
            }
            const double end_angle =
                arc.start_angle +
                arc.sweep_angle;
            const double new_start =
                direction(arc.center, current);
            const auto sweep =
                sameDirectionSweep(
                    new_start,
                    end_angle,
                    arc.sweep_angle);
            if (!sweep) return std::nullopt;
            arc.start_angle = new_start;
            arc.sweep_angle = *sweep;
            return validReplacement(arc)
                ? std::optional<
                      DirectManipulationGeometry>{
                      std::move(result)}
                : std::nullopt;
        }

    case SketchGripRole::arc_end:
        if (result.arcs.size() != 1U ||
            !result.lines.empty() ||
            !result.circles.empty()) {
            return std::nullopt;
        } else {
            auto& arc = result.arcs.front();
            if (current == arc.center) {
                return std::nullopt;
            }
            const double end_angle =
                direction(arc.center, current);
            const auto sweep =
                sameDirectionSweep(
                    arc.start_angle,
                    end_angle,
                    arc.sweep_angle);
            if (!sweep) return std::nullopt;
            arc.sweep_angle = *sweep;
            return validReplacement(arc)
                ? std::optional<
                      DirectManipulationGeometry>{
                      std::move(result)}
                : std::nullopt;
        }

    case SketchGripRole::line_center:
    case SketchGripRole::circle_center:
    case SketchGripRole::arc_center:
        return std::nullopt;
    }

    return std::nullopt;
}

bool SketchInteractionState::
continueDirectManipulationCopyPlacement() noexcept {
    if (!manipulation_ ||
        !manipulation_->copy_enabled) {
        return false;
    }

    manipulation_->current_input =
        ResolvedSketchInput{manipulation_->pivot};
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    consumeTemporarySnapOverride();
    clearHover();
    return true;
}

std::optional<std::vector<SketchLineState>>
SketchInteractionState::directManipulationGeometry()
    const {
    const auto geometry =
        directManipulationGeometryState();
    if (!geometry ||
        !geometry->circles.empty() ||
        !geometry->arcs.empty()) {
        return std::nullopt;
    }
    return geometry->lines;
}

void SketchInteractionState::finishDirectManipulation()
    noexcept {
    manipulation_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    consumeTemporarySnapOverride();
    clearHover();
}

void SketchInteractionState::cancelDirectManipulation()
    noexcept {
    manipulation_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    temporary_snap_override_.reset();
    clearHover();
}

std::optional<EntityId>
SketchInteractionState::deterministicPrimary()
    const noexcept {
    if (selected_.empty()) {
        return std::nullopt;
    }

    return *std::min_element(
        selected_.begin(),
        selected_.end());
}

void SketchInteractionState::resetToSelect() noexcept {
    resetMeasure();
    tool_ = SketchTool::select;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetRectangleStage();
    resetCommonTransform();
}

void SketchInteractionState::resetLineStage()
    noexcept {
    line_stage_ =
        LineStage::await_first_point;
    line_anchor_.reset();
    line_relative_reference_.reset();
    pending_line_request_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    temporary_snap_override_.reset();
}

void SketchInteractionState::resetCircleStage()
    noexcept {
    circle_stage_ =
        CircleStage::await_center;
    circle_center_.reset();
    circle_radius_lock_.reset();
    pending_circle_request_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    temporary_snap_override_.reset();
}

void SketchInteractionState::resetArcStage()
    noexcept {
    arc_stage_ =
        ArcStage::await_start;
    arc_start_.reset();
    arc_end_.reset();
    arc_radius_lock_.reset();
    pending_arc_request_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    temporary_snap_override_.reset();
}

void SketchInteractionState::resetRectangleStage()
    noexcept {
    rectangle_stage_ =
        RectangleStage::await_first_corner;
    rectangle_first_corner_.reset();
    rectangle_width_lock_.reset();
    rectangle_height_lock_.reset();
    pending_rectangle_request_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    temporary_snap_override_.reset();
}

void SketchInteractionState::resetCommonTransform()
    noexcept {
    transform_session_.reset();
    point_pointer_candidate_.reset();
    point_field_locks_ = {};
    temporary_snap_override_.reset();
}

void SketchInteractionState::resetMeasure() noexcept {
    measure_target_.reset();
    measure_between_active_ = false;
    measure_first_target_.reset();
    measure_second_target_.reset();
}

} // namespace simplesolid2::sketch
