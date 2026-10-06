#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/axis.hpp>

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
#include <filesystem>
#include <iostream>
#include <optional>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04B2 Axis Tree/Viewer/UI CHECK failed at line "
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
        if (!scene.valid()) return false;
        sketch_scene = scene;
        return true;
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

    void emitSelection(
        viewer::PresentationToken token) {
        CHECK(static_cast<bool>(
            selection_handler));
        selection_handler(
            viewer::SelectionIntent{
                token,
                viewer::SelectionIntentMode::
                    replace});
    }

    viewer::CameraState camera_;
    viewer::ReferenceScene reference_scene;
    viewer::SketchScene sketch_scene;
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

const viewer::ReferencePresentation*
authoredAxisPresentation(
    const viewer::ReferenceScene& scene) {
    const auto found =
        std::find_if(
            scene.references.begin(),
            scene.references.end(),
            [](const viewer::ReferencePresentation& item) {
                return item.kind ==
                    viewer::ReferencePresentationKind::
                        axis;
            });
    return found == scene.references.end()
        ? nullptr
        : &*found;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "pm04b2-axis-ui.ss2part"},
        std::move(document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(sketch_created.ok());
    CHECK(sketch_created.sketch_id.has_value());
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto line_created =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {1.0, 2.0},
                {11.0, 2.0}});
    CHECK(line_created.ok());
    CHECK(line_created.entity_id.has_value());
    const auto line_id =
        *line_created.entity_id;

    EmptyKernel kernel;
    const auto axis_created =
        session.execute(
            application::CreateAxisCommand{
                {sketch_id, line_id},
                session.document().revision(),
                {},
                true},
            kernel);
    CHECK(axis_created.ok());
    CHECK(axis_created.axis_id.has_value());
    const auto axis_id =
        *axis_created.axis_id;

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
    auto* properties =
        workbench.findChild<QStackedWidget*>(
            QStringLiteral(
                "propertiesContextStack"));
    auto* axis_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisToolButton"));
    auto* axis_finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisFinishButton"));
    auto* axis_cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisCancelButton"));
    auto* axis_identity =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyIdentity"));
    auto* axis_source_sketch =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertySourceSketch"));
    auto* axis_source_line =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertySourceLine"));
    auto* axis_visibility =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyVisibility"));
    auto* axis_status =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyStatus"));
    auto* axis_origin =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyOrigin"));
    auto* axis_direction =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyDirection"));
    auto* axis_edit =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "editAxisPropertyButton"));
    auto* hide =
        workbench.findChild<QAction*>(
            QStringLiteral(
                "hideBuiltinReferencesAction"));
    auto* show =
        workbench.findChild<QAction*>(
            QStringLiteral(
                "showBuiltinReferencesAction"));

    CHECK(
        tree && properties && axis_tool &&
        axis_finish && axis_cancel &&
        axis_identity &&
        axis_source_sketch &&
        axis_source_line &&
        axis_visibility &&
        axis_status &&
        axis_origin &&
        axis_direction &&
        axis_edit &&
        hide && show);

    // Authored Axis is a Part-level child of its source Sketch, not a
    // synthetic Origin/Reference Geometry object.
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

    // Viewer publishes a finite line presentation but its binding remains
    // semantic AxisId; source EntityId is not the pick identity.
    const auto* axis_presentation =
        authoredAxisPresentation(
            viewport->reference_scene);
    CHECK(axis_presentation != nullptr);
    CHECK(axis_presentation->token.valid());
    CHECK(axis_presentation->extent > 0.0);
    CHECK(
        axis_presentation->origin.x == 1.0);
    CHECK(
        axis_presentation->origin.y == 2.0);
    CHECK(
        axis_presentation->origin.z == 0.0);
    CHECK(
        axis_presentation->u_axis.x == 1.0);
    CHECK(
        axis_presentation->u_axis.y == 0.0);
    const auto axis_token =
        axis_presentation->token;

    // Viewport pick resolves through AxisId and drives the same Tree /
    // Properties selection surface.
    viewport->emitSelection(axis_token);
    QApplication::processEvents();
    CHECK(
        tree->currentItem() != nullptr);
    CHECK(
        tree->currentItem()->text(0) ==
        QStringLiteral("Axis001"));
    CHECK(
        properties->currentWidget()->
            objectName() ==
        QStringLiteral("axisPropertiesPage"));
    CHECK(
        axis_identity->text() ==
        QString::fromStdString(
            axis_id.serialized()));
    CHECK(
        axis_source_sketch->text() ==
        QString::fromStdString(
            sketch_id.value()));
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
        axis_origin->text().contains(
            QStringLiteral("1")));
    CHECK(
        axis_direction->text().contains(
            QStringLiteral("1")));

    // Per-Axis Show/Hide changes the one authored visibility truth and
    // removes/restores only the finite presentation.
    CHECK(hide->isEnabled());
    hide->trigger();
    QApplication::processEvents();
    CHECK(
        !session.document()
             .findAxis(axis_id)->visible);
    CHECK(
        authoredAxisPresentation(
            viewport->reference_scene) ==
        nullptr);

    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    CHECK(axis_item != nullptr);
    selectOnly(*tree, axis_item);
    CHECK(show->isEnabled());
    show->trigger();
    QApplication::processEvents();
    CHECK(
        session.document()
            .findAxis(axis_id)->visible);
    CHECK(
        authoredAxisPresentation(
            viewport->reference_scene) !=
        nullptr);

    // Edit uses the shared AxisDraft and current semantic source. Cancel is
    // zero authored mutation.
    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    selectOnly(*tree, axis_item);
    CHECK(axis_edit->isEnabled());
    const auto undo_before_edit_cancel =
        session.undoDepth();
    axis_edit->click();
    QApplication::processEvents();
    CHECK(axis_tool->isChecked());
    CHECK(axis_finish->isEnabled());
    CHECK(
        workbench.cadInputPrompt().find(
            "AXIS") != std::string::npos);
    axis_cancel->click();
    QApplication::processEvents();
    CHECK(!axis_tool->isChecked());
    CHECK(
        session.undoDepth() ==
        undo_before_edit_cancel);
    CHECK(
        session.document().findAxis(axis_id) !=
        nullptr);

    // Command-first owns the same Axis runtime surface and Cancel authors
    // nothing even before a source is acquired.
    auto result =
        workbench.submitCadInput(
            "AXIS",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(axis_tool->isChecked());
    CHECK(
        workbench.cadInputPrompt().find(
            "AXIS") != std::string::npos);
    CHECK(
        workbench.submitCadInput(
            "CANCEL",
            workbench.cadInputContextGeneration())
            .accepted);
    CHECK(!axis_tool->isChecked());
    CHECK(
        session.document().findAxis(axis_id) !=
        nullptr);

    // A missing source Line leaves the Axis authored and repairable. Viewer
    // must not publish the old successful line.
    CHECK(
        session.execute(
            application::EraseSketchEntityCommand{
                sketch_id,
                line_id})
            .ok());
    CHECK(
        workbench.activateDocument(
            &session,
            {}));
    QApplication::processEvents();
    axis_item =
        findItem(
            *tree,
            QStringLiteral(
                "Axis001 [Missing]"));
    CHECK(axis_item != nullptr);
    CHECK(
        authoredAxisPresentation(
            viewport->reference_scene) ==
        nullptr);

    std::cout
        << "PM-04B2 Axis Tree/Viewer/UI tests passed\n";
    return EXIT_SUCCESS;
}
