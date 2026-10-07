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
    CHECK(
        operations && title && parameter &&
        finish && cancel && edit && suppress && remove);

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

    std::cout
        << "PM05E2_CAD_WORKBENCH_EDGE_LIFECYCLE_PASS"
        << " tool_stage=1"
        << " cancel_zero_mutation=1"
        << " stable_feature_id=1"
        << " suppress=1"
        << " delete_undo=1\n";
    return EXIT_SUCCESS;
}
