#include "sketch_viewport_mapping.hpp"
#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QMouseEvent>
#include <QTest>
#include <QTreeWidget>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

bool near(
    double left,
    double right,
    double tolerance) {
    return std::abs(left - right) <= tolerance;
}

double rasterWorldTolerance(
    const QWidget& widget,
    const viewer::CameraState& camera) {
    const auto physical_height =
        std::max(
            1.0,
            static_cast<double>(widget.height()) *
                widget.devicePixelRatioF());

    // Camera scale is the orthographic vertical world extent. Allow a few
    // physical pixels for integer raster quantization and Qt/OCCT rounding,
    // but remain tight enough to catch a wrong DPR or coordinate transform.
    return 3.0 * camera.scale / physical_height;
}

int fail(std::string_view stage) {
    std::cerr
        << "SK-03A native input failure: "
        << stage << '\n';
    return EXIT_FAILURE;
}

void sendMouseEvent(
    QWidget& widget,
    QEvent::Type type,
    const QPoint& local,
    Qt::MouseButton button,
    Qt::MouseButtons buttons,
    Qt::KeyboardModifiers modifiers =
        Qt::NoModifier) {
    const QPoint global =
        widget.mapToGlobal(local);
    QMouseEvent event{
        type,
        QPointF{local},
        QPointF{global},
        button,
        buttons,
        modifiers};
    QApplication::sendEvent(
        &widget,
        &event);
}

void sendMouseMove(
    QWidget& widget,
    const QPoint& local) {
    sendMouseEvent(
        widget,
        QEvent::MouseMove,
        local,
        Qt::NoButton,
        Qt::NoButton);
}

void sendLeftClick(
    QWidget& widget,
    const QPoint& local) {
    sendMouseEvent(
        widget,
        QEvent::MouseButtonPress,
        local,
        Qt::LeftButton,
        Qt::LeftButton);
    sendMouseEvent(
        widget,
        QEvent::MouseButtonRelease,
        local,
        Qt::LeftButton,
        Qt::NoButton);
}

const viewer::SpatialPointerEvent* findLastMoveAt(
    const std::vector<viewer::SpatialPointerEvent>& events,
    const QPoint& point) {
    for (auto it = events.rbegin();
         it != events.rend();
         ++it) {
        if (it->phase ==
                viewer::SpatialPointerPhase::move &&
            it->position.x ==
                static_cast<double>(point.x()) &&
            it->position.y ==
                static_cast<double>(point.y())) {
            return &*it;
        }
    }

    return nullptr;
}

std::optional<sketch::Point2>
centerUvFromCamera(
    const part::SketchPlacement& placement,
    const viewer::CameraState& camera) {
    return ui::detail::sketchPointFromRay(
        placement,
        viewer::Ray3{
            camera.eye,
            camera.target - camera.eye});
}

int failMapping(
    std::string_view stage,
    const QWidget& widget,
    const viewer::SpatialPointerEvent& event,
    const viewer::CameraState& camera,
    const std::optional<sketch::Point2>& expected,
    const std::optional<sketch::Point2>& actual,
    double tolerance) {
    std::cerr
        << "SK-03A native input mapping failure: "
        << stage
        << "\n  widget=" << widget.width() << 'x' << widget.height()
        << " dpr=" << widget.devicePixelRatioF()
        << " tolerance=" << tolerance
        << "\n  logical=(" << event.position.x << ',' << event.position.y << ')'
        << "\n  ray.origin=(" << event.ray.origin.x << ','
        << event.ray.origin.y << ',' << event.ray.origin.z << ')'
        << "\n  ray.direction=(" << event.ray.direction.x << ','
        << event.ray.direction.y << ',' << event.ray.direction.z << ')'
        << "\n  camera.eye=(" << camera.eye.x << ','
        << camera.eye.y << ',' << camera.eye.z << ')'
        << "\n  camera.target=(" << camera.target.x << ','
        << camera.target.y << ',' << camera.target.z << ')'
        << "\n  camera.scale=" << camera.scale;

    if (expected) {
        std::cerr
            << "\n  expected_uv=("
            << expected->u << ',' << expected->v << ')';
    } else {
        std::cerr << "\n  expected_uv=<none>";
    }

    if (actual) {
        std::cerr
            << "\n  actual_uv=("
            << actual->u << ',' << actual->v << ')';
    } else {
        std::cerr << "\n  actual_uv=<none>";
    }

    std::cerr << '\n';
    return EXIT_FAILURE;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    viewer_qt_occt::QtOcctViewerWidget widget;
    // Use odd dimensions so one integer pixel is the exact viewport center.
    widget.resize(901, 641);
    widget.show();

    if (!QTest::qWaitForWindowExposed(
            &widget,
            3000)) {
        return fail("window exposure");
    }

    viewer::SketchScene scene;
    scene.origin =
        viewer::SketchOriginPresentation{
            viewer::Point3{0.0, 0.0, 0.0}};
    scene.lines.push_back(
        viewer::SketchLinePresentation{
            viewer::PresentationToken{0x10000U},
            viewer::Point3{-10.0, 0.0, 0.0},
            viewer::Point3{10.0, 0.0, 0.0}});
    if (!widget.setSketchScene(scene)) {
        return fail("authored Sketch scene");
    }

    const viewer::CameraState sketch_camera{
        viewer::Point3{0.0, 0.0, 100.0},
        viewer::Point3{0.0, 0.0, 0.0},
        viewer::Vec3{0.0, 1.0, 0.0},
        viewer::CameraProjection::orthographic,
        100.0};
    if (!widget.setCameraState(sketch_camera)) {
        return fail("known Sketch camera");
    }
    QApplication::processEvents();

    // E0 native OCCT consumer regression: a re-evaluated solved stage
    // replaces native AIS_Line geometry, not only a neutral scene. The
    // semantic pick must follow the newly published world coordinates
    // and never retain an authored seed or a previously solved AIS.
    // This is a genuine QtOcctViewerWidget test with a scripted stage
    // scene, NOT a claim that original PlaneGCS is linked in this lane.
    constexpr viewer::PresentationToken stage_line_token{0x10100U};
    const auto putStageLine =
        [&widget, stage_line_token](viewer::Point3 start, viewer::Point3 end) {
            viewer::SketchScene staged;
            staged.lines.push_back(
                viewer::SketchLinePresentation{
                    stage_line_token, start, end});
            return widget.setSketchScene(staged);
        };
    const auto project =
        [&widget](viewer::Point3 point)
            -> std::optional<viewer::ViewportPoint2> {
            return widget.projectWorldPoint(point);
        };
    const auto picksToken =
        [&widget, stage_line_token](viewer::ViewportPoint2 pixel) {
            const auto hit = widget.querySketchPresentation(pixel);
            return hit.completed && hit.token &&
                *hit.token == stage_line_token;
        };
    const auto picksNothing =
        [&widget](viewer::ViewportPoint2 pixel) {
            const auto hit = widget.querySketchPresentation(pixel);
            return hit.completed && !hit.token;
        };
    if (!putStageLine(
            viewer::Point3{-10.0, 1.0, 0.0},
            viewer::Point3{10.0, 3.0, 0.0})) {
        return fail("native E0 authored seed scene");
    }
    const auto skew_point =
        project(viewer::Point3{0.0, 2.0, 0.0});
    const auto horizontal_point =
        project(viewer::Point3{0.0, 0.0, 0.0});
    if (!skew_point || !horizontal_point ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
        !picksToken(*skew_point)) {
        return fail("native E0 authored AIS/pick");
    }

    // Use a solved Horizontal stage at y=0: the former skew midpoint
    // is now >6 physical pixels off the native solved segment.
    if (!putStageLine(
            viewer::Point3{-10.0, 0.0, 0.0},
            viewer::Point3{10.0, 0.0, 0.0}) ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
        !picksToken(*horizontal_point) ||
        !picksNothing(*skew_point)) {
        return fail("native E0 solved AIS geometry/pick");
    }

    // A refused or revoked solver publishes NO current geometry. Both
    // the OCCT AIS object count and hit query must become empty; a new
    // valid stage then reacquires exactly one native selectable object.
    if (!widget.setSketchScene(viewer::SketchScene{}) ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
        !picksNothing(*horizontal_point) ||
        !picksNothing(*skew_point)) {
        return fail("native E0 fail-closed AIS/pick");
    }
    if (!putStageLine(
            viewer::Point3{-10.0, -1.0, 0.0},
            viewer::Point3{10.0, -1.0, 0.0})) {
        return fail("native E0 scene recovery");
    }
    const auto recovered_point =
        project(viewer::Point3{0.0, -1.0, 0.0});
    if (!recovered_point ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
        !picksToken(*recovered_point) ||
        !picksNothing(*skew_point)) {
        return fail("native E0 recovered AIS/pick");
    }
    // E0 placement regression: real 3D OCCT AIS presentation and
    // semantic picking must honor the Part-defined Sketch local frame,
    // not silently treat effective local U/V as global X/Y. Use the
    // native built-in XZ support, an oblique camera and the SAME token.
    const auto xz_support = part::partSketchSupportForBuiltinPlane(
        core::BuiltinReferenceRole::xz_plane);
    const auto xz = xz_support
        ? part::sketchPlacementForSupport(*xz_support)
        : std::nullopt;
    if (!xz) return fail("native E0 XZ placement");
    const auto toWorld = [&xz](double u, double v) {
        return viewer::Point3{
            xz->origin[0] + u * xz->u_axis[0] + v * xz->v_axis[0],
            xz->origin[1] + u * xz->u_axis[1] + v * xz->v_axis[1],
            xz->origin[2] + u * xz->u_axis[2] + v * xz->v_axis[2]};
    };
    const viewer::CameraState oblique_camera{
        viewer::Point3{60.0, 75.0, 100.0},
        viewer::Point3{0.0, 0.0, 0.0},
        viewer::Vec3{0.0, 0.0, 1.0},
        viewer::CameraProjection::orthographic, 100.0};
    if (!widget.setCameraState(oblique_camera)) {
        return fail("native E0 oblique camera");
    }
    const auto old_xyz = toWorld(0.0, 8.0);
    const auto new_xyz = toWorld(0.0, -8.0);
    if (std::abs(old_xyz.z - new_xyz.z) < 8.0) {
        return fail("native E0 XZ support did not project into 3D");
    }
    if (!putStageLine(
            toWorld(-10.0, 6.0), toWorld(10.0, 10.0))) {
        return fail("native E0 authored XZ AIS");
    }
    const auto old_xz_hit = project(old_xyz);
    const auto new_xz_hit = project(new_xyz);
    if (!old_xz_hit || !new_xz_hit ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
        !picksToken(*old_xz_hit)) {
        return fail("native E0 authored XZ pick in oblique view");
    }
    if (!putStageLine(
            toWorld(-10.0, -8.0), toWorld(10.0, -8.0)) ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
        !picksToken(*new_xz_hit) ||
        !picksNothing(*old_xz_hit)) {
        return fail("native E0 solved XZ picks in oblique view");
    }
    if (!widget.setSketchScene(viewer::SketchScene{}) ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
        !picksNothing(*new_xz_hit)) {
        return fail("native E0 XZ fail-closed scene");
    }

    // E0 native joined-path proof: unlike the scripted scenes above,
    // the Part controller now creates the exact SketchScene that the
    // real QtOcctViewerWidget converts into AIS objects and native hit
    // tokens. A deterministic solver double is intentionally used;
    // original PlaneGCS production/distribution is an independent gate.
    {
        struct NativeStageSolver final : sketch::ILocalSketchSolver {
            int calls{};
            bool reject{};
            double solved_v{-15.0};

            [[nodiscard]] sketch::LocalBackendSolveResult solve(
                const sketch::LocalSolveProblem& problem) override {
                ++calls;
                if (reject) {
                    return {sketch::LocalBackendStatus::failed,
                            std::nullopt, {}, {}, std::nullopt};
                }
                auto candidate = problem.authored;
                if (candidate.lines.size() != 1U) {
                    return {sketch::LocalBackendStatus::failed,
                            std::nullopt, {}, {}, std::nullopt};
                }
                candidate.lines.front().start.v = solved_v;
                candidate.lines.front().end.v = solved_v;
                return {sketch::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, std::nullopt};
            }
        } backend;

        application::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        if (!created.ok() || !created.sketch_id)
            return fail("E0 joined Part create Sketch");
        const auto sid = *created.sketch_id;
        const auto added = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 1.0}, {5.0, 2.0}});
        if (!added.ok() || !added.entity_id)
            return fail("E0 joined Part skew seed");
        const auto relation = session.execute(
            application::AddHorizontalSketchConstraintCommand{
                sid, session.document().revision(), *added.entity_id});
        if (!relation.ok() || !relation.constraint_id)
            return fail("E0 joined Part relation authoring");
        const auto revision = session.document().revision();
        const auto undo_depth = session.undoDepth();

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        ui::PartViewportController controller{tree_controller, &widget};
        controller.setDocumentSession(&session);
        controller.setSketchEditSketch(sid);
        if (!widget.setCameraState(sketch_camera)) {
            return fail("E0 joined native camera");
        }
        const auto old_pixel =
            project(viewer::Point3{2.5, 1.5, 0.0});
        const auto solved_pixel =
            project(viewer::Point3{2.5, -15.0, 0.0});
        if (!old_pixel || !solved_pixel)
            return fail("E0 joined projected hit points");
        const auto original_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto old_hit = widget.querySketchPresentation(*old_pixel);
        if (!original_token || !old_hit.completed || !old_hit.token ||
            *old_hit.token != *original_token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U) {
            return fail("E0 joined original Part geometry AIS/pick");
        }

        controller.setDevelopmentStageSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto current_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto current_hit = widget.querySketchPresentation(*solved_pixel);
        const auto obsolete_hit = widget.querySketchPresentation(*old_pixel);
        const auto* model = controller.currentSketchInteractionModel();
        if (backend.calls != 1 ||
            !current_token || !current_hit.completed || !current_hit.token ||
            *current_hit.token != *current_token ||
            !obsolete_hit.completed || obsolete_hit.token ||
            !model || model->state().lines.size() != 1U ||
            !near(model->state().lines.front().start.v, -15.0, 1.0e-7) ||
            !near(model->state().lines.front().end.v, -15.0, 1.0e-7) ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U) {
            return fail("E0 joined solved Part to native AIS/pick");
        }
        const auto semantic =
            controller.sketchEntityFor(*current_hit.token);
        if (!semantic || semantic->sketch_id != sid ||
            semantic->entity_id != *added.entity_id ||
            session.document().revision() != revision ||
            session.undoDepth() != undo_depth ||
            session.document().findSketch(sid)->
                model.state().lines.front().end.v != 2.0) {
            return fail("E0 joined semantic pick is authored identity");
        }

        backend.reject = true;
        controller.refreshPresentation();
        const auto rejected_hit =
            widget.querySketchPresentation(*solved_pixel);
        if (backend.calls != 2 || !rejected_hit.completed ||
            rejected_hit.token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
            controller.sketchPresentationFor(*added.entity_id) ||
            controller.currentSketchInteractionModel()) {
            return fail("E0 joined rejected Part stage clears native AIS");
        }

        backend.reject = false;
        backend.solved_v = -10.0;
        controller.refreshPresentation();
        const auto recovered_pixel =
            project(viewer::Point3{2.5, -10.0, 0.0});
        if (!recovered_pixel)
            return fail("E0 joined recovered native pixel");
        const auto recovered_hit =
            widget.querySketchPresentation(*recovered_pixel);
        const auto stale_hit =
            widget.querySketchPresentation(*solved_pixel);
        const auto recovered_token =
            controller.sketchPresentationFor(*added.entity_id);
        if (backend.calls != 3 ||
            !recovered_token || !recovered_hit.completed ||
            recovered_hit.token != recovered_token ||
            !stale_hit.completed || stale_hit.token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
            session.document().revision() != revision ||
            session.undoDepth() != undo_depth) {
            return fail("E0 joined recovery uses fresh native geometry");
        }
        controller.clear();
    }
    // Restore the original XY camera and scene so the pre-existing
    // spatial ray/orbit tests are not affected by this 3D E0 witness.
    if (!widget.setCameraState(sketch_camera) ||
        !widget.setSketchScene(scene) ||
        widget.runtimeDiagnostics().sketch_native_objects_current != 1U) {
        return fail("native E0 baseline restore");
    }
    QApplication::processEvents();

    viewer::SketchPreviewScene preview;
    preview.lines.push_back(
        viewer::SketchPreviewLine{
            viewer::Point3{0.0, 0.0, 0.0},
            viewer::Point3{0.0, 8.0, 0.0}});
    if (!widget.setSketchPreviewScene(preview)) {
        return fail("preview scene");
    }
    if (!widget.setSketchPreviewScene(
            viewer::SketchPreviewScene{})) {
        return fail("preview clear");
    }

    int selection_intents = 0;
    widget.setSelectionIntentHandler(
        [&selection_intents](
            const viewer::SelectionIntent&) {
            ++selection_intents;
        });

    std::vector<viewer::SpatialPointerEvent>
        spatial_events;
    widget.setSpatialPointerHandler(
        [&spatial_events](
            const viewer::SpatialPointerEvent& event) {
            spatial_events.push_back(event);
        });

    widget.setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    widget.setCursorMode(
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    const QPoint center{
        widget.width() / 2,
        widget.height() / 2};
    const QPoint offset{
        center.x() - 31,
        center.y() - 19};

    // Prove passive pointer movement independently from primary actions.
    sendMouseMove(widget, offset);
    sendMouseMove(widget, center);
    QApplication::processEvents();

    bool saw_move = false;
    for (const auto& event : spatial_events) {
        if (!event.valid()) {
            return fail("invalid passive spatial event");
        }
        saw_move =
            saw_move ||
            event.phase ==
                viewer::SpatialPointerPhase::move;
    }
    if (!saw_move) {
        return fail("passive move routing");
    }

    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    if (!support) {
        return fail("XY support");
    }
    const auto placement =
        part::sketchPlacementForSupport(*support);
    if (!placement) {
        return fail("XY placement");
    }

    const auto camera_before =
        widget.cameraState();
    if (!camera_before) {
        return fail("camera before orbit");
    }

    const auto* center_event_before =
        findLastMoveAt(
            spatial_events,
            center);
    if (center_event_before == nullptr) {
        return fail(
            "missing synthetic center move before orbit");
    }

    const auto expected_before =
        centerUvFromCamera(
            *placement,
            *camera_before);
    const auto actual_before =
        ui::detail::sketchPointFromRay(
            *placement,
            center_event_before->ray);
    const auto tolerance_before =
        rasterWorldTolerance(
            widget,
            *camera_before);

    if (!expected_before ||
        !actual_before ||
        !near(
            actual_before->u,
            expected_before->u,
            tolerance_before) ||
        !near(
            actual_before->v,
            expected_before->v,
            tolerance_before)) {
        return failMapping(
            "center ray mapping before orbit",
            widget,
            *center_event_before,
            *camera_before,
            expected_before,
            actual_before,
            tolerance_before);
    }

    // Prove primary routing is exclusive: spatial mode emits press/release
    // but no SelectionIntent for the same action.
    spatial_events.clear();
    const auto selections_before_spatial_click =
        selection_intents;

    sendLeftClick(widget, center);
    QApplication::processEvents();

    bool saw_press = false;
    bool saw_release = false;
    for (const auto& event : spatial_events) {
        if (!event.valid()) {
            return fail("invalid spatial primary event");
        }
        saw_press =
            saw_press ||
            event.phase ==
                viewer::SpatialPointerPhase::
                    primary_press;
        saw_release =
            saw_release ||
            event.phase ==
                viewer::SpatialPointerPhase::
                    primary_release;
    }
    if (!saw_press || !saw_release) {
        return fail("spatial primary press/release");
    }
    if (selection_intents !=
        selections_before_spatial_click) {
        return fail("spatial click also emitted SelectionIntent");
    }

    // Orbit changes the camera but the provider ray and host plane mapping
    // must remain mutually consistent.
    spatial_events.clear();
    widget.orbitByRadians(0.35, -0.22);

    const auto camera_after =
        widget.cameraState();
    if (!camera_after) {
        return fail("camera after orbit");
    }
    if (camera_after->eye == sketch_camera.eye) {
        return fail("orbit did not change camera");
    }

    sendMouseMove(widget, offset);
    sendMouseMove(widget, center);
    QApplication::processEvents();
    if (spatial_events.empty()) {
        return fail("move after orbit");
    }

    const auto* center_event_after =
        findLastMoveAt(
            spatial_events,
            center);
    if (center_event_after == nullptr) {
        return fail(
            "missing synthetic center move after orbit");
    }

    const auto expected_after =
        centerUvFromCamera(
            *placement,
            *camera_after);
    const auto actual_after =
        ui::detail::sketchPointFromRay(
            *placement,
            center_event_after->ray);
    const auto tolerance_after =
        rasterWorldTolerance(
            widget,
            *camera_after);

    if (!expected_after ||
        !actual_after ||
        !near(
            actual_after->u,
            expected_after->u,
            tolerance_after) ||
        !near(
            actual_after->v,
            expected_after->v,
            tolerance_after)) {
        return failMapping(
            "center ray mapping after orbit",
            widget,
            *center_event_after,
            *camera_after,
            expected_after,
            actual_after,
            tolerance_after);
    }

    // Presentation-selection mode owns the primary click. Passive movement
    // may still generate spatial move events, but no spatial primary phase
    // may be generated by this click.
    widget.setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting::
            presentation_selection);
    widget.setCursorMode(
        viewer::ViewportCursorMode::
            select_pick_box);

    const auto event_index_before_pick =
        spatial_events.size();
    const auto selections_before_pick =
        selection_intents;

    sendLeftClick(
        widget,
        QPoint{5, 5});
    QApplication::processEvents();

    if (selection_intents <= selections_before_pick) {
        return fail("presentation-selection click");
    }

    for (std::size_t index =
             event_index_before_pick;
         index < spatial_events.size();
         ++index) {
        const auto phase =
            spatial_events[index].phase;
        if (phase ==
                viewer::SpatialPointerPhase::
                    primary_press ||
            phase ==
                viewer::SpatialPointerPhase::
                    primary_release) {
            return fail(
                "presentation click emitted spatial primary phase");
        }
    }

    widget.setCursorMode(
        viewer::ViewportCursorMode::
            system_default);

    widget.close();
    return EXIT_SUCCESS;
}
