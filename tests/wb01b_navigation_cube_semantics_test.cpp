#include <simplesolid2/viewer/navigation_cube.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

#define CHECK(expr)     do {         if (!(expr)) {             std::cerr                 << "WB-01B Navigation Cube CHECK failed at line "                 << __LINE__ << ": " #expr "\n";             return EXIT_FAILURE;         }     } while (false)

bool near(double a, double b) {
    return std::abs(a - b) <= 1.0e-10;
}

bool sameDirection(
    const viewer::Vec3& actual,
    viewer::Vec3 expected) {
    const auto a = viewer::normalized(actual);
    const auto e = viewer::normalized(expected);
    return a && e &&
           near(a->x, e->x) &&
           near(a->y, e->y) &&
           near(a->z, e->z);
}

} // namespace

int main() {
    viewer::CameraState camera;
    camera.eye = {0.0, -10.0, 0.0};
    camera.target = {0.0, 0.0, 0.0};
    camera.up = {0.0, 0.0, 1.0};
    camera.projection =
        viewer::CameraProjection::perspective;
    camera.scale = 42.0;

    struct TargetCase {
        viewer::NavigationCubeTarget target;
        viewer::Vec3 direction;
    };

    const TargetCase targets[] = {
        {viewer::NavigationCubeTarget::front, {0, 1, 0}},
        {viewer::NavigationCubeTarget::back, {0, -1, 0}},
        {viewer::NavigationCubeTarget::left, {1, 0, 0}},
        {viewer::NavigationCubeTarget::right, {-1, 0, 0}},
        {viewer::NavigationCubeTarget::top, {0, 0, -1}},
        {viewer::NavigationCubeTarget::bottom, {0, 0, 1}},
        {viewer::NavigationCubeTarget::top_front, {0, 1, -1}},
        {viewer::NavigationCubeTarget::top_back, {0, -1, -1}},
        {viewer::NavigationCubeTarget::top_left, {1, 0, -1}},
        {viewer::NavigationCubeTarget::top_right, {-1, 0, -1}},
        {viewer::NavigationCubeTarget::bottom_front, {0, 1, 1}},
        {viewer::NavigationCubeTarget::bottom_back, {0, -1, 1}},
        {viewer::NavigationCubeTarget::bottom_left, {1, 0, 1}},
        {viewer::NavigationCubeTarget::bottom_right, {-1, 0, 1}},
        {viewer::NavigationCubeTarget::front_left, {1, 1, 0}},
        {viewer::NavigationCubeTarget::front_right, {-1, 1, 0}},
        {viewer::NavigationCubeTarget::back_left, {1, -1, 0}},
        {viewer::NavigationCubeTarget::back_right, {-1, -1, 0}},
        {viewer::NavigationCubeTarget::top_front_left, {1, 1, -1}},
        {viewer::NavigationCubeTarget::top_front_right, {-1, 1, -1}},
        {viewer::NavigationCubeTarget::top_back_left, {1, -1, -1}},
        {viewer::NavigationCubeTarget::top_back_right, {-1, -1, -1}},
        {viewer::NavigationCubeTarget::bottom_front_left, {1, 1, 1}},
        {viewer::NavigationCubeTarget::bottom_front_right, {-1, 1, 1}},
        {viewer::NavigationCubeTarget::bottom_back_left, {1, -1, 1}},
        {viewer::NavigationCubeTarget::bottom_back_right, {-1, -1, 1}},
    };

    for (const auto& item : targets) {
        const auto next =
            viewer::cameraForNavigationCubeTarget(
                camera,
                item.target);
        CHECK(next.has_value());
        CHECK(
            next->projection ==
            viewer::CameraProjection::orthographic);
        CHECK(near(next->scale, camera.scale));
        CHECK(next->target == camera.target);
        CHECK(sameDirection(
            next->target - next->eye,
            item.direction));
    }

    auto front =
        viewer::cameraForNavigationCubeTarget(
            camera,
            viewer::NavigationCubeTarget::front);
    CHECK(front.has_value());
    CHECK(viewer::navigationCubeFaceAligned(*front));

    auto right =
        viewer::navigationForCubeAction(
            *front,
            {viewer::NavigationCubeActionKind::
                 adjacent_right,
             {}});
    CHECK(right.has_value());
    CHECK(sameDirection(
        right->camera.target -
            right->camera.eye,
        {-1, 0, 0}));

    auto left =
        viewer::navigationForCubeAction(
            *front,
            {viewer::NavigationCubeActionKind::
                 adjacent_left,
             {}});
    CHECK(left.has_value());
    CHECK(sameDirection(
        left->camera.target -
            left->camera.eye,
        {1, 0, 0}));

    auto top =
        viewer::navigationForCubeAction(
            *front,
            {viewer::NavigationCubeActionKind::
                 adjacent_up,
             {}});
    CHECK(top.has_value());
    CHECK(sameDirection(
        top->camera.target -
            top->camera.eye,
        {0, 0, -1}));

    auto bottom =
        viewer::navigationForCubeAction(
            *front,
            {viewer::NavigationCubeActionKind::
                 adjacent_down,
             {}});
    CHECK(bottom.has_value());
    CHECK(sameDirection(
        bottom->camera.target -
            bottom->camera.eye,
        {0, 0, 1}));

    auto roll_cw =
        viewer::navigationForCubeAction(
            *front,
            {viewer::NavigationCubeActionKind::
                 roll_clockwise,
             {}});
    CHECK(roll_cw.has_value());
    CHECK(sameDirection(
        roll_cw->camera.target -
            roll_cw->camera.eye,
        {0, 1, 0}));
    CHECK(sameDirection(
        roll_cw->camera.up,
        {1, 0, 0}));

    auto roll_ccw =
        viewer::navigationForCubeAction(
            *front,
            {viewer::NavigationCubeActionKind::
                 roll_counterclockwise,
             {}});
    CHECK(roll_ccw.has_value());
    CHECK(sameDirection(
        roll_ccw->camera.up,
        {-1, 0, 0}));

    const auto home =
        viewer::navigationForCubeAction(
            camera,
            {viewer::NavigationCubeActionKind::home,
             {}});
    CHECK(home.has_value());
    CHECK(home->fit_all);
    CHECK(
        home->camera.projection ==
        viewer::CameraProjection::orthographic);
    CHECK(sameDirection(
        home->camera.target -
            home->camera.eye,
        {-1, 1, -1}));

    const auto iso =
        viewer::cameraForNavigationCubeTarget(
            camera,
            viewer::NavigationCubeTarget::
                top_front_right);
    CHECK(iso.has_value());
    CHECK(!viewer::navigationCubeFaceAligned(*iso));
    CHECK(!viewer::navigationForCubeAction(
        *iso,
        {viewer::NavigationCubeActionKind::
             adjacent_right,
         {}}));

    return EXIT_SUCCESS;
}
