#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QTimer>

#include <cstdlib>

using namespace simplesolid2;

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    widget.resize(900, 640);
    widget.show();

    int result = EXIT_FAILURE;

    QTimer::singleShot(
        100,
        &app,
        [&] {
            viewer::ReferenceScene scene;
            scene.grid = viewer::GridPresentation{};

            viewer::ReferencePresentation origin;
            origin.token = {1U};
            origin.kind =
                viewer::ReferencePresentationKind::point;
            origin.extent = 3.0;
            scene.references.push_back(origin);

            viewer::ReferencePresentation x_axis;
            x_axis.token = {2U};
            x_axis.kind =
                viewer::ReferencePresentationKind::x_axis;
            x_axis.u_axis = {1.0, 0.0, 0.0};
            x_axis.extent = 45.0;
            scene.references.push_back(x_axis);

            viewer::ReferencePresentation y_axis;
            y_axis.token = {3U};
            y_axis.kind =
                viewer::ReferencePresentationKind::y_axis;
            y_axis.u_axis = {0.0, 1.0, 0.0};
            y_axis.extent = 45.0;
            scene.references.push_back(y_axis);

            viewer::ReferencePresentation z_axis;
            z_axis.token = {4U};
            z_axis.kind =
                viewer::ReferencePresentationKind::z_axis;
            z_axis.u_axis = {0.0, 0.0, 1.0};
            z_axis.extent = 45.0;
            scene.references.push_back(z_axis);

            viewer::ReferencePresentation xy_plane;
            xy_plane.token = {5U};
            xy_plane.kind =
                viewer::ReferencePresentationKind::plane;
            xy_plane.u_axis = {1.0, 0.0, 0.0};
            xy_plane.v_axis = {0.0, 1.0, 0.0};
            xy_plane.extent = 35.0;
            scene.references.push_back(xy_plane);

            bool ok = scene.valid();
            ok = ok && widget.setReferenceScene(scene);
            ok = ok && widget.setStandardView(
                           viewer::StandardView::isometric);
            ok = ok && widget.setProjection(
                           viewer::CameraProjection::orthographic);

            widget.fitAll();

            const auto camera = widget.cameraState();
            ok = ok && camera.has_value();
            ok = ok &&
                 camera->projection ==
                     viewer::CameraProjection::orthographic;

            // R10 Construction cadence is a provider presentation effect.
            // Short/long committed Lines and preview Lines share the same
            // construction flag without authored dash segmentation.
            viewer::SketchScene construction_scene;
            construction_scene.lines = {
                {
                    viewer::PresentationToken{0x201U},
                    {0.0, 0.0, 0.0},
                    {5.0, 0.0, 0.0},
                    true},
                {
                    viewer::PresentationToken{0x202U},
                    {0.0, 5.0, 0.0},
                    {100.0, 5.0, 0.0},
                    true},
            };
            ok = ok &&
                 construction_scene.valid() &&
                 widget.setSketchScene(
                     construction_scene);

            viewer::SketchPreviewScene
                construction_preview;
            construction_preview.lines = {
                {
                    {0.0, 10.0, 0.0},
                    {100.0, 10.0, 0.0},
                    true},
            };
            ok = ok &&
                 construction_preview.valid() &&
                 widget.setSketchPreviewScene(
                     construction_preview);
            ok = ok &&
                 widget.setSketchPreviewScene(
                     viewer::SketchPreviewScene{});

            // R11 snap/OTRACK presentation is a real provider overlay:
            // one transient current marker/label plus up to two acquired
            // tracking anchors, all non-selectable and runtime-only.
            viewer::SketchSnapInferenceScene
                snap_scene;
            snap_scene.current =
                viewer::SketchSnapMarkerPresentation{
                    {15.0, 15.0, 0.0},
                    viewer::SketchSnapMarkerKind::
                        endpoint,
                    "Endpoint"};
            snap_scene.acquired = {
                {
                    {10.0, 15.0, 0.0},
                    viewer::SketchSnapMarkerKind::
                        midpoint,
                    "Midpoint"},
                {
                    {20.0, 15.0, 0.0},
                    viewer::SketchSnapMarkerKind::
                        center,
                    "Center"},
            };
            snap_scene.guides = {
                {
                    {10.0, 15.0, 0.0},
                    {1.0, 0.0, 0.0},
                    viewer::SketchInferenceGuideKind::
                        sketch_u},
                {
                    {20.0, 15.0, 0.0},
                    {0.0, 1.0, 0.0},
                    viewer::SketchInferenceGuideKind::
                        sketch_v},
            };
            ok = ok &&
                 snap_scene.valid() &&
                 widget.setSketchSnapInferenceScene(
                     snap_scene);

            snap_scene.current =
                viewer::SketchSnapMarkerPresentation{
                    {28.0, 16.0, 0.0},
                    viewer::SketchSnapMarkerKind::
                        extension,
                    "EXT"};
            snap_scene.extension_guide =
                viewer::SketchExtensionGuidePresentation{
                    {20.0, 16.0, 0.0},
                    {10.0, 0.0, 0.0},
                    viewer::Point3{
                        28.0,
                        16.0,
                        0.0}};
            snap_scene.acquired.resize(1U);
            snap_scene.guides.resize(1U);
            snap_scene.guides[0].kind =
                viewer::SketchInferenceGuideKind::
                    additional_direction;
            snap_scene.guides[0].direction =
                {1.0, 1.0, 0.0};
            ok = ok &&
                 widget.setSketchSnapInferenceScene(
                     snap_scene);
            ok = ok &&
                 widget.setSketchSnapInferenceScene(
                     viewer::
                         SketchSnapInferenceScene{});

            viewer::SketchDynamicInputOverlay dyn_overlay{
                viewer::ViewportPoint2{
                    895.0,
                    635.0},
                {
                    {
                        "Distance",
                        "100 mm",
                        viewer::SketchDynamicInputValueState::
                            locked},
                    {
                        "Angle",
                        "45\xC2\xB0",
                        viewer::SketchDynamicInputValueState::
                            assisted},
                    {
                        "dU",
                        {},
                        viewer::SketchDynamicInputValueState::
                            free},
                },
                1U};
            ok = ok &&
                 widget.setSketchDynamicInputOverlay(
                     dyn_overlay);
            dyn_overlay.anchor =
                viewer::ViewportPoint2{
                    900.0,
                    640.0};
            dyn_overlay.fields[0].display_value =
                "101 mm";
            ok = ok &&
                 widget.setSketchDynamicInputOverlay(
                     dyn_overlay);
            widget.clearSketchDynamicInputOverlay();

            result = ok ? EXIT_SUCCESS : EXIT_FAILURE;
            widget.close();
            app.quit();
        });

    app.exec();
    return result;
}
