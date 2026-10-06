#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QEventLoop>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTimer>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

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
            << "PM-04D2 Revolve workbench CHECK failed at line "
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
    std::vector<kernel::AngularRevolveInput>
        revolve_inputs;
    std::vector<kernel::AngularRevolveInput>
        preview_inputs;

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::missing_upstream;
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

    kernel::SolidModelingResult revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::missing_upstream;
            return result;
        }
        revolve_inputs.push_back(input);
        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }

    kernel::SolidPresentationResult
    revolvePreviewMesh(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        if (!input.valid() ||
            (input.operation ==
                 kernel::SolidBooleanOperation::cut &&
             upstream == nullptr)) {
            return {
                kernel::SolidPresentationStatus::invalid_input,
                {}};
        }
        preview_inputs.push_back(input);
        return {
            kernel::SolidPresentationStatus::ok,
            triangleMesh()};
    }

    kernel::SolidPresentationResult
    presentationMesh(
        kernel::RuntimeSolidHandle solid) noexcept override {
        if (dynamic_cast<const FakeSolid*>(
                solid.get()) == nullptr) {
            return {
                kernel::SolidPresentationStatus::provider_mismatch,
                {}};
        }
        return {
            kernel::SolidPresentationStatus::ok,
            triangleMesh()};
    }

private:
    static kernel::SolidPresentationMesh
    triangleMesh() {
        kernel::SolidPresentationMesh mesh;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, 0.0},
                {10.0, 0.0, 0.0},
                {0.0, 10.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0}});
        return mesh;
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
        if (!scene.valid()) return false;
        reference_scene = scene;
        return true;
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

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting) override {}

    void setCursorMode(
        viewer::ViewportCursorMode) override {}

    viewer::CameraState camera_;
    viewer::ReferenceScene reference_scene;
    viewer::BodyScene body_scene;
    viewer::SolidScene solid_scene;
    viewer::SolidPreviewScene solid_preview;
    viewer::ProfileScene profile_scene;
    viewer::PresentationSelection presentation_selection;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
};

part::ProfileId createProfile(
    application::DocumentSession& session) {
    const auto sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
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
        sketch::analyzeRegions(source->model);
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

QTreeWidgetItem* findItem(
    QTreeWidget& tree,
    const QString& text) {
    const auto matches =
        tree.findItems(
            text,
            Qt::MatchContains |
                Qt::MatchRecursive,
            0);
    return matches.empty()
        ? nullptr
        : matches.front();
}

const viewer::ReferencePresentation*
xAxisPresentation(
    const viewer::ReferenceScene& scene) {
    const auto found =
        std::find_if(
            scene.references.begin(),
            scene.references.end(),
            [](const auto& reference) {
                return reference.kind ==
                    viewer::ReferencePresentationKind::x_axis;
            });
    return found == scene.references.end()
        ? nullptr
        : &*found;
}

bool selectionContains(
    const viewer::PresentationSelection& selection,
    viewer::PresentationToken token) {
    return std::find(
               selection.selected.begin(),
               selection.selected.end(),
               token) !=
           selection.selected.end();
}

void selectOnly(
    QTreeWidget& tree,
    QTreeWidgetItem* item) {
    CHECK(item != nullptr);
    tree.clearSelection();
    item->setSelected(true);
    tree.setCurrentItem(item);
    QApplication::processEvents();
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

    const auto hide_x =
        session.execute(
            application::
                SetBuiltinReferenceVisibilityCommand{
                    {core::BuiltinReferenceRole::x_axis},
                    false});
    CHECK(hide_x.ok());
    CHECK(
        !session.document()
             .builtinReferenceVisible(
                 core::BuiltinReferenceRole::x_axis));

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
    QApplication::processEvents();

    auto* tree =
        workbench.findChild<QTreeWidget*>();
    auto* revolve =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveToolButton"));
    auto* operations =
        workbench.findChild<QWidget*>(
            QStringLiteral("revolveOperationsWidget"));
    auto* profile_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("revolveProfileLabel"));
    auto* axis_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("revolveAxisLabel"));
    auto* angle =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("revolveAngleEdit"));
    auto* reverse =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveReverseButton"));
    auto* midplane =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveMidplaneButton"));
    auto* one_side =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveOneSideButton"));
    auto* finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveFinishButton"));
    auto* cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral("revolveCancelButton"));
    CHECK(
        tree && revolve && operations &&
        profile_label && axis_label && angle &&
        reverse && midplane && one_side &&
        finish && cancel);

    auto* profile_item =
        findItem(*tree, QStringLiteral("Profile"));
    CHECK(profile_item != nullptr);

    const auto* x_before =
        xAxisPresentation(
            viewport->reference_scene);
    CHECK(x_before != nullptr);
    CHECK(!x_before->visible);

    // Command-first and source-order parity: Axis may be acquired before the
    // Profile. Nothing authored changes while the draft is incomplete.
    const auto undo_before_cancel =
        session.undoDepth();
    auto result =
        workbench.submitCadInput(
            "REVOLVE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    QApplication::processEvents();
    CHECK(operations->isVisible());
    CHECK(revolve->isChecked());
    CHECK(
        workbench.cadInputPrompt().find(
            "Profile") !=
        std::string::npos);
    CHECK(
        angle->text().contains(
            QStringLiteral("360")));

    result =
        workbench.submitCadInput(
            "X",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    QApplication::processEvents();
    CHECK(
        axis_label->text().contains(
            QStringLiteral("Origin X")));
    CHECK(
        workbench.cadInputPrompt().find(
            "Profile") !=
        std::string::npos);

    const auto* x_draft =
        xAxisPresentation(
            viewport->reference_scene);
    CHECK(x_draft != nullptr);
    CHECK(x_draft->visible);
    CHECK(
        selectionContains(
            viewport->presentation_selection,
            x_draft->token));
    CHECK(
        !session.document()
             .builtinReferenceVisible(
                 core::BuiltinReferenceRole::x_axis));

    selectOnly(*tree, profile_item);
    CHECK(
        profile_label->text() ==
        QString::fromStdString(
            profile_id.serialized()));
    CHECK(finish->isEnabled());
    CHECK(!viewport->solid_preview.empty());
    CHECK(
        viewport->solid_preview.tone ==
        viewer::SolidPreviewTone::additive);
    CHECK(viewport->profile_scene.profiles.empty());
    CHECK(
        session.document()
            .profilePresentationVisible(
                profile_id));

    midplane->click();
    CHECK(midplane->isChecked());
    CHECK(!reverse->isEnabled());
    one_side->click();
    CHECK(one_side->isChecked());
    CHECK(reverse->isEnabled());

    // Dynamic Input is the same semantic Angle setter. Mutating the draft
    // invalidates the prior context generation; stale input fails closed.
    const auto stale_generation =
        workbench.cadInputContextGeneration();
    result =
        workbench.lockCadDynamicInputField(
            0U,
            "180 deg",
            stale_generation);
    CHECK(result.accepted);
    CHECK(
        angle->text().contains(
            QStringLiteral("180")));
    const auto stale_finish =
        workbench.submitCadInput(
            "FINISH",
            stale_generation);
    CHECK(!stale_finish.accepted);
    CHECK(finish->isEnabled());

    cancel->click();
    QApplication::processEvents();
    CHECK(!operations->isVisible());
    CHECK(
        session.undoDepth() ==
        undo_before_cancel);
    CHECK(viewport->solid_preview.empty());
    const auto* x_after_cancel =
        xAxisPresentation(
            viewport->reference_scene);
    CHECK(x_after_cancel != nullptr);
    CHECK(!x_after_cancel->visible);
    CHECK(
        !selectionContains(
            viewport->presentation_selection,
            x_after_cancel->token));

    // Selection-first seeds the selected Profile into the same draft.
    profile_item =
        findItem(*tree, QStringLiteral("Profile"));
    selectOnly(*tree, profile_item);
    const auto undo_before_create =
        session.undoDepth();
    revolve->click();
    QApplication::processEvents();
    CHECK(operations->isVisible());
    CHECK(
        profile_label->text() ==
        QString::fromStdString(
            profile_id.serialized()));
    CHECK(!finish->isEnabled());

    result =
        workbench.submitCadInput(
            "X",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(finish->isEnabled());
    CHECK(!viewport->solid_preview.empty());
    CHECK(!kernel.preview_inputs.empty());

    finish->click();
    QApplication::processEvents();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        session.undoDepth() ==
        undo_before_create + 1U);
    CHECK(viewport->solid_preview.empty());
    CHECK(!viewport->body_scene.empty());
    CHECK(!kernel.revolve_inputs.empty());

    const auto& feature =
        session.document().body()
            .features.front();
    const auto created_feature_id =
        feature.id;
    const auto* created_revolve =
        std::get_if<part::RevolveFeature>(
            &feature.definition);
    CHECK(created_revolve != nullptr);
    CHECK(
        part::builtinOriginAxisForAxisReference(
            created_revolve->axis) ==
        core::BuiltinReferenceRole::x_axis);
    const auto* created_extent =
        std::get_if<
            part::OneSidedRevolveExtent>(
            &created_revolve->extent);
    CHECK(created_extent != nullptr);
    CHECK(
        std::abs(
            created_extent->angle.radians -
            2.0 * std::numbers::pi_v<double>) <
        1.0e-12);
    CHECK(
        kernel.preview_inputs.back() ==
        kernel.revolve_inputs.back());

    const auto* x_after_finish =
        xAxisPresentation(
            viewport->reference_scene);
    CHECK(x_after_finish != nullptr);
    CHECK(!x_after_finish->visible);

    // Edit routes Revolve (not Extrude) back through the same draft and
    // preserves FeatureId. Invalid Angle removes preview, but the source Axis
    // remains a runtime-only spatial cue.
    auto* feature_item =
        findItem(*tree, QStringLiteral("Revolve"));
    CHECK(feature_item != nullptr);
    selectOnly(*tree, feature_item);

    auto* edit =
        workbench.findChild<QPushButton*>(
            QStringLiteral("featureEditExtrudeButton"));
    CHECK(edit != nullptr);
    CHECK(
        edit->text() ==
        QStringLiteral("Edit Revolve"));

    const auto undo_before_edit =
        session.undoDepth();
    edit->click();
    QApplication::processEvents();
    CHECK(operations->isVisible());
    CHECK(
        axis_label->text().contains(
            QStringLiteral("Origin X")));
    CHECK(
        xAxisPresentation(
            viewport->reference_scene)
            ->visible);

    angle->clear();
    CHECK(!finish->isEnabled());
    CHECK(viewport->solid_preview.empty());
    CHECK(!viewport->profile_scene.profiles.empty());
    CHECK(
        xAxisPresentation(
            viewport->reference_scene)
            ->visible);

    angle->setText(
        QStringLiteral("180 deg"));
    CHECK(!finish->isEnabled());
    waitForPreviewDebounce();
    CHECK(finish->isEnabled());
    CHECK(!viewport->solid_preview.empty());

    finish->click();
    QApplication::processEvents();
    CHECK(
        session.document().body()
            .features.size() == 1U);
    CHECK(
        session.document().body()
            .features.front().id ==
        created_feature_id);
    CHECK(
        session.undoDepth() ==
        undo_before_edit + 1U);
    const auto* edited_revolve =
        std::get_if<part::RevolveFeature>(
            &session.document().body()
                 .features.front()
                 .definition);
    CHECK(edited_revolve != nullptr);
    const auto* edited_extent =
        std::get_if<
            part::OneSidedRevolveExtent>(
            &edited_revolve->extent);
    CHECK(edited_extent != nullptr);
    CHECK(
        std::abs(
            edited_extent->angle.radians -
            std::numbers::pi_v<double>) <
        1.0e-12);
    CHECK(
        !session.document()
             .builtinReferenceVisible(
                 core::BuiltinReferenceRole::x_axis));
    CHECK(
        !xAxisPresentation(
             viewport->reference_scene)
             ->visible);

    std::cout
        << "PM-04D2 Revolve workbench tests passed\n";
    return EXIT_SUCCESS;
}
