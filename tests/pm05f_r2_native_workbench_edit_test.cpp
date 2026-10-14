#include "cad_workbench.hpp"
#include "part_viewport_controller.hpp"


#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDebug>
#include <QLabel>
#include <QMouseEvent>
#include <QMenu>
#include <QPushButton>
#include <QToolButton>
#include <QScrollArea>
#include <QTest>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <iostream>
#include <iterator>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

using namespace simplesolid2;

namespace {

std::atomic<std::uint32_t> owner_edge_outline_warnings{0U};

// Real Windows private committed bytes, not an inferred Body/mesh size.
// Load the stock system psapi.dll at runtime so the existing native test
// target and build/link machinery remain unchanged. Unsupported platforms
// report unavailable instead of manufacturing parity.
[[nodiscard]] std::optional<std::uint64_t>
currentProcessPrivateBytes() {
#if defined(_WIN32)
    using QueryProcessMemory =
        BOOL (WINAPI*)(HANDLE, PPROCESS_MEMORY_COUNTERS, DWORD);
    const HMODULE module = LoadLibraryW(L"psapi.dll");
    if (module == nullptr) return std::nullopt;
    const auto query = reinterpret_cast<QueryProcessMemory>(
        GetProcAddress(module, "GetProcessMemoryInfo"));
    std::optional<std::uint64_t> result;
    if (query != nullptr) {
        PROCESS_MEMORY_COUNTERS_EX metrics{};
        metrics.cb = sizeof(metrics);
        if (query(GetCurrentProcess(),
                  reinterpret_cast<PPROCESS_MEMORY_COUNTERS>(&metrics),
                  metrics.cb)) {
            result = static_cast<std::uint64_t>(
                metrics.PrivateUsage);
        }
    }
    FreeLibrary(module);
    return result;
#else
    return std::nullopt;
#endif
}

void captureOwnerEdgeOutlineWarning(
    QtMsgType type,
    const QMessageLogContext&,
    const QString& message) {
    if (type != QtWarningMsg ||
        !message.contains(
            QStringLiteral(
                "SS2 Viewer display-only Edge outline"))) {
        return;
    }
    owner_edge_outline_warnings.fetch_add(
        1U, std::memory_order_relaxed);
    std::cerr
        << "PM05F_R2_OWNER_OUTLINE_WARNING "
        << message.toStdString()
        << '\n';
}

void check(bool ok, const char* expression, int line) {
    if (ok) return;
    std::cerr << "PM05F R2 native Workbench Edit CHECK failed at "
              << line << ": " << expression << '\n';
    std::exit(EXIT_FAILURE);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

// D2-R2/R3: the original sanitized Owner regression files are immutable v1
// modeling-policy inputs. The production loader must reject them, while a
// temporary TEST-ONLY v2 repackage exercises their original authored
// geometry and feature history under the new production kernel policy.
[[nodiscard]] part::PartLoadResult
loadOwnerFixtureV2ForTest(
    const part::PartDocumentStore& store,
    const std::filesystem::path& source) {
    // PG-01E/E2: these are ORIGINAL Owner-provided modeling-v1
    // regression files. Verify exact bytes around the production
    // typed refusal AND the separate test-only temporary v2 package.
    // No in-place migration, schema repair or write-back is legal.
    const auto sourceBytes = [&source]() {
        std::ifstream in{source, std::ios::binary};
        CHECK(static_cast<bool>(in));
        return std::string{
            std::istreambuf_iterator<char>{in},
            std::istreambuf_iterator<char>{}};
    };
    const auto original_bytes = sourceBytes();
    CHECK(!original_bytes.empty());
    const auto rejected = store.load(source);
    CHECK(!rejected.ok());
    CHECK(rejected.diagnostic.code ==
          part::PartStoreErrorCode::unsupported_modeling_semantics);
    CHECK(sourceBytes() == original_bytes);
    const auto native =
        persistence::readNativeDocumentContainer(source);
    CHECK(native.ok());
    auto authored = native.package->authored_json;
    // PartDocumentStore's canonical pretty JSON contains this one
    // exact persisted modeling-policy field. Test-only replacement is
    // deliberately bounded: no private UUID/history/reference rewrite
    // and no production migration or extra CMake JSON dependency.
    const auto needle = std::string{
        "\"modeling_semantics_version\": 1"};
    const auto at = authored.find(needle);
    CHECK(at != std::string::npos);
    CHECK(authored.find(needle, at + needle.size()) ==
          std::string::npos);
    authored.replace(
        at, needle.size(),
        "\"modeling_semantics_version\": 2");
    const auto built =
        persistence::buildNativeDocumentContainer(
            native.package->descriptor, authored);
    CHECK(built.ok());
    QTemporaryDir scratch;
    CHECK(scratch.isValid());
    const auto temp_path =
        std::filesystem::path{scratch.path().toStdWString()} /
        source.filename();
    {
        std::ofstream output{
            temp_path, std::ios::binary | std::ios::trunc};
        CHECK(static_cast<bool>(output));
        output.write(
            built.bytes->data(),
            static_cast<std::streamsize>(built.bytes->size()));
        CHECK(static_cast<bool>(output));
    }
    auto loaded = store.load(temp_path);
    CHECK(loaded.ok());
    CHECK(loaded.document->modelingSemanticsVersion() ==
          part::current_modeling_semantics_version);
    CHECK(sourceBytes() == original_bytes);
    return loaded;
}

application::DocumentSession makeBaseSession(
    kernel_occt::OcctSolidModelingKernel& kernel,
    double width = 40.0,
    double depth = 30.0,
    double height = 20.0) {
    auto source =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{{}, std::move(source)};
    const auto sketch =
        session.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);
    CHECK(session.execute(application::AddSketchRectangleCommand{
        *sketch.sketch_id,
        session.document().revision(),
        {0.0, 0.0},
        {width, depth},
        sketch::EntityRole::regular,
        false}).ok());
    const auto* model =
        session.document().findSketch(*sketch.sketch_id);
    CHECK(model != nullptr);
    const auto regions = sketch::analyzeRegions(model->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto base = session.execute(
        application::CreateExtrudeFeatureCommand{
            *profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{
                core::LengthValue{height}, false},
            "Base"},
        kernel);
    CHECK(base.ok() && base.feature_id);
    return session;
}

application::DocumentSession makeRevolveSession(
    double angle) {
    auto source =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{{}, std::move(source)};
    const auto sketch =
        session.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);
    CHECK(session.execute(application::AddSketchRectangleCommand{
        *sketch.sketch_id,
        session.document().revision(),
        {10.0, 0.0},
        {20.0, 10.0},
        sketch::EntityRole::regular,
        false}).ok());
    const auto* model =
        session.document().findSketch(*sketch.sketch_id);
    CHECK(model != nullptr);
    const auto regions =
        sketch::analyzeRegions(model->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto id = state.body.next_feature_id.allocate();
    CHECK(id.has_value());
    state.body.features.push_back(part::PartFeature{
        *id,
        "Revolve",
        false,
        part::RevolveFeature{
            *profile.profile_id,
            part::AxisReference{
                part::BuiltinOriginAxisReference{
                    core::BuiltinReferenceRole::x_axis}},
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{angle}, false}}});
    auto restored = part::PartDocument::restore(
        session.document().documentId(),
        std::move(state),
        session.document().revision());
    CHECK(restored.ok());
    return application::DocumentSession{
        {}, std::move(*restored.document)};
}

struct WorldEdgeProbe final {
    part::MaterialEdgeReference reference;
    viewer::Point3 world;
};

std::vector<WorldEdgeProbe> authorableEdgeProbes(
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto evaluated =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology.has_value());
    CHECK(evaluated.current_topology->complete());
    CHECK(evaluated.body_solid != nullptr);
    const auto presented =
        kernel.bodyPresentation(evaluated.body_solid);
    CHECK(presented.ok());
    std::vector<WorldEdgeProbe> probes;
    for (const auto& edge : presented.body.edges) {
        const auto authored =
            part::authorMaterialEdgeReference(
                *evaluated.current_topology, edge.runtime_token);
        if (!authored.ok() || edge.points.size() < 2U) {
            continue;
        }
        const auto a = edge.points[edge.points.size() / 2U - 1U];
        const auto b = edge.points[edge.points.size() / 2U];
        const viewer::Point3 center{
            (a.x + b.x) / 2.0,
            (a.y + b.y) / 2.0,
            (a.z + b.z) / 2.0};
        probes.push_back({*authored.reference, center});
    }
    CHECK(probes.size() >= 4U);
    return probes;
}

std::vector<std::array<viewer::Point3, 3U>>
authorableTrihedralCornerProbes(
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto evaluated =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology);
    CHECK(evaluated.current_topology->complete());
    const auto presented =
        kernel.bodyPresentation(evaluated.body_solid);
    CHECK(presented.ok());
    const auto isSameTestPoint =
        [](const kernel::Point3& a,
           const kernel::Point3& b) {
            constexpr double epsilon = 1.0e-6;
            return std::abs(a.x - b.x) <= epsilon &&
                   std::abs(a.y - b.y) <= epsilon &&
                   std::abs(a.z - b.z) <= epsilon;
        };
    std::vector<std::array<viewer::Point3, 3U>> corners;
    for (const auto& vertex : presented.body.vertices) {
        std::vector<viewer::Point3> incident_edge_centers;
        for (const auto& edge : presented.body.edges) {
            if (edge.points.size() < 2U ||
                (!isSameTestPoint(
                    edge.points.front(), vertex.point) &&
                 !isSameTestPoint(
                    edge.points.back(), vertex.point))) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *evaluated.current_topology,
                    edge.runtime_token);
            if (!authored.ok()) {
                continue;
            }
            const auto& first = edge.points.front();
            const auto& last = edge.points.back();
            incident_edge_centers.push_back({
                (first.x + last.x) / 2.0,
                (first.y + last.y) / 2.0,
                (first.z + last.z) / 2.0});
        }
        if (incident_edge_centers.size() == 3U) {
            corners.push_back({
                incident_edge_centers[0],
                incident_edge_centers[1],
                incident_edge_centers[2]});
        }
    }
    CHECK(corners.size() == 8U);
    return corners;
}

// Edge Picking R1 G1: test-only front-visibility witness on the actual
// OCCT Body presentation mesh. A camera-aligned orthographic ray is cast
// through the EXACT world-space point, independently of the Viewer's
// rounded projection and candidate/ranking algorithm. A sample is admitted
// as a reliable positive control only if its nearest mesh intersection
// coincides with that point; silhouette/tessellation gaps stay UNCLASSIFIED.
// No ray hit, a nearer blocking Face or perspective camera => not proof.
[[nodiscard]] bool independentOrthographicEdgeVisibility(
    const kernel::SolidPresentationMesh& mesh,
    const viewer::CameraState& camera,
    viewer::Point3 world) {
    if (camera.projection !=
            viewer::CameraProjection::orthographic ||
        !viewer::finite(world) || mesh.triangles.empty()) {
        return false;
    }
    const auto towards_eye =
        viewer::normalized(camera.eye - camera.target);
    if (!towards_eye) return false;
    constexpr double distance = 1000.0;
    const auto origin = world + *towards_eye * distance;
    const auto direction = *towards_eye * -1.0;
    std::optional<double> nearest;
    for (const auto& triangle : mesh.triangles) {
        const auto a = viewer::Point3{
            triangle.first.x, triangle.first.y, triangle.first.z};
        const auto b = viewer::Point3{
            triangle.second.x, triangle.second.y, triangle.second.z};
        const auto c = viewer::Point3{
            triangle.third.x, triangle.third.y, triangle.third.z};
        const auto ab = b - a;
        const auto ac = c - a;
        const auto p = viewer::cross(direction, ac);
        const double det = viewer::dot(ab, p);
        if (std::abs(det) < 1.0e-11) continue;
        const double inv_det = 1.0 / det;
        const auto offset = origin - a;
        const double u = viewer::dot(offset, p) * inv_det;
        if (u < -1.0e-9 || u > 1.0 + 1.0e-9) continue;
        const auto q = viewer::cross(offset, ab);
        const double v = viewer::dot(direction, q) * inv_det;
        if (v < -1.0e-9 || u + v > 1.0 + 1.0e-9) continue;
        const double depth = viewer::dot(ac, q) * inv_det;
        if (depth <= 1.0e-8 || !std::isfinite(depth)) continue;
        if (!nearest || depth < *nearest) nearest = depth;
    }
    // A curve/tessellation mismatch or near-wall ambiguity is not a
    // trustworthy positive witness. This tolerance is classification-only
    // and NEVER changes the Viewer's depth or picking policy.
    constexpr double mesh_match_mm = 0.02;
    return nearest &&
        std::abs(*nearest - distance) <= mesh_match_mm;
}

QTreeWidgetItem* featureTreeItem(
    QTreeWidget& tree, part::FeatureId feature_id) {
    constexpr int feature_id_data = Qt::UserRole + 45;
    const auto id = QString::fromStdString(feature_id.serialized());
    std::vector<QTreeWidgetItem*> pending;
    for (int i = 0; i < tree.topLevelItemCount(); ++i) {
        pending.push_back(tree.topLevelItem(i));
    }
    while (!pending.empty()) {
        auto* item = pending.back();
        pending.pop_back();
        if (item == nullptr) continue;
        if (item->data(0, feature_id_data).toString() == id) {
            return item;
        }
        for (int i = 0; i < item->childCount(); ++i) {
            pending.push_back(item->child(i));
        }
    }
    return nullptr;
}

// UI-01 P0 matrix: exercise the real Tree actions against an already
// active Feature Edit draft; selection for Properties is allowed to move,
// but Operations/draft identity and authored Part state must not.
void verifyUi01FeatureDraftRejectsTreeEdits(
    ui::CadWorkbench& workbench,
    application::DocumentSession& session,
    QTreeWidget& tree,
    QTreeWidgetItem* feature_item,
    const char* operation_widget_name) {
    auto* sketch_action = workbench.findChild<QAction*>(
        QStringLiteral("editSketchAction"));
    auto* profile_action = workbench.findChild<QAction*>(
        QStringLiteral("editProfileAction"));
    auto* sketch_finish = workbench.findChild<QPushButton*>(
        QStringLiteral("finishSketchButton"));
    auto* status = workbench.findChild<QLabel*>(
        QStringLiteral("workbenchStatus"));
    auto* active_operations = workbench.findChild<QWidget*>(
        QString::fromLatin1(operation_widget_name));
    CHECK(sketch_action && profile_action && sketch_finish &&
          status && active_operations && feature_item);
    // G3 regression at the actual OCCT Feature Edit boundary: grouped
    // toolbar presentation must not interfere with the sole live draft,
    // original Tree command entry or fixed Document controls.
    auto* contextual_scroll = workbench.findChild<QScrollArea*>(
        QStringLiteral("documentToolsScroll"));
    auto* more_tools = workbench.findChild<QToolButton*>(
        QStringLiteral("workbenchToolbarMoreButton"));
    CHECK(contextual_scroll && more_tools);
    CHECK(contextual_scroll->horizontalScrollBarPolicy() ==
          Qt::ScrollBarAlwaysOff);
    CHECK(more_tools->menu() != nullptr);
    auto* native_modify_dropdown =
        workbench.findChild<QToolButton*>(
            QStringLiteral(
                "workbenchToolbarGroupDropdown_Part_Modify"));
    CHECK(native_modify_dropdown != nullptr);
    CHECK(native_modify_dropdown->menu() != nullptr);
    bool native_fillet_menu = false;
    bool native_chamfer_menu = false;
    for (const auto* action : native_modify_dropdown->menu()->actions()) {
        if (action == nullptr) continue;
        native_fillet_menu |=
            action->text() == QStringLiteral("Fillet");
        native_chamfer_menu |=
            action->text() == QStringLiteral("Chamfer");
    }
    CHECK(native_fillet_menu && native_chamfer_menu);
    // Tier 2/3 presentation uses the SAME Extrude/Revolve commands.
    // The pinned Create named dropdown must not disappear under a
    // concurrent real OCCT Feature Edit, even if layout hides it.
    auto* native_create_dropdown =
        workbench.findChild<QToolButton*>(
            QStringLiteral(
                "workbenchToolbarGroupDropdown_Part_Create"));
    CHECK(native_create_dropdown != nullptr);
    CHECK(native_create_dropdown->menu() != nullptr);
    auto* original_extrude =
        workbench.findChild<QPushButton*>(
            QStringLiteral("extrudeToolButton"));
    auto* original_revolve =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveToolButton"));
    CHECK(original_extrude && original_revolve);
    auto* original_fillet = workbench.findChild<QPushButton*>(
        QStringLiteral("filletToolButton"));
    auto* original_chamfer = workbench.findChild<QPushButton*>(
        QStringLiteral("chamferToolButton"));
    CHECK(original_fillet && original_chamfer);
    // UI-01 G3 native RED: an existing Feature Edit owns Operations
    // and the viewport cue, NOT the corresponding Create toolbar
    // command. None of the original create buttons may advertise
    // "engaged" merely because an Edit draft exists.
    CHECK(!original_extrude->isChecked());
    CHECK(!original_revolve->isChecked());
    CHECK(!original_fillet->isChecked());
    CHECK(!original_chamfer->isChecked());
    bool native_extrude_menu = false;
    bool native_revolve_menu = false;
    for (const auto* action : native_create_dropdown->menu()->actions()) {
        if (action == nullptr) continue;
        native_extrude_menu |=
            action->text() == QStringLiteral("Extrude");
        native_revolve_menu |=
            action->text() == QStringLiteral("Revolve");
    }
    CHECK(native_extrude_menu && native_revolve_menu);
    CHECK(sketch_finish->isHidden());
    CHECK(!active_operations->isHidden());
    CHECK(!session.document().sketches().empty());

    // G2: the same native Finish/Cancel widgets stay in the fixed footer
    // while inspecting other Tree entities; the cue follows the live
    // Feature draft and must not be rebound to the inspected selection.
    auto* footer = workbench.findChild<QWidget*>(
        QStringLiteral("workbenchOperationsFooter"));
    auto* operations_scroll = workbench.findChild<QScrollArea*>(
        QStringLiteral("workbenchOperationsScroll"));
    auto* operations_cue = workbench.findChild<QLabel*>(
        QStringLiteral("workbenchOperationsModeCue"));
    auto* viewport_cue = workbench.findChild<QLabel*>(
        QStringLiteral("workbenchViewportModeCue"));
    CHECK(footer && operations_scroll && operations_cue && viewport_cue);
    CHECK(footer->isVisible());
    const auto context_cue = operations_cue->text();
    CHECK(!context_cue.isEmpty());
    CHECK(viewport_cue->text() == context_cue);
    // The cue must identify the actual immutable Feature target, not
    // merely the tool category or an unrelated Tree inspection row.
    const auto selected_feature_key =
        feature_item->data(0, Qt::UserRole + 45).toString();
    QString draft_target_name;
    for (const auto& feature : session.document().body().features) {
        if (QString::fromStdString(feature.id.serialized()) ==
                selected_feature_key) {
            draft_target_name = QString::fromStdString(feature.name);
            break;
        }
    }
    CHECK(!draft_target_name.isEmpty());
    CHECK(context_cue.contains(draft_target_name));
    const auto finish_name =
        std::string_view{operation_widget_name} ==
            "revolveOperationsWidget"
            ? QStringLiteral("revolveFinishButton")
            : std::string_view{operation_widget_name} ==
                  "extrudeOperationsWidget"
                ? QStringLiteral("extrudeFinishButton")
                : QStringLiteral("edgeFeatureFinishButton");
    const auto cancel_name =
        std::string_view{operation_widget_name} ==
            "revolveOperationsWidget"
            ? QStringLiteral("revolveCancelButton")
            : std::string_view{operation_widget_name} ==
                  "extrudeOperationsWidget"
                ? QStringLiteral("extrudeCancelButton")
                : QStringLiteral("edgeFeatureCancelButton");
    auto* original_finish =
        workbench.findChild<QPushButton*>(finish_name);
    auto* original_cancel =
        workbench.findChild<QPushButton*>(cancel_name);
    CHECK(original_finish && original_cancel);
    CHECK(footer->isAncestorOf(original_finish));
    CHECK(footer->isAncestorOf(original_cancel));
    CHECK(!operations_scroll->isAncestorOf(original_finish));
    CHECK(!operations_scroll->isAncestorOf(original_cancel));
    CHECK(original_finish->isVisible());
    CHECK(original_cancel->isVisible());

    const auto original_state = session.document().state();
    const auto original_revision = session.document().revision();
    const auto original_undo = session.undoDepth();
    const bool original_dirty = session.needsSave();

    // Native Qt/OCCT P0 guard: entering and leaving toolbar Customize
    // while a real Feature Edit draft is live changes neither draft
    // ownership/cue nor the authored Part/revision/Undo/dirty state.
    auto* customize_toolbar =
        workbench.findChild<QToolButton*>(
            QStringLiteral("workbenchToolbarCustomizeButton"));
    auto* create_group_handle =
        workbench.findChild<QToolButton*>(
            QStringLiteral("workbenchToolbarHandle_Part_Create"));
    CHECK(customize_toolbar && create_group_handle);
    CHECK(!customize_toolbar->isChecked());
    CHECK(create_group_handle->isHidden());
    customize_toolbar->click();
    QApplication::processEvents();
    CHECK(customize_toolbar->isChecked());
    CHECK(create_group_handle->menu() != nullptr);
    CHECK(create_group_handle->popupMode() ==
          QToolButton::MenuButtonPopup);
    CHECK(!create_group_handle->isHidden());
    CHECK(create_group_handle->toolTip().contains(
        QStringLiteral("Przeciągnij")));
    // This is the same real Feature Edit that rejected Sketch/Profile
    // conflicts. The drag affordance itself may exist, but merely
    // entering Customize cannot create a CAD command or ghost draft.
    customize_toolbar->click();
    QApplication::processEvents();
    CHECK(!customize_toolbar->isChecked());
    CHECK(create_group_handle->isHidden());
    CHECK(active_operations->isVisible());
    CHECK(operations_cue->text() == context_cue);
    CHECK(viewport_cue->text() == context_cue);
    CHECK(session.document().state() == original_state);
    CHECK(session.document().revision() == original_revision);
    CHECK(session.undoDepth() == original_undo);
    CHECK(session.needsSave() == original_dirty);

    const auto sketch_key = QString::fromStdString(
        std::string{session.document().sketches().front().id.value()});
    QTreeWidgetItem* sketch_item = nullptr;
    QTreeWidgetItem* profile_item = nullptr;
    for (QTreeWidgetItemIterator it(&tree); *it; ++it) {
        if ((*it)->data(0, Qt::UserRole + 42).toString() ==
            sketch_key) {
            sketch_item = *it;
        }
        if (!(*it)->data(0, Qt::UserRole + 43)
                 .toString().isEmpty()) {
            profile_item = *it;
        }
    }
    CHECK(sketch_item && profile_item);
    const auto attempt = [&](QTreeWidgetItem* target,
                             QAction* action) {
        tree.clearSelection();
        tree.setCurrentItem(target);
        target->setSelected(true);
        QApplication::processEvents();
        action->trigger();
        QApplication::processEvents();
        CHECK(sketch_finish->isHidden());
        CHECK(!active_operations->isHidden());
        CHECK(footer->isVisible());
        CHECK(operations_cue->text() == context_cue);
        CHECK(viewport_cue->text() == context_cue);
        CHECK(original_finish->isVisible());
        CHECK(original_cancel->isVisible());
        CHECK(status->text().contains(
            QStringLiteral(
                "Finish or cancel the active Feature draft")));
        CHECK(session.document().state() == original_state);
        CHECK(session.document().revision() == original_revision);
        CHECK(session.undoDepth() == original_undo);
        CHECK(session.needsSave() == original_dirty);
    };
    attempt(sketch_item, sketch_action);
    attempt(profile_item, profile_action);

    // Double-click is a second real Tree entry route via the controller's
    // QEvent::MouseButtonDblClick filter, not an alias for QAction::trigger.
    for (auto* target : {sketch_item, profile_item}) {
        tree.scrollToItem(target);
        tree.clearSelection();
        tree.setCurrentItem(target);
        target->setSelected(true);
        QApplication::processEvents();
        const auto at = tree.visualItemRect(target).center();
        CHECK(tree.itemAt(at) == target);
        QTest::mouseDClick(
            tree.viewport(), Qt::LeftButton, Qt::NoModifier, at);
        QApplication::processEvents();
        CHECK(sketch_finish->isHidden());
        CHECK(!active_operations->isHidden());
        CHECK(footer->isVisible());
        CHECK(operations_cue->text() == context_cue);
        CHECK(viewport_cue->text() == context_cue);
        CHECK(original_finish->isVisible());
        CHECK(original_cancel->isVisible());
        CHECK(status->text().contains(
            QStringLiteral(
                "Finish or cancel the active Feature draft")));
        CHECK(session.document().state() == original_state);
        CHECK(session.document().revision() == original_revision);
        CHECK(session.undoDepth() == original_undo);
        CHECK(session.needsSave() == original_dirty);
    }

    tree.clearSelection();
    tree.setCurrentItem(feature_item);
    feature_item->setSelected(true);
    QApplication::processEvents();
    CHECK(!active_operations->isHidden());
    CHECK(sketch_finish->isHidden());
}

// The existing SK-04B native suite confirms that QTest::mouseMove
// can follow the suite-global OS cursor on Windows CI. Dispatch a real
// synchronous QWidget MouseMove event for deterministic hover authority.
// Clicks remain actual QTest QWidget mouseClick gestures.
void sendNativeWorkbenchHover(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    QPoint pixel) {
    QMouseEvent event{
        QEvent::MouseMove, QPointF{pixel}, Qt::NoButton,
        Qt::NoButton, Qt::NoModifier};
    CHECK(QApplication::sendEvent(&viewport, &event));
}

bool nativeClick(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    viewer::Point3 world) {
    const auto screen = viewport.projectWorldPoint(world);
    if (!screen || !std::isfinite(screen->x) ||
        !std::isfinite(screen->y)) {
        return false;
    }
    const QPoint pixel{
        static_cast<int>(std::lround(screen->x)),
        static_cast<int>(std::lround(screen->y))};
    if (!viewport.rect().contains(pixel)) {
        return false;
    }
    const auto query = viewport.queryBodyTopology(
        *screen,
        viewer::BodyTopologyPickFilter{false, true, false});
    if (!query.valid() || !query.completed ||
        query.candidates.empty()) {
        return false;
    }
    QTest::mouseMove(&viewport, pixel);
    QTest::mouseClick(
        &viewport, Qt::LeftButton, Qt::NoModifier, pixel);
    QApplication::processEvents();
    return true;
}

bool selectedCount(const QLabel& label, int count) {
    return label.text().contains(
        QStringLiteral("Selected edges: %1").arg(count));
}

// R2-C/E: query and CLICK actual OCCT-presented circular Edges of full
// and partial Revolve. The world point is only a test cursor fixture:
// authoring still requires the Workbench strict semantic Edge resolver.
bool clickAuthorableRevolveCircle(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel,
    const QLabel& selection) {
    const auto evaluated =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology);
    CHECK(evaluated.current_topology->complete());
    CHECK(evaluated.body_solid != nullptr);
    const auto presented =
        kernel.bodyPresentation(evaluated.body_solid);
    CHECK(presented.ok());
    std::size_t semantic_circles = 0U;
    for (const auto& edge :
         evaluated.current_topology->edges) {
        if (edge.curve_kind !=
            kernel::CurveKind::circle) {
            continue;
        }
        const auto ref =
            part::authorMaterialEdgeReference(
                *evaluated.current_topology,
                edge.runtime_token);
        if (!ref.ok()) continue;
        ++semantic_circles;
        const auto path = std::find_if(
            presented.body.edges.begin(),
            presented.body.edges.end(),
            [&edge](const auto& item) {
                return item.runtime_token ==
                       edge.runtime_token;
            });
        if (path == presented.body.edges.end() ||
            path->points.size() < 2U) {
            continue;
        }
        constexpr std::size_t attempts = 24U;
        for (std::size_t index = 0U;
             index < attempts; ++index) {
            const auto segment =
                (index * (path->points.size() - 1U)) /
                attempts;
            const auto& a = path->points[segment];
            const auto& b = path->points[segment + 1U];
            const viewer::Point3 point{
                (a.x + b.x) / 2.0,
                (a.y + b.y) / 2.0,
                (a.z + b.z) / 2.0};
            if (!nativeClick(viewport, point)) {
                continue;
            }
            if (selectedCount(selection, 1)) {
                return true;
            }
        }
    }
    std::cout
        << "PM05F_R2_REVOLVE_CIRCLE_PICK_FAIL"
        << " semantic_circles=" << semantic_circles
        << '\n';
    CHECK(semantic_circles > 0U);
    return false;
}

bool nativePlanarFaceClick(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    viewer::Point3 world) {
    const auto screen = viewport.projectWorldPoint(world);
    if (!screen || !std::isfinite(screen->x) ||
        !std::isfinite(screen->y)) {
        return false;
    }
    const QPoint pixel{
        static_cast<int>(std::lround(screen->x)),
        static_cast<int>(std::lround(screen->y))};
    if (!viewport.rect().contains(pixel)) return false;
    const auto query = viewport.queryBodyTopology(
        *screen,
        viewer::BodyTopologyPickFilter{true, false, false});
    if (!query.valid() || !query.completed ||
        query.candidates.empty()) {
        return false;
    }
    QTest::mouseMove(&viewport, pixel);
    QTest::mouseClick(
        &viewport, Qt::LeftButton, Qt::NoModifier, pixel);
    QApplication::processEvents();
    return true;
}

// PG-01D D0 native SS2 semantic-side characterization.
// The independent native OCCT wire/hole proof is recorded in work/.
// This test checks the *actual* current Part/OCCT catalog, not OCP:
// strict bounded Face identity is distinct from Surface-carrier admission,
// and only existing material Edge authoring can create accepted sources.
// The provider-facing wire membership query is intentionally NOT invented.
void verifyPg01dNativeStrictFaceAndMaterialCatalog(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto session = makeBaseSession(kernel);
    const auto hole_sketch =
        session.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(hole_sketch.ok() && hole_sketch.sketch_id);
    const auto circle = session.execute(
        application::AddSketchCircleCommand{
            *hole_sketch.sketch_id,
            {20.0, 15.0},
            5.0,
            sketch::EntityRole::regular});
    CHECK(circle.ok());
    const auto* authored =
        session.document().findSketch(*hole_sketch.sketch_id);
    CHECK(authored != nullptr);
    const auto regions = sketch::analyzeRegions(authored->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *hole_sketch.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto through_cut = session.execute(
        application::CreateExtrudeFeatureCommand{
            *profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{25.0}, false},
            "PG01D native through-hole D0"},
        kernel);
    CHECK(through_cut.ok() && through_cut.feature_id);

    const auto evaluation =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluation.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.current_topology.has_value());
    const auto& catalog = *evaluation.current_topology;
    CHECK(catalog.complete());
    CHECK(catalog.stage.feature_id &&
          *catalog.stage.feature_id == *through_cut.feature_id);

    std::size_t strict_face_count = 0U;
    std::size_t carrier_only_face_count = 0U;
    std::size_t other_face_count = 0U;
    for (const auto& face : catalog.faces) {
        CHECK(face.valid());
        if (face.semantic_address) {
            ++strict_face_count;
            CHECK(!face.surface_candidates.empty());
        } else if (!face.surface_candidates.empty()) {
            // Sketch-support admission by Surface is NOT strict
            // bounded-Face Project Geometry authoring permission.
            ++carrier_only_face_count;
        } else {
            ++other_face_count;
        }
    }
    CHECK(!catalog.faces.empty());
    CHECK(strict_face_count > 0U);

    std::vector<part::MaterialEdgeReference>
        current_material_edges;
    std::size_t nonmaterial_edge_count = 0U;
    for (const auto& edge : catalog.edges) {
        CHECK(edge.valid());
        const auto authored_edge =
            part::authorMaterialEdgeReference(
                catalog, edge.runtime_token);
        if (!authored_edge.ok()) {
            ++nonmaterial_edge_count;
            continue;
        }
        CHECK(authored_edge.reference.has_value());
        CHECK(authored_edge.reference->stage == catalog.stage);
        current_material_edges.push_back(
            *authored_edge.reference);
    }
    CHECK(!current_material_edges.empty());
    std::sort(
        current_material_edges.begin(),
        current_material_edges.end());
    CHECK(std::adjacent_find(
        current_material_edges.begin(),
        current_material_edges.end()) ==
        current_material_edges.end());
    CHECK(strict_face_count + carrier_only_face_count +
              other_face_count == catalog.faces.size());
    CHECK(current_material_edges.size() +
              nonmaterial_edge_count == catalog.edges.size());

    // First true SS2 native provider/Part reconciliation. The selected
    // bounded Face remains strict; its native wire uses are only current
    // runtime tokens. Every admitted material use must pass the existing
    // PG-01B authorMaterialEdgeReference contract.
    std::size_t planar_faces = 0U;
    std::size_t nonplanar_faces = 0U;
    std::size_t nonplanar_native_wires = 0U;
    std::size_t nonplanar_material_members = 0U;
    std::size_t nonplanar_nonmaterial_members = 0U;
    std::size_t nonplanar_periodic_seam_edges = 0U;
    std::size_t nonplanar_periodic_seam_uses = 0U;
    std::size_t native_faces_with_holes = 0U;
    std::size_t strict_faces_with_holes = 0U;
    std::size_t fully_material_strict_holed_faces = 0U;
    std::size_t hole_rejected_edges = 0U;
    for (const auto& face : catalog.faces) {
        const auto source =
            kernel.bindFaceToBody(
                evaluation.body_solid, face.runtime_token);
        CHECK(source && source->valid());
        const auto boundary =
            kernel.queryFaceBoundary(
                evaluation.body_solid, *source);
        if (boundary.status ==
                kernel::FaceBoundaryStatus::
                    unsupported_surface) {
            ++nonplanar_faces;
            // Face Boundary E0: the original planar-only query must
            // continue to reject this native cylinder/curved Face.
            // The separate optional read is scoped to the same Body
            // and returns real oriented wires, not a projected Face.
            const auto raw =
                kernel.queryFaceBoundaryAnySurface(
                    evaluation.body_solid, *source);
            if (!raw.ok()) {
                continue;
            }
            nonplanar_native_wires += raw.wires.size();
            // Real OCCT cylinder's parameterization seam is not an
            // engineering material boundary even if the native Face
            // wire reports this Edge twice. Only the exact Part
            // catalog's typed periodic_seam bit is authoritative.
            std::map<std::uint64_t, std::vector<bool>>
                native_face_seam_uses;
            for (const auto& wire : raw.wires) {
                CHECK(wire.valid());
                for (const auto& use : wire.edges) {
                    const auto catalog_member = std::find_if(
                        catalog.edges.begin(),
                        catalog.edges.end(),
                        [&use](const auto& edge) {
                            return edge.runtime_token == use.edge;
                        });
                    CHECK(catalog_member != catalog.edges.end());
                    const auto strict =
                        part::authorMaterialEdgeReference(
                            catalog, use.edge);
                    if (catalog_member->periodic_seam) {
                        CHECK(!strict.ok());
                        CHECK(!catalog_member->representation_partition);
                        native_face_seam_uses[use.edge.value].push_back(
                            use.reversed);
                        ++nonplanar_periodic_seam_uses;
                    }
                    if (strict.ok()) {
                        CHECK(strict.reference);
                        ++nonplanar_material_members;
                    } else {
                        ++nonplanar_nonmaterial_members;
                    }
                }
            }
            for (const auto& [token, uses] :
                 native_face_seam_uses) {
                // Two opposite native wire uses of ONE exact seam Edge.
                // Distinct token values or one use cannot certify a seam.
                CHECK(token != 0U);
                CHECK(uses.size() == 2U);
                CHECK(uses.front() != uses.back());
                ++nonplanar_periodic_seam_edges;
            }
            continue;
        }
        CHECK(boundary.ok());
        ++planar_faces;
        if (boundary.wires.size() <= 1U) continue;
        ++native_faces_with_holes;
        std::size_t outer_wires = 0U;
        bool all_material = true;
        std::vector<part::MaterialEdgeReference>
            member_sources;
        for (const auto& wire : boundary.wires) {
            CHECK(wire.valid());
            if (wire.outer) ++outer_wires;
            for (const auto& member : wire.edges) {
                CHECK(member.valid());
                const auto authored_source =
                    part::authorMaterialEdgeReference(
                        catalog, member.edge);
                if (!authored_source.ok()) {
                    ++hole_rejected_edges;
                    all_material = false;
                    continue;
                }
                CHECK(authored_source.reference &&
                      authored_source.reference->stage ==
                          catalog.stage);
                member_sources.push_back(
                    *authored_source.reference);
            }
        }
        CHECK(outer_wires == 1U);
        std::sort(
            member_sources.begin(),
            member_sources.end());
        CHECK(std::adjacent_find(
            member_sources.begin(),
            member_sources.end()) ==
            member_sources.end());
        if (!face.semantic_address) {
            continue; // Surface carrier alone cannot admit PG-01D.
        }
        ++strict_faces_with_holes;
        if (all_material) {
            ++fully_material_strict_holed_faces;
        }
    }
    // Owner D2 manual multi-Face scope: native Face selection only,
    // not an inferred connected Surface region. The existing through-
    // Cut solid has both planar and cylindrical Face realizations.
    // Explicit shared MATERIAL Edge identity must survive selecting
    // two different Faces; a known cylinder seam must not be authored.
    std::vector<part::SelectedFaceBoundaryAdmission>
        individually_selected;
    std::size_t selected_planar = 0U;
    std::size_t selected_nonplanar = 0U;
    std::size_t selected_seams = 0U;
    std::size_t selected_material = 0U;
    for (const auto& face : catalog.faces) {
        const auto selected =
            part::inspectSelectedFaceBoundary(
                evaluation.features.back(),
                face.runtime_token, kernel);
        if (!selected.ok()) {
            std::cerr
                << "PG01D_MANUAL_FACE_E0_BLOCKED token="
                << face.runtime_token.value
                << " status=" << static_cast<int>(selected.status)
                << '\n';
            continue;
        }
        CHECK(selected.bounded_face == face.runtime_token);
        const auto scope =
            kernel.bindFaceToBody(
                evaluation.body_solid, face.runtime_token);
        CHECK(scope && scope->valid());
        const auto planar = kernel.queryFaceBoundary(
            evaluation.body_solid, *scope);
        if (planar.ok()) {
            ++selected_planar;
        } else {
            CHECK(planar.status ==
                  kernel::FaceBoundaryStatus::
                      unsupported_surface);
            ++selected_nonplanar;
        }
        std::vector<kernel::RuntimeEdgeToken> own_material;
        for (const auto& wire : selected.wires) {
            for (const auto& use : wire.edges) {
                CHECK(use.valid());
                CHECK(use.native_use.start_vertex);
                CHECK(use.native_use.end_vertex);
                if (use.excluded_nonmaterial) {
                    CHECK(!use.material);
                    const auto edge = std::find_if(
                        catalog.edges.begin(), catalog.edges.end(),
                        [&use](const auto& item) {
                            return item.runtime_token ==
                                use.native_use.edge;
                        });
                    CHECK(edge != catalog.edges.end());
                    CHECK(edge->periodic_seam ||
                          edge->representation_partition);
                    CHECK(edge->accounting_class ==
                          part::TopologyAccountingClass::
                              known_representation_artifact);
                    if (edge->periodic_seam) ++selected_seams;
                } else {
                    CHECK(use.material);
                    CHECK(use.material->stage == catalog.stage);
                    CHECK(std::find(
                        own_material.begin(), own_material.end(),
                        use.native_use.edge) ==
                        own_material.end());
                    own_material.push_back(use.native_use.edge);
                    ++selected_material;
                }
            }
        }
        const auto verified_again =
            part::inspectSelectedFaceBoundary(
                evaluation.features.back(),
                face.runtime_token, kernel);
        CHECK(verified_again == selected);
        individually_selected.push_back(selected);
    }
    CHECK(selected_planar >= 1U);
    CHECK(selected_nonplanar >= 1U);
    CHECK(selected_seams >= 2U);
    CHECK(selected_material >= 2U);
    std::size_t shared_material_pairs = 0U;
    for (std::size_t i = 0U;
         i < individually_selected.size(); ++i) {
        for (std::size_t j = i + 1U;
             j < individually_selected.size(); ++j) {
            for (const auto& left : individually_selected[i].wires) {
                for (const auto& a : left.edges) {
                    if (!a.material) continue;
                    for (const auto& right :
                         individually_selected[j].wires) {
                        for (const auto& b : right.edges) {
                            if (!b.material ||
                                *a.material != *b.material) {
                                continue;
                            }
                            CHECK(a.native_use.edge ==
                                  b.native_use.edge);
                            ++shared_material_pairs;
                        }
                    }
                }
            }
        }
    }
    CHECK(shared_material_pairs >= 1U);
    CHECK(!part::inspectSelectedFaceBoundary(
        evaluation.features.back(),
        kernel::RuntimeFaceToken{}, kernel).ok());
    // Exact current Body generation is provider-bound. Another
    // evaluation must never reuse a captured scoped Face to cross
    // provider generations on the strength of numeric token values.
    std::cout
        << "PG01D_MANUAL_MULTI_FACE_NATIVE_E0_PASS"
        << " planar_faces=" << selected_planar
        << " curved_faces=" << selected_nonplanar
        << " real_shared_material_pairs=" << shared_material_pairs
        << " excluded_seam_uses=" << selected_seams
        << " accepted_material_uses=" << selected_material
        << " inferred_carrier_region=0"
        << '\n';

    std::cerr
        << "PG01D_D0_NATIVE_FACE_WIRE_STATUS"
        << " planar=" << planar_faces
        << " nonplanar=" << nonplanar_faces
        << " nonplanar_wires=" << nonplanar_native_wires
        << " nonplanar_material_uses="
        << nonplanar_material_members
        << " nonplanar_nonmaterial_uses="
        << nonplanar_nonmaterial_members
        << " nonplanar_seam_edges="
        << nonplanar_periodic_seam_edges
        << " nonplanar_seam_uses="
        << nonplanar_periodic_seam_uses
        << " holes=" << native_faces_with_holes
        << " strict_holes=" << strict_faces_with_holes
        << " strict_holes_all_material="
        << fully_material_strict_holed_faces
        << " material_rejections=" << hole_rejected_edges
        << std::endl;
    CHECK(native_faces_with_holes >= 2U);
    CHECK(nonplanar_faces >= 1U);
    CHECK(nonplanar_native_wires >= 1U);
    CHECK(nonplanar_material_members >= 1U);
    CHECK(nonplanar_periodic_seam_edges >= 1U);
    CHECK(nonplanar_periodic_seam_uses ==
          2U * nonplanar_periodic_seam_edges);
    CHECK(strict_faces_with_holes > 0U);
    CHECK(fully_material_strict_holed_faces > 0U);

    // A token scoped to one provider realization must not gain authority
    // over a different body just because its numeric Face ID is reused.
    CHECK(!kernel.bindFaceToBody(
        evaluation.body_solid, kernel::RuntimeFaceToken{}));
    const auto invalid_scope =
        kernel.queryFaceBoundary(
            evaluation.body_solid,
            kernel::ScopedBoundaryFace{});
    CHECK(invalid_scope.status ==
          kernel::FaceBoundaryStatus::invalid_input);

    const auto fresh_evaluation =
        part::evaluatePart(session.document(), kernel);
    CHECK(fresh_evaluation.body_solid);
    CHECK(fresh_evaluation.body_solid.get() !=
          evaluation.body_solid.get());
    const auto source_face = catalog.faces.front().runtime_token;
    const auto stale_bound =
        kernel.bindFaceToBody(
            evaluation.body_solid, source_face);
    CHECK(stale_bound);
    const auto stale_query =
        kernel.queryFaceBoundary(
            fresh_evaluation.body_solid, *stale_bound);
    CHECK(stale_query.status ==
          kernel::FaceBoundaryStatus::provider_mismatch);

    std::cout << "PG01D_D0_NATIVE_STRICT_FACE_CATALOG_PASS"
              << " strict_faces=" << strict_face_count
              << " carrier_only_faces=" << carrier_only_face_count
              << " other_faces=" << other_face_count
              << " material_edges=" << current_material_edges.size()
              << " rejected_nonmaterial=" << nonmaterial_edge_count
              << " provider_face_wires_not_yet_claimed=1"
              << '\n';
}

// PG-01D D1: the actual SS2 Part-after-two-Cuts case must preserve an
// *individually bounded* planar Face with two real native inner wires.
// A Surface carrier alone never grants Face Project Geometry admission.
void verifyPg01dNativeTwoHoleFaceBoundary(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto session = makeBaseSession(kernel);
    const auto cut_circle = [&](
        double x, const char* label) {
        const auto sketch =
            session.execute(application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(sketch.ok() && sketch.sketch_id);
        const auto circle = session.execute(
            application::AddSketchCircleCommand{
                *sketch.sketch_id,
                {x, 15.0},
                4.0,
                sketch::EntityRole::regular});
        CHECK(circle.ok());
        const auto* authored =
            session.document().findSketch(*sketch.sketch_id);
        CHECK(authored);
        const auto regions =
            sketch::analyzeRegions(authored->model);
        CHECK(regions.complete() &&
              regions.regions.size() == 1U);
        const auto intent =
            part::makeProfileRegionIntent(regions.regions.front());
        CHECK(intent);
        const auto profile = session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
        CHECK(profile.ok() && profile.profile_id);
        const auto cut = session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{25.0}, false},
                label},
            kernel);
        CHECK(cut.ok() && cut.feature_id);
        return *cut.feature_id;
    };
    cut_circle(12.0, "PG01D First Through Cut");
    const auto last_cut_id =
        cut_circle(28.0, "PG01D Second Through Cut");

    const auto result =
        part::evaluatePart(session.document(), kernel);
    CHECK(result.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(result.body_solid && result.current_topology);
    const auto& catalog = *result.current_topology;
    CHECK(catalog.complete());

    std::size_t observed_two_hole_faces = 0U;
    std::size_t strict_two_hole_faces = 0U;
    std::size_t strict_two_hole_material_faces = 0U;
    std::size_t admitted_complete_face_batches = 0U;
    std::size_t excluded_periodic_seams = 0U;
    std::size_t excluded_partition_edges = 0U;
    for (const auto& edge : catalog.edges) {
        if (!edge.periodic_seam &&
            !edge.representation_partition) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog, edge.runtime_token);
        CHECK(!authored.ok());
        if (edge.periodic_seam) ++excluded_periodic_seams;
        if (edge.representation_partition) {
            ++excluded_partition_edges;
        }
    }

    for (const auto& face : catalog.faces) {
        const auto scoped =
            kernel.bindFaceToBody(
                result.body_solid, face.runtime_token);
        CHECK(scoped && scoped->valid());
        const auto boundary =
            kernel.queryFaceBoundary(result.body_solid, *scoped);
        if (boundary.status ==
                kernel::FaceBoundaryStatus::unsupported_surface) {
            continue;
        }
        CHECK(boundary.ok());
        if (boundary.wires.size() != 3U) continue;
        ++observed_two_hole_faces;
        std::size_t outers = 0U;
        bool all_material = true;
        std::vector<part::MaterialEdgeReference> sources;
        for (const auto& wire : boundary.wires) {
            if (wire.outer) ++outers;
            for (const auto& member : wire.edges) {
                const auto identity =
                    part::authorMaterialEdgeReference(
                        catalog, member.edge);
                if (!identity.ok()) {
                    all_material = false;
                    continue;
                }
                CHECK(identity.reference &&
                      identity.reference->stage ==
                          catalog.stage);
                sources.push_back(*identity.reference);
            }
        }
        CHECK(outers == 1U);
        std::sort(sources.begin(), sources.end());
        CHECK(std::adjacent_find(
            sources.begin(), sources.end()) == sources.end());
        const auto admitted =
            part::inspectMaterialFaceBoundary(
                result.features.back(),
                face.runtime_token, kernel);
        if (!face.semantic_address) {
            CHECK(!admitted.ok());
            CHECK(admitted.status ==
                  part::MaterialFaceBoundaryStatus::face_not_strict);
            continue;
        }
        ++strict_two_hole_faces;
        if (!all_material) {
            CHECK(!admitted.ok());
            CHECK(admitted.status ==
                  part::MaterialFaceBoundaryStatus::
                      material_edge_unavailable);
            continue;
        }
        ++strict_two_hole_material_faces;
        CHECK(admitted.ok());
        CHECK(admitted.bounded_face ==
              face.semantic_address);
        CHECK(admitted.wires.size() ==
              boundary.wires.size());
        std::size_t accepted_members = 0U;
        for (std::size_t wire_index = 0U;
             wire_index < admitted.wires.size();
             ++wire_index) {
            const auto& actual = admitted.wires[wire_index];
            const auto& source = boundary.wires[wire_index];
            CHECK(actual.outer == source.outer);
            CHECK(actual.edges.size() == source.edges.size());
            for (std::size_t edge_index = 0U;
                 edge_index < source.edges.size();
                 ++edge_index) {
                const auto& item = actual.edges[edge_index];
                CHECK(item.valid());
                CHECK(item.current_edge ==
                      source.edges[edge_index].edge);
                CHECK(item.reversed ==
                      source.edges[edge_index].reversed);
                CHECK(item.reference.stage == catalog.stage);
                ++accepted_members;
            }
        }
        CHECK(accepted_members == 6U);
        ++admitted_complete_face_batches;
    }
    std::cerr << "PG01D_D1_TWO_HOLE_FACE_STATUS"
              << " observed=" << observed_two_hole_faces
              << " strict=" << strict_two_hole_faces
              << " strict_material="
              << strict_two_hole_material_faces
              << " complete_face_admissions="
              << admitted_complete_face_batches
              << " excluded_seams="
              << excluded_periodic_seams
              << " excluded_partitions="
              << excluded_partition_edges << std::endl;
    CHECK(observed_two_hole_faces >= 2U);
    CHECK(strict_two_hole_faces > 0U);
    CHECK(strict_two_hole_material_faces > 0U);
    CHECK(admitted_complete_face_batches ==
          strict_two_hole_material_faces);

    // D1 lifecycle: a Face observed before an authored upstream change
    // cannot authorize a new read in a different provider generation.
    // Suppression must reject Face admission without a cached result;
    // unsuppression must rebuild a fresh, strictly mapped outer+holes.
    const auto old_scoped = kernel.bindFaceToBody(
        result.body_solid,
        catalog.faces.front().runtime_token);
    CHECK(old_scoped && old_scoped->valid());
    auto absent_catalog = result.features.back();
    absent_catalog.result_topology.reset();
    const auto invalid_catalog = part::inspectMaterialFaceBoundary(
        absent_catalog,
        catalog.faces.front().runtime_token, kernel);
    CHECK(!invalid_catalog.ok());
    CHECK(invalid_catalog.status ==
          part::MaterialFaceBoundaryStatus::invalid_stage);

    const auto suppressed = session.execute(
        application::SetFeatureSuppressedCommand{
            last_cut_id, session.document().revision(), true});
    CHECK(suppressed.ok());
    const auto suppressed_eval =
        part::evaluatePart(session.document(), kernel);
    CHECK(suppressed_eval.features.size() ==
          result.features.size());
    const auto blocked = part::inspectMaterialFaceBoundary(
        suppressed_eval.features.back(),
        catalog.faces.front().runtime_token, kernel);
    CHECK(!blocked.ok());
    CHECK(blocked.status ==
          part::MaterialFaceBoundaryStatus::invalid_stage);

    const auto unsuppressed = session.execute(
        application::SetFeatureSuppressedCommand{
            last_cut_id, session.document().revision(), false});
    CHECK(unsuppressed.ok());
    const auto rebuilt =
        part::evaluatePart(session.document(), kernel);
    CHECK(rebuilt.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(rebuilt.body_solid && rebuilt.current_topology);
    CHECK(rebuilt.current_topology->complete());
    CHECK(rebuilt.body_solid.get() != result.body_solid.get());
    const auto stale_native = kernel.queryFaceBoundary(
        rebuilt.body_solid, *old_scoped);
    CHECK(stale_native.status ==
          kernel::FaceBoundaryStatus::provider_mismatch);

    std::size_t rebuilt_two_hole_admissions = 0U;
    for (const auto& face : rebuilt.current_topology->faces) {
        const auto admitted = part::inspectMaterialFaceBoundary(
            rebuilt.features.back(), face.runtime_token, kernel);
        if (!admitted.ok() || admitted.wires.size() != 3U) {
            continue;
        }
        std::size_t outer_count = 0U;
        std::size_t material_members = 0U;
        for (const auto& wire : admitted.wires) {
            if (wire.outer) ++outer_count;
            for (const auto& member : wire.edges) {
                CHECK(member.valid());
                CHECK(member.reference.stage ==
                      rebuilt.current_topology->stage);
                ++material_members;
            }
        }
        CHECK(outer_count == 1U);
        CHECK(material_members == 6U);
        ++rebuilt_two_hole_admissions;
    }
    CHECK(rebuilt_two_hole_admissions > 0U);
    std::cout << "PG01D_D1_SUPPRESS_REBUILD_FACE_PASS"
              << " refreshed_strict_two_hole_faces="
              << rebuilt_two_hole_admissions << '\n';
}

// PG-01D D3 negative boundary seam: keep the real native Body, Face
// catalog, strict semantic mapping and OCCT provider as the baseline.
// Inject only a malformed/read-failing provider *answer* to prove Part
// does not accept a guessed or partially healed boundary. No production
// fault injection and no fabricated authored CAD identities.
class Pg01dFaultedBoundaryProvider final
    : public kernel::IFaceBoundaryQuery {
public:
    enum class Fault {
        none,
        bind_unavailable,
        query_failure,
        missing_outer,
        duplicate_outer,
        missing_signed_vertex,
        bogus_edge,
        duplicated_material_edge,
    };

    Pg01dFaultedBoundaryProvider(
        kernel_occt::OcctSolidModelingKernel& native, Fault fault)
        : native_{native}, fault_{fault} {}

    std::optional<kernel::ScopedBoundaryFace> bindFaceToBody(
        kernel::RuntimeSolidHandle body,
        kernel::RuntimeFaceToken face) noexcept override {
        if (fault_ == Fault::bind_unavailable ||
            !native_.bindFaceToBody(body, face)) {
            return std::nullopt;
        }
        return makeScopedFace(std::move(body), face);
    }

    kernel::FaceBoundaryResult queryFaceBoundary(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedBoundaryFace& scoped) noexcept override {
        using Status = kernel::FaceBoundaryStatus;
        if (!body || !scoped.valid() ||
            body.get() != scoped.sourceBody().get()) {
            return {Status::provider_mismatch, {}};
        }
        if (fault_ == Fault::query_failure) {
            return {Status::provider_failure, {}};
        }
        const auto real = native_.bindFaceToBody(
            body, scoped.face());
        if (!real) return {Status::face_unavailable, {}};
        auto result = native_.queryFaceBoundary(body, *real);
        if (!result.ok() || fault_ == Fault::none) {
            return result;
        }
        switch (fault_) {
        case Fault::missing_outer:
            for (auto& wire : result.wires) wire.outer = false;
            break;
        case Fault::duplicate_outer: {
            const auto outer = std::find_if(
                result.wires.begin(), result.wires.end(),
                [](const auto& wire) { return wire.outer; });
            if (outer != result.wires.end()) {
                result.wires.push_back(*outer);
            }
            break;
        }
        case Fault::missing_signed_vertex:
            // A malformed native half-populated Vertex occurrence
            // cannot certify any boundary; never classify as geometric
            // curve Unsupported.
            result.wires.front().edges.front().start_vertex.reset();
            break;
        case Fault::bogus_edge:
            result.wires.front().edges.front().edge =
                kernel::RuntimeEdgeToken{
                    std::numeric_limits<std::uint64_t>::max()};
            break;
        case Fault::duplicated_material_edge:
            result.wires.front().edges.push_back(
                result.wires.front().edges.front());
            break;
        case Fault::none:
        case Fault::bind_unavailable:
        case Fault::query_failure:
            break;
        }
        return result;
    }

    // Manual Face Boundary uses this separate bounded any-surface
    // observation. For this deliberately planar fault fixture, return
    // the SAME forged native wire answer as strict Planar Face; this
    // never creates synthetic authorable Part references.
    kernel::FaceBoundaryResult queryFaceBoundaryAnySurface(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedBoundaryFace& scoped) noexcept override {
        return queryFaceBoundary(std::move(body), scoped);
    }

private:
    kernel_occt::OcctSolidModelingKernel& native_;
    Fault fault_;
};

void verifyPg01dMalformedBoundaryFailClosed(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto session = makeBaseSession(kernel);
    const auto before = session.document().state();
    const auto revision = session.document().revision();
    const auto undo = session.undoDepth();
    const auto result = part::evaluatePart(
        session.document(), kernel);
    CHECK(result.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(result.current_topology);
    CHECK(result.current_topology->complete());
    CHECK(!result.features.empty());
    const auto& stage = result.features.back();
    std::optional<kernel::RuntimeFaceToken> source_face;
    for (const auto& face : result.current_topology->faces) {
        const auto admitted = part::inspectMaterialFaceBoundary(
            stage, face.runtime_token, kernel);
        if (admitted.ok() && admitted.wires.size() == 1U &&
            admitted.wires.front().outer &&
            admitted.wires.front().edges.size() == 4U) {
            source_face = face.runtime_token;
            break;
        }
    }
    CHECK(source_face);
    const auto valid = part::inspectMaterialFaceBoundary(
        stage, *source_face, kernel);
    CHECK(valid.ok());
    CHECK(valid.wires.size() == 1U);
    CHECK(valid.wires.front().edges.size() == 4U);

    // No guessed Face when the source token or the entire stage is
    // unavailable, even though some planar Surface may still exist.
    const auto no_token = part::inspectMaterialFaceBoundary(
        stage, kernel::RuntimeFaceToken{}, kernel);
    CHECK(no_token.status ==
          part::MaterialFaceBoundaryStatus::face_unavailable);
    const auto alien_token = part::inspectMaterialFaceBoundary(
        stage,
        kernel::RuntimeFaceToken{
            std::numeric_limits<std::uint64_t>::max()},
        kernel);
    CHECK(alien_token.status ==
          part::MaterialFaceBoundaryStatus::face_unavailable);
    auto missing_catalog = stage;
    missing_catalog.result_topology.reset();
    CHECK(part::inspectMaterialFaceBoundary(
        missing_catalog, *source_face, kernel).status ==
        part::MaterialFaceBoundaryStatus::invalid_stage);
    auto carrier_only = stage;
    CHECK(carrier_only.result_topology);
    const auto face_record = std::find_if(
        carrier_only.result_topology->faces.begin(),
        carrier_only.result_topology->faces.end(),
        [&](const auto& item) {
            return item.runtime_token == *source_face;
        });
    CHECK(face_record != carrier_only.result_topology->faces.end());
    face_record->semantic_address.reset();
    CHECK(part::inspectMaterialFaceBoundary(
        carrier_only, *source_face, kernel).status ==
        part::MaterialFaceBoundaryStatus::face_not_strict);

    using Fault = Pg01dFaultedBoundaryProvider::Fault;
    const auto expect_failure = [&](Fault fault,
                                    part::MaterialFaceBoundaryStatus expected) {
        Pg01dFaultedBoundaryProvider provider{kernel, fault};
        const auto admitted = part::inspectMaterialFaceBoundary(
            stage, *source_face, provider);
        CHECK(!admitted.ok());
        if (admitted.status != expected) {
            std::cerr
                << "PG01D_NEGATIVE_ACTUAL_STATUS fault="
                << static_cast<int>(fault)
                << " actual=" << static_cast<int>(admitted.status)
                << " expected=" << static_cast<int>(expected)
                << '\n';
        }
        CHECK(admitted.status == expected);
        CHECK(admitted.wires.empty());
        CHECK(!admitted.bounded_face);
    };
    expect_failure(
        Fault::bind_unavailable,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::query_failure,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::missing_outer,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::duplicate_outer,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::bogus_edge,
        part::MaterialFaceBoundaryStatus::material_edge_unavailable);
    // Since the native provider supplies exact directed Vertices,
    // appended repeated EdgeUse invalidates wire closure itself. It
    // fails at native topology accounting BEFORE semantic Edge mapping.
    expect_failure(
        Fault::duplicated_material_edge,
        part::MaterialFaceBoundaryStatus::
            native_boundary_unavailable);

    // Later Owner D2: manual chosen-Face admission does not demand
    // a persistent strict Face address, but EVERY bounded native
    // occurrence remains integrity-checked. Invalid Face and wire
    // evidence cannot be reinterpreted as geometric Unsupported.
    const auto manual_control =
        part::inspectSelectedFaceBoundary(
            stage, *source_face, kernel);
    CHECK(manual_control.ok());
    CHECK(manual_control.bounded_face == *source_face);
    CHECK(!part::inspectSelectedFaceBoundary(
        stage, kernel::RuntimeFaceToken{}, kernel).ok());
    CHECK(!part::inspectSelectedFaceBoundary(
        missing_catalog, *source_face, kernel).ok());
    const auto carrier_only_manual =
        part::inspectSelectedFaceBoundary(
            carrier_only, *source_face, kernel);
    CHECK(carrier_only_manual.ok());
    CHECK(carrier_only_manual.wires ==
          manual_control.wires);

    // A complete but SEMANTICALLY UNSUPPORTED material Edge must not be
    // skipped like a geometrically Unsupported projected curve. Make
    // the failure observable at the exact native wire member, without
    // copying any Owner Part, document IDs or private fixture geometry.
    auto unsupported_edge_stage = stage;
    CHECK(unsupported_edge_stage.result_topology);
    const auto selected_edge =
        manual_control.wires.front().edges.front().native_use.edge;
    auto& records = unsupported_edge_stage.result_topology->edges;
    const auto unsupported_record = std::find_if(
        records.begin(), records.end(),
        [selected_edge](const auto& item) {
            return item.runtime_token == selected_edge;
        });
    CHECK(unsupported_record != records.end());
    CHECK(unsupported_record->accounting_class ==
          part::TopologyAccountingClass::referenceable);
    unsupported_record->accounting_class =
        part::TopologyAccountingClass::semantically_unsupported;
    unsupported_record->referenceability =
        kernel::ReferenceStatus::unsupported;
    unsupported_record->curve_candidates.clear();
    CHECK(unsupported_edge_stage.result_topology->complete());
    const auto semantic_refusal =
        part::inspectSelectedFaceBoundary(
            unsupported_edge_stage, *source_face, kernel);
    CHECK(semantic_refusal.status ==
          part::MaterialFaceBoundaryStatus::
              material_edge_unavailable);
    CHECK(!semantic_refusal.ok());
    CHECK(semantic_refusal.wires.empty());
    CHECK(semantic_refusal.rejected_edge);
    CHECK(semantic_refusal.rejected_edge->edge == selected_edge);
    CHECK(semantic_refusal.rejected_edge->wire_index == 0U);
    CHECK(semantic_refusal.rejected_edge->edge_index == 0U);
    CHECK(semantic_refusal.rejected_edge->kind ==
          part::SelectedFaceBoundaryRejectKind::
              uncertified_material_edge);
    CHECK(semantic_refusal.rejected_edge->accounting_class ==
          part::TopologyAccountingClass::semantically_unsupported);
    CHECK(semantic_refusal.rejected_edge->referenceability ==
          kernel::ReferenceStatus::unsupported);
    CHECK(semantic_refusal.rejected_edge->curve_candidate_count ==
          0U);
    CHECK(semantic_refusal.rejected_edge->curve_family_realizations ==
          0U);
    CHECK(semantic_refusal.rejected_edge->
          certified_semantic_endpoints <=
          semantic_refusal.rejected_edge->incident_vertices);
    CHECK(!semantic_refusal.rejected_edge->periodic_seam);
    CHECK(!semantic_refusal.rejected_edge->representation_partition);

    const auto expect_selected_failure = [&](
        Fault fault, part::MaterialFaceBoundaryStatus expected) {
        Pg01dFaultedBoundaryProvider provider{kernel, fault};
        const auto rejected =
            part::inspectSelectedFaceBoundary(
                stage, *source_face, provider);
        CHECK(!rejected.ok());
        CHECK(rejected.status == expected);
        CHECK(rejected.wires.empty());
        CHECK(!rejected.bounded_face.valid());
    };
    expect_selected_failure(
        Fault::bind_unavailable,
        part::MaterialFaceBoundaryStatus::
            native_boundary_unavailable);
    expect_selected_failure(
        Fault::query_failure,
        part::MaterialFaceBoundaryStatus::
            native_boundary_unavailable);
    expect_selected_failure(
        Fault::missing_outer,
        part::MaterialFaceBoundaryStatus::
            native_boundary_unavailable);
    expect_selected_failure(
        Fault::duplicate_outer,
        part::MaterialFaceBoundaryStatus::
            native_boundary_unavailable);
    expect_selected_failure(
        Fault::bogus_edge,
        part::MaterialFaceBoundaryStatus::
            material_edge_unavailable);
    {
        Pg01dFaultedBoundaryProvider provider{
            kernel, Fault::bogus_edge};
        const auto rejected =
            part::inspectSelectedFaceBoundary(
                stage, *source_face, provider);
        CHECK(rejected.rejected_edge);
        CHECK(rejected.rejected_edge->kind ==
              part::SelectedFaceBoundaryRejectKind::
                  missing_catalog_edge);
        CHECK(rejected.rejected_edge->edge.value ==
              std::numeric_limits<std::uint64_t>::max());
        CHECK(rejected.rejected_edge->wire_index == 0U);
        CHECK(rejected.rejected_edge->edge_index == 0U);
        CHECK(!rejected.rejected_edge->accounting_class);
        CHECK(!rejected.rejected_edge->referenceability);
        CHECK(rejected.rejected_edge->curve_family_realizations ==
              0U);
        CHECK(rejected.rejected_edge->incident_vertices == 0U);
        CHECK(rejected.rejected_edge->
              certified_semantic_endpoints == 0U);
    }
    expect_selected_failure(
        Fault::missing_signed_vertex,
        part::MaterialFaceBoundaryStatus::
            native_boundary_unavailable);
    {
        Pg01dFaultedBoundaryProvider provider{
            kernel, Fault::duplicated_material_edge};
        const auto rejected =
            part::inspectSelectedFaceBoundary(
                stage, *source_face, provider);
        CHECK(!rejected.ok());
        CHECK(rejected.status ==
                  part::MaterialFaceBoundaryStatus::
                      native_boundary_unavailable ||
              rejected.status ==
                  part::MaterialFaceBoundaryStatus::
                      material_edge_unavailable);
    }
    std::cout
        << "PG01D_BOUNDARY_REJECTION_DETAIL_PASS"
        << " native_member_identified=1"
        << " semantically_unsupported_refused=1"
        << " bogus_catalog_source_refused=1"
        << " no_partial_authoring=1"
        << '\n';
    std::cout
        << "PG01D_MANUAL_FACE_MALFORMED_E0_PASS"
        << " faults=7"
        << " no_strict_face_address_required=1"
        << " unknown_source_skippable=0"
        << '\n';

    Pg01dFaultedBoundaryProvider passthrough{
        kernel, Fault::none};
    const auto control = part::inspectMaterialFaceBoundary(
        stage, *source_face, passthrough);
    CHECK(control.ok());
    CHECK(control.wires.size() == valid.wires.size());
    CHECK(control.wires.front().edges.size() ==
          valid.wires.front().edges.size());
    for (std::size_t i = 0U; i < valid.wires.front().edges.size(); ++i) {
        CHECK(control.wires.front().edges[i].reference ==
              valid.wires.front().edges[i].reference);
    }
    CHECK(session.document().state() == before);
    CHECK(session.document().revision() == revision);
    CHECK(session.undoDepth() == undo);
    std::cout << "PG01D_D3_MALFORMED_BOUNDARY_FAIL_CLOSED_PASS"
              << " native_strict_source=1"
              << " provider_fault_variants=6"
              << " invalid_scene_or_carrier_variants=4"
              << " duplicate_material_rejected=1"
              << " document_unchanged=1\\n";
}

// Bounded independent native probe for the Owner's Add + Add + Chamfer
// manual Face rejection. Reuse the already-sanitized two-Add test Part;
// never copy the Owner's private v15 document or its authored identifiers.
// This is classification EVIDENCE, not an assertion that every resulting
// Chamfer boundary must have a durable MaterialEdgeReference.
void probePg01dChamferedAddFaceAdmission(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto fixture =
        std::filesystem::path{__FILE__}.parent_path() /
        "fixtures" / "pm05f_r2_part008_sanitized.ss2part";
    const part::PartDocumentStore store;
    auto loaded = loadOwnerFixtureV2ForTest(store, fixture);
    CHECK(loaded.ok());
    application::DocumentSession session{
        {}, std::move(*loaded.document)};
    CHECK(session.document().body().features.size() == 2U);
    const auto base = part::evaluatePart(
        session.document(), kernel);
    CHECK(base.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(base.current_topology);
    CHECK(base.current_topology->complete());

    const auto before_revision = session.document().revision();
    bool chamfer_committed = false;
    for (const auto& edge : base.current_topology->edges) {
        const auto reference =
            part::authorMaterialEdgeReference(
                *base.current_topology, edge.runtime_token);
        if (!reference.ok() || !reference.reference) continue;
        const auto committed = session.execute(
            application::CreateChamferFeatureCommand{
                {*reference.reference},
                session.document().revision(),
                core::LengthValue{1.0},
                "PG01D sanitized Add/Chamfer boundary probe"},
            kernel);
        if (committed.ok()) {
            chamfer_committed = true;
            break;
        }
        CHECK(session.document().revision() == before_revision);
    }
    CHECK(chamfer_committed);
    CHECK(session.document().body().features.size() == 3U);
    const auto evaluated = part::evaluatePart(
        session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology &&
          evaluated.current_topology->complete());
    CHECK(evaluated.features.size() == 3U);
    const auto& catalog = *evaluated.current_topology;

    std::size_t referenceable_faces = 0U;
    std::size_t admitted_faces = 0U;
    std::size_t blocked_material_faces = 0U;
    std::size_t blocked_other_faces = 0U;
    std::size_t shared_carrier_pairs = 0U;
    for (std::size_t i = 0U; i < catalog.faces.size(); ++i) {
        const auto& face = catalog.faces[i];
        if (face.accounting_class !=
                part::TopologyAccountingClass::referenceable) {
            continue;
        }
        ++referenceable_faces;
        for (std::size_t j = i + 1U;
             j < catalog.faces.size(); ++j) {
            const auto& other = catalog.faces[j];
            for (const auto& carrier : face.surface_candidates) {
                if (std::find(
                        other.surface_candidates.begin(),
                        other.surface_candidates.end(),
                        carrier) != other.surface_candidates.end()) {
                    ++shared_carrier_pairs;
                }
            }
        }
        const auto admission =
            part::inspectSelectedFaceBoundary(
                evaluated.features.back(),
                face.runtime_token, kernel);
        if (admission.ok()) {
            ++admitted_faces;
            continue;
        }
        if (admission.status ==
                part::MaterialFaceBoundaryStatus::
                    material_edge_unavailable) {
            ++blocked_material_faces;
            const auto* failure = admission.rejected_edge
                ? &*admission.rejected_edge : nullptr;
            std::cout
                << "PG01D_ADD_CHAMFER_FACE_BLOCKED"
                << " face=" << face.runtime_token.value
                << " failed_edge="
                << (failure ? failure->edge.value : 0U)
                << " kind="
                << (failure ? static_cast<int>(failure->kind) : -1)
                << " accounting="
                << (failure && failure->accounting_class
                    ? static_cast<int>(*failure->accounting_class)
                    : -1)
                << " reference="
                << (failure && failure->referenceability
                    ? static_cast<int>(*failure->referenceability)
                    : -1)
                << " candidates="
                << (failure ? failure->curve_candidate_count : 0U)
                << " representation_partition="
                << (failure && failure->representation_partition)
                << '\n';
        } else {
            ++blocked_other_faces;
        }
    }
    CHECK(referenceable_faces > 0U);
    std::cout
        << "PG01D_ADD_CHAMFER_BOUNDARY_PROBE_PASS"
        << " referenceable_faces=" << referenceable_faces
        << " admitted=" << admitted_faces
        << " material_blocked=" << blocked_material_faces
        << " other_blocked=" << blocked_other_faces
        << " same_carrier_pairs=" << shared_carrier_pairs
        << " private_document_committed=0"
        << '\n';
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    const bool s3a_only =
        argc == 2 && std::string_view{argv[1]} == "--s3a";
    const bool pg01c_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01c";
    const bool ui01_p0_only =
        argc == 2 && std::string_view{argv[1]} == "--ui01-p0";
    const bool pg01d_ui_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01d-ui";
    const bool pg01d_d0_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01d-d0";
    const bool pg01d_negative_only =
        argc == 2 &&
        std::string_view{argv[1]} == "--pg01d-negative";
    kernel_occt::OcctSolidModelingKernel kernel;
    if (pg01d_negative_only) {
        verifyPg01dMalformedBoundaryFailClosed(kernel);
        probePg01dChamferedAddFaceAdmission(kernel);
        return EXIT_SUCCESS;
    }
    if (pg01d_d0_only) {
        // PG-01D's real OCCT/Part topology proof must remain isolated
        // from the unrelated, cursor/timing-sensitive PG-01C GUI test.
        // Reuse this compiled native test target without Viewer actions.
        verifyPg01dNativeStrictFaceAndMaterialCatalog(kernel);
        verifyPg01dNativeTwoHoleFaceBoundary(kernel);
        std::cout << "PG01D_D0_D1_ISOLATED_NATIVE_PASS"
                  << std::endl;
        return EXIT_SUCCESS;
    }
    viewer_qt_occt::QtOcctViewerWidget* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            viewport =
                new viewer_qt_occt::QtOcctViewerWidget(parent);
            return ui::ViewportSurface{viewport, viewport};
        },
        &kernel};
    CHECK(viewport != nullptr);
    auto session = makeBaseSession(kernel);
    CHECK(workbench.activateDocument(&session, {}));
    workbench.resize(1400, 900);
    workbench.show();
    int result = EXIT_FAILURE;

    QTimer::singleShot(150, &app, [&] {
        auto* tree = workbench.findChild<QTreeWidget*>();
        auto* finish = workbench.findChild<QPushButton*>(
            QStringLiteral("edgeFeatureFinishButton"));
        auto* cancel = workbench.findChild<QPushButton*>(
            QStringLiteral("edgeFeatureCancelButton"));
        auto* edit = workbench.findChild<QPushButton*>(
            QStringLiteral("featureEditExtrudeButton"));
        auto* label = workbench.findChild<QLabel*>(
            QStringLiteral("edgeFeatureSelectionLabel"));
        CHECK(tree && finish && cancel && edit && label);
        CHECK(viewport->isVisible());

        if (s3a_only || (!pg01c_only && !pg01d_ui_only &&
                         !ui01_p0_only)) {
        // RG-02B1 P4-S3A: positive *native* Workbench gesture witness.
        // The camera/world points only aim Qt mouse clicks; they never
        // enter authoring APIs or stand in for semantic Part provenance.
        // This additive block runs in the registered full-only native
        // Workbench suite and via --s3a for focused diagnosis.
        {
            auto s3a = makeBaseSession(kernel);
            CHECK(workbench.activateDocument(&s3a, {}));
            QApplication::processEvents();
            auto* datum_tool = workbench.findChild<QPushButton*>(
                QStringLiteral("datumPlaneToolButton"));
            auto* constructor = workbench.findChild<QComboBox*>(
                QStringLiteral("datumPlaneConstructorCombo"));
            auto* datum_result = workbench.findChild<QLabel*>(
                QStringLiteral("datumPlaneResultLabel"));
            auto* datum_finish = workbench.findChild<QPushButton*>(
                QStringLiteral("datumPlaneFinishButton"));
            auto* datum_cancel = workbench.findChild<QPushButton*>(
                QStringLiteral("datumPlaneCancelButton"));
            CHECK(datum_tool && constructor && datum_result &&
                  datum_finish && datum_cancel);
            CHECK(datum_tool->isEnabled());
            const auto current = part::evaluatePart(
                s3a.document(), kernel);
            CHECK(current.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(current.body_solid);
            CHECK(current.current_topology);
            CHECK(current.current_topology->complete());
            const auto geometry =
                kernel.bodyPresentation(current.body_solid);
            CHECK(geometry.ok());
            CHECK(geometry.body.edges.size() >= 4U);
            const auto revision_before = s3a.document().revision();
            const auto undo_before = s3a.undoDepth();
            const auto state_before = s3a.document().state();
            const auto activate_datum = workbench.submitCadInput(
                "DATUMPLANE", workbench.cadInputContextGeneration());
            CHECK(activate_datum.accepted);
            constructor->setCurrentIndex(1);
            QApplication::processEvents();
            CHECK(constructor->currentIndex() == 1);
            CHECK(!datum_finish->isEnabled());
            CHECK(s3a.document().state() == state_before);
            CHECK(s3a.undoDepth() == undo_before);

            // Select an actually projected, queryable current Body Edge
            // away from both terminal vertices and its MID marker.
            bool selected_edge = false;
            QPoint certified_mid_pixel;
            viewer::Point3 certified_edge_world{0.0, 0.0, 0.0};
            viewer::Point3 certified_mid_world{0.0, 0.0, 0.0};
            auto certified_view =
                viewer::StandardView::top_front_right;
            const auto choosePixel = [viewport](
                viewer::Point3 world) -> std::optional<QPoint> {
                const auto projection =
                    viewport->projectWorldPoint(world);
                if (!projection || !std::isfinite(projection->x) ||
                    !std::isfinite(projection->y))
                    return std::nullopt;
                const QPoint pixel{
                    static_cast<int>(std::lround(projection->x)),
                    static_cast<int>(std::lround(projection->y))};
                return viewport->rect().contains(pixel)
                    ? std::optional<QPoint>{pixel}
                    : std::nullopt;
            };
            for (const auto view : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_right}) {
                if (selected_edge) break;
                CHECK(viewport->setStandardView(view));
                viewport->fitAll();
                QApplication::processEvents();
                for (const auto& edge : geometry.body.edges) {
                    if (edge.points.size() < 2U) continue;
                    const auto& a = edge.points.front();
                    const auto& b = edge.points.back();
                    const viewer::Point3 quarter{
                        0.75 * a.x + 0.25 * b.x,
                        0.75 * a.y + 0.25 * b.y,
                        0.75 * a.z + 0.25 * b.z};
                    const viewer::Point3 midpoint{
                        0.5 * (a.x + b.x),
                        0.5 * (a.y + b.y),
                        0.5 * (a.z + b.z)};
                    const auto edge_pixel = choosePixel(quarter);
                    const auto mid_pixel = choosePixel(midpoint);
                    if (!edge_pixel || !mid_pixel ||
                        (*edge_pixel - *mid_pixel).manhattanLength() < 20)
                        continue;
                    const viewer::ViewportPoint2 probe{
                        static_cast<double>(edge_pixel->x()),
                        static_cast<double>(edge_pixel->y())};
                    const auto hit = viewport->queryBodyTopology(
                        probe,
                        viewer::BodyTopologyPickFilter{
                            false, true, false});
                    if (!hit.valid() || !hit.completed ||
                        hit.candidates.empty())
                        continue;
                    QTest::mouseMove(viewport, *edge_pixel);
                    QTest::mouseClick(
                        viewport, Qt::LeftButton,
                        Qt::NoModifier, *edge_pixel);
                    QApplication::processEvents();
                    if (datum_result->text().contains(
                            QStringLiteral("Click the highlighted MID"))) {
                        selected_edge = true;
                        certified_mid_pixel = *mid_pixel;
                        certified_edge_world = quarter;
                        certified_mid_world = midpoint;
                        certified_view = view;
                        break;
                    }
                    // Failed native gesture is not an accepted source.
                    // Reset the incomplete draft, retaining zero changes.
                    constructor->setCurrentIndex(0);
                    constructor->setCurrentIndex(1);
                    QApplication::processEvents();
                }
            }
            CHECK(selected_edge);
            CHECK(!datum_finish->isEnabled());
            CHECK(s3a.document().state() == state_before);
            QTest::mouseMove(viewport, certified_mid_pixel);
            QTest::mouseClick(
                viewport, Qt::LeftButton,
                Qt::NoModifier, certified_mid_pixel);
            QApplication::processEvents();
            CHECK(datum_result->text().contains(
                QStringLiteral("Resolved MID")));
            CHECK(datum_finish->isEnabled());
            CHECK(s3a.document().edgePointDatumPlanes().empty());
            CHECK(s3a.undoDepth() == undo_before);
            CHECK(workbench.submitCadInput(
                "REVERSE",
                workbench.cadInputContextGeneration()).accepted);
            CHECK(workbench.submitCadInput(
                "FINISH",
                workbench.cadInputContextGeneration()).accepted);
            QApplication::processEvents();
            CHECK(s3a.document().edgePointDatumPlanes().size() == 1U);
            CHECK(s3a.document().revision() != revision_before);
            CHECK(s3a.undoDepth() == undo_before + 1U);
            const auto authored =
                s3a.document().edgePointDatumPlanes().front();
            CHECK(std::holds_alternative<part::SelectedMaterialEdgeMid>(
                authored.source));
            CHECK(authored.orientation.reverse);
            const auto evaluated =
                part::evaluatePart(s3a.document(), kernel);
            CHECK(evaluated.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto datums =
                part::evaluateDatums(s3a.document(), evaluated, kernel);
            const auto* evaluated_datum = datums.find(authored.id);
            CHECK(evaluated_datum && evaluated_datum->valid());
            CHECK(evaluated_datum->frame.has_value());

            CHECK(s3a.undo().changed);
            CHECK(s3a.document().edgePointDatumPlanes().empty());
            CHECK(s3a.redo().changed);
            CHECK(s3a.document().edgePointDatumPlanes().size() == 1U);
            CHECK(*s3a.document().findEdgePointDatumPlane(authored.id) ==
                  authored);

            QTemporaryDir s3a_scratch;
            CHECK(s3a_scratch.isValid());
            const std::filesystem::path native_path =
                std::filesystem::path{
                    s3a_scratch.path().toStdWString()} /
                "s3a-real-workbench-mid.ss2part";
            part::PartDocumentStore native_store;
            CHECK(native_store.createNew(
                native_path, s3a.document()).ok());
            const auto reopened = native_store.load(native_path);
            CHECK(reopened.ok() && reopened.document);
            const auto* persisted =
                reopened.document->findEdgePointDatumPlane(authored.id);
            CHECK(persisted && *persisted == authored);
            kernel_occt::OcctSolidModelingKernel cold_kernel;
            const auto cold_part =
                part::evaluatePart(*reopened.document, cold_kernel);
            CHECK(cold_part.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto cold_datums = part::evaluateDatums(
                *reopened.document, cold_part, cold_kernel);
            const auto* cold = cold_datums.find(authored.id);
            CHECK(cold && cold->valid() && cold->frame.has_value());

            // Independent second Part/Workbench session: point-first
            // really means a Qt MID anchor click before an explicit
            // selected material Edge (no implicit source commit).
            auto s3a_mid_first = makeBaseSession(kernel);
            CHECK(workbench.activateDocument(&s3a_mid_first, {}));
            QApplication::processEvents();
            CHECK(viewport->setStandardView(certified_view));
            viewport->fitAll();
            QApplication::processEvents();
            const auto point_edge_pixel =
                choosePixel(certified_edge_world);
            const auto point_mid_pixel =
                choosePixel(certified_mid_world);
            CHECK(point_edge_pixel && point_mid_pixel);
            const auto point_state =
                s3a_mid_first.document().state();
            const auto point_undo =
                s3a_mid_first.undoDepth();
            CHECK(workbench.submitCadInput(
                "DATUM PLANE",
                workbench.cadInputContextGeneration()).accepted);
            constructor->setCurrentIndex(1);
            QApplication::processEvents();
            CHECK(constructor->currentIndex() == 1);
            // Dispatch synchronous Qt mouseMove to exercise the real
            // Workbench hover path independently of the global OS cursor.
            sendNativeWorkbenchHover(
                *viewport, viewport->rect().topLeft() + QPoint{3, 3});
            const auto s3a_edge_probe = viewport->queryBodyTopology(
                viewer::ViewportPoint2{double(point_edge_pixel->x()),
                                       double(point_edge_pixel->y())},
                viewer::BodyTopologyPickFilter{false,true,false});
            CHECK(s3a_edge_probe.valid() &&
                  s3a_edge_probe.completed &&
                  !s3a_edge_probe.candidates.empty());
            sendNativeWorkbenchHover(*viewport, *point_edge_pixel);
            sendNativeWorkbenchHover(*viewport, *point_mid_pixel);
            QTest::mouseClick(
                viewport, Qt::LeftButton,
                Qt::NoModifier, *point_mid_pixel);
            QApplication::processEvents();
            CHECK(datum_result->text().contains(
                QStringLiteral("MID selected first")));
            CHECK(!datum_finish->isEnabled());
            CHECK(!workbench.submitCadInput(
                "FINISH",
                workbench.cadInputContextGeneration()).accepted);
            CHECK(s3a_mid_first.document().state() == point_state);
            CHECK(s3a_mid_first.undoDepth() == point_undo);
            sendNativeWorkbenchHover(*viewport, *point_edge_pixel);
            QTest::mouseClick(
                viewport, Qt::LeftButton,
                Qt::NoModifier, *point_edge_pixel);
            QApplication::processEvents();
            CHECK(datum_finish->isEnabled());
            CHECK(workbench.submitCadInput(
                "", workbench.cadInputContextGeneration()).accepted);
            CHECK(s3a_mid_first.document().edgePointDatumPlanes().size() ==
                  1U);
            CHECK(std::holds_alternative<part::SelectedMaterialEdgeMid>(
                s3a_mid_first.document()
                    .edgePointDatumPlanes().front().source));
            CHECK(s3a_mid_first.undoDepth() == point_undo + 1U);
            std::cout << "RG02B1_S3A_NATIVE_MID_POINT_FIRST_PASS"
                      << " same_current_edge=1"
                      << " incomplete_finish_refused=1"
                      << " empty_enter_commits=1\n";
            CHECK(workbench.activateDocument(&s3a, {}));
            QApplication::processEvents();


            // S3A-F01/F03: real native shared-corner END acquisition.
            // Each chosen box terminal belongs to three incident current
            // material Edges. The actual first click chooses one Edge;
            // the second native Qt click supplies that corner, without
            // authoring whichever incident Vertex happens to hover.
            auto s3a_end_session = makeBaseSession(kernel);
            CHECK(workbench.activateDocument(&s3a_end_session, {}));
            QApplication::processEvents();
            const auto end_original =
                s3a_end_session.document().state();
            const auto end_undo_before =
                s3a_end_session.undoDepth();
            CHECK(workbench.submitCadInput(
                "DATUMPLANE",
                workbench.cadInputContextGeneration()).accepted);
            constructor->setCurrentIndex(1);
            QApplication::processEvents();
            CHECK(constructor->currentIndex() == 1);
            bool native_end_created = false;
            std::size_t shared_corner_trials = 0U;
            for (const auto view : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_right}) {
                if (native_end_created) break;
                CHECK(viewport->setStandardView(view));
                viewport->fitAll();
                QApplication::processEvents();
                for (const auto& edge : geometry.body.edges) {
                    if (edge.points.size() < 2U) continue;
                    const auto& a = edge.points.front();
                    const auto& b = edge.points.back();
                    const viewer::Point3 quarter{
                        0.75*a.x + 0.25*b.x,
                        0.75*a.y + 0.25*b.y,
                        0.75*a.z + 0.25*b.z};
                    const auto quarter_pixel = choosePixel(quarter);
                    if (!quarter_pixel) continue;
                    const viewer::ViewportPoint2 edge_probe{
                        double(quarter_pixel->x()),
                        double(quarter_pixel->y())};
                    const auto edge_hits =
                        viewport->queryBodyTopology(
                            edge_probe,
                            viewer::BodyTopologyPickFilter{
                                false,true,false});
                    if (!edge_hits.valid() || !edge_hits.completed ||
                        edge_hits.candidates.empty())
                        continue;
                    for (const auto& terminal :
                         {edge.points.front(), edge.points.back()}) {
                        const auto sameCorner = [&terminal](
                            const auto& point) {
                            return std::abs(point.x-terminal.x)<1e-7 &&
                                   std::abs(point.y-terminal.y)<1e-7 &&
                                   std::abs(point.z-terminal.z)<1e-7;
                        };
                        std::size_t incident = 0U;
                        for (const auto& adjacent : geometry.body.edges) {
                            if (adjacent.points.size()<2U) continue;
                            if (sameCorner(adjacent.points.front()) ||
                                sameCorner(adjacent.points.back()))
                                ++incident;
                        }
                        if (incident!=3U) continue;
                        const auto corner_pixel =
                            choosePixel(viewer::Point3{
                                terminal.x,terminal.y,terminal.z});
                        if (!corner_pixel ||
                            (*quarter_pixel - *corner_pixel)
                                .manhattanLength()<14)
                            continue;
                        const viewer::ViewportPoint2 vertex_probe{
                            double(corner_pixel->x()),
                            double(corner_pixel->y())};
                        const auto vertex_hits =
                            viewport->queryBodyTopology(
                                vertex_probe,
                                viewer::BodyTopologyPickFilter{
                                    false,false,true});
                        if (!vertex_hits.valid() ||
                            !vertex_hits.completed ||
                            vertex_hits.candidates.empty())
                            continue;
                        ++shared_corner_trials;
                        sendNativeWorkbenchHover(
                            *viewport,*quarter_pixel);
                        QTest::mouseClick(
                            viewport,Qt::LeftButton,
                            Qt::NoModifier,*quarter_pixel);
                        QApplication::processEvents();
                        if (!datum_result->text().contains(
                                QStringLiteral(
                                    "Click the highlighted MID"))) {
                            constructor->setCurrentIndex(0);
                            constructor->setCurrentIndex(1);
                            QApplication::processEvents();
                            continue;
                        }
                        sendNativeWorkbenchHover(
                            *viewport,*corner_pixel);
                        QTest::mouseClick(
                            viewport,Qt::LeftButton,
                            Qt::NoModifier,*corner_pixel);
                        QApplication::processEvents();
                        if (datum_result->text().contains(
                                QStringLiteral("Resolved END")) &&
                            datum_finish->isEnabled()) {
                            native_end_created = true;
                            break;
                        }
                        // No half-authored attempt may produce a Datum.
                        CHECK(s3a_end_session.document().state() ==
                              end_original);
                        CHECK(s3a_end_session.undoDepth() ==
                              end_undo_before);
                        constructor->setCurrentIndex(0);
                        constructor->setCurrentIndex(1);
                        QApplication::processEvents();
                    }
                    if (native_end_created) break;
                }
            }
            std::cerr << "RG02B1_S3A_SHARED_END_CANDIDATES="
                      << shared_corner_trials << "\n";
            CHECK(shared_corner_trials>0U);
            CHECK(native_end_created);
            CHECK(s3a_end_session.document().state() == end_original);
            CHECK(s3a_end_session.undoDepth() == end_undo_before);
            CHECK(workbench.submitCadInput(
                "FINISH",workbench.cadInputContextGeneration()).accepted);
            CHECK(s3a_end_session.document()
                      .edgePointDatumPlanes().size()==1U);
            const auto& created_end =
                s3a_end_session.document().edgePointDatumPlanes().front();
            const auto created_end_id = created_end.id;
            CHECK(std::holds_alternative<
                part::SelectedMaterialEdgeEnd>(created_end.source));
            CHECK(s3a_end_session.undoDepth()==end_undo_before+1U);
            const auto end_evaluation =
                part::evaluatePart(s3a_end_session.document(),kernel);
            const auto end_planes = part::evaluateDatums(
                s3a_end_session.document(),end_evaluation,kernel);
            const auto* end_resolved =
                end_planes.find(created_end.id);
            CHECK(end_resolved && end_resolved->valid() &&
                  end_resolved->frame.has_value());
            CHECK(s3a_end_session.undo().changed);
            CHECK(s3a_end_session.document()
                      .edgePointDatumPlanes().empty());
            CHECK(s3a_end_session.redo().changed);
            CHECK(s3a_end_session.document()
                      .findEdgePointDatumPlane(created_end_id));

            // S3A-F06/F07: dependents of a mouse-authored END Datum
            // must survive the actual Properties Edit operation and
            // native cold Save/Close/Reopen without new identities.
            const auto support =
                part::partSketchSupportForDatumPlane(created_end_id);
            CHECK(support.has_value());
            const auto dependent_sketch =
                s3a_end_session.execute(
                    application::CreatePartSketchOnSupportCommand{
                        *support,s3a_end_session.document().revision()},
                    &kernel);
            CHECK(dependent_sketch.ok() &&
                  dependent_sketch.sketch_id.has_value());
            const auto source_line = s3a_end_session.execute(
                application::AddSketchLineCommand{
                    *dependent_sketch.sketch_id,{0.0,0.0},{12.0,0.0},
                    sketch::EntityRole::construction});
            CHECK(source_line.ok() && source_line.entity_id);
            const auto dependent_axis = s3a_end_session.execute(
                application::CreateAxisCommand{
                    part::SketchLineAxisSource{
                        *dependent_sketch.sketch_id,
                        *source_line.entity_id},
                    s3a_end_session.document().revision(),
                    "Axis on native END Datum",true},kernel);
            CHECK(dependent_axis.ok() && dependent_axis.axis_id);
            CHECK(s3a_end_session.document().findSketch(
                *dependent_sketch.sketch_id));
            CHECK(s3a_end_session.document().findAxis(
                *dependent_axis.axis_id));
            const auto source_body_for_axis = part::evaluatePart(
                s3a_end_session.document(),kernel);
            const auto source_datums_for_axis = part::evaluateDatums(
                s3a_end_session.document(),source_body_for_axis,kernel);
            const auto source_axis_frame = part::resolveAxisReference(
                s3a_end_session.document(),
                part::AxisReference{part::AuthoredAxisReference{
                    *dependent_axis.axis_id}},
                &source_body_for_axis,&source_datums_for_axis);
            std::cerr << "S3A_DEPENDENT_AXIS_STATUS="
                      << static_cast<int>(source_axis_frame.status)
                      << " diagnostic="
                      << static_cast<int>(source_axis_frame.diagnostic)
                      << "\\n";
            CHECK(source_axis_frame.status ==
                  part::AxisEvaluationStatus::resolved);
            // Headless fixture commands above intentionally bypass the
            // Workbench UI mutation callbacks. Reopen the session from a
            // different active Part so the real Tree/Properties rebuilds.
            CHECK(workbench.activateDocument(&s3a, {}));
            QApplication::processEvents();
            CHECK(workbench.activateDocument(&s3a_end_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* native_end_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0).startsWith(
                        QStringLiteral("Datum Plane 1 (Edge END)"))) {
                    native_end_tree_item = *it;
                    break;
                }
            }
            if (!native_end_tree_item) {
                for (QTreeWidgetItemIterator it(tree); *it; ++it)
                    std::cerr << "S3A_END_TREE_NODE "
                              << (*it)->text(0).toStdString() << "\n";
            }
            CHECK(native_end_tree_item);
            auto* end_properties_edit =
                workbench.findChild<QPushButton*>(
                    QStringLiteral("editDatumPlanePropertyButton"));
            CHECK(end_properties_edit);
            tree->clearSelection();
            tree->setCurrentItem(native_end_tree_item);
            native_end_tree_item->setSelected(true);
            QApplication::processEvents();
            CHECK(end_properties_edit->isEnabled());
            auto* end_properties_constructor =
                workbench.findChild<QLabel*>(
                    QStringLiteral("datumPropertyConstructor"));
            auto* end_properties_source =
                workbench.findChild<QLabel*>(
                    QStringLiteral("datumPropertySource"));
            auto* end_properties_status =
                workbench.findChild<QLabel*>(
                    QStringLiteral("datumPropertyStatus"));
            auto* end_properties_id =
                workbench.findChild<QLabel*>(
                    QStringLiteral("datumPropertyIdentity"));
            CHECK(end_properties_constructor &&
                  end_properties_source &&
                  end_properties_status &&
                  end_properties_id);
            CHECK(end_properties_constructor->text() ==
                  QStringLiteral("Edge + Point"));
            CHECK(end_properties_source->text().contains(
                  QStringLiteral("Edge END")));
            CHECK(end_properties_status->text() ==
                  QStringLiteral("Resolved"));
            CHECK(end_properties_id->text() ==
                  QString::fromStdString(created_end_id.serialized()));
            bool resolved_axis_in_tree = false;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                        QStringLiteral("Axis on native END Datum")) {
                    resolved_axis_in_tree = true;
                    break;
                }
            }
            CHECK(resolved_axis_in_tree);
            const auto original_reverse =
                s3a_end_session.document()
                    .findEdgePointDatumPlane(created_end_id)
                    ->orientation.reverse;
            const auto edit_undo_before =
                s3a_end_session.undoDepth();
            end_properties_edit->click();
            QApplication::processEvents();
            CHECK(datum_finish->isEnabled());
            CHECK(workbench.submitCadInput(
                "REVERSE",
                workbench.cadInputContextGeneration()).accepted);
            CHECK(workbench.submitCadInput(
                "FINISH",
                workbench.cadInputContextGeneration()).accepted);
            CHECK(s3a_end_session.undoDepth() ==
                  edit_undo_before + 1U);
            const auto* edited_end =
                s3a_end_session.document()
                    .findEdgePointDatumPlane(created_end_id);
            CHECK(edited_end);
            CHECK(edited_end->orientation.reverse !=
                  original_reverse);
            CHECK(std::holds_alternative<
                part::SelectedMaterialEdgeEnd>(edited_end->source));
            CHECK(s3a_end_session.document().findSketch(
                *dependent_sketch.sketch_id));
            CHECK(s3a_end_session.document().findAxis(
                *dependent_axis.axis_id));
            CHECK(s3a_end_session.undo().changed);
            CHECK(s3a_end_session.document()
                      .findEdgePointDatumPlane(created_end_id)
                      ->orientation.reverse == original_reverse);
            CHECK(s3a_end_session.redo().changed);
            CHECK(s3a_end_session.document()
                      .findEdgePointDatumPlane(created_end_id)
                      ->orientation.reverse != original_reverse);

            QTemporaryDir dependent_scratch;
            CHECK(dependent_scratch.isValid());
            const std::filesystem::path dependent_path =
                std::filesystem::path{
                    dependent_scratch.path().toStdWString()} /
                "s3a-end-sketch-axis-cold.ss2part";
            part::PartDocumentStore dependent_store;
            CHECK(dependent_store.createNew(
                dependent_path,s3a_end_session.document()).ok());
            const auto reopened_dependent =
                dependent_store.load(dependent_path);
            CHECK(reopened_dependent.ok() &&
                  reopened_dependent.document);
            CHECK(reopened_dependent.document
                      ->findEdgePointDatumPlane(created_end_id));
            CHECK(reopened_dependent.document->findSketch(
                *dependent_sketch.sketch_id));
            CHECK(reopened_dependent.document->findAxis(
                *dependent_axis.axis_id));
            kernel_occt::OcctSolidModelingKernel dependent_cold;
            const auto cold_end_part =
                part::evaluatePart(
                    *reopened_dependent.document,dependent_cold);
            CHECK(cold_end_part.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto cold_end_planes =
                part::evaluateDatums(
                    *reopened_dependent.document,
                    cold_end_part,dependent_cold);
            const auto* cold_end_plane =
                cold_end_planes.find(created_end_id);
            CHECK(cold_end_plane && cold_end_plane->valid() &&
                  cold_end_plane->frame.has_value());
            const auto cold_axis_frame = part::resolveAxisReference(
                *reopened_dependent.document,
                part::AxisReference{part::AuthoredAxisReference{
                    *dependent_axis.axis_id}},
                &cold_end_part,&cold_end_planes);
            CHECK(cold_axis_frame.valid() &&
                  cold_axis_frame.status ==
                      part::AxisEvaluationStatus::resolved &&
                  cold_axis_frame.line &&
                  cold_axis_frame.line->valid());

            // S3A-F06: a *real Workbench Properties Edit* also replaces
            // the authored END source with a newly certified MID click.
            // One Edit transaction preserves DatumId, SketchId and
            // AxisId and the fresh native kernel resolves all of them.
            CHECK(workbench.activateDocument(&s3a, {}));
            QApplication::processEvents();
            CHECK(workbench.activateDocument(&s3a_end_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* end_replacement_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0).startsWith(
                        QStringLiteral("Datum Plane 1 (Edge END)"))) {
                    end_replacement_item = *it;
                    break;
                }
            }
            CHECK(end_replacement_item);
            tree->clearSelection();
            tree->setCurrentItem(end_replacement_item);
            end_replacement_item->setSelected(true);
            QApplication::processEvents();
            CHECK(end_properties_edit->isEnabled());
            const auto source_before_repick =
                s3a_end_session.document()
                    .findEdgePointDatumPlane(created_end_id)->source;
            CHECK(std::holds_alternative<
                part::SelectedMaterialEdgeEnd>(source_before_repick));
            const auto undo_before_repick =
                s3a_end_session.undoDepth();
            end_properties_edit->click();
            QApplication::processEvents();
            CHECK(datum_finish->isEnabled());
            // A dependent Sketch/Axis can project over one camera's
            // Edge quarter. Try only actual visible, strict current
            // Body Edge candidates; a Reference/Sketch click does not
            // count as re-source evidence and never bypasses Qt routing.
            std::optional<QPoint> repick_mid_pixel;
            std::size_t native_repick_probes = 0U;
            for (const auto view : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_right}) {
                if (repick_mid_pixel) break;
                CHECK(viewport->setStandardView(view));
                viewport->fitAll();
                QApplication::processEvents();
                for (const auto& edge : geometry.body.edges) {
                    if (edge.points.size() < 2U) continue;
                    const auto& a = edge.points.front();
                    const auto& b = edge.points.back();
                    const viewer::Point3 quarter{
                        0.75*a.x + 0.25*b.x,
                        0.75*a.y + 0.25*b.y,
                        0.75*a.z + 0.25*b.z};
                    const viewer::Point3 midpoint{
                        0.5*(a.x+b.x),0.5*(a.y+b.y),
                        0.5*(a.z+b.z)};
                    const auto edge_pixel = choosePixel(quarter);
                    const auto mid_pixel = choosePixel(midpoint);
                    if (!edge_pixel || !mid_pixel ||
                        (*edge_pixel-*mid_pixel).manhattanLength()<20)
                        continue;
                    const auto probe = viewport->queryBodyTopology(
                        viewer::ViewportPoint2{
                            double(edge_pixel->x()),
                            double(edge_pixel->y())},
                        viewer::BodyTopologyPickFilter{
                            false,true,false});
                    if (!probe.valid() || !probe.completed ||
                        probe.candidates.empty())
                        continue;
                    ++native_repick_probes;
                    sendNativeWorkbenchHover(*viewport,*edge_pixel);
                    QTest::mouseClick(
                        viewport,Qt::LeftButton,
                        Qt::NoModifier,*edge_pixel);
                    QApplication::processEvents();
                    if (datum_result->text().contains(
                            QStringLiteral("Click the highlighted MID"))) {
                        repick_mid_pixel = *mid_pixel;
                        break;
                    }
                    CHECK(s3a_end_session.undoDepth() ==
                          undo_before_repick);
                }
            }
            std::cerr << "S3A_NATIVE_RESOURCE_EDGE_PROBES="
                      << native_repick_probes
                      << " label=" << datum_result->text().toStdString()
                      << "\n";
            CHECK(repick_mid_pixel.has_value());
            CHECK(!datum_finish->isEnabled());
            sendNativeWorkbenchHover(*viewport,*repick_mid_pixel);
            QTest::mouseClick(
                viewport,Qt::LeftButton,
                Qt::NoModifier,*repick_mid_pixel);
            QApplication::processEvents();
            CHECK(datum_result->text().contains(
                QStringLiteral("Resolved replacement END/MID")));
            CHECK(datum_finish->isEnabled());
            CHECK(s3a_end_session.undoDepth()==undo_before_repick);
            CHECK(workbench.submitCadInput(
                "FINISH",workbench.cadInputContextGeneration()).accepted);
            CHECK(s3a_end_session.undoDepth()==
                  undo_before_repick+1U);
            const auto* repicked =
                s3a_end_session.document()
                    .findEdgePointDatumPlane(created_end_id);
            CHECK(repicked && repicked->id==created_end_id);
            CHECK(std::holds_alternative<
                part::SelectedMaterialEdgeMid>(repicked->source));
            CHECK(repicked->source!=source_before_repick);
            CHECK(s3a_end_session.document().findSketch(
                *dependent_sketch.sketch_id));
            CHECK(s3a_end_session.document().findAxis(
                *dependent_axis.axis_id));
            const auto repick_body = part::evaluatePart(
                s3a_end_session.document(),kernel);
            const auto repick_datums = part::evaluateDatums(
                s3a_end_session.document(),repick_body,kernel);
            const auto* repick_plane =
                repick_datums.find(created_end_id);
            CHECK(repick_plane && repick_plane->valid() &&
                  repick_plane->frame.has_value());
            const auto repick_axis = part::resolveAxisReference(
                s3a_end_session.document(),
                part::AxisReference{part::AuthoredAxisReference{
                    *dependent_axis.axis_id}},
                &repick_body,&repick_datums);
            CHECK(repick_axis.valid() && repick_axis.line);
            CHECK(s3a_end_session.undo().changed);
            CHECK(s3a_end_session.document()
                      .findEdgePointDatumPlane(created_end_id)->source ==
                  source_before_repick);
            CHECK(s3a_end_session.redo().changed);
            CHECK(std::holds_alternative<
                part::SelectedMaterialEdgeMid>(
                    s3a_end_session.document()
                        .findEdgePointDatumPlane(created_end_id)->source));

            QTemporaryDir replacement_scratch;
            CHECK(replacement_scratch.isValid());
            const std::filesystem::path replacement_path =
                std::filesystem::path{
                    replacement_scratch.path().toStdWString()} /
                "s3a-end-to-mid-edit-cold.ss2part";
            part::PartDocumentStore replacement_store;
            CHECK(replacement_store.createNew(
                replacement_path,s3a_end_session.document()).ok());
            const auto reopened_replacement =
                replacement_store.load(replacement_path);
            CHECK(reopened_replacement.ok() &&
                  reopened_replacement.document);
            const auto* persisted_replacement =
                reopened_replacement.document
                    ->findEdgePointDatumPlane(created_end_id);
            CHECK(persisted_replacement &&
                  std::holds_alternative<
                      part::SelectedMaterialEdgeMid>(
                      persisted_replacement->source));
            CHECK(reopened_replacement.document->findSketch(
                *dependent_sketch.sketch_id));
            CHECK(reopened_replacement.document->findAxis(
                *dependent_axis.axis_id));
            kernel_occt::OcctSolidModelingKernel replacement_cold;
            const auto replacement_part = part::evaluatePart(
                *reopened_replacement.document,replacement_cold);
            CHECK(replacement_part.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto replacement_datums = part::evaluateDatums(
                *reopened_replacement.document,
                replacement_part,replacement_cold);
            const auto* replacement_plane =
                replacement_datums.find(created_end_id);
            CHECK(replacement_plane && replacement_plane->valid() &&
                  replacement_plane->frame.has_value());
            const auto replacement_axis = part::resolveAxisReference(
                *reopened_replacement.document,
                part::AxisReference{part::AuthoredAxisReference{
                    *dependent_axis.axis_id}},
                &replacement_part,&replacement_datums);
            CHECK(replacement_axis.valid() && replacement_axis.line);
            std::cout << "RG02B1_S3A_NATIVE_END_TO_MID_EDIT_PASS"
                      << " real_qt_occt_clicks=1"
                      << " unchanged_datum_sketch_axis_ids=1"
                      << " one_undo=1"
                      << " cold_v17=1\n";
            CHECK(workbench.activateDocument(&s3a_end_session, {}));
            QApplication::processEvents();

            std::cout << "RG02B1_S3A_NATIVE_END_DEPENDENT_EDIT_PASS"
                      << " datum_id_same=1"
                      << " sketch_axis_id_same=1"
                      << " edit_undo=1"
                      << " cold_reopen_v17=1\n";
            CHECK(workbench.activateDocument(&s3a_end_session, {}));
            QApplication::processEvents();

            std::cout << "RG02B1_S3A_NATIVE_SHARED_END_PASS"
                      << " actual_qt_mouse=1"
                      << " trihedral_corner=1"
                      << " one_undo=1\n";
            CHECK(workbench.activateDocument(&s3a, {}));
            QApplication::processEvents();

            std::cout << "RG02B1_S3A_NATIVE_MID_EDGE_FIRST_PASS"
                      << " real_qt_occt_mouse=1"
                      << " part_command_undo=1"
                      << " cold_provider_save_reopen=1\n";
            CHECK(workbench.activateDocument(&session, {}));
            QApplication::processEvents();
        }
        }
        if (s3a_only) {
            result = EXIT_SUCCESS;
            workbench.close();
            app.quit();
            return;
        }

        // UI-01 G0 native RED (full-only registered native Qt/OCCT target):
        // an existing Feature Edit draft must refuse Tree Sketch and
        // Profile Edit, without mutating authored CAD.
        // Run in the ordinary native suite AND via --ui01-p0 for focused
        // characterization. No UI-01 product fix is included in this RED.
        if (ui01_p0_only ||
            (!pg01d_ui_only && !pg01c_only)) {
            auto* edit_sketch = workbench.findChild<QAction*>(
                QStringLiteral("editSketchAction"));
            auto* edit_profile = workbench.findChild<QAction*>(
                QStringLiteral("editProfileAction"));
            auto* extrude_operations = workbench.findChild<QWidget*>(
                QStringLiteral("extrudeOperationsWidget"));
            auto* extrude_cancel = workbench.findChild<QPushButton*>(
                QStringLiteral("extrudeCancelButton"));
            auto* sketch_finish = workbench.findChild<QPushButton*>(
                QStringLiteral("finishSketchButton"));
            CHECK(edit_sketch && edit_profile && extrude_operations &&
                  extrude_cancel && sketch_finish);
            CHECK(session.document().body().features.size() == 1U);
            auto* feature_item = featureTreeItem(
                *tree, session.document().body().features.front().id);
            CHECK(feature_item);
            tree->clearSelection();
            tree->setCurrentItem(feature_item);
            feature_item->setSelected(true);
            QApplication::processEvents();
            CHECK(edit->isEnabled());
            edit->click();
            QApplication::processEvents();
            CHECK(!extrude_operations->isHidden());
            CHECK(sketch_finish->isHidden());

            const auto baseline_state = session.document().state();
            const auto baseline_revision =
                session.document().revision();
            const auto baseline_undo = session.undoDepth();
            const auto baseline_dirty = session.needsSave();
            const auto sketch_key = QString::fromStdString(
                std::string{session.document().sketches().front().id.value()});
            QTreeWidgetItem* sketch_item = nullptr;
            QTreeWidgetItem* profile_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->data(0, Qt::UserRole + 42).toString() ==
                    sketch_key) {
                    sketch_item = *it;
                }
                if (!(*it)->data(0, Qt::UserRole + 43)
                         .toString().isEmpty()) {
                    profile_item = *it;
                }
            }
            CHECK(sketch_item && profile_item);
            tree->clearSelection();
            tree->setCurrentItem(sketch_item);
            sketch_item->setSelected(true);
            QApplication::processEvents();
            edit_sketch->trigger();
            QApplication::processEvents();
            // Historic G0 RED at 8948176: requestEditSketch() entered
            // Sketch despite a live Extrude draft. G1 now must refuse it.
            CHECK(sketch_finish->isHidden());
            CHECK(!extrude_operations->isHidden());
            CHECK(session.document().state() == baseline_state);
            CHECK(session.document().revision() == baseline_revision);
            CHECK(session.undoDepth() == baseline_undo);
            CHECK(session.needsSave() == baseline_dirty);

            tree->clearSelection();
            tree->setCurrentItem(profile_item);
            profile_item->setSelected(true);
            QApplication::processEvents();
            edit_profile->trigger();
            QApplication::processEvents();
            CHECK(sketch_finish->isHidden());
            CHECK(!extrude_operations->isHidden());
            CHECK(session.document().state() == baseline_state);
            CHECK(session.document().revision() == baseline_revision);
            CHECK(session.undoDepth() == baseline_undo);
            CHECK(session.needsSave() == baseline_dirty);

            extrude_cancel->click();
            QApplication::processEvents();
            CHECK(extrude_operations->isHidden());
            CHECK(sketch_finish->isHidden());
            CHECK(session.document().state() == baseline_state);
            CHECK(session.document().revision() == baseline_revision);
            CHECK(session.undoDepth() == baseline_undo);
            CHECK(session.needsSave() == baseline_dirty);

            // Legal negative control: once the Feature draft is explicitly
            // cancelled, Sketch Edit is allowed and an Axis draft can be
            // opened INSIDE that Sketch. Neither the new P0 gate nor the
            // test is allowed to prohibit this supported workflow.
            tree->clearSelection();
            tree->setCurrentItem(sketch_item);
            sketch_item->setSelected(true);
            QApplication::processEvents();
            edit_sketch->trigger();
            QApplication::processEvents();
            CHECK(!sketch_finish->isHidden());
            auto* axis_operations = workbench.findChild<QWidget*>(
                QStringLiteral("axisOperationsWidget"));
            CHECK(axis_operations);
            auto axis_reply = workbench.submitCadInput(
                "AXIS", workbench.cadInputContextGeneration());
            CHECK(axis_reply.accepted);
            QApplication::processEvents();
            CHECK(!axis_operations->isHidden());
            CHECK(!sketch_finish->isHidden());
            axis_reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(axis_reply.accepted);
            QApplication::processEvents();
            CHECK(axis_operations->isHidden());
            CHECK(!sketch_finish->isHidden());
            sketch_finish->click();
            QApplication::processEvents();
            CHECK(sketch_finish->isHidden());
            CHECK(session.document().state() == baseline_state);
            CHECK(session.document().revision() == baseline_revision);
            CHECK(session.undoDepth() == baseline_undo);
            CHECK(session.needsSave() == baseline_dirty);
            std::cout << "UI01_P0_NATIVE_EDIT_EXCLUSIVITY_PASS"
                      << " feature=Extrude"
                      << " sketch_refused=1 profile_refused=1"
                      << " dirty_revision_undo_unchanged=1"
                      << std::endl;
            // UI-01 G1: creation drafts are also exclusive. Exercise the
            // actual Workbench command dispatcher rather than fabricating
            // Feature-state fixtures. This complements the four committed
            // Feature Edit / Tree double-click cases later in this suite.
            CHECK(workbench.activateDocument(&session, {}));
            QApplication::processEvents();
            auto* revolve_operations = workbench.findChild<QWidget*>(
                QStringLiteral("revolveOperationsWidget"));
            auto* revolve_cancel = workbench.findChild<QPushButton*>(
                QStringLiteral("revolveCancelButton"));
            auto* edge_operations = workbench.findChild<QWidget*>(
                QStringLiteral("edgeFeatureOperationsWidget"));
            auto* creation_status = workbench.findChild<QLabel*>(
                QStringLiteral("workbenchStatus"));
            CHECK(revolve_operations && revolve_cancel &&
                  edge_operations && creation_status);
            const auto profile_key = QString::fromStdString(
                session.document().profiles().front().id.serialized());
            for (const char* command : {"REVOLVE", "FILLET", "CHAMFER"}) {
                const bool revolve =
                    std::string_view{command} == "REVOLVE";
                auto* operations =
                    revolve ? revolve_operations : edge_operations;
                auto* cancel_draft =
                    revolve ? revolve_cancel : cancel;
                const auto reply = workbench.submitCadInput(
                    command, workbench.cadInputContextGeneration());
                CHECK(reply.accepted);
                QApplication::processEvents();
                CHECK(!operations->isHidden());
                CHECK(sketch_finish->isHidden());
                const auto draft_state = session.document().state();
                const auto draft_revision = session.document().revision();
                const auto draft_undo = session.undoDepth();
                const bool draft_dirty = session.needsSave();

                QTreeWidgetItem* draft_sketch_item = nullptr;
                QTreeWidgetItem* draft_profile_item = nullptr;
                for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                    if ((*it)->data(0, Qt::UserRole + 42)
                            .toString() == sketch_key) {
                        draft_sketch_item = *it;
                    }
                    if ((*it)->data(0, Qt::UserRole + 43)
                            .toString() == profile_key) {
                        draft_profile_item = *it;
                    }
                }
                CHECK(draft_sketch_item && draft_profile_item);
                // A rejected entry is repeatable, does not clear the draft,
                // and never creates an extra Undo step or dirty checkpoint.
                for (int repeat = 0; repeat < 2; ++repeat) {
                    for (auto* target : {draft_sketch_item,
                                         draft_profile_item}) {
                        tree->clearSelection();
                        tree->setCurrentItem(target);
                        target->setSelected(true);
                        QApplication::processEvents();
                        if (target == draft_sketch_item) {
                            edit_sketch->trigger();
                        } else {
                            edit_profile->trigger();
                        }
                        QApplication::processEvents();
                        CHECK(sketch_finish->isHidden());
                        CHECK(!operations->isHidden());
                        CHECK(creation_status->text().contains(
                            QStringLiteral(
                                "Finish or cancel the active Feature draft")));
                        CHECK(session.document().state() == draft_state);
                        CHECK(session.document().revision() == draft_revision);
                        CHECK(session.undoDepth() == draft_undo);
                        CHECK(session.needsSave() == draft_dirty);
                    }
                }
                cancel_draft->click();
                QApplication::processEvents();
                CHECK(operations->isHidden());
                CHECK(session.document().state() == draft_state);
                CHECK(session.document().revision() == draft_revision);
                CHECK(session.undoDepth() == draft_undo);
                CHECK(session.needsSave() == draft_dirty);
                std::cout << "UI01_P0_CREATION_DRAFT_GUARD_PASS"
                          << " feature=" << command
                          << " repeated_denials=4"
                          << " authored_state_unchanged=1" << std::endl;
            }
            if (ui01_p0_only) {
                result = EXIT_SUCCESS;
                workbench.close();
                app.quit();
                return;
            }
        }
        if (!pg01d_ui_only && !pg01c_only) {
            // Edge Picking R1 G1 — exactly the untouched Owner native v16/v2
            // Part, unlike the auxiliary synthetic viewer visibility RED.
            // This baseline proves real Part/OCCT evaluation and logs Body
            // query coverage across camera rotations. A visible-on-screen
            // query miss by itself is NOT yet proof of a front-visible Edge.
            const auto fixture =
                std::filesystem::path{__FILE__}.parent_path() /
                "fixtures" /
                "edge_picking_r1_select_edge_test_01.ss2part";
            const part::PartDocumentStore store;
            auto loaded = store.load(fixture);
            if (!loaded.ok()) {
                std::cerr << "EDGE_R1_OWNER_PART_LOAD_FAILURE code="
                          << static_cast<int>(loaded.diagnostic.code)
                          << " message=" << loaded.diagnostic.message
                          << std::endl;
            }
            CHECK(loaded.ok());
            CHECK(loaded.document->modelingSemanticsVersion() ==
                  part::current_modeling_semantics_version);
            application::DocumentSession owner_session{
                {}, std::move(*loaded.document)};
            CHECK(owner_session.document().state().body.features.size() ==
                  4U);
            const auto evaluated =
                part::evaluatePart(owner_session.document(), kernel);
            CHECK(evaluated.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(evaluated.body_solid != nullptr);
            CHECK(evaluated.current_topology.has_value());
            CHECK(evaluated.current_topology->complete());
            const auto presented =
                kernel.bodyPresentation(evaluated.body_solid);
            CHECK(presented.ok());
            CHECK(!presented.body.edges.empty());
            CHECK(!presented.body.mesh.triangles.empty());
            const auto owner_probes =
                authorableEdgeProbes(owner_session, kernel);
            CHECK(workbench.activateDocument(&owner_session, {}));
            QApplication::processEvents();
            std::vector<double> owner_pick_microseconds;
            std::size_t query_count = 0U;
            std::size_t edge_hit_count = 0U;
            std::size_t edge_miss_count = 0U;
            std::size_t independently_visible_samples = 0U;
            std::size_t visible_sample_misses = 0U;
            std::size_t visible_sample_hits = 0U;
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_right,
                     viewer::StandardView::bottom_back_left}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                std::size_t hits_for_view = 0U;
                std::size_t misses_for_view = 0U;
                for (const auto& probe : owner_probes) {
                    const auto screen =
                        viewport->projectWorldPoint(probe.world);
                    if (!screen ||
                        screen->x < 0.0 || screen->y < 0.0 ||
                        screen->x >= viewport->width() ||
                        screen->y >= viewport->height()) {
                        continue;
                    }
                    const auto started =
                        std::chrono::steady_clock::now();
                    const auto query = viewport->queryBodyTopology(
                        *screen,
                        viewer::BodyTopologyPickFilter{
                            false, true, false});
                    const double microseconds =
                        std::chrono::duration<double, std::micro>{
                            std::chrono::steady_clock::now() - started}
                            .count();
                    CHECK(std::isfinite(microseconds) &&
                          microseconds >= 0.0);
                    owner_pick_microseconds.push_back(microseconds);
                    CHECK(query.valid());
                    CHECK(query.completed);
                    CHECK(query.generation.valid());
                    ++query_count;
                    const bool detected = std::any_of(
                        query.candidates.begin(),
                        query.candidates.end(),
                        [](const auto& candidate) {
                            return candidate.kind ==
                                viewer::BodyTopologyPresentationKind::edge;
                        });
                    if (detected) {
                        ++edge_hit_count;
                        ++hits_for_view;
                    } else {
                        ++edge_miss_count;
                        ++misses_for_view;
                    }
                }
                // Sample original OCCT authorable material Edges, not a
                // newly fabricated scene. This is an independent geometric
                // FRONT-VISIBLE control; the Viewer's own query has no role
                // in classifying the mesh ray.
                const auto camera_state = viewport->cameraState();
                CHECK(camera_state.has_value());
                const std::size_t max_sampled_segments = 7U;
                std::size_t front_samples_this_view = 0U;
                std::size_t misses_this_view = 0U;
                for (const auto& edge : presented.body.edges) {
                    if (edge.points.size() < 2U) continue;
                    const auto authored =
                        part::authorMaterialEdgeReference(
                            *evaluated.current_topology,
                            edge.runtime_token);
                    if (!authored.ok()) continue;
                    const auto segment_count = edge.points.size() - 1U;
                    const auto stride = std::max<std::size_t>(
                        1U, (segment_count + max_sampled_segments - 1U) /
                                max_sampled_segments);
                    for (std::size_t index = 0U;
                         index < segment_count; index += stride) {
                        const auto& a = edge.points[index];
                        const auto& b = edge.points[index + 1U];
                        for (const double t : {0.25, 0.5, 0.75}) {
                            const viewer::Point3 world{
                                a.x + (b.x - a.x) * t,
                                a.y + (b.y - a.y) * t,
                                a.z + (b.z - a.z) * t};
                            if (!independentOrthographicEdgeVisibility(
                                    presented.body.mesh,
                                    *camera_state, world)) continue;
                            const auto screen =
                                viewport->projectWorldPoint(world);
                            if (!screen || screen->x < 0.0 ||
                                screen->y < 0.0 ||
                                screen->x >= viewport->width() ||
                                screen->y >= viewport->height()) continue;
                            ++independently_visible_samples;
                            ++front_samples_this_view;
                            const auto query = viewport->queryBodyTopology(
                                *screen,
                                viewer::BodyTopologyPickFilter{
                                    false, true, false});
                            CHECK(query.valid() && query.completed);
                            CHECK(query.generation.valid());
                            const bool any_edge = std::any_of(
                                query.candidates.begin(),
                                query.candidates.end(),
                                [](const auto& candidate) {
                                    return candidate.kind ==
                                        viewer::BodyTopologyPresentationKind::edge;
                                });
                            if (any_edge) {
                                ++visible_sample_hits;
                            } else {
                                ++visible_sample_misses;
                                ++misses_this_view;
                                if (visible_sample_misses <= 20U) {
                                    std::cerr
                                        << "EDGE_R1_REAL_MESH_VISIBLE_MISS"
                                        << " orientation="
                                        << static_cast<int>(orientation)
                                        << " runtime_edge="
                                        << edge.runtime_token.value
                                        << " segment=" << index
                                        << " fraction=" << t
                                        << " world=" << world.x << ","
                                        << world.y << "," << world.z
                                        << " screen=" << screen->x << ","
                                        << screen->y
                                        << " candidate_count="
                                        << query.candidates.size()
                                        << std::endl;
                                }
                            }
                        }
                    }
                }
                std::cerr
                    << "EDGE_R1_REAL_MESH_VISIBILITY"
                    << " orientation=" << static_cast<int>(orientation)
                    << " independently_front_visible="
                    << front_samples_this_view
                    << " omitted_any_edge=" << misses_this_view
                    << std::endl;
                std::cerr
                    << "EDGE_R1_OWNER_REAL_BODY_VIEW orientation="
                    << static_cast<int>(orientation)
                    << " hits=" << hits_for_view
                    << " misses=" << misses_for_view
                    << std::endl;
            }
            CHECK(query_count > 0U);
            CHECK(edge_hit_count > 0U);
            std::cerr
                << "EDGE_R1_OWNER_REAL_BODY_BASELINE"
                << " features=4"
                << " faces=" << presented.body.faces.size()
                << " edges=" << presented.body.edges.size()
                << " triangles=" << presented.body.mesh.triangles.size()
                << " authorable_probes=" << owner_probes.size()
                << " visible_candidate_hits=" << edge_hit_count
                << " query_misses_unclassified=" << edge_miss_count
                << " independently_front_visible="
                << independently_visible_samples
                << " visible_edge_any_hit=" << visible_sample_hits
                << " visible_edge_any_miss=" << visible_sample_misses
                << " camera_exact_owner_match=0"
                << " owner_target_token_mapping_pending=1"
                << std::endl;
            CHECK(independently_visible_samples > 0U);
            CHECK(!owner_pick_microseconds.empty());
            std::sort(
                owner_pick_microseconds.begin(),
                owner_pick_microseconds.end());
            const auto measured = [](const auto& samples, double p) {
                const auto index = static_cast<std::size_t>(
                    std::floor((samples.size() - 1U) * p));
                return samples[index];
            };
            const double p50_microseconds =
                measured(owner_pick_microseconds, 0.5);
            const double p95_microseconds =
                measured(owner_pick_microseconds, 0.95);
            std::cerr << "EDGE_R1_G5_REAL_BODY_QUERY"
                      << " samples=" << owner_pick_microseconds.size()
                      << " p50_us=" << p50_microseconds
                      << " p95_us=" << p95_microseconds
                      << " triangles="
                      << presented.body.mesh.triangles.size()
                      << " edges=" << presented.body.edges.size()
                      << " orientations=5"
                      << " camera_zoom=fitted"
                      << std::endl;
            // G5 local/native probe contract: this is a fitted-view
            // preliminary timing distribution only, with five rotations.
            // Never treat it as a Release performance budget, a baseline
            // comparison, or proof of three-zoom / DPR parity.
            // GitHub's ordinary passing CTest hides test stdout. Publish
            // ONLY this bounded, test-derived measurement to the normal
            // Actions step summary without modifying workflows or product
            // source. No performance budget or before/after claim implied.
            if (const char* location =
                    std::getenv("GITHUB_STEP_SUMMARY")) {
                std::ofstream summary{location, std::ios::app};
                if (summary) {
                    summary
                        << "\n### Edge Picking R1 — G5 native query samples\n"
                        << "Real Owner v16/v2 Part, five fitted orthographic"
                        << " orientations, "
                        << owner_pick_microseconds.size()
                        << " material Edge-only queries, "
                        << presented.body.edges.size()
                        << " Edges, "
                        << presented.body.mesh.triangles.size()
                        << " triangles.\n\n"
                        << "| Metric | G4 candidate |\n"
                        << "|---|---:|\n"
                        << "| p50 (microseconds) | "
                        << p50_microseconds << " |\n"
                        << "| p95 (microseconds) | "
                        << p95_microseconds << " |\n\n"
                        << "One-run diagnostic; before-R1 comparison and "
                        << "multi-zoom timings still pending.\n";
                }
            }
            // G5 extended same-machine characterization. Fixed authored
            // material-Edge probe coordinates and deterministic +-6 logical
            // px cursor traces; measuring actual current Body queries, not
            // just projection math or a fabricated test scene. This does not
            // claim equivalent pre-R1 measurement or new latency budgets.
            struct G5ViewTiming final {
                viewer::StandardView orientation;
                viewer::CameraProjection projection;
                double scale_factor{};
                std::size_t samples{};
                double p50_us{};
                double p95_us{};
                double max_us{};
            };
            std::vector<G5ViewTiming> g5_views;
            const auto incoming_camera = viewport->cameraState();
            CHECK(incoming_camera.has_value());
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::bottom_front_right}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                const auto fitted = viewport->cameraState();
                CHECK(fitted.has_value());
                for (const auto projection : {
                         viewer::CameraProjection::orthographic,
                         viewer::CameraProjection::perspective}) {
                    CHECK(viewport->setCameraState(*fitted));
                    CHECK(viewport->setProjection(projection));
                    const auto projected = viewport->cameraState();
                    CHECK(projected.has_value());
                    CHECK(projected->projection == projection);
                    for (const double scale_factor :
                         {0.67, 1.0, 1.5}) {
                        auto camera = *projected;
                        camera.scale *= scale_factor;
                        CHECK(viewport->setCameraState(camera));
                        QApplication::processEvents();
                        std::vector<double> durations_us;
                        for (const auto& probe : owner_probes) {
                            const auto point =
                                viewport->projectWorldPoint(probe.world);
                            if (!point) continue;
                            for (const double dx : {-6.0, 0.0, 6.0}) {
                                const viewer::ViewportPoint2 cursor{
                                    point->x + dx, point->y};
                                if (cursor.x < 0.0 || cursor.y < 0.0 ||
                                    cursor.x >= viewport->width() ||
                                    cursor.y >= viewport->height()) {
                                    continue;
                                }
                                const auto started =
                                    std::chrono::steady_clock::now();
                                const auto query =
                                    viewport->queryBodyTopology(
                                        cursor,
                                        viewer::BodyTopologyPickFilter{
                                            false, true, false});
                                const double duration_us =
                                    std::chrono::duration<
                                        double, std::micro>{
                                        std::chrono::steady_clock::now() -
                                        started}.count();
                                CHECK(query.valid() && query.completed);
                                CHECK(query.generation.valid());
                                CHECK(std::isfinite(duration_us) &&
                                      duration_us >= 0.0);
                                durations_us.push_back(duration_us);
                            }
                        }
                        CHECK(!durations_us.empty());
                        std::sort(durations_us.begin(),
                                  durations_us.end());
                        g5_views.push_back(G5ViewTiming{
                            orientation,
                            projection,
                            scale_factor,
                            durations_us.size(),
                            measured(durations_us, 0.5),
                            measured(durations_us, 0.95),
                            durations_us.back()});
                    }
                }
            }
            CHECK(g5_views.size() == 12U);
            // Emit exact comparable cohorts to CTest failure/diagnostics
            // instead of relying only on GitHub's UI Step Summary.
            // Pre-fix evidence PR #315 uses identical cursor sampling.
            for (const auto& row : g5_views) {
                std::cerr << "EDGE_R1_G5_R1_COHORT"
                          << " view=" << static_cast<int>(row.orientation)
                          << " projection="
                          << static_cast<int>(row.projection)
                          << " factor=" << row.scale_factor
                          << " queries=" << row.samples
                          << " p50_us=" << row.p50_us
                          << " p95_us=" << row.p95_us
                          << " max_us=" << row.max_us << std::endl;
            }
            // Restore the exact preceding orthographic test camera before
            // running the existing Fillet/Chamfer Workbench selection suite.
            // A G5 measurement must not leak perspective/zoom state into
            // unrelated native click and lifecycle regressions.
            CHECK(viewport->setCameraState(*incoming_camera));
            QApplication::processEvents();
            const double measured_dpr =
                viewport->devicePixelRatioF();
            CHECK(std::isfinite(measured_dpr) && measured_dpr > 0.0);
            // Only the DPI actually provided by the Windows runner is
            // evidenced. Never report simulated 1.5x/2x as native coverage.
            std::cerr << "EDGE_R1_G5_VIEW_MATRIX"
                      << " cohorts=" << g5_views.size()
                      << " measured_dpr=" << measured_dpr
                      << " edges=" << presented.body.edges.size()
                      << " triangles="
                      << presented.body.mesh.triangles.size()
                      << std::endl;
            if (const char* location =
                    std::getenv("GITHUB_STEP_SUMMARY")) {
                std::ofstream summary{location, std::ios::app};
                if (summary) {
                    summary
                        << "\n### Edge Picking R1 — G5 extended view matrix\n"
                        << "Real Owner v16/v2 Part; native Windows Qt/OCCT;"
                        << " Body material Edge-only cursor traces"
                        << " (−6/0/+6 logical px from fixed projected probes)."
                        << " Steady-clock query duration only, no rendering/"
                        << "evaluation in timed region. Measured device"
                        << " pixel ratio: " << measured_dpr << ".\n\n"
                        << "| View | Projection | Camera scale factor"
                        << " | Queries | p50 us | p95 us | max us |\n"
                        << "|---|---|---:|---:|---:|---:|---:|\n";
                    for (const auto& row : g5_views) {
                        summary
                            << "| " << static_cast<int>(row.orientation)
                            << " | "
                            << (row.projection ==
                                    viewer::CameraProjection::orthographic
                                    ? "ortho" : "perspective")
                            << " | " << row.scale_factor
                            << " | " << row.samples
                            << " | " << row.p50_us
                            << " | " << row.p95_us
                            << " | " << row.max_us << " |\n";
                    }
                    summary
                        << "\nDiagnostic sampling only; measured DPR"
                        << " does not prove unavailable modes."
                        << " Equivalent pre-R1 run, bounded worst-case"
                        << " work and Owner W01-A–G remain open.\n";
                }
            }
            // G5 repeated native query stress: exercise the production
            // Body/QtOCCT picker for a sustained deterministic pointer
            // trace on the unchanged evaluated Owner's Part. This is
            // application-level CPU timing, NOT the event-loop frame rate,
            // an accepted SLA, a memory bound, or an alternate selection
            // identity source. Preserve the subsequent historic UI tests.
            CHECK(viewport->setCameraState(*incoming_camera));
            QApplication::processEvents();
            const auto private_bytes_before =
                currentProcessPrivateBytes();
            std::vector<double> g5_trace_microseconds;
            g5_trace_microseconds.reserve(
                24U * owner_probes.size() * 3U);
            std::size_t g5_trace_edge_hits = 0U;
            for (int sweep = 0; sweep < 24; ++sweep) {
                for (const auto& probe : owner_probes) {
                    const auto at =
                        viewport->projectWorldPoint(probe.world);
                    if (!at) continue;
                    for (const double dx : {-6.0, 0.0, 6.0}) {
                        const viewer::ViewportPoint2 cursor{
                            at->x + dx, at->y};
                        if (cursor.x < 0.0 || cursor.y < 0.0 ||
                            cursor.x >= viewport->width() ||
                            cursor.y >= viewport->height()) {
                            continue;
                        }
                        const auto started =
                            std::chrono::steady_clock::now();
                        const auto query =
                            viewport->queryBodyTopology(
                                cursor,
                                viewer::BodyTopologyPickFilter{
                                    false, true, false});
                        const double duration_us =
                            std::chrono::duration<double, std::micro>{
                                std::chrono::steady_clock::now() - started}
                                .count();
                        CHECK(query.valid() && query.completed);
                        CHECK(query.generation.valid());
                        CHECK(std::isfinite(duration_us) &&
                              duration_us >= 0.0);
                        g5_trace_microseconds.push_back(duration_us);
                        if (std::any_of(
                                query.candidates.begin(),
                                query.candidates.end(),
                                [](const auto& candidate) {
                                    return candidate.kind ==
                                        viewer::BodyTopologyPresentationKind::
                                            edge;
                                })) {
                            ++g5_trace_edge_hits;
                        }
                    }
                }
            }
            CHECK(g5_trace_microseconds.size() >= 1000U);
            CHECK(g5_trace_edge_hits > 0U);
            const auto private_bytes_after =
                currentProcessPrivateBytes();
            std::sort(g5_trace_microseconds.begin(),
                      g5_trace_microseconds.end());
            const double trace_p50_us =
                measured(g5_trace_microseconds, 0.5);
            const double trace_p95_us =
                measured(g5_trace_microseconds, 0.95);
            std::cerr
                << "EDGE_R1_G5_REPEATED_QUERY_TRACE"
                << " samples=" << g5_trace_microseconds.size()
                << " sweeps=24"
                << " edge_candidate_queries=" << g5_trace_edge_hits
                << " p50_us=" << trace_p50_us
                << " p95_us=" << trace_p95_us
                << " max_us=" << g5_trace_microseconds.back()
                << " dpr=" << measured_dpr
                << " triangles="
                << presented.body.mesh.triangles.size()
                << " edges=" << presented.body.edges.size()
                << " private_bytes_before="
                << (private_bytes_before
                        ? std::to_string(*private_bytes_before)
                        : "unavailable")
                << " private_bytes_after="
                << (private_bytes_after
                        ? std::to_string(*private_bytes_after)
                        : "unavailable")
                << std::endl;
            if (const char* location =
                    std::getenv("GITHUB_STEP_SUMMARY")) {
                std::ofstream summary{location, std::ios::app};
                if (summary) {
                    summary
                        << "\n### R1 G5 sustained same-camera query trace\n"
                        << "Unchanged real Owner Part. 24 deterministic"
                        << " sweeps through projected 33 Edge probes at"
                        << " -6/0/+6 logical px; no GUI events in timed"
                        << " block; not a frame rate or memory test. "
                        << g5_trace_microseconds.size()
                        << " queries, "
                        << g5_trace_edge_hits
                        << " with Edge candidate. p50 "
                        << trace_p50_us
                        << " us, p95 "
                        << trace_p95_us
                        << " us, max "
                        << g5_trace_microseconds.back()
                        << " us; native DPR " << measured_dpr
                        << "; process private committed bytes before "
                        << (private_bytes_before
                                ? std::to_string(*private_bytes_before)
                                : "unavailable")
                        << ", after "
                        << (private_bytes_after
                                ? std::to_string(*private_bytes_after)
                                : "unavailable")
                        << ". This is a two-point sample, not proof of"
                        << " no leak or an accepted memory SLA.\n";
                }
            }
            // G5 real modeled-scene complexity: compare the existing
            // picker on a progressively elaborated *valid OCCT Body*.
            // These are actual source Sketch/Profile + Boolean Cut
            // Features, not copied triangles, fabricated view tokens or
            // a proxy for durable Part identity. Keep the original Owner
            // witness above unchanged and restore it afterward.
            auto complexity_session =
                makeBaseSession(kernel, 120.0, 60.0, 20.0);
            const auto add_complexity_cut =
                [&kernel, &complexity_session](
                    double x, double y, int index) {
                const auto sketch =
                    complexity_session.execute(
                        application::CreatePartSketchCommand{
                            core::BuiltinReferenceRole::xy_plane});
                CHECK(sketch.ok() && sketch.sketch_id);
                CHECK(complexity_session.execute(
                    application::AddSketchCircleCommand{
                        *sketch.sketch_id,
                        {x, y},
                        5.0,
                        sketch::EntityRole::regular}).ok());
                const auto* authored =
                    complexity_session.document().findSketch(
                        *sketch.sketch_id);
                CHECK(authored != nullptr);
                const auto regions =
                    sketch::analyzeRegions(authored->model);
                CHECK(regions.complete() &&
                      regions.regions.size() == 1U);
                const auto intent =
                    part::makeProfileRegionIntent(
                        regions.regions.front());
                CHECK(intent.has_value());
                const auto profile =
                    complexity_session.execute(
                        application::CreateProfileCommand{
                            *sketch.sketch_id,
                            complexity_session.document().revision(),
                            *intent});
                CHECK(profile.ok() && profile.profile_id);
                const auto cut =
                    complexity_session.execute(
                        application::CreateExtrudeFeatureCommand{
                            *profile.profile_id,
                            complexity_session.document().revision(),
                            part::ExtrudeOperation::cut,
                            part::OneSidedExtrudeExtent{
                                core::LengthValue{25.0}, false},
                            "G5 Complexity Cut " +
                                std::to_string(index)},
                        kernel);
                CHECK(cut.ok() && cut.feature_id);
            };
            std::size_t previous_complexity_edges = 0U;
            std::size_t previous_complexity_triangles = 0U;
            int cut_count = 0;
            for (const int required_cuts : {0, 3, 8}) {
                while (cut_count < required_cuts) {
                    const int index = cut_count++;
                    const double x = 20.0 +
                        20.0 * static_cast<double>(index % 5);
                    const double y = index < 5 ? 15.0 : 45.0;
                    add_complexity_cut(x, y, index + 1);
                }
                const auto complexity_evaluated =
                    part::evaluatePart(
                        complexity_session.document(), kernel);
                CHECK(complexity_evaluated.body_status ==
                    part::BodyEvaluationStatus::up_to_date);
                CHECK(complexity_evaluated.body_solid != nullptr);
                CHECK(complexity_evaluated.current_topology.has_value());
                CHECK(complexity_evaluated.current_topology->complete());
                const auto complexity_presentation =
                    kernel.bodyPresentation(
                        complexity_evaluated.body_solid);
                CHECK(complexity_presentation.ok());
                const auto complexity_edges =
                    complexity_presentation.body.edges.size();
                const auto complexity_triangles =
                    complexity_presentation.body.mesh.triangles.size();
                CHECK(complexity_edges > previous_complexity_edges);
                CHECK(complexity_triangles >
                      previous_complexity_triangles);
                previous_complexity_edges = complexity_edges;
                previous_complexity_triangles = complexity_triangles;
                const auto complexity_probes =
                    authorableEdgeProbes(
                        complexity_session, kernel);
                CHECK(!complexity_probes.empty());
                CHECK(workbench.activateDocument(
                    &complexity_session, {}));
                CHECK(viewport->setStandardView(
                    viewer::StandardView::top_front_right));
                viewport->fitAll();
                QApplication::processEvents();

                std::vector<double> timings_us;
                std::size_t candidate_hits = 0U;
                for (int sweep = 0; sweep < 3; ++sweep) {
                    for (const auto& probe : complexity_probes) {
                        const auto at =
                            viewport->projectWorldPoint(probe.world);
                        if (!at) continue;
                        for (const double dx : {-6.0, 0.0, 6.0}) {
                            const viewer::ViewportPoint2 cursor{
                                at->x + dx, at->y};
                            if (cursor.x < 0.0 || cursor.y < 0.0 ||
                                cursor.x >= viewport->width() ||
                                cursor.y >= viewport->height()) {
                                continue;
                            }
                            const auto started =
                                std::chrono::steady_clock::now();
                            const auto query =
                                viewport->queryBodyTopology(
                                    cursor,
                                    viewer::BodyTopologyPickFilter{
                                        false, true, false});
                            const double duration_us =
                                std::chrono::duration<
                                    double, std::micro>{
                                        std::chrono::steady_clock::now() -
                                        started}.count();
                            CHECK(query.valid() && query.completed);
                            CHECK(query.generation.valid());
                            CHECK(std::isfinite(duration_us) &&
                                  duration_us >= 0.0);
                            timings_us.push_back(duration_us);
                            if (std::any_of(
                                    query.candidates.begin(),
                                    query.candidates.end(),
                                    [](const auto& candidate) {
                                        return candidate.kind ==
                                            viewer::
                                                BodyTopologyPresentationKind::
                                                    edge;
                                    })) {
                                ++candidate_hits;
                            }
                        }
                    }
                }
                CHECK(timings_us.size() >= 30U);
                CHECK(candidate_hits > 0U);
                std::sort(timings_us.begin(), timings_us.end());
                const double p50_us = measured(timings_us, 0.5);
                const double p95_us = measured(timings_us, 0.95);
                std::cerr
                    << "EDGE_R1_G5_REAL_COMPLEXITY"
                    << " cuts=" << required_cuts
                    << " edges=" << complexity_edges
                    << " triangles=" << complexity_triangles
                    << " authorable_probes="
                    << complexity_probes.size()
                    << " samples=" << timings_us.size()
                    << " candidate_hits=" << candidate_hits
                    << " p50_us=" << p50_us
                    << " p95_us=" << p95_us
                    << " max_us=" << timings_us.back()
                    << " dpr=" << measured_dpr
                    << std::endl;
                if (const char* location =
                        std::getenv("GITHUB_STEP_SUMMARY")) {
                    std::ofstream summary{location, std::ios::app};
                    if (summary) {
                        summary
                            << "\n### R1 G5 real OCCT Cut complexity\n"
                            << "Current Body after "
                            << required_cuts
                            << " valid separated circular through-Cuts"
                            << " (120×60×20 mm base): "
                            << complexity_edges
                            << " Body Edges, "
                            << complexity_triangles
                            << " mesh triangles, "
                            << complexity_probes.size()
                            << " strict material Edge probes, "
                            << timings_us.size()
                            << " native queries (3 sweeps, -6/0/+6"
                            << " logical px). Edge candidate queries "
                            << candidate_hits << ". p50 "
                            << p50_us << " us, p95 "
                            << p95_us << " us, max "
                            << timings_us.back()
                            << " us, DPR " << measured_dpr
                            << ". Times exclude Body construction and"
                            << " GUI repaint; not a memory bound.\n";
                    }
                }
            }
            // Restore the same Owner session/scene for the original
            // independently visible-mesh assertion below; it must not
            // observe the complexity-control Body stage or tokens.
            CHECK(workbench.activateDocument(
                &owner_session, {}));
            QApplication::processEvents();
            // Expected R1 RED ONLY when a truly front-visible sample on
            // the real Owner's native Part has NO Edge candidate at all.
            // Samples on tessellation gaps / blocked behind Body material
            // were excluded above; negative occlusion policy is untouched.
            CHECK(visible_sample_misses == 0U);
            // Return the original session before the pre-existing native
            // Workbench tests. Do not carry selection or runtime tokens out.
            CHECK(workbench.activateDocument(&session, {}));
            QApplication::processEvents();
        }
        if (pg01d_ui_only) {
            // Real Qt/OCCT Face cursor admission, never a fabricated
            // Viewer token or a provider ordinal. The target is a
            // separate XY Sketch after the legal upstream Extrude.
            auto face_session = makeBaseSession(kernel);
            const auto target_sketch =
                face_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(target_sketch.ok() && target_sketch.sketch_id);
            CHECK(workbench.activateDocument(&face_session, {}));
            QApplication::processEvents();
            auto* sketch_edit = workbench.findChild<QAction*>(
                QStringLiteral("editSketchAction"));
            auto* face_mode = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeSourceFaceButton"));
            auto* edge_mode = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeSourceEdgesButton"));
            auto* pg_finish = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeFinishButton"));
            auto* pg_remove = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeRemoveButton"));
            auto* pg_clear = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeClearButton"));
            auto* pg_count = workbench.findChild<QLabel*>(
                QStringLiteral("projectEdgeSelectionLabel"));
            auto* pg_face_detail = workbench.findChild<QLabel*>(
                QStringLiteral("projectEdgeResultLabel"));
            ui::PartViewportController* controller = nullptr;
            for (auto* child : workbench.children()) {
                if (auto* found =
                        dynamic_cast<ui::PartViewportController*>(child)) {
                    controller = found;
                    break;
                }
            }
            CHECK(sketch_edit && face_mode && edge_mode &&
                  pg_finish && pg_remove && pg_clear && pg_count &&
                  pg_face_detail && controller);
            QTreeWidgetItem* sketch_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    sketch_item = *it;
                    break;
                }
            }
            CHECK(sketch_item);
            tree->clearSelection();
            tree->setCurrentItem(sketch_item);
            sketch_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();

            const auto before_state = face_session.document().state();
            const auto before_revision =
                face_session.document().revision();
            const auto before_undo = face_session.undoDepth();
            auto reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(face_mode->isChecked());
            CHECK(!edge_mode->isChecked());
            CHECK(!pg_finish->isEnabled());

            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto admission =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(admission.ok());
            CHECK(admission.wires.size() == 1U);
            CHECK(admission.wires.front().outer);
            CHECK(admission.wires.front().edges.size() == 4U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Outer Edge 1: supported")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 4U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(pg_finish->isEnabled());
            CHECK(face_session.document().state() == before_state);
            CHECK(face_session.undoDepth() == before_undo);

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(face_session.undoDepth() == before_undo + 1U);
            CHECK(face_session.document().revision() !=
                  before_revision);
            const auto* projected =
                face_session.document().findSketch(
                    *target_sketch.sketch_id);
            CHECK(projected);
            CHECK(projected->projection_bindings.size() == 4U);
            CHECK(face_session.undo().changed);
            CHECK(face_session.document()
                      .findSketch(*target_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(face_session.redo().changed);
            CHECK(face_session.document()
                      .findSketch(*target_sketch.sketch_id)
                      ->projection_bindings.size() == 4U);

            std::cout << "PG01D_D2_NATIVE_FACE_FINISH_PASS"
                      << " outer_lines=4"
                      << " linked_entities=4"
                      << " one_undo=1"
                      << " undo_redo=1\n";

            // D3 fail-closed: a real previously acquired Face cannot be
            // finished after its source Feature is suppressed. Undo/Redo
            // above advanced document revision through the headless
            // session; rebind the live Workbench scene to that revision
            // *before* acquiring a genuine native Face anew.
            CHECK(workbench.activateDocument(&face_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* fresh_sketch_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) == QStringLiteral("Sketch 2")) {
                    fresh_sketch_item = *it;
                    break;
                }
            }
            CHECK(fresh_sketch_item);
            tree->clearSelection();
            tree->setCurrentItem(fresh_sketch_item);
            fresh_sketch_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 4U);
            const auto prior_link_count =
                face_session.document().findSketch(
                    *target_sketch.sketch_id)
                    ->projection_bindings.size();
            const auto base_id =
                face_session.document().body().features.front().id;
            CHECK(face_session.execute(
                application::SetFeatureSuppressedCommand{
                    base_id, face_session.document().revision(),
                    true}).ok());
            const auto suppressed_revision =
                face_session.document().revision();
            const auto suppressed_undo =
                face_session.undoDepth();
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(!reply.accepted);
            CHECK(face_session.document().revision() ==
                  suppressed_revision);
            CHECK(face_session.undoDepth() == suppressed_undo);
            CHECK(face_session.document().findSketch(
                *target_sketch.sketch_id)
                ->projection_bindings.size() == prior_link_count);
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            const auto suppressed_body = part::evaluatePart(
                face_session.document(), kernel);
            const auto unresolved_link =
                part::evaluateEffectiveSketchProjection(
                    face_session.document(),
                    *target_sketch.sketch_id,
                    suppressed_body, kernel);
            CHECK(!unresolved_link ||
                  !unresolved_link->allResolved());
            CHECK(face_session.execute(
                application::SetFeatureSuppressedCommand{
                    base_id, face_session.document().revision(),
                    false}).ok());
            const auto restored_body = part::evaluatePart(
                face_session.document(), kernel);
            const auto restored_links =
                part::evaluateEffectiveSketchProjection(
                    face_session.document(),
                    *target_sketch.sketch_id,
                    restored_body, kernel);
            CHECK(restored_links);
            CHECK(restored_links->allResolved());
            CHECK(restored_links->outcomes.size() == 4U);
            std::cout << "PG01D_D3_STALE_SUPPRESSED_FACE_PASS"
                      << " stale_finish_rejected=1"
                      << " linked_state_preserved=1"
                      << " overlay_cleared=1"
                      << " suppression_recovery=1\n";

            // D3 native Face-with-two-holes: two *real* SS2 Cut features,
            // not a fabricated wire or a painted/tessellated perimeter.
            // The top cap has four material outer Lines and two distinct
            // one-Circle hole wires, all scoped to one legal prior stage.
            auto holed_session = makeBaseSession(kernel);
            const auto make_hole = [&](double x, const char* label) {
                const auto hole_sketch = holed_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
                CHECK(hole_sketch.ok() && hole_sketch.sketch_id);
                CHECK(holed_session.execute(
                    application::AddSketchCircleCommand{
                        *hole_sketch.sketch_id,
                        {x, 15.0}, 4.0,
                        sketch::EntityRole::regular}).ok());
                const auto* authored = holed_session.document()
                    .findSketch(*hole_sketch.sketch_id);
                CHECK(authored);
                const auto regions =
                    sketch::analyzeRegions(authored->model);
                CHECK(regions.complete() &&
                      regions.regions.size() == 1U);
                const auto intent =
                    part::makeProfileRegionIntent(
                        regions.regions.front());
                CHECK(intent);
                const auto profile = holed_session.execute(
                    application::CreateProfileCommand{
                        *hole_sketch.sketch_id,
                        holed_session.document().revision(),
                        *intent});
                CHECK(profile.ok() && profile.profile_id);
                const auto cut = holed_session.execute(
                    application::CreateExtrudeFeatureCommand{
                        *profile.profile_id,
                        holed_session.document().revision(),
                        part::ExtrudeOperation::cut,
                        part::OneSidedExtrudeExtent{
                            core::LengthValue{25.0}, false},
                        label},
                    kernel);
                CHECK(cut.ok() && cut.feature_id);
            };
            make_hole(12.0, "PG01D D3 native hole 1");
            make_hole(28.0, "PG01D D3 native hole 2");
            const auto holed_target = holed_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
            CHECK(holed_target.ok() && holed_target.sketch_id);
            CHECK(workbench.activateDocument(&holed_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* holed_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 4")) {
                    holed_tree_item = *it;
                    break;
                }
            }
            CHECK(holed_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(holed_tree_item);
            holed_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto holed_state = holed_session.document().state();
            const auto holed_revision =
                holed_session.document().revision();
            const auto holed_undo = holed_session.undoDepth();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);

            // Mixed acquisition must deduplicate an independently picked
            // native material cap Edge against that same Edge expanded by
            // the later Face gesture. The first pick is an actual Viewer
            // Edge click, not an injected material reference.
            bool mixed_manual_picked = false;
            const std::array<viewer::Point3, 4> cap_midpoints{{
                {20.0, 0.0, 20.0},
                {20.0, 30.0, 20.0},
                {0.0, 15.0, 20.0},
                {40.0, 15.0, 20.0}
            }};
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::top}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                for (const auto& midpoint : cap_midpoints) {
                    if (!nativeClick(*viewport, midpoint)) {
                        continue;
                    }
                    if (pg_count->text().contains(
                            QStringLiteral("selected: 1"))) {
                        mixed_manual_picked = true;
                        break;
                    }
                    reply = workbench.submitCadInput(
                        "CLEAR", workbench.cadInputContextGeneration());
                    CHECK(reply.accepted);
                }
                if (mixed_manual_picked) break;
            }
            CHECK(mixed_manual_picked);
            const auto manual_source =
                controller->selectedMaterialEdgeReferences();
            CHECK(manual_source && manual_source->size() == 1U);
            CHECK(holed_session.document().state() == holed_state);
            CHECK(holed_session.undoDepth() == holed_undo);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto two_hole_admitted =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(two_hole_admitted.ok());
            CHECK(two_hole_admitted.wires.size() == 3U);
            std::size_t outer_count = 0U;
            std::size_t inner_count = 0U;
            std::size_t material_members = 0U;
            std::vector<part::MaterialEdgeReference>
                exact_sources;
            for (const auto& wire : two_hole_admitted.wires) {
                if (wire.outer) {
                    ++outer_count;
                    CHECK(wire.edges.size() == 4U);
                } else {
                    ++inner_count;
                    CHECK(wire.edges.size() == 1U);
                }
                for (const auto& member : wire.edges) {
                    CHECK(member.valid());
                    exact_sources.push_back(member.reference);
                    ++material_members;
                }
            }
            std::sort(exact_sources.begin(), exact_sources.end());
            CHECK(std::adjacent_find(
                exact_sources.begin(), exact_sources.end()) ==
                exact_sources.end());
            CHECK(outer_count == 1U);
            CHECK(inner_count == 2U);
            CHECK(material_members == 6U);
            CHECK(std::binary_search(
                exact_sources.begin(), exact_sources.end(),
                manual_source->front()));
            // A duplicate Edge picked manually and through Face is still
            // one linked source and will produce only one target EntityId.
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 6")));
            CHECK(pg_count->text().contains(
                QStringLiteral("holes 2")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Outer Edge 1: supported")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Hole 1 Edge 1: supported")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Hole 2 Edge 1: supported")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 6U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(pg_finish->isEnabled());
            CHECK(holed_session.document().state() == holed_state);
            CHECK(holed_session.document().revision() == holed_revision);
            CHECK(holed_session.undoDepth() == holed_undo);

            // Face REMOVE must discard just the transient Face gesture
            // while retaining the independently acquired manual Edge.
            CHECK(pg_remove->isEnabled());
            reply = workbench.submitCadInput(
                "REMOVE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 1")));
            CHECK(!pg_face_detail->text().contains(
                QStringLiteral("Hole 1 Edge 1")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(holed_session.document().state() == holed_state);
            CHECK(holed_session.undoDepth() == holed_undo);
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 6")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Hole 2 Edge 1: supported")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 6U);
            CHECK(pg_finish->isEnabled());

            // Return to manual Edge selection before Finish. The existing
            // Face gesture must stay authoritative, and the Viewer must
            // restore all six generation-bound Edge picks from semantic
            // references, not leak the old Face presentation token.
            reply = workbench.submitCadInput(
                "EDGES", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(edge_mode->isChecked());
            CHECK(!face_mode->isChecked());
            const auto restored_sources =
                controller->selectedMaterialEdgeReferences();
            CHECK(restored_sources);
            CHECK(*restored_sources == exact_sources);
            CHECK(pg_finish->isEnabled());

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(holed_session.undoDepth() == holed_undo + 1U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            const auto* holed_linked = holed_session.document()
                .findSketch(*holed_target.sketch_id);
            CHECK(holed_linked);
            CHECK(holed_linked->projection_bindings.size() == 6U);
            std::vector<part::MaterialEdgeReference> saved_sources;
            for (const auto& linked :
                 holed_linked->projection_bindings) {
                saved_sources.push_back(linked.source);
            }
            std::sort(
                saved_sources.begin(), saved_sources.end());
            CHECK(saved_sources == exact_sources);
            CHECK(holed_session.undo().changed);
            CHECK(holed_session.document()
                .findSketch(*holed_target.sketch_id)
                ->projection_bindings.empty());
            CHECK(holed_session.redo().changed);
            CHECK(holed_session.document()
                .findSketch(*holed_target.sketch_id)
                ->projection_bindings.size() == 6U);

            // Cold OCCT recovery must not depend on the selected Face token,
            // its wire count, or its original provider generation.
            QTemporaryDir holed_store_dir;
            CHECK(holed_store_dir.isValid());
            const std::filesystem::path holed_path =
                std::filesystem::path{
                    holed_store_dir.path().toStdWString()} /
                "PG01DTwoHoleFace.ss2part";
            const part::PartDocumentStore store;
            CHECK(store.createNew(
                holed_path, holed_session.document()).ok());
            const auto loaded = store.load(holed_path);
            CHECK(loaded.ok());
            CHECK(loaded.document->state() ==
                  holed_session.document().state());
            kernel_occt::OcctSolidModelingKernel cold_kernel;
            const auto cold_body = part::evaluatePart(
                *loaded.document, cold_kernel);
            CHECK(cold_body.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto cold_projection =
                part::evaluateEffectiveSketchProjection(
                    *loaded.document, *holed_target.sketch_id,
                    cold_body, cold_kernel);
            CHECK(cold_projection);
            CHECK(cold_projection->allResolved());
            CHECK(cold_projection->outcomes.size() == 6U);
            CHECK(cold_projection->model.entityCount() == 6U);
            CHECK(cold_projection->model.state().lines.size() == 4U);
            CHECK(cold_projection->model.state().circles.size() == 2U);
            const auto cold_regions =
                sketch::analyzeRegions(cold_projection->model);
            CHECK(cold_regions.complete());
            bool full_face_region = false;
            for (const auto& region : cold_regions.regions) {
                if (region.holes.size() == 2U) {
                    full_face_region = true;
                }
            }
            CHECK(full_face_region);

            // PG-01E/E1-A+B: the SAME native Viewer-acquired mixed
            // manual Edge + bounded Planar Face links (4 Lines + 2
            // hole Circles) must drive a real downstream Feature.
            // The prior PG-01D check stopped after a cold Sketch
            // projection, without a linked Profile/Feature consumer.
            const auto region_with_holes = std::find_if(
                cold_regions.regions.begin(),
                cold_regions.regions.end(),
                [](const auto& region) {
                    return region.holes.size() == 2U;
                });
            CHECK(region_with_holes != cold_regions.regions.end());
            const auto derived_face_intent =
                part::makeProfileRegionIntent(*region_with_holes);
            CHECK(derived_face_intent);
            auto downstream_copy = part::PartDocument::restore(
                loaded.document->documentId(),
                loaded.document->state());
            CHECK(downstream_copy.ok());
            application::DocumentSession linked_face_session{
                {}, std::move(*downstream_copy.document)};
            const auto projected_profile = linked_face_session.execute(
                application::CreateProfileCommand{
                    *holed_target.sketch_id,
                    linked_face_session.document().revision(),
                    *derived_face_intent},
                cold_kernel);
            CHECK(projected_profile.ok() &&
                  projected_profile.profile_id);
            const auto dependent_add = linked_face_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *projected_profile.profile_id,
                    linked_face_session.document().revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{23.0}, false},
                    "PG-01E Native Face-Linked Add"},
                cold_kernel);
            CHECK(dependent_add.ok() && dependent_add.feature_id);
            const auto linked_face_evaluation =
                part::evaluatePart(
                    linked_face_session.document(), cold_kernel);
            CHECK(linked_face_evaluation.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto* linked_face_feature =
                linked_face_evaluation.findFeature(
                    *dependent_add.feature_id);
            CHECK(linked_face_feature &&
                  linked_face_feature->status ==
                      part::FeatureEvaluationStatus::up_to_date);
            const auto downstream_file =
                std::filesystem::path{
                    holed_store_dir.path().toStdWString()} /
                "PG01ENativeFaceLinkedAdd.ss2part";
            CHECK(store.createNew(
                downstream_file, linked_face_session.document()).ok());
            const auto downstream_loaded = store.load(downstream_file);
            CHECK(downstream_loaded.ok() &&
                  downstream_loaded.document);
            CHECK(downstream_loaded.document->state() ==
                  linked_face_session.document().state());
            CHECK(downstream_loaded.document->findProfile(
                *projected_profile.profile_id));
            CHECK(downstream_loaded.document->findFeature(
                *dependent_add.feature_id));
            const auto* downstream_sketch =
                downstream_loaded.document->findSketch(
                    *holed_target.sketch_id);
            CHECK(downstream_sketch &&
                  downstream_sketch->projection_bindings.size() == 6U);
            kernel_occt::OcctSolidModelingKernel downstream_fresh_kernel;
            const auto downstream_evaluation = part::evaluatePart(
                *downstream_loaded.document, downstream_fresh_kernel);
            CHECK(downstream_evaluation.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto* downstream_feature =
                downstream_evaluation.findFeature(
                    *dependent_add.feature_id);
            CHECK(downstream_feature &&
                  downstream_feature->status ==
                      part::FeatureEvaluationStatus::up_to_date);
            CHECK(downstream_feature->result_solid);
            const auto downstream_projection =
                part::evaluateEffectiveSketchProjection(
                    *downstream_loaded.document,
                    *holed_target.sketch_id,
                    downstream_evaluation,
                    downstream_fresh_kernel,
                    *dependent_add.feature_id);
            CHECK(downstream_projection &&
                  downstream_projection->allResolved());
            CHECK(downstream_projection->model.state().lines.size() ==
                  4U);
            CHECK(downstream_projection->model.state().circles.size() ==
                  2U);
            CHECK(downstream_projection->outcomes.size() == 6U);
            std::cout << "PG01E_E1_FACE_LINKED_CONSUMER_COLD_PASS"
                      << " real_occt=1"
                      << " mixed_manual_planar_face_dedup=1"
                      << " outer_lines=4"
                      << " hole_circles=2"
                      << " stable_profile_and_feature_ids=1"
                      << " independent_provider_cold_reopen=1\\n";

            std::cout << "PG01D_D3_NATIVE_TWO_HOLE_COLD_PASS"
                      << " outer=4"
                      << " inner=2"
                      << " linked=6"
                      << " distinct_semantic_sources=6"
                      << " mixed_manual_face_dedup=1"
                      << " restored_edge_picks=6"
                      << " one_undo=1"
                      << " cold_v15=1"
                      << " region_holes=2\n";


            // PG-01D D3 positive Partial: exact native Circle cannot be
            // projected as a Circle/Arc onto a perpendicular YZ Sketch,
            // but the four *skewed* planar perimeter Lines project
            // without degeneracy. This is genuine OCCT geometry and a
            // genuine strict material Face, not a mocked projection.
            auto partial_session = makeBaseSession(kernel);
            const auto partial_base_state =
                partial_session.document().state();
            const auto& partial_base =
                partial_base_state.sketches.front();
            const double skew_cos = std::cos(0.31);
            const double skew_sin = std::sin(0.31);
            const auto skew = [skew_cos, skew_sin](
                                  sketch::Point2 p) {
                return sketch::Point2{
                    p.u * skew_cos - p.v * skew_sin,
                    p.u * skew_sin + p.v * skew_cos};
            };
            std::vector<application::SketchLineGeometryUpdate>
                partial_updates;
            for (const auto& source :
                 partial_base.model.state().lines) {
                partial_updates.push_back({
                    source.id, skew(source.start),
                    skew(source.end)});
            }
            CHECK(partial_updates.size() == 4U);
            CHECK(partial_session.execute(
                application::UpdateSketchLinesCommand{
                    partial_base.id,
                    partial_session.document().revision(),
                    std::move(partial_updates)}).ok());
            const auto partial_hole = partial_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
            CHECK(partial_hole.ok() && partial_hole.sketch_id);
            CHECK(partial_session.execute(
                application::AddSketchCircleCommand{
                    *partial_hole.sketch_id,
                    {20.0, 15.0}, 3.0,
                    sketch::EntityRole::regular}).ok());
            const auto* partial_hole_model =
                partial_session.document().findSketch(
                    *partial_hole.sketch_id);
            CHECK(partial_hole_model);
            const auto partial_hole_regions =
                sketch::analyzeRegions(
                    partial_hole_model->model);
            CHECK(partial_hole_regions.complete());
            CHECK(partial_hole_regions.regions.size() == 1U);
            const auto partial_hole_intent =
                part::makeProfileRegionIntent(
                    partial_hole_regions.regions.front());
            CHECK(partial_hole_intent);
            const auto partial_profile =
                partial_session.execute(
                    application::CreateProfileCommand{
                        *partial_hole.sketch_id,
                        partial_session.document().revision(),
                        *partial_hole_intent});
            CHECK(partial_profile.ok() &&
                  partial_profile.profile_id);
            const auto partial_cut = partial_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *partial_profile.profile_id,
                    partial_session.document().revision(),
                    part::ExtrudeOperation::cut,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{25.0}, false},
                    "PG01D Partial native circular cut"},
                kernel);
            CHECK(partial_cut.ok() && partial_cut.feature_id);
            const auto partial_target = partial_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::yz_plane});
            CHECK(partial_target.ok() && partial_target.sketch_id);
            const auto partial_eval = part::evaluatePart(
                partial_session.document(), kernel);
            CHECK(partial_eval.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(partial_eval.current_topology);
            const auto partial_frame =
                part::resolveCurrentProjectionSketchFrame(
                    partial_session.document(),
                    *partial_target.sketch_id, partial_eval);
            CHECK(partial_frame);
            std::size_t partial_native_faces = 0U;
            for (const auto& face :
                 partial_eval.current_topology->faces) {
                const auto admitted =
                    part::inspectMaterialFaceBoundary(
                        partial_eval.features.back(),
                        face.runtime_token, kernel);
                if (!admitted.ok() ||
                    admitted.wires.size() != 2U) {
                    continue;
                }
                std::size_t supported = 0U;
                std::size_t unsupported = 0U;
                for (const auto& wire : admitted.wires) {
                    for (const auto& member : wire.edges) {
                        const auto projected =
                            part::projectStrictMaterialEdge(
                                partial_session.document(),
                                member.reference, partial_eval,
                                kernel, *partial_frame);
                        if (projected.status ==
                                part::ProjectedSketchSourceStatus::
                                    resolved) {
                            ++supported;
                        } else if (projected.status ==
                                   part::ProjectedSketchSourceStatus::
                                       unsupported_projection) {
                            ++unsupported;
                        } else {
                            CHECK(false);
                        }
                    }
                }
                if (supported == 4U && unsupported == 1U) {
                    ++partial_native_faces;
                }
            }
            CHECK(partial_native_faces > 0U);
            std::cout << "PG01D_D3_NATIVE_GEOMETRIC_PARTIAL_PROOF"
                      << " strict_face=1"
                      << " supported_lines=4"
                      << " unsupported_circle=1\\n";

            CHECK(workbench.activateDocument(
                &partial_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* partial_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 3")) {
                    partial_tree_item = *it;
                    break;
                }
            }
            CHECK(partial_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(partial_tree_item);
            partial_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto partial_original =
                partial_session.document().state();
            const auto partial_rev =
                partial_session.document().revision();
            const auto partial_undo =
                partial_session.undoDepth();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            // Same real unsupported oblique Circle projection, but
            // through the new manual bounded-Face acquisition. One
            // unsupported curve does NOT discard four strict Lines.
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            const auto manual_hole_click = skew({20.0, 15.0});
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{
                    manual_hole_click.u,
                    manual_hole_click.v, 20.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Face 1 Hole 1 Edge 1: SKIPPED — geometric Unsupported")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face Boundary")));
            CHECK(pg_finish->isEnabled());
            CHECK(partial_session.document().state() ==
                  partial_original);
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(partial_session.document().revision() ==
                  partial_rev);
            CHECK(partial_session.undoDepth() == partial_undo);
            std::cout
                << "PG01D_MANUAL_FACE_UNSUPPORTED_CIRCLE_PREVIEW_PASS"
                << " supported_lines=4"
                << " exact_geometric_circle_skipped=1"
                << " prior_state_unchanged=1"
                << '\n';
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            const auto click_point = skew({20.0, 15.0});
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{
                    click_point.u, click_point.v, 20.0}));
            const auto partial_admission =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(partial_admission.ok());
            CHECK(partial_admission.wires.size() == 2U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_count->text().contains(
                QStringLiteral("holes 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("SKIPPED")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 4U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 1U);
            CHECK(pg_finish->isEnabled());
            CHECK(partial_session.document().state() ==
                  partial_original);
            CHECK(partial_session.document().revision() ==
                  partial_rev);
            CHECK(partial_session.undoDepth() == partial_undo);

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(partial_session.undoDepth() == partial_undo + 1U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            const auto* partial_linked =
                partial_session.document().findSketch(
                    *partial_target.sketch_id);
            CHECK(partial_linked);
            CHECK(partial_linked->projection_bindings.size() == 4U);
            const auto partial_after = part::evaluatePart(
                partial_session.document(), kernel);
            const auto partial_projection =
                part::evaluateEffectiveSketchProjection(
                    partial_session.document(),
                    *partial_target.sketch_id,
                    partial_after, kernel);
            CHECK(partial_projection);
            CHECK(partial_projection->allResolved());
            CHECK(partial_projection->outcomes.size() == 4U);
            CHECK(partial_projection->model.state().lines.size() == 4U);
            CHECK(partial_projection->model.state().circles.empty());
            CHECK(sketch::analyzeRegions(
                partial_projection->model).regions.empty());
            CHECK(partial_session.undo().changed);
            CHECK(partial_session.document().findSketch(
                *partial_target.sketch_id)
                ->projection_bindings.empty());
            CHECK(partial_session.redo().changed);
            CHECK(partial_session.document().findSketch(
                *partial_target.sketch_id)
                ->projection_bindings.size() == 4U);
            std::cout << "PG01D_D3_NATIVE_PARTIAL_FINISH_PASS"
                      << " cyan_ais=4 red_ais=1"
                      << " geometric_skipped=1"
                      << " linked=4"
                      << " open_no_profile=1"
                      << " one_undo=1\\n";

            // D3 all-Unsupported gate: one native circle-only planar cap
            // projects obliquely to YZ. Strict Face admission succeeds,
            // but no representable Edge remains: Finish must stay disabled
            // and even a typed FINISH must not author a partial empty batch.
            auto disk_document = part::PartDocument::create(
                core::DocumentId::generate());
            application::DocumentSession disk_session{
                {}, std::move(disk_document)};
            const auto disk_source = disk_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
            CHECK(disk_source.ok() && disk_source.sketch_id);
            CHECK(disk_session.execute(
                application::AddSketchCircleCommand{
                    *disk_source.sketch_id,
                    {20.0, 15.0}, 9.0,
                    sketch::EntityRole::regular}).ok());
            const auto* disk_source_sketch =
                disk_session.document().findSketch(
                    *disk_source.sketch_id);
            CHECK(disk_source_sketch);
            const auto disk_regions =
                sketch::analyzeRegions(disk_source_sketch->model);
            CHECK(disk_regions.complete() &&
                  disk_regions.regions.size() == 1U);
            const auto disk_intent =
                part::makeProfileRegionIntent(
                    disk_regions.regions.front());
            CHECK(disk_intent);
            const auto disk_profile = disk_session.execute(
                application::CreateProfileCommand{
                    *disk_source.sketch_id,
                    disk_session.document().revision(),
                    *disk_intent});
            CHECK(disk_profile.ok() && disk_profile.profile_id);
            const auto disk_extrude = disk_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *disk_profile.profile_id,
                    disk_session.document().revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{20.0}, false},
                    "PG01D circular cap source"},
                kernel);
            CHECK(disk_extrude.ok() && disk_extrude.feature_id);
            const auto disk_target = disk_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::yz_plane});
            CHECK(disk_target.ok() && disk_target.sketch_id);
            CHECK(workbench.activateDocument(&disk_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* disk_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    disk_tree_item = *it;
                    break;
                }
            }
            CHECK(disk_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(disk_tree_item);
            disk_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto disk_original = disk_session.document().state();
            const auto disk_revision =
                disk_session.document().revision();
            const auto disk_undo = disk_session.undoDepth();
            // Manual Face Boundary: all members are semantic material
            // sources but the oblique Circle image is Unsupported.
            // A zero-supported Face may remain staged and highlighted;
            // it must NOT generate a zero-member authored transaction.
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 0")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Face 1 Outer Edge 1: SKIPPED — geometric Unsupported")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face Boundary")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 1U);
            CHECK(!pg_finish->isEnabled());
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(!reply.accepted);
            CHECK(disk_session.document().state() == disk_original);
            CHECK(disk_session.document().revision() ==
                  disk_revision);
            CHECK(disk_session.undoDepth() == disk_undo);
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            std::cout
                << "PG01D_MANUAL_ALL_UNSUPPORTED_NOOP_PASS"
                << " selected_faces=1"
                << " unsupported_circle=1"
                << " zero_command=1"
                << '\n';

            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto disk_admitted =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(disk_admitted.ok());
            CHECK(disk_admitted.wires.size() == 1U);
            CHECK(disk_admitted.wires.front().outer);
            CHECK(disk_admitted.wires.front().edges.size() == 1U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 0")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face: 1")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 1U);
            CHECK(!pg_finish->isEnabled());
            CHECK(pg_clear->isEnabled());
            CHECK(pg_remove->isEnabled());
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(!reply.accepted);
            CHECK(disk_session.document().state() == disk_original);
            CHECK(disk_session.document().revision() == disk_revision);
            CHECK(disk_session.undoDepth() == disk_undo);
            CHECK(disk_session.document().findSketch(
                *disk_target.sketch_id)
                ->projection_bindings.empty());
            reply = workbench.submitCadInput(
                "CLEAR", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(disk_session.document().state() == disk_original);
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(disk_session.document().revision() == disk_revision);
            CHECK(disk_session.undoDepth() == disk_undo);
            std::cout << "PG01D_D3_NATIVE_ALL_UNSUPPORTED_NOOP_PASS"
                      << " red_ais=1"
                      << " finish_blocked=1"
                      << " clear_cancel_noop=1\\n";

            // Owner D2 E2: manual Face Boundary is NOT a carrier
            // traversal. Click two distinct bounded native Faces,
            // preserve their one actual shared material Edge, and
            // skip vertical edges whose exact XY image degenerates.
            // One Finish should author only the unique supported
            // source references; open/overlapping Sketch geometry
            // is never silently healed into a Profile.
            auto manual_session = makeBaseSession(kernel);
            const auto manual_target =
                manual_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(manual_target.ok() && manual_target.sketch_id);
            CHECK(workbench.activateDocument(
                &manual_session, {}));
            QApplication::processEvents();
            auto* boundary_mode =
                workbench.findChild<QPushButton*>(
                    QStringLiteral(
                        "projectEdgeSourceBoundaryButton"));
            CHECK(boundary_mode);
            QTreeWidgetItem* manual_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                        QStringLiteral("Sketch 2")) {
                    manual_tree_item = *it;
                    break;
                }
            }
            CHECK(manual_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(manual_tree_item);
            manual_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto manual_before =
                manual_session.document().state();
            const auto manual_revision =
                manual_session.document().revision();
            const auto manual_undo =
                manual_session.undoDepth();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(boundary_mode->isChecked());
            CHECK(!face_mode->isChecked());
            CHECK(!edge_mode->isChecked());
            CHECK(!pg_finish->isEnabled());
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto top_picked =
                controller->primaryBodyTopologySelection();
            CHECK(top_picked && top_picked->valid());
            const auto top_native =
                controller->inspectCurrentSelectedFaceBoundary(
                    *top_picked);
            CHECK(top_native.ok());
            CHECK(top_native.wires.size() == 1U);
            // A second picked Face token which does not belong to
            // this exact live Body generation is an identity fault,
            // never a skippable unsupported curve. The prior valid
            // Face/UI draft is still present after the refused query.
            auto forged_new_face = *top_picked;
            forged_new_face.runtime_token_value =
                std::numeric_limits<std::uint64_t>::max();
            const auto refused_new_face =
                controller->inspectCurrentSelectedFaceBoundary(
                    forged_new_face);
            CHECK(!refused_new_face.ok());
            CHECK(refused_new_face.status ==
                  part::MaterialFaceBoundaryStatus::
                      face_unavailable);
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(manual_session.document().state() == manual_before);
            CHECK(manual_session.undoDepth() == manual_undo);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 1U);
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_finish->isEnabled());

            bool second_staged = false;
            for (const auto& [camera, point] :
                 std::array<std::pair<
                     viewer::StandardView, viewer::Point3>, 2>{{
                     {viewer::StandardView::front,
                      viewer::Point3{20.0, 0.0, 10.0}},
                     {viewer::StandardView::back,
                      viewer::Point3{20.0, 30.0, 10.0}},
                 }}) {
                CHECK(viewport->setStandardView(camera));
                viewport->fitAll();
                QApplication::processEvents();
                if (!nativePlanarFaceClick(*viewport, point)) {
                    continue;
                }
                const auto side_picked =
                    controller->primaryBodyTopologySelection();
                if (!side_picked || *side_picked == *top_picked) {
                    continue;
                }
                const auto side_native =
                    controller->inspectCurrentSelectedFaceBoundary(
                        *side_picked);
                CHECK(side_native.ok());
                if (pg_count->text().contains(
                        QStringLiteral("Faces staged: 2"))) {
                    second_staged = true;
                    break;
                }
            }
            CHECK(second_staged);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 2U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 2")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Face 2 Outer Edge")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("SKIPPED — geometric Degenerate")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face Boundary")));
            CHECK(pg_finish->isEnabled());
            CHECK(manual_session.document().state() == manual_before);
            CHECK(manual_session.document().revision() ==
                  manual_revision);
            CHECK(manual_session.undoDepth() == manual_undo);

            // Selective removal must preserve the first clicked Face.
            reply = workbench.submitCadInput(
                "REMOVE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 1U);
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_finish->isEnabled());
            CHECK(viewport->setStandardView(
                viewer::StandardView::front));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 0.0, 10.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 2")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            CHECK(pg_finish->isEnabled());

            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 2U);
            // Exercise all THREE acquisition modes without applying
            // a Face union or throwing away previously staged Edges.
            // The second mode switch is not a second document command.
            reply = workbench.submitCadInput(
                "EDGES", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(edge_mode->isChecked());
            CHECK(!boundary_mode->isChecked());
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 2")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            const auto preserved_edge_picks =
                controller->selectedMaterialEdgeReferences();
            CHECK(preserved_edge_picks &&
                  preserved_edge_picks->size() == 5U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 2U);
            CHECK(pg_finish->isEnabled());
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(face_mode->isChecked());
            CHECK(!edge_mode->isChecked());
            CHECK(!boundary_mode->isChecked());
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 2")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            CHECK(pg_finish->isEnabled());
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(boundary_mode->isChecked());
            CHECK(!face_mode->isChecked());
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 2")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            CHECK(pg_finish->isEnabled());
            CHECK(manual_session.document().state() == manual_before);
            CHECK(manual_session.undoDepth() == manual_undo);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 2U);

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 0U);
            CHECK(manual_session.undoDepth() == manual_undo + 1U);
            const auto* saved =
                manual_session.document().findSketch(
                    *manual_target.sketch_id);
            CHECK(saved && saved->projection_bindings.size() == 5U);
            CHECK(manual_session.undo().changed);
            CHECK(manual_session.document().findSketch(
                *manual_target.sketch_id)
                ->projection_bindings.empty());
            CHECK(manual_session.redo().changed);
            CHECK(manual_session.document().findSketch(
                *manual_target.sketch_id)
                ->projection_bindings.size() == 5U);
            // The Face selection, Face-wire start, native provider
            // generation and Face token are NOT authored dependencies.
            // Cold native reopen must reevaluate five independent
            // strict material Edge references without these picks.
            QTemporaryDir manual_store_dir;
            CHECK(manual_store_dir.isValid());
            const std::filesystem::path manual_path =
                std::filesystem::path{
                    manual_store_dir.path().toStdWString()} /
                "PG01DManualTwoFacePartial.ss2part";
            const part::PartDocumentStore manual_store;
            CHECK(manual_store.createNew(
                manual_path, manual_session.document()).ok());
            const auto manual_loaded =
                manual_store.load(manual_path);
            CHECK(manual_loaded.ok());
            CHECK(manual_loaded.document->state() ==
                  manual_session.document().state());
            const auto* cold_saved =
                manual_loaded.document->findSketch(
                    *manual_target.sketch_id);
            CHECK(cold_saved &&
                  cold_saved->projection_bindings.size() == 5U);
            kernel_occt::OcctSolidModelingKernel manual_cold_kernel;
            const auto cold_manual_part =
                part::evaluatePart(
                    *manual_loaded.document, manual_cold_kernel);
            CHECK(cold_manual_part.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto cold_manual_projection =
                part::evaluateEffectiveSketchProjection(
                    *manual_loaded.document,
                    *manual_target.sketch_id,
                    cold_manual_part, manual_cold_kernel);
            CHECK(cold_manual_projection);
            CHECK(cold_manual_projection->allResolved());
            CHECK(cold_manual_projection->outcomes.size() == 5U);
            CHECK(cold_manual_projection->model.entityCount() == 5U);
            CHECK(cold_manual_projection->model.state().lines.size() ==
                  5U);
            CHECK(cold_manual_projection->model.state().circles.empty());

            // PG-01E/E1-C: two *manually* picked bounded Faces can
            // produce five independent strict Edge links whose planar
            // images overlap. A region must NOT be silently healed by
            // treating equal coordinates as equal material identity.
            // A human-authorized explicit Sketch Erase (existing
            // Command) of ONE precisely identified collocated target
            // is an allowed repair; the other four strict linked
            // EntityIds and material sources must survive.
            const auto unedited_regions =
                sketch::analyzeRegions(cold_manual_projection->model);
            CHECK(unedited_regions.regions.empty());
            CHECK(!unedited_regions.complete());
            const auto five_lines =
                cold_manual_projection->model.state().lines;
            CHECK(five_lines.size() == 5U);
            const auto point_same = [](sketch::Point2 a,
                                       sketch::Point2 b) {
                return std::abs(a.u - b.u) < 1.0e-7 &&
                       std::abs(a.v - b.v) < 1.0e-7;
            };
            std::optional<sketch::EntityId> explicit_erase_id;
            std::optional<sketch::EntityId> other_overlap_id;
            std::size_t overlapping_pairs = 0U;
            for (std::size_t first = 0U;
                 first < five_lines.size(); ++first) {
                for (std::size_t second = first + 1U;
                     second < five_lines.size(); ++second) {
                    const auto& a = five_lines[first];
                    const auto& b = five_lines[second];
                    if ((point_same(a.start, b.start) &&
                         point_same(a.end, b.end)) ||
                        (point_same(a.start, b.end) &&
                         point_same(a.end, b.start))) {
                        ++overlapping_pairs;
                        explicit_erase_id = b.id;
                        other_overlap_id = a.id;
                    }
                }
            }
            CHECK(overlapping_pairs == 1U);
            CHECK(explicit_erase_id && other_overlap_id);
            const auto& original_links =
                cold_saved->projection_bindings;
            const auto erase_source = std::find_if(
                original_links.begin(), original_links.end(),
                [&explicit_erase_id](const auto& link) {
                    return link.target_entity == *explicit_erase_id;
                });
            const auto preserved_source = std::find_if(
                original_links.begin(), original_links.end(),
                [&other_overlap_id](const auto& link) {
                    return link.target_entity == *other_overlap_id;
                });
            CHECK(erase_source != original_links.end());
            CHECK(preserved_source != original_links.end());
            CHECK(erase_source->source != preserved_source->source);

            auto repaired_copy = part::PartDocument::restore(
                manual_loaded.document->documentId(),
                manual_loaded.document->state());
            CHECK(repaired_copy.ok());
            application::DocumentSession repaired_manual{
                {}, std::move(*repaired_copy.document)};
            const auto delete_one = repaired_manual.execute(
                application::EraseSketchEntityCommand{
                    *manual_target.sketch_id, *explicit_erase_id});
            CHECK(delete_one.ok() && delete_one.changed);
            const auto* repaired_sketch =
                repaired_manual.document().findSketch(
                    *manual_target.sketch_id);
            CHECK(repaired_sketch &&
                  repaired_sketch->projection_bindings.size() == 4U);
            CHECK(repaired_sketch->model.entityCount() == 4U);
            CHECK(!repaired_sketch->model.contains(*explicit_erase_id));
            CHECK(repaired_sketch->model.contains(*other_overlap_id));
            CHECK(std::none_of(
                repaired_sketch->projection_bindings.begin(),
                repaired_sketch->projection_bindings.end(),
                [&explicit_erase_id](const auto& link) {
                    return link.target_entity == *explicit_erase_id;
                }));
            // New provider, not a cached upstream Face token or seed.
            kernel_occt::OcctSolidModelingKernel repair_provider;
            const auto repaired_body = part::evaluatePart(
                repaired_manual.document(), repair_provider);
            CHECK(repaired_body.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto repaired_projection =
                part::evaluateEffectiveSketchProjection(
                    repaired_manual.document(),
                    *manual_target.sketch_id,
                    repaired_body, repair_provider);
            CHECK(repaired_projection &&
                  repaired_projection->allResolved());
            CHECK(repaired_projection->outcomes.size() == 4U);
            const auto repaired_regions =
                sketch::analyzeRegions(repaired_projection->model);
            CHECK(repaired_regions.complete());
            CHECK(repaired_regions.regions.size() == 1U);
            const auto repaired_intent = part::makeProfileRegionIntent(
                repaired_regions.regions.front());
            CHECK(repaired_intent);
            const auto repaired_profile = repaired_manual.execute(
                application::CreateProfileCommand{
                    *manual_target.sketch_id,
                    repaired_manual.document().revision(),
                    *repaired_intent},
                repair_provider);
            CHECK(repaired_profile.ok() &&
                  repaired_profile.profile_id);
            const auto repaired_add = repaired_manual.execute(
                application::CreateExtrudeFeatureCommand{
                    *repaired_profile.profile_id,
                    repaired_manual.document().revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{25.0}, false},
                    "PG-01E Explicit Multi-Face Repair Add"},
                repair_provider);
            CHECK(repaired_add.ok() && repaired_add.feature_id);
            const auto multi_feature = part::evaluatePart(
                repaired_manual.document(), repair_provider);
            CHECK(multi_feature.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto* accepted_multi_feature =
                multi_feature.findFeature(*repaired_add.feature_id);
            CHECK(accepted_multi_feature &&
                  accepted_multi_feature->status ==
                      part::FeatureEvaluationStatus::up_to_date);
            // End-to-end Save/Close/Reopen: original five distinct
            // material IDs were not silently rebound; only the
            // explicitly erased ID and its binding were removed.
            const std::filesystem::path multi_path =
                std::filesystem::path{
                    manual_store_dir.path().toStdWString()} /
                "PG01EManualMultiFaceRepaired.ss2part";
            CHECK(manual_store.createNew(
                multi_path, repaired_manual.document()).ok());
            const auto multi_reopened = manual_store.load(multi_path);
            CHECK(multi_reopened.ok() && multi_reopened.document);
            CHECK(multi_reopened.document->state() ==
                  repaired_manual.document().state());
            CHECK(multi_reopened.document->findProfile(
                *repaired_profile.profile_id));
            CHECK(multi_reopened.document->findFeature(
                *repaired_add.feature_id));
            kernel_occt::OcctSolidModelingKernel multi_fresh_provider;
            const auto multi_cold_body = part::evaluatePart(
                *multi_reopened.document, multi_fresh_provider);
            CHECK(multi_cold_body.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(multi_cold_body.findFeature(
                *repaired_add.feature_id)->status ==
                part::FeatureEvaluationStatus::up_to_date);
            const auto multi_cold_projection =
                part::evaluateEffectiveSketchProjection(
                    *multi_reopened.document,
                    *manual_target.sketch_id,
                    multi_cold_body,
                    multi_fresh_provider,
                    *repaired_add.feature_id);
            CHECK(multi_cold_projection &&
                  multi_cold_projection->allResolved());
            CHECK(multi_cold_projection->outcomes.size() == 4U);
            CHECK(multi_cold_projection->model.entityCount() == 4U);
            CHECK(multi_reopened.document->modelingSemanticsVersion() ==
                  part::current_modeling_semantics_version);

            // PG-01E/E1-C: batch Break Link on two of the remaining
            // four strictly identified manual multi-Face source Edges.
            // A suppressed earlier material source must refuse the
            // WHOLE batch without detaching any target or consuming
            // the old authored 2D seed. The accepted positive branch
            // must detach both atomically in one Undo/Redo transaction.
            const auto* multi_linked_sketch =
                multi_reopened.document->findSketch(
                    *manual_target.sketch_id);
            CHECK(multi_linked_sketch &&
                  multi_linked_sketch->projection_bindings.size() == 4U);
            const auto pair_first =
                multi_linked_sketch->projection_bindings.at(0)
                    .target_entity;
            const auto pair_second =
                multi_linked_sketch->projection_bindings.at(1)
                    .target_entity;
            CHECK(pair_first != pair_second);
            auto broken_multi_state =
                multi_reopened.document->state();
            CHECK(!broken_multi_state.body.features.empty());
            broken_multi_state.body.features.front().suppressed = true;
            auto broken_multi_document =
                part::PartDocument::restore(
                    multi_reopened.document->documentId(),
                    std::move(broken_multi_state));
            CHECK(broken_multi_document.ok());
            application::DocumentSession broken_multi_session{
                {}, std::move(*broken_multi_document.document)};
            const auto before_refusal =
                broken_multi_session.document().state();
            const auto before_refusal_revision =
                broken_multi_session.document().revision();
            const auto before_refusal_undo =
                broken_multi_session.undoDepth();
            kernel_occt::OcctSolidModelingKernel
                suppressed_material_provider;
            const auto denied_batch = broken_multi_session.execute(
                application::BreakProjectedEdgeLinksCommand{
                    *manual_target.sketch_id,
                    {pair_first, pair_second},
                    before_refusal_revision},
                suppressed_material_provider);
            CHECK(!denied_batch.ok() && !denied_batch.changed);
            CHECK(denied_batch.status ==
                  application::BreakProjectedEdgeLinkStatus::
                      source_unavailable);
            CHECK(broken_multi_session.document().state() ==
                  before_refusal);
            CHECK(broken_multi_session.document().revision() ==
                  before_refusal_revision);
            CHECK(broken_multi_session.undoDepth() ==
                  before_refusal_undo);
            CHECK(broken_multi_session.document()
                  .findSketch(*manual_target.sketch_id)
                  ->projection_bindings.size() == 4U);

            auto valid_multi_document =
                part::PartDocument::restore(
                    multi_reopened.document->documentId(),
                    multi_reopened.document->state());
            CHECK(valid_multi_document.ok());
            application::DocumentSession detachable_multi_session{
                {}, std::move(*valid_multi_document.document)};
            const auto before_detach =
                detachable_multi_session.document().state();
            const auto before_detach_undo =
                detachable_multi_session.undoDepth();
            kernel_occt::OcctSolidModelingKernel
                material_detach_provider;
            const auto detached_pair = detachable_multi_session.execute(
                application::BreakProjectedEdgeLinksCommand{
                    *manual_target.sketch_id,
                    {pair_first, pair_second},
                    detachable_multi_session.document().revision()},
                material_detach_provider);
            CHECK(detached_pair.ok());
            CHECK(detachable_multi_session.undoDepth() ==
                  before_detach_undo + 1U);
            CHECK(detachable_multi_session.document()
                  .findSketch(*manual_target.sketch_id)
                  ->projection_bindings.size() == 2U);
            // Explicit two-target detach must materialize precisely
            // the CURRENT projected curves, not retained stale seeds.
            // The two remaining linked targets must still name their
            // unchanged original material Edge semantic sources.
            const auto* pair_detached_sketch =
                detachable_multi_session.document().findSketch(
                    *manual_target.sketch_id);
            CHECK(pair_detached_sketch &&
                  pair_detached_sketch->projection_bindings.size() == 2U);
            for (const auto id : {pair_first, pair_second}) {
                CHECK(std::none_of(
                    pair_detached_sketch->projection_bindings.begin(),
                    pair_detached_sketch->projection_bindings.end(),
                    [id](const auto& binding) {
                        return binding.target_entity == id;
                    }));
                const auto* detached_line =
                    pair_detached_sketch->model.findLine(id);
                const auto* current_line =
                    multi_cold_projection->model.findLine(id);
                CHECK(detached_line && current_line);
                CHECK(point_same(
                    detached_line->start(), current_line->start()));
                CHECK(point_same(
                    detached_line->end(), current_line->end()));
            }
            for (const auto& binding :
                 pair_detached_sketch->projection_bindings) {
                const auto original_binding = std::find_if(
                    multi_linked_sketch->projection_bindings.begin(),
                    multi_linked_sketch->projection_bindings.end(),
                    [&binding](const auto& candidate) {
                        return candidate.target_entity ==
                               binding.target_entity;
                    });
                CHECK(original_binding !=
                      multi_linked_sketch->projection_bindings.end());
                CHECK(original_binding->source == binding.source);
                CHECK(binding.target_entity != pair_first);
                CHECK(binding.target_entity != pair_second);
            }
            CHECK(detachable_multi_session.undo().changed);
            CHECK(detachable_multi_session.document().state() ==
                  before_detach);
            CHECK(detachable_multi_session.redo().changed);
            CHECK(detachable_multi_session.document()
                  .findSketch(*manual_target.sketch_id)
                  ->projection_bindings.size() == 2U);
            const auto after_detach_eval = part::evaluatePart(
                detachable_multi_session.document(),
                material_detach_provider);
            CHECK(after_detach_eval.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(after_detach_eval.findFeature(
                *repaired_add.feature_id)->status ==
                  part::FeatureEvaluationStatus::up_to_date);
            std::cout
                << "PG01E_E1_MULTI_FACE_BATCH_BREAK_FAIL_CLOSED_PASS"
                << " original_material_links=4"
                << " suppressed_source_batch_refused=1"
                << " state_revision_undo_unchanged=1"
                << " positive_pair_atomic=1"
                << " one_undo_redo=1"
                << " unaffected_linked_feature_current=1\n";

            // PG-01E/E3 Windows observation on a REAL repaired
            // two-Face Part/Feature/linked Profile: each iteration
            // independently loads the SAME immutable native document
            // and builds a FRESH OCCT provider. Do not infer no leak
            // from this small bounded sampling window.
            constexpr std::size_t e3_warmup = 2U;
            constexpr std::size_t e3_measured = 8U;
            const auto e3_native_bytes =
                std::filesystem::file_size(multi_path);
            const auto e3_private_before =
                currentProcessPrivateBytes();
            std::vector<long long> e3_elapsed_microseconds;
            std::vector<std::optional<std::uint64_t>>
                e3_private_samples;
            e3_elapsed_microseconds.reserve(e3_measured);
            e3_private_samples.reserve(e3_measured);
            for (std::size_t e3_cycle = 0U;
                 e3_cycle < e3_warmup + e3_measured;
                 ++e3_cycle) {
                const auto started =
                    std::chrono::steady_clock::now();
                {
                    const auto reloaded =
                        manual_store.load(multi_path);
                    CHECK(reloaded.ok() && reloaded.document);
                    CHECK(reloaded.document->state() ==
                          repaired_manual.document().state());
                    kernel_occt::OcctSolidModelingKernel
                        cycle_kernel;
                    const auto cycle_body = part::evaluatePart(
                        *reloaded.document, cycle_kernel);
                    CHECK(cycle_body.body_status ==
                          part::BodyEvaluationStatus::up_to_date);
                    const auto* cycle_feature =
                        cycle_body.findFeature(
                            *repaired_add.feature_id);
                    CHECK(cycle_feature &&
                          cycle_feature->status ==
                              part::FeatureEvaluationStatus::
                                  up_to_date);
                    const auto cycle_projection =
                        part::evaluateEffectiveSketchProjection(
                            *reloaded.document,
                            *manual_target.sketch_id,
                            cycle_body,
                            cycle_kernel,
                            *repaired_add.feature_id);
                    CHECK(cycle_projection &&
                          cycle_projection->allResolved());
                    CHECK(cycle_projection->outcomes.size() == 4U);
                    CHECK(cycle_projection->model.entityCount() ==
                          4U);
                }
                const auto duration =
                    std::chrono::duration_cast<
                        std::chrono::microseconds>(
                        std::chrono::steady_clock::now() -
                        started).count();
                CHECK(duration >= 0);
                CHECK(std::filesystem::file_size(multi_path) ==
                      e3_native_bytes);
                if (e3_cycle >= e3_warmup) {
                    e3_elapsed_microseconds.push_back(duration);
                    // Sample after the temporary kernel/provider
                    // and loaded Part have left their scope.
                    e3_private_samples.push_back(
                        currentProcessPrivateBytes());
                }
            }
            CHECK(e3_elapsed_microseconds.size() == e3_measured);
            CHECK(e3_private_samples.size() == e3_measured);
            const auto e3_extrema = std::minmax_element(
                e3_elapsed_microseconds.begin(),
                e3_elapsed_microseconds.end());
            long long e3_sum = 0;
            for (const auto duration : e3_elapsed_microseconds) {
                e3_sum += duration;
            }
            const auto e3_private_after =
                currentProcessPrivateBytes();
            const auto format_bytes = [](
                std::optional<std::uint64_t> value) {
                return value
                    ? std::to_string(*value)
                    : std::string{"UNAVAILABLE"};
            };
            std::cerr
                << "PG01E_E3_WINDOWS_MULTI_FACE_REPLAY"
                << " warmup=" << e3_warmup
                << " measured=" << e3_measured
                << " min_us=" << *e3_extrema.first
                << " mean_us=" << e3_sum /
                    static_cast<long long>(e3_measured)
                << " max_us=" << *e3_extrema.second
                << " package_bytes=" << e3_native_bytes
                << " private_before="
                << format_bytes(e3_private_before)
                << " private_after="
                << format_bytes(e3_private_after)
                << std::endl;
            if (const char* summary_path =
                    std::getenv("GITHUB_STEP_SUMMARY")) {
                std::ofstream e3_summary{
                    summary_path, std::ios::app};
                if (e3_summary) {
                    e3_summary
                        << "\n### PG-01E/E3 native Qt/OCCT multi-Face "
                        << "linked Feature cold replay\n"
                        << "Real bounded two-Face input with explicit "
                        << "overlap-target deletion; current Part v16/"
                        << "modeling-v2 linked Profile + Extrude Add. "
                        << "2 warm-up, 8 measured distinct native "
                        << "file loads, fresh OCCT providers, and "
                        << "post-teardown Windows private-byte samples."
                        << " Not a memory SLA or leak proof.\n\n"
                        << "| Metric | Observed |\n"
                        << "| --- | ---: |\n"
                        << "| min/mean/max cold replay (us) | "
                        << *e3_extrema.first << " / "
                        << e3_sum /
                           static_cast<long long>(e3_measured)
                        << " / " << *e3_extrema.second
                        << " |\n"
                        << "| native package bytes | "
                        << e3_native_bytes << " (unchanged) |\n"
                        << "| Windows private bytes before | "
                        << format_bytes(e3_private_before)
                        << " |\n"
                        << "| Windows private bytes after | "
                        << format_bytes(e3_private_after)
                        << " |\n"
                        << "| per-cycle private bytes (post teardown) | ";
                    for (std::size_t m = 0U;
                         m < e3_private_samples.size(); ++m) {
                        if (m) e3_summary << ", ";
                        e3_summary
                            << format_bytes(e3_private_samples[m]);
                    }
                    e3_summary
                        << " |\n"
                        << "One bounded run only, with process-wide "
                        << "memory affected by Qt, CTest and caches; "
                        << "no comparative baseline, native handle "
                        << "series, leak conclusion or budget claim.\n";
                }
            }
            std::cout
                << "PG01E_E1_MANUAL_MULTI_FACE_EXPLICIT_REPAIR_COLD_PASS"
                << " two_native_faces=1"
                << " supported_semantic_links_before=5"
                << " overlapping_planar_pair=1"
                << " erased_one_explicit_link=1"
                << " repaired_linked_profile=1"
                << " dependent_extrude_add=1"
                << " fresh_occt_provider=1"
                << " stable_profile_and_feature_ids=1\n";

            std::cout
                << "PG01D_MANUAL_MULTI_FACE_NATIVE_COLD_V15_PASS"
                << " unique_links=5"
                << " skipped_degenerate=2"
                << " fresh_provider_generation=1"
                << " recovered_links=5"
                << '\n';
            std::cout
                << "PG01D_MANUAL_MULTI_FACE_NATIVE_UI_PARTIAL_PASS"
                << " clicked_faces=2"
                << " exact_shared_material_dedup=1"
                << " supported_unique=5"
                << " geometric_degenerate_skipped=2"
                << " remove_readd=1"
                << " one_undo_redo=1"
                << '\n';

            // E2 D2 fail-closed negative: a transient Face gesture is
            // revision-bound, even if an external command only changes
            // an otherwise unrelated Sketch. It must never Finish
            // against a previously cached provider generation.
            auto stale_session = makeBaseSession(kernel);
            const auto stale_target =
                stale_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(stale_target.ok() && stale_target.sketch_id);
            CHECK(workbench.activateDocument(
                &stale_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* stale_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    stale_tree_item = *it;
                    break;
                }
            }
            CHECK(stale_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(stale_tree_item);
            stale_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{
                    20.0, 15.0, 20.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_finish->isEnabled());
            const auto stale_pre_revision =
                stale_session.document().revision();
            const auto stale_pre_undo =
                stale_session.undoDepth();
            const auto unrelated_edit = stale_session.execute(
                application::AddSketchLineCommand{
                    *stale_target.sketch_id,
                    {2.0, 2.0}, {7.0, 7.0},
                    sketch::EntityRole::construction});
            CHECK(unrelated_edit.ok());
            CHECK(stale_session.document().revision() !=
                  stale_pre_revision);
            CHECK(stale_session.undoDepth() ==
                  stale_pre_undo + 1U);
            const auto changed_state =
                stale_session.document().state();
            const auto changed_revision =
                stale_session.document().revision();
            const auto changed_undo = stale_session.undoDepth();
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(!reply.accepted);
            CHECK(stale_session.document().state() ==
                  changed_state);
            CHECK(stale_session.document().revision() ==
                  changed_revision);
            CHECK(stale_session.undoDepth() == changed_undo);
            CHECK(stale_session.document().findSketch(
                *stale_target.sketch_id)
                ->projection_bindings.empty());
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            std::cout
                << "PG01D_MANUAL_FACE_STALE_REVISION_FAIL_CLOSED_PASS"
                << " drafted_faces=1"
                << " external_sketch_edit=1"
                << " invalidated_finish=1"
                << " additional_undo=0"
                << '\n';

            // Owner D2 mixed-acquisition contract: one native bounded
            // Face yields four top-cap edges; one genuinely separately
            // clicked BOTTOM material Edge survives removing the Face
            // gesture, regardless of coincident projected coordinates.
            auto mixed_session = makeBaseSession(kernel);
            const auto mixed_target =
                mixed_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(mixed_target.ok() && mixed_target.sketch_id);
            CHECK(workbench.activateDocument(
                &mixed_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* mixed_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    mixed_tree_item = *it;
                    break;
                }
            }
            CHECK(mixed_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(mixed_tree_item);
            mixed_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{
                    20.0, 15.0, 20.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 1U);
            reply = workbench.submitCadInput(
                "EDGES", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::bottom));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativeClick(
                *viewport, viewer::Point3{20.0, 0.0, 0.0}));
            const auto mixed_picks =
                controller->selectedMaterialEdgeReferences();
            CHECK(mixed_picks && mixed_picks->size() == 5U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 1U);
            reply = workbench.submitCadInput(
                "FACEBOUNDARY",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 5")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 1")));
            reply = workbench.submitCadInput(
                "REMOVE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Faces staged: 0")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_selected_overlays_current == 0U);
            CHECK(pg_finish->isEnabled());
            const auto mixed_previous_revision =
                mixed_session.document().revision();
            const auto mixed_previous_undo =
                mixed_session.undoDepth();
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(mixed_session.document().revision() !=
                  mixed_previous_revision);
            CHECK(mixed_session.undoDepth() ==
                  mixed_previous_undo + 1U);
            const auto* mixed_saved =
                mixed_session.document().findSketch(
                    *mixed_target.sketch_id);
            CHECK(mixed_saved &&
                  mixed_saved->projection_bindings.size() == 1U);
            CHECK(mixed_session.undo().changed);
            CHECK(mixed_session.document().findSketch(
                *mixed_target.sketch_id)
                ->projection_bindings.empty());
            CHECK(mixed_session.redo().changed);
            CHECK(mixed_session.document().findSketch(
                *mixed_target.sketch_id)
                ->projection_bindings.size() == 1U);
            std::cout
                << "PG01D_MANUAL_FACE_INDEPENDENT_EDGE_MIX_PASS"
                << " staged_face=1"
                << " staged_face_material=4"
                << " separate_bottom_edge=1"
                << " face_removed=1"
                << " manual_edge_survived=1"
                << " one_undo_redo=1"
                << '\n';

            result = EXIT_SUCCESS;
            workbench.close();
            app.quit();
            return;
        }
        if (pg01c_only) {
        // PG-01C C1: real OCCT native cursor -> Workbench Project Geometry,
        // not a fabricated presentation token. The separate later Sketch
        // makes the base Extrude a legal prior Body stage.
        auto pg_session = makeBaseSession(kernel);
        const auto pg_sketch =
            pg_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
        CHECK(pg_sketch.ok() && pg_sketch.sketch_id);
        CHECK(workbench.activateDocument(&pg_session, {}));
        QApplication::processEvents();

        auto* pg_action = workbench.findChild<QAction*>(
            QStringLiteral("editSketchAction"));
        auto* pg_button = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeToolButton"));
        auto* pg_panel = workbench.findChild<QWidget*>(
            QStringLiteral("projectEdgeOperationsWidget"));
        auto* pg_count = workbench.findChild<QLabel*>(
            QStringLiteral("projectEdgeSelectionLabel"));
        auto* pg_result = workbench.findChild<QLabel*>(
            QStringLiteral("projectEdgeResultLabel"));
        auto* pg_finish = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeFinishButton"));
        CHECK(pg_action && pg_button && pg_panel &&
              pg_count && pg_result && pg_finish);
        auto* pg_source_edges = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeSourceEdgesButton"));
        auto* pg_source_face = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeSourceFaceButton"));
        CHECK(pg_source_edges && pg_source_face);
        QTreeWidgetItem* pg_tree_item = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) == QStringLiteral("Sketch 2")) {
                pg_tree_item = *it;
                break;
            }
        }
        CHECK(pg_tree_item);
        tree->clearSelection();
        tree->setCurrentItem(pg_tree_item);
        pg_tree_item->setSelected(true);
        pg_action->trigger();
        QApplication::processEvents();
        CHECK(!pg_button->isHidden());
        CHECK(pg_button->isEnabled());

        const auto pg_before_state = pg_session.document().state();
        const auto pg_before_revision =
            pg_session.document().revision();
        const auto pg_before_undo = pg_session.undoDepth();
        auto pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_button->isChecked());
        CHECK(!pg_panel->isHidden());
        CHECK(!pg_finish->isEnabled());
        CHECK(workbench.acceptsEmptyCadInput());
        CHECK(pg_source_edges->isChecked());
        CHECK(!pg_source_face->isChecked());
        pg_reply = workbench.submitCadInput(
            "FACE", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_source_face->isChecked());
        CHECK(!pg_source_edges->isChecked());
        CHECK(!pg_finish->isEnabled());
        CHECK(pg_session.document().state() == pg_before_state);
        CHECK(pg_session.document().revision() == pg_before_revision);
        pg_reply = workbench.submitCadInput(
            "EDGES", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_source_edges->isChecked());
        CHECK(!pg_source_face->isChecked());
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "CONSTRUCTION", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        pg_reply = workbench.submitCadInput(
            "FINISH", workbench.cadInputContextGeneration());
        CHECK(!pg_reply.accepted);
        CHECK(pg_session.document().state() == pg_before_state);
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "CANCEL", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.document().state() == pg_before_state);

        // Repeat through the same semantic Command Line activation and
        // select an exact source Edge in the native provider.
        pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        const auto pg_probes = authorableEdgeProbes(pg_session, kernel);
        bool pg_picked = false;
        std::optional<viewer::Point3> pg_source_point;
        int native_queries = 0;
        int native_clicks = 0;
        // Try the same genuine native picking from several standard
        // cameras. Exact Body pick/occlusion and Sketch support overlays
        // vary by camera; no synthetic provider token is accepted.
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_right,
                 viewer::StandardView::bottom_back_left}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (const auto& probe : pg_probes) {
                // Only horizontal caps project as non-degenerate
                // Line onto the selected XY Sketch support.
                if (std::abs(probe.world.z) > 1.0e-6 &&
                    std::abs(probe.world.z - 20.0) > 1.0e-6) {
                    continue;
                }
                const auto point =
                    viewport->projectWorldPoint(probe.world);
                if (!point) continue;
                const auto query = viewport->queryBodyTopology(
                    *point,
                    viewer::BodyTopologyPickFilter{
                        false, true, false});
                if (query.valid() && query.completed &&
                    !query.candidates.empty()) {
                    ++native_queries;
                }
                if (!nativeClick(*viewport, probe.world)) {
                    continue;
                }
                ++native_clicks;
                std::cerr << "PG01C_NATIVE_PICK_ATTEMPT"
                          << " orientation=" << static_cast<int>(orientation)
                          << " label=" << pg_count->text().toStdString()
                          << " project_active=" << pg_button->isChecked()
                          << " query_edges=" << query.candidates.size()
                          << std::endl;
                CHECK(pg_button->isChecked());
                pg_picked = pg_count->text().contains(
                    QStringLiteral("selected: 1"));
                if (pg_picked) {
                    pg_source_point = probe.world;
                    break;
                }
                pg_reply = workbench.submitCadInput(
                    "CLEAR", workbench.cadInputContextGeneration());
                CHECK(pg_reply.accepted);
            }
            if (pg_picked) break;
        }
        std::cerr << "PG01C_NATIVE_PICK_SUMMARY"
                  << " queries=" << native_queries
                  << " clicks=" << native_clicks
                  << " selected=" << pg_count->text().toStdString()
                  << std::endl;
        CHECK(pg_picked && pg_source_point);
        CHECK(pg_finish->isEnabled());
        // C2: a live strict analytic projection preview, rather than
        // only a staged Edge token, is required to enable Finish.
        CHECK(pg_result->text().contains(
            QStringLiteral("Current preview: 1 derived Edge(s)")));
        CHECK(pg_result->text().contains(
            QStringLiteral("Regular")));
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "CONSTRUCTION", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_result->text().contains(
            QStringLiteral("Construction")));
        CHECK(pg_finish->isEnabled());
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "REGULAR", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_result->text().contains(
            QStringLiteral("Regular")));

        // Owner remediation: a real material vertical Edge projected
        // onto this XY Sketch is degenerate. Reject that *new* pick
        // without destroying an already staged valid horizontal Edge.
        ui::PartViewportController* pg_reject_controller = nullptr;
        for (auto* object : workbench.findChildren<QObject*>()) {
            if (auto* candidate =
                    dynamic_cast<ui::PartViewportController*>(
                        object)) {
                pg_reject_controller = candidate;
                break;
            }
        }
        CHECK(pg_reject_controller);
        const auto pg_prior_selection =
            pg_reject_controller->selectedMaterialEdgeReferences();
        CHECK(pg_prior_selection &&
              pg_prior_selection->size() == 1U);
        const auto pg_vertical = std::find_if(
            pg_probes.begin(), pg_probes.end(),
            [](const WorldEdgeProbe& probe) {
                return probe.world.z > 1.0e-6 &&
                       probe.world.z < 20.0 - 1.0e-6;
            });
        CHECK(pg_vertical != pg_probes.end());
        auto pg_unsupported_batch = *pg_prior_selection;
        pg_unsupported_batch.push_back(
            pg_vertical->reference);
        const auto pg_restored_tokens =
            pg_reject_controller->
                restoreMaterialEdgeToolSelection(
                    pg_unsupported_batch);
        CHECK(pg_restored_tokens &&
              pg_restored_tokens->empty());
        QApplication::processEvents();
        const auto pg_after_reject =
            pg_reject_controller->selectedMaterialEdgeReferences();
        CHECK(pg_after_reject &&
              *pg_after_reject == *pg_prior_selection);
        CHECK(pg_count->text().contains(
            QStringLiteral("selected: 1")));
        CHECK(pg_result->text().contains(
            QStringLiteral("Current preview: 1 derived Edge(s)")));
        CHECK(pg_finish->isEnabled());
        CHECK(pg_session.document().revision() ==
              pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        std::cout
            << "PG01C_OWNER_UNSUPPORTED_PICK_PRESERVES_STAGING_PASS"
            << " prior=1 rejected=1 committed=0\\n";

        // Rejected Command Line text is diagnostic-only: it does not
        // discard the exact staged source or leave the tool.
        pg_reply = workbench.submitCadInput(
            "UNSUPPORTED", workbench.cadInputContextGeneration());
        CHECK(!pg_reply.accepted);
        CHECK(pg_button->isChecked());
        CHECK(pg_finish->isEnabled());

        // Hierarchical Esc while a material source is staged: discard
        // pending selection first, exit Sketch Project tool on next Esc.
        QTest::keyClick(viewport, Qt::Key_Escape);
        QApplication::processEvents();
        CHECK(pg_button->isChecked());
        CHECK(!pg_panel->isHidden());
        CHECK(!pg_finish->isEnabled());
        CHECK(pg_count->text().contains(
            QStringLiteral("selected: 0")));
        CHECK(!pg_result->text().contains(
            QStringLiteral("Current preview:")));
        CHECK(pg_session.document().state() == pg_before_state);
        QTest::keyClick(viewport, Qt::Key_Escape);
        QApplication::processEvents();
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);

        // Toolbar and Command Line re-enter the *same* stage-scoped
        // material Edge picker; use the same genuine OCCT source click.
        pg_button->click();
        QApplication::processEvents();
        CHECK(pg_button->isChecked());
        CHECK(!pg_panel->isHidden());
        CHECK(nativeClick(*viewport, *pg_source_point));
        CHECK(pg_count->text().contains(
            QStringLiteral("selected: 1")));
        CHECK(pg_finish->isEnabled());

        pg_reply = workbench.submitCadInput(
            "REGULAR", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        pg_finish->click();
        QApplication::processEvents();
        const auto* pg_authored =
            pg_session.document().findSketch(*pg_sketch.sketch_id);
        CHECK(pg_authored);
        CHECK(pg_authored->projection_bindings.size() == 1U);
        CHECK(pg_authored->model.entityCount() == 1U);
        CHECK(pg_session.undoDepth() == pg_before_undo + 1U);
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.undo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        CHECK(pg_session.redo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);

        // D2-E: all three Finish adapters must reach the same atomic
        // command path. The above pass used the right-panel button;
        // redo/undo is followed by typed FINISH, then viewport Enter.
        auto* pg_undo = workbench.findChild<QPushButton*>(
            QStringLiteral("undoDocumentButton"));
        CHECK(pg_undo);
        pg_undo->click();
        QApplication::processEvents();
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(nativeClick(*viewport, *pg_source_point));
        CHECK(pg_finish->isEnabled());
        pg_reply = workbench.submitCadInput(
            "FINISH", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(!pg_button->isChecked());
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_session.undoDepth() == pg_before_undo + 1U);

        pg_undo->click();
        QApplication::processEvents();
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(nativeClick(*viewport, *pg_source_point));
        CHECK(pg_finish->isEnabled());
        QTest::keyClick(viewport, Qt::Key_Return);
        QApplication::processEvents();
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_session.undoDepth() == pg_before_undo + 1U);

        // PG-01C C3: durable Save -> close/open with a cold OCCT provider.
        // Use the native v15 .ss2part store, not a fabricated in-memory
        // restored Part or a serialized Viewer token.
        QTemporaryDir pg_persistence_dir;
        CHECK(pg_persistence_dir.isValid());
        const std::filesystem::path pg_persistence_path =
            std::filesystem::path{
                pg_persistence_dir.path().toStdWString()} /
            "ProjectGeometry.ss2part";
        const part::PartDocumentStore pg_store;
        const auto pg_saved = pg_store.createNew(
            pg_persistence_path, pg_session.document());
        CHECK(pg_saved.ok());
        auto pg_loaded = pg_store.load(pg_persistence_path);
        CHECK(pg_loaded.ok());
        CHECK(pg_loaded.document->documentId() ==
              pg_session.documentId());
        CHECK(pg_loaded.document->state() ==
              pg_session.document().state());
        application::DocumentSession pg_reopened{
            pg_persistence_path,
            std::move(*pg_loaded.document),
            *pg_loaded.checkpoint};
        kernel_occt::OcctSolidModelingKernel pg_cold_kernel;
        const auto pg_cold_evaluation = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        CHECK(pg_cold_evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto pg_cold_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(), *pg_sketch.sketch_id,
                pg_cold_evaluation, pg_cold_kernel);
        CHECK(pg_cold_projection);
        CHECK(pg_cold_projection->allResolved());
        CHECK(pg_cold_projection->outcomes.size() == 1U);
        CHECK(pg_cold_projection->model.entityCount() == 1U);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);

        // C3 D2-D: select the *linked Sketch curve* through a real
        // Viewport mouse pick after the Project tool has finished.
        // The Operations Break Link action must detach the currently
        // evaluated source geometry with one new Undo transaction.
        CHECK(workbench.activateDocument(&pg_session, {}));
        QApplication::processEvents();
        auto* pg_link_status = workbench.findChild<QLabel*>(
            QStringLiteral("projectLinkedEdgeStatusLabel"));
        auto* pg_break = workbench.findChild<QPushButton*>(
            QStringLiteral("projectBreakLinkButton"));
        CHECK(pg_link_status && pg_break);
        const auto* pg_linked =
            pg_session.document().findSketch(*pg_sketch.sketch_id);
        CHECK(pg_linked &&
              pg_linked->projection_bindings.size() == 1U);
        const auto pg_target =
            pg_linked->projection_bindings.front().target_entity;
        const auto* pg_line = pg_linked->model.findLine(pg_target);
        CHECK(pg_line);
        const viewer::Point3 pg_linked_midpoint{
            (pg_line->start().u + pg_line->end().u) / 2.0,
            (pg_line->start().v + pg_line->end().v) / 2.0,
            0.0};
        bool pg_sketch_selected = false;
        for (const auto orientation : {
                 viewer::StandardView::top,
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::bottom_front_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            const auto cursor =
                viewport->projectWorldPoint(pg_linked_midpoint);
            if (!cursor) continue;
            const QPoint pixel{
                static_cast<int>(std::lround(cursor->x)),
                static_cast<int>(std::lround(cursor->y))};
            if (!viewport->rect().contains(pixel)) continue;
            const auto queried =
                viewport->querySketchPresentation(*cursor);
            std::cerr
                << "PG01C_BREAK_LINK_SKETCH_PICK"
                << " view=" << static_cast<int>(orientation)
                << " completed=" << queried.completed
                << " token=" << queried.token.has_value()
                << std::endl;
            if (!queried.valid() || !queried.completed ||
                !queried.token) continue;
            QTest::mouseMove(viewport, pixel);
            QTest::mouseClick(
                viewport, Qt::LeftButton,
                Qt::NoModifier, pixel);
            QApplication::processEvents();
            pg_sketch_selected = !pg_break->isHidden() &&
                                 pg_break->isEnabled();
            if (pg_sketch_selected) break;
        }
        CHECK(pg_sketch_selected);
        CHECK(pg_link_status->text().contains(
            QStringLiteral("Projected Edge")));
        CHECK(pg_link_status->text().contains(
            QStringLiteral("Source stage:")));
        const auto pg_break_undo = pg_session.undoDepth();
        pg_break->click();
        QApplication::processEvents();
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.contains(pg_target));
        CHECK(pg_session.undoDepth() == pg_break_undo + 1U);
        CHECK(pg_session.undo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_session.redo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        // Open the saved native file as a new Workbench session after
        // the original was edited further. The persisted link is
        // independent of transient selection/Undo state of pg_session.
        CHECK(workbench.activateDocument(&pg_reopened, {}));
        QApplication::processEvents();
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        QTreeWidgetItem* pg_reopened_item = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) == QStringLiteral("Sketch 2")) {
                pg_reopened_item = *it;
                break;
            }
        }
        CHECK(pg_reopened_item);
        tree->clearSelection();
        tree->setCurrentItem(pg_reopened_item);
        pg_reopened_item->setSelected(true);
        pg_action->trigger();
        QApplication::processEvents();
        CHECK(!pg_button->isHidden());
        CHECK(viewport->setStandardView(
            viewer::StandardView::top));
        viewport->fitAll();
        QApplication::processEvents();
        const auto* pg_cold_line =
            pg_cold_projection->model.findLine(pg_target);
        CHECK(pg_cold_line);
        const viewer::Point3 pg_reopened_midpoint{
            (pg_cold_line->start().u +
             pg_cold_line->end().u) / 2.0,
            (pg_cold_line->start().v +
             pg_cold_line->end().v) / 2.0,
            0.0};
        const auto pg_reopened_cursor =
            viewport->projectWorldPoint(pg_reopened_midpoint);
        CHECK(pg_reopened_cursor);
        const auto pg_reopened_pick =
            viewport->querySketchPresentation(
                *pg_reopened_cursor);
        CHECK(pg_reopened_pick.valid());
        CHECK(pg_reopened_pick.completed);
        CHECK(pg_reopened_pick.token.has_value());

        // Source change after cold reopen: stretch the *upstream*
        // rectangle from 40 x 30 to 50 x 40 using one semantic
        // UpdateSketchLinesCommand. Every cap Edge changes, including
        // a bottom/left source; its linked local seed MUST remain
        // unchanged while current projection follows the new Body.
        const auto& pg_upstream =
            pg_reopened.document().state().sketches.front();
        CHECK(pg_upstream.id != *pg_sketch.sketch_id);
        std::vector<application::SketchLineGeometryUpdate>
            pg_upstream_updates;
        const auto upstream_lines = pg_upstream.model.state().lines;
        CHECK(upstream_lines.size() == 4U);
        for (const auto& item : upstream_lines) {
            const auto remap = [](sketch::Point2 point) {
                if (point.u == 40.0) point.u = 50.0;
                if (point.v == 30.0) point.v = 40.0;
                return point;
            };
            pg_upstream_updates.push_back({
                item.id,
                remap(item.start),
                remap(item.end)});
        }
        const auto pg_seed_before =
            pg_reopened.document()
                .findSketch(*pg_sketch.sketch_id)
                ->model.state();
        const auto pg_upstream_update =
            pg_reopened.execute(
                application::UpdateSketchLinesCommand{
                    pg_upstream.id,
                    pg_reopened.document().revision(),
                    std::move(pg_upstream_updates)});
        CHECK(pg_upstream_update.ok());
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.state() == pg_seed_before);
        const auto pg_updated_body =
            part::evaluatePart(
                pg_reopened.document(), pg_cold_kernel);
        CHECK(pg_updated_body.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto pg_updated_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_updated_body,
                pg_cold_kernel);
        CHECK(pg_updated_projection);
        CHECK(pg_updated_projection->allResolved());
        const auto* pg_updated_line =
            pg_updated_projection->model.findLine(pg_target);
        CHECK(pg_updated_line);
        CHECK(*pg_updated_line != *pg_cold_line);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);

        // Suppression/recovery on the exact same FeatureId must not
        // discard binding intent or materialize the old authored seed
        // as a currently resolved Sketch entity.
        CHECK(pg_reopened.document().state().body.features.size() == 1U);
        const auto pg_source_feature =
            pg_reopened.document().state().body.features.front().id;
        const auto pg_suppress = pg_reopened.execute(
            application::SetFeatureSuppressedCommand{
                pg_source_feature,
                pg_reopened.document().revision(),
                true});
        CHECK(pg_suppress.ok());
        const auto pg_missing_body = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        const auto pg_missing_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_missing_body,
                pg_cold_kernel);
        CHECK(!pg_missing_projection ||
              (!pg_missing_projection->allResolved() &&
               !pg_missing_projection->model.contains(pg_target)));
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.state() == pg_seed_before);

        const auto pg_unsuppress = pg_reopened.execute(
            application::SetFeatureSuppressedCommand{
                pg_source_feature,
                pg_reopened.document().revision(),
                false});
        CHECK(pg_unsuppress.ok());
        const auto pg_recovered_body = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        CHECK(pg_recovered_body.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto pg_recovered_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_recovered_body,
                pg_cold_kernel);
        CHECK(pg_recovered_projection);
        CHECK(pg_recovered_projection->allResolved());
        const auto* pg_recovered_line =
            pg_recovered_projection->model.findLine(pg_target);
        CHECK(pg_recovered_line);
        CHECK(*pg_recovered_line == *pg_updated_line);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.state() == pg_seed_before);
        // PG-01C C2: authored linked seed no longer closes a Profile
        // after the upstream resize, but the current effective source
        // does. Profile FIND and hover must follow the same rendered
        // Sketch snapshot as OSNAP/Measure, never the old seed.
        const auto pg_origin_a = pg_recovered_line->start();
        const auto pg_origin_b = pg_recovered_line->end();
        const sketch::Point2 pg_side{
            (pg_origin_a.v - pg_origin_b.v) * 0.25,
            (pg_origin_b.u - pg_origin_a.u) * 0.25};
        const sketch::Point2 pg_far_a{
            pg_origin_a.u + pg_side.u,
            pg_origin_a.v + pg_side.v};
        const sketch::Point2 pg_far_b{
            pg_origin_b.u + pg_side.u,
            pg_origin_b.v + pg_side.v};
        for (const auto& segment : {
                 std::pair{pg_origin_b, pg_far_b},
                 std::pair{pg_far_b, pg_far_a},
                 std::pair{pg_far_a, pg_origin_a}}) {
            const auto added = pg_reopened.execute(
                application::AddSketchLineCommand{
                    *pg_sketch.sketch_id,
                    segment.first,
                    segment.second,
                    sketch::EntityRole::regular});
            CHECK(added.ok());
        }
        const auto* pg_authored_region =
            pg_reopened.document().findSketch(
                *pg_sketch.sketch_id);
        CHECK(pg_authored_region);
        const auto pg_seed_analysis =
            sketch::analyzeRegions(pg_authored_region->model);
        CHECK(pg_seed_analysis.regions.empty());
        const auto pg_region_eval = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        const auto pg_region_effective =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_region_eval,
                pg_cold_kernel);
        CHECK(pg_region_effective);
        CHECK(pg_region_effective->allResolved());
        const auto pg_current_regions =
            sketch::analyzeRegions(pg_region_effective->model);
        CHECK(pg_current_regions.regions.size() == 1U);
        // The upstream update above intentionally bypassed the GUI.
        // Tear down the *old* edit context before revisiting this
        // Document, otherwise Workbench legitimately rejects a second
        // Edit Sketch and the revision-bound scene fails closed.
        CHECK(workbench.activateDocument(&session, {}));
        QApplication::processEvents();
        CHECK(workbench.activateDocument(&pg_reopened, {}));
        QApplication::processEvents();
        QTreeWidgetItem* pg_profile_sketch_item = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) ==
                QStringLiteral("Sketch 2")) {
                pg_profile_sketch_item = *it;
                break;
            }
        }
        CHECK(pg_profile_sketch_item);
        tree->clearSelection();
        tree->setCurrentItem(pg_profile_sketch_item);
        pg_profile_sketch_item->setSelected(true);
        pg_action->trigger();
        QApplication::processEvents();
        // Verify the exact scene authority supplied to Profile before
        // diagnosing any mouse-to-Sketch coordinate routing.
        ui::PartViewportController* pg_ui_controller = nullptr;
        for (auto* object :
             workbench.findChildren<QObject*>()) {
            if (auto* candidate =
                    dynamic_cast<ui::PartViewportController*>(
                        object)) {
                pg_ui_controller = candidate;
                break;
            }
        }
        CHECK(pg_ui_controller);
        const auto* pg_ui_model =
            pg_ui_controller->currentSketchInteractionModel();
        std::cerr << "PG01C_PROFILE_SCENE_DEBUG"
                  << " current_scene=" << (pg_ui_model != nullptr)
                  << " lines=" << (pg_ui_model
                                     ? pg_ui_model->state().lines.size()
                                     : 0U)
                  << " regions=" << (pg_ui_model
                                       ? sketch::analyzeRegions(
                                             *pg_ui_model).regions.size()
                                       : 0U)
                  << std::endl;
        CHECK(pg_ui_model);
        CHECK(sketch::analyzeRegions(*pg_ui_model)
                  .regions.size() == 1U);
        pg_reply = workbench.submitCadInput(
            "PROFILE", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        // UI Profile uses its existing tool/viewport pointer
        // grammar; FIND is intentionally not a global CAD keyword.
        // Hover/click the CURRENT derived interior, which does not
        // exist as a closed region in the authored linked seed model.
        auto* pg_profile_result = workbench.findChild<QLabel*>(
            QStringLiteral("profileCurrentResult"));
        auto* pg_profile_finish = workbench.findChild<QPushButton*>(
            QStringLiteral("profileFinishButton"));
        CHECK(pg_profile_result && pg_profile_finish);
        CHECK(viewport->setStandardView(
            viewer::StandardView::top));
        viewport->fitAll();
        QApplication::processEvents();
        const viewer::Point3 pg_region_world{
            (pg_origin_a.u + pg_origin_b.u + pg_side.u) * 0.5,
            (pg_origin_a.v + pg_origin_b.v + pg_side.v) * 0.5,
            0.0};
        // Prove the world-space point is inside the current
        // evaluated region before attributing any failure to Qt input.
        const auto pg_direct_pick = sketch::pickRegion(
            *pg_ui_model,
            sketch::analyzeRegions(*pg_ui_model),
            sketch::Point2{
                pg_region_world.x, pg_region_world.y});
        std::cerr << "PG01C_PROFILE_REGION_DIRECT"
                  << " location=" << static_cast<int>(
                         pg_direct_pick.location)
                  << " region=" << pg_direct_pick.region_index.has_value()
                  << std::endl;
        CHECK(pg_direct_pick.region_index.has_value());
        const auto pg_region_pos =
            viewport->projectWorldPoint(pg_region_world);
        CHECK(pg_region_pos);
        const QPoint pg_region_pixel{
            static_cast<int>(std::lround(pg_region_pos->x)),
            static_cast<int>(std::lround(pg_region_pos->y))};
        CHECK(viewport->rect().contains(pg_region_pixel));
        QTest::mouseMove(viewport, pg_region_pixel);
        QApplication::processEvents();
        auto* pg_profile_panel = workbench.findChild<QWidget*>(
            QStringLiteral("profileOperationsWidget"));
        auto* pg_status = workbench.findChild<QLabel*>(
            QStringLiteral("workbenchStatus"));
        CHECK(pg_profile_panel && pg_status);
        std::cerr << "PG01C_PROFILE_POINTER_DEBUG"
                  << " profile_panel_hidden=" << pg_profile_panel->isHidden()
                  << " finish_enabled=" << pg_profile_finish->isEnabled()
                  << " region_px=" << pg_region_pixel.x()
                  << "," << pg_region_pixel.y()
                  << " status=" << pg_status->text().toStdString()
                  << " result=" << pg_profile_result->text().toStdString()
                  << std::endl;
        QTest::mouseClick(
            viewport, Qt::LeftButton,
            Qt::NoModifier, pg_region_pixel);
        QApplication::processEvents();
        std::cerr << "PG01C_PROFILE_CLICK_DEBUG"
                  << " result=" << pg_profile_result->text().toStdString()
                  << " finish=" << pg_profile_finish->isEnabled()
                  << std::endl;
        CHECK(pg_profile_result->text().contains(
            QStringLiteral("Status: Valid")));
        CHECK(pg_profile_finish->isEnabled());
        CHECK(pg_profile_result->text().contains(
            QStringLiteral("Status: Valid")));
        pg_reply = workbench.submitCadInput(
            "CANCEL", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        std::cout
            << "PG01C_C2_CURRENT_PROFILE_REGIONS_PASS"
            << " persisted_seed_regions=0"
            << " current_linked_regions=1"
            << " native_profile_hover=1"
            << " native_profile_draft=1\\n";
        std::cout
            << "PG01C_C3_SUPPRESSION_RECOVERY_PASS"
            << " no_saved_seed_fallback=1"
            << " same_source_id=1"
            << " exact_reprojection=1\\n";
        std::cout
            << "PG01C_C3_UPSTREAM_RECOMPUTE_PASS"
            << " current_curve_changed=1"
            << " authored_seed_unchanged=1"
            << " cold_occt=1\\n";
        std::cout
            << "PG01C_C3_NATIVE_SAVE_REOPEN_PASS"
            << " native_v15=1"
            << " cold_occt=1"
            << " current_linked_scene=1\\n";
        std::cout
            << "PG01C_C3_NATIVE_BREAK_LINK_PASS"
            << " real_sketch_pick=1"
            << " current_geometry=1"
            << " undo_redo=1\\n";
        std::cout
            << "PG01C_C1_NATIVE_EDGE_TOOL_PASS"
            << " actual_cursor=1"
            << " semantic_finish=1"
            << " cancel_zero_mutation=1"
            << " undo_redo=1\n";

            // PG-01C C1 acceptance: two different actual OCCT cap
            // material Edges are staged through one native multi-pick
            // session (Ctrl-click adds to the staged selection), and
            // Finish authors two linked entities in ONE Undo entry.
            auto pg_batch_session = makeBaseSession(kernel);
            const auto pg_batch_sketch =
                pg_batch_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(pg_batch_sketch.ok() && pg_batch_sketch.sketch_id);
            CHECK(workbench.activateDocument(&pg_batch_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* pg_batch_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    pg_batch_item = *it;
                    break;
                }
            }
            CHECK(pg_batch_item);
            tree->clearSelection();
            tree->setCurrentItem(pg_batch_item);
            pg_batch_item->setSelected(true);
            pg_action->trigger();
            QApplication::processEvents();
            pg_reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(pg_reply.accepted);
            const auto pg_batch_before_undo =
                pg_batch_session.undoDepth();
            const auto pg_batch_before_revision =
                pg_batch_session.document().revision();
            bool pg_batch_picked = false;
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_right,
                     viewer::StandardView::bottom_back_left}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                if (!nativeClick(*viewport, *pg_source_point) ||
                    !pg_count->text().contains(
                        QStringLiteral("selected: 1"))) {
                    pg_reply = workbench.submitCadInput(
                        "CLEAR", workbench.cadInputContextGeneration());
                    CHECK(pg_reply.accepted);
                    continue;
                }
                for (const auto& probe : pg_probes) {
                    if (std::abs(probe.world.z) > 1.0e-6 &&
                        std::abs(probe.world.z - 20.0) > 1.0e-6) {
                        continue;
                    }
                    if (probe.world == *pg_source_point) {
                        continue;
                    }
                    const auto pixel_pos =
                        viewport->projectWorldPoint(probe.world);
                    if (!pixel_pos) continue;
                    const QPoint pixel{
                        static_cast<int>(std::lround(pixel_pos->x)),
                        static_cast<int>(std::lround(pixel_pos->y))};
                    if (!viewport->rect().contains(pixel)) continue;
                    QTest::mouseMove(viewport, pixel);
                    QTest::mouseClick(
                        viewport, Qt::LeftButton,
                        Qt::ControlModifier, pixel);
                    QApplication::processEvents();
                    if (pg_count->text().contains(
                            QStringLiteral("selected: 2")) &&
                        pg_finish->isEnabled()) {
                        pg_batch_picked = true;
                        break;
                    }
                    pg_reply = workbench.submitCadInput(
                        "CLEAR", workbench.cadInputContextGeneration());
                    CHECK(pg_reply.accepted);
                    CHECK(nativeClick(*viewport, *pg_source_point));
                    CHECK(pg_count->text().contains(
                        QStringLiteral("selected: 1")));
                }
                if (pg_batch_picked) break;
                pg_reply = workbench.submitCadInput(
                    "CLEAR", workbench.cadInputContextGeneration());
                CHECK(pg_reply.accepted);
            }
            CHECK(pg_batch_picked);
            CHECK(pg_result->text().contains(
                QStringLiteral("Current preview: 2 derived Edge(s)")));
            CHECK(pg_batch_session.document().revision() ==
                  pg_batch_before_revision);
            CHECK(pg_batch_session.undoDepth() ==
                  pg_batch_before_undo);
            pg_finish->click();
            QApplication::processEvents();
            const auto* pg_batch_authored =
                pg_batch_session.document().findSketch(
                    *pg_batch_sketch.sketch_id);
            CHECK(pg_batch_authored);
            CHECK(pg_batch_authored->projection_bindings.size() == 2U);
            CHECK(pg_batch_session.undoDepth() ==
                  pg_batch_before_undo + 1U);
            const auto pg_batch_links =
                pg_batch_authored->projection_bindings;
            CHECK(pg_batch_session.undo().changed);
            CHECK(pg_batch_session.document()
                      .findSketch(*pg_batch_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(pg_batch_session.redo().changed);
            CHECK(pg_batch_session.document()
                      .findSketch(*pg_batch_sketch.sketch_id)
                      ->projection_bindings == pg_batch_links);
            std::cout << "PG01C_C1_TWO_EDGE_ATOMIC_PASS"
                      << " native_ctrl_selection=1"
                      << " linked_edges=2"
                      << " undo_batches=1\\n";

            // D2-K synthetic native OCCT regression: a rectangle
            // after Chamfer is authored from four real same-stage
            // material Edges. Their projected end vertices must be
            // exactly connected; no Sketcher tolerance healing.
            auto pg_chamfer_session = makeBaseSession(kernel);
            // A skewed source forces nontrivial analytic endpoint
            // evaluation. Equal authored source vertices are mapped
            // with one function so they remain exactly shared.
            const auto pg_skew_authored =
                pg_chamfer_session.document().state();
            const auto& pg_skew_source =
                pg_skew_authored.sketches.front();
            const double skew_cos = std::cos(0.31);
            const double skew_sin = std::sin(0.31);
            const auto rotate = [skew_cos, skew_sin](
                                    sketch::Point2 p) {
                return sketch::Point2{
                    p.u * skew_cos - p.v * skew_sin,
                    p.u * skew_sin + p.v * skew_cos};
            };
            std::vector<application::SketchLineGeometryUpdate>
                pg_skew_updates;
            for (const auto& line :
                 pg_skew_source.model.state().lines) {
                pg_skew_updates.push_back({
                    line.id,
                    rotate(line.start),
                    rotate(line.end)});
            }
            CHECK(pg_skew_updates.size() == 4U);
            CHECK(pg_chamfer_session.execute(
                application::UpdateSketchLinesCommand{
                    pg_skew_source.id,
                    pg_chamfer_session.document().revision(),
                    std::move(pg_skew_updates)}).ok());
            std::vector<part::MaterialEdgeReference>
                pg_pre_chamfer_cap;
            for (const auto& probe :
                 authorableEdgeProbes(pg_chamfer_session, kernel)) {
                if (std::abs(probe.world.z - 20.0) < 1.0e-6) {
                    pg_pre_chamfer_cap.push_back(probe.reference);
                }
            }
            CHECK(pg_pre_chamfer_cap.size() == 4U);
            std::sort(
                pg_pre_chamfer_cap.begin(),
                pg_pre_chamfer_cap.end());
            const auto pg_chamfer =
                pg_chamfer_session.execute(
                    application::CreateChamferFeatureCommand{
                        pg_pre_chamfer_cap,
                        pg_chamfer_session.document().revision(),
                        core::LengthValue{2.0},
                        "PG01C vertex continuity Chamfer"},
                    kernel);
            CHECK(pg_chamfer.ok());
            const auto pg_chamfer_sketch =
                pg_chamfer_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(pg_chamfer_sketch.ok() &&
                  pg_chamfer_sketch.sketch_id);
            std::vector<part::MaterialEdgeReference>
                pg_post_chamfer_cap;
            for (const auto& probe :
                 authorableEdgeProbes(pg_chamfer_session, kernel)) {
                if (std::abs(probe.world.z - 20.0) < 1.0e-6) {
                    pg_post_chamfer_cap.push_back(probe.reference);
                }
            }
            CHECK(pg_post_chamfer_cap.size() == 4U);
            const auto pg_chamfer_projected =
                pg_chamfer_session.execute(
                    application::CreateProjectedSketchEdgesCommand{
                        *pg_chamfer_sketch.sketch_id,
                        pg_chamfer_session.document().revision(),
                        pg_post_chamfer_cap,
                        sketch::EntityRole::regular},
                    kernel);
            CHECK(pg_chamfer_projected.ok());
            CHECK(pg_chamfer_projected.entity_ids.size() == 4U);
            const auto pg_chamfer_current =
                part::evaluateEffectiveSketchProjection(
                    pg_chamfer_session.document(),
                    *pg_chamfer_sketch.sketch_id,
                    part::evaluatePart(
                        pg_chamfer_session.document(), kernel),
                    kernel);
            CHECK(pg_chamfer_current &&
                  pg_chamfer_current->allResolved());
            const auto pg_chamfer_regions =
                sketch::analyzeRegions(pg_chamfer_current->model);
            CHECK(pg_chamfer_regions.complete());
            CHECK(pg_chamfer_regions.regions.size() == 1U);
            const auto pg_chamfer_intent =
                part::makeProfileRegionIntent(
                    pg_chamfer_regions.regions.front());
            CHECK(pg_chamfer_intent);
            const auto pg_chamfer_profile =
                pg_chamfer_session.execute(
                    application::CreateProfileCommand{
                        *pg_chamfer_sketch.sketch_id,
                        pg_chamfer_session.document().revision(),
                        *pg_chamfer_intent},
                    kernel);
            CHECK(pg_chamfer_profile.ok());
            std::cout << "PG01C_D2K_CHAMFER_LINKED_REGION_PASS"
                      << " edges=4 native_occt=1"
                      << " no_tolerance_heal=1\\n";

            // Owner identity regression: an all-linked rectangle
            // Profile remains the SAME Profile as each linked Edge is
            // detached, including mixed linked/authored intermediate
            // boundaries. No ProfileId or RegionIntent recreation.
            auto pg_all_session = makeBaseSession(kernel);
            const auto pg_all_sketch =
                pg_all_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(pg_all_sketch.ok() &&
                  pg_all_sketch.sketch_id);
            std::vector<part::MaterialEdgeReference>
                pg_cap_sources;
            for (const auto& probe :
                 authorableEdgeProbes(pg_all_session, kernel)) {
                if (std::abs(probe.world.z - 20.0) < 1.0e-6) {
                    pg_cap_sources.push_back(probe.reference);
                }
            }
            CHECK(pg_cap_sources.size() == 4U);
            const auto pg_all_authored =
                pg_all_session.execute(
                    application::CreateProjectedSketchEdgesCommand{
                        *pg_all_sketch.sketch_id,
                        pg_all_session.document().revision(),
                        pg_cap_sources,
                        sketch::EntityRole::regular},
                    kernel);
            CHECK(pg_all_authored.ok());
            CHECK(pg_all_authored.entity_ids.size() == 4U);
            const auto* pg_all_linked =
                pg_all_session.document().findSketch(
                    *pg_all_sketch.sketch_id);
            CHECK(pg_all_linked &&
                  pg_all_linked->projection_bindings.size() == 4U);
            const auto pg_all_targets =
                pg_all_linked->projection_bindings;
            const auto pg_all_body = part::evaluatePart(
                pg_all_session.document(), kernel);
            const auto pg_all_effective =
                part::evaluateEffectiveSketchProjection(
                    pg_all_session.document(),
                    *pg_all_sketch.sketch_id,
                    pg_all_body, kernel);
            CHECK(pg_all_effective &&
                  pg_all_effective->allResolved());
            const auto pg_all_regions =
                sketch::analyzeRegions(pg_all_effective->model);
            CHECK(pg_all_regions.complete() &&
                  pg_all_regions.regions.size() == 1U);
            const auto pg_all_intent =
                part::makeProfileRegionIntent(
                    pg_all_regions.regions.front());
            CHECK(pg_all_intent);

            // D2-P: change the upstream rectangle *after* projecting
            // all four linked targets. Persisted linked seeds stay
            // identical; the new current region (50x40) must be the
            // semantic authority for Create Profile, not old 40x30.
            auto pg_changed_doc =
                part::PartDocument::restore(
                    pg_all_session.document().documentId(),
                    pg_all_session.document().state(),
                    pg_all_session.document().revision());
            CHECK(pg_changed_doc.ok());
            application::DocumentSession pg_changed_session{
                {}, std::move(*pg_changed_doc.document)};
            const auto pg_changed_authored_state =
                pg_changed_session.document().state();
            const auto& pg_changed_upstream =
                pg_changed_authored_state.sketches.front();
            CHECK(pg_changed_upstream.id != *pg_all_sketch.sketch_id);
            std::vector<application::SketchLineGeometryUpdate>
                pg_changed_updates;
            const auto pg_changed_lines =
                pg_changed_upstream.model.state().lines;
            CHECK(pg_changed_lines.size() == 4U);
            for (const auto& item : pg_changed_lines) {
                const auto remap = [](sketch::Point2 p) {
                    if (p.u == 40.0) p.u = 50.0;
                    if (p.v == 30.0) p.v = 40.0;
                    return p;
                };
                pg_changed_updates.push_back({
                    item.id, remap(item.start), remap(item.end)});
            }
            const auto pg_changed_seed =
                pg_changed_session.document()
                    .findSketch(*pg_all_sketch.sketch_id)->model.state();
            const auto pg_changed_result =
                pg_changed_session.execute(
                    application::UpdateSketchLinesCommand{
                        pg_changed_upstream.id,
                        pg_changed_session.document().revision(),
                        std::move(pg_changed_updates)});
            CHECK(pg_changed_result.ok());
            CHECK(pg_changed_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->model.state() == pg_changed_seed);
            const auto pg_changed_effective =
                part::evaluateEffectiveSketchProjection(
                    pg_changed_session.document(),
                    *pg_all_sketch.sketch_id,
                    part::evaluatePart(
                        pg_changed_session.document(), kernel),
                    kernel);
            CHECK(pg_changed_effective &&
                  pg_changed_effective->allResolved());
            const auto pg_changed_regions =
                sketch::analyzeRegions(pg_changed_effective->model);
            CHECK(pg_changed_regions.complete() &&
                  pg_changed_regions.regions.size() == 1U);
            CHECK(pg_changed_regions.regions.front().area !=
                  pg_all_regions.regions.front().area);
            const auto pg_changed_intent =
                part::makeProfileRegionIntent(
                    pg_changed_regions.regions.front());
            CHECK(pg_changed_intent);
            const auto pg_changed_rev =
                pg_changed_session.document().revision();
            const auto pg_changed_reject =
                pg_changed_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_changed_rev,
                        *pg_changed_intent});
            CHECK(!pg_changed_reject.ok());
            CHECK(pg_changed_session.document().revision() ==
                  pg_changed_rev);
            const auto pg_changed_profile =
                pg_changed_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_changed_rev,
                        *pg_changed_intent},
                    kernel);
            CHECK(pg_changed_profile.ok() &&
                  pg_changed_profile.profile_id);
            CHECK(pg_changed_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->model.state() == pg_changed_seed);
            std::cout << "PG01C_D2P_MOVED_SOURCE_CREATE_PASS"
                      << " authored_seed_stale=1"
                      << " current_provider=1"
                      << " no_provider_rejected=1\\n";

            const auto pg_legacy_revision =
                pg_all_session.document().revision();
            const auto pg_legacy_undo =
                pg_all_session.undoDepth();
            const auto pg_legacy_rejected =
                pg_all_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_legacy_revision,
                        *pg_all_intent});
            CHECK(!pg_legacy_rejected.ok());
            CHECK(pg_all_session.document().revision() ==
                  pg_legacy_revision);
            CHECK(pg_all_session.undoDepth() ==
                  pg_legacy_undo);
            const auto pg_all_created =
                pg_all_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_all_session.document().revision(),
                        *pg_all_intent},
                    kernel);
            CHECK(pg_all_created.ok() &&
                  pg_all_created.profile_id);
            const auto pg_all_profile =
                *pg_all_session.document().findProfile(
                    *pg_all_created.profile_id);
            // The authored-only API intentionally refuses linked
            // Profile evaluation; effective source is the authority.
            CHECK(!pg_all_session.document()
                      .evaluateProfile(pg_all_profile.id));

            // D2-T real Workbench Tree must agree with the effective
            // Viewer/Properties Profile, not label all-linked geometry
            // Invalid merely because authored-only evaluation refuses.
            CHECK(workbench.activateDocument(&pg_all_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* pg_linked_profile_tree = nullptr;
            const auto pg_profile_identity =
                QString::fromStdString(
                    pg_all_profile.id.serialized());
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->toolTip(0).contains(
                        QStringLiteral("ProfileId: ") +
                        pg_profile_identity)) {
                    pg_linked_profile_tree = *it;
                    break;
                }
            }
            CHECK(pg_linked_profile_tree != nullptr);
            CHECK(!pg_linked_profile_tree->text(0).contains(
                QStringLiteral("[Invalid]")));
            CHECK(pg_linked_profile_tree->toolTip(0).contains(
                QStringLiteral("Status: Valid")));
            std::cout << "PG01C_D2T_LINKED_TREE_VALID_PASS"
                      << " all_linked=4"
                      << " authored_only_rejected=1\\n";

            // D2-B: four linked Edges are detached in one semantic
            // transaction without changing Profile or EntityIds. Invalid
            // duplicate input rolls back the whole staged operation.
            auto pg_clone =
                part::PartDocument::restore(
                    pg_all_session.document().documentId(),
                    pg_all_session.document().state(),
                    pg_all_session.document().revision());
            CHECK(pg_clone.ok());
            application::DocumentSession pg_batch_detach_session{
                {}, std::move(*pg_clone.document)};
            std::vector<sketch::EntityId> pg_batch_targets;
            for (const auto& binding : pg_all_targets) {
                pg_batch_targets.push_back(binding.target_entity);
            }
            const auto pg_batch_revision =
                pg_batch_detach_session.document().revision();
            const auto pg_batch_undo =
                pg_batch_detach_session.undoDepth();
            const auto pg_batch_reject =
                pg_batch_detach_session.execute(
                    application::BreakProjectedEdgeLinksCommand{
                        *pg_all_sketch.sketch_id,
                        {pg_batch_targets.front(),
                         pg_batch_targets.front()},
                        pg_batch_revision},
                    kernel);
            CHECK(!pg_batch_reject.ok());
            CHECK(pg_batch_detach_session.document().revision() ==
                  pg_batch_revision);
            CHECK(pg_batch_detach_session.undoDepth() ==
                  pg_batch_undo);
            const auto pg_batch_detach =
                pg_batch_detach_session.execute(
                    application::BreakProjectedEdgeLinksCommand{
                        *pg_all_sketch.sketch_id,
                        pg_batch_targets,
                        pg_batch_revision},
                    kernel);
            CHECK(pg_batch_detach.ok());
            CHECK(pg_batch_detach_session.undoDepth() ==
                  pg_batch_undo + 1U);
            CHECK(pg_batch_detach_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(*pg_batch_detach_session.document().findProfile(
                      pg_all_profile.id) == pg_all_profile);
            const auto* pg_detach_authored =
                pg_batch_detach_session.document().findSketch(
                    *pg_all_sketch.sketch_id);
            CHECK(pg_detach_authored);
            for (const auto& binding : pg_all_targets) {
                const auto* actual =
                    pg_detach_authored->model.findLine(
                        binding.target_entity);
                const auto* expected =
                    pg_all_effective->model.findLine(
                        binding.target_entity);
                CHECK(actual && expected && *actual == *expected);
            }
            CHECK(pg_batch_detach_session.document()
                      .evaluateProfile(pg_all_profile.id)->valid());
            CHECK(pg_batch_detach_session.undo().changed);
            CHECK(pg_batch_detach_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.size() == 4U);
            CHECK(*pg_batch_detach_session.document().findProfile(
                      pg_all_profile.id) == pg_all_profile);
            CHECK(pg_batch_detach_session.redo().changed);
            CHECK(pg_batch_detach_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(pg_batch_detach_session.document()
                      .evaluateProfile(pg_all_profile.id)->valid());
            std::cout << "PG01C_D2B_ATOMIC_BREAK_LINK_PASS"
                      << " linked=4 undo_batches=1"
                      << " invalid_rollbacks=1"
                      << " profile_unchanged=1\\n";

            for (const auto& binding : pg_all_targets) {
                const auto before_body = part::evaluatePart(
                    pg_all_session.document(), kernel);
                const auto before =
                    part::evaluateEffectiveSketchProjection(
                        pg_all_session.document(),
                        *pg_all_sketch.sketch_id,
                        before_body, kernel);
                CHECK(before && before->allResolved());
                const auto* before_line =
                    before->model.findLine(
                        binding.target_entity);
                CHECK(before_line);
                const auto expected_line = *before_line;
                const auto detached = pg_all_session.execute(
                    application::BreakProjectedEdgeLinkCommand{
                        *pg_all_sketch.sketch_id,
                        binding.target_entity,
                        pg_all_session.document().revision()},
                    kernel);
                CHECK(detached.ok());
                const auto* after_sketch =
                    pg_all_session.document().findSketch(
                        *pg_all_sketch.sketch_id);
                CHECK(after_sketch);
                const auto* frozen_line =
                    after_sketch->model.findLine(
                        binding.target_entity);
                CHECK(frozen_line &&
                      *frozen_line == expected_line);
                CHECK(*pg_all_session.document().findProfile(
                          pg_all_profile.id) == pg_all_profile);
                const auto after_body = part::evaluatePart(
                    pg_all_session.document(), kernel);
                const auto after =
                    part::evaluateEffectiveSketchProjection(
                        pg_all_session.document(),
                        *pg_all_sketch.sketch_id,
                        after_body, kernel);
                CHECK(after && after->allResolved());
                const auto resolved =
                    part::resolveProfileRegionIntent(
                        after->model,
                        pg_all_profile.region_intent);
                CHECK(resolved.valid());
                CHECK(resolved.region->area ==
                      pg_all_regions.regions.front().area);
            }
            CHECK(pg_all_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.empty());
            const auto pg_all_final =
                pg_all_session.document().evaluateProfile(
                    pg_all_profile.id);
            CHECK(pg_all_final && pg_all_final->valid());
            CHECK(pg_all_session.undo().changed);
            CHECK(pg_all_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.size() == 1U);
            CHECK(*pg_all_session.document().findProfile(
                      pg_all_profile.id) == pg_all_profile);
            CHECK(pg_all_session.redo().changed);
            CHECK(pg_all_session.document()
                      .evaluateProfile(pg_all_profile.id)
                      ->valid());
            QTemporaryDir pg_all_dir;
            CHECK(pg_all_dir.isValid());
            const auto pg_all_path =
                std::filesystem::path{
                    pg_all_dir.path().toStdWString()} /
                "LinkedProfileBreak.ss2part";
            const auto pg_all_saved =
                pg_store.createNew(
                    pg_all_path, pg_all_session.document());
            CHECK(pg_all_saved.ok());
            const auto pg_all_loaded =
                pg_store.load(pg_all_path);
            CHECK(pg_all_loaded.ok());
            CHECK(*pg_all_loaded.document->findProfile(
                      pg_all_profile.id) == pg_all_profile);
            CHECK(pg_all_loaded.document
                      ->evaluateProfile(pg_all_profile.id)
                      ->valid());
            std::cout
                << "PG01C_OWNER_PROFILE_LINK_BREAK_IDENTITY_PASS"
                << " all_linked=4 mixed_then_authored=1"
                << " stable_ids=1 undo_redo=1"
                << " cold_reopen=1\\n";

            CHECK(workbench.activateDocument(&session, {}));
            result = EXIT_SUCCESS;
            workbench.close();
            app.quit();
            return;
        }
        CHECK(viewport->setStandardView(
            viewer::StandardView::top_front_right));
        viewport->fitAll();
        QApplication::processEvents();
        const auto probes = authorableEdgeProbes(session, kernel);

        // UI-01 G3 positive control for newly created Features:
        // while Create owns the native draft, its ORIGINAL toolbar
        // button is engaged; an authored Feature Edit is not.
        auto* create_fillet_button =
            workbench.findChild<QPushButton*>(
                QStringLiteral("filletToolButton"));
        auto* create_chamfer_button =
            workbench.findChild<QPushButton*>(
                QStringLiteral("chamferToolButton"));
        CHECK(create_fillet_button && create_chamfer_button);
        CHECK(!create_fillet_button->isChecked());
        CHECK(!create_chamfer_button->isChecked());
        // Create the original Fillet through actual native cursor input,
        // not a fabricated provider-neutral PresentationToken intent.
        auto reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        QApplication::processEvents();
        CHECK(create_fillet_button->isChecked());
        CHECK(!create_chamfer_button->isChecked());
        reply = workbench.lockCadDynamicInputField(
            0U, "1", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        std::optional<WorldEdgeProbe> first;
        for (const auto& probe : probes) {
            if (!nativeClick(*viewport, probe.world)) {
                continue;
            }
            if (selectedCount(*label, 1) && finish->isEnabled()) {
                first = probe;
                break;
            }
            reply = workbench.submitCadInput(
                "CLEAR", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
        }
        CHECK(first.has_value());
        CHECK(finish->isEnabled());
        // R2-D: real Workbench preview stays local, transient and distinct
        // from the neutral displayed accepted Body.
        const auto creating_preview =
            viewport->runtimeDiagnostics();
        CHECK(creating_preview.solid_preview_displayed);
        CHECK(creating_preview.solid_preview_style_expected);
        CHECK(creating_preview.solid_committed_displayed);
        CHECK(creating_preview.solid_committed_style_expected);
        CHECK(creating_preview.solid_shading_styles_isolated);
        finish->click();
        QApplication::processEvents();
        CHECK(!create_fillet_button->isChecked());
        CHECK(!create_chamfer_button->isChecked());
        CHECK(session.document().body().features.size() == 2U);
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        const auto feature_id =
            session.document().body().features.back().id;
        const auto* feature =
            session.document().findFeature(feature_id);
        CHECK(feature != nullptr);
        const auto* authored =
            std::get_if<part::FilletFeature>(&feature->definition);
        CHECK(authored != nullptr);
        CHECK(authored->edges.size() == 1U);
        const auto originally_authored = authored->edges.front();

        auto* item = featureTreeItem(*tree, feature_id);
        CHECK(item != nullptr);
        tree->clearSelection();
        tree->setCurrentItem(item);
        item->setSelected(true);
        QApplication::processEvents();
        CHECK(edit->isEnabled());
        const auto undo_before_edit = session.undoDepth();
        edit->click();
        QApplication::processEvents();
        CHECK(selectedCount(*label, 1));
        CHECK(viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(viewport->runtimeDiagnostics().solid_preview_style_expected);

        // This is the central R2-B acceptance boundary: real native click,
        // tool_stage Viewer's query, controller semantic selection and
        // Workbench draft state must all agree.
        CHECK(nativeClick(*viewport, first->world));
        CHECK(selectedCount(*label, 0));
        CHECK(!finish->isEnabled());
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(nativeClick(*viewport, first->world));
        CHECK(selectedCount(*label, 1));
        CHECK(finish->isEnabled());
        CHECK(viewport->runtimeDiagnostics().solid_preview_displayed);

        std::optional<WorldEdgeProbe> second;
        for (const auto& probe : probes) {
            if (probe.reference == originally_authored) continue;
            if (!nativeClick(*viewport, probe.world)) continue;
            if (selectedCount(*label, 2)) {
                second = probe;
                break;
            }
            if (selectedCount(*label, 0)) {
                CHECK(nativeClick(*viewport, first->world));
                CHECK(selectedCount(*label, 1));
            }
        }
        CHECK(second.has_value());
        CHECK(nativeClick(*viewport, second->world));
        CHECK(selectedCount(*label, 1));
        CHECK(finish->isEnabled());
        // Run the Tree edit-entry matrix only AFTER the R2-B cursor
        // sequence; changing Tree inspection selection must not alter
        // an active Feature draft, but it can affect cursor highlighting.
        verifyUi01FeatureDraftRejectsTreeEdits(
            workbench, session, *tree, item,
            "edgeFeatureOperationsWidget");
        cancel->click();
        QApplication::processEvents();
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(session.undoDepth() == undo_before_edit);
        CHECK(session.document().body().features.size() == 2U);
        feature = session.document().findFeature(feature_id);
        CHECK(feature != nullptr);
        authored =
            std::get_if<part::FilletFeature>(&feature->definition);
        CHECK(authored != nullptr);
        CHECK(authored->edges.size() == 1U);
        CHECK(authored->edges.front() == originally_authored);

        item = featureTreeItem(*tree, feature_id);
        CHECK(item != nullptr);
        tree->clearSelection();
        tree->setCurrentItem(item);
        item->setSelected(true);
        QApplication::processEvents();
        CHECK(edit->isEnabled());
        edit->click();
        QApplication::processEvents();
        CHECK(selectedCount(*label, 1));
        reply = workbench.lockCadDynamicInputField(
            0U, "1.25", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        CHECK(finish->isEnabled());
        finish->click();
        QApplication::processEvents();
        CHECK(session.undoDepth() == undo_before_edit + 1U);
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(session.document().findFeature(feature_id) != nullptr);
        const auto evaluation =
            part::evaluatePart(session.document(), kernel);
        CHECK(evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(evaluation.current_topology.has_value());
        CHECK(evaluation.current_topology->complete());

        // R2-C/E: real displayed OCCT analytic ring Edge, not a synthetic
        // scene, for full 360-degree and partial 90-degree Revolve Bodies.
        // Both operations consume the same semantic MaterialEdgeReference
        // contract. The preview is transient; Finish publishes the Body.
        // Retain both DocumentSession objects throughout the whole
        // Workbench switch sequence: the active document pointer must
        // never outlive a loop-local temporary during deactivation.
        auto full_revolve_session =
            makeRevolveSession(2.0 * std::acos(-1.0));
        auto partial_revolve_session =
            makeRevolveSession(std::acos(-1.0) / 2.0);
        for (int variant = 0; variant < 2; ++variant) {
            auto& revolve_session =
                variant == 0
                    ? full_revolve_session
                    : partial_revolve_session;
            const double angle =
                variant == 0
                    ? 2.0 * std::acos(-1.0)
                    : std::acos(-1.0) / 2.0;
            const char* command =
                variant == 0 ? "FILLET" : "CHAMFER";
            CHECK(workbench.activateDocument(
                &revolve_session, {}));
            QApplication::processEvents();
            // UI-01 P0 Revolve Edit: Tree Sketch/Profile attempts must
            // reject without touching this particular Revolve draft.
            auto* revolve_item = featureTreeItem(
                *tree, revolve_session.document()
                    .body().features.front().id);
            auto* revolve_cancel = workbench.findChild<QPushButton*>(
                QStringLiteral("revolveCancelButton"));
            auto* revolve_operations = workbench.findChild<QWidget*>(
                QStringLiteral("revolveOperationsWidget"));
            CHECK(revolve_item && revolve_cancel && revolve_operations);
            tree->clearSelection();
            tree->setCurrentItem(revolve_item);
            revolve_item->setSelected(true);
            QApplication::processEvents();
            CHECK(edit->isEnabled());
            edit->click();
            QApplication::processEvents();
            CHECK(!revolve_operations->isHidden());
            verifyUi01FeatureDraftRejectsTreeEdits(
                workbench, revolve_session, *tree, revolve_item,
                "revolveOperationsWidget");
            revolve_cancel->click();
            QApplication::processEvents();
            CHECK(revolve_operations->isHidden());
            CHECK(viewport->setStandardView(
                viewer::StandardView::top_front_right));
            viewport->fitAll();
            QApplication::processEvents();

            reply = workbench.submitCadInput(
                command,
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            QApplication::processEvents();
            CHECK(create_fillet_button->isChecked() ==
                  (variant == 0));
            CHECK(create_chamfer_button->isChecked() ==
                  (variant == 1));
            reply = workbench.lockCadDynamicInputField(
                0U, "0.75",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(clickAuthorableRevolveCircle(
                *viewport, revolve_session, kernel, *label));
            CHECK(selectedCount(*label, 1));
            CHECK(finish->isEnabled());
            finish->click();
            QApplication::processEvents();
            CHECK(!create_fillet_button->isChecked());
            CHECK(!create_chamfer_button->isChecked());
            CHECK(
                revolve_session.document().body()
                    .features.size() == 2U);
            const auto outcome =
                part::evaluatePart(
                    revolve_session.document(), kernel);
            CHECK(outcome.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(outcome.current_topology);
            CHECK(outcome.current_topology->complete());
            CHECK(!viewport->runtimeDiagnostics()
                       .solid_preview_displayed);
            CHECK(viewport->runtimeDiagnostics()
                      .solid_committed_displayed);
            std::cout
                << "PM05F_R2_NATIVE_REVOLVE_EDGE_PASS"
                << " angle=" << angle
                << " operation=" << command << '\n';
        }

        // R2-E: an actual OCCT circle sampled with the production
        // BodyPresentation path must remain selectable at high zoom.
        // The analytic midpoint is only a cursor-position test probe:
        // authoring always uses the strict Part semantic Curve catalog.
        auto precision_session =
            makeRevolveSession(2.0 * std::acos(-1.0));
        CHECK(workbench.activateDocument(
            &precision_session, {}));
        QApplication::processEvents();
        const auto precision_evaluation =
            part::evaluatePart(
                precision_session.document(), kernel);
        CHECK(precision_evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(precision_evaluation.current_topology);
        const auto precision_body =
            kernel.bodyPresentation(
                precision_evaluation.body_solid);
        CHECK(precision_body.ok());
        double max_screen_gap = 0.0;
        std::size_t testable_curved_segments = 0U;
        std::size_t missed_curved_segments = 0U;
        for (const auto& edge :
             precision_body.body.edges) {
            if (edge.points.size() < 3U) continue;
            const auto resolved =
                part::authorMaterialEdgeReference(
                    *precision_evaluation.current_topology,
                    edge.runtime_token);
            if (!resolved.ok()) continue;
            // The fixture is a circle about the world X-axis.
            // Only its front x=20 cap rim is guaranteed unobstructed.
            if (std::abs(edge.points.front().x - 20.0) >
                1.0e-5) {
                continue;
            }
            const auto& a = edge.points[0];
            const auto& b = edge.points[1];
            const double radius =
                std::hypot(a.y, a.z);
            if (radius < 9.0 ||
                std::abs(std::hypot(b.y, b.z) - radius) >
                    1.0e-4) {
                continue;
            }
            const double mid_y = a.y + b.y;
            const double mid_z = a.z + b.z;
            const double mid_len =
                std::hypot(mid_y, mid_z);
            if (mid_len <= 0.0) continue;
            const viewer::Point3 true_mid{
                a.x,
                mid_y * radius / mid_len,
                mid_z * radius / mid_len};
            // Verify both a tight view and a much tighter view against
            // the *production* OCCT edge path, not a densely hand-built
            // synthetic ring. Pixel-distance evidence never becomes CAD
            // identity; visible, authorable material Edges must hit.
            for (const double view_height : {8.0, 2.0}) {
                const viewer::CameraState close_view{
                    viewer::Point3{100.0, 0.0, 0.0},
                    true_mid,
                    viewer::Vec3{0.0, 0.0, 1.0},
                    viewer::CameraProjection::orthographic,
                    view_height};
                CHECK(viewport->setCameraState(close_view));
                QApplication::processEvents();
                const auto sa =
                    viewport->projectWorldPoint(
                        {a.x, a.y, a.z});
                const auto sb =
                    viewport->projectWorldPoint(
                        {b.x, b.y, b.z});
                const auto sm =
                    viewport->projectWorldPoint(true_mid);
                CHECK(sa && sb && sm);
                const double dx = sb->x - sa->x;
                const double dy = sb->y - sa->y;
                const double squared = dx * dx + dy * dy;
                CHECK(squared > 0.0);
                const double t = std::clamp(
                    ((sm->x - sa->x) * dx +
                     (sm->y - sa->y) * dy) / squared,
                    0.0, 1.0);
                const double screen_gap = std::hypot(
                    sm->x - sa->x - t * dx,
                    sm->y - sa->y - t * dy);
                max_screen_gap =
                    std::max(max_screen_gap, screen_gap);
                const auto q =
                    viewport->queryBodyTopology(
                        *sm,
                        viewer::BodyTopologyPickFilter{
                            false, true, false});
                CHECK(q.valid() && q.completed);
                CHECK(q.generation.valid());
                const auto expected =
                    std::any_of(
                        q.candidates.begin(),
                        q.candidates.end(),
                        [](const auto& candidate) {
                            return candidate.kind ==
                                viewer::BodyTopologyPresentationKind::edge;
                        });
                ++testable_curved_segments;
                if (!expected) {
                    ++missed_curved_segments;
                }
                std::cerr
                    << "PM05F_R2_HIGH_ZOOM_CURVE"
                    << " scale=" << view_height
                    << " deflection_gap_pixels=" << screen_gap
                    << " edge_found=" << expected
                    << " samples=" << edge.points.size()
                    << '\n';
            }
            break;
        }
        CHECK(testable_curved_segments > 0U);
        CHECK(missed_curved_segments == 0U);
        std::cout
            << "PM05F_R2_CURVE_PICK_HIGH_ZOOM"
            << " max_chord_gap_px=" << max_screen_gap
            << " tested=" << testable_curved_segments
            << '\n';

        // R2-A: native full click -> Workbench three converging Edge
        // selection -> Fillet Finish -> fresh Part evaluation and visible
        // committed Body. Coordinates here only choose mouse positions,
        // never persistent Edge identity.
        auto trihedral_session =
            makeBaseSession(kernel, 20.0, 20.0, 20.0);
        CHECK(workbench.activateDocument(
            &trihedral_session, {}));
        QApplication::processEvents();
        CHECK(viewport->setStandardView(
            viewer::StandardView::top_front_right));
        viewport->fitAll();
        QApplication::processEvents();
        const auto corners =
            authorableTrihedralCornerProbes(
                trihedral_session, kernel);
        reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        reply = workbench.lockCadDynamicInputField(
            0U, "2", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);

        bool trihedral_selected = false;
        for (const auto& corner : corners) {
            reply = workbench.submitCadInput(
                "CLEAR", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            bool all_clicked = true;
            for (std::size_t index = 0U;
                 index < corner.size(); ++index) {
                if (!nativeClick(*viewport, corner[index]) ||
                    !selectedCount(
                        *label,
                        static_cast<int>(index + 1U))) {
                    all_clicked = false;
                    break;
                }
            }
            if (all_clicked && finish->isEnabled()) {
                trihedral_selected = true;
                break;
            }
        }
        CHECK(trihedral_selected);
        CHECK(selectedCount(*label, 3));
        CHECK(finish->isEnabled());
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        finish->click();
        QApplication::processEvents();
        CHECK(trihedral_session.document()
                  .body().features.size() == 2U);
        const auto after_three =
            part::evaluatePart(
                trihedral_session.document(), kernel);
        CHECK(after_three.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(after_three.current_topology);
        CHECK(after_three.current_topology->complete());
        CHECK(!viewport->runtimeDiagnostics()
                   .solid_preview_displayed);
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_style_expected);
        std::cout
            << "PM05F_R2_NATIVE_TRIHEDRAL_FILLET_PASS"
            << " body_present=1"
            << " edges=3"
            << " radius=2"
            << '\n';

        // R2-A Owner attempt 3 (Part013): persist-equivalent history.
        // A 20 x 20 x 20 box in negative Y, three specific upper
        // (20, -20, 20) material Edges, Fillet radius 2. Unlike the
        // native mouse control above, this isolates reconstructed CAD
        // evaluation and body presentation from cursor/camera effects.
        // The private Owner document is not checked into the repository.
        auto owner_corner_session =
            makeBaseSession(kernel, 20.0, -20.0, 20.0);
        const auto owner_base =
            part::evaluatePart(
                owner_corner_session.document(), kernel);
        CHECK(owner_base.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(owner_base.current_topology);
        CHECK(owner_base.current_topology->complete());
        const auto owner_base_presentation =
            kernel.bodyPresentation(owner_base.body_solid);
        CHECK(owner_base_presentation.ok());

        const auto is_owner_vertex =
            [](const auto& point) {
                constexpr double tolerance = 1.0e-5;
                return std::abs(point.x - 20.0) < tolerance &&
                       std::abs(point.y + 20.0) < tolerance &&
                       std::abs(point.z - 20.0) < tolerance;
            };
        std::vector<part::MaterialEdgeReference> owner_edges;
        for (const auto& edge :
             owner_base_presentation.body.edges) {
            if (edge.points.size() < 2U ||
                (!is_owner_vertex(edge.points.front()) &&
                 !is_owner_vertex(edge.points.back()))) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *owner_base.current_topology,
                    edge.runtime_token);
            CHECK(authored.ok());
            owner_edges.push_back(*authored.reference);
        }
        CHECK(owner_edges.size() == 3U);
        std::sort(owner_edges.begin(), owner_edges.end());
        CHECK(std::adjacent_find(
            owner_edges.begin(), owner_edges.end()) ==
            owner_edges.end());

        auto owner_state = owner_corner_session.document().state();
        const auto owner_fillet_id =
            owner_state.body.next_feature_id.allocate();
        CHECK(owner_fillet_id.has_value());
        owner_state.body.features.push_back(
            part::PartFeature{
                *owner_fillet_id,
                "Fillet002",
                false,
                part::FilletFeature{
                    owner_edges,
                    core::LengthValue{2.0}}});
        auto owner_restored =
            part::PartDocument::restore(
                owner_corner_session.document().documentId(),
                std::move(owner_state),
                owner_corner_session.document().revision());
        CHECK(owner_restored.ok());
        application::DocumentSession owner_committed{
            {}, std::move(*owner_restored.document)};
        const auto owner_result =
            part::evaluatePart(owner_committed.document(), kernel);
        const bool owner_body_up_to_date =
            owner_result.body_status ==
                part::BodyEvaluationStatus::up_to_date &&
            owner_result.body_solid != nullptr &&
            owner_result.current_topology &&
            owner_result.current_topology->complete();
        CHECK(owner_body_up_to_date);

        const auto owner_presentation =
            kernel.bodyPresentation(owner_result.body_solid);
        std::cerr
            << "PM05F_R2_OWNER_NEGATIVE_Y_CORNER"
            << " body_up_to_date=1"
            << " presentation_status="
            << static_cast<int>(owner_presentation.status)
            << " presentation_ok="
            << (owner_presentation.ok() ? 1 : 0)
            << " faces="
            << owner_presentation.body.faces.size()
            << " edges="
            << owner_presentation.body.edges.size()
            << '\n';
        CHECK(owner_presentation.ok());

        CHECK(workbench.activateDocument(&owner_committed, {}));
        QApplication::processEvents();
        const auto owner_viewer =
            viewport->runtimeDiagnostics();
        std::cerr
            << "PM05F_R2_OWNER_NEGATIVE_Y_VIEWER"
            << " body_displayed="
            << (owner_viewer.solid_committed_displayed ? 1 : 0)
            << " expected_style="
            << (owner_viewer.solid_committed_style_expected ? 1 : 0)
            << '\n';
        CHECK(owner_viewer.solid_committed_displayed);
        CHECK(owner_viewer.solid_committed_style_expected);

        // Owner Part013's actual authored sketch segment order, Profile
        // region intent and three durable MaterialEdgeReferences are read
        // from a sanitized copy of the native document. This catches
        // differences that an equivalent bounding box cannot reproduce.
        // Only regenerated document/sketch UUIDs differ from Owner input.
        const auto owner_fixture =
            std::filesystem::path{__FILE__}.parent_path() /
            "fixtures" /
            "pm05f_r2_part013_sanitized.ss2part";
        const part::PartDocumentStore owner_store;
        auto owner_loaded = loadOwnerFixtureV2ForTest(owner_store, owner_fixture);
        CHECK(owner_loaded.ok());
        application::DocumentSession owner_exact_session{
            {}, std::move(*owner_loaded.document)};
        const auto owner_exact_result =
            part::evaluatePart(owner_exact_session.document(), kernel);
        const bool owner_exact_up_to_date =
            owner_exact_result.body_status ==
                part::BodyEvaluationStatus::up_to_date &&
            owner_exact_result.body_solid != nullptr &&
            owner_exact_result.current_topology &&
            owner_exact_result.current_topology->complete();
        std::cerr
            << "PM05F_R2_OWNER_EXACT_NATIVE"
            << " body_up_to_date="
            << (owner_exact_up_to_date ? 1 : 0)
            << '\n';
        CHECK(owner_exact_up_to_date);
        const auto owner_exact_presentation =
            kernel.bodyPresentation(owner_exact_result.body_solid);
        std::cerr
            << "PM05F_R2_OWNER_EXACT_PRESENTATION"
            << " status="
            << static_cast<int>(owner_exact_presentation.status)
            << " ok="
            << (owner_exact_presentation.ok() ? 1 : 0)
            << " faces="
            << owner_exact_presentation.body.faces.size()
            << " edges="
            << owner_exact_presentation.body.edges.size()
            << '\n';
        CHECK(owner_exact_presentation.ok());
        // Audit two-point Edge presentation spans in the exact stored
        // Owner geometry. A mathematically collapsed path cannot form
        // an OCCT AIS_Wire; this is not a license to fabricate CAD
        // geometry or silently enlarge numerical tolerances.
        for (const auto& edge :
             owner_exact_presentation.body.edges) {
            if (edge.points.size() != 2U) {
                continue;
            }
            const auto& a = edge.points.front();
            const auto& b = edge.points.back();
            const auto length = std::hypot(
                std::hypot(b.x - a.x, b.y - a.y),
                b.z - a.z);
            std::cerr
                << "PM05F_R2_OWNER_TWO_POINT_EDGE"
                << " runtime_token=" << edge.runtime_token.value
                << " length_mm=" << length
                << " a=" << a.x << "," << a.y << "," << a.z
                << " b=" << b.x << "," << b.y << "," << b.z
                << '\n';
        }
        // Owner screenshot uses Shaded + Edges, not the widget's
        // default Shaded mode. Style synchronization builds an
        // additional OCCT wire for every material Edge and must never
        // make the otherwise valid committed Body disappear.
        // Capture skipped OCCT outline objects. A visible Body alone
        // is necessary but does not prove complete material Edge drawing.
        owner_edge_outline_warnings.store(
            0U, std::memory_order_relaxed);
        const auto old_qt_handler =
            qInstallMessageHandler(
                captureOwnerEdgeOutlineWarning);
        CHECK(workbench.activateDocument(&session, {}));
        QApplication::processEvents();
        const auto owner_shaded_edges_enabled =
            viewport->setViewStyle(
                viewer::ViewStyle::shaded_with_edges);
        std::cerr
            << "PM05F_R2_OWNER_EDGE_STYLE_ENABLED="
            << (owner_shaded_edges_enabled ? 1 : 0)
            << '\n';
        CHECK(owner_shaded_edges_enabled);
        CHECK(workbench.activateDocument(&owner_exact_session, {}));
        QApplication::processEvents();
        const auto owner_exact_viewer = viewport->runtimeDiagnostics();
        std::cerr
            << "PM05F_R2_OWNER_EXACT_VIEWER"
            << " body_displayed="
            << (owner_exact_viewer.solid_committed_displayed ? 1 : 0)
            << '\n';
        CHECK(owner_exact_viewer.solid_committed_displayed);
        CHECK(owner_exact_viewer.solid_committed_style_expected);
        // A failure in optional Edge outlines must not hide the current
        // solid when switching among all supported view styles.
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_hidden_edges));
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded));
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_edges));
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        qInstallMessageHandler(old_qt_handler);
        const auto owner_skipped =
            owner_edge_outline_warnings.load(
                std::memory_order_relaxed);
        std::cerr
            << "PM05F_R2_OWNER_OUTLINE_SKIPPED="
            << owner_skipped << '\n';
        CHECK(owner_skipped == 0U);

        // R2 P0 Owner Part008: two opposite-direction Add Extrudes,
        // intersecting at the XY plane. This exact persisted profile
        // geometry differs from all prior concave Cut-pocket controls.
        // The private document/sketch UUIDs are replaced in the fixture.
        const auto crash_fixture =
            std::filesystem::path{__FILE__}.parent_path() /
            "fixtures" /
            "pm05f_r2_part008_sanitized.ss2part";
        const part::PartDocumentStore crash_store;
        auto crash_loaded = loadOwnerFixtureV2ForTest(crash_store, crash_fixture);
        if (!crash_loaded.ok()) {
            std::cerr
                << "PM05F_R2_PART008_LOAD_FAILED"
                << " code="
                << static_cast<int>(crash_loaded.diagnostic.code)
                << " message="
                << crash_loaded.diagnostic.message
                << std::endl;
        }
        CHECK(crash_loaded.ok());
        application::DocumentSession crash_session{
            {}, std::move(*crash_loaded.document)};
        const auto crash_eval =
            part::evaluatePart(crash_session.document(), kernel);
        std::cerr
            << "PM05F_R2_PART008_EVALUATION"
            << " body_status="
            << static_cast<int>(crash_eval.body_status)
            << " features=" << crash_eval.features.size()
            << std::endl;
        CHECK(crash_eval.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(crash_eval.features.size() == 2U);
        CHECK(crash_eval.current_topology);
        CHECK(crash_eval.current_topology->complete());
        const auto crash_presentation =
            kernel.bodyPresentation(crash_eval.body_solid);
        CHECK(crash_presentation.ok());

        // Screenshot: selected count=1, radius=2, current stage
        // After Feature 2; carrier from Extrude001 (Feature 1),
        // side/side between Sketch entities 3 and 4. Resolve it
        // semantically from the current Body. Never persist a
        // provider runtime token or guess by XYZ nearest geometry.
        std::optional<part::MaterialEdgeReference>
            screenshot_edge;
        std::vector<viewer::Point3> screenshot_points;
        std::size_t matching_screenshot_edges = 0U;
        for (const auto& edge :
             crash_presentation.body.edges) {
            const auto found =
                std::find_if(
                    crash_eval.current_topology->edges.begin(),
                    crash_eval.current_topology->edges.end(),
                    [&edge](const auto& candidate) {
                        return candidate.runtime_token ==
                               edge.runtime_token;
                    });
            if (found ==
                crash_eval.current_topology->edges.end()) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *crash_eval.current_topology,
                    edge.runtime_token);
            if (!authored.ok()) {
                continue;
            }
            const auto& curve = authored.reference->curve;
            if (curve.producer_feature_id.serialized() != "1" ||
                curve.role != part::FeatureCurveRoleKind::side_side ||
                curve.adjacent_surfaces.size() != 2U) {
                continue;
            }
            std::vector<std::string> side_entities;
            for (const auto& adjacent :
                 curve.adjacent_surfaces) {
                if (adjacent.producer_feature_id.serialized() != "1" ||
                    adjacent.role !=
                        part::FeatureSurfaceRoleKind::side ||
                    !adjacent.source_entity) {
                    continue;
                }
                side_entities.push_back(
                    adjacent.source_entity->serialized());
            }
            std::sort(
                side_entities.begin(),
                side_entities.end());
            if (side_entities !=
                std::vector<std::string>{"3", "4"}) {
                continue;
            }
            ++matching_screenshot_edges;
            screenshot_edge = *authored.reference;
            screenshot_points.clear();
            for (const auto& point : edge.points) {
                screenshot_points.push_back({
                    point.x, point.y, point.z});
            }
            std::cerr
                << "PM05F_R2_PART008_SCREENSHOT_EDGE"
                << " runtime_token=" << edge.runtime_token.value
                << " point_count=" << edge.points.size()
                << std::endl;
        }
        CHECK(matching_screenshot_edges == 1U);
        CHECK(screenshot_edge);
        CHECK(screenshot_points.size() >= 2U);

        // First isolate the fully evaluated candidate and exact local
        // material delta from real pointer/GUI dispatch.
        auto screenshot_draft =
            application::FilletDraft::beginCreate(
                crash_session, {*screenshot_edge});
        CHECK(screenshot_draft);
        CHECK(screenshot_draft->setRadius(
            core::LengthValue{2.0}));
        const auto prior_revision =
            crash_session.document().revision();
        std::cerr
            << "PM05F_R2_PART008_DRAFT_BEGIN"
            << " radius=2 selection=1"
            << std::endl;
        const auto screenshot_preview =
            crash_session.evaluateFilletDraft(
                *screenshot_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_DRAFT_END"
            << " status="
            << static_cast<int>(screenshot_preview.status)
            << " target="
            << (screenshot_preview.target_status
                ? static_cast<int>(*screenshot_preview.target_status)
                : -1)
            << std::endl;
        CHECK(screenshot_preview.status ==
                  application::EdgeFeatureDraftEvaluationStatus::ok ||
              screenshot_preview.status ==
                  application::EdgeFeatureDraftEvaluationStatus::
                      target_failed);
        CHECK(crash_session.document().revision() ==
              prior_revision);

        // P0 Part008: this is the *second* edge transition missing
        // from FULL #1813. Candidate identities are obtained from
        // the current strict material Edge catalog. Prioritize true
        // two-producer Boolean intersections, then ordinary Edges.
        // Never persist or guess OCCT runtime tokens for authored CAD.
        struct Part008SecondaryEdge {
            part::MaterialEdgeReference reference;
            part::FeatureCurveRoleKind role;
            std::vector<viewer::Point3> points;
        };
        std::vector<Part008SecondaryEdge> secondary_edges;
        for (const auto& edge :
             crash_presentation.body.edges) {
            const auto authored =
                part::authorMaterialEdgeReference(
                    *crash_eval.current_topology,
                    edge.runtime_token);
            if (!authored.ok() ||
                *authored.reference == *screenshot_edge ||
                edge.points.size() < 2U) {
                continue;
            }
            const auto already_added =
                std::any_of(
                    secondary_edges.begin(),
                    secondary_edges.end(),
                    [&](const auto& item) {
                        return item.reference ==
                            *authored.reference;
                    });
            if (already_added) {
                continue;
            }
            Part008SecondaryEdge next{
                *authored.reference,
                authored.reference->curve.role,
                {}};
            for (const auto& point : edge.points) {
                next.points.push_back({
                    point.x, point.y, point.z});
            }
            secondary_edges.push_back(std::move(next));
        }
        std::stable_sort(
            secondary_edges.begin(),
            secondary_edges.end(),
            [](const auto& a, const auto& b) {
                const auto intersection =
                    part::FeatureCurveRoleKind::
                        boolean_intersection;
                return (a.role == intersection) &&
                       (b.role != intersection);
            });
        CHECK(!secondary_edges.empty());
        std::cerr
            << "PM05F_R2_PART008_SECOND_CANDIDATES"
            << " count=" << secondary_edges.size()
            << std::endl;

        // #1814 stopped on the SECOND candidate (index 1) while the
        // first candidate had completed. Isolate that one exact
        // semantic two-Edge pair without trying any other pair.
        constexpr std::size_t failing_candidate_index = 1U;
        CHECK(secondary_edges.size() > failing_candidate_index);
        const auto& isolated_secondary =
            secondary_edges[failing_candidate_index];
        std::cerr
            << "PM05F_R2_PART008_ISOLATED_CANDIDATE"
            << " index=" << failing_candidate_index
            << " role=" << static_cast<int>(isolated_secondary.role)
            << " producer="
            << isolated_secondary.reference.curve.producer_feature_id.serialized()
            << " points=" << isolated_secondary.points.size()
            << std::endl;
        for (const auto& surface :
             isolated_secondary.reference.curve.adjacent_surfaces) {
            std::cerr
                << "PM05F_R2_PART008_CANDIDATE_SURFACE"
                << " producer=" << surface.producer_feature_id.serialized()
                << " role=" << static_cast<int>(surface.role)
                << " sketch_entity="
                << (surface.source_entity
                    ? surface.source_entity->serialized()
                    : std::string{"none"})
                << std::endl;
        }
        for (const auto& point : isolated_secondary.points) {
            std::cerr
                << "PM05F_R2_PART008_CANDIDATE_POINT"
                << " x=" << point.x
                << " y=" << point.y
                << " z=" << point.z
                << std::endl;
        }

        const auto input =
            part::resolveKernelEdgeFeatureInput(
                part::FilletFeature{
                    {*screenshot_edge, isolated_secondary.reference},
                    core::LengthValue{2.0}},
                &*crash_eval.current_topology);
        CHECK(input.ok());
        CHECK(input.input->edges.size() == 2U);
        std::cerr
            << "PM05F_R2_PART008_KERNEL_DIRECT_BEGIN"
            << " edge0=" << input.input->edges[0].value
            << " edge1=" << input.input->edges[1].value
            << std::endl;
        const auto direct =
            kernel.edgeFeature(
                *input.input,
                crash_eval.body_solid);
        std::cerr
            << "PM05F_R2_PART008_KERNEL_DIRECT_END"
            << " status=" << static_cast<int>(direct.status)
            << std::endl;
        if (direct.status ==
                kernel::SolidModelingStatus::ok) {
            std::cerr
                << "PM05F_R2_PART008_DELTA_DIRECT_BEGIN"
                << std::endl;
            const auto delta =
                kernel.materialDifferencePreview(
                    crash_eval.body_solid, direct.solid);
            std::cerr
                << "PM05F_R2_PART008_DELTA_DIRECT_END"
                << " status=" << static_cast<int>(delta.status)
                << " removed=" << (delta.removed ? 1 : 0)
                << " added=" << (delta.added ? 1 : 0)
                << std::endl;
            CHECK(delta.status ==
                  kernel::SolidPresentationStatus::ok);
            CHECK(delta.removed.has_value());
            CHECK(delta.added.has_value());
            std::cerr
                << "PM05F_R2_PART008_RESULT_PRESENTATION_BEGIN"
                << std::endl;
            const auto result_presentation =
                kernel.bodyPresentation(direct.solid);
            std::cerr
                << "PM05F_R2_PART008_RESULT_PRESENTATION_END"
                << " status="
                << static_cast<int>(result_presentation.status)
                << " faces="
                << result_presentation.body.faces.size()
                << " edges="
                << result_presentation.body.edges.size()
                << std::endl;
            CHECK(result_presentation.ok());
        }

        auto isolated_draft =
            application::FilletDraft::beginCreate(
                crash_session, {*screenshot_edge});
        CHECK(isolated_draft);
        CHECK(isolated_draft->setRadius(core::LengthValue{2.0}));
        CHECK(isolated_draft->setEdges(
            {*screenshot_edge, isolated_secondary.reference}));
        std::cerr
            << "PM05F_R2_PART008_ISOLATED_DRAFT_BEGIN"
            << std::endl;
        const auto isolated_result =
            crash_session.evaluateFilletDraft(
                *isolated_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_ISOLATED_DRAFT_END"
            << " status=" << static_cast<int>(isolated_result.status)
            << std::endl;
        CHECK(isolated_result.status ==
                  application::EdgeFeatureDraftEvaluationStatus::ok ||
              isolated_result.status ==
                  application::EdgeFeatureDraftEvaluationStatus::
                      target_failed);
        CHECK(crash_session.document().revision() ==
              prior_revision);

        // Exercise the same single selected Edge through native Qt/OCCT
        // click routing. Query and hit-testing remain authoritative;
        // sample alternate standard views to find an unoccluded view.
        CHECK(workbench.activateDocument(&crash_session, {}));
        QApplication::processEvents();
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_edges));
        auto crash_reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(crash_reply.accepted);
        crash_reply = workbench.lockCadDynamicInputField(
            0U, "2", workbench.cadInputContextGeneration());
        CHECK(crash_reply.accepted);
        std::cerr
            << "PM05F_R2_PART008_GUI_BEFORE_CLICK"
            << std::endl;
        bool clicked = false;
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_left,
                 viewer::StandardView::bottom_back_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (std::size_t k = 1U;
                 k < screenshot_points.size() &&
                 !clicked; ++k) {
                const auto& a = screenshot_points[k - 1U];
                const auto& b = screenshot_points[k];
                const viewer::Point3 point{
                    (a.x + b.x) / 2.0,
                    (a.y + b.y) / 2.0,
                    (a.z + b.z) / 2.0};
                if (!nativeClick(*viewport, point)) {
                    continue;
                }
                clicked = selectedCount(*label, 1);
                std::cerr
                    << "PM05F_R2_PART008_GUI_CLICK"
                    << " selected_one=" << clicked
                    << std::endl;
            }
            if (clicked) break;
        }
        CHECK(clicked);
        CHECK(selectedCount(*label, 1));
        std::cerr
            << "PM05F_R2_PART008_GUI_ONE_EDGE_PASS"
            << std::endl;

        // Attempt the transition selected 1 -> selected 2 through the
        // native pointer path (not synthetic draft setEdges). A selected
        // second Edge invokes Workbench's synchronous exact delta preview.
        // Log before each native click so a Debug CRT assertion isolates
        // the point of failure even if the process terminates.
        bool selected_two = false;
        for (std::size_t i = failing_candidate_index;
             i <= failing_candidate_index && !selected_two; ++i) {
            const auto& second = secondary_edges[i];
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_left,
                     viewer::StandardView::bottom_back_right}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                for (std::size_t k = 1U;
                     k < second.points.size() && !selected_two;
                     ++k) {
                    const auto& a = second.points[k - 1U];
                    const auto& b = second.points[k];
                    const viewer::Point3 point{
                        (a.x + b.x) / 2.0,
                        (a.y + b.y) / 2.0,
                        (a.z + b.z) / 2.0};
                    const auto screen =
                        viewport->projectWorldPoint(point);
                    if (!screen) {
                        continue;
                    }
                    const QPoint pixel{
                        static_cast<int>(std::lround(screen->x)),
                        static_cast<int>(std::lround(screen->y))};
                    if (!viewport->rect().contains(pixel)) {
                        continue;
                    }
                    const auto query =
                        viewport->queryBodyTopology(
                            *screen,
                            viewer::BodyTopologyPickFilter{
                                false, true, false});
                    if (!query.valid() || !query.completed ||
                        query.candidates.empty()) {
                        continue;
                    }
                    std::cerr
                        << "PM05F_R2_PART008_SECOND_CLICK_BEGIN"
                        << " index=" << i
                        << " role=" << static_cast<int>(second.role)
                        << " segment=" << k
                        << std::endl;
                    QTest::mouseMove(viewport, pixel);
                    QTest::mouseClick(
                        viewport, Qt::LeftButton,
                        Qt::NoModifier, pixel);
                    QApplication::processEvents();
                    selected_two = selectedCount(*label, 2);
                    std::cerr
                        << "PM05F_R2_PART008_SECOND_CLICK_END"
                        << " index=" << i
                        << " selected_two=" << selected_two
                        << " selected_one=" << selectedCount(*label, 1)
                        << std::endl;
                    if (!selectedCount(*label, 1) &&
                        !selected_two) {
                        // A hit on the already selected Edge toggles
                        // it off. Do not silently assert we selected a
                        // second authored Edge; report diagnostic RED.
                        break;
                    }
                }
                if (!selectedCount(*label, 1) && !selected_two) {
                    break;
                }
                if (selected_two) break;
            }
            if (!selectedCount(*label, 1) && !selected_two) {
                break;
            }
        }
        CHECK(selected_two);
        std::cerr
            << "PM05F_R2_PART008_GUI_TWO_EDGE_PASS"
            << std::endl;
        CHECK(finish->isEnabled());
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_BEGIN"
            << " features="
            << crash_session.document().body().features.size()
            << std::endl;
        finish->click();
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_CLICK_RETURNED"
            << std::endl;
        QApplication::processEvents();
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_EVENTS_RETURNED"
            << std::endl;
        CHECK(crash_session.document().revision() !=
              prior_revision);
        CHECK(crash_session.document().body()
                  .features.size() == 3U);
        std::cerr
            << "PM05F_R2_PART008_REEVALUATION_BEGIN"
            << std::endl;
        const auto crash_committed =
            part::evaluatePart(crash_session.document(), kernel);
        std::cerr
            << "PM05F_R2_PART008_REEVALUATION_END"
            << " body_status="
            << static_cast<int>(crash_committed.body_status)
            << std::endl;
        CHECK(crash_committed.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_PASS"
            << std::endl;

        // The Owner has PASS for this Part008 Fillet R2/Finish.
        // Independently cover the complementary equal-distance Chamfer
        // on the *same original two-Extrude* Body stage. Using a fresh
        // session prevents the new Chamfer from accidentally consuming
        // the committed Fillet stage above.
        auto chamfer_loaded = loadOwnerFixtureV2ForTest(crash_store, crash_fixture);
        CHECK(chamfer_loaded.ok());
        application::DocumentSession chamfer_session{
            {}, std::move(*chamfer_loaded.document)};
        const auto chamfer_before_revision =
            chamfer_session.document().revision();
        auto chamfer_draft =
            application::ChamferDraft::beginCreate(
                chamfer_session,
                {*screenshot_edge, isolated_secondary.reference});
        CHECK(chamfer_draft);
        CHECK(chamfer_draft->setDistance(core::LengthValue{1.0}));
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_DRAFT_BEGIN"
            << std::endl;
        const auto chamfer_eval =
            chamfer_session.evaluateChamferDraft(
                *chamfer_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_DRAFT_END"
            << " status=" << static_cast<int>(chamfer_eval.status)
            << " removed=" << (chamfer_eval.preview_mesh ? 1 : 0)
            << " added="
            << (chamfer_eval.preview_added_mesh ? 1 : 0)
            << std::endl;
        CHECK(chamfer_eval.committable());
        CHECK(chamfer_eval.preview_mesh.has_value());
        CHECK(chamfer_eval.preview_added_mesh.has_value());
        CHECK(chamfer_session.document().revision() ==
              chamfer_before_revision);
        CHECK(chamfer_eval.body_solid);
        const auto chamfer_presentation =
            kernel.bodyPresentation(chamfer_eval.body_solid);
        CHECK(chamfer_presentation.ok());

        // Owner's new Chamfer corner report: inspect *actual* provider
        // surface kinds rather than infer curvature from displayed
        // triangle edges or interpolated shading. The generated corner
        // transition may be planar or nonplanar independently of the
        // flat equal-distance chamfers along each authored straight Edge.
        const auto chamfer_kernel_input =
            part::resolveKernelEdgeFeatureInput(
                part::ChamferFeature{
                    {*screenshot_edge, isolated_secondary.reference},
                    core::LengthValue{1.0}},
                &*crash_eval.current_topology);
        CHECK(chamfer_kernel_input.ok());
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_PAIR_KERNEL_BEGIN"
            << std::endl;
        const auto chamfer_kernel_result =
            kernel.edgeFeature(
                *chamfer_kernel_input.input,
                crash_eval.body_solid);
        CHECK(chamfer_kernel_result.ok());
        std::size_t chamfer_corner_plane = 0U;
        std::size_t chamfer_corner_curved = 0U;
        std::size_t chamfer_edge_plane = 0U;
        std::size_t chamfer_edge_curved = 0U;
        for (const auto& carrier :
             chamfer_kernel_result.edge_feature_surfaces) {
            const bool planar =
                carrier.surface_kind == kernel::SurfaceKind::plane;
            if (carrier.kind ==
                kernel::EdgeFeatureGeneratedSurfaceKind::
                    corner_transition) {
                planar ? ++chamfer_corner_plane :
                    ++chamfer_corner_curved;
            } else {
                planar ? ++chamfer_edge_plane :
                    ++chamfer_edge_curved;
            }
        }
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_PAIR_SURFACES"
            << " corner_plane=" << chamfer_corner_plane
            << " corner_nonplane=" << chamfer_corner_curved
            << " edge_plane=" << chamfer_edge_plane
            << " edge_nonplane=" << chamfer_edge_curved
            << std::endl;
        CHECK(!chamfer_kernel_result.edge_feature_surfaces.empty());

        // The Owner's full screenshot identifies the third selected
        // Edge by strict authored lineage: Extrude001, Side/Entity 1
        // + Side/Entity 4. This was NOT the prior test's sides 1+2.
        // Reproduce the exact 3-Edge authored semantic set on Part008,
        // without relying on provider runtime token or XYZ proximity.
        std::optional<part::MaterialEdgeReference> third_owner_edge;
        for (const auto& probe : secondary_edges) {
            const auto& curve = probe.reference.curve;
            if (curve.producer_feature_id.serialized() != "1" ||
                curve.role != part::FeatureCurveRoleKind::side_side ||
                curve.adjacent_surfaces.size() != 2U) {
                continue;
            }
            std::vector<std::string> side_entities;
            for (const auto& adjacent : curve.adjacent_surfaces) {
                if (adjacent.producer_feature_id.serialized() != "1" ||
                    adjacent.role != part::FeatureSurfaceRoleKind::side ||
                    !adjacent.source_entity) continue;
                side_entities.push_back(
                    adjacent.source_entity->serialized());
            }
            std::sort(side_entities.begin(), side_entities.end());
            if (side_entities ==
                std::vector<std::string>{"1", "4"}) {
                third_owner_edge = probe.reference;
                break;
            }
        }
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THIRD"
            << " referenceable=" << (third_owner_edge ? 1 : 0)
            << std::endl;
        CHECK(third_owner_edge);
        CHECK(*third_owner_edge != *screenshot_edge);
        CHECK(*third_owner_edge != isolated_secondary.reference);

        auto triple_loaded = loadOwnerFixtureV2ForTest(crash_store, crash_fixture);
        CHECK(triple_loaded.ok());
        application::DocumentSession triple_session{
            {}, std::move(*triple_loaded.document)};
        const auto triple_source_revision =
            triple_session.document().revision();

        auto lone_draft =
            application::ChamferDraft::beginCreate(
                triple_session, {*third_owner_edge});
        CHECK(lone_draft);
        CHECK(lone_draft->setDistance(
            core::LengthValue{1.0}));
        const auto lone =
            triple_session.evaluateChamferDraft(
                *lone_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_THIRD_ALONE"
            << " status=" << static_cast<int>(lone.status)
            << " target="
            << (lone.target_status
                ? static_cast<int>(*lone.target_status)
                : -1)
            << " committable=" << (lone.committable() ? 1 : 0)
            << std::endl;

        auto three_draft =
            application::ChamferDraft::beginCreate(
                triple_session,
                {*screenshot_edge, isolated_secondary.reference,
                 *third_owner_edge});
        CHECK(three_draft);
        CHECK(three_draft->setDistance(
            core::LengthValue{1.0}));
        const auto before_triple_kernel =
            part::resolveKernelEdgeFeatureInput(
                part::ChamferFeature{
                    {*screenshot_edge, isolated_secondary.reference,
                     *third_owner_edge},
                    core::LengthValue{1.0}},
                &*crash_eval.current_topology);
        CHECK(before_triple_kernel.ok());
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THREE_BEGIN"
            << std::endl;
        const auto kernel_three =
            kernel.edgeFeature(
                *before_triple_kernel.input,
                crash_eval.body_solid);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THREE_KERNEL"
            << " status=" << static_cast<int>(kernel_three.status)
            << " solid_count=" << kernel_three.solid_count
            << " brep_valid=" << (kernel_three.brep_valid ? 1 : 0)
            << std::endl;
        const auto three =
            triple_session.evaluateChamferDraft(
                *three_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THREE_END"
            << " status=" << static_cast<int>(three.status)
            << " target="
            << (three.target_status
                ? static_cast<int>(*three.target_status)
                : -1)
            << " removed=" << (three.preview_mesh ? 1 : 0)
            << " added=" << (three.preview_added_mesh ? 1 : 0)
            << std::endl;
        CHECK(triple_session.document().revision() ==
              triple_source_revision);
        CHECK(lone.committable());

        CHECK(kernel_three.ok());
        CHECK(three.committable());
        // A mixed signed CSG result must expose *both* exact local
        // material changes to the Workbench, not merely a valid B-Rep.
        // Guard the D2-B requirement after the production fallback.
        CHECK(three.preview_mesh.has_value());
        CHECK(three.preview_added_mesh.has_value());
        CHECK(three.preview_mesh->valid());
        CHECK(three.preview_added_mesh->valid());
        CHECK(three.previewSolidAvailable());

        // D2-B negative admission: never deduplicate authored Edges,
        // guess a missing reference or publish a partial fallback.
        auto duplicate_edge_input = *before_triple_kernel.input;
        CHECK(duplicate_edge_input.edges.size() == 3U);
        duplicate_edge_input.edges[2] =
            duplicate_edge_input.edges.front();
        CHECK(!duplicate_edge_input.valid());
        const auto duplicate_edge_result =
            kernel.edgeFeature(
                duplicate_edge_input, crash_eval.body_solid);
        CHECK(!duplicate_edge_result.ok());
        CHECK(duplicate_edge_result.status ==
              kernel::SolidModelingStatus::invalid_input);
        CHECK(!duplicate_edge_result.solid);

        // Strict runtime-stage membership: a structurally valid but
        // unknown Edge token fails; no XYZ or similarity rebind exists.
        auto missing_edge_input = *before_triple_kernel.input;
        std::uint64_t highest_token = 0U;
        for (const auto& current :
             crash_eval.current_topology->edges) {
            highest_token = std::max(
                highest_token, current.runtime_token.value);
        }
        CHECK(highest_token <
              std::numeric_limits<std::uint64_t>::max());
        const auto missing_edge_token =
            kernel::RuntimeEdgeToken{highest_token + 1U};
        CHECK(missing_edge_token.valid());
        missing_edge_input.edges[2] = missing_edge_token;
        CHECK(missing_edge_input.valid());
        const auto missing_edge_result =
            kernel.edgeFeature(
                missing_edge_input, crash_eval.body_solid);
        CHECK(!missing_edge_result.ok());
        CHECK(missing_edge_result.status ==
              kernel::SolidModelingStatus::provider_mismatch);
        CHECK(!missing_edge_result.solid);
        CHECK(triple_session.document().revision() ==
              triple_source_revision);

        // D2-B Owner Part008 strict permutation/size and full current
        // Face/Edge/Vertex inventory. Transient runtime tokens may order
        // evaluation but must never become durable source identity.
        auto trial_edges = before_triple_kernel.input->edges;
        std::sort(
            trial_edges.begin(), trial_edges.end(),
            [](const auto& a, const auto& b) {
                return a.value < b.value;
            });
        std::size_t passing_variants = 0U;
        do {
            for (const double d : {1.0, 0.5, 0.25}) {
                auto input = *before_triple_kernel.input;
                input.edges = trial_edges;
                input.parameter_mm = d;
                const auto trial =
                    kernel.edgeFeature(input, crash_eval.body_solid);
                CHECK(trial.ok());
                CHECK(trial.edge_feature_input_membership);
                CHECK(trial.edge_feature_input_membership
                          ->exactFor(input.edges));
                CHECK(trial.face_count == 21U);
                CHECK(trial.current_faces.size() == trial.face_count);
                CHECK(trial.current_edges.size() == trial.edge_count);
                CHECK(trial.current_vertices.size() ==
                      trial.vertex_count);
                CHECK(trial.current_edge_semantics.size() ==
                      trial.edge_count);
                CHECK(trial.current_vertex_semantics.size() ==
                      trial.vertex_count);
                CHECK(trial.edge_feature_surfaces.size() == 5U);
                // Check strict provenance, not merely that five output
                // Surfaces happened to be present in the result B-Rep.
                std::vector<kernel::RuntimeEdgeToken>
                    certified_strip_sources;
                std::vector<kernel::RuntimeVertexToken>
                    certified_corner_sources;
                for (const auto& surface :
                     trial.edge_feature_surfaces) {
                    CHECK(surface.valid());
                    CHECK(surface.operation ==
                          kernel::EdgeFeatureOperation::chamfer);
                    CHECK(surface.surface_kind ==
                          kernel::SurfaceKind::plane);
                    CHECK(surface.current_faces.size() == 1U);
                    if (surface.kind ==
                        kernel::EdgeFeatureGeneratedSurfaceKind::
                            edge_transition) {
                        CHECK(surface.source_edge.has_value());
                        CHECK(std::find(
                                  input.edges.begin(), input.edges.end(),
                                  *surface.source_edge) !=
                              input.edges.end());
                        CHECK(std::find(
                                  certified_strip_sources.begin(),
                                  certified_strip_sources.end(),
                                  *surface.source_edge) ==
                              certified_strip_sources.end());
                        certified_strip_sources.push_back(
                            *surface.source_edge);
                    } else {
                        CHECK(surface.kind ==
                              kernel::EdgeFeatureGeneratedSurfaceKind::
                                  corner_transition);
                        CHECK(surface.source_vertex.has_value());
                        CHECK(surface.incident_source_edges.size() == 2U);
                        CHECK(surface.incident_source_edges[0] !=
                              surface.incident_source_edges[1]);
                        for (const auto edge :
                             surface.incident_source_edges) {
                            CHECK(std::find(
                                      input.edges.begin(),
                                      input.edges.end(), edge) !=
                                  input.edges.end());
                        }
                        CHECK(std::find(
                                  certified_corner_sources.begin(),
                                  certified_corner_sources.end(),
                                  *surface.source_vertex) ==
                              certified_corner_sources.end());
                        // A corner's claimed Edge pair must be the
                        // exact selected pair incident at its *source*
                        // Vertex, not merely two authored Edges somewhere
                        // in the same Body. All tokens are stage-local.
                        const auto source_vertex = std::find_if(
                            crash_eval.current_topology->vertices.begin(),
                            crash_eval.current_topology->vertices.end(),
                            [&](const auto& vertex) {
                                return vertex.runtime_token ==
                                    *surface.source_vertex;
                            });
                        CHECK(source_vertex !=
                              crash_eval.current_topology->vertices.end());
                        std::size_t selected_incidence = 0U;
                        for (const auto edge : input.edges) {
                            const bool incident = std::find(
                                source_vertex->incident_material_edges.begin(),
                                source_vertex->incident_material_edges.end(),
                                edge) !=
                                source_vertex->incident_material_edges.end();
                            if (!incident) continue;
                            ++selected_incidence;
                            CHECK(std::find(
                                      surface.incident_source_edges.begin(),
                                      surface.incident_source_edges.end(),
                                      edge) !=
                                  surface.incident_source_edges.end());
                        }
                        CHECK(selected_incidence ==
                              surface.incident_source_edges.size());
                        certified_corner_sources.push_back(
                            *surface.source_vertex);
                    }
                }
                CHECK(certified_strip_sources.size() ==
                      input.edges.size());
                CHECK(certified_corner_sources.size() == 2U);
                for (const auto& face : trial.current_faces) {
                    std::size_t owners = 0U;
                    for (const auto& surface :
                         trial.inherited_surfaces) {
                        if (surface.surface_status ==
                                kernel::ReferenceStatus::resolved &&
                            std::find(
                                surface.current_faces.begin(),
                                surface.current_faces.end(),
                                face) != surface.current_faces.end()) {
                            ++owners;
                        }
                    }
                    for (const auto& surface :
                         trial.edge_feature_surfaces) {
                        if (std::find(
                                surface.current_faces.begin(),
                                surface.current_faces.end(),
                                face) != surface.current_faces.end()) {
                            ++owners;
                        }
                    }
                    CHECK(owners == 1U);
                }
                ++passing_variants;
            }
        } while (std::next_permutation(
            trial_edges.begin(), trial_edges.end(),
            [](const auto& a, const auto& b) {
                return a.value < b.value;
            }));
        CHECK(passing_variants == 18U);

        // D2-B one-authored-Feature lifecycle, using a new temporary native
        // file. The original sanitized Owner fixture is never mutated.
        QTemporaryDir lifecycle_dir;
        CHECK(lifecycle_dir.isValid());
        const auto lifecycle_path =
            std::filesystem::path{
                lifecycle_dir.path().toStdWString()} /
            "part008_d2b_three_edge_chamfer.ss2part";
        auto lifecycle_source = loadOwnerFixtureV2ForTest(crash_store, crash_fixture);
        CHECK(lifecycle_source.ok());
        const auto initial_file = crash_store.createNew(
            lifecycle_path, *lifecycle_source.document);
        CHECK(initial_file.ok());
        application::DocumentSession lifecycle{
            lifecycle_path,
            std::move(*lifecycle_source.document),
            *initial_file.checkpoint};
        std::vector<part::MaterialEdgeReference>
            authored_three{
                *screenshot_edge,
                isolated_secondary.reference,
                *third_owner_edge};
        // Direct Commands require canonical authored Edge order; Draft
        // normalizes the user's click sequence before constructing them.
        // A raw unsorted command must fail closed with no mutation.
        const auto lifecycle_before_invalid =
            lifecycle.document().revision();
        CHECK(!std::is_sorted(
            authored_three.begin(), authored_three.end()));
        const auto noncanonical = lifecycle.execute(
            application::CreateChamferFeatureCommand{
                authored_three,
                lifecycle.document().revision(),
                core::LengthValue{1.0},
                "Noncanonical input must fail"},
            kernel);
        CHECK(!noncanonical.ok());
        CHECK(!noncanonical.changed);
        CHECK(lifecycle.document().revision() ==
              lifecycle_before_invalid);
        CHECK(lifecycle.document().body().features.size() == 2U);
        std::sort(authored_three.begin(), authored_three.end());
        const auto create_feature =
            lifecycle.execute(
                application::CreateChamferFeatureCommand{
                    authored_three,
                    lifecycle.document().revision(),
                    core::LengthValue{1.0},
                    "D2B Three-Edge Chamfer"},
                kernel);
        std::cerr
            << "PM05F_R2_PART008_D2B_CREATE_DIAGNOSTIC"
            << " changed=" << create_feature.changed
            << " code="
            << static_cast<int>(create_feature.diagnostic.code)
            << " commit_code="
            << static_cast<int>(create_feature.diagnostic.commit_code)
            << " evaluation_diagnostic="
            << (create_feature.evaluation_diagnostic
                ? static_cast<int>(*create_feature.evaluation_diagnostic)
                : -1)
            << " message=" << create_feature.diagnostic.message
            << std::endl;
        CHECK(create_feature.ok());
        CHECK(create_feature.changed);
        CHECK(create_feature.feature_id);
        CHECK(lifecycle.document().body().features.size() == 3U);
        const auto check_live_three = [&]() {
            const auto evaluated = part::evaluatePart(
                lifecycle.document(), kernel);
            CHECK(evaluated.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(evaluated.body_solid);
            CHECK(evaluated.current_topology);
            CHECK(evaluated.current_topology->complete());
            const auto* feature =
                lifecycle.document().findFeature(
                    *create_feature.feature_id);
            CHECK(feature != nullptr);
            const auto* chamfer =
                std::get_if<part::ChamferFeature>(
                    &feature->definition);
            CHECK(chamfer != nullptr);
            CHECK(chamfer->edges.size() == 3U);
        };
        check_live_three();

        CHECK(lifecycle.undo().ok());
        CHECK(lifecycle.document().body().features.size() == 2U);
        CHECK(lifecycle.redo().ok());
        check_live_three();

        // Edit preview must use the exact authored source stage,
        // preserve DocumentRevision and expose both signed deltas
        // for this mixed-material three-Edge Chamfer.
        const auto before_edit_draft_revision =
            lifecycle.document().revision();
        auto edit_draft_three =
            application::ChamferDraft::beginEdit(
                lifecycle, *create_feature.feature_id);
        CHECK(edit_draft_three);
        CHECK(edit_draft_three->setDistance(
            core::LengthValue{0.5}));
        const auto preview_edit_three =
            lifecycle.evaluateChamferDraft(
                *edit_draft_three, kernel);
        CHECK(preview_edit_three.committable());
        CHECK(preview_edit_three.preview_mesh.has_value());
        CHECK(preview_edit_three.preview_added_mesh.has_value());
        CHECK(preview_edit_three.previewSolidAvailable());
        CHECK(lifecycle.document().revision() ==
              before_edit_draft_revision);

        const auto edit_feature =
            lifecycle.execute(
                application::EditChamferFeatureCommand{
                    *create_feature.feature_id,
                    authored_three,
                    lifecycle.document().revision(),
                    core::LengthValue{0.5},
                    "D2B Three-Edge Chamfer Edited"},
                kernel);
        CHECK(edit_feature.ok());
        CHECK(edit_feature.changed);
        check_live_three();
        // An Edit draft evaluated before the committed transaction
        // is stale afterwards; its Finish must not commit a second edit.
        const auto revision_after_edit =
            lifecycle.document().revision();
        const auto stale_finish_three =
            application::finishChamferDraft(
                lifecycle, *edit_draft_three,
                preview_edit_three, kernel);
        CHECK(!stale_finish_three.ok());
        CHECK(!stale_finish_three.changed);
        CHECK(stale_finish_three.status ==
              application::EdgeFeatureDraftFinishStatus::
                  stale_context);
        CHECK(lifecycle.document().revision() ==
              revision_after_edit);
        CHECK(lifecycle.document().body().features.size() == 3U);
        const auto* edited =
            std::get_if<part::ChamferFeature>(
                &lifecycle.document().findFeature(
                    *create_feature.feature_id)->definition);
        CHECK(edited != nullptr);
        CHECK(edited->distance == core::LengthValue{0.5});

        CHECK(lifecycle.undo().ok());
        check_live_three();
        const auto* restored =
            std::get_if<part::ChamferFeature>(
                &lifecycle.document().findFeature(
                    *create_feature.feature_id)->definition);
        CHECK(restored != nullptr);
        CHECK(restored->distance == core::LengthValue{1.0});
        CHECK(lifecycle.redo().ok());
        check_live_three();
        CHECK(lifecycle.save().ok());
        CHECK(!lifecycle.needsSave());

        auto reopened = crash_store.load(lifecycle_path);
        CHECK(reopened.ok());
        CHECK(reopened.document->state() ==
              lifecycle.document().state());
        const auto reopened_eval =
            part::evaluatePart(*reopened.document, kernel);
        CHECK(reopened_eval.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(reopened_eval.body_solid);
        CHECK(reopened_eval.current_topology);
        CHECK(reopened_eval.current_topology->complete());
        std::cerr
            << "PM05F_R2_PART008_D2B_LIFECYCLE_PASS"
            << " create=1 edit=1 undo=1 redo=1"
            << " save=1 reopen=1 replay=1"
            << std::endl;

        std::cerr
            << "PM05F_R2_PART008_D2B_PRODUCTION_MATRIX"
            << " passing=" << passing_variants
            << " face_count=21 complete_lineage=1"
            << std::endl;

        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_CORNER_EXPECTATION"
            << " corner_planar=" << chamfer_corner_plane
            << " corner_nonplanar=" << chamfer_corner_curved
            << " edge_planar=" << chamfer_edge_plane
            << " edge_nonplanar=" << chamfer_edge_curved
            << std::endl;

        // Exercise the real Qt/OCCT Chamfer picker and Finish, rather
        // than passing a fabricated PresentationToken or issuing a
        // direct persistence command in lieu of the Owner interaction.
        CHECK(workbench.activateDocument(&chamfer_session, {}));
        QApplication::processEvents();
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_edges));
        auto chamfer_reply = workbench.submitCadInput(
            "CHAMFER", workbench.cadInputContextGeneration());
        CHECK(chamfer_reply.accepted);
        chamfer_reply = workbench.lockCadDynamicInputField(
            0U, "1", workbench.cadInputContextGeneration());
        CHECK(chamfer_reply.accepted);
        bool chamfer_first_selected = false;
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_left,
                 viewer::StandardView::bottom_back_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (std::size_t k = 1U;
                 k < screenshot_points.size() &&
                 !chamfer_first_selected; ++k) {
                const auto& a = screenshot_points[k - 1U];
                const auto& b = screenshot_points[k];
                const viewer::Point3 midpoint{
                    (a.x + b.x) / 2.0,
                    (a.y + b.y) / 2.0,
                    (a.z + b.z) / 2.0};
                if (nativeClick(*viewport, midpoint)) {
                    chamfer_first_selected = selectedCount(*label, 1);
                }
            }
            if (chamfer_first_selected) break;
        }
        CHECK(chamfer_first_selected);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_ONE_EDGE"
            << std::endl;

        bool chamfer_two_selected = false;
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_left,
                 viewer::StandardView::bottom_back_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (std::size_t k = 1U;
                 k < isolated_secondary.points.size() &&
                 !chamfer_two_selected; ++k) {
                const auto& a = isolated_secondary.points[k - 1U];
                const auto& b = isolated_secondary.points[k];
                const viewer::Point3 midpoint{
                    (a.x + b.x) / 2.0,
                    (a.y + b.y) / 2.0,
                    (a.z + b.z) / 2.0};
                if (!nativeClick(*viewport, midpoint)) {
                    continue;
                }
                chamfer_two_selected = selectedCount(*label, 2);
                if (!chamfer_two_selected &&
                    !selectedCount(*label, 1)) {
                    // The click toggled OFF the first material Edge.
                    // Do not claim a 2-Edge successful native pick.
                    break;
                }
            }
            if (chamfer_two_selected ||
                !selectedCount(*label, 1)) break;
        }
        CHECK(chamfer_two_selected);
        CHECK(finish->isEnabled());
        CHECK(viewport->runtimeDiagnostics().solid_preview_displayed);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_FINISH_BEGIN"
            << std::endl;
        finish->click();
        QApplication::processEvents();
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_FINISH_RETURN"
            << std::endl;
        CHECK(chamfer_session.document().revision() !=
              chamfer_before_revision);
        CHECK(chamfer_session.document().body()
                  .features.size() == 3U);
        const auto* persisted_chamfer =
            std::get_if<part::ChamferFeature>(
                &chamfer_session.document().body()
                    .features.back().definition);
        CHECK(persisted_chamfer != nullptr);
        CHECK(persisted_chamfer->edges.size() == 2U);
        const auto chamfer_committed =
            part::evaluatePart(chamfer_session.document(), kernel);
        CHECK(chamfer_committed.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(viewport->runtimeDiagnostics().solid_committed_displayed);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_FINISH_PASS"
            << std::endl;

        // UI-01 P0 Chamfer Edit: the real committed 2-Edge Chamfer
        // opens an edit draft that must remain exclusive to Operations.
        auto* chamfer_item = featureTreeItem(
            *tree, chamfer_session.document()
                .body().features.back().id);
        CHECK(chamfer_item);
        tree->clearSelection();
        tree->setCurrentItem(chamfer_item);
        chamfer_item->setSelected(true);
        QApplication::processEvents();
        CHECK(edit->isEnabled());
        edit->click();
        QApplication::processEvents();
        CHECK(!cancel->isHidden());
        verifyUi01FeatureDraftRejectsTreeEdits(
            workbench, chamfer_session, *tree, chamfer_item,
            "edgeFeatureOperationsWidget");
        cancel->click();
        QApplication::processEvents();
        // The parent Operations widget hides after Cancel; the child
        // QPushButton itself need not have QWidget::isHidden() set.
        CHECK(workbench.findChild<QWidget*>(
            QStringLiteral("edgeFeatureOperationsWidget"))->isHidden());

        // Restore a known-live session before the temporary Revolve
        // fixtures are destroyed and the Workbench is closed.
        CHECK(workbench.activateDocument(&session, {}));
        QApplication::processEvents();

        std::cout
            << "PM05F_R2_NATIVE_WORKBENCH_EDIT_PASS"
            << " mouse_selection=1"
            << " tool_stage=1"
            << " toggle=1"
            << " add_remove=1"
            << " cancel_zero_mutation=1"
            << " edit_finish=1\n";
        result = EXIT_SUCCESS;
        workbench.close();
        app.quit();
    });
    app.exec();
    return result;
}
