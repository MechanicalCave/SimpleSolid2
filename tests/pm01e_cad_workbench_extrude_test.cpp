#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01E workbench CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class FakeSolid final
    : public kernel::RuntimeSolid {};

class FakeKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }
        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }

    kernel::SolidPresentationResult
    presentationMesh(
        kernel::RuntimeSolidHandle solid) noexcept override {
        if (dynamic_cast<const FakeSolid*>(
                solid.get()) == nullptr) {
            return {
                kernel::SolidPresentationStatus::
                    provider_mismatch,
                {}};
        }
        kernel::SolidPresentationMesh mesh;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, 0.0},
                {10.0, 0.0, 0.0},
                {0.0, 10.0, 0.0},
                {0.0, 0.0, 1.0}});
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
    }
};

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
    bool setSolidScene(
        const viewer::SolidScene& scene) override {
        if (!scene.valid()) return false;
        solid_scene = scene;
        return true;
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
    bool setPresentationSelection(
        const viewer::PresentationSelection& scene) override {
        return scene.valid();
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
    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting) override {}
    void setCursorMode(
        viewer::ViewportCursorMode) override {}

    viewer::CameraState camera_;
    viewer::SolidScene solid_scene;
    viewer::SolidPreviewScene solid_preview;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
};

part::ProfileId createProfile(
    application::DocumentSession& session) {
    const auto sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {20.0, 10.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* source =
        session.document().findSketch(
            *sketch.sketch_id);
    CHECK(source != nullptr);
    const auto regions =
        sketch::analyzeRegions(
            source->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);
    return *profile.profile_id;
}

QTreeWidgetItem* findProfileItem(
    QTreeWidget& tree) {
    const auto matches =
        tree.findItems(
            QStringLiteral("Profile"),
            Qt::MatchContains |
                Qt::MatchRecursive,
            0);
    return matches.empty()
        ? nullptr
        : matches.front();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};
    const auto profile_id =
        createProfile(session);
    CHECK(profile_id.valid());

    FakeKernel kernel;
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
    CHECK(
        workbench.activateDocument(
            &session,
            {}));

    auto* tree =
        workbench.findChild<QTreeWidget*>();
    CHECK(tree != nullptr);
    auto* profile_item =
        findProfileItem(*tree);
    CHECK(profile_item != nullptr);
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);

    auto* extrude =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "extrudeToolButton"));
    auto* distance =
        workbench.findChild<QLineEdit*>(
            QStringLiteral(
                "extrudeDistanceEdit"));
    auto* reverse =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "extrudeReverseButton"));
    auto* midplane =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "extrudeMidplaneButton"));
    auto* one_side =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "extrudeOneSideButton"));
    auto* finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "extrudeFinishButton"));
    auto* cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "extrudeCancelButton"));
    CHECK(
        extrude && distance && reverse &&
        midplane && one_side &&
        finish && cancel);
    CHECK(extrude->isEnabled());

    const auto undo_before =
        session.undoDepth();
    extrude->click();
    CHECK(!finish->isEnabled());
    CHECK(viewport->solid_preview.empty());

    distance->setText(
        QStringLiteral("10 mm"));
    CHECK(finish->isEnabled());
    CHECK(!viewport->solid_preview.empty());
    CHECK(
        viewport->solid_preview.tone ==
        viewer::SolidPreviewTone::additive);

    reverse->click();
    CHECK(reverse->isChecked());
    CHECK(!viewport->solid_preview.empty());

    midplane->click();
    CHECK(midplane->isChecked());
    CHECK(!reverse->isEnabled());
    CHECK(!reverse->isChecked());

    one_side->click();
    CHECK(one_side->isChecked());
    CHECK(reverse->isEnabled());

    finish->click();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        session.undoDepth() ==
        undo_before + 1U);
    CHECK(viewport->solid_preview.empty());
    CHECK(!viewport->solid_scene.empty());

    // Command Line enters the same draft API and CANCEL remains non-authoring.
    profile_item =
        findProfileItem(*tree);
    CHECK(profile_item != nullptr);
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);

    auto result =
        workbench.submitCadInput(
            "EXTRUDE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        workbench.cadInputPrompt().find(
            "EXTRUDE") !=
        std::string::npos);

    result =
        workbench.submitCadInput(
            "5mm",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!viewport->solid_preview.empty());

    result =
        workbench.submitCadInput(
            "MIDPLANE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!reverse->isEnabled());

    const auto feature_count_before_cancel =
        session.document().body()
            .features.size();
    const auto undo_before_cancel =
        session.undoDepth();
    result =
        workbench.submitCadInput(
            "CANCEL",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        session.document().body()
            .features.size() ==
        feature_count_before_cancel);
    CHECK(
        session.undoDepth() ==
        undo_before_cancel);
    CHECK(viewport->solid_preview.empty());

    return EXIT_SUCCESS;
}
