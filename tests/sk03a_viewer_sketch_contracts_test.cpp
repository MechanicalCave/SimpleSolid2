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

    return EXIT_SUCCESS;
}
