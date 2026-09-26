#include <simplesolid2/sketch/interaction_state.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-06A interaction CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

bool near(double left, double right, double tolerance = 1.0e-10) {
    return std::abs(left - right) <= tolerance;
}

template <class State>
const State* findState(
    const std::vector<State>& values,
    sketch::EntityId id) {
    for (const auto& value : values) {
        if (value.id == id) {
            return &value;
        }
    }
    return nullptr;
}

} // namespace

int main() {
    constexpr double pi =
        std::numbers::pi_v<double>;
    const double quadrant =
        std::sqrt(0.5);

    sketch::SketchModel model;
    const auto line_id =
        model.addLine(
            {-2.0, 1.0},
            {2.0, 1.0});
    const auto circle_id =
        model.addCircle(
            {5.0, 5.0},
            3.0);
    const auto arc_id =
        model.addArc(
            {0.0, 0.0},
            10.0,
            0.0,
            pi * 0.5);

    // Creation tools preserve the existing semantic selection.
    sketch::SketchInteractionState creation;
    CHECK(creation.addSelection(line_id));

    creation.activateCircle();
    CHECK(
        creation.tool() ==
        sketch::SketchTool::circle);
    CHECK(creation.selectedEntities().size() == 1U);
    CHECK(
        creation.circleStage() ==
        sketch::CircleStage::await_center);

    const auto center =
        creation.acceptCirclePoint({0.0, 0.0});
    CHECK(
        center.outcome ==
        sketch::CirclePointOutcome::center_accepted);
    CHECK(
        creation.circleStage() ==
        sketch::CircleStage::await_radius);

    const auto circle_preview =
        creation.previewCircle({3.0, 4.0});
    CHECK(circle_preview.has_value());
    CHECK(circle_preview->center == sketch::Point2{0.0, 0.0});
    CHECK(near(circle_preview->radius, 5.0));

    const auto zero_circle =
        creation.acceptCirclePoint({0.0, 0.0});
    CHECK(
        zero_circle.outcome ==
        sketch::CirclePointOutcome::zero_radius_ignored);

    const auto circle_request =
        creation.acceptCirclePoint({3.0, 4.0});
    CHECK(
        circle_request.outcome ==
        sketch::CirclePointOutcome::circle_requested);
    CHECK(circle_request.request.has_value());
    CHECK(near(circle_request.request->radius, 5.0));
    CHECK(creation.resolveCircleRequest(true));
    CHECK(
        creation.circleStage() ==
        sketch::CircleStage::await_center);
    CHECK(
        creation.tool() ==
        sketch::SketchTool::circle);
    CHECK(creation.selectedEntities().front() == line_id);

    // First Esc exits the ready-for-next Circle tool, selection remains.
    CHECK(creation.escape());
    CHECK(
        creation.tool() ==
        sketch::SketchTool::select);
    CHECK(creation.selectedEntities().size() == 1U);

    // 3-point Arc: positive short branch.
    creation.activateArc();
    CHECK(
        creation.acceptArcPoint({1.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::start_accepted);
    CHECK(
        creation.acceptArcPoint(
            {quadrant, quadrant}).outcome ==
        sketch::ArcPointOutcome::through_accepted);

    const auto arc_preview =
        creation.previewArc({0.0, 1.0});
    CHECK(arc_preview.has_value());
    CHECK(near(arc_preview->center.u, 0.0));
    CHECK(near(arc_preview->center.v, 0.0));
    CHECK(near(arc_preview->radius, 1.0));
    CHECK(near(arc_preview->sweep_angle, pi * 0.5));

    const auto short_ccw =
        creation.acceptArcPoint({0.0, 1.0});
    CHECK(
        short_ccw.outcome ==
        sketch::ArcPointOutcome::arc_requested);
    CHECK(short_ccw.request.has_value());
    CHECK(short_ccw.request->sweep_angle > 0.0);
    CHECK(
        std::abs(short_ccw.request->sweep_angle) <
        pi);
    CHECK(creation.resolveArcRequest(true));
    CHECK(
        creation.arcStage() ==
        sketch::ArcStage::await_start);
    CHECK(
        creation.tool() ==
        sketch::SketchTool::arc);

    // Positive long branch is selected by the Through point.
    creation.activateArc();
    CHECK(
        creation.acceptArcPoint({1.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::start_accepted);
    CHECK(
        creation.acceptArcPoint({-1.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::through_accepted);
    const auto long_ccw =
        creation.acceptArcPoint({0.0, -1.0});
    CHECK(long_ccw.request.has_value());
    CHECK(long_ccw.request->sweep_angle > pi);
    CHECK(
        long_ccw.request->sweep_angle <
        2.0 * pi);
    CHECK(creation.resolveArcRequest(true));

    // Negative short branch.
    creation.activateArc();
    CHECK(
        creation.acceptArcPoint({1.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::start_accepted);
    CHECK(
        creation.acceptArcPoint(
            {quadrant, -quadrant}).outcome ==
        sketch::ArcPointOutcome::through_accepted);
    const auto short_cw =
        creation.acceptArcPoint({0.0, -1.0});
    CHECK(short_cw.request.has_value());
    CHECK(short_cw.request->sweep_angle < 0.0);
    CHECK(
        std::abs(short_cw.request->sweep_angle) <
        pi);
    CHECK(creation.resolveArcRequest(true));

    // Duplicate/collinear construction points fail closed.
    creation.activateArc();
    CHECK(
        creation.acceptArcPoint({0.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::start_accepted);
    CHECK(
        creation.acceptArcPoint({0.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::degenerate_ignored);
    CHECK(
        creation.arcStage() ==
        sketch::ArcStage::await_through);
    CHECK(
        creation.acceptArcPoint({1.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::through_accepted);
    CHECK(
        creation.acceptArcPoint({2.0, 0.0}).outcome ==
        sketch::ArcPointOutcome::degenerate_ignored);
    CHECK(
        creation.arcStage() ==
        sketch::ArcStage::await_end);
    CHECK(!creation.previewArc({2.0, 0.0}).has_value());

    // Direct manipulation works on a frozen mixed selection.
    sketch::SketchInteractionState edit;
    CHECK(edit.addSelection(line_id));
    CHECK(edit.addSelection(circle_id));
    CHECK(edit.addSelection(arc_id));
    CHECK(edit.selectedEntities().size() == 3U);

    CHECK(
        edit.beginDirectManipulation(
            model,
            {circle_id,
             sketch::SketchGripRole::circle_center}));
    CHECK(
        edit.directEditMode() ==
        sketch::DirectEditMode::move);
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {8.0, 3.0}}));
    CHECK(!edit.toggleSelection(line_id));

    auto moved =
        edit.directManipulationGeometryState();
    CHECK(moved.has_value());
    CHECK(moved->lines.size() == 1U);
    CHECK(moved->circles.size() == 1U);
    CHECK(moved->arcs.size() == 1U);

    const auto* moved_line =
        findState(moved->lines, line_id);
    const auto* moved_circle =
        findState(moved->circles, circle_id);
    const auto* moved_arc =
        findState(moved->arcs, arc_id);
    CHECK(moved_line != nullptr);
    CHECK(moved_circle != nullptr);
    CHECK(moved_arc != nullptr);
    CHECK(
        moved_line->start ==
        sketch::Point2{1.0, -1.0});
    CHECK(
        moved_line->end ==
        sketch::Point2{5.0, -1.0});
    CHECK(
        moved_circle->center ==
        sketch::Point2{8.0, 3.0});
    CHECK(
        moved_arc->center ==
        sketch::Point2{3.0, -2.0});
    CHECK(near(moved_circle->radius, 3.0));
    CHECK(near(moved_arc->radius, 10.0));
    CHECK(near(moved_arc->start_angle, 0.0));
    CHECK(near(moved_arc->sweep_angle, pi * 0.5));

    edit.cancelDirectManipulation();
    CHECK(!edit.directManipulationActive());
    CHECK(edit.selectedEntities().size() == 3U);

    // Circle radius reshape is owner-only even with mixed selection.
    CHECK(
        edit.beginDirectManipulation(
            model,
            {circle_id,
             sketch::SketchGripRole::
                 circle_quadrant_pos_u}));
    CHECK(
        edit.directEditMode() ==
        sketch::DirectEditMode::reshape);
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {5.0, 10.0}}));
    const auto circle_reshape =
        edit.directManipulationGeometryState();
    CHECK(circle_reshape.has_value());
    CHECK(circle_reshape->lines.empty());
    CHECK(circle_reshape->arcs.empty());
    CHECK(circle_reshape->circles.size() == 1U);
    CHECK(
        circle_reshape->circles.front().id ==
        circle_id);
    CHECK(
        circle_reshape->circles.front().center ==
        sketch::Point2{5.0, 5.0});
    CHECK(near(
        circle_reshape->circles.front().radius,
        5.0));
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {5.0, 5.0}}));
    CHECK(
        !edit.directManipulationGeometryState()
             .has_value());
    edit.cancelDirectManipulation();

    // Arc Center is the same common Move over the frozen selection.
    CHECK(
        edit.beginDirectManipulation(
            model,
            {arc_id,
             sketch::SketchGripRole::arc_center}));
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {2.0, 1.0}}));
    const auto arc_center_move =
        edit.directManipulationGeometryState();
    CHECK(arc_center_move.has_value());
    CHECK(
        findState(
            arc_center_move->lines,
            line_id)->start ==
        sketch::Point2{0.0, 2.0});
    CHECK(
        findState(
            arc_center_move->circles,
            circle_id)->center ==
        sketch::Point2{7.0, 6.0});
    CHECK(
        findState(
            arc_center_move->arcs,
            arc_id)->center ==
        sketch::Point2{2.0, 1.0});
    edit.cancelDirectManipulation();

    // Arc/Mid changes radius only.
    CHECK(
        edit.beginDirectManipulation(
            model,
            {arc_id,
             sketch::SketchGripRole::arc_mid}));
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {0.0, 20.0}}));
    const auto arc_mid =
        edit.directManipulationGeometryState();
    CHECK(arc_mid.has_value());
    CHECK(arc_mid->arcs.size() == 1U);
    CHECK(near(arc_mid->arcs.front().radius, 20.0));
    CHECK(near(arc_mid->arcs.front().start_angle, 0.0));
    CHECK(near(
        arc_mid->arcs.front().sweep_angle,
        pi * 0.5));
    edit.cancelDirectManipulation();

    // Arc Start preserves center/radius/fixed End direction and sign.
    CHECK(
        edit.beginDirectManipulation(
            model,
            {arc_id,
             sketch::SketchGripRole::arc_start}));
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {10.0 * quadrant,
                 -10.0 * quadrant}}));
    const auto arc_start =
        edit.directManipulationGeometryState();
    CHECK(arc_start.has_value());
    CHECK(arc_start->arcs.size() == 1U);
    const auto& start_edit =
        arc_start->arcs.front();
    CHECK(start_edit.center == sketch::Point2{0.0, 0.0});
    CHECK(near(start_edit.radius, 10.0));
    CHECK(near(start_edit.start_angle, -pi * 0.25));
    CHECK(near(start_edit.sweep_angle, pi * 0.75));
    CHECK(near(
        start_edit.start_angle +
            start_edit.sweep_angle,
        pi * 0.5));
    edit.cancelDirectManipulation();

    // Arc End preserves center/radius/Start direction and CW/CCW sign.
    CHECK(
        edit.beginDirectManipulation(
            model,
            {arc_id,
             sketch::SketchGripRole::arc_end}));
    CHECK(
        edit.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {-10.0 * quadrant,
                 10.0 * quadrant}}));
    const auto arc_end =
        edit.directManipulationGeometryState();
    CHECK(arc_end.has_value());
    CHECK(arc_end->arcs.size() == 1U);
    const auto& end_edit =
        arc_end->arcs.front();
    CHECK(end_edit.center == sketch::Point2{0.0, 0.0});
    CHECK(near(end_edit.radius, 10.0));
    CHECK(near(end_edit.start_angle, 0.0));
    CHECK(near(end_edit.sweep_angle, pi * 0.75));
    edit.cancelDirectManipulation();

    // History cancellation drops transient manipulation but preserves selection.
    CHECK(
        edit.beginDirectManipulation(
            model,
            {line_id,
             sketch::SketchGripRole::line_center}));
    edit.cancelForHistory();
    CHECK(!edit.directManipulationActive());
    CHECK(
        edit.tool() ==
        sketch::SketchTool::select);
    CHECK(edit.selectedEntities().size() == 3U);

    return EXIT_SUCCESS;
}
