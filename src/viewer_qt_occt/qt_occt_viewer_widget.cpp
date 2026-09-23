#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include "navigation_mapping.hpp"

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <Graphic3d_Camera.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <TopoDS_Compound.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_Window.hxx>
#include <gp_Ax3.hxx>
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
#include <cstdint>
#include <set>
#include <utility>
#include <vector>

namespace simplesolid2::viewer_qt_occt {
namespace {

gp_Pnt toGpPoint(const viewer::Point3& point) {
    return {point.x, point.y, point.z};
}

viewer::Point3 offset(
    const viewer::Point3& origin,
    const viewer::Vec3& axis,
    double amount) noexcept {
    return {
        origin.x + axis.x * amount,
        origin.y + axis.y * amount,
        origin.z + axis.z * amount,
    };
}

struct ReferenceStyle final {
    Quantity_Color color;
    double line_width{1.0};
};

ReferenceStyle styleFor(
    const viewer::ReferencePresentation& reference) {
    if (reference.role ==
        viewer::PresentationRole::primary_selection) {
        return {
            Quantity_Color{
                1.00, 0.78, 0.18,
                Quantity_TOC_RGB},
            3.2};
    }

    if (reference.role ==
        viewer::PresentationRole::secondary_selection) {
        return {
            Quantity_Color{
                0.25, 0.78, 1.00,
                Quantity_TOC_RGB},
            2.6};
    }

    switch (reference.kind) {
    case viewer::ReferencePresentationKind::x_axis:
        return {
            Quantity_Color{
                0.92, 0.24, 0.24,
                Quantity_TOC_RGB},
            2.0};
    case viewer::ReferencePresentationKind::y_axis:
        return {
            Quantity_Color{
                0.30, 0.82, 0.38,
                Quantity_TOC_RGB},
            2.0};
    case viewer::ReferencePresentationKind::z_axis:
        return {
            Quantity_Color{
                0.30, 0.48, 0.96,
                Quantity_TOC_RGB},
            2.0};
    case viewer::ReferencePresentationKind::plane:
        return {
            Quantity_Color{
                0.52, 0.58, 0.68,
                Quantity_TOC_RGB},
            1.2};
    case viewer::ReferencePresentationKind::point:
        return {
            Quantity_Color{
                0.88, 0.88, 0.90,
                Quantity_TOC_RGB},
            1.8};
    }

    return {
        Quantity_Color{
            0.78, 0.78, 0.80,
            Quantity_TOC_RGB},
        1.0};
}

Handle(AIS_Shape) makeAxisObject(
    const viewer::ReferencePresentation& reference) {
    const auto direction =
        viewer::normalized(reference.u_axis);
    if (!direction) return {};

    const auto half = reference.extent * 0.5;
    const auto start =
        offset(reference.origin, *direction, -half);
    const auto end =
        offset(reference.origin, *direction, half);

    BRepBuilderAPI_MakeEdge edge{
        toGpPoint(start),
        toGpPoint(end)};
    if (!edge.IsDone()) return {};

    return new AIS_Shape(edge.Edge());
}

Handle(AIS_Shape) makePlaneObject(
    const viewer::ReferencePresentation& reference) {
    const auto u =
        viewer::normalized(reference.u_axis);
    const auto v =
        viewer::normalized(reference.v_axis);
    if (!u || !v) return {};

    const auto half = reference.extent * 0.5;

    const auto p0 =
        offset(
            offset(reference.origin, *u, -half),
            *v,
            -half);
    const auto p1 =
        offset(
            offset(reference.origin, *u, half),
            *v,
            -half);
    const auto p2 =
        offset(
            offset(reference.origin, *u, half),
            *v,
            half);
    const auto p3 =
        offset(
            offset(reference.origin, *u, -half),
            *v,
            half);

    BRepBuilderAPI_MakePolygon polygon;
    polygon.Add(toGpPoint(p0));
    polygon.Add(toGpPoint(p1));
    polygon.Add(toGpPoint(p2));
    polygon.Add(toGpPoint(p3));
    polygon.Close();

    if (!polygon.IsDone()) return {};

    return new AIS_Shape(polygon.Wire());
}

Handle(AIS_Shape) makePointObject(
    const viewer::ReferencePresentation& reference) {
    const auto half =
        std::max(0.5, reference.extent * 0.5);

    TopoDS_Compound compound;
    BRep_Builder builder;
    builder.MakeCompound(compound);

    const std::array<viewer::Vec3, 3> axes{
        viewer::Vec3{1.0, 0.0, 0.0},
        viewer::Vec3{0.0, 1.0, 0.0},
        viewer::Vec3{0.0, 0.0, 1.0},
    };

    for (const auto& axis : axes) {
        const auto start =
            offset(reference.origin, axis, -half);
        const auto end =
            offset(reference.origin, axis, half);

        BRepBuilderAPI_MakeEdge edge{
            toGpPoint(start),
            toGpPoint(end)};
        if (!edge.IsDone()) return {};

        builder.Add(compound, edge.Edge());
    }

    return new AIS_Shape(compound);
}

Handle(AIS_InteractiveObject) makeReferenceObject(
    const viewer::ReferencePresentation& reference) {
    Handle(AIS_Shape) shape;

    switch (reference.kind) {
    case viewer::ReferencePresentationKind::point:
        shape = makePointObject(reference);
        break;
    case viewer::ReferencePresentationKind::x_axis:
    case viewer::ReferencePresentationKind::y_axis:
    case viewer::ReferencePresentationKind::z_axis:
        shape = makeAxisObject(reference);
        break;
    case viewer::ReferencePresentationKind::plane:
        shape = makePlaneObject(reference);
        break;
    }

    if (shape.IsNull()) return {};

    const auto style = styleFor(reference);
    shape->SetColor(style.color);
    shape->SetWidth(style.line_width);
    return shape;
}

bool validReferenceScene(
    const viewer::ReferenceScene& scene) {
    std::set<std::uint64_t> tokens;

    for (const auto& reference :
         scene.references) {
        if (!reference.valid()) return false;
        if (!tokens.insert(
                reference.token.value)
                 .second) {
            return false;
        }
    }

    return true;
}

} // namespace

class QtOcctViewerWidget::Impl final {
public:
    explicit Impl(QtOcctViewerWidget& owner)
        : owner_{owner} {}

    void ensureInitialized() {
        if (!view_.IsNull()) return;

        Handle(Aspect_DisplayConnection) display =
            new Aspect_DisplayConnection();

        driver_ =
            new OpenGl_GraphicDriver(display);
        viewer_ =
            new V3d_Viewer(driver_);
        viewer_->SetDefaultLights();
        viewer_->SetLightOn();

        context_ =
            new AIS_InteractiveContext(viewer_);

        view_ = viewer_->CreateView();
        window_ = new WNT_Window(
            reinterpret_cast<Aspect_Handle>(
                owner_.winId()));

        view_->SetWindow(window_);
        if (!window_->IsMapped()) {
            window_->Map();
        }

        view_->SetBackgroundColor(
            Quantity_Color{
                0.12, 0.14, 0.18,
                Quantity_TOC_RGB});
        view_->SetAutoZFitMode(true, 1.05);

        viewer::CameraState initial;
        initial.projection =
            viewer::CameraProjection::orthographic;
        static_cast<void>(
            setCameraState(initial));

        if (reference_grid_) {
            applyReferenceGrid(
                *reference_grid_,
                false);
        }

        rebuildReferenceScene(false);
        view_->Redraw();
    }

    std::optional<viewer::CameraState>
    cameraState() const {
        if (view_.IsNull()) {
            return std::nullopt;
        }

        const auto camera =
            view_->Camera();
        if (camera.IsNull()) {
            return std::nullopt;
        }

        const auto eye = camera->Eye();
        const auto center = camera->Center();
        const auto up = camera->Up();

        viewer::CameraState state;
        state.eye = {
            eye.X(), eye.Y(), eye.Z()};
        state.target = {
            center.X(),
            center.Y(),
            center.Z()};
        state.up = {
            up.X(), up.Y(), up.Z()};
        state.projection =
            camera->IsOrthographic()
                ? viewer::CameraProjection::
                      orthographic
                : viewer::CameraProjection::
                      perspective;
        state.scale = camera->Scale();

        return viewer::validateCameraState(
                   state)
                   .valid
            ? std::optional<
                  viewer::CameraState>{state}
            : std::nullopt;
    }

    bool setCameraState(
        const viewer::CameraState& state) {
        if (!viewer::validateCameraState(
                 state)
                 .valid) {
            return false;
        }

        if (view_.IsNull()) {
            ensureInitialized();
            if (view_.IsNull()) return false;
        }

        const auto camera =
            view_->Camera();
        if (camera.IsNull()) return false;

        camera->SetProjectionType(
            state.projection ==
                    viewer::CameraProjection::
                        orthographic
                ? Graphic3d_Camera::
                      Projection_Orthographic
                : Graphic3d_Camera::
                      Projection_Perspective);

        camera->SetEyeAndCenter(
            gp_Pnt{
                state.eye.x,
                state.eye.y,
                state.eye.z},
            gp_Pnt{
                state.target.x,
                state.target.y,
                state.target.z});

        camera->SetUp(
            gp_Dir{
                state.up.x,
                state.up.y,
                state.up.z});
        camera->OrthogonalizeUp();
        camera->SetScale(state.scale);
        view_->Redraw();
        return true;
    }

    bool setStandardView(
        viewer::StandardView standard_view) {
        ensureInitialized();

        const auto current =
            cameraState();
        if (!current) return false;

        const auto next =
            viewer::cameraForStandardView(
                *current,
                standard_view);

        return next &&
               setCameraState(*next);
    }

    bool setProjection(
        viewer::CameraProjection projection) {
        ensureInitialized();

        const auto current =
            cameraState();
        if (!current) return false;

        const auto next =
            viewer::cameraWithProjection(
                *current,
                projection);

        return next &&
               setCameraState(*next);
    }

    void fitAll() {
        ensureInitialized();
        if (view_.IsNull()) return;

        view_->FitAll(0.05, false);
        view_->Redraw();
    }

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) {
        if (!validReferenceScene(scene)) {
            return false;
        }

        reference_scene_ = scene;

        if (!view_.IsNull()) {
            rebuildReferenceScene(true);
        }

        return true;
    }

    bool setReferenceGrid(
        const viewer::ReferenceGridPresentation& grid) {
        if (!grid.valid()) return false;

        reference_grid_ = grid;

        if (!view_.IsNull()) {
            applyReferenceGrid(grid, true);
        }

        return true;
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) {
        selection_intent_handler_ =
            std::move(handler);
    }

    void zoomByFactor(double factor) {
        ensureInitialized();

        if (view_.IsNull() ||
            !std::isfinite(factor) ||
            factor <= 0.0) {
            return;
        }

        view_->SetZoom(factor, true);
        view_->Redraw();
    }

    void panByPixels(
        int delta_x,
        int delta_y) {
        ensureInitialized();
        if (view_.IsNull()) return;

        const auto dpr =
            owner_.devicePixelRatioF();

        const auto dx =
            view_->Convert(
                static_cast<int>(
                    std::lround(
                        delta_x * dpr)));

        const auto dy =
            view_->Convert(
                static_cast<int>(
                    std::lround(
                        delta_y * dpr)));

        view_->Panning(
            -dx,
            dy,
            1.0,
            true);
        view_->Redraw();
    }

    void orbitByScreenAngles(
        const detail::OrbitScreenAngles&
            angles) {
        ensureInitialized();

        if (view_.IsNull() ||
            !std::isfinite(angles.x) ||
            !std::isfinite(angles.y) ||
            !std::isfinite(angles.z)) {
            return;
        }

        view_->Rotate(
            angles.x,
            angles.y,
            angles.z,
            true);
        view_->Redraw();
    }

    void orbitByRadians(
        double horizontal,
        double vertical) {
        orbitByScreenAngles(
            {horizontal, vertical, 0.0});
    }

    void resize() {
        if (view_.IsNull()) return;

        const auto native_window =
            view_->Window();
        if (!native_window.IsNull()) {
            native_window->DoResize();
        }

        view_->MustBeResized();
        view_->Redraw();
    }

    void redraw() {
        if (!view_.IsNull()) {
            view_->Redraw();
        }
    }

    void beginMiddleDrag(
        int x,
        int y,
        bool orbit) noexcept {
        middle_dragging_ = true;
        middle_orbit_ = orbit;
        last_mouse_x_ = x;
        last_mouse_y_ = y;
    }

    void updateMiddleDrag(
        int x,
        int y,
        bool orbit) {
        if (!middle_dragging_) return;

        const auto dx =
            x - last_mouse_x_;
        const auto dy =
            y - last_mouse_y_;

        last_mouse_x_ = x;
        last_mouse_y_ = y;
        middle_orbit_ = orbit;

        if (dx == 0 && dy == 0) return;

        if (middle_orbit_) {
            constexpr double
                radians_per_pixel = 0.006;

            orbitByScreenAngles(
                detail::
                    orbitScreenAnglesFromMouseDelta(
                        dx,
                        dy,
                        radians_per_pixel));
        } else {
            panByPixels(-dx, -dy);
        }
    }

    void endMiddleDrag() noexcept {
        middle_dragging_ = false;
    }

    void zoomAtLogicalPoint(
        int logical_x,
        int logical_y,
        int angle_delta_y) {
        ensureInitialized();

        if (view_.IsNull() ||
            angle_delta_y == 0) {
            return;
        }

        const auto dpr =
            owner_.devicePixelRatioF();

        const auto x =
            static_cast<int>(
                std::lround(
                    static_cast<double>(
                        logical_x) *
                    dpr));

        const auto y =
            static_cast<int>(
                std::lround(
                    static_cast<double>(
                        logical_y) *
                    dpr));

        const auto fraction =
            detail::wheelZoomDragFraction(
                angle_delta_y);

        const auto dx =
            static_cast<int>(
                std::lround(
                    std::max(
                        1.0,
                        static_cast<double>(
                            owner_.width()) *
                            dpr) *
                    fraction));

        const auto dy =
            static_cast<int>(
                std::lround(
                    std::max(
                        1.0,
                        static_cast<double>(
                            owner_.height()) *
                            dpr) *
                    fraction));

        view_->StartZoomAtPoint(x, y);
        view_->ZoomAtPoint(
            x,
            y,
            x + dx,
            y + dy);
        view_->Redraw();
    }

    void moveToLogicalPoint(
        int logical_x,
        int logical_y) {
        ensureInitialized();

        if (view_.IsNull() ||
            context_.IsNull()) {
            return;
        }

        const auto dpr =
            owner_.devicePixelRatioF();

        const auto x =
            static_cast<int>(
                std::lround(
                    logical_x * dpr));

        const auto y =
            static_cast<int>(
                std::lround(
                    logical_y * dpr));

        context_->MoveTo(
            x,
            y,
            view_,
            true);
    }

    void emitPickAtLogicalPoint(
        int logical_x,
        int logical_y,
        bool toggle) {
        moveToLogicalPoint(
            logical_x,
            logical_y);

        if (!selection_intent_handler_) {
            return;
        }

        viewer::SelectionIntent intent;
        intent.mode =
            toggle
                ? viewer::SelectionIntentMode::
                      toggle
                : viewer::SelectionIntentMode::
                      replace;

        if (!context_.IsNull() &&
            context_->HasDetected()) {
            const auto detected =
                context_->DetectedInteractive();

            for (const auto& entry :
                 reference_objects_) {
                if (entry.object == detected) {
                    intent.token =
                        entry.token;
                    break;
                }
            }
        }

        selection_intent_handler_(intent);
    }

private:
    struct ReferenceObjectEntry final {
        viewer::PresentationToken token;
        Handle(AIS_InteractiveObject) object;
    };

    void rebuildReferenceScene(
        bool redraw) {
        if (context_.IsNull()) return;

        for (const auto& entry :
             reference_objects_) {
            if (!entry.object.IsNull()) {
                context_->Remove(
                    entry.object,
                    false);
            }
        }

        reference_objects_.clear();

        for (const auto& reference :
             reference_scene_.references) {
            if (!reference.visible) {
                continue;
            }

            const auto object =
                makeReferenceObject(reference);
            if (object.IsNull()) {
                continue;
            }

            context_->Display(
                object,
                false);

            reference_objects_.push_back(
                ReferenceObjectEntry{
                    reference.token,
                    object});
        }

        if (redraw &&
            !view_.IsNull()) {
            view_->Redraw();
        }
    }

    void applyReferenceGrid(
        const viewer::ReferenceGridPresentation&
            grid,
        bool redraw) {
        if (viewer_.IsNull()) return;

        if (!grid.visible) {
            viewer_->DeactivateGrid();
            if (redraw &&
                !view_.IsNull()) {
                view_->Redraw();
            }
            return;
        }

        const auto u =
            viewer::normalized(grid.u_axis);
        const auto v =
            viewer::normalized(grid.v_axis);

        if (!u || !v) return;

        const auto normal =
            viewer::normalized(
                viewer::cross(*u, *v));
        if (!normal) return;

        viewer_->SetPrivilegedPlane(
            gp_Ax3{
                toGpPoint(grid.origin),
                gp_Dir{
                    normal->x,
                    normal->y,
                    normal->z},
                gp_Dir{
                    u->x,
                    u->y,
                    u->z}});

        viewer_->SetRectangularGridValues(
            0.0,
            0.0,
            grid.spacing,
            grid.spacing,
            0.0);

        viewer_->
            SetRectangularGridGraphicValues(
                grid.extent * 2.0,
                grid.extent * 2.0,
                0.0);

        viewer_->ActivateGrid(
            Aspect_GT_Rectangular,
            Aspect_GDM_Lines);

        if (redraw &&
            !view_.IsNull()) {
            view_->Redraw();
        }
    }

    QtOcctViewerWidget& owner_;

    bool middle_dragging_{};
    bool middle_orbit_{};
    int last_mouse_x_{};
    int last_mouse_y_{};

    viewer::ReferenceScene reference_scene_;
    std::optional<
        viewer::ReferenceGridPresentation>
        reference_grid_;
    viewer::SelectionIntentHandler
        selection_intent_handler_;

    std::vector<ReferenceObjectEntry>
        reference_objects_;

    Handle(OpenGl_GraphicDriver) driver_;
    Handle(V3d_Viewer) viewer_;
    Handle(AIS_InteractiveContext) context_;
    Handle(V3d_View) view_;
    Handle(WNT_Window) window_;
};

QtOcctViewerWidget::QtOcctViewerWidget(
    QWidget* parent)
    : QWidget{parent},
      impl_{std::make_unique<Impl>(*this)} {
    setAttribute(
        Qt::WA_NativeWindow,
        true);
    setAttribute(
        Qt::WA_PaintOnScreen,
        true);
    setAttribute(
        Qt::WA_NoSystemBackground,
        true);
    setAutoFillBackground(false);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

QtOcctViewerWidget::~QtOcctViewerWidget() =
    default;

std::optional<viewer::CameraState>
QtOcctViewerWidget::cameraState() const {
    return impl_->cameraState();
}

bool QtOcctViewerWidget::setCameraState(
    const viewer::CameraState& state) {
    return impl_->setCameraState(state);
}

bool QtOcctViewerWidget::setStandardView(
    viewer::StandardView view) {
    return impl_->setStandardView(view);
}

bool QtOcctViewerWidget::setProjection(
    viewer::CameraProjection projection) {
    return impl_->setProjection(projection);
}

void QtOcctViewerWidget::fitAll() {
    impl_->fitAll();
}

bool QtOcctViewerWidget::setReferenceScene(
    const viewer::ReferenceScene& scene) {
    return impl_->setReferenceScene(scene);
}

bool QtOcctViewerWidget::setReferenceGrid(
    const viewer::ReferenceGridPresentation&
        grid) {
    return impl_->setReferenceGrid(grid);
}

void QtOcctViewerWidget::
setSelectionIntentHandler(
    viewer::SelectionIntentHandler handler) {
    impl_->setSelectionIntentHandler(
        std::move(handler));
}

void QtOcctViewerWidget::zoomByFactor(
    double factor) {
    impl_->zoomByFactor(factor);
}

void QtOcctViewerWidget::panByPixels(
    int delta_x,
    int delta_y) {
    impl_->panByPixels(
        delta_x,
        delta_y);
}

void QtOcctViewerWidget::orbitByRadians(
    double horizontal,
    double vertical) {
    impl_->orbitByRadians(
        horizontal,
        vertical);
}

QPaintEngine*
QtOcctViewerWidget::paintEngine() const {
    return nullptr;
}

void QtOcctViewerWidget::paintEvent(
    QPaintEvent* event) {
    Q_UNUSED(event);
    impl_->ensureInitialized();
    impl_->redraw();
}

void QtOcctViewerWidget::resizeEvent(
    QResizeEvent* event) {
    QWidget::resizeEvent(event);
    impl_->resize();
}

void QtOcctViewerWidget::showEvent(
    QShowEvent* event) {
    QWidget::showEvent(event);
    impl_->ensureInitialized();
    impl_->resize();

    QTimer::singleShot(
        0,
        this,
        [this] {
            if (isVisible()) {
                impl_->resize();
            }
        });
}

void QtOcctViewerWidget::mousePressEvent(
    QMouseEvent* event) {
    setFocus(Qt::MouseFocusReason);

    if (event->button() ==
        Qt::MiddleButton) {
        const auto point =
            event->position().toPoint();

        impl_->beginMiddleDrag(
            point.x(),
            point.y(),
            (event->modifiers() &
             Qt::ShiftModifier) != 0);

        event->accept();
        return;
    }

    if (event->button() ==
        Qt::LeftButton) {
        const auto point =
            event->position().toPoint();

        impl_->emitPickAtLogicalPoint(
            point.x(),
            point.y(),
            (event->modifiers() &
             Qt::ControlModifier) != 0);

        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void QtOcctViewerWidget::mouseMoveEvent(
    QMouseEvent* event) {
    if ((event->buttons() &
         Qt::MiddleButton) != 0) {
        const auto point =
            event->position().toPoint();

        impl_->updateMiddleDrag(
            point.x(),
            point.y(),
            (event->modifiers() &
             Qt::ShiftModifier) != 0);

        event->accept();
        return;
    }

    const auto point =
        event->position().toPoint();
    impl_->moveToLogicalPoint(
        point.x(),
        point.y());

    QWidget::mouseMoveEvent(event);
}

void QtOcctViewerWidget::mouseReleaseEvent(
    QMouseEvent* event) {
    if (event->button() ==
        Qt::MiddleButton) {
        impl_->endMiddleDrag();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void QtOcctViewerWidget::
mouseDoubleClickEvent(
    QMouseEvent* event) {
    if (event->button() ==
        Qt::MiddleButton) {
        impl_->endMiddleDrag();
        impl_->fitAll();
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(
        event);
}

void QtOcctViewerWidget::wheelEvent(
    QWheelEvent* event) {
    const auto point =
        event->position().toPoint();

    impl_->zoomAtLogicalPoint(
        point.x(),
        point.y(),
        event->angleDelta().y());

    event->accept();
}

} // namespace simplesolid2::viewer_qt_occt
