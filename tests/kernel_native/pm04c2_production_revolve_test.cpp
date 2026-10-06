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

    }

    std::cout
        << "PM-04C2 production OCCT Revolve tests passed\n";
    return EXIT_SUCCESS;
}
