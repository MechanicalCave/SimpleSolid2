#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>

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
            << "SK-07A MOVE controller CHECK failed at line "
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
    viewer::SketchInteractionPresentation
        interaction_presentation_;
    viewer::SketchGripQueryResult grip_query_{
        true,
        std::nullopt};
    viewer::PresentationSelection selection_;
    viewer::SketchPointQueryResult point_query_{
        true,
        std::nullopt};
    viewer::SketchRectangleQueryResult rectangle_query_{
        true,
        {}};
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

void click(
    ui::PartSketchInteractionController& interaction,
    const sketch::SketchId& sketch_id,
    double sx,
    double sy,
    double u,
    double v,
    bool control = false) {
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        sx, sy, u, v, control));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_release,
        sx, sy, u, v, control));
}

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        const auto id =
            core::DocumentId::generate();
        path =
            std::filesystem::temp_directory_path() /
            ("ss2-sk07a-move-" +
             std::string{id.value()});
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    part::PartDocumentStore store;
    const auto path =
        temp.path / "sk07a-move.ss2part";

    auto document =
        part::PartDocument::create(core::DocumentId::generate());
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{
        path,
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
    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {20.0, 10.0},
                5.0});
    const auto arc =
        session.execute(
            application::AddSketchArcCommand{
                sketch_id,
                {-10.0, 5.0},
                4.0,
                0.0,
                1.0});
    CHECK(line.ok() && line.entity_id);
    CHECK(circle.ok() && circle.entity_id);
    CHECK(arc.ok() && arc.entity_id);
    const auto line_id = *line.entity_id;
    const auto circle_id = *circle.entity_id;
    const auto arc_id = *arc.entity_id;

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

    CHECK(viewport.sketch_scene_.lines.size() == 1U);
    CHECK(viewport.sketch_scene_.curves.size() == 2U);
    const auto line_token =
        viewport.sketch_scene_.lines.front().token;
    const auto circle_token =
        viewport.sketch_scene_.curves[0].token;
    const auto arc_token =
        viewport.sketch_scene_.curves[1].token;

    // Build a mixed selection using the ordinary semantic point bridge.
    viewport.point_query_ = {true, line_token};
    click(interaction, sketch_id, 10.0, 10.0, 5.0, 0.0);
    viewport.point_query_ = {true, circle_token};
    click(interaction, sketch_id, 20.0, 20.0, 20.0, 10.0);
    viewport.point_query_ = {true, arc_token};
    click(interaction, sketch_id, 30.0, 30.0, -6.0, 5.0);
    CHECK(interaction.selectedCount() == 3U);

    // Selection-first MOVE skips collection and captures one revision.
    CHECK(interaction.activateMove());
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::await_base_point);
    CHECK(viewport.grip_scene_.grips.empty());
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::create_edit_crosshair);

    const auto state_before_preview =
        session.document().state();
    const auto revision_before_preview =
        session.document().revision();
    const auto undo_before_preview =
        session.undoDepth();

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        40.0, 40.0,
        2.0, 3.0));
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::await_destination);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        60.0, 60.0,
        7.0, 1.0));
    CHECK(!viewport.preview_scene_.lines.empty());
    CHECK(session.document().state() == state_before_preview);
    CHECK(session.document().revision() == revision_before_preview);
    CHECK(session.undoDepth() == undo_before_preview);

    CHECK(interaction.commitMove());
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before_preview + 1U);

    const auto* moved =
        session.document().findSketch(sketch_id);
    CHECK(moved != nullptr);
    CHECK(
        moved->model.findLine(line_id)->start() ==
        sketch::Point2{5.0, -2.0});
    CHECK(
        moved->model.findCircle(circle_id)->center() ==
        sketch::Point2{25.0, 8.0});
    CHECK(
        moved->model.findArc(arc_id)->center() ==
        sketch::Point2{-5.0, 3.0});
    CHECK(moved->model.findLine(line_id)->id() == line_id);
    CHECK(moved->model.findCircle(circle_id)->id() == circle_id);
    CHECK(moved->model.findArc(arc_id)->id() == arc_id);

    // A zero-delta completion exits MOVE without revision/history change.
    const auto zero_revision =
        session.document().revision();
    const auto zero_undo =
        session.undoDepth();
    CHECK(interaction.activateMove());
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        50.0, 50.0,
        3.0, 4.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        51.0, 51.0,
        3.0, 4.0));
    CHECK(interaction.commitMove());
    CHECK(session.document().revision() == zero_revision);
    CHECK(session.undoDepth() == zero_undo);
    CHECK(interaction.selectedCount() == 3U);

    // Clear ordinary selection, then verify command-first collection.
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 0U);
    CHECK(interaction.activateMove());
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::select_objects);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::select_pick_box);

    // Blank LMB is a no-op.
    viewport.point_query_ = {true, std::nullopt};
    click(interaction, sketch_id, 70.0, 70.0, 0.0, 0.0);
    CHECK(interaction.selectedCount() == 0U);
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::select_objects);

    // Point-add one entity, then Window-add the two curve tokens.
    const auto refreshed_line =
        viewport.sketch_scene_.lines.front().token;
    const auto refreshed_circle =
        viewport.sketch_scene_.curves[0].token;
    const auto refreshed_arc =
        viewport.sketch_scene_.curves[1].token;
    viewport.point_query_ = {true, refreshed_line};
    click(interaction, sketch_id, 10.0, 10.0, 5.0, -2.0);
    CHECK(interaction.selectedCount() == 1U);

    viewport.rectangle_query_ = {
        true,
        {refreshed_circle, refreshed_arc}};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        10.0, 10.0,
        0.0, 0.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        50.0, 50.0,
        0.0, 0.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_release,
        50.0, 50.0,
        0.0, 0.0));
    CHECK(interaction.selectedCount() == 3U);

    CHECK(interaction.completeMoveSelection());
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::await_base_point);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::create_edit_crosshair);

    // Esc from Base Point preserves collected objects as normal selection.
    CHECK(interaction.escape());
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);

    // Undo/Redo boundary cancels a transient MOVE first.
    CHECK(interaction.activateMove());
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        80.0, 80.0,
        1.0, 1.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        90.0, 90.0,
        9.0, 9.0));
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::await_destination);

    interaction.cancelForHistory();
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    const auto undo_result = session.undo();
    CHECK(undo_result.ok() && undo_result.changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(interaction.selectedCount() == 3U);

    const auto redo_result = session.redo();
    CHECK(redo_result.ok() && redo_result.changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(interaction.selectedCount() == 3U);

    // A revision change after MOVE starts makes the later commit stale.
    CHECK(interaction.activateMove());
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        110.0, 110.0,
        12.0, 6.0));
    CHECK(
        interaction.moveStage() ==
        sketch::MoveStage::await_destination);

    const auto external_change =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {100.0, 100.0},
                {110.0, 100.0}});
    CHECK(external_change.ok() && external_change.changed);
    const auto state_after_external_change =
        session.document().state();
    const auto revision_after_external_change =
        session.document().revision();
    const auto undo_after_external_change =
        session.undoDepth();

    CHECK(!interaction.commitMove());
    CHECK(
        session.document().state() ==
        state_after_external_change);
    CHECK(
        session.document().revision() ==
        revision_after_external_change);
    CHECK(
        session.undoDepth() ==
        undo_after_external_change);
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);

    // The moved mixed geometry and EntityIds survive the existing v4
    // persistence path.
    CHECK(session.save().ok());
    auto loaded = store.load(path);
    CHECK(loaded.ok());
    const auto* reopened =
        loaded.document->findSketch(sketch_id);
    CHECK(reopened != nullptr);
    CHECK(
        reopened->model.findLine(line_id)->start() ==
        sketch::Point2{5.0, -2.0});
    CHECK(
        reopened->model.findCircle(circle_id)->center() ==
        sketch::Point2{25.0, 8.0});
    CHECK(
        reopened->model.findArc(arc_id)->center() ==
        sketch::Point2{-5.0, 3.0});
    CHECK(reopened->model.findLine(line_id)->id() == line_id);
    CHECK(reopened->model.findCircle(circle_id)->id() == circle_id);
    CHECK(reopened->model.findArc(arc_id)->id() == arc_id);

    return EXIT_SUCCESS;
}
