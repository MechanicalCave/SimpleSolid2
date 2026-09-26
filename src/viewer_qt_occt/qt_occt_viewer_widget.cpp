#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include "navigation_mapping.hpp"

#include <AIS_AnimationCamera.hxx>
#include <AIS_InteractiveContext.hxx>
#include <AIS_InteractiveObject.hxx>
#include <AIS_Line.hxx>
#include <AIS_Point.hxx>
#include <AIS_RubberBand.hxx>
#include <AIS_Shape.hxx>
#include <AIS_TextLabel.hxx>
#include <AIS_ViewCube.hxx>
#include <Aspect_DisplayConnection.hxx>
#include <Aspect_TypeOfLine.hxx>
#include <Aspect_TypeOfTriedronPosition.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <Geom_CartesianPoint.hxx>
#include <Graphic3d_Camera.hxx>
#include <Graphic3d_HorizontalTextAlignment.hxx>
#include <Graphic3d_TransformPers.hxx>
#include <Graphic3d_VerticalTextAlignment.hxx>
#include <Graphic3d_Vec2.hxx>
#include <Graphic3d_ZLayerId.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <Quantity_Color.hxx>
#include <Standard_Failure.hxx>
#include <TCollection_ExtendedString.hxx>
#include <V3d_TypeOfOrientation.hxx>
#include <V3d_View.hxx>
#include <V3d_Viewer.hxx>
#include <WNT_Window.hxx>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <gp_Pnt.hxx>

#include <QColor>
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
#include <QWidget>

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

template <typename Result, typename Function>
Result guardedResult(
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
    return Result{};
}

struct ScreenPoint final {
    double x{};
    double y{};
};

struct ScreenRect final {
    double min_x{};
    double min_y{};
    double max_x{};
    double max_y{};

    [[nodiscard]] bool contains(
        ScreenPoint point) const noexcept {
        return point.x >= min_x &&
               point.x <= max_x &&
               point.y >= min_y &&
               point.y <= max_y;
    }
};

[[nodiscard]] double pointSegmentDistanceSquared(
    ScreenPoint point,
    ScreenPoint start,
    ScreenPoint end) noexcept {
    const double dx = end.x - start.x;
    const double dy = end.y - start.y;
    const double length_squared =
        dx * dx + dy * dy;

    if (length_squared <= 0.0) {
        const double px = point.x - start.x;
        const double py = point.y - start.y;
        return px * px + py * py;
    }

    const double parameter =
        std::clamp(
            ((point.x - start.x) * dx +
             (point.y - start.y) * dy) /
                length_squared,
            0.0,
            1.0);

    const double closest_x =
        start.x + parameter * dx;
    const double closest_y =
        start.y + parameter * dy;
    const double px = point.x - closest_x;
    const double py = point.y - closest_y;
    return px * px + py * py;
}

[[nodiscard]] double cross2(
    ScreenPoint a,
    ScreenPoint b,
    ScreenPoint c) noexcept {
    return (b.x - a.x) * (c.y - a.y) -
           (b.y - a.y) * (c.x - a.x);
}

[[nodiscard]] bool between(
    double value,
    double left,
    double right) noexcept {
    return value >= std::min(left, right) &&
           value <= std::max(left, right);
}

[[nodiscard]] bool segmentIntersectsSegment(
    ScreenPoint a,
    ScreenPoint b,
    ScreenPoint c,
    ScreenPoint d) noexcept {
    const double ab_c = cross2(a, b, c);
    const double ab_d = cross2(a, b, d);
    const double cd_a = cross2(c, d, a);
    const double cd_b = cross2(c, d, b);

    const auto opposite =
        [](double left, double right) noexcept {
            return (left < 0.0 && right > 0.0) ||
                   (left > 0.0 && right < 0.0);
        };

    if (opposite(ab_c, ab_d) &&
        opposite(cd_a, cd_b)) {
        return true;
    }

    const auto on_segment =
        [](ScreenPoint start,
           ScreenPoint end,
           ScreenPoint point,
           double cross_value) noexcept {
            return cross_value == 0.0 &&
                   between(
                       point.x,
                       start.x,
                       end.x) &&
                   between(
                       point.y,
                       start.y,
                       end.y);
        };

    return on_segment(a, b, c, ab_c) ||
           on_segment(a, b, d, ab_d) ||
           on_segment(c, d, a, cd_a) ||
           on_segment(c, d, b, cd_b);
}

[[nodiscard]] bool segmentIntersectsRect(
    ScreenPoint start,
    ScreenPoint end,
    const ScreenRect& rect) noexcept {
    if (rect.contains(start) ||
        rect.contains(end)) {
        return true;
    }

    const ScreenPoint top_left{
        rect.min_x,
        rect.min_y};
    const ScreenPoint top_right{
        rect.max_x,
        rect.min_y};
    const ScreenPoint bottom_right{
        rect.max_x,
        rect.max_y};
    const ScreenPoint bottom_left{
        rect.min_x,
        rect.max_y};

    return
        segmentIntersectsSegment(
            start,
            end,
            top_left,
            top_right) ||
        segmentIntersectsSegment(
            start,
            end,
            top_right,
            bottom_right) ||
        segmentIntersectsSegment(
            start,
            end,
            bottom_right,
            bottom_left) ||
        segmentIntersectsSegment(
            start,
            end,
            bottom_left,
            top_left);
}

[[nodiscard]] std::optional<viewer::NavigationCubeTarget>
navigationCubeTargetForOrientation(
    V3d_TypeOfOrientation orientation) noexcept {
    using Target = viewer::NavigationCubeTarget;

    switch (orientation) {
    case V3d_Yneg: return Target::front;
    case V3d_Ypos: return Target::back;
    case V3d_Xneg: return Target::left;
    case V3d_Xpos: return Target::right;
    case V3d_Zpos: return Target::top;
    case V3d_Zneg: return Target::bottom;

    case V3d_YnegZpos: return Target::top_front;
    case V3d_YposZpos: return Target::top_back;
    case V3d_XnegZpos: return Target::top_left;
    case V3d_XposZpos: return Target::top_right;
    case V3d_YnegZneg: return Target::bottom_front;
    case V3d_YposZneg: return Target::bottom_back;
    case V3d_XnegZneg: return Target::bottom_left;
    case V3d_XposZneg: return Target::bottom_right;
    case V3d_XnegYneg: return Target::front_left;
    case V3d_XposYneg: return Target::front_right;
    case V3d_XnegYpos: return Target::back_left;
    case V3d_XposYpos: return Target::back_right;

    case V3d_XnegYnegZpos:
        return Target::top_front_left;
    case V3d_XposYnegZpos:
        return Target::top_front_right;
    case V3d_XnegYposZpos:
        return Target::top_back_left;
    case V3d_XposYposZpos:
        return Target::top_back_right;
    case V3d_XnegYnegZneg:
        return Target::bottom_front_left;
    case V3d_XposYnegZneg:
        return Target::bottom_front_right;
    case V3d_XnegYposZneg:
        return Target::bottom_back_left;
    case V3d_XposYposZneg:
        return Target::bottom_back_right;
    }

    return std::nullopt;
}



} // namespace

class QtOcctViewerWidget::Impl final {
public:
    explicit Impl(QtOcctViewerWidget& owner)
        : owner_{owner},
          navigation_animation_timer_{
              new QTimer{&owner_}} {
        navigation_animation_timer_->setInterval(16);
        QObject::connect(
            navigation_animation_timer_,
            &QTimer::timeout,
            &owner_,
            [this] {
                updateNavigationAnimation();
            });
    }

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

        navigation_cube_ = new AIS_ViewCube();
        navigation_cube_->SetAutoStartAnimation(false);
        navigation_cube_->SetDrawAxes(false);
        navigation_cube_->SetDrawEdges(true);
        navigation_cube_->SetDrawVertices(true);
        navigation_cube_->SetSize(58.0);
        navigation_cube_->SetBoxFacetExtension(9.0);
        navigation_cube_->SetBoxEdgeMinSize(5.0);
        navigation_cube_->SetBoxCornerMinSize(6.0);
        navigation_cube_->SetFontHeight(12.0);
        navigation_cube_->SetFitSelected(false);
        navigation_cube_->SetResetCamera(true);
        navigation_cube_->SetBoxSideLabel(
            V3d_Yneg,
            "FRONT");
        navigation_cube_->SetBoxSideLabel(
            V3d_Ypos,
            "BACK");
        navigation_cube_->SetBoxSideLabel(
            V3d_Xneg,
            "LEFT");
        navigation_cube_->SetBoxSideLabel(
            V3d_Xpos,
            "RIGHT");
        navigation_cube_->SetBoxSideLabel(
            V3d_Zpos,
            "TOP");
        navigation_cube_->SetBoxSideLabel(
            V3d_Zneg,
            "BOTTOM");
        navigation_cube_->SetTransformPersistence(
            new Graphic3d_TransformPers(
                Graphic3d_TMF_TriedronPers,
                Aspect_TOTP_RIGHT_UPPER,
                Graphic3d_Vec2i{86, 86}));
        context_->Display(
            navigation_cube_,
            false);

        createNavigationControlLabels();
        syncNavigationControlVisibility();

        context_->UpdateCurrentViewer();
        view_->Redraw();
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
        syncNavigationControlVisibility();
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

    struct NavigationControl final {
        Handle(AIS_TextLabel) label;
        viewer::NavigationCubeAction action;
        int offset_x{};
        int offset_y{};
        int half_width{};
        int half_height{};
        bool face_only{};
        bool visible{};
    };

    void createNavigationControlLabels() {
        if (context_.IsNull()) return;

        navigation_controls_.clear();

        auto add =
            [this](
                const char* text,
                viewer::NavigationCubeAction action,
                int offset_x,
                int offset_y,
                int half_width,
                int half_height,
                bool face_only) {
                Handle(AIS_TextLabel) label =
                    new AIS_TextLabel();
                label->SetText(
                    TCollection_ExtendedString{text});
                label->SetPosition(
                    gp_Pnt{0.0, 0.0, 0.0});
                label->SetColor(
                    Quantity_Color{
                        0.94,
                        0.94,
                        0.96,
                        Quantity_TOC_RGB});
                label->SetHeight(12.0);
                label->SetHJustification(
                    Graphic3d_HTA_CENTER);
                label->SetVJustification(
                    Graphic3d_VTA_CENTER);
                label->SetZoomable(false);
                label->SetZLayer(
                    Graphic3d_ZLayerId_Topmost);
                label->SetTransformPersistence(
                    new Graphic3d_TransformPers(
                        Graphic3d_TMF_2d,
                        Aspect_TOTP_RIGHT_UPPER,
                        Graphic3d_Vec2i{
                            offset_x,
                            offset_y}));
                context_->Display(
                    label,
                    false);
                context_->Deactivate(label);

                navigation_controls_.push_back(
                    NavigationControl{
                        label,
                        action,
                        offset_x,
                        offset_y,
                        half_width,
                        half_height,
                        face_only,
                        true});
            };

        add(
            "HOME",
            {viewer::NavigationCubeActionKind::home, {}},
            158,
            18,
            28,
            11,
            false);

        add(
            "ORTHO/PERSP",
            {viewer::NavigationCubeActionKind::
                 toggle_projection,
             {}},
            86,
            178,
            42,
            11,
            false);

        add(
            "<",
            {viewer::NavigationCubeActionKind::
                 adjacent_left,
             {}},
            148,
            86,
            12,
            15,
            true);
        add(
            ">",
            {viewer::NavigationCubeActionKind::
                 adjacent_right,
             {}},
            24,
            86,
            12,
            15,
            true);
        add(
            "^",
            {viewer::NavigationCubeActionKind::
                 adjacent_up,
             {}},
            86,
            22,
            15,
            12,
            true);
        add(
            "v",
            {viewer::NavigationCubeActionKind::
                 adjacent_down,
             {}},
            86,
            150,
            15,
            12,
            true);

        add(
            "CCW90",
            {viewer::NavigationCubeActionKind::
                 roll_counterclockwise,
             {}},
            146,
            46,
            28,
            10,
            true);
        add(
            "CW90",
            {viewer::NavigationCubeActionKind::
                 roll_clockwise,
             {}},
            28,
            46,
            24,
            10,
            true);
    }

    void syncNavigationControlVisibility() {
        if (context_.IsNull()) return;

        const auto camera = cameraState();
        const bool face_aligned =
            camera &&
            viewer::navigationCubeFaceAligned(
                *camera);

        for (auto& control : navigation_controls_) {
            const bool next_visible =
                !control.face_only ||
                face_aligned;
            if (next_visible == control.visible) {
                continue;
            }

            if (next_visible) {
                context_->Display(
                    control.label,
                    false);
                context_->Deactivate(
                    control.label);
            } else {
                context_->Erase(
                    control.label,
                    false);
            }

            control.visible = next_visible;
        }
    }

    [[nodiscard]] std::optional<
        viewer::NavigationCubeAction>
    navigationControlAt(
        int logical_x,
        int logical_y) const {
        for (const auto& control :
             navigation_controls_) {
            if (!control.visible) continue;

            const int center_x =
                owner_.width() -
                control.offset_x;
            const int center_y =
                control.offset_y;

            if (std::abs(
                    logical_x -
                    center_x) <=
                    control.half_width &&
                std::abs(
                    logical_y -
                    center_y) <=
                    control.half_height) {
                return control.action;
            }
        }

        return std::nullopt;
    }

    void emitNavigationCubeAction(
        const viewer::NavigationCubeAction& action) {
        if (navigation_cube_action_handler_) {
            navigation_cube_action_handler_(action);
        }
    }

    void setNavigationCubeActionHandler(
        viewer::NavigationCubeActionHandler handler) {
        navigation_cube_action_handler_ =
            std::move(handler);
    }

    bool animateCameraState(
        const viewer::CameraState& state,
        double duration_seconds,
        bool fit_all) {
        if (!viewer::validateCameraState(state).valid ||
            !std::isfinite(duration_seconds) ||
            duration_seconds < 0.0) {
            return false;
        }

        ensureInitialized();
        if (view_.IsNull()) {
            return false;
        }

        if (!navigation_animation_.IsNull()) {
            navigation_animation_->Stop();
        }
        navigation_animation_timer_->stop();

        if (duration_seconds <= 0.0) {
            if (!setCameraState(state)) {
                return false;
            }
            if (fit_all) {
                fitAll();
            }
            return true;
        }

        const auto current_camera =
            view_->Camera();
        if (current_camera.IsNull()) {
            return false;
        }

        Handle(Graphic3d_Camera) start_camera =
            new Graphic3d_Camera(current_camera);
        Handle(Graphic3d_Camera) end_camera =
            new Graphic3d_Camera(current_camera);

        end_camera->SetProjectionType(
            state.projection ==
                    viewer::CameraProjection::orthographic
                ? Graphic3d_Camera::
                      Projection_Orthographic
                : Graphic3d_Camera::
                      Projection_Perspective);
        end_camera->SetEyeAndCenter(
            gp_Pnt{
                state.eye.x,
                state.eye.y,
                state.eye.z},
            gp_Pnt{
                state.target.x,
                state.target.y,
                state.target.z});
        end_camera->SetUp(
            gp_Dir{
                state.up.x,
                state.up.y,
                state.up.z});
        end_camera->OrthogonalizeUp();
        end_camera->SetScale(state.scale);

        navigation_animation_ =
            new AIS_AnimationCamera(
                "SS2.NavigationCube",
                view_);
        navigation_animation_->SetCameraStart(
            start_camera);
        navigation_animation_->SetCameraEnd(
            end_camera);
        navigation_animation_->SetOwnDuration(
            duration_seconds);
        navigation_animation_target_ = state;
        navigation_animation_fit_all_ = fit_all;

        for (auto& control : navigation_controls_) {
            if (control.face_only &&
                control.visible) {
                context_->Erase(
                    control.label,
                    false);
                control.visible = false;
            }
        }

        navigation_animation_->StartTimer(
            0.0,
            1.0,
            true);
        navigation_animation_timer_->start();
        return true;
    }

    void updateNavigationAnimation() {
        if (navigation_animation_.IsNull() ||
            view_.IsNull()) {
            navigation_animation_timer_->stop();
            return;
        }

        static_cast<void>(
            navigation_animation_->UpdateTimer());
        view_->Redraw();

        if (!navigation_animation_->IsStopped()) {
            return;
        }

        navigation_animation_timer_->stop();

        if (navigation_animation_target_) {
            static_cast<void>(
                setCameraState(
                    *navigation_animation_target_));
        }

        if (navigation_animation_fit_all_) {
            view_->FitAll(0.05, false);
            view_->Redraw();
        }

        syncNavigationControlVisibility();

        navigation_animation_.Nullify();
        navigation_animation_target_.reset();
        navigation_animation_fit_all_ = false;
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

        clearSketchGripScene();
        sketch_interaction_presentation_ = {};
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

    bool setSketchGripScene(
        const viewer::SketchGripScene& scene) {
        if (!scene.valid()) return false;

        ensureInitialized();
        if (context_.IsNull() || view_.IsNull()) {
            return false;
        }

        clearSketchGripScene();

        try {
            for (const auto& grip : scene.grips) {
                Handle(Geom_CartesianPoint) point =
                    new Geom_CartesianPoint(
                        toPoint(grip.position));
                Handle(AIS_Point) object =
                    new AIS_Point(point);

                context_->Display(object, false);
                context_->Deactivate(object);
                sketch_grip_objects_.push_back(
                    SketchGripObject{
                        grip.key,
                        object});
            }

            sketch_grip_scene_ = scene;
            applySketchInteractionStyles();
            context_->UpdateCurrentViewer();
            view_->Redraw();
            return true;
        } catch (...) {
            clearSketchGripScene();
            throw;
        }
    }

    bool setSketchInteractionPresentation(
        const viewer::SketchInteractionPresentation& presentation) {
        if (!presentation.valid()) return false;

        sketch_interaction_presentation_ =
            presentation;
        if (!context_.IsNull()) {
            applySelectionStyles();
            applySketchInteractionStyles();
            context_->UpdateCurrentViewer();
        }
        if (!view_.IsNull()) {
            view_->Redraw();
        }
        return true;
    }

    viewer::SketchGripQueryResult querySketchGrip(
        viewer::ViewportPoint2 point) {
        if (!point.valid()) {
            return {};
        }

        ensureInitialized();
        if (view_.IsNull()) {
            return {};
        }

        const double dpr =
            owner_.devicePixelRatioF();
        if (!std::isfinite(dpr) ||
            dpr <= 0.0) {
            return {};
        }

        const ScreenPoint query{
            point.x * dpr,
            point.y * dpr};
        const double tolerance =
            9.0 * dpr;
        double best_distance_squared =
            tolerance * tolerance;
        std::optional<viewer::SketchGripKey>
            best;

        for (const auto& grip :
             sketch_grip_scene_.grips) {
            const auto projected =
                projectToScreen(grip.position);
            if (!projected) {
                return {};
            }

            const double dx =
                query.x - projected->x;
            const double dy =
                query.y - projected->y;
            const double distance_squared =
                dx * dx + dy * dy;
            if (distance_squared >
                best_distance_squared) {
                continue;
            }

            const bool strictly_better =
                !best ||
                distance_squared <
                    best_distance_squared;
            const bool deterministic_tie =
                best &&
                distance_squared ==
                    best_distance_squared &&
                (grip.key.owner.value <
                     best->owner.value ||
                 (grip.key.owner.value ==
                      best->owner.value &&
                  static_cast<std::uint8_t>(
                      grip.key.role) <
                  static_cast<std::uint8_t>(
                      best->role)));

            if (strictly_better ||
                deterministic_tie) {
                best_distance_squared =
                    distance_squared;
                best = grip.key;
            }
        }

        return viewer::SketchGripQueryResult{
            true,
            best};
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

    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) {
        if (!point.valid()) {
            return {};
        }

        ensureInitialized();
        if (view_.IsNull()) {
            return {};
        }

        const double dpr =
            owner_.devicePixelRatioF();
        if (!std::isfinite(dpr) ||
            dpr <= 0.0) {
            return {};
        }

        const ScreenPoint query{
            point.x * dpr,
            point.y * dpr};
        const double tolerance =
            6.0 * dpr;
        double best_distance_squared =
            tolerance * tolerance;
        std::optional<viewer::PresentationToken>
            best;

        for (const auto& line :
             sketch_scene_.lines) {
            const auto start =
                projectToScreen(line.start);
            const auto end =
                projectToScreen(line.end);
            if (!start || !end) {
                return {};
            }

            const double distance_squared =
                pointSegmentDistanceSquared(
                    query,
                    *start,
                    *end);
            if (distance_squared <=
                    best_distance_squared &&
                (!best ||
                 distance_squared <
                     best_distance_squared)) {
                best_distance_squared =
                    distance_squared;
                best = line.token;
            }
        }

        return viewer::SketchPointQueryResult{
            true,
            best};
    }

    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule rule) {
        if (!rectangle.valid()) {
            return {};
        }

        ensureInitialized();
        if (view_.IsNull()) {
            return {};
        }

        const double dpr =
            owner_.devicePixelRatioF();
        if (!std::isfinite(dpr) ||
            dpr <= 0.0) {
            return {};
        }

        const ScreenRect screen_rect{
            rectangle.minimum.x * dpr,
            rectangle.minimum.y * dpr,
            rectangle.maximum.x * dpr,
            rectangle.maximum.y * dpr};

        viewer::SketchRectangleQueryResult result;
        result.completed = true;
        result.tokens.reserve(
            sketch_scene_.lines.size());

        for (const auto& line :
             sketch_scene_.lines) {
            const auto start =
                projectToScreen(line.start);
            const auto end =
                projectToScreen(line.end);
            if (!start || !end) {
                return {};
            }

            const bool hit =
                rule ==
                        viewer::SketchRectangleSelectionRule::
                            window
                    ? screen_rect.contains(*start) &&
                          screen_rect.contains(*end)
                    : segmentIntersectsRect(
                          *start,
                          *end,
                          screen_rect);

            if (hit) {
                result.tokens.push_back(
                    line.token);
            }
        }

        return result;
    }

    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) {
        if (!overlay.valid()) {
            return false;
        }

        ensureInitialized();
        if (context_.IsNull() || view_.IsNull()) {
            return false;
        }

        const auto dpr = owner_.devicePixelRatioF();
        const auto min_x = static_cast<Standard_Integer>(
            std::lround(
                std::min(overlay.anchor.x, overlay.current.x) *
                dpr));
        const auto max_x = static_cast<Standard_Integer>(
            std::lround(
                std::max(overlay.anchor.x, overlay.current.x) *
                dpr));

        // Qt logical pointer coordinates use a top-left origin while
        // AIS_RubberBand's 2D overlay uses bottom-left screen Y.
        const auto viewport_height =
            static_cast<Standard_Integer>(
                std::lround(
                    static_cast<double>(owner_.height()) *
                    dpr));
        const auto anchor_y =
            viewport_height -
            static_cast<Standard_Integer>(
                std::lround(overlay.anchor.y * dpr));
        const auto current_y =
            viewport_height -
            static_cast<Standard_Integer>(
                std::lround(overlay.current.y * dpr));
        const auto min_y = std::min(anchor_y, current_y);
        const auto max_y = std::max(anchor_y, current_y);

        const bool crossing =
            overlay.rule ==
            viewer::SketchRectangleSelectionRule::crossing;

        const Quantity_Color border{
            crossing ? 0.35 : 0.35,
            crossing ? 0.82 : 0.57,
            crossing ? 0.51 : 0.96,
            Quantity_TOC_RGB};
        const Quantity_Color fill = border;
        const auto line_type =
            crossing ? Aspect_TOL_DASH : Aspect_TOL_SOLID;

        if (selection_rubber_band_.IsNull()) {
            selection_rubber_band_ =
                new AIS_RubberBand(
                    border,
                    line_type,
                    fill,
                    0.86,
                    1.0);
        } else {
            selection_rubber_band_->SetLineColor(border);
            selection_rubber_band_->SetLineType(line_type);
            selection_rubber_band_->SetFillColor(fill);
            selection_rubber_band_->SetFillTransparency(0.86);
        }

        selection_rubber_band_->SetRectangle(
            min_x,
            min_y,
            max_x,
            max_y);

        if (!selection_rubber_band_visible_) {
            context_->Display(
                selection_rubber_band_,
                false);
            selection_rubber_band_visible_ = true;
        } else {
            context_->Redisplay(
                selection_rubber_band_,
                false);
        }

        context_->UpdateCurrentViewer();
        return true;
    }

    void clearSketchSelectionBoxOverlay() {
        if (context_.IsNull() ||
            selection_rubber_band_.IsNull() ||
            !selection_rubber_band_visible_) {
            return;
        }

        context_->Remove(
            selection_rubber_band_,
            false);
        selection_rubber_band_visible_ = false;
        context_->UpdateCurrentViewer();
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
        viewer::SpatialPointerPhase phase,
        bool control) {
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
                    direction_z}},
            viewer::SpatialPointerModifiers{
                control}};

        if (!event.valid()) {
            return false;
        }

        spatial_pointer_handler_(event);
        return true;
    }

    bool updateNavigationCubeHover(
        int logical_x,
        int logical_y) {
        ensureInitialized();
        if (context_.IsNull() ||
            view_.IsNull() ||
            navigation_cube_.IsNull()) {
            return false;
        }

        if (navigationControlAt(
                logical_x,
                logical_y)) {
            context_->ClearDetected(false);
            return true;
        }

        const auto dpr =
            owner_.devicePixelRatioF();
        const auto x =
            static_cast<int>(
                std::lround(
                    static_cast<double>(logical_x) *
                    dpr));
        const auto y =
            static_cast<int>(
                std::lround(
                    static_cast<double>(logical_y) *
                    dpr));

        context_->MoveTo(
            x,
            y,
            view_,
            true);

        if (!context_->HasDetected()) {
            return false;
        }

        const auto cube_owner =
            Handle(AIS_ViewCubeOwner)::DownCast(
                context_->DetectedOwner());
        return !cube_owner.IsNull();
    }

    bool activateNavigationCubeAt(
        int logical_x,
        int logical_y) {
        ensureInitialized();

        if (const auto control =
                navigationControlAt(
                    logical_x,
                    logical_y)) {
            navigation_cube_press_active_ = true;
            context_->ClearDetected(false);
            emitNavigationCubeAction(*control);
            return true;
        }

        if (!updateNavigationCubeHover(
                logical_x,
                logical_y)) {
            return false;
        }

        navigation_cube_press_active_ = true;

        const auto cube_owner =
            Handle(AIS_ViewCubeOwner)::DownCast(
                context_->DetectedOwner());
        if (cube_owner.IsNull()) {
            return false;
        }

        const auto target =
            navigationCubeTargetForOrientation(
                cube_owner->MainOrientation());
        if (!target) {
            return true;
        }

        context_->ClearDetected(false);

        emitNavigationCubeAction(
            viewer::NavigationCubeAction::
                orientTo(*target));

        return true;
    }

    bool consumeNavigationCubeRelease() noexcept {
        const bool active =
            navigation_cube_press_active_;
        navigation_cube_press_active_ = false;
        return active;
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
        syncNavigationControlVisibility();
    }

    void orbitByRadians(double horizontal, double vertical) {
        orbitByScreenAngles({horizontal, vertical, 0.0});
    }

    [[nodiscard]] std::optional<ScreenPoint>
    projectToScreen(
        const viewer::Point3& point) const {
        if (view_.IsNull() ||
            !viewer::finite(point)) {
            return std::nullopt;
        }

        int pixel_x{};
        int pixel_y{};
        view_->Convert(
            point.x,
            point.y,
            point.z,
            pixel_x,
            pixel_y);

        return ScreenPoint{
            static_cast<double>(pixel_x),
            static_cast<double>(pixel_y)};
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

    struct SketchGripObject final {
        viewer::SketchGripKey key;
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

    void clearSketchGripScene() noexcept {
        if (!context_.IsNull()) {
            for (const auto& entry :
                 sketch_grip_objects_) {
                if (entry.object.IsNull()) continue;
                const auto retained = entry.object;
                guardedVoid(
                    "removeSketchGripObject",
                    [this, retained] {
                        context_->Remove(
                            retained,
                            false);
                    });
            }
        }

        sketch_grip_objects_.clear();
        sketch_grip_scene_.grips.clear();
        sketch_interaction_presentation_ = {};
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
            const bool hovered =
                sketch_interaction_presentation_.
                    hovered_entity &&
                *sketch_interaction_presentation_.
                    hovered_entity == entry.token;

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
                        : hovered
                            ? Quantity_Color{
                                  0.22, 0.82, 0.96,
                                  Quantity_TOC_RGB}
                            : Quantity_Color{
                                  0.92, 0.92, 0.94,
                                  Quantity_TOC_RGB},
                false);
            context_->SetWidth(
                entry.object,
                primary
                    ? 4.0
                    : (selected
                           ? 3.0
                           : (hovered ? 3.0 : 2.0)),
                false);
        }
    }

    void applySketchInteractionStyles() {
        if (context_.IsNull()) return;

        for (const auto& entry :
             sketch_grip_objects_) {
            if (entry.object.IsNull()) continue;

            const bool active =
                sketch_interaction_presentation_.
                    active_grip &&
                *sketch_interaction_presentation_.
                    active_grip == entry.key;
            const bool hovered =
                sketch_interaction_presentation_.
                    hovered_grip &&
                *sketch_interaction_presentation_.
                    hovered_grip == entry.key;

            context_->SetColor(
                entry.object,
                active
                    ? Quantity_Color{
                          1.0, 0.90, 0.25,
                          Quantity_TOC_RGB}
                    : hovered
                        ? Quantity_Color{
                              0.22, 0.82, 0.96,
                              Quantity_TOC_RGB}
                        : Quantity_Color{
                              1.0, 0.63, 0.18,
                              Quantity_TOC_RGB},
                false);
            context_->SetWidth(
                entry.object,
                active ? 5.0 : (hovered ? 4.0 : 3.0),
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
    viewer::SketchGripScene sketch_grip_scene_;
    viewer::SketchInteractionPresentation
        sketch_interaction_presentation_;
    viewer::PresentationSelection selection_;
    viewer::SelectionIntentHandler selection_intent_handler_;
    viewer::SpatialPointerHandler spatial_pointer_handler_;
    viewer::NavigationCubeActionHandler
        navigation_cube_action_handler_;
    viewer::PrimaryPointerRouting primary_pointer_routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
    Handle(AIS_RubberBand)
        selection_rubber_band_;
    bool selection_rubber_band_visible_{};

    Handle(AIS_ViewCube) navigation_cube_;
    Handle(AIS_AnimationCamera)
        navigation_animation_;
    QTimer* navigation_animation_timer_{};
    std::optional<viewer::CameraState>
        navigation_animation_target_;
    bool navigation_animation_fit_all_{};
    bool navigation_cube_press_active_{};
    std::vector<NavigationControl>
        navigation_controls_;
    std::vector<ReferenceObject> reference_objects_;
    std::vector<SketchObject> sketch_objects_;
    std::vector<SketchGripObject>
        sketch_grip_objects_;
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

void QtOcctViewerWidget::setNavigationCubeActionHandler(
    viewer::NavigationCubeActionHandler handler) {
    guardedVoid(
        "setNavigationCubeActionHandler",
        [this, handler = std::move(handler)]() mutable {
            impl_->setNavigationCubeActionHandler(
                std::move(handler));
        });
}

bool QtOcctViewerWidget::animateCameraState(
    const viewer::CameraState& state,
    double duration_seconds,
    bool fit_all) {
    return guardedBool(
        "animateCameraState",
        [this, &state, duration_seconds, fit_all] {
            return impl_->animateCameraState(
                state,
                duration_seconds,
                fit_all);
        });
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

bool QtOcctViewerWidget::setSketchGripScene(
    const viewer::SketchGripScene& scene) {
    return guardedBool(
        "setSketchGripScene",
        [this, &scene] {
            return impl_->setSketchGripScene(scene);
        });
}

bool QtOcctViewerWidget::setSketchInteractionPresentation(
    const viewer::SketchInteractionPresentation& presentation) {
    return guardedBool(
        "setSketchInteractionPresentation",
        [this, &presentation] {
            return impl_->setSketchInteractionPresentation(
                presentation);
        });
}

viewer::SketchGripQueryResult
QtOcctViewerWidget::querySketchGrip(
    viewer::ViewportPoint2 point) {
    return guardedResult<
        viewer::SketchGripQueryResult>(
        "querySketchGrip",
        [this, point] {
            return impl_->querySketchGrip(point);
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

viewer::SketchPointQueryResult
QtOcctViewerWidget::querySketchPresentation(
    viewer::ViewportPoint2 point) {
    return guardedResult<
        viewer::SketchPointQueryResult>(
        "querySketchPresentation",
        [this, point] {
            return impl_->querySketchPresentation(
                point);
        });
}

viewer::SketchRectangleQueryResult
QtOcctViewerWidget::querySketchPresentations(
    const viewer::ViewportRect2& rectangle,
    viewer::SketchRectangleSelectionRule rule) {
    return guardedResult<
        viewer::SketchRectangleQueryResult>(
        "querySketchPresentations",
        [this, &rectangle, rule] {
            return impl_->querySketchPresentations(
                rectangle,
                rule);
        });
}

bool QtOcctViewerWidget::setSketchSelectionBoxOverlay(
    const viewer::SketchSelectionBoxOverlay& overlay) {
    return guardedBool(
        "setSketchSelectionBoxOverlay",
        [this, &overlay] {
            return impl_->setSketchSelectionBoxOverlay(
                overlay);
        });
}

void QtOcctViewerWidget::clearSketchSelectionBoxOverlay() {
    guardedVoid(
        "clearSketchSelectionBoxOverlay",
        [this] {
            impl_->clearSketchSelectionBoxOverlay();
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
        const auto point =
            event->position().toPoint();
        if (guardedBool(
                "navigationCubePress",
                [this, point] {
                    return impl_->
                        activateNavigationCubeAt(
                            point.x(),
                            point.y());
                })) {
            event->accept();
            return;
        }

        if (impl_->primaryPointerRouting() ==
            viewer::PrimaryPointerRouting::
                spatial_tool_input) {
            const auto point = event->position();
            guardedVoid(
                "leftSpatialPress",
                [this, point, event] {
                    static_cast<void>(
                        impl_->emitSpatialPointer(
                            point.x(),
                            point.y(),
                            viewer::SpatialPointerPhase::
                                primary_press,
                            (event->modifiers() &
                             Qt::ControlModifier) != 0));
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
    const auto point_int = point.toPoint();
    if (guardedBool(
            "navigationCubeHover",
            [this, point_int] {
                return impl_->
                    updateNavigationCubeHover(
                        point_int.x(),
                        point_int.y());
            })) {
        event->accept();
        return;
    }

    guardedVoid(
        "spatialMove",
        [this, point, event] {
            static_cast<void>(
                impl_->emitSpatialPointer(
                    point.x(),
                    point.y(),
                    viewer::SpatialPointerPhase::move,
                    (event->modifiers() &
                     Qt::ControlModifier) != 0));
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
        impl_->consumeNavigationCubeRelease()) {
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
            [this, point, event] {
                static_cast<void>(
                    impl_->emitSpatialPointer(
                        point.x(),
                        point.y(),
                        viewer::SpatialPointerPhase::
                            primary_release,
                        (event->modifiers() &
                         Qt::ControlModifier) != 0));
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
