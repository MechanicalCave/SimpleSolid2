#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>
#include <simplesolid2/sketch/measurement.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
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

class EchoConvergedRelationSolver final
    : public sketch::ILocalSketchSolver {
public:
    int calls{};
    std::function<void()> on_solve;

    [[nodiscard]] sketch::LocalBackendSolveResult solve(
        const sketch::LocalSolveProblem& problem) override {
        ++calls;
        if (on_solve) on_solve();
        // Valid only for the already-horizontal controller fixture. The
        // production acceptance gate must still review every residual.
        return {sketch::LocalBackendStatus::converged,
                problem.authored, {}, {}, std::nullopt};
    }
};

class RejectedRelationSolver final
    : public sketch::ILocalSketchSolver {
public:
    sketch::LocalBackendStatus status{
        sketch::LocalBackendStatus::conflict};
    int calls{};

    [[nodiscard]] sketch::LocalBackendSolveResult solve(
        const sketch::LocalSolveProblem&) override {
        ++calls;
        // Valid diagnostic-only backend response; no accepted candidate.
        return {status, std::nullopt, {}, {}, std::nullopt};
    }
};

class NoopKernel final
    : public kernel::ISolidModelingKernel {
public:
    [[nodiscard]] kernel::SolidModelingResult
    extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle) noexcept override {
        return {};
    }
};

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

    std::optional<viewer::ViewportPoint2> projectWorldPoint(
        viewer::Point3 point) const override {
        if (!enable_point_projection_ ||
            !viewer::finite(point)) return std::nullopt;
        if (on_world_projection_) on_world_projection_();
        // Stub projective frame is explicit, deterministic and opt-in:
        // 10 screen logical pixels per unit in the Sketch XY plane.
        return viewer::ViewportPoint2{
            100.0 + point.x * 10.0,
            100.0 + point.y * 10.0};
    }

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
        if (on_interaction_presentation_) on_interaction_presentation_();
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
        if (on_point_query_) on_point_query_();
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

    bool enable_point_projection_{false};
    std::function<void()> on_world_projection_;
    viewer::CameraState camera_;
    viewer::SketchScene sketch_scene_;
    viewer::ProfileScene profile_scene_;
    viewer::ProfilePreviewScene profile_preview_scene_;
    viewer::SketchPreviewScene preview_scene_;
    viewer::SketchGripScene grip_scene_;
    viewer::SketchInteractionPresentation
        interaction_presentation_;
    std::function<void()> on_interaction_presentation_;
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
    std::function<void()> on_point_query_;
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

    // Preserve diagnostics if Profile Finish rejects a formerly
    // supported ordinary authored-only Sketch.
    std::string profile_finish_diagnostic;
    interaction.setStatusHandler(
        [&profile_finish_diagnostic](const std::string& message) {
            profile_finish_diagnostic = message;
        });
    const bool profile_finished = interaction.finishProfile();
    if (!profile_finished) {
        const auto current_status =
            interaction.profileDraftResolutionStatus();
        const auto* source =
            session.document().findSketch(sketch_id);
        std::cerr
            << "SK04C_PROFILE_FINISH_REJECTED"
            << " diagnostic=" << profile_finish_diagnostic
            << " linked=" << (source && !source->projection_bindings.empty())
            << " draft_valid=" << interaction.profileDraftValid()
            << " resolution=" << (current_status
                ? static_cast<int>(*current_status)
                : -1)
            << std::endl;
    }
    CHECK(profile_finished);
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

    sketch::resetRegionAnalysisInvocationCount();
    interaction.onPointer(pointer(
        sketch_id,
        viewer::SpatialPointerPhase::move,
        303.0, 303.0,
        102.0, 102.0));
    CHECK(
        sketch::regionAnalysisInvocationCount() ==
        0U);
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
        auto axis_line_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            axis_line_session{
                std::filesystem::path{
                    "pm04f-line-axis.ss2part"},
                std::move(axis_line_document)};
        const auto axis_line_created =
            axis_line_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            axis_line_created.ok() &&
            axis_line_created.sketch_id);
        const auto axis_line_sketch_id =
            *axis_line_created.sketch_id;

        QTreeWidget axis_line_tree;
        ui::PartDocumentTreeController
            axis_line_tree_controller{
                axis_line_tree};
        TestViewport axis_line_viewport;
        ui::PartViewportController
            axis_line_viewport_controller{
                axis_line_tree_controller,
                &axis_line_viewport};
        axis_line_viewport_controller
            .setDocumentSession(
                &axis_line_session);
        axis_line_viewport_controller
            .setSketchEditSketch(
                axis_line_sketch_id);

        NoopKernel axis_line_kernel;
        ui::PartSketchInteractionController
            axis_line_interaction{
                axis_line_viewport_controller};
        axis_line_interaction
            .setSolidModelingKernel(
                &axis_line_kernel);
        axis_line_interaction.begin(
            axis_line_session,
            axis_line_sketch_id);
        axis_line_interaction.activateLine();
        CHECK(
            axis_line_interaction.setCreationRole(
                sketch::EntityRole::construction));
        CHECK(
            axis_line_interaction
                .setLineAxisDesignation(true));
        CHECK(
            axis_line_interaction
                .lineAxisDesignation());

        const auto axis_line_undo_before =
            axis_line_session.undoDepth();
        axis_line_interaction.onPointer(
            pointer(
                axis_line_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                10.0, 10.0,
                0.0, 0.0));
        axis_line_interaction.onPointer(
            pointer(
                axis_line_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                10.0, 10.0,
                0.0, 0.0));
        CHECK(
            axis_line_interaction
                .lineAxisDesignation());
        CHECK(
            axis_line_session.document()
                .axes().empty());
        CHECK(
            axis_line_session.undoDepth() ==
            axis_line_undo_before);

        axis_line_interaction.onPointer(
            pointer(
                axis_line_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                60.0, 10.0,
                5.0, 0.0));
        CHECK(
            !axis_line_interaction
                 .lineAxisDesignation());
        CHECK(
            axis_line_session.undoDepth() ==
            axis_line_undo_before + 1U);
        CHECK(
            axis_line_session.document()
                .axes().size() == 1U);
        CHECK(
            axis_line_session.document()
                .findSketch(
                    axis_line_sketch_id)
                ->model.state().lines.size() ==
            1U);
        const auto first_axis_id =
            axis_line_session.document()
                .axes().front().id;
        const auto first_axis_source =
            axis_line_session.document()
                .axes().front().source;
        // This existing Sketch interaction fixture must continue
        // observing the real Sketch-Line source after PartAxis gains
        // the separate, strictly semantic Body Edge source variant.
        const auto* first_sketch_axis_source =
            std::get_if<part::SketchLineAxisSource>(
                &first_axis_source);
        CHECK(first_sketch_axis_source != nullptr);
        CHECK(
            axis_line_session.document()
                .findSketch(
                    axis_line_sketch_id)
                ->model.findLine(
                    first_sketch_axis_source->entity_id)
                ->role() ==
            sketch::EntityRole::construction);

        axis_line_interaction.onPointer(
            pointer(
                axis_line_sketch_id,
                viewer::SpatialPointerPhase::
                    primary_press,
                110.0, 10.0,
                10.0, 0.0));
        CHECK(
            axis_line_session.document()
                .findSketch(
                    axis_line_sketch_id)
                ->model.state().lines.size() ==
            2U);
        CHECK(
            axis_line_session.document()
                .axes().size() == 1U);
        CHECK(
            axis_line_session.undoDepth() ==
            axis_line_undo_before + 2U);

        axis_line_interaction.cancelForHistory();
        CHECK(axis_line_session.undo().ok());
        CHECK(
            axis_line_session.document()
                .axes().size() == 1U);
        CHECK(axis_line_session.undo().ok());
        CHECK(
            axis_line_session.document()
                .axes().empty());
        CHECK(
            axis_line_session.document()
                .findSketch(
                    axis_line_sketch_id)
                ->model.state().lines.empty());

        CHECK(axis_line_session.redo().ok());
        CHECK(
            axis_line_session.document()
                .findAxis(first_axis_id) !=
            nullptr);
        CHECK(
            axis_line_session.document()
                .findAxis(first_axis_id)
                ->source ==
            first_axis_source);
        CHECK(
            axis_line_session.document()
                .findSketch(
                    axis_line_sketch_id)
                ->model.findLine(
                    first_sketch_axis_source->entity_id) !=
            nullptr);

        axis_line_interaction.end();
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

    // R13-A2.0 native controller proves command-first Coincident does NOT
    // depend on preselected/visible editable grips and never calls an
    // unsolved Part Add command. Screen projection is opt-in in this stub.
    {
        auto a2_document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession a2_session{
            {}, std::move(a2_document)};
        const auto created_a2 = a2_session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created_a2.ok() && created_a2.sketch_id);
        const auto sid = *created_a2.sketch_id;
        const auto a = a2_session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto b = a2_session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(a.ok() && a.entity_id);
        CHECK(b.ok() && b.entity_id);
        QTreeWidget a2_tree;
        ui::PartDocumentTreeController a2_tree_controller{a2_tree};
        TestViewport a2_viewport;
        a2_viewport.enable_point_projection_ = true;
        ui::PartViewportController a2_viewport_controller{
            a2_tree_controller, &a2_viewport};
        a2_viewport_controller.setDocumentSession(&a2_session);
        a2_viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController a2_interaction{
            a2_viewport_controller};
        std::string a2_status;
        a2_interaction.setStatusHandler(
            [&a2_status](const std::string& message) {
                a2_status = message;
            });
        a2_interaction.begin(a2_session, sid);
        CHECK(a2_interaction.selectedCount() == 0U);
        a2_interaction.activateRelations();
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::coincident));
        const auto original_revision =
            a2_session.document().revision();
        const auto original_undo = a2_session.undoDepth();
        const auto original_sketch =
            a2_session.document().findSketch(sid)->model.state();

        // End of first Line is at (150,100) in test screen frame.
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            150.0, 100.0, 5.0, 0.0));
        CHECK(a2_interaction.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *a.entity_id, sketch::ConstraintElement::line_end}));
        // Mid-line body click is not silently converted into Start/End.
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(a2_interaction.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *a.entity_id, sketch::ConstraintElement::line_end}));
        // Second endpoint completes ONLY a semantic request: no write.
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            200.0, 100.0, 10.0, 0.0));
        CHECK(!a2_interaction.firstRelationTarget());
        CHECK(a2_interaction.relationKind() ==
              sketch::ConstraintKind::coincident);
        CHECK(a2_session.document().revision() == original_revision);
        CHECK(a2_session.undoDepth() == original_undo);
        CHECK(a2_session.document().findSketch(sid)->model.state() ==
              original_sketch);

        // One body click is sufficient for Horizontal target capture,
        // even without a preselected Line, but not a solver commit.
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        CHECK(!a2_viewport.sketch_scene_.lines.empty());
        a2_viewport.point_query_ = {
            true, a2_viewport.sketch_scene_.lines.front().token};
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(a2_session.document().revision() == original_revision);
        CHECK(a2_session.undoDepth() == original_undo);
        CHECK(a2_session.document().findSketch(sid)->model.state() ==
              original_sketch);

        // Selection-first: two previously selected local Lines fill the
        // Parallel transient intent when the kind is selected, without
        // another click, solver bypass or Part Undo entry.
        a2_interaction.activateSelect();
        CHECK(a2_viewport.sketch_scene_.lines.size() == 2U);
        a2_viewport.point_query_ = {
            true, a2_viewport.sketch_scene_.lines[0].token};
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            125.0, 100.0, 2.5, 0.0));
        a2_viewport.point_query_ = {
            true, a2_viewport.sketch_scene_.lines[1].token};
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            225.0, 100.0, 12.5, 0.0));
        CHECK(a2_interaction.selectedCount() == 2U);
        a2_interaction.activateRelations();
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(a2_interaction.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(!a2_interaction.firstRelationTarget());
        CHECK(a2_session.document().revision() == original_revision);
        CHECK(a2_session.undoDepth() == original_undo);
        CHECK(a2_session.document().findSketch(sid)->model.state() ==
              original_sketch);

        // Preselected line identities do not determine Coincident
        // endpoint roles; they may not be silently guessed.
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::coincident));
        CHECK(!a2_interaction.firstRelationTarget());

        // RED-FIRST: connect already acquired native viewport / Operations
        // semantic targets to the guarded solver-backed Part writer ONLY
        // when a development backend is explicitly injected. Default
        // Windows application has no approved production backend.
        EchoConvergedRelationSolver backend;
        a2_interaction.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        a2_interaction.activateRelations();
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        a2_viewport.point_query_ = {
            true, a2_viewport.sketch_scene_.lines.front().token};
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(a2_interaction.relationKind() ==
              sketch::ConstraintKind::horizontal);
        // A legitimate commit advances Part revision but must still
        // refresh and acknowledge the current Operations UI context.
        CHECK(a2_status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(a2_session.document().revision() != original_revision);
        CHECK(a2_session.undoDepth() == original_undo + 1U);
        const auto after_click =
            a2_session.document().findSketch(sid)->model.state();
        CHECK(after_click.constraints.size() == 1U);
        CHECK(after_click.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        CHECK(after_click.lines == original_sketch.lines);

        // Duplicate creation fails before a second backend invocation,
        // with neither a revision nor Undo/ConstraintId side effect.
        const auto committed_revision =
            a2_session.document().revision();
        // A successful Part transaction rebuilds the native Viewer scene.
        // Requery the current-generation semantic token rather than
        // replaying a stale presentation hit from before that transaction.
        CHECK(!a2_viewport.sketch_scene_.lines.empty());
        a2_viewport.point_query_ = {
            true, a2_viewport.sketch_scene_.lines.front().token};
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(a2_session.document().revision() == committed_revision);
        CHECK(a2_session.undoDepth() == original_undo + 1U);
        CHECK(a2_session.document().findSketch(sid)->model.state() ==
              after_click);
        CHECK(a2_status.find("redundant") != std::string::npos);
        CHECK(a2_status.find("No command accepted") != std::string::npos);

        // Selection-first with two existing local Line identities commits
        // the same validated command without a redundant viewport click.
        a2_interaction.activateRelations();
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(backend.calls == 2);
        CHECK(a2_session.undoDepth() == original_undo + 2U);
        const auto after_selection =
            a2_session.document().findSketch(sid)->model.state();
        CHECK(after_selection.constraints.size() == 2U);
        CHECK(after_selection.constraints.back().kind ==
              sketch::ConstraintKind::parallel);
        CHECK(after_selection.lines == original_sketch.lines);
        CHECK(a2_session.undo().ok());
        CHECK(a2_session.document().findSketch(sid)->model.state()
              .constraints.size() == 1U);
        CHECK(a2_session.redo().ok());
        CHECK(a2_session.document().findSketch(sid)->model.state()
              .constraints == after_selection.constraints);

        // RED-FIRST borrowed-backend lifecycle: end/begin may switch the
        // runtime Part incarnation while the old external DEV solver is
        // destroyed. A newly opened editing context MUST NOT reuse its
        // previous registration, even on equal durable Sketch/Part IDs.
        a2_interaction.end();
        a2_interaction.begin(a2_session, sid);
        a2_interaction.activateRelations();
        CHECK(a2_interaction.selectRelationKind(
            sketch::ConstraintKind::fix));
        CHECK(a2_viewport.sketch_scene_.lines.size() == 2U);
        a2_viewport.point_query_ = {
            true, a2_viewport.sketch_scene_.lines[1].token};
        const auto before_new_context =
            a2_session.document().findSketch(sid)->model.state();
        const auto revision_before_new_context =
            a2_session.document().revision();
        const auto undo_before_new_context = a2_session.undoDepth();
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(backend.calls == 2);
        CHECK(a2_session.document().revision() ==
              revision_before_new_context);
        CHECK(a2_session.undoDepth() == undo_before_new_context);
        CHECK(a2_session.document().findSketch(sid)->model.state() ==
              before_new_context);

        // A new edit context may explicitly register a new live DEV
        // backend. Fix Current Position must then take the dedicated
        // two-solve guarded Part transaction, never unsolved A1 authoring.
        a2_interaction.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        a2_interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(backend.calls == 4);
        const auto after_fix =
            a2_session.document().findSketch(sid)->model.state();
        CHECK(after_fix.constraints.size() == 3U);
        CHECK(after_fix.constraints.back().kind ==
              sketch::ConstraintKind::fix);
        CHECK(after_fix.constraints.back().first.entity ==
              *b.entity_id);
        CHECK(after_fix.lines == original_sketch.lines);
        CHECK(a2_session.undoDepth() == undo_before_new_context + 1U);
        CHECK(a2_session.undo().ok());
        const auto after_fix_undo =
            a2_session.document().findSketch(sid)->model.state();
        auto expected_fix_undo = before_new_context;
        // A1 requires monotonic ConstraintId high-water even across Undo.
        // Only authored geometry/relations rewind; the retired ID cursor
        // must not be made available for reuse.
        expected_fix_undo.next_constraint_id =
            after_fix.next_constraint_id;
        CHECK(after_fix_undo == expected_fix_undo);
        CHECK(a2_session.redo().ok());
        CHECK(a2_session.document().findSketch(sid)->model.state()
              .constraints == after_fix.constraints);
        a2_interaction.end();
    }

    // R13-A2.0: a numerical rejection is a TYPED, actionable Operations
    // status, not a generic "semantic or numeric validation" message.
    // Both cases keep the tool active and leave Part/Undo/ID untouched.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 2.0}});
        CHECK(line.ok() && line.entity_id);

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        RejectedRelationSolver solver;
        controller.setDevelopmentRelationSolver(
            &solver, {1.0e-7, 1.0e-6});
        controller.activateRelations();

        const auto initial_revision = session.document().revision();
        const auto initial_undo = session.undoDepth();
        const auto initial_state =
            session.document().findSketch(sid)->model.state();

        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        CHECK(viewport.sketch_scene_.lines.size() == 1U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines.front().token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 110.0, 2.5, 1.0));
        CHECK(solver.calls == 1);
        CHECK(status.find("conflict") != std::string::npos);
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(session.document().revision() == initial_revision);
        CHECK(session.undoDepth() == initial_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              initial_state);

        solver.status = sketch::LocalBackendStatus::unsupported;
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::vertical));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 110.0, 2.5, 1.0));
        CHECK(solver.calls == 2);
        CHECK(status.find("unsupported") != std::string::npos);
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::vertical);
        CHECK(session.document().revision() == initial_revision);
        CHECK(session.undoDepth() == initial_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              initial_state);
        controller.end();
    }

    // R13-A2.0: a two-click semantic gesture is revision-bound.
    // A different Part command may update only an unrelated Circle, leaving
    // both Line geometries/roles identical. Accepting the old first hit
    // against the new revision would silently retarget the user's gesture.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& text) { status = text; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *first.entity_id, sketch::ConstraintElement::line}));
        CHECK(backend.calls == 0);

        // Unrelated Part authoring changes the revision but neither picked
        // Line nor its geometry, semantic role, or durable EntityId.
        const auto circle = session.execute(
            application::AddSketchCircleCommand{
                sid, {30.0, 30.0}, 2.0});
        CHECK(circle.ok() && circle.entity_id);
        viewport_controller.refreshPresentation();
        const auto updated_revision = session.document().revision();
        const auto updated_undo = session.undoDepth();
        const auto updated_state =
            session.document().findSketch(sid)->model.state();
        CHECK(updated_state.constraints.empty());
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(!controller.firstRelationTarget());
        CHECK(status.find("stale") != std::string::npos);
        CHECK(backend.calls == 0);
        CHECK(session.document().revision() == updated_revision);
        CHECK(session.undoDepth() == updated_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              updated_state);
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);

        // The same mode can begin a fresh two-hit gesture on the current
        // revision and commit exactly once through the guarded solver.
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(controller.firstRelationTarget());
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == updated_undo + 1U);
        const auto after =
            session.document().findSketch(sid)->model.state();
        CHECK(after.constraints.size() == 1U);
        CHECK(after.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        CHECK(after.circles == updated_state.circles);
        controller.end();
    }

    // R13-A2.0 selection-first reentrancy: an owning UI callback may
    // commit an unrelated Part edit between first-target acceptance and
    // display refresh. That callback must not grant the OLD pick the
    // newly incremented revision (or silently authorize later commit).
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && second.ok());
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateSelect();
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            125.0, 100.0, 2.5, 0.0));
        CHECK(controller.selectedCount() == 1U);
        controller.activateRelations();

        const auto before_selection = session.document().revision();
        bool injected = false;
        controller.setStateChangedHandler([&] {
            if (injected || !controller.firstRelationTarget()) return;
            injected = true;
            const auto circle = session.execute(
                application::AddSketchCircleCommand{
                    sid, {30.0, 30.0}, 2.0});
            CHECK(circle.ok() && circle.entity_id);
            viewport_controller.refreshPresentation();
        });
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(injected);
        CHECK(session.document().revision() != before_selection);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(!controller.firstRelationTarget());
        CHECK(backend.calls == 0);
        const auto revised = session.document().revision();
        const auto undo_depth = session.undoDepth();
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.empty());
        CHECK(state.circles.size() == 1U);
        CHECK(session.document().revision() == revised);
        CHECK(session.undoDepth() == undo_depth);
        controller.end();
    }

    // R13-A2.0: native pick provider reentrancy cannot upgrade a
    // pointer begun on an older Part revision. The provider may invoke
    // application callbacks while resolving a hit and even return a
    // valid NEW-scene token after rebuilding that scene.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        CHECK(viewport.sketch_scene_.lines.size() == 1U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines.front().token};

        const auto before = session.document().revision();
        bool injected = false;
        viewport.on_point_query_ = [&] {
            if (injected) return;
            injected = true;
            const auto circle = session.execute(
                application::AddSketchCircleCommand{
                    sid, {30.0, 30.0}, 2.0});
            CHECK(circle.ok() && circle.entity_id);
            viewport_controller.refreshPresentation();
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            // Not a stale token: the provider returns an exact current
            // hit, but the user's gesture started at the old revision.
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines.front().token};
        };
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(injected);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(backend.calls == 0);
        CHECK(!controller.firstRelationTarget());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(session.document().revision() != before);
        const auto after_injection =
            session.document().findSketch(sid)->model.state();
        CHECK(after_injection.constraints.empty());
        CHECK(after_injection.circles.size() == 1U);
        const auto stable_revision = session.document().revision();
        const auto stable_undo = session.undoDepth();

        // With the provider callback disabled, a NEW current-stage
        // click is valid and authors one guarded revision/Undo.
        viewport.on_point_query_ = {};
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines.front().token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == stable_undo + 1U);
        CHECK(session.document().revision() != stable_revision);
        const auto accepted =
            session.document().findSketch(sid)->model.state();
        CHECK(accepted.constraints.size() == 1U);
        CHECK(accepted.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        CHECK(accepted.circles == after_injection.circles);
        controller.end();
    }

    // R13-A2.0 Coincident uses projected semantic Line endpoints rather
    // than Viewer Line hit tokens. Even this provider callback can reenter
    // an independent Part transaction while an old SketchModel pointer
    // exists in the controller. Reject before dereferencing old topology.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        viewport.enable_point_projection_ = true;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::coincident));

        const auto before_revision = session.document().revision();
        bool injected = false;
        viewport.on_world_projection_ = [&] {
            if (injected) return;
            injected = true;
            const auto circle = session.execute(
                application::AddSketchCircleCommand{
                    sid, {30.0, 30.0}, 2.0});
            CHECK(circle.ok() && circle.entity_id);
            viewport_controller.refreshPresentation();
        };
        // First Line End projects to (150,100); provider reenters before
        // semantic endpoint hit resolution finishes.
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            150.0, 100.0, 5.0, 0.0));
        CHECK(injected);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(!controller.firstRelationTarget());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::coincident);
        CHECK(backend.calls == 0);
        CHECK(session.document().revision() != before_revision);
        const auto after_injection =
            session.document().findSketch(sid)->model.state();
        CHECK(after_injection.constraints.empty());
        CHECK(after_injection.circles.size() == 1U);

        // A new hit against the current stage is still a valid FIRST
        // Coincident endpoint, without authoring until second semantic hit.
        viewport.on_world_projection_ = {};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            150.0, 100.0, 5.0, 0.0));
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *first.entity_id,
                  sketch::ConstraintElement::line_end}));
        CHECK(backend.calls == 0);
        CHECK(session.document().findSketch(sid)->model.state() ==
              after_injection);
        controller.end();
    }

    // R13-A2.0: selection-first is already a semantic Part-stage gesture
    // when the Relations type is clicked. A stateChanged callback in the
    // FIRST notifyStateChanged() must not renew an old two-Line selection
    // onto an unrelated newly committed Part revision.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        controller.activateSelect();
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            125.0, 100.0, 2.5, 0.0));
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            225.0, 100.0, 12.5, 0.0));
        CHECK(controller.selectedCount() == 2U);
        controller.activateRelations();
        EchoConvergedRelationSolver solver;
        controller.setDevelopmentRelationSolver(
            &solver, {1.0e-7, 1.0e-6});

        const auto start_revision = session.document().revision();
        bool injected = false;
        controller.setStateChangedHandler([&] {
            if (injected ||
                controller.relationKind() !=
                    sketch::ConstraintKind::parallel ||
                controller.firstRelationTarget()) {
                return;
            }
            injected = true;
            const auto circle = session.execute(
                application::AddSketchCircleCommand{
                    sid, {30.0, 30.0}, 2.0});
            CHECK(circle.ok() && circle.entity_id);
            viewport_controller.refreshPresentation();
        });
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(injected);
        CHECK(session.document().revision() != start_revision);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(!controller.firstRelationTarget());
        CHECK(solver.calls == 0);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.empty());
        CHECK(state.circles.size() == 1U);

        // After the stale command is rejected, the same two durable
        // selected Line IDs remain eligible for a NEW selection-first
        // command on the current revision. No extra Apply/click is needed.
        const auto revision_before_retry =
            session.document().revision();
        const auto undo_before_retry = session.undoDepth();
        controller.setStateChangedHandler({});
        CHECK(controller.selectedCount() == 2U);
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(solver.calls == 1);
        CHECK(session.document().revision() != revision_before_retry);
        CHECK(session.undoDepth() == undo_before_retry + 1U);
        const auto after_retry =
            session.document().findSketch(sid)->model.state();
        CHECK(after_retry.constraints.size() == 1U);
        CHECK(after_retry.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        CHECK(after_retry.circles == state.circles);
        controller.end();
    }

    // R13-A2.0: the active development solver can be withdrawn while
    // a numeric solve is in flight. The old borrowed reference must not
    // authorize that result on an otherwise unchanged Part revision;
    // explicit re-registration may authorize a subsequent fresh click.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();
        const auto before_state =
            session.document().findSketch(sid)->model.state();
        CHECK(viewport.sketch_scene_.lines.size() == 1U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        backend.on_solve = [&] {
            // Revoke an already-borrowed solver while solve() runs.
            // This must not authorize the in-flight relation.
            controller.setDevelopmentRelationSolver(
                nullptr, {1.0e-7, 1.0e-6});
        };
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(session.document().revision() == before_revision);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              before_state);

        // ABA regression: the callback registers the EXACT SAME backend
        // address again during solve. Pointer identity alone is not
        // sufficient authority; the transient UI epoch must invalidate
        // this in-flight solve without spending revision/Undo/ConstraintId.
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        backend.on_solve = [&] {
            controller.setDevelopmentRelationSolver(
                &backend, {1.0e-7, 1.0e-6});
        };
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 2);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(session.document().revision() == before_revision);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              before_state);

        // Same-document/context ABA: the backend synchronously ends the
        // edit and begins an edit of this SAME Sketch, then explicitly
        // registers the SAME solver pointer and selects SAME relation.
        // IDs/revision/semantic kind all match again; only the transient
        // controller generation distinguishes the two edit sessions.
        backend.on_solve = [&] {
            controller.end();
            controller.begin(session, sid);
            controller.setDevelopmentRelationSolver(
                &backend, {1.0e-7, 1.0e-6});
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
        };
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 3);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(session.document().revision() == before_revision);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              before_state);

        // Positive control: same Part/geometry, now with a stable UI
        // context, must genuinely pass solver and commit the same intent.
        backend.on_solve = {};
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(backend.calls == 4);
        CHECK(session.undoDepth() == before_undo + 1U);
        CHECK(session.document().revision() != before_revision);
        const auto valid =
            session.document().findSketch(sid)->model.state();
        CHECK(valid.constraints.size() == 1U);
        CHECK(valid.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        controller.end();
    }

    // R13-A2.0 native picking ABA: querySketchPresentation may call
    // end()/begin() for the SAME Sketch, then re-register the very SAME
    // backend and relation kind. DocumentId, Part revision, incarnation,
    // semantic target and solver address are all identical again, but the
    // old pointer press belongs to the previous edit context.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        EchoConvergedRelationSolver solver;
        controller.setDevelopmentRelationSolver(
            &solver, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        CHECK(viewport.sketch_scene_.lines.size() == 1U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines.front().token};

        const auto original_revision = session.document().revision();
        const auto original_undo = session.undoDepth();
        const auto original_state =
            session.document().findSketch(sid)->model.state();
        bool reentered = false;
        viewport.on_point_query_ = [&] {
            if (reentered) return;
            reentered = true;
            controller.end();
            controller.begin(session, sid);
            controller.setDevelopmentRelationSolver(
                &solver, {1.0e-7, 1.0e-6});
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            // A valid hit token of the NEW edit session, not a stale token.
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines.front().token};
        };
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(reentered);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(!controller.firstRelationTarget());
        CHECK(solver.calls == 0);
        CHECK(session.document().revision() == original_revision);
        CHECK(session.undoDepth() == original_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              original_state);

        // A separate fresh click in that new edit context must succeed.
        viewport.on_point_query_ = {};
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines.front().token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(solver.calls == 1);
        CHECK(session.undoDepth() == original_undo + 1U);
        const auto committed =
            session.document().findSketch(sid)->model.state();
        CHECK(committed.constraints.size() == 1U);
        CHECK(committed.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        controller.end();
    }

    // R13-A2.0 selection-first edit-session ABA: a synchronous
    // stateChanged handler ends and reopens the SAME Sketch, with the
    // same Part revision, same backend address and same relation kind.
    // The old preselected EntityIds must not be consumed by that new
    // edit session merely because durable identity and revision match.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateSelect();
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            125.0, 100.0, 2.5, 0.0));
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            225.0, 100.0, 12.5, 0.0));
        CHECK(controller.selectedCount() == 2U);
        controller.activateRelations();

        const auto revision = session.document().revision();
        const auto undo_depth = session.undoDepth();
        const auto authored =
            session.document().findSketch(sid)->model.state();
        bool reopened = false;
        controller.setStateChangedHandler([&] {
            if (reopened ||
                controller.relationKind() !=
                    sketch::ConstraintKind::parallel) {
                return;
            }
            reopened = true;
            controller.end();
            controller.begin(session, sid);
            controller.setDevelopmentRelationSolver(
                &backend, {1.0e-7, 1.0e-6});
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::parallel));
            // New-session command-first acquisition starts DURING the
            // prior selection-first stateChanged callback.
            CHECK(viewport.sketch_scene_.lines.size() == 2U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(controller.firstRelationTarget() ==
                  (sketch::ConstraintReference{
                      *first.entity_id, sketch::ConstraintElement::line}));
        });
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(reopened);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(controller.selectedCount() == 0U);
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *first.entity_id, sketch::ConstraintElement::line}));
        CHECK(backend.calls == 0);
        CHECK(session.document().revision() == revision);
        CHECK(session.undoDepth() == undo_depth);
        CHECK(session.document().findSketch(sid)->model.state() ==
              authored);

        // The old selection-first callback must not destroy the new
        // context's first target: the second hit creates one transaction.
        controller.setStateChangedHandler({});
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == undo_depth + 1U);
        const auto accepted =
            session.document().findSketch(sid)->model.state();
        CHECK(accepted.constraints.size() == 1U);
        CHECK(accepted.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        controller.end();
    }

    // R13-A2.0: a stale outer Viewer callback must not CANCEL an entirely
    // new, legitimate first target acquired by a reentrant new Sketch edit.
    // It can only discard a pending target from its OWN edit generation.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& value) { status = value; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver solver;
        controller.setDevelopmentRelationSolver(
            &solver, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();
        const auto before_state =
            session.document().findSketch(sid)->model.state();

        bool reentered = false;
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        viewport.on_point_query_ = [&] {
            if (reentered) return;
            reentered = true;
            controller.end();
            controller.begin(session, sid);
            controller.setDevelopmentRelationSolver(
                &solver, {1.0e-7, 1.0e-6});
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::parallel));
            // Take the NEW first target inside the new edit session.
            CHECK(viewport.sketch_scene_.lines.size() == 2U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[1].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                225.0, 100.0, 12.5, 0.0));
            CHECK(controller.firstRelationTarget() ==
                  (sketch::ConstraintReference{
                      *second.entity_id,
                      sketch::ConstraintElement::line}));
        };
        // Old first hit resumes after the new first target is accepted.
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(reentered);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *second.entity_id,
                  sketch::ConstraintElement::line}));
        CHECK(status.find("stale") != std::string::npos);
        CHECK(status.find("newer edit-context selection preserved") !=
              std::string::npos);
        CHECK(solver.calls == 0);
        CHECK(session.document().revision() == before_revision);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              before_state);

        // The new context retains its first hit; one further current
        // hit must create precisely one valid Parallel relation.
        viewport.on_point_query_ = {};
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(solver.calls == 1);
        CHECK(session.undoDepth() == before_undo + 1U);
        const auto accepted =
            session.document().findSketch(sid)->model.state();
        CHECK(accepted.constraints.size() == 1U);
        CHECK(accepted.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        controller.end();
    }

    // R13-A2.0 Operations status reentrancy: activating Relations notifies
    // the UI, which can immediately choose Horizontal and complete a
    // valid click-to-author operation. The outer activation must not
    // overwrite the successful command status with its older generic
    // "solver not yet enabled" message on return from the callback.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();

        bool reentered = false;
        controller.setStateChangedHandler([&] {
            if (reentered ||
                controller.tool() != sketch::SketchTool::relations) {
                return;
            }
            reentered = true;
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(backend.calls == 1);
            CHECK(status.find("accepted by current Part solver") !=
                  std::string::npos);
        });
        controller.activateRelations();
        CHECK(reentered);
        CHECK(status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::horizontal);
        CHECK(session.document().revision() != before_revision);
        CHECK(session.undoDepth() == before_undo + 1U);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.size() == 1U);
        CHECK(state.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        controller.end();
    }

    // R13-A2.0: selection-first's SECOND stateChanged notification
    // happens AFTER preselected targets are consumed. A callback can
    // close/reopen the exact same Sketch and acquire a NEW first target.
    // The old selection-first continuation cannot clear the new owner's
    // pending relation, even though revision, IDs, kind and solver match.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver solver;
        controller.setDevelopmentRelationSolver(
            &solver, {1.0e-7, 1.0e-6});
        controller.activateSelect();
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            125.0, 100.0, 2.5, 0.0));
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_release,
            225.0, 100.0, 12.5, 0.0));
        CHECK(controller.selectedCount() == 2U);
        controller.activateRelations();
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();
        const auto before_state =
            session.document().findSketch(sid)->model.state();

        int notifications = 0;
        bool reentered = false;
        controller.setStateChangedHandler([&] {
            if (reentered ||
                controller.relationKind() !=
                    sketch::ConstraintKind::parallel) {
                return;
            }
            ++notifications;
            if (notifications != 2) return;
            reentered = true;
            controller.end();
            controller.begin(session, sid);
            controller.setDevelopmentRelationSolver(
                &solver, {1.0e-7, 1.0e-6});
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::parallel));
            CHECK(viewport.sketch_scene_.lines.size() == 2U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(controller.firstRelationTarget() ==
                  (sketch::ConstraintReference{
                      *first.entity_id, sketch::ConstraintElement::line}));
        });
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(reentered);
        CHECK(notifications == 2);
        CHECK(status.find("stale") != std::string::npos);
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *first.entity_id, sketch::ConstraintElement::line}));
        CHECK(solver.calls == 0);
        CHECK(session.document().revision() == before_revision);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              before_state);

        // The newly acquired first target must remain actionable.
        controller.setStateChangedHandler({});
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(solver.calls == 1);
        CHECK(session.undoDepth() == before_undo + 1U);
        const auto after =
            session.document().findSketch(sid)->model.state();
        CHECK(after.constraints.size() == 1U);
        CHECK(after.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        controller.end();
    }


    // R13-A2.0: activating the still-disabled Dimensions tool publishes
    // its placeholder status only for the CURRENT UI event. A synchronous
    // Operations listener may switch tools and author a valid DEV relation.
    // The obsolete outer Dimensions activation must not replace the newer
    // successfully committed relation status after its notification.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto added = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(added.ok() && added.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();
        bool reentered = false;
        controller.setStateChangedHandler([&] {
            if (reentered ||
                controller.tool() != sketch::SketchTool::dimensions) {
                return;
            }
            reentered = true;
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines.front().token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(backend.calls == 1);
            CHECK(status.find("accepted by current Part solver") !=
                  std::string::npos);
        });
        controller.activateDimensions();
        CHECK(reentered);
        CHECK(controller.tool() == sketch::SketchTool::relations);
        CHECK(status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(session.document().revision() != before_revision);
        CHECK(session.undoDepth() == before_undo + 1U);
        CHECK(session.document().findSketch(sid)->
                  model.state().constraints.size() == 1U);
        controller.end();
    }

    // R13-A2.0: selecting a disabled dimension kind notifies Operations
    // listeners after its transient state changes. An inner callback can
    // author a valid relation; the superseded outer dimension-kind status
    // must not overwrite the accepted Part transaction's diagnostics.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto added = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(added.ok() && added.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) {
                status = message;
            });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateDimensions();
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();
        bool reentered = false;
        controller.setStateChangedHandler([&] {
            if (reentered ||
                controller.tool() != sketch::SketchTool::dimensions) {
                return;
            }
            reentered = true;
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines.front().token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(backend.calls == 1);
            CHECK(status.find("accepted by current Part solver") !=
                  std::string::npos);
        });
        CHECK(controller.selectDimensionKind(
            sketch::DrivingDimensionToolKind::length));
        CHECK(reentered);
        CHECK(controller.tool() == sketch::SketchTool::relations);
        CHECK(status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(session.document().revision() != before_revision);
        CHECK(session.undoDepth() == before_undo + 1U);
        CHECK(session.document().findSketch(sid)->
                  model.state().constraints.size() == 1U);
        controller.end();
    }


    // R13-A2.0: a native Viewer projection is also a synchronous reentrant
    // boundary, BEFORE Operations stateChanged. The outer activation/choice
    // must never overwrite a newer valid Relations transaction status.

    // Projection callback during Relations activation; one guarded Part commit wins.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        
        const auto revision_before = session.document().revision();
        const auto undo_before = session.undoDepth();
        bool reentered = false;
        viewport.on_interaction_presentation_ = [&] {
            if (reentered || controller.tool() !=
                    sketch::SketchTool::relations) {
                return;
            }
            reentered = true;
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(backend.calls == 1);
            CHECK(status.find("accepted by current Part solver") !=
                  std::string::npos);
        };
        controller.activateRelations();
        CHECK(reentered);
        CHECK(controller.tool() == sketch::SketchTool::relations);
        CHECK(status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() != revision_before);
        CHECK(session.undoDepth() == undo_before + 1U);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.size() == 1U);
        CHECK(state.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        viewport.on_interaction_presentation_ = {};
        controller.end();
    }

    // Projection callback during Dimensions activation; one guarded Part commit wins.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        
        const auto revision_before = session.document().revision();
        const auto undo_before = session.undoDepth();
        bool reentered = false;
        viewport.on_interaction_presentation_ = [&] {
            if (reentered || controller.tool() !=
                    sketch::SketchTool::dimensions) {
                return;
            }
            reentered = true;
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(backend.calls == 1);
            CHECK(status.find("accepted by current Part solver") !=
                  std::string::npos);
        };
        controller.activateDimensions();
        CHECK(reentered);
        CHECK(controller.tool() == sketch::SketchTool::relations);
        CHECK(status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() != revision_before);
        CHECK(session.undoDepth() == undo_before + 1U);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.size() == 1U);
        CHECK(state.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        viewport.on_interaction_presentation_ = {};
        controller.end();
    }

    // Projection callback during Dimensions kind choice; one guarded Part commit wins.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto line = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        CHECK(line.ok() && line.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        std::string status;
        controller.setStatusHandler(
            [&status](const std::string& message) { status = message; });
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        controller.activateDimensions();
        const auto revision_before = session.document().revision();
        const auto undo_before = session.undoDepth();
        bool reentered = false;
        viewport.on_interaction_presentation_ = [&] {
            if (reentered || controller.tool() !=
                    sketch::SketchTool::dimensions) {
                return;
            }
            reentered = true;
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::horizontal));
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(backend.calls == 1);
            CHECK(status.find("accepted by current Part solver") !=
                  std::string::npos);
        };
        CHECK(controller.selectDimensionKind(sketch::DrivingDimensionToolKind::length));
        CHECK(reentered);
        CHECK(controller.tool() == sketch::SketchTool::relations);
        CHECK(status.find("accepted by current Part solver") !=
              std::string::npos);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() != revision_before);
        CHECK(session.undoDepth() == undo_before + 1U);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.size() == 1U);
        CHECK(state.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        viewport.on_interaction_presentation_ = {};
        controller.end();
    }


    // R13-A2.0: reportStatus itself is a synchronous, reentrant UI
    // boundary. Moving the obsolete placeholder before Viewer callbacks
    // must not let the old tool invocation clear a NEW edit generation's
    // first Line target/revision lease after an end/begin ABA restart.
    {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);
        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        controller.begin(session, sid);
        EchoConvergedRelationSolver backend;
        controller.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        const auto before_revision = session.document().revision();
        const auto before_undo = session.undoDepth();
        const auto before_state =
            session.document().findSketch(sid)->model.state();
        bool reentered = false;
        controller.setStatusHandler([&](const std::string& status) {
            // This callback is triggered by the *provisional* activation
            // message in either standard (no backend) or explicitly
            // opted-in DEV preview builds. The preview label differs,
            // but the stale-activation/ABA invariant is identical.
            const bool provisional_activation =
                status.find("RELATIONS: numeric production solver") !=
                    std::string::npos ||
                status.find(
                    "RELATIONS DEV PREVIEW: numerical backend registered") !=
                    std::string::npos;
            if (reentered || !provisional_activation) {
                return;
            }
            reentered = true;
            controller.end();
            controller.begin(session, sid);
            controller.setDevelopmentRelationSolver(
                &backend, {1.0e-7, 1.0e-6});
            controller.activateRelations();
            CHECK(controller.selectRelationKind(
                sketch::ConstraintKind::parallel));
            CHECK(viewport.sketch_scene_.lines.size() == 2U);
            viewport.point_query_ = {
                true, viewport.sketch_scene_.lines[0].token};
            controller.onPointer(pointer(
                sid, viewer::SpatialPointerPhase::primary_press,
                125.0, 100.0, 2.5, 0.0));
            CHECK(controller.firstRelationTarget() ==
                  (sketch::ConstraintReference{
                      *first.entity_id,
                      sketch::ConstraintElement::line}));
        });
        controller.activateRelations();
        CHECK(reentered);
        CHECK(controller.active());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *first.entity_id, sketch::ConstraintElement::line}));
        CHECK(backend.calls == 0);
        CHECK(session.document().revision() == before_revision);
        CHECK(session.undoDepth() == before_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              before_state);

        // A valid second hit MUST still complete the newer draft:
        // an obsolete outer activation may not clear its revision lease.
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(backend.calls == 1);
        CHECK(session.undoDepth() == before_undo + 1U);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.size() == 1U);
        CHECK(state.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        controller.end();
    }


    // R13-A2.0: a two-click draft belongs to its borrowed solver/policy
    // registration epoch. Replacing or disabling the test-only provider
    // must revoke the first target even if the same Sketch and Part
    // revision remain unchanged. The next pick starts a fresh pair;
    // no old target may be silently committed through the new backend.
    for (int replacement = 0; replacement != 3; ++replacement) {
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession session{{}, std::move(document)};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto first = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 0.0}, {5.0, 0.0}});
        const auto second = session.execute(
            application::AddSketchLineCommand{
                sid, {10.0, 0.0}, {15.0, 0.0}});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController viewport_controller{
            tree_controller, &viewport};
        viewport_controller.setDocumentSession(&session);
        viewport_controller.setSketchEditSketch(sid);
        ui::PartSketchInteractionController controller{
            viewport_controller};
        controller.begin(session, sid);
        EchoConvergedRelationSolver original_backend;
        EchoConvergedRelationSolver replacement_backend;
        controller.setDevelopmentRelationSolver(
            &original_backend, {1.0e-7, 1.0e-6});
        controller.activateRelations();
        CHECK(controller.selectRelationKind(
            sketch::ConstraintKind::parallel));
        CHECK(viewport.sketch_scene_.lines.size() == 2U);
        const auto initial_revision = session.document().revision();
        const auto initial_undo = session.undoDepth();
        const auto initial_state =
            session.document().findSketch(sid)->model.state();

        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *first.entity_id, sketch::ConstraintElement::line}));
        CHECK(original_backend.calls == 0);
        CHECK(session.document().revision() == initial_revision);

        if (replacement == 0) {
            // Different backend, same accepted policy.
            controller.setDevelopmentRelationSolver(
                &replacement_backend, {1.0e-7, 1.0e-6});
        } else if (replacement == 1) {
            // Disable explicitly, then re-enable a new backend.
            controller.setDevelopmentRelationSolver(
                nullptr, {1.0e-7, 1.0e-6});
            CHECK(!controller.firstRelationTarget());
            controller.setDevelopmentRelationSolver(
                &replacement_backend, {1.0e-7, 1.0e-6});
        } else {
            // Same pointer, a new numeric policy/registration lease.
            controller.setDevelopmentRelationSolver(
                &original_backend, {1.0e-6, 1.0e-5});
        }

        CHECK(!controller.firstRelationTarget());
        CHECK(controller.relationKind() ==
              sketch::ConstraintKind::parallel);
        CHECK(session.document().revision() == initial_revision);
        CHECK(session.undoDepth() == initial_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              initial_state);

        // This click must NOT use the stale first target and commit.
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[1].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            225.0, 100.0, 12.5, 0.0));
        CHECK(original_backend.calls == 0);
        CHECK(replacement_backend.calls == 0);
        CHECK(controller.firstRelationTarget() ==
              (sketch::ConstraintReference{
                  *second.entity_id, sketch::ConstraintElement::line}));
        CHECK(session.document().revision() == initial_revision);
        CHECK(session.undoDepth() == initial_undo);
        CHECK(session.document().findSketch(sid)->model.state() ==
              initial_state);

        // One additional fresh current-stage hit may now commit through
        // precisely the selected registration, without a duplicate ID.
        viewport.point_query_ = {
            true, viewport.sketch_scene_.lines[0].token};
        controller.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 100.0, 2.5, 0.0));
        CHECK(original_backend.calls ==
              (replacement == 2 ? 1 : 0));
        CHECK(replacement_backend.calls ==
              (replacement == 2 ? 0 : 1));
        CHECK(session.undoDepth() == initial_undo + 1U);
        CHECK(session.document().revision() != initial_revision);
        const auto state =
            session.document().findSketch(sid)->model.state();
        CHECK(state.constraints.size() == 1U);
        CHECK(state.constraints.front().kind ==
              sketch::ConstraintKind::parallel);
        controller.end();
    }

    // R13-A2 E0 — EXPECTED RED until the existing Part stage-solved
    // evaluation is connected to the real current Sketch scene.
    // An authored Horizontal relation and an accepted numerical solver
    // candidate MUST NOT leave a skew Line visible to the user.
    // This is test-only evidence: no new runtime API or solver dependency.
    {
        struct SolveHorizontal final : sketch::ILocalSketchSolver {
            int calls{};
            bool fail{false};

            [[nodiscard]] sketch::LocalBackendSolveResult solve(
                const sketch::LocalSolveProblem& problem) override {
                ++calls;
                if (fail) {
                    return {sketch::LocalBackendStatus::failed,
                            std::nullopt, {}, {}, std::nullopt};
                }
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 1U);
                candidate.lines.front().end.v =
                    candidate.lines.front().start.v;
                return {sketch::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, std::nullopt};
            }
        } backend;
        auto document =
            part::PartDocument::create(core::DocumentId::generate());
        application::DocumentSession solved_session{{}, std::move(document)};
        const auto created_sketch = solved_session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created_sketch.ok() && created_sketch.sketch_id);
        const auto sid = *created_sketch.sketch_id;
        const auto added = solved_session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 1.0}, {5.0, 2.0}});
        CHECK(added.ok() && added.entity_id);
        const auto revision_before =
            solved_session.document().revision();
        const auto undo_before = solved_session.undoDepth();
        QTreeWidget scene_tree;
        ui::PartDocumentTreeController scene_tree_controller{scene_tree};
        TestViewport scene_viewport;
        ui::PartViewportController scene_controller{
            scene_tree_controller, &scene_viewport};
        scene_controller.setDocumentSession(&solved_session);
        scene_controller.setSketchEditSketch(sid);
        CHECK(scene_viewport.sketch_scene_.lines.size() == 1U);
        const auto before = scene_viewport.sketch_scene_.lines.front();
        CHECK(!near(before.start.y, before.end.y));
        ui::PartSketchInteractionController interaction{scene_controller};
        interaction.begin(solved_session, sid);
        interaction.setDevelopmentRelationSolver(
            &backend, {1.0e-7, 1.0e-6});
        // Explicit opt-in only, reproducing the Workbench's original
        // PlaneGCS DEV preview registration. Authoring alone must not
        // implicitly trigger solves in unrelated controller fixtures.
        scene_controller.setDevelopmentStageSolver(
            &backend, {1.0e-7, 1.0e-6});
        interaction.activateRelations();
        CHECK(interaction.selectRelationKind(
            sketch::ConstraintKind::horizontal));
        CHECK(scene_viewport.sketch_scene_.lines.size() == 1U);
        scene_viewport.point_query_ = {
            true, scene_viewport.sketch_scene_.lines.front().token};
        interaction.onPointer(pointer(
            sid, viewer::SpatialPointerPhase::primary_press,
            125.0, 115.0, 2.5, 1.5));
        // One invocation validates the Part command; one more
        // evaluates and publishes the same revision-bound SketchScene.
        CHECK(backend.calls == 2);
        CHECK(solved_session.undoDepth() == undo_before + 1U);
        CHECK(solved_session.document().revision() != revision_before);
        const auto authored =
            solved_session.document().findSketch(sid)->model.state();
        CHECK(authored.constraints.size() == 1U);
        CHECK(authored.constraints.front().kind ==
              sketch::ConstraintKind::horizontal);
        CHECK(authored.lines.size() == 1U);
        CHECK(near(authored.lines.front().start.v, 1.0));
        CHECK(near(authored.lines.front().end.v, 2.0));
        // The accepted Part stage-solver gateway is ALREADY capable of
        // producing the same Horizontal effective geometry independently
        // from the authored, deliberately skew Line seed. This check is
        // a domain-side E0 oracle, not a new Viewer API, shortcut or
        // second numerical solver. It must PASS before the Viewer RED.
        struct NoProjectedEdges final : kernel::IEdgeProjectionQuery {
            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle,
                kernel::RuntimeEdgeToken) noexcept override {
                return std::nullopt;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle,
                const kernel::ScopedProjectionEdge&,
                const kernel::Frame3&) noexcept override {
                return {};
            }
        } projection;
        part::PartEvaluation current_stage;
        current_stage.source_document_id =
            solved_session.document().documentId();
        current_stage.source_revision =
            solved_session.document().revision();
        current_stage.source_incarnation =
            solved_session.document().incarnationToken();
        const sketch::LocalSolverAcceptancePolicy policy{1.0e-7, 1.0e-6};
        const auto part_solved = part::evaluateStageLocalSolve(
            solved_session.document(), sid, current_stage, projection,
            policy, backend);
        CHECK(part_solved);
        CHECK(part_solved->source_document_id ==
              solved_session.document().documentId());
        CHECK(part_solved->source_revision ==
              solved_session.document().revision());
        CHECK(part_solved->source_sketch_id == sid);
        CHECK(part_solved->solution.status ==
              sketch::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(part_solved->solution.candidate);
        CHECK(part_solved->solution.candidate->lines.size() == 1U);
        const auto& solved_line =
            part_solved->solution.candidate->lines.front();
        CHECK(solved_line.id == *added.entity_id);
        CHECK(near(solved_line.start.v, solved_line.end.v));
        CHECK(near(solved_line.start.v, authored.lines.front().start.v));
        CHECK(!near(solved_line.end.v, authored.lines.front().end.v));
        CHECK(backend.calls == 3);
        CHECK(solved_session.document().findSketch(sid)->model.state() ==
              authored);

        // E0 integration RED: the same current Part stage has a verified
        // horizontal candidate, yet the ACTUAL SketchScene still renders
        // stale authored coordinates rather than that candidate.
        CHECK(scene_viewport.sketch_scene_.lines.size() == 1U);
        const auto shown = scene_viewport.sketch_scene_.lines.front();
        CHECK(near(shown.start.y, shown.end.y));
        CHECK(near(shown.start.y, solved_line.start.v));
        CHECK(near(shown.end.y, solved_line.end.v));
        // E0 failure is fail-closed, never a "last successful"
        // horizontal scene nor a fallback to the authored skew seed.
        backend.fail = true;
        const auto revision_while_failed =
            solved_session.document().revision();
        const auto undo_while_failed = solved_session.undoDepth();
        scene_controller.refreshPresentation();
        CHECK(scene_viewport.sketch_scene_.lines.empty());
        const auto* unavailable_pick =
            scene_controller.currentSketchInteractionModel();
        CHECK(unavailable_pick != nullptr);
        CHECK(unavailable_pick->state().lines.empty());
        CHECK(solved_session.document().revision() ==
              revision_while_failed);
        CHECK(solved_session.undoDepth() == undo_while_failed);
        CHECK(solved_session.document().findSketch(sid)->model.state() ==
              authored);

        // An unchanged, valid backend may recover on a fresh current-
        // revision refresh without copying the saved/skew seed.
        backend.fail = false;
        scene_controller.refreshPresentation();
        CHECK(scene_viewport.sketch_scene_.lines.size() == 1U);
        CHECK(near(scene_viewport.sketch_scene_.lines.front().start.y,
                   scene_viewport.sketch_scene_.lines.front().end.y));

        // Part Undo makes the constraint disappear: published geometry
        // must be authored/skew again. Redo republishes solved geometry
        // while the persisted Line coordinates never change.
        CHECK(solved_session.undo().changed);
        scene_controller.refreshPresentation();
        CHECK(scene_viewport.sketch_scene_.lines.size() == 1U);
        CHECK(!near(scene_viewport.sketch_scene_.lines.front().start.y,
                    scene_viewport.sketch_scene_.lines.front().end.y));
        CHECK(solved_session.redo().changed);
        scene_controller.refreshPresentation();
        CHECK(scene_viewport.sketch_scene_.lines.size() == 1U);
        CHECK(near(scene_viewport.sketch_scene_.lines.front().start.y,
                   scene_viewport.sketch_scene_.lines.front().end.y));
        CHECK(solved_session.document().findSketch(sid)->model.state() ==
              authored);
        interaction.end();
    }

    // E0: a Profile and its active SketchScene must consume the SAME
    // disposable Part stage result during one refresh. A translating
    // numerical provider exposes even a small difference between a
    // second, independent solve and the geometry actually drawn.
    {
        struct ShiftStageSolver final : sketch::ILocalSketchSolver {
            int calls{};
            double shift_u{3.0};
            bool reject{false};

            [[nodiscard]] sketch::LocalBackendSolveResult solve(
                const sketch::LocalSolveProblem& problem) override {
                ++calls;
                if (reject) {
                    return {sketch::LocalBackendStatus::failed,
                            std::nullopt, {}, {}, std::nullopt};
                }
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 4U);
                for (auto& line : candidate.lines) {
                    line.start.u += shift_u;
                    line.end.u += shift_u;
                }
                return {sketch::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, std::nullopt};
            }
        } stage_solver;
        application::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto rectangle = session.execute(
            application::AddSketchRectangleCommand{
                sid, session.document().revision(),
                {0.0, 0.0}, {10.0, 8.0},
                sketch::EntityRole::regular, false});
        CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
        const auto authored =
            session.document().findSketch(sid)->model.state();
        const auto regions = sketch::analyzeRegions(
            session.document().findSketch(sid)->model);
        CHECK(regions.complete() && regions.regions.size() == 1U);
        const auto intent =
            part::makeProfileRegionIntent(regions.regions.front());
        CHECK(intent);
        const auto profile = session.execute(
            application::CreateProfileCommand{
                sid, session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id);
        const auto relation = session.execute(
            application::AddHorizontalSketchConstraintCommand{
                sid, session.document().revision(),
                rectangle.entity_ids.front()});
        CHECK(relation.ok() && relation.constraint_id);
        const auto source_revision = session.document().revision();
        const auto source_undo = session.undoDepth();

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController controller{tree_controller, &viewport};
        controller.setDocumentSession(&session);
        controller.setSketchEditSketch(sid);
        // Pre-existing A1 authored-only editing is still intentionally
        // inspectable before any E0 DEV solver lease is acquired. This
        // legacy scene is not being claimed as numerically solved.
        CHECK(viewport.sketch_scene_.lines.size() == 4U);
        CHECK(controller.currentSketchInteractionModel() != nullptr);
        CHECK(controller.currentSketchInteractionModel()->
                  state().lines.size() == 4U);
        CHECK(viewport.profile_scene_.profiles.empty());
        // Explicit E0 activation runs the existing Part stage gateway.
        controller.setDevelopmentStageSolver(
            &stage_solver, {1.0e-7, 1.0e-6});
        const auto checkSamePublishedSource = [&](double shift) {
            CHECK(viewport.sketch_scene_.lines.size() == 4U);
            CHECK(viewport.profile_scene_.profiles.size() == 1U);
            const auto& loop =
                viewport.profile_scene_.profiles.front().region.outer;
            CHECK(loop.size() >= 4U);
            const auto leftmost = [](const auto& points) {
                double minimum = points.front().x;
                for (const auto& point : points) {
                    minimum = std::min(minimum, point.x);
                }
                return minimum;
            };
            CHECK(near(leftmost(loop), shift));
            double sketch_leftmost =
                viewport.sketch_scene_.lines.front().start.x;
            for (const auto& line : viewport.sketch_scene_.lines) {
                sketch_leftmost = std::min(
                    sketch_leftmost,
                    std::min(line.start.x, line.end.x));
            }
            CHECK(near(sketch_leftmost, shift));
            const auto effective =
                controller.currentSketchInteractionModel();
            CHECK(effective != nullptr);
            CHECK(effective->state().lines.size() == 4U);
            const auto resolved =
                controller.currentProfileResolution(
                    *profile.profile_id);
            CHECK(resolved && resolved->valid());
            CHECK(session.document().revision() == source_revision);
            CHECK(session.undoDepth() == source_undo);
            // The authored source before the Horizontal command is
            // unchanged; the only new persisted intent is the relation.
            CHECK(session.document().findSketch(sid)->
                  model.state().lines == authored.lines);
        };
        CHECK(stage_solver.calls == 1);
        checkSamePublishedSource(3.0);
        // No extra solve on semantic pick or Properties/Profile access.
        CHECK(stage_solver.calls == 1);

        stage_solver.shift_u = 5.0;
        controller.refreshPresentation();
        CHECK(stage_solver.calls == 2);
        checkSamePublishedSource(5.0);

        stage_solver.reject = true;
        controller.refreshPresentation();
        CHECK(stage_solver.calls == 3);
        CHECK(viewport.sketch_scene_.lines.empty());
        CHECK(viewport.profile_scene_.profiles.empty());
        CHECK(!controller.currentProfileResolution(
            *profile.profile_id));
        CHECK(session.document().revision() == source_revision);
        CHECK(session.undoDepth() == source_undo);
        stage_solver.reject = false;
        controller.refreshPresentation();
        CHECK(stage_solver.calls == 4);
        checkSamePublishedSource(5.0);

        // Explicitly revoking the only stage solver MUST also revoke
        // solved-geometry authority. Without a usable numeric provider,
        // showing authored rectangle seeds as current solved geometry is
        // a false-current success, even though it happens to be closed.
        controller.setDevelopmentStageSolver(nullptr, {});
        CHECK(viewport.profile_scene_.profiles.empty());
        CHECK(viewport.sketch_scene_.lines.empty());
        const auto* disabled_model =
            controller.currentSketchInteractionModel();
        CHECK(disabled_model == nullptr ||
              disabled_model->state().lines.empty());
        CHECK(stage_solver.calls == 4);
        CHECK(session.document().revision() == source_revision);
        CHECK(session.undoDepth() == source_undo);

        // Re-registering the identical backend after a lease gap is an
        // explicit fresh evaluation; it must not revive a cached scene.
        controller.setDevelopmentStageSolver(
            &stage_solver, {1.0e-7, 1.0e-6});
        CHECK(stage_solver.calls == 5);
        checkSamePublishedSource(5.0);

        // Genuine native .ss2part cold load, not SketchModel::restore,
        // same-session Undo or copying PartAuthoredState. No solved
        // points or UI/backend cache are stored in this file.
        const auto cold_path =
            std::filesystem::temp_directory_path() /
            ("ss2_e0_cold_stage_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count()) + ".ss2part");
        struct DeleteNativeFile final {
            std::filesystem::path path;
            ~DeleteNativeFile() {
                std::error_code ignored;
                std::filesystem::remove(path, ignored);
            }
        } cleanup{cold_path};
        part::PartDocumentStore store;
        const auto persisted = store.createNew(
            cold_path, session.document());
        CHECK(persisted.ok());
        auto cold = store.load(cold_path);
        CHECK(cold.ok() && cold.document && cold.checkpoint);
        CHECK(cold.document->documentId() ==
              session.document().documentId());
        CHECK(cold.document->incarnationToken() !=
              session.document().incarnationToken());
        const auto* saved_sketch = cold.document->findSketch(sid);
        CHECK(saved_sketch != nullptr);
        CHECK(saved_sketch->model.state().lines == authored.lines);
        CHECK(saved_sketch->model.state().constraints.size() == 1U);
        CHECK(saved_sketch->model.state().constraints.front().id ==
              *relation.constraint_id);
        CHECK(cold.document->findProfile(*profile.profile_id) != nullptr);

        application::DocumentSession reopened{
            cold_path, std::move(*cold.document), *cold.checkpoint};
        // A new native Part incarnation must never replay the previous
        // temporary answer (shift 5). The new backend answer (shift 7)
        // must be evaluated from durable relation intent, once.
        controller.setDocumentSession(nullptr);
        controller.setDocumentSession(&reopened);
        controller.setSketchEditSketch(sid);
        stage_solver.shift_u = 7.0;
        controller.setDevelopmentStageSolver(
            &stage_solver, {1.0e-7, 1.0e-6});
        CHECK(stage_solver.calls == 6);
        CHECK(viewport.sketch_scene_.lines.size() == 4U);
        CHECK(viewport.profile_scene_.profiles.size() == 1U);
        CHECK(near(viewport.sketch_scene_.lines.front().start.x, 7.0));
        CHECK(near(viewport.profile_scene_.profiles.front()
                       .region.outer.front().x, 7.0));
        CHECK(reopened.document().findSketch(sid)->
                  model.state().lines == authored.lines);
        CHECK(reopened.document().findSketch(sid)->
                  model.state().constraints.front().id ==
              *relation.constraint_id);
        CHECK(!reopened.needsSave());
        CHECK(reopened.undoDepth() == 0U);

        // Cold-incarnation failure must not resurrect a candidate
        // obtained before save. With no current accepted solution,
        // both Profile and Sketch picking remain unavailable.
        stage_solver.reject = true;
        controller.refreshPresentation();
        CHECK(stage_solver.calls == 7);
        CHECK(viewport.sketch_scene_.lines.empty());
        CHECK(viewport.profile_scene_.profiles.empty());
        const auto* unavailable_cold_pick =
            controller.currentSketchInteractionModel();
        CHECK(unavailable_cold_pick == nullptr ||
              unavailable_cold_pick->state().lines.empty());
        CHECK(!controller.currentProfileResolution(
            *profile.profile_id));
        CHECK(!reopened.needsSave());

        // A new successful response must use the NEW current numeric
        // solution, not the pre-save answer or rejected stage.
        stage_solver.reject = false;
        stage_solver.shift_u = 9.0;
        controller.refreshPresentation();
        CHECK(stage_solver.calls == 8);
        CHECK(viewport.sketch_scene_.lines.size() == 4U);
        CHECK(viewport.profile_scene_.profiles.size() == 1U);
        CHECK(near(viewport.sketch_scene_.lines.front().start.x, 9.0));
        CHECK(reopened.document().findSketch(sid)->
                  model.state().lines == authored.lines);
        CHECK(!reopened.needsSave());
    }

    // E0 Part provenance under synchronous backend reentry: while a
    // current-stage candidate is being evaluated, the user may Undo
    // and trigger a *new* Viewer refresh before the old solve returns.
    // The old outer refresh must never republish its obsolete stage or
    // clear the newer committed scene / semantic picking snapshot.
    {
        struct UndoReentrantStageSolver final : sketch::ILocalSketchSolver {
            int calls{};
            std::function<void()> on_solve;
            [[nodiscard]] sketch::LocalBackendSolveResult solve(
                const sketch::LocalSolveProblem& problem) override {
                ++calls;
                if (on_solve) on_solve();
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 1U);
                candidate.lines.front().end.v =
                    candidate.lines.front().start.v;
                return {sketch::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, std::nullopt};
            }
        } backend;
        application::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto sid = *created.sketch_id;
        const auto added = session.execute(
            application::AddSketchLineCommand{
                sid, {0.0, 1.0}, {5.0, 2.0}});
        CHECK(added.ok() && added.entity_id);
        const auto added_relation = session.execute(
            application::AddHorizontalSketchConstraintCommand{
                sid, session.document().revision(),
                *added.entity_id});
        CHECK(added_relation.ok() && added_relation.constraint_id);
        const auto initial_revision = session.document().revision();
        const auto initial_undo = session.undoDepth();

        QTreeWidget tree;
        ui::PartDocumentTreeController tree_controller{tree};
        TestViewport viewport;
        ui::PartViewportController controller{
            tree_controller, &viewport};
        controller.setDocumentSession(&session);
        controller.setSketchEditSketch(sid);
        CHECK(viewport.sketch_scene_.lines.size() == 1U);
        CHECK(!near(viewport.sketch_scene_.lines.front().start.y,
                    viewport.sketch_scene_.lines.front().end.y));

        bool callback_fired = false;
        backend.on_solve = [&] {
            CHECK(!callback_fired);
            callback_fired = true;
            const auto undone = session.undo();
            CHECK(undone.ok() && undone.changed);
            CHECK(session.document().findSketch(sid) != nullptr);
            CHECK(session.document().findSketch(sid)->
                      model.state().constraints.empty());
            // The newer, post-Undo scene is published before the
            // obsolete outer numerical solver call returns.
            controller.refreshPresentation();
            CHECK(viewport.sketch_scene_.lines.size() == 1U);
            CHECK(!near(viewport.sketch_scene_.lines.front().start.y,
                        viewport.sketch_scene_.lines.front().end.y));
        };
        controller.setDevelopmentStageSolver(
            &backend, {1.0e-7, 1.0e-6});

        CHECK(callback_fired);
        CHECK(backend.calls == 1);
        CHECK(session.document().revision() != initial_revision);
        CHECK(session.undoDepth() + 1U == initial_undo);
        CHECK(session.document().findSketch(sid)->
                  model.state().constraints.empty());
        CHECK(viewport.sketch_scene_.lines.size() == 1U);
        CHECK(!near(viewport.sketch_scene_.lines.front().start.y,
                    viewport.sketch_scene_.lines.front().end.y));
        const auto* current = controller.currentSketchInteractionModel();
        CHECK(current != nullptr);
        CHECK(current->state().lines.size() == 1U);
        CHECK(current->state().constraints.empty());
        CHECK(!near(current->state().lines.front().start.v,
                    current->state().lines.front().end.v));
        backend.on_solve = {};
    }

    return EXIT_SUCCESS;
}
