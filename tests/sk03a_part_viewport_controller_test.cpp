#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <algorithm>
#include <cstddef>
#include <map>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

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

    bool setBodyScene(
        const viewer::BodyScene& scene) override {
        ++body_scene_calls_;
        if (fail_body_scene_ ||
            !scene.valid()) {
            return false;
        }
        body_scene_ = scene;
        return true;
    }

    viewer::ViewStyle
    viewStyle() const noexcept override {
        return view_style_;
    }

    bool setViewStyle(
        viewer::ViewStyle style) override {
        ++view_style_calls_;
        view_style_ = style;
        return true;
    }

    void setViewStyleActionHandler(
        viewer::ViewStyleActionHandler handler) override {
        view_style_handler_ =
            std::move(handler);
    }

    void requestViewStyle(
        viewer::ViewStyle style) {
        CHECK(static_cast<bool>(
            view_style_handler_));
        view_style_handler_(style);
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
    viewer::BodyScene body_scene_;
    viewer::ViewStyle view_style_{
        viewer::ViewStyle::shaded};
    viewer::ViewStyleActionHandler
        view_style_handler_;
    viewer::SolidPreviewScene
        solid_preview_scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene preview_scene_;
    std::size_t solid_scene_calls_{};
    std::size_t body_scene_calls_{};
    std::size_t view_style_calls_{};
    std::size_t solid_preview_scene_calls_{};
    std::size_t sketch_scene_calls_{};
    std::size_t preview_scene_calls_{};
    bool fail_preview_{};
    bool fail_sketch_scene_{};
    bool fail_body_scene_{};
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
    : public kernel::RuntimeSolid {
public:
    struct Edge final {
        kernel::RuntimeEdgeToken token;
        std::vector<kernel::RuntimeSurfaceToken>
            surfaces;
        kernel::Point3 start;
        kernel::Point3 end;
    };

    struct Vertex final {
        kernel::RuntimeVertexToken token;
        std::vector<kernel::RuntimeSurfaceToken>
            surfaces;
        std::vector<kernel::RuntimeEdgeToken>
            incident_edges;
        kernel::Point3 point;
    };

    std::vector<kernel::RuntimeFaceToken> faces;
    std::vector<kernel::RuntimeSurfaceToken>
        surfaces;
    std::vector<Edge> edges;
    std::vector<Vertex> vertices;
    std::uint64_t next_face{1U};
    std::uint64_t next_surface{1U};
    std::uint64_t next_edge{1U};
    std::uint64_t next_vertex{1U};
};

class FakeSolidKernel final
    : public kernel::ISolidModelingKernel {
public:
    bool fail_mesh{};
    std::size_t extrude_calls{};
    std::size_t mesh_calls{};
    std::size_t body_presentation_calls{};

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

        auto runtime =
            std::make_shared<FakeSolid>();
        std::map<std::uint64_t,
                 kernel::RuntimeEdgeToken>
            edge_descendants;

        if (upstream) {
            const auto* previous =
                dynamic_cast<const FakeSolid*>(
                    upstream.get());
            if (previous == nullptr ||
                previous->faces.size() !=
                    previous->surfaces.size()) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }

            runtime->next_face =
                previous->next_face;
            runtime->next_surface =
                previous->next_surface;
            runtime->next_edge =
                previous->next_edge;
            runtime->next_vertex =
                previous->next_vertex;

            for (std::size_t index = 0U;
                 index < previous->faces.size();
                 ++index) {
                const auto face =
                    previous->faces[index];
                const auto surface =
                    previous->surfaces[index];
                runtime->faces.push_back(face);
                runtime->surfaces.push_back(surface);
                result.inherited_faces.push_back(
                    {
                        face,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                    });
                result.inherited_surfaces.push_back(
                    {
                        surface,
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kernel::SurfaceKind::plane,
                        kernel::Frame3{},
                        {face},
                    });
            }

            for (const auto& old_edge :
                 previous->edges) {
                const kernel::RuntimeEdgeToken
                    edge{
                        runtime->next_edge++};
                edge_descendants.emplace(
                    old_edge.token.value,
                    edge);
                runtime->edges.push_back(
                    {
                        edge,
                        old_edge.surfaces,
                        old_edge.start,
                        old_edge.end,
                    });
                result.inherited_edge_realizations
                    .push_back(
                        {
                            old_edge.token,
                            kernel::ReferenceStatus::
                                resolved,
                            1U,
                            {edge},
                        });
            }

            for (const auto& old_vertex :
                 previous->vertices) {
                const kernel::RuntimeVertexToken
                    vertex{
                        runtime->next_vertex++};
                std::vector<kernel::RuntimeEdgeToken>
                    incident;
                for (const auto old_edge :
                     old_vertex.incident_edges) {
                    const auto found =
                        edge_descendants.find(
                            old_edge.value);
                    if (found !=
                        edge_descendants.end()) {
                        incident.push_back(
                            found->second);
                    }
                }
                runtime->vertices.push_back(
                    {
                        vertex,
                        old_vertex.surfaces,
                        std::move(incident),
                        old_vertex.point,
                    });
                result.inherited_vertex_realizations
                    .push_back(
                        {
                            old_vertex.token,
                            kernel::ReferenceStatus::
                                resolved,
                            1U,
                            {vertex},
                        });
            }
        }

        const auto publish_surface =
            [&result, &runtime](
                kernel::ExtrudeFaceRole role) {
                const kernel::RuntimeFaceToken
                    face{runtime->next_face++};
                const kernel::RuntimeSurfaceToken
                    surface{
                        runtime->next_surface++};
                runtime->faces.push_back(face);
                runtime->surfaces.push_back(surface);
                result.new_faces.push_back(
                    {
                        role,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        face,
                    });
                result.new_surfaces.push_back(
                    {
                        std::move(role),
                        kernel::ReferenceStatus::
                            resolved,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        kernel::SurfaceKind::plane,
                        kernel::Frame3{},
                        surface,
                        {face},
                    });
                return surface;
            };

        std::vector<kernel::RuntimeSurfaceToken>
            created_surfaces;
        created_surfaces.reserve(6U);
        created_surfaces.push_back(
            publish_surface(
                {
                    kernel::ExtrudeGeneratedFaceRoleKind::cap,
                    input.start_cap_role,
                    std::nullopt,
                }));
        created_surfaces.push_back(
            publish_surface(
                {
                    kernel::ExtrudeGeneratedFaceRoleKind::cap,
                    input.end_cap_role,
                    std::nullopt,
                }));

        for (const auto& use :
             input.profile.outer.boundary) {
            created_surfaces.push_back(
                publish_surface(
                    {
                        kernel::ExtrudeGeneratedFaceRoleKind::side,
                        std::nullopt,
                        use.provenance,
                    }));
        }
        if (created_surfaces.size() != 6U) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        }

        const auto s0 = created_surfaces[0];
        const auto s1 = created_surfaces[1];
        const auto s2 = created_surfaces[2];
        const auto s3 = created_surfaces[3];
        const auto s4 = created_surfaces[4];
        const auto s5 = created_surfaces[5];

        const auto add_edge =
            [&runtime](
                kernel::RuntimeSurfaceToken first,
                kernel::RuntimeSurfaceToken second,
                kernel::Point3 start,
                kernel::Point3 end) {
                const kernel::RuntimeEdgeToken
                    token{runtime->next_edge++};
                runtime->edges.push_back(
                    {
                        token,
                        {first, second},
                        start,
                        end,
                    });
                return token;
            };

        const double z0 =
            input.start_offset_mm;
        const double z1 =
            input.end_offset_mm;
        const kernel::Point3 p000{0.0, 0.0, z0};
        const kernel::Point3 p100{1.0, 0.0, z0};
        const kernel::Point3 p110{1.0, 1.0, z0};
        const kernel::Point3 p010{0.0, 1.0, z0};
        const kernel::Point3 p001{0.0, 0.0, z1};
        const kernel::Point3 p101{1.0, 0.0, z1};
        const kernel::Point3 p111{1.0, 1.0, z1};
        const kernel::Point3 p011{0.0, 1.0, z1};

        const std::vector<kernel::RuntimeEdgeToken>
            new_edges{
                add_edge(s0, s2, p000, p100),
                add_edge(s0, s3, p100, p110),
                add_edge(s0, s4, p110, p010),
                add_edge(s0, s5, p010, p000),
                add_edge(s1, s2, p001, p101),
                add_edge(s1, s3, p101, p111),
                add_edge(s1, s4, p111, p011),
                add_edge(s1, s5, p011, p001),
                add_edge(s2, s3, p100, p101),
                add_edge(s3, s4, p110, p111),
                add_edge(s4, s5, p010, p011),
                add_edge(s5, s2, p000, p001),
            };

        const auto add_vertex =
            [&runtime](
                std::vector<kernel::RuntimeSurfaceToken>
                    surfaces,
                std::vector<kernel::RuntimeEdgeToken>
                    edges,
                kernel::Point3 point) {
                const kernel::RuntimeVertexToken
                    token{runtime->next_vertex++};
                runtime->vertices.push_back(
                    {
                        token,
                        std::move(surfaces),
                        std::move(edges),
                        point,
                    });
            };

        add_vertex(
            {s0, s2, s5},
            {new_edges[0], new_edges[3], new_edges[11]},
            p000);
        add_vertex(
            {s0, s2, s3},
            {new_edges[0], new_edges[1], new_edges[8]},
            p100);
        add_vertex(
            {s0, s3, s4},
            {new_edges[1], new_edges[2], new_edges[9]},
            p110);
        add_vertex(
            {s0, s4, s5},
            {new_edges[2], new_edges[3], new_edges[10]},
            p010);
        add_vertex(
            {s1, s2, s5},
            {new_edges[4], new_edges[7], new_edges[11]},
            p001);
        add_vertex(
            {s1, s2, s3},
            {new_edges[4], new_edges[5], new_edges[8]},
            p101);
        add_vertex(
            {s1, s3, s4},
            {new_edges[5], new_edges[6], new_edges[9]},
            p111);
        add_vertex(
            {s1, s4, s5},
            {new_edges[6], new_edges[7], new_edges[10]},
            p011);

        result.current_faces = runtime->faces;
        result.current_edges.reserve(
            runtime->edges.size());
        result.current_edge_semantics.reserve(
            runtime->edges.size());
        for (const auto& edge :
             runtime->edges) {
            result.current_edges.push_back(
                edge.token);
            result.current_edge_semantics.push_back(
                {
                    edge.token,
                    kernel::CurveKind::line,
                    false,
                    edge.surfaces,
                });
        }

        result.current_vertices.reserve(
            runtime->vertices.size());
        result.current_vertex_semantics.reserve(
            runtime->vertices.size());
        for (const auto& vertex :
             runtime->vertices) {
            result.current_vertices.push_back(
                vertex.token);
            result.current_vertex_semantics.push_back(
                {
                    vertex.token,
                    vertex.surfaces,
                    vertex.incident_edges,
                    vertex.point,
                });
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid = runtime;
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count =
            result.current_faces.size();
        result.edge_count =
            result.current_edges.size();
        result.vertex_count =
            result.current_vertices.size();
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
        if (!solid ||
            dynamic_cast<const FakeSolid*>(
                solid.get()) == nullptr) {
            return {
                solid
                    ? kernel::SolidPresentationStatus::
                          provider_mismatch
                    : kernel::SolidPresentationStatus::
                          invalid_input,
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

    kernel::BodyPresentationResult
    bodyPresentation(
        kernel::RuntimeSolidHandle solid) noexcept override {
        ++mesh_calls;
        ++body_presentation_calls;

        kernel::BodyPresentationResult result;
        if (fail_mesh) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }

        const auto* runtime =
            solid
                ? dynamic_cast<
                      const FakeSolid*>(solid.get())
                : nullptr;
        if (runtime == nullptr) {
            result.status =
                solid
                    ? kernel::SolidPresentationStatus::
                          provider_mismatch
                    : kernel::SolidPresentationStatus::
                          invalid_input;
            return result;
        }

        result.faces.reserve(
            runtime->faces.size());
        for (std::size_t index = 0U;
             index < runtime->faces.size();
             ++index) {
            const double x =
                static_cast<double>(index) * 2.0;
            const auto first =
                result.mesh.triangles.size();
            result.mesh.triangles.push_back(
                {
                    {x, 0.0, 0.0},
                    {x + 1.0, 0.0, 0.0},
                    {x, 1.0, 0.0},
                    {0.0, 0.0, 1.0},
                    {0.0, 0.0, 1.0},
                    {0.0, 0.0, 1.0}});
            result.faces.push_back(
                {
                    runtime->faces[index],
                    first,
                    1U,
                });
        }

        result.edges.reserve(
            runtime->edges.size());
        for (const auto& edge :
             runtime->edges) {
            result.edges.push_back(
                {
                    edge.token,
                    {edge.start, edge.end},
                });
        }

        result.vertices.reserve(
            runtime->vertices.size());
        for (const auto& vertex :
             runtime->vertices) {
            result.vertices.push_back(
                {
                    vertex.token,
                    vertex.point,
                });
        }

        result.status =
            kernel::SolidPresentationStatus::ok;
        if (!result.ok()) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
        }
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
        CHECK(viewport.solid_scene_calls_ == 0U);
        CHECK(viewport.body_scene_.valid());
        CHECK(!viewport.body_scene_.empty());
        CHECK(
            viewport.body_scene_
                .generation.valid());
        CHECK(
            viewport.body_scene_
                .authoritative_for_modeling);
        CHECK(
            viewport.body_scene_.faces.size() ==
            6U);
        CHECK(
            viewport.body_scene_.edges.size() ==
            12U);
        CHECK(
            viewport.body_scene_.vertices.size() ==
            8U);
        CHECK(
            viewport.body_scene_.triangles.size() ==
            6U);
        CHECK(
            controller.viewStyle() ==
            viewer::ViewStyle::shaded);
        CHECK(
            viewport.view_style_ ==
            viewer::ViewStyle::shaded);

        const auto authored_revision_before_style =
            solid_session.document().revision();
        const auto undo_before_style =
            solid_session.undoDepth();
        const bool dirty_before_style =
            solid_session.needsSave();

        viewport.requestViewStyle(
            viewer::ViewStyle::
                shaded_with_edges);
        CHECK(
            controller.viewStyle() ==
            viewer::ViewStyle::
                shaded_with_edges);
        CHECK(
            viewport.view_style_ ==
            viewer::ViewStyle::
                shaded_with_edges);
        CHECK(
            solid_session.document().revision() ==
            authored_revision_before_style);
        CHECK(
            solid_session.undoDepth() ==
            undo_before_style);
        CHECK(
            solid_session.needsSave() ==
            dirty_before_style);

        viewport.requestViewStyle(
            viewer::ViewStyle::
                shaded_with_hidden_edges);
        CHECK(
            controller.viewStyle() ==
            viewer::ViewStyle::
                shaded_with_hidden_edges);
        CHECK(
            viewport.view_style_ ==
            viewer::ViewStyle::
                shaded_with_hidden_edges);
        CHECK(
            solid_session.document().revision() ==
            authored_revision_before_style);
        CHECK(
            solid_session.undoDepth() ==
            undo_before_style);
        CHECK(
            solid_session.needsSave() ==
            dirty_before_style);

        const auto first_body_generation =
            viewport.body_scene_.generation;

        const auto extrude_calls_after_publish =
            solid_kernel.extrude_calls;
        const auto mesh_calls_after_publish =
            solid_kernel.mesh_calls;
        const auto body_presentation_calls_after_publish =
            solid_kernel.body_presentation_calls;
        controller.refreshPresentation();
        CHECK(
            solid_kernel.extrude_calls ==
            extrude_calls_after_publish);
        CHECK(
            solid_kernel.mesh_calls ==
            mesh_calls_after_publish);
        CHECK(
            solid_kernel.body_presentation_calls ==
            body_presentation_calls_after_publish);
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
        CHECK(viewport.body_scene_.empty());
        CHECK(
            controller.presentationDegraded());

        solid_kernel.fail_mesh = false;
        controller.refreshPresentation();
        CHECK(!viewport.body_scene_.empty());
        CHECK(
            viewport.body_scene_
                .authoritative_for_modeling);
        CHECK(
            viewport.body_scene_.generation.value >
            first_body_generation.value);
        const auto recovered_generation =
            viewport.body_scene_.generation;
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
        CHECK(viewport.body_scene_.empty());
        CHECK(
            !controller.presentationDegraded());

        CHECK(solid_session.undo().changed);
        controller.refreshPresentation();
        CHECK(!viewport.body_scene_.empty());
        CHECK(
            viewport.body_scene_.generation.value >
            recovered_generation.value);

        controller.setSolidModelingKernel(
            nullptr);
        CHECK(viewport.body_scene_.empty());
        controller.clear();
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
        CHECK(viewport.body_scene_.valid());
        CHECK(
            viewport.body_scene_
                .authoritative_for_modeling);
        CHECK(
            viewport.body_scene_.faces.size() ==
            12U);
        CHECK(
            viewport.body_scene_.edges.size() ==
            24U);
        CHECK(
            viewport.body_scene_.vertices.size() ==
            16U);
        const auto full_body_generation =
            viewport.body_scene_.generation;

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
        CHECK(viewport.body_scene_.valid());
        CHECK(!viewport.body_scene_.empty());
        CHECK(
            !viewport.body_scene_
                 .authoritative_for_modeling);
        CHECK(
            viewport.body_scene_.faces.size() ==
            6U);
        CHECK(
            viewport.body_scene_.edges.size() ==
            12U);
        CHECK(
            viewport.body_scene_.vertices.size() ==
            8U);
        CHECK(
            viewport.body_scene_.generation.value >
            full_body_generation.value);

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
        CHECK(viewport.body_scene_.empty());

        controller.setSolidModelingKernel(
            nullptr);
        controller.clear();
    }

    return EXIT_SUCCESS;
}
