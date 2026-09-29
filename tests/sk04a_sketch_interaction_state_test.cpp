#include <simplesolid2/sketch/interaction_state.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04A interaction CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    sketch::SketchInteractionState state;

    CHECK(
        state.tool() ==
        sketch::SketchTool::select);
    CHECK(!state.lineStage().has_value());
    CHECK(!state.lineAnchor().has_value());
    CHECK(!state.lineRequestPending());
    CHECK(!state.previewLine(
        sketch::Point2{1.0, 2.0})
        .has_value());

    const auto inactive =
        state.acceptLinePoint(
            sketch::Point2{1.0, 2.0});
    CHECK(
        inactive.outcome ==
        sketch::LinePointOutcome::inactive_tool);
    CHECK(!inactive.request.has_value());

    state.activateLine();
    CHECK(
        state.tool() ==
        sketch::SketchTool::line);
    CHECK(
        state.lineStage() ==
        sketch::LineStage::await_first_point);

    const auto invalid =
        state.acceptLinePoint(
            sketch::Point2{
                std::numeric_limits<double>::
                    infinity(),
                0.0});
    CHECK(
        invalid.outcome ==
        sketch::LinePointOutcome::invalid_point);
    CHECK(
        state.lineStage() ==
        sketch::LineStage::await_first_point);

    const sketch::Point2 a{1.0, 2.0};
    const sketch::Point2 b{4.0, 6.0};
    const sketch::Point2 c{8.0, 9.0};
    const sketch::Point2 d{12.0, 5.0};

    const auto first =
        state.acceptLinePoint(a);
    CHECK(
        first.outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);
    CHECK(!first.request.has_value());
    CHECK(
        state.lineStage() ==
        sketch::LineStage::await_next_point);
    CHECK(state.lineAnchor() == a);

    const auto preview =
        state.previewLine(b);
    CHECK(preview.has_value());
    CHECK((
        *preview ==
        sketch::LineSegmentIntent{a, b}));
    CHECK(!state.previewLine(a).has_value());

    const auto zero =
        state.acceptLinePoint(a);
    CHECK(
        zero.outcome ==
        sketch::LinePointOutcome::
            zero_length_ignored);
    CHECK(!zero.request.has_value());
    CHECK(state.lineAnchor() == a);

    const auto request_ab =
        state.acceptLinePoint(b);
    CHECK(
        request_ab.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    CHECK(request_ab.request.has_value());
    CHECK((
        *request_ab.request ==
        sketch::LineSegmentIntent{a, b}));
    CHECK(state.lineRequestPending());
    CHECK(!state.previewLine(c).has_value());

    const auto blocked =
        state.acceptLinePoint(c);
    CHECK(
        blocked.outcome ==
        sketch::LinePointOutcome::
            request_pending);
    CHECK(!blocked.request.has_value());

    CHECK(state.resolveLineRequest(false));
    CHECK(!state.lineRequestPending());
    CHECK(state.lineAnchor() == a);

    const auto retry_ab =
        state.acceptLinePoint(b);
    CHECK(
        retry_ab.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    CHECK(state.resolveLineRequest(true));
    CHECK(state.lineAnchor() == b);

    const auto request_bc =
        state.acceptLinePoint(c);
    CHECK(
        request_bc.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    CHECK(state.resolveLineRequest(true));
    CHECK(state.lineAnchor() == c);

    const auto request_cd =
        state.acceptLinePoint(d);
    CHECK(
        request_cd.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    CHECK(state.resolveLineRequest(true));
    CHECK(state.lineAnchor() == d);

    CHECK(!state.resolveLineRequest(true));

    state.finishTool();
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);
    CHECK(!state.lineStage().has_value());
    CHECK(!state.lineAnchor().has_value());

    // Finish from AwaitFirstPoint is also a clean return to Select.
    state.activateLine();
    CHECK(
        state.lineStage() ==
        sketch::LineStage::await_first_point);
    state.finishTool();
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);
    CHECK(!state.lineRequestPending());

    // Finish from AwaitNextPoint clears uncommitted pending request state.
    state.activateLine();
    CHECK(
        state.acceptLinePoint(a).outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);
    CHECK(
        state.acceptLinePoint(b).outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    CHECK(state.lineRequestPending());
    state.finishTool();
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);
    CHECK(!state.lineRequestPending());
    CHECK(!state.lineAnchor().has_value());

    // Esc from AwaitNextPoint, including a pending segment request,
    // cancels only the pending stage and keeps Line active at first point.
    state.activateLine();
    CHECK(
        state.acceptLinePoint(a).outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);
    CHECK(
        state.acceptLinePoint(b).outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    CHECK(state.escape());
    CHECK(
        state.tool() ==
        sketch::SketchTool::line);
    CHECK(
        state.lineStage() ==
        sketch::LineStage::await_first_point);
    CHECK(!state.lineAnchor().has_value());
    CHECK(!state.lineRequestPending());

    CHECK(state.escape());
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);
    CHECK(!state.escape());

    state.activateLine();
    CHECK(
        state.acceptLinePoint(a).outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);
    CHECK(
        state.acceptLinePoint(b).outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    state.cancelTool();
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);
    CHECK(!state.lineRequestPending());

    state.activateLine();
    CHECK(
        state.acceptLinePoint(a).outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);
    state.cancelForHistory();
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);

    // R9 Rectangle is one semantic two-corner tool with exact U/V
    // decomposition. One scalar Direct Distance is deliberately disabled.
    state.activateRectangle();
    CHECK(
        state.tool() ==
        sketch::SketchTool::rectangle);
    CHECK(
        state.rectangleStage() ==
        sketch::RectangleStage::await_first_corner);

    auto rectangle_point_request =
        state.activePointRequest();
    CHECK(rectangle_point_request.has_value());
    CHECK(!rectangle_point_request->base.has_value());
    CHECK(!rectangle_point_request->direct_distance_enabled);

    CHECK(
        state.acceptRectanglePoint(a).outcome ==
        sketch::RectanglePointOutcome::
            first_corner_accepted);
    CHECK(
        state.rectangleStage() ==
        sketch::RectangleStage::await_opposite_corner);

    rectangle_point_request =
        state.activePointRequest();
    CHECK(rectangle_point_request.has_value());
    CHECK(rectangle_point_request->base == a);
    CHECK(!rectangle_point_request->direct_distance_enabled);

    const sketch::Point2 rectangle_opposite{5.0, 7.0};
    const auto rectangle_preview =
        state.previewRectangle(rectangle_opposite);
    CHECK(rectangle_preview.has_value());
    CHECK(rectangle_preview->valid());

    const auto rectangle_perimeter =
        rectangle_preview->perimeter();
    CHECK((
        rectangle_perimeter[0] ==
        sketch::LineSegmentIntent{
            a,
            {rectangle_opposite.u, a.v}}));
    CHECK((
        rectangle_perimeter[1] ==
        sketch::LineSegmentIntent{
            {rectangle_opposite.u, a.v},
            rectangle_opposite}));
    CHECK((
        rectangle_perimeter[2] ==
        sketch::LineSegmentIntent{
            rectangle_opposite,
            {a.u, rectangle_opposite.v}}));
    CHECK((
        rectangle_perimeter[3] ==
        sketch::LineSegmentIntent{
            {a.u, rectangle_opposite.v},
            a}));

    const auto rectangle_diagonals =
        rectangle_preview->diagonals();
    CHECK((
        rectangle_diagonals[0] ==
        sketch::LineSegmentIntent{
            a,
            rectangle_opposite}));
    CHECK((
        rectangle_diagonals[1] ==
        sketch::LineSegmentIntent{
            {rectangle_opposite.u, a.v},
            {a.u, rectangle_opposite.v}}));

    const sketch::Point2 rectangle_quadrants[] = {
        {5.0, 7.0},
        {-3.0, 7.0},
        {-3.0, -4.0},
        {5.0, -4.0},
    };
    for (const auto opposite :
         rectangle_quadrants) {
        const auto quadrant_preview =
            state.previewRectangle(opposite);
        CHECK(quadrant_preview.has_value());
        CHECK(quadrant_preview->valid());
        const auto quadrant_perimeter =
            quadrant_preview->perimeter();
        CHECK(quadrant_perimeter[0].start == a);
        CHECK(quadrant_perimeter[1].end == opposite);
        CHECK(quadrant_perimeter[3].end == a);
    }

    CHECK(
        state.acceptRectanglePoint(
                 {a.u, rectangle_opposite.v})
                .outcome ==
        sketch::RectanglePointOutcome::
            degenerate_ignored);
    CHECK(
        state.rectangleStage() ==
        sketch::RectangleStage::await_opposite_corner);

    CHECK(
        state.acceptRectanglePoint(
                 rectangle_opposite)
                .outcome ==
        sketch::RectanglePointOutcome::
            rectangle_requested);
    CHECK(
        !state.previewRectangle({8.0, 8.0})
             .has_value());

    CHECK(state.resolveRectangleRequest(false));
    CHECK(
        state.rectangleStage() ==
        sketch::RectangleStage::await_opposite_corner);

    CHECK(
        state.acceptRectanglePoint(
                 rectangle_opposite)
                .outcome ==
        sketch::RectanglePointOutcome::
            rectangle_requested);
    CHECK(state.resolveRectangleRequest(true));
    CHECK(
        state.rectangleStage() ==
        sketch::RectangleStage::await_first_corner);
    CHECK(
        state.tool() ==
        sketch::SketchTool::rectangle);

    CHECK(
        state.acceptRectanglePoint(a).outcome ==
        sketch::RectanglePointOutcome::
            first_corner_accepted);
    CHECK(state.escape());
    CHECK(
        state.tool() ==
        sketch::SketchTool::rectangle);
    CHECK(
        state.rectangleStage() ==
        sketch::RectangleStage::await_first_corner);
    CHECK(state.escape());
    CHECK(
        state.tool() ==
        sketch::SketchTool::select);

    sketch::SketchModel model;
    const auto id1 =
        model.addLine(
            sketch::Point2{0.0, 0.0},
            sketch::Point2{1.0, 0.0});
    const auto id2 =
        model.addLine(
            sketch::Point2{1.0, 0.0},
            sketch::Point2{2.0, 0.0});
    const auto id3 =
        model.addLine(
            sketch::Point2{2.0, 0.0},
            sketch::Point2{3.0, 0.0});

    CHECK(state.replaceSelection(id1));
    CHECK(state.selectedEntities().size() == 1U);
    CHECK(state.selectedEntities()[0] == id1);
    CHECK(state.primarySelection() == id1);

    CHECK(state.toggleSelection(id2));
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(state.primarySelection() == id2);

    CHECK(state.toggleSelection(id2));
    CHECK(state.selectedEntities().size() == 1U);
    CHECK(state.selectedEntities()[0] == id1);
    CHECK(state.primarySelection() == id1);

    CHECK(state.toggleSelection(id2));
    CHECK(state.toggleSelection(id3));
    CHECK(state.selectedEntities().size() == 3U);
    CHECK(state.primarySelection() == id3);

    const auto before_invalid =
        state.selectedEntities();
    const auto before_primary =
        state.primarySelection();

    CHECK(!state.replaceSelection(
        std::vector<sketch::EntityId>{
            id1,
            id1},
        id1));
    CHECK(
        state.selectedEntities() ==
        before_invalid);
    CHECK(
        state.primarySelection() ==
        before_primary);

    CHECK(!state.replaceSelection(
        std::vector<sketch::EntityId>{
            id1,
            id2},
        id3));
    CHECK(
        state.selectedEntities() ==
        before_invalid);

    CHECK(state.replaceSelection(
        std::vector<sketch::EntityId>{
            id1,
            id2,
            id3},
        id2));
    CHECK(state.primarySelection() == id2);

    state.clearSelection();
    CHECK(state.selectedEntities().empty());
    CHECK(!state.primarySelection().has_value());

    CHECK(state.replaceSelection(
        std::vector<sketch::EntityId>{
            id1,
            id2,
            id3},
        id2));
    CHECK(state.primarySelection() == id2);

    CHECK(model.erase(id2));
    state.reconcileSelection(model);
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(
        state.selectedEntities()[0] == id1);
    CHECK(
        state.selectedEntities()[1] == id3);
    CHECK(state.primarySelection() == id1);

    CHECK(model.erase(id1));
    CHECK(model.erase(id3));
    state.reconcileSelection(model);
    CHECK(state.selectedEntities().empty());
    CHECK(!state.primarySelection().has_value());

    state.clearSelection();
    CHECK(state.selectedEntities().empty());
    CHECK(!state.primarySelection().has_value());

    return EXIT_SUCCESS;
}
