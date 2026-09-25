#include "sketch_viewport_mapping.hpp"

#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QTest>

#include <cmath>
#include <cstdlib>
#include <optional>
#include <vector>

using namespace simplesolid2;

namespace {

bool near(double left, double right) {
    return std::abs(left - right) <= 1.0e-5;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    widget.resize(900, 640);
    widget.show();

    if (!QTest::qWaitForWindowExposed(
            &widget,
            3000)) {
        return EXIT_FAILURE;
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
        return EXIT_FAILURE;
    }

    viewer::SketchPreviewScene preview;
    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Point3{0.0, 8.0, 0.0}});
    if (!widget.setSketchPreviewScene(preview)) {
        return EXIT_FAILURE;
    }
    if (!widget.setSketchPreviewScene(
            viewer::SketchPreviewScene{})) {
        return EXIT_FAILURE;
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

    QTest::mouseMove(&widget, center);
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

    if (spatial_events.size() < 3U ||
        selection_intents != 0) {
        return EXIT_FAILURE;
    }

    bool saw_move = false;
    bool saw_press = false;
    bool saw_release = false;
    for (const auto& event : spatial_events) {
        if (!event.valid()) {
            return EXIT_FAILURE;
        }
        saw_move =
            saw_move ||
            event.phase ==
                viewer::SpatialPointerPhase::move;
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
    if (!saw_move || !saw_press || !saw_release) {
        return EXIT_FAILURE;
    }

    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    if (!support) return EXIT_FAILURE;
    const auto placement =
        part::sketchPlacementForSupport(*support);
    if (!placement) return EXIT_FAILURE;

    auto center_uv =
        ui::detail::sketchPointFromRay(
            *placement,
            spatial_events.back().ray);
    if (!center_uv ||
        !near(center_uv->u, 0.0) ||
        !near(center_uv->v, 0.0)) {
        return EXIT_FAILURE;
    }

    spatial_events.clear();
    widget.orbitByRadians(0.35, -0.22);
    QTest::mouseMove(&widget, center);
    QApplication::processEvents();
    if (spatial_events.empty()) {
        return EXIT_FAILURE;
    }

    const auto orbit_uv =
        ui::detail::sketchPointFromRay(
            *placement,
            spatial_events.back().ray);
    if (!orbit_uv ||
        !near(orbit_uv->u, 0.0) ||
        !near(orbit_uv->v, 0.0)) {
        return EXIT_FAILURE;
    }

    widget.setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting::
            presentation_selection);
    widget.setCursorMode(
        viewer::ViewportCursorMode::
            select_pick_box);

    const auto spatial_before =
        spatial_events.size();
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{5, 5});
    QApplication::processEvents();

    if (selection_intents == 0 ||
        spatial_events.size() != spatial_before) {
        return EXIT_FAILURE;
    }

    widget.setCursorMode(
        viewer::ViewportCursorMode::
            system_default);

    widget.close();
    return EXIT_SUCCESS;
}
