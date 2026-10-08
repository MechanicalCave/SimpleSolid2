#include "cad_workbench.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool ok, const char* expression, int line) {
    if (ok) return;
    std::cerr << "PM05F R2 native Workbench Edit CHECK failed at "
              << line << ": " << expression << '\n';
    std::exit(EXIT_FAILURE);
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

application::DocumentSession makeBaseSession(
    kernel_occt::OcctSolidModelingKernel& kernel,
    double width = 40.0,
    double depth = 30.0,
    double height = 20.0) {
    auto source =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{{}, std::move(source)};
    const auto sketch =
        session.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);
    CHECK(session.execute(application::AddSketchRectangleCommand{
        *sketch.sketch_id,
        session.document().revision(),
        {0.0, 0.0},
        {width, depth},
        sketch::EntityRole::regular,
        false}).ok());
    const auto* model =
        session.document().findSketch(*sketch.sketch_id);
    CHECK(model != nullptr);
    const auto regions = sketch::analyzeRegions(model->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto base = session.execute(
        application::CreateExtrudeFeatureCommand{
            *profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{
                core::LengthValue{height}, false},
            "Base"},
        kernel);
    CHECK(base.ok() && base.feature_id);
    return session;
}

application::DocumentSession makeRevolveSession(
    double angle) {
    auto source =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{{}, std::move(source)};
    const auto sketch =
        session.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);
    CHECK(session.execute(application::AddSketchRectangleCommand{
        *sketch.sketch_id,
        session.document().revision(),
        {10.0, 0.0},
        {20.0, 10.0},
        sketch::EntityRole::regular,
        false}).ok());
    const auto* model =
        session.document().findSketch(*sketch.sketch_id);
    CHECK(model != nullptr);
    const auto regions =
        sketch::analyzeRegions(model->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto id = state.body.next_feature_id.allocate();
    CHECK(id.has_value());
    state.body.features.push_back(part::PartFeature{
        *id,
        "Revolve",
        false,
        part::RevolveFeature{
            *profile.profile_id,
            part::AxisReference{
                part::BuiltinOriginAxisReference{
                    core::BuiltinReferenceRole::x_axis}},
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{angle}, false}}});
    auto restored = part::PartDocument::restore(
        session.document().documentId(),
        std::move(state),
        session.document().revision());
    CHECK(restored.ok());
    return application::DocumentSession{
        {}, std::move(*restored.document)};
}

struct WorldEdgeProbe final {
    part::MaterialEdgeReference reference;
    viewer::Point3 world;
};

std::vector<WorldEdgeProbe> authorableEdgeProbes(
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto evaluated =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology.has_value());
    CHECK(evaluated.current_topology->complete());
    CHECK(evaluated.body_solid != nullptr);
    const auto presented =
        kernel.bodyPresentation(evaluated.body_solid);
    CHECK(presented.ok());
    std::vector<WorldEdgeProbe> probes;
    for (const auto& edge : presented.body.edges) {
        const auto authored =
            part::authorMaterialEdgeReference(
                *evaluated.current_topology, edge.runtime_token);
        if (!authored.ok() || edge.points.size() < 2U) {
            continue;
        }
        const auto a = edge.points[edge.points.size() / 2U - 1U];
        const auto b = edge.points[edge.points.size() / 2U];
        const viewer::Point3 center{
            (a.x + b.x) / 2.0,
            (a.y + b.y) / 2.0,
            (a.z + b.z) / 2.0};
        probes.push_back({*authored.reference, center});
    }
    CHECK(probes.size() >= 4U);
    return probes;
}

std::vector<std::array<viewer::Point3, 3U>>
authorableTrihedralCornerProbes(
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto evaluated =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology);
    CHECK(evaluated.current_topology->complete());
    const auto presented =
        kernel.bodyPresentation(evaluated.body_solid);
    CHECK(presented.ok());
    const auto isSameTestPoint =
        [](const kernel::Point3& a,
           const kernel::Point3& b) {
            constexpr double epsilon = 1.0e-6;
            return std::abs(a.x - b.x) <= epsilon &&
                   std::abs(a.y - b.y) <= epsilon &&
                   std::abs(a.z - b.z) <= epsilon;
        };
    std::vector<std::array<viewer::Point3, 3U>> corners;
    for (const auto& vertex : presented.body.vertices) {
        std::vector<viewer::Point3> incident_edge_centers;
        for (const auto& edge : presented.body.edges) {
            if (edge.points.size() < 2U ||
                (!isSameTestPoint(
                    edge.points.front(), vertex.point) &&
                 !isSameTestPoint(
                    edge.points.back(), vertex.point))) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *evaluated.current_topology,
                    edge.runtime_token);
            if (!authored.ok()) {
                continue;
            }
            const auto& first = edge.points.front();
            const auto& last = edge.points.back();
            incident_edge_centers.push_back({
                (first.x + last.x) / 2.0,
                (first.y + last.y) / 2.0,
                (first.z + last.z) / 2.0});
        }
        if (incident_edge_centers.size() == 3U) {
            corners.push_back({
                incident_edge_centers[0],
                incident_edge_centers[1],
                incident_edge_centers[2]});
        }
    }
    CHECK(corners.size() == 8U);
    return corners;
}

QTreeWidgetItem* featureTreeItem(
    QTreeWidget& tree, part::FeatureId feature_id) {
    constexpr int feature_id_data = Qt::UserRole + 45;
    const auto id = QString::fromStdString(feature_id.serialized());
    std::vector<QTreeWidgetItem*> pending;
    for (int i = 0; i < tree.topLevelItemCount(); ++i) {
        pending.push_back(tree.topLevelItem(i));
    }
    while (!pending.empty()) {
        auto* item = pending.back();
        pending.pop_back();
        if (item == nullptr) continue;
        if (item->data(0, feature_id_data).toString() == id) {
            return item;
        }
        for (int i = 0; i < item->childCount(); ++i) {
            pending.push_back(item->child(i));
        }
    }
    return nullptr;
}

bool nativeClick(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    viewer::Point3 world) {
    const auto screen = viewport.projectWorldPoint(world);
    if (!screen || !std::isfinite(screen->x) ||
        !std::isfinite(screen->y)) {
        return false;
    }
    const QPoint pixel{
        static_cast<int>(std::lround(screen->x)),
        static_cast<int>(std::lround(screen->y))};
    if (!viewport.rect().contains(pixel)) {
        return false;
    }
    const auto query = viewport.queryBodyTopology(
        *screen,
        viewer::BodyTopologyPickFilter{false, true, false});
    if (!query.valid() || !query.completed ||
        query.candidates.empty()) {
        return false;
    }
    QTest::mouseMove(&viewport, pixel);
    QTest::mouseClick(
        &viewport, Qt::LeftButton, Qt::NoModifier, pixel);
    QApplication::processEvents();
    return true;
}

bool selectedCount(const QLabel& label, int count) {
    return label.text().contains(
        QStringLiteral("Selected edges: %1").arg(count));
}

// R2-C/E: query and CLICK actual OCCT-presented circular Edges of full
// and partial Revolve. The world point is only a test cursor fixture:
// authoring still requires the Workbench strict semantic Edge resolver.
bool clickAuthorableRevolveCircle(
    viewer_qt_occt::QtOcctViewerWidget& viewport,
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel,
    const QLabel& selection) {
    const auto evaluated =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology);
    CHECK(evaluated.current_topology->complete());
    CHECK(evaluated.body_solid != nullptr);
    const auto presented =
        kernel.bodyPresentation(evaluated.body_solid);
    CHECK(presented.ok());
    std::size_t semantic_circles = 0U;
    for (const auto& edge :
         evaluated.current_topology->edges) {
        if (edge.curve_kind !=
            kernel::CurveKind::circle) {
            continue;
        }
        const auto ref =
            part::authorMaterialEdgeReference(
                *evaluated.current_topology,
                edge.runtime_token);
        if (!ref.ok()) continue;
        ++semantic_circles;
        const auto path = std::find_if(
            presented.body.edges.begin(),
            presented.body.edges.end(),
            [&edge](const auto& item) {
                return item.runtime_token ==
                       edge.runtime_token;
            });
        if (path == presented.body.edges.end() ||
            path->points.size() < 2U) {
            continue;
        }
        constexpr std::size_t attempts = 24U;
        for (std::size_t index = 0U;
             index < attempts; ++index) {
            const auto segment =
                (index * (path->points.size() - 1U)) /
                attempts;
            const auto& a = path->points[segment];
            const auto& b = path->points[segment + 1U];
            const viewer::Point3 point{
                (a.x + b.x) / 2.0,
                (a.y + b.y) / 2.0,
                (a.z + b.z) / 2.0};
            if (!nativeClick(viewport, point)) {
                continue;
            }
            if (selectedCount(selection, 1)) {
                return true;
            }
        }
    }
    std::cout
        << "PM05F_R2_REVOLVE_CIRCLE_PICK_FAIL"
        << " semantic_circles=" << semantic_circles
        << '\n';
    CHECK(semantic_circles > 0U);
    return false;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    kernel_occt::OcctSolidModelingKernel kernel;
    viewer_qt_occt::QtOcctViewerWidget* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            viewport =
                new viewer_qt_occt::QtOcctViewerWidget(parent);
            return ui::ViewportSurface{viewport, viewport};
        },
        &kernel};
    CHECK(viewport != nullptr);
    auto session = makeBaseSession(kernel);
    CHECK(workbench.activateDocument(&session, {}));
    workbench.resize(1400, 900);
    workbench.show();
    int result = EXIT_FAILURE;

    QTimer::singleShot(150, &app, [&] {
        auto* tree = workbench.findChild<QTreeWidget*>();
        auto* finish = workbench.findChild<QPushButton*>(
            QStringLiteral("edgeFeatureFinishButton"));
        auto* cancel = workbench.findChild<QPushButton*>(
            QStringLiteral("edgeFeatureCancelButton"));
        auto* edit = workbench.findChild<QPushButton*>(
            QStringLiteral("featureEditExtrudeButton"));
        auto* label = workbench.findChild<QLabel*>(
            QStringLiteral("edgeFeatureSelectionLabel"));
        CHECK(tree && finish && cancel && edit && label);
        CHECK(viewport->isVisible());
        CHECK(viewport->setStandardView(
            viewer::StandardView::top_front_right));
        viewport->fitAll();
        QApplication::processEvents();
        const auto probes = authorableEdgeProbes(session, kernel);

        // Create the original Fillet through actual native cursor input,
        // not a fabricated provider-neutral PresentationToken intent.
        auto reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        reply = workbench.lockCadDynamicInputField(
            0U, "1", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        std::optional<WorldEdgeProbe> first;
        for (const auto& probe : probes) {
            if (!nativeClick(*viewport, probe.world)) {
                continue;
            }
            if (selectedCount(*label, 1) && finish->isEnabled()) {
                first = probe;
                break;
            }
            reply = workbench.submitCadInput(
                "CLEAR", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
        }
        CHECK(first.has_value());
        CHECK(finish->isEnabled());
        // R2-D: real Workbench preview stays local, transient and distinct
        // from the neutral displayed accepted Body.
        const auto creating_preview =
            viewport->runtimeDiagnostics();
        CHECK(creating_preview.solid_preview_displayed);
        CHECK(creating_preview.solid_preview_style_expected);
        CHECK(creating_preview.solid_committed_displayed);
        CHECK(creating_preview.solid_committed_style_expected);
        CHECK(creating_preview.solid_shading_styles_isolated);
        finish->click();
        QApplication::processEvents();
        CHECK(session.document().body().features.size() == 2U);
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        const auto feature_id =
            session.document().body().features.back().id;
        const auto* feature =
            session.document().findFeature(feature_id);
        CHECK(feature != nullptr);
        const auto* authored =
            std::get_if<part::FilletFeature>(&feature->definition);
        CHECK(authored != nullptr);
        CHECK(authored->edges.size() == 1U);
        const auto originally_authored = authored->edges.front();

        auto* item = featureTreeItem(*tree, feature_id);
        CHECK(item != nullptr);
        tree->clearSelection();
        tree->setCurrentItem(item);
        item->setSelected(true);
        QApplication::processEvents();
        CHECK(edit->isEnabled());
        const auto undo_before_edit = session.undoDepth();
        edit->click();
        QApplication::processEvents();
        CHECK(selectedCount(*label, 1));
        CHECK(viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(viewport->runtimeDiagnostics().solid_preview_style_expected);

        // This is the central R2-B acceptance boundary: real native click,
        // tool_stage Viewer's query, controller semantic selection and
        // Workbench draft state must all agree.
        CHECK(nativeClick(*viewport, first->world));
        CHECK(selectedCount(*label, 0));
        CHECK(!finish->isEnabled());
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(nativeClick(*viewport, first->world));
        CHECK(selectedCount(*label, 1));
        CHECK(finish->isEnabled());
        CHECK(viewport->runtimeDiagnostics().solid_preview_displayed);

        std::optional<WorldEdgeProbe> second;
        for (const auto& probe : probes) {
            if (probe.reference == originally_authored) continue;
            if (!nativeClick(*viewport, probe.world)) continue;
            if (selectedCount(*label, 2)) {
                second = probe;
                break;
            }
            if (selectedCount(*label, 0)) {
                CHECK(nativeClick(*viewport, first->world));
                CHECK(selectedCount(*label, 1));
            }
        }
        CHECK(second.has_value());
        CHECK(nativeClick(*viewport, second->world));
        CHECK(selectedCount(*label, 1));
        CHECK(finish->isEnabled());
        cancel->click();
        QApplication::processEvents();
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(session.undoDepth() == undo_before_edit);
        CHECK(session.document().body().features.size() == 2U);
        feature = session.document().findFeature(feature_id);
        CHECK(feature != nullptr);
        authored =
            std::get_if<part::FilletFeature>(&feature->definition);
        CHECK(authored != nullptr);
        CHECK(authored->edges.size() == 1U);
        CHECK(authored->edges.front() == originally_authored);

        item = featureTreeItem(*tree, feature_id);
        CHECK(item != nullptr);
        tree->clearSelection();
        tree->setCurrentItem(item);
        item->setSelected(true);
        QApplication::processEvents();
        CHECK(edit->isEnabled());
        edit->click();
        QApplication::processEvents();
        CHECK(selectedCount(*label, 1));
        reply = workbench.lockCadDynamicInputField(
            0U, "1.25", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        CHECK(finish->isEnabled());
        finish->click();
        QApplication::processEvents();
        CHECK(session.undoDepth() == undo_before_edit + 1U);
        CHECK(!viewport->runtimeDiagnostics().solid_preview_displayed);
        CHECK(session.document().findFeature(feature_id) != nullptr);
        const auto evaluation =
            part::evaluatePart(session.document(), kernel);
        CHECK(evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(evaluation.current_topology.has_value());
        CHECK(evaluation.current_topology->complete());

        // R2-C/E: real displayed OCCT analytic ring Edge, not a synthetic
        // scene, for full 360-degree and partial 90-degree Revolve Bodies.
        // Both operations consume the same semantic MaterialEdgeReference
        // contract. The preview is transient; Finish publishes the Body.
        // Retain both DocumentSession objects throughout the whole
        // Workbench switch sequence: the active document pointer must
        // never outlive a loop-local temporary during deactivation.
        auto full_revolve_session =
            makeRevolveSession(2.0 * std::acos(-1.0));
        auto partial_revolve_session =
            makeRevolveSession(std::acos(-1.0) / 2.0);
        for (int variant = 0; variant < 2; ++variant) {
            auto& revolve_session =
                variant == 0
                    ? full_revolve_session
                    : partial_revolve_session;
            const double angle =
                variant == 0
                    ? 2.0 * std::acos(-1.0)
                    : std::acos(-1.0) / 2.0;
            const char* command =
                variant == 0 ? "FILLET" : "CHAMFER";
            CHECK(workbench.activateDocument(
                &revolve_session, {}));
            QApplication::processEvents();
            CHECK(viewport->setStandardView(
                viewer::StandardView::top_front_right));
            viewport->fitAll();
            QApplication::processEvents();

            reply = workbench.submitCadInput(
                command,
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.lockCadDynamicInputField(
                0U, "0.75",
                workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(clickAuthorableRevolveCircle(
                *viewport, revolve_session, kernel, *label));
            CHECK(selectedCount(*label, 1));
            CHECK(finish->isEnabled());
            finish->click();
            QApplication::processEvents();
            CHECK(
                revolve_session.document().body()
                    .features.size() == 2U);
            const auto outcome =
                part::evaluatePart(
                    revolve_session.document(), kernel);
            CHECK(outcome.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(outcome.current_topology);
            CHECK(outcome.current_topology->complete());
            CHECK(!viewport->runtimeDiagnostics()
                       .solid_preview_displayed);
            CHECK(viewport->runtimeDiagnostics()
                      .solid_committed_displayed);
            std::cout
                << "PM05F_R2_NATIVE_REVOLVE_EDGE_PASS"
                << " angle=" << angle
                << " operation=" << command << '\n';
        }

        // R2-E: an actual OCCT circle sampled with the production
        // BodyPresentation path must remain selectable at high zoom.
        // The analytic midpoint is only a cursor-position test probe:
        // authoring always uses the strict Part semantic Curve catalog.
        auto precision_session =
            makeRevolveSession(2.0 * std::acos(-1.0));
        CHECK(workbench.activateDocument(
            &precision_session, {}));
        QApplication::processEvents();
        const auto precision_evaluation =
            part::evaluatePart(
                precision_session.document(), kernel);
        CHECK(precision_evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(precision_evaluation.current_topology);
        const auto precision_body =
            kernel.bodyPresentation(
                precision_evaluation.body_solid);
        CHECK(precision_body.ok());
        double max_screen_gap = 0.0;
        std::size_t testable_curved_segments = 0U;
        std::size_t missed_curved_segments = 0U;
        for (const auto& edge :
             precision_body.body.edges) {
            if (edge.points.size() < 3U) continue;
            const auto resolved =
                part::authorMaterialEdgeReference(
                    *precision_evaluation.current_topology,
                    edge.runtime_token);
            if (!resolved.ok()) continue;
            // The fixture is a circle about the world X-axis.
            // Only its front x=20 cap rim is guaranteed unobstructed.
            if (std::abs(edge.points.front().x - 20.0) >
                1.0e-5) {
                continue;
            }
            const auto& a = edge.points[0];
            const auto& b = edge.points[1];
            const double radius =
                std::hypot(a.y, a.z);
            if (radius < 9.0 ||
                std::abs(std::hypot(b.y, b.z) - radius) >
                    1.0e-4) {
                continue;
            }
            const double mid_y = a.y + b.y;
            const double mid_z = a.z + b.z;
            const double mid_len =
                std::hypot(mid_y, mid_z);
            if (mid_len <= 0.0) continue;
            const viewer::Point3 true_mid{
                a.x,
                mid_y * radius / mid_len,
                mid_z * radius / mid_len};
            // Verify both a tight view and a much tighter view against
            // the *production* OCCT edge path, not a densely hand-built
            // synthetic ring. Pixel-distance evidence never becomes CAD
            // identity; visible, authorable material Edges must hit.
            for (const double view_height : {8.0, 2.0}) {
                const viewer::CameraState close_view{
                    viewer::Point3{100.0, 0.0, 0.0},
                    true_mid,
                    viewer::Vec3{0.0, 0.0, 1.0},
                    viewer::CameraProjection::orthographic,
                    view_height};
                CHECK(viewport->setCameraState(close_view));
                QApplication::processEvents();
                const auto sa =
                    viewport->projectWorldPoint(
                        {a.x, a.y, a.z});
                const auto sb =
                    viewport->projectWorldPoint(
                        {b.x, b.y, b.z});
                const auto sm =
                    viewport->projectWorldPoint(true_mid);
                CHECK(sa && sb && sm);
                const double dx = sb->x - sa->x;
                const double dy = sb->y - sa->y;
                const double squared = dx * dx + dy * dy;
                CHECK(squared > 0.0);
                const double t = std::clamp(
                    ((sm->x - sa->x) * dx +
                     (sm->y - sa->y) * dy) / squared,
                    0.0, 1.0);
                const double screen_gap = std::hypot(
                    sm->x - sa->x - t * dx,
                    sm->y - sa->y - t * dy);
                max_screen_gap =
                    std::max(max_screen_gap, screen_gap);
                const auto q =
                    viewport->queryBodyTopology(
                        *sm,
                        viewer::BodyTopologyPickFilter{
                            false, true, false});
                CHECK(q.valid() && q.completed);
                CHECK(q.generation.valid());
                const auto expected =
                    std::any_of(
                        q.candidates.begin(),
                        q.candidates.end(),
                        [](const auto& candidate) {
                            return candidate.kind ==
                                viewer::BodyTopologyPresentationKind::edge;
                        });
                ++testable_curved_segments;
                if (!expected) {
                    ++missed_curved_segments;
                }
                std::cerr
                    << "PM05F_R2_HIGH_ZOOM_CURVE"
                    << " scale=" << view_height
                    << " deflection_gap_pixels=" << screen_gap
                    << " edge_found=" << expected
                    << " samples=" << edge.points.size()
                    << '\n';
            }
            break;
        }
        CHECK(testable_curved_segments > 0U);
        CHECK(missed_curved_segments == 0U);
        std::cout
            << "PM05F_R2_CURVE_PICK_HIGH_ZOOM"
            << " max_chord_gap_px=" << max_screen_gap
            << " tested=" << testable_curved_segments
            << '\n';

        // R2-A: native full click -> Workbench three converging Edge
        // selection -> Fillet Finish -> fresh Part evaluation and visible
        // committed Body. Coordinates here only choose mouse positions,
        // never persistent Edge identity.
        auto trihedral_session =
            makeBaseSession(kernel, 20.0, 20.0, 20.0);
        CHECK(workbench.activateDocument(
            &trihedral_session, {}));
        QApplication::processEvents();
        CHECK(viewport->setStandardView(
            viewer::StandardView::top_front_right));
        viewport->fitAll();
        QApplication::processEvents();
        const auto corners =
            authorableTrihedralCornerProbes(
                trihedral_session, kernel);
        reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);
        reply = workbench.lockCadDynamicInputField(
            0U, "2", workbench.cadInputContextGeneration());
        CHECK(reply.accepted);

        bool trihedral_selected = false;
        for (const auto& corner : corners) {
            reply = workbench.submitCadInput(
                "CLEAR", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            bool all_clicked = true;
            for (std::size_t index = 0U;
                 index < corner.size(); ++index) {
                if (!nativeClick(*viewport, corner[index]) ||
                    !selectedCount(
                        *label,
                        static_cast<int>(index + 1U))) {
                    all_clicked = false;
                    break;
                }
            }
            if (all_clicked && finish->isEnabled()) {
                trihedral_selected = true;
                break;
            }
        }
        CHECK(trihedral_selected);
        CHECK(selectedCount(*label, 3));
        CHECK(finish->isEnabled());
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        finish->click();
        QApplication::processEvents();
        CHECK(trihedral_session.document()
                  .body().features.size() == 2U);
        const auto after_three =
            part::evaluatePart(
                trihedral_session.document(), kernel);
        CHECK(after_three.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(after_three.current_topology);
        CHECK(after_three.current_topology->complete());
        CHECK(!viewport->runtimeDiagnostics()
                   .solid_preview_displayed);
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_style_expected);
        std::cout
            << "PM05F_R2_NATIVE_TRIHEDRAL_FILLET_PASS"
            << " body_present=1"
            << " edges=3"
            << " radius=2"
            << '\n';

        // Restore a known-live session before the temporary Revolve
        // fixtures are destroyed and the Workbench is closed.
        CHECK(workbench.activateDocument(&session, {}));
        QApplication::processEvents();

        std::cout
            << "PM05F_R2_NATIVE_WORKBENCH_EDIT_PASS"
            << " mouse_selection=1"
            << " tool_stage=1"
            << " toggle=1"
            << " add_remove=1"
            << " cancel_zero_mutation=1"
            << " edit_finish=1\n";
        result = EXIT_SUCCESS;
        workbench.close();
        app.quit();
    });
    app.exec();
    return result;
}
