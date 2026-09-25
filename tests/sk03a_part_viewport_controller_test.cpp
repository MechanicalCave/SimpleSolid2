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

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-03A viewport controller CHECK failed at line "
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
        viewer::StandardView view) override {
        const auto next =
            viewer::cameraForStandardView(
                camera_,
                view);
        if (!next) return false;
        camera_ = *next;
        return true;
    }

    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                camera_,
                projection);
        if (!next) return false;
        camera_ = *next;
        return true;
    }

    void fitAll() override {}

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        if (!scene.valid()) return false;
        reference_scene_ = scene;
        return true;
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
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

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        return selection.valid();
    }

    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        return viewer::SketchPointQueryResult{
            point.finite(),
            std::nullopt};
    }

    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule) override {
        return viewer::SketchRectangleQueryResult{
            rectangle.valid(),
            {}};
    }

    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override {
        return overlay.valid();
    }

    void clearSketchSelectionBoxOverlay() override {}

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler_ =
            std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_handler_ =
            std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing) override {
        routing_ = routing;
    }

    void setCursorMode(
        viewer::ViewportCursorMode mode) override {
        cursor_mode_ = mode;
    }

    void emitSpatial(
        const viewer::SpatialPointerEvent& event) {
        CHECK(static_cast<bool>(spatial_handler_));
        spatial_handler_(event);
    }

    viewer::CameraState camera_;
    viewer::ReferenceScene reference_scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene preview_scene_;
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
            "sk03a-controller.ss2part"},
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
                sketch::Point2{1.0, 2.0},
                sketch::Point2{4.0, 2.0}});
    const auto second =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{4.0, 2.0},
                sketch::Point2{4.0, 6.0}});
    CHECK(first.ok() && first.entity_id.has_value());
    CHECK(second.ok() && second.entity_id.has_value());

    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{
        tree};
    TestViewport viewport;
    ui::PartViewportController controller{
        tree_controller,
        &viewport};

    controller.setDocumentSession(&session);
    CHECK(viewport.sketch_scene_.lines.empty());
    CHECK(!viewport.sketch_scene_.origin.has_value());
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            presentation_selection);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            system_default);

    controller.setSketchEditSketch(sketch_id);
    CHECK(viewport.sketch_scene_.lines.size() == 2U);
    CHECK(viewport.sketch_scene_.origin.has_value());
    CHECK((
        viewport.sketch_scene_.origin->position ==
        viewer::Point3{0.0, 0.0, 0.0}));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            presentation_selection);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            select_pick_box);

    const auto first_token =
        viewport.sketch_scene_.lines[0].token;
    const auto second_token =
        viewport.sketch_scene_.lines[1].token;
    CHECK(first_token != second_token);

    const auto first_address =
        controller.sketchEntityFor(first_token);
    const auto second_address =
        controller.sketchEntityFor(second_token);
    CHECK(first_address.has_value());
    CHECK(second_address.has_value());
    CHECK(first_address->sketch_id == sketch_id);
    CHECK(second_address->sketch_id == sketch_id);
    CHECK(first_address->entity_id == *first.entity_id);
    CHECK(second_address->entity_id == *second.entity_id);

    CHECK((
        viewport.sketch_scene_.lines[0].start ==
        viewer::Point3{1.0, 2.0, 0.0}));
    CHECK((
        viewport.sketch_scene_.lines[0].end ==
        viewer::Point3{4.0, 2.0, 0.0}));

    const auto revision_before_preview =
        session.document().revision();
    const auto undo_before_preview =
        session.undoDepth();
    const bool dirty_before_preview =
        session.needsSave();

    CHECK(
        controller.setSketchPreview(
            {
                ui::SketchPreviewLine2D{
                    sketch::Point2{2.0, 3.0},
                    sketch::Point2{7.0, 3.0}},
            }));
    CHECK(viewport.preview_scene_.lines.size() == 1U);
    CHECK((
        viewport.preview_scene_.lines[0].start ==
        viewer::Point3{2.0, 3.0, 0.0}));
    CHECK((
        viewport.preview_scene_.lines[0].end ==
        viewer::Point3{7.0, 3.0, 0.0}));

    controller.clearSketchPreview();
    CHECK(viewport.preview_scene_.lines.empty());
    CHECK(
        session.document().revision() ==
        revision_before_preview);
    CHECK(session.undoDepth() == undo_before_preview);
    CHECK(session.needsSave() == dirty_before_preview);

    std::optional<ui::SketchPointerInput>
        resolved;
    controller.setSketchPointerHandler(
        [&resolved](
            const ui::SketchPointerInput& input) {
            resolved = input;
        });

    viewport.emitSpatial(
        viewer::SpatialPointerEvent{
            viewer::SpatialPointerPhase::move,
            viewer::ViewportPoint2{100.0, 120.0},
            viewer::Ray3{
                viewer::Point3{3.0, 5.0, 10.0},
                viewer::Vec3{0.0, 0.0, -1.0}}});
    CHECK(resolved.has_value());
    CHECK(resolved->sketch_id == sketch_id);
    CHECK(
        resolved->phase ==
        viewer::SpatialPointerPhase::move);
    CHECK((
        resolved->position ==
        sketch::Point2{3.0, 5.0}));

    CHECK(controller.setSketchSpatialToolInput(true));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    CHECK(controller.setSketchSpatialToolInput(false));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            presentation_selection);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            select_pick_box);

    CHECK(
        session.execute(
            application::EraseSketchEntityCommand{
                sketch_id,
                *second.entity_id})
            .ok());
    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.size() == 1U);

    CHECK(session.undo().changed);
    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    CHECK(session.redo().changed);
    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.size() == 1U);

    controller.setSketchEditSketch(std::nullopt);
    CHECK(viewport.sketch_scene_.lines.empty());
    CHECK(!viewport.sketch_scene_.origin.has_value());
    CHECK(viewport.preview_scene_.lines.empty());
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            system_default);
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            presentation_selection);

    // A hosted Sketch can disappear through history while the controller
    // still carries its runtime edit ID. refreshPresentation() must fail
    // closed: clear authored/preview presentation and restore normal routing.
    const auto transient =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xz_plane});
    CHECK(transient.ok());
    CHECK(transient.sketch_id.has_value());

    controller.setSketchEditSketch(
        *transient.sketch_id);
    CHECK(viewport.sketch_scene_.origin.has_value());
    CHECK(
        controller.setSketchPreview(
            {
                ui::SketchPreviewLine2D{
                    sketch::Point2{1.0, 1.0},
                    sketch::Point2{2.0, 1.0}},
            }));
    CHECK(controller.setSketchSpatialToolInput(true));
    CHECK(!viewport.preview_scene_.lines.empty());

    CHECK(session.undo().changed);
    CHECK(
        session.document().findSketch(
            *transient.sketch_id) == nullptr);

    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.empty());
    CHECK(!viewport.sketch_scene_.origin.has_value());
    CHECK(viewport.preview_scene_.lines.empty());
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            system_default);
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            presentation_selection);
    CHECK(!controller.setSketchSpatialToolInput(true));

    return EXIT_SUCCESS;
}
