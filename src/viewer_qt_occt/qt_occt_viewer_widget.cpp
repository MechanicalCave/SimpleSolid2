#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include "navigation_mapping.hpp"

#include <Aspect_DisplayConnection.hxx>
#include <Graphic3d_Camera.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_Window.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <QMouseEvent>
#include <QPaintEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace simplesolid2::viewer_qt_occt {

class QtOcctViewerWidget::Impl final {
public:
    explicit Impl(QtOcctViewerWidget& owner) : owner_{owner} {}

    void ensureInitialized() {
        if (!view_.IsNull()) return;

        Handle(Aspect_DisplayConnection) display = new Aspect_DisplayConnection();
        driver_ = new OpenGl_GraphicDriver(display);
        viewer_ = new V3d_Viewer(driver_);
        viewer_->SetDefaultLights();
        viewer_->SetLightOn();

        view_ = viewer_->CreateView();
        window_ = new WNT_Window(
            reinterpret_cast<Aspect_Handle>(owner_.winId()));
        view_->SetWindow(window_);
        if (!window_->IsMapped()) window_->Map();

        view_->SetBackgroundColor(
            Quantity_Color{0.12, 0.14, 0.18, Quantity_TOC_RGB});
        view_->SetAutoZFitMode(true, 1.05);

        viewer::CameraState initial;
        initial.projection = viewer::CameraProjection::orthographic;
        static_cast<void>(setCameraState(initial));
    }

    std::optional<viewer::CameraState> cameraState() const {
        if (view_.IsNull()) return std::nullopt;

        const auto camera = view_->Camera();
        if (camera.IsNull()) return std::nullopt;

        const auto eye = camera->Eye();
        const auto center = camera->Center();
        const auto up = camera->Up();

        viewer::CameraState state;
        state.eye = {eye.X(), eye.Y(), eye.Z()};
        state.target = {center.X(), center.Y(), center.Z()};
        state.up = {up.X(), up.Y(), up.Z()};
        state.projection = camera->IsOrthographic()
            ? viewer::CameraProjection::orthographic
            : viewer::CameraProjection::perspective;
        state.scale = camera->Scale();

        return viewer::validateCameraState(state).valid
            ? std::optional<viewer::CameraState>{state}
            : std::nullopt;
    }

    bool setCameraState(const viewer::CameraState& state) {
        if (!viewer::validateCameraState(state).valid) return false;

        if (view_.IsNull()) {
            ensureInitialized();
            if (view_.IsNull()) return false;
        }

        const auto camera = view_->Camera();
        if (camera.IsNull()) return false;

        camera->SetProjectionType(
            state.projection == viewer::CameraProjection::orthographic
                ? Graphic3d_Camera::Projection_Orthographic
                : Graphic3d_Camera::Projection_Perspective);
        camera->SetEyeAndCenter(
            gp_Pnt{state.eye.x, state.eye.y, state.eye.z},
            gp_Pnt{state.target.x, state.target.y, state.target.z});
        camera->SetUp(gp_Dir{state.up.x, state.up.y, state.up.z});
        camera->OrthogonalizeUp();
        camera->SetScale(state.scale);
        view_->Redraw();
        return true;
    }

    bool setStandardView(viewer::StandardView standard_view) {
        ensureInitialized();
        const auto current = cameraState();
        if (!current) return false;
        const auto next = viewer::cameraForStandardView(*current, standard_view);
        return next && setCameraState(*next);
    }

    bool setProjection(viewer::CameraProjection projection) {
        ensureInitialized();
        const auto current = cameraState();
        if (!current) return false;
        const auto next = viewer::cameraWithProjection(*current, projection);
        return next && setCameraState(*next);
    }

    void fitAll() {
        ensureInitialized();
        if (view_.IsNull()) return;
        view_->FitAll(0.05, false);
        view_->Redraw();
    }

    void zoomByFactor(double factor) {
        ensureInitialized();
        if (view_.IsNull() || !std::isfinite(factor) || factor <= 0.0) return;
        view_->SetZoom(factor, true);
        view_->Redraw();
    }

    void panByPixels(int delta_x, int delta_y) {
        ensureInitialized();
        if (view_.IsNull()) return;

        const auto dpr = owner_.devicePixelRatioF();
        const auto dx = view_->Convert(
            static_cast<int>(std::lround(delta_x * dpr)));
        const auto dy = view_->Convert(
            static_cast<int>(std::lround(delta_y * dpr)));
        view_->Panning(-dx, dy, 1.0, true);
        view_->Redraw();
    }

    void orbitByScreenAngles(const detail::OrbitScreenAngles& angles) {
        ensureInitialized();
        if (view_.IsNull() ||
            !std::isfinite(angles.x) ||
            !std::isfinite(angles.y) ||
            !std::isfinite(angles.z)) {
            return;
        }

        view_->Rotate(angles.x, angles.y, angles.z, true);
        view_->Redraw();
    }

    void orbitByRadians(double horizontal, double vertical) {
        orbitByScreenAngles({horizontal, vertical, 0.0});
    }

    void resize() {
        if (view_.IsNull()) return;
        const auto native_window = view_->Window();
        if (!native_window.IsNull()) native_window->DoResize();
        view_->MustBeResized();
        view_->Redraw();
    }

    void redraw() {
        if (!view_.IsNull()) view_->Redraw();
    }

    void beginMiddleDrag(int x, int y, bool orbit) noexcept {
        middle_dragging_ = true;
        middle_orbit_ = orbit;
        last_mouse_x_ = x;
        last_mouse_y_ = y;
    }

    void updateMiddleDrag(int x, int y, bool orbit) {
        if (!middle_dragging_) return;

        const auto dx = x - last_mouse_x_;
        const auto dy = y - last_mouse_y_;
        last_mouse_x_ = x;
        last_mouse_y_ = y;
        middle_orbit_ = orbit;

        if (dx == 0 && dy == 0) return;

        if (middle_orbit_) {
            constexpr double radians_per_pixel = 0.006;
            orbitByScreenAngles(
                detail::orbitScreenAnglesFromMouseDelta(
                    dx, dy, radians_per_pixel));
        } else {
            panByPixels(-dx, -dy);
        }
    }

    void endMiddleDrag() noexcept {
        middle_dragging_ = false;
    }

    void zoomAtLogicalPoint(int logical_x, int logical_y, int angle_delta_y) {
        ensureInitialized();
        if (view_.IsNull() || angle_delta_y == 0) return;

        const auto dpr = owner_.devicePixelRatioF();
        const auto x = static_cast<int>(
            std::lround(static_cast<double>(logical_x) * dpr));
        const auto y = static_cast<int>(
            std::lround(static_cast<double>(logical_y) * dpr));
        const auto fraction = detail::wheelZoomDragFraction(angle_delta_y);

        const auto dx = static_cast<int>(std::lround(
            std::max(1.0, static_cast<double>(owner_.width()) * dpr) *
            fraction));
        const auto dy = static_cast<int>(std::lround(
            std::max(1.0, static_cast<double>(owner_.height()) * dpr) *
            fraction));

        view_->StartZoomAtPoint(x, y);
        view_->ZoomAtPoint(x, y, x + dx, y + dy);
        view_->Redraw();
    }

private:
    QtOcctViewerWidget& owner_;
    bool middle_dragging_{};
    bool middle_orbit_{};
    int last_mouse_x_{};
    int last_mouse_y_{};

    Handle(OpenGl_GraphicDriver) driver_;
    Handle(V3d_Viewer) viewer_;
    Handle(V3d_View) view_;
    Handle(WNT_Window) window_;
};

QtOcctViewerWidget::QtOcctViewerWidget(QWidget* parent)
    : QWidget{parent},
      impl_{std::make_unique<Impl>(*this)} {
    setAttribute(Qt::WA_NativeWindow, true);
    setAttribute(Qt::WA_PaintOnScreen, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

QtOcctViewerWidget::~QtOcctViewerWidget() = default;

std::optional<viewer::CameraState> QtOcctViewerWidget::cameraState() const {
    return impl_->cameraState();
}

bool QtOcctViewerWidget::setCameraState(const viewer::CameraState& state) {
    return impl_->setCameraState(state);
}

bool QtOcctViewerWidget::setStandardView(viewer::StandardView view) {
    return impl_->setStandardView(view);
}

bool QtOcctViewerWidget::setProjection(viewer::CameraProjection projection) {
    return impl_->setProjection(projection);
}

void QtOcctViewerWidget::fitAll() {
    impl_->fitAll();
}

void QtOcctViewerWidget::zoomByFactor(double factor) {
    impl_->zoomByFactor(factor);
}

void QtOcctViewerWidget::panByPixels(int delta_x, int delta_y) {
    impl_->panByPixels(delta_x, delta_y);
}

void QtOcctViewerWidget::orbitByRadians(double horizontal, double vertical) {
    impl_->orbitByRadians(horizontal, vertical);
}

QPaintEngine* QtOcctViewerWidget::paintEngine() const {
    return nullptr;
}

void QtOcctViewerWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    impl_->ensureInitialized();
    impl_->redraw();
}

void QtOcctViewerWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    impl_->resize();
}

void QtOcctViewerWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    impl_->ensureInitialized();
    impl_->resize();

    QTimer::singleShot(0, this, [this] {
        if (isVisible()) impl_->resize();
    });
}

void QtOcctViewerWidget::mousePressEvent(QMouseEvent* event) {
    setFocus(Qt::MouseFocusReason);

    if (event->button() == Qt::MiddleButton) {
        const auto point = event->position().toPoint();
        impl_->beginMiddleDrag(
            point.x(),
            point.y(),
            (event->modifiers() & Qt::ShiftModifier) != 0);
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void QtOcctViewerWidget::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::MiddleButton) != 0) {
        const auto point = event->position().toPoint();
        impl_->updateMiddleDrag(
            point.x(),
            point.y(),
            (event->modifiers() & Qt::ShiftModifier) != 0);
        event->accept();
        return;
    }

    QWidget::mouseMoveEvent(event);
}

void QtOcctViewerWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        impl_->endMiddleDrag();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void QtOcctViewerWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        impl_->endMiddleDrag();
        impl_->fitAll();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void QtOcctViewerWidget::wheelEvent(QWheelEvent* event) {
    const auto point = event->position().toPoint();
    impl_->zoomAtLogicalPoint(
        point.x(),
        point.y(),
        event->angleDelta().y());
    event->accept();
}

} // namespace simplesolid2::viewer_qt_occt
