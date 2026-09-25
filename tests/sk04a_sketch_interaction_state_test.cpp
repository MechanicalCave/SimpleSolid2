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

    state.activateLine();
    CHECK(
        state.acceptLinePoint(a).outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);
    CHECK(state.escape());
    CHECK(
        state.tool() ==
        sketch::SketchTool::line);
    CHECK(
        state.lineStage() ==
        sketch::LineStage::await_first_point);
    CHECK(!state.lineAnchor().has_value());

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

    CHECK(model.erase(id2));
    state.reconcileSelection(model);
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(
        state.selectedEntities()[0] == id1);
    CHECK(
        state.selectedEntities()[1] == id3);
    CHECK(state.primarySelection() == id3);

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
