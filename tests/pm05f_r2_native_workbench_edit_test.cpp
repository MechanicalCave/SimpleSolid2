#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool ok, const char* expression, int line) {
    if (ok) return;
    std::cerr << "PM05F R2 native Workbench Edit CHECK failed at "
              << line << ": " << expression << '\n';
    std::exit(EXIT_FAILURE);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

application::DocumentSession makeBaseSession(
    kernel_occt::OcctSolidModelingKernel& kernel) {
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
        {40.0, 30.0},
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
                core::LengthValue{20.0}, false},
            "Base"},
        kernel);
    CHECK(base.ok() && base.feature_id);
    return session;
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

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
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
