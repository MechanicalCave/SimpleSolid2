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

#include <cmath>
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

bool near(double first, double second) {
    return std::abs(first - second) <= 1.0e-10;
}

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
             .show_islands);

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
    CHECK(!profile_command.accepted);

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

    // SR-01: island truth stays active even when its presentation option is
    // hidden. Add one bounded material island inside the authored hole and
    // verify the same non-zero semantic count with Show Islands ON and OFF.
    const auto nested_island_circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {102.0, 102.0},
                0.5});
    CHECK(nested_island_circle.ok());
    CHECK(
        interaction.activateProfileEdit(
            profile_id));
    CHECK(
        interaction.profileIslandCount() ==
        1U);
    const auto island_analysis_builds =
        interaction.profileAnalysisBuildCount();
    profile_options =
        interaction.profileToolOptions();
    CHECK(profile_options.show_islands);
    profile_options.show_islands = false;
    CHECK(
        interaction.setProfileToolOptions(
            profile_options));
    CHECK(
        interaction.profileIslandCount() ==
        1U);
    CHECK(
        interaction.profileAnalysisBuildCount() ==
        island_analysis_builds);
    profile_options.show_islands = true;
    CHECK(
        interaction.setProfileToolOptions(
            profile_options));
    CHECK(
        interaction.profileIslandCount() ==
        1U);
    interaction.cancelProfile();

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

    {
        auto rectangle_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            rectangle_session{
                std::filesystem::path{
                    "sk04c-rectangle.ss2part"},
                std::move(rectangle_document)};

        const auto created_rectangle_sketch =
            rectangle_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            created_rectangle_sketch.ok() &&
            created_rectangle_sketch.sketch_id
                .has_value());
        const auto rectangle_sketch_id =
            *created_rectangle_sketch.sketch_id;

        QTreeWidget rectangle_tree;
        ui::PartDocumentTreeController
            rectangle_tree_controller{
                rectangle_tree};
        TestViewport rectangle_viewport;
        ui::PartViewportController
            rectangle_viewport_controller{
                rectangle_tree_controller,
                &rectangle_viewport};
        rectangle_viewport_controller
            .setDocumentSession(
                &rectangle_session);
        rectangle_viewport_controller
            .setSketchEditSketch(
                rectangle_sketch_id);

        ui::PartSketchInteractionController
            rectangle_interaction{
                rectangle_viewport_controller};
        rectangle_interaction.begin(
            rectangle_session,
            rectangle_sketch_id);

        CHECK(
            rectangle_interaction.creationRole() ==
            sketch::EntityRole::regular);
        CHECK(
            !rectangle_interaction
                 .rectangleDrawDiagonals());

        const auto option_revision =
            rectangle_session.document().revision();
        const auto option_undo =
            rectangle_session.undoDepth();
        CHECK(
            rectangle_interaction.setCreationRole(
                sketch::EntityRole::construction));
        CHECK(
            rectangle_interaction.creationRole() ==
            sketch::EntityRole::construction);
        CHECK(
            rectangle_session.document().revision() ==
            option_revision);
        CHECK(
            rectangle_session.undoDepth() ==
            option_undo);
        CHECK(
            rectangle_interaction.setCreationRole(
                sketch::EntityRole::regular));
        CHECK(
            rectangle_session.document().revision() ==
            option_revision);
        CHECK(
            rectangle_session.undoDepth() ==
            option_undo);

        CHECK(
            rectangle_interaction
                .setRectangleDrawDiagonals(true));
        CHECK(
            rectangle_interaction
                .rectangleDrawDiagonals());
        CHECK(
            rectangle_session.document().revision() ==
            option_revision);
        CHECK(
            rectangle_session.undoDepth() ==
            option_undo);

        rectangle_interaction.activateRectangle();
        CHECK(
            rectangle_interaction.tool() ==
            sketch::SketchTool::rectangle);
        CHECK(
            rectangle_interaction.rectangleStage() ==
            sketch::RectangleStage::
                await_first_corner);

        rectangle_interaction.onPointer(
            pointer(
                rectangle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                0.0, 0.0,
                0.0, 0.0));
        CHECK(
            rectangle_interaction.rectangleStage() ==
            sketch::RectangleStage::
                await_opposite_corner);
        const auto rectangle_request =
            rectangle_interaction.activePointRequest();
        CHECK(rectangle_request.has_value());
        CHECK(!rectangle_request->direct_distance_enabled);

        rectangle_interaction.onPointer(
            pointer(
                rectangle_sketch_id,
                viewer::SpatialPointerPhase::move,
                40.0, 20.0,
                4.0, 2.0));
        CHECK(
            rectangle_viewport.preview_scene_
                .lines.size() == 6U);
        for (std::size_t index = 0U;
             index < 4U;
             ++index) {
            CHECK(
                !rectangle_viewport.preview_scene_
                     .lines[index].construction);
        }
        CHECK(
            rectangle_viewport.preview_scene_
                .lines[4].construction);
        CHECK(
            rectangle_viewport.preview_scene_
                .lines[5].construction);

        const auto rectangle_undo =
            rectangle_session.undoDepth();
        rectangle_interaction.onPointer(
            pointer(
                rectangle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                40.0, 20.0,
                4.0, 2.0));
        CHECK(
            rectangle_session.undoDepth() ==
            rectangle_undo + 1U);
        const auto* authored_rectangle =
            rectangle_session.document()
                .findSketch(rectangle_sketch_id);
        CHECK(authored_rectangle != nullptr);
        CHECK(
            authored_rectangle->model.entityCount() ==
            6U);
        CHECK(
            rectangle_interaction.rectangleStage() ==
            sketch::RectangleStage::
                await_first_corner);
        CHECK(
            rectangle_interaction
                .lastRepeatableCommand() ==
            sketch::SketchTool::rectangle);

        const auto& rectangle_lines =
            authored_rectangle->model.state().lines;
        CHECK(rectangle_lines.size() == 6U);
        for (std::size_t index = 0U;
             index < 4U;
             ++index) {
            CHECK(
                rectangle_lines[index].role ==
                sketch::EntityRole::regular);
        }
        CHECK(
            rectangle_lines[4].role ==
            sketch::EntityRole::construction);
        CHECK(
            rectangle_lines[5].role ==
            sketch::EntityRole::construction);

        rectangle_interaction.activateSelect();
        CHECK(
            rectangle_interaction.repeatLastCommand());
        CHECK(
            rectangle_interaction.tool() ==
            sketch::SketchTool::rectangle);
        CHECK(
            rectangle_interaction
                .rectangleDrawDiagonals());

        CHECK(
            rectangle_interaction.setCreationRole(
                sketch::EntityRole::construction));
        rectangle_interaction.activateLine();
        CHECK(
            rectangle_interaction.creationRole() ==
            sketch::EntityRole::construction);
        rectangle_interaction.activateCircle();
        CHECK(
            rectangle_interaction.creationRole() ==
            sketch::EntityRole::construction);
        rectangle_interaction.activateRectangle();
        CHECK(
            rectangle_interaction.creationRole() ==
            sketch::EntityRole::construction);
        CHECK(
            rectangle_interaction
                .rectangleDrawDiagonals());

        // First Corner captures DocumentRevision. Any intervening authored
        // mutation invalidates the pending Rectangle and the second click
        // must fail closed without adding perimeter geometry.
        rectangle_interaction.activateSelect();
        CHECK(
            rectangle_interaction.setCreationRole(
                sketch::EntityRole::regular));
        CHECK(
            rectangle_interaction
                .setRectangleDrawDiagonals(false));
        rectangle_interaction.activateRectangle();
        rectangle_interaction.onPointer(
            pointer(
                rectangle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                100.0, 100.0,
                10.0, 10.0));
        CHECK(
            rectangle_interaction.rectangleStage() ==
            sketch::RectangleStage::
                await_opposite_corner);

        const auto external_mutation =
            rectangle_session.execute(
                application::AddSketchLineCommand{
                    rectangle_sketch_id,
                    {20.0, 20.0},
                    {21.0, 20.0}});
        CHECK(
            external_mutation.ok() &&
            external_mutation.changed);
        const auto stale_entity_count =
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount();
        const auto stale_undo_depth =
            rectangle_session.undoDepth();

        rectangle_interaction.onPointer(
            pointer(
                rectangle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                140.0, 120.0,
                14.0, 12.0));
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount() ==
            stale_entity_count);
        CHECK(
            rectangle_session.undoDepth() ==
            stale_undo_depth);
        CHECK(
            rectangle_interaction.rectangleStage() ==
            sketch::RectangleStage::
                await_first_corner);

        rectangle_interaction.end();
        rectangle_interaction.begin(
            rectangle_session,
            rectangle_sketch_id);
        CHECK(
            rectangle_interaction.creationRole() ==
            sketch::EntityRole::regular);
        CHECK(
            !rectangle_interaction
                 .rectangleDrawDiagonals());
        rectangle_interaction.end();
    }

    {
        auto structural_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            structural_session{
                {},
                std::move(structural_document)};
        const auto structural_created =
            structural_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            structural_created.ok() &&
            structural_created.sketch_id);
        const auto structural_sketch_id =
            *structural_created.sketch_id;

        const auto boundary =
            structural_session.execute(
                application::AddSketchLineCommand{
                    structural_sketch_id,
                    {4.0, -2.0},
                    {4.0, 2.0}});
        const auto target =
            structural_session.execute(
                application::AddSketchLineCommand{
                    structural_sketch_id,
                    {0.0, 0.0},
                    {10.0, 0.0}});
        CHECK(boundary.ok() && boundary.entity_id);
        CHECK(target.ok() && target.entity_id);

        QTreeWidget structural_tree;
        ui::PartDocumentTreeController
            structural_tree_controller{
                structural_tree};
        TestViewport structural_viewport;
        ui::PartViewportController
            structural_viewport_controller{
                structural_tree_controller,
                &structural_viewport};
        structural_viewport_controller
            .setDocumentSession(
                &structural_session);
        structural_viewport_controller
            .setSketchEditSketch(
                structural_sketch_id);

        ui::PartSketchInteractionController
            structural_interaction{
                structural_viewport_controller};
        structural_interaction.begin(
            structural_session,
            structural_sketch_id);

        CHECK(
            structural_viewport.sketch_scene_
                .lines.size() == 2U);
        const auto boundary_token =
            structural_viewport.sketch_scene_
                .lines[0].token;
        const auto target_token =
            structural_viewport.sketch_scene_
                .lines[1].token;

        structural_viewport.point_query_ = {
            true,
            boundary_token};
        structural_interaction.onPointer(
            pointer(
                structural_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                40.0, 20.0,
                4.0, 0.0));
        structural_interaction.onPointer(
            pointer(
                structural_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_release,
                40.0, 20.0,
                4.0, 0.0));
        CHECK(
            structural_interaction
                .selectedCount() == 1U);

        CHECK(
            structural_interaction.activateTrim());
        CHECK(
            structural_interaction.tool() ==
            sketch::SketchTool::trim);
        CHECK(
            structural_interaction
                .structuralBoundaries()
                .size() == 1U);
        CHECK(
            structural_viewport.cursor_mode_ ==
            viewer::ViewportCursorMode::
                create_edit_crosshair);

        structural_viewport.point_query_ = {
            true,
            target_token};
        structural_interaction.onPointer(
            pointer(
                structural_sketch_id,
                viewer::SpatialPointerPhase::move,
                90.0, 20.0,
                9.0, 0.0));
        CHECK(
            structural_viewport.preview_scene_
                .lines.size() == 1U);

        const auto trim_undo =
            structural_session.undoDepth();
        structural_interaction.onPointer(
            pointer(
                structural_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                90.0, 20.0,
                9.0, 0.0));
        CHECK(
            structural_session.undoDepth() ==
            trim_undo + 1U);
        const auto* trimmed =
            structural_session.document()
                .findSketch(
                    structural_sketch_id)
                ->model.findLine(
                    *target.entity_id);
        CHECK(trimmed != nullptr);
        CHECK((
            trimmed->end() ==
            sketch::Point2{4.0, 0.0}));
        CHECK(
            structural_interaction.tool() ==
            sketch::SketchTool::trim);
        CHECK(
            structural_viewport.preview_scene_
                .lines.empty());

        CHECK(structural_interaction.escape());
        CHECK(
            structural_interaction.tool() ==
            sketch::SketchTool::select);
        CHECK(
            structural_interaction
                .selectedCount() == 1U);
        structural_interaction.end();
    }

    {
        auto staged_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession staged_session{
            {},
            std::move(staged_document)};
        const auto staged_created =
            staged_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(staged_created.ok() && staged_created.sketch_id);
        const auto staged_sketch_id =
            *staged_created.sketch_id;
        const auto boundary =
            staged_session.execute(
                application::AddSketchLineCommand{
                    staged_sketch_id,
                    {4.0, -2.0},
                    {4.0, 2.0}});
        CHECK(boundary.ok() && boundary.entity_id);

        QTreeWidget staged_tree;
        ui::PartDocumentTreeController staged_tree_controller{
            staged_tree};
        TestViewport staged_viewport;
        ui::PartViewportController staged_viewport_controller{
            staged_tree_controller,
            &staged_viewport};
        staged_viewport_controller.setDocumentSession(
            &staged_session);
        staged_viewport_controller.setSketchEditSketch(
            staged_sketch_id);

        ui::PartSketchInteractionController staged_interaction{
            staged_viewport_controller};
        staged_interaction.begin(
            staged_session,
            staged_sketch_id);

        const auto before_state =
            staged_session.document().state();
        const auto before_revision =
            staged_session.document().revision();
        const auto before_undo =
            staged_session.undoDepth();

        CHECK(staged_interaction.activateTrim());
        CHECK(
            staged_interaction
                .structuralBoundarySelectionPending());
        CHECK(staged_interaction.structuralBoundaries().empty());

        CHECK(staged_viewport.sketch_scene_.lines.size() == 1U);
        staged_viewport.point_query_ = {
            true,
            staged_viewport.sketch_scene_.lines.front().token};
        staged_interaction.onPointer(
            pointer(
                staged_sketch_id,
                viewer::SpatialPointerPhase::primary_press,
                40.0, 20.0,
                4.0, 0.0));
        CHECK(staged_interaction.selectedCount() == 1U);
        CHECK(staged_interaction.structuralBoundaries().empty());

        CHECK(
            staged_interaction
                .completeStructuralBoundarySelection());
        CHECK(
            !staged_interaction
                 .structuralBoundarySelectionPending());
        CHECK(
            staged_interaction.structuralBoundaries().size() ==
            1U);
        CHECK(
            staged_session.document().state() ==
            before_state);
        CHECK(
            staged_session.document().revision() ==
            before_revision);
        CHECK(staged_session.undoDepth() == before_undo);
        staged_interaction.end();
    }

    {
        auto mutual_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession mutual_session{
            {},
            std::move(mutual_document)};
        const auto mutual_created =
            mutual_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            mutual_created.ok() &&
            mutual_created.sketch_id);
        const auto mutual_sketch_id =
            *mutual_created.sketch_id;
        const auto first =
            mutual_session.execute(
                application::AddSketchLineCommand{
                    mutual_sketch_id,
                    {0.0, 0.0},
                    {1.0, 0.0}});
        const auto second =
            mutual_session.execute(
                application::AddSketchLineCommand{
                    mutual_sketch_id,
                    {3.0, 2.0},
                    {3.0, 1.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);

        QTreeWidget mutual_tree;
        ui::PartDocumentTreeController
            mutual_tree_controller{mutual_tree};
        TestViewport mutual_viewport;
        ui::PartViewportController
            mutual_viewport_controller{
                mutual_tree_controller,
                &mutual_viewport};
        mutual_viewport_controller
            .setDocumentSession(&mutual_session);
        mutual_viewport_controller
            .setSketchEditSketch(
                mutual_sketch_id);

        ui::PartSketchInteractionController
            mutual_interaction{
                mutual_viewport_controller};
        mutual_interaction.begin(
            mutual_session,
            mutual_sketch_id);
        CHECK(
            mutual_viewport.sketch_scene_
                .lines.size() == 2U);
        const auto first_token =
            mutual_viewport.sketch_scene_
                .lines[0].token;
        const auto second_token =
            mutual_viewport.sketch_scene_
                .lines[1].token;

        mutual_interaction.activateExtendBoth();
        CHECK(
            mutual_interaction.tool() ==
            sketch::SketchTool::extend_both);

        mutual_viewport.point_query_ = {
            true,
            first_token};
        mutual_interaction.onPointer(
            pointer(
                mutual_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                10.0, 10.0,
                1.0, 0.0));
        CHECK(
            mutual_interaction
                .extendBothFirstLine() ==
            *first.entity_id);

        mutual_viewport.point_query_ = {
            true,
            second_token};
        mutual_interaction.onPointer(
            pointer(
                mutual_sketch_id,
                viewer::SpatialPointerPhase::move,
                30.0, 10.0,
                3.0, 1.0));
        CHECK(
            mutual_viewport.preview_scene_
                .lines.size() == 2U);

        const auto mutual_undo =
            mutual_session.undoDepth();
        mutual_interaction.onPointer(
            pointer(
                mutual_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                30.0, 10.0,
                3.0, 1.0));
        CHECK(
            mutual_session.undoDepth() ==
            mutual_undo + 1U);
        CHECK(
            !mutual_interaction
                 .extendBothFirstLine());
        const auto* mutual_sketch =
            mutual_session.document()
                .findSketch(mutual_sketch_id);
        CHECK(mutual_sketch != nullptr);
        CHECK((
            mutual_sketch->model
                .findLine(*first.entity_id)
                ->end() ==
            sketch::Point2{3.0, 0.0}));
        CHECK((
            mutual_sketch->model
                .findLine(*second.entity_id)
                ->end() ==
            sketch::Point2{3.0, 0.0}));
        CHECK(
            mutual_viewport.preview_scene_
                .lines.empty());
        mutual_interaction.end();
    }

    {
        // R12 Circle Trim UI path: finite Line boundary selection drives the
        // removed-span preview, then commits one fresh Arc replacement.
        auto circle_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession circle_session{
            {},
            std::move(circle_document)};
        const auto circle_created =
            circle_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            circle_created.ok() &&
            circle_created.sketch_id);
        const auto circle_sketch_id =
            *circle_created.sketch_id;

        const auto target_circle =
            circle_session.execute(
                application::AddSketchCircleCommand{
                    circle_sketch_id,
                    {0.0, 0.0},
                    10.0,
                    sketch::EntityRole::construction});
        const auto circle_boundary =
            circle_session.execute(
                application::AddSketchLineCommand{
                    circle_sketch_id,
                    {-20.0, 0.0},
                    {20.0, 0.0}});
        CHECK(
            target_circle.ok() &&
            target_circle.entity_id);
        CHECK(
            circle_boundary.ok() &&
            circle_boundary.entity_id);

        QTreeWidget circle_tree;
        ui::PartDocumentTreeController
            circle_tree_controller{circle_tree};
        TestViewport circle_viewport;
        ui::PartViewportController
            circle_viewport_controller{
                circle_tree_controller,
                &circle_viewport};
        circle_viewport_controller
            .setDocumentSession(&circle_session);
        circle_viewport_controller
            .setSketchEditSketch(circle_sketch_id);

        ui::PartSketchInteractionController
            circle_interaction{
                circle_viewport_controller};
        circle_interaction.begin(
            circle_session,
            circle_sketch_id);

        CHECK(
            circle_viewport.sketch_scene_
                .lines.size() == 1U);
        CHECK(
            circle_viewport.sketch_scene_
                .curves.size() == 1U);
        const auto boundary_token =
            circle_viewport.sketch_scene_
                .lines.front().token;
        const auto circle_token =
            circle_viewport.sketch_scene_
                .curves.front().token;

        circle_viewport.point_query_ = {
            true,
            boundary_token};
        circle_interaction.onPointer(
            pointer(
                circle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                50.0, 50.0,
                0.0, 0.0));
        circle_interaction.onPointer(
            pointer(
                circle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_release,
                50.0, 50.0,
                0.0, 0.0));
        CHECK(
            circle_interaction.selectedCount() ==
            1U);
        CHECK(circle_interaction.activateTrim());

        circle_viewport.point_query_ = {
            true,
            circle_token};
        circle_interaction.onPointer(
            pointer(
                circle_sketch_id,
                viewer::SpatialPointerPhase::move,
                50.0, 90.0,
                0.0, -10.0));
        CHECK(
            !circle_viewport.preview_scene_
                 .lines.empty());

        const auto circle_undo =
            circle_session.undoDepth();
        circle_interaction.onPointer(
            pointer(
                circle_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                50.0, 90.0,
                0.0, -10.0));
        CHECK(
            circle_session.undoDepth() ==
            circle_undo + 1U);
        const auto* circle_after =
            circle_session.document()
                .findSketch(circle_sketch_id);
        CHECK(circle_after != nullptr);
        CHECK(
            circle_after->model.findCircle(
                *target_circle.entity_id) ==
            nullptr);
        CHECK(
            circle_after->model.entityCount() ==
            2U);
        const auto circle_state =
            circle_after->model.state();
        CHECK(circle_state.arcs.size() == 1U);
        const auto replacement =
            circle_state.arcs.front().id;
        CHECK(
            replacement !=
            *target_circle.entity_id);
        const auto* replacement_arc =
            circle_after->model.findArc(
                replacement);
        CHECK(replacement_arc != nullptr);
        CHECK(
            replacement_arc->role() ==
            sketch::EntityRole::construction);
        CHECK(
            circle_viewport.preview_scene_
                .lines.empty());

        CHECK(circle_session.undo().changed);
        const auto* circle_undone =
            circle_session.document()
                .findSketch(circle_sketch_id);
        CHECK(circle_undone != nullptr);
        CHECK(
            circle_undone->model.findCircle(
                *target_circle.entity_id) !=
            nullptr);
        CHECK(
            circle_undone->model.findArc(
                replacement) == nullptr);
        circle_interaction.end();
    }

    {
        // R12 ordinary Extend UI path: the selected finite boundary stays
        // unchanged while the clicked target end previews and commits.
        auto extend_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession extend_session{
            {},
            std::move(extend_document)};
        const auto extend_created =
            extend_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            extend_created.ok() &&
            extend_created.sketch_id);
        const auto extend_sketch_id =
            *extend_created.sketch_id;

        const auto extend_boundary =
            extend_session.execute(
                application::AddSketchLineCommand{
                    extend_sketch_id,
                    {5.0, -2.0},
                    {5.0, 2.0},
                    sketch::EntityRole::construction});
        const auto extend_target =
            extend_session.execute(
                application::AddSketchLineCommand{
                    extend_sketch_id,
                    {0.0, 0.0},
                    {1.0, 0.0}});
        CHECK(
            extend_boundary.ok() &&
            extend_boundary.entity_id);
        CHECK(
            extend_target.ok() &&
            extend_target.entity_id);

        QTreeWidget extend_tree;
        ui::PartDocumentTreeController
            extend_tree_controller{extend_tree};
        TestViewport extend_viewport;
        ui::PartViewportController
            extend_viewport_controller{
                extend_tree_controller,
                &extend_viewport};
        extend_viewport_controller
            .setDocumentSession(&extend_session);
        extend_viewport_controller
            .setSketchEditSketch(extend_sketch_id);

        ui::PartSketchInteractionController
            extend_interaction{
                extend_viewport_controller};
        extend_interaction.begin(
            extend_session,
            extend_sketch_id);

        CHECK(
            extend_viewport.sketch_scene_
                .lines.size() == 2U);
        const auto extend_boundary_token =
            extend_viewport.sketch_scene_
                .lines[0].token;
        const auto extend_target_token =
            extend_viewport.sketch_scene_
                .lines[1].token;

        extend_viewport.point_query_ = {
            true,
            extend_boundary_token};
        extend_interaction.onPointer(
            pointer(
                extend_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                50.0, 20.0,
                5.0, 0.0));
        extend_interaction.onPointer(
            pointer(
                extend_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_release,
                50.0, 20.0,
                5.0, 0.0));
        CHECK(
            extend_interaction.selectedCount() ==
            1U);
        CHECK(extend_interaction.activateExtend());

        extend_viewport.point_query_ = {
            true,
            extend_target_token};
        extend_interaction.onPointer(
            pointer(
                extend_sketch_id,
                viewer::SpatialPointerPhase::move,
                10.0, 20.0,
                1.0, 0.0));
        CHECK(
            extend_viewport.preview_scene_
                .lines.size() == 1U);

        const auto extend_undo =
            extend_session.undoDepth();
        extend_interaction.onPointer(
            pointer(
                extend_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                10.0, 20.0,
                1.0, 0.0));
        CHECK(
            extend_session.undoDepth() ==
            extend_undo + 1U);

        const auto* extend_after =
            extend_session.document()
                .findSketch(extend_sketch_id);
        CHECK(extend_after != nullptr);
        const auto* extended_line =
            extend_after->model.findLine(
                *extend_target.entity_id);
        CHECK(extended_line != nullptr);
        CHECK(near(
            extended_line->end().u,
            5.0));
        CHECK(near(
            extended_line->end().v,
            0.0));
        const auto* boundary_line =
            extend_after->model.findLine(
                *extend_boundary.entity_id);
        CHECK(boundary_line != nullptr);
        CHECK((
            boundary_line->start() ==
            sketch::Point2{5.0, -2.0}));
        CHECK((
            boundary_line->end() ==
            sketch::Point2{5.0, 2.0}));
        CHECK(
            extend_interaction.tool() ==
            sketch::SketchTool::extend);
        CHECK(
            extend_viewport.preview_scene_
                .lines.empty());
        extend_interaction.end();
    }

    return EXIT_SUCCESS;
}
