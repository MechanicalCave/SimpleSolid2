#include "sketch_viewport_mapping.hpp"
#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"
#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QMouseEvent>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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

// Native Workbench + actual Qt/OCCT provider, not the scripted
// TestViewportWidget used by wb01.cad_workbench. In the ordinary
// preview-OFF build this protects the non-authoring fallback. In a
// separately configured original PlaneGCS UI_PREVIEW=ON build it
// exercises real spatial input and an accepted Horizontal command.
// No original-provider success is claimed from preview-OFF CI.
int testNativeWorkbenchRelationsHost() {
    const auto workspace =
        std::filesystem::temp_directory_path() /
        ("ss2_e0_native_workbench_" +
         std::to_string(
             std::filesystem::file_time_type::clock::now()
                 .time_since_epoch().count()));
    std::error_code ec;
    std::filesystem::create_directories(workspace, ec);
    if (ec) return fail("E0 native Workbench temporary workspace");
    struct RemoveWorkspace final {
        std::filesystem::path path;
        ~RemoveWorkspace() {
            std::error_code ignored;
            std::filesystem::remove_all(path, ignored);
        }
    } cleanup{workspace};
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(
        QSettings::IniFormat,
        QSettings::UserScope,
        QString::fromStdString((workspace / "prefs").string()));

    application::DocumentSession session{
        workspace / "Part001.ss2part",
        part::PartDocument::create(core::DocumentId::generate())};
    const auto created = session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    if (!created.ok() || !created.sketch_id)
        return fail("E0 native Workbench create Sketch");
    const auto sid = *created.sketch_id;
    const auto line = session.execute(
        application::AddSketchLineCommand{
            sid, {0.0, 1.0}, {5.0, 2.0}});
    if (!line.ok() || !line.entity_id)
        return fail("E0 native Workbench skew authored Line");
    const auto baseline_revision = session.document().revision();
    const auto baseline_undo = session.undoDepth();

    viewer_qt_occt::QtOcctViewerWidget* native = nullptr;
    ui::CadWorkbench workbench{
        [&native](QWidget* parent) {
            native = new viewer_qt_occt::QtOcctViewerWidget{parent};
            return ui::ViewportSurface{native, native};
        }};
    if (!native) return fail("E0 native Workbench provider factory");
    workbench.resize(1400, 900);
    workbench.show();
    if (!QTest::qWaitForWindowExposed(&workbench, 3000))
        return fail("E0 native Workbench exposure");
    if (!workbench.activateDocument(&session, workspace))
        return fail("E0 native Workbench activate Part");
    auto* tree = workbench.findChild<QTreeWidget*>(
        QStringLiteral("documentTree"));
    auto* edit = workbench.findChild<QAction*>(
        QStringLiteral("editSketchAction"));
    auto* relations = workbench.findChild<QPushButton*>(
        QStringLiteral("relationsSketchToolButton"));
    auto* horizontal = workbench.findChild<QPushButton*>(
        QStringLiteral("sketchRelationHorizontalButton"));
    auto* feedback = workbench.findChild<QLabel*>(
        QStringLiteral("sketchRelationsFeedbackLabel"));
    auto* authored = workbench.findChild<QListWidget*>(
        QStringLiteral("sketchAuthoredRelationsList"));
    auto* undo = workbench.findChild<QPushButton*>(
        QStringLiteral("undoDocumentButton"));
    auto* redo = workbench.findChild<QPushButton*>(
        QStringLiteral("redoDocumentButton"));
    auto* save = workbench.findChild<QPushButton*>(
        QStringLiteral("saveDocumentButton"));
    if (!tree || !edit || !relations || !horizontal ||
        !feedback || !authored || !undo || !redo || !save ||
        !native->isVisible())
        return fail("E0 native Workbench actual UI host");
    QTreeWidgetItem* sketch_item = nullptr;
    for (QTreeWidgetItemIterator it(tree); *it; ++it) {
        if ((*it)->text(0) == QStringLiteral("Sketch 1")) {
            sketch_item = *it;
            break;
        }
    }
    if (!sketch_item) return fail("E0 native Workbench Sketch tree");
    tree->setCurrentItem(sketch_item);
    QApplication::processEvents();
    if (!edit->isEnabled())
        return fail("E0 native Workbench Sketch Edit action");
    edit->trigger();
    QApplication::processEvents();
    // Fix a known camera for exact native pixel and hit evidence.
    const viewer::CameraState camera{
        viewer::Point3{0.0, 0.0, 100.0},
        viewer::Point3{0.0, 0.0, 0.0},
        viewer::Vec3{0.0, 1.0, 0.0},
        viewer::CameraProjection::orthographic, 30.0};
    if (!native->setCameraState(camera))
        return fail("E0 native Workbench Sketch camera");
    QApplication::processEvents();

    // No neutral scene getter: measure the actual native projected AIS
    // hit crossing at two well-separated X positions. A sloped seed
    // must have different Y crossings; a solved Horizontal must have
    // the SAME crossing in current 3D world geometry. This is a
    // semantic native picking oracle, not a QT label or fake scene.
    const auto nativeLineCrossing = [native](double x)
        -> std::optional<
            std::pair<double, viewer::PresentationToken>> {
        std::optional<viewer::PresentationToken> owner;
        double sum_v{};
        int hits{};
        for (int sample = 0; sample <= 160; ++sample) {
            const double v = -2.0 + 0.05 * sample;
            const auto pixel = native->projectWorldPoint(
                viewer::Point3{x, v, 0.0});
            if (!pixel) return std::nullopt;
            const auto picked =
                native->querySketchPresentation(*pixel);
            if (!picked.completed) return std::nullopt;
            if (!picked.token) continue;
            if (owner && *picked.token != *owner)
                return std::nullopt;
            owner = *picked.token;
            sum_v += v;
            ++hits;
        }
        if (!owner || hits < 2) return std::nullopt;
        return std::pair<double, viewer::PresentationToken>{
            sum_v / hits, *owner};
    };
    const auto authored_left = nativeLineCrossing(0.5);
    const auto authored_right = nativeLineCrossing(4.5);
    if (!authored_left || !authored_right ||
        authored_left->second != authored_right->second ||
        !near(authored_left->first, 1.1, 0.2) ||
        !near(authored_right->first, 1.9, 0.2) ||
        std::abs(authored_right->first - authored_left->first) < 0.55) {
        return fail("E0 native Workbench authored skew world AIS/pick");
    }
    const auto screen =
        native->projectWorldPoint(viewer::Point3{2.5, 1.5, 0.0});
    const auto seed_hit = screen
        ? native->querySketchPresentation(*screen)
        : viewer::SketchPointQueryResult{};
    if (!screen || !seed_hit.completed || !seed_hit.token ||
        native->runtimeDiagnostics().sketch_native_objects_current != 1U)
        return fail("E0 native Workbench genuine skew AIS/pick");

    // A genuine mouse click goes through the installed native input
    // handler, unlike the synthetic semantic pick in wb01.
    sendLeftClick(
        *native,
        QPoint{static_cast<int>(std::lround(screen->x)),
               static_cast<int>(std::lround(screen->y))});
    QApplication::processEvents();
    relations->click();
    QApplication::processEvents();
    if (!relations->isChecked() || !horizontal->isEnabled())
        return fail("E0 native Workbench Relations controls");
    horizontal->click();
    QApplication::processEvents();
    const bool original_preview = workbench.property(
        "ss2OriginalPlaneGcsDevRelationsPreview").toBool();
    const auto* current = session.document().findSketch(sid);
    if (!current || current->model.state().lines.size() != 1U ||
        !near(current->model.state().lines.front().start.v, 1.0, 1.0e-8) ||
        !near(current->model.state().lines.front().end.v, 2.0, 1.0e-8))
        return fail("E0 native Workbench durable line seed");

    if (original_preview) {
        // This branch runs only when the entire UI library is built
        // with the source-verified real original PlaneGCS 0.8.0.
        // A real native click + Qt buttons must author exactly one
        // constraint without overwriting the skew authored Line.
        if (current->model.state().constraints.size() != 1U ||
            current->model.state().constraints.front().kind !=
                sketch::ConstraintKind::horizontal ||
            session.document().revision() == baseline_revision ||
            session.undoDepth() != baseline_undo + 1U ||
            native->runtimeDiagnostics().sketch_native_objects_current != 1U ||
            !feedback->text().contains(
                QStringLiteral("accepted by current Part solver")) ||
            authored->count() != 1 ||
            !authored->item(0)->text().contains(
                QStringLiteral("Horizontal"))) {
            return fail("E0 original PlaneGCS native Workbench Horizontal");
        }
        const auto accepted_id =
            current->model.state().constraints.front().id;
        const auto solved_left = nativeLineCrossing(0.5);
        const auto solved_right = nativeLineCrossing(4.5);
        if (!solved_left || !solved_right ||
            solved_left->second != solved_right->second ||
            solved_left->second == authored_left->second ||
            std::abs(solved_left->first - solved_right->first) > 0.15 ||
            !near(solved_left->first, 1.5, 0.75)) {
            return fail("E0 original PlaneGCS Horizontal native 3D geometry");
        }

        // The *actual Workbench* Undo/Redo actions must restore skew
        // then recompute a new 3D numerical stage; there must be no
        // replay of the old native AIS token or persisted solved seed.
        if (!undo->isEnabled())
            return fail("E0 original PlaneGCS Workbench Undo enabled");
        undo->click();
        QApplication::processEvents();
        const auto* undone = session.document().findSketch(sid);
        const auto undo_left = nativeLineCrossing(0.5);
        const auto undo_right = nativeLineCrossing(4.5);
        if (!undone || !undone->model.state().constraints.empty() ||
            session.undoDepth() != baseline_undo ||
            !undo_left || !undo_right ||
            undo_left->second != undo_right->second ||
            undo_left->second == solved_left->second ||
            !near(undo_left->first, 1.1, 0.2) ||
            !near(undo_right->first, 1.9, 0.2) ||
            native->runtimeDiagnostics().sketch_native_objects_current != 1U) {
            return fail("E0 original PlaneGCS native Workbench Undo/skew");
        }
        if (!redo->isEnabled())
            return fail("E0 original PlaneGCS Workbench Redo enabled");
        redo->click();
        QApplication::processEvents();
        const auto* redone = session.document().findSketch(sid);
        const auto redo_left = nativeLineCrossing(0.5);
        const auto redo_right = nativeLineCrossing(4.5);
        if (!redone || redone->model.state().constraints.size() != 1U ||
            redone->model.state().constraints.front().id != accepted_id ||
            session.undoDepth() != baseline_undo + 1U ||
            !redo_left || !redo_right ||
            redo_left->second != redo_right->second ||
            redo_left->second == undo_left->second ||
            redo_left->second == solved_left->second ||
            std::abs(redo_left->first - redo_right->first) > 0.15 ||
            !near(redo_left->first, 1.5, 0.75) ||
            !near(redone->model.state().lines.front().start.v, 1.0, 1.0e-8) ||
            !near(redone->model.state().lines.front().end.v, 2.0, 1.0e-8) ||
            native->runtimeDiagnostics().sketch_native_objects_current != 1U) {
            return fail("E0 original PlaneGCS native Workbench Redo/solved");
        }
        std::cout
            << "E0_ORIGINAL_PLANEGCS_NATIVE_WORKBENCH_PREVIEW_ON_PASS"
            << '\n';
    } else {
        // A preview-OFF Workbench may stage a relation kind but cannot
        // masquerade as successful source-backed authoring.
        const auto still_skew_left = nativeLineCrossing(0.5);
        const auto still_skew_right = nativeLineCrossing(4.5);
        if (!current->model.state().constraints.empty() ||
            session.document().revision() != baseline_revision ||
            session.undoDepth() != baseline_undo ||
            !still_skew_left || !still_skew_right ||
            still_skew_left->second != still_skew_right->second ||
            !near(still_skew_left->first, 1.1, 0.2) ||
            !near(still_skew_right->first, 1.9, 0.2) ||
            native->runtimeDiagnostics().sketch_native_objects_current != 1U)
            return fail("E0 native Workbench preview-OFF no authoring");
        std::cout << "E0_NATIVE_WORKBENCH_PREVIEW_OFF_PASS" << '\n';
    }
    // User-visible document switch: A's native Sketch AIS/semantic
    // selection must be revoked before opening a different Part. A
    // later return to A must republish a *new* token and a fresh
    // native stage from the correct Part/sketch, never reuse B's hit.
    const auto before_switch_left = nativeLineCrossing(0.5);
    if (!before_switch_left)
        return fail("E0 native Workbench switch source AIS");
    const auto old_world_pixel = native->projectWorldPoint(
        viewer::Point3{0.5, before_switch_left->first, 0.0});
    if (!old_world_pixel)
        return fail("E0 native Workbench switch source projection");
    const auto before_switch_revision = session.document().revision();
    const auto before_switch_undo = session.undoDepth();

    application::DocumentSession other_session{
        workspace / "Part002.ss2part",
        part::PartDocument::create(core::DocumentId::generate())};
    const auto other_created = other_session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    if (!other_created.ok() || !other_created.sketch_id)
        return fail("E0 native Workbench other Part Sketch");
    const auto other_line = other_session.execute(
        application::AddSketchLineCommand{
            *other_created.sketch_id, {0.0, 8.0}, {5.0, 10.0}});
    if (!other_line.ok() || !other_line.entity_id)
        return fail("E0 native Workbench other Part Line");
    const auto other_revision = other_session.document().revision();
    const auto other_undo = other_session.undoDepth();

    const auto nativeHasNoOldPick =
        [native](viewer::ViewportPoint2 point) {
            const auto hit = native->querySketchPresentation(point);
            return hit.completed && !hit.token;
        };
    const auto openSketchFromTree = [&]() {
        auto* item = static_cast<QTreeWidgetItem*>(nullptr);
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) == QStringLiteral("Sketch 1")) {
                item = *it;
                break;
            }
        }
        if (!item) return false;
        tree->setCurrentItem(item);
        QApplication::processEvents();
        if (!edit->isEnabled()) return false;
        edit->trigger();
        QApplication::processEvents();
        return native->setCameraState(camera);
    };

    if (!workbench.activateDocument(&other_session, workspace))
        return fail("E0 native Workbench A to B activate");
    QApplication::processEvents();
    if (native->runtimeDiagnostics().sketch_native_objects_current != 0U ||
        !nativeHasNoOldPick(*old_world_pixel))
        return fail("E0 native Workbench A to B revokes stale AIS");
    if (!openSketchFromTree())
        return fail("E0 native Workbench B enters Sketch Edit");

    const auto other_pixel = native->projectWorldPoint(
        viewer::Point3{2.5, 9.0, 0.0});
    const auto other_hit = other_pixel
        ? native->querySketchPresentation(*other_pixel)
        : viewer::SketchPointQueryResult{};
    if (!other_pixel || !other_hit.completed || !other_hit.token ||
        other_hit.token == before_switch_left->second ||
        !nativeHasNoOldPick(*old_world_pixel) ||
        native->runtimeDiagnostics().sketch_native_objects_current != 1U ||
        other_session.document().revision() != other_revision ||
        other_session.undoDepth() != other_undo)
        return fail("E0 native Workbench B owns distinct native AIS");

    if (!workbench.activateDocument(&session, workspace))
        return fail("E0 native Workbench B to A activate");
    QApplication::processEvents();
    if (native->runtimeDiagnostics().sketch_native_objects_current != 0U ||
        !nativeHasNoOldPick(*other_pixel))
        return fail("E0 native Workbench B to A revokes B native AIS");
    if (!openSketchFromTree())
        return fail("E0 native Workbench A re-enters Sketch Edit");

    const auto returned_left = nativeLineCrossing(0.5);
    const auto returned_right = nativeLineCrossing(4.5);
    if (!returned_left || !returned_right ||
        returned_left->second != returned_right->second ||
        returned_left->second == before_switch_left->second ||
        returned_left->second == other_hit.token ||
        native->runtimeDiagnostics().sketch_native_objects_current != 1U ||
        session.document().revision() != before_switch_revision ||
        session.undoDepth() != before_switch_undo) {
        return fail("E0 native Workbench A re-entry fresh AIS/lease");
    }
    if (original_preview) {
        const auto* returned_sketch = session.document().findSketch(sid);
        if (!returned_sketch ||
            returned_sketch->model.state().constraints.size() != 1U ||
            std::abs(returned_left->first - returned_right->first) > 0.15 ||
            !near(returned_left->first, 1.5, 0.75))
            return fail("E0 original PlaneGCS A re-entry accepted stage");
    } else if (!near(returned_left->first, 1.1, 0.2) ||
               !near(returned_right->first, 1.9, 0.2)) {
        return fail("E0 native Workbench preview-OFF A re-entry seed");
    }
    // Unlike the PartViewportController cold-load test, exercise the
    // actual Workbench Save button, true .ss2part container persistence,
    // document deactivation, new DocumentSession incarnation, and the
    // SAME live Qt/OCCT Workbench's re-entry after a cold load.
    const auto before_save_left = nativeLineCrossing(0.5);
    const auto before_save_right = nativeLineCrossing(4.5);
    if (!before_save_left || !before_save_right || !save->isEnabled())
        return fail("E0 native Workbench cold Save prerequisites");
    const auto persisted_revision = session.document().revision();
    const auto persisted_undo = session.undoDepth();
    const auto persisted_sketch = session.document().findSketch(sid);
    if (!persisted_sketch ||
        persisted_sketch->model.state().lines.size() != 1U)
        return fail("E0 native Workbench durable Sketch before Save");

    save->click();
    QApplication::processEvents();
    if (!session.fileCheckpoint() || session.needsSave() ||
        !std::filesystem::is_regular_file(session.path()) ||
        session.document().revision() != persisted_revision ||
        session.undoDepth() != persisted_undo)
        return fail("E0 native Workbench Qt Save .ss2part");

    part::PartDocumentStore store;
    auto disk = store.load(session.path());
    if (!disk.ok() || !disk.document || !disk.checkpoint ||
        disk.document->documentId() != session.documentId() ||
        disk.document->incarnationToken() ==
            session.document().incarnationToken())
        return fail("E0 native Workbench cold loaded Part incarnation");
    const auto* durable_sketch = disk.document->findSketch(sid);
    if (!durable_sketch ||
        durable_sketch->model.state().lines !=
            persisted_sketch->model.state().lines ||
        durable_sketch->model.state().constraints !=
            persisted_sketch->model.state().constraints ||
        durable_sketch->model.state().constraints.size() !=
            (original_preview ? 1U : 0U))
        return fail("E0 native Workbench cold authored relation/seed");

    const auto prior_native_token = before_save_left->second;
    const auto prior_native_pixel = native->projectWorldPoint(
        viewer::Point3{0.5, before_save_left->first, 0.0});
    if (!prior_native_pixel)
        return fail("E0 native Workbench saved stage projected point");
    application::DocumentSession cold_session{
        session.path(), std::move(*disk.document), *disk.checkpoint};

    workbench.deactivateDocument();
    QApplication::processEvents();
    if (workbench.activeDocumentId() ||
        native->runtimeDiagnostics().sketch_native_objects_current != 0U ||
        !nativeHasNoOldPick(*prior_native_pixel))
        return fail("E0 native Workbench cold deactivation revokes AIS");
    if (!workbench.activateDocument(&cold_session, workspace) ||
        !openSketchFromTree())
        return fail("E0 native Workbench cold session re-enters Sketch");
    QApplication::processEvents();

    const auto cold_left = nativeLineCrossing(0.5);
    const auto cold_right = nativeLineCrossing(4.5);
    const auto* reloaded_sketch = cold_session.document().findSketch(sid);
    if (!reloaded_sketch || !cold_left || !cold_right ||
        cold_left->second != cold_right->second ||
        cold_left->second == prior_native_token ||
        cold_left->second == other_hit.token ||
        native->runtimeDiagnostics().sketch_native_objects_current != 1U ||
        cold_session.needsSave() || cold_session.undoDepth() != 0U ||
        cold_session.document().findSketch(sid)->
            model.state().lines != persisted_sketch->model.state().lines ||
        cold_session.document().findSketch(sid)->
            model.state().constraints !=
                persisted_sketch->model.state().constraints)
        return fail("E0 native Workbench cold fresh AIS and authored state");

    if (original_preview) {
        // The new Sketch Edit lease must reauthorize the original real
        // PlaneGCS provider; a previously saved numerical AIS answer is
        // not an authored Sketch and cannot be resurrected.
        if (reloaded_sketch->model.state().constraints.size() != 1U ||
            std::abs(cold_left->first - cold_right->first) > 0.15 ||
            !near(cold_left->first, 1.5, 0.75))
            return fail("E0 original PlaneGCS Workbench cold solver stage");
    } else if (!near(cold_left->first, 1.1, 0.2) ||
               !near(cold_right->first, 1.9, 0.2)) {
        return fail("E0 native Workbench cold preview-OFF skew seed");
    }
    workbench.deactivateDocument();
    if (native->runtimeDiagnostics().sketch_native_objects_current != 0U ||
        !nativeHasNoOldPick(*prior_native_pixel))
        return fail("E0 native Workbench exit revokes AIS");

    if (original_preview) {
        std::cout
            << "E0_ORIGINAL_PLANEGCS_NATIVE_WORKBENCH_COLD_REOPEN_PASS"
            << '\n';
    } else {
        std::cout << "E0_NATIVE_WORKBENCH_COLD_REOPEN_PREVIEW_OFF_PASS"
                  << '\n';
    }
    workbench.close();
    return EXIT_SUCCESS;
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
        const auto rejected_authored_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto* rejected_model =
            controller.currentSketchInteractionModel();
        // A valid *empty* current snapshot is allowed by the E0
        // authority contract. It carries zero selectable geometry;
        // nullptr is not the only legitimate fail-closed result.
        if (backend.calls != 2 || !rejected_hit.completed ||
            rejected_hit.token ||
            !rejected_authored_hit.completed ||
            rejected_authored_hit.token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
            controller.sketchPresentationFor(*added.entity_id) ||
            (rejected_model &&
             (!rejected_model->state().lines.empty() ||
              !rejected_model->state().constraints.empty()))) {
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

        // Native manual-readiness regression: Undo removes the authored
        // relation, returns to the skew seed without calling the stage
        // backend and MUST revoke the formerly solved AIS and token.
        const auto undone = session.undo();
        if (!undone.ok() || !undone.changed)
            return fail("E0 joined Undo command");
        controller.refreshPresentation();
        const auto undone_seed_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto undone_solved_hit =
            widget.querySketchPresentation(*recovered_pixel);
        const auto undone_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto* undone_model =
            controller.currentSketchInteractionModel();
        if (backend.calls != 3 ||
            session.undoDepth() + 1U != undo_depth ||
            session.document().findSketch(sid)->
                model.state().constraints.size() != 0U ||
            !undone_seed_hit.completed ||
            undone_seed_hit.token != undone_token ||
            !undone_token ||
            !undone_solved_hit.completed || undone_solved_hit.token ||
            controller.sketchEntityFor(*recovered_token) ||
            !undone_model || undone_model->state().lines.size() != 1U ||
            !near(undone_model->state().lines.front().end.v, 2.0, 1.0e-8) ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U) {
            return fail("E0 joined Undo stale native AIS/token");
        }

        // Redo must authorize a NEW Part numerical stage and fresh
        // semantic presentation token, not replay the pre-Undo AIS.
        const auto redone = session.redo();
        if (!redone.ok() || !redone.changed)
            return fail("E0 joined Redo command");
        controller.refreshPresentation();
        const auto redone_hit =
            widget.querySketchPresentation(*recovered_pixel);
        const auto redone_seed_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto redone_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto redone_semantic =
            redone_token ? controller.sketchEntityFor(*redone_token)
                         : std::nullopt;
        if (backend.calls != 4 ||
            session.undoDepth() != undo_depth ||
            session.document().findSketch(sid)->
                model.state().constraints.size() != 1U ||
            session.document().findSketch(sid)->
                model.state().constraints.front().id != *relation.constraint_id ||
            !redone_token || redone_token == recovered_token ||
            !redone_hit.completed || redone_hit.token != redone_token ||
            !redone_seed_hit.completed || redone_seed_hit.token ||
            controller.sketchEntityFor(*undone_token) ||
            !redone_semantic || redone_semantic->sketch_id != sid ||
            redone_semantic->entity_id != *added.entity_id ||
            session.document().findSketch(sid)->
                model.state().lines.front().end.v != 2.0 ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U) {
            return fail("E0 joined Redo rebuild native AIS/semantic pick");
        }

        // Ending a DEV solver lease is not a license to display the
        // authored skew line as though it were an evaluated current
        // solution. Clearing must revoke native objects and both picks.
        controller.setDevelopmentStageSolver(nullptr, {});
        const auto revoked_hit =
            widget.querySketchPresentation(*recovered_pixel);
        const auto revoked_seed_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto* revoked_model =
            controller.currentSketchInteractionModel();
        if (backend.calls != 4 || !revoked_hit.completed ||
            revoked_hit.token || !revoked_seed_hit.completed ||
            revoked_seed_hit.token ||
            controller.sketchPresentationFor(*added.entity_id) ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
            (revoked_model && !revoked_model->state().lines.empty())) {
            return fail("E0 joined lease revoke clears native geometry");
        }

        // Re-registration is a new evaluation, not a resurrected AIS.
        controller.setDevelopmentStageSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto restored_hit =
            widget.querySketchPresentation(*recovered_pixel);
        const auto restored_token =
            controller.sketchPresentationFor(*added.entity_id);
        if (backend.calls != 5 || !restored_token ||
            !restored_hit.completed || restored_hit.token != restored_token ||
            restored_token == redone_token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
            session.undoDepth() != undo_depth ||
            session.document().findSketch(sid)->
                model.state().lines.front().end.v != 2.0) {
            return fail("E0 joined lease reactivation native stage");
        }
        // Persist actual .ss2part authored state. A new loaded Part
        // incarnation must not inherit any UI/OCCT stage cache, native
        // AIS object, selection token or borrowed solver authority.
        // This is a cold DocumentSession/PartStore load in this test
        // process, NOT a distinct-process original PlaneGCS proof.
        const auto cold_path =
            std::filesystem::temp_directory_path() /
            ("ss2_e0_native_cold_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count()) + ".ss2part");
        struct RemoveColdNativeFile final {
            std::filesystem::path path;
            ~RemoveColdNativeFile() {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
        } cleanup{cold_path};
        part::PartDocumentStore store;
        if (!store.createNew(cold_path, session.document()).ok()) {
            return fail("E0 joined native cold persist .ss2part");
        }
        auto cold = store.load(cold_path);
        if (!cold.ok() || !cold.document || !cold.checkpoint ||
            cold.document->documentId() != session.documentId() ||
            cold.document->incarnationToken() ==
                session.document().incarnationToken()) {
            return fail("E0 joined native cold loaded Part incarnation");
        }
        const auto* durable = cold.document->findSketch(sid);
        if (!durable || durable->model.state().lines.size() != 1U ||
            !near(durable->model.state().lines.front().start.v, 1.0, 1.0e-8) ||
            !near(durable->model.state().lines.front().end.v, 2.0, 1.0e-8) ||
            durable->model.state().constraints.size() != 1U ||
            durable->model.state().constraints.front().id !=
                *relation.constraint_id) {
            return fail("E0 joined native cold retained authored intent");
        }

        application::DocumentSession reopened{
            cold_path, std::move(*cold.document), *cold.checkpoint};
        controller.setDocumentSession(nullptr);
        const auto closed_hit =
            widget.querySketchPresentation(*recovered_pixel);
        if (!closed_hit.completed || closed_hit.token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
            controller.sketchEntityFor(*restored_token) ||
            backend.calls != 5) {
            return fail("E0 joined native close revokes old AIS/token");
        }
        controller.setDocumentSession(&reopened);
        controller.setSketchEditSketch(sid);
        // The new backend answer is deliberately different from the
        // previously displayed solved stage at v=-10.
        backend.solved_v = -24.0;
        controller.setDevelopmentStageSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto cold_pixel =
            project(viewer::Point3{2.5, -24.0, 0.0});
        const auto cold_hit = cold_pixel
            ? widget.querySketchPresentation(*cold_pixel)
            : viewer::SketchPointQueryResult{};
        const auto obsolete_cold_hit =
            widget.querySketchPresentation(*recovered_pixel);
        const auto obsolete_seed_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto cold_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto cold_semantic = cold_token
            ? controller.sketchEntityFor(*cold_token)
            : std::nullopt;
        const auto* cold_model =
            controller.currentSketchInteractionModel();
        if (backend.calls != 6 || !cold_pixel || !cold_token ||
            cold_token == restored_token ||
            !cold_hit.completed || cold_hit.token != cold_token ||
            !obsolete_cold_hit.completed || obsolete_cold_hit.token ||
            !obsolete_seed_hit.completed || obsolete_seed_hit.token ||
            controller.sketchEntityFor(*restored_token) ||
            !cold_semantic || cold_semantic->sketch_id != sid ||
            cold_semantic->entity_id != *added.entity_id ||
            !cold_model || cold_model->state().lines.size() != 1U ||
            !near(cold_model->state().lines.front().start.v, -24.0, 1.0e-7) ||
            !near(cold_model->state().lines.front().end.v, -24.0, 1.0e-7) ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
            reopened.document().findSketch(sid)->
                model.state().constraints.front().id != *relation.constraint_id ||
            reopened.document().findSketch(sid)->
                model.state().lines.front().end.v != 2.0 ||
            reopened.needsSave() || reopened.undoDepth() != 0U) {
            return fail("E0 joined native cold reopens fresh effective stage");
        }

        // A failure on the NEW incarnation must still clear that new
        // result rather than reviving either old or newly cached AIS.
        backend.reject = true;
        controller.refreshPresentation();
        const auto failed_cold_hit =
            widget.querySketchPresentation(*cold_pixel);
        if (backend.calls != 7 || !failed_cold_hit.completed ||
            failed_cold_hit.token ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
            controller.sketchPresentationFor(*added.entity_id) ||
            controller.sketchEntityFor(*cold_token) ||
            reopened.needsSave() || reopened.undoDepth() != 0U) {
            return fail("E0 joined native cold failed stage clears AIS");
        }
        controller.clear();
    }

    // Native E0 non-XY joined-path regression. The earlier XZ example
    // authored a SketchScene directly; here the real Part controller
    // must resolve XZ local U/V into OCCT world XYZ and keep semantic
    // picking attached to the same current validated numerical stage.
    // A horizontal world-XY shortcut would fail the oblique native hit.
    {
        struct XZStageSolver final : sketch::ILocalSketchSolver {
            int calls{};
            bool reject{};

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
                candidate.lines.front().start.v = -18.0;
                candidate.lines.front().end.v = -18.0;
                return {sketch::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, std::nullopt};
            }
        } backend;
        application::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xz_plane});
        if (!created.ok() || !created.sketch_id)
            return fail("E0 joined XZ Part create Sketch");
        const auto sid = *created.sketch_id;
        const auto added = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 1.0}, {5.0, 2.0}});
        if (!added.ok() || !added.entity_id)
            return fail("E0 joined XZ Part skew seed");
        const auto relation = session.execute(
            application::AddHorizontalSketchConstraintCommand{
                sid, session.document().revision(), *added.entity_id});
        if (!relation.ok() || !relation.constraint_id)
            return fail("E0 joined XZ Horizontal relation");
        const auto revision = session.document().revision();
        const auto undo_depth = session.undoDepth();

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        ui::PartViewportController controller{tree_controller, &widget};
        controller.setDocumentSession(&session);
        controller.setSketchEditSketch(sid);
        if (!widget.setCameraState(oblique_camera))
            return fail("E0 joined XZ oblique camera");

        const auto old_xyz = toWorld(2.5, 1.5);
        const auto solved_xyz = toWorld(2.5, -18.0);
        const auto old_pixel = project(old_xyz);
        const auto solved_pixel = project(solved_xyz);
        const auto original_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto original_hit = old_pixel
            ? widget.querySketchPresentation(*old_pixel)
            : viewer::SketchPointQueryResult{};
        if (!old_pixel || !solved_pixel ||
            std::abs(old_xyz.z - solved_xyz.z) < 10.0 ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
            !original_token || !original_hit.completed ||
            original_hit.token != original_token) {
            return fail("E0 joined XZ authored world-space AIS");
        }

        controller.setDevelopmentStageSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto current_token =
            controller.sketchPresentationFor(*added.entity_id);
        const auto solved_hit =
            widget.querySketchPresentation(*solved_pixel);
        const auto stale_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto semantic = current_token
            ? controller.sketchEntityFor(*current_token)
            : std::nullopt;
        const auto* current =
            controller.currentSketchInteractionModel();
        if (backend.calls != 1 || !current_token ||
            current_token == original_token ||
            !solved_hit.completed || solved_hit.token != current_token ||
            !stale_hit.completed || stale_hit.token ||
            controller.sketchEntityFor(*original_token) ||
            !semantic || semantic->sketch_id != sid ||
            semantic->entity_id != *added.entity_id ||
            !current || current->state().lines.size() != 1U ||
            !near(current->state().lines.front().start.v, -18.0, 1.0e-7) ||
            !near(current->state().lines.front().end.v, -18.0, 1.0e-7) ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 1U ||
            session.document().revision() != revision ||
            session.undoDepth() != undo_depth ||
            session.document().findSketch(sid)->
                model.state().lines.front().end.v != 2.0) {
            return fail("E0 joined XZ effective stage native AIS/pick");
        }

        backend.reject = true;
        controller.refreshPresentation();
        const auto rejected_hit =
            widget.querySketchPresentation(*solved_pixel);
        const auto rejected_seed_hit =
            widget.querySketchPresentation(*old_pixel);
        const auto* rejected =
            controller.currentSketchInteractionModel();
        if (backend.calls != 2 || !rejected_hit.completed ||
            rejected_hit.token || !rejected_seed_hit.completed ||
            rejected_seed_hit.token ||
            controller.sketchPresentationFor(*added.entity_id) ||
            widget.runtimeDiagnostics().sketch_native_objects_current != 0U ||
            (rejected && !rejected->state().lines.empty()) ||
            session.document().revision() != revision ||
            session.undoDepth() != undo_depth) {
            return fail("E0 joined XZ rejected stage revokes AIS/pick");
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
    if (testNativeWorkbenchRelationsHost() != EXIT_SUCCESS)
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}
