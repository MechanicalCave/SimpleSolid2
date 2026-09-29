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
            << "SK-07E Space CycleEditMode CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

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

sketch::SketchInteractionState selectedState(
    sketch::EntityId line_id,
    sketch::EntityId circle_id,
    sketch::EntityId arc_id) {
    sketch::SketchInteractionState state;
    CHECK(state.addSelection(line_id));
    CHECK(state.addSelection(circle_id));
    CHECK(state.addSelection(arc_id));
    return state;
}

} // namespace

int main() {
    constexpr double pi =
        std::numbers::pi_v<double>;

    sketch::SketchModel model;
    const auto line_id =
        model.addLine(
            {0.0, 0.0},
            {10.0, 0.0});
    const auto circle_id =
        model.addCircle(
            {20.0, 0.0},
            5.0);
    const auto arc_id =
        model.addArc(
            {0.0, 20.0},
            10.0,
            0.0,
            pi * 0.5);
    CHECK(
        model.setEntityRole(
            line_id,
            sketch::EntityRole::construction));

    // Detailed endpoint cycle: one current pointer position must be
    // interpreted from the same interaction-start geometry in both modes.
    auto state =
        selectedState(
            line_id,
            circle_id,
            arc_id);
    const auto frozen_selection =
        state.selectedEntities();
    const auto frozen_primary =
        state.primarySelection();

    const sketch::SketchGripRef line_start{
        line_id,
        sketch::SketchGripRole::line_start};
    CHECK(
        state.beginDirectManipulation(
            model,
            line_start));
    CHECK(state.activeGrip() == line_start);
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::reshape);
    CHECK(!state.directManipulationCopyEnabled());
    CHECK(state.enableDirectManipulationCopy());
    CHECK(state.directManipulationCopyEnabled());
    CHECK(state.enableDirectManipulationCopy());
    CHECK(state.directManipulationCopyEnabled());

    CHECK(
        state.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {2.0, 3.0}}));

    const auto reshape =
        state.directManipulationGeometryState();
    CHECK(reshape.has_value());
    CHECK(reshape->lines.size() == 1U);
    CHECK(
        reshape->lines.front().role ==
        sketch::EntityRole::construction);
    CHECK(reshape->circles.empty());
    CHECK(reshape->arcs.empty());
    CHECK(
        reshape->lines.front().start ==
        sketch::Point2{2.0, 3.0});
    CHECK(
        reshape->lines.front().end ==
        sketch::Point2{10.0, 0.0});

    CHECK(state.cycleDirectEditMode());
    CHECK(state.activeGrip() == line_start);
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::move);
    CHECK(!state.directManipulationCopyEnabled());
    CHECK(
        state.selectedEntities() ==
        frozen_selection);
    CHECK(
        state.primarySelection() ==
        frozen_primary);

    const auto moved =
        state.directManipulationGeometryState();
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
        sketch::Point2{2.0, 3.0});
    CHECK(
        moved_line->end ==
        sketch::Point2{12.0, 3.0});
    CHECK(
        moved_circle->center ==
        sketch::Point2{22.0, 3.0});
    CHECK(
        moved_arc->center ==
        sketch::Point2{2.0, 23.0});

    CHECK(state.cycleDirectEditMode());
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::reshape);
    const auto reshape_again =
        state.directManipulationGeometryState();
    CHECK(reshape_again == reshape);

    CHECK(state.cycleDirectEditMode());
    const auto moved_again =
        state.directManipulationGeometryState();
    CHECK(moved_again == moved);

    // A pointer position invalid for Reshape can still be valid for Move.
    CHECK(state.cycleDirectEditMode());
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::reshape);
    CHECK(
        state.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                {10.0, 0.0}}));
    CHECK(
        !state.directManipulationGeometryState()
             .has_value());
    CHECK(state.cycleDirectEditMode());
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::move);
    CHECK(
        state.directManipulationGeometryState()
            .has_value());
    CHECK(state.cycleDirectEditMode());
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::reshape);
    CHECK(
        !state.directManipulationGeometryState()
             .has_value());
    state.cancelDirectManipulation();

    // Every non-center grip keeps its existing Reshape default and cycles.
    const std::vector<sketch::SketchGripRef> two_mode_grips{
        {line_id, sketch::SketchGripRole::line_start},
        {line_id, sketch::SketchGripRole::line_end},
        {circle_id, sketch::SketchGripRole::circle_quadrant_pos_u},
        {circle_id, sketch::SketchGripRole::circle_quadrant_pos_v},
        {circle_id, sketch::SketchGripRole::circle_quadrant_neg_u},
        {circle_id, sketch::SketchGripRole::circle_quadrant_neg_v},
        {arc_id, sketch::SketchGripRole::arc_start},
        {arc_id, sketch::SketchGripRole::arc_end},
        {arc_id, sketch::SketchGripRole::arc_mid},
    };

    for (const auto grip : two_mode_grips) {
        auto cycling =
            selectedState(
                line_id,
                circle_id,
                arc_id);
        const auto selected =
            cycling.selectedEntities();
        const auto primary =
            cycling.primarySelection();

        CHECK(
            cycling.beginDirectManipulation(
                model,
                grip));
        CHECK(cycling.activeGrip() == grip);
        CHECK(
            cycling.directEditMode() ==
            sketch::DirectEditMode::reshape);

        CHECK(cycling.cycleDirectEditMode());
        CHECK(cycling.activeGrip() == grip);
        CHECK(
            cycling.directEditMode() ==
            sketch::DirectEditMode::move);
        CHECK(
            cycling.selectedEntities() ==
            selected);
        CHECK(
            cycling.primarySelection() ==
            primary);

        CHECK(cycling.cycleDirectEditMode());
        CHECK(cycling.activeGrip() == grip);
        CHECK(
            cycling.directEditMode() ==
            sketch::DirectEditMode::reshape);
        CHECK(
            cycling.selectedEntities() ==
            selected);
        CHECK(
            cycling.primarySelection() ==
            primary);
        cycling.cancelDirectManipulation();
    }

    // Center grips are Move-only; Space/Cycle is a semantic no-op.
    const std::vector<sketch::SketchGripRef> move_only_grips{
        {line_id, sketch::SketchGripRole::line_center},
        {circle_id, sketch::SketchGripRole::circle_center},
        {arc_id, sketch::SketchGripRole::arc_center},
    };

    for (const auto grip : move_only_grips) {
        auto move_only =
            selectedState(
                line_id,
                circle_id,
                arc_id);
        const auto selected =
            move_only.selectedEntities();
        const auto primary =
            move_only.primarySelection();

        CHECK(
            move_only.beginDirectManipulation(
                model,
                grip));
        CHECK(
            move_only.directEditMode() ==
            sketch::DirectEditMode::move);
        CHECK(!move_only.directManipulationCopyEnabled());
        CHECK(move_only.enableDirectManipulationCopy());
        CHECK(move_only.directManipulationCopyEnabled());

        CHECK(!move_only.cycleDirectEditMode());
        CHECK(move_only.directManipulationCopyEnabled());
        CHECK(move_only.activeGrip() == grip);
        CHECK(
            move_only.directEditMode() ==
            sketch::DirectEditMode::move);
        CHECK(
            move_only.selectedEntities() ==
            selected);
        CHECK(
            move_only.primarySelection() ==
            primary);
        CHECK(
            move_only.directManipulationGeometryState()
                .has_value());

        move_only.cancelDirectManipulation();
    }

    CHECK(!state.directManipulationActive());
    CHECK(!state.directManipulationCopyEnabled());
    CHECK(!state.enableDirectManipulationCopy());
    CHECK(!state.cycleDirectEditMode());

    return EXIT_SUCCESS;
}
