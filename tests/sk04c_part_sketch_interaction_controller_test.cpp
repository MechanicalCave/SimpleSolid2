#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

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
    bool setProfileScene(
        const viewer::ProfileScene& scene) override {
        if (!scene.valid()) return false;
        profile_scene_ = scene;
        return true;
    }
    bool setProfilePreviewScene(
        const viewer::ProfilePreviewScene& scene) override {
        if (!scene.valid()) return false;
        profile_preview_scene_ = scene;
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
    viewer::ProfileScene profile_scene_;
    viewer::ProfilePreviewScene profile_preview_scene_;
    viewer::SketchPreviewScene preview_scene_;
    viewer::SketchGripScene grip_scene_;
    viewer::SketchInteractionPresentation
        interaction_presentation_;
    viewer::SketchGripQueryResult grip_query_{
        true,
        std::nullopt};
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
    CHECK(interaction.selectedCount() == 1U);
    CHECK(interaction.escape());
    CHECK(interaction.selectedCount() == 0U);
    CHECK(!interaction.escape());

    // Package F Command Line uses the existing semantic endpoint and the
    // same controller-owned Profile session; no QWidget parser/state exists.
    application::SketchCadInputSemanticEndpoint
        profile_command_endpoint{
            interaction,
            application::CadInputNumberFormat{"."}};

    auto profile_command =
        profile_command_endpoint.submit(
            "PROFILE");
    CHECK(profile_command.accepted);
    CHECK(interaction.profileToolActive());
    CHECK(
        interaction.profileAreaMode() ==
        part::ProfileAreaEditMode::add_area);

    profile_command =
        profile_command_endpoint.submit(
            "SUBTRACT");
    CHECK(profile_command.accepted);
    CHECK(
        interaction.profileAreaMode() ==
        part::ProfileAreaEditMode::subtract_area);

    profile_command =
        profile_command_endpoint.submit(
            "ISLANDS OFF");
    CHECK(profile_command.accepted);
    CHECK(
        !interaction.profileToolOptions()
             .detect_islands);

    profile_command =
        profile_command_endpoint.submit(
            "BOUNDARIES ON");
    CHECK(profile_command.accepted);
    CHECK(
        interaction.profileToolOptions()
            .show_region_boundaries);

    profile_command =
        profile_command_endpoint.submit(
            "FIND");
    CHECK(profile_command.accepted);

    profile_command =
        profile_command_endpoint.submit(
            "CANCEL");
    CHECK(profile_command.accepted);
    CHECK(!interaction.profileToolActive());

    profile_command =
        profile_command_endpoint.submit(
            "EDITPROFILE");
    CHECK(!profile_command.accepted);
    CHECK(
        profile_command.diagnostic ==
        "EDITPROFILE requires exactly one selected Profile.");

    // Package F Profile tool session: hover/draft are runtime-only and the
    // full Create/Edit session commits exactly once on Finish.
    for (const auto& line :
         std::vector<std::pair<sketch::Point2, sketch::Point2>>{
             {{100.0, 100.0}, {104.0, 100.0}},
             {{104.0, 100.0}, {104.0, 104.0}},
             {{104.0, 104.0}, {100.0, 104.0}},
             {{100.0, 104.0}, {100.0, 100.0}}}) {
        const auto result =
            session.execute(
                application::AddSketchLineCommand{
                    sketch_id,
                    line.first,
                    line.second});
        CHECK(result.ok());
    }

    const auto before_profile_tool =
        session.undoDepth();
    CHECK(
        session.document()
            .profileIdCursor()
            .serialized() == "1");
    CHECK(session.document().profiles().empty());

    CHECK(interaction.activateProfileCreate());
    CHECK(interaction.profileToolActive());
    CHECK(
        interaction.profileToolSessionKind() ==
        ui::ProfileToolSessionKind::create);
    CHECK(
        interaction.profileAreaMode() ==
        part::ProfileAreaEditMode::add_area);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        300.0, 300.0,
        102.0, 102.0));
    CHECK(
        interaction.profileAnalysisBuildCount() ==
        1U);
    CHECK(
        interaction.profileHoverStatus() ==
        part::ProfileAreaEditStatus::changed);
    CHECK(
        interaction.profileHoverPreview()
            .has_value());
    CHECK(
        viewport.profile_preview_scene_
            .region.has_value());

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        301.0, 301.0,
        101.0, 101.0));
    CHECK(
        interaction.profileAnalysisBuildCount() ==
        1U);
    CHECK(
        session.undoDepth() ==
        before_profile_tool);
    CHECK(session.document().profiles().empty());
    CHECK(
        session.document()
            .profileIdCursor()
            .serialized() == "1");

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        301.0, 301.0,
        101.0, 101.0));
    CHECK(
        interaction.profileDraftIntent()
            .has_value());
    CHECK(
        session.undoDepth() ==
        before_profile_tool);
    CHECK(session.document().profiles().empty());
    CHECK(
        session.document()
            .profileIdCursor()
            .serialized() == "1");

    CHECK(interaction.finishProfile());
    CHECK(!interaction.profileToolActive());
    CHECK(
        session.undoDepth() ==
        before_profile_tool + 1U);
    CHECK(session.document().profiles().size() == 1U);
    const auto profile_id =
        session.document().profiles().front().id;
    CHECK(profile_id.serialized() == "1");

    // Add source geometry first, then Edit Profile subtracts it only in the
    // transient draft until Finish.
    const auto inner_circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {102.0, 102.0},
                1.0});
    CHECK(inner_circle.ok());
    const auto before_edit =
        session.undoDepth();

    CHECK(
        interaction.activateProfileEdit(
            profile_id));
    CHECK(
        interaction.profileToolSessionKind() ==
        ui::ProfileToolSessionKind::edit);
    CHECK(
        interaction.editedProfileId() ==
        profile_id);
    CHECK(
        interaction.setProfileAreaMode(
            part::ProfileAreaEditMode::
                subtract_area));

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        302.0, 302.0,
        102.0, 102.0));
    CHECK(
        interaction.profileAnalysisBuildCount() ==
        1U);
    CHECK(
        interaction.profileHoverStatus() ==
        part::ProfileAreaEditStatus::changed);
    CHECK(
        interaction.profileHoverPreview()
            ->holes.size() == 1U);

    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        302.0, 302.0,
        102.0, 102.0));
    CHECK(
        session.undoDepth() ==
        before_edit);
    CHECK(
        session.document()
            .findProfile(profile_id)
            ->region_intent !=
        *interaction.profileDraftIntent());

    CHECK(interaction.finishProfile());
    CHECK(
        session.undoDepth() ==
        before_edit + 1U);
    CHECK(
        session.document()
            .findProfile(profile_id) !=
        nullptr);
    CHECK(
        session.document()
            .evaluateProfile(profile_id)
            ->valid());
    CHECK(
        session.document()
            .evaluateProfile(profile_id)
            ->region->holes.size() == 1U);

    // Cancel discards a fresh draft without allocating identity/history.
    const auto before_cancel =
        session.undoDepth();
    const auto cursor_before_cancel =
        session.document()
            .profileIdCursor()
            .serialized();
    CHECK(interaction.activateProfileCreate());
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        300.0, 300.0,
        101.0, 101.0));
    CHECK(interaction.profileDraftIntent().has_value());
    CHECK(interaction.escape());
    CHECK(!interaction.profileToolActive());
    CHECK(
        !viewport.profile_preview_scene_
             .region.has_value());
    CHECK(session.undoDepth() == before_cancel);
    CHECK(
        session.document()
            .profileIdCursor()
            .serialized() ==
        cursor_before_cancel);

    interaction.end();
    CHECK(!interaction.active());

    return EXIT_SUCCESS;
}
