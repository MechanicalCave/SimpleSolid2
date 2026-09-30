#include <simplesolid2/sketch/interaction_state.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
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

    // Selection-first normal MOVE freezes the existing selection and
    // uses Base Point -> destination against the same transform core.
    sketch::SketchInteractionState selection_first;
    CHECK(selection_first.addSelection(line_id));
    CHECK(selection_first.addSelection(circle_id));
    CHECK(selection_first.addSelection(arc_id));
    CHECK(selection_first.activateMove(model));
    CHECK(
        selection_first.tool() ==
        sketch::SketchTool::move);
    CHECK(
        selection_first.moveStage() ==
        sketch::MoveStage::await_base_point);
    CHECK(!selection_first.toggleSelection(line_id));

    CHECK(
        selection_first.acceptMoveBasePoint(
            sketch::ResolvedSketchInput{
                {1.0, 2.0}}));
    CHECK(
        selection_first.moveStage() ==
        sketch::MoveStage::await_destination);
    CHECK(
        selection_first.updateMoveDestination(
            sketch::ResolvedSketchInput{
                {9.0, -1.0}}));

    const auto selection_first_geometry =
        selection_first.moveGeometryState();
    CHECK(selection_first_geometry.has_value());
    CHECK(*selection_first_geometry == *translated);

    CHECK(selection_first.escape());
    CHECK(
        selection_first.tool() ==
        sketch::SketchTool::select);
    CHECK(selection_first.selectedEntities().size() == 3U);

    // Command-first MOVE starts in object collection and freezes only
    // when collection is explicitly completed.
    sketch::SketchInteractionState command_first;
    CHECK(command_first.activateMove(model));
    CHECK(
        command_first.moveStage() ==
        sketch::MoveStage::select_objects);
    CHECK(!command_first.completeMoveSelection(model));

    CHECK(command_first.addSelection(line_id));
    CHECK(command_first.addSelection(circle_id));
    CHECK(
        command_first.completeMoveSelection(model));
    CHECK(
        command_first.moveStage() ==
        sketch::MoveStage::await_base_point);
    CHECK(!command_first.addSelection(arc_id));

    CHECK(
        command_first.acceptMoveBasePoint(
            sketch::ResolvedSketchInput{
                {-4.0, 3.0}}));
    CHECK(
        command_first.updateMoveDestination(
            sketch::ResolvedSketchInput{
                {-4.0, 3.0}}));

    const auto zero_move =
        command_first.moveGeometryState();
    CHECK(zero_move.has_value());

    const auto command_capture =
        sketch::captureSketchTransformGeometry(
            model,
            {line_id, circle_id});
    CHECK(command_capture.has_value());
    CHECK(*zero_move == *command_capture);

    command_first.finishMove();
    CHECK(
        command_first.tool() ==
        sketch::SketchTool::select);
    CHECK(command_first.selectedEntities().size() == 2U);

    // Profile topology reliability: a common transform of a closed
    // Line+Arc contour must not change whether it is a bounded region.
    // This regression is part of the final R10 exact-head FULL closeout.
    // This is the production reproducer from the 2026-09-30 audit.
    sketch::SketchModel dome;
    const auto dome_arc =
        dome.addArc(
            {0.0, 0.0},
            25.0,
            0.0,
            pi);
    const auto dome_left =
        dome.addLine(
            {-25.0, 0.0},
            {-25.0, -50.0});
    const auto dome_bottom =
        dome.addLine(
            {-25.0, -50.0},
            {25.0, -50.0});
    const auto dome_right =
        dome.addLine(
            {25.0, -50.0},
            {25.0, 0.0});

    const auto baseline_regions =
        sketch::analyzeRegions(dome);
    CHECK(baseline_regions.complete());
    CHECK(baseline_regions.regions.size() == 1U);
    CHECK(baseline_regions.regions.front().holes.empty());

    const std::vector<sketch::EntityId> dome_ids{
        dome_arc,
        dome_left,
        dome_bottom,
        dome_right};
    const auto dome_capture =
        sketch::captureSketchTransformGeometry(
            dome,
            dome_ids);
    CHECK(dome_capture.has_value());

    const double dome_area =
        2500.0 + 312.5 * pi;
    const double dome_perimeter =
        150.0 + 25.0 * pi;

    const auto model_from_geometry =
        [&](const sketch::SketchTransformGeometry& geometry) {
            auto state = dome.state();
            state.lines = geometry.lines;
            state.circles = geometry.circles;
            state.arcs = geometry.arcs;
            return sketch::SketchModel::restore(
                std::move(state));
        };

    const auto verify_geometry =
        [&](const sketch::SketchTransformGeometry& geometry,
            double expected_area,
            double expected_perimeter) {
            auto transformed_model =
                model_from_geometry(geometry);
            CHECK(transformed_model.has_value());

            const auto regions =
                sketch::analyzeRegions(*transformed_model);
            CHECK(regions.complete());
            CHECK(regions.regions.size() == 1U);
            CHECK(regions.regions.front().holes.empty());
            CHECK(near(
                regions.regions.front().area,
                expected_area,
                1.0e-8));
            CHECK(near(
                regions.regions.front().perimeter,
                expected_perimeter,
                1.0e-8));
        };

    const auto verify_dome_translation =
        [&](double delta_u) {
            const auto moved =
                sketch::translateSketchGeometry(
                    *dome_capture,
                    {delta_u, 0.0});
            CHECK(moved.has_value());
            verify_geometry(
                *moved,
                dome_area,
                dome_perimeter);
        };

    verify_dome_translation(0.0);
    verify_dome_translation(-45.65);
    verify_dome_translation(-42.77);
    verify_dome_translation(-41.59);

    // Rigid/common transforms preserve the already-closed topology. Metrics
    // remain invariant for Rotate/Mirror and scale geometrically for Scale.
    const auto rotated =
        sketch::rotateSketchGeometry(
            *dome_capture,
            {13.25, -7.5},
            37.0 * pi / 180.0);
    CHECK(rotated.has_value());

    // Each preserved Line<->Arc endpoint remains one exact endpoint
    // intersection after rotation; no near-duplicate quadratic root may
    // fragment the boundary.
    auto rotated_model =
        model_from_geometry(*rotated);
    CHECK(rotated_model.has_value());
    for (const auto line_id :
         {dome_left, dome_right}) {
        const auto relation =
            sketch::analyzeCurveRelation(
                *rotated_model,
                line_id,
                dome_arc);
        CHECK(
            relation.status ==
            sketch::CurveRelationStatus::discrete);
        CHECK(relation.intersections.size() == 1U);
        CHECK(
            relation.intersections.front()
                .first_endpoint ||
            relation.intersections.front()
                .second_endpoint);
    }

    verify_geometry(
        *rotated,
        dome_area,
        dome_perimeter);

    const auto scaled =
        sketch::scaleSketchGeometry(
            *dome_capture,
            {-8.75, 11.5},
            1.75);
    CHECK(scaled.has_value());
    verify_geometry(
        *scaled,
        dome_area * 1.75 * 1.75,
        dome_perimeter * 1.75);

    const auto mirrored =
        sketch::mirrorSketchGeometry(
            *dome_capture,
            {-12.0, 4.0},
            {19.0, 17.0});
    CHECK(mirrored.has_value());
    verify_geometry(
        *mirrored,
        dome_area,
        dome_perimeter);

    // Repeated mixed transforms preserve the same topology rather than
    // accumulating a Line/Arc endpoint split.
    const auto moved_once =
        sketch::translateSketchGeometry(
            *dome_capture,
            {-45.65, 7.125});
    CHECK(moved_once.has_value());
    const auto rotated_after_move =
        sketch::rotateSketchGeometry(
            *moved_once,
            {3.5, -2.25},
            -23.0 * pi / 180.0);
    CHECK(rotated_after_move.has_value());
    const auto scaled_after_rotate =
        sketch::scaleSketchGeometry(
            *rotated_after_move,
            {1.0, 2.0},
            0.625);
    CHECK(scaled_after_rotate.has_value());
    const auto mirrored_after_scale =
        sketch::mirrorSketchGeometry(
            *scaled_after_rotate,
            {-5.0, -3.0},
            {8.0, 9.0});
    CHECK(mirrored_after_scale.has_value());
    verify_geometry(
        *mirrored_after_scale,
        dome_area * 0.625 * 0.625,
        dome_perimeter * 0.625);

    // A real authored gap must remain a gap. The transform provenance may
    // preserve only contacts proven to exist in the source geometry; it must
    // never weld merely-near endpoints.
    auto open_dome = dome;
    CHECK(
        open_dome.updateLine(
            dome_right,
            {25.001, -50.0},
            {25.001, 0.0}));
    const auto open_analysis =
        sketch::analyzeRegions(open_dome);
    CHECK(open_analysis.regions.empty());
    CHECK(!open_analysis.complete());

    bool saw_open_boundary = false;
    for (const auto& diagnostic :
         open_analysis.diagnostics) {
        if (diagnostic.kind ==
            sketch::RegionAnalysisDiagnosticKind::
                open_boundary) {
            saw_open_boundary = true;
        }
    }
    CHECK(saw_open_boundary);

    const auto open_capture =
        sketch::captureSketchTransformGeometry(
            open_dome,
            dome_ids);
    CHECK(open_capture.has_value());
    CHECK(open_capture->line_arc_contacts.size() == 1U);
    CHECK(
        open_capture->line_arc_contacts.front().line_id ==
        dome_left);
    CHECK(
        open_capture->line_arc_contacts.front().arc_id ==
        dome_arc);

    const auto open_moved =
        sketch::translateSketchGeometry(
            *open_capture,
            {-45.65, 0.0});
    CHECK(open_moved.has_value());
    auto open_moved_model =
        [&]() {
            auto state = open_dome.state();
            state.lines = open_moved->lines;
            state.circles = open_moved->circles;
            state.arcs = open_moved->arcs;
            return sketch::SketchModel::restore(
                std::move(state));
        }();
    CHECK(open_moved_model.has_value());
    const auto open_moved_analysis =
        sketch::analyzeRegions(*open_moved_model);
    CHECK(open_moved_analysis.regions.empty());
    CHECK(!open_moved_analysis.complete());

    // A single diameter Line can own both exact endpoint contacts with one
    // semicircular Arc. Both contacts must survive a common transform.
    sketch::SketchModel semicircle;
    const auto semicircle_arc =
        semicircle.addArc(
            {0.0, 0.0},
            25.0,
            0.0,
            pi);
    const auto semicircle_diameter =
        semicircle.addLine(
            {-25.0, 0.0},
            {25.0, 0.0});
    const std::vector<sketch::EntityId>
        semicircle_ids{
            semicircle_arc,
            semicircle_diameter};
    const auto semicircle_capture =
        sketch::captureSketchTransformGeometry(
            semicircle,
            semicircle_ids);
    CHECK(semicircle_capture.has_value());
    CHECK(
        semicircle_capture->line_arc_contacts.size() ==
        2U);

    const auto semicircle_moved =
        sketch::translateSketchGeometry(
            *semicircle_capture,
            {-45.65, 13.375});
    CHECK(semicircle_moved.has_value());
    auto semicircle_state = semicircle.state();
    semicircle_state.lines =
        semicircle_moved->lines;
    semicircle_state.circles =
        semicircle_moved->circles;
    semicircle_state.arcs =
        semicircle_moved->arcs;
    auto moved_semicircle =
        sketch::SketchModel::restore(
            std::move(semicircle_state));
    CHECK(moved_semicircle.has_value());
    const auto semicircle_regions =
        sketch::analyzeRegions(
            *moved_semicircle);
    CHECK(semicircle_regions.complete());
    CHECK(semicircle_regions.regions.size() == 1U);
    CHECK(near(
        semicircle_regions.regions.front().area,
        312.5 * pi,
        1.0e-8));

    return EXIT_SUCCESS;
}
