#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02C production Edge/Vertex CHECK failed at line "
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

kernel::LinearExtrudeInput extrude(
    kernel::PlanarProfileInput profile,
    double distance,
    kernel::SolidBooleanOperation operation =
        kernel::SolidBooleanOperation::add) {
    kernel::LinearExtrudeInput input;
    input.profile = std::move(profile);
    input.start_offset_mm = 0.0;
    input.end_offset_mm = distance;
    input.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    input.operation = operation;
    CHECK(input.valid());
    return input;
}

const kernel::NewSurfaceLineage* newCap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    const auto found = std::find_if(
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

const kernel::NewSurfaceLineage* newSide(
    const kernel::SolidModelingResult& result,
    std::string_view source) {
    const auto found = std::find_if(
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

bool samePair(
    const std::vector<kernel::RuntimeSurfaceToken>& surfaces,
    kernel::RuntimeSurfaceToken first,
    kernel::RuntimeSurfaceToken second) {
    return surfaces.size() == 2U &&
           std::find(surfaces.begin(), surfaces.end(), first) !=
               surfaces.end() &&
           std::find(surfaces.begin(), surfaces.end(), second) !=
               surfaces.end();
}

std::vector<const kernel::CurrentEdgeSemanticRecord*>
edgesForPair(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeSurfaceToken first,
    kernel::RuntimeSurfaceToken second) {
    std::vector<const kernel::CurrentEdgeSemanticRecord*> found;
    for (const auto& edge : result.current_edge_semantics) {
        if (samePair(edge.adjacent_surfaces, first, second)) {
            found.push_back(&edge);
        }
    }
    return found;
}

kernel::SolidModelingResult makeBase(
    kernel_occt::OcctSolidModelingKernel& provider) {
    return provider.extrude(
        extrude(
            rectangle(
                0.0, 0.0,
                40.0, 20.0,
                0.0,
                "base"),
            10.0));
}

void verifyPristine(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base = makeBase(provider);
    CHECK(base.ok());
    CHECK(base.edge_count == 12U);
    CHECK(base.current_edge_semantics.size() == 12U);
    CHECK(base.vertex_count == 8U);
    CHECK(base.current_vertex_semantics.size() == 8U);

    for (const auto& edge : base.current_edge_semantics) {
        CHECK(!edge.representation_artifact);
        CHECK(!edge.integrity_failure);
        CHECK(edge.status == kernel::ReferenceStatus::resolved);
        CHECK(edge.candidate_count == 1U);
        CHECK(edge.curve_kind == kernel::CurveKind::line);
        CHECK(edge.adjacent_surfaces.size() == 2U);
        CHECK(
            edge.role == kernel::EdgeSemanticRoleKind::cap_side ||
            edge.role == kernel::EdgeSemanticRoleKind::side_side);
    }

    for (const auto& vertex : base.current_vertex_semantics) {
        CHECK(!vertex.integrity_failure);
        CHECK(vertex.status == kernel::ReferenceStatus::resolved);
        CHECK(vertex.candidate_count == 1U);
        CHECK(vertex.adjacent_surfaces.size() == 3U);
        CHECK(vertex.incident_material_edge_count == 3U);
        CHECK(vertex.provider_point.has_value());
    }
}

void verifyCylinderSeam(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto cylinder =
        provider.extrude(
            extrude(
                circleProfile(0.0, "circle"),
                10.0));
    CHECK(cylinder.ok());

    std::size_t seams = 0U;
    std::size_t circular_material_edges = 0U;
    for (const auto& edge :
         cylinder.current_edge_semantics) {
        if (edge.representation_artifact) {
            ++seams;
            CHECK(
                edge.role ==
                kernel::EdgeSemanticRoleKind::periodic_seam);
            CHECK(
                edge.status ==
                kernel::ReferenceStatus::unsupported);
            continue;
        }
        if (edge.role ==
                kernel::EdgeSemanticRoleKind::cap_side &&
            edge.curve_kind == kernel::CurveKind::circle) {
            ++circular_material_edges;
            CHECK(
                edge.status ==
                kernel::ReferenceStatus::resolved);
        }
    }

    CHECK(seams >= 1U);
    CHECK(circular_material_edges == 2U);
}

void verifyBooleanHistory(
    kernel_occt::OcctSolidModelingKernel& provider) {
    struct Scenario {
        double x0;
        double y0;
        double x1;
        double y1;
        std::size_t expected;
        kernel::ReferenceStatus status;
    };

    const Scenario scenarios[] = {
        {30.0, 10.0, 35.0, 15.0, 1U,
         kernel::ReferenceStatus::resolved},
        {30.0, -5.0, 45.0, 5.0, 1U,
         kernel::ReferenceStatus::resolved},
        {15.0, -5.0, 25.0, 5.0, 2U,
         kernel::ReferenceStatus::ambiguous},
        {-5.0, -5.0, 45.0, 5.0, 0U,
         kernel::ReferenceStatus::missing},
    };

    for (const auto& scenario : scenarios) {
        const auto base = makeBase(provider);
        CHECK(base.ok());
        const auto* top =
            newCap(
                base,
                kernel::ExtrudeCapRole::extent_cap);
        const auto* front =
            newSide(base, "base-bottom");
        CHECK(top != nullptr && top->resolved_token);
        CHECK(front != nullptr && front->resolved_token);

        const auto cut =
            provider.extrude(
                extrude(
                    rectangle(
                        scenario.x0,
                        scenario.y0,
                        scenario.x1,
                        scenario.y1,
                        5.0,
                        "edge-cut"),
                    10.0,
                    kernel::SolidBooleanOperation::cut),
                base.solid);
        CHECK(cut.ok());

        const auto matches =
            edgesForPair(
                cut,
                *top->resolved_token,
                *front->resolved_token);
        CHECK(matches.size() == scenario.expected);
        for (const auto* edge : matches) {
            CHECK(
                edge->status ==
                scenario.status);
            CHECK(
                edge->role ==
                kernel::EdgeSemanticRoleKind::cap_side);
            CHECK(
                edge->curve_kind ==
                kernel::CurveKind::line);
            CHECK(
                edge->candidate_count ==
                scenario.expected);
        }
    }
}

void verifyBooleanIntersection(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base = makeBase(provider);
    CHECK(base.ok());
    const auto* top =
        newCap(
            base,
            kernel::ExtrudeCapRole::extent_cap);
    CHECK(top != nullptr && top->resolved_token);

    const auto cut =
        provider.extrude(
            extrude(
                rectangle(
                    10.0, 5.0,
                    30.0, 15.0,
                    5.0,
                    "tool"),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(cut.ok());

    const auto* tool_bottom =
        newSide(cut, "tool-bottom");
    CHECK(tool_bottom != nullptr);
    CHECK(tool_bottom->resolved_token.has_value());

    const auto matches =
        edgesForPair(
            cut,
            *top->resolved_token,
            *tool_bottom->resolved_token);
    CHECK(matches.size() == 1U);
    CHECK(
        matches.front()->role ==
        kernel::EdgeSemanticRoleKind::boolean_intersection);
    CHECK(
        matches.front()->curve_kind ==
        kernel::CurveKind::line);
    CHECK(
        matches.front()->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(matches.front()->candidate_count == 1U);
    CHECK(matches.front()->produced_by_current_operation);
}

void verifyMultiBranchNoWinner(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base = makeBase(provider);
    CHECK(base.ok());
    const auto* top =
        newCap(
            base,
            kernel::ExtrudeCapRole::extent_cap);
    const auto* front =
        newSide(base, "base-bottom");
    CHECK(top != nullptr && top->resolved_token);
    CHECK(front != nullptr && front->resolved_token);

    const auto cut =
        provider.extrude(
            extrude(
                rectangle(
                    15.0, -5.0,
                    25.0, 25.0,
                    5.0,
                    "branch-cut"),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(cut.ok());

    const auto branches =
        edgesForPair(
            cut,
            *top->resolved_token,
            *front->resolved_token);
    CHECK(branches.size() == 2U);
    for (const auto* edge : branches) {
        CHECK(
            edge->status ==
            kernel::ReferenceStatus::ambiguous);
        CHECK(edge->candidate_count == 2U);
        CHECK(edge->curve_kind == kernel::CurveKind::line);
    }
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    verifyPristine(provider);
    verifyCylinderSeam(provider);
    verifyBooleanHistory(provider);
    verifyBooleanIntersection(provider);
    verifyMultiBranchNoWinner(provider);

    std::cout
        << "PM02C_PRODUCTION_EDGE_VERTEX_SEMANTICS_PASS"
        << " provider_order_identity=0"
        << " geometry_similarity_identity=0"
        << " seam_referenceable=0"
        << " branch_auto_winner=0"
        << '\n';
    return EXIT_SUCCESS;
}
