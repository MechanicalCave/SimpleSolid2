#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

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
            << "SK-07D repeat command CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

class TestViewport final
    : public QWidget,
      public viewer::IDocumentViewport {
public:
    using QWidget::QWidget;

    std::optional<viewer::CameraState> cameraState() const override {
        return camera_;
    }
    bool setCameraState(const viewer::CameraState& state) override {
        camera_ = state;
        return true;
    }
    bool setStandardView(viewer::StandardView) override { return true; }
    bool setProjection(viewer::CameraProjection) override { return true; }
    void fitAll() override {}

    bool setReferenceScene(const viewer::ReferenceScene& scene) override {
        return scene.valid();
    }
    bool setSketchScene(const viewer::SketchScene& scene) override {
        if (!scene.valid()) return false;
        sketch_scene_ = scene;
        return true;
    }
    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        if (!scene.valid()) return false;
        preview_scene_ = scene;
        return true;
    }
    bool setSketchGripScene(
        const viewer::SketchGripScene& scene) override {
        if (!scene.valid()) return false;
        grip_scene_ = scene;
        return true;
    }
    bool setSketchInteractionPresentation(
        const viewer::SketchInteractionPresentation& presentation) override {
        if (!presentation.valid()) return false;
        interaction_presentation_ = presentation;
        return true;
    }
    viewer::SketchGripQueryResult querySketchGrip(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return grip_query_;
    }
    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        if (!selection.valid()) return false;
        selection_ = selection;
        return true;
    }
    viewer::SketchPointQueryResult querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return point_query_;
    }
    viewer::SketchRectangleQueryResult querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule) override {
        if (!rectangle.valid()) return {};
        return rectangle_query_;
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
        viewer::PrimaryPointerRouting routing) override {
        routing_ = routing;
    }
    void setCursorMode(
        viewer::ViewportCursorMode mode) override {
        cursor_mode_ = mode;
    }

    viewer::CameraState camera_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene preview_scene_;
    viewer::SketchGripScene grip_scene_;
    viewer::SketchInteractionPresentation interaction_presentation_;
    viewer::SketchGripQueryResult grip_query_{true, std::nullopt};
    viewer::PresentationSelection selection_;
    viewer::SketchPointQueryResult point_query_{true, std::nullopt};
    viewer::SketchRectangleQueryResult rectangle_query_{true, {}};
    viewer::SelectionIntentHandler selection_handler_;
    viewer::SpatialPointerHandler spatial_handler_;
    viewer::PrimaryPointerRouting routing_{
        viewer::PrimaryPointerRouting::presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::system_default};
};

ui::SketchPointerInput pointer(
    const sketch::SketchId& sketch_id,
    viewer::SpatialPointerPhase phase,
    double sx,
    double sy,
    double u,
    double v) {
    return {
        sketch_id,
        phase,
        {sx, sy},
        {u, v},
        false};
}

void click(
    ui::PartSketchInteractionController& interaction,
    const sketch::SketchId& sketch_id,
    double sx,
    double sy,
    double u,
    double v) {
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        sx, sy, u, v));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_release,
        sx, sy, u, v));
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{"sk07d-repeat.ss2part"},
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok() && created.sketch_id);
    const auto sketch_id = *created.sketch_id;

    const auto line =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 0.0},
                {10.0, 0.0}});
    CHECK(line.ok() && line.entity_id);

    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    TestViewport viewport;
    ui::PartViewportController viewport_controller{
        tree_controller,
        &viewport};
    viewport_controller.setDocumentSession(&session);
    viewport_controller.setSketchEditSketch(sketch_id);

    ui::PartSketchInteractionController interaction{
        viewport_controller};
    interaction.begin(session, sketch_id);

    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(!interaction.lastRepeatableCommand().has_value());
    CHECK(!interaction.repeatLastCommand());

    // Creation commands become repeatable only after successful activation.
    interaction.activateLine();
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::line);
    CHECK(!interaction.repeatLastCommand());
    CHECK(interaction.escape());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::line);
    CHECK(interaction.repeatLastCommand());
    CHECK(interaction.tool() == sketch::SketchTool::line);
    CHECK(interaction.escape());

    interaction.activateCircle();
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::circle);
    CHECK(interaction.escape());
    CHECK(interaction.repeatLastCommand());
    CHECK(interaction.tool() == sketch::SketchTool::circle);
    CHECK(interaction.escape());

    interaction.activateArc();
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::arc);
    CHECK(interaction.escape());

    // SELECT never replaces the remembered repeatable command.
    interaction.activateSelect();
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::arc);

    // Empty-selection transforms repeat through command-first grammar.
    CHECK(interaction.activateMove());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::move);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::select_objects);
    CHECK(interaction.escape());
    CHECK(interaction.repeatLastCommand());
    CHECK(interaction.tool() == sketch::SketchTool::move);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::select_objects);
    CHECK(interaction.escape());

    // Ordinary selection does not replace MOVE, and repeating MOVE now
    // uses the current selection-first entry path.
    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.lines.front().token};
    click(interaction, sketch_id, 20.0, 20.0, 5.0, 0.0);
    CHECK(interaction.selectedCount() == 1U);
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::move);
    CHECK(interaction.repeatLastCommand());
    CHECK(interaction.tool() == sketch::SketchTool::move);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 1U);

    // COPY repeat starts fresh from Base Point; prior accepted Base Point
    // is not remembered by Repeat Last Command.
    CHECK(interaction.activateCopy());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::copy);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    click(interaction, sketch_id, 30.0, 30.0, 2.0, 2.0);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);
    CHECK(interaction.escape());
    CHECK(interaction.repeatLastCommand());
    CHECK(interaction.tool() == sketch::SketchTool::copy);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(interaction.escape());

    CHECK(interaction.activateRotate());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::rotate);
    CHECK(interaction.escape());
    CHECK(interaction.repeatLastCommand());
    CHECK(interaction.tool() == sketch::SketchTool::rotate);
    CHECK(interaction.escape());

    CHECK(interaction.activateScale());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::scale);
    CHECK(interaction.escape());

    CHECK(interaction.activateMirror());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::mirror);
    CHECK(interaction.escape());

    // Grip-started direct manipulation is not a repeatable-command
    // activation and therefore does not replace MIRROR.
    const auto token =
        viewport.sketch_scene_.lines.front().token;
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            token,
            viewer::SketchGripRole::line_center}};
    click(interaction, sketch_id, 20.0, 20.0, 5.0, 0.0);
    CHECK(interaction.directManipulationActive());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::mirror);
    CHECK(!interaction.repeatLastCommand());
    CHECK(interaction.escape());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::mirror);

    // History cancellation does not overwrite the remembered command.
    CHECK(interaction.activateRotate());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::rotate);
    interaction.cancelForHistory();
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::rotate);

    // Delete does not become the repeat target.
    CHECK(interaction.deleteSelection());
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::rotate);

    // The memory is scoped to one Sketch edit runtime session.
    interaction.end();
    CHECK(!interaction.lastRepeatableCommand().has_value());
    interaction.begin(session, sketch_id);
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(!interaction.lastRepeatableCommand().has_value());
    CHECK(!interaction.repeatLastCommand());

    return EXIT_SUCCESS;
}
