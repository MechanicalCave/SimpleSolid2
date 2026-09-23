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

            result = ok ? EXIT_SUCCESS : EXIT_FAILURE;
            widget.close();
            app.quit();
        });

    app.exec();
    return result;
}
