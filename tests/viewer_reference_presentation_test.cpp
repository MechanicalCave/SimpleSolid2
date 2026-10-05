#include <simplesolid2/viewer/reference_presentation.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2::viewer;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 reference CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    ReferencePresentation point;
    point.token = {1U};
    point.kind = ReferencePresentationKind::point;
    point.extent = 4.0;
    CHECK(point.valid());

    ReferencePresentation axis;
    axis.token = {2U};
    axis.kind = ReferencePresentationKind::x_axis;
    axis.u_axis = {1.0, 0.0, 0.0};
    axis.extent = 50.0;
    CHECK(axis.valid());

    ReferencePresentation plane;
    plane.token = {3U};
    plane.kind = ReferencePresentationKind::plane;
    plane.u_axis = {1.0, 0.0, 0.0};
    plane.v_axis = {0.0, 1.0, 0.0};
    plane.extent = 50.0;
    CHECK(plane.valid());

    plane.v_axis = {2.0, 0.0, 0.0};
    CHECK(!plane.valid());

    axis.token = {};
    CHECK(!axis.valid());

    GridPresentation grid;
    CHECK(grid.valid());

    grid.spacing = 0.0;
    CHECK(!grid.valid());

    grid = {};
    grid.v_axis = {2.0, 0.0, 0.0};
    CHECK(!grid.valid());

    plane.v_axis = {0.0, 1.0, 0.0};
    CHECK(plane.valid());

    ReferencePresentation datum;
    datum.token = {4U};
    datum.kind =
        ReferencePresentationKind::datum_plane;
    datum.origin = {0.0, 0.0, 5.0};
    datum.u_axis = {1.0, 0.0, 0.0};
    datum.v_axis = {0.0, 1.0, 0.0};
    datum.extent = 40.0;
    CHECK(datum.valid());

    ReferenceOwnedLineOverlay overlay;
    overlay.owner = datum.token;
    overlay.segments.push_back(
        {
            {-5.0, 0.0, 5.0},
            {5.0, 0.0, 5.0}});
    CHECK(overlay.valid());

    ReferenceScene scene;
    scene.grid = GridPresentation{};
    scene.references = {point, plane, datum};
    scene.overlays = {overlay};
    CHECK(scene.valid());

    ReferencePlanePreviewPresentation preview;
    preview.origin = {0.0, 0.0, 10.0};
    preview.u_axis = {1.0, 0.0, 0.0};
    preview.v_axis = {0.0, 1.0, 0.0};
    preview.extent = 40.0;
    preview.intersection_segments.push_back(
        {
            {-4.0, 0.0, 10.0},
            {4.0, 0.0, 10.0}});
    CHECK(preview.valid());
    scene.preview = preview;
    CHECK(scene.valid());

    auto invalid_preview = scene;
    invalid_preview.preview->v_axis =
        {2.0, 0.0, 0.0};
    CHECK(!invalid_preview.valid());

    auto invalid_preview_segment = scene;
    invalid_preview_segment.preview->
        intersection_segments.front().second =
        invalid_preview_segment.preview->
            intersection_segments.front().first;
    CHECK(!invalid_preview_segment.valid());

    // Overlay ownership is semantic presentation ownership only: it must
    // resolve to one visible Datum Plane and may never introduce a second
    // token/identity.
    auto invalid_owner = scene;
    invalid_owner.overlays.front().owner = point.token;
    CHECK(!invalid_owner.valid());

    auto hidden_owner = scene;
    hidden_owner.references.back().visible = false;
    CHECK(!hidden_owner.valid());

    auto duplicate_overlay = scene;
    duplicate_overlay.overlays.push_back(overlay);
    CHECK(!duplicate_overlay.valid());

    auto invalid_segment = scene;
    invalid_segment.overlays.front().segments.front().second =
        invalid_segment.overlays.front().segments.front().first;
    CHECK(!invalid_segment.valid());

    scene.references.push_back(point);
    CHECK(!scene.valid());

    return EXIT_SUCCESS;
}
