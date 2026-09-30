#include <simplesolid2/viewer/sketch_presentation.hpp>
#include <simplesolid2/viewer/spatial_pointer.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-03A Viewer contracts CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    viewer::SketchScene authored;
    CHECK(authored.valid());

    authored.origin =
        viewer::SketchOriginPresentation{
            viewer::Point3{0.0, 0.0, 0.0}};
    authored.lines.push_back(
        viewer::SketchLinePresentation{
            viewer::PresentationToken{1U},
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Point3{10.0, 0.0, 0.0}});
    authored.lines.push_back(
        viewer::SketchLinePresentation{
            viewer::PresentationToken{2U},
            viewer::Point3{10.0, 0.0, 0.0},
            viewer::Point3{10.0, 5.0, 0.0}});
    CHECK(authored.valid());

    authored.curves.push_back(
        viewer::SketchCurvePresentation{
            viewer::PresentationToken{3U},
            {
                viewer::Point3{0.0, 0.0, 0.0},
                viewer::Point3{2.0, 2.0, 0.0},
                viewer::Point3{4.0, 0.0, 0.0},
            }});
    CHECK(authored.valid());

    auto duplicate_curve_token = authored;
    duplicate_curve_token.curves[0].token =
        duplicate_curve_token.lines[0].token;
    CHECK(!duplicate_curve_token.valid());

    auto invalid_curve = authored;
    invalid_curve.curves[0].points[1] =
        invalid_curve.curves[0].points[0];
    CHECK(!invalid_curve.valid());

    auto duplicate_token = authored;
    duplicate_token.lines[1].token =
        duplicate_token.lines[0].token;
    CHECK(!duplicate_token.valid());

    auto invalid_token = authored;
    invalid_token.lines[0].token = {};
    CHECK(!invalid_token.valid());

    auto zero_line = authored;
    zero_line.lines[0].end =
        zero_line.lines[0].start;
    CHECK(!zero_line.valid());

    auto non_finite_line = authored;
    non_finite_line.lines[0].start.x =
        std::numeric_limits<double>::infinity();
    CHECK(!non_finite_line.valid());

    auto non_finite_origin = authored;
    non_finite_origin.origin->position.y =
        std::numeric_limits<double>::quiet_NaN();
    CHECK(!non_finite_origin.valid());

    viewer::SketchPreviewScene preview;
    CHECK(preview.valid());
    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{1.0, 2.0, 3.0},
            viewer::Point3{4.0, 5.0, 6.0}});
    CHECK(preview.valid());
    CHECK(authored.lines.size() == 2U);

    preview.lines.clear();
    CHECK(preview.valid());
    CHECK(authored.lines.size() == 2U);

    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{2.0, 2.0, 2.0},
            viewer::Point3{2.0, 2.0, 2.0}});
    CHECK(!preview.valid());

    const viewer::Ray3 ray{
        viewer::Point3{0.0, 0.0, 10.0},
        viewer::Vec3{0.0, 0.0, -1.0}};
    CHECK(ray.valid());

    viewer::Ray3 zero_ray = ray;
    zero_ray.direction = {};
    CHECK(!zero_ray.valid());

    viewer::Ray3 non_finite_ray = ray;
    non_finite_ray.origin.z =
        std::numeric_limits<double>::infinity();
    CHECK(!non_finite_ray.valid());

    const viewer::ViewportPoint2 point{
        125.5,
        87.25};
    CHECK(point.valid());

    viewer::ViewportPoint2 invalid_point = point;
    invalid_point.x =
        std::numeric_limits<double>::quiet_NaN();
    CHECK(!invalid_point.valid());

    const viewer::SpatialPointerEvent move{
        viewer::SpatialPointerPhase::move,
        point,
        ray};
    CHECK(move.valid());

    auto invalid_event = move;
    invalid_event.ray.direction = {};
    CHECK(!invalid_event.valid());

    CHECK(
        viewer::PrimaryPointerRouting::
            presentation_selection !=
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    CHECK(
        viewer::ViewportCursorMode::
            system_default !=
        viewer::ViewportCursorMode::
            select_pick_box);
    CHECK(
        viewer::ViewportCursorMode::
            select_pick_box !=
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    // R8B provider-neutral measurement marker/cue contracts carry only
    // presentation tokens and runtime roles, never CAD EntityId.
    viewer::SketchMeasureMarkerScene marker_scene;
    marker_scene.markers.push_back(
        viewer::SketchMeasureMarkerPresentation{
            {
                viewer::PresentationToken{11U},
                viewer::SketchMeasureMarkerRole::line_start},
            viewer::Point3{1.0, 2.0, 3.0}});
    marker_scene.markers.push_back(
        viewer::SketchMeasureMarkerPresentation{
            {
                viewer::PresentationToken{11U},
                viewer::SketchMeasureMarkerRole::line_end},
            viewer::Point3{4.0, 5.0, 6.0}});
    marker_scene.selected.push_back(
        marker_scene.markers.front().key);
    CHECK(marker_scene.valid());
    CHECK(!marker_scene.empty());

    auto duplicate_marker = marker_scene;
    duplicate_marker.markers.push_back(
        duplicate_marker.markers.front());
    CHECK(!duplicate_marker.valid());

    auto missing_selected = marker_scene;
    missing_selected.selected = {
        viewer::SketchMeasureMarkerKey{
            viewer::PresentationToken{99U},
            viewer::SketchMeasureMarkerRole::circle_center}};
    CHECK(!missing_selected.valid());

    viewer::SketchMeasureMarkerQueryResult marker_query{
        true,
        {
            marker_scene.markers[0].key,
            marker_scene.markers[1].key}};
    CHECK(marker_query.valid());
    auto duplicate_query = marker_query;
    duplicate_query.markers.push_back(
        duplicate_query.markers.front());
    CHECK(!duplicate_query.valid());
    viewer::SketchMeasureMarkerQueryResult incomplete_query{
        false,
        {marker_scene.markers.front().key}};
    CHECK(!incomplete_query.valid());

    viewer::SketchMeasureCueScene cue_scene;
    cue_scene.highlighted_entities.push_back(
        viewer::PresentationToken{11U});
    cue_scene.segments.push_back(
        viewer::SketchMeasureCueSegment{
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Point3{5.0, 0.0, 0.0},
            viewer::SketchMeasureCueSegmentKind::relation});
    cue_scene.segments.push_back(
        viewer::SketchMeasureCueSegment{
            viewer::Point3{5.0, 0.0, 0.0},
            viewer::Point3{8.0, 0.0, 0.0},
            viewer::SketchMeasureCueSegmentKind::
                supporting_line_continuation});
    cue_scene.cue_point =
        viewer::Point3{8.0, 0.0, 0.0};
    CHECK(cue_scene.valid());
    CHECK(!cue_scene.empty());

    auto duplicate_highlight = cue_scene;
    duplicate_highlight.highlighted_entities.push_back(
        duplicate_highlight.highlighted_entities.front());
    CHECK(!duplicate_highlight.valid());

    auto invalid_cue = cue_scene;
    invalid_cue.segments.front().end =
        invalid_cue.segments.front().start;
    CHECK(!invalid_cue.valid());

    // R10 DYN is a provider-neutral screen-space presentation. The
    // viewer receives formatted text plus visual state; parsing and
    // request-local locks remain in the application interaction layer.
    viewer::SketchDynamicInputOverlay dyn_overlay{
        viewer::ViewportPoint2{320.0, 180.0},
        {
            {
                "Distance",
                "100 mm",
                viewer::SketchDynamicInputValueState::locked},
            {
                "Angle",
                "45\xC2\xB0",
                viewer::SketchDynamicInputValueState::assisted},
            {
                "dU",
                {},
                viewer::SketchDynamicInputValueState::free},
        },
        1U};
    CHECK(dyn_overlay.valid());

    auto invalid_dyn_anchor = dyn_overlay;
    invalid_dyn_anchor.anchor.x =
        std::numeric_limits<double>::quiet_NaN();
    CHECK(!invalid_dyn_anchor.valid());

    auto invalid_dyn_focus = dyn_overlay;
    invalid_dyn_focus.focused_index =
        invalid_dyn_focus.fields.size();
    CHECK(!invalid_dyn_focus.valid());

    auto invalid_dyn_field = dyn_overlay;
    invalid_dyn_field.fields.front().label.clear();
    CHECK(!invalid_dyn_field.valid());

    auto empty_dyn = dyn_overlay;
    empty_dyn.fields.clear();
    CHECK(!empty_dyn.valid());

    return EXIT_SUCCESS;
}
