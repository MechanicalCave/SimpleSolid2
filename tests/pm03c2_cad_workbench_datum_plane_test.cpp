#include "cad_workbench.hpp"

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/datum.hpp>

#include <QApplication>
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
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "datumPlaneConstructorLabel"));
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
        constructor->text() ==
        QStringLiteral("Constructor: Offset"));
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

    reverse->click();
    CHECK(
        offset->text().contains(
            QStringLiteral("-10")));
    reverse->click();
    CHECK(
        offset->text().contains(
            QStringLiteral("10")));

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

    // Global CadInputSession empty Enter is Finish while Datum Plane owns the
    // endpoint, exactly as the GUI Finish button calls the same Finish path.
    tree->clearSelection();
    tree->setCurrentItem(nullptr);
    QApplication::processEvents();

    application::CadInputSession input;
    input.attachEndpoint(&workbench);
    input.setBuffer("DATUM PLANE");
    CHECK(input.submit().accepted);
    CHECK(!finish->isEnabled());

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

    std::cout
        << "PM-03C2 CadWorkbench Datum Plane parity PASS\n";
    return EXIT_SUCCESS;
}
