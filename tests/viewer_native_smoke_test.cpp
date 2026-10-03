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

            widget.resetRuntimeDiagnostics();
            ok = ok && widget.setReferenceScene(scene);
            const auto same_reference_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 same_reference_metrics.
                         update_current_viewer_calls == 0U &&
                 same_reference_metrics.redraw_calls == 0U;

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

            viewer::SolidScene solid_scene;
            solid_scene.triangles.push_back(
                viewer::SolidTrianglePresentation{
                    {0.0, 0.0, 0.0},
                    {20.0, 0.0, 0.0},
                    {0.0, 20.0, 0.0},
                    {0.0, 0.0, 1.0},
                    {0.0, 0.0, 1.0},
                    {0.0, 0.0, 1.0}});
            ok = ok &&
                 solid_scene.valid() &&
                 widget.setSolidScene(
                     solid_scene);

            const auto committed_solid_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 committed_solid_metrics.
                         solid_committed_displayed &&
                 !committed_solid_metrics.
                          solid_preview_displayed &&
                 committed_solid_metrics.
                         solid_committed_style_expected &&
                 committed_solid_metrics.
                         solid_preview_style_expected &&
                 committed_solid_metrics.
                         solid_shading_styles_isolated;

            viewer::SolidPreviewScene
                solid_preview;
            solid_preview.triangles =
                solid_scene.triangles;
            solid_preview.tone =
                viewer::SolidPreviewTone::
                    subtractive;
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 solid_preview.valid() &&
                 widget.setSolidPreviewScene(
                     solid_preview);
            const auto solid_preview_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 solid_preview_metrics.
                         update_current_viewer_calls == 1U &&
                 solid_preview_metrics.
                         solid_committed_displayed &&
                 solid_preview_metrics.
                         solid_preview_displayed &&
                 solid_preview_metrics.
                         solid_committed_style_expected &&
                 solid_preview_metrics.
                         solid_preview_style_expected &&
                 solid_preview_metrics.
                         solid_shading_styles_isolated;

            // H6: replacing orange Cut preview with blue Add preview must
            // change only the preview-owned shading aspect.
            viewer::SolidPreviewScene additive_preview =
                solid_preview;
            additive_preview.tone =
                viewer::SolidPreviewTone::additive;
            ok = ok &&
                 widget.setSolidPreviewScene(
                     additive_preview);
            const auto additive_preview_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 additive_preview_metrics.
                         solid_committed_displayed &&
                 additive_preview_metrics.
                         solid_preview_displayed &&
                 additive_preview_metrics.
                         solid_committed_style_expected &&
                 additive_preview_metrics.
                         solid_preview_style_expected &&
                 additive_preview_metrics.
                         solid_shading_styles_isolated;

            // Restore subtractive preview for same-scene no-op coverage.
            ok = ok &&
                 widget.setSolidPreviewScene(
                     solid_preview);

            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSolidPreviewScene(
                     solid_preview);
            const auto same_solid_preview_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 same_solid_preview_metrics.
                         update_current_viewer_calls == 0U &&
                 same_solid_preview_metrics.
                         redraw_calls == 0U;

            ok = ok &&
                 widget.setSolidPreviewScene(
                     viewer::SolidPreviewScene{});
            const auto cleared_solid_preview_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 cleared_solid_preview_metrics.
                         solid_committed_displayed &&
                 !cleared_solid_preview_metrics.
                          solid_preview_displayed &&
                 cleared_solid_preview_metrics.
                         solid_committed_style_expected &&
                 cleared_solid_preview_metrics.
                         solid_preview_style_expected &&
                 cleared_solid_preview_metrics.
                         solid_shading_styles_isolated;

            ok = ok &&
                 widget.setSolidScene(
                     viewer::SolidScene{});

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
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 construction_scene.valid() &&
                 widget.setSketchScene(
                     construction_scene);
            const auto changed_sketch_scene_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 changed_sketch_scene_metrics.
                         update_current_viewer_calls == 1U &&
                 changed_sketch_scene_metrics.redraw_calls == 0U;

            viewer::SketchPreviewScene
                construction_preview;
            construction_preview.lines = {
                {
                    {0.0, 10.0, 0.0},
                    {100.0, 10.0, 0.0},
                    true},
            };
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 construction_preview.valid() &&
                 widget.setSketchPreviewScene(
                     construction_preview);
            const auto changed_preview_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 changed_preview_metrics.
                         update_current_viewer_calls == 1U &&
                 changed_preview_metrics.redraw_calls == 0U;

            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSketchPreviewScene(
                     viewer::SketchPreviewScene{});
            const auto cleared_preview_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 cleared_preview_metrics.
                         update_current_viewer_calls == 1U &&
                 cleared_preview_metrics.redraw_calls == 0U;

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

            // SR-02: an identical runtime-only snap/inference scene is an
            // exact provider no-op. It must not create another OCCT viewer
            // update/redraw pair.
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSketchSnapInferenceScene(
                     snap_scene);
            const auto same_snap_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 same_snap_metrics.
                         update_current_viewer_calls == 0U &&
                 same_snap_metrics.redraw_calls == 0U;

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
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSketchSnapInferenceScene(
                     snap_scene);
            const auto changed_snap_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 changed_snap_metrics.
                         update_current_viewer_calls == 1U &&
                 changed_snap_metrics.redraw_calls == 1U;

            ok = ok &&
                 widget.setSketchSnapInferenceScene(
                     viewer::
                         SketchSnapInferenceScene{});
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSketchSnapInferenceScene(
                     viewer::
                         SketchSnapInferenceScene{});
            const auto empty_snap_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 empty_snap_metrics.
                         update_current_viewer_calls == 0U &&
                 empty_snap_metrics.redraw_calls == 0U;

            // SR-02 Finish Sketch teardown repeatedly requests empty
            // runtime-only scenes that may already be empty. These exact
            // no-ops must not create provider flushes.
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSketchPreviewScene(
                     viewer::SketchPreviewScene{}) &&
                 widget.setSketchGripScene(
                     viewer::SketchGripScene{}) &&
                 widget.setSketchInteractionPresentation(
                     viewer::SketchInteractionPresentation{}) &&
                 widget.setSketchMeasureMarkerScene(
                     viewer::SketchMeasureMarkerScene{}) &&
                 widget.setSketchMeasureCueScene(
                     viewer::SketchMeasureCueScene{}) &&
                 widget.setProfilePreviewScene(
                     viewer::ProfilePreviewScene{}) &&
                 widget.setPresentationSelection(
                     viewer::PresentationSelection{}) &&
                 widget.setProfileScene(
                     viewer::ProfileScene{});
            const auto empty_runtime_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 empty_runtime_metrics.
                         update_current_viewer_calls == 0U &&
                 empty_runtime_metrics.redraw_calls == 0U;

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

            // Empty authored Sketch replacement still clears a real scene
            // once, then a repeated empty request is an exact no-op.
            ok = ok &&
                 widget.setSketchScene(
                     viewer::SketchScene{});
            widget.resetRuntimeDiagnostics();
            ok = ok &&
                 widget.setSketchScene(
                     viewer::SketchScene{});
            const auto empty_sketch_metrics =
                widget.runtimeDiagnostics();
            ok = ok &&
                 empty_sketch_metrics.
                         update_current_viewer_calls == 0U &&
                 empty_sketch_metrics.redraw_calls == 0U;

            result = ok ? EXIT_SUCCESS : EXIT_FAILURE;
            widget.close();
            app.quit();
        });

    app.exec();
    return result;
}
