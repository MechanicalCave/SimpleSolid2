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

    std::cout << "SK-07F precision input state PASS\n";
    return EXIT_SUCCESS;
}
