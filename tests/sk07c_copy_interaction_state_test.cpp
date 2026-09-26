#include <simplesolid2/sketch/interaction_state.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <cstdlib>
#include <iostream>
#include <numbers>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-07C COPY interaction CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

void selectAll(
    sketch::SketchInteractionState& state,
    sketch::EntityId line,
    sketch::EntityId circle,
    sketch::EntityId arc) {
    CHECK(state.addSelection(line));
    CHECK(state.addSelection(circle));
    CHECK(state.addSelection(arc));
}

} // namespace

int main() {
    constexpr double pi =
        std::numbers::pi_v<double>;

    sketch::SketchModel model;
    const auto line =
        model.addLine({1.0, 0.0}, {3.0, 0.0});
    const auto circle =
        model.addCircle({2.0, 1.0}, 2.0);
    const auto arc =
        model.addArc(
            {1.0, 2.0},
            4.0,
            pi * 0.25,
            pi * 0.75);

    const auto source =
        sketch::captureSketchTransformGeometry(
            model,
            {line, circle, arc});
    CHECK(source.has_value());

    // Selection-first COPY freezes the existing semantic selection.
    sketch::SketchInteractionState copy;
    selectAll(copy, line, circle, arc);
    CHECK(copy.activateCopy(model));
    CHECK(copy.tool() == sketch::SketchTool::copy);
    CHECK(
        copy.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(!copy.toggleSelection(line));

    CHECK(
        copy.acceptTransformPoint(
            sketch::ResolvedSketchInput{{0.0, 0.0}}));
    CHECK(
        copy.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);

    // Base-point preview is geometrically unchanged. The controller owns
    // the accepted zero-displacement rule and must not commit this geometry.
    const auto zero =
        copy.transformGeometryState();
    CHECK(zero.has_value());
    CHECK(*zero == *source);

    CHECK(
        copy.updateTransformPreview(
            sketch::ResolvedSketchInput{{5.0, 2.0}}));
    const auto first =
        copy.transformGeometryState();
    CHECK(first.has_value());
    const auto expected_first =
        sketch::translateSketchGeometry(
            *source,
            {5.0, 2.0});
    CHECK(expected_first.has_value());
    CHECK(*first == *expected_first);

    // One accepted placement does not end COPY. It clears only transient
    // placement preview while preserving source snapshot and Base Point.
    CHECK(copy.continueCopyPlacement());
    CHECK(copy.tool() == sketch::SketchTool::copy);
    CHECK(
        copy.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);
    CHECK(!copy.transformGeometryState().has_value());
    CHECK(copy.selectedEntities().size() == 3U);
    CHECK(!copy.addSelection(line));

    // Repeated placement must derive from the original source, not the
    // previously previewed/committed copy.
    CHECK(
        copy.updateTransformPreview(
            sketch::ResolvedSketchInput{{-3.0, 4.0}}));
    const auto second =
        copy.transformGeometryState();
    CHECK(second.has_value());
    const auto expected_second =
        sketch::translateSketchGeometry(
            *source,
            {-3.0, 4.0});
    CHECK(expected_second.has_value());
    CHECK(*second == *expected_second);

    CHECK(copy.escape());
    CHECK(copy.tool() == sketch::SketchTool::select);
    CHECK(copy.selectedEntities().size() == 3U);

    // Command-first COPY reuses common-transform object collection.
    sketch::SketchInteractionState command_first;
    CHECK(command_first.activateCopy(model));
    CHECK(command_first.tool() == sketch::SketchTool::copy);
    CHECK(
        command_first.commonTransformStage() ==
        sketch::CommonTransformStage::select_objects);
    CHECK(!command_first.completeTransformSelection(model));

    CHECK(command_first.addSelection(line));
    CHECK(command_first.addSelection(circle));
    CHECK(command_first.completeTransformSelection(model));
    CHECK(
        command_first.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(!command_first.addSelection(arc));

    CHECK(
        command_first.acceptTransformPoint(
            sketch::ResolvedSketchInput{{1.0, 1.0}}));
    CHECK(
        command_first.updateTransformPreview(
            sketch::ResolvedSketchInput{{4.0, 6.0}}));
    const auto command_preview =
        command_first.transformGeometryState();
    CHECK(command_preview.has_value());

    const auto command_source =
        sketch::captureSketchTransformGeometry(
            model,
            {line, circle});
    CHECK(command_source.has_value());
    const auto expected_command =
        sketch::translateSketchGeometry(
            *command_source,
            {3.0, 5.0});
    CHECK(expected_command.has_value());
    CHECK(*command_preview == *expected_command);

    command_first.cancelForHistory();
    CHECK(
        command_first.tool() ==
        sketch::SketchTool::select);
    CHECK(command_first.selectedEntities().size() == 2U);

    return EXIT_SUCCESS;
}
