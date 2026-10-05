#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <cmath>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02J R2 Add Surface continuation CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    kernel::Frame3 frame,
    double u0,
    double v0,
    double u1,
    double v1,
    std::string_view prefix) {
    kernel::PlanarProfileInput profile;
    profile.frame = frame;
    profile.outer.boundary = {
        lineUse(
            {u0, v0},
            {u1, v0},
            std::string{prefix} + "-bottom",
            0U),
        lineUse(
            {u1, v0},
            {u1, v1},
            std::string{prefix} + "-right",
            1U),
        lineUse(
            {u1, v1},
            {u0, v1},
            std::string{prefix} + "-top",
            2U),
        lineUse(
            {u0, v1},
            {u0, v0},
            std::string{prefix} + "-left",
            3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput add(
    kernel::PlanarProfileInput profile,
    double distance) {
    kernel::LinearExtrudeInput input;
    input.profile = std::move(profile);
    input.start_offset_mm = 0.0;
    input.end_offset_mm = distance;
    input.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());
    return input;
}

const kernel::NewSurfaceLineage*
newCap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [role](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::cap &&
                       surface.role.cap_role ==
                           std::optional<kernel::ExtrudeCapRole>{role};
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::NewSurfaceLineage*
newSide(
    const kernel::SolidModelingResult& result,
    std::string_view source) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [source](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::side &&
                       surface.role.side_provenance &&
                       surface.role.side_provenance->source_entity ==
                           source;
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::InheritedSurfaceLineage*
inheritedSurface(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeSurfaceToken token) {
    const auto found =
        std::find_if(
            result.inherited_surfaces.begin(),
            result.inherited_surfaces.end(),
            [token](const auto& surface) {
                return surface.token == token;
            });
    return found == result.inherited_surfaces.end()
        ? nullptr
        : &*found;
}

sketch::Point2 projectToFrame(
    const kernel::Frame3& frame,
    kernel::Point3 point) {
    const kernel::Point3 delta{
        point.x - frame.origin.x,
        point.y - frame.origin.y,
        point.z - frame.origin.z};
    return {
        delta.x * frame.u_axis.x +
            delta.y * frame.u_axis.y +
            delta.z * frame.u_axis.z,
        delta.x * frame.v_axis.x +
            delta.y * frame.v_axis.y +
            delta.z * frame.v_axis.z,
    };
}

part::ProfileId createRectangleProfile(
    application::DocumentSession& session,
    sketch::SketchId sketch_id,
    sketch::Point2 first,
    sketch::Point2 opposite) {
    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                first,
                opposite,
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto regions =
        sketch::analyzeRegions(hosted->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok());
    CHECK(profile.profile_id.has_value());
    return *profile.profile_id;
}

void verifyPartIntegration() {
    kernel_occt::OcctSolidModelingKernel provider;
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "pm02jr2-add-continuation.ss2part"},
        std::move(document)};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(base_sketch.ok());
    CHECK(base_sketch.sketch_id.has_value());

    const auto base_profile =
        createRectangleProfile(
            session,
            *base_sketch.sketch_id,
            {0.0, 0.0},
            {40.0, 30.0});

    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                base_profile,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            provider);
    CHECK(base.ok());
    CHECK(base.feature_id.has_value());

    const auto base_eval =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        base_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(base_eval.current_topology.has_value());

    const auto& base_topology =
        *base_eval.current_topology;
    const auto side =
        std::find_if(
            base_topology.surfaces.begin(),
            base_topology.surfaces.end(),
            [id = *base.feature_id](
                const auto& surface) {
                return surface.status ==
                           kernel::ReferenceStatus::resolved &&
                       surface.address.producer_feature_id ==
                           id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::side &&
                       surface.surface_kind ==
                           kernel::SurfaceKind::plane &&
                       surface.canonical_frame &&
                       std::abs(
                           surface.canonical_frame
                               ->normal.x) > 0.9;
            });
    CHECK(side != base_topology.surfaces.end());
    CHECK(side->canonical_frame.has_value());

    const part::SurfaceReference side_ref{
        base_topology.stage,
        side->address};
    const auto side_support =
        part::partSketchSupportForBodyPlanarSurface(
            side_ref);
    CHECK(side_support.has_value());

    const auto extension_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *side_support,
                session.document().revision()},
            &provider);
    CHECK(extension_sketch.ok());
    CHECK(extension_sketch.sketch_id.has_value());

    const auto& frame = *side->canonical_frame;
    const double side_x =
        frame.normal.x > 0.0
            ? 40.0
            : 0.0;
    const std::vector<kernel::Point3> corners{
        {side_x, 5.0, 0.0},
        {side_x, 25.0, 0.0},
        {side_x, 5.0, 10.0},
        {side_x, 25.0, 10.0},
    };
    std::vector<sketch::Point2> uv;
    uv.reserve(corners.size());
    for (const auto& point : corners) {
        uv.push_back(
            projectToFrame(frame, point));
    }
    double min_u = uv.front().x;
    double max_u = uv.front().x;
    double min_v = uv.front().y;
    double max_v = uv.front().y;
    for (const auto point : uv) {
        min_u = std::min(min_u, point.x);
        max_u = std::max(max_u, point.x);
        min_v = std::min(min_v, point.y);
        max_v = std::max(max_v, point.y);
    }
    CHECK(max_u - min_u > 1.0);
    CHECK(max_v - min_v > 1.0);

    const auto extension_profile =
        createRectangleProfile(
            session,
            *extension_sketch.sketch_id,
            {min_u, min_v},
            {max_u, max_v});

    const auto extension =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                extension_profile,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Extension"},
            provider);
    CHECK(extension.ok());
    CHECK(extension.feature_id.has_value());

    const auto final_eval =
        part::evaluatePart(
            session.document(),
            provider);
    CHECK(
        final_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(final_eval.current_topology.has_value());
    const auto& topology =
        *final_eval.current_topology;

    const auto top =
        std::find_if(
            topology.surfaces.begin(),
            topology.surfaces.end(),
            [id = *base.feature_id](
                const auto& surface) {
                return surface.address
                           .producer_feature_id == id &&
                       surface.address.role ==
                           part::FeatureSurfaceRoleKind::
                               extent_cap;
            });
    CHECK(top != topology.surfaces.end());
    CHECK(
        top->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(top->current_faces.size() >= 2U);
    CHECK(
        top->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);

    for (const auto face_token :
         top->current_faces) {
        const auto face =
            std::find_if(
                topology.faces.begin(),
                topology.faces.end(),
                [face_token](const auto& record) {
                    return record.runtime_token ==
                           face_token;
                });
        CHECK(face != topology.faces.end());
        CHECK(face->surface_candidates.size() == 1U);
        CHECK(
            face->surface_candidates.front() ==
            top->address);
    }

    const auto contribution =
        part::currentFeatureContribution(
            topology,
            *extension.feature_id);
    CHECK(contribution.valid());
    CHECK(!contribution.faces.empty());

    const auto partition_count =
        std::count_if(
            topology.edges.begin(),
            topology.edges.end(),
            [](const auto& edge) {
                return edge.representation_partition &&
                       edge.accounting_class ==
                           part::TopologyAccountingClass::
                               known_representation_artifact &&
                       edge.referenceability ==
                           kernel::ReferenceStatus::
                               unsupported;
            });
    CHECK(partition_count >= 1);

    // Picking either bounded Face fragment maps to the same SurfaceReference,
    // so Create Sketch is singular and succeeds from the current Body stage.
    const part::SurfaceReference final_top_ref{
        topology.stage,
        top->address};
    const auto top_support =
        part::partSketchSupportForBodyPlanarSurface(
            final_top_ref);
    CHECK(top_support.has_value());

    const auto first_new_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *top_support,
                session.document().revision()},
            &provider);
    CHECK(first_new_sketch.ok());
    CHECK(first_new_sketch.sketch_id.has_value());

    const auto second_new_sketch =
        session.execute(
            application::CreatePartSketchOnSupportCommand{
                *top_support,
                session.document().revision()},
            &provider);
    CHECK(second_new_sketch.ok());
    CHECK(second_new_sketch.sketch_id.has_value());
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    kernel::Frame3 xy;
    const auto base =
        provider.extrude(
            add(
                rectangle(
                    xy,
                    0.0,
                    0.0,
                    40.0,
                    30.0,
                    "base"),
                10.0));
    CHECK(base.ok());

    const auto* base_top =
        newCap(
            base,
            kernel::ExtrudeCapRole::extent_cap);
    CHECK(base_top != nullptr);
    CHECK(
        base_top->surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(base_top->resolved_token.has_value());

    // Extend the box from its +X side through the full Z height. The extension
    // top lies on the existing z=10 engineering Surface. OCCT may retain the
    // old/new top boundary as a B-Rep partition, but ADR-0017 requires one
    // semantic carrier rather than competing coplanar Surface identities.
    kernel::Frame3 yz;
    yz.origin = {40.0, 0.0, 0.0};
    yz.u_axis = {0.0, 1.0, 0.0};
    yz.v_axis = {0.0, 0.0, 1.0};
    yz.normal = {1.0, 0.0, 0.0};
    CHECK(yz.valid());

    const auto extended =
        provider.extrude(
            add(
                rectangle(
                    yz,
                    5.0,
                    0.0,
                    25.0,
                    10.0,
                    "extension"),
                10.0),
            base.solid);
    CHECK(extended.ok());

    const auto* inherited_top =
        inheritedSurface(
            extended,
            *base_top->resolved_token);
    CHECK(inherited_top != nullptr);
    CHECK(
        inherited_top->surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(inherited_top->canonical_frame.has_value());
    CHECK(inherited_top->current_faces.size() >= 2U);
    CHECK(
        inherited_top->strict_face_status ==
        kernel::ReferenceStatus::ambiguous);

    const auto* created_top =
        newSide(
            extended,
            "extension-top");
    CHECK(created_top != nullptr);
    CHECK(created_top->continued_into.has_value());
    CHECK(
        *created_top->continued_into ==
        *base_top->resolved_token);
    CHECK(
        created_top->surface_status ==
        kernel::ReferenceStatus::unsupported);
    CHECK(!created_top->resolved_token.has_value());
    CHECK(created_top->current_faces.empty());
    CHECK(!created_top->contribution_faces.empty());

    for (const auto token :
         created_top->contribution_faces) {
        CHECK(
            std::find(
                inherited_top->current_faces.begin(),
                inherited_top->current_faces.end(),
                token) !=
            inherited_top->current_faces.end());
    }

    std::size_t partition_edges = 0U;
    for (const auto& edge :
         extended.current_edge_semantics) {
        if (!edge.same_surface_partition) {
            continue;
        }
        ++partition_edges;
        CHECK(!edge.periodic_seam);
        CHECK(edge.adjacent_surfaces.size() == 1U);
        CHECK(
            edge.adjacent_surfaces.front() ==
            *base_top->resolved_token ||
            edge.adjacent_surfaces.front().valid());
    }
    CHECK(partition_edges >= 1U);

    verifyPartIntegration();

    std::cout
        << "PM02JR2_ADD_SURFACE_CONTINUATION_PASS"
        << " inherited_top_faces="
        << inherited_top->current_faces.size()
        << " contribution_faces="
        << created_top->contribution_faces.size()
        << " partition_edges="
        << partition_edges
        << " geometry_similarity_authority=0"
        << '\n';

    return EXIT_SUCCESS;
}
