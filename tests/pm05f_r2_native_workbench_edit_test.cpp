#include "cad_workbench.hpp"


#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
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
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

std::atomic<std::uint32_t> owner_edge_outline_warnings{0U};

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

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    const bool pg01c_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01c";
    kernel_occt::OcctSolidModelingKernel kernel;
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
        pg_reply = workbench.submitCadInput(
            "PROFILE", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        pg_reply = workbench.submitCadInput(
            "FIND", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        auto* pg_profile_status = workbench.findChild<QLabel*>(
            QStringLiteral("workbenchStatus"));
        CHECK(pg_profile_status);
        CHECK(pg_profile_status->text().contains(
            QStringLiteral("Profile regions: 1;")));
        pg_reply = workbench.submitCadInput(
            "CANCEL", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        std::cout
            << "PG01C_C2_CURRENT_PROFILE_REGIONS_PASS"
            << " persisted_seed_regions=0"
            << " current_linked_regions=1"
            << " ui_find_regions=1\\n";
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

        // Create the original Fillet through actual native cursor input,
        // not a fabricated provider-neutral PresentationToken intent.
        auto reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
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
            CHECK(viewport->setStandardView(
                viewer::StandardView::top_front_right));
            viewport->fitAll();
            QApplication::processEvents();

            reply = workbench.submitCadInput(
                command,
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
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
        auto owner_loaded = owner_store.load(owner_fixture);
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
        auto crash_loaded = crash_store.load(crash_fixture);
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
        auto chamfer_loaded = crash_store.load(crash_fixture);
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

        auto triple_loaded = crash_store.load(crash_fixture);
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
        auto lifecycle_source = crash_store.load(crash_fixture);
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
