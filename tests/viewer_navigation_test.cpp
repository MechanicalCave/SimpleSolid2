#include <simplesolid2/viewer/navigation.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace simplesolid2::viewer;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 navigation CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

bool near(double a, double b, double eps = 1.0e-12) {
    return std::abs(a - b) <= eps;
}

double cameraDistance(const CameraState& state) {
    const auto delta = state.target - state.eye;
    return std::sqrt(delta.squaredLength());
}

} // namespace

int main() {
    CameraState state;
    state.eye = {12.0, -6.0, 10.0};
    state.target = {2.0, 3.0, 4.0};
    state.up = {0.0, 0.0, 1.0};
    state.scale = 72.0;
    state.projection = CameraProjection::perspective;
    CHECK(validateCameraState(state).valid);

    const auto original_distance = cameraDistance(state);

    for (const auto standard : {
             StandardView::front,
             StandardView::back,
             StandardView::left,
             StandardView::right,
             StandardView::top,
             StandardView::bottom,
             StandardView::isometric}) {
        const auto changed = cameraForStandardView(state, standard);
        CHECK(changed.has_value());
        CHECK(validateCameraState(*changed).valid);
        CHECK(changed->target == state.target);
        CHECK(near(changed->scale, state.scale));
        CHECK(changed->projection == state.projection);
        CHECK(near(cameraDistance(*changed), original_distance));
    }

    const auto top = cameraForStandardView(state, StandardView::top);
    CHECK(top.has_value());
    CHECK(near(top->eye.x, state.target.x));
    CHECK(near(top->eye.y, state.target.y));
    CHECK(top->eye.z > state.target.z);

    const auto front = cameraForStandardView(state, StandardView::front);
    CHECK(front.has_value());
    CHECK(front->eye.y < state.target.y);

    const auto orthographic =
        cameraWithProjection(state, CameraProjection::orthographic);
    CHECK(orthographic.has_value());
    CHECK(orthographic->projection == CameraProjection::orthographic);
    CHECK(orthographic->eye == state.eye);

    return EXIT_SUCCESS;
}
