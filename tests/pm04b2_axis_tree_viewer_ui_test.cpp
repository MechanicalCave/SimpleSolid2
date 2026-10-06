#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/axis_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/sketch/entity_role.hpp>

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04B2 Axis UI CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class EmptyKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        kernel::SolidModelingResult result;
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
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
        return scene.valid();
    }

    bool setSolidScene(
        const viewer::SolidScene& scene) override {
        return scene.valid();
    }

    bool setSolidPreviewScene(
        const viewer::SolidPreviewScene& scene) override {
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

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        return scene.valid();
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        return scene.valid();
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        if (!selection.valid()) return false;
        presentation_selection = selection;
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

    void emitSelectionIntent(
        const viewer::SelectionIntent& intent) {
        CHECK(static_cast<bool>(selection_handler));
        selection_handler(intent);
        QApplication::processEvents();
    }

    viewer::CameraState camera_;
    viewer::ReferenceScene reference_scene;
    viewer::PresentationSelection
        presentation_selection;
    viewer::SelectionIntentHandler
        selection_handler;
    viewer::SpatialPointerHandler
        spatial_handler;
};

QTreeWidgetItem* findItem(
    QTreeWidget& tree,
    const QString& text) {
    const auto matches =
        tree.findItems(
            text,
            Qt::MatchExactly |
                Qt::MatchRecursive,
            0);
    return matches.empty()
        ? nullptr
        : matches.front();
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

std::optional<viewer::ReferencePresentation>
axisPresentation(
    const viewer::ReferenceScene& scene) {
    const auto found =
        std::find_if(
            scene.references.begin(),
            scene.references.end(),
            [](const auto& reference) {
                return reference.kind ==
                    viewer::
                        ReferencePresentationKind::
                            axis;
            });
    return found == scene.references.end()
        ? std::nullopt
        : std::optional<
              viewer::ReferencePresentation>{
              *found};
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());

    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());

    const auto sketch_id =
        sketch::SketchId::generate();
    sketch::SketchModel model;
    const auto line_id =
        model.addLine(
            {0.0, 0.0},
            {10.0, 0.0},
            sketch::EntityRole::regular);

    auto state = document.state();
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *support,
            true,
            std::move(model)});
    part::PartDocumentTransaction transaction{
        document};
    transaction.replaceState(std::move(state));
    CHECK(transaction.commit().ok());

    application::DocumentSession session{
        {},
        std::move(document)};
    EmptyKernel kernel;

    const auto created =
        session.execute(
            application::CreateAxisCommand{
                {sketch_id, line_id},
                session.document().revision(),
                {},
                true},
            kernel);
    CHECK(created.ok());
    CHECK(created.axis_id.has_value());
    const auto axis_id =
        *created.axis_id;
    CHECK(axis_id.serialized() == "1");

    TestViewport* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            auto* created_viewport =
                new TestViewport(parent);
            viewport = created_viewport;
            return ui::ViewportSurface{
                created_viewport,
                created_viewport};
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
    auto* axis_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisToolButton"));
    auto* axis_page =
        workbench.findChild<QWidget*>(
            QStringLiteral("axisPropertiesPage"));
    auto* properties =
        workbench.findChild<QStackedWidget*>();
    auto* axis_identity =
        workbench.findChild<QLabel*>(
            QStringLiteral("axisPropertyIdentity"));
    auto* axis_source_sketch =
        workbench.findChild<QLabel*>(
            QStringLiteral("axisPropertySourceSketch"));
    auto* axis_source_line =
        workbench.findChild<QLabel*>(
            QStringLiteral("axisPropertySourceLine"));
    auto* axis_visibility =
        workbench.findChild<QLabel*>(
            QStringLiteral("axisPropertyVisibility"));
    auto* axis_status =
        workbench.findChild<QLabel*>(
            QStringLiteral("axisPropertyStatus"));
    auto* axis_direction =
        workbench.findChild<QLabel*>(
            QStringLiteral("axisPropertyDirection"));
    auto* axis_edit =
        workbench.findChild<QPushButton*>(
            QStringLiteral("editAxisPropertyButton"));
    auto* axis_operations =
        workbench.findChild<QWidget*>(
            QStringLiteral("axisOperationsWidget"));
    auto* axis_finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisFinishButton"));
    auto* axis_cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisCancelButton"));

    CHECK(
        tree && axis_page &&
        properties && axis_identity &&
        axis_source_sketch && axis_source_line &&
        axis_visibility && axis_status &&
        axis_direction && axis_edit &&
        axis_operations && axis_finish &&
        axis_cancel);
    CHECK(axis_tool == nullptr);

    // Authored Axis is a Part-owned child of its source Sketch, alongside
    // Profile ownership level rather than a Sketch entity or Origin object.
    auto* sketch_item =
        findItem(
            *tree,
            QStringLiteral("Sketch 1"));
    auto* axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    CHECK(sketch_item != nullptr);
    CHECK(axis_item != nullptr);
    CHECK(axis_item->parent() == sketch_item);

    // Resolved + visible Axis has one finite Viewer presentation. The token is
    // presentation-only and picking it must drive semantic Axis selection.
    auto presentation =
        axisPresentation(
            viewport->reference_scene);
    CHECK(presentation.has_value());
    CHECK(presentation->token.valid());
    CHECK(presentation->extent > 0.0);
    CHECK(
        presentation->u_axis.squaredLength() >
        0.0);

    viewport->emitSelectionIntent(
        viewer::SelectionIntent{
            presentation->token,
            viewer::SelectionIntentMode::replace});
    CHECK(axis_item->isSelected());
    CHECK(properties->currentWidget() == axis_page);
    CHECK(
        axis_identity->text() ==
        QStringLiteral("1"));
    CHECK(
        axis_source_sketch->text() ==
        QString::fromUtf8(
            sketch_id.value().data(),
            static_cast<qsizetype>(
                sketch_id.value().size())));
    CHECK(
        axis_source_line->text() ==
        QString::fromStdString(
            line_id.serialized()));
    CHECK(
        axis_visibility->text() ==
        QStringLiteral("Shown"));
    CHECK(
        axis_status->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        axis_direction->text().contains(
            QStringLiteral("1")));

    // Tree Show/Hide authors independent Axis visibility and removes/restores
    // only the finite presentation. Evaluation remains semantic truth.
    auto* hide_action =
        tree->findChild<QAction*>(
            QStringLiteral(
                "hideBuiltinReferencesAction"));
    auto* show_action =
        tree->findChild<QAction*>(
            QStringLiteral(
                "showBuiltinReferencesAction"));
    CHECK(hide_action && show_action);

    selectOnly(*tree, axis_item);
    CHECK(hide_action->isEnabled());
    hide_action->trigger();
    QApplication::processEvents();
    CHECK(
        !session.document()
             .findAxis(axis_id)
             ->visible);
    CHECK(
        !axisPresentation(
             viewport->reference_scene)
             .has_value());

    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    CHECK(axis_item != nullptr);
    selectOnly(*tree, axis_item);
    CHECK(show_action->isEnabled());
    show_action->trigger();
    QApplication::processEvents();
    CHECK(
        session.document()
            .findAxis(axis_id)
            ->visible);
    presentation =
        axisPresentation(
            viewport->reference_scene);
    CHECK(presentation.has_value());

    // GUI Edit uses the shared AxisDraft. Current source is already valid, so
    // Finish is immediately committable and preserves the durable AxisId.
    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    selectOnly(*tree, axis_item);
    CHECK(axis_edit->isEnabled());
    axis_edit->click();
    QApplication::processEvents();
    CHECK(axis_operations->isVisible());
    CHECK(axis_finish->isEnabled());
    CHECK(
        workbench.cadInputPrompt().find(
            "AXIS") !=
        std::string::npos);

    axis_finish->click();
    QApplication::processEvents();
    CHECK(!axis_operations->isVisible());
    CHECK(
        session.document()
            .findAxis(axis_id) != nullptr);
    CHECK(
        session.document()
            .findAxis(axis_id)
            ->id == axis_id);
    CHECK(
        session.document().axes().size() ==
        1U);

    // Command-first activates the same workbench draft without a standalone
    // GUI Axis authoring button. Cancel authors nothing; AXIS remains available
    // through CAD Input while Sketch Edit is active.
    auto result =
        workbench.submitCadInput(
            "AXIS",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    QApplication::processEvents();
    CHECK(axis_operations->isVisible());

    result =
        workbench.submitCadInput(
            "CANCEL",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    QApplication::processEvents();
    CHECK(!axis_operations->isVisible());
    CHECK(
        session.document().axes().size() ==
        1U);

    std::cout
        << "PM-04B2 Axis Tree/Viewer/UI tests passed\n";
    return EXIT_SUCCESS;
}
