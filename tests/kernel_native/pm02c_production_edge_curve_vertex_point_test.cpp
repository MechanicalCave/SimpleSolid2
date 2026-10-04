#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02C production Edge/Curve Vertex/Point CHECK failed at line "
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

kernel::BoundaryUse2D circleUse(
    kernel::Point2 center,
    double radius,
    std::string source) {
    return {
        kernel::Circle2{center, radius},
        0.0,
        1.0,
        true,
        false,
        true,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            0U,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    double x0,
    double y0,
    double x1,
    double y1,
    double z,
    std::string_view prefix) {
    kernel::PlanarProfileInput profile;
    profile.frame.origin = {0.0, 0.0, z};
    profile.outer.boundary = {
        lineUse(
            {x0, y0},
            {x1, y0},
            std::string{prefix} + "-bottom",
            0U),
        lineUse(
            {x1, y0},
            {x1, y1},
            std::string{prefix} + "-right",
            1U),
        lineUse(
            {x1, y1},
            {x0, y1},
            std::string{prefix} + "-top",
            2U),
        lineUse(
            {x0, y1},
            {x0, y0},
            std::string{prefix} + "-left",
            3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput circleProfile(
    double z,
    std::string source) {
    kernel::PlanarProfileInput profile;
    profile.frame.origin = {0.0, 0.0, z};
    profile.outer.boundary = {
        circleUse(
            {0.0, 0.0},
            10.0,
            std::move(source)),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput oneSide(
    kernel::PlanarProfileInput profile,
    double distance,
    kernel::SolidBooleanOperation operation =
        kernel::SolidBooleanOperation::add) {
    kernel::LinearExtrudeInput result;
    result.profile = std::move(profile);
    result.start_offset_mm = 0.0;
    result.end_offset_mm = distance;
    result.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    result.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    result.operation = operation;
    CHECK(result.valid());
    return result;
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

bool hasSurface(
    const kernel::CurrentEdgeSemanticObservation& edge,
    kernel::RuntimeSurfaceToken surface) {
    return std::find(
               edge.adjacent_surfaces.begin(),
               edge.adjacent_surfaces.end(),
               surface) !=
           edge.adjacent_surfaces.end();
}

std::vector<const kernel::CurrentEdgeSemanticObservation*>
edgesForSurfacePair(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeSurfaceToken first,
    kernel::RuntimeSurfaceToken second) {
    std::vector<const kernel::CurrentEdgeSemanticObservation*> matches;
    for (const auto& edge : result.current_edge_semantics) {
        if (edge.periodic_seam) continue;
        if (hasSurface(edge, first) &&
            hasSurface(edge, second)) {
            matches.push_back(&edge);
        }
    }
    return matches;
}

const kernel::InheritedEdgeRealizationLineage*
edgeLineage(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeEdgeToken source) {
    const auto found =
        std::find_if(
            result.inherited_edge_realizations.begin(),
            result.inherited_edge_realizations.end(),
            [source](const auto& lineage) {
                return lineage.source_token == source;
            });
    return found ==
                   result.inherited_edge_realizations.end()
        ? nullptr
        : &*found;
}

void verifyPristineAndSeam(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto prism =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "base"),
                10.0));
    CHECK(prism.ok());
    CHECK(prism.current_edges.size() == 12U);
    CHECK(prism.current_edge_semantics.size() == 12U);
    CHECK(prism.current_vertices.size() == 8U);
    CHECK(prism.current_vertex_semantics.size() == 8U);

    for (const auto& edge : prism.current_edge_semantics) {
        CHECK(edge.runtime_token.valid());
        CHECK(!edge.periodic_seam);
        CHECK(
            edge.provider_curve_kind ==
            kernel::CurveKind::line);
        CHECK(edge.adjacent_surfaces.size() == 2U);
    }

    for (const auto& vertex :
         prism.current_vertex_semantics) {
        CHECK(vertex.runtime_token.valid());
        CHECK(vertex.adjacent_surfaces.size() == 3U);
        CHECK(vertex.incident_material_edges.size() == 3U);
        CHECK(vertex.provider_point.has_value());
        CHECK(std::isfinite(vertex.provider_point->x));
        CHECK(std::isfinite(vertex.provider_point->y));
        CHECK(std::isfinite(vertex.provider_point->z));
    }

    const auto cylinder =
        provider.extrude(
            oneSide(
                circleProfile(
                    0.0,
                    "circle"),
                10.0));
    CHECK(cylinder.ok());

    std::size_t seam_count = 0U;
    std::size_t material_circle_count = 0U;
    for (const auto& edge :
         cylinder.current_edge_semantics) {
        if (edge.periodic_seam) {
            ++seam_count;
            continue;
        }
        CHECK(
            edge.provider_curve_kind ==
            kernel::CurveKind::circle);
        CHECK(edge.adjacent_surfaces.size() == 2U);
        ++material_circle_count;
    }
    CHECK(seam_count >= 1U);
    CHECK(material_circle_count == 2U);
}

void verifyInheritedEdgeLifecycle(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "base"),
                10.0));
    CHECK(base.ok());

    const auto* extent =
        newCap(
            base,
            kernel::ExtrudeCapRole::extent_cap);
    const auto* bottom =
        newSide(
            base,
            "base-bottom");
    CHECK(extent != nullptr);
    CHECK(bottom != nullptr);
    CHECK(extent->resolved_token.has_value());
    CHECK(bottom->resolved_token.has_value());

    const auto source_edges =
        edgesForSurfacePair(
            base,
            *extent->resolved_token,
            *bottom->resolved_token);
    CHECK(source_edges.size() == 1U);
    const auto source =
        source_edges.front()->runtime_token;

    struct Scenario final {
        double x0;
        double y0;
        double x1;
        double y1;
        kernel::ReferenceStatus expected;
        std::size_t candidates;
        const char* name;
    };

    const std::vector<Scenario> scenarios = {
        {30.0, 10.0, 35.0, 15.0,
         kernel::ReferenceStatus::resolved, 1U,
         "unchanged"},
        {30.0, -5.0, 45.0, 5.0,
         kernel::ReferenceStatus::resolved, 1U,
         "trim"},
        {15.0, -5.0, 25.0, 5.0,
         kernel::ReferenceStatus::ambiguous, 2U,
         "split"},
        {-5.0, -5.0, 45.0, 5.0,
         kernel::ReferenceStatus::missing, 0U,
         "remove"},
    };

    for (const auto& scenario : scenarios) {
        const auto result =
            provider.extrude(
                oneSide(
                    rectangle(
                        scenario.x0,
                        scenario.y0,
                        scenario.x1,
                        scenario.y1,
                        5.0,
                        std::string{"probe-"} +
                            scenario.name),
                    10.0,
                    kernel::SolidBooleanOperation::cut),
                base.solid);
        CHECK(result.ok());

        const auto* lineage =
            edgeLineage(
                result,
                source);
        CHECK(lineage != nullptr);
        CHECK(lineage->status == scenario.expected);
        CHECK(
            lineage->candidate_count ==
            scenario.candidates);
        CHECK(
            lineage->current_edges.size() ==
            scenario.candidates);
        for (const auto token :
             lineage->current_edges) {
            CHECK(token.valid());
            CHECK(
                std::count(
                    result.current_edges.begin(),
                    result.current_edges.end(),
                    token) == 1);
        }
    }
}

void verifyBooleanIntersectionAndBranchAmbiguity(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "base"),
                10.0));
    CHECK(base.ok());

    const auto* base_extent =
        newCap(
            base,
            kernel::ExtrudeCapRole::extent_cap);
    const auto* base_bottom =
        newSide(
            base,
            "base-bottom");
    CHECK(base_extent != nullptr);
    CHECK(base_bottom != nullptr);
    CHECK(base_extent->resolved_token.has_value());
    CHECK(base_bottom->resolved_token.has_value());

    const auto unique_cut =
        provider.extrude(
            oneSide(
                rectangle(
                    10.0, 5.0,
                    30.0, 15.0,
                    5.0,
                    "tool"),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(unique_cut.ok());
    const auto* tool_bottom =
        newSide(
            unique_cut,
            "tool-bottom");
    CHECK(tool_bottom != nullptr);
    CHECK(tool_bottom->resolved_token.has_value());

    const auto unique_intersections =
        edgesForSurfacePair(
            unique_cut,
            *base_extent->resolved_token,
            *tool_bottom->resolved_token);
    CHECK(unique_intersections.size() == 1U);
    CHECK(
        unique_intersections.front()
            ->provider_curve_kind ==
        kernel::CurveKind::line);

    // The cut is internal to the base footprint; all original outer prism
    // vertices survive singularly through provider history.
    CHECK(
        unique_cut.inherited_vertex_realizations.size() ==
        base.current_vertices.size());
    for (const auto& lineage :
         unique_cut.inherited_vertex_realizations) {
        CHECK(
            lineage.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(lineage.candidate_count == 1U);
        CHECK(lineage.current_vertices.size() == 1U);
    }

    const auto branch_cut =
        provider.extrude(
            oneSide(
                rectangle(
                    15.0, -5.0,
                    25.0, 25.0,
                    5.0,
                    "branch"),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(branch_cut.ok());

    const auto branches =
        edgesForSurfacePair(
            branch_cut,
            *base_extent->resolved_token,
            *base_bottom->resolved_token);
    CHECK(branches.size() == 2U);
    for (const auto* edge : branches) {
        CHECK(edge != nullptr);
        CHECK(
            edge->provider_curve_kind ==
            kernel::CurveKind::line);
    }
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    verifyPristineAndSeam(provider);
    verifyInheritedEdgeLifecycle(provider);
    verifyBooleanIntersectionAndBranchAmbiguity(provider);

    std::cout
        << "PM02C_PRODUCTION_EDGE_CURVE_VERTEX_POINT_PASS"
        << " rectangle_edges=12"
        << " rectangle_vertices=8"
        << " seam=representation_artifact_input"
        << " edge_split=2"
        << " pair_multibranch=2"
        << " vertex_xyz=diagnostic_only"
        << '\n';

    return EXIT_SUCCESS;
}
