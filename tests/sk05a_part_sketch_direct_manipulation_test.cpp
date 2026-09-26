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
            << "SK-05A direct interaction CHECK failed at line "
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

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{"sk05a-direct.ss2part"},
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok() && created.sketch_id);
    const auto sketch_id = *created.sketch_id;

    const auto first =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 0.0},
                {10.0, 0.0}});
    const auto second =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 10.0},
                {10.0, 10.0}});
    CHECK(first.ok() && first.entity_id);
    CHECK(second.ok() && second.entity_id);
    const auto id1 = *first.entity_id;
    const auto id2 = *second.entity_id;

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

    const auto token1 =
        viewport.sketch_scene_.lines[0].token;
    const auto token2 =
        viewport.sketch_scene_.lines[1].token;

    viewport.point_query_ = {true, token1};
    click(interaction, sketch_id, 20.0, 20.0, 5.0, 0.0);
    CHECK(interaction.selectedCount() == 1U);

    viewport.point_query_ = {true, token2};
    click(interaction, sketch_id, 20.0, 40.0, 5.0, 10.0);
    CHECK(interaction.selectedCount() == 2U);
    CHECK(viewport.grip_scene_.grips.size() == 6U);
    CHECK(viewport.selection_.primary == token2);

    // Grip hover has priority and remains runtime-only.
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            token1,
            viewer::SketchGripRole::line_center}};
    const auto revision_before_hover =
        session.document().revision();
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        20.0, 20.0,
        5.0, 0.0));
    CHECK(
        viewport.interaction_presentation_.
            hovered_grip.has_value());
    CHECK(
        session.document().revision() ==
        revision_before_hover);

    // Clicking a Center grip starts Move on the frozen selected set.
    click(interaction, sketch_id, 20.0, 20.0, 5.0, 0.0);
    CHECK(interaction.directManipulationActive());
    CHECK(
        viewport.interaction_presentation_.
            active_grip.has_value());

    const auto before_move_state =
        session.document().state();
    const auto before_move_revision =
        session.document().revision();
    const auto before_move_undo =
        session.undoDepth();

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        50.0, 50.0,
        8.0, 4.0));
    CHECK(viewport.preview_scene_.lines.size() == 2U);
    CHECK(session.document().state() == before_move_state);
    CHECK(session.document().revision() == before_move_revision);
    CHECK(session.undoDepth() == before_move_undo);

    // Second LMB commits one atomic command.
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        50.0, 50.0,
        8.0, 4.0));
    CHECK(!interaction.directManipulationActive());
    CHECK(session.undoDepth() == before_move_undo + 1U);
    CHECK(interaction.selectedCount() == 2U);

    const auto* moved =
        session.document().findSketch(sketch_id);
    CHECK(moved != nullptr);
    CHECK(moved->model.findLine(id1)->start() ==
          sketch::Point2{3.0, 4.0});
    CHECK(moved->model.findLine(id2)->start() ==
          sketch::Point2{3.0, 14.0});
    CHECK(moved->model.findLine(id1)->id() == id1);
    CHECK(moved->model.findLine(id2)->id() == id2);

    // Start grip reshapes only its owner even with multi-selection.
    const auto refreshed_token1 =
        viewport.sketch_scene_.lines[0].token;
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            refreshed_token1,
            viewer::SketchGripRole::line_start}};
    click(interaction, sketch_id, 25.0, 25.0, 3.0, 4.0);
    CHECK(interaction.directManipulationActive());

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        30.0, 30.0,
        -2.0, 1.0));
    CHECK(viewport.preview_scene_.lines.size() == 1U);
    CHECK(interaction.commitDirectManipulation());
    CHECK(interaction.selectedCount() == 2U);

    const auto* reshaped =
        session.document().findSketch(sketch_id);
    CHECK(reshaped->model.findLine(id1)->start() ==
          sketch::Point2{-2.0, 1.0});
    CHECK(reshaped->model.findLine(id2)->start() ==
          sketch::Point2{3.0, 14.0});

    // Line creation preserves the semantic selection, hides grips,
    // and does not auto-select the newly authored Line.
    interaction.activateLine();
    CHECK(interaction.selectedCount() == 2U);
    CHECK(viewport.grip_scene_.grips.empty());

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        70.0, 70.0,
        30.0, 30.0));
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        90.0, 70.0,
        40.0, 30.0));
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.entityCount() == 3U);
    CHECK(interaction.selectedCount() == 2U);

    interaction.finishLine();
    CHECK(interaction.selectedCount() == 2U);
    CHECK(viewport.grip_scene_.grips.size() == 6U);

    // Esc during manipulation preserves selection; next Esc clears it.
    const auto after_line_token1 =
        viewport.sketch_scene_.lines[0].token;
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            after_line_token1,
            viewer::SketchGripRole::line_center}};
    click(interaction, sketch_id, 20.0, 20.0, -2.0, 1.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 2U);
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 0U);

    return EXIT_SUCCESS;
}
