#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

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

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-07F precision input controller CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

bool near(double a, double b) {
    return std::abs(a - b) < 1.0e-12;
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

    std::optional<viewer::ViewportPoint2>
    projectWorldPoint(
        viewer::Point3 point) const override {
        if (!viewer::finite(point)) {
            return std::nullopt;
        }
        return viewer::ViewportPoint2{
            point.x,
            point.y};
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
    bool setSketchSnapInferenceScene(
        const viewer::SketchSnapInferenceScene& scene) override {
        if (!scene.valid()) return false;
        snap_inference_scene_ = scene;
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
        last_rectangle_query_ = rectangle;
        last_rectangle_rule_ = rule;
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
    viewer::SketchSnapInferenceScene snap_inference_scene_;
    viewer::SketchGripQueryResult grip_query_{true, std::nullopt};
    viewer::PresentationSelection selection_;
    viewer::SketchPointQueryResult point_query_{true, std::nullopt};
    viewer::SketchRectangleQueryResult rectangle_query_{true, {}};
    std::optional<viewer::ViewportRect2> last_rectangle_query_;
    std::optional<viewer::SketchRectangleSelectionRule>
        last_rectangle_rule_;
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
            ("ss2-sk07f-precision-controller-" +
             std::string{
                 core::DocumentId::generate().value()});
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
    const auto path =
        temp.path / "sk07f-precision-controller.ss2part";

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
    application::SketchCadInputSemanticEndpoint semantic_input{
        interaction,
        application::CadInputNumberFormat{"."}};

    // LINE: pointer supplies direction, numeric scalar supplies exact distance.
    interaction.activateLine();
    click(interaction, sketch_id, 10.0, 10.0, 0.0, 0.0);
    movePointer(interaction, sketch_id, 20.0, 20.0, 3.0, 4.0);
    CHECK(interaction.submitDirectDistance(50.0));

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(hosted->model.entityCount() == 1U);
    auto model_state = hosted->model.state();
    CHECK(model_state.lines.size() == 1U);
    const auto source_id = model_state.lines.front().id;
    CHECK(near(model_state.lines.front().start.u, 0.0));
    CHECK(near(model_state.lines.front().start.v, 0.0));
    CHECK(near(model_state.lines.front().end.u, 30.0));
    CHECK(near(model_state.lines.front().end.v, 40.0));

    // Select the authored Line through the existing semantic query bridge.
    interaction.activateSelect();
    CHECK(!viewport.sketch_scene_.lines.empty());
    viewport.point_query_ = {
        true,
        viewport.sketch_scene_.lines.front().token};
    click(interaction, sketch_id, 30.0, 30.0, 15.0, 20.0);
    CHECK(interaction.selectedCount() == 1U);

    // MOVE: base alone must not supply a stale direction. After pointer
    // movement, Direct Distance commits through the existing update command.
    CHECK(interaction.activateMove());
    click(interaction, sketch_id, 40.0, 40.0, 5.0, 5.0);
    const auto revision_before_direction =
        session.document().revision();
    CHECK(!interaction.submitDirectDistance(50.0));
    CHECK(
        session.document().revision() ==
        revision_before_direction);

    movePointer(interaction, sketch_id, 50.0, 50.0, 8.0, 9.0);
    CHECK(interaction.submitDirectDistance(50.0));
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto* moved_source =
        hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, 30.0));
    CHECK(near(moved_source->start().v, 40.0));
    CHECK(near(moved_source->end().u, 60.0));
    CHECK(near(moved_source->end().v, 80.0));

    // COPY: numeric placement uses the existing repeated-copy/fresh-ID
    // commit path. Each placement requires a fresh pointer direction.
    const auto copy_undo_before = session.undoDepth();
    CHECK(interaction.activateCopy());
    click(interaction, sketch_id, 60.0, 60.0, 0.0, 0.0);
    movePointer(interaction, sketch_id, 60.0, 80.0, 0.0, 1.0);
    CHECK(interaction.submitDirectDistance(50.0));
    CHECK(interaction.tool() == sketch::SketchTool::copy);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::await_destination);
    CHECK(session.undoDepth() == copy_undo_before + 1U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 2U);

    const auto after_first_copy_revision =
        session.document().revision();
    CHECK(!interaction.submitDirectDistance(25.0));
    CHECK(
        session.document().revision() ==
        after_first_copy_revision);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 2U);

    movePointer(interaction, sketch_id, 80.0, 60.0, 1.0, 0.0);
    CHECK(interaction.submitDirectDistance(25.0));
    CHECK(session.undoDepth() == copy_undo_before + 2U);
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 3U);

    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, 30.0));
    CHECK(near(moved_source->start().v, 40.0));
    CHECK(near(moved_source->end().u, 60.0));
    CHECK(near(moved_source->end().v, 80.0));

    CHECK(interaction.escape());
    CHECK(interaction.tool() == sketch::SketchTool::select);
    CHECK(interaction.selectedCount() == 1U);

    // Grip Reshape consumes the same request/resolver.
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(interaction, sketch_id, 90.0, 90.0, 30.0, 40.0);
    CHECK(interaction.directManipulationActive());
    movePointer(interaction, sketch_id, 90.0, 100.0, 30.0, 41.0);
    CHECK(interaction.submitDirectDistance(10.0));
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, 30.0));
    CHECK(near(moved_source->start().v, 50.0));
    CHECK(near(moved_source->end().u, 60.0));
    CHECK(near(moved_source->end().v, 80.0));

    // Space/CycleEditMode does not replace the request: Move uses the
    // same raw pointer-direction semantics and frozen selection.
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(interaction, sketch_id, 90.0, 100.0, 30.0, 50.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::move);
    movePointer(interaction, sketch_id, 100.0, 100.0, 31.0, 50.0);
    CHECK(interaction.submitDirectDistance(10.0));
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, 40.0));
    CHECK(near(moved_source->start().v, 50.0));
    CHECK(near(moved_source->end().u, 70.0));
    CHECK(near(moved_source->end().v, 80.0));

    // SK-07G Grip Copy: tool-local C enables Copy without authored mutation.
    // Reshape+Copy duplicates only the grip owner and remains active for
    // repeated placement through the existing Direct Distance resolver.
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(interaction, sketch_id, 120.0, 120.0, 40.0, 50.0);
    CHECK(interaction.directManipulationActive());
    CHECK(!interaction.directManipulationCopyEnabled());

    const auto grip_copy_revision_before =
        session.document().revision();
    const auto grip_copy_undo_before =
        session.undoDepth();
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto grip_copy_count_before =
        hosted->model.entityCount();
    const auto source_before_grip_copy =
        *hosted->model.findLine(source_id);

    movePointer(
        interaction,
        sketch_id,
        120.0,
        130.0,
        40.0,
        51.0);
    auto semantic_result =
        semantic_input.submit(" c ");
    CHECK(semantic_result.accepted);
    CHECK(interaction.directManipulationCopyEnabled());
    CHECK(
        session.document().revision() ==
        grip_copy_revision_before);
    CHECK(
        session.undoDepth() ==
        grip_copy_undo_before);

    semantic_result = semantic_input.submit("10");
    CHECK(semantic_result.accepted);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.directManipulationCopyEnabled());
    CHECK(
        session.document().revision().value() ==
        grip_copy_revision_before.value() + 1U);
    CHECK(
        session.undoDepth() ==
        grip_copy_undo_before + 1U);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(
        hosted->model.entityCount() ==
        grip_copy_count_before + 1U);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(*moved_source == source_before_grip_copy);

    // Repeated Grip Copy requires a fresh pointer direction, mirroring
    // normal repeated COPY.
    const auto first_grip_copy_revision =
        session.document().revision();
    semantic_result = semantic_input.submit("5");
    CHECK(!semantic_result.accepted);
    CHECK(
        session.document().revision() ==
        first_grip_copy_revision);

    movePointer(
        interaction,
        sketch_id,
        130.0,
        120.0,
        41.0,
        50.0);
    semantic_result = semantic_input.submit("5");
    CHECK(semantic_result.accepted);
    CHECK(
        session.undoDepth() ==
        grip_copy_undo_before + 2U);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(
        hosted->model.entityCount() ==
        grip_copy_count_before + 2U);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(*moved_source == source_before_grip_copy);

    // Space changes the DirectEditMode and turns Copy OFF.
    CHECK(interaction.cycleDirectEditMode());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::move);
    CHECK(!interaction.directManipulationCopyEnabled());
    CHECK(interaction.escape());

    // Outside an active grip, C remains an unknown Sketch command.
    semantic_result = semantic_input.submit("C");
    CHECK(!semantic_result.accepted);
    CHECK(semantic_result.diagnostic ==
          "Unknown Sketch command.");

    // Esc clears transient precision-input state with no authored commit.
    CHECK(interaction.activateMove());
    click(interaction, sketch_id, 110.0, 110.0, 0.0, 0.0);
    movePointer(interaction, sketch_id, 110.0, 120.0, 0.0, 1.0);
    CHECK(interaction.activePointRequest().has_value());
    const auto before_escape =
        session.document().state();
    CHECK(interaction.escape());
    CHECK(!interaction.activePointRequest().has_value());
    CHECK(session.document().state() == before_escape);

    // R10 point grammar uses the same semantic endpoint and PointRequest.
    // Unitless coordinates follow the durable Part display/input length unit.
    const auto set_inches =
        session.execute(
            application::SetPartLengthUnitCommand{
                core::LengthUnit::inch});
    CHECK(set_inches.ok());
    CHECK(set_inches.changed);

    application::SketchCadInputSemanticEndpoint inch_input{
        interaction,
        application::CadInputNumberFormat{
            ".",
            core::LengthUnit::inch}};

    interaction.activateLine();
    semantic_result = inch_input.submit("1;2");
    CHECK(semantic_result.accepted);
    CHECK(
        interaction.lineStage() ==
        sketch::LineStage::await_next_point);

    semantic_result = inch_input.submit("@1;0.5");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(!model_state.lines.empty());
    const auto& exact_cartesian =
        model_state.lines.back();
    CHECK(near(exact_cartesian.start.u, 25.4));
    CHECK(near(exact_cartesian.start.v, 50.8));
    CHECK(near(exact_cartesian.end.u, 50.8));
    CHECK(near(exact_cartesian.end.v, 63.5));

    semantic_result = inch_input.submit("@1<90");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(model_state.lines.size() >= 2U);
    const auto& exact_polar =
        model_state.lines.back();
    CHECK(near(exact_polar.start.u, 50.8));
    CHECK(near(exact_polar.start.v, 63.5));
    CHECK(near(exact_polar.end.u, 50.8));
    CHECK(near(exact_polar.end.v, 88.9));
    CHECK(interaction.escape());

    // Circle Center -> Size defaults to Diameter at Sketch-edit entry.
    interaction.activateCircle();
    semantic_result = inch_input.submit("1;1");
    CHECK(semantic_result.accepted);
    CHECK(
        interaction.circleSizeInputMode() ==
        application::CircleSizeInputMode::diameter);
    semantic_result = inch_input.submit("2");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(!model_state.circles.empty());
    CHECK(near(model_state.circles.back().center.u, 25.4));
    CHECK(near(model_state.circles.back().center.v, 25.4));
    CHECK(near(model_state.circles.back().radius, 25.4));

    // R is an explicit runtime setter and persists across Circle
    // activations within this Sketch edit.
    interaction.activateCircle();
    semantic_result = inch_input.submit("2;2");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("R");
    CHECK(semantic_result.accepted);
    CHECK(
        interaction.circleSizeInputMode() ==
        application::CircleSizeInputMode::radius);
    semantic_result = inch_input.submit("1");
    CHECK(semantic_result.accepted);
    interaction.activateCircle();
    CHECK(
        interaction.circleSizeInputMode() ==
        application::CircleSizeInputMode::radius);
    CHECK(interaction.escape());

    // Arc: Start -> End -> Arc Point / Radius. Pointer side supplies
    // the typed-Radius bulge, while an explicit Arc Point outranks a
    // previously locked Radius.
    interaction.activateArc();
    semantic_result = inch_input.submit("0;0");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("@4;0");
    CHECK(semantic_result.accepted);
    movePointer(
        interaction,
        sketch_id,
        150.0,
        150.0,
        50.8,
        20.0);
    semantic_result = inch_input.submit("2");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(!model_state.arcs.empty());
    CHECK(near(model_state.arcs.back().radius, 50.8));

    // Arc Start -> End accepts the same bare Direct Distance path as
    // the visible helper chord. This is the manual keyboard workflow:
    // point Start, aim the pointer/Polar direction, type chord length.
    interaction.activateArc();
    semantic_result = inch_input.submit("0;0");
    CHECK(semantic_result.accepted);
    const auto arc_chord_request =
        interaction.activePointRequest();
    CHECK(arc_chord_request.has_value());
    CHECK(arc_chord_request->base.has_value());
    CHECK(arc_chord_request->direct_distance_enabled);
    movePointer(
        interaction,
        sketch_id,
        180.0,
        100.0,
        100.0,
        0.0);
    semantic_result = inch_input.submit("4");
    CHECK(semantic_result.accepted);
    CHECK(
        interaction.arcStage() ==
        sketch::ArcStage::await_arc_point);
    movePointer(
        interaction,
        sketch_id,
        180.0,
        130.0,
        50.8,
        20.0);
    semantic_result = inch_input.submit("2");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(!model_state.arcs.empty());
    CHECK(near(model_state.arcs.back().radius, 50.8));

    interaction.activateArc();
    semantic_result = inch_input.submit("0;0");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("@2;0");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("2");
    CHECK(semantic_result.accepted);
    CHECK(
        interaction.arcStage() ==
        sketch::ArcStage::await_arc_point);
    const auto arc_count_before_point =
        session.document().findSketch(sketch_id)->
            model.state().arcs.size();
    semantic_result = inch_input.submit("1;1");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(
        hosted->model.state().arcs.size() ==
        arc_count_before_point + 1U);

    // Rectangle owns Width;Height at stage two. Pointer supplies left/up
    // quadrant while the pair supplies exact positive magnitudes.
    interaction.activateRectangle();
    semantic_result = inch_input.submit("10;10");
    CHECK(semantic_result.accepted);
    movePointer(
        interaction,
        sketch_id,
        160.0,
        160.0,
        200.0,
        300.0);
    const auto lines_before_rectangle =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();
    semantic_result = inch_input.submit("2;1");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_rectangle + 4U);
    const auto& rectangle_first_edge =
        model_state.lines[lines_before_rectangle];
    CHECK(near(rectangle_first_edge.start.u, 254.0));
    CHECK(near(rectangle_first_edge.start.v, 254.0));
    CHECK(near(rectangle_first_edge.end.u, 203.2));
    CHECK(near(rectangle_first_edge.end.v, 254.0));

    // ROTATE final stage owns an Angle request; bare values are degrees.
    CHECK(interaction.selectedCount() == 1U);
    CHECK(interaction.activateRotate());
    semantic_result = inch_input.submit("0;0");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("1;0");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("90");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, -50.0));
    CHECK(near(moved_source->start().v, 40.0));
    CHECK(near(moved_source->end().u, -80.0));
    CHECK(near(moved_source->end().v, 70.0));

    // SCALE final stage owns a positive dimensionless Factor. Invalid zero
    // fails closed without ending the transform; 0.5 then commits exactly.
    CHECK(interaction.activateScale());
    semantic_result = inch_input.submit("0;0");
    CHECK(semantic_result.accepted);
    semantic_result = inch_input.submit("1;0");
    CHECK(semantic_result.accepted);
    const auto revision_before_bad_factor =
        session.document().revision();
    semantic_result = inch_input.submit("0");
    CHECK(!semantic_result.accepted);
    CHECK(
        session.document().revision() ==
        revision_before_bad_factor);
    CHECK(
        interaction.commonTransformStage() ==
        sketch::CommonTransformStage::
            await_destination);
    semantic_result = inch_input.submit("0.5");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, -25.0));
    CHECK(near(moved_source->start().v, 20.0));
    CHECK(near(moved_source->end().u, -40.0));
    CHECK(near(moved_source->end().v, 35.0));

    // Grip Rotate: exact Angle is signed and pointer-independent.
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(
        interaction,
        sketch_id,
        210.0,
        210.0,
        -25.0,
        20.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::rotate);
    semantic_result = semantic_input.submit("90");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, -25.0));
    CHECK(near(moved_source->start().v, 20.0));
    CHECK(near(moved_source->end().u, -40.0));
    CHECK(near(moved_source->end().v, 5.0));

    // Grip Scale: exact positive Factor owns the final geometry.
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(
        interaction,
        sketch_id,
        220.0,
        220.0,
        -25.0,
        20.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::scale);
    const auto revision_before_bad_grip_factor =
        session.document().revision();
    semantic_result = semantic_input.submit("0");
    CHECK(!semantic_result.accepted);
    CHECK(
        session.document().revision() ==
        revision_before_bad_grip_factor);
    CHECK(interaction.directManipulationActive());
    semantic_result = semantic_input.submit("2");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, -25.0));
    CHECK(near(moved_source->start().v, 20.0));
    CHECK(near(moved_source->end().u, -55.0));
    CHECK(near(moved_source->end().v, -10.0));

    // Grip Mirror: typed Axis Angle is absolute from Sketch +U.
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(
        interaction,
        sketch_id,
        230.0,
        230.0,
        -25.0,
        20.0);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::mirror);
    semantic_result = semantic_input.submit("90");
    CHECK(semantic_result.accepted);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    moved_source = hosted->model.findLine(source_id);
    CHECK(moved_source != nullptr);
    CHECK(near(moved_source->start().u, -25.0));
    CHECK(near(moved_source->start().v, 20.0));
    CHECK(near(moved_source->end().u, 5.0));
    CHECK(near(moved_source->end().v, -10.0));

    // R11 static OSNAP resolves exact Sketch geometry from logical screen
    // aperture before Polar. A remote target avoids accidental capture from
    // the earlier precision-input geometry in this integration test.
    const auto snap_target_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {5000.0, 5007.0},
                {5010.0, 5007.0},
                sketch::EntityRole::regular});
    CHECK(snap_target_result.ok());
    CHECK(snap_target_result.changed);
    viewport_controller.refreshPresentation();

    application::CadInteractionSettings snap_settings;
    snap_settings.polar.primary_spacing =
        std::numbers::pi_v<double> / 4.0;
    interaction.setCadInteractionSettingsProvider(
        [&snap_settings] {
            return snap_settings;
        });

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            4900.0,
            4900.0}));
    const auto lines_before_osnap =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();

    // Raw pointer is closer to a 45-degree Polar ray than to the exact
    // endpoint, but OSNAP has higher semantic priority.
    movePointer(
        interaction,
        sketch_id,
        5004.0,
        5010.0,
        5004.0,
        5010.0);
    auto point_resolution =
        interaction.pointResolution();
    CHECK(point_resolution.has_value());
    CHECK(
        point_resolution->source ==
        sketch::PointResolutionSource::object_snap);
    CHECK(point_resolution->object_snap.has_value());
    CHECK((
        point_resolution->position ==
        sketch::Point2{5000.0, 5007.0}));
    CHECK(
        viewport.snap_inference_scene_.current.
            has_value());
    CHECK(
        viewport.snap_inference_scene_.current->
            kind ==
        viewer::SketchSnapMarkerKind::endpoint);
    CHECK(
        viewport.snap_inference_scene_.current->
            label == "END");
    CHECK(near(
        viewport.snap_inference_scene_.current->
            position.x,
        5000.0));
    CHECK(near(
        viewport.snap_inference_scene_.current->
            position.y,
        5007.0));
    CHECK(
        viewport.snap_inference_scene_.acquired.
            empty());

    click(
        interaction,
        sketch_id,
        5004.0,
        5010.0,
        5004.0,
        5010.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_osnap + 1U);
    CHECK((
        model_state.lines.back().start ==
        sketch::Point2{4900.0, 4900.0}));
    CHECK((
        model_state.lines.back().end ==
        sketch::Point2{5000.0, 5007.0}));
    CHECK(interaction.escape());

    // Master OFF preserves mode choices but removes OSNAP from pointer
    // resolution. With Polar also OFF, the same near-target click stays raw.
    snap_settings.object_snap.master_enabled = false;
    snap_settings.polar.enabled = false;
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            4900.0,
            4900.0}));
    const auto lines_before_raw =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();
    movePointer(
        interaction,
        sketch_id,
        5004.0,
        5010.0,
        5004.0,
        5010.0);
    point_resolution =
        interaction.pointResolution();
    CHECK(point_resolution.has_value());
    CHECK(
        point_resolution->source ==
        sketch::PointResolutionSource::raw_pointer);
    CHECK(!point_resolution->object_snap.has_value());
    CHECK(
        !viewport.snap_inference_scene_.current.
             has_value());
    CHECK(
        viewport.snap_inference_scene_.acquired.
            empty());

    click(
        interaction,
        sketch_id,
        5004.0,
        5010.0,
        5004.0,
        5010.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_raw + 1U);
    CHECK((
        model_state.lines.back().end ==
        sketch::Point2{5004.0, 5010.0}));
    CHECK(interaction.escape());

    // Intersection uses a bounded crossing query around the logical-pixel
    // aperture, then exact Shared2D finite-curve relation authority.
    const auto int_horizontal_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {5190.0, 5200.0},
                {5230.0, 5200.0},
                sketch::EntityRole::regular});
    const auto int_vertical_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {5200.0, 5170.0},
                {5200.0, 5220.0},
                sketch::EntityRole::construction});
    CHECK(int_horizontal_result.ok());
    CHECK(int_horizontal_result.changed);
    CHECK(int_vertical_result.ok());
    CHECK(int_vertical_result.changed);

    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    const auto int_horizontal =
        model_state.lines[
            model_state.lines.size() - 2U].id;
    const auto int_vertical =
        model_state.lines.back().id;

    viewport_controller.refreshPresentation();
    const auto int_horizontal_token =
        viewport_controller.sketchPresentationFor(
            int_horizontal);
    const auto int_vertical_token =
        viewport_controller.sketchPresentationFor(
            int_vertical);
    CHECK(int_horizontal_token.has_value());
    CHECK(int_vertical_token.has_value());
    viewport.rectangle_query_ = {
        true,
        {
            *int_vertical_token,
            *int_horizontal_token,
        }};

    application::CadInteractionSettings int_settings;
    int_settings.polar.enabled = false;
    int_settings.object_snap.endpoint = false;
    int_settings.object_snap.midpoint = false;
    int_settings.object_snap.center = false;
    int_settings.object_snap.quadrant = false;
    int_settings.object_snap.origin = false;
    int_settings.object_snap.intersection = true;
    interaction.setCadInteractionSettingsProvider(
        [&int_settings] {
            return int_settings;
        });

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            5100.0,
            5100.0}));

    movePointer(
        interaction,
        sketch_id,
        5204.0,
        5203.0,
        5204.0,
        5203.0);
    const auto int_resolution =
        interaction.pointResolution();
    CHECK(int_resolution.has_value());
    CHECK(
        int_resolution->source ==
        sketch::PointResolutionSource::object_snap);
    CHECK(int_resolution->object_snap.has_value());
    CHECK(
        int_resolution->object_snap->kind ==
        sketch::SnapKind::intersection);
    CHECK((
        int_resolution->position ==
        sketch::Point2{5200.0, 5200.0}));
    CHECK(viewport.last_rectangle_query_.has_value());
    CHECK(viewport.last_rectangle_rule_.has_value());
    CHECK(
        *viewport.last_rectangle_rule_ ==
        viewer::SketchRectangleSelectionRule::crossing);
    CHECK(near(
        viewport.last_rectangle_query_->
            minimum.x,
        5195.0));
    CHECK(near(
        viewport.last_rectangle_query_->
            minimum.y,
        5194.0));
    CHECK(near(
        viewport.last_rectangle_query_->
            maximum.x,
        5213.0));
    CHECK(near(
        viewport.last_rectangle_query_->
            maximum.y,
        5212.0));

    const auto lines_before_int =
        hosted->model.state().lines.size();
    click(
        interaction,
        sketch_id,
        5204.0,
        5203.0,
        5204.0,
        5203.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_int + 1U);
    CHECK((
        model_state.lines.back().end ==
        sketch::Point2{5200.0, 5200.0}));
    CHECK(interaction.escape());
    viewport.rectangle_query_ = {true, {}};

    // OSNAP remains available under partial Dynamic Input locks. A compatible
    // Endpoint fills the free coordinate and keeps exact snap provenance;
    // an incompatible Endpoint is rejected and cannot advertise a marker.
    const auto locked_snap_source_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {11000.0, 11020.0},
                {11030.0, 11020.0},
                sketch::EntityRole::regular});
    CHECK(
        locked_snap_source_result.ok() &&
        locked_snap_source_result.changed);
    viewport_controller.refreshPresentation();

    application::CadInteractionSettings locked_snap_settings;
    locked_snap_settings.polar.enabled = false;
    locked_snap_settings.object_snap.endpoint = true;
    locked_snap_settings.object_snap.midpoint = false;
    locked_snap_settings.object_snap.center = false;
    locked_snap_settings.object_snap.quadrant = false;
    locked_snap_settings.object_snap.intersection = false;
    locked_snap_settings.object_snap.origin = false;
    interaction.setCadInteractionSettingsProvider(
        [&locked_snap_settings] {
            return locked_snap_settings;
        });

    interaction.activateLine();
    CHECK(interaction.lockCadInputSemanticPointField(
        application::CadDynamicInputFieldSemantic::u,
        11000.0));
    movePointer(
        interaction,
        sketch_id,
        11004.0,
        11023.0,
        11004.0,
        11023.0);
    auto locked_snap_resolution =
        interaction.pointResolution();
    CHECK(locked_snap_resolution.has_value());
    CHECK(
        locked_snap_resolution->source ==
        sketch::PointResolutionSource::
            numeric_lock);
    CHECK(
        locked_snap_resolution->object_snap.
            has_value());
    CHECK(
        locked_snap_resolution->object_snap->kind ==
        sketch::SnapKind::endpoint);
    CHECK((
        locked_snap_resolution->position ==
        sketch::Point2{11000.0, 11020.0}));

    CHECK(interaction.lockCadInputSemanticPointField(
        application::CadDynamicInputFieldSemantic::u,
        11001.0));
    movePointer(
        interaction,
        sketch_id,
        11004.0,
        11023.0,
        11004.0,
        11023.0);
    locked_snap_resolution =
        interaction.pointResolution();
    CHECK(locked_snap_resolution.has_value());
    CHECK(
        locked_snap_resolution->source ==
        sketch::PointResolutionSource::
            numeric_lock);
    CHECK(
        !locked_snap_resolution->object_snap.
             has_value());
    CHECK((
        locked_snap_resolution->position ==
        sketch::Point2{11001.0, 11023.0}));
    CHECK(interaction.escape());
    CHECK(interaction.escape());

    // Request-relative PER uses the active request base and exact finite
    // source geometry from the bounded nearby-source set.
    const auto per_source_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {6090.0, 6100.0},
                {6130.0, 6100.0},
                sketch::EntityRole::regular});
    CHECK(per_source_result.ok());
    CHECK(per_source_result.changed);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    const auto per_source =
        model_state.lines.back().id;
    viewport_controller.refreshPresentation();
    const auto per_token =
        viewport_controller.sketchPresentationFor(
            per_source);
    CHECK(per_token.has_value());
    viewport.rectangle_query_ = {true, {*per_token}};

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            6105.0,
            6050.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            perpendicular));
    movePointer(
        interaction,
        sketch_id,
        6108.0,
        6103.0,
        6108.0,
        6103.0);
    auto local_resolution =
        interaction.pointResolution();
    CHECK(local_resolution.has_value());
    CHECK(
        local_resolution->source ==
        sketch::PointResolutionSource::object_snap);
    CHECK(local_resolution->object_snap.has_value());
    CHECK(
        local_resolution->object_snap->kind ==
        sketch::SnapKind::perpendicular);
    CHECK((
        local_resolution->position ==
        sketch::Point2{6105.0, 6100.0}));
    CHECK(interaction.escape());

    // NEA is a continuous finite-curve projection and remains an explicit
    // one-shot family here because its persistent default is OFF.
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            6000.0,
            6000.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            nearest));
    movePointer(
        interaction,
        sketch_id,
        6112.0,
        6104.0,
        6112.0,
        6104.0);
    local_resolution =
        interaction.pointResolution();
    CHECK(local_resolution.has_value());
    CHECK(local_resolution->object_snap.has_value());
    CHECK(
        local_resolution->object_snap->kind ==
        sketch::SnapKind::nearest);
    CHECK((
        local_resolution->position ==
        sketch::Point2{6112.0, 6100.0}));
    CHECK(interaction.escape());

    // TAN-from-point uses exact supporting-circle geometry. The pointer only
    // chooses between admissible tangent branches in screen space.
    const auto tangent_circle_result =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {6200.0, 6200.0},
                10.0,
                sketch::EntityRole::regular});
    CHECK(tangent_circle_result.ok());
    CHECK(tangent_circle_result.changed);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    const auto tangent_circle =
        model_state.circles.back().id;
    viewport_controller.refreshPresentation();
    const auto tangent_token =
        viewport_controller.sketchPresentationFor(
            tangent_circle);
    CHECK(tangent_token.has_value());
    viewport.rectangle_query_ = {
        true,
        {*tangent_token}};

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            6230.0,
            6200.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            tangent));
    movePointer(
        interaction,
        sketch_id,
        6204.0,
        6209.0,
        6204.0,
        6209.0);
    local_resolution =
        interaction.pointResolution();
    CHECK(local_resolution.has_value());
    CHECK(local_resolution->object_snap.has_value());
    CHECK(
        local_resolution->object_snap->kind ==
        sketch::SnapKind::tangent);
    CHECK(near(
        local_resolution->position.u,
        6200.0 + 100.0 / 30.0));
    CHECK(near(
        local_resolution->position.v,
        6200.0 +
            10.0 * std::sqrt(8.0 / 9.0)));
    CHECK(interaction.escape());

    // A non-NONE Temporary Override is restrictive. With no requested-family
    // candidate there is no raw or Polar fallback and stale resolution is
    // explicitly cleared.
    viewport.rectangle_query_ = {true, {}};
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            7000.0,
            7000.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            endpoint));
    movePointer(
        interaction,
        sketch_id,
        7100.0,
        7100.0,
        7100.0,
        7100.0);
    CHECK(!interaction.pointResolution().has_value());
    CHECK(interaction.clearTemporarySnapOverride());
    movePointer(
        interaction,
        sketch_id,
        7100.0,
        7100.0,
        7100.0,
        7100.0);
    local_resolution =
        interaction.pointResolution();
    CHECK(local_resolution.has_value());
    CHECK(
        local_resolution->source ==
        sketch::PointResolutionSource::raw_pointer);
    CHECK(interaction.escape());

    // OTRACK: deliberate 400 ms dwell acquires semantic anchors. Two
    // anchors are retained with no FIFO eviction. Their U/V GuideIntersection
    // outranks Polar/raw and resolves through the same PointResolution path.
    const auto track_a_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {8000.0, 8000.0},
                {8040.0, 8000.0},
                sketch::EntityRole::regular});
    const auto track_b_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {8100.0, 8100.0},
                {8140.0, 8100.0},
                sketch::EntityRole::construction});
    const auto track_c_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {8200.0, 8200.0},
                {8240.0, 8200.0},
                sketch::EntityRole::regular});
    CHECK(track_a_result.ok() && track_a_result.changed);
    CHECK(track_b_result.ok() && track_b_result.changed);
    CHECK(track_c_result.ok() && track_c_result.changed);
    viewport_controller.refreshPresentation();

    application::CadInteractionSettings tracking_settings;
    tracking_settings.polar.enabled = false;
    tracking_settings.object_snap.endpoint = true;
    tracking_settings.object_snap.midpoint = false;
    tracking_settings.object_snap.center = false;
    tracking_settings.object_snap.quadrant = false;
    tracking_settings.object_snap.intersection = false;
    tracking_settings.object_snap.origin = false;
    tracking_settings.object_snap.object_tracking_enabled = true;
    interaction.setCadInteractionSettingsProvider(
        [&tracking_settings] {
            return tracking_settings;
        });

    auto tracking_now =
        std::chrono::steady_clock::time_point{};
    interaction.setTrackingClockProvider(
        [&tracking_now] {
            return tracking_now;
        });

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            7900.0,
            7900.0}));

    movePointer(
        interaction,
        sketch_id,
        8000.0,
        8000.0,
        8000.0,
        8000.0);
    CHECK(interaction.trackingAnchorCount() == 0U);
    tracking_now += std::chrono::milliseconds{400};
    movePointer(
        interaction,
        sketch_id,
        8000.0,
        8000.0,
        8000.0,
        8000.0);
    CHECK(interaction.trackingAnchorCount() == 1U);
    CHECK(
        viewport.snap_inference_scene_.acquired.
            size() == 1U);
    CHECK(
        viewport.snap_inference_scene_.acquired[0].
            kind ==
        viewer::SketchSnapMarkerKind::endpoint);
    CHECK(
        viewport.snap_inference_scene_.acquired[0].
            label == "END");
    CHECK(near(
        viewport.snap_inference_scene_.acquired[0].
            position.x,
        8000.0));
    CHECK(near(
        viewport.snap_inference_scene_.acquired[0].
            position.y,
        8000.0));

    movePointer(
        interaction,
        sketch_id,
        8100.0,
        8100.0,
        8100.0,
        8100.0);
    tracking_now += std::chrono::milliseconds{400};
    movePointer(
        interaction,
        sketch_id,
        8100.0,
        8100.0,
        8100.0,
        8100.0);
    CHECK(interaction.trackingAnchorCount() == 2U);

    movePointer(
        interaction,
        sketch_id,
        8200.0,
        8200.0,
        8200.0,
        8200.0);
    tracking_now += std::chrono::milliseconds{400};
    movePointer(
        interaction,
        sketch_id,
        8200.0,
        8200.0,
        8200.0,
        8200.0);
    CHECK(interaction.trackingAnchorCount() == 2U);

    movePointer(
        interaction,
        sketch_id,
        8004.0,
        8104.0,
        8004.0,
        8104.0);
    auto tracking_resolution =
        interaction.pointResolution();
    CHECK(tracking_resolution.has_value());
    CHECK(
        tracking_resolution->source ==
        sketch::PointResolutionSource::
            tracking_inference);
    CHECK((
        tracking_resolution->position ==
        sketch::Point2{8000.0, 8100.0}));

    tracking_settings.object_snap.object_tracking_enabled = false;
    movePointer(
        interaction,
        sketch_id,
        8004.0,
        8104.0,
        8004.0,
        8104.0);
    tracking_resolution =
        interaction.pointResolution();
    CHECK(tracking_resolution.has_value());
    CHECK(
        tracking_resolution->source ==
        sketch::PointResolutionSource::raw_pointer);
    CHECK(interaction.trackingAnchorCount() == 2U);
    CHECK(
        viewport.snap_inference_scene_.acquired.
            empty());

    tracking_settings.object_snap.object_tracking_enabled = true;
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::none));
    movePointer(
        interaction,
        sketch_id,
        8004.0,
        8104.0,
        8004.0,
        8104.0);
    tracking_resolution =
        interaction.pointResolution();
    CHECK(tracking_resolution.has_value());
    CHECK(
        tracking_resolution->source ==
        sketch::PointResolutionSource::raw_pointer);
    CHECK(interaction.trackingAnchorCount() == 2U);
    CHECK(interaction.clearTemporarySnapOverride());

    movePointer(
        interaction,
        sketch_id,
        8004.0,
        8104.0,
        8004.0,
        8104.0);
    tracking_resolution =
        interaction.pointResolution();
    CHECK(tracking_resolution.has_value());
    CHECK(
        tracking_resolution->source ==
        sketch::PointResolutionSource::
            tracking_inference);

    const auto lines_before_tracking_commit =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();
    click(
        interaction,
        sketch_id,
        8004.0,
        8104.0,
        8004.0,
        8104.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_tracking_commit + 1U);
    CHECK((
        model_state.lines.back().end ==
        sketch::Point2{8000.0, 8100.0}));
    CHECK(interaction.trackingAnchorCount() == 0U);
    CHECK(interaction.escape());

    // EXT is two-phase runtime inference: acquire one Line endpoint
    // reference, then resolve only the positive ray beyond that endpoint.
    const auto extension_line_result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {9000.0, 9000.0},
                {9010.0, 9000.0},
                sketch::EntityRole::regular});
    CHECK(
        extension_line_result.ok() &&
        extension_line_result.changed);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    const auto extension_line =
        model_state.lines.back().id;
    viewport_controller.refreshPresentation();
    const auto extension_token =
        viewport_controller.sketchPresentationFor(
            extension_line);
    CHECK(extension_token.has_value());
    viewport.rectangle_query_ = {
        true,
        {*extension_token}};

    application::CadInteractionSettings extension_settings;
    extension_settings.polar.enabled = false;
    extension_settings.object_snap.endpoint = false;
    extension_settings.object_snap.midpoint = false;
    extension_settings.object_snap.center = false;
    extension_settings.object_snap.quadrant = false;
    extension_settings.object_snap.intersection = false;
    extension_settings.object_snap.origin = false;
    interaction.setCadInteractionSettingsProvider(
        [&extension_settings] {
            return extension_settings;
        });

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            8900.0,
            8900.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            extension));

    movePointer(
        interaction,
        sketch_id,
        9010.0,
        9000.0,
        9010.0,
        9000.0);
    CHECK(!interaction.pointResolution().has_value());
    auto extension_request =
        interaction.activePointRequest();
    CHECK(extension_request.has_value());
    CHECK(
        extension_request->deferred_snap_reference.
            has_value());
    CHECK(
        extension_request->deferred_snap_reference->
            kind ==
        sketch::DeferredSnapReferenceKind::
            line_extension);

    movePointer(
        interaction,
        sketch_id,
        9020.0,
        9004.0,
        9020.0,
        9004.0);
    auto extension_resolution =
        interaction.pointResolution();
    CHECK(extension_resolution.has_value());
    CHECK(
        extension_resolution->source ==
        sketch::PointResolutionSource::
            tracking_inference);
    CHECK((
        extension_resolution->position ==
        sketch::Point2{9020.0, 9000.0}));

    const auto lines_before_extension =
        hosted->model.state().lines.size();
    click(
        interaction,
        sketch_id,
        9020.0,
        9004.0,
        9020.0,
        9004.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_extension + 1U);
    CHECK((
        model_state.lines.back().end ==
        sketch::Point2{9020.0, 9000.0}));
    CHECK(
        !interaction.activePointRequest()->
             deferred_snap_reference.has_value());
    CHECK(interaction.escape());

    // EXT never applies to the finite segment itself.
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            8900.0,
            8900.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            extension));
    movePointer(
        interaction,
        sketch_id,
        9010.0,
        9000.0,
        9010.0,
        9000.0);
    movePointer(
        interaction,
        sketch_id,
        9005.0,
        9003.0,
        9005.0,
        9003.0);
    CHECK(!interaction.pointResolution().has_value());
    CHECK(interaction.escape());

    // Switching the one-shot family EXT -> PER preserves the request-local
    // Extension reference. The exact perpendicular foot may lie outside the
    // finite Line and still resolve on the explicit positive ray.
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            9020.0,
            9010.0}));
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            extension));
    movePointer(
        interaction,
        sketch_id,
        9010.0,
        9000.0,
        9010.0,
        9000.0);
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::
            perpendicular));
    movePointer(
        interaction,
        sketch_id,
        9020.0,
        9004.0,
        9020.0,
        9004.0);
    extension_resolution =
        interaction.pointResolution();
    CHECK(extension_resolution.has_value());
    CHECK(
        extension_resolution->source ==
        sketch::PointResolutionSource::
            tracking_inference);
    CHECK((
        extension_resolution->position ==
        sketch::Point2{9020.0, 9000.0}));
    CHECK(interaction.escape());
    viewport.rectangle_query_ = {true, {}};

    // Deferred/Common TAN: first TAN hover captures one Circle source but
    // no point. Second Circle selects an exact common-tangent branch; preview
    // and commit use the two exact contact points as one ordinary Line.
    const auto common_first_result =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {10000.0, 10000.0},
                10.0,
                sketch::EntityRole::regular});
    const auto common_second_result =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {10040.0, 10000.0},
                10.0,
                sketch::EntityRole::construction});
    CHECK(common_first_result.ok() &&
          common_first_result.changed);
    CHECK(common_second_result.ok() &&
          common_second_result.changed);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    const auto common_first =
        model_state.circles[
            model_state.circles.size() - 2U].id;
    const auto common_second =
        model_state.circles.back().id;
    viewport_controller.refreshPresentation();
    const auto common_first_token =
        viewport_controller.sketchPresentationFor(
            common_first);
    const auto common_second_token =
        viewport_controller.sketchPresentationFor(
            common_second);
    CHECK(common_first_token.has_value());
    CHECK(common_second_token.has_value());

    application::CadInteractionSettings common_settings;
    common_settings.polar.enabled = false;
    common_settings.object_snap.endpoint = false;
    common_settings.object_snap.midpoint = false;
    common_settings.object_snap.center = false;
    common_settings.object_snap.quadrant = false;
    common_settings.object_snap.intersection = false;
    common_settings.object_snap.origin = false;
    interaction.setCadInteractionSettingsProvider(
        [&common_settings] {
            return common_settings;
        });

    interaction.activateLine();
    CHECK(interaction.setTemporarySnapOverride(
        sketch::TemporarySnapOverrideKind::tangent));
    viewport.rectangle_query_ = {
        true,
        {*common_first_token}};
    movePointer(
        interaction,
        sketch_id,
        10000.0,
        10010.0,
        10000.0,
        10010.0);
    CHECK(!interaction.pointResolution().has_value());
    auto common_request =
        interaction.activePointRequest();
    CHECK(common_request.has_value());
    CHECK(
        common_request->deferred_snap_reference.
            has_value());
    CHECK(
        common_request->deferred_snap_reference->
            kind ==
        sketch::DeferredSnapReferenceKind::
            tangent_curve);

    const auto lines_before_common =
        hosted->model.state().lines.size();
    viewport.rectangle_query_ = {
        true,
        {*common_second_token}};
    movePointer(
        interaction,
        sketch_id,
        10040.0,
        10010.0,
        10040.0,
        10010.0);
    const auto common_resolution =
        interaction.pointResolution();
    CHECK(common_resolution.has_value());
    CHECK(
        common_resolution->source ==
        sketch::PointResolutionSource::object_snap);
    CHECK(common_resolution->object_snap.has_value());
    CHECK(
        common_resolution->object_snap->kind ==
        sketch::SnapKind::tangent);
    CHECK(near(
        common_resolution->position.u,
        10000.0));
    CHECK(near(
        common_resolution->position.v,
        10010.0));
    CHECK(viewport.preview_scene_.lines.size() == 1U);
    CHECK(near(
        viewport.preview_scene_.lines[0].start.x,
        10000.0));
    CHECK(near(
        viewport.preview_scene_.lines[0].start.y,
        10010.0));
    CHECK(near(
        viewport.preview_scene_.lines[0].end.x,
        10040.0));
    CHECK(near(
        viewport.preview_scene_.lines[0].end.y,
        10010.0));

    click(
        interaction,
        sketch_id,
        10040.0,
        10010.0,
        10040.0,
        10010.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_common + 1U);
    CHECK(near(
        model_state.lines.back().start.u,
        10000.0));
    CHECK(near(
        model_state.lines.back().start.v,
        10010.0));
    CHECK(near(
        model_state.lines.back().end.u,
        10040.0));
    CHECK(near(
        model_state.lines.back().end.v,
        10010.0));
    common_request = interaction.activePointRequest();
    CHECK(common_request.has_value());
    CHECK(
        !common_request->deferred_snap_reference.
             has_value());
    CHECK(interaction.escape());
    viewport.rectangle_query_ = {true, {}};

    // Polar is a logical-screen-space magnet. With 90-degree Absolute
    // tracks, (20,1) captures +U. Direct Distance owns magnitude only.
    application::CadInteractionSettings polar_settings;
    polar_settings.object_snap.master_enabled = false;
    polar_settings.polar.primary_spacing =
        std::numbers::pi_v<double> / 2.0;
    interaction.setCadInteractionSettingsProvider(
        [&polar_settings] {
            return polar_settings;
        });

    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            0.0,
            0.0}));
    const auto line_count_before_polar =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();
    movePointer(
        interaction,
        sketch_id,
        20.0,
        1.0,
        20.0,
        1.0);

    point_resolution =
        interaction.pointResolution();
    CHECK(point_resolution.has_value());
    CHECK(
        point_resolution->source ==
        sketch::PointResolutionSource::polar);
    CHECK(!point_resolution->object_snap.has_value());

    auto dyn_snapshots =
        semantic_input.dynamicInputFieldSnapshots();
    CHECK(dyn_snapshots.size() == 4U);
    CHECK(
        dyn_snapshots[0].value->state ==
        application::CadDynamicInputValueState::free);
    CHECK(near(
        dyn_snapshots[0].value->canonical_value,
        std::sqrt(401.0)));
    CHECK(
        dyn_snapshots[1].value->state ==
        application::CadDynamicInputValueState::assisted);
    CHECK(near(
        dyn_snapshots[1].value->canonical_value,
        0.0));
    CHECK(
        dyn_snapshots[2].value->state ==
        application::CadDynamicInputValueState::assisted);
    CHECK(
        dyn_snapshots[3].value->state ==
        application::CadDynamicInputValueState::assisted);

    CHECK(interaction.submitDirectDistance(100.0));
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        line_count_before_polar + 1U);
    CHECK(near(model_state.lines.back().start.u, 0.0));
    CHECK(near(model_state.lines.back().start.v, 0.0));
    CHECK(near(model_state.lines.back().end.u, 100.0));
    CHECK(near(model_state.lines.back().end.v, 0.0));

    // Arc Start -> End uses the same Polar-resolved point as a visible
    // helper chord before the End point is accepted.
    interaction.activateArc();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            10.0,
            10.0}));
    movePointer(
        interaction,
        sketch_id,
        30.0,
        1.0,
        30.0,
        11.0);
    CHECK(viewport.preview_scene_.lines.size() == 1U);
    CHECK(near(viewport.preview_scene_.lines[0].start.x, 10.0));
    CHECK(near(viewport.preview_scene_.lines[0].start.y, 10.0));
    CHECK(viewport.preview_scene_.lines[0].end.x > 30.0);
    CHECK(viewport.preview_scene_.lines[0].end.y == 10.0);
    CHECK(interaction.escape());

    // Precision polar coordinates must be able to author an exactly
    // closed cardinal loop. Region/Profile topology is exact by design,
    // so R10 must not manufacture sub-floating-point gaps at 90-degree
    // directions that are semantically exact.
    interaction.activateLine();
    const auto closed_loop_first_line =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();
    CHECK(semantic_input.submit("0;0").accepted);
    CHECK(semantic_input.submit("@100<0").accepted);
    CHECK(semantic_input.submit("@50<90").accepted);
    CHECK(semantic_input.submit("@100<180").accepted);
    CHECK(semantic_input.submit("@50<270").accepted);

    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        closed_loop_first_line + 4U);
    const auto& closing_line = model_state.lines.back();
    CHECK(closing_line.end.u == 0.0);
    CHECK(closing_line.end.v == 0.0);

    sketch::SketchModel exact_closed_loop;
    for (std::size_t index = closed_loop_first_line;
         index < model_state.lines.size();
         ++index) {
        static_cast<void>(
            exact_closed_loop.addLine(
                model_state.lines[index].start,
                model_state.lines[index].end));
    }
    const auto closed_analysis =
        sketch::analyzeRegions(exact_closed_loop);
    CHECK(closed_analysis.regions.size() == 1U);
    CHECK(closed_analysis.diagnostics.empty());
    CHECK(interaction.escape());

    // Manual-workflow regression: pointer Polar attraction followed by
    // bare Direct Distance must preserve the same exact cardinal closure.
    // This is the path used when a rectangle is drawn as four LINE
    // segments with Polar -> distance -> Polar -> distance.
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            1000.0,
            1000.0}));
    const auto polar_distance_first_line =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();

    movePointer(interaction, sketch_id, 1020.0, 1001.0, 1020.0, 1001.0);
    CHECK(interaction.submitDirectDistance(100.0));
    movePointer(interaction, sketch_id, 1101.0, 1020.0, 1101.0, 1020.0);
    CHECK(interaction.submitDirectDistance(50.0));
    movePointer(interaction, sketch_id, 1080.0, 1051.0, 1080.0, 1051.0);
    CHECK(interaction.submitDirectDistance(100.0));
    movePointer(interaction, sketch_id, 999.0, 1030.0, 999.0, 1030.0);
    CHECK(interaction.submitDirectDistance(50.0));

    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        polar_distance_first_line + 4U);
    CHECK(model_state.lines.back().end.u == 1000.0);
    CHECK(model_state.lines.back().end.v == 1000.0);

    sketch::SketchModel polar_distance_loop;
    for (std::size_t index = polar_distance_first_line;
         index < model_state.lines.size();
         ++index) {
        static_cast<void>(
            polar_distance_loop.addLine(
                model_state.lines[index].start,
                model_state.lines[index].end));
    }
    const auto polar_distance_analysis =
        sketch::analyzeRegions(polar_distance_loop);
    CHECK(polar_distance_analysis.regions.size() == 1U);
    CHECK(polar_distance_analysis.diagnostics.empty());
    CHECK(interaction.escape());

    // Relative without an explicit semantic reference never falls back
    // to Absolute. The same pointer remains raw.
    polar_settings.polar.reference_mode =
        application::PolarReferenceMode::relative;
    interaction.activateLine();
    CHECK(interaction.submitExplicitPoint(
        {
            sketch::ExplicitPointInputKind::
                absolute_cartesian,
            0.0,
            0.0}));
    movePointer(
        interaction,
        sketch_id,
        20.0,
        1.0,
        20.0,
        1.0);
    CHECK(interaction.submitDirectDistance(100.0));
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(model_state.lines.back().end.v > 4.0);
    CHECK(model_state.lines.back().end.v < 6.0);

    // Dynamic Input point-field locks route through the same semantic
    // endpoint and existing PointRequest resolver.
    interaction.activateLine();

    auto dyn_lock =
        semantic_input.lockDynamicInputField(
            0U,
            "10");
    CHECK(dyn_lock.accepted);
    dyn_lock =
        semantic_input.lockDynamicInputField(
            1U,
            "20mm");
    CHECK(dyn_lock.accepted);

    dyn_snapshots =
        semantic_input.dynamicInputFieldSnapshots();
    CHECK(dyn_snapshots.size() == 2U);
    CHECK(
        dyn_snapshots[0].value->state ==
        application::CadDynamicInputValueState::locked);
    CHECK(near(
        dyn_snapshots[0].value->canonical_value,
        10.0));
    CHECK(
        dyn_snapshots[1].value->state ==
        application::CadDynamicInputValueState::locked);
    CHECK(near(
        dyn_snapshots[1].value->canonical_value,
        20.0));

    click(
        interaction,
        sketch_id,
        300.0,
        300.0,
        1.0,
        2.0);
    CHECK(
        interaction.lineStage() ==
        sketch::LineStage::await_next_point);

    dyn_lock =
        semantic_input.lockDynamicInputField(
            0U,
            "100");
    CHECK(dyn_lock.accepted);
    dyn_lock =
        semantic_input.lockDynamicInputField(
            1U,
            "90");
    CHECK(dyn_lock.accepted);

    dyn_snapshots =
        semantic_input.dynamicInputFieldSnapshots();
    CHECK(dyn_snapshots.size() == 4U);
    CHECK(
        dyn_snapshots[0].value->state ==
        application::CadDynamicInputValueState::locked);
    CHECK(near(
        dyn_snapshots[0].value->canonical_value,
        100.0));
    CHECK(
        dyn_snapshots[1].value->state ==
        application::CadDynamicInputValueState::locked);
    CHECK(near(
        dyn_snapshots[1].value->canonical_value,
        std::numbers::pi_v<double> / 2.0));

    CHECK(interaction.submitCadInputDynamicRequest());

    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(!model_state.lines.empty());
    const auto& dyn_locked_line =
        model_state.lines.back();
    CHECK(near(dyn_locked_line.start.u, 10.0));
    CHECK(near(dyn_locked_line.start.v, 20.0));
    CHECK(near(dyn_locked_line.end.u, 10.0));
    CHECK(near(dyn_locked_line.end.v, 120.0));

    // Dynamic Input creation locks stay request-local and the normal
    // pointer commit path supplies only remaining placement/orientation.
    interaction.activateCircle();
    semantic_result = semantic_input.submit("0;0");
    CHECK(semantic_result.accepted);
    semantic_result = semantic_input.submit("D");
    CHECK(semantic_result.accepted);
    CHECK(
        interaction.circleSizeInputMode() ==
        application::CircleSizeInputMode::diameter);
    dyn_lock =
        semantic_input.lockDynamicInputField(
            0U,
            "20mm");
    CHECK(dyn_lock.accepted);
    const auto circles_before_dyn =
        session.document().findSketch(sketch_id)->
            model.state().circles.size();
    click(
        interaction,
        sketch_id,
        320.0,
        320.0,
        500.0,
        0.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.circles.size() ==
        circles_before_dyn + 1U);
    CHECK(near(model_state.circles.back().radius, 10.0));

    interaction.activateArc();
    semantic_result = semantic_input.submit("0;0");
    CHECK(semantic_result.accepted);
    semantic_result = semantic_input.submit("100;0");
    CHECK(semantic_result.accepted);
    dyn_lock =
        semantic_input.lockDynamicInputField(
            0U,
            "70mm");
    CHECK(dyn_lock.accepted);
    const auto arcs_before_dyn =
        session.document().findSketch(sketch_id)->
            model.state().arcs.size();
    click(
        interaction,
        sketch_id,
        330.0,
        330.0,
        50.0,
        30.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.arcs.size() ==
        arcs_before_dyn + 1U);
    CHECK(near(model_state.arcs.back().radius, 70.0));

    interaction.activateRectangle();
    semantic_result = semantic_input.submit("0;0");
    CHECK(semantic_result.accepted);
    dyn_lock =
        semantic_input.lockDynamicInputField(
            0U,
            "50mm");
    CHECK(dyn_lock.accepted);
    dyn_lock =
        semantic_input.lockDynamicInputField(
            1U,
            "30mm");
    CHECK(dyn_lock.accepted);
    const auto lines_before_dyn_rectangle =
        session.document().findSketch(sketch_id)->
            model.state().lines.size();
    click(
        interaction,
        sketch_id,
        340.0,
        340.0,
        -500.0,
        400.0);
    hosted = session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    model_state = hosted->model.state();
    CHECK(
        model_state.lines.size() ==
        lines_before_dyn_rectangle + 4U);
    const auto& dyn_rect_first =
        model_state.lines[
            lines_before_dyn_rectangle];
    CHECK(near(dyn_rect_first.start.u, 0.0));
    CHECK(near(dyn_rect_first.start.v, 0.0));
    CHECK(near(dyn_rect_first.end.u, -50.0));
    CHECK(near(dyn_rect_first.end.v, 0.0));

    // Empty-token Esc clears a request-local Grip Rotate Angle lock
    // without discarding the manipulation revision needed for a later commit.
    interaction.activateSelect();
    CHECK(viewport.selection_.primary.has_value());
    viewport.grip_query_ = {
        true,
        viewer::SketchGripKey{
            *viewport.selection_.primary,
            viewer::SketchGripRole::line_start}};
    click(
        interaction,
        sketch_id,
        430.0,
        430.0,
        dyn_locked_line.start.u,
        dyn_locked_line.start.v);
    CHECK(interaction.directManipulationActive());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(interaction.cycleDirectEditMode());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::rotate);

    dyn_lock =
        semantic_input.lockDynamicInputField(
            0U,
            "45");
    CHECK(dyn_lock.accepted);
    const auto revision_before_unlock_escape =
        session.document().revision();
    const auto undo_before_unlock_escape =
        session.undoDepth();

    CHECK(interaction.escape());
    CHECK(interaction.directManipulationActive());
    CHECK(
        interaction.directEditMode() ==
        sketch::DirectEditMode::rotate);
    CHECK(
        session.document().revision() ==
        revision_before_unlock_escape);
    CHECK(
        session.undoDepth() ==
        undo_before_unlock_escape);

    semantic_result = semantic_input.submit("90");
    CHECK(semantic_result.accepted);
    CHECK(!interaction.directManipulationActive());
    const auto revision_after_unlock_commit =
        revision_before_unlock_escape.next();
    CHECK(revision_after_unlock_commit.has_value());
    CHECK(
        session.document().revision() ==
        *revision_after_unlock_commit);
    CHECK(
        session.undoDepth() ==
        undo_before_unlock_escape + 1U);

    std::cout
        << "SK-07F precision input controller PASS\n";
    return EXIT_SUCCESS;
}
