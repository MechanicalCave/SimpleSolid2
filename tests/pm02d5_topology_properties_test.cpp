#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer/document_viewport.hpp>

#include <QApplication>
#include <QLabel>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QWidget>

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
            << "PM-02D5 topology Properties CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class FakeSolid final : public kernel::RuntimeSolid {};

class TopologyKernel final
    : public kernel::ISolidModelingKernel {
public:
    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid() || upstream ||
            input.profile.outer.boundary.size() != 1U) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }

        constexpr kernel::RuntimeFaceToken cap_face_token{101U};
        constexpr kernel::RuntimeFaceToken side_face_token{102U};
        constexpr kernel::RuntimeEdgeToken seam_edge_token{201U};
        constexpr kernel::RuntimeEdgeToken circle_edge_token{202U};
        constexpr kernel::RuntimeEdgeToken unsupported_edge_token{203U};
        constexpr kernel::RuntimeVertexToken vertex_token{301U};
        constexpr kernel::RuntimeSurfaceToken cap_surface_token{401U};
        constexpr kernel::RuntimeSurfaceToken side_surface_token{402U};

        kernel::ExtrudeFaceRole cap_role;
        cap_role.kind =
            kernel::ExtrudeGeneratedFaceRoleKind::cap;
        cap_role.cap_role =
            kernel::ExtrudeCapRole::profile_cap;

        kernel::ExtrudeFaceRole side_role;
        side_role.kind =
            kernel::ExtrudeGeneratedFaceRoleKind::side;
        side_role.side_provenance =
            input.profile.outer.boundary.front().provenance;
        CHECK(side_role.side_provenance.has_value());

        kernel::NewFaceLineage cap_face;
        cap_face.role = cap_role;
        cap_face.status =
            kernel::ReferenceStatus::resolved;
        cap_face.candidate_count = 1U;
        cap_face.resolved_token = cap_face_token;

        kernel::NewFaceLineage side_face;
        side_face.role = side_role;
        side_face.status =
            kernel::ReferenceStatus::resolved;
        side_face.candidate_count = 1U;
        side_face.resolved_token = side_face_token;

        kernel::NewSurfaceLineage cap_surface;
        cap_surface.role = cap_role;
        cap_surface.surface_status =
            kernel::ReferenceStatus::resolved;
        cap_surface.strict_face_status =
            kernel::ReferenceStatus::resolved;
        cap_surface.candidate_face_count = 1U;
        cap_surface.surface_kind =
            kernel::SurfaceKind::plane;
        cap_surface.canonical_frame =
            input.profile.frame;
        cap_surface.resolved_token =
            cap_surface_token;
        cap_surface.current_faces = {
            cap_face_token};

        kernel::NewSurfaceLineage side_surface;
        side_surface.role = side_role;
        side_surface.surface_status =
            kernel::ReferenceStatus::resolved;
        side_surface.strict_face_status =
            kernel::ReferenceStatus::resolved;
        side_surface.candidate_face_count = 1U;
        side_surface.surface_kind =
            kernel::SurfaceKind::cylinder;
        side_surface.resolved_token =
            side_surface_token;
        side_surface.current_faces = {
            side_face_token};

        kernel::CurrentEdgeSemanticObservation seam;
        seam.runtime_token = seam_edge_token;
        seam.provider_curve_kind =
            kernel::CurveKind::line;
        seam.periodic_seam = true;
        seam.adjacent_surfaces = {
            side_surface_token};

        kernel::CurrentEdgeSemanticObservation circle;
        circle.runtime_token = circle_edge_token;
        circle.provider_curve_kind =
            kernel::CurveKind::circle;
        circle.adjacent_surfaces = {
            cap_surface_token,
            side_surface_token};

        kernel::CurrentEdgeSemanticObservation unsupported;
        unsupported.runtime_token =
            unsupported_edge_token;
        unsupported.provider_curve_kind =
            kernel::CurveKind::line;
        unsupported.adjacent_surfaces = {
            side_surface_token};

        kernel::CurrentVertexSemanticObservation vertex;
        vertex.runtime_token = vertex_token;
        vertex.adjacent_surfaces = {
            side_surface_token};
        vertex.incident_material_edges = {
            circle_edge_token,
            unsupported_edge_token};
        vertex.provider_point =
            kernel::Point3{1.0, 2.0, 3.0};

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        result.face_count = 2U;
        result.edge_count = 3U;
        result.vertex_count = 1U;
        result.current_faces = {
            cap_face_token,
            side_face_token};
        result.current_edges = {
            seam_edge_token,
            circle_edge_token,
            unsupported_edge_token};
        result.current_vertices = {
            vertex_token};
        result.new_faces = {
            std::move(cap_face),
            std::move(side_face)};
        result.new_surfaces = {
            std::move(cap_surface),
            std::move(side_surface)};
        result.current_edge_semantics = {
            std::move(seam),
            std::move(circle),
            std::move(unsupported)};
        result.current_vertex_semantics = {
            std::move(vertex)};
        return result;
    }

    kernel::BodyPresentationResult bodyPresentation(
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
        result.body.mesh.triangles = {
            {
                {-5.0, -5.0, 0.0},
                {5.0, -5.0, 0.0},
                {0.0, 5.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
            },
            {
                {-5.0, 0.0, 0.0},
                {5.0, 0.0, 0.0},
                {0.0, 0.0, 5.0},
                {0.0, 1.0, 0.0},
                {0.0, 1.0, 0.0},
                {0.0, 1.0, 0.0},
            },
        };
        result.body.faces = {
            {kernel::RuntimeFaceToken{101U}, 0U, 1U},
            {kernel::RuntimeFaceToken{102U}, 1U, 1U},
        };
        result.body.edges = {
            {
                kernel::RuntimeEdgeToken{201U},
                {{0.0, -5.0, 0.0},
                 {0.0, -5.0, 5.0}},
            },
            {
                kernel::RuntimeEdgeToken{202U},
                {{-5.0, 0.0, 0.0},
                 {5.0, 0.0, 0.0}},
            },
            {
                kernel::RuntimeEdgeToken{203U},
                {{0.0, 5.0, 0.0},
                 {0.0, 5.0, 5.0}},
            },
        };
        result.body.vertices = {
            {
                kernel::RuntimeVertexToken{301U},
                {1.0, 2.0, 3.0},
            },
        };
        return result;
    }
};

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
                camera_, view);
        if (!next) return false;
        camera_ = *next;
        return true;
    }
    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                camera_, projection);
        if (!next) return false;
        camera_ = *next;
        return true;
    }
    void fitAll() override {}

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        return scene.valid();
    }
    bool setBodyScene(
        const viewer::BodyScene& scene) override {
        if (!scene.valid()) return false;
        body_scene = scene;
        return true;
    }
    bool setBodyTopologyOverlayScene(
        const viewer::BodyTopologyOverlayScene& scene) override {
        return scene.valid();
    }
    viewer::ViewStyle viewStyle() const noexcept override {
        return style;
    }
    bool setViewStyle(
        viewer::ViewStyle next) override {
        style = next;
        return true;
    }
    void setViewStyleActionHandler(
        viewer::ViewStyleActionHandler handler) override {
        view_style_handler =
            std::move(handler);
    }
    viewer::BodyTopologyPickQueryResult
    queryBodyTopology(
        viewer::ViewportPoint2,
        viewer::BodyTopologyPickFilter = {}) override {
        return {};
    }
    void setBodyTopologySelectionIntentHandler(
        viewer::BodyTopologySelectionIntentHandler handler) override {
        topology_handler =
            std::move(handler);
    }
    void setBodyTopologyPreselectionIntentHandler(
        viewer::BodyTopologyPreselectionIntentHandler handler) override {
        preselection_handler =
            std::move(handler);
    }
    void setBodyTopologyCycleIntentHandler(
        viewer::BodyTopologyCycleIntentHandler handler) override {
        cycle_handler =
            std::move(handler);
    }
    bool setBodyTopologyPreselection(
        std::optional<viewer::PresentationToken>) override {
        return true;
    }

    bool setSolidScene(
        const viewer::SolidScene& scene) override {
        return scene.valid();
    }
    bool setSolidPreviewScene(
        const viewer::SolidPreviewScene& scene) override {
        return scene.valid();
    }
    bool setProfileScene(
        const viewer::ProfileScene& scene) override {
        return scene.valid();
    }
    bool setProfilePreviewScene(
        const viewer::ProfilePreviewScene& scene) override {
        return scene.valid();
    }
    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        return scene.valid();
    }
    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        return scene.valid();
    }
    bool setPresentationSelection(
        const viewer::PresentationSelection& scene) override {
        selection = scene;
        return scene.valid();
    }
    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        return {point.valid(), std::nullopt};
    }
    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rect,
        viewer::SketchRectangleSelectionRule) override {
        return {rect.valid(), {}};
    }
    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override {
        return overlay.valid();
    }
    void clearSketchSelectionBoxOverlay() override {}
    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler =
            std::move(handler);
    }
    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_handler =
            std::move(handler);
    }
    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting) override {}
    void setCursorMode(
        viewer::ViewportCursorMode) override {}

    void clickBody(
        viewer::PresentationToken token,
        viewer::BodyTopologyPresentationKind kind) {
        CHECK(static_cast<bool>(topology_handler));
        viewer::BodyTopologyPickQueryResult query;
        query.completed = true;
        query.generation = body_scene.generation;
        query.candidates.push_back(
            {token, kind, 0.0, 1.0});
        CHECK(query.valid());
        topology_handler(
            query,
            viewer::SelectionIntentMode::replace);
    }

    viewer::CameraState camera_;
    viewer::BodyScene body_scene;
    viewer::ViewStyle style{
        viewer::ViewStyle::shaded};
    viewer::PresentationSelection selection;
    viewer::ViewStyleActionHandler view_style_handler;
    viewer::BodyTopologySelectionIntentHandler
        topology_handler;
    viewer::BodyTopologyPreselectionIntentHandler
        preselection_handler;
    viewer::BodyTopologyCycleIntentHandler cycle_handler;
    viewer::SelectionIntentHandler selection_handler;
    viewer::SpatialPointerHandler spatial_handler;
};

part::ProfileId createProfile(
    application::DocumentSession& session) {
    const auto sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);

    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                *sketch.sketch_id,
                {0.0, 0.0},
                5.0,
                sketch::EntityRole::regular});
    CHECK(circle.ok() && circle.entity_id);

    const auto* source =
        session.document().findSketch(
            *sketch.sketch_id);
    CHECK(source != nullptr);
    const auto regions =
        sketch::analyzeRegions(source->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);
    return *profile.profile_id;
}

QTreeWidgetItem* findItem(
    QTreeWidget& tree,
    const QString& prefix) {
    const auto items =
        tree.findItems(
            prefix,
            Qt::MatchStartsWith |
                Qt::MatchRecursive,
            0);
    return items.empty()
        ? nullptr
        : items.front();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "pm02d6-inspection.ss2part"},
        std::move(document)};
    const auto profile_id =
        createProfile(session);

    TopologyKernel kernel;
    const auto feature =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{5.0},
                    false},
                {}},
            kernel);
    CHECK(feature.ok() && feature.feature_id);

    TestViewport* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            auto* created =
                new TestViewport(parent);
            viewport = created;
            return ui::ViewportSurface{
                created,
                created};
        },
        &kernel};
    CHECK(viewport != nullptr);
    CHECK(workbench.activateDocument(
        &session,
        {}));
    CHECK(viewport->body_scene.generation.valid());
    CHECK(viewport->body_scene.faces.size() == 2U);
    CHECK(viewport->body_scene.edges.size() == 3U);
    CHECK(viewport->body_scene.vertices.size() == 1U);

    auto* tree =
        workbench.findChild<QTreeWidget*>();
    CHECK(tree != nullptr);

    // Body Properties use only current successful final-Body catalog.
    auto* body_item =
        findItem(*tree, QStringLiteral("Body"));
    CHECK(body_item != nullptr);
    tree->clearSelection();
    body_item->setSelected(true);
    tree->setCurrentItem(body_item);

    auto* body_counts =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "bodyPropertyTopologyCounts"));
    auto* body_accounting =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "bodyPropertyTopologyAccounting"));
    CHECK(body_counts && body_accounting);
    CHECK(body_counts->text().contains(
        QStringLiteral("Faces 2")));
    CHECK(body_counts->text().contains(
        QStringLiteral("Edges 3")));
    CHECK(body_counts->text().contains(
        QStringLiteral("Vertices 1")));
    CHECK(body_accounting->text().contains(
        QStringLiteral("1 referenceable")));

    // Feature Properties use the same semantic contribution query as the
    // viewport overlay, rather than reading green presentation pixels.
    auto* feature_item =
        findItem(*tree, QStringLiteral("Extrude"));
    CHECK(feature_item != nullptr);
    tree->clearSelection();
    feature_item->setSelected(true);
    tree->setCurrentItem(feature_item);

    auto* contribution =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "featurePropertyContribution"));
    auto* contribution_diagnostic =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "featurePropertyContributionDiagnostics"));
    CHECK(contribution && contribution_diagnostic);
    CHECK(contribution->text().contains(
        QStringLiteral("Faces 2")));
    CHECK(contribution_diagnostic->text().contains(
        QStringLiteral("Missing 0")));

    // Direct Face selection becomes the single primary Properties subject.
    const auto face =
        viewport->body_scene.faces.front().token;
    viewport->clickBody(
        face,
        viewer::BodyTopologyPresentationKind::face);

    auto* topology_page =
        workbench.findChild<QWidget*>(
            QStringLiteral("topologyPropertiesPage"));
    auto* stack =
        workbench.findChild<QStackedWidget*>(
            QStringLiteral("propertiesContextStack"));
    auto* kind =
        workbench.findChild<QLabel*>(
            QStringLiteral("topologyPropertyKind"));
    auto* strict_reference =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "topologyPropertyStrictReference"));
    auto* carrier_reference =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "topologyPropertyCarrierReference"));
    auto* carrier_type =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "topologyPropertyCarrierType"));
    auto* support =
        workbench.findChild<QLabel*>(
            QStringLiteral(
                "topologyPropertySketchSupport"));
    auto* carrier =
        workbench.findChild<QLabel*>(
            QStringLiteral("topologyPropertyCarrier"));
    CHECK(
        topology_page && stack && kind &&
        strict_reference && carrier_reference &&
        carrier_type && support && carrier);
    CHECK(stack->currentWidget() == topology_page);
    CHECK(kind->text() == QStringLiteral("Face"));
    CHECK(
        strict_reference->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        carrier_reference->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        carrier_type->text() ==
        QStringLiteral("Plane"));
    CHECK(
        support->text() ==
        QStringLiteral("Supported"));

    // Product Properties expose semantic meaning, never provider/runtime IDs.
    const auto all_text =
        kind->text() +
        strict_reference->text() +
        carrier_reference->text() +
        carrier_type->text() +
        support->text() +
        carrier->text();
    CHECK(!all_text.contains(
        QStringLiteral("Runtime")));
    CHECK(!all_text.contains(
        QStringLiteral("PresentationToken")));
    CHECK(!all_text.contains(
        QStringLiteral("101")));
    CHECK(!all_text.contains(
        QStringLiteral("401")));

    // D6 acceptance hardening: the second current Face is a semantic
    // cylindrical side Surface. It remains an ordinary selectable Face, but
    // standard planar Sketch support is explicitly unsupported.
    const auto cylinder_face =
        viewport->body_scene.faces.at(1U).token;
    viewport->clickBody(
        cylinder_face,
        viewer::BodyTopologyPresentationKind::face);
    CHECK(stack->currentWidget() == topology_page);
    CHECK(kind->text() == QStringLiteral("Face"));
    CHECK(
        strict_reference->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        carrier_reference->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        carrier_type->text() ==
        QStringLiteral("Cylinder"));
    CHECK(
        support->text() ==
        QStringLiteral("Unsupported — non-planar"));

    // The periodic seam is fully accounted but deliberately not an ordinary
    // material/pickable Edge. Body summary reports the artifact.
    CHECK(!viewport->body_scene.edges.at(0U).material);
    CHECK(!viewport->body_scene.edges.at(0U).ordinary_pickable);
    CHECK(body_accounting->text().contains(
        QStringLiteral("1 artifacts")));

    auto* accounting =
        workbench.findChild<QLabel*>(
            QStringLiteral("topologyPropertyAccounting"));
    auto* geometry =
        workbench.findChild<QLabel*>(
            QStringLiteral("topologyPropertyGeometry"));
    CHECK(accounting && geometry);

    // A defensible material cap/side boundary is a semantic Circle.
    viewport->clickBody(
        viewport->body_scene.edges.at(1U).token,
        viewer::BodyTopologyPresentationKind::edge);
    CHECK(kind->text() == QStringLiteral("Edge"));
    CHECK(
        carrier_type->text() ==
        QStringLiteral("Circle"));
    CHECK(
        strict_reference->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        carrier_reference->text() ==
        QStringLiteral("Resolved"));
    CHECK(
        support->text() ==
        QStringLiteral("Not applicable"));

    // A material Edge without a defensible two-Surface semantic relation
    // remains directly inspectable, but durable meaning is Unsupported.
    viewport->clickBody(
        viewport->body_scene.edges.at(2U).token,
        viewer::BodyTopologyPresentationKind::edge);
    CHECK(kind->text() == QStringLiteral("Edge"));
    CHECK(
        accounting->text() ==
        QStringLiteral("Semantically Unsupported"));
    CHECK(
        strict_reference->text() ==
        QStringLiteral("Unsupported"));
    CHECK(
        carrier_reference->text() ==
        QStringLiteral("Unsupported"));
    CHECK(
        carrier_type->text() ==
        QStringLiteral("Line"));

    // Vertex stays inspectable even without a durable semantic Point claim;
    // XYZ is presentation/geometry diagnostics only.
    viewport->clickBody(
        viewport->body_scene.vertices.front().token,
        viewer::BodyTopologyPresentationKind::vertex);
    CHECK(kind->text() == QStringLiteral("Vertex"));
    CHECK(
        carrier_type->text() ==
        QStringLiteral("Point"));
    CHECK(
        strict_reference->text() ==
        QStringLiteral("Unsupported"));
    CHECK(
        carrier_reference->text() ==
        QStringLiteral("Unsupported"));
    CHECK(
        geometry->text() ==
        QStringLiteral("XYZ = (1, 2, 3)"));
    CHECK(
        support->text() ==
        QStringLiteral("Not applicable"));

    const auto hardening_text =
        kind->text() +
        accounting->text() +
        strict_reference->text() +
        carrier_reference->text() +
        carrier_type->text() +
        support->text() +
        geometry->text();
    CHECK(!hardening_text.contains(
        QStringLiteral("Runtime")));
    CHECK(!hardening_text.contains(
        QStringLiteral("PresentationToken")));
    CHECK(!hardening_text.contains(
        QStringLiteral("201")));
    CHECK(!hardening_text.contains(
        QStringLiteral("301")));

    return EXIT_SUCCESS;
}
