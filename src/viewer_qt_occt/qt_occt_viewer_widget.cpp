#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include "navigation_mapping.hpp"

#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Line.hxx>
#include <AIS_Point.hxx>
#include <AIS_Shape.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Graphic3d_Camera.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Failure.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_Window.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include <QContextMenuEvent>
#include <QCursor>
#include <QDebug>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPixmap>
#include <QResizeEvent>
#include <QShowEvent>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <exception>
#include <utility>
#include <vector>

namespace simplesolid2::viewer_qt_occt {
namespace {

void logProviderFailure(
    const char* operation,
    const char* message) noexcept {
    qWarning().noquote()
        << "SS2 Viewer provider failure in"
        << operation
        << ":"
        << (message != nullptr ? message : "<no message>");
}

template <typename Function>
bool guardedBool(
    const char* operation,
    Function&& function) noexcept {
    try {
        return function();
    } catch (const Standard_Failure& failure) {
        logProviderFailure(
            operation,
            failure.GetMessageString());
    } catch (const std::exception& failure) {
        logProviderFailure(
            operation,
            failure.what());
    } catch (...) {
        logProviderFailure(
            operation,
            "<unknown exception>");
    }
    return false;
}

template <typename Function>
void guardedVoid(
    const char* operation,
    Function&& function) noexcept {
    try {
        function();
    } catch (const Standard_Failure& failure) {
        logProviderFailure(
            operation,
            failure.GetMessageString());
    } catch (const std::exception& failure) {
        logProviderFailure(
            operation,
            failure.what());
    } catch (...) {
        logProviderFailure(
            operation,
            "<unknown exception>");
    }
}

template <typename Function>
std::optional<viewer::CameraState> guardedCameraState(
    const char* operation,
    Function&& function) noexcept {
    try {
        return function();
    } catch (const Standard_Failure& failure) {
        logProviderFailure(
            operation,
            failure.GetMessageString());
    } catch (const std::exception& failure) {
        logProviderFailure(
            operation,
            failure.what());
    } catch (...) {
        logProviderFailure(
            operation,
            "<unknown exception>");
    }
    return std::nullopt;
}

} // namespace

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
        context_ = new AIS_InteractiveContext(viewer_);

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

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) {
        if (!scene.valid()) return false;

        ensureInitialized();
        if (context_.IsNull() || view_.IsNull()) return false;

        clearReferenceScene();

        try {
            if (scene.grid && scene.grid->visible) {
                buildGrid(*scene.grid);
            }

            for (const auto& reference : scene.references) {
                if (!reference.visible) continue;

                const auto object =
                    makeReferenceObject(reference);

                if (object.IsNull()) {
                    clearReferenceScene();
                    return false;
                }

                reference_objects_.push_back(
                    ReferenceObject{
                        reference.token,
                        reference.kind,
                        object});
                context_->Display(object, false);

            }

            applySelectionStyles();
            context_->UpdateCurrentViewer();
            view_->Redraw();
            reference_scene_ = scene;
            return true;
        } catch (...) {
            // Leave the provider in a coherent empty-scene state even
            // if OCCT fails after only part of the replacement was built.
            clearReferenceScene();
            throw;
        }
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) {
        if (!scene.valid()) return false;

        ensureInitialized();
        if (context_.IsNull() || view_.IsNull()) {
            return false;
        }

        clearSketchScene();

        try {
            for (const auto& line : scene.lines) {
                Handle(Geom_CartesianPoint) start =
                    new Geom_CartesianPoint(
                        toPoint(line.start));
                Handle(Geom_CartesianPoint) end =
                    new Geom_CartesianPoint(
                        toPoint(line.end));
                Handle(AIS_Line) object =
                    new AIS_Line(start, end);

                sketch_objects_.push_back(
                    SketchObject{
                        line.token,
                        object});
                context_->Display(object, false);
            }

            if (scene.origin) {
                Handle(Geom_CartesianPoint) point =
                    new Geom_CartesianPoint(
                        toPoint(
                            scene.origin->position));
                sketch_origin_object_ =
                    new AIS_Point(point);
                context_->Display(
                    sketch_origin_object_,
                    false);
                context_->Deactivate(
                    sketch_origin_object_);
                context_->SetColor(
                    sketch_origin_object_,
                    Quantity_Color{
                        0.95, 0.78, 0.20,
                        Quantity_TOC_RGB},
                    false);
            }

            sketch_scene_ = scene;
            applySelectionStyles();
            context_->UpdateCurrentViewer();
            view_->Redraw();
            return true;
        } catch (...) {
            clearSketchScene();
            throw;
        }
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) {
        if (!scene.valid()) return false;

        ensureInitialized();
        if (context_.IsNull() || view_.IsNull()) {
            return false;
        }

        clearSketchPreviewScene();

        try {
            for (const auto& line : scene.lines) {
                Handle(Geom_CartesianPoint) start =
                    new Geom_CartesianPoint(
                        toPoint(line.start));
                Handle(Geom_CartesianPoint) end =
                    new Geom_CartesianPoint(
                        toPoint(line.end));
                Handle(AIS_Line) object =
                    new AIS_Line(start, end);

                context_->Display(object, false);
                context_->SetColor(
                    object,
                    Quantity_Color{
                        0.22, 0.82, 0.96,
                        Quantity_TOC_RGB},
                    false);
                context_->SetWidth(
                    object,
                    1.6,
                    false);
                context_->Deactivate(object);
                sketch_preview_objects_.push_back(
                    object);
            }

            sketch_preview_scene_ = scene;
            context_->UpdateCurrentViewer();
            view_->Redraw();
            return true;
        } catch (...) {
            clearSketchPreviewScene();
            throw;
        }
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) {
        if (!selection.valid()) return false;

        selection_ = selection;
        if (!context_.IsNull()) {
            applySelectionStyles();
            context_->UpdateCurrentViewer();
        }
        if (!view_.IsNull()) view_->Redraw();
        return true;
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) {
        selection_intent_handler_ = std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) {
        spatial_pointer_handler_ =
            std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing) {
        if (primary_pointer_routing_ == routing) {
            return;
        }

        primary_pointer_routing_ = routing;

        if (routing ==
                viewer::PrimaryPointerRouting::
                    spatial_tool_input &&
            !context_.IsNull()) {
            // Spatial tool callbacks may synchronously rebuild presentation.
            // Drop stale provider detection before entering that route.
            context_->ClearDetected(false);
            context_->UpdateCurrentViewer();
        }
    }

    [[nodiscard]] viewer::PrimaryPointerRouting
    primaryPointerRouting() const noexcept {
        return primary_pointer_routing_;
    }

    void setCursorMode(
        viewer::ViewportCursorMode mode) {
        cursor_mode_ = mode;

        switch (mode) {
        case viewer::ViewportCursorMode::system_default:
            owner_.unsetCursor();
            return;

        case viewer::ViewportCursorMode::select_pick_box: {
            QPixmap pixmap{17, 17};
            pixmap.fill(Qt::transparent);
            QPainter painter{&pixmap};
            painter.setPen(Qt::white);
            painter.drawRect(5, 5, 6, 6);
            painter.end();
            owner_.setCursor(
                QCursor{pixmap, 8, 8});
            return;
        }

        case viewer::ViewportCursorMode::create_edit_crosshair:
            owner_.setCursor(
                QCursor{Qt::CrossCursor});
            return;
        }
    }

    bool emitSpatialPointer(
        double logical_x,
        double logical_y,
        viewer::SpatialPointerPhase phase) {
        ensureInitialized();
        if (view_.IsNull() ||
            !spatial_pointer_handler_) {
            return false;
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

        double world_x{};
        double world_y{};
        double world_z{};
        double direction_x{};
        double direction_y{};
        double direction_z{};

        view_->ConvertWithProj(
            x,
            y,
            world_x,
            world_y,
            world_z,
            direction_x,
            direction_y,
            direction_z);

        const viewer::SpatialPointerEvent event{
            phase,
            viewer::ViewportPoint2{
                logical_x,
                logical_y},
            viewer::Ray3{
                viewer::Point3{
                    world_x,
                    world_y,
                    world_z},
                viewer::Vec3{
                    direction_x,
                    direction_y,
                    direction_z}}};

        if (!event.valid()) {
            return false;
        }

        spatial_pointer_handler_(event);
        return true;
    }

    void pickAtLogicalPoint(
        int logical_x,
        int logical_y,
        bool toggle) {
        ensureInitialized();
        if (context_.IsNull() ||
            view_.IsNull() ||
            !selection_intent_handler_) {
            return;
        }

        const auto dpr = owner_.devicePixelRatioF();
        const auto x = static_cast<int>(
            std::lround(static_cast<double>(logical_x) * dpr));
        const auto y = static_cast<int>(
            std::lround(static_cast<double>(logical_y) * dpr));

        context_->MoveTo(x, y, view_, true);

        std::optional<viewer::PresentationToken> detected_token;
        if (context_->HasDetected()) {
            const auto detected =
                context_->DetectedInteractive();

            if (!detected.IsNull()) {
                for (const auto& entry : reference_objects_) {
                    if (entry.object == detected) {
                        detected_token = entry.token;
                        break;
                    }
                }

                if (!detected_token) {
                    for (const auto& entry : sketch_objects_) {
                        if (entry.object == detected) {
                            detected_token = entry.token;
                            break;
                        }
                    }
                }
            }
        }

        // MoveTo owns transient detected/highlight state inside OCCT.
        // Clear it before invoking application callbacks because a callback
        // may synchronously refresh or replace the presentation scene.
        context_->ClearDetected(false);

        if (detected_token) {
            selection_intent_handler_(
                viewer::SelectionIntent{
                    *detected_token,
                    toggle
                        ? viewer::SelectionIntentMode::toggle
                        : viewer::SelectionIntentMode::replace});
            return;
        }

        selection_intent_handler_(
            viewer::SelectionIntent{
                {},
                viewer::SelectionIntentMode::clear});
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

    struct ReferenceObject final {
        viewer::PresentationToken token;
        viewer::ReferencePresentationKind kind;
        Handle(AIS_InteractiveObject) object;
    };

    struct SketchObject final {
        viewer::PresentationToken token;
        Handle(AIS_InteractiveObject) object;
    };

    [[nodiscard]] static gp_Pnt toPoint(
        const viewer::Point3& point) {
        return gp_Pnt{point.x, point.y, point.z};
    }

    [[nodiscard]] static gp_Dir toDirection(
        const viewer::Vec3& vector) {
        return gp_Dir{vector.x, vector.y, vector.z};
    }

    [[nodiscard]] static Quantity_Color baseColor(
        viewer::ReferencePresentationKind kind) {
        switch (kind) {
        case viewer::ReferencePresentationKind::x_axis:
            return Quantity_Color{0.86, 0.24, 0.24, Quantity_TOC_RGB};
        case viewer::ReferencePresentationKind::y_axis:
            return Quantity_Color{0.30, 0.78, 0.36, Quantity_TOC_RGB};
        case viewer::ReferencePresentationKind::z_axis:
            return Quantity_Color{0.28, 0.48, 0.92, Quantity_TOC_RGB};
        case viewer::ReferencePresentationKind::plane:
            return Quantity_Color{0.42, 0.58, 0.82, Quantity_TOC_RGB};
        case viewer::ReferencePresentationKind::point:
            return Quantity_Color{0.92, 0.92, 0.92, Quantity_TOC_RGB};
        }
        return Quantity_Color{0.75, 0.75, 0.75, Quantity_TOC_RGB};
    }

    [[nodiscard]] Handle(AIS_InteractiveObject)
    makeReferenceObject(
        const viewer::ReferencePresentation& reference) {
        if (reference.kind ==
            viewer::ReferencePresentationKind::point) {
            Handle(Geom_CartesianPoint) point =
                new Geom_CartesianPoint(toPoint(reference.origin));
            Handle(AIS_Point) object = new AIS_Point(point);
            return object;
        }

        const auto u = viewer::normalized(reference.u_axis);
        if (!u) return {};

        if (reference.kind !=
            viewer::ReferencePresentationKind::plane) {
            const auto offset = *u * reference.extent;
            Handle(Geom_CartesianPoint) start =
                new Geom_CartesianPoint(
                    toPoint(reference.origin - offset));
            Handle(Geom_CartesianPoint) end =
                new Geom_CartesianPoint(
                    toPoint(reference.origin + offset));
            Handle(AIS_Line) object = new AIS_Line(start, end);
            return object;
        }

        const auto v = viewer::normalized(reference.v_axis);
        if (!v) return {};

        const auto normal = viewer::normalized(
            viewer::cross(*u, *v));
        if (!normal) return {};

        const gp_Pln plane{
            toPoint(reference.origin),
            toDirection(*normal)};
        BRepBuilderAPI_MakeFace face{
            plane,
            -reference.extent,
            reference.extent,
            -reference.extent,
            reference.extent};
        if (!face.IsDone()) {
            return {};
        }

        // A finite AIS_Shape avoids the unstable AIS_Plane lifecycle
        // observed during repeated native scene replacement on Windows.
        // The neutral PresentationToken remains the semantic transport.
        Handle(AIS_Shape) object =
            new AIS_Shape(face.Shape());
        return object;
    }

    void clearSketchScene() noexcept {
        if (!context_.IsNull()) {
            guardedVoid(
                "clearSketchDetected",
                [this] {
                    context_->ClearDetected(false);
                });

            for (const auto& entry : sketch_objects_) {
                if (entry.object.IsNull()) continue;
                const auto object = entry.object;
                guardedVoid(
                    "removeSketchObject",
                    [this, object] {
                        context_->Remove(
                            object,
                            false);
                    });
            }

            if (!sketch_origin_object_.IsNull()) {
                const auto object =
                    sketch_origin_object_;
                guardedVoid(
                    "removeSketchOrigin",
                    [this, object] {
                        context_->Remove(
                            object,
                            false);
                    });
            }
        }

        sketch_objects_.clear();
        sketch_origin_object_.Nullify();
        sketch_scene_.lines.clear();
        sketch_scene_.origin.reset();
    }

    void clearSketchPreviewScene() noexcept {
        if (!context_.IsNull()) {
            for (const auto& object :
                 sketch_preview_objects_) {
                if (object.IsNull()) continue;
                const auto retained = object;
                guardedVoid(
                    "removeSketchPreviewObject",
                    [this, retained] {
                        context_->Remove(
                            retained,
                            false);
                    });
            }
        }

        sketch_preview_objects_.clear();
        sketch_preview_scene_.lines.clear();
    }

    void clearReferenceScene() noexcept {
        if (!context_.IsNull()) {
            // Scene objects may still be held by transient OCCT detection
            // or native selection state after user input. Release those
            // references before removing presentation objects.
            guardedVoid(
                "clearDetected",
                [this] {
                    context_->ClearDetected(false);
                });
            guardedVoid(
                "clearSelected",
                [this] {
                    context_->ClearSelected(false);
                });

            for (const auto& entry : reference_objects_) {
                if (entry.object.IsNull()) continue;
                const auto object = entry.object;
                guardedVoid(
                    "removeReferenceObject",
                    [this, object] {
                        context_->Remove(
                            object,
                            false);
                    });
            }

            for (const auto& object : grid_objects_) {
                if (object.IsNull()) continue;
                guardedVoid(
                    "removeGridObject",
                    [this, object] {
                        context_->Remove(
                            object,
                            false);
                    });
            }
        }

        reference_objects_.clear();
        grid_objects_.clear();
        reference_scene_.references.clear();
        reference_scene_.grid.reset();
    }

    void buildGrid(
        const viewer::GridPresentation& grid) {
        const auto u = viewer::normalized(grid.u_axis);
        const auto v = viewer::normalized(grid.v_axis);
        if (!u || !v) return;

        const auto line_count =
            static_cast<int>(
                std::floor(grid.extent / grid.spacing));

        for (int index = -line_count;
             index <= line_count;
             ++index) {
            const auto offset =
                static_cast<double>(index) * grid.spacing;
            const bool major =
                (std::abs(index) %
                 static_cast<int>(grid.major_every)) == 0;

            const auto u_offset = *u * offset;
            const auto v_extent = *v * grid.extent;
            Handle(Geom_CartesianPoint) u_start =
                new Geom_CartesianPoint(
                    toPoint(grid.origin + u_offset - v_extent));
            Handle(Geom_CartesianPoint) u_end =
                new Geom_CartesianPoint(
                    toPoint(grid.origin + u_offset + v_extent));
            Handle(AIS_Line) u_line =
                new AIS_Line(u_start, u_end);
            context_->Display(u_line, false);
            context_->SetColor(
                u_line,
                major
                    ? Quantity_Color{0.42, 0.44, 0.48, Quantity_TOC_RGB}
                    : Quantity_Color{0.27, 0.29, 0.32, Quantity_TOC_RGB},
                false);
            context_->SetWidth(
                u_line,
                major ? 1.4 : 0.6,
                false);
            context_->Deactivate(u_line);
            grid_objects_.push_back(u_line);

            const auto v_offset = *v * offset;
            const auto u_extent = *u * grid.extent;
            Handle(Geom_CartesianPoint) v_start =
                new Geom_CartesianPoint(
                    toPoint(grid.origin + v_offset - u_extent));
            Handle(Geom_CartesianPoint) v_end =
                new Geom_CartesianPoint(
                    toPoint(grid.origin + v_offset + u_extent));
            Handle(AIS_Line) v_line =
                new AIS_Line(v_start, v_end);
            context_->Display(v_line, false);
            context_->SetColor(
                v_line,
                major
                    ? Quantity_Color{0.42, 0.44, 0.48, Quantity_TOC_RGB}
                    : Quantity_Color{0.27, 0.29, 0.32, Quantity_TOC_RGB},
                false);
            context_->SetWidth(
                v_line,
                major ? 1.4 : 0.6,
                false);
            context_->Deactivate(v_line);
            grid_objects_.push_back(v_line);
        }
    }

    [[nodiscard]] bool isSelected(
        viewer::PresentationToken token) const {
        return std::find(
                   selection_.selected.begin(),
                   selection_.selected.end(),
                   token) != selection_.selected.end();
    }

    void applySelectionStyles() {
        if (context_.IsNull()) return;

        for (const auto& entry : reference_objects_) {
            if (entry.object.IsNull()) continue;

            const bool selected = isSelected(entry.token);
            const bool primary =
                selection_.primary &&
                *selection_.primary == entry.token;

            const auto color =
                primary
                    ? Quantity_Color{
                          1.0, 0.90, 0.25, Quantity_TOC_RGB}
                    : selected
                        ? Quantity_Color{
                              1.0, 0.63, 0.18, Quantity_TOC_RGB}
                        : baseColor(entry.kind);

            context_->SetColor(
                entry.object,
                color,
                false);

            context_->SetWidth(
                entry.object,
                primary ? 4.0 : (selected ? 3.0 : 1.8),
                false);

            if (entry.kind ==
                viewer::ReferencePresentationKind::plane) {
                context_->SetTransparency(
                    entry.object,
                    primary ? 0.55 : (selected ? 0.68 : 0.82),
                    false);
            }
        }

        for (const auto& entry : sketch_objects_) {
            if (entry.object.IsNull()) continue;

            const bool selected =
                isSelected(entry.token);
            const bool primary =
                selection_.primary &&
                *selection_.primary == entry.token;

            context_->SetColor(
                entry.object,
                primary
                    ? Quantity_Color{
                          1.0, 0.90, 0.25,
                          Quantity_TOC_RGB}
                    : selected
                        ? Quantity_Color{
                              1.0, 0.63, 0.18,
                              Quantity_TOC_RGB}
                        : Quantity_Color{
                              0.92, 0.92, 0.94,
                              Quantity_TOC_RGB},
                false);
            context_->SetWidth(
                entry.object,
                primary
                    ? 4.0
                    : (selected ? 3.0 : 2.0),
                false);
        }
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

    viewer::ReferenceScene reference_scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene sketch_preview_scene_;
    viewer::PresentationSelection selection_;
    viewer::SelectionIntentHandler selection_intent_handler_;
    viewer::SpatialPointerHandler spatial_pointer_handler_;
    viewer::PrimaryPointerRouting primary_pointer_routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
    std::vector<ReferenceObject> reference_objects_;
    std::vector<SketchObject> sketch_objects_;
    std::vector<Handle(AIS_InteractiveObject)>
        sketch_preview_objects_;
    Handle(AIS_InteractiveObject)
        sketch_origin_object_;
    std::vector<Handle(AIS_InteractiveObject)> grid_objects_;

    Handle(OpenGl_GraphicDriver) driver_;
    Handle(V3d_Viewer) viewer_;
    Handle(AIS_InteractiveContext) context_;
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
    return guardedCameraState(
        "cameraState",
        [this] { return impl_->cameraState(); });
}

bool QtOcctViewerWidget::setCameraState(
    const viewer::CameraState& state) {
    return guardedBool(
        "setCameraState",
        [this, &state] {
            return impl_->setCameraState(state);
        });
}

bool QtOcctViewerWidget::setStandardView(
    viewer::StandardView view) {
    return guardedBool(
        "setStandardView",
        [this, view] {
            return impl_->setStandardView(view);
        });
}

bool QtOcctViewerWidget::setProjection(
    viewer::CameraProjection projection) {
    return guardedBool(
        "setProjection",
        [this, projection] {
            return impl_->setProjection(projection);
        });
}

void QtOcctViewerWidget::fitAll() {
    guardedVoid(
        "fitAll",
        [this] { impl_->fitAll(); });
}

bool QtOcctViewerWidget::setReferenceScene(
    const viewer::ReferenceScene& scene) {
    return guardedBool(
        "setReferenceScene",
        [this, &scene] {
            return impl_->setReferenceScene(scene);
        });
}

bool QtOcctViewerWidget::setSketchScene(
    const viewer::SketchScene& scene) {
    return guardedBool(
        "setSketchScene",
        [this, &scene] {
            return impl_->setSketchScene(scene);
        });
}

bool QtOcctViewerWidget::setSketchPreviewScene(
    const viewer::SketchPreviewScene& scene) {
    return guardedBool(
        "setSketchPreviewScene",
        [this, &scene] {
            return impl_->setSketchPreviewScene(scene);
        });
}

bool QtOcctViewerWidget::setPresentationSelection(
    const viewer::PresentationSelection& selection) {
    return guardedBool(
        "setPresentationSelection",
        [this, &selection] {
            return impl_->setPresentationSelection(selection);
        });
}

void QtOcctViewerWidget::setSelectionIntentHandler(
    viewer::SelectionIntentHandler handler) {
    guardedVoid(
        "setSelectionIntentHandler",
        [this, handler = std::move(handler)]() mutable {
            impl_->setSelectionIntentHandler(
                std::move(handler));
        });
}

void QtOcctViewerWidget::setSpatialPointerHandler(
    viewer::SpatialPointerHandler handler) {
    guardedVoid(
        "setSpatialPointerHandler",
        [this, handler = std::move(handler)]() mutable {
            impl_->setSpatialPointerHandler(
                std::move(handler));
        });
}

void QtOcctViewerWidget::setPrimaryPointerRouting(
    viewer::PrimaryPointerRouting routing) {
    guardedVoid(
        "setPrimaryPointerRouting",
        [this, routing] {
            impl_->setPrimaryPointerRouting(
                routing);
        });
}

void QtOcctViewerWidget::setCursorMode(
    viewer::ViewportCursorMode mode) {
    guardedVoid(
        "setCursorMode",
        [this, mode] {
            impl_->setCursorMode(mode);
        });
}

void QtOcctViewerWidget::zoomByFactor(double factor) {
    guardedVoid(
        "zoomByFactor",
        [this, factor] {
            impl_->zoomByFactor(factor);
        });
}

void QtOcctViewerWidget::panByPixels(
    int delta_x,
    int delta_y) {
    guardedVoid(
        "panByPixels",
        [this, delta_x, delta_y] {
            impl_->panByPixels(delta_x, delta_y);
        });
}

void QtOcctViewerWidget::orbitByRadians(
    double horizontal,
    double vertical) {
    guardedVoid(
        "orbitByRadians",
        [this, horizontal, vertical] {
            impl_->orbitByRadians(horizontal, vertical);
        });
}

QPaintEngine* QtOcctViewerWidget::paintEngine() const {
    return nullptr;
}

void QtOcctViewerWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    guardedVoid(
        "paintEvent",
        [this] {
            impl_->ensureInitialized();
            impl_->redraw();
        });
}

void QtOcctViewerWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    guardedVoid(
        "resizeEvent",
        [this] { impl_->resize(); });
}

void QtOcctViewerWidget::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    guardedVoid(
        "showEvent",
        [this] {
            impl_->ensureInitialized();
            impl_->resize();
        });

    QTimer::singleShot(0, this, [this] {
        if (!isVisible()) return;
        guardedVoid(
            "deferredResize",
            [this] { impl_->resize(); });
    });
}

void QtOcctViewerWidget::mousePressEvent(QMouseEvent* event) {
    setFocus(Qt::MouseFocusReason);

    if (event->button() == Qt::MiddleButton) {
        const auto point = event->position().toPoint();
        guardedVoid(
            "middlePress",
            [this, point, event] {
                impl_->beginMiddleDrag(
                    point.x(),
                    point.y(),
                    (event->modifiers() & Qt::ShiftModifier) != 0);
            });
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton) {
        if (impl_->primaryPointerRouting() ==
            viewer::PrimaryPointerRouting::
                spatial_tool_input) {
            const auto point = event->position();
            guardedVoid(
                "leftSpatialPress",
                [this, point] {
                    static_cast<void>(
                        impl_->emitSpatialPointer(
                            point.x(),
                            point.y(),
                            viewer::SpatialPointerPhase::
                                primary_press));
                });
        } else {
            const auto point =
                event->position().toPoint();
            const bool toggle =
                (event->modifiers() &
                 Qt::ControlModifier) != 0;
            guardedVoid(
                "leftClickPick",
                [this, point, toggle] {
                    impl_->pickAtLogicalPoint(
                        point.x(),
                        point.y(),
                        toggle);
                });
        }
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        // Reserved for a future context menu. WB-01A makes the
        // current behavior an explicit no-op rather than delegating
        // an uncontrolled input path to QWidget.
        event->accept();
        return;
    }

    QWidget::mousePressEvent(event);
}

void QtOcctViewerWidget::mouseMoveEvent(QMouseEvent* event) {
    if ((event->buttons() & Qt::MiddleButton) != 0) {
        const auto point = event->position().toPoint();
        const bool orbit =
            (event->modifiers() & Qt::ShiftModifier) != 0;
        guardedVoid(
            "middleDrag",
            [this, point, orbit] {
                impl_->updateMiddleDrag(
                    point.x(),
                    point.y(),
                    orbit);
            });
        event->accept();
        return;
    }

    const auto point = event->position();
    guardedVoid(
        "spatialMove",
        [this, point] {
            static_cast<void>(
                impl_->emitSpatialPointer(
                    point.x(),
                    point.y(),
                    viewer::SpatialPointerPhase::move));
        });

    QWidget::mouseMoveEvent(event);
}

void QtOcctViewerWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        guardedVoid(
            "middleRelease",
            [this] { impl_->endMiddleDrag(); });
        event->accept();
        return;
    }

    if (event->button() == Qt::LeftButton &&
        impl_->primaryPointerRouting() ==
            viewer::PrimaryPointerRouting::
                spatial_tool_input) {
        const auto point = event->position();
        guardedVoid(
            "leftSpatialRelease",
            [this, point] {
                static_cast<void>(
                    impl_->emitSpatialPointer(
                        point.x(),
                        point.y(),
                        viewer::SpatialPointerPhase::
                            primary_release));
            });
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void QtOcctViewerWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        guardedVoid(
            "middleDoubleClickFit",
            [this] {
                impl_->endMiddleDrag();
                impl_->fitAll();
            });
        event->accept();
        return;
    }

    if (event->button() == Qt::RightButton) {
        event->accept();
        return;
    }

    QWidget::mouseDoubleClickEvent(event);
}

void QtOcctViewerWidget::wheelEvent(QWheelEvent* event) {
    const auto point = event->position().toPoint();
    const auto delta = event->angleDelta().y();
    guardedVoid(
        "wheelZoom",
        [this, point, delta] {
            impl_->zoomAtLogicalPoint(
                point.x(),
                point.y(),
                delta);
        });
    event->accept();
}

void QtOcctViewerWidget::contextMenuEvent(
    QContextMenuEvent* event) {
    // WB-01A reserves right click for a future context menu.
    // Until one exists, keep it an explicit no-op.
    event->accept();
}

} // namespace simplesolid2::viewer_qt_occt
