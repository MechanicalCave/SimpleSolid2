#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
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

    bool setBodyScene(
        const viewer::BodyScene& scene) override {
        ++body_scene_calls_;
        if (!scene.valid()) return false;
        body_scene_ = scene;
        return true;
    }

    viewer::ViewStyle
    viewStyle() const noexcept override {
        return view_style_;
    }

    bool setViewStyle(
        viewer::ViewStyle style) override {
        view_style_ = style;
        return true;
    }

    void setViewStyleActionHandler(
        viewer::ViewStyleActionHandler handler) override {
        view_style_handler_ =
            std::move(handler);
    }

    viewer::BodyTopologyPickQueryResult
    queryBodyTopology(
        viewer::ViewportPoint2,
        viewer::BodyTopologyPickFilter = {}) override {
        return body_query_;
    }

    void setBodyTopologySelectionIntentHandler(
        viewer::BodyTopologySelectionIntentHandler handler) override {
        body_topology_handler_ =
            std::move(handler);
    }

    void setBodyTopologyPreselectionIntentHandler(
        viewer::BodyTopologyPreselectionIntentHandler handler) override {
        body_topology_preselection_handler_ =
            std::move(handler);
    }

    void setBodyTopologyCycleIntentHandler(
        viewer::BodyTopologyCycleIntentHandler handler) override {
        body_topology_cycle_handler_ =
            std::move(handler);
    }

    bool setBodyTopologyPreselection(
        std::optional<viewer::PresentationToken> token) override {
        if (token && !token->valid()) {
            return false;
        }
        body_preselection_ = token;
        return true;
    }

    bool setBodyTopologyOverlayScene(
        const viewer::BodyTopologyOverlayScene& scene) override {
        ++body_topology_overlay_scene_calls_;
        if (!scene.valid()) {
            return false;
        }
        body_topology_overlay_scene_ = scene;
        return true;
    }

    bool setSolidScene(
        const viewer::SolidScene& scene) override {
        ++solid_scene_calls_;
        if (!scene.valid()) return false;
        solid_scene_ = scene;
        return true;
    }

    bool setSolidPreviewScene(
        const viewer::SolidPreviewScene& scene) override {
        ++solid_preview_scene_calls_;
        if (!scene.valid()) return false;
        solid_preview_scene_ = scene;
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
        if (!selection.valid()) return false;
        presentation_selection_ = selection;
        return true;
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

    void emitBodyTopology(
        const viewer::BodyTopologyPickQueryResult& query,
        viewer::SelectionIntentMode mode) {
        CHECK(static_cast<bool>(
            body_topology_handler_));
        body_topology_handler_(
            query,
            mode);
    }

    void emitBodyTopologyPreselection(
        const viewer::BodyTopologyPickQueryResult& query,
        viewer::ViewportPoint2 point) {
        CHECK(static_cast<bool>(
            body_topology_preselection_handler_));
        body_topology_preselection_handler_(
            query,
            point);
    }

    void emitBodyTopologyCycle(bool reverse) {
        CHECK(static_cast<bool>(
            body_topology_cycle_handler_));
        body_topology_cycle_handler_(reverse);
    }

    viewer::CameraState camera_;
    viewer::ReferenceScene reference_scene_;
    viewer::BodyScene body_scene_;
    viewer::SolidScene solid_scene_;
    viewer::SolidPreviewScene
        solid_preview_scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene preview_scene_;
    viewer::PresentationSelection
        presentation_selection_;
    viewer::BodyTopologyPickQueryResult
        body_query_;
    viewer::BodyTopologyOverlayScene
        body_topology_overlay_scene_;
    std::size_t body_topology_overlay_scene_calls_{};
    viewer::ViewStyle view_style_{
        viewer::ViewStyle::shaded};
    std::size_t body_scene_calls_{};
    std::size_t solid_scene_calls_{};
    std::size_t solid_preview_scene_calls_{};
    std::size_t sketch_scene_calls_{};
    std::size_t preview_scene_calls_{};
    bool fail_preview_{};
    bool fail_sketch_scene_{};
    viewer::SelectionIntentHandler selection_handler_;
    viewer::BodyTopologySelectionIntentHandler
        body_topology_handler_;
    viewer::BodyTopologyPreselectionIntentHandler
        body_topology_preselection_handler_;
    viewer::BodyTopologyCycleIntentHandler
        body_topology_cycle_handler_;
    std::optional<viewer::PresentationToken>
        body_preselection_;
    viewer::ViewStyleActionHandler
        view_style_handler_;
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
    bool fail_mesh{};
    std::size_t extrude_calls{};
    std::size_t mesh_calls{};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        ++extrude_calls;
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
        ++mesh_calls;
        if (fail_mesh) {
            return {
                kernel::SolidPresentationStatus::
                    provider_failure,
                {}};
        }
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
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0}});
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
    }
};

class StageAwareSketchSolid final
    : public kernel::RuntimeSolid {
public:
    struct Surface final {
        kernel::RuntimeFaceToken face;
        kernel::RuntimeSurfaceToken surface;
        kernel::Frame3 frame;
    };

    std::vector<Surface> surfaces;
    std::uint64_t next_face{1001U};
    std::uint64_t next_surface{4001U};
};

kernel::Frame3 offsetStageFrame(
    const kernel::Frame3& source,
    double offset) {
    auto frame = source;
    frame.origin.x += source.normal.x * offset;
    frame.origin.y += source.normal.y * offset;
    frame.origin.z += source.normal.z * offset;
    return frame;
}

class StageAwareSketchKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }

        auto runtime =
            std::make_shared<StageAwareSketchSolid>();
        if (upstream) {
            const auto* prior =
                dynamic_cast<const StageAwareSketchSolid*>(
                    upstream.get());
            if (prior == nullptr) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }
            runtime->surfaces = prior->surfaces;
            runtime->next_face = prior->next_face;
            runtime->next_surface = prior->next_surface;

            for (const auto& inherited :
                 prior->surfaces) {
                result.current_faces.push_back(
                    inherited.face);
                result.inherited_faces.push_back(
                    {
                        inherited.face,
                        kernel::ReferenceStatus::resolved,
                        1U,
                    });
                result.inherited_surfaces.push_back(
                    {
                        inherited.surface,
                        kernel::ReferenceStatus::resolved,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        kernel::SurfaceKind::plane,
                        inherited.frame,
                        {inherited.face},
                    });
            }
        }

        const auto publish =
            [&result, &runtime](
                const kernel::ExtrudeFaceRole& role,
                const kernel::Frame3& frame) {
                const kernel::RuntimeFaceToken face{
                    runtime->next_face++};
                const kernel::RuntimeSurfaceToken surface{
                    runtime->next_surface++};

                runtime->surfaces.push_back(
                    {
                        face,
                        surface,
                        frame,
                    });
                result.current_faces.push_back(face);
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::resolved,
                        1U,
                        face,
                    });

                kernel::NewSurfaceLineage lineage;
                lineage.role = role;
                lineage.surface_status =
                    kernel::ReferenceStatus::resolved;
                lineage.strict_face_status =
                    kernel::ReferenceStatus::resolved;
                lineage.candidate_face_count = 1U;
                lineage.surface_kind =
                    kernel::SurfaceKind::plane;
                lineage.canonical_frame = frame;
                lineage.resolved_token = surface;
                lineage.current_faces = {face};
                result.new_surfaces.push_back(
                    std::move(lineage));
            };

        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.start_cap_role,
                std::nullopt,
            },
            offsetStageFrame(
                input.profile.frame,
                input.start_offset_mm));
        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.end_cap_role,
                std::nullopt,
            },
            offsetStageFrame(
                input.profile.frame,
                input.end_offset_mm));

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid = std::move(runtime);
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count = 0U;
        result.vertex_count = 0U;
        return result;
    }

    kernel::BodyPresentationResult bodyPresentation(
        kernel::RuntimeSolidHandle solid) noexcept override {
        kernel::BodyPresentationResult result;
        const auto* runtime =
            dynamic_cast<const StageAwareSketchSolid*>(
                solid.get());
        if (runtime == nullptr) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_mismatch;
            return result;
        }

        result.status =
            kernel::SolidPresentationStatus::ok;
        for (std::size_t index = 0U;
             index < runtime->surfaces.size();
             ++index) {
            const auto& surface =
                runtime->surfaces[index];
            const auto& origin = surface.frame.origin;
            const auto& u = surface.frame.u_axis;
            const auto& v = surface.frame.v_axis;
            result.body.mesh.triangles.push_back(
                {
                    origin,
                    {
                        origin.x + u.x,
                        origin.y + u.y,
                        origin.z + u.z,
                    },
                    {
                        origin.x + v.x,
                        origin.y + v.y,
                        origin.z + v.z,
                    },
                    surface.frame.normal,
                    surface.frame.normal,
                    surface.frame.normal,
                });
            result.body.faces.push_back(
                {
                    surface.face,
                    index,
                    1U,
                });
        }
        return result;
    }
};

class TopologyFakeSolidKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid() || upstream) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }

        constexpr kernel::RuntimeFaceToken
            face_token{101U};
        constexpr kernel::RuntimeEdgeToken
            edge_token{201U};
        constexpr kernel::RuntimeVertexToken
            vertex_token{301U};
        constexpr kernel::RuntimeSurfaceToken
            surface_token{401U};

        kernel::ExtrudeFaceRole role;
        role.kind =
            kernel::ExtrudeGeneratedFaceRoleKind::cap;
        role.cap_role =
            kernel::ExtrudeCapRole::profile_cap;

        kernel::NewFaceLineage face;
        face.role = role;
        face.status =
            kernel::ReferenceStatus::resolved;
        face.candidate_count = 1U;
        face.resolved_token = face_token;

        kernel::NewSurfaceLineage surface;
        surface.role = role;
        surface.surface_status =
            kernel::ReferenceStatus::resolved;
        surface.strict_face_status =
            kernel::ReferenceStatus::resolved;
        surface.candidate_face_count = 1U;
        surface.surface_kind =
            kernel::SurfaceKind::plane;
        surface.canonical_frame =
            input.profile.frame;
        surface.resolved_token =
            surface_token;
        surface.current_faces = {
            face_token};

        kernel::CurrentEdgeSemanticObservation
            edge_observation;
        edge_observation.runtime_token =
            edge_token;
        edge_observation.provider_curve_kind =
            kernel::CurveKind::line;

        kernel::CurrentVertexSemanticObservation
            vertex_observation;
        vertex_observation.runtime_token =
            vertex_token;
        vertex_observation.provider_point =
            kernel::Point3{0.0, 0.0, 0.0};

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count = 1U;
        result.edge_count = 1U;
        result.vertex_count = 1U;
        result.current_faces = {face_token};
        result.current_edges = {edge_token};
        result.current_vertices = {
            vertex_token};
        result.new_faces = {
            std::move(face)};
        result.new_surfaces = {
            std::move(surface)};
        result.current_edge_semantics = {
            std::move(edge_observation)};
        result.current_vertex_semantics = {
            std::move(vertex_observation)};
        return result;
    }

    kernel::BodyPresentationResult
    bodyPresentation(
        kernel::RuntimeSolidHandle solid) noexcept override {
        kernel::BodyPresentationResult result;
        if (!solid ||
            dynamic_cast<const FakeSolid*>(
                solid.get()) == nullptr) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_mismatch;
            return result;
        }

        result.status =
            kernel::SolidPresentationStatus::ok;
        result.body.mesh.triangles.push_back(
            {
                {-10.0, -10.0, 0.0},
                {10.0, -10.0, 0.0},
                {0.0, 10.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0}});
        result.body.faces.push_back(
            {
                kernel::RuntimeFaceToken{101U},
                0U,
                1U});
        result.body.edges.push_back(
            {
                kernel::RuntimeEdgeToken{201U},
                {
                    {-10.0, 0.0, 0.0},
                    {10.0, 0.0, 0.0},
                }});
        result.body.vertices.push_back(
            {
                kernel::RuntimeVertexToken{301U},
                {0.0, 0.0, 0.0}});
        return result;
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
            viewport.body_scene_
                .triangles.size() == 1U);
        CHECK(viewport.body_scene_.generation.valid());
        CHECK(
            viewport.body_scene_.purpose ==
            viewer::BodyScenePurpose::current_body);
        CHECK(viewport.solid_scene_calls_ == 0U);
        CHECK(viewport.body_scene_calls_ > 0U);
        const auto first_body_generation =
            viewport.body_scene_.generation;

        const auto extrude_calls_after_publish =
            solid_kernel.extrude_calls;
        const auto mesh_calls_after_publish =
            solid_kernel.mesh_calls;
        controller.refreshPresentation();
        CHECK(
            solid_kernel.extrude_calls ==
            extrude_calls_after_publish);
        CHECK(
            solid_kernel.mesh_calls ==
            mesh_calls_after_publish);
        CHECK(
            viewport.body_scene_.generation ==
            first_body_generation);

        auto preview_solid =
            std::make_shared<FakeSolid>();
        CHECK(
            controller.setSolidPreview(
                preview_solid,
                viewer::SolidPreviewTone::
                    additive));
        CHECK(
            viewport.solid_preview_scene_
                .triangles.size() == 1U);
        controller.clearSolidPreview();
        CHECK(
            viewport.solid_preview_scene_
                .triangles.empty());

        auto properties =
            solid_session.document()
                .properties();
        properties.title =
            "PM-01D cache invalidation";
        const auto property_change =
            solid_session.execute(
                application::
                    SetDocumentPropertiesCommand{
                        std::move(properties)});
        CHECK(
            property_change.ok() &&
            property_change.changed);

        solid_kernel.fail_mesh = true;
        controller.refreshPresentation();
        CHECK(
            viewport.body_scene_
                .triangles.empty());
        CHECK(
            controller.presentationDegraded());

        solid_kernel.fail_mesh = false;
        controller.refreshPresentation();
        CHECK(
            viewport.body_scene_
                .triangles.size() == 1U);
        CHECK(viewport.body_scene_.generation.valid());
        CHECK(
            viewport.body_scene_.generation !=
            first_body_generation);
        CHECK(
            viewport.body_scene_.purpose ==
            viewer::BodyScenePurpose::current_body);
        CHECK(
            !controller.presentationDegraded());

        const auto delete_profile =
            solid_session.execute(
                application::DeleteProfileCommand{
                    *profile.profile_id,
                    solid_session.document()
                        .revision()});
        CHECK(delete_profile.ok());
        controller.refreshPresentation();
        CHECK(
            viewport.body_scene_
                .triangles.empty());
        CHECK(
            !controller.presentationDegraded());

        CHECK(solid_session.undo().changed);
        controller.refreshPresentation();
        CHECK(
            viewport.body_scene_
                .triangles.size() == 1U);

        controller.setSolidModelingKernel(
            nullptr);
        CHECK(
            viewport.body_scene_
                .triangles.empty());
        controller.clear();
    }

    // PM-02J R3: editing a face-supported Sketch after a downstream
    // Extrude must resolve presentation against the Sketch support's exact
    // earlier Body stage, not only the final Body topology catalog.
    {
        auto stage_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession stage_session{
            std::filesystem::path{
                "pm02jr3-stage-sketch-edit.ss2part"},
            std::move(stage_document)};
        StageAwareSketchKernel stage_kernel;

        const auto base_sketch =
            stage_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            base_sketch.ok() &&
            base_sketch.sketch_id);

        CHECK(
            stage_session.execute(
                application::AddSketchRectangleCommand{
                    *base_sketch.sketch_id,
                    stage_session.document().revision(),
                    {0.0, 0.0},
                    {20.0, 10.0},
                    sketch::EntityRole::regular,
                    false})
                .ok());

        const auto* base_hosted =
            stage_session.document().findSketch(
                *base_sketch.sketch_id);
        CHECK(base_hosted != nullptr);
        const auto base_regions =
            sketch::analyzeRegions(
                base_hosted->model);
        CHECK(
            base_regions.complete() &&
            base_regions.regions.size() == 1U);
        const auto base_intent =
            part::makeProfileRegionIntent(
                base_regions.regions.front());
        CHECK(base_intent);

        const auto base_profile =
            stage_session.execute(
                application::CreateProfileCommand{
                    *base_sketch.sketch_id,
                    stage_session.document().revision(),
                    *base_intent});
        CHECK(
            base_profile.ok() &&
            base_profile.profile_id);

        const auto base_feature =
            stage_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *base_profile.profile_id,
                    stage_session.document().revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{10.0},
                        false},
                    "Base"},
                stage_kernel);
        CHECK(
            base_feature.ok() &&
            base_feature.feature_id);

        const auto base_eval =
            part::evaluatePart(
                stage_session.document(),
                stage_kernel);
        CHECK(
            base_eval.body_status ==
            part::BodyEvaluationStatus::
                up_to_date);
        CHECK(base_eval.current_topology);

        const auto cap =
            std::find_if(
                base_eval.current_topology
                    ->surfaces.begin(),
                base_eval.current_topology
                    ->surfaces.end(),
                [feature_id =
                     *base_feature.feature_id](
                    const auto& surface) {
                    return surface.status ==
                               kernel::ReferenceStatus::
                                   resolved &&
                           surface.address
                                   .producer_feature_id ==
                               feature_id &&
                           surface.address.role ==
                               part::FeatureSurfaceRoleKind::
                                   extent_cap;
                });
        CHECK(
            cap !=
            base_eval.current_topology
                ->surfaces.end());
        const auto support =
            part::partSketchSupportForBodyPlanarSurface(
                part::SurfaceReference{
                    base_eval.current_topology->stage,
                    cap->address});
        CHECK(support);

        const auto face_sketch =
            stage_session.execute(
                application::
                    CreatePartSketchOnSupportCommand{
                    *support,
                    stage_session.document().revision()},
                &stage_kernel);
        CHECK(
            face_sketch.ok() &&
            face_sketch.sketch_id);

        CHECK(
            stage_session.execute(
                application::AddSketchRectangleCommand{
                    *face_sketch.sketch_id,
                    stage_session.document().revision(),
                    {2.0, 3.0},
                    {8.0, 9.0},
                    sketch::EntityRole::regular,
                    false})
                .ok());

        const auto* face_hosted =
            stage_session.document().findSketch(
                *face_sketch.sketch_id);
        CHECK(face_hosted != nullptr);
        const auto face_regions =
            sketch::analyzeRegions(
                face_hosted->model);
        CHECK(
            face_regions.complete() &&
            face_regions.regions.size() == 1U);
        const auto face_intent =
            part::makeProfileRegionIntent(
                face_regions.regions.front());
        CHECK(face_intent);

        const auto face_profile =
            stage_session.execute(
                application::CreateProfileCommand{
                    *face_sketch.sketch_id,
                    stage_session.document().revision(),
                    *face_intent});
        CHECK(
            face_profile.ok() &&
            face_profile.profile_id);

        const auto downstream =
            stage_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *face_profile.profile_id,
                    stage_session.document().revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{4.0},
                        false},
                    "Downstream"},
                stage_kernel);
        CHECK(
            downstream.ok() &&
            downstream.feature_id);

        QTreeWidget stage_tree;
        ui::PartDocumentTreeController
            stage_tree_controller{
                stage_tree};
        TestViewport stage_viewport;
        ui::PartViewportController
            stage_controller{
                stage_tree_controller,
                &stage_viewport};
        stage_controller.setSolidModelingKernel(
            &stage_kernel);
        stage_controller.setDocumentSession(
            &stage_session);

        CHECK(
            stage_viewport.body_scene_.faces.size() ==
            4U);

        stage_controller.setSketchEditSketch(
            *face_sketch.sketch_id);

        CHECK(
            stage_viewport.sketch_scene_.lines.size() ==
            4U);
        CHECK(
            stage_viewport.reference_scene_.grid
                .has_value());
        CHECK(
            stage_viewport.reference_scene_.grid
                ->origin.z == 10.0);

        stage_controller.setSketchEditSketch(
            std::nullopt);
        stage_controller.setSolidModelingKernel(
            nullptr);
        stage_controller.clear();
    }

    // PM-02D2: Controller owns Body candidate policy and runtime selection.
    // Provider order is deliberately Face, Edge, Vertex; policy must choose
    // Vertex first and stale scene generations must be ignored.
    {
        auto topology_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            topology_session{
                std::filesystem::path{
                    "pm02d2-body-selection.ss2part"},
                std::move(topology_document)};
        TopologyFakeSolidKernel
            topology_kernel;

        const auto created_sketch =
            topology_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            created_sketch.ok() &&
            created_sketch.sketch_id);

        const auto rectangle =
            topology_session.execute(
                application::AddSketchRectangleCommand{
                    *created_sketch.sketch_id,
                    topology_session.document()
                        .revision(),
                    {-5.0, -5.0},
                    {5.0, 5.0},
                    sketch::EntityRole::regular,
                    false});
        CHECK(rectangle.ok());

        const auto* hosted =
            topology_session.document()
                .findSketch(
                    *created_sketch.sketch_id);
        CHECK(hosted != nullptr);
        const auto regions =
            sketch::analyzeRegions(
                hosted->model);
        CHECK(regions.complete());
        CHECK(regions.regions.size() == 1U);
        const auto intent =
            part::makeProfileRegionIntent(
                regions.regions.front());
        CHECK(intent);

        const auto profile =
            topology_session.execute(
                application::CreateProfileCommand{
                    *created_sketch.sketch_id,
                    topology_session.document()
                        .revision(),
                    *intent});
        CHECK(
            profile.ok() &&
            profile.profile_id);

        const auto feature =
            topology_session.execute(
                application::
                    CreateExtrudeFeatureCommand{
                    *profile.profile_id,
                    topology_session.document()
                        .revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{5.0},
                        false},
                    {}},
                topology_kernel);
        CHECK(
            feature.ok() &&
            feature.feature_id);

        QTreeWidget topology_tree;
        ui::PartDocumentTreeController
            topology_tree_controller{
                topology_tree};
        TestViewport topology_viewport;
        ui::PartViewportController
            topology_controller{
                topology_tree_controller,
                &topology_viewport};

        topology_controller.setSolidModelingKernel(
            &topology_kernel);
        std::optional<part::FeatureId>
            hovered_tree_feature;
        std::size_t feature_hover_events = 0U;
        topology_tree_controller.setFeatureHoverHandler(
            [&hovered_tree_feature,
             &feature_hover_events](
                std::optional<part::FeatureId> id) {
                hovered_tree_feature = id;
                ++feature_hover_events;
            });

        topology_controller.setDocumentSession(
            &topology_session);

        std::size_t topology_inspection_events = 0U;
        std::optional<ui::BodyTopologyInspection>
            last_topology_inspection;
        topology_controller
            .setBodyTopologySelectionChangedHandler(
                [&topology_inspection_events,
                 &last_topology_inspection](
                    std::optional<
                        ui::BodyTopologyInspection>
                        inspection) {
                    ++topology_inspection_events;
                    last_topology_inspection =
                        std::move(inspection);
                });

        const auto body_summary =
            topology_controller.bodyTopologySummary();
        CHECK(body_summary.has_value());
        CHECK(body_summary->faces.total == 1U);
        CHECK(body_summary->faces.referenceable == 1U);
        CHECK(body_summary->edges.total == 1U);
        CHECK(
            body_summary->edges
                .semantically_unsupported == 1U);
        CHECK(body_summary->vertices.total == 1U);
        CHECK(
            body_summary->vertices
                .semantically_unsupported == 1U);

        const auto feature_summary =
            topology_controller.featureContributionSummary(
                *feature.feature_id);
        CHECK(feature_summary.has_value());
        CHECK(feature_summary->faces == 1U);
        CHECK(
            feature_summary->current_stage.feature_id ==
            std::optional<part::FeatureId>{
                *feature.feature_id});

        // PM-02D4: tree hover emits semantic FeatureId only and does not
        // mutate tree selection. Leave clears the transient hover.
        topology_tree.resize(420, 320);
        topology_tree.expandAll();
        topology_tree.show();
        QApplication::processEvents();

        QTreeWidgetItem* feature_item = nullptr;
        for (QTreeWidgetItemIterator it{&topology_tree};
             *it != nullptr;
             ++it) {
            if ((*it)->text(0).contains(
                    QStringLiteral("Extrude"))) {
                feature_item = *it;
                break;
            }
        }
        CHECK(feature_item != nullptr);
        const auto selected_before_hover =
            topology_tree_controller.selectedFeatureIds();
        const auto feature_rect =
            topology_tree.visualItemRect(
                feature_item);
        CHECK(feature_rect.isValid());

        QMouseEvent hover_event{
            QEvent::MouseMove,
            QPointF{feature_rect.center()},
            Qt::NoButton,
            Qt::NoButton,
            Qt::NoModifier};
        CHECK(QApplication::sendEvent(
            topology_tree.viewport(),
            &hover_event));
        CHECK(feature_hover_events >= 1U);
        CHECK(
            hovered_tree_feature ==
            std::optional<part::FeatureId>{
                *feature.feature_id});
        CHECK(
            topology_tree_controller.selectedFeatureIds() ==
            selected_before_hover);

        QEvent leave_event{QEvent::Leave};
        CHECK(QApplication::sendEvent(
            topology_tree.viewport(),
            &leave_event));
        CHECK(!hovered_tree_feature.has_value());
        CHECK(
            topology_tree_controller.selectedFeatureIds() ==
            selected_before_hover);

        CHECK(
            topology_viewport.body_scene_
                .faces.size() == 1U);
        CHECK(
            topology_viewport.body_scene_
                .edges.size() == 1U);
        CHECK(
            topology_viewport.body_scene_
                .vertices.size() == 1U);
        const auto generation =
            topology_viewport.body_scene_
                .generation;
        CHECK(generation.valid());

        const auto face_token =
            topology_viewport.body_scene_
                .faces.front().token;
        const auto edge_token =
            topology_viewport.body_scene_
                .edges.front().token;
        const auto vertex_token =
            topology_viewport.body_scene_
                .vertices.front().token;

        // PM-02D4: selected Feature contribution is a presentation overlay
        // over the same current BodyScene token. It must not create a second
        // topology identity or alter ordinary Body selection.
        topology_controller.setFeatureContributionSelection(
            *feature.feature_id);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .generation == generation);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.size() == 1U);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.front().role ==
            viewer::BodyTopologyOverlayRole::
                feature_contribution_selected);
        CHECK(
            std::find(
                topology_viewport
                    .body_topology_overlay_scene_
                    .groups.front().tokens.begin(),
                topology_viewport
                    .body_topology_overlay_scene_
                    .groups.front().tokens.end(),
                face_token) !=
            topology_viewport
                .body_topology_overlay_scene_
                .groups.front().tokens.end());

        // Hovering the already-selected Feature must not duplicate the same
        // green contribution as a second overlay group.
        topology_controller.setFeatureContributionHover(
            *feature.feature_id);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.size() == 1U);

        // With persistent selection cleared, the same semantic contribution
        // becomes a transient hover role without touching authored state.
        topology_controller.setFeatureContributionSelection(
            std::nullopt);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.size() == 1U);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.front().role ==
            viewer::BodyTopologyOverlayRole::
                feature_contribution_hover);
        topology_controller.setFeatureContributionHover(
            std::nullopt);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .empty());

        topology_controller.setFeatureContributionSelection(
            *feature.feature_id);
        const auto contribution_generation =
            topology_viewport.body_topology_overlay_scene_
                .generation;

        viewer::BodyTopologyPickQueryResult
            competing;
        competing.completed = true;
        competing.generation = generation;
        competing.candidates = {
            {
                face_token,
                viewer::BodyTopologyPresentationKind::
                    face,
                0.0,
                1.0},
            {
                edge_token,
                viewer::BodyTopologyPresentationKind::
                    edge,
                0.5,
                1.0},
            {
                vertex_token,
                viewer::BodyTopologyPresentationKind::
                    vertex,
                5.0,
                1.0},
        };
        CHECK(competing.valid());

        // PM-02D3 hover/candidate stack is controller-owned. Provider order
        // deliberately starts with Face; ordinary priority must still make
        // Vertex the visible first preselection.
        topology_viewport.emitBodyTopologyPreselection(
            competing,
            {100.0, 100.0});
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                vertex_token});

        topology_viewport.emitBodyTopologyCycle(false);
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                edge_token});

        // Moving inside the same 4 px hit neighborhood with the same
        // candidate stack preserves the current cycle index.
        topology_viewport.emitBodyTopologyPreselection(
            competing,
            {102.0, 101.0});
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                edge_token});

        topology_viewport.emitBodyTopologyCycle(false);
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                face_token});

        // Click commits the visible cycled preselection, not the first
        // provider candidate nor an independently re-ranked candidate.
        topology_viewport.emitBodyTopology(
            competing,
            viewer::SelectionIntentMode::replace);
        CHECK(
            topology_controller
                .primaryBodyTopologySelection()
                ->kind ==
            viewer::BodyTopologyPresentationKind::
                face);
        CHECK(topology_inspection_events == 1U);
        CHECK(last_topology_inspection.has_value());
        CHECK(
            last_topology_inspection->kind ==
            viewer::BodyTopologyPresentationKind::
                face);
        CHECK(
            last_topology_inspection
                ->accounting_class ==
            part::TopologyAccountingClass::
                referenceable);
        CHECK(
            last_topology_inspection
                ->strict_referenceability ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            last_topology_inspection
                ->carrier_referenceability ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            last_topology_inspection->surface_kind ==
            std::optional<kernel::SurfaceKind>{
                kernel::SurfaceKind::plane});
        CHECK(
            last_topology_inspection
                ->producer_feature_id ==
            std::optional<part::FeatureId>{
                *feature.feature_id});
        CHECK(
            last_topology_inspection
                ->sketch_support ==
            ui::SketchSupportInspectionCapability::
                supported);

        // Leaving the hit neighborhood resets the runtime stack to the
        // ordinary Vertex -> Edge -> Face ranking.
        topology_viewport.emitBodyTopologyPreselection(
            competing,
            {110.0, 110.0});
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                vertex_token});

        topology_viewport.emitBodyTopologyCycle(true);
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                face_token});
        topology_viewport.emitBodyTopologyCycle(false);
        CHECK(
            topology_viewport.body_preselection_ ==
            std::optional<viewer::PresentationToken>{
                vertex_token});

        // Hover/cycling alone never changes the primary Properties subject.
        CHECK(topology_inspection_events == 1U);

        topology_viewport.emitBodyTopology(
            competing,
            viewer::SelectionIntentMode::replace);

        CHECK(topology_inspection_events == 2U);
        CHECK(last_topology_inspection.has_value());
        CHECK(
            last_topology_inspection->kind ==
            viewer::BodyTopologyPresentationKind::
                vertex);
        CHECK(
            last_topology_inspection
                ->strict_referenceability ==
            kernel::ReferenceStatus::unsupported);
        CHECK(
            last_topology_inspection
                ->carrier_referenceability ==
            kernel::ReferenceStatus::unsupported);
        const std::optional<kernel::Point3>
            expected_provider_point{
                kernel::Point3{0.0, 0.0, 0.0}};
        CHECK(
            last_topology_inspection
                ->provider_point ==
            expected_provider_point);
        CHECK(
            last_topology_inspection
                ->sketch_support ==
            ui::SketchSupportInspectionCapability::
                not_applicable);

        const auto primary =
            topology_controller
                .primaryBodyTopologySelection();
        CHECK(primary.has_value());
        CHECK(
            primary->kind ==
            viewer::BodyTopologyPresentationKind::
                vertex);
        CHECK(primary->runtime_token_value == 301U);
        CHECK(
            topology_viewport
                .presentation_selection_
                .primary ==
            std::optional<
                viewer::PresentationToken>{
                vertex_token});

        viewer::BodyTopologyPickQueryResult stale =
            competing;
        stale.generation = {
            generation.value + 1U};
        stale.candidates = {
            {
                face_token,
                viewer::BodyTopologyPresentationKind::
                    face,
                0.0,
                0.5},
        };
        topology_viewport.emitBodyTopology(
            stale,
            viewer::SelectionIntentMode::replace);
        CHECK(
            topology_controller
                .primaryBodyTopologySelection()
                ->kind ==
            viewer::BodyTopologyPresentationKind::
                vertex);
        CHECK(topology_inspection_events == 2U);

        topology_viewport.emitBodyTopologyPreselection(
            stale,
            {100.0, 100.0});
        CHECK(
            !topology_viewport.body_preselection_);

        viewer::BodyTopologyPickQueryResult edge_toggle;
        edge_toggle.completed = true;
        edge_toggle.generation = generation;
        edge_toggle.candidates = {
            {
                edge_token,
                viewer::BodyTopologyPresentationKind::
                    edge,
                0.0,
                1.0},
        };
        topology_viewport.emitBodyTopology(
            edge_toggle,
            viewer::SelectionIntentMode::toggle);
        CHECK(
            topology_controller
                .bodyTopologySelection()
                .size() == 2U);
        CHECK(
            topology_controller
                .primaryBodyTopologySelection()
                ->kind ==
            viewer::BodyTopologyPresentationKind::
                edge);
        CHECK(topology_inspection_events == 3U);
        CHECK(last_topology_inspection.has_value());
        CHECK(
            last_topology_inspection->kind ==
            viewer::BodyTopologyPresentationKind::
                edge);
        CHECK(
            last_topology_inspection
                ->selection_count == 2U);
        CHECK(
            last_topology_inspection
                ->strict_referenceability ==
            kernel::ReferenceStatus::unsupported);
        CHECK(
            last_topology_inspection
                ->carrier_referenceability ==
            kernel::ReferenceStatus::unsupported);
        CHECK(
            last_topology_inspection->curve_kind ==
            std::optional<kernel::CurveKind>{
                kernel::CurveKind::line});

        const auto revision_before_style =
            topology_session.document()
                .revision();
        CHECK(
            topology_controller.setViewStyle(
                viewer::ViewStyle::
                    shaded_with_hidden_edges));
        CHECK(
            topology_controller.viewStyle() ==
            viewer::ViewStyle::
                shaded_with_hidden_edges);
        CHECK(
            topology_viewport.view_style_ ==
            viewer::ViewStyle::
                shaded_with_hidden_edges);
        CHECK(
            topology_session.document()
                .revision() ==
            revision_before_style);

        auto properties =
            topology_session.document()
                .properties();
        properties.title =
            "PM-02D2 generation invalidation";
        const auto property_change =
            topology_session.execute(
                application::
                    SetDocumentPropertiesCommand{
                        std::move(properties)});
        CHECK(property_change.ok());
        topology_controller.refreshPresentation();
        CHECK(
            topology_viewport.body_scene_
                .generation != generation);
        CHECK(
            topology_controller
                .bodyTopologySelection()
                .empty());
        CHECK(
            !topology_controller
                 .primaryBodyTopologySelection());
        CHECK(
            !topology_viewport.body_preselection_);
        CHECK(topology_inspection_events == 4U);
        CHECK(!last_topology_inspection.has_value());

        // A document revision rebuilds BodyScene with a new generation and
        // the semantic Feature contribution is reprojected onto the new
        // PresentationTokens for that generation.
        auto topology_properties =
            topology_session.document().properties();
        topology_properties.title =
            "PM-02D4 contribution generation refresh";
        const auto topology_property_change =
            topology_session.execute(
                application::SetDocumentPropertiesCommand{
                    std::move(topology_properties)});
        CHECK(
            topology_property_change.ok() &&
            topology_property_change.changed);
        topology_controller.refreshPresentation();
        CHECK(
            topology_viewport.body_scene_.generation.valid());
        CHECK(
            topology_viewport.body_scene_.generation !=
            contribution_generation);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .generation ==
            topology_viewport.body_scene_.generation);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.size() == 1U);
        CHECK(
            topology_viewport.body_topology_overlay_scene_
                .groups.front().role ==
            viewer::BodyTopologyOverlayRole::
                feature_contribution_selected);

        topology_controller.setSolidModelingKernel(
            nullptr);
        topology_controller.clear();
    }

    // H7: if a higher Feature loses its source Profile, the final Body is
    // unavailable but the current-revision lower Feature prefix remains
    // visible. If the first Feature loses its Profile, no prefix exists.
    {
        auto solid_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            solid_session{
                std::filesystem::path{
                    "pm01h7-prefix.ss2part"},
                std::move(solid_document)};
        FakeSolidKernel solid_kernel;

        const auto make_profile =
            [&solid_session](double x0)
                -> part::ProfileId {
            const auto created_sketch =
                solid_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::
                            xy_plane});
            CHECK(
                created_sketch.ok() &&
                created_sketch.sketch_id);

            const auto rectangle =
                solid_session.execute(
                    application::AddSketchRectangleCommand{
                        *created_sketch.sketch_id,
                        solid_session.document()
                            .revision(),
                        {x0, 0.0},
                        {x0 + 10.0, 10.0},
                        sketch::EntityRole::regular,
                        false});
            CHECK(rectangle.ok());

            const auto* source =
                solid_session.document()
                    .findSketch(
                        *created_sketch.sketch_id);
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
                        *created_sketch.sketch_id,
                        solid_session.document()
                            .revision(),
                        *intent});
            CHECK(
                profile.ok() &&
                profile.profile_id);
            return *profile.profile_id;
        };

        const auto lower_profile =
            make_profile(0.0);
        const auto lower_feature =
            solid_session.execute(
                application::
                    CreateExtrudeFeatureCommand{
                    lower_profile,
                    solid_session.document()
                        .revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{5.0},
                        false},
                    {}},
                solid_kernel);
        CHECK(
            lower_feature.ok() &&
            lower_feature.feature_id);

        const auto higher_profile =
            make_profile(20.0);
        const auto higher_feature =
            solid_session.execute(
                application::
                    CreateExtrudeFeatureCommand{
                    higher_profile,
                    solid_session.document()
                        .revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{5.0},
                        false},
                    {}},
                solid_kernel);
        CHECK(
            higher_feature.ok() &&
            higher_feature.feature_id);

        controller.setSolidModelingKernel(
            &solid_kernel);
        controller.setDocumentSession(
            &solid_session);
        CHECK(
            viewport.body_scene_
                .triangles.size() == 1U);

        const auto delete_higher =
            solid_session.execute(
                application::DeleteProfileCommand{
                    higher_profile,
                    solid_session.document()
                        .revision()});
        CHECK(delete_higher.ok());
        controller.refreshPresentation();

        const auto higher_failed =
            part::evaluatePart(
                solid_session.document(),
                solid_kernel);
        CHECK(
            higher_failed.body_status ==
            part::BodyEvaluationStatus::
                unavailable);
        CHECK(higher_failed.body_solid == nullptr);
        CHECK(
            higher_failed.resolved_prefix_solid !=
            nullptr);
        CHECK(
            higher_failed.features[0].status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(
            higher_failed.features[1].status ==
            part::FeatureEvaluationStatus::
                blocked);
        CHECK(
            viewport.body_scene_
                .triangles.size() == 1U);
        CHECK(viewport.body_scene_.generation.valid());
        CHECK(
            viewport.body_scene_.purpose ==
            viewer::BodyScenePurpose::
                diagnostic_prefix);

        const auto delete_lower =
            solid_session.execute(
                application::DeleteProfileCommand{
                    lower_profile,
                    solid_session.document()
                        .revision()});
        CHECK(delete_lower.ok());
        controller.refreshPresentation();

        const auto first_failed =
            part::evaluatePart(
                solid_session.document(),
                solid_kernel);
        CHECK(
            first_failed.body_status ==
            part::BodyEvaluationStatus::
                unavailable);
        CHECK(first_failed.body_solid == nullptr);
        CHECK(
            first_failed.resolved_prefix_solid ==
            nullptr);
        CHECK(
            viewport.body_scene_
                .triangles.empty());
        CHECK(
            !viewport.body_scene_.generation.valid());

        controller.setSolidModelingKernel(
            nullptr);
        controller.clear();
    }

    return EXIT_SUCCESS;
}
