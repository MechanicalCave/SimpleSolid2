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
        std::cerr << "SK-04C interaction CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

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
        viewer::SketchRectangleSelectionRule rule) override {
        if (!rectangle.valid()) return {};
        last_rectangle_rule_ = rule;
        return rectangle_query_;
    }

    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override {
        if (!overlay.valid()) return false;
        overlay_ = overlay;
        return true;
    }
    void clearSketchSelectionBoxOverlay() override {
        overlay_.reset();
    }

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
    viewer::PresentationSelection selection_;
    viewer::SketchPointQueryResult point_query_{true, std::nullopt};
    viewer::SketchRectangleQueryResult rectangle_query_{true, {}};
    std::optional<viewer::SketchRectangleSelectionRule>
        last_rectangle_rule_;
    std::optional<viewer::SketchSelectionBoxOverlay> overlay_;
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
    double v,
    bool control = false) {
    return {
        sketch_id,
        phase,
        {sx, sy},
        {u, v},
        control};
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{"sk04c-interaction.ss2part"},
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok() && created.sketch_id.has_value());
    const auto sketch_id = *created.sketch_id;
    const auto baseline_undo = session.undoDepth();

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

    CHECK(interaction.active());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::spatial_tool_input);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::select_pick_box);

    interaction.activateLine();
    CHECK(interaction.tool() == sketch::SketchTool::line);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::create_edit_crosshair);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        20.0, 20.0,
        0.0, 0.0));
    CHECK(
        interaction.lineStage() ==
        sketch::LineStage::await_next_point);
    CHECK(session.undoDepth() == baseline_undo);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        80.0, 20.0,
        10.0, 0.0));
    CHECK(viewport.preview_scene_.lines.size() == 1U);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        80.0, 20.0,
        10.0, 0.0));
    CHECK(
        session.document()
            .findSketch(sketch_id)->model.entityCount() == 1U);
    CHECK(session.undoDepth() == baseline_undo + 1U);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        80.0, 80.0,
        10.0, 10.0));
    CHECK(
        session.document()
            .findSketch(sketch_id)->model.entityCount() == 2U);
    CHECK(session.undoDepth() == baseline_undo + 2U);

    interaction.finishLine();
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    const auto first_token =
        viewport.sketch_scene_.lines[0].token;
    viewport.point_query_ = {true, first_token};

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        50.0, 20.0,
        5.0, 0.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_release,
        50.0, 20.0,
        5.0, 0.0));

    CHECK(interaction.selectedCount() == 1U);
    CHECK(viewport.selection_.primary.has_value());

    CHECK(interaction.deleteSelection());
    CHECK(
        session.document()
            .findSketch(sketch_id)->model.entityCount() == 1U);
    CHECK(session.undoDepth() == baseline_undo + 3U);
    CHECK(interaction.selectedCount() == 0U);

    interaction.cancelForHistory();
    CHECK(session.undo().changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(
        session.document()
            .findSketch(sketch_id)->model.entityCount() == 2U);

    CHECK(viewport.sketch_scene_.lines.size() == 2U);
    const auto rect_first =
        viewport.sketch_scene_.lines[0].token;
    const auto rect_second =
        viewport.sketch_scene_.lines[1].token;
    viewport.rectangle_query_ = {
        true,
        {rect_first, rect_second}};

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        200.0, 20.0,
        20.0, 0.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        100.0, 100.0,
        10.0, 10.0));
    CHECK(viewport.overlay_.has_value());
    CHECK(
        viewport.overlay_->rule ==
        viewer::SketchRectangleSelectionRule::crossing);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_release,
        100.0, 100.0,
        10.0, 10.0));
    CHECK(!viewport.overlay_.has_value());
    CHECK(interaction.selectedCount() == 2U);
    CHECK(!viewport.selection_.primary.has_value());
    CHECK(
        viewport.last_rectangle_rule_ ==
        viewer::SketchRectangleSelectionRule::crossing);

    viewport.point_query_ = {true, rect_first};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        60.0, 20.0,
        6.0, 0.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_release,
        60.0, 20.0,
        6.0, 0.0,
        true));
    CHECK(interaction.selectedCount() == 1U);

    interaction.activateLine();
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        10.0, 10.0,
        2.0, 2.0));
    const auto before_zero = session.undoDepth();
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        10.0, 10.0,
        2.0, 2.0));
    CHECK(session.undoDepth() == before_zero);

    CHECK(interaction.escape());
    CHECK(
        interaction.lineStage() ==
        sketch::LineStage::await_first_point);
    CHECK(interaction.escape());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(!interaction.escape());

    interaction.end();
    CHECK(!interaction.active());

    return EXIT_SUCCESS;
}
