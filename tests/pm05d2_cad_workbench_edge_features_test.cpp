#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

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
            << "PM-05D2 edge-feature Workbench CHECK failed at line "
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
            viewer::cameraForStandardView(
                camera_, view);
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
        selection_handler =
            std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_handler =
            std::move(handler);
    }

    void setBodyTopologySelectionIntentHandler(
        viewer::BodyTopologySelectionIntentHandler handler) override {
        topology_handler =
            std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting) override {}

    void setCursorMode(
        viewer::ViewportCursorMode) override {}

    void emitEdge(
        viewer::PresentationToken token,
        viewer::SelectionIntentMode mode =
            viewer::SelectionIntentMode::replace) {
        CHECK(static_cast<bool>(topology_handler));
        CHECK(body_scene.generation.valid());
        viewer::BodyTopologyPickQueryResult query;
        query.completed = true;
        query.generation =
            body_scene.generation;
        query.candidates.push_back(
            {
                token,
                viewer::BodyTopologyPresentationKind::edge,
                0.0,
                1.0});
        CHECK(query.valid());
        topology_handler(query, mode);
    }

    viewer::CameraState camera_;
    viewer::BodyScene body_scene;
    viewer::SolidPreviewScene solid_preview;
    viewer::PresentationSelection presentation_selection;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
    viewer::BodyTopologySelectionIntentHandler
        topology_handler;
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
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

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

std::vector<viewer::PresentationToken>
materialEdgeTokens(
    const viewer::BodyScene& scene) {
    std::vector<viewer::PresentationToken> result;
    for (const auto& edge : scene.edges) {
        if (edge.material &&
            edge.ordinary_pickable) {
            result.push_back(edge.token);
        }
    }
    CHECK(result.size() >= 2U);
    return result;
}

bool selectionLabelHas(
    const QLabel* label,
    std::size_t count) {
    return label != nullptr &&
           label->text() ==
               QStringLiteral("Selected edges: %1")
                   .arg(
                       static_cast<qulonglong>(
                           count));
}

void seedSelectionFirst(
    TestViewport& viewport,
    QPushButton& tool,
    QPushButton& cancel,
    QWidget& operations,
    QLabel& selection_label) {
    const auto tokens =
        materialEdgeTokens(
            viewport.body_scene);
    for (const auto token : tokens) {
        viewport.emitEdge(
            token,
            viewer::SelectionIntentMode::replace);
        QApplication::processEvents();

        tool.click();
        QApplication::processEvents();
        CHECK(operations.isVisible());

        if (selectionLabelHas(
                &selection_label,
                1U)) {
            return;
        }

        cancel.click();
        QApplication::processEvents();
    }
    CHECK(false);
}

void selectAtLeastTwoDraftEdges(
    TestViewport& viewport,
    QLabel& selection_label) {
    const auto tokens =
        materialEdgeTokens(
            viewport.body_scene);
    for (const auto token : tokens) {
        if (selectionLabelHas(
                &selection_label,
                2U)) {
            return;
        }
        viewport.emitEdge(
            token,
            viewer::SelectionIntentMode::replace);
        QApplication::processEvents();
    }
    CHECK(selectionLabelHas(
        &selection_label,
        2U));
}

void verifyCommandFirstFinish(
    ui::CadWorkbench& workbench,
    TestViewport& viewport,
    application::DocumentSession& session,
    const char* command,
    bool fillet) {
    auto* operations =
        workbench.findChild<QWidget*>(
            QStringLiteral(
                "edgeFeatureOperationsWidget"));
    auto* selection_label =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "edgeFeatureSelectionLabel"));
    auto* finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "edgeFeatureFinishButton"));
    CHECK(
        operations &&
        selection_label &&
        finish);

    const auto undo_before =
        session.undoDepth();
    const auto feature_count_before =
        session.document().body()
            .features.size();

    auto result =
        workbench.submitCadInput(
            command,
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    QApplication::processEvents();
    CHECK(operations->isVisible());

    result =
        workbench.submitCadInput(
            "CLEAR",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(selectionLabelHas(
        selection_label,
        0U));

    selectAtLeastTwoDraftEdges(
        viewport,
        *selection_label);
    CHECK(selectionLabelHas(
        selection_label,
        2U));
    CHECK(finish->isEnabled() == false);

    const auto before_parameter =
        workbench.cadInputContextGeneration();
    result =
        workbench.lockCadDynamicInputField(
            0U,
            "1",
            before_parameter);
    CHECK(result.accepted);
    CHECK(
        workbench.cadInputContextGeneration() !=
        before_parameter);
    CHECK(!viewport.solid_preview.empty());
    CHECK(finish->isEnabled());

    const auto stale_finish =
        workbench.submitCadInput(
            "FINISH",
            before_parameter);
    CHECK(!stale_finish.accepted);
    CHECK(
        session.document().body()
            .features.size() ==
        feature_count_before);

    result =
        workbench.submitCadInput(
            "FINISH",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    QApplication::processEvents();

    CHECK(
        session.document().body()
            .features.size() ==
        feature_count_before + 1U);
    CHECK(
        session.undoDepth() ==
        undo_before + 1U);
    CHECK(viewport.solid_preview.empty());

    const auto& feature =
        session.document().body()
            .features.back();
    if (fillet) {
        CHECK(
            std::get_if<part::FilletFeature>(
                &feature.definition) != nullptr);
    } else {
        CHECK(
            std::get_if<part::ChamferFeature>(
                &feature.definition) != nullptr);
    }
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    kernel_occt::OcctSolidModelingKernel kernel;

    TestViewport* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            auto* created =
                new TestViewport(parent);
            viewport = created;
            return ui::ViewportSurface{
                created,
                created};
        },
        &kernel};
    CHECK(viewport != nullptr);
    workbench.resize(1400, 900);
    workbench.show();
    QApplication::processEvents();

    auto* create_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("partCreateToolsLabel"));
    auto* modify_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("partModifyToolsLabel"));
    auto* fillet =
        workbench.findChild<QPushButton*>(
            QStringLiteral("filletToolButton"));
    auto* chamfer =
        workbench.findChild<QPushButton*>(
            QStringLiteral("chamferToolButton"));
    auto* operations =
        workbench.findChild<QWidget*>(
            QStringLiteral(
                "edgeFeatureOperationsWidget"));
    auto* selection_label =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "edgeFeatureSelectionLabel"));
    auto* parameter =
        workbench.findChild<QLineEdit*>(
            QStringLiteral(
                "edgeFeatureParameterEdit"));
    auto* finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "edgeFeatureFinishButton"));
    auto* cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "edgeFeatureCancelButton"));
    CHECK(
        create_label &&
        modify_label &&
        fillet &&
        chamfer &&
        operations &&
        selection_label &&
        parameter &&
        finish &&
        cancel);

    // Selection-first Fillet seeds durable semantic Edge intent, previews the
    // exact candidate Body and Cancel remains zero-mutation.
    auto fillet_session =
        makeBaseSession(kernel);
    CHECK(
        workbench.activateDocument(
            &fillet_session,
            {}));
    QApplication::processEvents();
    CHECK(create_label->isVisible());
    CHECK(modify_label->isVisible());
    CHECK(fillet->isEnabled());
    CHECK(chamfer->isEnabled());
    CHECK(!viewport->body_scene.empty());

    const auto fillet_undo =
        fillet_session.undoDepth();
    seedSelectionFirst(
        *viewport,
        *fillet,
        *cancel,
        *operations,
        *selection_label);
    CHECK(fillet->isChecked());
    CHECK(!chamfer->isEnabled());
    CHECK(
        workbench.cadInputPrompt().find(
            "FILLET") != std::string::npos);

    auto result =
        workbench.lockCadDynamicInputField(
            0U,
            "1",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!viewport->solid_preview.empty());
    CHECK(finish->isEnabled());
    cancel->click();
    QApplication::processEvents();
    CHECK(!operations->isVisible());
    CHECK(
        fillet_session.undoDepth() ==
        fillet_undo);
    CHECK(
        fillet_session.document().body()
            .features.size() == 1U);
    CHECK(viewport->solid_preview.empty());

    // Command-first Fillet supports explicit multi-Edge toggle selection,
    // Dynamic Input parity and stale-context rejection before one Finish.
    verifyCommandFirstFinish(
        workbench,
        *viewport,
        fillet_session,
        "FILLET",
        true);

    // Selection-first and command-first Chamfer use the same shared draft,
    // selection and exact-preview meaning on a fresh Body.
    auto chamfer_session =
        makeBaseSession(kernel);
    CHECK(
        workbench.activateDocument(
            &chamfer_session,
            {}));
    QApplication::processEvents();
    CHECK(!viewport->body_scene.empty());

    const auto chamfer_undo =
        chamfer_session.undoDepth();
    seedSelectionFirst(
        *viewport,
        *chamfer,
        *cancel,
        *operations,
        *selection_label);
    CHECK(chamfer->isChecked());
    CHECK(!fillet->isEnabled());
    CHECK(
        workbench.cadInputPrompt().find(
            "CHAMFER") != std::string::npos);

    result =
        workbench.lockCadDynamicInputField(
            0U,
            "1",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!viewport->solid_preview.empty());
    cancel->click();
    QApplication::processEvents();
    CHECK(
        chamfer_session.undoDepth() ==
        chamfer_undo);
    CHECK(
        chamfer_session.document().body()
            .features.size() == 1U);

    verifyCommandFirstFinish(
        workbench,
        *viewport,
        chamfer_session,
        "CHAMFER",
        false);

    std::cout
        << "PM05D2_CAD_WORKBENCH_EDGE_FEATURES_PASS"
        << " toolbar_groups=1"
        << " selection_first=1"
        << " command_first=1"
        << " multi_edge=1"
        << " exact_preview=1"
        << " stale_context=1"
        << " fillet=1"
        << " chamfer=1\n";
    return EXIT_SUCCESS;
}
