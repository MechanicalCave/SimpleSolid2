#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
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
        return {true, {}};
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

    std::cout
        << "SK-07F precision input controller PASS\n";
    return EXIT_SUCCESS;
}
