#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
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

    std::optional<viewer::ViewportPoint2>
    projectWorldPoint(
        viewer::Point3 point) const override {
        if (!viewer::finite(point)) {
            return std::nullopt;
        }
        return viewer::ViewportPoint2{
            point.x * 10.0,
            point.y * 10.0};
    }

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        if (!scene.valid()) return false;
        reference_scene_ = scene;
        return true;
    }

    bool setSolidScene(
        const viewer::SolidScene& scene) override {
        ++solid_scene_calls_;
        if (!scene.valid()) return false;
        solid_scene_ = scene;
        return true;
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        ++sketch_scene_calls_;
        if (fail_sketch_scene_) return false;
        if (!scene.valid()) return false;
        sketch_scene_ = scene;
        return true;
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        ++preview_scene_calls_;
        if (fail_preview_) return false;
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
            point.valid(),
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
    viewer::SolidScene solid_scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene preview_scene_;
    std::size_t solid_scene_calls_{};
    std::size_t sketch_scene_calls_{};
    std::size_t preview_scene_calls_{};
    bool fail_preview_{};
    bool fail_sketch_scene_{};
    viewer::SelectionIntentHandler selection_handler_;
    viewer::SpatialPointerHandler spatial_handler_;
    viewer::PrimaryPointerRouting routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
};

class FakeSolid final
    : public kernel::RuntimeSolid {};

class FakeSolidKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }

    kernel::SolidPresentationResult
    presentationMesh(
        kernel::RuntimeSolidHandle solid) noexcept override {
        if (!solid) {
            return {
                kernel::SolidPresentationStatus::
                    invalid_input,
                {}};
        }
        if (dynamic_cast<const FakeSolid*>(
                solid.get()) == nullptr) {
            return {
                kernel::SolidPresentationStatus::
                    provider_mismatch,
                {}};
        }

        kernel::SolidPresentationMesh mesh;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, 0.0},
                {10.0, 0.0, 0.0},
                {0.0, 10.0, 0.0},
                {0.0, 0.0, 1.0}});
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
    }
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

    std::vector<bool> presentation_transitions;
    controller.setPresentationStateChangedHandler(
        [&presentation_transitions](bool degraded) {
            presentation_transitions.push_back(degraded);
        });

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

    const auto projected =
        controller.projectSketchPointToViewport(
            sketch::Point2{2.0, 3.0});
    CHECK(projected.has_value());
    CHECK((
        *projected ==
        viewer::ViewportPoint2{20.0, 30.0}));

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

    // E2 Phase A: creation and transform previews stay on the transient
    // preview channel; they must not rebuild the authored Sketch scene.
    const auto sketch_scene_calls_before_previews =
        viewport.sketch_scene_calls_;

    CHECK(controller.setSketchCirclePreview(
        sketch::CircleIntent{
            sketch::Point2{3.0, 3.0},
            2.0}));
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_previews);

    CHECK(controller.setSketchArcPreview(
        sketch::ArcIntent{
            sketch::Point2{3.0, 3.0},
            2.0,
            0.0,
            1.0}));
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_previews);

    sketch::DirectManipulationGeometry geometry_preview;
    geometry_preview.lines.push_back(
        sketch::SketchLineState{
            *first.entity_id,
            sketch::Point2{1.0, 2.0},
            sketch::Point2{5.0, 2.0}});
    CHECK(controller.setSketchGeometryPreview(
        geometry_preview));
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_previews);

    const auto state_before_failed_preview =
        session.document().state();
    const auto revision_before_failed_preview =
        session.document().revision();
    const auto undo_before_failed_preview =
        session.undoDepth();
    const auto dirty_before_failed_preview =
        session.needsSave();
    const auto preview_calls_before_failure =
        viewport.preview_scene_calls_;

    viewport.fail_preview_ = true;
    CHECK(!controller.setSketchPreview(
        {
            ui::SketchPreviewLine2D{
                sketch::Point2{2.0, 3.0},
                sketch::Point2{8.0, 3.0}},
        }));
    viewport.fail_preview_ = false;

    CHECK(
        viewport.preview_scene_calls_ ==
        preview_calls_before_failure + 1U);
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_previews);
    CHECK(session.document().state() ==
          state_before_failed_preview);
    CHECK(session.document().revision() ==
          revision_before_failed_preview);
    CHECK(session.undoDepth() ==
          undo_before_failed_preview);
    CHECK(session.needsSave() ==
          dirty_before_failed_preview);

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

    const auto sketch_scene_calls_before_pointer_move =
        viewport.sketch_scene_calls_;
    viewport.emitSpatial(
        viewer::SpatialPointerEvent{
            viewer::SpatialPointerPhase::move,
            viewer::ViewportPoint2{100.0, 120.0},
            viewer::Ray3{
                viewer::Point3{3.0, 5.0, 10.0},
                viewer::Vec3{0.0, 0.0, -1.0}}});
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_pointer_move);
    CHECK(resolved.has_value());
    CHECK(resolved->sketch_id == sketch_id);
    CHECK(
        resolved->phase ==
        viewer::SpatialPointerPhase::move);
    CHECK((
        resolved->position ==
        sketch::Point2{3.0, 5.0}));

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

    CHECK(
        controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                presentation_selection));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            presentation_selection);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            select_pick_box);

    const auto erased =
        session.execute(
            application::EraseSketchEntityCommand{
                sketch_id,
                *second.entity_id});
    CHECK(erased.ok() && erased.changed);

    const auto state_after_commit =
        session.document().state();
    const auto revision_after_commit =
        session.document().revision();
    const auto undo_after_commit =
        session.undoDepth();
    const auto dirty_after_commit =
        session.needsSave();
    const auto sketch_scene_calls_before_commit_refresh =
        viewport.sketch_scene_calls_;

    viewport.fail_sketch_scene_ = true;
    controller.refreshPresentation();

    CHECK(controller.presentationDegraded());
    CHECK(
        presentation_transitions ==
        std::vector<bool>{true});
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_commit_refresh + 1U);
    CHECK(session.document().state() ==
          state_after_commit);
    CHECK(session.document().revision() ==
          revision_after_commit);
    CHECK(session.undoDepth() ==
          undo_after_commit);
    CHECK(session.needsSave() ==
          dirty_after_commit);

    // Recovery is always a full rebuild from the current authored model.
    viewport.fail_sketch_scene_ = false;
    controller.refreshPresentation();

    CHECK(!controller.presentationDegraded());
    CHECK(
        presentation_transitions ==
        (std::vector<bool>{true, false}));
    CHECK(
        viewport.sketch_scene_calls_ ==
        sketch_scene_calls_before_commit_refresh + 2U);
    CHECK(viewport.sketch_scene_.lines.size() == 1U);
    CHECK(session.document().state() ==
          state_after_commit);
    CHECK(session.document().revision() ==
          revision_after_commit);
    CHECK(session.undoDepth() ==
          undo_after_commit);
    CHECK(session.needsSave() ==
          dirty_after_commit);

    CHECK(session.undo().changed);
    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    CHECK(session.redo().changed);
    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.size() == 1U);

    controller.setSketchEditSketch(std::nullopt);
    CHECK(
        !controller.projectSketchPointToViewport(
             sketch::Point2{2.0, 3.0})
             .has_value());
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
    CHECK(
        controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));
    CHECK(
        controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                create_edit_crosshair));
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
    CHECK(
        !controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));
    CHECK(
        !controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                create_edit_crosshair));


    // PM-01D: final Body presentation is rebuilt from current authored state
    // through a provider-neutral mesh. Failed/Blocked current truth clears
    // the solid scene instead of retaining stale last-good geometry.
    {
        auto solid_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            solid_session{
                std::filesystem::path{
                    "pm01d-solid.ss2part"},
                std::move(solid_document)};
        FakeSolidKernel solid_kernel;

        const auto profile_sketch =
            solid_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            profile_sketch.ok() &&
            profile_sketch.sketch_id);

        const auto rectangle =
            solid_session.execute(
                application::AddSketchRectangleCommand{
                    *profile_sketch.sketch_id,
                    solid_session.document()
                        .revision(),
                    {0.0, 0.0},
                    {20.0, 10.0},
                    sketch::EntityRole::regular,
                    false});
        CHECK(rectangle.ok());

        const auto* source =
            solid_session.document()
                .findSketch(
                    *profile_sketch.sketch_id);
        CHECK(source != nullptr);
        const auto analysis =
            sketch::analyzeRegions(
                source->model);
        CHECK(analysis.complete());
        CHECK(analysis.regions.size() == 1U);
        const auto intent =
            part::makeProfileRegionIntent(
                analysis.regions.front());
        CHECK(intent);

        const auto profile =
            solid_session.execute(
                application::CreateProfileCommand{
                    *profile_sketch.sketch_id,
                    solid_session.document()
                        .revision(),
                    *intent});
        CHECK(
            profile.ok() &&
            profile.profile_id);

        const auto feature =
            solid_session.execute(
                application::
                    CreateExtrudeFeatureCommand{
                    *profile.profile_id,
                    solid_session.document()
                        .revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{5.0},
                        false},
                    {}},
                solid_kernel);
        CHECK(
            feature.ok() &&
            feature.feature_id);

        controller.setSolidModelingKernel(
            &solid_kernel);
        controller.setDocumentSession(
            &solid_session);
        CHECK(
            viewport.solid_scene_
                .triangles.size() == 1U);

        const auto delete_profile =
            solid_session.execute(
                application::DeleteProfileCommand{
                    *profile.profile_id,
                    solid_session.document()
                        .revision()});
        CHECK(delete_profile.ok());
        controller.refreshPresentation();
        CHECK(
            viewport.solid_scene_
                .triangles.empty());

        CHECK(solid_session.undo().changed);
        controller.refreshPresentation();
        CHECK(
            viewport.solid_scene_
                .triangles.size() == 1U);

        controller.setSolidModelingKernel(
            nullptr);
        CHECK(
            viewport.solid_scene_
                .triangles.empty());
    }

    return EXIT_SUCCESS;
}
