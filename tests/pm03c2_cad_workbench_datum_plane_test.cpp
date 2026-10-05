#include "cad_workbench.hpp"

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/datum.hpp>

#include <QAction>
#include <QApplication>
#include <QStackedWidget>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QWidget>

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
            << "PM-03C2 CadWorkbench Datum Plane CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class EmptyKernel final
    : public kernel::ISolidModelingKernel {
public:
    std::size_t extrude_calls{};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        ++extrude_calls;
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
        if (!next) {
            return false;
        }
        camera_ = *next;
        return true;
    }

    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                camera_, projection);
        if (!next) {
            return false;
        }
        camera_ = *next;
        return true;
    }

    void fitAll() override {}

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        if (!scene.valid()) {
            return false;
        }
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
    viewer::ReferenceScene reference_scene;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
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

const part::OffsetDatumPlane& requireDatum(
    const application::DocumentSession& session,
    std::size_t index) {
    CHECK(
        session.document().datumPlanes().size() >
        index);
    return session.document().datumPlanes()[index];
}

core::BuiltinReferenceRole requireOriginSource(
    const part::OffsetDatumPlane& datum) {
    const auto role =
        part::builtinOriginPlaneForPlaneReference(
            datum.source);
    CHECK(role.has_value());
    return *role;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    application::DocumentSession session{
        {},
        part::PartDocument::create(
            core::DocumentId::generate())};

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
    auto* tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "datumPlaneToolButton"));
    auto* constructor =
        workbench.findChild<QComboBox*>(
            QStringLiteral(
                "datumPlaneConstructorCombo"));
    auto* source =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPlaneSourceLabel"));
    auto* offset =
        workbench.findChild<QLineEdit*>(
            QStringLiteral(
                "datumPlaneOffsetEdit"));
    auto* reverse =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "datumPlaneReverseButton"));
    auto* finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "datumPlaneFinishButton"));
    auto* cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "datumPlaneCancelButton"));

    CHECK(
        tree && tool && constructor && source &&
        offset && reverse && finish && cancel);
    CHECK(tool->isEnabled());
    CHECK(!tool->isChecked());

    // Selection-first GUI: the one Datum Plane tool acquires the selected
    // Origin plane into the same runtime draft with the accepted 10 mm
    // default. Reverse changes only the signed value.
    selectOnly(
        *tree,
        findItem(
            *tree,
            QStringLiteral("XY Plane")));

    const auto undo_before_gui =
        session.undoDepth();
    tool->click();
    CHECK(tool->isChecked());
    CHECK(
        constructor->currentText() ==
        QStringLiteral("Offset"));
    CHECK(
        source->text().contains(
            QStringLiteral("XY Plane")));
    CHECK(
        offset->text().contains(
            QStringLiteral("10")));
    CHECK(finish->isEnabled());
    CHECK(
        workbench.cadInputPrompt().find(
            "DATUM PLANE") !=
        std::string::npos);
    CHECK(viewport->reference_scene.preview.has_value());
    CHECK(
        viewport->reference_scene.preview->
            origin.z == 10.0);
    CHECK(
        session.document().datumPlanes().empty());

    reverse->click();
    CHECK(
        offset->text().contains(
            QStringLiteral("-10")));
    CHECK(viewport->reference_scene.preview.has_value());
    CHECK(
        viewport->reference_scene.preview->
            origin.z == -10.0);
    reverse->click();
    CHECK(
        offset->text().contains(
            QStringLiteral("10")));
    CHECK(viewport->reference_scene.preview.has_value());
    CHECK(
        viewport->reference_scene.preview->
            origin.z == 10.0);

    // Invalid transient input clears the visual draft immediately and does
    // not alter the last valid authored candidate. Command Line then restores
    // the same shared draft and its visual preview.
    offset->setText(QStringLiteral("invalid"));
    QApplication::processEvents();
    CHECK(!viewport->reference_scene.preview.has_value());
    auto preview_restore =
        workbench.submitCadInput(
            "10 mm",
            workbench.cadInputContextGeneration());
    CHECK(preview_restore.accepted);
    CHECK(viewport->reference_scene.preview.has_value());
    CHECK(
        viewport->reference_scene.preview->
            origin.z == 10.0);

    finish->click();
    CHECK(
        session.document().datumPlanes().size() ==
        1U);
    CHECK(
        session.undoDepth() ==
        undo_before_gui + 1U);
    CHECK(
        requireOriginSource(
            requireDatum(session, 0U)) ==
        core::BuiltinReferenceRole::xy_plane);
    CHECK(
        requireDatum(
            session,
            0U).offset.millimetres ==
        10.0);
    CHECK(!tool->isChecked());
    CHECK(!viewport->reference_scene.preview.has_value());
    CHECK(kernel.extrude_calls == 0U);

    // Command-first enters the same draft without a source, then ordinary
    // Tree semantic selection supplies XZ. Command Line Offset/Reverse
    // updates the same Operations UI. Cancel authors nothing.
    tree->clearSelection();
    tree->setCurrentItem(nullptr);
    QApplication::processEvents();

    auto result =
        workbench.submitCadInput(
            "DATUMPLANE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(tool->isChecked());
    CHECK(!finish->isEnabled());
    CHECK(!viewport->reference_scene.preview.has_value());
    CHECK(
        workbench.cadInputPrompt().find(
            "Select XY/XZ/YZ") !=
        std::string::npos);

    selectOnly(
        *tree,
        findItem(
            *tree,
            QStringLiteral("XZ Plane")));
    CHECK(
        source->text().contains(
            QStringLiteral("XZ Plane")));
    CHECK(finish->isEnabled());
    CHECK(viewport->reference_scene.preview.has_value());

    result =
        workbench.submitCadInput(
            "-5 mm",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        offset->text().contains(
            QStringLiteral("-5")));

    result =
        workbench.submitCadInput(
            "REVERSE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        offset->text().contains(
            QStringLiteral("5")));

    const auto datum_count_before_cancel =
        session.document().datumPlanes().size();
    const auto undo_before_cancel =
        session.undoDepth();
    result =
        workbench.submitCadInput(
            "CANCEL",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        session.document().datumPlanes().size() ==
        datum_count_before_cancel);
    CHECK(
        session.undoDepth() ==
        undo_before_cancel);
    CHECK(!tool->isChecked());
    CHECK(!viewport->reference_scene.preview.has_value());

    // Global CadInputSession empty Enter is Finish while Datum Plane owns the
    // endpoint, exactly as the GUI Finish button calls the same Finish path.
    tree->clearSelection();
    tree->setCurrentItem(nullptr);
    QApplication::processEvents();

    application::CadInputSession input;
    input.attachEndpoint(&workbench);
    auto input_settings =
        input.interactionSettings();
    input_settings.dynamic_input_enabled = true;
    CHECK(
        input.setInteractionSettings(
            input_settings));
    input.setBuffer("DATUM PLANE");
    CHECK(input.submit().accepted);
    CHECK(!finish->isEnabled());
    const auto datum_fields =
        input.dynamicInputFields();
    CHECK(datum_fields.size() == 1U);
    CHECK(
        datum_fields.front().label ==
        "Offset");

    selectOnly(
        *tree,
        findItem(
            *tree,
            QStringLiteral("YZ Plane")));
    CHECK(finish->isEnabled());

    input.setBuffer("-3 mm");
    CHECK(input.submit().accepted);
    CHECK(
        offset->text().contains(
            QStringLiteral("-3")));

    const auto undo_before_cli_finish =
        session.undoDepth();
    CHECK(input.submit().accepted);
    CHECK(
        session.document().datumPlanes().size() ==
        2U);
    CHECK(
        session.undoDepth() ==
        undo_before_cli_finish + 1U);
    CHECK(
        requireOriginSource(
            requireDatum(session, 1U)) ==
        core::BuiltinReferenceRole::yz_plane);
    CHECK(
        requireDatum(
            session,
            1U).offset.millimetres ==
        -3.0);
    CHECK(!tool->isChecked());
    CHECK(kernel.extrude_calls == 0U);

    // PM-03D2: authored Datums live under Reference Geometry directly
    // below Origin. Tree/Properties selection is DatumId-based, while
    // Show/Hide remains the existing per-Datum authored visibility truth.
    auto* properties_stack =
        workbench.findChild<QStackedWidget*>(
            QStringLiteral(
                "propertiesContextStack"));
    auto* datum_identity =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPropertyIdentity"));
    auto* datum_source_property =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPropertySource"));
    auto* datum_offset_property =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPropertyOffset"));
    auto* datum_visibility_property =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPropertyVisibility"));
    auto* datum_status_property =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPropertyStatus"));
    auto* datum_edit_property =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "editDatumPlanePropertyButton"));
    auto* hide_references =
        workbench.findChild<QAction*>(
            QStringLiteral(
                "hideBuiltinReferencesAction"));
    auto* show_references =
        workbench.findChild<QAction*>(
            QStringLiteral(
                "showBuiltinReferencesAction"));

    CHECK(
        properties_stack &&
        datum_identity &&
        datum_source_property &&
        datum_offset_property &&
        datum_visibility_property &&
        datum_status_property &&
        datum_edit_property &&
        hide_references &&
        show_references);

    auto* root =
        tree->topLevelItem(0);
    CHECK(root != nullptr);
    CHECK(root->childCount() >= 3);
    CHECK(
        root->child(0)->text(0) ==
        QStringLiteral("Origin"));
    CHECK(
        root->child(1)->text(0) ==
        QStringLiteral("Reference Geometry"));
    auto* reference_geometry =
        root->child(1);
    CHECK(reference_geometry->childCount() == 2);

    const auto first_datum_id =
        requireDatum(session, 0U).id;
    auto* datum_one =
        findItem(
            *tree,
            QStringLiteral("Datum Plane 1"));
    CHECK(datum_one != nullptr);
    selectOnly(*tree, datum_one);

    CHECK(
        properties_stack->currentWidget()->
            objectName() ==
        QStringLiteral("datumPropertiesPage"));
    CHECK(
        datum_identity->text() ==
        QString::fromStdString(
            first_datum_id.serialized()));
    CHECK(
        datum_source_property->text() ==
        QStringLiteral("XY Plane"));
    CHECK(
        datum_offset_property->text().contains(
            QStringLiteral("10")));
    CHECK(
        datum_visibility_property->text() ==
        QStringLiteral("Shown"));
    CHECK(
        datum_status_property->text() ==
        QStringLiteral("Resolved"));
    CHECK(datum_edit_property->isEnabled());

    // Per-Datum Tree visibility uses one existing authored visibility field
    // and one command transaction.
    const auto undo_before_hide_one =
        session.undoDepth();
    CHECK(hide_references->isEnabled());
    hide_references->trigger();
    QApplication::processEvents();
    CHECK(
        !session.document()
             .findDatumPlane(first_datum_id)->
             visible);
    CHECK(
        session.undoDepth() ==
        undo_before_hide_one + 1U);
    datum_one =
        findItem(
            *tree,
            QStringLiteral("Datum Plane 1"));
    CHECK(datum_one != nullptr);
    CHECK(datum_one->font(0).italic());
    selectOnly(*tree, datum_one);
    CHECK(
        datum_visibility_property->text() ==
        QStringLiteral("Hidden"));

    CHECK(show_references->isEnabled());
    show_references->trigger();
    QApplication::processEvents();
    CHECK(
        session.document()
            .findDatumPlane(first_datum_id)->
            visible);
    CHECK(
        session.undoDepth() ==
        undo_before_hide_one + 2U);

    // An earlier Datum Plane is a semantic source for the same create draft.
    datum_one =
        findItem(
            *tree,
            QStringLiteral("Datum Plane 1"));
    selectOnly(*tree, datum_one);
    tool->click();
    CHECK(tool->isChecked());
    CHECK(finish->isEnabled());
    CHECK(
        source->text().contains(
            QString::fromStdString(
                first_datum_id.serialized())));

    const auto undo_before_datum_source =
        session.undoDepth();
    finish->click();
    CHECK(
        session.document().datumPlanes().size() ==
        3U);
    CHECK(
        session.undoDepth() ==
        undo_before_datum_source + 1U);

    const auto third_datum_id =
        requireDatum(session, 2U).id;
    const auto third_source =
        part::datumPlaneIdForPlaneReference(
            requireDatum(session, 2U).source);
    CHECK(third_source.has_value());
    CHECK(*third_source == first_datum_id);

    // Edit reuses the shared Datum draft and preserves durable DatumId.
    auto* datum_three =
        findItem(
            *tree,
            QStringLiteral("Datum Plane 3"));
    CHECK(datum_three != nullptr);
    selectOnly(*tree, datum_three);
    CHECK(
        datum_identity->text() ==
        QString::fromStdString(
            third_datum_id.serialized()));
    CHECK(
        datum_source_property->text().contains(
            QString::fromStdString(
                first_datum_id.serialized())));

    datum_edit_property->click();
    CHECK(tool->isChecked());
    CHECK(
        source->text().contains(
            QString::fromStdString(
                first_datum_id.serialized())));
    offset->setText(
        QStringLiteral("-4 mm"));
    QApplication::processEvents();
    CHECK(finish->isEnabled());

    const auto datum_count_before_edit =
        session.document().datumPlanes().size();
    const auto undo_before_edit =
        session.undoDepth();
    finish->click();
    CHECK(
        session.document().datumPlanes().size() ==
        datum_count_before_edit);
    CHECK(
        session.undoDepth() ==
        undo_before_edit + 1U);
    const auto* edited =
        session.document()
            .findDatumPlane(third_datum_id);
    CHECK(edited != nullptr);
    CHECK(edited->id == third_datum_id);
    CHECK(edited->offset.millimetres == -4.0);

    // Group visibility is a bulk command over authored Datum visibility.
    // The group itself owns no persisted visibility flag.
    root = tree->topLevelItem(0);
    CHECK(root != nullptr);
    CHECK(
        root->child(0)->text(0) ==
        QStringLiteral("Origin"));
    CHECK(
        root->child(1)->text(0) ==
        QStringLiteral("Reference Geometry"));
    reference_geometry = root->child(1);
    selectOnly(*tree, reference_geometry);

    const auto undo_before_hide_group =
        session.undoDepth();
    CHECK(hide_references->isEnabled());
    hide_references->trigger();
    QApplication::processEvents();
    CHECK(
        session.undoDepth() ==
        undo_before_hide_group + 1U);
    for (const auto& datum :
         session.document().datumPlanes()) {
        CHECK(!datum.visible);
    }

    reference_geometry =
        findItem(
            *tree,
            QStringLiteral("Reference Geometry"));
    CHECK(reference_geometry != nullptr);
    selectOnly(*tree, reference_geometry);
    CHECK(show_references->isEnabled());
    show_references->trigger();
    QApplication::processEvents();
    CHECK(
        session.undoDepth() ==
        undo_before_hide_group + 2U);
    for (const auto& datum :
         session.document().datumPlanes()) {
        CHECK(datum.visible);
    }

    // PM-03E: the same semantic Datum selection is an ordinary Sketch
    // support candidate. No Viewer token or derived Datum frame is authored.
    auto* sketch_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchToolButton"));
    auto* finish_sketch =
        workbench.findChild<QPushButton*>(
            QStringLiteral("finishSketchButton"));
    CHECK(sketch_tool && finish_sketch);

    const auto sketch_count_before =
        session.document().sketches().size();
    const auto undo_before_datum_sketch =
        session.undoDepth();

    sketch_tool->click();
    QApplication::processEvents();

    datum_one =
        findItem(
            *tree,
            QStringLiteral("Datum Plane 1"));
    CHECK(datum_one != nullptr);
    selectOnly(*tree, datum_one);

    CHECK(!finish_sketch->isHidden());
    CHECK(finish_sketch->isEnabled());
    CHECK(
        finish_sketch->text() ==
        QStringLiteral("Create Sketch"));

    finish_sketch->click();
    QApplication::processEvents();

    CHECK(
        session.document().sketches().size() ==
        sketch_count_before + 1U);
    CHECK(
        session.undoDepth() ==
        undo_before_datum_sketch + 1U);

    const auto& datum_sketch =
        session.document().sketches().back();
    const auto datum_support =
        part::datumPlaneIdForSketchSupport(
            datum_sketch.support);
    CHECK(datum_support.has_value());
    CHECK(*datum_support == first_datum_id);

    CHECK(
        finish_sketch->text() ==
        QStringLiteral("Finish Sketch"));
    finish_sketch->click();
    QApplication::processEvents();

    std::cout
        << "PM-03C2 / PM-03D2 / PM-03E CadWorkbench Datum Plane parity PASS\n";
    return EXIT_SUCCESS;
}
