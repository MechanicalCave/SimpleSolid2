#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QComboBox>
#include <QEventLoop>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QWidget>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <variant>

using namespace simplesolid2;

namespace {

void waitForPreviewDebounce() {
    QEventLoop loop;
    QTimer::singleShot(
        130,
        &loop,
        &QEventLoop::quit);
    loop.exec();
}

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
    extrudePreviewMesh(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        if (!input.valid() ||
            (input.operation ==
                 kernel::SolidBooleanOperation::cut &&
             upstream == nullptr)) {
            return {
                kernel::SolidPresentationStatus::
                    invalid_input,
                {}};
        }
        kernel::SolidPresentationMesh mesh;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, 0.0},
                {10.0, 0.0, 0.0},
                {0.0, 10.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0}});
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
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
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
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
    bool setBodyScene(
        const viewer::BodyScene& scene) override {
        if (!scene.valid()) return false;
        body_scene = scene;
        return true;
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
    bool setProfileScene(
        const viewer::ProfileScene& scene) override {
        if (!scene.valid()) return false;
        profile_scene = scene;
        return true;
    }
    bool setProfilePreviewScene(
        const viewer::ProfilePreviewScene& scene) override {
        return scene.valid();
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
    viewer::BodyScene body_scene;
    viewer::SolidScene solid_scene;
    viewer::SolidPreviewScene solid_preview;
    viewer::ProfileScene profile_scene;
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


QTreeWidgetItem* findFeatureItem(
    QTreeWidget& tree,
    const QString& status =
        QStringLiteral("UpToDate")) {
    const auto matches =
        tree.findItems(
            QStringLiteral("Extrude"),
            Qt::MatchContains |
                Qt::MatchRecursive,
            0);
    for (auto* item : matches) {
        if (item != nullptr &&
            item->text(0).contains(
                QStringLiteral("[") +
                status +
                QStringLiteral("]"))) {
            return item;
        }
    }
    return nullptr;
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

    auto* root_item = tree->topLevelItem(0);
    CHECK(root_item != nullptr);
    CHECK(root_item->childCount() >= 1);
    CHECK(
        root_item
            ->child(root_item->childCount() - 1)
            ->text(0)
            .startsWith(QStringLiteral("Body")));

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
    // Command-first Extrude is available without a preselected Profile.
    CHECK(extrude->isEnabled());
    CHECK(!extrude->isChecked());

    const auto undo_before =
        session.undoDepth();
    extrude->click();
    CHECK(extrude->isChecked());
    CHECK(
        workbench.cadInputPrompt().find(
            "Select one valid Profile") !=
        std::string::npos);
    CHECK(viewport->solid_preview.empty());

    // Selecting one valid Profile completes the pick state and immediately
    // starts the same Extrude draft with the product default 10 mm preview.
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);
    CHECK(distance->text().contains(
        QStringLiteral("10")));
    CHECK(finish->isEnabled());
    CHECK(!viewport->solid_preview.empty());
    CHECK(
        viewport->solid_preview.tone ==
        viewer::SolidPreviewTone::additive);
    // H5: a ready solid preview hides only the rendered source Profile.
    // The authored automatic visibility policy remains untouched.
    CHECK(viewport->profile_scene.profiles.empty());
    CHECK(
        session.document().profilePresentationVisible(
            profile_id));

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
    CHECK(viewport->profile_scene.profiles.empty());

    // Feature Tree + Properties expose the durable relationship without
    // nesting the source Profile under the Feature.
    auto* feature_item =
        findFeatureItem(*tree);
    CHECK(feature_item != nullptr);
    feature_item->setSelected(true);
    tree->setCurrentItem(feature_item);

    auto* feature_page =
        workbench.findChild<QWidget*>(
            QStringLiteral(
                "featurePropertiesPage"));
    auto* feature_id_label =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "featurePropertyIdentity"));
    auto* source_profile_label =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "featurePropertySourceProfile"));
    auto* edit_feature =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "featureEditExtrudeButton"));
    auto* go_profile =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "featureGoToProfileButton"));
    CHECK(
        feature_page &&
        feature_id_label &&
        source_profile_label &&
        edit_feature &&
        go_profile);
    CHECK(
        feature_id_label->text() ==
        QStringLiteral("1"));
    CHECK(
        source_profile_label->text() ==
        QString::fromStdString(
            profile_id.serialized()));

    const auto feature_id_before_edit =
        session.document().body()
            .features.front().id;
    const auto undo_before_edit =
        session.undoDepth();
    edit_feature->click();
    CHECK(viewport->profile_scene.profiles.empty());
    CHECK(distance->text().contains(
        QStringLiteral("10")));

    // Invalid text clears the preview and reveals the source Profile only as
    // transient diagnostic presentation.
    distance->clear();
    CHECK(!finish->isEnabled());
    CHECK(viewport->solid_preview.empty());
    CHECK(!viewport->profile_scene.profiles.empty());

    distance->setText(
        QStringLiteral("12 mm"));
    // Typing updates the draft immediately but expensive Body evaluation /
    // tessellation is debounced so the edit control remains responsive.
    CHECK(!finish->isEnabled());
    waitForPreviewDebounce();
    CHECK(finish->isEnabled());
    CHECK(viewport->profile_scene.profiles.empty());
    finish->click();

    CHECK(
        session.document().body()
            .features.front().id ==
        feature_id_before_edit);
    CHECK(
        session.undoDepth() ==
        undo_before_edit + 1U);
    const auto* edited_extrude =
        std::get_if<part::ExtrudeFeature>(
            &session.document().body()
                 .features.front()
                 .definition);
    CHECK(edited_extrude != nullptr);
    const auto* edited_extent =
        std::get_if<
            part::OneSidedExtrudeExtent>(
            &edited_extrude->extent);
    CHECK(edited_extent != nullptr);
    CHECK(
        edited_extent->distance.millimetres ==
        12.0);
    CHECK(viewport->profile_scene.profiles.empty());

    // Bidirectional relationship navigation: Feature -> Profile -> Feature.
    feature_item =
        findFeatureItem(*tree);
    CHECK(feature_item != nullptr);
    feature_item->setSelected(true);
    tree->setCurrentItem(feature_item);
    go_profile->click();

    auto* consuming =
        workbench.findChild<QComboBox*>(
            QStringLiteral(
                "profileConsumingFeaturesCombo"));
    auto* go_feature =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "profileGoToFeatureButton"));
    CHECK(consuming && go_feature);
    CHECK(consuming->count() == 1);
    CHECK(go_feature->isEnabled());
    go_feature->click();
    CHECK(
        feature_id_label->text() ==
        QStringLiteral("1"));

    // Feature lifecycle is authored, Undoable, and drives automatic Profile
    // visibility without introducing Body/Feature visibility state.
    auto* suppress_feature =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "featureSuppressButton"));
    auto* delete_feature =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "featureDeleteButton"));
    CHECK(suppress_feature && delete_feature);

    const auto undo_before_suppress =
        session.undoDepth();
    suppress_feature->click();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        session.document().body()
            .features.front().suppressed);
    CHECK(
        session.undoDepth() ==
        undo_before_suppress + 1U);
    CHECK(viewport->solid_scene.empty());
    CHECK(!viewport->profile_scene.profiles.empty());
    CHECK(
        findFeatureItem(
            *tree,
            QStringLiteral("Suppressed")) !=
        nullptr);
    CHECK(
        suppress_feature->text() ==
        QStringLiteral("Unsuppress Feature"));
    CHECK(!edit_feature->isEnabled());

    workbench.requestUndo();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        !session.document().body()
             .features.front().suppressed);
    CHECK(
        session.document().body()
            .features.front().id ==
        feature_id_before_edit);
    CHECK(!viewport->solid_scene.empty());
    CHECK(viewport->profile_scene.profiles.empty());

    workbench.requestRedo();
    CHECK(
        session.document().body()
            .features.front().suppressed);
    CHECK(viewport->solid_scene.empty());
    CHECK(!viewport->profile_scene.profiles.empty());

    suppress_feature->click();
    CHECK(
        !session.document().body()
             .features.front().suppressed);
    CHECK(!viewport->solid_scene.empty());
    CHECK(viewport->profile_scene.profiles.empty());
    CHECK(
        findFeatureItem(*tree) != nullptr);

    feature_item =
        findFeatureItem(*tree);
    CHECK(feature_item != nullptr);
    tree->clearSelection();
    feature_item->setSelected(true);
    tree->setCurrentItem(feature_item);
    const auto undo_before_delete =
        session.undoDepth();
    delete_feature->click();
    CHECK(
        session.document().body()
            .features.empty());
    CHECK(
        session.undoDepth() ==
        undo_before_delete + 1U);
    CHECK(viewport->solid_scene.empty());
    CHECK(!viewport->profile_scene.profiles.empty());

    workbench.requestUndo();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        !session.document().body()
             .features.front().suppressed);
    CHECK(!viewport->solid_scene.empty());
    CHECK(viewport->profile_scene.profiles.empty());
    CHECK(
        session.document().body()
            .features.front().id ==
        feature_id_before_edit);

    workbench.requestRedo();
    CHECK(
        session.document().body()
            .features.empty());
    CHECK(viewport->solid_scene.empty());
    CHECK(!viewport->profile_scene.profiles.empty());

    workbench.requestUndo();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        session.document().body()
            .features.front().id ==
        feature_id_before_edit);
    CHECK(!viewport->solid_scene.empty());
    CHECK(viewport->profile_scene.profiles.empty());

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
    CHECK(distance->text().contains(
        QStringLiteral("10")));
    CHECK(!viewport->solid_preview.empty());

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

    // Global Command Line empty Enter is Finish only while Extrude owns the
    // semantic endpoint; legacy empty-Enter behavior remains default elsewhere.
    profile_item =
        findProfileItem(*tree);
    CHECK(profile_item != nullptr);
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);

    application::CadInputSession input;
    input.attachEndpoint(&workbench);
    input.setBuffer("EXTRUDE");
    CHECK(input.submit().accepted);
    input.setBuffer("3mm");
    CHECK(input.submit().accepted);
    const auto before_enter_count =
        session.document().body()
            .features.size();
    CHECK(input.submit().accepted);
    CHECK(
        session.document().body()
            .features.size() ==
        before_enter_count + 1U);
    CHECK(viewport->solid_preview.empty());

    return EXIT_SUCCESS;
}
