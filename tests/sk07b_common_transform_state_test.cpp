#include <simplesolid2/sketch/interaction_state.hpp>
#include <simplesolid2/sketch/transform.hpp>

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
            << "SK-07B interaction CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

bool near(double left, double right, double tolerance = 1.0e-11) {
    return std::abs(left - right) <= tolerance;
}

bool nearPoint(
    sketch::Point2 left,
    sketch::Point2 right,
    double tolerance = 1.0e-11) {
    return near(left.u, right.u, tolerance) &&
           near(left.v, right.v, tolerance);
}

sketch::SketchModel makeModel(
    sketch::EntityId& line_id,
    sketch::EntityId& circle_id,
    sketch::EntityId& arc_id) {
    sketch::SketchModel model;
    line_id = model.addLine({1.0, 0.0}, {3.0, 0.0});
    circle_id = model.addCircle({2.0, 1.0}, 2.0);
    arc_id = model.addArc(
        {1.0, 2.0},
        4.0,
        std::numbers::pi_v<double> * 0.25,
        std::numbers::pi_v<double> * 0.75);
    return model;
}

void selectAll(
    sketch::SketchInteractionState& state,
    sketch::EntityId line_id,
    sketch::EntityId circle_id,
    sketch::EntityId arc_id) {
    CHECK(state.addSelection(line_id));
    CHECK(state.addSelection(circle_id));
    CHECK(state.addSelection(arc_id));
}

} // namespace

int main() {
    constexpr double pi = std::numbers::pi_v<double>;

    sketch::EntityId line_id;
    sketch::EntityId circle_id;
    sketch::EntityId arc_id;
    auto model =
        makeModel(line_id, circle_id, arc_id);

    const auto initial =
        sketch::captureSketchTransformGeometry(
            model,
            {line_id, circle_id, arc_id});
    CHECK(initial.has_value());

    // Selection-first ROTATE.
    sketch::SketchInteractionState rotate;
    selectAll(rotate, line_id, circle_id, arc_id);
    CHECK(rotate.activateRotate(model));
    CHECK(rotate.tool() == sketch::SketchTool::rotate);
    CHECK(
        rotate.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(!rotate.toggleSelection(line_id));

    CHECK(
        rotate.acceptTransformPoint(
            sketch::ResolvedSketchInput{{0.0, 0.0}}));
    CHECK(
        rotate.commonTransformStage() ==
        sketch::CommonTransformStage::await_reference_point);

    // Degenerate reference does not advance.
    CHECK(
        !rotate.acceptTransformPoint(
            sketch::ResolvedSketchInput{{0.0, 0.0}}));
    CHECK(
        rotate.commonTransformStage() ==
        sketch::CommonTransformStage::await_reference_point);

    CHECK(
        rotate.acceptTransformPoint(
            sketch::ResolvedSketchInput{{1.0, 0.0}}));
    CHECK(
        rotate.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);
    CHECK(
        rotate.updateTransformPreview(
            sketch::ResolvedSketchInput{{0.0, 1.0}}));

    const auto rotated =
        rotate.transformGeometryState();
    CHECK(rotated.has_value());
    const auto expected_rotated =
        sketch::rotateSketchGeometry(
            *initial,
            {0.0, 0.0},
            pi * 0.5);
    CHECK(expected_rotated.has_value());
    CHECK(*rotated == *expected_rotated);

    // Preview always derives from the original snapshot.
    CHECK(
        rotate.updateTransformPreview(
            sketch::ResolvedSketchInput{{-1.0, 0.0}}));
    const auto rotated_180 =
        rotate.transformGeometryState();
    CHECK(rotated_180.has_value());
    const auto expected_180 =
        sketch::rotateSketchGeometry(
            *initial,
            {0.0, 0.0},
            pi);
    CHECK(expected_180.has_value());
    CHECK(
        nearPoint(
            rotated_180->lines.front().start,
            expected_180->lines.front().start));
    CHECK(
        nearPoint(
            rotated_180->circles.front().center,
            expected_180->circles.front().center));

    rotate.finishTransform();
    CHECK(rotate.tool() == sketch::SketchTool::select);
    CHECK(rotate.selectedEntities().size() == 3U);

    // Command-first SCALE.
    sketch::SketchInteractionState scale;
    CHECK(scale.activateScale(model));
    CHECK(scale.tool() == sketch::SketchTool::scale);
    CHECK(
        scale.commonTransformStage() ==
        sketch::CommonTransformStage::select_objects);
    CHECK(!scale.completeTransformSelection(model));

    CHECK(scale.addSelection(line_id));
    CHECK(scale.addSelection(circle_id));
    CHECK(scale.addSelection(arc_id));
    CHECK(scale.completeTransformSelection(model));
    CHECK(
        scale.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(!scale.addSelection(line_id));

    CHECK(
        scale.acceptTransformPoint(
            sketch::ResolvedSketchInput{{1.0, 1.0}}));
    CHECK(
        scale.acceptTransformPoint(
            sketch::ResolvedSketchInput{{2.0, 1.0}}));
    CHECK(
        scale.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);

    // Destination at Base Point would create factor 0 and is invalid.
    CHECK(
        scale.updateTransformPreview(
            sketch::ResolvedSketchInput{{1.0, 1.0}}));
    CHECK(!scale.transformGeometryState().has_value());

    CHECK(
        scale.updateTransformPreview(
            sketch::ResolvedSketchInput{{3.0, 1.0}}));
    const auto scaled =
        scale.transformGeometryState();
    CHECK(scaled.has_value());
    const auto expected_scaled =
        sketch::scaleSketchGeometry(
            *initial,
            {1.0, 1.0},
            2.0);
    CHECK(expected_scaled.has_value());
    CHECK(*scaled == *expected_scaled);

    // Factor 1 is a valid exact no-op geometry result.
    CHECK(
        scale.updateTransformPreview(
            sketch::ResolvedSketchInput{{2.0, 1.0}}));
    const auto identity_scale =
        scale.transformGeometryState();
    CHECK(identity_scale.has_value());
    CHECK(*identity_scale == *initial);

    CHECK(scale.escape());
    CHECK(scale.tool() == sketch::SketchTool::select);
    CHECK(scale.selectedEntities().size() == 3U);

    // Command-first MIRROR preserves collected selection on Esc.
    sketch::SketchInteractionState mirror;
    CHECK(mirror.activateMirror(model));
    CHECK(
        mirror.commonTransformStage() ==
        sketch::CommonTransformStage::select_objects);
    CHECK(mirror.addSelection(line_id));
    CHECK(mirror.addSelection(arc_id));
    CHECK(mirror.completeTransformSelection(model));
    CHECK(
        mirror.commonTransformStage() ==
        sketch::CommonTransformStage::await_axis_start);

    CHECK(
        mirror.acceptTransformPoint(
            sketch::ResolvedSketchInput{{0.0, 0.0}}));
    CHECK(
        mirror.commonTransformStage() ==
        sketch::CommonTransformStage::await_axis_end);

    CHECK(
        mirror.updateTransformPreview(
            sketch::ResolvedSketchInput{{1.0, 0.0}}));
    const auto mirrored =
        mirror.transformGeometryState();
    CHECK(mirrored.has_value());

    const auto mirror_capture =
        sketch::captureSketchTransformGeometry(
            model,
            {line_id, arc_id});
    CHECK(mirror_capture.has_value());
    const auto expected_mirror =
        sketch::mirrorSketchGeometry(
            *mirror_capture,
            {0.0, 0.0},
            {1.0, 0.0});
    CHECK(expected_mirror.has_value());
    CHECK(*mirrored == *expected_mirror);
    CHECK(
        near(
            mirrored->arcs.front().sweep_angle,
            -pi * 0.75));

    CHECK(
        mirror.updateTransformPreview(
            sketch::ResolvedSketchInput{{0.0, 0.0}}));
    CHECK(!mirror.transformGeometryState().has_value());

    CHECK(mirror.escape());
    CHECK(mirror.tool() == sketch::SketchTool::select);
    CHECK(mirror.selectedEntities().size() == 2U);

    // History cancellation drops transient transform but preserves selection.
    CHECK(mirror.activateRotate(model));
    CHECK(
        mirror.acceptTransformPoint(
            sketch::ResolvedSketchInput{{0.0, 0.0}}));
    mirror.cancelForHistory();
    CHECK(mirror.tool() == sketch::SketchTool::select);
    CHECK(mirror.selectedEntities().size() == 2U);

    return EXIT_SUCCESS;
}
