#include <simplesolid2/sketch/interaction_state.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-07F precision input CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

bool near(double a, double b) {
    return std::abs(a - b) < 1.0e-12;
}
} // namespace

int main() {
    sketch::SketchInteractionState state;

    state.activateLine();
    auto request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(!request->base.has_value());
    CHECK(!request->direct_distance_enabled);
    CHECK(request->absolute_cartesian_enabled);
    CHECK(!request->relative_cartesian_enabled);
    CHECK(!request->relative_polar_enabled);
    CHECK(!state.resolveDirectDistance(50.0).has_value());

    const auto first =
        state.resolveExplicitPoint(
            {sketch::ExplicitPointInputKind::
                 absolute_cartesian,
             10.0,
             20.0});
    CHECK(first.has_value());
    CHECK(
        state.acceptLinePoint(first->position).outcome ==
        sketch::LinePointOutcome::first_point_accepted);

    request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(request->base == sketch::Point2{10.0, 20.0});
    CHECK(request->direct_distance_enabled);
    CHECK(request->absolute_cartesian_enabled);
    CHECK(request->relative_cartesian_enabled);
    CHECK(request->relative_polar_enabled);
    CHECK(!state.resolveDirectDistance(50.0).has_value());

    const auto relative_cartesian =
        state.resolveExplicitPoint(
            {sketch::ExplicitPointInputKind::
                 relative_cartesian,
             5.0,
             -2.0});
    CHECK(relative_cartesian.has_value());
    CHECK(
        relative_cartesian->position ==
        sketch::Point2{15.0, 18.0});

    const auto relative_polar =
        state.resolveExplicitPoint(
            {sketch::ExplicitPointInputKind::
                 relative_polar,
             10.0,
             std::numbers::pi_v<double> / 2.0});
    CHECK(relative_polar.has_value());
    CHECK(near(relative_polar->position.u, 10.0));
    CHECK(near(relative_polar->position.v, 30.0));

    CHECK(state.resolvePointerInput({13.0, 24.0}).has_value());

    const auto direct = state.resolveDirectDistance(50.0);
    CHECK(direct.has_value());
    CHECK(near(direct->position.u, 40.0));
    CHECK(near(direct->position.v, 60.0));
    CHECK(
        near(
            std::hypot(
                direct->position.u - 10.0,
                direct->position.v - 20.0),
            50.0));

    const auto zero = state.resolveDirectDistance(0.0);
    CHECK(zero.has_value());
    CHECK(zero->position == sketch::Point2{10.0, 20.0});
    CHECK(!state.resolveDirectDistance(-1.0).has_value());

    const auto line_request =
        state.acceptLinePoint(direct->position);
    CHECK(
        line_request.outcome ==
        sketch::LinePointOutcome::segment_requested);
    CHECK(state.resolveLineRequest(true));

    request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(request->base == direct->position);
    CHECK(request->pointer_candidate == direct->position);
    CHECK(!state.resolveDirectDistance(25.0).has_value());

    sketch::SketchModel model;
    const auto line_id =
        model.addLine({0.0, 0.0}, {10.0, 0.0});
    state.finishTool();
    CHECK(state.addSelection(line_id));
    CHECK(state.activateMove(model));
    request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(!request->direct_distance_enabled);
    CHECK(request->absolute_cartesian_enabled);
    CHECK(!request->relative_cartesian_enabled);
    CHECK(!request->relative_polar_enabled);

    const auto base =
        state.resolvePointerInput({5.0, 5.0});
    CHECK(base.has_value());
    CHECK(state.acceptTransformPoint(*base));
    request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(request->base == sketch::Point2{5.0, 5.0});
    CHECK(request->direct_distance_enabled);
    CHECK(request->absolute_cartesian_enabled);
    CHECK(request->relative_cartesian_enabled);
    CHECK(request->relative_polar_enabled);

    const auto move_relative =
        state.resolveExplicitPoint(
            {sketch::ExplicitPointInputKind::
                 relative_cartesian,
             3.0,
             4.0});
    CHECK(move_relative.has_value());
    CHECK(
        move_relative->position ==
        sketch::Point2{8.0, 9.0});

    CHECK(state.resolvePointerInput({8.0, 9.0}).has_value());

    const auto move50 = state.resolveDirectDistance(50.0);
    CHECK(move50.has_value());
    CHECK(near(move50->position.u, 35.0));
    CHECK(near(move50->position.v, 45.0));
    CHECK(state.updateTransformPreview(*move50));
    const auto moved = state.transformGeometryState();
    CHECK(moved.has_value());
    CHECK(moved->lines.size() == 1U);
    CHECK(moved->lines.front().start == sketch::Point2{30.0, 40.0});
    CHECK(moved->lines.front().end == sketch::Point2{40.0, 40.0});

    state.finishTransform();
    CHECK(state.addSelection(line_id));
    CHECK(
        state.beginDirectManipulation(
            model,
            {line_id, sketch::SketchGripRole::line_start}));
    CHECK(state.resolvePointerInput({0.0, 10.0}).has_value());
    const auto reshape50 = state.resolveDirectDistance(50.0);
    CHECK(reshape50.has_value());
    CHECK(reshape50->position == sketch::Point2{0.0, 50.0});
    CHECK(state.updateDirectManipulation(*reshape50));
    const auto reshaped = state.directManipulationGeometryState();
    CHECK(reshaped.has_value());
    CHECK(reshaped->lines.front().start == sketch::Point2{0.0, 50.0});

    CHECK(state.cycleDirectEditMode());
    const auto after_cycle = state.activePointRequest();
    CHECK(after_cycle.has_value());
    CHECK(after_cycle->pointer_candidate == sketch::Point2{0.0, 10.0});
    const auto move25 = state.resolveDirectDistance(25.0);
    CHECK(move25.has_value());
    CHECK(move25->position == sketch::Point2{0.0, 25.0});
    CHECK(state.updateDirectManipulation(*move25));
    const auto grip_moved = state.directManipulationGeometryState();
    CHECK(grip_moved.has_value());
    CHECK(grip_moved->lines.front().start == sketch::Point2{0.0, 25.0});
    CHECK(grip_moved->lines.front().end == sketch::Point2{10.0, 25.0});

    state.cancelDirectManipulation();
    CHECK(!state.activePointRequest().has_value());

    // Circle Center is an unbased exact point request. Size is a
    // separate typed Length request, so no generic point request remains
    // after Center is accepted.
    {
        sketch::SketchInteractionState circle;
        circle.activateCircle();
        auto circle_request = circle.activePointRequest();
        CHECK(circle_request.has_value());
        CHECK(!circle_request->base.has_value());
        CHECK(circle_request->absolute_cartesian_enabled);
        CHECK(!circle_request->relative_cartesian_enabled);
        CHECK(!circle_request->relative_polar_enabled);
        CHECK(!circle_request->direct_distance_enabled);

        const auto center =
            circle.resolveExplicitPoint(
                {sketch::ExplicitPointInputKind::
                     absolute_cartesian,
                 5.0,
                 6.0});
        CHECK(center.has_value());
        CHECK(
            circle.acceptCirclePoint(center->position).outcome ==
            sketch::CirclePointOutcome::center_accepted);
        CHECK(!circle.activePointRequest().has_value());

        CHECK(
            circle.acceptCircleRadius(0.0).outcome ==
            sketch::CirclePointOutcome::invalid_radius);
        const auto exact_circle =
            circle.acceptCircleRadius(7.5);
        CHECK(
            exact_circle.outcome ==
            sketch::CirclePointOutcome::circle_requested);
        CHECK(exact_circle.request.has_value());
        CHECK(near(exact_circle.request->radius, 7.5));
        CHECK(circle.resolveCircleRequest(true));
    }

    // Arc follows Start -> End -> Arc Point. End alone has Start as the
    // relative base; the third Arc Point does not invent a relative base.
    {
        sketch::SketchInteractionState arc;
        arc.activateArc();

        auto arc_request = arc.activePointRequest();
        CHECK(arc_request.has_value());
        CHECK(!arc_request->base.has_value());
        CHECK(arc_request->absolute_cartesian_enabled);
        CHECK(!arc_request->relative_cartesian_enabled);
        CHECK(!arc_request->relative_polar_enabled);
        CHECK(!arc_request->direct_distance_enabled);

        const auto start =
            arc.resolveExplicitPoint(
                {sketch::ExplicitPointInputKind::
                     absolute_cartesian,
                 1.0,
                 2.0});
        CHECK(start.has_value());
        CHECK(
            arc.acceptArcPoint(start->position).outcome ==
            sketch::ArcPointOutcome::start_accepted);

        arc_request = arc.activePointRequest();
        CHECK(arc_request.has_value());
        CHECK(arc_request->base == sketch::Point2{1.0, 2.0});
        CHECK(arc_request->absolute_cartesian_enabled);
        CHECK(arc_request->relative_cartesian_enabled);
        CHECK(arc_request->relative_polar_enabled);
        CHECK(!arc_request->direct_distance_enabled);

        const auto end =
            arc.resolveExplicitPoint(
                {sketch::ExplicitPointInputKind::
                     relative_cartesian,
                 4.0,
                 0.0});
        CHECK(end.has_value());
        CHECK(end->position == sketch::Point2{5.0, 2.0});
        CHECK(
            arc.acceptArcPoint(end->position).outcome ==
            sketch::ArcPointOutcome::end_accepted);

        arc_request = arc.activePointRequest();
        CHECK(arc_request.has_value());
        CHECK(!arc_request->base.has_value());
        CHECK(arc_request->absolute_cartesian_enabled);
        CHECK(!arc_request->relative_cartesian_enabled);
        CHECK(!arc_request->relative_polar_enabled);
        CHECK(!arc_request->direct_distance_enabled);
        CHECK(
            !arc.resolveExplicitPoint(
                    {sketch::ExplicitPointInputKind::
                         relative_cartesian,
                     1.0,
                     1.0})
                 .has_value());
    }

    // Rectangle second-stage pair is transient Width;Height. Pointer
    // supplies only quadrant; exact magnitudes remain locked.
    {
        sketch::SketchInteractionState rectangle;
        rectangle.activateRectangle();
        CHECK(
            rectangle.acceptRectanglePoint(
                         {10.0, 10.0}).outcome ==
            sketch::RectanglePointOutcome::
                first_corner_accepted);
        CHECK(
            rectangle.acceptRectangleSize(
                         4.0, 2.0).outcome ==
            sketch::RectanglePointOutcome::size_locked);
        CHECK(
            rectangle.resolvePointerInput(
                         {9.0, 11.0}).has_value());
        const auto exact_rectangle =
            rectangle.acceptRectanglePoint(
                {9.0, 11.0});
        CHECK(
            exact_rectangle.outcome ==
            sketch::RectanglePointOutcome::
                rectangle_requested);
        CHECK(exact_rectangle.request.has_value());
        CHECK(
            exact_rectangle.request->opposite_corner ==
            sketch::Point2{6.0, 12.0});
        CHECK(rectangle.resolveRectangleRequest(true));
    }

    // Exact Rotate Angle and Scale Factor are runtime locks. Once set,
    // later pointer motion cannot alter the exact numeric result.
    {
        sketch::SketchInteractionState rotate;
        CHECK(rotate.addSelection(line_id));
        CHECK(rotate.activateRotate(model));
        CHECK(
            rotate.acceptTransformPoint(
                sketch::ResolvedSketchInput{{0.0, 0.0}}));
        CHECK(
            rotate.acceptTransformPoint(
                sketch::ResolvedSketchInput{{1.0, 0.0}}));
        CHECK(
            rotate.acceptTransformValue(
                std::numbers::pi_v<double> / 2.0));
        CHECK(
            rotate.updateTransformPreview(
                sketch::ResolvedSketchInput{{-1.0, 0.0}}));

        const auto source =
            sketch::captureSketchTransformGeometry(
                model, {line_id});
        CHECK(source.has_value());
        const auto exact_rotate =
            rotate.transformGeometryState();
        const auto expected_rotate =
            sketch::rotateSketchGeometry(
                *source,
                {0.0, 0.0},
                std::numbers::pi_v<double> / 2.0);
        CHECK(exact_rotate.has_value());
        CHECK(expected_rotate.has_value());
        CHECK(*exact_rotate == *expected_rotate);
    }

    {
        sketch::SketchInteractionState scale;
        CHECK(scale.addSelection(line_id));
        CHECK(scale.activateScale(model));
        CHECK(
            scale.acceptTransformPoint(
                sketch::ResolvedSketchInput{{0.0, 0.0}}));
        CHECK(
            scale.acceptTransformPoint(
                sketch::ResolvedSketchInput{{1.0, 0.0}}));
        CHECK(!scale.acceptTransformValue(0.0));
        CHECK(scale.acceptTransformValue(2.5));
        CHECK(
            scale.updateTransformPreview(
                sketch::ResolvedSketchInput{{10.0, 0.0}}));

        const auto source =
            sketch::captureSketchTransformGeometry(
                model, {line_id});
        CHECK(source.has_value());
        const auto exact_scale =
            scale.transformGeometryState();
        const auto expected_scale =
            sketch::scaleSketchGeometry(
                *source,
                {0.0, 0.0},
                2.5);
        CHECK(exact_scale.has_value());
        CHECK(expected_scale.has_value());
        CHECK(*exact_scale == *expected_scale);
    }

    // Grip Rotate/Scale/Mirror exact values are request-local locks over
    // the same frozen interaction-start selection geometry.
    {
        sketch::SketchInteractionState grip;
        CHECK(grip.addSelection(line_id));
        CHECK(
            grip.beginDirectManipulation(
                model,
                {line_id,
                 sketch::SketchGripRole::line_start}));
        const auto pointer =
            grip.resolvePointerInput({1.0, 0.0});
        CHECK(pointer.has_value());
        CHECK(grip.updateDirectManipulation(*pointer));

        CHECK(grip.cycleDirectEditMode());
        CHECK(
            grip.directEditMode() ==
            sketch::DirectEditMode::move);
        CHECK(grip.cycleDirectEditMode());
        CHECK(
            grip.directEditMode() ==
            sketch::DirectEditMode::rotate);
        CHECK(
            grip.acceptDirectManipulationValue(
                std::numbers::pi_v<double> / 2.0));
        CHECK(
            grip.updateDirectManipulation(
                sketch::ResolvedSketchInput{
                    {-1.0, 0.0}}));

        const auto source =
            sketch::captureSketchTransformGeometry(
                model, {line_id});
        CHECK(source.has_value());
        const auto rotated =
            grip.directManipulationGeometryState();
        const auto expected_rotated =
            sketch::rotateSketchGeometry(
                *source,
                {0.0, 0.0},
                std::numbers::pi_v<double> / 2.0);
        CHECK(rotated.has_value());
        CHECK(expected_rotated.has_value());
        CHECK(*rotated == *expected_rotated);

        CHECK(grip.cycleDirectEditMode());
        CHECK(
            grip.directEditMode() ==
            sketch::DirectEditMode::scale);
        CHECK(!grip.acceptDirectManipulationValue(0.0));
        CHECK(grip.acceptDirectManipulationValue(2.0));
        CHECK(
            grip.updateDirectManipulation(
                sketch::ResolvedSketchInput{
                    {10.0, 0.0}}));
        const auto scaled =
            grip.directManipulationGeometryState();
        const auto expected_scaled =
            sketch::scaleSketchGeometry(
                *source,
                {0.0, 0.0},
                2.0);
        CHECK(scaled.has_value());
        CHECK(expected_scaled.has_value());
        CHECK(*scaled == *expected_scaled);

        CHECK(grip.cycleDirectEditMode());
        CHECK(
            grip.directEditMode() ==
            sketch::DirectEditMode::mirror);
        CHECK(
            grip.acceptDirectManipulationValue(
                std::numbers::pi_v<double> / 2.0));
        CHECK(
            grip.updateDirectManipulation(
                sketch::ResolvedSketchInput{
                    {1.0, 0.0}}));
        const auto mirrored =
            grip.directManipulationGeometryState();
        const auto expected_mirrored =
            sketch::mirrorSketchGeometry(
                *source,
                {0.0, 0.0},
                {
                    std::cos(
                        std::numbers::pi_v<double> /
                        2.0),
                    std::sin(
                        std::numbers::pi_v<double> /
                        2.0)});
        CHECK(mirrored.has_value());
        CHECK(expected_mirrored.has_value());
        CHECK(*mirrored == *expected_mirrored);
    }

    // Polar Relative uses only semantic references explicitly owned by
    // the active request. Continuous Line exposes the previous committed
    // segment direction; Rotate exposes its accepted reference vector.
    {
        sketch::SketchInteractionState polar_line;
        polar_line.activateLine();
        CHECK(
            polar_line.acceptLinePoint(
                {0.0, 0.0}).outcome ==
            sketch::LinePointOutcome::
                first_point_accepted);
        const auto segment =
            polar_line.acceptLinePoint(
                {3.0, 4.0});
        CHECK(
            segment.outcome ==
            sketch::LinePointOutcome::
                segment_requested);
        CHECK(polar_line.resolveLineRequest(true));
        const auto request =
            polar_line.activePointRequest();
        CHECK(request.has_value());
        CHECK(
            request->polar_relative_reference
                .has_value());
        CHECK(near(
            *request->polar_relative_reference,
            std::atan2(4.0, 3.0)));
    }

    {
        sketch::SketchInteractionState polar_rotate;
        CHECK(polar_rotate.addSelection(line_id));
        CHECK(polar_rotate.activateRotate(model));
        CHECK(
            polar_rotate.acceptTransformPoint(
                sketch::ResolvedSketchInput{
                    {0.0, 0.0}}));
        CHECK(
            polar_rotate.acceptTransformPoint(
                sketch::ResolvedSketchInput{
                    {0.0, 2.0}}));
        const auto request =
            polar_rotate.activePointRequest();
        CHECK(request.has_value());
        CHECK(
            request->polar_relative_reference
                .has_value());
        CHECK(near(
            *request->polar_relative_reference,
            std::numbers::pi_v<double> / 2.0));
    }

    {
        sketch::SketchInteractionState polar_grip;
        CHECK(polar_grip.addSelection(line_id));
        CHECK(
            polar_grip.beginDirectManipulation(
                model,
                {line_id,
                 sketch::SketchGripRole::line_start}));
        CHECK(
            polar_grip.updateDirectManipulation(
                sketch::ResolvedSketchInput{
                    {1.0, 0.0}}));
        CHECK(polar_grip.cycleDirectEditMode());
        CHECK(polar_grip.cycleDirectEditMode());
        CHECK(
            polar_grip.directEditMode() ==
            sketch::DirectEditMode::rotate);
        const auto request =
            polar_grip.activePointRequest();
        CHECK(request.has_value());
        CHECK(
            request->polar_relative_reference
                .has_value());
        CHECK(near(
            *request->polar_relative_reference,
            0.0));
    }

    // Request-local point locks constrain the existing pointer resolver.
    // Polar (Distance/Angle) and Cartesian (dU/dV) lock families may not
    // be mixed; complete point submission remains a separate higher-priority path.
    {
        sketch::SketchInteractionState dyn_line;
        dyn_line.activateLine();

        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::u,
            10.0));
        auto resolved =
            dyn_line.resolvePointerInput(
                {1.0, 2.0});
        CHECK(resolved.has_value());
        CHECK(near(resolved->position.u, 10.0));
        CHECK(near(resolved->position.v, 2.0));

        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::v,
            -5.0));
        resolved =
            dyn_line.resolvePointerInput(
                {100.0, 200.0});
        CHECK(resolved.has_value());
        CHECK(near(resolved->position.u, 10.0));
        CHECK(near(resolved->position.v, -5.0));

        CHECK(
            dyn_line.acceptLinePoint(
                resolved->position).outcome ==
            sketch::LinePointOutcome::
                first_point_accepted);
        CHECK(dyn_line.pointFieldLocks().empty());

        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::distance,
            100.0));
        resolved =
            dyn_line.resolvePointerInput(
                {13.0, -1.0});
        CHECK(resolved.has_value());
        CHECK(near(resolved->position.u, 110.0));
        CHECK(near(resolved->position.v, -5.0));

        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::angle,
            std::numbers::pi_v<double> / 2.0));
        resolved =
            dyn_line.resolvePointerInput(
                {500.0, 500.0});
        CHECK(resolved.has_value());
        CHECK(near(resolved->position.u, 10.0));
        CHECK(near(resolved->position.v, 95.0));
        CHECK(!dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::delta_u,
            1.0));

        dyn_line.clearPointFieldLocks();
        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::delta_u,
            30.0));
        resolved =
            dyn_line.resolvePointerInput(
                {15.0, 35.0});
        CHECK(resolved.has_value());
        CHECK(near(resolved->position.u, 40.0));
        CHECK(near(resolved->position.v, 35.0));

        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::delta_v,
            -10.0));
        resolved =
            dyn_line.resolvePointerInput(
                {999.0, 999.0});
        CHECK(resolved.has_value());
        CHECK(near(resolved->position.u, 40.0));
        CHECK(near(resolved->position.v, -15.0));
        CHECK(!dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::distance,
            25.0));

        dyn_line.clearPointFieldLocks();
        CHECK(dyn_line.lockPointField(
            sketch::PointFieldLockSemantic::angle,
            0.0));
        resolved =
            dyn_line.resolvePointerInput(
                {10.0, -5.0});
        CHECK(resolved.has_value());
        CHECK(resolved->position ==
              sketch::Point2{10.0, -5.0});
    }

    std::cout << "SK-07F precision input state PASS\n";
    return EXIT_SUCCESS;
}
