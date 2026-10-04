#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_commands.hpp>
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
        if (!input.valid() || upstream) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }

        constexpr kernel::RuntimeFaceToken face_token{101U};
        constexpr kernel::RuntimeEdgeToken edge_token{201U};
        constexpr kernel::RuntimeVertexToken vertex_token{301U};
        constexpr kernel::RuntimeSurfaceToken surface_token{401U};

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
        surface.resolved_token = surface_token;
        surface.current_faces = {face_token};

        kernel::CurrentEdgeSemanticObservation edge;
        edge.runtime_token = edge_token;
        edge.provider_curve_kind =
            kernel::CurveKind::line;

        kernel::CurrentVertexSemanticObservation vertex;
        vertex.runtime_token = vertex_token;
        vertex.provider_point =
            kernel::Point3{1.0, 2.0, 3.0};

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
        result.current_vertices = {vertex_token};
        result.new_faces = {std::move(face)};
        result.new_surfaces = {std::move(surface)};
        result.current_edge_semantics = {
            std::move(edge)};
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
        result.body.mesh.triangles.push_back(
            {
                {-5.0, -5.0, 0.0},
                {5.0, -5.0, 0.0},
                {0.0, 5.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
            });
        result.body.faces.push_back(
            {kernel::RuntimeFaceToken{101U}, 0U, 1U});
        result.body.edges.push_back(
            {
                kernel::RuntimeEdgeToken{201U},
                {{-5.0, 0.0, 0.0},
                 {5.0, 0.0, 0.0}},
            });
        result.body.vertices.push_back(
            {
                kernel::RuntimeVertexToken{301U},
                {1.0, 2.0, 3.0},
            });
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

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch.sketch_id,
                session.document().revision(),
                {-5.0, -5.0},
                {5.0, 5.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

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
            "pm02d5-properties.ss2part"},
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
    CHECK(viewport->body_scene.faces.size() == 1U);

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
        QStringLiteral("Faces 1")));
    CHECK(body_counts->text().contains(
        QStringLiteral("Edges 1")));
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
        QStringLiteral("Faces 1")));
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

    return EXIT_SUCCESS;
}
