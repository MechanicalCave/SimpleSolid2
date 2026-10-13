#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04C2 OCCT Revolve CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double pi =
    std::numbers::pi_v<double>;
constexpr double full_turn =
    2.0 * pi;

kernel::BoundaryUse2D lineUse(
    std::string source,
    std::uint32_t use_index,
    kernel::Point2 start,
    kernel::Point2 end) {
    kernel::BoundaryUse2D use;
    use.curve =
        kernel::Line2{start, end};
    use.start_parameter = 0.0;
    use.end_parameter = 1.0;
    use.follows_source_direction = true;
    use.provenance = {
        std::move(source),
        0U,
        use_index,
        false};
    return use;
}

kernel::PlanarProfileInput rectangle(
    double min_u,
    double max_u,
    double min_v,
    double max_v) {
    kernel::PlanarProfileInput input;
    input.frame = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}};
    input.outer.boundary = {
        lineUse(
            "bottom",
            0U,
            {min_u, min_v},
            {max_u, min_v}),
        lineUse(
            "right",
            1U,
            {max_u, min_v},
            {max_u, max_v}),
        lineUse(
            "top",
            2U,
            {max_u, max_v},
            {min_u, max_v}),
        lineUse(
            "left",
            3U,
            {min_u, max_v},
            {min_u, min_v}),
    };
    CHECK(input.valid());
    return input;
}

kernel::AngularRevolveInput revolveInput(
    double start,
    double end,
    kernel::SolidBooleanOperation operation =
        kernel::SolidBooleanOperation::add) {
    kernel::AngularRevolveInput input;
    input.profile =
        rectangle(
            10.0,
            20.0,
            0.0,
            10.0);
    input.axis = {
        {0.0, 0.0, 0.0},
        {0.0, 1.0, 0.0}};
    input.start_angle_radians = start;
    input.end_angle_radians = end;
    input.operation = operation;
    CHECK(input.valid());
    return input;
}

const kernel::NewSurfaceLineage*
surfaceByRole(
    const kernel::SolidModelingResult& result,
    kernel::GeneratedFaceRoleKind role) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [role](
                const kernel::NewSurfaceLineage& surface) {
                return surface.role.kind == role;
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::NewSurfaceLineage*
sideBySource(
    const kernel::SolidModelingResult& result,
    const std::string& source) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [&source](
                const kernel::NewSurfaceLineage& surface) {
                return surface.role.kind ==
                           kernel::GeneratedFaceRoleKind::
                               revolve_side &&
                       surface.role.side_provenance &&
                       surface.role.side_provenance
                               ->source_entity ==
                           source;
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

void verifyCompleteRuntimeInventory(
    const kernel::SolidModelingResult& result) {
    CHECK(result.ok());
    CHECK(
        result.current_faces.size() ==
        result.face_count);
    CHECK(
        result.current_edges.size() ==
        result.edge_count);
    CHECK(
        result.current_vertices.size() ==
        result.vertex_count);
    CHECK(
        result.current_edge_semantics.size() ==
        result.current_edges.size());
    CHECK(
        result.current_vertex_semantics.size() ==
        result.current_vertices.size());
}

kernel::LinearExtrudeInput boxInput(
    double half_xy,
    double start_z,
    double end_z) {
    kernel::LinearExtrudeInput input;
    input.profile =
        rectangle(
            -half_xy,
            half_xy,
            -half_xy,
            half_xy);
    input.start_offset_mm = start_z;
    input.end_offset_mm = end_z;
    input.start_cap_role =
        kernel::ExtrudeCapRole::negative_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::positive_cap;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());
    return input;
}

part::PartDocument makePartRevolve(
    core::BuiltinReferenceRole axis_role,
    sketch::Point2 first,
    sketch::Point2 opposite,
    double angle) {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto rectangle_created =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                first,
                opposite,
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle_created.ok());

    const auto* sketch =
        session.document().findSketch(
            sketch_id);
    CHECK(sketch != nullptr);
    const auto analysis =
        sketch::analyzeRegions(
            sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "Revolve001",
            false,
            part::RevolveFeature{
                *profile.profile_id,
                part::AxisReference{
                    part::BuiltinOriginAxisReference{
                        axis_role}},
                part::RevolveOperation::add,
                part::OneSidedRevolveExtent{
                    core::AngleValue{angle},
                    false}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

part::PartDocument makeExtrudeCylinder() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

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
                10.0,
                sketch::EntityRole::regular});
    CHECK(circle.ok());
    const auto* model =
        session.document().findSketch(
            *sketch.sketch_id);
    CHECK(model != nullptr);
    const auto regions =
        sketch::analyzeRegions(model->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());
    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto id = state.body.next_feature_id.allocate();
    CHECK(id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *id,
            "ExtrudeCylinder",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}});
    auto restored = part::PartDocument::restore(
        session.document().documentId(),
        std::move(state),
        session.document().revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

// G0/V08: a real analytic cylinder clipped by one planar Boolean
// Cut through its axis. Two distinct native rim Vertices can share the
// same THREE semantic Surface sources (cap, cylinder, cut wall).
part::PartDocument withCylinderHalfCut(
    const part::PartDocument& source) {
    auto restored = part::PartDocument::restore(
        source.documentId(),
        source.state(),
        source.revision());
    CHECK(restored.ok());
    application::DocumentSession session{
        {},
        std::move(*restored.document)};
    const auto sketch_created = session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto rectangle = session.execute(
        application::AddSketchRectangleCommand{
            *sketch_created.sketch_id,
            session.document().revision(),
            {-15.0, -15.0},
            {0.0, 15.0},
            sketch::EntityRole::regular,
            false});
    CHECK(rectangle.ok());
    const auto* sketch = session.document().findSketch(
        *sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto analysis = sketch::analyzeRegions(
        sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent = part::makeProfileRegionIntent(
        analysis.regions.front());
    CHECK(intent.has_value());
    const auto profile = session.execute(
        application::CreateProfileCommand{
            *sketch_created.sketch_id,
            session.document().revision(),
            *intent});
    CHECK(profile.ok() && profile.profile_id);
    auto state = session.document().state();
    const auto feature_id =
        state.body.next_feature_id.allocate();
    CHECK(feature_id.has_value());
    state.body.features.push_back(
        part::PartFeature{
            *feature_id,
            "G0HalfCylinderCut",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{12.0},
                    false}}});
    auto rebuilt = part::PartDocument::restore(
        session.document().documentId(),
        std::move(state),
        session.document().revision());
    CHECK(rebuilt.ok());
    return std::move(*rebuilt.document);
}

std::optional<part::MaterialEdgeReference>
firstAuthorableCircle(
    const part::BodyStageTopologyCatalog& catalog) {
    for (const auto& edge : catalog.edges) {
        if (edge.curve_kind !=
                kernel::CurveKind::circle) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog, edge.runtime_token);
        if (authored.ok()) {
            return authored.reference;
        }
    }
    return std::nullopt;
}

void verifyCylinderEdgeFeature(
    const part::PartDocument& source,
    const part::MaterialEdgeReference& edge,
    kernel::EdgeFeatureOperation operation,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto state = source.state();
    const auto id = state.body.next_feature_id.allocate();
    CHECK(id.has_value());
    part::PartFeature feature;
    feature.id = *id;
    feature.name = "CircularBoundary";
    if (operation ==
        kernel::EdgeFeatureOperation::fillet) {
        feature.definition = part::FilletFeature{
            {edge}, core::LengthValue{1.0}};
    } else {
        feature.definition = part::ChamferFeature{
            {edge}, core::LengthValue{1.0}};
    }
    state.body.features.push_back(
        std::move(feature));
    auto restored = part::PartDocument::restore(
        source.documentId(),
        std::move(state),
        source.revision());
    CHECK(restored.ok());
    const auto evaluated =
        part::evaluatePart(
            *restored.document, kernel);
    CHECK(
        evaluated.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated.features.size() == 2U);
    CHECK(
        evaluated.features.back().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(evaluated.current_topology.has_value());
    CHECK(evaluated.current_topology->complete());
    // A new evaluation has no authority to rely on a previous provider
    // session/preview identity.
    const auto rebuilt =
        part::evaluatePart(
            *restored.document, kernel);
    CHECK(rebuilt.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(rebuilt.current_topology.has_value());
    CHECK(rebuilt.current_topology->complete());
}

std::size_t countSurfaceRole(
    const part::FeatureEvaluation& feature,
    part::FeatureSurfaceRoleKind role) {
    return static_cast<std::size_t>(
        std::count_if(
            feature.produced_surfaces.begin(),
            feature.produced_surfaces.end(),
            [role](const auto& surface) {
                return surface.address.role ==
                       role;
            }));
}

// RG-02A/G0 V04: independent real native OCCT and Part semantic inventories.
// None of these runtime tokens or provider XYZ values becomes authored intent.
struct G0RevolveVertexCounts final {
    std::size_t native{};
    std::size_t resolved{};
    std::size_t missing{};
    std::size_t ambiguous{};
    std::size_t unsupported{};
    std::size_t three_surface_witnesses{};
};

G0RevolveVertexCounts observeRevolvePartVertices(
    const part::BodyStageTopologyCatalog& catalog,
    const char* fixture) {
    CHECK(catalog.valid());
    CHECK(catalog.complete());
    G0RevolveVertexCounts counts;
    counts.native = catalog.vertices.size();
    for (const auto& vertex : catalog.vertices) {
        CHECK(vertex.runtime_token.valid());
        CHECK(
            vertex.accounting_class !=
            part::TopologyAccountingClass::integrity_failure);
        switch (vertex.referenceability) {
        case kernel::ReferenceStatus::resolved: {
            ++counts.resolved;
            CHECK(vertex.point_candidates.size() == 1U);
            const auto& address =
                vertex.point_candidates.front();
            CHECK(address.valid());
            CHECK(address.adjacent_surfaces.size() == 3U);
            const auto point = std::find_if(
                catalog.points.begin(),
                catalog.points.end(),
                [&address](const auto& candidate) {
                    return candidate.address == address;
                });
            CHECK(point != catalog.points.end());
            CHECK(point->status ==
                  kernel::ReferenceStatus::resolved);
            CHECK(point->current_vertices.size() == 1U);
            CHECK(point->current_vertices.front() ==
                  vertex.runtime_token);
            ++counts.three_surface_witnesses;
            break;
        }
        case kernel::ReferenceStatus::missing:
            ++counts.missing;
            break;
        case kernel::ReferenceStatus::ambiguous:
            ++counts.ambiguous;
            break;
        case kernel::ReferenceStatus::unsupported:
            ++counts.unsupported;
            break;
        }
    }
    CHECK(counts.native ==
          counts.resolved + counts.missing +
          counts.ambiguous + counts.unsupported);
    std::cout
        << "RG02A_G0_V04_PART"
        << " fixture=" << fixture
        << " native=" << counts.native
        << " resolved=" << counts.resolved
        << " missing=" << counts.missing
        << " ambiguous=" << counts.ambiguous
        << " unsupported=" << counts.unsupported
        << " three_surface_witnesses="
        << counts.three_surface_witnesses
        << " viewport_pickability=NOT_PROVEN\\n";
    return counts;
}

struct G0ProviderAdjacencyCounts final {
    std::size_t native{};
    std::size_t two_surfaces{};
    std::size_t three_surfaces{};
    std::size_t other_surfaces{};
};

G0ProviderAdjacencyCounts observeRevolveProviderVertices(
    const kernel::SolidModelingResult& result,
    const char* fixture) {
    verifyCompleteRuntimeInventory(result);
    std::size_t two_surfaces = 0U;
    std::size_t three_surfaces = 0U;
    std::size_t other_surfaces = 0U;
    for (const auto& vertex :
         result.current_vertex_semantics) {
        CHECK(vertex.runtime_token.valid());
        const auto count = vertex.adjacent_surfaces.size();
        if (count == 2U) {
            ++two_surfaces;
        } else if (count == 3U) {
            ++three_surfaces;
        } else {
            ++other_surfaces;
        }
    }
    CHECK(two_surfaces + three_surfaces +
          other_surfaces ==
          result.current_vertices.size());
    std::cout
        << "RG02A_G0_V04_PROVIDER"
        << " fixture=" << fixture
        << " native=" << result.current_vertices.size()
        << " two_surfaces=" << two_surfaces
        << " three_surfaces=" << three_surfaces
        << " other_surfaces=" << other_surfaces
        << " semantic_identity=NOT_INFERRED\\n";
    return {
        result.current_vertices.size(),
        two_surfaces,
        three_surfaces,
        other_surfaces};
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;

    // Partial OneSide: deterministic start/end material caps plus one
    // provenance-tied side claim for each Profile boundary use.
    {
        const auto result =
            kernel.revolve(
                revolveInput(
                    0.0,
                    pi / 2.0));
        verifyCompleteRuntimeInventory(result);
        const auto g0_partial_provider =
            observeRevolveProviderVertices(
                result, "V04-partial-90-provider");
        CHECK(g0_partial_provider.native == 8U);
        CHECK(g0_partial_provider.two_surfaces == 0U);
        CHECK(g0_partial_provider.three_surfaces == 8U);
        CHECK(g0_partial_provider.other_surfaces == 0U);
        CHECK(result.new_surfaces.size() == 6U);

        const auto* start =
            surfaceByRole(
                result,
                kernel::GeneratedFaceRoleKind::
                    revolve_start_cap);
        const auto* end =
            surfaceByRole(
                result,
                kernel::GeneratedFaceRoleKind::
                    revolve_end_cap);
        CHECK(start != nullptr);
        CHECK(end != nullptr);
        CHECK(
            start->surface_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            end->surface_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            start->surface_kind ==
            kernel::SurfaceKind::plane);
        CHECK(
            end->surface_kind ==
            kernel::SurfaceKind::plane);
        CHECK(
            start->canonical_frame &&
            start->canonical_frame->valid());
        CHECK(
            end->canonical_frame &&
            end->canonical_frame->valid());

        const auto* bottom =
            sideBySource(result, "bottom");
        const auto* right =
            sideBySource(result, "right");
        const auto* top =
            sideBySource(result, "top");
        const auto* left =
            sideBySource(result, "left");
        CHECK(bottom && right && top && left);
        CHECK(
            bottom->surface_kind ==
            kernel::SurfaceKind::plane);
        CHECK(
            top->surface_kind ==
            kernel::SurfaceKind::plane);
        CHECK(
            right->surface_kind ==
            kernel::SurfaceKind::cylinder);
        CHECK(
            left->surface_kind ==
            kernel::SurfaceKind::cylinder);
        CHECK(
            bottom->canonical_frame &&
            bottom->canonical_frame->valid());
        CHECK(
            top->canonical_frame &&
            top->canonical_frame->valid());
        CHECK(!right->canonical_frame);
        CHECK(!left->canonical_frame);
    }

    // Reverse partial uses the same authored start role and the signed end
    // angle; provider traversal order must not swap semantic cap identity.
    {
        const auto result =
            kernel.revolve(
                revolveInput(
                    0.0,
                    -pi / 2.0));
        verifyCompleteRuntimeInventory(result);
        CHECK(
            surfaceByRole(
                result,
                kernel::GeneratedFaceRoleKind::
                    revolve_start_cap) != nullptr);
        CHECK(
            surfaceByRole(
                result,
                kernel::GeneratedFaceRoleKind::
                    revolve_end_cap) != nullptr);
    }

    // Full turn: no artificial start/end Surface; periodic seam is complete
    // provider topology but explicitly observed as a seam representation.
    {
        const auto result =
            kernel.revolve(
                revolveInput(
                    0.0,
                    full_turn));
        verifyCompleteRuntimeInventory(result);
        const auto g0_full_provider =
            observeRevolveProviderVertices(
                result, "V04-full-360-provider");
        CHECK(g0_full_provider.native == 4U);
        CHECK(g0_full_provider.two_surfaces == 4U);
        CHECK(g0_full_provider.three_surfaces == 0U);
        CHECK(g0_full_provider.other_surfaces == 0U);
        CHECK(result.new_surfaces.size() == 4U);
        CHECK(
            surfaceByRole(
                result,
                kernel::GeneratedFaceRoleKind::
                    revolve_start_cap) == nullptr);
        CHECK(
            surfaceByRole(
                result,
                kernel::GeneratedFaceRoleKind::
                    revolve_end_cap) == nullptr);

        const auto seam_count =
            static_cast<std::size_t>(
                std::count_if(
                    result.current_edge_semantics.begin(),
                    result.current_edge_semantics.end(),
                    [](const auto& edge) {
                        return edge.periodic_seam;
                    }));
        CHECK(seam_count > 0U);

        for (const auto& edge :
             result.current_edge_semantics) {
            if (!edge.periodic_seam) {
                continue;
            }
            CHECK(
                !edge.adjacent_surfaces.empty());
        }
    }

    // Axis-contact collapse: the boundary coincident with the Axis produces
    // no material side Surface, but the successful Body inventory remains
    // complete and the missing semantic claim is explicit.
    {
        auto input =
            revolveInput(
                0.0,
                full_turn);
        input.profile =
            rectangle(
                0.0,
                20.0,
                0.0,
                10.0);
        CHECK(input.valid());

        const auto result =
            kernel.revolve(input);
        verifyCompleteRuntimeInventory(result);
        const auto* collapsed =
            sideBySource(result, "left");
        CHECK(collapsed != nullptr);
        CHECK(
            collapsed->surface_status ==
            kernel::ReferenceStatus::missing);
        CHECK(
            collapsed->candidate_face_count == 0U);
        CHECK(!collapsed->resolved_token);
    }

    // Revolve Add/Cut reuse the exact existing Boolean lineage path.
    {
        const auto base =
            kernel.extrude(
                boxInput(
                    5.0,
                    -5.0,
                    5.0));
        CHECK(base.ok());

        auto add_input =
            revolveInput(
                0.0,
                full_turn,
                kernel::SolidBooleanOperation::add);
        add_input.profile =
            rectangle(
                3.0,
                8.0,
                -2.0,
                2.0);
        CHECK(add_input.valid());

        const auto added =
            kernel.revolve(
                add_input,
                base.solid);
        verifyCompleteRuntimeInventory(added);
        CHECK(!added.inherited_surfaces.empty());
        CHECK(!added.new_surfaces.empty());

        const auto large_base =
            kernel.extrude(
                boxInput(
                    25.0,
                    -25.0,
                    25.0));
        CHECK(large_base.ok());

        auto cut_input =
            revolveInput(
                0.0,
                full_turn,
                kernel::SolidBooleanOperation::cut);
        cut_input.profile =
            rectangle(
                5.0,
                10.0,
                -5.0,
                5.0);
        CHECK(cut_input.valid());

        const auto cut =
            kernel.revolve(
                cut_input,
                large_base.solid);
        verifyCompleteRuntimeInventory(cut);
        CHECK(!cut.inherited_surfaces.empty());
        CHECK(!cut.new_surfaces.empty());
    }

    // Real Part evaluator: a Profile boundary coincident with Origin X is
    // allowed, collapses under full rotation, and the remaining OCCT periodic
    // seam reaches the PM-02 catalog only as a representation artifact.
    {
        auto document =
            makePartRevolve(
                core::BuiltinReferenceRole::x_axis,
                {10.0, 0.0},
                {20.0, 10.0},
                full_turn);
        const auto evaluated =
            part::evaluatePart(
                document,
                kernel);
        CHECK(
            evaluated.body_status ==
            part::BodyEvaluationStatus::
                up_to_date);
        CHECK(evaluated.features.size() == 1U);
        const auto& feature =
            evaluated.features.front();
        CHECK(
            feature.status ==
            part::FeatureEvaluationStatus::
                up_to_date);
        CHECK(feature.result_topology.has_value());
        CHECK(feature.result_topology->complete());
        const auto g0_full_part = observeRevolvePartVertices(
            *feature.result_topology,
            "V04-full-360-part");
        CHECK(g0_full_part.native == 2U);
        CHECK(g0_full_part.resolved == 0U);
        CHECK(g0_full_part.ambiguous == 0U);
        CHECK(g0_full_part.missing == 0U);
        CHECK(g0_full_part.unsupported == 2U);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_start_cap) == 0U);
        CHECK(
            countSurfaceRole(
                feature,
                part::FeatureSurfaceRoleKind::
                    revolve_end_cap) == 0U);

        const auto representation_edges =
            static_cast<std::size_t>(
                std::count_if(
                    feature.result_topology->
                        edges.begin(),
                    feature.result_topology->
                        edges.end(),
                    [](const auto& edge) {
                        return edge.accounting_class ==
                                   part::TopologyAccountingClass::
                                       known_representation_artifact &&
                               edge.referenceability ==
                                   kernel::ReferenceStatus::
                                       unsupported;
                    }));
        CHECK(representation_edges > 0U);

        // PM-05F R2-C: a valid full-turn Revolve must have ordinary,
        // authorable material Edges in addition to non-authorable seams.
        // This counts strict semantic authoring, not geometry-nearest hits.
        std::size_t candidate_edges = 0U;
        std::size_t authorable_edges = 0U;
        std::size_t unsupported_edges = 0U;
        for (const auto& edge :
             feature.result_topology->edges) {
            if (edge.periodic_seam ||
                edge.representation_partition ||
                edge.accounting_class ==
                    part::TopologyAccountingClass::
                        known_representation_artifact) {
                continue;
            }
            ++candidate_edges;
            const auto authored =
                part::authorMaterialEdgeReference(
                    *feature.result_topology,
                    edge.runtime_token);
            if (authored.ok()) {
                const auto resolved =
                    part::resolveMaterialEdgeReference(
                        *authored.reference,
                        *feature.result_topology);
                CHECK(resolved.has_value());
                CHECK(resolved->resolved());
                ++authorable_edges;
            } else {
                ++unsupported_edges;
            }
        }
        std::cout
            << "PM05F_R2_REVOLVE_EDGE_CLASSIFICATION"
            << " candidates=" << candidate_edges
            << " authorable=" << authorable_edges
            << " nonauthorable=" << unsupported_edges
            << " artifacts=" << representation_edges
            << '\n';
        CHECK(candidate_edges > 0U);
        CHECK(authorable_edges > 0U);
        const auto circle =
            firstAuthorableCircle(
                *feature.result_topology);
        CHECK(circle.has_value());
        verifyCylinderEdgeFeature(
            document,
            *circle,
            kernel::EdgeFeatureOperation::fillet,
            kernel);
        verifyCylinderEdgeFeature(
            document,
            *circle,
            kernel::EdgeFeatureOperation::chamfer,
            kernel);
    }

    // Mathematically equivalent cylinder dimensions through an Extrude
    // circle Profile. Both generators must use one strict material-Edge
    // authoring/resolution and Fillet/Chamfer contract.
    {
        const auto extrude = makeExtrudeCylinder();
        const auto evaluated =
            part::evaluatePart(extrude, kernel);
        CHECK(evaluated.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(evaluated.current_topology.has_value());
        CHECK(evaluated.current_topology->complete());
        // RG-02A/G0 V10: real OCCT cylinder seam Vertices. Ordinary
        // Viewport pick eligibility only excludes integrity failure and
        // representation artifacts; it does not certify a durable Point.
        const auto g0_cylinder = observeRevolvePartVertices(
            *evaluated.current_topology,
            "V10-cylinder-extrude");
        CHECK(g0_cylinder.native == 2U);
        CHECK(g0_cylinder.resolved == 0U);
        CHECK(g0_cylinder.missing == 0U);
        CHECK(g0_cylinder.ambiguous == 0U);
        CHECK(g0_cylinder.unsupported == 2U);
        std::size_t ordinary_policy_eligible_unsupported = 0U;
        for (const auto& vertex :
             evaluated.current_topology->vertices) {
            if (vertex.referenceability !=
                    kernel::ReferenceStatus::unsupported) {
                continue;
            }
            CHECK(vertex.accounting_class ==
                  part::TopologyAccountingClass::
                      semantically_unsupported);
            CHECK(vertex.point_candidates.empty());
            CHECK(vertex.runtime_token.valid());
            // This corresponds to the existing ordinary Viewer
            // accounting-class filter; this test does not execute UI.
            ++ordinary_policy_eligible_unsupported;
        }
        CHECK(ordinary_policy_eligible_unsupported == 2U);
        CHECK(ordinary_policy_eligible_unsupported ==
              g0_cylinder.unsupported);
        // G0/V08: check a real two-point intersection of one analytic
        // cylinder, a cap plane and a planar Boolean Cut wall. If
        // multiple native Vertices claim one semantic 3-Surface Point,
        // Part must mark Ambiguous and never choose one by XYZ.
        const auto half_document =
            withCylinderHalfCut(extrude);
        const auto half_evaluation =
            part::evaluatePart(half_document, kernel);
        CHECK(
            half_evaluation.body_status ==
            part::BodyEvaluationStatus::up_to_date);
        CHECK(half_evaluation.features.size() == 2U);
        CHECK(
            half_evaluation.features[1].result_topology);
        const auto& half_catalog =
            *half_evaluation.features[1].result_topology;
        const auto half_counts = observeRevolvePartVertices(
            half_catalog, "V08-half-cylinder-cut");
        CHECK(half_counts.native == 6U);
        CHECK(half_counts.resolved == 0U);
        CHECK(half_counts.missing == 0U);
        CHECK(half_counts.ambiguous == 4U);
        CHECK(half_counts.unsupported == 2U);
        std::size_t branched_points = 0U;
        for (const auto& point : half_catalog.points) {
            if (point.status !=
                    kernel::ReferenceStatus::ambiguous) {
                continue;
            }
            CHECK(point.candidate_vertex_count > 1U);
            CHECK(point.current_vertices.size() > 1U);
            CHECK(point.address.valid());
            CHECK(point.address.adjacent_surfaces.size() == 3U);
            ++branched_points;
        }
        CHECK(branched_points == 2U);
        CHECK(half_counts.ambiguous == 4U);

        // RG-02B1/E0-06: actual OCCT ambiguous native Point meaning.
        // Each visually present candidate maps to an intrinsically
        // Ambiguous three-Surface semantic Point; current Edge incidence
        // cannot turn it into an independent Resolved Datum Point.
        std::size_t e0_ambiguous_realizations = 0U;
        std::size_t e0_unsupported_no_address = 0U;
        for (const auto& vertex : half_catalog.vertices) {
            if (vertex.referenceability ==
                kernel::ReferenceStatus::ambiguous) {
                CHECK(vertex.point_candidates.size() == 1U);
                const auto& address =
                    vertex.point_candidates.front();
                CHECK(address.valid());
                const auto found = std::find_if(
                    half_catalog.points.begin(),
                    half_catalog.points.end(),
                    [&address](const auto& point) {
                        return point.address == address;
                    });
                CHECK(found != half_catalog.points.end());
                CHECK(found->status ==
                      kernel::ReferenceStatus::ambiguous);
                CHECK(found->current_vertices.size() > 1U);
                CHECK(
                    std::find(
                        found->current_vertices.begin(),
                        found->current_vertices.end(),
                        vertex.runtime_token) !=
                    found->current_vertices.end());
                ++e0_ambiguous_realizations;
            } else if (vertex.referenceability ==
                       kernel::ReferenceStatus::unsupported) {
                CHECK(vertex.point_candidates.empty());
                ++e0_unsupported_no_address;
            }
        }
        CHECK(e0_ambiguous_realizations == 4U);
        CHECK(e0_unsupported_no_address == 2U);
        std::cout
            << "RG02B1_E0_NEGATIVE"
            << " ambiguous_native_point_realizations="
            << e0_ambiguous_realizations
            << " unsupported_without_point="
            << e0_unsupported_no_address
            << " edge_incidence_not_point_authority=1"
            << " source_end_selector=NOT_IMPLEMENTED\\n";
        std::cout
            << "RG02A_G0_V08"
            << " ambiguous_three_surface_point_groups="
            << branched_points
            << " ambiguous_native_vertices="
            << half_counts.ambiguous
            << " nearest_vertex_chosen=0\\n";
        std::cout
            << "RG02A_G0_V10"
            << " unsupported_native_eligible_by_policy="
            << ordinary_policy_eligible_unsupported
            << " ui_interaction=NOT_TESTED"
            << " durable_point=UNSUPPORTED\\n";
        const auto circle =
            firstAuthorableCircle(
                *evaluated.current_topology);
        CHECK(circle.has_value());
        verifyCylinderEdgeFeature(
            extrude,
            *circle,
            kernel::EdgeFeatureOperation::fillet,
            kernel);
        verifyCylinderEdgeFeature(
            extrude,
            *circle,
            kernel::EdgeFeatureOperation::chamfer,
            kernel);
    }

    // PM-05F R2-C: the exact same shared semantic Curve/Edge authoring
    // pipeline also handles partial Revolve's real planar start/end caps.
    // No new Curve address layout or transient provider-order identity.
    {
        auto partial =
            makePartRevolve(
                core::BuiltinReferenceRole::x_axis,
                {10.0, 0.0},
                {20.0, 10.0},
                pi / 2.0);
        const auto evaluated =
            part::evaluatePart(partial, kernel);
        CHECK(evaluated.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(evaluated.features.size() == 1U);
        const auto& feature = evaluated.features.front();
        CHECK(feature.result_topology.has_value());
        CHECK(feature.result_topology->complete());
        const auto g0_partial_part = observeRevolvePartVertices(
            *feature.result_topology,
            "V04-partial-90-part");
        CHECK(g0_partial_part.native == 6U);
        CHECK(g0_partial_part.resolved == 6U);
        CHECK(g0_partial_part.ambiguous == 0U);
        CHECK(g0_partial_part.missing == 0U);
        CHECK(g0_partial_part.unsupported == 0U);

        std::size_t authorable_edges = 0U;
        std::size_t authorable_lines = 0U;
        std::size_t authorable_circles = 0U;
        std::size_t cap_side_curves = 0U;
        for (const auto& curve :
             feature.result_topology->curves) {
            if (curve.address.role ==
                    part::FeatureCurveRoleKind::cap_side &&
                curve.status ==
                    kernel::ReferenceStatus::resolved) {
                ++cap_side_curves;
            }
        }
        for (const auto& edge :
             feature.result_topology->edges) {
            const auto authored =
                part::authorMaterialEdgeReference(
                    *feature.result_topology,
                    edge.runtime_token);
            if (!authored.ok()) {
                continue;
            }
            const auto resolved =
                part::resolveMaterialEdgeReference(
                    *authored.reference,
                    *feature.result_topology);
            CHECK(resolved.has_value());
            CHECK(resolved->resolved());
            ++authorable_edges;
            if (edge.curve_kind ==
                    kernel::CurveKind::line) {
                ++authorable_lines;
            }
            if (edge.curve_kind ==
                    kernel::CurveKind::circle) {
                ++authorable_circles;
            }
        }
        std::cout
            << "PM05F_R2_PARTIAL_REVOLVE_EDGE_CLASSIFICATION"
            << " authorable=" << authorable_edges
            << " line=" << authorable_lines
            << " circle=" << authorable_circles
            << " cap_side=" << cap_side_curves
            << '\n';
        CHECK(authorable_edges > 0U);
        CHECK(authorable_lines > 0U);
        CHECK(cap_side_curves > 0U);
    }

    std::cout
        << "PM-04C2 production OCCT Revolve tests passed\n";
    return EXIT_SUCCESS;
}
