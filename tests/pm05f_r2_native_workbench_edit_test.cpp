#include "cad_workbench.hpp"
#include "part_viewport_controller.hpp"


#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QTest>
#include <QTemporaryDir>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QTreeWidgetItemIterator>

#include <algorithm>
#include <atomic>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

std::atomic<std::uint32_t> owner_edge_outline_warnings{0U};

void captureOwnerEdgeOutlineWarning(
    QtMsgType type,
    const QMessageLogContext&,
    const QString& message) {
    if (type != QtWarningMsg ||
        !message.contains(
            QStringLiteral(
                "SS2 Viewer display-only Edge outline"))) {
        return;
    }
    owner_edge_outline_warnings.fetch_add(
        1U, std::memory_order_relaxed);
    std::cerr
        << "PM05F_R2_OWNER_OUTLINE_WARNING "
        << message.toStdString()
        << '\n';
}

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

bool nativePlanarFaceClick(
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
    if (!viewport.rect().contains(pixel)) return false;
    const auto query = viewport.queryBodyTopology(
        *screen,
        viewer::BodyTopologyPickFilter{true, false, false});
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

// PG-01D D0 native SS2 semantic-side characterization.
// The independent native OCCT wire/hole proof is recorded in work/.
// This test checks the *actual* current Part/OCCT catalog, not OCP:
// strict bounded Face identity is distinct from Surface-carrier admission,
// and only existing material Edge authoring can create accepted sources.
// The provider-facing wire membership query is intentionally NOT invented.
void verifyPg01dNativeStrictFaceAndMaterialCatalog(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto session = makeBaseSession(kernel);
    const auto hole_sketch =
        session.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(hole_sketch.ok() && hole_sketch.sketch_id);
    const auto circle = session.execute(
        application::AddSketchCircleCommand{
            *hole_sketch.sketch_id,
            {20.0, 15.0},
            5.0,
            sketch::EntityRole::regular});
    CHECK(circle.ok());
    const auto* authored =
        session.document().findSketch(*hole_sketch.sketch_id);
    CHECK(authored != nullptr);
    const auto regions = sketch::analyzeRegions(authored->model);
    CHECK(regions.complete() && regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *hole_sketch.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto through_cut = session.execute(
        application::CreateExtrudeFeatureCommand{
            *profile.profile_id,
            session.document().revision(),
            part::ExtrudeOperation::cut,
            part::OneSidedExtrudeExtent{
                core::LengthValue{25.0}, false},
            "PG01D native through-hole D0"},
        kernel);
    CHECK(through_cut.ok() && through_cut.feature_id);

    const auto evaluation =
        part::evaluatePart(session.document(), kernel);
    CHECK(evaluation.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.current_topology.has_value());
    const auto& catalog = *evaluation.current_topology;
    CHECK(catalog.complete());
    CHECK(catalog.stage.feature_id &&
          *catalog.stage.feature_id == *through_cut.feature_id);

    std::size_t strict_face_count = 0U;
    std::size_t carrier_only_face_count = 0U;
    std::size_t other_face_count = 0U;
    for (const auto& face : catalog.faces) {
        CHECK(face.valid());
        if (face.semantic_address) {
            ++strict_face_count;
            CHECK(!face.surface_candidates.empty());
        } else if (!face.surface_candidates.empty()) {
            // Sketch-support admission by Surface is NOT strict
            // bounded-Face Project Geometry authoring permission.
            ++carrier_only_face_count;
        } else {
            ++other_face_count;
        }
    }
    CHECK(!catalog.faces.empty());
    CHECK(strict_face_count > 0U);

    std::vector<part::MaterialEdgeReference>
        current_material_edges;
    std::size_t nonmaterial_edge_count = 0U;
    for (const auto& edge : catalog.edges) {
        CHECK(edge.valid());
        const auto authored_edge =
            part::authorMaterialEdgeReference(
                catalog, edge.runtime_token);
        if (!authored_edge.ok()) {
            ++nonmaterial_edge_count;
            continue;
        }
        CHECK(authored_edge.reference.has_value());
        CHECK(authored_edge.reference->stage == catalog.stage);
        current_material_edges.push_back(
            *authored_edge.reference);
    }
    CHECK(!current_material_edges.empty());
    std::sort(
        current_material_edges.begin(),
        current_material_edges.end());
    CHECK(std::adjacent_find(
        current_material_edges.begin(),
        current_material_edges.end()) ==
        current_material_edges.end());
    CHECK(strict_face_count + carrier_only_face_count +
              other_face_count == catalog.faces.size());
    CHECK(current_material_edges.size() +
              nonmaterial_edge_count == catalog.edges.size());

    // First true SS2 native provider/Part reconciliation. The selected
    // bounded Face remains strict; its native wire uses are only current
    // runtime tokens. Every admitted material use must pass the existing
    // PG-01B authorMaterialEdgeReference contract.
    std::size_t planar_faces = 0U;
    std::size_t nonplanar_faces = 0U;
    std::size_t nonplanar_native_wires = 0U;
    std::size_t nonplanar_material_members = 0U;
    std::size_t nonplanar_nonmaterial_members = 0U;
    std::size_t nonplanar_periodic_seam_edges = 0U;
    std::size_t nonplanar_periodic_seam_uses = 0U;
    std::size_t native_faces_with_holes = 0U;
    std::size_t strict_faces_with_holes = 0U;
    std::size_t fully_material_strict_holed_faces = 0U;
    std::size_t hole_rejected_edges = 0U;
    for (const auto& face : catalog.faces) {
        const auto source =
            kernel.bindFaceToBody(
                evaluation.body_solid, face.runtime_token);
        CHECK(source && source->valid());
        const auto boundary =
            kernel.queryFaceBoundary(
                evaluation.body_solid, *source);
        if (boundary.status ==
                kernel::FaceBoundaryStatus::
                    unsupported_surface) {
            ++nonplanar_faces;
            // Face Boundary E0: the original planar-only query must
            // continue to reject this native cylinder/curved Face.
            // The separate optional read is scoped to the same Body
            // and returns real oriented wires, not a projected Face.
            const auto raw =
                kernel.queryFaceBoundaryAnySurface(
                    evaluation.body_solid, *source);
            if (!raw.ok()) {
                continue;
            }
            nonplanar_native_wires += raw.wires.size();
            // Real OCCT cylinder's parameterization seam is not an
            // engineering material boundary even if the native Face
            // wire reports this Edge twice. Only the exact Part
            // catalog's typed periodic_seam bit is authoritative.
            std::map<std::uint64_t, std::vector<bool>>
                native_face_seam_uses;
            for (const auto& wire : raw.wires) {
                CHECK(wire.valid());
                for (const auto& use : wire.edges) {
                    const auto catalog_member = std::find_if(
                        catalog.edges.begin(),
                        catalog.edges.end(),
                        [&use](const auto& edge) {
                            return edge.runtime_token == use.edge;
                        });
                    CHECK(catalog_member != catalog.edges.end());
                    const auto strict =
                        part::authorMaterialEdgeReference(
                            catalog, use.edge);
                    if (catalog_member->periodic_seam) {
                        CHECK(!strict.ok());
                        CHECK(!catalog_member->representation_partition);
                        native_face_seam_uses[use.edge.value].push_back(
                            use.reversed);
                        ++nonplanar_periodic_seam_uses;
                    }
                    if (strict.ok()) {
                        CHECK(strict.reference);
                        ++nonplanar_material_members;
                    } else {
                        ++nonplanar_nonmaterial_members;
                    }
                }
            }
            for (const auto& [token, uses] :
                 native_face_seam_uses) {
                // Two opposite native wire uses of ONE exact seam Edge.
                // Distinct token values or one use cannot certify a seam.
                CHECK(token != 0U);
                CHECK(uses.size() == 2U);
                CHECK(uses.front() != uses.back());
                ++nonplanar_periodic_seam_edges;
            }
            continue;
        }
        CHECK(boundary.ok());
        ++planar_faces;
        if (boundary.wires.size() <= 1U) continue;
        ++native_faces_with_holes;
        std::size_t outer_wires = 0U;
        bool all_material = true;
        std::vector<part::MaterialEdgeReference>
            member_sources;
        for (const auto& wire : boundary.wires) {
            CHECK(wire.valid());
            if (wire.outer) ++outer_wires;
            for (const auto& member : wire.edges) {
                CHECK(member.valid());
                const auto authored_source =
                    part::authorMaterialEdgeReference(
                        catalog, member.edge);
                if (!authored_source.ok()) {
                    ++hole_rejected_edges;
                    all_material = false;
                    continue;
                }
                CHECK(authored_source.reference &&
                      authored_source.reference->stage ==
                          catalog.stage);
                member_sources.push_back(
                    *authored_source.reference);
            }
        }
        CHECK(outer_wires == 1U);
        std::sort(
            member_sources.begin(),
            member_sources.end());
        CHECK(std::adjacent_find(
            member_sources.begin(),
            member_sources.end()) ==
            member_sources.end());
        if (!face.semantic_address) {
            continue; // Surface carrier alone cannot admit PG-01D.
        }
        ++strict_faces_with_holes;
        if (all_material) {
            ++fully_material_strict_holed_faces;
        }
    }
    // Owner D2 manual multi-Face scope: native Face selection only,
    // not an inferred connected Surface region. The existing through-
    // Cut solid has both planar and cylindrical Face realizations.
    // Explicit shared MATERIAL Edge identity must survive selecting
    // two different Faces; a known cylinder seam must not be authored.
    std::vector<part::SelectedFaceBoundaryAdmission>
        individually_selected;
    std::size_t selected_planar = 0U;
    std::size_t selected_nonplanar = 0U;
    std::size_t selected_seams = 0U;
    std::size_t selected_material = 0U;
    for (const auto& face : catalog.faces) {
        const auto selected =
            part::inspectSelectedFaceBoundary(
                evaluation.features.back(),
                face.runtime_token, kernel);
        if (!selected.ok()) {
            std::cerr
                << "PG01D_MANUAL_FACE_E0_BLOCKED token="
                << face.runtime_token.value
                << " status=" << static_cast<int>(selected.status)
                << '\n';
            continue;
        }
        CHECK(selected.bounded_face == face.runtime_token);
        const auto scope =
            kernel.bindFaceToBody(
                evaluation.body_solid, face.runtime_token);
        CHECK(scope && scope->valid());
        const auto planar = kernel.queryFaceBoundary(
            evaluation.body_solid, *scope);
        if (planar.ok()) {
            ++selected_planar;
        } else {
            CHECK(planar.status ==
                  kernel::FaceBoundaryStatus::
                      unsupported_surface);
            ++selected_nonplanar;
        }
        std::vector<kernel::RuntimeEdgeToken> own_material;
        for (const auto& wire : selected.wires) {
            for (const auto& use : wire.edges) {
                CHECK(use.valid());
                CHECK(use.native_use.start_vertex);
                CHECK(use.native_use.end_vertex);
                if (use.excluded_nonmaterial) {
                    CHECK(!use.material);
                    const auto edge = std::find_if(
                        catalog.edges.begin(), catalog.edges.end(),
                        [&use](const auto& item) {
                            return item.runtime_token ==
                                use.native_use.edge;
                        });
                    CHECK(edge != catalog.edges.end());
                    CHECK(edge->periodic_seam ||
                          edge->representation_partition);
                    CHECK(edge->accounting_class ==
                          part::TopologyAccountingClass::
                              known_representation_artifact);
                    if (edge->periodic_seam) ++selected_seams;
                } else {
                    CHECK(use.material);
                    CHECK(use.material->stage == catalog.stage);
                    CHECK(std::find(
                        own_material.begin(), own_material.end(),
                        use.native_use.edge) ==
                        own_material.end());
                    own_material.push_back(use.native_use.edge);
                    ++selected_material;
                }
            }
        }
        individually_selected.push_back(selected);
    }
    CHECK(selected_planar >= 1U);
    CHECK(selected_nonplanar >= 1U);
    CHECK(selected_seams >= 2U);
    CHECK(selected_material >= 2U);
    std::size_t shared_material_pairs = 0U;
    for (std::size_t i = 0U;
         i < individually_selected.size(); ++i) {
        for (std::size_t j = i + 1U;
             j < individually_selected.size(); ++j) {
            for (const auto& left : individually_selected[i].wires) {
                for (const auto& a : left.edges) {
                    if (!a.material) continue;
                    for (const auto& right :
                         individually_selected[j].wires) {
                        for (const auto& b : right.edges) {
                            if (!b.material ||
                                *a.material != *b.material) {
                                continue;
                            }
                            CHECK(a.native_use.edge ==
                                  b.native_use.edge);
                            ++shared_material_pairs;
                        }
                    }
                }
            }
        }
    }
    CHECK(shared_material_pairs >= 1U);
    CHECK(!part::inspectSelectedFaceBoundary(
        evaluation.features.back(),
        kernel::RuntimeFaceToken{}, kernel).ok());
    // Exact current Body generation is provider-bound. Another
    // evaluation must never reuse a captured scoped Face to cross
    // provider generations on the strength of numeric token values.
    std::cout
        << "PG01D_MANUAL_MULTI_FACE_NATIVE_E0_PASS"
        << " planar_faces=" << selected_planar
        << " curved_faces=" << selected_nonplanar
        << " real_shared_material_pairs=" << shared_material_pairs
        << " excluded_seam_uses=" << selected_seams
        << " accepted_material_uses=" << selected_material
        << " inferred_carrier_region=0"
        << '\n';

    std::cerr
        << "PG01D_D0_NATIVE_FACE_WIRE_STATUS"
        << " planar=" << planar_faces
        << " nonplanar=" << nonplanar_faces
        << " nonplanar_wires=" << nonplanar_native_wires
        << " nonplanar_material_uses="
        << nonplanar_material_members
        << " nonplanar_nonmaterial_uses="
        << nonplanar_nonmaterial_members
        << " nonplanar_seam_edges="
        << nonplanar_periodic_seam_edges
        << " nonplanar_seam_uses="
        << nonplanar_periodic_seam_uses
        << " holes=" << native_faces_with_holes
        << " strict_holes=" << strict_faces_with_holes
        << " strict_holes_all_material="
        << fully_material_strict_holed_faces
        << " material_rejections=" << hole_rejected_edges
        << std::endl;
    CHECK(native_faces_with_holes >= 2U);
    CHECK(nonplanar_faces >= 1U);
    CHECK(nonplanar_native_wires >= 1U);
    CHECK(nonplanar_material_members >= 1U);
    CHECK(nonplanar_periodic_seam_edges >= 1U);
    CHECK(nonplanar_periodic_seam_uses ==
          2U * nonplanar_periodic_seam_edges);
    CHECK(strict_faces_with_holes > 0U);
    CHECK(fully_material_strict_holed_faces > 0U);

    // A token scoped to one provider realization must not gain authority
    // over a different body just because its numeric Face ID is reused.
    CHECK(!kernel.bindFaceToBody(
        evaluation.body_solid, kernel::RuntimeFaceToken{}));
    const auto invalid_scope =
        kernel.queryFaceBoundary(
            evaluation.body_solid,
            kernel::ScopedBoundaryFace{});
    CHECK(invalid_scope.status ==
          kernel::FaceBoundaryStatus::invalid_input);

    const auto fresh_evaluation =
        part::evaluatePart(session.document(), kernel);
    CHECK(fresh_evaluation.body_solid);
    CHECK(fresh_evaluation.body_solid.get() !=
          evaluation.body_solid.get());
    const auto source_face = catalog.faces.front().runtime_token;
    const auto stale_bound =
        kernel.bindFaceToBody(
            evaluation.body_solid, source_face);
    CHECK(stale_bound);
    const auto stale_query =
        kernel.queryFaceBoundary(
            fresh_evaluation.body_solid, *stale_bound);
    CHECK(stale_query.status ==
          kernel::FaceBoundaryStatus::provider_mismatch);

    std::cout << "PG01D_D0_NATIVE_STRICT_FACE_CATALOG_PASS"
              << " strict_faces=" << strict_face_count
              << " carrier_only_faces=" << carrier_only_face_count
              << " other_faces=" << other_face_count
              << " material_edges=" << current_material_edges.size()
              << " rejected_nonmaterial=" << nonmaterial_edge_count
              << " provider_face_wires_not_yet_claimed=1"
              << '\n';
}

// PG-01D D1: the actual SS2 Part-after-two-Cuts case must preserve an
// *individually bounded* planar Face with two real native inner wires.
// A Surface carrier alone never grants Face Project Geometry admission.
void verifyPg01dNativeTwoHoleFaceBoundary(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto session = makeBaseSession(kernel);
    const auto cut_circle = [&](
        double x, const char* label) {
        const auto sketch =
            session.execute(application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(sketch.ok() && sketch.sketch_id);
        const auto circle = session.execute(
            application::AddSketchCircleCommand{
                *sketch.sketch_id,
                {x, 15.0},
                4.0,
                sketch::EntityRole::regular});
        CHECK(circle.ok());
        const auto* authored =
            session.document().findSketch(*sketch.sketch_id);
        CHECK(authored);
        const auto regions =
            sketch::analyzeRegions(authored->model);
        CHECK(regions.complete() &&
              regions.regions.size() == 1U);
        const auto intent =
            part::makeProfileRegionIntent(regions.regions.front());
        CHECK(intent);
        const auto profile = session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
        CHECK(profile.ok() && profile.profile_id);
        const auto cut = session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{25.0}, false},
                label},
            kernel);
        CHECK(cut.ok() && cut.feature_id);
        return *cut.feature_id;
    };
    cut_circle(12.0, "PG01D First Through Cut");
    const auto last_cut_id =
        cut_circle(28.0, "PG01D Second Through Cut");

    const auto result =
        part::evaluatePart(session.document(), kernel);
    CHECK(result.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(result.body_solid && result.current_topology);
    const auto& catalog = *result.current_topology;
    CHECK(catalog.complete());

    std::size_t observed_two_hole_faces = 0U;
    std::size_t strict_two_hole_faces = 0U;
    std::size_t strict_two_hole_material_faces = 0U;
    std::size_t admitted_complete_face_batches = 0U;
    std::size_t excluded_periodic_seams = 0U;
    std::size_t excluded_partition_edges = 0U;
    for (const auto& edge : catalog.edges) {
        if (!edge.periodic_seam &&
            !edge.representation_partition) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog, edge.runtime_token);
        CHECK(!authored.ok());
        if (edge.periodic_seam) ++excluded_periodic_seams;
        if (edge.representation_partition) {
            ++excluded_partition_edges;
        }
    }

    for (const auto& face : catalog.faces) {
        const auto scoped =
            kernel.bindFaceToBody(
                result.body_solid, face.runtime_token);
        CHECK(scoped && scoped->valid());
        const auto boundary =
            kernel.queryFaceBoundary(result.body_solid, *scoped);
        if (boundary.status ==
                kernel::FaceBoundaryStatus::unsupported_surface) {
            continue;
        }
        CHECK(boundary.ok());
        if (boundary.wires.size() != 3U) continue;
        ++observed_two_hole_faces;
        std::size_t outers = 0U;
        bool all_material = true;
        std::vector<part::MaterialEdgeReference> sources;
        for (const auto& wire : boundary.wires) {
            if (wire.outer) ++outers;
            for (const auto& member : wire.edges) {
                const auto identity =
                    part::authorMaterialEdgeReference(
                        catalog, member.edge);
                if (!identity.ok()) {
                    all_material = false;
                    continue;
                }
                CHECK(identity.reference &&
                      identity.reference->stage ==
                          catalog.stage);
                sources.push_back(*identity.reference);
            }
        }
        CHECK(outers == 1U);
        std::sort(sources.begin(), sources.end());
        CHECK(std::adjacent_find(
            sources.begin(), sources.end()) == sources.end());
        const auto admitted =
            part::inspectMaterialFaceBoundary(
                result.features.back(),
                face.runtime_token, kernel);
        if (!face.semantic_address) {
            CHECK(!admitted.ok());
            CHECK(admitted.status ==
                  part::MaterialFaceBoundaryStatus::face_not_strict);
            continue;
        }
        ++strict_two_hole_faces;
        if (!all_material) {
            CHECK(!admitted.ok());
            CHECK(admitted.status ==
                  part::MaterialFaceBoundaryStatus::
                      material_edge_unavailable);
            continue;
        }
        ++strict_two_hole_material_faces;
        CHECK(admitted.ok());
        CHECK(admitted.bounded_face ==
              face.semantic_address);
        CHECK(admitted.wires.size() ==
              boundary.wires.size());
        std::size_t accepted_members = 0U;
        for (std::size_t wire_index = 0U;
             wire_index < admitted.wires.size();
             ++wire_index) {
            const auto& actual = admitted.wires[wire_index];
            const auto& source = boundary.wires[wire_index];
            CHECK(actual.outer == source.outer);
            CHECK(actual.edges.size() == source.edges.size());
            for (std::size_t edge_index = 0U;
                 edge_index < source.edges.size();
                 ++edge_index) {
                const auto& item = actual.edges[edge_index];
                CHECK(item.valid());
                CHECK(item.current_edge ==
                      source.edges[edge_index].edge);
                CHECK(item.reversed ==
                      source.edges[edge_index].reversed);
                CHECK(item.reference.stage == catalog.stage);
                ++accepted_members;
            }
        }
        CHECK(accepted_members == 6U);
        ++admitted_complete_face_batches;
    }
    std::cerr << "PG01D_D1_TWO_HOLE_FACE_STATUS"
              << " observed=" << observed_two_hole_faces
              << " strict=" << strict_two_hole_faces
              << " strict_material="
              << strict_two_hole_material_faces
              << " complete_face_admissions="
              << admitted_complete_face_batches
              << " excluded_seams="
              << excluded_periodic_seams
              << " excluded_partitions="
              << excluded_partition_edges << std::endl;
    CHECK(observed_two_hole_faces >= 2U);
    CHECK(strict_two_hole_faces > 0U);
    CHECK(strict_two_hole_material_faces > 0U);
    CHECK(admitted_complete_face_batches ==
          strict_two_hole_material_faces);

    // D1 lifecycle: a Face observed before an authored upstream change
    // cannot authorize a new read in a different provider generation.
    // Suppression must reject Face admission without a cached result;
    // unsuppression must rebuild a fresh, strictly mapped outer+holes.
    const auto old_scoped = kernel.bindFaceToBody(
        result.body_solid,
        catalog.faces.front().runtime_token);
    CHECK(old_scoped && old_scoped->valid());
    auto absent_catalog = result.features.back();
    absent_catalog.result_topology.reset();
    const auto invalid_catalog = part::inspectMaterialFaceBoundary(
        absent_catalog,
        catalog.faces.front().runtime_token, kernel);
    CHECK(!invalid_catalog.ok());
    CHECK(invalid_catalog.status ==
          part::MaterialFaceBoundaryStatus::invalid_stage);

    const auto suppressed = session.execute(
        application::SetFeatureSuppressedCommand{
            last_cut_id, session.document().revision(), true});
    CHECK(suppressed.ok());
    const auto suppressed_eval =
        part::evaluatePart(session.document(), kernel);
    CHECK(suppressed_eval.features.size() ==
          result.features.size());
    const auto blocked = part::inspectMaterialFaceBoundary(
        suppressed_eval.features.back(),
        catalog.faces.front().runtime_token, kernel);
    CHECK(!blocked.ok());
    CHECK(blocked.status ==
          part::MaterialFaceBoundaryStatus::invalid_stage);

    const auto unsuppressed = session.execute(
        application::SetFeatureSuppressedCommand{
            last_cut_id, session.document().revision(), false});
    CHECK(unsuppressed.ok());
    const auto rebuilt =
        part::evaluatePart(session.document(), kernel);
    CHECK(rebuilt.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(rebuilt.body_solid && rebuilt.current_topology);
    CHECK(rebuilt.current_topology->complete());
    CHECK(rebuilt.body_solid.get() != result.body_solid.get());
    const auto stale_native = kernel.queryFaceBoundary(
        rebuilt.body_solid, *old_scoped);
    CHECK(stale_native.status ==
          kernel::FaceBoundaryStatus::provider_mismatch);

    std::size_t rebuilt_two_hole_admissions = 0U;
    for (const auto& face : rebuilt.current_topology->faces) {
        const auto admitted = part::inspectMaterialFaceBoundary(
            rebuilt.features.back(), face.runtime_token, kernel);
        if (!admitted.ok() || admitted.wires.size() != 3U) {
            continue;
        }
        std::size_t outer_count = 0U;
        std::size_t material_members = 0U;
        for (const auto& wire : admitted.wires) {
            if (wire.outer) ++outer_count;
            for (const auto& member : wire.edges) {
                CHECK(member.valid());
                CHECK(member.reference.stage ==
                      rebuilt.current_topology->stage);
                ++material_members;
            }
        }
        CHECK(outer_count == 1U);
        CHECK(material_members == 6U);
        ++rebuilt_two_hole_admissions;
    }
    CHECK(rebuilt_two_hole_admissions > 0U);
    std::cout << "PG01D_D1_SUPPRESS_REBUILD_FACE_PASS"
              << " refreshed_strict_two_hole_faces="
              << rebuilt_two_hole_admissions << '\n';
}

// PG-01D D3 negative boundary seam: keep the real native Body, Face
// catalog, strict semantic mapping and OCCT provider as the baseline.
// Inject only a malformed/read-failing provider *answer* to prove Part
// does not accept a guessed or partially healed boundary. No production
// fault injection and no fabricated authored CAD identities.
class Pg01dFaultedBoundaryProvider final
    : public kernel::IFaceBoundaryQuery {
public:
    enum class Fault {
        none,
        bind_unavailable,
        query_failure,
        missing_outer,
        duplicate_outer,
        bogus_edge,
        duplicated_material_edge,
    };

    Pg01dFaultedBoundaryProvider(
        kernel_occt::OcctSolidModelingKernel& native, Fault fault)
        : native_{native}, fault_{fault} {}

    std::optional<kernel::ScopedBoundaryFace> bindFaceToBody(
        kernel::RuntimeSolidHandle body,
        kernel::RuntimeFaceToken face) noexcept override {
        if (fault_ == Fault::bind_unavailable ||
            !native_.bindFaceToBody(body, face)) {
            return std::nullopt;
        }
        return makeScopedFace(std::move(body), face);
    }

    kernel::FaceBoundaryResult queryFaceBoundary(
        kernel::RuntimeSolidHandle body,
        const kernel::ScopedBoundaryFace& scoped) noexcept override {
        using Status = kernel::FaceBoundaryStatus;
        if (!body || !scoped.valid() ||
            body.get() != scoped.sourceBody().get()) {
            return {Status::provider_mismatch, {}};
        }
        if (fault_ == Fault::query_failure) {
            return {Status::provider_failure, {}};
        }
        const auto real = native_.bindFaceToBody(
            body, scoped.face());
        if (!real) return {Status::face_unavailable, {}};
        auto result = native_.queryFaceBoundary(body, *real);
        if (!result.ok() || fault_ == Fault::none) {
            return result;
        }
        switch (fault_) {
        case Fault::missing_outer:
            for (auto& wire : result.wires) wire.outer = false;
            break;
        case Fault::duplicate_outer: {
            const auto outer = std::find_if(
                result.wires.begin(), result.wires.end(),
                [](const auto& wire) { return wire.outer; });
            if (outer != result.wires.end()) {
                result.wires.push_back(*outer);
            }
            break;
        }
        case Fault::bogus_edge:
            result.wires.front().edges.front().edge =
                kernel::RuntimeEdgeToken{
                    std::numeric_limits<std::uint64_t>::max()};
            break;
        case Fault::duplicated_material_edge:
            result.wires.front().edges.push_back(
                result.wires.front().edges.front());
            break;
        case Fault::none:
        case Fault::bind_unavailable:
        case Fault::query_failure:
            break;
        }
        return result;
    }

private:
    kernel_occt::OcctSolidModelingKernel& native_;
    Fault fault_;
};

void verifyPg01dMalformedBoundaryFailClosed(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto session = makeBaseSession(kernel);
    const auto before = session.document().state();
    const auto revision = session.document().revision();
    const auto undo = session.undoDepth();
    const auto result = part::evaluatePart(
        session.document(), kernel);
    CHECK(result.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(result.current_topology);
    CHECK(result.current_topology->complete());
    CHECK(!result.features.empty());
    const auto& stage = result.features.back();
    std::optional<kernel::RuntimeFaceToken> source_face;
    for (const auto& face : result.current_topology->faces) {
        const auto admitted = part::inspectMaterialFaceBoundary(
            stage, face.runtime_token, kernel);
        if (admitted.ok() && admitted.wires.size() == 1U &&
            admitted.wires.front().outer &&
            admitted.wires.front().edges.size() == 4U) {
            source_face = face.runtime_token;
            break;
        }
    }
    CHECK(source_face);
    const auto valid = part::inspectMaterialFaceBoundary(
        stage, *source_face, kernel);
    CHECK(valid.ok());
    CHECK(valid.wires.size() == 1U);
    CHECK(valid.wires.front().edges.size() == 4U);

    // No guessed Face when the source token or the entire stage is
    // unavailable, even though some planar Surface may still exist.
    const auto no_token = part::inspectMaterialFaceBoundary(
        stage, kernel::RuntimeFaceToken{}, kernel);
    CHECK(no_token.status ==
          part::MaterialFaceBoundaryStatus::face_unavailable);
    const auto alien_token = part::inspectMaterialFaceBoundary(
        stage,
        kernel::RuntimeFaceToken{
            std::numeric_limits<std::uint64_t>::max()},
        kernel);
    CHECK(alien_token.status ==
          part::MaterialFaceBoundaryStatus::face_unavailable);
    auto missing_catalog = stage;
    missing_catalog.result_topology.reset();
    CHECK(part::inspectMaterialFaceBoundary(
        missing_catalog, *source_face, kernel).status ==
        part::MaterialFaceBoundaryStatus::invalid_stage);
    auto carrier_only = stage;
    CHECK(carrier_only.result_topology);
    const auto face_record = std::find_if(
        carrier_only.result_topology->faces.begin(),
        carrier_only.result_topology->faces.end(),
        [&](const auto& item) {
            return item.runtime_token == *source_face;
        });
    CHECK(face_record != carrier_only.result_topology->faces.end());
    face_record->semantic_address.reset();
    CHECK(part::inspectMaterialFaceBoundary(
        carrier_only, *source_face, kernel).status ==
        part::MaterialFaceBoundaryStatus::face_not_strict);

    using Fault = Pg01dFaultedBoundaryProvider::Fault;
    const auto expect_failure = [&](Fault fault,
                                    part::MaterialFaceBoundaryStatus expected) {
        Pg01dFaultedBoundaryProvider provider{kernel, fault};
        const auto admitted = part::inspectMaterialFaceBoundary(
            stage, *source_face, provider);
        CHECK(!admitted.ok());
        CHECK(admitted.status == expected);
        CHECK(admitted.wires.empty());
        CHECK(!admitted.bounded_face);
    };
    expect_failure(
        Fault::bind_unavailable,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::query_failure,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::missing_outer,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::duplicate_outer,
        part::MaterialFaceBoundaryStatus::native_boundary_unavailable);
    expect_failure(
        Fault::bogus_edge,
        part::MaterialFaceBoundaryStatus::material_edge_unavailable);
    expect_failure(
        Fault::duplicated_material_edge,
        part::MaterialFaceBoundaryStatus::material_edge_unavailable);

    Pg01dFaultedBoundaryProvider passthrough{
        kernel, Fault::none};
    const auto control = part::inspectMaterialFaceBoundary(
        stage, *source_face, passthrough);
    CHECK(control.ok());
    CHECK(control.wires.size() == valid.wires.size());
    CHECK(control.wires.front().edges.size() ==
          valid.wires.front().edges.size());
    for (std::size_t i = 0U; i < valid.wires.front().edges.size(); ++i) {
        CHECK(control.wires.front().edges[i].reference ==
              valid.wires.front().edges[i].reference);
    }
    CHECK(session.document().state() == before);
    CHECK(session.document().revision() == revision);
    CHECK(session.undoDepth() == undo);
    std::cout << "PG01D_D3_MALFORMED_BOUNDARY_FAIL_CLOSED_PASS"
              << " native_strict_source=1"
              << " provider_fault_variants=6"
              << " invalid_scene_or_carrier_variants=4"
              << " duplicate_material_rejected=1"
              << " document_unchanged=1\\n";
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    const bool pg01c_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01c";
    const bool pg01d_ui_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01d-ui";
    const bool pg01d_d0_only =
        argc == 2 && std::string_view{argv[1]} == "--pg01d-d0";
    const bool pg01d_negative_only =
        argc == 2 &&
        std::string_view{argv[1]} == "--pg01d-negative";
    kernel_occt::OcctSolidModelingKernel kernel;
    if (pg01d_negative_only) {
        verifyPg01dMalformedBoundaryFailClosed(kernel);
        return EXIT_SUCCESS;
    }
    if (pg01d_d0_only) {
        // PG-01D's real OCCT/Part topology proof must remain isolated
        // from the unrelated, cursor/timing-sensitive PG-01C GUI test.
        // Reuse this compiled native test target without Viewer actions.
        verifyPg01dNativeStrictFaceAndMaterialCatalog(kernel);
        verifyPg01dNativeTwoHoleFaceBoundary(kernel);
        std::cout << "PG01D_D0_D1_ISOLATED_NATIVE_PASS"
                  << std::endl;
        return EXIT_SUCCESS;
    }
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
        if (pg01d_ui_only) {
            // Real Qt/OCCT Face cursor admission, never a fabricated
            // Viewer token or a provider ordinal. The target is a
            // separate XY Sketch after the legal upstream Extrude.
            auto face_session = makeBaseSession(kernel);
            const auto target_sketch =
                face_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(target_sketch.ok() && target_sketch.sketch_id);
            CHECK(workbench.activateDocument(&face_session, {}));
            QApplication::processEvents();
            auto* sketch_edit = workbench.findChild<QAction*>(
                QStringLiteral("editSketchAction"));
            auto* face_mode = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeSourceFaceButton"));
            auto* edge_mode = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeSourceEdgesButton"));
            auto* pg_finish = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeFinishButton"));
            auto* pg_remove = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeRemoveButton"));
            auto* pg_clear = workbench.findChild<QPushButton*>(
                QStringLiteral("projectEdgeClearButton"));
            auto* pg_count = workbench.findChild<QLabel*>(
                QStringLiteral("projectEdgeSelectionLabel"));
            auto* pg_face_detail = workbench.findChild<QLabel*>(
                QStringLiteral("projectEdgeResultLabel"));
            ui::PartViewportController* controller = nullptr;
            for (auto* child : workbench.children()) {
                if (auto* found =
                        dynamic_cast<ui::PartViewportController*>(child)) {
                    controller = found;
                    break;
                }
            }
            CHECK(sketch_edit && face_mode && edge_mode &&
                  pg_finish && pg_remove && pg_clear && pg_count &&
                  pg_face_detail && controller);
            QTreeWidgetItem* sketch_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    sketch_item = *it;
                    break;
                }
            }
            CHECK(sketch_item);
            tree->clearSelection();
            tree->setCurrentItem(sketch_item);
            sketch_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();

            const auto before_state = face_session.document().state();
            const auto before_revision =
                face_session.document().revision();
            const auto before_undo = face_session.undoDepth();
            auto reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(face_mode->isChecked());
            CHECK(!edge_mode->isChecked());
            CHECK(!pg_finish->isEnabled());

            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto admission =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(admission.ok());
            CHECK(admission.wires.size() == 1U);
            CHECK(admission.wires.front().outer);
            CHECK(admission.wires.front().edges.size() == 4U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Outer Edge 1: supported")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 4U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(pg_finish->isEnabled());
            CHECK(face_session.document().state() == before_state);
            CHECK(face_session.undoDepth() == before_undo);

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(face_session.undoDepth() == before_undo + 1U);
            CHECK(face_session.document().revision() !=
                  before_revision);
            const auto* projected =
                face_session.document().findSketch(
                    *target_sketch.sketch_id);
            CHECK(projected);
            CHECK(projected->projection_bindings.size() == 4U);
            CHECK(face_session.undo().changed);
            CHECK(face_session.document()
                      .findSketch(*target_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(face_session.redo().changed);
            CHECK(face_session.document()
                      .findSketch(*target_sketch.sketch_id)
                      ->projection_bindings.size() == 4U);

            std::cout << "PG01D_D2_NATIVE_FACE_FINISH_PASS"
                      << " outer_lines=4"
                      << " linked_entities=4"
                      << " one_undo=1"
                      << " undo_redo=1\n";

            // D3 fail-closed: a real previously acquired Face cannot be
            // finished after its source Feature is suppressed. Undo/Redo
            // above advanced document revision through the headless
            // session; rebind the live Workbench scene to that revision
            // *before* acquiring a genuine native Face anew.
            CHECK(workbench.activateDocument(&face_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* fresh_sketch_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) == QStringLiteral("Sketch 2")) {
                    fresh_sketch_item = *it;
                    break;
                }
            }
            CHECK(fresh_sketch_item);
            tree->clearSelection();
            tree->setCurrentItem(fresh_sketch_item);
            fresh_sketch_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 4U);
            const auto prior_link_count =
                face_session.document().findSketch(
                    *target_sketch.sketch_id)
                    ->projection_bindings.size();
            const auto base_id =
                face_session.document().body().features.front().id;
            CHECK(face_session.execute(
                application::SetFeatureSuppressedCommand{
                    base_id, face_session.document().revision(),
                    true}).ok());
            const auto suppressed_revision =
                face_session.document().revision();
            const auto suppressed_undo =
                face_session.undoDepth();
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(!reply.accepted);
            CHECK(face_session.document().revision() ==
                  suppressed_revision);
            CHECK(face_session.undoDepth() == suppressed_undo);
            CHECK(face_session.document().findSketch(
                *target_sketch.sketch_id)
                ->projection_bindings.size() == prior_link_count);
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            const auto suppressed_body = part::evaluatePart(
                face_session.document(), kernel);
            const auto unresolved_link =
                part::evaluateEffectiveSketchProjection(
                    face_session.document(),
                    *target_sketch.sketch_id,
                    suppressed_body, kernel);
            CHECK(!unresolved_link ||
                  !unresolved_link->allResolved());
            CHECK(face_session.execute(
                application::SetFeatureSuppressedCommand{
                    base_id, face_session.document().revision(),
                    false}).ok());
            const auto restored_body = part::evaluatePart(
                face_session.document(), kernel);
            const auto restored_links =
                part::evaluateEffectiveSketchProjection(
                    face_session.document(),
                    *target_sketch.sketch_id,
                    restored_body, kernel);
            CHECK(restored_links);
            CHECK(restored_links->allResolved());
            CHECK(restored_links->outcomes.size() == 4U);
            std::cout << "PG01D_D3_STALE_SUPPRESSED_FACE_PASS"
                      << " stale_finish_rejected=1"
                      << " linked_state_preserved=1"
                      << " overlay_cleared=1"
                      << " suppression_recovery=1\n";

            // D3 native Face-with-two-holes: two *real* SS2 Cut features,
            // not a fabricated wire or a painted/tessellated perimeter.
            // The top cap has four material outer Lines and two distinct
            // one-Circle hole wires, all scoped to one legal prior stage.
            auto holed_session = makeBaseSession(kernel);
            const auto make_hole = [&](double x, const char* label) {
                const auto hole_sketch = holed_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
                CHECK(hole_sketch.ok() && hole_sketch.sketch_id);
                CHECK(holed_session.execute(
                    application::AddSketchCircleCommand{
                        *hole_sketch.sketch_id,
                        {x, 15.0}, 4.0,
                        sketch::EntityRole::regular}).ok());
                const auto* authored = holed_session.document()
                    .findSketch(*hole_sketch.sketch_id);
                CHECK(authored);
                const auto regions =
                    sketch::analyzeRegions(authored->model);
                CHECK(regions.complete() &&
                      regions.regions.size() == 1U);
                const auto intent =
                    part::makeProfileRegionIntent(
                        regions.regions.front());
                CHECK(intent);
                const auto profile = holed_session.execute(
                    application::CreateProfileCommand{
                        *hole_sketch.sketch_id,
                        holed_session.document().revision(),
                        *intent});
                CHECK(profile.ok() && profile.profile_id);
                const auto cut = holed_session.execute(
                    application::CreateExtrudeFeatureCommand{
                        *profile.profile_id,
                        holed_session.document().revision(),
                        part::ExtrudeOperation::cut,
                        part::OneSidedExtrudeExtent{
                            core::LengthValue{25.0}, false},
                        label},
                    kernel);
                CHECK(cut.ok() && cut.feature_id);
            };
            make_hole(12.0, "PG01D D3 native hole 1");
            make_hole(28.0, "PG01D D3 native hole 2");
            const auto holed_target = holed_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
            CHECK(holed_target.ok() && holed_target.sketch_id);
            CHECK(workbench.activateDocument(&holed_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* holed_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 4")) {
                    holed_tree_item = *it;
                    break;
                }
            }
            CHECK(holed_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(holed_tree_item);
            holed_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto holed_state = holed_session.document().state();
            const auto holed_revision =
                holed_session.document().revision();
            const auto holed_undo = holed_session.undoDepth();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);

            // Mixed acquisition must deduplicate an independently picked
            // native material cap Edge against that same Edge expanded by
            // the later Face gesture. The first pick is an actual Viewer
            // Edge click, not an injected material reference.
            bool mixed_manual_picked = false;
            const std::array<viewer::Point3, 4> cap_midpoints{{
                {20.0, 0.0, 20.0},
                {20.0, 30.0, 20.0},
                {0.0, 15.0, 20.0},
                {40.0, 15.0, 20.0}
            }};
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::top}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                for (const auto& midpoint : cap_midpoints) {
                    if (!nativeClick(*viewport, midpoint)) {
                        continue;
                    }
                    if (pg_count->text().contains(
                            QStringLiteral("selected: 1"))) {
                        mixed_manual_picked = true;
                        break;
                    }
                    reply = workbench.submitCadInput(
                        "CLEAR", workbench.cadInputContextGeneration());
                    CHECK(reply.accepted);
                }
                if (mixed_manual_picked) break;
            }
            CHECK(mixed_manual_picked);
            const auto manual_source =
                controller->selectedMaterialEdgeReferences();
            CHECK(manual_source && manual_source->size() == 1U);
            CHECK(holed_session.document().state() == holed_state);
            CHECK(holed_session.undoDepth() == holed_undo);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto two_hole_admitted =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(two_hole_admitted.ok());
            CHECK(two_hole_admitted.wires.size() == 3U);
            std::size_t outer_count = 0U;
            std::size_t inner_count = 0U;
            std::size_t material_members = 0U;
            std::vector<part::MaterialEdgeReference>
                exact_sources;
            for (const auto& wire : two_hole_admitted.wires) {
                if (wire.outer) {
                    ++outer_count;
                    CHECK(wire.edges.size() == 4U);
                } else {
                    ++inner_count;
                    CHECK(wire.edges.size() == 1U);
                }
                for (const auto& member : wire.edges) {
                    CHECK(member.valid());
                    exact_sources.push_back(member.reference);
                    ++material_members;
                }
            }
            std::sort(exact_sources.begin(), exact_sources.end());
            CHECK(std::adjacent_find(
                exact_sources.begin(), exact_sources.end()) ==
                exact_sources.end());
            CHECK(outer_count == 1U);
            CHECK(inner_count == 2U);
            CHECK(material_members == 6U);
            CHECK(std::binary_search(
                exact_sources.begin(), exact_sources.end(),
                manual_source->front()));
            // A duplicate Edge picked manually and through Face is still
            // one linked source and will produce only one target EntityId.
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 6")));
            CHECK(pg_count->text().contains(
                QStringLiteral("holes 2")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Outer Edge 1: supported")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Hole 1 Edge 1: supported")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Hole 2 Edge 1: supported")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 6U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(pg_finish->isEnabled());
            CHECK(holed_session.document().state() == holed_state);
            CHECK(holed_session.document().revision() == holed_revision);
            CHECK(holed_session.undoDepth() == holed_undo);

            // Face REMOVE must discard just the transient Face gesture
            // while retaining the independently acquired manual Edge.
            CHECK(pg_remove->isEnabled());
            reply = workbench.submitCadInput(
                "REMOVE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 1")));
            CHECK(!pg_face_detail->text().contains(
                QStringLiteral("Hole 1 Edge 1")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(holed_session.document().state() == holed_state);
            CHECK(holed_session.undoDepth() == holed_undo);
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 6")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("Hole 2 Edge 1: supported")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 6U);
            CHECK(pg_finish->isEnabled());

            // Return to manual Edge selection before Finish. The existing
            // Face gesture must stay authoritative, and the Viewer must
            // restore all six generation-bound Edge picks from semantic
            // references, not leak the old Face presentation token.
            reply = workbench.submitCadInput(
                "EDGES", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(edge_mode->isChecked());
            CHECK(!face_mode->isChecked());
            const auto restored_sources =
                controller->selectedMaterialEdgeReferences();
            CHECK(restored_sources);
            CHECK(*restored_sources == exact_sources);
            CHECK(pg_finish->isEnabled());

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(holed_session.undoDepth() == holed_undo + 1U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            const auto* holed_linked = holed_session.document()
                .findSketch(*holed_target.sketch_id);
            CHECK(holed_linked);
            CHECK(holed_linked->projection_bindings.size() == 6U);
            std::vector<part::MaterialEdgeReference> saved_sources;
            for (const auto& linked :
                 holed_linked->projection_bindings) {
                saved_sources.push_back(linked.source);
            }
            std::sort(
                saved_sources.begin(), saved_sources.end());
            CHECK(saved_sources == exact_sources);
            CHECK(holed_session.undo().changed);
            CHECK(holed_session.document()
                .findSketch(*holed_target.sketch_id)
                ->projection_bindings.empty());
            CHECK(holed_session.redo().changed);
            CHECK(holed_session.document()
                .findSketch(*holed_target.sketch_id)
                ->projection_bindings.size() == 6U);

            // Cold OCCT recovery must not depend on the selected Face token,
            // its wire count, or its original provider generation.
            QTemporaryDir holed_store_dir;
            CHECK(holed_store_dir.isValid());
            const std::filesystem::path holed_path =
                std::filesystem::path{
                    holed_store_dir.path().toStdWString()} /
                "PG01DTwoHoleFace.ss2part";
            const part::PartDocumentStore store;
            CHECK(store.createNew(
                holed_path, holed_session.document()).ok());
            const auto loaded = store.load(holed_path);
            CHECK(loaded.ok());
            CHECK(loaded.document->state() ==
                  holed_session.document().state());
            kernel_occt::OcctSolidModelingKernel cold_kernel;
            const auto cold_body = part::evaluatePart(
                *loaded.document, cold_kernel);
            CHECK(cold_body.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            const auto cold_projection =
                part::evaluateEffectiveSketchProjection(
                    *loaded.document, *holed_target.sketch_id,
                    cold_body, cold_kernel);
            CHECK(cold_projection);
            CHECK(cold_projection->allResolved());
            CHECK(cold_projection->outcomes.size() == 6U);
            CHECK(cold_projection->model.entityCount() == 6U);
            CHECK(cold_projection->model.state().lines.size() == 4U);
            CHECK(cold_projection->model.state().circles.size() == 2U);
            const auto cold_regions =
                sketch::analyzeRegions(cold_projection->model);
            CHECK(cold_regions.complete());
            bool full_face_region = false;
            for (const auto& region : cold_regions.regions) {
                if (region.holes.size() == 2U) {
                    full_face_region = true;
                }
            }
            CHECK(full_face_region);
            std::cout << "PG01D_D3_NATIVE_TWO_HOLE_COLD_PASS"
                      << " outer=4"
                      << " inner=2"
                      << " linked=6"
                      << " distinct_semantic_sources=6"
                      << " mixed_manual_face_dedup=1"
                      << " restored_edge_picks=6"
                      << " one_undo=1"
                      << " cold_v15=1"
                      << " region_holes=2\n";


            // PG-01D D3 positive Partial: exact native Circle cannot be
            // projected as a Circle/Arc onto a perpendicular YZ Sketch,
            // but the four *skewed* planar perimeter Lines project
            // without degeneracy. This is genuine OCCT geometry and a
            // genuine strict material Face, not a mocked projection.
            auto partial_session = makeBaseSession(kernel);
            const auto partial_base_state =
                partial_session.document().state();
            const auto& partial_base =
                partial_base_state.sketches.front();
            const double skew_cos = std::cos(0.31);
            const double skew_sin = std::sin(0.31);
            const auto skew = [skew_cos, skew_sin](
                                  sketch::Point2 p) {
                return sketch::Point2{
                    p.u * skew_cos - p.v * skew_sin,
                    p.u * skew_sin + p.v * skew_cos};
            };
            std::vector<application::SketchLineGeometryUpdate>
                partial_updates;
            for (const auto& source :
                 partial_base.model.state().lines) {
                partial_updates.push_back({
                    source.id, skew(source.start),
                    skew(source.end)});
            }
            CHECK(partial_updates.size() == 4U);
            CHECK(partial_session.execute(
                application::UpdateSketchLinesCommand{
                    partial_base.id,
                    partial_session.document().revision(),
                    std::move(partial_updates)}).ok());
            const auto partial_hole = partial_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
            CHECK(partial_hole.ok() && partial_hole.sketch_id);
            CHECK(partial_session.execute(
                application::AddSketchCircleCommand{
                    *partial_hole.sketch_id,
                    {20.0, 15.0}, 3.0,
                    sketch::EntityRole::regular}).ok());
            const auto* partial_hole_model =
                partial_session.document().findSketch(
                    *partial_hole.sketch_id);
            CHECK(partial_hole_model);
            const auto partial_hole_regions =
                sketch::analyzeRegions(
                    partial_hole_model->model);
            CHECK(partial_hole_regions.complete());
            CHECK(partial_hole_regions.regions.size() == 1U);
            const auto partial_hole_intent =
                part::makeProfileRegionIntent(
                    partial_hole_regions.regions.front());
            CHECK(partial_hole_intent);
            const auto partial_profile =
                partial_session.execute(
                    application::CreateProfileCommand{
                        *partial_hole.sketch_id,
                        partial_session.document().revision(),
                        *partial_hole_intent});
            CHECK(partial_profile.ok() &&
                  partial_profile.profile_id);
            const auto partial_cut = partial_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *partial_profile.profile_id,
                    partial_session.document().revision(),
                    part::ExtrudeOperation::cut,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{25.0}, false},
                    "PG01D Partial native circular cut"},
                kernel);
            CHECK(partial_cut.ok() && partial_cut.feature_id);
            const auto partial_target = partial_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::yz_plane});
            CHECK(partial_target.ok() && partial_target.sketch_id);
            const auto partial_eval = part::evaluatePart(
                partial_session.document(), kernel);
            CHECK(partial_eval.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(partial_eval.current_topology);
            const auto partial_frame =
                part::resolveCurrentProjectionSketchFrame(
                    partial_session.document(),
                    *partial_target.sketch_id, partial_eval);
            CHECK(partial_frame);
            std::size_t partial_native_faces = 0U;
            for (const auto& face :
                 partial_eval.current_topology->faces) {
                const auto admitted =
                    part::inspectMaterialFaceBoundary(
                        partial_eval.features.back(),
                        face.runtime_token, kernel);
                if (!admitted.ok() ||
                    admitted.wires.size() != 2U) {
                    continue;
                }
                std::size_t supported = 0U;
                std::size_t unsupported = 0U;
                for (const auto& wire : admitted.wires) {
                    for (const auto& member : wire.edges) {
                        const auto projected =
                            part::projectStrictMaterialEdge(
                                partial_session.document(),
                                member.reference, partial_eval,
                                kernel, *partial_frame);
                        if (projected.status ==
                                part::ProjectedSketchSourceStatus::
                                    resolved) {
                            ++supported;
                        } else if (projected.status ==
                                   part::ProjectedSketchSourceStatus::
                                       unsupported_projection) {
                            ++unsupported;
                        } else {
                            CHECK(false);
                        }
                    }
                }
                if (supported == 4U && unsupported == 1U) {
                    ++partial_native_faces;
                }
            }
            CHECK(partial_native_faces > 0U);
            std::cout << "PG01D_D3_NATIVE_GEOMETRIC_PARTIAL_PROOF"
                      << " strict_face=1"
                      << " supported_lines=4"
                      << " unsupported_circle=1\\n";

            CHECK(workbench.activateDocument(
                &partial_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* partial_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 3")) {
                    partial_tree_item = *it;
                    break;
                }
            }
            CHECK(partial_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(partial_tree_item);
            partial_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto partial_original =
                partial_session.document().state();
            const auto partial_rev =
                partial_session.document().revision();
            const auto partial_undo =
                partial_session.undoDepth();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            const auto click_point = skew({20.0, 15.0});
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{
                    click_point.u, click_point.v, 20.0}));
            const auto partial_admission =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(partial_admission.ok());
            CHECK(partial_admission.wires.size() == 2U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 4")));
            CHECK(pg_count->text().contains(
                QStringLiteral("holes 1")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("SKIPPED")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 4U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 1U);
            CHECK(pg_finish->isEnabled());
            CHECK(partial_session.document().state() ==
                  partial_original);
            CHECK(partial_session.document().revision() ==
                  partial_rev);
            CHECK(partial_session.undoDepth() == partial_undo);

            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(partial_session.undoDepth() == partial_undo + 1U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            const auto* partial_linked =
                partial_session.document().findSketch(
                    *partial_target.sketch_id);
            CHECK(partial_linked);
            CHECK(partial_linked->projection_bindings.size() == 4U);
            const auto partial_after = part::evaluatePart(
                partial_session.document(), kernel);
            const auto partial_projection =
                part::evaluateEffectiveSketchProjection(
                    partial_session.document(),
                    *partial_target.sketch_id,
                    partial_after, kernel);
            CHECK(partial_projection);
            CHECK(partial_projection->allResolved());
            CHECK(partial_projection->outcomes.size() == 4U);
            CHECK(partial_projection->model.state().lines.size() == 4U);
            CHECK(partial_projection->model.state().circles.empty());
            CHECK(sketch::analyzeRegions(
                partial_projection->model).regions.empty());
            CHECK(partial_session.undo().changed);
            CHECK(partial_session.document().findSketch(
                *partial_target.sketch_id)
                ->projection_bindings.empty());
            CHECK(partial_session.redo().changed);
            CHECK(partial_session.document().findSketch(
                *partial_target.sketch_id)
                ->projection_bindings.size() == 4U);
            std::cout << "PG01D_D3_NATIVE_PARTIAL_FINISH_PASS"
                      << " cyan_ais=4 red_ais=1"
                      << " geometric_skipped=1"
                      << " linked=4"
                      << " open_no_profile=1"
                      << " one_undo=1\\n";

            // D3 all-Unsupported gate: one native circle-only planar cap
            // projects obliquely to YZ. Strict Face admission succeeds,
            // but no representable Edge remains: Finish must stay disabled
            // and even a typed FINISH must not author a partial empty batch.
            auto disk_document = part::PartDocument::create(
                core::DocumentId::generate());
            application::DocumentSession disk_session{
                {}, std::move(disk_document)};
            const auto disk_source = disk_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
            CHECK(disk_source.ok() && disk_source.sketch_id);
            CHECK(disk_session.execute(
                application::AddSketchCircleCommand{
                    *disk_source.sketch_id,
                    {20.0, 15.0}, 9.0,
                    sketch::EntityRole::regular}).ok());
            const auto* disk_source_sketch =
                disk_session.document().findSketch(
                    *disk_source.sketch_id);
            CHECK(disk_source_sketch);
            const auto disk_regions =
                sketch::analyzeRegions(disk_source_sketch->model);
            CHECK(disk_regions.complete() &&
                  disk_regions.regions.size() == 1U);
            const auto disk_intent =
                part::makeProfileRegionIntent(
                    disk_regions.regions.front());
            CHECK(disk_intent);
            const auto disk_profile = disk_session.execute(
                application::CreateProfileCommand{
                    *disk_source.sketch_id,
                    disk_session.document().revision(),
                    *disk_intent});
            CHECK(disk_profile.ok() && disk_profile.profile_id);
            const auto disk_extrude = disk_session.execute(
                application::CreateExtrudeFeatureCommand{
                    *disk_profile.profile_id,
                    disk_session.document().revision(),
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{20.0}, false},
                    "PG01D circular cap source"},
                kernel);
            CHECK(disk_extrude.ok() && disk_extrude.feature_id);
            const auto disk_target = disk_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::yz_plane});
            CHECK(disk_target.ok() && disk_target.sketch_id);
            CHECK(workbench.activateDocument(&disk_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* disk_tree_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    disk_tree_item = *it;
                    break;
                }
            }
            CHECK(disk_tree_item);
            tree->clearSelection();
            tree->setCurrentItem(disk_tree_item);
            disk_tree_item->setSelected(true);
            sketch_edit->trigger();
            QApplication::processEvents();
            const auto disk_original = disk_session.document().state();
            const auto disk_revision =
                disk_session.document().revision();
            const auto disk_undo = disk_session.undoDepth();
            reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            reply = workbench.submitCadInput(
                "FACE", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->setStandardView(
                viewer::StandardView::top));
            viewport->fitAll();
            QApplication::processEvents();
            CHECK(nativePlanarFaceClick(
                *viewport, viewer::Point3{20.0, 15.0, 20.0}));
            const auto disk_admitted =
                controller->selectedMaterialFaceBoundaryAdmission();
            CHECK(disk_admitted.ok());
            CHECK(disk_admitted.wires.size() == 1U);
            CHECK(disk_admitted.wires.front().outer);
            CHECK(disk_admitted.wires.front().edges.size() == 1U);
            CHECK(pg_count->text().contains(
                QStringLiteral("selected: 0")));
            CHECK(pg_count->text().contains(
                QStringLiteral("Unsupported skipped: 1")));
            CHECK(pg_face_detail->text().contains(
                QStringLiteral("PARTIAL Face: 1")));
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 1U);
            CHECK(!pg_finish->isEnabled());
            CHECK(pg_clear->isEnabled());
            CHECK(pg_remove->isEnabled());
            reply = workbench.submitCadInput(
                "FINISH", workbench.cadInputContextGeneration());
            CHECK(!reply.accepted);
            CHECK(disk_session.document().state() == disk_original);
            CHECK(disk_session.document().revision() == disk_revision);
            CHECK(disk_session.undoDepth() == disk_undo);
            CHECK(disk_session.document().findSketch(
                *disk_target.sketch_id)
                ->projection_bindings.empty());
            reply = workbench.submitCadInput(
                "CLEAR", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_supported_overlays_current == 0U);
            CHECK(viewport->runtimeDiagnostics()
                .project_face_skipped_overlays_current == 0U);
            CHECK(disk_session.document().state() == disk_original);
            reply = workbench.submitCadInput(
                "CANCEL", workbench.cadInputContextGeneration());
            CHECK(reply.accepted);
            CHECK(disk_session.document().revision() == disk_revision);
            CHECK(disk_session.undoDepth() == disk_undo);
            std::cout << "PG01D_D3_NATIVE_ALL_UNSUPPORTED_NOOP_PASS"
                      << " red_ais=1"
                      << " finish_blocked=1"
                      << " clear_cancel_noop=1\\n";

            result = EXIT_SUCCESS;
            workbench.close();
            app.quit();
            return;
        }
        if (pg01c_only) {
        // PG-01C C1: real OCCT native cursor -> Workbench Project Geometry,
        // not a fabricated presentation token. The separate later Sketch
        // makes the base Extrude a legal prior Body stage.
        auto pg_session = makeBaseSession(kernel);
        const auto pg_sketch =
            pg_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
        CHECK(pg_sketch.ok() && pg_sketch.sketch_id);
        CHECK(workbench.activateDocument(&pg_session, {}));
        QApplication::processEvents();

        auto* pg_action = workbench.findChild<QAction*>(
            QStringLiteral("editSketchAction"));
        auto* pg_button = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeToolButton"));
        auto* pg_panel = workbench.findChild<QWidget*>(
            QStringLiteral("projectEdgeOperationsWidget"));
        auto* pg_count = workbench.findChild<QLabel*>(
            QStringLiteral("projectEdgeSelectionLabel"));
        auto* pg_result = workbench.findChild<QLabel*>(
            QStringLiteral("projectEdgeResultLabel"));
        auto* pg_finish = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeFinishButton"));
        CHECK(pg_action && pg_button && pg_panel &&
              pg_count && pg_result && pg_finish);
        auto* pg_source_edges = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeSourceEdgesButton"));
        auto* pg_source_face = workbench.findChild<QPushButton*>(
            QStringLiteral("projectEdgeSourceFaceButton"));
        CHECK(pg_source_edges && pg_source_face);
        QTreeWidgetItem* pg_tree_item = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) == QStringLiteral("Sketch 2")) {
                pg_tree_item = *it;
                break;
            }
        }
        CHECK(pg_tree_item);
        tree->clearSelection();
        tree->setCurrentItem(pg_tree_item);
        pg_tree_item->setSelected(true);
        pg_action->trigger();
        QApplication::processEvents();
        CHECK(!pg_button->isHidden());
        CHECK(pg_button->isEnabled());

        const auto pg_before_state = pg_session.document().state();
        const auto pg_before_revision =
            pg_session.document().revision();
        const auto pg_before_undo = pg_session.undoDepth();
        auto pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_button->isChecked());
        CHECK(!pg_panel->isHidden());
        CHECK(!pg_finish->isEnabled());
        CHECK(workbench.acceptsEmptyCadInput());
        CHECK(pg_source_edges->isChecked());
        CHECK(!pg_source_face->isChecked());
        pg_reply = workbench.submitCadInput(
            "FACE", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_source_face->isChecked());
        CHECK(!pg_source_edges->isChecked());
        CHECK(!pg_finish->isEnabled());
        CHECK(pg_session.document().state() == pg_before_state);
        CHECK(pg_session.document().revision() == pg_before_revision);
        pg_reply = workbench.submitCadInput(
            "EDGES", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_source_edges->isChecked());
        CHECK(!pg_source_face->isChecked());
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "CONSTRUCTION", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        pg_reply = workbench.submitCadInput(
            "FINISH", workbench.cadInputContextGeneration());
        CHECK(!pg_reply.accepted);
        CHECK(pg_session.document().state() == pg_before_state);
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "CANCEL", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.document().state() == pg_before_state);

        // Repeat through the same semantic Command Line activation and
        // select an exact source Edge in the native provider.
        pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        const auto pg_probes = authorableEdgeProbes(pg_session, kernel);
        bool pg_picked = false;
        std::optional<viewer::Point3> pg_source_point;
        int native_queries = 0;
        int native_clicks = 0;
        // Try the same genuine native picking from several standard
        // cameras. Exact Body pick/occlusion and Sketch support overlays
        // vary by camera; no synthetic provider token is accepted.
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_right,
                 viewer::StandardView::bottom_back_left}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (const auto& probe : pg_probes) {
                // Only horizontal caps project as non-degenerate
                // Line onto the selected XY Sketch support.
                if (std::abs(probe.world.z) > 1.0e-6 &&
                    std::abs(probe.world.z - 20.0) > 1.0e-6) {
                    continue;
                }
                const auto point =
                    viewport->projectWorldPoint(probe.world);
                if (!point) continue;
                const auto query = viewport->queryBodyTopology(
                    *point,
                    viewer::BodyTopologyPickFilter{
                        false, true, false});
                if (query.valid() && query.completed &&
                    !query.candidates.empty()) {
                    ++native_queries;
                }
                if (!nativeClick(*viewport, probe.world)) {
                    continue;
                }
                ++native_clicks;
                std::cerr << "PG01C_NATIVE_PICK_ATTEMPT"
                          << " orientation=" << static_cast<int>(orientation)
                          << " label=" << pg_count->text().toStdString()
                          << " project_active=" << pg_button->isChecked()
                          << " query_edges=" << query.candidates.size()
                          << std::endl;
                CHECK(pg_button->isChecked());
                pg_picked = pg_count->text().contains(
                    QStringLiteral("selected: 1"));
                if (pg_picked) {
                    pg_source_point = probe.world;
                    break;
                }
                pg_reply = workbench.submitCadInput(
                    "CLEAR", workbench.cadInputContextGeneration());
                CHECK(pg_reply.accepted);
            }
            if (pg_picked) break;
        }
        std::cerr << "PG01C_NATIVE_PICK_SUMMARY"
                  << " queries=" << native_queries
                  << " clicks=" << native_clicks
                  << " selected=" << pg_count->text().toStdString()
                  << std::endl;
        CHECK(pg_picked && pg_source_point);
        CHECK(pg_finish->isEnabled());
        // C2: a live strict analytic projection preview, rather than
        // only a staged Edge token, is required to enable Finish.
        CHECK(pg_result->text().contains(
            QStringLiteral("Current preview: 1 derived Edge(s)")));
        CHECK(pg_result->text().contains(
            QStringLiteral("Regular")));
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "CONSTRUCTION", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_result->text().contains(
            QStringLiteral("Construction")));
        CHECK(pg_finish->isEnabled());
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        pg_reply = workbench.submitCadInput(
            "REGULAR", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(pg_result->text().contains(
            QStringLiteral("Regular")));

        // Owner remediation: a real material vertical Edge projected
        // onto this XY Sketch is degenerate. Reject that *new* pick
        // without destroying an already staged valid horizontal Edge.
        ui::PartViewportController* pg_reject_controller = nullptr;
        for (auto* object : workbench.findChildren<QObject*>()) {
            if (auto* candidate =
                    dynamic_cast<ui::PartViewportController*>(
                        object)) {
                pg_reject_controller = candidate;
                break;
            }
        }
        CHECK(pg_reject_controller);
        const auto pg_prior_selection =
            pg_reject_controller->selectedMaterialEdgeReferences();
        CHECK(pg_prior_selection &&
              pg_prior_selection->size() == 1U);
        const auto pg_vertical = std::find_if(
            pg_probes.begin(), pg_probes.end(),
            [](const WorldEdgeProbe& probe) {
                return probe.world.z > 1.0e-6 &&
                       probe.world.z < 20.0 - 1.0e-6;
            });
        CHECK(pg_vertical != pg_probes.end());
        auto pg_unsupported_batch = *pg_prior_selection;
        pg_unsupported_batch.push_back(
            pg_vertical->reference);
        const auto pg_restored_tokens =
            pg_reject_controller->
                restoreMaterialEdgeToolSelection(
                    pg_unsupported_batch);
        CHECK(pg_restored_tokens &&
              pg_restored_tokens->empty());
        QApplication::processEvents();
        const auto pg_after_reject =
            pg_reject_controller->selectedMaterialEdgeReferences();
        CHECK(pg_after_reject &&
              *pg_after_reject == *pg_prior_selection);
        CHECK(pg_count->text().contains(
            QStringLiteral("selected: 1")));
        CHECK(pg_result->text().contains(
            QStringLiteral("Current preview: 1 derived Edge(s)")));
        CHECK(pg_finish->isEnabled());
        CHECK(pg_session.document().revision() ==
              pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);
        std::cout
            << "PG01C_OWNER_UNSUPPORTED_PICK_PRESERVES_STAGING_PASS"
            << " prior=1 rejected=1 committed=0\\n";

        // Rejected Command Line text is diagnostic-only: it does not
        // discard the exact staged source or leave the tool.
        pg_reply = workbench.submitCadInput(
            "UNSUPPORTED", workbench.cadInputContextGeneration());
        CHECK(!pg_reply.accepted);
        CHECK(pg_button->isChecked());
        CHECK(pg_finish->isEnabled());

        // Hierarchical Esc while a material source is staged: discard
        // pending selection first, exit Sketch Project tool on next Esc.
        QTest::keyClick(viewport, Qt::Key_Escape);
        QApplication::processEvents();
        CHECK(pg_button->isChecked());
        CHECK(!pg_panel->isHidden());
        CHECK(!pg_finish->isEnabled());
        CHECK(pg_count->text().contains(
            QStringLiteral("selected: 0")));
        CHECK(!pg_result->text().contains(
            QStringLiteral("Current preview:")));
        CHECK(pg_session.document().state() == pg_before_state);
        QTest::keyClick(viewport, Qt::Key_Escape);
        QApplication::processEvents();
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.document().revision() == pg_before_revision);
        CHECK(pg_session.undoDepth() == pg_before_undo);

        // Toolbar and Command Line re-enter the *same* stage-scoped
        // material Edge picker; use the same genuine OCCT source click.
        pg_button->click();
        QApplication::processEvents();
        CHECK(pg_button->isChecked());
        CHECK(!pg_panel->isHidden());
        CHECK(nativeClick(*viewport, *pg_source_point));
        CHECK(pg_count->text().contains(
            QStringLiteral("selected: 1")));
        CHECK(pg_finish->isEnabled());

        pg_reply = workbench.submitCadInput(
            "REGULAR", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        pg_finish->click();
        QApplication::processEvents();
        const auto* pg_authored =
            pg_session.document().findSketch(*pg_sketch.sketch_id);
        CHECK(pg_authored);
        CHECK(pg_authored->projection_bindings.size() == 1U);
        CHECK(pg_authored->model.entityCount() == 1U);
        CHECK(pg_session.undoDepth() == pg_before_undo + 1U);
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.undo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        CHECK(pg_session.redo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);

        // D2-E: all three Finish adapters must reach the same atomic
        // command path. The above pass used the right-panel button;
        // redo/undo is followed by typed FINISH, then viewport Enter.
        auto* pg_undo = workbench.findChild<QPushButton*>(
            QStringLiteral("undoDocumentButton"));
        CHECK(pg_undo);
        pg_undo->click();
        QApplication::processEvents();
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(nativeClick(*viewport, *pg_source_point));
        CHECK(pg_finish->isEnabled());
        pg_reply = workbench.submitCadInput(
            "FINISH", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(!pg_button->isChecked());
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_session.undoDepth() == pg_before_undo + 1U);

        pg_undo->click();
        QApplication::processEvents();
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        pg_reply = workbench.submitCadInput(
            "PROJECT", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        CHECK(nativeClick(*viewport, *pg_source_point));
        CHECK(pg_finish->isEnabled());
        QTest::keyClick(viewport, Qt::Key_Return);
        QApplication::processEvents();
        CHECK(!pg_button->isChecked());
        CHECK(pg_panel->isHidden());
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_session.undoDepth() == pg_before_undo + 1U);

        // PG-01C C3: durable Save -> close/open with a cold OCCT provider.
        // Use the native v15 .ss2part store, not a fabricated in-memory
        // restored Part or a serialized Viewer token.
        QTemporaryDir pg_persistence_dir;
        CHECK(pg_persistence_dir.isValid());
        const std::filesystem::path pg_persistence_path =
            std::filesystem::path{
                pg_persistence_dir.path().toStdWString()} /
            "ProjectGeometry.ss2part";
        const part::PartDocumentStore pg_store;
        const auto pg_saved = pg_store.createNew(
            pg_persistence_path, pg_session.document());
        CHECK(pg_saved.ok());
        auto pg_loaded = pg_store.load(pg_persistence_path);
        CHECK(pg_loaded.ok());
        CHECK(pg_loaded.document->documentId() ==
              pg_session.documentId());
        CHECK(pg_loaded.document->state() ==
              pg_session.document().state());
        application::DocumentSession pg_reopened{
            pg_persistence_path,
            std::move(*pg_loaded.document),
            *pg_loaded.checkpoint};
        kernel_occt::OcctSolidModelingKernel pg_cold_kernel;
        const auto pg_cold_evaluation = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        CHECK(pg_cold_evaluation.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto pg_cold_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(), *pg_sketch.sketch_id,
                pg_cold_evaluation, pg_cold_kernel);
        CHECK(pg_cold_projection);
        CHECK(pg_cold_projection->allResolved());
        CHECK(pg_cold_projection->outcomes.size() == 1U);
        CHECK(pg_cold_projection->model.entityCount() == 1U);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);

        // C3 D2-D: select the *linked Sketch curve* through a real
        // Viewport mouse pick after the Project tool has finished.
        // The Operations Break Link action must detach the currently
        // evaluated source geometry with one new Undo transaction.
        CHECK(workbench.activateDocument(&pg_session, {}));
        QApplication::processEvents();
        auto* pg_link_status = workbench.findChild<QLabel*>(
            QStringLiteral("projectLinkedEdgeStatusLabel"));
        auto* pg_break = workbench.findChild<QPushButton*>(
            QStringLiteral("projectBreakLinkButton"));
        CHECK(pg_link_status && pg_break);
        const auto* pg_linked =
            pg_session.document().findSketch(*pg_sketch.sketch_id);
        CHECK(pg_linked &&
              pg_linked->projection_bindings.size() == 1U);
        const auto pg_target =
            pg_linked->projection_bindings.front().target_entity;
        const auto* pg_line = pg_linked->model.findLine(pg_target);
        CHECK(pg_line);
        const viewer::Point3 pg_linked_midpoint{
            (pg_line->start().u + pg_line->end().u) / 2.0,
            (pg_line->start().v + pg_line->end().v) / 2.0,
            0.0};
        bool pg_sketch_selected = false;
        for (const auto orientation : {
                 viewer::StandardView::top,
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::bottom_front_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            const auto cursor =
                viewport->projectWorldPoint(pg_linked_midpoint);
            if (!cursor) continue;
            const QPoint pixel{
                static_cast<int>(std::lround(cursor->x)),
                static_cast<int>(std::lround(cursor->y))};
            if (!viewport->rect().contains(pixel)) continue;
            const auto queried =
                viewport->querySketchPresentation(*cursor);
            std::cerr
                << "PG01C_BREAK_LINK_SKETCH_PICK"
                << " view=" << static_cast<int>(orientation)
                << " completed=" << queried.completed
                << " token=" << queried.token.has_value()
                << std::endl;
            if (!queried.valid() || !queried.completed ||
                !queried.token) continue;
            QTest::mouseMove(viewport, pixel);
            QTest::mouseClick(
                viewport, Qt::LeftButton,
                Qt::NoModifier, pixel);
            QApplication::processEvents();
            pg_sketch_selected = !pg_break->isHidden() &&
                                 pg_break->isEnabled();
            if (pg_sketch_selected) break;
        }
        CHECK(pg_sketch_selected);
        CHECK(pg_link_status->text().contains(
            QStringLiteral("Projected Edge")));
        CHECK(pg_link_status->text().contains(
            QStringLiteral("Source stage:")));
        const auto pg_break_undo = pg_session.undoDepth();
        pg_break->click();
        QApplication::processEvents();
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.contains(pg_target));
        CHECK(pg_session.undoDepth() == pg_break_undo + 1U);
        CHECK(pg_session.undo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_session.redo().changed);
        CHECK(pg_session.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.empty());
        // Open the saved native file as a new Workbench session after
        // the original was edited further. The persisted link is
        // independent of transient selection/Undo state of pg_session.
        CHECK(workbench.activateDocument(&pg_reopened, {}));
        QApplication::processEvents();
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        QTreeWidgetItem* pg_reopened_item = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) == QStringLiteral("Sketch 2")) {
                pg_reopened_item = *it;
                break;
            }
        }
        CHECK(pg_reopened_item);
        tree->clearSelection();
        tree->setCurrentItem(pg_reopened_item);
        pg_reopened_item->setSelected(true);
        pg_action->trigger();
        QApplication::processEvents();
        CHECK(!pg_button->isHidden());
        CHECK(viewport->setStandardView(
            viewer::StandardView::top));
        viewport->fitAll();
        QApplication::processEvents();
        const auto* pg_cold_line =
            pg_cold_projection->model.findLine(pg_target);
        CHECK(pg_cold_line);
        const viewer::Point3 pg_reopened_midpoint{
            (pg_cold_line->start().u +
             pg_cold_line->end().u) / 2.0,
            (pg_cold_line->start().v +
             pg_cold_line->end().v) / 2.0,
            0.0};
        const auto pg_reopened_cursor =
            viewport->projectWorldPoint(pg_reopened_midpoint);
        CHECK(pg_reopened_cursor);
        const auto pg_reopened_pick =
            viewport->querySketchPresentation(
                *pg_reopened_cursor);
        CHECK(pg_reopened_pick.valid());
        CHECK(pg_reopened_pick.completed);
        CHECK(pg_reopened_pick.token.has_value());

        // Source change after cold reopen: stretch the *upstream*
        // rectangle from 40 x 30 to 50 x 40 using one semantic
        // UpdateSketchLinesCommand. Every cap Edge changes, including
        // a bottom/left source; its linked local seed MUST remain
        // unchanged while current projection follows the new Body.
        const auto& pg_upstream =
            pg_reopened.document().state().sketches.front();
        CHECK(pg_upstream.id != *pg_sketch.sketch_id);
        std::vector<application::SketchLineGeometryUpdate>
            pg_upstream_updates;
        const auto upstream_lines = pg_upstream.model.state().lines;
        CHECK(upstream_lines.size() == 4U);
        for (const auto& item : upstream_lines) {
            const auto remap = [](sketch::Point2 point) {
                if (point.u == 40.0) point.u = 50.0;
                if (point.v == 30.0) point.v = 40.0;
                return point;
            };
            pg_upstream_updates.push_back({
                item.id,
                remap(item.start),
                remap(item.end)});
        }
        const auto pg_seed_before =
            pg_reopened.document()
                .findSketch(*pg_sketch.sketch_id)
                ->model.state();
        const auto pg_upstream_update =
            pg_reopened.execute(
                application::UpdateSketchLinesCommand{
                    pg_upstream.id,
                    pg_reopened.document().revision(),
                    std::move(pg_upstream_updates)});
        CHECK(pg_upstream_update.ok());
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.state() == pg_seed_before);
        const auto pg_updated_body =
            part::evaluatePart(
                pg_reopened.document(), pg_cold_kernel);
        CHECK(pg_updated_body.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto pg_updated_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_updated_body,
                pg_cold_kernel);
        CHECK(pg_updated_projection);
        CHECK(pg_updated_projection->allResolved());
        const auto* pg_updated_line =
            pg_updated_projection->model.findLine(pg_target);
        CHECK(pg_updated_line);
        CHECK(*pg_updated_line != *pg_cold_line);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);

        // Suppression/recovery on the exact same FeatureId must not
        // discard binding intent or materialize the old authored seed
        // as a currently resolved Sketch entity.
        CHECK(pg_reopened.document().state().body.features.size() == 1U);
        const auto pg_source_feature =
            pg_reopened.document().state().body.features.front().id;
        const auto pg_suppress = pg_reopened.execute(
            application::SetFeatureSuppressedCommand{
                pg_source_feature,
                pg_reopened.document().revision(),
                true});
        CHECK(pg_suppress.ok());
        const auto pg_missing_body = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        const auto pg_missing_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_missing_body,
                pg_cold_kernel);
        CHECK(!pg_missing_projection ||
              (!pg_missing_projection->allResolved() &&
               !pg_missing_projection->model.contains(pg_target)));
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->projection_bindings.size() == 1U);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.state() == pg_seed_before);

        const auto pg_unsuppress = pg_reopened.execute(
            application::SetFeatureSuppressedCommand{
                pg_source_feature,
                pg_reopened.document().revision(),
                false});
        CHECK(pg_unsuppress.ok());
        const auto pg_recovered_body = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        CHECK(pg_recovered_body.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        const auto pg_recovered_projection =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_recovered_body,
                pg_cold_kernel);
        CHECK(pg_recovered_projection);
        CHECK(pg_recovered_projection->allResolved());
        const auto* pg_recovered_line =
            pg_recovered_projection->model.findLine(pg_target);
        CHECK(pg_recovered_line);
        CHECK(*pg_recovered_line == *pg_updated_line);
        CHECK(pg_reopened.document()
                  .findSketch(*pg_sketch.sketch_id)
                  ->model.state() == pg_seed_before);
        // PG-01C C2: authored linked seed no longer closes a Profile
        // after the upstream resize, but the current effective source
        // does. Profile FIND and hover must follow the same rendered
        // Sketch snapshot as OSNAP/Measure, never the old seed.
        const auto pg_origin_a = pg_recovered_line->start();
        const auto pg_origin_b = pg_recovered_line->end();
        const sketch::Point2 pg_side{
            (pg_origin_a.v - pg_origin_b.v) * 0.25,
            (pg_origin_b.u - pg_origin_a.u) * 0.25};
        const sketch::Point2 pg_far_a{
            pg_origin_a.u + pg_side.u,
            pg_origin_a.v + pg_side.v};
        const sketch::Point2 pg_far_b{
            pg_origin_b.u + pg_side.u,
            pg_origin_b.v + pg_side.v};
        for (const auto& segment : {
                 std::pair{pg_origin_b, pg_far_b},
                 std::pair{pg_far_b, pg_far_a},
                 std::pair{pg_far_a, pg_origin_a}}) {
            const auto added = pg_reopened.execute(
                application::AddSketchLineCommand{
                    *pg_sketch.sketch_id,
                    segment.first,
                    segment.second,
                    sketch::EntityRole::regular});
            CHECK(added.ok());
        }
        const auto* pg_authored_region =
            pg_reopened.document().findSketch(
                *pg_sketch.sketch_id);
        CHECK(pg_authored_region);
        const auto pg_seed_analysis =
            sketch::analyzeRegions(pg_authored_region->model);
        CHECK(pg_seed_analysis.regions.empty());
        const auto pg_region_eval = part::evaluatePart(
            pg_reopened.document(), pg_cold_kernel);
        const auto pg_region_effective =
            part::evaluateEffectiveSketchProjection(
                pg_reopened.document(),
                *pg_sketch.sketch_id,
                pg_region_eval,
                pg_cold_kernel);
        CHECK(pg_region_effective);
        CHECK(pg_region_effective->allResolved());
        const auto pg_current_regions =
            sketch::analyzeRegions(pg_region_effective->model);
        CHECK(pg_current_regions.regions.size() == 1U);
        // The upstream update above intentionally bypassed the GUI.
        // Tear down the *old* edit context before revisiting this
        // Document, otherwise Workbench legitimately rejects a second
        // Edit Sketch and the revision-bound scene fails closed.
        CHECK(workbench.activateDocument(&session, {}));
        QApplication::processEvents();
        CHECK(workbench.activateDocument(&pg_reopened, {}));
        QApplication::processEvents();
        QTreeWidgetItem* pg_profile_sketch_item = nullptr;
        for (QTreeWidgetItemIterator it(tree); *it; ++it) {
            if ((*it)->text(0) ==
                QStringLiteral("Sketch 2")) {
                pg_profile_sketch_item = *it;
                break;
            }
        }
        CHECK(pg_profile_sketch_item);
        tree->clearSelection();
        tree->setCurrentItem(pg_profile_sketch_item);
        pg_profile_sketch_item->setSelected(true);
        pg_action->trigger();
        QApplication::processEvents();
        // Verify the exact scene authority supplied to Profile before
        // diagnosing any mouse-to-Sketch coordinate routing.
        ui::PartViewportController* pg_ui_controller = nullptr;
        for (auto* object :
             workbench.findChildren<QObject*>()) {
            if (auto* candidate =
                    dynamic_cast<ui::PartViewportController*>(
                        object)) {
                pg_ui_controller = candidate;
                break;
            }
        }
        CHECK(pg_ui_controller);
        const auto* pg_ui_model =
            pg_ui_controller->currentSketchInteractionModel();
        std::cerr << "PG01C_PROFILE_SCENE_DEBUG"
                  << " current_scene=" << (pg_ui_model != nullptr)
                  << " lines=" << (pg_ui_model
                                     ? pg_ui_model->state().lines.size()
                                     : 0U)
                  << " regions=" << (pg_ui_model
                                       ? sketch::analyzeRegions(
                                             *pg_ui_model).regions.size()
                                       : 0U)
                  << std::endl;
        CHECK(pg_ui_model);
        CHECK(sketch::analyzeRegions(*pg_ui_model)
                  .regions.size() == 1U);
        pg_reply = workbench.submitCadInput(
            "PROFILE", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        // UI Profile uses its existing tool/viewport pointer
        // grammar; FIND is intentionally not a global CAD keyword.
        // Hover/click the CURRENT derived interior, which does not
        // exist as a closed region in the authored linked seed model.
        auto* pg_profile_result = workbench.findChild<QLabel*>(
            QStringLiteral("profileCurrentResult"));
        auto* pg_profile_finish = workbench.findChild<QPushButton*>(
            QStringLiteral("profileFinishButton"));
        CHECK(pg_profile_result && pg_profile_finish);
        CHECK(viewport->setStandardView(
            viewer::StandardView::top));
        viewport->fitAll();
        QApplication::processEvents();
        const viewer::Point3 pg_region_world{
            (pg_origin_a.u + pg_origin_b.u + pg_side.u) * 0.5,
            (pg_origin_a.v + pg_origin_b.v + pg_side.v) * 0.5,
            0.0};
        // Prove the world-space point is inside the current
        // evaluated region before attributing any failure to Qt input.
        const auto pg_direct_pick = sketch::pickRegion(
            *pg_ui_model,
            sketch::analyzeRegions(*pg_ui_model),
            sketch::Point2{
                pg_region_world.x, pg_region_world.y});
        std::cerr << "PG01C_PROFILE_REGION_DIRECT"
                  << " location=" << static_cast<int>(
                         pg_direct_pick.location)
                  << " region=" << pg_direct_pick.region_index.has_value()
                  << std::endl;
        CHECK(pg_direct_pick.region_index.has_value());
        const auto pg_region_pos =
            viewport->projectWorldPoint(pg_region_world);
        CHECK(pg_region_pos);
        const QPoint pg_region_pixel{
            static_cast<int>(std::lround(pg_region_pos->x)),
            static_cast<int>(std::lround(pg_region_pos->y))};
        CHECK(viewport->rect().contains(pg_region_pixel));
        QTest::mouseMove(viewport, pg_region_pixel);
        QApplication::processEvents();
        auto* pg_profile_panel = workbench.findChild<QWidget*>(
            QStringLiteral("profileOperationsWidget"));
        auto* pg_status = workbench.findChild<QLabel*>(
            QStringLiteral("workbenchStatus"));
        CHECK(pg_profile_panel && pg_status);
        std::cerr << "PG01C_PROFILE_POINTER_DEBUG"
                  << " profile_panel_hidden=" << pg_profile_panel->isHidden()
                  << " finish_enabled=" << pg_profile_finish->isEnabled()
                  << " region_px=" << pg_region_pixel.x()
                  << "," << pg_region_pixel.y()
                  << " status=" << pg_status->text().toStdString()
                  << " result=" << pg_profile_result->text().toStdString()
                  << std::endl;
        QTest::mouseClick(
            viewport, Qt::LeftButton,
            Qt::NoModifier, pg_region_pixel);
        QApplication::processEvents();
        std::cerr << "PG01C_PROFILE_CLICK_DEBUG"
                  << " result=" << pg_profile_result->text().toStdString()
                  << " finish=" << pg_profile_finish->isEnabled()
                  << std::endl;
        CHECK(pg_profile_result->text().contains(
            QStringLiteral("Status: Valid")));
        CHECK(pg_profile_finish->isEnabled());
        CHECK(pg_profile_result->text().contains(
            QStringLiteral("Status: Valid")));
        pg_reply = workbench.submitCadInput(
            "CANCEL", workbench.cadInputContextGeneration());
        CHECK(pg_reply.accepted);
        std::cout
            << "PG01C_C2_CURRENT_PROFILE_REGIONS_PASS"
            << " persisted_seed_regions=0"
            << " current_linked_regions=1"
            << " native_profile_hover=1"
            << " native_profile_draft=1\\n";
        std::cout
            << "PG01C_C3_SUPPRESSION_RECOVERY_PASS"
            << " no_saved_seed_fallback=1"
            << " same_source_id=1"
            << " exact_reprojection=1\\n";
        std::cout
            << "PG01C_C3_UPSTREAM_RECOMPUTE_PASS"
            << " current_curve_changed=1"
            << " authored_seed_unchanged=1"
            << " cold_occt=1\\n";
        std::cout
            << "PG01C_C3_NATIVE_SAVE_REOPEN_PASS"
            << " native_v15=1"
            << " cold_occt=1"
            << " current_linked_scene=1\\n";
        std::cout
            << "PG01C_C3_NATIVE_BREAK_LINK_PASS"
            << " real_sketch_pick=1"
            << " current_geometry=1"
            << " undo_redo=1\\n";
        std::cout
            << "PG01C_C1_NATIVE_EDGE_TOOL_PASS"
            << " actual_cursor=1"
            << " semantic_finish=1"
            << " cancel_zero_mutation=1"
            << " undo_redo=1\n";

            // PG-01C C1 acceptance: two different actual OCCT cap
            // material Edges are staged through one native multi-pick
            // session (Ctrl-click adds to the staged selection), and
            // Finish authors two linked entities in ONE Undo entry.
            auto pg_batch_session = makeBaseSession(kernel);
            const auto pg_batch_sketch =
                pg_batch_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(pg_batch_sketch.ok() && pg_batch_sketch.sketch_id);
            CHECK(workbench.activateDocument(&pg_batch_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* pg_batch_item = nullptr;
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->text(0) ==
                    QStringLiteral("Sketch 2")) {
                    pg_batch_item = *it;
                    break;
                }
            }
            CHECK(pg_batch_item);
            tree->clearSelection();
            tree->setCurrentItem(pg_batch_item);
            pg_batch_item->setSelected(true);
            pg_action->trigger();
            QApplication::processEvents();
            pg_reply = workbench.submitCadInput(
                "PROJECT", workbench.cadInputContextGeneration());
            CHECK(pg_reply.accepted);
            const auto pg_batch_before_undo =
                pg_batch_session.undoDepth();
            const auto pg_batch_before_revision =
                pg_batch_session.document().revision();
            bool pg_batch_picked = false;
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_right,
                     viewer::StandardView::bottom_back_left}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                if (!nativeClick(*viewport, *pg_source_point) ||
                    !pg_count->text().contains(
                        QStringLiteral("selected: 1"))) {
                    pg_reply = workbench.submitCadInput(
                        "CLEAR", workbench.cadInputContextGeneration());
                    CHECK(pg_reply.accepted);
                    continue;
                }
                for (const auto& probe : pg_probes) {
                    if (std::abs(probe.world.z) > 1.0e-6 &&
                        std::abs(probe.world.z - 20.0) > 1.0e-6) {
                        continue;
                    }
                    if (probe.world == *pg_source_point) {
                        continue;
                    }
                    const auto pixel_pos =
                        viewport->projectWorldPoint(probe.world);
                    if (!pixel_pos) continue;
                    const QPoint pixel{
                        static_cast<int>(std::lround(pixel_pos->x)),
                        static_cast<int>(std::lround(pixel_pos->y))};
                    if (!viewport->rect().contains(pixel)) continue;
                    QTest::mouseMove(viewport, pixel);
                    QTest::mouseClick(
                        viewport, Qt::LeftButton,
                        Qt::ControlModifier, pixel);
                    QApplication::processEvents();
                    if (pg_count->text().contains(
                            QStringLiteral("selected: 2")) &&
                        pg_finish->isEnabled()) {
                        pg_batch_picked = true;
                        break;
                    }
                    pg_reply = workbench.submitCadInput(
                        "CLEAR", workbench.cadInputContextGeneration());
                    CHECK(pg_reply.accepted);
                    CHECK(nativeClick(*viewport, *pg_source_point));
                    CHECK(pg_count->text().contains(
                        QStringLiteral("selected: 1")));
                }
                if (pg_batch_picked) break;
                pg_reply = workbench.submitCadInput(
                    "CLEAR", workbench.cadInputContextGeneration());
                CHECK(pg_reply.accepted);
            }
            CHECK(pg_batch_picked);
            CHECK(pg_result->text().contains(
                QStringLiteral("Current preview: 2 derived Edge(s)")));
            CHECK(pg_batch_session.document().revision() ==
                  pg_batch_before_revision);
            CHECK(pg_batch_session.undoDepth() ==
                  pg_batch_before_undo);
            pg_finish->click();
            QApplication::processEvents();
            const auto* pg_batch_authored =
                pg_batch_session.document().findSketch(
                    *pg_batch_sketch.sketch_id);
            CHECK(pg_batch_authored);
            CHECK(pg_batch_authored->projection_bindings.size() == 2U);
            CHECK(pg_batch_session.undoDepth() ==
                  pg_batch_before_undo + 1U);
            const auto pg_batch_links =
                pg_batch_authored->projection_bindings;
            CHECK(pg_batch_session.undo().changed);
            CHECK(pg_batch_session.document()
                      .findSketch(*pg_batch_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(pg_batch_session.redo().changed);
            CHECK(pg_batch_session.document()
                      .findSketch(*pg_batch_sketch.sketch_id)
                      ->projection_bindings == pg_batch_links);
            std::cout << "PG01C_C1_TWO_EDGE_ATOMIC_PASS"
                      << " native_ctrl_selection=1"
                      << " linked_edges=2"
                      << " undo_batches=1\\n";

            // D2-K synthetic native OCCT regression: a rectangle
            // after Chamfer is authored from four real same-stage
            // material Edges. Their projected end vertices must be
            // exactly connected; no Sketcher tolerance healing.
            auto pg_chamfer_session = makeBaseSession(kernel);
            // A skewed source forces nontrivial analytic endpoint
            // evaluation. Equal authored source vertices are mapped
            // with one function so they remain exactly shared.
            const auto pg_skew_authored =
                pg_chamfer_session.document().state();
            const auto& pg_skew_source =
                pg_skew_authored.sketches.front();
            const double skew_cos = std::cos(0.31);
            const double skew_sin = std::sin(0.31);
            const auto rotate = [skew_cos, skew_sin](
                                    sketch::Point2 p) {
                return sketch::Point2{
                    p.u * skew_cos - p.v * skew_sin,
                    p.u * skew_sin + p.v * skew_cos};
            };
            std::vector<application::SketchLineGeometryUpdate>
                pg_skew_updates;
            for (const auto& line :
                 pg_skew_source.model.state().lines) {
                pg_skew_updates.push_back({
                    line.id,
                    rotate(line.start),
                    rotate(line.end)});
            }
            CHECK(pg_skew_updates.size() == 4U);
            CHECK(pg_chamfer_session.execute(
                application::UpdateSketchLinesCommand{
                    pg_skew_source.id,
                    pg_chamfer_session.document().revision(),
                    std::move(pg_skew_updates)}).ok());
            std::vector<part::MaterialEdgeReference>
                pg_pre_chamfer_cap;
            for (const auto& probe :
                 authorableEdgeProbes(pg_chamfer_session, kernel)) {
                if (std::abs(probe.world.z - 20.0) < 1.0e-6) {
                    pg_pre_chamfer_cap.push_back(probe.reference);
                }
            }
            CHECK(pg_pre_chamfer_cap.size() == 4U);
            std::sort(
                pg_pre_chamfer_cap.begin(),
                pg_pre_chamfer_cap.end());
            const auto pg_chamfer =
                pg_chamfer_session.execute(
                    application::CreateChamferFeatureCommand{
                        pg_pre_chamfer_cap,
                        pg_chamfer_session.document().revision(),
                        core::LengthValue{2.0},
                        "PG01C vertex continuity Chamfer"},
                    kernel);
            CHECK(pg_chamfer.ok());
            const auto pg_chamfer_sketch =
                pg_chamfer_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(pg_chamfer_sketch.ok() &&
                  pg_chamfer_sketch.sketch_id);
            std::vector<part::MaterialEdgeReference>
                pg_post_chamfer_cap;
            for (const auto& probe :
                 authorableEdgeProbes(pg_chamfer_session, kernel)) {
                if (std::abs(probe.world.z - 20.0) < 1.0e-6) {
                    pg_post_chamfer_cap.push_back(probe.reference);
                }
            }
            CHECK(pg_post_chamfer_cap.size() == 4U);
            const auto pg_chamfer_projected =
                pg_chamfer_session.execute(
                    application::CreateProjectedSketchEdgesCommand{
                        *pg_chamfer_sketch.sketch_id,
                        pg_chamfer_session.document().revision(),
                        pg_post_chamfer_cap,
                        sketch::EntityRole::regular},
                    kernel);
            CHECK(pg_chamfer_projected.ok());
            CHECK(pg_chamfer_projected.entity_ids.size() == 4U);
            const auto pg_chamfer_current =
                part::evaluateEffectiveSketchProjection(
                    pg_chamfer_session.document(),
                    *pg_chamfer_sketch.sketch_id,
                    part::evaluatePart(
                        pg_chamfer_session.document(), kernel),
                    kernel);
            CHECK(pg_chamfer_current &&
                  pg_chamfer_current->allResolved());
            const auto pg_chamfer_regions =
                sketch::analyzeRegions(pg_chamfer_current->model);
            CHECK(pg_chamfer_regions.complete());
            CHECK(pg_chamfer_regions.regions.size() == 1U);
            const auto pg_chamfer_intent =
                part::makeProfileRegionIntent(
                    pg_chamfer_regions.regions.front());
            CHECK(pg_chamfer_intent);
            const auto pg_chamfer_profile =
                pg_chamfer_session.execute(
                    application::CreateProfileCommand{
                        *pg_chamfer_sketch.sketch_id,
                        pg_chamfer_session.document().revision(),
                        *pg_chamfer_intent},
                    kernel);
            CHECK(pg_chamfer_profile.ok());
            std::cout << "PG01C_D2K_CHAMFER_LINKED_REGION_PASS"
                      << " edges=4 native_occt=1"
                      << " no_tolerance_heal=1\\n";

            // Owner identity regression: an all-linked rectangle
            // Profile remains the SAME Profile as each linked Edge is
            // detached, including mixed linked/authored intermediate
            // boundaries. No ProfileId or RegionIntent recreation.
            auto pg_all_session = makeBaseSession(kernel);
            const auto pg_all_sketch =
                pg_all_session.execute(
                    application::CreatePartSketchCommand{
                        core::BuiltinReferenceRole::xy_plane});
            CHECK(pg_all_sketch.ok() &&
                  pg_all_sketch.sketch_id);
            std::vector<part::MaterialEdgeReference>
                pg_cap_sources;
            for (const auto& probe :
                 authorableEdgeProbes(pg_all_session, kernel)) {
                if (std::abs(probe.world.z - 20.0) < 1.0e-6) {
                    pg_cap_sources.push_back(probe.reference);
                }
            }
            CHECK(pg_cap_sources.size() == 4U);
            const auto pg_all_authored =
                pg_all_session.execute(
                    application::CreateProjectedSketchEdgesCommand{
                        *pg_all_sketch.sketch_id,
                        pg_all_session.document().revision(),
                        pg_cap_sources,
                        sketch::EntityRole::regular},
                    kernel);
            CHECK(pg_all_authored.ok());
            CHECK(pg_all_authored.entity_ids.size() == 4U);
            const auto* pg_all_linked =
                pg_all_session.document().findSketch(
                    *pg_all_sketch.sketch_id);
            CHECK(pg_all_linked &&
                  pg_all_linked->projection_bindings.size() == 4U);
            const auto pg_all_targets =
                pg_all_linked->projection_bindings;
            const auto pg_all_body = part::evaluatePart(
                pg_all_session.document(), kernel);
            const auto pg_all_effective =
                part::evaluateEffectiveSketchProjection(
                    pg_all_session.document(),
                    *pg_all_sketch.sketch_id,
                    pg_all_body, kernel);
            CHECK(pg_all_effective &&
                  pg_all_effective->allResolved());
            const auto pg_all_regions =
                sketch::analyzeRegions(pg_all_effective->model);
            CHECK(pg_all_regions.complete() &&
                  pg_all_regions.regions.size() == 1U);
            const auto pg_all_intent =
                part::makeProfileRegionIntent(
                    pg_all_regions.regions.front());
            CHECK(pg_all_intent);

            // D2-P: change the upstream rectangle *after* projecting
            // all four linked targets. Persisted linked seeds stay
            // identical; the new current region (50x40) must be the
            // semantic authority for Create Profile, not old 40x30.
            auto pg_changed_doc =
                part::PartDocument::restore(
                    pg_all_session.document().documentId(),
                    pg_all_session.document().state(),
                    pg_all_session.document().revision());
            CHECK(pg_changed_doc.ok());
            application::DocumentSession pg_changed_session{
                {}, std::move(*pg_changed_doc.document)};
            const auto pg_changed_authored_state =
                pg_changed_session.document().state();
            const auto& pg_changed_upstream =
                pg_changed_authored_state.sketches.front();
            CHECK(pg_changed_upstream.id != *pg_all_sketch.sketch_id);
            std::vector<application::SketchLineGeometryUpdate>
                pg_changed_updates;
            const auto pg_changed_lines =
                pg_changed_upstream.model.state().lines;
            CHECK(pg_changed_lines.size() == 4U);
            for (const auto& item : pg_changed_lines) {
                const auto remap = [](sketch::Point2 p) {
                    if (p.u == 40.0) p.u = 50.0;
                    if (p.v == 30.0) p.v = 40.0;
                    return p;
                };
                pg_changed_updates.push_back({
                    item.id, remap(item.start), remap(item.end)});
            }
            const auto pg_changed_seed =
                pg_changed_session.document()
                    .findSketch(*pg_all_sketch.sketch_id)->model.state();
            const auto pg_changed_result =
                pg_changed_session.execute(
                    application::UpdateSketchLinesCommand{
                        pg_changed_upstream.id,
                        pg_changed_session.document().revision(),
                        std::move(pg_changed_updates)});
            CHECK(pg_changed_result.ok());
            CHECK(pg_changed_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->model.state() == pg_changed_seed);
            const auto pg_changed_effective =
                part::evaluateEffectiveSketchProjection(
                    pg_changed_session.document(),
                    *pg_all_sketch.sketch_id,
                    part::evaluatePart(
                        pg_changed_session.document(), kernel),
                    kernel);
            CHECK(pg_changed_effective &&
                  pg_changed_effective->allResolved());
            const auto pg_changed_regions =
                sketch::analyzeRegions(pg_changed_effective->model);
            CHECK(pg_changed_regions.complete() &&
                  pg_changed_regions.regions.size() == 1U);
            CHECK(pg_changed_regions.regions.front().area !=
                  pg_all_regions.regions.front().area);
            const auto pg_changed_intent =
                part::makeProfileRegionIntent(
                    pg_changed_regions.regions.front());
            CHECK(pg_changed_intent);
            const auto pg_changed_rev =
                pg_changed_session.document().revision();
            const auto pg_changed_reject =
                pg_changed_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_changed_rev,
                        *pg_changed_intent});
            CHECK(!pg_changed_reject.ok());
            CHECK(pg_changed_session.document().revision() ==
                  pg_changed_rev);
            const auto pg_changed_profile =
                pg_changed_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_changed_rev,
                        *pg_changed_intent},
                    kernel);
            CHECK(pg_changed_profile.ok() &&
                  pg_changed_profile.profile_id);
            CHECK(pg_changed_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->model.state() == pg_changed_seed);
            std::cout << "PG01C_D2P_MOVED_SOURCE_CREATE_PASS"
                      << " authored_seed_stale=1"
                      << " current_provider=1"
                      << " no_provider_rejected=1\\n";

            const auto pg_legacy_revision =
                pg_all_session.document().revision();
            const auto pg_legacy_undo =
                pg_all_session.undoDepth();
            const auto pg_legacy_rejected =
                pg_all_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_legacy_revision,
                        *pg_all_intent});
            CHECK(!pg_legacy_rejected.ok());
            CHECK(pg_all_session.document().revision() ==
                  pg_legacy_revision);
            CHECK(pg_all_session.undoDepth() ==
                  pg_legacy_undo);
            const auto pg_all_created =
                pg_all_session.execute(
                    application::CreateProfileCommand{
                        *pg_all_sketch.sketch_id,
                        pg_all_session.document().revision(),
                        *pg_all_intent},
                    kernel);
            CHECK(pg_all_created.ok() &&
                  pg_all_created.profile_id);
            const auto pg_all_profile =
                *pg_all_session.document().findProfile(
                    *pg_all_created.profile_id);
            // The authored-only API intentionally refuses linked
            // Profile evaluation; effective source is the authority.
            CHECK(!pg_all_session.document()
                      .evaluateProfile(pg_all_profile.id));

            // D2-T real Workbench Tree must agree with the effective
            // Viewer/Properties Profile, not label all-linked geometry
            // Invalid merely because authored-only evaluation refuses.
            CHECK(workbench.activateDocument(&pg_all_session, {}));
            QApplication::processEvents();
            QTreeWidgetItem* pg_linked_profile_tree = nullptr;
            const auto pg_profile_identity =
                QString::fromStdString(
                    pg_all_profile.id.serialized());
            for (QTreeWidgetItemIterator it(tree); *it; ++it) {
                if ((*it)->toolTip(0).contains(
                        QStringLiteral("ProfileId: ") +
                        pg_profile_identity)) {
                    pg_linked_profile_tree = *it;
                    break;
                }
            }
            CHECK(pg_linked_profile_tree != nullptr);
            CHECK(!pg_linked_profile_tree->text(0).contains(
                QStringLiteral("[Invalid]")));
            CHECK(pg_linked_profile_tree->toolTip(0).contains(
                QStringLiteral("Status: Valid")));
            std::cout << "PG01C_D2T_LINKED_TREE_VALID_PASS"
                      << " all_linked=4"
                      << " authored_only_rejected=1\\n";

            // D2-B: four linked Edges are detached in one semantic
            // transaction without changing Profile or EntityIds. Invalid
            // duplicate input rolls back the whole staged operation.
            auto pg_clone =
                part::PartDocument::restore(
                    pg_all_session.document().documentId(),
                    pg_all_session.document().state(),
                    pg_all_session.document().revision());
            CHECK(pg_clone.ok());
            application::DocumentSession pg_batch_detach_session{
                {}, std::move(*pg_clone.document)};
            std::vector<sketch::EntityId> pg_batch_targets;
            for (const auto& binding : pg_all_targets) {
                pg_batch_targets.push_back(binding.target_entity);
            }
            const auto pg_batch_revision =
                pg_batch_detach_session.document().revision();
            const auto pg_batch_undo =
                pg_batch_detach_session.undoDepth();
            const auto pg_batch_reject =
                pg_batch_detach_session.execute(
                    application::BreakProjectedEdgeLinksCommand{
                        *pg_all_sketch.sketch_id,
                        {pg_batch_targets.front(),
                         pg_batch_targets.front()},
                        pg_batch_revision},
                    kernel);
            CHECK(!pg_batch_reject.ok());
            CHECK(pg_batch_detach_session.document().revision() ==
                  pg_batch_revision);
            CHECK(pg_batch_detach_session.undoDepth() ==
                  pg_batch_undo);
            const auto pg_batch_detach =
                pg_batch_detach_session.execute(
                    application::BreakProjectedEdgeLinksCommand{
                        *pg_all_sketch.sketch_id,
                        pg_batch_targets,
                        pg_batch_revision},
                    kernel);
            CHECK(pg_batch_detach.ok());
            CHECK(pg_batch_detach_session.undoDepth() ==
                  pg_batch_undo + 1U);
            CHECK(pg_batch_detach_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(*pg_batch_detach_session.document().findProfile(
                      pg_all_profile.id) == pg_all_profile);
            const auto* pg_detach_authored =
                pg_batch_detach_session.document().findSketch(
                    *pg_all_sketch.sketch_id);
            CHECK(pg_detach_authored);
            for (const auto& binding : pg_all_targets) {
                const auto* actual =
                    pg_detach_authored->model.findLine(
                        binding.target_entity);
                const auto* expected =
                    pg_all_effective->model.findLine(
                        binding.target_entity);
                CHECK(actual && expected && *actual == *expected);
            }
            CHECK(pg_batch_detach_session.document()
                      .evaluateProfile(pg_all_profile.id)->valid());
            CHECK(pg_batch_detach_session.undo().changed);
            CHECK(pg_batch_detach_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.size() == 4U);
            CHECK(*pg_batch_detach_session.document().findProfile(
                      pg_all_profile.id) == pg_all_profile);
            CHECK(pg_batch_detach_session.redo().changed);
            CHECK(pg_batch_detach_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.empty());
            CHECK(pg_batch_detach_session.document()
                      .evaluateProfile(pg_all_profile.id)->valid());
            std::cout << "PG01C_D2B_ATOMIC_BREAK_LINK_PASS"
                      << " linked=4 undo_batches=1"
                      << " invalid_rollbacks=1"
                      << " profile_unchanged=1\\n";

            for (const auto& binding : pg_all_targets) {
                const auto before_body = part::evaluatePart(
                    pg_all_session.document(), kernel);
                const auto before =
                    part::evaluateEffectiveSketchProjection(
                        pg_all_session.document(),
                        *pg_all_sketch.sketch_id,
                        before_body, kernel);
                CHECK(before && before->allResolved());
                const auto* before_line =
                    before->model.findLine(
                        binding.target_entity);
                CHECK(before_line);
                const auto expected_line = *before_line;
                const auto detached = pg_all_session.execute(
                    application::BreakProjectedEdgeLinkCommand{
                        *pg_all_sketch.sketch_id,
                        binding.target_entity,
                        pg_all_session.document().revision()},
                    kernel);
                CHECK(detached.ok());
                const auto* after_sketch =
                    pg_all_session.document().findSketch(
                        *pg_all_sketch.sketch_id);
                CHECK(after_sketch);
                const auto* frozen_line =
                    after_sketch->model.findLine(
                        binding.target_entity);
                CHECK(frozen_line &&
                      *frozen_line == expected_line);
                CHECK(*pg_all_session.document().findProfile(
                          pg_all_profile.id) == pg_all_profile);
                const auto after_body = part::evaluatePart(
                    pg_all_session.document(), kernel);
                const auto after =
                    part::evaluateEffectiveSketchProjection(
                        pg_all_session.document(),
                        *pg_all_sketch.sketch_id,
                        after_body, kernel);
                CHECK(after && after->allResolved());
                const auto resolved =
                    part::resolveProfileRegionIntent(
                        after->model,
                        pg_all_profile.region_intent);
                CHECK(resolved.valid());
                CHECK(resolved.region->area ==
                      pg_all_regions.regions.front().area);
            }
            CHECK(pg_all_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.empty());
            const auto pg_all_final =
                pg_all_session.document().evaluateProfile(
                    pg_all_profile.id);
            CHECK(pg_all_final && pg_all_final->valid());
            CHECK(pg_all_session.undo().changed);
            CHECK(pg_all_session.document()
                      .findSketch(*pg_all_sketch.sketch_id)
                      ->projection_bindings.size() == 1U);
            CHECK(*pg_all_session.document().findProfile(
                      pg_all_profile.id) == pg_all_profile);
            CHECK(pg_all_session.redo().changed);
            CHECK(pg_all_session.document()
                      .evaluateProfile(pg_all_profile.id)
                      ->valid());
            QTemporaryDir pg_all_dir;
            CHECK(pg_all_dir.isValid());
            const auto pg_all_path =
                std::filesystem::path{
                    pg_all_dir.path().toStdWString()} /
                "LinkedProfileBreak.ss2part";
            const auto pg_all_saved =
                pg_store.createNew(
                    pg_all_path, pg_all_session.document());
            CHECK(pg_all_saved.ok());
            const auto pg_all_loaded =
                pg_store.load(pg_all_path);
            CHECK(pg_all_loaded.ok());
            CHECK(*pg_all_loaded.document->findProfile(
                      pg_all_profile.id) == pg_all_profile);
            CHECK(pg_all_loaded.document
                      ->evaluateProfile(pg_all_profile.id)
                      ->valid());
            std::cout
                << "PG01C_OWNER_PROFILE_LINK_BREAK_IDENTITY_PASS"
                << " all_linked=4 mixed_then_authored=1"
                << " stable_ids=1 undo_redo=1"
                << " cold_reopen=1\\n";

            CHECK(workbench.activateDocument(&session, {}));
            result = EXIT_SUCCESS;
            workbench.close();
            app.quit();
            return;
        }
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

        // R2-A Owner attempt 3 (Part013): persist-equivalent history.
        // A 20 x 20 x 20 box in negative Y, three specific upper
        // (20, -20, 20) material Edges, Fillet radius 2. Unlike the
        // native mouse control above, this isolates reconstructed CAD
        // evaluation and body presentation from cursor/camera effects.
        // The private Owner document is not checked into the repository.
        auto owner_corner_session =
            makeBaseSession(kernel, 20.0, -20.0, 20.0);
        const auto owner_base =
            part::evaluatePart(
                owner_corner_session.document(), kernel);
        CHECK(owner_base.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(owner_base.current_topology);
        CHECK(owner_base.current_topology->complete());
        const auto owner_base_presentation =
            kernel.bodyPresentation(owner_base.body_solid);
        CHECK(owner_base_presentation.ok());

        const auto is_owner_vertex =
            [](const auto& point) {
                constexpr double tolerance = 1.0e-5;
                return std::abs(point.x - 20.0) < tolerance &&
                       std::abs(point.y + 20.0) < tolerance &&
                       std::abs(point.z - 20.0) < tolerance;
            };
        std::vector<part::MaterialEdgeReference> owner_edges;
        for (const auto& edge :
             owner_base_presentation.body.edges) {
            if (edge.points.size() < 2U ||
                (!is_owner_vertex(edge.points.front()) &&
                 !is_owner_vertex(edge.points.back()))) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *owner_base.current_topology,
                    edge.runtime_token);
            CHECK(authored.ok());
            owner_edges.push_back(*authored.reference);
        }
        CHECK(owner_edges.size() == 3U);
        std::sort(owner_edges.begin(), owner_edges.end());
        CHECK(std::adjacent_find(
            owner_edges.begin(), owner_edges.end()) ==
            owner_edges.end());

        auto owner_state = owner_corner_session.document().state();
        const auto owner_fillet_id =
            owner_state.body.next_feature_id.allocate();
        CHECK(owner_fillet_id.has_value());
        owner_state.body.features.push_back(
            part::PartFeature{
                *owner_fillet_id,
                "Fillet002",
                false,
                part::FilletFeature{
                    owner_edges,
                    core::LengthValue{2.0}}});
        auto owner_restored =
            part::PartDocument::restore(
                owner_corner_session.document().documentId(),
                std::move(owner_state),
                owner_corner_session.document().revision());
        CHECK(owner_restored.ok());
        application::DocumentSession owner_committed{
            {}, std::move(*owner_restored.document)};
        const auto owner_result =
            part::evaluatePart(owner_committed.document(), kernel);
        const bool owner_body_up_to_date =
            owner_result.body_status ==
                part::BodyEvaluationStatus::up_to_date &&
            owner_result.body_solid != nullptr &&
            owner_result.current_topology &&
            owner_result.current_topology->complete();
        CHECK(owner_body_up_to_date);

        const auto owner_presentation =
            kernel.bodyPresentation(owner_result.body_solid);
        std::cerr
            << "PM05F_R2_OWNER_NEGATIVE_Y_CORNER"
            << " body_up_to_date=1"
            << " presentation_status="
            << static_cast<int>(owner_presentation.status)
            << " presentation_ok="
            << (owner_presentation.ok() ? 1 : 0)
            << " faces="
            << owner_presentation.body.faces.size()
            << " edges="
            << owner_presentation.body.edges.size()
            << '\n';
        CHECK(owner_presentation.ok());

        CHECK(workbench.activateDocument(&owner_committed, {}));
        QApplication::processEvents();
        const auto owner_viewer =
            viewport->runtimeDiagnostics();
        std::cerr
            << "PM05F_R2_OWNER_NEGATIVE_Y_VIEWER"
            << " body_displayed="
            << (owner_viewer.solid_committed_displayed ? 1 : 0)
            << " expected_style="
            << (owner_viewer.solid_committed_style_expected ? 1 : 0)
            << '\n';
        CHECK(owner_viewer.solid_committed_displayed);
        CHECK(owner_viewer.solid_committed_style_expected);

        // Owner Part013's actual authored sketch segment order, Profile
        // region intent and three durable MaterialEdgeReferences are read
        // from a sanitized copy of the native document. This catches
        // differences that an equivalent bounding box cannot reproduce.
        // Only regenerated document/sketch UUIDs differ from Owner input.
        const auto owner_fixture =
            std::filesystem::path{__FILE__}.parent_path() /
            "fixtures" /
            "pm05f_r2_part013_sanitized.ss2part";
        const part::PartDocumentStore owner_store;
        auto owner_loaded = owner_store.load(owner_fixture);
        CHECK(owner_loaded.ok());
        application::DocumentSession owner_exact_session{
            {}, std::move(*owner_loaded.document)};
        const auto owner_exact_result =
            part::evaluatePart(owner_exact_session.document(), kernel);
        const bool owner_exact_up_to_date =
            owner_exact_result.body_status ==
                part::BodyEvaluationStatus::up_to_date &&
            owner_exact_result.body_solid != nullptr &&
            owner_exact_result.current_topology &&
            owner_exact_result.current_topology->complete();
        std::cerr
            << "PM05F_R2_OWNER_EXACT_NATIVE"
            << " body_up_to_date="
            << (owner_exact_up_to_date ? 1 : 0)
            << '\n';
        CHECK(owner_exact_up_to_date);
        const auto owner_exact_presentation =
            kernel.bodyPresentation(owner_exact_result.body_solid);
        std::cerr
            << "PM05F_R2_OWNER_EXACT_PRESENTATION"
            << " status="
            << static_cast<int>(owner_exact_presentation.status)
            << " ok="
            << (owner_exact_presentation.ok() ? 1 : 0)
            << " faces="
            << owner_exact_presentation.body.faces.size()
            << " edges="
            << owner_exact_presentation.body.edges.size()
            << '\n';
        CHECK(owner_exact_presentation.ok());
        // Audit two-point Edge presentation spans in the exact stored
        // Owner geometry. A mathematically collapsed path cannot form
        // an OCCT AIS_Wire; this is not a license to fabricate CAD
        // geometry or silently enlarge numerical tolerances.
        for (const auto& edge :
             owner_exact_presentation.body.edges) {
            if (edge.points.size() != 2U) {
                continue;
            }
            const auto& a = edge.points.front();
            const auto& b = edge.points.back();
            const auto length = std::hypot(
                std::hypot(b.x - a.x, b.y - a.y),
                b.z - a.z);
            std::cerr
                << "PM05F_R2_OWNER_TWO_POINT_EDGE"
                << " runtime_token=" << edge.runtime_token.value
                << " length_mm=" << length
                << " a=" << a.x << "," << a.y << "," << a.z
                << " b=" << b.x << "," << b.y << "," << b.z
                << '\n';
        }
        // Owner screenshot uses Shaded + Edges, not the widget's
        // default Shaded mode. Style synchronization builds an
        // additional OCCT wire for every material Edge and must never
        // make the otherwise valid committed Body disappear.
        // Capture skipped OCCT outline objects. A visible Body alone
        // is necessary but does not prove complete material Edge drawing.
        owner_edge_outline_warnings.store(
            0U, std::memory_order_relaxed);
        const auto old_qt_handler =
            qInstallMessageHandler(
                captureOwnerEdgeOutlineWarning);
        CHECK(workbench.activateDocument(&session, {}));
        QApplication::processEvents();
        const auto owner_shaded_edges_enabled =
            viewport->setViewStyle(
                viewer::ViewStyle::shaded_with_edges);
        std::cerr
            << "PM05F_R2_OWNER_EDGE_STYLE_ENABLED="
            << (owner_shaded_edges_enabled ? 1 : 0)
            << '\n';
        CHECK(owner_shaded_edges_enabled);
        CHECK(workbench.activateDocument(&owner_exact_session, {}));
        QApplication::processEvents();
        const auto owner_exact_viewer = viewport->runtimeDiagnostics();
        std::cerr
            << "PM05F_R2_OWNER_EXACT_VIEWER"
            << " body_displayed="
            << (owner_exact_viewer.solid_committed_displayed ? 1 : 0)
            << '\n';
        CHECK(owner_exact_viewer.solid_committed_displayed);
        CHECK(owner_exact_viewer.solid_committed_style_expected);
        // A failure in optional Edge outlines must not hide the current
        // solid when switching among all supported view styles.
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_hidden_edges));
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded));
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_edges));
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        qInstallMessageHandler(old_qt_handler);
        const auto owner_skipped =
            owner_edge_outline_warnings.load(
                std::memory_order_relaxed);
        std::cerr
            << "PM05F_R2_OWNER_OUTLINE_SKIPPED="
            << owner_skipped << '\n';
        CHECK(owner_skipped == 0U);

        // R2 P0 Owner Part008: two opposite-direction Add Extrudes,
        // intersecting at the XY plane. This exact persisted profile
        // geometry differs from all prior concave Cut-pocket controls.
        // The private document/sketch UUIDs are replaced in the fixture.
        const auto crash_fixture =
            std::filesystem::path{__FILE__}.parent_path() /
            "fixtures" /
            "pm05f_r2_part008_sanitized.ss2part";
        const part::PartDocumentStore crash_store;
        auto crash_loaded = crash_store.load(crash_fixture);
        if (!crash_loaded.ok()) {
            std::cerr
                << "PM05F_R2_PART008_LOAD_FAILED"
                << " code="
                << static_cast<int>(crash_loaded.diagnostic.code)
                << " message="
                << crash_loaded.diagnostic.message
                << std::endl;
        }
        CHECK(crash_loaded.ok());
        application::DocumentSession crash_session{
            {}, std::move(*crash_loaded.document)};
        const auto crash_eval =
            part::evaluatePart(crash_session.document(), kernel);
        std::cerr
            << "PM05F_R2_PART008_EVALUATION"
            << " body_status="
            << static_cast<int>(crash_eval.body_status)
            << " features=" << crash_eval.features.size()
            << std::endl;
        CHECK(crash_eval.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(crash_eval.features.size() == 2U);
        CHECK(crash_eval.current_topology);
        CHECK(crash_eval.current_topology->complete());
        const auto crash_presentation =
            kernel.bodyPresentation(crash_eval.body_solid);
        CHECK(crash_presentation.ok());

        // Screenshot: selected count=1, radius=2, current stage
        // After Feature 2; carrier from Extrude001 (Feature 1),
        // side/side between Sketch entities 3 and 4. Resolve it
        // semantically from the current Body. Never persist a
        // provider runtime token or guess by XYZ nearest geometry.
        std::optional<part::MaterialEdgeReference>
            screenshot_edge;
        std::vector<viewer::Point3> screenshot_points;
        std::size_t matching_screenshot_edges = 0U;
        for (const auto& edge :
             crash_presentation.body.edges) {
            const auto found =
                std::find_if(
                    crash_eval.current_topology->edges.begin(),
                    crash_eval.current_topology->edges.end(),
                    [&edge](const auto& candidate) {
                        return candidate.runtime_token ==
                               edge.runtime_token;
                    });
            if (found ==
                crash_eval.current_topology->edges.end()) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *crash_eval.current_topology,
                    edge.runtime_token);
            if (!authored.ok()) {
                continue;
            }
            const auto& curve = authored.reference->curve;
            if (curve.producer_feature_id.serialized() != "1" ||
                curve.role != part::FeatureCurveRoleKind::side_side ||
                curve.adjacent_surfaces.size() != 2U) {
                continue;
            }
            std::vector<std::string> side_entities;
            for (const auto& adjacent :
                 curve.adjacent_surfaces) {
                if (adjacent.producer_feature_id.serialized() != "1" ||
                    adjacent.role !=
                        part::FeatureSurfaceRoleKind::side ||
                    !adjacent.source_entity) {
                    continue;
                }
                side_entities.push_back(
                    adjacent.source_entity->serialized());
            }
            std::sort(
                side_entities.begin(),
                side_entities.end());
            if (side_entities !=
                std::vector<std::string>{"3", "4"}) {
                continue;
            }
            ++matching_screenshot_edges;
            screenshot_edge = *authored.reference;
            screenshot_points.clear();
            for (const auto& point : edge.points) {
                screenshot_points.push_back({
                    point.x, point.y, point.z});
            }
            std::cerr
                << "PM05F_R2_PART008_SCREENSHOT_EDGE"
                << " runtime_token=" << edge.runtime_token.value
                << " point_count=" << edge.points.size()
                << std::endl;
        }
        CHECK(matching_screenshot_edges == 1U);
        CHECK(screenshot_edge);
        CHECK(screenshot_points.size() >= 2U);

        // First isolate the fully evaluated candidate and exact local
        // material delta from real pointer/GUI dispatch.
        auto screenshot_draft =
            application::FilletDraft::beginCreate(
                crash_session, {*screenshot_edge});
        CHECK(screenshot_draft);
        CHECK(screenshot_draft->setRadius(
            core::LengthValue{2.0}));
        const auto prior_revision =
            crash_session.document().revision();
        std::cerr
            << "PM05F_R2_PART008_DRAFT_BEGIN"
            << " radius=2 selection=1"
            << std::endl;
        const auto screenshot_preview =
            crash_session.evaluateFilletDraft(
                *screenshot_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_DRAFT_END"
            << " status="
            << static_cast<int>(screenshot_preview.status)
            << " target="
            << (screenshot_preview.target_status
                ? static_cast<int>(*screenshot_preview.target_status)
                : -1)
            << std::endl;
        CHECK(screenshot_preview.status ==
                  application::EdgeFeatureDraftEvaluationStatus::ok ||
              screenshot_preview.status ==
                  application::EdgeFeatureDraftEvaluationStatus::
                      target_failed);
        CHECK(crash_session.document().revision() ==
              prior_revision);

        // P0 Part008: this is the *second* edge transition missing
        // from FULL #1813. Candidate identities are obtained from
        // the current strict material Edge catalog. Prioritize true
        // two-producer Boolean intersections, then ordinary Edges.
        // Never persist or guess OCCT runtime tokens for authored CAD.
        struct Part008SecondaryEdge {
            part::MaterialEdgeReference reference;
            part::FeatureCurveRoleKind role;
            std::vector<viewer::Point3> points;
        };
        std::vector<Part008SecondaryEdge> secondary_edges;
        for (const auto& edge :
             crash_presentation.body.edges) {
            const auto authored =
                part::authorMaterialEdgeReference(
                    *crash_eval.current_topology,
                    edge.runtime_token);
            if (!authored.ok() ||
                *authored.reference == *screenshot_edge ||
                edge.points.size() < 2U) {
                continue;
            }
            const auto already_added =
                std::any_of(
                    secondary_edges.begin(),
                    secondary_edges.end(),
                    [&](const auto& item) {
                        return item.reference ==
                            *authored.reference;
                    });
            if (already_added) {
                continue;
            }
            Part008SecondaryEdge next{
                *authored.reference,
                authored.reference->curve.role,
                {}};
            for (const auto& point : edge.points) {
                next.points.push_back({
                    point.x, point.y, point.z});
            }
            secondary_edges.push_back(std::move(next));
        }
        std::stable_sort(
            secondary_edges.begin(),
            secondary_edges.end(),
            [](const auto& a, const auto& b) {
                const auto intersection =
                    part::FeatureCurveRoleKind::
                        boolean_intersection;
                return (a.role == intersection) &&
                       (b.role != intersection);
            });
        CHECK(!secondary_edges.empty());
        std::cerr
            << "PM05F_R2_PART008_SECOND_CANDIDATES"
            << " count=" << secondary_edges.size()
            << std::endl;

        // #1814 stopped on the SECOND candidate (index 1) while the
        // first candidate had completed. Isolate that one exact
        // semantic two-Edge pair without trying any other pair.
        constexpr std::size_t failing_candidate_index = 1U;
        CHECK(secondary_edges.size() > failing_candidate_index);
        const auto& isolated_secondary =
            secondary_edges[failing_candidate_index];
        std::cerr
            << "PM05F_R2_PART008_ISOLATED_CANDIDATE"
            << " index=" << failing_candidate_index
            << " role=" << static_cast<int>(isolated_secondary.role)
            << " producer="
            << isolated_secondary.reference.curve.producer_feature_id.serialized()
            << " points=" << isolated_secondary.points.size()
            << std::endl;
        for (const auto& surface :
             isolated_secondary.reference.curve.adjacent_surfaces) {
            std::cerr
                << "PM05F_R2_PART008_CANDIDATE_SURFACE"
                << " producer=" << surface.producer_feature_id.serialized()
                << " role=" << static_cast<int>(surface.role)
                << " sketch_entity="
                << (surface.source_entity
                    ? surface.source_entity->serialized()
                    : std::string{"none"})
                << std::endl;
        }
        for (const auto& point : isolated_secondary.points) {
            std::cerr
                << "PM05F_R2_PART008_CANDIDATE_POINT"
                << " x=" << point.x
                << " y=" << point.y
                << " z=" << point.z
                << std::endl;
        }

        const auto input =
            part::resolveKernelEdgeFeatureInput(
                part::FilletFeature{
                    {*screenshot_edge, isolated_secondary.reference},
                    core::LengthValue{2.0}},
                &*crash_eval.current_topology);
        CHECK(input.ok());
        CHECK(input.input->edges.size() == 2U);
        std::cerr
            << "PM05F_R2_PART008_KERNEL_DIRECT_BEGIN"
            << " edge0=" << input.input->edges[0].value
            << " edge1=" << input.input->edges[1].value
            << std::endl;
        const auto direct =
            kernel.edgeFeature(
                *input.input,
                crash_eval.body_solid);
        std::cerr
            << "PM05F_R2_PART008_KERNEL_DIRECT_END"
            << " status=" << static_cast<int>(direct.status)
            << std::endl;
        if (direct.status ==
                kernel::SolidModelingStatus::ok) {
            std::cerr
                << "PM05F_R2_PART008_DELTA_DIRECT_BEGIN"
                << std::endl;
            const auto delta =
                kernel.materialDifferencePreview(
                    crash_eval.body_solid, direct.solid);
            std::cerr
                << "PM05F_R2_PART008_DELTA_DIRECT_END"
                << " status=" << static_cast<int>(delta.status)
                << " removed=" << (delta.removed ? 1 : 0)
                << " added=" << (delta.added ? 1 : 0)
                << std::endl;
            CHECK(delta.status ==
                  kernel::SolidPresentationStatus::ok);
            CHECK(delta.removed.has_value());
            CHECK(delta.added.has_value());
            std::cerr
                << "PM05F_R2_PART008_RESULT_PRESENTATION_BEGIN"
                << std::endl;
            const auto result_presentation =
                kernel.bodyPresentation(direct.solid);
            std::cerr
                << "PM05F_R2_PART008_RESULT_PRESENTATION_END"
                << " status="
                << static_cast<int>(result_presentation.status)
                << " faces="
                << result_presentation.body.faces.size()
                << " edges="
                << result_presentation.body.edges.size()
                << std::endl;
            CHECK(result_presentation.ok());
        }

        auto isolated_draft =
            application::FilletDraft::beginCreate(
                crash_session, {*screenshot_edge});
        CHECK(isolated_draft);
        CHECK(isolated_draft->setRadius(core::LengthValue{2.0}));
        CHECK(isolated_draft->setEdges(
            {*screenshot_edge, isolated_secondary.reference}));
        std::cerr
            << "PM05F_R2_PART008_ISOLATED_DRAFT_BEGIN"
            << std::endl;
        const auto isolated_result =
            crash_session.evaluateFilletDraft(
                *isolated_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_ISOLATED_DRAFT_END"
            << " status=" << static_cast<int>(isolated_result.status)
            << std::endl;
        CHECK(isolated_result.status ==
                  application::EdgeFeatureDraftEvaluationStatus::ok ||
              isolated_result.status ==
                  application::EdgeFeatureDraftEvaluationStatus::
                      target_failed);
        CHECK(crash_session.document().revision() ==
              prior_revision);

        // Exercise the same single selected Edge through native Qt/OCCT
        // click routing. Query and hit-testing remain authoritative;
        // sample alternate standard views to find an unoccluded view.
        CHECK(workbench.activateDocument(&crash_session, {}));
        QApplication::processEvents();
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_edges));
        auto crash_reply = workbench.submitCadInput(
            "FILLET", workbench.cadInputContextGeneration());
        CHECK(crash_reply.accepted);
        crash_reply = workbench.lockCadDynamicInputField(
            0U, "2", workbench.cadInputContextGeneration());
        CHECK(crash_reply.accepted);
        std::cerr
            << "PM05F_R2_PART008_GUI_BEFORE_CLICK"
            << std::endl;
        bool clicked = false;
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_left,
                 viewer::StandardView::bottom_back_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (std::size_t k = 1U;
                 k < screenshot_points.size() &&
                 !clicked; ++k) {
                const auto& a = screenshot_points[k - 1U];
                const auto& b = screenshot_points[k];
                const viewer::Point3 point{
                    (a.x + b.x) / 2.0,
                    (a.y + b.y) / 2.0,
                    (a.z + b.z) / 2.0};
                if (!nativeClick(*viewport, point)) {
                    continue;
                }
                clicked = selectedCount(*label, 1);
                std::cerr
                    << "PM05F_R2_PART008_GUI_CLICK"
                    << " selected_one=" << clicked
                    << std::endl;
            }
            if (clicked) break;
        }
        CHECK(clicked);
        CHECK(selectedCount(*label, 1));
        std::cerr
            << "PM05F_R2_PART008_GUI_ONE_EDGE_PASS"
            << std::endl;

        // Attempt the transition selected 1 -> selected 2 through the
        // native pointer path (not synthetic draft setEdges). A selected
        // second Edge invokes Workbench's synchronous exact delta preview.
        // Log before each native click so a Debug CRT assertion isolates
        // the point of failure even if the process terminates.
        bool selected_two = false;
        for (std::size_t i = failing_candidate_index;
             i <= failing_candidate_index && !selected_two; ++i) {
            const auto& second = secondary_edges[i];
            for (const auto orientation : {
                     viewer::StandardView::top_front_right,
                     viewer::StandardView::top_front_left,
                     viewer::StandardView::top_back_left,
                     viewer::StandardView::top_back_right,
                     viewer::StandardView::bottom_front_left,
                     viewer::StandardView::bottom_back_right}) {
                CHECK(viewport->setStandardView(orientation));
                viewport->fitAll();
                QApplication::processEvents();
                for (std::size_t k = 1U;
                     k < second.points.size() && !selected_two;
                     ++k) {
                    const auto& a = second.points[k - 1U];
                    const auto& b = second.points[k];
                    const viewer::Point3 point{
                        (a.x + b.x) / 2.0,
                        (a.y + b.y) / 2.0,
                        (a.z + b.z) / 2.0};
                    const auto screen =
                        viewport->projectWorldPoint(point);
                    if (!screen) {
                        continue;
                    }
                    const QPoint pixel{
                        static_cast<int>(std::lround(screen->x)),
                        static_cast<int>(std::lround(screen->y))};
                    if (!viewport->rect().contains(pixel)) {
                        continue;
                    }
                    const auto query =
                        viewport->queryBodyTopology(
                            *screen,
                            viewer::BodyTopologyPickFilter{
                                false, true, false});
                    if (!query.valid() || !query.completed ||
                        query.candidates.empty()) {
                        continue;
                    }
                    std::cerr
                        << "PM05F_R2_PART008_SECOND_CLICK_BEGIN"
                        << " index=" << i
                        << " role=" << static_cast<int>(second.role)
                        << " segment=" << k
                        << std::endl;
                    QTest::mouseMove(viewport, pixel);
                    QTest::mouseClick(
                        viewport, Qt::LeftButton,
                        Qt::NoModifier, pixel);
                    QApplication::processEvents();
                    selected_two = selectedCount(*label, 2);
                    std::cerr
                        << "PM05F_R2_PART008_SECOND_CLICK_END"
                        << " index=" << i
                        << " selected_two=" << selected_two
                        << " selected_one=" << selectedCount(*label, 1)
                        << std::endl;
                    if (!selectedCount(*label, 1) &&
                        !selected_two) {
                        // A hit on the already selected Edge toggles
                        // it off. Do not silently assert we selected a
                        // second authored Edge; report diagnostic RED.
                        break;
                    }
                }
                if (!selectedCount(*label, 1) && !selected_two) {
                    break;
                }
                if (selected_two) break;
            }
            if (!selectedCount(*label, 1) && !selected_two) {
                break;
            }
        }
        CHECK(selected_two);
        std::cerr
            << "PM05F_R2_PART008_GUI_TWO_EDGE_PASS"
            << std::endl;
        CHECK(finish->isEnabled());
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_BEGIN"
            << " features="
            << crash_session.document().body().features.size()
            << std::endl;
        finish->click();
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_CLICK_RETURNED"
            << std::endl;
        QApplication::processEvents();
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_EVENTS_RETURNED"
            << std::endl;
        CHECK(crash_session.document().revision() !=
              prior_revision);
        CHECK(crash_session.document().body()
                  .features.size() == 3U);
        std::cerr
            << "PM05F_R2_PART008_REEVALUATION_BEGIN"
            << std::endl;
        const auto crash_committed =
            part::evaluatePart(crash_session.document(), kernel);
        std::cerr
            << "PM05F_R2_PART008_REEVALUATION_END"
            << " body_status="
            << static_cast<int>(crash_committed.body_status)
            << std::endl;
        CHECK(crash_committed.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(viewport->runtimeDiagnostics()
                  .solid_committed_displayed);
        std::cerr
            << "PM05F_R2_PART008_GUI_FINISH_PASS"
            << std::endl;

        // The Owner has PASS for this Part008 Fillet R2/Finish.
        // Independently cover the complementary equal-distance Chamfer
        // on the *same original two-Extrude* Body stage. Using a fresh
        // session prevents the new Chamfer from accidentally consuming
        // the committed Fillet stage above.
        auto chamfer_loaded = crash_store.load(crash_fixture);
        CHECK(chamfer_loaded.ok());
        application::DocumentSession chamfer_session{
            {}, std::move(*chamfer_loaded.document)};
        const auto chamfer_before_revision =
            chamfer_session.document().revision();
        auto chamfer_draft =
            application::ChamferDraft::beginCreate(
                chamfer_session,
                {*screenshot_edge, isolated_secondary.reference});
        CHECK(chamfer_draft);
        CHECK(chamfer_draft->setDistance(core::LengthValue{1.0}));
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_DRAFT_BEGIN"
            << std::endl;
        const auto chamfer_eval =
            chamfer_session.evaluateChamferDraft(
                *chamfer_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_DRAFT_END"
            << " status=" << static_cast<int>(chamfer_eval.status)
            << " removed=" << (chamfer_eval.preview_mesh ? 1 : 0)
            << " added="
            << (chamfer_eval.preview_added_mesh ? 1 : 0)
            << std::endl;
        CHECK(chamfer_eval.committable());
        CHECK(chamfer_eval.preview_mesh.has_value());
        CHECK(chamfer_eval.preview_added_mesh.has_value());
        CHECK(chamfer_session.document().revision() ==
              chamfer_before_revision);
        CHECK(chamfer_eval.body_solid);
        const auto chamfer_presentation =
            kernel.bodyPresentation(chamfer_eval.body_solid);
        CHECK(chamfer_presentation.ok());

        // Owner's new Chamfer corner report: inspect *actual* provider
        // surface kinds rather than infer curvature from displayed
        // triangle edges or interpolated shading. The generated corner
        // transition may be planar or nonplanar independently of the
        // flat equal-distance chamfers along each authored straight Edge.
        const auto chamfer_kernel_input =
            part::resolveKernelEdgeFeatureInput(
                part::ChamferFeature{
                    {*screenshot_edge, isolated_secondary.reference},
                    core::LengthValue{1.0}},
                &*crash_eval.current_topology);
        CHECK(chamfer_kernel_input.ok());
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_PAIR_KERNEL_BEGIN"
            << std::endl;
        const auto chamfer_kernel_result =
            kernel.edgeFeature(
                *chamfer_kernel_input.input,
                crash_eval.body_solid);
        CHECK(chamfer_kernel_result.ok());
        std::size_t chamfer_corner_plane = 0U;
        std::size_t chamfer_corner_curved = 0U;
        std::size_t chamfer_edge_plane = 0U;
        std::size_t chamfer_edge_curved = 0U;
        for (const auto& carrier :
             chamfer_kernel_result.edge_feature_surfaces) {
            const bool planar =
                carrier.surface_kind == kernel::SurfaceKind::plane;
            if (carrier.kind ==
                kernel::EdgeFeatureGeneratedSurfaceKind::
                    corner_transition) {
                planar ? ++chamfer_corner_plane :
                    ++chamfer_corner_curved;
            } else {
                planar ? ++chamfer_edge_plane :
                    ++chamfer_edge_curved;
            }
        }
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_PAIR_SURFACES"
            << " corner_plane=" << chamfer_corner_plane
            << " corner_nonplane=" << chamfer_corner_curved
            << " edge_plane=" << chamfer_edge_plane
            << " edge_nonplane=" << chamfer_edge_curved
            << std::endl;
        CHECK(!chamfer_kernel_result.edge_feature_surfaces.empty());

        // The Owner's full screenshot identifies the third selected
        // Edge by strict authored lineage: Extrude001, Side/Entity 1
        // + Side/Entity 4. This was NOT the prior test's sides 1+2.
        // Reproduce the exact 3-Edge authored semantic set on Part008,
        // without relying on provider runtime token or XYZ proximity.
        std::optional<part::MaterialEdgeReference> third_owner_edge;
        for (const auto& probe : secondary_edges) {
            const auto& curve = probe.reference.curve;
            if (curve.producer_feature_id.serialized() != "1" ||
                curve.role != part::FeatureCurveRoleKind::side_side ||
                curve.adjacent_surfaces.size() != 2U) {
                continue;
            }
            std::vector<std::string> side_entities;
            for (const auto& adjacent : curve.adjacent_surfaces) {
                if (adjacent.producer_feature_id.serialized() != "1" ||
                    adjacent.role != part::FeatureSurfaceRoleKind::side ||
                    !adjacent.source_entity) continue;
                side_entities.push_back(
                    adjacent.source_entity->serialized());
            }
            std::sort(side_entities.begin(), side_entities.end());
            if (side_entities ==
                std::vector<std::string>{"1", "4"}) {
                third_owner_edge = probe.reference;
                break;
            }
        }
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THIRD"
            << " referenceable=" << (third_owner_edge ? 1 : 0)
            << std::endl;
        CHECK(third_owner_edge);
        CHECK(*third_owner_edge != *screenshot_edge);
        CHECK(*third_owner_edge != isolated_secondary.reference);

        auto triple_loaded = crash_store.load(crash_fixture);
        CHECK(triple_loaded.ok());
        application::DocumentSession triple_session{
            {}, std::move(*triple_loaded.document)};
        const auto triple_source_revision =
            triple_session.document().revision();

        auto lone_draft =
            application::ChamferDraft::beginCreate(
                triple_session, {*third_owner_edge});
        CHECK(lone_draft);
        CHECK(lone_draft->setDistance(
            core::LengthValue{1.0}));
        const auto lone =
            triple_session.evaluateChamferDraft(
                *lone_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_THIRD_ALONE"
            << " status=" << static_cast<int>(lone.status)
            << " target="
            << (lone.target_status
                ? static_cast<int>(*lone.target_status)
                : -1)
            << " committable=" << (lone.committable() ? 1 : 0)
            << std::endl;

        auto three_draft =
            application::ChamferDraft::beginCreate(
                triple_session,
                {*screenshot_edge, isolated_secondary.reference,
                 *third_owner_edge});
        CHECK(three_draft);
        CHECK(three_draft->setDistance(
            core::LengthValue{1.0}));
        const auto before_triple_kernel =
            part::resolveKernelEdgeFeatureInput(
                part::ChamferFeature{
                    {*screenshot_edge, isolated_secondary.reference,
                     *third_owner_edge},
                    core::LengthValue{1.0}},
                &*crash_eval.current_topology);
        CHECK(before_triple_kernel.ok());
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THREE_BEGIN"
            << std::endl;
        const auto kernel_three =
            kernel.edgeFeature(
                *before_triple_kernel.input,
                crash_eval.body_solid);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THREE_KERNEL"
            << " status=" << static_cast<int>(kernel_three.status)
            << " solid_count=" << kernel_three.solid_count
            << " brep_valid=" << (kernel_three.brep_valid ? 1 : 0)
            << std::endl;
        const auto three =
            triple_session.evaluateChamferDraft(
                *three_draft, kernel);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_THREE_END"
            << " status=" << static_cast<int>(three.status)
            << " target="
            << (three.target_status
                ? static_cast<int>(*three.target_status)
                : -1)
            << " removed=" << (three.preview_mesh ? 1 : 0)
            << " added=" << (three.preview_added_mesh ? 1 : 0)
            << std::endl;
        CHECK(triple_session.document().revision() ==
              triple_source_revision);
        CHECK(lone.committable());

        CHECK(kernel_three.ok());
        CHECK(three.committable());
        // A mixed signed CSG result must expose *both* exact local
        // material changes to the Workbench, not merely a valid B-Rep.
        // Guard the D2-B requirement after the production fallback.
        CHECK(three.preview_mesh.has_value());
        CHECK(three.preview_added_mesh.has_value());
        CHECK(three.preview_mesh->valid());
        CHECK(three.preview_added_mesh->valid());
        CHECK(three.previewSolidAvailable());

        // D2-B negative admission: never deduplicate authored Edges,
        // guess a missing reference or publish a partial fallback.
        auto duplicate_edge_input = *before_triple_kernel.input;
        CHECK(duplicate_edge_input.edges.size() == 3U);
        duplicate_edge_input.edges[2] =
            duplicate_edge_input.edges.front();
        CHECK(!duplicate_edge_input.valid());
        const auto duplicate_edge_result =
            kernel.edgeFeature(
                duplicate_edge_input, crash_eval.body_solid);
        CHECK(!duplicate_edge_result.ok());
        CHECK(duplicate_edge_result.status ==
              kernel::SolidModelingStatus::invalid_input);
        CHECK(!duplicate_edge_result.solid);

        // Strict runtime-stage membership: a structurally valid but
        // unknown Edge token fails; no XYZ or similarity rebind exists.
        auto missing_edge_input = *before_triple_kernel.input;
        std::uint64_t highest_token = 0U;
        for (const auto& current :
             crash_eval.current_topology->edges) {
            highest_token = std::max(
                highest_token, current.runtime_token.value);
        }
        CHECK(highest_token <
              std::numeric_limits<std::uint64_t>::max());
        const auto missing_edge_token =
            kernel::RuntimeEdgeToken{highest_token + 1U};
        CHECK(missing_edge_token.valid());
        missing_edge_input.edges[2] = missing_edge_token;
        CHECK(missing_edge_input.valid());
        const auto missing_edge_result =
            kernel.edgeFeature(
                missing_edge_input, crash_eval.body_solid);
        CHECK(!missing_edge_result.ok());
        CHECK(missing_edge_result.status ==
              kernel::SolidModelingStatus::provider_mismatch);
        CHECK(!missing_edge_result.solid);
        CHECK(triple_session.document().revision() ==
              triple_source_revision);

        // D2-B Owner Part008 strict permutation/size and full current
        // Face/Edge/Vertex inventory. Transient runtime tokens may order
        // evaluation but must never become durable source identity.
        auto trial_edges = before_triple_kernel.input->edges;
        std::sort(
            trial_edges.begin(), trial_edges.end(),
            [](const auto& a, const auto& b) {
                return a.value < b.value;
            });
        std::size_t passing_variants = 0U;
        do {
            for (const double d : {1.0, 0.5, 0.25}) {
                auto input = *before_triple_kernel.input;
                input.edges = trial_edges;
                input.parameter_mm = d;
                const auto trial =
                    kernel.edgeFeature(input, crash_eval.body_solid);
                CHECK(trial.ok());
                CHECK(trial.edge_feature_input_membership);
                CHECK(trial.edge_feature_input_membership
                          ->exactFor(input.edges));
                CHECK(trial.face_count == 21U);
                CHECK(trial.current_faces.size() == trial.face_count);
                CHECK(trial.current_edges.size() == trial.edge_count);
                CHECK(trial.current_vertices.size() ==
                      trial.vertex_count);
                CHECK(trial.current_edge_semantics.size() ==
                      trial.edge_count);
                CHECK(trial.current_vertex_semantics.size() ==
                      trial.vertex_count);
                CHECK(trial.edge_feature_surfaces.size() == 5U);
                // Check strict provenance, not merely that five output
                // Surfaces happened to be present in the result B-Rep.
                std::vector<kernel::RuntimeEdgeToken>
                    certified_strip_sources;
                std::vector<kernel::RuntimeVertexToken>
                    certified_corner_sources;
                for (const auto& surface :
                     trial.edge_feature_surfaces) {
                    CHECK(surface.valid());
                    CHECK(surface.operation ==
                          kernel::EdgeFeatureOperation::chamfer);
                    CHECK(surface.surface_kind ==
                          kernel::SurfaceKind::plane);
                    CHECK(surface.current_faces.size() == 1U);
                    if (surface.kind ==
                        kernel::EdgeFeatureGeneratedSurfaceKind::
                            edge_transition) {
                        CHECK(surface.source_edge.has_value());
                        CHECK(std::find(
                                  input.edges.begin(), input.edges.end(),
                                  *surface.source_edge) !=
                              input.edges.end());
                        CHECK(std::find(
                                  certified_strip_sources.begin(),
                                  certified_strip_sources.end(),
                                  *surface.source_edge) ==
                              certified_strip_sources.end());
                        certified_strip_sources.push_back(
                            *surface.source_edge);
                    } else {
                        CHECK(surface.kind ==
                              kernel::EdgeFeatureGeneratedSurfaceKind::
                                  corner_transition);
                        CHECK(surface.source_vertex.has_value());
                        CHECK(surface.incident_source_edges.size() == 2U);
                        CHECK(surface.incident_source_edges[0] !=
                              surface.incident_source_edges[1]);
                        for (const auto edge :
                             surface.incident_source_edges) {
                            CHECK(std::find(
                                      input.edges.begin(),
                                      input.edges.end(), edge) !=
                                  input.edges.end());
                        }
                        CHECK(std::find(
                                  certified_corner_sources.begin(),
                                  certified_corner_sources.end(),
                                  *surface.source_vertex) ==
                              certified_corner_sources.end());
                        // A corner's claimed Edge pair must be the
                        // exact selected pair incident at its *source*
                        // Vertex, not merely two authored Edges somewhere
                        // in the same Body. All tokens are stage-local.
                        const auto source_vertex = std::find_if(
                            crash_eval.current_topology->vertices.begin(),
                            crash_eval.current_topology->vertices.end(),
                            [&](const auto& vertex) {
                                return vertex.runtime_token ==
                                    *surface.source_vertex;
                            });
                        CHECK(source_vertex !=
                              crash_eval.current_topology->vertices.end());
                        std::size_t selected_incidence = 0U;
                        for (const auto edge : input.edges) {
                            const bool incident = std::find(
                                source_vertex->incident_material_edges.begin(),
                                source_vertex->incident_material_edges.end(),
                                edge) !=
                                source_vertex->incident_material_edges.end();
                            if (!incident) continue;
                            ++selected_incidence;
                            CHECK(std::find(
                                      surface.incident_source_edges.begin(),
                                      surface.incident_source_edges.end(),
                                      edge) !=
                                  surface.incident_source_edges.end());
                        }
                        CHECK(selected_incidence ==
                              surface.incident_source_edges.size());
                        certified_corner_sources.push_back(
                            *surface.source_vertex);
                    }
                }
                CHECK(certified_strip_sources.size() ==
                      input.edges.size());
                CHECK(certified_corner_sources.size() == 2U);
                for (const auto& face : trial.current_faces) {
                    std::size_t owners = 0U;
                    for (const auto& surface :
                         trial.inherited_surfaces) {
                        if (surface.surface_status ==
                                kernel::ReferenceStatus::resolved &&
                            std::find(
                                surface.current_faces.begin(),
                                surface.current_faces.end(),
                                face) != surface.current_faces.end()) {
                            ++owners;
                        }
                    }
                    for (const auto& surface :
                         trial.edge_feature_surfaces) {
                        if (std::find(
                                surface.current_faces.begin(),
                                surface.current_faces.end(),
                                face) != surface.current_faces.end()) {
                            ++owners;
                        }
                    }
                    CHECK(owners == 1U);
                }
                ++passing_variants;
            }
        } while (std::next_permutation(
            trial_edges.begin(), trial_edges.end(),
            [](const auto& a, const auto& b) {
                return a.value < b.value;
            }));
        CHECK(passing_variants == 18U);

        // D2-B one-authored-Feature lifecycle, using a new temporary native
        // file. The original sanitized Owner fixture is never mutated.
        QTemporaryDir lifecycle_dir;
        CHECK(lifecycle_dir.isValid());
        const auto lifecycle_path =
            std::filesystem::path{
                lifecycle_dir.path().toStdWString()} /
            "part008_d2b_three_edge_chamfer.ss2part";
        auto lifecycle_source = crash_store.load(crash_fixture);
        CHECK(lifecycle_source.ok());
        const auto initial_file = crash_store.createNew(
            lifecycle_path, *lifecycle_source.document);
        CHECK(initial_file.ok());
        application::DocumentSession lifecycle{
            lifecycle_path,
            std::move(*lifecycle_source.document),
            *initial_file.checkpoint};
        std::vector<part::MaterialEdgeReference>
            authored_three{
                *screenshot_edge,
                isolated_secondary.reference,
                *third_owner_edge};
        // Direct Commands require canonical authored Edge order; Draft
        // normalizes the user's click sequence before constructing them.
        // A raw unsorted command must fail closed with no mutation.
        const auto lifecycle_before_invalid =
            lifecycle.document().revision();
        CHECK(!std::is_sorted(
            authored_three.begin(), authored_three.end()));
        const auto noncanonical = lifecycle.execute(
            application::CreateChamferFeatureCommand{
                authored_three,
                lifecycle.document().revision(),
                core::LengthValue{1.0},
                "Noncanonical input must fail"},
            kernel);
        CHECK(!noncanonical.ok());
        CHECK(!noncanonical.changed);
        CHECK(lifecycle.document().revision() ==
              lifecycle_before_invalid);
        CHECK(lifecycle.document().body().features.size() == 2U);
        std::sort(authored_three.begin(), authored_three.end());
        const auto create_feature =
            lifecycle.execute(
                application::CreateChamferFeatureCommand{
                    authored_three,
                    lifecycle.document().revision(),
                    core::LengthValue{1.0},
                    "D2B Three-Edge Chamfer"},
                kernel);
        std::cerr
            << "PM05F_R2_PART008_D2B_CREATE_DIAGNOSTIC"
            << " changed=" << create_feature.changed
            << " code="
            << static_cast<int>(create_feature.diagnostic.code)
            << " commit_code="
            << static_cast<int>(create_feature.diagnostic.commit_code)
            << " evaluation_diagnostic="
            << (create_feature.evaluation_diagnostic
                ? static_cast<int>(*create_feature.evaluation_diagnostic)
                : -1)
            << " message=" << create_feature.diagnostic.message
            << std::endl;
        CHECK(create_feature.ok());
        CHECK(create_feature.changed);
        CHECK(create_feature.feature_id);
        CHECK(lifecycle.document().body().features.size() == 3U);
        const auto check_live_three = [&]() {
            const auto evaluated = part::evaluatePart(
                lifecycle.document(), kernel);
            CHECK(evaluated.body_status ==
                  part::BodyEvaluationStatus::up_to_date);
            CHECK(evaluated.body_solid);
            CHECK(evaluated.current_topology);
            CHECK(evaluated.current_topology->complete());
            const auto* feature =
                lifecycle.document().findFeature(
                    *create_feature.feature_id);
            CHECK(feature != nullptr);
            const auto* chamfer =
                std::get_if<part::ChamferFeature>(
                    &feature->definition);
            CHECK(chamfer != nullptr);
            CHECK(chamfer->edges.size() == 3U);
        };
        check_live_three();

        CHECK(lifecycle.undo().ok());
        CHECK(lifecycle.document().body().features.size() == 2U);
        CHECK(lifecycle.redo().ok());
        check_live_three();

        // Edit preview must use the exact authored source stage,
        // preserve DocumentRevision and expose both signed deltas
        // for this mixed-material three-Edge Chamfer.
        const auto before_edit_draft_revision =
            lifecycle.document().revision();
        auto edit_draft_three =
            application::ChamferDraft::beginEdit(
                lifecycle, *create_feature.feature_id);
        CHECK(edit_draft_three);
        CHECK(edit_draft_three->setDistance(
            core::LengthValue{0.5}));
        const auto preview_edit_three =
            lifecycle.evaluateChamferDraft(
                *edit_draft_three, kernel);
        CHECK(preview_edit_three.committable());
        CHECK(preview_edit_three.preview_mesh.has_value());
        CHECK(preview_edit_three.preview_added_mesh.has_value());
        CHECK(preview_edit_three.previewSolidAvailable());
        CHECK(lifecycle.document().revision() ==
              before_edit_draft_revision);

        const auto edit_feature =
            lifecycle.execute(
                application::EditChamferFeatureCommand{
                    *create_feature.feature_id,
                    authored_three,
                    lifecycle.document().revision(),
                    core::LengthValue{0.5},
                    "D2B Three-Edge Chamfer Edited"},
                kernel);
        CHECK(edit_feature.ok());
        CHECK(edit_feature.changed);
        check_live_three();
        // An Edit draft evaluated before the committed transaction
        // is stale afterwards; its Finish must not commit a second edit.
        const auto revision_after_edit =
            lifecycle.document().revision();
        const auto stale_finish_three =
            application::finishChamferDraft(
                lifecycle, *edit_draft_three,
                preview_edit_three, kernel);
        CHECK(!stale_finish_three.ok());
        CHECK(!stale_finish_three.changed);
        CHECK(stale_finish_three.status ==
              application::EdgeFeatureDraftFinishStatus::
                  stale_context);
        CHECK(lifecycle.document().revision() ==
              revision_after_edit);
        CHECK(lifecycle.document().body().features.size() == 3U);
        const auto* edited =
            std::get_if<part::ChamferFeature>(
                &lifecycle.document().findFeature(
                    *create_feature.feature_id)->definition);
        CHECK(edited != nullptr);
        CHECK(edited->distance == core::LengthValue{0.5});

        CHECK(lifecycle.undo().ok());
        check_live_three();
        const auto* restored =
            std::get_if<part::ChamferFeature>(
                &lifecycle.document().findFeature(
                    *create_feature.feature_id)->definition);
        CHECK(restored != nullptr);
        CHECK(restored->distance == core::LengthValue{1.0});
        CHECK(lifecycle.redo().ok());
        check_live_three();
        CHECK(lifecycle.save().ok());
        CHECK(!lifecycle.needsSave());

        auto reopened = crash_store.load(lifecycle_path);
        CHECK(reopened.ok());
        CHECK(reopened.document->state() ==
              lifecycle.document().state());
        const auto reopened_eval =
            part::evaluatePart(*reopened.document, kernel);
        CHECK(reopened_eval.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(reopened_eval.body_solid);
        CHECK(reopened_eval.current_topology);
        CHECK(reopened_eval.current_topology->complete());
        std::cerr
            << "PM05F_R2_PART008_D2B_LIFECYCLE_PASS"
            << " create=1 edit=1 undo=1 redo=1"
            << " save=1 reopen=1 replay=1"
            << std::endl;

        std::cerr
            << "PM05F_R2_PART008_D2B_PRODUCTION_MATRIX"
            << " passing=" << passing_variants
            << " face_count=21 complete_lineage=1"
            << std::endl;

        std::cerr
            << "PM05F_R2_PART008_CHAMFER_OWNER_CORNER_EXPECTATION"
            << " corner_planar=" << chamfer_corner_plane
            << " corner_nonplanar=" << chamfer_corner_curved
            << " edge_planar=" << chamfer_edge_plane
            << " edge_nonplanar=" << chamfer_edge_curved
            << std::endl;

        // Exercise the real Qt/OCCT Chamfer picker and Finish, rather
        // than passing a fabricated PresentationToken or issuing a
        // direct persistence command in lieu of the Owner interaction.
        CHECK(workbench.activateDocument(&chamfer_session, {}));
        QApplication::processEvents();
        CHECK(viewport->setViewStyle(
            viewer::ViewStyle::shaded_with_edges));
        auto chamfer_reply = workbench.submitCadInput(
            "CHAMFER", workbench.cadInputContextGeneration());
        CHECK(chamfer_reply.accepted);
        chamfer_reply = workbench.lockCadDynamicInputField(
            0U, "1", workbench.cadInputContextGeneration());
        CHECK(chamfer_reply.accepted);
        bool chamfer_first_selected = false;
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_left,
                 viewer::StandardView::bottom_back_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (std::size_t k = 1U;
                 k < screenshot_points.size() &&
                 !chamfer_first_selected; ++k) {
                const auto& a = screenshot_points[k - 1U];
                const auto& b = screenshot_points[k];
                const viewer::Point3 midpoint{
                    (a.x + b.x) / 2.0,
                    (a.y + b.y) / 2.0,
                    (a.z + b.z) / 2.0};
                if (nativeClick(*viewport, midpoint)) {
                    chamfer_first_selected = selectedCount(*label, 1);
                }
            }
            if (chamfer_first_selected) break;
        }
        CHECK(chamfer_first_selected);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_ONE_EDGE"
            << std::endl;

        bool chamfer_two_selected = false;
        for (const auto orientation : {
                 viewer::StandardView::top_front_right,
                 viewer::StandardView::top_front_left,
                 viewer::StandardView::top_back_left,
                 viewer::StandardView::top_back_right,
                 viewer::StandardView::bottom_front_left,
                 viewer::StandardView::bottom_back_right}) {
            CHECK(viewport->setStandardView(orientation));
            viewport->fitAll();
            QApplication::processEvents();
            for (std::size_t k = 1U;
                 k < isolated_secondary.points.size() &&
                 !chamfer_two_selected; ++k) {
                const auto& a = isolated_secondary.points[k - 1U];
                const auto& b = isolated_secondary.points[k];
                const viewer::Point3 midpoint{
                    (a.x + b.x) / 2.0,
                    (a.y + b.y) / 2.0,
                    (a.z + b.z) / 2.0};
                if (!nativeClick(*viewport, midpoint)) {
                    continue;
                }
                chamfer_two_selected = selectedCount(*label, 2);
                if (!chamfer_two_selected &&
                    !selectedCount(*label, 1)) {
                    // The click toggled OFF the first material Edge.
                    // Do not claim a 2-Edge successful native pick.
                    break;
                }
            }
            if (chamfer_two_selected ||
                !selectedCount(*label, 1)) break;
        }
        CHECK(chamfer_two_selected);
        CHECK(finish->isEnabled());
        CHECK(viewport->runtimeDiagnostics().solid_preview_displayed);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_FINISH_BEGIN"
            << std::endl;
        finish->click();
        QApplication::processEvents();
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_FINISH_RETURN"
            << std::endl;
        CHECK(chamfer_session.document().revision() !=
              chamfer_before_revision);
        CHECK(chamfer_session.document().body()
                  .features.size() == 3U);
        const auto* persisted_chamfer =
            std::get_if<part::ChamferFeature>(
                &chamfer_session.document().body()
                    .features.back().definition);
        CHECK(persisted_chamfer != nullptr);
        CHECK(persisted_chamfer->edges.size() == 2U);
        const auto chamfer_committed =
            part::evaluatePart(chamfer_session.document(), kernel);
        CHECK(chamfer_committed.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(viewport->runtimeDiagnostics().solid_committed_displayed);
        std::cerr
            << "PM05F_R2_PART008_CHAMFER_FINISH_PASS"
            << std::endl;

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
