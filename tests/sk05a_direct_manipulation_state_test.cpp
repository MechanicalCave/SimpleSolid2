#include <simplesolid2/sketch/interaction_state.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-05A direct manipulation CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    sketch::SketchModel model;
    const auto id1 =
        model.addLine({0.0, 0.0}, {10.0, 0.0});
    const auto id2 =
        model.addLine({0.0, 10.0}, {10.0, 10.0});
    const auto id3 =
        model.addLine({0.0, 20.0}, {10.0, 20.0});

    sketch::SketchInteractionState state;

    CHECK(state.addSelection(id1));
    CHECK(state.addSelection(id2));
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(state.primarySelection() == id2);

    // Ordinary click on an already selected entity preserves the set
    // and only makes that entity primary.
    CHECK(state.addSelection(id1));
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(state.primarySelection() == id1);

    // Rectangle membership is canonicalized by EntityId and does not
    // let provider hit ordering pick a primary entity.
    CHECK(state.addSelection(
        std::vector<sketch::EntityId>{id3}));
    CHECK(state.selectedEntities().size() == 3U);
    CHECK(state.primarySelection() == id1);

    CHECK(state.toggleSelection(
        std::vector<sketch::EntityId>{id1}));
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(state.primarySelection() == id2);

    CHECK(state.setHoveredEntity(id2));
    CHECK(state.hoveredEntity() == id2);
    CHECK(state.setHoveredGrip(
        sketch::LineGripRef{
            id2,
            sketch::LineHandleRole::center}));
    CHECK(!state.hoveredEntity().has_value());
    CHECK(state.hoveredGrip().has_value());

    // Center starts Move for the frozen selected set.
    CHECK(state.beginDirectManipulation(
        model,
        sketch::LineGripRef{
            id2,
            sketch::LineHandleRole::center}));
    CHECK(state.directManipulationActive());
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::move);

    // Selection is frozen for the lifetime of manipulation.
    CHECK(!state.addSelection(id1));
    CHECK(!state.toggleSelection(id3));
    CHECK(state.selectedEntities().size() == 2U);

    const auto resolved =
        sketch::resolveSketchInput(
            sketch::Point2{8.0, 14.0});
    CHECK(resolved.has_value());
    CHECK(state.updateDirectManipulation(*resolved));

    const auto moved =
        state.directManipulationGeometry();
    CHECK(moved.has_value());
    CHECK(moved->size() == 2U);
    CHECK((*moved)[0].id == id2);
    CHECK((*moved)[0].start ==
          sketch::Point2{3.0, 14.0});
    CHECK((*moved)[0].end ==
          sketch::Point2{13.0, 14.0});
    CHECK((*moved)[1].id == id3);
    CHECK((*moved)[1].start ==
          sketch::Point2{3.0, 24.0});

    // Esc cancels only the transient session and preserves selection.
    CHECK(state.escape());
    CHECK(!state.directManipulationActive());
    CHECK(state.selectedEntities().size() == 2U);

    // A second Select-mode Esc clears selection.
    CHECK(state.escape());
    CHECK(state.selectedEntities().empty());
    CHECK(!state.primarySelection().has_value());

    CHECK(state.addSelection(id1));
    CHECK(state.addSelection(id2));
    CHECK(state.beginDirectManipulation(
        model,
        sketch::LineGripRef{
            id2,
            sketch::LineHandleRole::start}));
    CHECK(
        state.directEditMode() ==
        sketch::DirectEditMode::reshape);

    const auto endpoint =
        sketch::resolveSketchInput(
            sketch::Point2{-2.0, 12.0});
    CHECK(endpoint.has_value());
    CHECK(state.updateDirectManipulation(*endpoint));

    const auto reshaped =
        state.directManipulationGeometry();
    CHECK(reshaped.has_value());
    CHECK(reshaped->size() == 1U);
    CHECK(reshaped->front().id == id2);
    CHECK(reshaped->front().start ==
          sketch::Point2{-2.0, 12.0});
    CHECK(reshaped->front().end ==
          sketch::Point2{10.0, 10.0});

    // Invalid zero-length reshape has no valid commit geometry.
    const auto zero =
        sketch::resolveSketchInput(
            sketch::Point2{10.0, 10.0});
    CHECK(zero.has_value());
    CHECK(state.updateDirectManipulation(*zero));
    CHECK(!state.directManipulationGeometry().has_value());

    state.cancelDirectManipulation();
    CHECK(!state.directManipulationActive());

    // Creation uses the same resolution seam and preserves selection.
    state.activateLine();
    CHECK(state.selectedEntities().size() == 2U);
    CHECK(
        sketch::resolveSketchInput(
            sketch::Point2{1.0, 2.0})
            .has_value());
    CHECK(
        !sketch::resolveSketchInput(
            sketch::Point2{
                std::numeric_limits<double>::infinity(),
                0.0})
             .has_value());

    return EXIT_SUCCESS;
}
