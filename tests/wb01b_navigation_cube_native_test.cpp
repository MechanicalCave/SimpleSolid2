#include <simplesolid2/viewer/navigation_cube.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QTest>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>

using namespace simplesolid2;

namespace {

#define CHECK(expr)     do {         if (!(expr)) {             std::cerr                 << "WB-01B native Navigation Cube CHECK failed at line "                 << __LINE__ << ": " #expr "\n";             return EXIT_FAILURE;         }     } while (false)

bool sameDirection(
    const viewer::Vec3& actual,
    const viewer::Vec3& expected,
    double tolerance = 1.0e-7) {
    const auto a = viewer::normalized(actual);
    const auto e = viewer::normalized(expected);
    if (!a || !e) return false;

    return std::abs(a->x - e->x) <= tolerance &&
           std::abs(a->y - e->y) <= tolerance &&
           std::abs(a->z - e->z) <= tolerance;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    widget.resize(901, 641);
    widget.show();

    CHECK(QTest::qWaitForWindowExposed(
        &widget,
        3000));

    viewer::CameraState front;
    front.eye = {0.0, -100.0, 0.0};
    front.target = {0.0, 0.0, 0.0};
    front.up = {0.0, 0.0, 1.0};
    front.projection =
        viewer::CameraProjection::orthographic;
    front.scale = 100.0;
    CHECK(widget.setCameraState(front));
    QApplication::processEvents();

    std::optional<viewer::NavigationCubeAction>
        last_action;
    int cube_actions = 0;
    widget.setNavigationCubeActionHandler(
        [&](
            const viewer::NavigationCubeAction& action) {
            last_action = action;
            ++cube_actions;
        });

    int spatial_primary = 0;
    widget.setSpatialPointerHandler(
        [&](
            const viewer::SpatialPointerEvent& event) {
            if (event.phase ==
                    viewer::SpatialPointerPhase::
                        primary_press ||
                event.phase ==
                    viewer::SpatialPointerPhase::
                        primary_release) {
                ++spatial_primary;
            }
        });
    widget.setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting::
            spatial_tool_input);

    // The AIS_ViewCube transform persistence is anchored 86 physical
    // pixels from the right/top corner. Its center should be a face hit.
    const QPoint cube_center{
        widget.width() - 86,
        86};
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        cube_center);
    QApplication::processEvents();

    CHECK(cube_actions == 1);
    CHECK(last_action.has_value());
    CHECK(
        last_action->kind ==
        viewer::NavigationCubeActionKind::orient);
    CHECK(spatial_primary == 0);

    // Provider-surface Home is always available.
    last_action.reset();
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{widget.width() - 158, 18});
    QApplication::processEvents();
    CHECK(last_action.has_value());
    CHECK(
        last_action->kind ==
        viewer::NavigationCubeActionKind::home);
    CHECK(spatial_primary == 0);

    // Face-aligned view exposes adjacent-view and roll controls.
    last_action.reset();
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{widget.width() - 24, 86});
    QApplication::processEvents();
    CHECK(last_action.has_value());
    CHECK(
        last_action->kind ==
        viewer::NavigationCubeActionKind::
            adjacent_right);

    last_action.reset();
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{widget.width() - 28, 46});
    QApplication::processEvents();
    CHECK(last_action.has_value());
    CHECK(
        last_action->kind ==
        viewer::NavigationCubeActionKind::
            roll_clockwise);

    // Native animation must land exactly on the semantic target.
    const auto target =
        viewer::cameraForNavigationCubeTarget(
            front,
            viewer::NavigationCubeTarget::
                top_front_right);
    CHECK(target.has_value());
    CHECK(widget.animateCameraState(
        *target,
        0.08,
        false));

    QTest::qWait(220);
    QApplication::processEvents();

    const auto animated = widget.cameraState();
    CHECK(animated.has_value());
    CHECK(
        animated->projection ==
        viewer::CameraProjection::orthographic);
    CHECK(sameDirection(
        animated->target - animated->eye,
        {-1.0, 1.0, -1.0}));
    CHECK(
        std::abs(
            animated->scale -
            target->scale) <= 1.0e-7);

    // Navigation overlay activity remains runtime-only from the provider's
    // perspective; ordinary CAD spatial routing still works away from it.
    const int spatial_before =
        spatial_primary;
    QTest::mouseClick(
        &widget,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{widget.width() / 2,
               widget.height() / 2});
    QApplication::processEvents();
    CHECK(spatial_primary >= spatial_before + 2);

    widget.close();
    return EXIT_SUCCESS;
}
