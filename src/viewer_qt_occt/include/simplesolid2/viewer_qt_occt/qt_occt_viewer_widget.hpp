#pragma once
#include <simplesolid2/viewer/camera_state.hpp>
#include <simplesolid2/viewer/navigation.hpp>
#include <QWidget>
#include <memory>
#include <optional>
class QPaintEngine; class QPaintEvent; class QResizeEvent; class QShowEvent; class QMouseEvent; class QWheelEvent;
namespace simplesolid2::viewer_qt_occt {
class QtOcctViewerWidget final:public QWidget {
public:
 explicit QtOcctViewerWidget(QWidget* parent=nullptr);
 ~QtOcctViewerWidget() override;
 QtOcctViewerWidget(const QtOcctViewerWidget&)=delete;
 QtOcctViewerWidget& operator=(const QtOcctViewerWidget&)=delete;
 [[nodiscard]] std::optional<viewer::CameraState> cameraState() const;
 [[nodiscard]] bool setCameraState(const viewer::CameraState& state);
 [[nodiscard]] bool setStandardView(viewer::StandardView view);
 [[nodiscard]] bool setProjection(viewer::CameraProjection projection);
 void fitAll(); void zoomByFactor(double factor); void panByPixels(int delta_x,int delta_y); void orbitByRadians(double horizontal_radians,double vertical_radians);
protected:
 QPaintEngine* paintEngine() const override;
 void paintEvent(QPaintEvent* event) override; void resizeEvent(QResizeEvent* event) override; void showEvent(QShowEvent* event) override;
 void mousePressEvent(QMouseEvent* event) override; void mouseMoveEvent(QMouseEvent* event) override; void mouseReleaseEvent(QMouseEvent* event) override;
 void mouseDoubleClickEvent(QMouseEvent* event) override; void wheelEvent(QWheelEvent* event) override;
private:
 class Impl; std::unique_ptr<Impl> impl_;
};
}
