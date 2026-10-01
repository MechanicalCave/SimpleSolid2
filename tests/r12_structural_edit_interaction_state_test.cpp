#include <simplesolid2/sketch/interaction_state.hpp>

#include <iostream>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << '\n'; \
            return 1; \
        } \
    } while (false)

} // namespace

int main() {
    using namespace simplesolid2::sketch;

    SketchModel model;
    const auto boundary_line =
        model.addLine(
            {0.0, -5.0},
            {0.0, 5.0});
    const auto boundary_circle =
        model.addCircle(
            {5.0, 0.0},
            2.0);
    const auto target_line =
        model.addLine(
            {-5.0, 0.0},
            {-1.0, 0.0});

    SketchInteractionState state;
    CHECK(state.addSelection(boundary_line));
    CHECK(state.addSelection(boundary_circle));

    CHECK(state.activateTrim(model));
    CHECK(state.tool() == SketchTool::trim);
    CHECK(state.structuralBoundaries().size() == 2U);
    CHECK(
        state.structuralBoundaries()[0] ==
        boundary_line);
    CHECK(
        state.structuralBoundaries()[1] ==
        boundary_circle);
    CHECK(state.escape());
    CHECK(state.tool() == SketchTool::select);
    CHECK(state.structuralBoundaries().empty());

    CHECK(state.activateExtend(model));
    CHECK(state.tool() == SketchTool::extend);
    CHECK(state.structuralBoundaries().size() == 2U);
    CHECK(state.escape());

    state.clearSelection();
    CHECK(state.activateTrim(model));
    CHECK(state.tool() == SketchTool::trim);
    CHECK(state.structuralBoundarySelectionPending());
    CHECK(state.structuralBoundaries().empty());
    CHECK(
        state.toggleStructuralBoundarySelection(
            model,
            boundary_line));
    CHECK(
        state.toggleStructuralBoundarySelection(
            model,
            boundary_circle));
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(
        state.completeStructuralBoundarySelection(
            model));
    CHECK(!state.structuralBoundarySelectionPending());
    CHECK(state.structuralBoundaries().size() == 2U);
    CHECK(state.escape());

    state.clearSelection();
    CHECK(state.activateExtend(model));
    CHECK(state.structuralBoundarySelectionPending());
    CHECK(
        !state.completeStructuralBoundarySelection(
            model));
    CHECK(
        state.toggleStructuralBoundarySelection(
            model,
            boundary_line));
    CHECK(
        state.completeStructuralBoundarySelection(
            model));
    CHECK(!state.structuralBoundarySelectionPending());
    CHECK(state.structuralBoundaries().size() == 1U);
    CHECK(state.escape());

    state.activateExtendBoth();
    CHECK(state.tool() == SketchTool::extend_both);
    CHECK(state.selectedEntities().empty());
    CHECK(
        !state.setExtendBothFirstLine(
            model,
            boundary_circle));
    CHECK(
        state.setExtendBothFirstLine(
            model,
            target_line));
    CHECK(
        state.extendBothFirstLine() ==
        target_line);
    state.clearExtendBothFirstLine();
    CHECK(!state.extendBothFirstLine());

    state.activateLine();
    CHECK(state.tool() == SketchTool::line);
    CHECK(state.structuralBoundaries().empty());
    CHECK(!state.extendBothFirstLine());

    return 0;
}
