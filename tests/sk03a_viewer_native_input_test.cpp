#include "sketch_viewport_mapping.hpp"

#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QMouseEvent>
#include <QTest>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

bool near(double left, double right) {
    return std::abs(left - right) <= 1.0e-5;
}

int fail(std::string_view stage) {
    std::cerr
        << "SK-03A native input failure: "
        << stage << '\n';
    return EXIT_FAILURE;
}

void sendMouseMove(
    QWidget& widget,
    const QPoint& local) {
    const QPoint global =
        widget.mapToGlobal(local);
    QMouseEvent event{
        QEvent::MouseMove,
        QPointF{local},
        QPointF{global},
        Qt::NoButton,
        Qt::NoButton,
        Qt::NoModifier};
    QApplication::sendEvent(
        &widget,
        &event);
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    // Use odd dimensions so one integer pixel is the exact viewport center.
    widget.resize(901, 641);
    widget.show();

    if (!QTest::qWaitForWindowExposed(
            &widget,
            3000)) {
        return fail("window exposure");
    }

    viewer::SketchScene scene;
    scene.origin =
        viewer::SketchOriginPresentation{
            viewer::Point3{0.0, 0.0, 0.0}};
    scene.lines.push_back(
        viewer::SketchLinePresentation{
            viewer::PresentationToken{0x10000U},
            viewer::Point3{-10.0, 0.0, 0.0},
            viewer::Point3{10.0, 0.0, 0.0}});
    if (!widget.setSketchScene(scene)) {
        return fail("authored Sketch scene");
    }

    const viewer::CameraState sketch_camera{
        viewer::Point3{0.0, 0.0, 100.0},
        viewer::Point3{0.0, 0.0, 0.0},
        viewer::Vec3{0.0, 1.0, 0.0},
        viewer::CameraProjection::orthographic,
        100.0};
    if (!widget.setCameraState(sketch_camera)) {
        return fail("known Sketch camera");
    }
    QApplication::processEvents();

    viewer::SketchPreviewScene preview;
    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Point3{0.0, 8.0, 0.0}});
    if (!widget.setSketchPreviewScene(preview)) {
        return fail("preview scene");
    }
    if (!widget.setSketchPreviewScene(
            viewer::SketchPreviewScene{})) {
        return fail("preview clear");
    }

    int selection_intents = 0;
    widget.setSelectionIntentHandler(
        [&selection_intents](
            const viewer::SelectionIntent&) {
            ++selection_intents;
        });

    std::vector<viewer::SpatialPointerEvent>
        spatial_events;
    widget.setSpatialPointerHandler(
        [&spatial_events](
            const viewer::SpatialPointerEvent& event) {
            spatial_events.push_back(event);
        });

    widget.setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    widget.setCursorMode(
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    const QPoint center{
        widget.width() / 2,
        widget.height() / 2};
    const QPoint offset{
        center.x() - 31,
        center.y() - 19};

    // Prove passive pointer movement independently from primary actions.
    sendMouseMove(widget, offset);
    sendMouseMove(widget, center);
    QApplication::processEvents();

    bool saw_move = false;
    for (const auto& event : spatial_events) {
        if (!event.valid()) {
            return fail("invalid passive spatial event");
        }
        saw_move =
            saw_move ||
            event.phase ==
                viewer::SpatialPointerPhase::move;
    }
    if (!saw_move) {
        return fail("passive move routing");
    }

    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    if (!support) {
        return fail("XY support");
    }
    const auto placement =
        part::sketchPlacementForSupport(*support);
    if (!placement) {
        return fail("XY placement");
    }

    const auto actual_before =
        ui::detail::sketchPointFromRay(
            *placement,
            spatial_events.back().ray);
    if (!actual_before ||
        !near(actual_before->u, 0.0) ||
        !near(actual_before->v, 0.0)) {
        return fail("center ray mapping before orbit");
    }

    // Prove primary routing is exclusive: spatial mode emits press/release
    // but no SelectionIntent for the same action.
    spatial_events.clear();
    const auto selections_before_spatial_click =
        selection_intents;

    QTest::mousePress(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        center);
    QTest::mouseRelease(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        center);
    QApplication::processEvents();

    bool saw_press = false;
    bool saw_release = false;
    for (const auto& event : spatial_events) {
        if (!event.valid()) {
            return fail("invalid spatial primary event");
        }
        saw_press =
            saw_press ||
            event.phase ==
                viewer::SpatialPointerPhase::
                    primary_press;
        saw_release =
            saw_release ||
            event.phase ==
                viewer::SpatialPointerPhase::
                    primary_release;
    }
    if (!saw_press || !saw_release) {
        return fail("spatial primary press/release");
    }
    if (selection_intents !=
        selections_before_spatial_click) {
        return fail("spatial click also emitted SelectionIntent");
    }

    // Orbit changes the camera but the provider ray and host plane mapping
    // must remain mutually consistent.
    spatial_events.clear();
    widget.orbitByRadians(0.35, -0.22);

    const auto camera_after =
        widget.cameraState();
    if (!camera_after) {
        return fail("camera after orbit");
    }
    if (camera_after->eye == sketch_camera.eye) {
        return fail("orbit did not change camera");
    }

    sendMouseMove(widget, offset);
    sendMouseMove(widget, center);
    QApplication::processEvents();
    if (spatial_events.empty()) {
        return fail("move after orbit");
    }

    const auto actual_after =
        ui::detail::sketchPointFromRay(
            *placement,
            spatial_events.back().ray);
    if (!actual_after ||
        !near(actual_after->u, 0.0) ||
        !near(actual_after->v, 0.0)) {
        return fail("center ray mapping after orbit");
    }

    // Presentation-selection mode owns the primary click. Passive movement
    // may still generate spatial move events, but no spatial primary phase
    // may be generated by this click.
    widget.setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting::
            presentation_selection);
    widget.setCursorMode(
        viewer::ViewportCursorMode::
            select_pick_box);

    const auto event_index_before_pick =
        spatial_events.size();
    const auto selections_before_pick =
        selection_intents;

    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{5, 5});
    QApplication::processEvents();

    if (selection_intents <= selections_before_pick) {
        return fail("presentation-selection click");
    }

    for (std::size_t index =
             event_index_before_pick;
         index < spatial_events.size();
         ++index) {
        const auto phase =
            spatial_events[index].phase;
        if (phase ==
                viewer::SpatialPointerPhase::
                    primary_press ||
            phase ==
                viewer::SpatialPointerPhase::
                    primary_release) {
            return fail(
                "presentation click emitted spatial primary phase");
        }
    }

    widget.setCursorMode(
        viewer::ViewportCursorMode::
            system_default);

    widget.close();
    return EXIT_SUCCESS;
}
