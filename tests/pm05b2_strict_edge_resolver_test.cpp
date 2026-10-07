#include <simplesolid2/part/feature_evaluation.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05B2 strict-edge CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::FeatureId featureId(const char* text) {
    const auto parsed =
        part::FeatureId::parse(text);
    CHECK(parsed.has_value());
    return *parsed;
}

part::FeatureSurfaceAddress surface(
    part::FeatureId producer,
    part::FeatureSurfaceRoleKind role) {
    part::FeatureSurfaceAddress result{
        producer,
        role,
        std::nullopt,
        0U,
        0U,
        false};
    CHECK(result.valid());
    return result;
}

part::FeatureCurveAddress curve(
    part::FeatureId producer,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeatureCurveAddress result{
        producer,
        part::FeatureCurveRoleKind::cap_side,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

part::FeaturePointAddress point(
    part::FeatureId producer,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second,
    part::FeatureSurfaceAddress third) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second),
            std::move(third)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeaturePointAddress result{
        producer,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

struct Vocabulary final {
    part::FeatureId producer{featureId("1")};
    part::BodyStageRef stage{
        part::BodyStageKind::after_feature,
        producer};
    part::FeatureSurfaceAddress first{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::
                profile_cap)};
    part::FeatureSurfaceAddress second{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::
                extent_cap)};
    part::FeatureSurfaceAddress third{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::
                negative_cap)};
    part::FeatureSurfaceAddress fourth{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::
                positive_cap)};
    part::FeatureCurveAddress curve_address{
        curve(
            producer,
            first,
            second)};
    part::FeaturePointAddress first_point{
        point(
            producer,
            first,
            second,
            third)};
    part::FeaturePointAddress second_point{
        point(
            producer,
            first,
            second,
            fourth)};
};

part::BodyEdgeTopologyRecord edgeRecord(
    kernel::RuntimeEdgeToken token,
    const part::FeatureCurveAddress& address,
    kernel::ReferenceStatus referenceability =
        kernel::ReferenceStatus::resolved) {
    part::BodyEdgeTopologyRecord result;
    result.runtime_token = token;
    result.accounting_class =
        part::TopologyAccountingClass::referenceable;
    result.referenceability =
        referenceability;
    result.curve_kind =
        kernel::CurveKind::line;
    result.curve_candidates = {address};
    CHECK(result.valid());
    return result;
}

part::BodyVertexTopologyRecord vertexRecord(
    kernel::RuntimeVertexToken token,
    const part::FeaturePointAddress& address,
    std::vector<kernel::RuntimeEdgeToken> edges,
    kernel::ReferenceStatus referenceability =
        kernel::ReferenceStatus::resolved) {
    part::BodyVertexTopologyRecord result;
    result.runtime_token = token;
    result.accounting_class =
        part::TopologyAccountingClass::referenceable;
    result.referenceability =
        referenceability;
    result.point_candidates = {address};
    result.incident_material_edges =
        std::move(edges);
    result.incident_material_edge_count =
        result.incident_material_edges.size();
    CHECK(result.valid());
    return result;
}

part::BodyStageTopologyCatalog singularCatalog(
    const Vocabulary& v,
    kernel::ReferenceStatus curve_status,
    std::vector<kernel::RuntimeEdgeToken> edges) {
    part::BodyStageTopologyCatalog catalog;
    catalog.stage = v.stage;

    part::FeatureCurveResolution resolution;
    resolution.address = v.curve_address;
    resolution.status = curve_status;
    resolution.curve_kind =
        kernel::CurveKind::line;
    resolution.current_edges = edges;
    resolution.candidate_edge_count =
        edges.size();
    if (curve_status ==
            kernel::ReferenceStatus::resolved) {
        resolution.strict_edge_status =
            edges.size() == 1U
                ? kernel::ReferenceStatus::resolved
                : kernel::ReferenceStatus::ambiguous;
    } else {
        resolution.strict_edge_status =
            curve_status;
    }
    CHECK(resolution.valid());
    catalog.curves.push_back(
        std::move(resolution));

    for (const auto edge : edges) {
        catalog.edges.push_back(
            edgeRecord(
                edge,
                v.curve_address,
                edges.size() == 1U
                    ? kernel::ReferenceStatus::resolved
                    : kernel::ReferenceStatus::ambiguous));
    }
    CHECK(catalog.complete());
    return catalog;
}

part::BodyStageTopologyCatalog branchedCatalog(
    const Vocabulary& v) {
    constexpr kernel::RuntimeEdgeToken edge1{101U};
    constexpr kernel::RuntimeEdgeToken edge2{102U};
    constexpr kernel::RuntimeVertexToken vertex1{201U};
    constexpr kernel::RuntimeVertexToken vertex2{202U};

    part::BodyStageTopologyCatalog catalog;
    catalog.stage = v.stage;
    catalog.edges = {
        edgeRecord(
            edge1,
            v.curve_address,
            kernel::ReferenceStatus::ambiguous),
        edgeRecord(
            edge2,
            v.curve_address,
            kernel::ReferenceStatus::ambiguous)};

    part::FeatureCurveResolution curve_resolution;
    curve_resolution.address =
        v.curve_address;
    curve_resolution.status =
        kernel::ReferenceStatus::ambiguous;
    curve_resolution.strict_edge_status =
        kernel::ReferenceStatus::ambiguous;
    curve_resolution.candidate_edge_count = 2U;
    curve_resolution.curve_kind =
        kernel::CurveKind::line;
    curve_resolution.current_edges =
        {edge1, edge2};
    CHECK(curve_resolution.valid());
    catalog.curves.push_back(
        curve_resolution);

    part::FeaturePointResolution first;
    first.address = v.first_point;
    first.status =
        kernel::ReferenceStatus::resolved;
    first.candidate_vertex_count = 1U;
    first.current_vertices = {vertex1};
    CHECK(first.valid());

    part::FeaturePointResolution second;
    second.address = v.second_point;
    second.status =
        kernel::ReferenceStatus::resolved;
    second.candidate_vertex_count = 1U;
    second.current_vertices = {vertex2};
    CHECK(second.valid());

    catalog.points = {first, second};
    catalog.vertices = {
        vertexRecord(
            vertex1,
            v.first_point,
            {edge1}),
        vertexRecord(
            vertex2,
            v.second_point,
            {edge1})};

    CHECK(catalog.complete());
    return catalog;
}

} // namespace

int main() {
    const Vocabulary v;
    constexpr kernel::RuntimeEdgeToken edge1{101U};
    constexpr kernel::RuntimeEdgeToken edge2{102U};

    // Singular authored meaning follows strict bounded-Edge survival:
    // 1 -> Resolved, 0 -> Missing, split -> Ambiguous.
    {
        auto catalog =
            singularCatalog(
                v,
                kernel::ReferenceStatus::resolved,
                {edge1});
        const part::MaterialEdgeReference reference{
            v.stage,
            v.curve_address,
            part::SingularAtAuthoredStage{}};
        const auto resolved =
            part::resolveMaterialEdgeReference(
                reference,
                catalog);
        CHECK(resolved.has_value());
        CHECK(resolved->resolved());
        CHECK(
            resolved->current_edges.front() ==
            edge1);

        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                edge1);
        CHECK(authored.ok());
        CHECK(
            std::holds_alternative<
                part::SingularAtAuthoredStage>(
                authored.reference->branch));
    }
    {
        auto catalog =
            singularCatalog(
                v,
                kernel::ReferenceStatus::missing,
                {});
        const part::MaterialEdgeReference reference{
            v.stage,
            v.curve_address,
            part::SingularAtAuthoredStage{}};
        const auto resolved =
            part::resolveMaterialEdgeReference(
                reference,
                catalog);
        CHECK(resolved.has_value());
        CHECK(
            resolved->status ==
            kernel::ReferenceStatus::missing);
        CHECK(resolved->current_edges.empty());
    }
    {
        auto catalog =
            singularCatalog(
                v,
                kernel::ReferenceStatus::resolved,
                {edge1, edge2});
        const part::MaterialEdgeReference reference{
            v.stage,
            v.curve_address,
            part::SingularAtAuthoredStage{}};
        const auto resolved =
            part::resolveMaterialEdgeReference(
                reference,
                catalog);
        CHECK(resolved.has_value());
        CHECK(
            resolved->status ==
            kernel::ReferenceStatus::ambiguous);
        CHECK(
            resolved->current_edges.size() ==
            2U);
    }

    // A semantic Point pair disambiguates exactly one bounded branch from a
    // multi-branch Curve family without coordinates or provider ordinals.
    {
        auto catalog =
            branchedCatalog(v);
        part::MaterialEdgeReference reference{
            v.stage,
            v.curve_address,
            part::BetweenSemanticPoints{
                v.first_point,
                v.second_point}};
        CHECK(reference.valid());

        const auto resolved =
            part::resolveMaterialEdgeReference(
                reference,
                catalog);
        CHECK(resolved.has_value());
        CHECK(resolved->resolved());
        CHECK(
            resolved->current_edges.front() ==
            edge1);

        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                edge1);
        CHECK(authored.ok());
        CHECK(
            std::holds_alternative<
                part::BetweenSemanticPoints>(
                authored.reference->branch));
        CHECK(
            part::resolveMaterialEdgeReference(
                *authored.reference,
                catalog)
                ->current_edges.front() ==
            edge1);

        // If the same authored Point pair carries two current branches, the
        // branch remains Ambiguous; never choose first/nearest/longest.
        catalog.vertices[0]
            .incident_material_edges
            .push_back(edge2);
        catalog.vertices[0]
            .incident_material_edge_count = 2U;
        catalog.vertices[1]
            .incident_material_edges
            .push_back(edge2);
        catalog.vertices[1]
            .incident_material_edge_count = 2U;
        CHECK(catalog.complete());

        const auto split =
            part::resolveMaterialEdgeReference(
                reference,
                catalog);
        CHECK(split.has_value());
        CHECK(
            split->status ==
            kernel::ReferenceStatus::ambiguous);
        CHECK(split->current_edges.size() == 2U);
    }

    // Multi-branch authoring without two defensible semantic endpoint Points
    // is explicitly Unsupported, not geometry-matched.
    {
        auto catalog =
            branchedCatalog(v);
        catalog.vertices.clear();
        catalog.points.clear();
        CHECK(catalog.complete());
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                edge1);
        CHECK(!authored.ok());
        CHECK(
            authored.status ==
            kernel::ReferenceStatus::unsupported);
    }

    // Resolution is stage-scoped. Passing final/other-stage evidence is an
    // invalid runtime context rather than an invitation to search globally.
    {
        auto catalog =
            singularCatalog(
                v,
                kernel::ReferenceStatus::resolved,
                {edge1});
        const auto other =
            featureId("2");
        catalog.stage = {
            part::BodyStageKind::after_feature,
            other};
        CHECK(catalog.complete());

        const part::MaterialEdgeReference reference{
            v.stage,
            v.curve_address,
            part::SingularAtAuthoredStage{}};
        CHECK(
            !part::resolveMaterialEdgeReference(
                 reference,
                 catalog)
                 .has_value());
    }

    std::cout
        << "PM05B2_STRICT_EDGE_RESOLVER_PASS"
        << " singular=3"
        << " point_pair=resolved_ambiguous"
        << " geometry_fallback=0"
        << " stage_global_search=0\n";
    return EXIT_SUCCESS;
}
