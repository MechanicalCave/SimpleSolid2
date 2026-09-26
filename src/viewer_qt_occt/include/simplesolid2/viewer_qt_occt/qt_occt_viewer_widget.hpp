#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QWidget>

#include <memory>
#include <optional>

class QContextMenuEvent;
class QMouseEvent;
class QPaintEngine;
class QPaintEvent;
class QResizeEvent;
class QShowEvent;
class QWheelEvent;

namespace simplesolid2::viewer_qt_occt {

class QtOcctViewerWidget final
    : public QWidget,
      public viewer::IDocumentViewport {
public:
    explicit QtOcctViewerWidget(QWidget* parent = nullptr);
    ~QtOcctViewerWidget() override;

    QtOcctViewerWidget(const QtOcctViewerWidget&) = delete;
    QtOcctViewerWidget& operator=(const QtOcctViewerWidget&) = delete;

    [[nodiscard]] std::optional<viewer::CameraState>
    cameraState() const override;

    [[nodiscard]] bool setCameraState(
        const viewer::CameraState& state) override;

    [[nodiscard]] bool setStandardView(
        viewer::StandardView view) override;

    [[nodiscard]] bool setProjection(
        viewer::CameraProjection projection) override;

    void fitAll() override;

    void setNavigationCubeActionHandler(
        viewer::NavigationCubeActionHandler handler) override;

    [[nodiscard]] bool animateCameraState(
        const viewer::CameraState& state,
        double duration_seconds,
        bool fit_all) override;

    [[nodiscard]] bool setReferenceScene(
        const viewer::ReferenceScene& scene) override;

    [[nodiscard]] bool setSketchScene(
        const viewer::SketchScene& scene) override;

    [[nodiscard]] bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override;

    [[nodiscard]] bool setSketchGripScene(
        const viewer::SketchGripScene& scene) override;

    [[nodiscard]] bool setSketchInteractionPresentation(
        const viewer::SketchInteractionPresentation& presentation) override;

    [[nodiscard]] viewer::SketchGripQueryResult
    querySketchGrip(
        viewer::ViewportPoint2 point) override;

    [[nodiscard]] bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override;

    [[nodiscard]] viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override;

    [[nodiscard]] viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule rule) override;

    [[nodiscard]] bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override;

    void clearSketchSelectionBoxOverlay() override;

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override;

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override;

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing) override;

    void setCursorMode(
        viewer::ViewportCursorMode mode) override;

    void zoomByFactor(double factor);
    void panByPixels(int delta_x, int delta_y);
    void orbitByRadians(
        double horizontal_radians,
        double vertical_radians);

protected:
    QPaintEngine* paintEngine() const override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace simplesolid2::viewer_qt_occt
