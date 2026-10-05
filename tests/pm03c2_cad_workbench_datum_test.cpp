#include "cad_workbench.hpp"

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/datum.hpp>

#include <QApplication>
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
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-03C2 workbench Datum CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class FakeKernel final
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
                camera_,
                view);
        if (!next) return false;
        camera_ = *next;
        return true;
    }

    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                camera_,
                projection);
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
        selection_handler_ = std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_handler_ = std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting) override {}

    void setCursorMode(
        viewer::ViewportCursorMode) override {}

private:
    viewer::CameraState camera_;
    viewer::SelectionIntentHandler selection_handler_;
    viewer::SpatialPointerHandler spatial_handler_;
};

QTreeWidgetItem* findTreeItem(
    QTreeWidget& tree,
    const QString& label) {
    const auto matches =
        tree.findItems(
            label,
            Qt::MatchExactly |
                Qt::MatchRecursive,
            0);
    return matches.empty()
        ? nullptr
        : matches.front();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    application::DocumentSession session{
        {},
        part::PartDocument::create(
            core::DocumentId::generate())};
    FakeKernel kernel;

    ui::CadWorkbench workbench{
        [](QWidget* parent) {
            auto* viewport =
                new TestViewport(parent);
            return ui::ViewportSurface{
                viewport,
                viewport};
        },
        &kernel};

    CHECK(
        workbench.activateDocument(
            &session,
            {}));

    auto* tree =
        workbench.findChild<QTreeWidget*>();
    auto* datum_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral(
                "datumPlaneToolButton"));
    auto* datum_panel =
        workbench.findChild<QWidget*>(
            QStringLiteral(
                "datumPlaneOperationsWidget"));
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
        tree && datum_button && datum_panel &&
        constructor && source && offset &&
        reverse && finish && cancel);
    CHECK(constructor->count() == 1);
    CHECK(
        constructor->currentText() ==
        QStringLiteral("Offset"));
    CHECK(datum_panel->isHidden());

    const auto undo_before =
        session.undoDepth();

    // Command-first entry owns the same workbench draft as the button.
    auto result =
        workbench.submitCadInput(
            "DATUM PLANE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(datum_button->isChecked());
    CHECK(!datum_panel->isHidden());
    CHECK(
        workbench.cadInputPrompt().find(
            "DATUM PLANE") !=
        std::string::npos);
    CHECK(offset->text().contains(
        QStringLiteral("10")));
    CHECK(!finish->isEnabled());
    CHECK(
        session.document().datumPlanes().empty());
    CHECK(session.undoDepth() == undo_before);

    // Origin selection supplies the semantic source to the already-active
    // draft; no authored mutation happens before Finish.
    auto* xy =
        findTreeItem(
            *tree,
            QStringLiteral("XY Plane"));
    CHECK(xy != nullptr);
    tree->clearSelection();
    xy->setSelected(true);
    tree->setCurrentItem(xy);
    CHECK(
        source->text().contains(
            QStringLiteral("XY Plane")));
    CHECK(finish->isEnabled());
    CHECK(
        session.document().datumPlanes().empty());

    // Command Line signed Offset updates the same draft.
    result =
        workbench.submitCadInput(
            "OFFSET -5 mm",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        offset->text().contains(
            QStringLiteral("-5")));
    CHECK(finish->isEnabled());

    // Dynamic Input exposes the same authored signed Offset field.
    const auto fields =
        workbench.cadDynamicInputFields();
    CHECK(fields.size() == 1U);
    CHECK(fields.front().label == "Offset");

    result =
        workbench.lockCadDynamicInputField(
            0U,
            "-7 mm",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        offset->text().contains(
            QStringLiteral("-7")));
    CHECK(finish->isEnabled());

    // Reverse is only a sign flip; there is no second authored direction bit.
    result =
        workbench.submitCadInput(
            "REVERSE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(
        offset->text().contains(
            QStringLiteral("7")));

    // Empty Enter through the global CAD input session finishes the current
    // Datum Plane because the workbench advertises empty input in this context.
    application::CadInputSession input;
    input.attachEndpoint(&workbench);
    CHECK(input.submit().accepted);

    CHECK(
        session.document().datumPlanes().size() ==
        1U);
    CHECK(
        session.undoDepth() ==
        undo_before + 1U);
    CHECK(!datum_panel->isVisible());
    CHECK(!datum_button->isChecked());

    const auto& authored =
        session.document().datumPlanes().front();
    CHECK(
        authored.offset.millimetres ==
        7.0);
    const auto origin =
        part::builtinOriginPlaneForPlaneReference(
            authored.source);
    CHECK(origin.has_value());
    CHECK(
        *origin ==
        core::BuiltinReferenceRole::xy_plane);

    // A second command sees the existing XY selection and acquires it
    // selection-first, but CANCEL still produces zero authored/history change.
    const auto state_before_cancel =
        session.document().state();
    const auto undo_before_cancel =
        session.undoDepth();

    result =
        workbench.submitCadInput(
            "DATUMPLANE",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!datum_panel->isHidden());
    CHECK(
        source->text().contains(
            QStringLiteral("XY Plane")));
    CHECK(finish->isEnabled());

    result =
        workbench.submitCadInput(
            "CANCEL",
            workbench.cadInputContextGeneration());
    CHECK(result.accepted);
    CHECK(!datum_panel->isVisible());
    CHECK(
        session.document().state() ==
        state_before_cancel);
    CHECK(
        session.undoDepth() ==
        undo_before_cancel);

    std::cout
        << "PM-03C2 CadWorkbench Datum Plane tests passed\n";
    return EXIT_SUCCESS;
}
