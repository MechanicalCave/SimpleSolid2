#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04B native selection query CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

bool contains(
    const std::vector<viewer::PresentationToken>& tokens,
    viewer::PresentationToken token) {
    return std::find(
               tokens.begin(),
               tokens.end(),
               token) != tokens.end();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    widget.resize(801, 601);
    widget.show();
    QApplication::processEvents();

    const viewer::CameraState camera{
        viewer::Point3{0.0, 0.0, 100.0},
        viewer::Point3{0.0, 0.0, 0.0},
        viewer::Vec3{0.0, 1.0, 0.0},
        viewer::CameraProjection::orthographic,
        100.0};
    CHECK(widget.setCameraState(camera));

    const viewer::PresentationToken short_token{
        0x4101U};
    const viewer::PresentationToken long_token{
        0x4102U};
    const viewer::PresentationToken reference_token{
        0x201U};

    viewer::ReferenceScene references;
    references.references.push_back(
        viewer::ReferencePresentation{
            reference_token,
            viewer::ReferencePresentationKind::point,
            viewer::Point3{0.0, 0.0, 0.0},
            {},
            {},
            3.0,
            viewer::PresentationRole::base,
            true});
    CHECK(widget.setReferenceScene(references));

    viewer::SketchScene scene;
    scene.lines = {
        viewer::SketchLinePresentation{
            short_token,
            viewer::Point3{-2.0, 0.0, 0.0},
            viewer::Point3{2.0, 0.0, 0.0}},
        viewer::SketchLinePresentation{
            long_token,
            viewer::Point3{0.0, -40.0, 0.0},
            viewer::Point3{0.0, 40.0, 0.0}},
    };
    scene.origin =
        viewer::SketchOriginPresentation{
            viewer::Point3{0.0, 0.0, 0.0}};
    CHECK(widget.setSketchScene(scene));

    viewer::SketchPreviewScene preview;
    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{-40.0, -40.0, 0.0},
            viewer::Point3{40.0, 40.0, 0.0}});
    CHECK(widget.setSketchPreviewScene(preview));

    int selection_intents = 0;
    widget.setSelectionIntentHandler(
        [&selection_intents](
            const viewer::SelectionIntent&) {
            ++selection_intents;
        });

    CHECK(widget.setPresentationSelection(
        viewer::PresentationSelection{
            {reference_token},
            reference_token}));

    const viewer::ViewportPoint2 center{
        static_cast<double>(widget.width()) / 2.0,
        static_cast<double>(widget.height()) / 2.0};

    const auto point_hit =
        widget.querySketchPresentation(center);
    CHECK(point_hit.valid());
    CHECK(point_hit.completed);
    CHECK(point_hit.token.has_value());
    CHECK(
        *point_hit.token == short_token ||
        *point_hit.token == long_token);
    CHECK(*point_hit.token != reference_token);
    CHECK(selection_intents == 0);

    const auto point_empty =
        widget.querySketchPresentation(
            viewer::ViewportPoint2{5.0, 5.0});
    CHECK(point_empty.valid());
    CHECK(point_empty.completed);
    CHECK(!point_empty.token.has_value());
    CHECK(selection_intents == 0);

    const auto nan =
        std::numeric_limits<double>::quiet_NaN();
    const auto point_failed =
        widget.querySketchPresentation(
            viewer::ViewportPoint2{nan, 0.0});
    CHECK(point_failed.valid());
    CHECK(!point_failed.completed);

    const auto central_rect =
        viewer::normalizedViewportRect(
            viewer::ViewportPoint2{
                center.x - 25.0,
                center.y - 25.0},
            viewer::ViewportPoint2{
                center.x + 25.0,
                center.y + 25.0});
    CHECK(central_rect.has_value());

    const auto window =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                window);
    CHECK(window.valid());
    CHECK(window.completed);
    CHECK(contains(window.tokens, short_token));
    CHECK(!contains(window.tokens, long_token));
    CHECK(!contains(window.tokens, reference_token));

    const auto crossing =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(crossing.valid());
    CHECK(crossing.completed);
    CHECK(contains(crossing.tokens, short_token));
    CHECK(contains(crossing.tokens, long_token));
    CHECK(!contains(crossing.tokens, reference_token));

    const auto full_rect =
        viewer::normalizedViewportRect(
            viewer::ViewportPoint2{1.0, 1.0},
            viewer::ViewportPoint2{
                static_cast<double>(
                    widget.width() - 2),
                static_cast<double>(
                    widget.height() - 2)});
    CHECK(full_rect.has_value());
    const auto all_window =
        widget.querySketchPresentations(
            *full_rect,
            viewer::SketchRectangleSelectionRule::
                window);
    CHECK(all_window.completed);
    CHECK(contains(all_window.tokens, short_token));
    CHECK(contains(all_window.tokens, long_token));

    const auto revisionless_overlay =
        viewer::SketchSelectionBoxOverlay{
            {
                center.x - 30.0,
                center.y - 20.0},
            {
                center.x + 30.0,
                center.y + 20.0},
            viewer::SketchRectangleSelectionRule::
                window};
    CHECK(
        widget.setSketchSelectionBoxOverlay(
            revisionless_overlay));
    QApplication::processEvents();

    CHECK(
        widget.findChild<QWidget*>(
            QStringLiteral(
                "ss2SketchSelectionBoxOverlay")) ==
        nullptr);

    for (int iteration = 0;
         iteration < 100;
         ++iteration) {
        const auto offset =
            static_cast<double>(iteration % 20);
        CHECK(widget.setSketchSelectionBoxOverlay(
            viewer::SketchSelectionBoxOverlay{
                {
                    revisionless_overlay.anchor.x - offset,
                    revisionless_overlay.anchor.y},
                {
                    revisionless_overlay.current.x + offset,
                    revisionless_overlay.current.y},
                (iteration % 2) == 0
                    ? viewer::SketchRectangleSelectionRule::
                          window
                    : viewer::SketchRectangleSelectionRule::
                          crossing}));
        QApplication::processEvents();
    }

    widget.clearSketchSelectionBoxOverlay();
    QApplication::processEvents();

    const auto after_overlay =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(after_overlay.completed);
    CHECK(contains(after_overlay.tokens, short_token));
    CHECK(contains(after_overlay.tokens, long_token));

    widget.orbitByRadians(0.35, -0.22);
    QApplication::processEvents();

    const auto after_orbit =
        widget.querySketchPresentations(
            *central_rect,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(after_orbit.valid());
    CHECK(after_orbit.completed);
    CHECK(contains(after_orbit.tokens, short_token));
    CHECK(contains(after_orbit.tokens, long_token));
    CHECK(selection_intents == 0);

    return EXIT_SUCCESS;
}
