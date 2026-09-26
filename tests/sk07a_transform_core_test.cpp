#include <simplesolid2/sketch/interaction_state.hpp>
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
            << "SK-07A transform CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

bool near(double left, double right, double tolerance = 1.0e-12) {
    return std::abs(left - right) <= tolerance;
}

} // namespace

int main() {
    constexpr double pi = std::numbers::pi_v<double>;

    sketch::SketchModel model;
    const auto line_id =
        model.addLine({-1.0, 0.5}, {3.0, 0.5});
    const auto circle_id =
        model.addCircle({4.0, 5.0}, 2.5);
    const auto arc_id =
        model.addArc(
            {-2.0, 7.0},
            6.0,
            pi * 0.25,
            -pi * 0.75);

    const std::vector<sketch::EntityId> ids{
        line_id,
        circle_id,
        arc_id};

    const auto captured =
        sketch::captureSketchTransformGeometry(
            model,
            ids);
    CHECK(captured.has_value());
    CHECK(captured->lines.size() == 1U);
    CHECK(captured->circles.size() == 1U);
    CHECK(captured->arcs.size() == 1U);

    const sketch::Point2 delta{8.0, -3.0};
    const auto translated =
        sketch::translateSketchGeometry(
            *captured,
            delta);
    CHECK(translated.has_value());

    CHECK(translated->lines.front().id == line_id);
    CHECK(
        translated->lines.front().start ==
        sketch::Point2{7.0, -2.5});
    CHECK(
        translated->lines.front().end ==
        sketch::Point2{11.0, -2.5});

    CHECK(translated->circles.front().id == circle_id);
    CHECK(
        translated->circles.front().center ==
        sketch::Point2{12.0, 2.0});
    CHECK(near(translated->circles.front().radius, 2.5));

    CHECK(translated->arcs.front().id == arc_id);
    CHECK(
        translated->arcs.front().center ==
        sketch::Point2{6.0, 4.0});
    CHECK(near(translated->arcs.front().radius, 6.0));
    CHECK(near(
        translated->arcs.front().start_angle,
        pi * 0.25));
    CHECK(near(
        translated->arcs.front().sweep_angle,
        -pi * 0.75));

    const auto zero =
        sketch::translateSketchGeometry(
            *captured,
            {0.0, 0.0});
    CHECK(zero.has_value());
    CHECK(*zero == *captured);

    CHECK(
        !sketch::translateSketchGeometry(
             *captured,
             {
                 std::numeric_limits<double>::infinity(),
                 0.0})
             .has_value());

    CHECK(
        !sketch::captureSketchTransformGeometry(
             model,
             {line_id, line_id})
             .has_value());

    const auto removed =
        model.addLine({10.0, 10.0}, {11.0, 10.0});
    CHECK(model.erase(removed));
    CHECK(
        !sketch::captureSketchTransformGeometry(
             model,
             {removed})
             .has_value());

    // Existing Center-grip Move must use the same transform result.
    sketch::SketchInteractionState interaction;
    CHECK(interaction.addSelection(line_id));
    CHECK(interaction.addSelection(circle_id));
    CHECK(interaction.addSelection(arc_id));

    CHECK(
        interaction.beginDirectManipulation(
            model,
            {
                circle_id,
                sketch::SketchGripRole::circle_center}));
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::move);

    const sketch::Point2 destination{
        4.0 + delta.u,
        5.0 + delta.v};
    CHECK(
        interaction.updateDirectManipulation(
            sketch::ResolvedSketchInput{
                destination}));

    const auto grip_geometry =
        interaction.directManipulationGeometryState();
    CHECK(grip_geometry.has_value());
    CHECK(*grip_geometry == *translated);

    interaction.cancelDirectManipulation();
    CHECK(!interaction.directManipulationActive());

    return EXIT_SUCCESS;
}
