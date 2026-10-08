#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05E2 Workbench lifecycle CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class TestViewport final
    : public QWidget,
      public viewer::IDocumentViewport {
public:
    using QWidget::QWidget;

    std::optional<viewer::CameraState>
    cameraState() const override {
        return camera_;
    }

    bool setCameraState(
        const viewer::CameraState& state) override {
        camera_ = state;
        return true;
    }

    bool setStandardView(
        viewer::StandardView view) override {
        const auto next =
            viewer::cameraForStandardView(camera_, view);
        if (!next) return false;
        camera_ = *next;
        return true;
    }

    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                camera_, projection);
        if (!next) return false;
        camera_ = *next;
        return true;
    }

    void fitAll() override {}

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        return scene.valid();
    }

    bool setBodyScene(
        const viewer::BodyScene& scene) override {
        if (!scene.valid()) return false;
        body_scene = scene;
        return true;
    }

    bool setBodyTopologyOverlayScene(
        const viewer::BodyTopologyOverlayScene& scene) override {
        return scene.valid();
    }

    bool setSolidPreviewScene(
        const viewer::SolidPreviewScene& scene) override {
        if (!scene.valid()) return false;
        solid_preview = scene;
        return true;
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        return scene.valid();
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        return scene.valid();
    }

    bool setProfileScene(
        const viewer::ProfileScene& scene) override {
        return scene.valid();
    }

    bool setProfilePreviewScene(
        const viewer::ProfilePreviewScene& scene) override {
        return scene.valid();
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& scene) override {
        if (!scene.valid()) return false;
        presentation_selection = scene;
        return true;
    }

    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        return {point.valid(), std::nullopt};
    }

    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule) override {
        return {rectangle.valid(), {}};
    }

    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override {
        return overlay.valid();
    }

    void clearSketchSelectionBoxOverlay() override {}

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler = std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_handler = std::move(handler);
    }

    void setBodyTopologySelectionIntentHandler(
        viewer::BodyTopologySelectionIntentHandler handler) override {
        topology_handler = std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting) override {}

    void setCursorMode(
        viewer::ViewportCursorMode) override {}

    void emitEdge(viewer::PresentationToken token) {
        CHECK(static_cast<bool>(topology_handler));
        CHECK(body_scene.generation.valid());
        viewer::BodyTopologyPickQueryResult query;
        query.completed = true;
        query.generation = body_scene.generation;
        query.candidates.push_back(
            {
                token,
                viewer::BodyTopologyPresentationKind::edge,
                0.0,
                1.0});
        CHECK(query.valid());
        topology_handler(
            query,
            viewer::SelectionIntentMode::replace);
    }

    viewer::CameraState camera_;
    viewer::BodyScene body_scene;
    viewer::SolidPreviewScene solid_preview;
    viewer::PresentationSelection presentation_selection;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
    viewer::BodyTopologySelectionIntentHandler topology_handler;
};

application::DocumentSession makeBaseSession(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch_created.ok() && sketch_created.sketch_id);

    CHECK(
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false})
            .ok());

    const auto* sketch =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto regions =
        sketch::analyzeRegions(sketch->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false},
                "Base"},
            kernel);
    CHECK(base.ok() && base.feature_id);
    return session;
}

part::MaterialEdgeReference missingEdgeReference(
    part::FeatureId base_id) {
    part::FeatureSurfaceAddress first{
        base_id,
        part::FeatureSurfaceRoleKind::profile_cap,
        std::nullopt,
        0U,
        0U,
        false};
    part::FeatureSurfaceAddress second{
        base_id,
        part::FeatureSurfaceRoleKind::negative_cap,
        std::nullopt,
        0U,
        0U,
        false};
    CHECK(first.valid());
    CHECK(second.valid());

    std::vector<part::FeatureSurfaceAddress> surfaces{
        std::move(first),
        std::move(second)};
    std::sort(surfaces.begin(), surfaces.end());

    part::FeatureCurveAddress curve{
        base_id,
        part::FeatureCurveRoleKind::cap_side,
        std::move(surfaces)};
    CHECK(curve.valid());

    part::MaterialEdgeReference result{
        {
            part::BodyStageKind::after_feature,
            base_id},
        std::move(curve),
        part::SingularAtAuthoredStage{}};
    CHECK(result.valid());
    return result;
}

std::pair<application::DocumentSession, part::FeatureId>
makeMissingRepairSession(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto base = makeBaseSession(kernel);
    auto state = base.document().state();
    const auto base_id =
        state.body.features.front().id;
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id);

    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "RepairFillet",
            false,
            part::FilletFeature{
                {missingEdgeReference(base_id)},
                core::LengthValue{1.0}}});

    auto restored =
        part::PartDocument::restore(
            base.documentId(),
            std::move(state),
            base.document().revision());
    CHECK(restored.ok());

    const auto evaluation =
        part::evaluatePart(
            *restored.document,
            kernel);
    const auto* target =
        evaluation.findFeature(*feature_id);
    CHECK(target != nullptr);
    CHECK(
        target->status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(
        target->diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            edge_reference_missing);
    CHECK(
        target->edge_reference_status ==
        kernel::ReferenceStatus::missing);

    return {
        application::DocumentSession{
            {},
            std::move(*restored.document)},
        *feature_id};
}

QTreeWidgetItem* featureTreeItem(
    QTreeWidget& tree,
    part::FeatureId feature_id) {
    constexpr int feature_id_data =
        Qt::UserRole + 45;
    const auto serialized =
        QString::fromStdString(
            feature_id.serialized());
    std::vector<QTreeWidgetItem*> pending;
    for (int index = 0;
         index < tree.topLevelItemCount();
         ++index) {
        pending.push_back(
            tree.topLevelItem(index));
    }
    while (!pending.empty()) {
        auto* item = pending.back();
        pending.pop_back();
        if (item != nullptr &&
            item->data(0, feature_id_data)
                    .toString() ==
                serialized) {
            return item;
        }
        if (item != nullptr) {
            for (int child = 0;
                 child < item->childCount();
                 ++child) {
                pending.push_back(
                    item->child(child));
            }
        }
    }
    return nullptr;
}

std::vector<viewer::PresentationToken>
materialEdgeTokens(const viewer::BodyScene& scene) {
    std::vector<viewer::PresentationToken> result;
    for (const auto& edge : scene.edges) {
        if (edge.material && edge.ordinary_pickable) {
            result.push_back(edge.token);
        }
    }
    CHECK(!result.empty());
    return result;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    kernel_occt::OcctSolidModelingKernel kernel;

    TestViewport* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            auto* created = new TestViewport(parent);
            viewport = created;
            return ui::ViewportSurface{created, created};
        },
        &kernel};
    CHECK(viewport != nullptr);
    workbench.resize(1400, 900);
    workbench.show();
    QApplication::processEvents();

    auto* operations =
        workbench.findChild<QWidget*>(
            QStringLiteral("edgeFeatureOperationsWidget"));
    auto* title =
        workbench.findChild<QLabel*>(
            QStringLiteral("edgeFeatureTitleLabel"));
    auto* parameter =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("edgeFeatureParameterEdit"));
    auto* selection_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("edgeFeatureSelectionLabel"));
    auto* finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral("edgeFeatureFinishButton"));
    auto* cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral("edgeFeatureCancelButton"));
    auto* edit =
        workbench.findChild<QPushButton*>(
            QStringLiteral("featureEditExtrudeButton"));
    auto* suppress =
        workbench.findChild<QPushButton*>(
            QStringLiteral("featureSuppressButton"));
    auto* remove =
        workbench.findChild<QPushButton*>(
            QStringLiteral("featureDeleteButton"));
    auto* tree =
        workbench.findChild<QTreeWidget*>();
    CHECK(
        operations && title && parameter &&
        selection_label && finish && cancel && edit &&
        suppress && remove && tree);

    auto session = makeBaseSession(kernel);
    const auto base_id =
        session.document().body().features.front().id;
    CHECK(workbench.activateDocument(&session, {}));
    QApplication::processEvents();
    CHECK(
        viewport->body_scene.purpose ==
        viewer::BodyScenePurpose::current_body);

    auto result =
        workbench.submitCadInput(
            "FILLET",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    result =
        workbench.lockCadDynamicInputField(
            0U,
            "1",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);

    bool selected = false;
    for (const auto token :
         materialEdgeTokens(viewport->body_scene)) {
        result =
            workbench.submitCadInput(
                "CLEAR",
                workbench.cadInputContextGeneration());
        CHECK(result.accepted);
        viewport->emitEdge(token);
        QApplication::processEvents();
        if (!viewport->solid_preview.empty() &&
            finish->isEnabled()) {
            selected = true;
            break;
        }
    }
    CHECK(selected);
    finish->click();
    QApplication::processEvents();

    CHECK(session.document().body().features.size() == 2U);
    const auto feature_id =
        session.document().body().features.back().id;
    CHECK(feature_id != base_id);
    const auto* authored =
        session.document().findFeature(feature_id);
    CHECK(authored != nullptr);
    const auto* original =
        std::get_if<part::FilletFeature>(
            &authored->definition);
    CHECK(original != nullptr);
    CHECK(original->radius.millimetres == 1.0);
    CHECK(featureTreeItem(*tree, feature_id) != nullptr);
    CHECK(edit->isEnabled());
    CHECK(suppress->isEnabled());
    CHECK(remove->isEnabled());

    const auto feature_count =
        session.document().body().features.size();
    const auto undo_before_edit = session.undoDepth();

    // Cancelled Edit uses the exact predecessor stage and leaves authored
    // state untouched.
    edit->click();
    QApplication::processEvents();
    CHECK(operations->isVisible());
    CHECK(title->text() == QStringLiteral("EDIT FILLET"));
    CHECK(
        viewport->body_scene.purpose ==
        viewer::BodyScenePurpose::tool_stage);
    CHECK(!viewport->solid_preview.empty());
    CHECK(finish->isEnabled());

    // R2-B: use the real controller/tool-stage selection bridge to toggle
    // the restored semantic Edge off/on, then add/remove another strict
    // authorable Edge. This remains a synthetic-token Workbench test; the
    // independent native Qt/OCCT cursor regression protects the Viewer side.
    std::vector<viewer::PresentationToken> restored_edges;
    for (const auto token :
         viewport->presentation_selection.selected) {
        if (std::any_of(
                viewport->body_scene.edges.begin(),
                viewport->body_scene.edges.end(),
                [token](const auto& edge) {
                    return edge.token == token;
                })) {
            restored_edges.push_back(token);
        }
    }
    CHECK(restored_edges.size() == 1U);
    const auto original_token = restored_edges.front();
    CHECK(
        selection_label->text() ==
        QStringLiteral("Selected edges: 1"));

    viewport->emitEdge(original_token);
    QApplication::processEvents();
    CHECK(
        selection_label->text() ==
        QStringLiteral("Selected edges: 0"));
    CHECK(!finish->isEnabled());

    viewport->emitEdge(original_token);
    QApplication::processEvents();
    CHECK(
        selection_label->text() ==
        QStringLiteral("Selected edges: 1"));
    CHECK(finish->isEnabled());

    bool added_second_edge = false;
    for (const auto token :
         materialEdgeTokens(viewport->body_scene)) {
        if (token == original_token) continue;
        viewport->emitEdge(token);
        QApplication::processEvents();
        if (selection_label->text() !=
                QStringLiteral("Selected edges: 2")) {
            continue;
        }
        added_second_edge = true;
        viewport->emitEdge(token);
        QApplication::processEvents();
        CHECK(
            selection_label->text() ==
            QStringLiteral("Selected edges: 1"));
        CHECK(finish->isEnabled());
        break;
    }
    CHECK(added_second_edge);

    result =
        workbench.lockCadDynamicInputField(
            0U,
            "1.25",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!viewport->solid_preview.empty());
    cancel->click();
    QApplication::processEvents();

    CHECK(
        viewport->body_scene.purpose ==
        viewer::BodyScenePurpose::current_body);
    CHECK(session.undoDepth() == undo_before_edit);
    CHECK(
        session.document().body().features.size() ==
        feature_count);
    authored = session.document().findFeature(feature_id);
    CHECK(authored != nullptr);
    original =
        std::get_if<part::FilletFeature>(
            &authored->definition);
    CHECK(original != nullptr);
    CHECK(original->radius.millimetres == 1.0);

    // Successful Edit preserves FeatureId and commits exactly one Undo step.
    CHECK(edit->isEnabled());
    edit->click();
    QApplication::processEvents();
    result =
        workbench.lockCadDynamicInputField(
            0U,
            "1.5",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(finish->isEnabled());
    finish->click();
    QApplication::processEvents();

    CHECK(
        session.document().body().features.size() ==
        feature_count);
    CHECK(session.undoDepth() == undo_before_edit + 1U);
    authored = session.document().findFeature(feature_id);
    CHECK(authored != nullptr);
    CHECK(authored->id == feature_id);
    const auto* edited =
        std::get_if<part::FilletFeature>(
            &authored->definition);
    CHECK(edited != nullptr);
    CHECK(edited->radius.millimetres == 1.5);
    CHECK(
        viewport->body_scene.purpose ==
        viewer::BodyScenePurpose::current_body);

    // PM-05E reuses the generic Feature lifecycle. No edge-specific
    // suppression/deletion command path is introduced.
    CHECK(suppress->isEnabled());
    suppress->click();
    QApplication::processEvents();
    authored = session.document().findFeature(feature_id);
    CHECK(authored != nullptr);
    CHECK(authored->suppressed);

    CHECK(suppress->isEnabled());
    suppress->click();
    QApplication::processEvents();
    authored = session.document().findFeature(feature_id);
    CHECK(authored != nullptr);
    CHECK(!authored->suppressed);

    CHECK(remove->isEnabled());
    remove->click();
    QApplication::processEvents();
    CHECK(session.document().findFeature(feature_id) == nullptr);
    CHECK(session.document().body().features.size() == 1U);

    CHECK(session.undo().ok());
    authored = session.document().findFeature(feature_id);
    CHECK(authored != nullptr);
    CHECK(authored->id == feature_id);
    edited =
        std::get_if<part::FilletFeature>(
            &authored->definition);
    CHECK(edited != nullptr);
    CHECK(edited->radius.millimetres == 1.5);

    // Explicit repair: a structurally valid authored Edge whose semantic
    // curve is Missing must never auto-rebind. Reopen through the Tree,
    // preserve the failing intent until Clear, then require one explicit
    // current-stage Edge pick and keep the same FeatureId.
    auto repair_fixture =
        makeMissingRepairSession(kernel);
    auto repair_session =
        std::move(repair_fixture.first);
    const auto repair_feature_id =
        repair_fixture.second;
    CHECK(
        workbench.activateDocument(
            &repair_session,
            {}));
    QApplication::processEvents();

    auto* repair_item =
        featureTreeItem(
            *tree,
            repair_feature_id);
    CHECK(repair_item != nullptr);
    tree->setCurrentItem(repair_item);
    repair_item->setSelected(true);
    QApplication::processEvents();
    CHECK(edit->isEnabled());
    edit->click();
    QApplication::processEvents();
    CHECK(operations->isVisible());
    CHECK(title->text() == QStringLiteral("EDIT FILLET"));
    CHECK(!finish->isEnabled());

    result =
        workbench.submitCadInput(
            "CLEAR",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!finish->isEnabled());

    bool repair_selected = false;
    for (const auto token :
         materialEdgeTokens(viewport->body_scene)) {
        viewport->emitEdge(token);
        QApplication::processEvents();
        if (!viewport->solid_preview.empty() &&
            finish->isEnabled()) {
            repair_selected = true;
            break;
        }
        result =
            workbench.submitCadInput(
                "CLEAR",
                workbench.cadInputContextGeneration());
        CHECK(result.accepted);
    }
    CHECK(repair_selected);

    finish->click();
    QApplication::processEvents();
    const auto* repaired =
        repair_session.document().findFeature(
            repair_feature_id);
    CHECK(repaired != nullptr);
    CHECK(repaired->id == repair_feature_id);
    const auto* repaired_fillet =
        std::get_if<part::FilletFeature>(
            &repaired->definition);
    CHECK(repaired_fillet != nullptr);
    CHECK(repaired_fillet->edges.size() == 1U);
    CHECK(
        repaired_fillet->edges.front() !=
        missingEdgeReference(
            repair_session.document()
                .body().features.front().id));
    const auto repaired_evaluation =
        part::evaluatePart(
            repair_session.document(),
            kernel);
    CHECK(
        repaired_evaluation.findFeature(
            repair_feature_id)->status ==
        part::FeatureEvaluationStatus::up_to_date);

    std::cout
        << "PM05E2_CAD_WORKBENCH_EDGE_LIFECYCLE_PASS"
        << " tool_stage=1"
        << " cancel_zero_mutation=1"
        << " stable_feature_id=1"
        << " suppress=1"
        << " delete_undo=1"
        << " tree_reopen=1"
        << " missing_repair=1\n";
    return EXIT_SUCCESS;
}
