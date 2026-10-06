#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/sketch/entity_role.hpp>

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QWidget>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04B2 Axis Tree/Viewer CHECK failed at line "
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
            kernel::SolidModelingStatus::provider_failure;
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
        sketch_scene = scene;
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
        return point.valid()
            ? point_query
            : viewer::SketchPointQueryResult{};
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

    void emitSpatialPointer(
        viewer::SpatialPointerPhase phase,
        double screen_x,
        double screen_y,
        double u,
        double v) {
        CHECK(static_cast<bool>(spatial_handler));
        spatial_handler(
            viewer::SpatialPointerEvent{
                phase,
                {screen_x, screen_y},
                {
                    {u, v, 10.0},
                    {0.0, 0.0, -1.0},
                },
                {}});
        QApplication::processEvents();
    }

    viewer::CameraState camera_;
    viewer::ReferenceScene reference_scene;
    viewer::SketchScene sketch_scene;
    viewer::SketchPointQueryResult
        point_query{true, std::nullopt};
    viewer::PresentationSelection
        presentation_selection;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
};

QTreeWidgetItem* findItem(
    QTreeWidget& tree,
    const QString& text,
    Qt::MatchFlag match = Qt::MatchExactly) {
    const auto matches =
        tree.findItems(
            text,
            match | Qt::MatchRecursive,
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
currentAxisPresentation(
    const TestViewport& viewport) {
    const auto found =
        std::find_if(
            viewport.reference_scene.references.begin(),
            viewport.reference_scene.references.end(),
            [](const auto& reference) {
                return reference.kind ==
                    viewer::ReferencePresentationKind::
                        axis;
            });
    return found ==
            viewport.reference_scene.references.end()
        ? std::nullopt
        : std::optional<
              viewer::ReferencePresentation>{*found};
}

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId sketch_id;
    sketch::EntityId line_id;
    part::AxisId axis_id;

    Fixture(
        application::DocumentSession session_value,
        sketch::SketchId sketch_id_value,
        sketch::EntityId line_id_value,
        part::AxisId axis_id_value)
        : session{std::move(session_value)},
          sketch_id{std::move(sketch_id_value)},
          line_id{line_id_value},
          axis_id{axis_id_value} {}
};

Fixture makeFixture() {
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
            false,
            std::move(model)});

    const auto axis_id =
        state.next_axis_id.allocate();
    CHECK(axis_id.has_value());
    state.axes.push_back(
        part::PartAxis{
            *axis_id,
            "Axis001",
            {sketch_id, line_id},
            true});

    part::PartDocumentTransaction tx{document};
    tx.replaceState(std::move(state));
    CHECK(tx.commit().ok());

    return Fixture{
        application::DocumentSession{
            std::filesystem::path{
                "AxisTreeViewer.ss2part"},
            std::move(document)},
        sketch_id,
        line_id,
        *axis_id};
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto fixture = makeFixture();
    auto& session = fixture.session;
    EmptyKernel kernel;
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
    auto* properties_stack =
        workbench.findChild<QStackedWidget*>(
            QStringLiteral(
                "propertiesContextStack"));
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
    auto* axis_diagnostic =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyDiagnostic"));
    auto* axis_origin =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyOrigin"));
    auto* axis_direction =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "axisPropertyDirection"));
    auto* axis_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral("axisToolButton"));
    auto* axis_operations =
        workbench.findChild<QWidget*>(
            QStringLiteral(
                "axisOperationsWidget"));
    auto* hide =
        workbench.findChild<QAction*>(
            QStringLiteral(
                "hideBuiltinReferencesAction"));
    auto* show =
        workbench.findChild<QAction*>(
            QStringLiteral(
                "showBuiltinReferencesAction"));

    CHECK(
        tree && properties_stack &&
        axis_identity && axis_source_sketch &&
        axis_source_line && axis_visibility &&
        axis_status && axis_diagnostic &&
        axis_origin && axis_direction &&
        axis_operations &&
        hide && show);
    CHECK(axis_tool == nullptr);

    // Authored Axis is a Part-owned child of its source Sketch even when the
    // source Sketch itself is hidden.
    auto* axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    CHECK(axis_item != nullptr);
    CHECK(axis_item->parent() != nullptr);
    CHECK(
        axis_item->parent()->text(0) ==
        QStringLiteral("Sketch 1"));

    const auto presented =
        currentAxisPresentation(*viewport);
    CHECK(presented.has_value());
    CHECK(presented->token.valid());
    CHECK(
        presented->kind ==
        viewer::ReferencePresentationKind::axis);
    CHECK(presented->origin.x == 0.0);
    CHECK(presented->origin.y == 0.0);
    CHECK(presented->origin.z == 0.0);
    CHECK(presented->u_axis.x > 0.999999);
    CHECK(presented->extent > 0.0);

    // Tree selection resolves through AxisId into Viewport selection and the
    // dedicated Properties page. Source EntityId never becomes pick truth.
    selectOnly(*tree, axis_item);
    CHECK(
        properties_stack->currentWidget()
            ->objectName() ==
        QStringLiteral("axisPropertiesPage"));
    CHECK(
        axis_identity->text() ==
        QString::fromStdString(
            fixture.axis_id.serialized()));
    CHECK(
        axis_source_sketch->text() ==
        QString::fromStdString(
            std::string{
                fixture.sketch_id.value()}));
    CHECK(
        axis_source_line->text() ==
        QString::fromStdString(
            fixture.line_id.serialized()));
    CHECK(
        axis_visibility->text() ==
        QStringLiteral("Shown"));
    CHECK(
        axis_status->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        axis_diagnostic->text() ==
        QStringLiteral("—"));
    CHECK(
        axis_origin->text() !=
        QStringLiteral("—"));
    CHECK(
        axis_direction->text() !=
        QStringLiteral("—"));
    CHECK(
        viewport->presentation_selection
            .primary ==
        presented->token);
    CHECK(
        std::find(
            viewport->presentation_selection
                .selected.begin(),
            viewport->presentation_selection
                .selected.end(),
            presented->token) !=
        viewport->presentation_selection
            .selected.end());

    // Viewer pick maps back to the same AxisId and reselects the same Tree /
    // Properties object.
    tree->clearSelection();
    tree->setCurrentItem(nullptr);
    QApplication::processEvents();
    viewport->emitSelectionIntent(
        viewer::SelectionIntent{
            presented->token,
            viewer::SelectionIntentMode::replace});
    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    CHECK(axis_item != nullptr);
    CHECK(axis_item->isSelected());
    CHECK(
        axis_identity->text() ==
        QString::fromStdString(
            fixture.axis_id.serialized()));

    // Hide changes only authored Axis presentation state. Evaluation remains
    // valid while the finite Viewer cue disappears.
    const auto undo_before_hide =
        session.undoDepth();
    CHECK(hide->isEnabled());
    hide->trigger();
    QApplication::processEvents();
    CHECK(
        session.document()
            .findAxis(fixture.axis_id) != nullptr);
    CHECK(
        !session.document()
             .findAxis(fixture.axis_id)
             ->visible);
    CHECK(
        session.undoDepth() ==
        undo_before_hide + 1U);
    CHECK(!currentAxisPresentation(*viewport));
    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"));
    CHECK(axis_item != nullptr);
    selectOnly(*tree, axis_item);
    CHECK(
        axis_status->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        axis_visibility->text() ==
        QStringLiteral("Hidden"));
    CHECK(show->isEnabled());

    workbench.requestUndo();
    QApplication::processEvents();
    CHECK(
        session.document()
            .findAxis(fixture.axis_id)
            ->visible);
    CHECK(currentAxisPresentation(*viewport));

    // Command-first activates the same shared Axis draft and Operations panel
    // without a standalone GUI Axis authoring button. Cancel authors nothing.
    auto* sketch_item =
        findItem(
            *tree,
            QStringLiteral("Sketch 1"));
    CHECK(sketch_item != nullptr);
    selectOnly(*tree, sketch_item);
    const auto axis_count_before_command =
        session.document().axes().size();
    const auto command_generation =
        workbench.cadInputContextGeneration();
    auto command =
        workbench.submitCadInput(
            "AXIS",
            command_generation);
    CHECK(command.accepted);
    CHECK(!axis_operations->isHidden());
    CHECK(
        workbench.cadInputPrompt().find(
            "AXIS") !=
        std::string::npos);
    CHECK(
        workbench.cadDynamicInputFields()
            .empty());
    CHECK(
        session.document().axes().size() ==
        axis_count_before_command);

    command =
        workbench.submitCadInput(
            "CANCEL",
            workbench.cadInputContextGeneration());
    CHECK(command.accepted);
    CHECK(
        session.document().axes().size() ==
        axis_count_before_command);

    // Normal GUI creation is contextual to Line. Arming Axis is runtime-only
    // until a Line successfully commits, and there is no standalone Axis
    // toolbar action.
    auto* line_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral("lineSketchToolButton"));
    auto* axis_designation =
        workbench.findChild<QCheckBox*>(
            QStringLiteral(
                "sketchAxisDesignationCheck"));
    CHECK(line_tool != nullptr);
    CHECK(axis_designation != nullptr);
    CHECK(axis_tool == nullptr);
    CHECK(!line_tool->isHidden());
    line_tool->click();
    QApplication::processEvents();
    CHECK(!axis_designation->isHidden());
    CHECK(axis_designation->isEnabled());
    CHECK(!axis_designation->isChecked());
    const auto axis_count_before_arming =
        session.document().axes().size();
    const auto undo_before_arming =
        session.undoDepth();
    axis_designation->click();
    QApplication::processEvents();
    CHECK(axis_designation->isChecked());
    CHECK(
        session.document().axes().size() ==
        axis_count_before_arming);
    CHECK(
        session.undoDepth() ==
        undo_before_arming);
    axis_designation->click();
    QApplication::processEvents();
    CHECK(!axis_designation->isChecked());

    // Existing selected Line projects the authored Part designation. ON->OFF
    // deletes exactly one Axis while preserving the Line; OFF->ON allocates a
    // fresh AxisId. Undo restores the original AxisId without coupling the
    // independent Regular/Construction role.
    auto* select_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "selectSketchToolButton"));
    auto* regular_role =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "sketchRegularRoleButton"));
    auto* construction_role =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "sketchConstructionRoleButton"));
    CHECK(select_tool != nullptr);
    CHECK(regular_role != nullptr);
    CHECK(construction_role != nullptr);
    select_tool->click();
    QApplication::processEvents();
    CHECK(viewport->sketch_scene.lines.size() == 1U);

    viewport->point_query = {
        true,
        viewport->sketch_scene.lines.front().token};
    viewport->emitSpatialPointer(
        viewer::SpatialPointerPhase::primary_press,
        50.0, 20.0,
        5.0, 0.0);
    viewport->emitSpatialPointer(
        viewer::SpatialPointerPhase::primary_release,
        50.0, 20.0,
        5.0, 0.0);
    CHECK(!axis_designation->isHidden());
    CHECK(axis_designation->isEnabled());
    CHECK(axis_designation->isChecked());

    const auto old_axis_id =
        fixture.axis_id;
    const auto delete_undo_before =
        session.undoDepth();
    axis_designation->click();
    QApplication::processEvents();
    CHECK(
        session.document().findAxis(
            old_axis_id) == nullptr);
    CHECK(
        session.document().findSketch(
            fixture.sketch_id)
            ->model.findLine(
                fixture.line_id) != nullptr);
    CHECK(
        session.undoDepth() ==
        delete_undo_before + 1U);

    axis_designation->click();
    QApplication::processEvents();
    CHECK(session.document().axes().size() == 1U);
    const auto new_axis_id =
        session.document().axes().front().id;
    CHECK(new_axis_id != old_axis_id);
    CHECK(
        session.document().axes().front().source ==
        part::SketchLineAxisSource{
            fixture.sketch_id,
            fixture.line_id});

    workbench.requestUndo();
    QApplication::processEvents();
    CHECK(session.document().axes().empty());
    workbench.requestUndo();
    QApplication::processEvents();
    CHECK(
        session.document().findAxis(
            old_axis_id) != nullptr);
    CHECK(
        session.document().findAxis(
            old_axis_id)
            ->id == old_axis_id);

    // Re-select after history reconciliation, then prove geometry role is
    // independent from the Axis designation.
    select_tool->click();
    QApplication::processEvents();
    viewport->point_query = {
        true,
        viewport->sketch_scene.lines.front().token};
    viewport->emitSpatialPointer(
        viewer::SpatialPointerPhase::primary_press,
        50.0, 20.0,
        5.0, 0.0);
    viewport->emitSpatialPointer(
        viewer::SpatialPointerPhase::primary_release,
        50.0, 20.0,
        5.0, 0.0);
    CHECK(axis_designation->isChecked());
    construction_role->click();
    QApplication::processEvents();
    CHECK(
        session.document().findAxis(
            old_axis_id) != nullptr);
    CHECK(
        session.document().findSketch(
            fixture.sketch_id)
            ->model.findLine(
                fixture.line_id)
            ->role() ==
        sketch::EntityRole::construction);
    regular_role->click();
    QApplication::processEvents();
    CHECK(
        session.document().findAxis(
            old_axis_id) != nullptr);

    // Legacy duplicate-source state remains representable but selected-Line
    // designation fails closed as indeterminate/disabled rather than choosing
    // an arbitrary AxisId owner.
    auto duplicate_state =
        session.document().state();
    const auto duplicate_axis_id =
        duplicate_state.next_axis_id.allocate();
    CHECK(duplicate_axis_id.has_value());
    duplicate_state.axes.push_back(
        part::PartAxis{
            *duplicate_axis_id,
            "Axis Legacy Duplicate",
            {
                fixture.sketch_id,
                fixture.line_id,
            },
            true});
    auto duplicate_document =
        part::PartDocument::restore(
            core::DocumentId::generate(),
            std::move(duplicate_state));
    CHECK(duplicate_document.ok());
    application::DocumentSession
        duplicate_session{
            {},
            std::move(
                *duplicate_document.document)};

    TestViewport* duplicate_viewport = nullptr;
    ui::CadWorkbench duplicate_workbench{
        [&duplicate_viewport](QWidget* parent) {
            auto* created =
                new TestViewport(parent);
            duplicate_viewport = created;
            return ui::ViewportSurface{
                created,
                created};
        },
        &kernel};
    CHECK(duplicate_viewport != nullptr);
    CHECK(
        duplicate_workbench.activateDocument(
            &duplicate_session,
            {}));
    duplicate_workbench.requestEditSketch(
        fixture.sketch_id);
    QApplication::processEvents();
    CHECK(
        duplicate_viewport->sketch_scene.lines.size() ==
        1U);
    duplicate_viewport->point_query = {
        true,
        duplicate_viewport->sketch_scene
            .lines.front().token};
    duplicate_viewport->emitSpatialPointer(
        viewer::SpatialPointerPhase::primary_press,
        50.0, 20.0,
        5.0, 0.0);
    duplicate_viewport->emitSpatialPointer(
        viewer::SpatialPointerPhase::primary_release,
        50.0, 20.0,
        5.0, 0.0);

    auto* duplicate_designation =
        duplicate_workbench.findChild<QCheckBox*>(
            QStringLiteral(
                "sketchAxisDesignationCheck"));
    CHECK(duplicate_designation != nullptr);
    CHECK(!duplicate_designation->isHidden());
    CHECK(duplicate_designation->isTristate());
    CHECK(
        duplicate_designation->checkState() ==
        Qt::PartiallyChecked);
    CHECK(!duplicate_designation->isEnabled());
    CHECK(
        duplicate_session.document().axes().size() ==
        2U);

    // Axis command opened source Sketch edit for semantic Line acquisition.
    // Finish that runtime context before the destructive repairability check.
    auto* finish_sketch =
        workbench.findChild<QPushButton*>(
            QStringLiteral("finishSketchButton"));
    CHECK(finish_sketch != nullptr);
    if (!finish_sketch->isHidden()) {
        finish_sketch->click();
        QApplication::processEvents();
    }

    // Source deletion keeps durable Axis intent as Missing and removes any
    // stale successful finite line from the Viewer.
    const auto erased =
        session.execute(
            application::EraseSketchEntityCommand{
                fixture.sketch_id,
                fixture.line_id});
    CHECK(erased.ok());
    CHECK(erased.changed);
    CHECK(
        workbench.activateDocument(
            &session,
            {}));

    axis_item =
        findItem(
            *tree,
            QStringLiteral("Axis001"),
            Qt::MatchStartsWith);
    CHECK(axis_item != nullptr);
    CHECK(
        axis_item->text(0).contains(
            QStringLiteral("Missing")));
    selectOnly(*tree, axis_item);
    CHECK(
        axis_status->text() ==
        QStringLiteral("Missing"));
    CHECK(
        axis_diagnostic->text().contains(
            QStringLiteral("Missing source Line")));
    CHECK(
        axis_origin->text() ==
        QStringLiteral("—"));
    CHECK(
        axis_direction->text() ==
        QStringLiteral("—"));
    CHECK(!currentAxisPresentation(*viewport));

    std::cout
        << "PM-04B2/PM-04F Axis contextual Tree/Viewer/Properties tests passed\n";
    return EXIT_SUCCESS;
}
