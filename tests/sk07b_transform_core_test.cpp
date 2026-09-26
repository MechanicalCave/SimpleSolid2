#include <simplesolid2/sketch/transform.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <numbers>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-07B transform CHECK failed at line "
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

} // namespace

int main() {
    constexpr double pi = std::numbers::pi_v<double>;

    sketch::SketchModel model;
    const auto line_id =
        model.addLine({1.0, 0.0}, {3.0, 0.0});
    const auto circle_id =
        model.addCircle({2.0, 1.0}, 2.5);
    const auto arc_id =
        model.addArc(
            {1.0, 2.0},
            4.0,
            pi * 0.25,
            pi * 0.75);

    const auto captured =
        sketch::captureSketchTransformGeometry(
            model,
            {line_id, circle_id, arc_id});
    CHECK(captured.has_value());

    // Rotate +90 deg about origin.
    const auto rotated =
        sketch::rotateSketchGeometry(
            *captured,
            {0.0, 0.0},
            pi * 0.5);
    CHECK(rotated.has_value());
    CHECK(rotated->lines.front().id == line_id);
    CHECK(nearPoint(
        rotated->lines.front().start,
        {0.0, 1.0}));
    CHECK(nearPoint(
        rotated->lines.front().end,
        {0.0, 3.0}));
    CHECK(rotated->circles.front().id == circle_id);
    CHECK(nearPoint(
        rotated->circles.front().center,
        {-1.0, 2.0}));
    CHECK(near(rotated->circles.front().radius, 2.5));
    CHECK(rotated->arcs.front().id == arc_id);
    CHECK(nearPoint(
        rotated->arcs.front().center,
        {-2.0, 1.0}));
    CHECK(near(rotated->arcs.front().radius, 4.0));
    CHECK(near(
        rotated->arcs.front().start_angle,
        pi * 0.75));
    CHECK(near(
        rotated->arcs.front().sweep_angle,
        pi * 0.75));

    const auto zero_rotation =
        sketch::rotateSketchGeometry(
            *captured,
            {5.0, -3.0},
            0.0);
    CHECK(zero_rotation.has_value());
    CHECK(*zero_rotation == *captured);

    // Positive uniform scale x2 about (1,1).
    const auto scaled =
        sketch::scaleSketchGeometry(
            *captured,
            {1.0, 1.0},
            2.0);
    CHECK(scaled.has_value());
    CHECK(nearPoint(
        scaled->lines.front().start,
        {1.0, -1.0}));
    CHECK(nearPoint(
        scaled->lines.front().end,
        {5.0, -1.0}));
    CHECK(nearPoint(
        scaled->circles.front().center,
        {3.0, 1.0}));
    CHECK(near(scaled->circles.front().radius, 5.0));
    CHECK(nearPoint(
        scaled->arcs.front().center,
        {1.0, 3.0}));
    CHECK(near(scaled->arcs.front().radius, 8.0));
    CHECK(near(
        scaled->arcs.front().start_angle,
        pi * 0.25));
    CHECK(near(
        scaled->arcs.front().sweep_angle,
        pi * 0.75));

    const auto identity_scale =
        sketch::scaleSketchGeometry(
            *captured,
            {-10.0, 8.0},
            1.0);
    CHECK(identity_scale.has_value());
    CHECK(*identity_scale == *captured);

    CHECK(
        !sketch::scaleSketchGeometry(
             *captured,
             {0.0, 0.0},
             0.0)
             .has_value());
    CHECK(
        !sketch::scaleSketchGeometry(
             *captured,
             {0.0, 0.0},
             -1.0)
             .has_value());

    // Mirror across the X axis.
    const auto mirrored =
        sketch::mirrorSketchGeometry(
            *captured,
            {0.0, 0.0},
            {1.0, 0.0});
    CHECK(mirrored.has_value());
    CHECK(nearPoint(
        mirrored->lines.front().start,
        {1.0, 0.0}));
    CHECK(nearPoint(
        mirrored->lines.front().end,
        {3.0, 0.0}));
    CHECK(nearPoint(
        mirrored->circles.front().center,
        {2.0, -1.0}));
    CHECK(near(mirrored->circles.front().radius, 2.5));
    CHECK(nearPoint(
        mirrored->arcs.front().center,
        {1.0, -2.0}));
    CHECK(near(mirrored->arcs.front().radius, 4.0));
    CHECK(near(
        mirrored->arcs.front().start_angle,
        -pi * 0.25));
    CHECK(near(
        mirrored->arcs.front().sweep_angle,
        -pi * 0.75));

    CHECK(
        !sketch::mirrorSketchGeometry(
             *captured,
             {1.0, 1.0},
             {1.0, 1.0})
             .has_value());

    const double inf =
        std::numeric_limits<double>::infinity();
    CHECK(
        !sketch::rotateSketchGeometry(
             *captured,
             {0.0, 0.0},
             inf)
             .has_value());
    CHECK(
        !sketch::scaleSketchGeometry(
             *captured,
             {inf, 0.0},
             2.0)
             .has_value());
    CHECK(
        !sketch::mirrorSketchGeometry(
             *captured,
             {0.0, 0.0},
             {inf, 0.0})
             .has_value());

    return EXIT_SUCCESS;
}
