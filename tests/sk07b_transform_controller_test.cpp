#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <cmath>
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
            << "SK-07B transform controller CHECK failed at line "
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
            ("ss2-sk07b-transform-" +
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
    part::PartDocumentStore store;
    const auto path =
        temp.path / "sk07b-transform.ss2part";

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    const auto published =
        store.createNew(path, document);
    CHECK(published.ok());

    application::DocumentSession session{
        path,
        std::move(document),
        *published.checkpoint};

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

    const auto line_id = *line.entity_id;
    const auto circle_id = *circle.entity_id;
    const auto arc_id = *arc.entity_id;
    const std::vector<sketch::EntityId> ids{
        line_id,
        circle_id,
        arc_id};

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

    // Build one mixed semantic selection through the normal point bridge.
    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.lines.front().token};
    click(interaction, sketch_id, 10.0, 10.0, 1.5, 0.0);
    CHECK(interaction.selectedCount() == 1U);

    // Mirroring a Line around its own axis is an exact semantic no-op.
    const auto mirror_noop_state =
        session.document().state();
    const auto mirror_noop_revision =
        session.document().revision();
    const auto mirror_noop_undo =
        session.undoDepth();
    CHECK(interaction.activateMirror());
    click(interaction, sketch_id, 30.0, 30.0, 0.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        40.0, 30.0,
        1.0, 0.0);
    CHECK(interaction.commitTransform());
    CHECK(session.document().state() == mirror_noop_state);
    CHECK(
        session.document().revision() ==
        mirror_noop_revision);
    CHECK(session.undoDepth() == mirror_noop_undo);
    CHECK(interaction.selectedCount() == 1U);

    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.curves[0].token};
    click(interaction, sketch_id, 20.0, 20.0, 2.0, 1.0);
    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.curves[1].token};
    click(interaction, sketch_id, 30.0, 30.0, 1.0, 6.0);
    CHECK(interaction.selectedCount() == 3U);

    // Selection-first ROTATE: Base -> Reference -> destination.
    const auto before_rotate =
        capture(session, sketch_id, ids);
    const auto revision_before_rotate =
        session.document().revision();
    const auto undo_before_rotate =
        session.undoDepth();

    CHECK(interaction.activateRotate());
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(viewport.grip_scene_.grips.empty());
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::create_edit_crosshair);

    click(interaction, sketch_id, 40.0, 40.0, 0.0, 0.0);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_reference_point);

    // Degenerate Reference Point does not advance.
    click(interaction, sketch_id, 41.0, 41.0, 0.0, 0.0);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_reference_point);

    click(interaction, sketch_id, 50.0, 40.0, 1.0, 0.0);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);

    const auto state_before_rotate_preview =
        session.document().state();
    interaction.resetLatencyDiagnostics();
    movePointer(
        interaction,
        sketch_id,
        40.0, 50.0,
        0.0, 1.0);
    CHECK(
        interaction.pointerResolutionCount() == 1U);
    CHECK(!viewport.preview_scene_.lines.empty());
    CHECK(
        session.document().state() ==
        state_before_rotate_preview);
    CHECK(
        session.document().revision() ==
        revision_before_rotate);
    CHECK(
        session.undoDepth() ==
        undo_before_rotate);

    CHECK(interaction.commitTransform());
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before_rotate + 1U);

    const auto expected_rotate =
        sketch::rotateSketchGeometry(
            before_rotate,
            {0.0, 0.0},
            pi * 0.5);
    CHECK(expected_rotate.has_value());
    CHECK(
        capture(session, sketch_id, ids) ==
        *expected_rotate);

    // Zero-angle ROTATE is a true no-op.
    const auto zero_rotate_state =
        session.document().state();
    const auto zero_rotate_revision =
        session.document().revision();
    const auto zero_rotate_undo =
        session.undoDepth();

    CHECK(interaction.activateRotate());
    click(interaction, sketch_id, 60.0, 60.0, 0.0, 0.0);
    click(interaction, sketch_id, 70.0, 60.0, 1.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        80.0, 60.0,
        2.0, 0.0);
    CHECK(interaction.commitTransform());
    CHECK(session.document().state() == zero_rotate_state);
    CHECK(
        session.document().revision() ==
        zero_rotate_revision);
    CHECK(session.undoDepth() == zero_rotate_undo);

    // Selection-first SCALE x2.
    const auto before_scale =
        capture(session, sketch_id, ids);
    const auto undo_before_scale =
        session.undoDepth();

    CHECK(interaction.activateScale());
    click(interaction, sketch_id, 50.0, 50.0, 0.0, 0.0);
    click(interaction, sketch_id, 60.0, 50.0, 1.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        70.0, 50.0,
        2.0, 0.0);
    CHECK(interaction.commitTransform());
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before_scale + 1U);

    const auto expected_scale =
        sketch::scaleSketchGeometry(
            before_scale,
            {0.0, 0.0},
            2.0);
    CHECK(expected_scale.has_value());
    CHECK(
        capture(session, sketch_id, ids) ==
        *expected_scale);

    // Factor-1 SCALE is also a no-op.
    const auto identity_scale_state =
        session.document().state();
    const auto identity_scale_revision =
        session.document().revision();
    const auto identity_scale_undo =
        session.undoDepth();

    CHECK(interaction.activateScale());
    click(interaction, sketch_id, 50.0, 50.0, 0.0, 0.0);
    click(interaction, sketch_id, 60.0, 50.0, 1.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        60.0, 50.0,
        1.0, 0.0);
    CHECK(interaction.commitTransform());
    CHECK(
        session.document().state() ==
        identity_scale_state);
    CHECK(
        session.document().revision() ==
        identity_scale_revision);
    CHECK(
        session.undoDepth() ==
        identity_scale_undo);

    // MIRROR around the Sketch X axis.
    const auto before_mirror =
        capture(session, sketch_id, ids);
    const auto undo_before_mirror =
        session.undoDepth();

    CHECK(interaction.activateMirror());
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_axis_start);
    click(interaction, sketch_id, 50.0, 50.0, 0.0, 0.0);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_axis_end);

    const auto mirror_preview_state =
        session.document().state();
    movePointer(
        interaction,
        sketch_id,
        60.0, 50.0,
        1.0, 0.0);
    CHECK(
        session.document().state() ==
        mirror_preview_state);
    CHECK(interaction.commitTransform());
    CHECK(interaction.selectedCount() == 3U);
    CHECK(session.undoDepth() == undo_before_mirror + 1U);

    const auto expected_mirror =
        sketch::mirrorSketchGeometry(
            before_mirror,
            {0.0, 0.0},
            {1.0, 0.0});
    CHECK(expected_mirror.has_value());
    const auto after_mirror =
        capture(session, sketch_id, ids);
    CHECK(after_mirror == *expected_mirror);
    CHECK(
        after_mirror.arcs.front().sweep_angle ==
        -before_mirror.arcs.front().sweep_angle);

    // Degenerate mirror axis cannot commit and Esc preserves selection.
    const auto invalid_mirror_state =
        session.document().state();
    const auto invalid_mirror_undo =
        session.undoDepth();
    CHECK(interaction.activateMirror());
    click(interaction, sketch_id, 50.0, 50.0, 0.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        50.0, 50.0,
        0.0, 0.0);
    CHECK(!interaction.commitTransform());
    CHECK(
        session.document().state() ==
        invalid_mirror_state);
    CHECK(session.undoDepth() == invalid_mirror_undo);
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 3U);

    // Command-first ROTATE reuses point/Window collection and blank LMB
    // remains a no-op.
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 0U);
    CHECK(interaction.activateRotate());
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::select_objects);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::select_pick_box);

    viewport.point_query_ = {true, std::nullopt};
    click(interaction, sketch_id, 90.0, 90.0, 0.0, 0.0);
    CHECK(interaction.selectedCount() == 0U);

    const auto current_line_token =
        viewport.sketch_scene_.lines.front().token;
    const auto current_circle_token =
        viewport.sketch_scene_.curves[0].token;
    const auto current_arc_token =
        viewport.sketch_scene_.curves[1].token;

    viewport.point_query_ = {true, current_line_token};
    click(interaction, sketch_id, 10.0, 10.0, 0.0, 0.0);
    CHECK(interaction.selectedCount() == 1U);

    viewport.rectangle_query_ = {
        true,
        {current_circle_token, current_arc_token}};
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
    CHECK(interaction.completeTransformSelection());
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_base_point);
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 3U);

    // Undo/Redo cancels a transient common transform first.
    CHECK(interaction.activateScale());
    click(interaction, sketch_id, 10.0, 10.0, 0.0, 0.0);
    click(interaction, sketch_id, 20.0, 10.0, 1.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        30.0, 10.0,
        3.0, 0.0);
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

    // A stale revision fails the complete transform atomically.
    CHECK(interaction.activateRotate());
    click(interaction, sketch_id, 100.0, 100.0, 0.0, 0.0);
    click(interaction, sketch_id, 110.0, 100.0, 1.0, 0.0);
    movePointer(
        interaction,
        sketch_id,
        100.0, 110.0,
        0.0, 1.0);

    const auto external =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {100.0, 100.0},
                {110.0, 100.0}});
    CHECK(external.ok() && external.changed);
    const auto state_after_external =
        session.document().state();
    const auto revision_after_external =
        session.document().revision();
    const auto undo_after_external =
        session.undoDepth();

    CHECK(!interaction.commitTransform());
    CHECK(
        session.document().state() ==
        state_after_external);
    CHECK(
        session.document().revision() ==
        revision_after_external);
    CHECK(
        session.undoDepth() ==
        undo_after_external);
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 3U);

    // Existing schema-v4 persistence preserves the transformed geometry
    // and original IDs.
    const auto persisted_geometry =
        capture(session, sketch_id, ids);
    CHECK(session.save().ok());

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    const auto* reopened =
        loaded.document->findSketch(sketch_id);
    CHECK(reopened != nullptr);
    const auto reopened_geometry =
        sketch::captureSketchTransformGeometry(
            reopened->model,
            ids);
    CHECK(reopened_geometry.has_value());
    CHECK(*reopened_geometry == persisted_geometry);
    CHECK(reopened->model.findLine(line_id)->id() == line_id);
    CHECK(reopened->model.findCircle(circle_id)->id() == circle_id);
    CHECK(reopened->model.findArc(arc_id)->id() == arc_id);

    return EXIT_SUCCESS;
}
