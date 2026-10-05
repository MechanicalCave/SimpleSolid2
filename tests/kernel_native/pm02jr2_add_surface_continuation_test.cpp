#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

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
