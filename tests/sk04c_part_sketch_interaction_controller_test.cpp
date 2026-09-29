#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/measurement.hpp>

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
    bool setSketchMeasureMarkerScene(
        const viewer::SketchMeasureMarkerScene& scene) override {
        if (!scene.valid()) return false;
        measure_marker_scene_ = scene;
        return true;
    }
    viewer::SketchMeasureMarkerQueryResult
    querySketchMeasureMarkers(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return measure_marker_query_;
    }
    bool setSketchMeasureCueScene(
        const viewer::SketchMeasureCueScene& scene) override {
        if (!scene.valid()) return false;
        measure_cue_scene_ = scene;
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
    viewer::ProfileScene profile_scene_;
    viewer::ProfilePreviewScene profile_preview_scene_;
    viewer::SketchPreviewScene preview_scene_;
    viewer::SketchGripScene grip_scene_;
    viewer::SketchInteractionPresentation
        interaction_presentation_;
    viewer::SketchGripQueryResult grip_query_{
        true,
        std::nullopt};
    viewer::SketchMeasureMarkerScene
        measure_marker_scene_;
    viewer::SketchMeasureMarkerQueryResult
        measure_marker_query_{true, {}};
    viewer::SketchMeasureCueScene
        measure_cue_scene_;
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

    // R8A Measure is read-only and owns a runtime target separate from
    // ordinary semantic selection.
    const auto measure_state =
        session.document().state();
    const auto measure_revision =
        session.document().revision();
    const auto measure_undo =
        session.undoDepth();
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::line);
    CHECK(interaction.activateMeasure());
    CHECK(interaction.tool() == sketch::SketchTool::measure);
    CHECK(interaction.measureTarget().has_value());
    CHECK(interaction.measureResult().has_value());
    CHECK(std::holds_alternative<sketch::LineMeasurement>(
        *interaction.measureResult()));
    const auto initial_measure_target =
        interaction.measureTarget();

    const auto second_measure_token =
        viewport.sketch_scene_.lines[1].token;
    viewport.point_query_ = {true, second_measure_token};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        80.0, 50.0,
        10.0, 5.0));
    CHECK(interaction.measureTarget().has_value());
    CHECK(
        interaction.measureTarget() !=
        initial_measure_target);
    CHECK(interaction.selectedCount() == 1U);
    CHECK(session.document().state() == measure_state);
    CHECK(session.document().revision() == measure_revision);
    CHECK(session.undoDepth() == measure_undo);
    CHECK(
        interaction.lastRepeatableCommand() ==
        sketch::SketchTool::line);

    viewport.point_query_ = {true, std::nullopt};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        150.0, 150.0,
        20.0, 20.0));
    CHECK(!interaction.measureTarget().has_value());
    CHECK(interaction.selectedCount() == 1U);

    // R8B: Between is a read-only submode of Measure. Marker query has
    // priority over entity body query and normal Sketch selection is stable.
    CHECK(interaction.activateMeasureBetween());
    CHECK(interaction.measureBetweenActive());
    CHECK(!interaction.measureTarget().has_value());
    CHECK(viewport.measure_marker_scene_.valid());
    CHECK(viewport.measure_marker_scene_.markers.size() == 6U);
    CHECK(viewport.measure_marker_scene_.selected.empty());

    const auto first_line_start_marker =
        std::find_if(
            viewport.measure_marker_scene_.markers.begin(),
            viewport.measure_marker_scene_.markers.end(),
            [first_token](
                const viewer::SketchMeasureMarkerPresentation& marker) {
                return marker.key.owner == first_token &&
                       marker.key.role ==
                           viewer::SketchMeasureMarkerRole::line_start;
            });
    const auto first_line_end_marker =
        std::find_if(
            viewport.measure_marker_scene_.markers.begin(),
            viewport.measure_marker_scene_.markers.end(),
            [first_token](
                const viewer::SketchMeasureMarkerPresentation& marker) {
                return marker.key.owner == first_token &&
                       marker.key.role ==
                           viewer::SketchMeasureMarkerRole::line_end;
            });
    CHECK(
        first_line_start_marker !=
        viewport.measure_marker_scene_.markers.end());
    CHECK(
        first_line_end_marker !=
        viewport.measure_marker_scene_.markers.end());

    // Distinct-coordinate overlap must fail closed rather than rank.
    viewport.measure_marker_query_ = {
        true,
        {
            first_line_start_marker->key,
            first_line_end_marker->key}};
    viewport.point_query_ = {true, second_measure_token};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        50.0, 20.0,
        5.0, 0.0));
    CHECK(
        !interaction.measureFirstRelationTarget()
             .has_value());
    CHECK(interaction.selectedCount() == 1U);

    // Explicit visible semantic point becomes Target A.
    viewport.measure_marker_query_ = {
        true,
        {first_line_start_marker->key}};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        20.0, 20.0,
        0.0, 0.0));
    CHECK(
        interaction.measureFirstRelationTarget()
            .has_value());
    CHECK(
        !interaction.measureSecondRelationTarget()
             .has_value());
    CHECK(viewport.measure_marker_scene_.selected.size() == 1U);
    CHECK(viewport.measure_cue_scene_.empty());

    // No marker hit: Line body is Target B. This produces point↔Line
    // perpendicular cue while normal selection remains unchanged.
    viewport.measure_marker_query_ = {true, {}};
    viewport.point_query_ = {true, second_measure_token};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        80.0, 50.0,
        10.0, 5.0));
    CHECK(
        interaction.measureSecondRelationTarget()
            .has_value());
    const auto between_point_line =
        interaction.measureRelationalResult();
    CHECK(between_point_line.has_value());
    CHECK(
        std::holds_alternative<
            sketch::PointLineMeasurement>(
            *between_point_line));
    CHECK(viewport.measure_cue_scene_.segments.size() == 1U);
    CHECK(
        viewport.measure_cue_scene_.
            highlighted_entities.size() == 1U);
    CHECK(interaction.selectedCount() == 1U);
    CHECK(session.document().state() == measure_state);
    CHECK(session.document().revision() == measure_revision);
    CHECK(session.undoDepth() == measure_undo);

    // After a result, the next accepted target becomes the next Target A.
    viewport.point_query_ = {true, first_token};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        50.0, 20.0,
        5.0, 0.0));
    CHECK(
        interaction.measureFirstRelationTarget()
            .has_value());
    CHECK(
        !interaction.measureSecondRelationTarget()
             .has_value());
    CHECK(!interaction.measureRelationalResult().has_value());
    CHECK(
        viewport.measure_cue_scene_.
            highlighted_entities.size() == 1U);
    CHECK(viewport.measure_cue_scene_.segments.empty());

    viewport.point_query_ = {true, second_measure_token};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        80.0, 50.0,
        10.0, 5.0));
    const auto between_lines =
        interaction.measureRelationalResult();
    CHECK(between_lines.has_value());
    CHECK(
        std::holds_alternative<
            sketch::LineLineAngleMeasurement>(
            *between_lines));
    CHECK(viewport.measure_cue_scene_.segments.empty());
    CHECK(
        viewport.measure_cue_scene_.
            highlighted_entities.size() == 2U);

    // Blank clears only relation state and stays in Between.
    viewport.point_query_ = {true, std::nullopt};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        150.0, 150.0,
        20.0, 20.0));
    CHECK(interaction.measureBetweenActive());
    CHECK(
        !interaction.measureFirstRelationTarget()
             .has_value());
    CHECK(viewport.measure_cue_scene_.empty());

    // Esc is hierarchical: Between -> quick Measure -> Select.
    CHECK(interaction.escape());
    CHECK(interaction.tool() == sketch::SketchTool::measure);
    CHECK(!interaction.measureBetweenActive());
    CHECK(viewport.measure_marker_scene_.empty());
    CHECK(viewport.measure_cue_scene_.empty());
    CHECK(interaction.selectedCount() == 1U);

    CHECK(interaction.escape());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 1U);
    CHECK(session.document().state() == measure_state);
    CHECK(session.document().revision() == measure_revision);
    CHECK(session.undoDepth() == measure_undo);

    viewport.point_query_ = {true, first_token};

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

    // Multi-selection does not silently choose primary/storage order.
    CHECK(interaction.activateMeasure());
    CHECK(!interaction.measureTarget().has_value());
    CHECK(interaction.selectedCount() == 2U);
    viewport.point_query_ = {true, rect_first};
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::primary_press,
        60.0, 20.0,
        6.0, 0.0));
    CHECK(interaction.measureTarget().has_value());
    CHECK(interaction.selectedCount() == 2U);
    CHECK(interaction.escape());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 2U);

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
    CHECK(
        !viewport.profile_preview_scene_
             .show_boundary);

    auto profile_options =
        interaction.profileToolOptions();
    profile_options.show_region_boundaries = true;
    CHECK(
        interaction.setProfileToolOptions(
            profile_options));
    CHECK(
        viewport.profile_preview_scene_
            .show_boundary);

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
    tree_controller.setProfileSelection(
        {profile_id},
        profile_id);
    CHECK(
        tree_controller.selectedProfileIds() ==
        std::vector<part::ProfileId>{profile_id});
    CHECK(
        tree_controller.primaryProfileId() ==
        profile_id);

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
    CHECK(
        viewport.profile_preview_scene_.tone ==
        viewer::ProfilePreviewTone::subtractive);
    CHECK(
        viewport.profile_preview_scene_
            .emphasis_region.has_value());

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

    // Manual-F regression: the ordinary Line tool must be deterministic when
    // endpoints are exactly shared. An open chain reports a problem; after an
    // exact closing segment, repeated hover/click/Finish creates one Profile.
    {
        auto exact_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession exact_session{
            std::filesystem::path{
                "sk04c-exact-profile.ss2part"},
            std::move(exact_document)};

        const auto exact_created =
            exact_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            exact_created.ok() &&
            exact_created.sketch_id.has_value());
        const auto exact_sketch_id =
            *exact_created.sketch_id;

        QTreeWidget exact_tree;
        ui::PartDocumentTreeController
            exact_tree_controller{exact_tree};
        TestViewport exact_viewport;
        ui::PartViewportController
            exact_viewport_controller{
                exact_tree_controller,
                &exact_viewport};
        exact_viewport_controller.setDocumentSession(
            &exact_session);
        exact_viewport_controller.setSketchEditSketch(
            exact_sketch_id);

        ui::PartSketchInteractionController
            exact_interaction{
                exact_viewport_controller};
        exact_interaction.begin(
            exact_session,
            exact_sketch_id);

        std::string exact_status;
        exact_interaction.setStatusHandler(
            [&exact_status](
                const std::string& message) {
                exact_status = message;
            });

        exact_interaction.activateLine();
        for (const auto& point :
             std::vector<sketch::Point2>{
                 {200.0, 200.0},
                 {204.0, 200.0},
                 {204.0, 204.0},
                 {200.0, 204.0}}) {
            exact_interaction.onPointer(
                pointer(
                    exact_sketch_id,
                    viewer::SpatialPointerPhase::
                        primary_press,
                    point.u,
                    point.v,
                    point.u,
                    point.v));
        }
        CHECK(
            exact_session.document()
                .findSketch(exact_sketch_id)
                ->model.state().lines.size() == 3U);

        CHECK(exact_interaction.activateProfileCreate());
        exact_interaction.onPointer(
            pointer(
                exact_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                202.0,
                202.0,
                202.0,
                202.0));
        CHECK(
            !exact_interaction.profileDraftIntent()
                 .has_value());
        CHECK(
            exact_status.find("open boundary") !=
            std::string::npos);
        exact_interaction.cancelProfile();

        exact_interaction.activateLine();
        exact_interaction.onPointer(
            pointer(
                exact_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                200.0,
                204.0,
                200.0,
                204.0));
        exact_interaction.onPointer(
            pointer(
                exact_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                200.0,
                200.0,
                200.0,
                200.0));
        CHECK(
            exact_session.document()
                .findSketch(exact_sketch_id)
                ->model.state().lines.size() == 4U);

        CHECK(exact_interaction.activateProfileCreate());
        for (int index = 0; index < 16; ++index) {
            exact_interaction.onPointer(
                pointer(
                    exact_sketch_id,
                    viewer::SpatialPointerPhase::move,
                    202.0,
                    202.0,
                    202.0,
                    202.0));
            CHECK(
                exact_interaction.profileHoverStatus() ==
                part::ProfileAreaEditStatus::changed);
            CHECK(
                exact_interaction.profileHoverPreview()
                    .has_value());
        }

        exact_interaction.onPointer(
            pointer(
                exact_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                202.0,
                202.0,
                202.0,
                202.0));
        CHECK(
            exact_interaction.profileDraftIntent()
                .has_value());
        CHECK(exact_interaction.finishProfile());
        CHECK(
            exact_session.document().profiles().size() ==
            1U);
        const auto exact_profile_id =
            exact_session.document().profiles()
                .front().id;
        exact_tree_controller.setProfileSelection(
            {exact_profile_id},
            exact_profile_id);
        CHECK(
            exact_tree_controller.selectedProfileIds() ==
            std::vector<part::ProfileId>{
                exact_profile_id});
        CHECK(
            exact_tree_controller.primaryProfileId() ==
            exact_profile_id);

        exact_interaction.end();
    }

    return EXIT_SUCCESS;
}
