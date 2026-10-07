#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05C2a production OCCT Edge Feature CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

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

kernel::LinearExtrudeInput boxInput() {
    kernel::LinearExtrudeInput input;
    input.profile.frame = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}};
    input.profile.outer.boundary = {
        lineUse(
            "bottom",
            0U,
            {0.0, 0.0},
            {40.0, 0.0}),
        lineUse(
            "right",
            1U,
            {40.0, 0.0},
            {40.0, 30.0}),
        lineUse(
            "top",
            2U,
            {40.0, 30.0},
            {0.0, 30.0}),
        lineUse(
            "left",
            3U,
            {0.0, 30.0},
            {0.0, 0.0}),
    };
    input.start_offset_mm = 0.0;
    input.end_offset_mm = 20.0;
    input.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());
    return input;
}

std::vector<kernel::RuntimeEdgeToken>
connectedPair(
    const kernel::SolidModelingResult& source) {
    for (const auto& vertex :
         source.current_vertex_semantics) {
        if (vertex.incident_material_edges.size() < 2U) {
            continue;
        }
        return {
            vertex.incident_material_edges[0],
            vertex.incident_material_edges[1]};
    }
    return {};
}

void verifyCompleteInventory(
    const char* stage,
    const kernel::SolidModelingResult& result) {
    if (!result.ok()) {
        std::cerr
            << "PM05C2A_RESULT"
            << " stage=" << stage
            << " status="
            << static_cast<int>(result.status)
            << " brep=" << (result.brep_valid ? 1 : 0)
            << " solids=" << result.solid_count
            << " faces=" << result.face_count
            << " edges=" << result.edge_count
            << " vertices=" << result.vertex_count
            << " inherited_surfaces="
            << result.inherited_surfaces.size()
            << " generated_surfaces="
            << result.edge_feature_surfaces.size()
            << " membership="
            << (result.edge_feature_input_membership
                    ? result.edge_feature_input_membership
                          ->provider_contour_edges.size()
                    : 0U)
            << '\n';
    }
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

void verifyInheritedRuntimeLineage(
    const kernel::SolidModelingResult& source,
    const kernel::SolidModelingResult& result) {
    CHECK(
        result.inherited_edge_realizations.size() ==
        source.current_edges.size());
    CHECK(
        result.inherited_vertex_realizations.size() ==
        source.current_vertices.size());
    CHECK(
        result.inherited_surfaces.size() ==
        source.new_surfaces.size());

    for (const auto& edge :
         result.inherited_edge_realizations) {
        CHECK(edge.source_token.valid());
        CHECK(
            edge.candidate_count ==
            edge.current_edges.size());
    }
    for (const auto& vertex :
         result.inherited_vertex_realizations) {
        CHECK(vertex.source_token.valid());
        CHECK(
            vertex.candidate_count ==
            vertex.current_vertices.size());
    }
}

class ForeignSolid final
    : public kernel::RuntimeSolid {};

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    const auto base =
        provider.extrude(
            boxInput());
    verifyCompleteInventory("base", base);
    const auto pair =
        connectedPair(base);
    CHECK(pair.size() == 2U);
    CHECK(pair[0] != pair[1]);

    // Constant-radius Fillet: connected explicit pair, one native operation,
    // exact provider contour set and complete inherited runtime lineage.
    kernel::EdgeFeatureInput fillet_input{
        kernel::EdgeFeatureOperation::fillet,
        pair,
        2.0};
    CHECK(fillet_input.valid());
    const auto fillet =
        provider.edgeFeature(
            fillet_input,
            base.solid);
    verifyCompleteInventory("fillet", fillet);
    CHECK(fillet.edge_feature_input_membership);
    CHECK(
        fillet.edge_feature_input_membership
            ->exactFor(pair));
    CHECK(
        fillet.edge_feature_surfaces.size() >=
        pair.size());
    verifyInheritedRuntimeLineage(
        base,
        fillet);

    // Provider traversal order carries no semantics.
    auto reverse_pair = pair;
    std::reverse(
        reverse_pair.begin(),
        reverse_pair.end());
    const auto reversed =
        provider.edgeFeature(
            {
                kernel::EdgeFeatureOperation::fillet,
                reverse_pair,
                2.0},
            base.solid);
    verifyCompleteInventory("reversed_fillet", reversed);
    CHECK(
        reversed.edge_feature_input_membership);
    CHECK(
        reversed.edge_feature_input_membership
            ->exactFor(reverse_pair));
    CHECK(
        reversed.face_count ==
        fillet.face_count);
    CHECK(
        reversed.edge_count ==
        fillet.edge_count);
    CHECK(
        reversed.vertex_count ==
        fillet.vertex_count);

    // Equal-distance Chamfer supports the same explicit connected set.
    const auto chamfer =
        provider.edgeFeature(
            {
                kernel::EdgeFeatureOperation::chamfer,
                pair,
                1.5},
            base.solid);
    verifyCompleteInventory("chamfer", chamfer);
    CHECK(chamfer.edge_feature_input_membership);
    CHECK(
        chamfer.edge_feature_input_membership
            ->exactFor(pair));
    CHECK(
        chamfer.edge_feature_surfaces.size() >=
        pair.size());
    verifyInheritedRuntimeLineage(
        base,
        chamfer);

    // Excessive geometry may fail, but T1 membership remains exact and the
    // provider never grows the authored set silently.
    const auto excessive =
        provider.edgeFeature(
            {
                kernel::EdgeFeatureOperation::fillet,
                pair,
                1000.0},
            base.solid);
    CHECK(!excessive.ok());
    CHECK(excessive.edge_feature_input_membership);
    CHECK(
        excessive.edge_feature_input_membership
            ->exactFor(pair));

    // Runtime token authority is the supplied upstream RuntimeSolid only.
    auto unknown = pair;
    unknown[0] =
        kernel::RuntimeEdgeToken{
            999999U};
    const auto missing_token =
        provider.edgeFeature(
            {
                kernel::EdgeFeatureOperation::fillet,
                unknown,
                2.0},
            base.solid);
    CHECK(!missing_token.ok());
    CHECK(
        missing_token.status ==
        kernel::SolidModelingStatus::
            provider_mismatch);

    const auto foreign =
        provider.edgeFeature(
            fillet_input,
            std::make_shared<ForeignSolid>());
    CHECK(!foreign.ok());
    CHECK(
        foreign.status ==
        kernel::SolidModelingStatus::
            provider_mismatch);

    std::cout
        << "PM05C2A_PRODUCTION_EDGE_PROVIDER_PASS"
        << " explicit_pair=1"
        << " t1_exact=1"
        << " order_invariant=1"
        << " global_token_registry=0"
        << " generated_runtime_surfaces=1\n";
    return EXIT_SUCCESS;
}
