#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-07C COPY controller CHECK failed at line "
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

void movePointer(
    ui::PartSketchInteractionController& interaction,
    const sketch::SketchId& sketch_id,
    double sx,
    double sy,
    double u,
    double v) {
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        sx, sy, u, v));
}

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("ss2-sk07c-copy-controller-" +
             std::string{
                 core::DocumentId::generate().value()});
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

sketch::SketchTransformGeometry capture(
    const application::DocumentSession& session,
    const sketch::SketchId& sketch_id,
    const std::vector<sketch::EntityId>& ids) {
    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto geometry =
        sketch::captureSketchTransformGeometry(
            hosted->model,
            ids);
    CHECK(geometry.has_value());
    return *geometry;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    constexpr double pi =
        std::numbers::pi_v<double>;

    TempDirectory temp;
    const auto path =
        temp.path / "sk07c-copy-controller.ss2part";

    part::PartDocumentStore store;
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
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
                {1.0, 0.0},
                {3.0, 0.0}});
    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {2.0, 1.0},
                2.0});
    const auto arc =
        session.execute(
            application::AddSketchArcCommand{
                sketch_id,
                {1.0, 2.0},
                4.0,
                pi * 0.25,
                pi * 0.75});
    CHECK(line.ok() && line.entity_id);
    CHECK(circle.ok() && circle.entity_id);
    CHECK(arc.ok() && arc.entity_id);

    const std::vector<sketch::EntityId> source_ids{
        *line.entity_id,
        *circle.entity_id,
        *arc.entity_id};
    const auto source =
        capture(session, sketch_id, source_ids);

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

    // Build source selection through the ordinary semantic point bridge.
    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.lines.front().token};
    click(interaction, sketch_id, 10.0, 10.0, 1.5, 0.0);

    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.curves[0].token};
    click(interaction, sketch_id, 20.0, 20.0, 2.0, 1.0);

    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.curves[1].token};
    click(interaction, sketch_id, 30.0, 30.0, 1.0, 6.0);
    CHECK(interaction.selectedCount() == 3U);

    const auto undo_before =
        session.undoDepth();
    const auto revision_before =
        session.document().revision();

    CHECK(interaction.activateCopy());
    CHECK(interaction.tool() == sketch::SketchTool::copy);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);

    click(interaction, sketch_id, 40.0, 40.0, 0.0, 0.0);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);

    // Zero displacement must not create an invisible coincident copy.
    movePointer(interaction, sketch_id, 40.0, 40.0, 0.0, 0.0);
    CHECK(!interaction.commitTransform());
    CHECK(interaction.tool() == sketch::SketchTool::copy);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before);
    CHECK(session.document().revision() == revision_before);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 3U);

    // First placement commits one mixed batch and leaves COPY active.
    movePointer(interaction, sketch_id, 60.0, 50.0, 5.0, 2.0);
    CHECK(interaction.commitTransform());
    CHECK(interaction.tool() == sketch::SketchTool::copy);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before + 1U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 6U);
    CHECK(capture(session, sketch_id, source_ids) == source);

    // A second placement derives from the same source/Base and is a
    // separate history entry.
    movePointer(interaction, sketch_id, 20.0, 70.0, -4.0, 6.0);
    CHECK(interaction.commitTransform());
    CHECK(interaction.tool() == sketch::SketchTool::copy);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before + 2U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 9U);
    CHECK(capture(session, sketch_id, source_ids) == source);

    // Undo is a history boundary: cancel transient COPY first, then
    // remove only the last committed placement.
    interaction.cancelForHistory();
    CHECK(interaction.tool() == sketch::SketchTool::select);
    const auto undo = session.undo();
    CHECK(undo.ok() && undo.changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(interaction.selectedCount() == 3U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 6U);

    const auto redo = session.redo();
    CHECK(redo.ok() && redo.changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(interaction.selectedCount() == 3U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 9U);

    // SK-07G Move+Copy through a center grip duplicates the complete frozen
    // mixed selection, not only the active grip owner.
    const auto grip_copy_count_before =
        session.document().findSketch(sketch_id)->
            model.entityCount();
    const auto grip_copy_undo_before =
        session.undoDepth();

    CHECK(!viewport.sketch_scene_.lines.empty());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            viewport.sketch_scene_.lines.front().token,
            viewer::SketchGripRole::line_center}};
    click(interaction, sketch_id, 95.0, 95.0, 2.0, 0.0);
    CHECK(interaction.directManipulationActive());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::move);
    CHECK(interaction.enableGripCopy());
    CHECK(interaction.directManipulationCopyEnabled());

    // Exact pivot/no-change is a clean no-op and leaves Grip Copy active.
    const auto grip_copy_revision_before =
        session.document().revision();
    click(interaction, sketch_id, 95.0, 95.0, 2.0, 0.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.directManipulationCopyEnabled());
    CHECK(
        session.document().revision() ==
        grip_copy_revision_before);
    CHECK(
        session.undoDepth() ==
        grip_copy_undo_before);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() ==
        grip_copy_count_before);

    click(interaction, sketch_id, 105.0, 105.0, 5.0, 3.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.directManipulationCopyEnabled());
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() ==
        grip_copy_count_before + source_ids.size());
    CHECK(
        session.undoDepth() ==
        grip_copy_undo_before + 1U);
    CHECK(capture(session, sketch_id, source_ids) == source);

    // Undo is a history boundary for active Grip Copy: cancel transient
    // state first, then undo only the last committed placement.
    interaction.cancelForHistory();
    CHECK(!interaction.directManipulationActive());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    const auto grip_copy_undo = session.undo();
    CHECK(grip_copy_undo.ok() && grip_copy_undo.changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(interaction.selectedCount() == 3U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() ==
        grip_copy_count_before);

    const auto grip_copy_redo = session.redo();
    CHECK(grip_copy_redo.ok() && grip_copy_redo.changed);
    CHECK(interaction.reconcileAfterHistory());
    CHECK(interaction.selectedCount() == 3U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() ==
        grip_copy_count_before + source_ids.size());
    CHECK(capture(session, sketch_id, source_ids) == source);

    // A stale DocumentRevision fails Grip Copy atomically and ends only
    // the transient direct-manipulation session.
    CHECK(!viewport.sketch_scene_.lines.empty());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            viewport.sketch_scene_.lines.front().token,
            viewer::SketchGripRole::line_center}};
    click(interaction, sketch_id, 115.0, 115.0, 2.0, 0.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.enableGripCopy());
    movePointer(interaction, sketch_id, 120.0, 120.0, 6.0, 4.0);

    const auto grip_external =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {120.0, 120.0},
                {130.0, 120.0}});
    CHECK(grip_external.ok() && grip_external.changed);
    const auto grip_count_after_external =
        session.document().findSketch(sketch_id)->
            model.entityCount();

    CHECK(!interaction.commitDirectManipulation());
    CHECK(!interaction.directManipulationActive());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() ==
        grip_count_after_external);

    // Stale revision fails the pending normal COPY atomically and ends the
    // transient command while preserving already committed copies.
    CHECK(interaction.activateCopy());
    click(interaction, sketch_id, 80.0, 80.0, 0.0, 0.0);
    movePointer(interaction, sketch_id, 90.0, 90.0, 2.0, 2.0);

    const auto external =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {100.0, 100.0},
                {110.0, 100.0}});
    CHECK(external.ok() && external.changed);

    const auto count_after_external =
        session.document().findSketch(sketch_id)->
            model.entityCount();
    CHECK(!interaction.commitTransform());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == count_after_external);

    return EXIT_SUCCESS;
}
