#include <simplesolid2/sketch/interaction_state.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

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
    CHECK(!state.resolveDirectDistance(50.0).has_value());

    const auto first =
        state.resolvePointerInput({10.0, 20.0});
    CHECK(first.has_value());
    CHECK(
        state.acceptLinePoint(first->position).outcome ==
        sketch::LinePointOutcome::first_point_accepted);

    request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(request->base == sketch::Point2{10.0, 20.0});
    CHECK(request->direct_distance_enabled);
    CHECK(!state.resolveDirectDistance(50.0).has_value());
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

    const auto base =
        state.resolvePointerInput({5.0, 5.0});
    CHECK(base.has_value());
    CHECK(state.acceptTransformPoint(*base));
    request = state.activePointRequest();
    CHECK(request.has_value());
    CHECK(request->base == sketch::Point2{5.0, 5.0});
    CHECK(request->direct_distance_enabled);
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

    std::cout << "SK-07F precision input state PASS\n";
    return EXIT_SUCCESS;
}
