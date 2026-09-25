#include "part_document_tree_controller.hpp"
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

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04B controller bridge CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

class TestViewport final
    : public QWidget,
      public viewer::IDocumentViewport {
public:
    using QWidget::QWidget;

    std::optional<viewer::CameraState>
    cameraState() const override {
        return camera_;
    }

    bool setCameraState(
        const viewer::CameraState& state) override {
        camera_ = state;
        return true;
    }

    bool setStandardView(
        viewer::StandardView) override {
        return true;
    }

    bool setProjection(
        viewer::CameraProjection) override {
        return true;
    }

    void fitAll() override {}

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        return scene.valid();
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        if (!scene.valid()) return false;
        sketch_scene_ = scene;
        return true;
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        return scene.valid();
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        if (!selection.valid()) return false;
        selection_ = selection;
        return true;
    }

    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return point_query_;
    }

    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule rule) override {
        if (!rectangle.valid()) return {};
        last_rule_ = rule;
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

    void emitReferenceIntent(
        const viewer::SelectionIntent& intent) {
        CHECK(static_cast<bool>(selection_handler_));
        selection_handler_(intent);
    }

    viewer::CameraState camera_;
    viewer::SketchScene sketch_scene_;
    viewer::PresentationSelection selection_;
    viewer::SketchPointQueryResult point_query_{
        true,
        std::nullopt};
    viewer::SketchRectangleQueryResult rectangle_query_{
        true,
        {}};
    std::optional<viewer::SketchRectangleSelectionRule>
        last_rule_;
    std::optional<viewer::SketchSelectionBoxOverlay>
        overlay_;
    viewer::SelectionIntentHandler selection_handler_;
    viewer::SpatialPointerHandler spatial_handler_;
    viewer::PrimaryPointerRouting routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
};

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "sk04b-controller.ss2part"},
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok());
    CHECK(created.sketch_id.has_value());
    const auto sketch_id = *created.sketch_id;

    const auto first =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{-10.0, 0.0},
                sketch::Point2{10.0, 0.0}});
    const auto second =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{0.0, -10.0},
                sketch::Point2{0.0, 10.0}});
    CHECK(first.ok() && first.entity_id.has_value());
    CHECK(second.ok() && second.entity_id.has_value());

    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    TestViewport viewport;
    ui::PartViewportController controller{
        tree_controller,
        &viewport};

    controller.setDocumentSession(&session);
    controller.setSketchEditSketch(sketch_id);
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    const auto first_token =
        viewport.sketch_scene_.lines[0].token;
    const auto second_token =
        viewport.sketch_scene_.lines[1].token;

    CHECK(
        controller.sketchPresentationFor(
            *first.entity_id) == first_token);
    CHECK(
        controller.sketchPresentationFor(
            *second.entity_id) == second_token);

    viewport.point_query_ = {
        true,
        first_token};
    const auto point_hit =
        controller.querySketchEntityAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(point_hit.completed);
    CHECK(point_hit.hit.has_value());
    CHECK(point_hit.hit->sketch_id == sketch_id);
    CHECK(point_hit.hit->entity_id == *first.entity_id);

    viewport.point_query_ = {
        true,
        std::nullopt};
    const auto point_empty =
        controller.querySketchEntityAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(point_empty.completed);
    CHECK(!point_empty.hit.has_value());

    viewport.point_query_ = {};
    const auto point_failed =
        controller.querySketchEntityAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(!point_failed.completed);

    const auto rectangle =
        viewer::normalizedViewportRect(
            viewer::ViewportPoint2{10.0, 10.0},
            viewer::ViewportPoint2{200.0, 150.0});
    CHECK(rectangle.has_value());

    viewport.rectangle_query_ = {
        true,
        {first_token, second_token}};
    const auto rectangle_hits =
        controller.querySketchEntities(
            *rectangle,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(rectangle_hits.completed);
    CHECK(rectangle_hits.hits.size() == 2U);
    CHECK(
        viewport.last_rule_ ==
        viewer::SketchRectangleSelectionRule::
            crossing);

    const auto revision_before_overlay =
        session.document().revision();
    const auto undo_before_overlay =
        session.undoDepth();
    const bool dirty_before_overlay =
        session.needsSave();

    const viewer::SketchSelectionBoxOverlay overlay{
        {20.0, 30.0},
        {140.0, 90.0},
        viewer::SketchRectangleSelectionRule::window};
    CHECK(
        controller.setSketchSelectionBoxOverlay(
            overlay));
    CHECK(viewport.overlay_ == overlay);
    controller.clearSketchSelectionBoxOverlay();
    CHECK(!viewport.overlay_.has_value());
    CHECK(
        session.document().revision() ==
        revision_before_overlay);
    CHECK(session.undoDepth() == undo_before_overlay);
    CHECK(session.needsSave() == dirty_before_overlay);

    // Existing built-in reference selection and Sketch highlight projection
    // can coexist in one runtime PresentationSelection.
    viewport.emitReferenceIntent(
        viewer::SelectionIntent{
            viewer::PresentationToken{0x101U},
            viewer::SelectionIntentMode::replace});
    CHECK(
        controller.projectSketchEntitySelection(
            {*first.entity_id, *second.entity_id},
            *second.entity_id));
    CHECK(viewport.selection_.selected.size() == 3U);
    CHECK(
        viewport.selection_.primary ==
        second_token);

    CHECK(!controller.projectSketchEntitySelection(
        {*first.entity_id},
        *second.entity_id));

    CHECK(
        controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            select_pick_box);

    CHECK(
        controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                create_edit_crosshair));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    CHECK(
        controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                select_pick_box));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);

    const auto stale_token = first_token;
    controller.refreshPresentation();
    CHECK(
        !controller.sketchEntityFor(
             stale_token)
             .has_value());
    const auto new_first_token =
        controller.sketchPresentationFor(
            *first.entity_id);
    CHECK(new_first_token.has_value());
    CHECK(*new_first_token != stale_token);

    controller.setSketchEditSketch(std::nullopt);
    CHECK(
        !controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));
    CHECK(
        !controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                create_edit_crosshair));
    CHECK(
        !controller.sketchPresentationFor(
             *first.entity_id)
             .has_value());

    return EXIT_SUCCESS;
}
