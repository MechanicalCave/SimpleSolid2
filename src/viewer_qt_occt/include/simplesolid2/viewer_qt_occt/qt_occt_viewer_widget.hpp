#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QWidget>

#include <cstddef>
#include <cstdint>
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

struct QtOcctRuntimeDiagnostics final {
    std::size_t update_current_viewer_calls{};
    std::size_t redraw_calls{};
    std::size_t sketch_native_objects_current{};
    std::size_t sketch_wire_style_applications{};
    std::size_t sketch_rectangle_queries{};
    std::size_t sketch_rectangle_segments{};
    std::uint64_t sketch_rectangle_token_lookups{};
    std::uint64_t sketch_rectangle_token_comparisons{};
};

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

    [[nodiscard]] std::optional<viewer::ViewportPoint2>
    projectWorldPoint(
        viewer::Point3 point) const override;

    void setNavigationCubeActionHandler(
        viewer::NavigationCubeActionHandler handler) override;

    [[nodiscard]] bool animateCameraState(
        const viewer::CameraState& state,
        double duration_seconds,
        bool fit_all) override;

    [[nodiscard]] bool setReferenceScene(
        const viewer::ReferenceScene& scene) override;

    [[nodiscard]] bool setSolidScene(
        const viewer::SolidScene& scene) override;

    [[nodiscard]] bool setSketchScene(
        const viewer::SketchScene& scene) override;

    [[nodiscard]] bool setProfileScene(
        const viewer::ProfileScene& scene) override;

    [[nodiscard]] bool setProfilePreviewScene(
        const viewer::ProfilePreviewScene& scene) override;

    [[nodiscard]] bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override;

    [[nodiscard]] bool setSketchGripScene(
        const viewer::SketchGripScene& scene) override;

    [[nodiscard]] bool setSketchInteractionPresentation(
        const viewer::SketchInteractionPresentation& presentation) override;

    [[nodiscard]] viewer::SketchGripQueryResult
    querySketchGrip(
        viewer::ViewportPoint2 point) override;

    [[nodiscard]] bool setSketchMeasureMarkerScene(
        const viewer::SketchMeasureMarkerScene& scene) override;

    [[nodiscard]] viewer::SketchMeasureMarkerQueryResult
    querySketchMeasureMarkers(
        viewer::ViewportPoint2 point) override;

    [[nodiscard]] bool setSketchMeasureCueScene(
        const viewer::SketchMeasureCueScene& scene) override;

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

    [[nodiscard]] bool setSketchSnapInferenceScene(
        const viewer::SketchSnapInferenceScene& scene) override;

    [[nodiscard]] bool setSketchDynamicInputOverlay(
        const viewer::SketchDynamicInputOverlay& overlay) override;

    void clearSketchDynamicInputOverlay() override;

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

    // SR-02 provider-private runtime diagnostics. These counters are not
    // part of IDocumentViewport and never carry CAD identity or persistence.
    void resetRuntimeDiagnostics() noexcept;
    [[nodiscard]] QtOcctRuntimeDiagnostics
    runtimeDiagnostics() const noexcept;

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
