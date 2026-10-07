#include <simplesolid2/part/feature_evaluation.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05E4 lifecycle matrix CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::FeatureId featureId(const char* text) {
    const auto parsed = part::FeatureId::parse(text);
    CHECK(parsed);
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
    std::vector<part::FeatureSurfaceAddress> adjacent{
        std::move(first),
        std::move(second)};
    std::sort(adjacent.begin(), adjacent.end());
    part::FeatureCurveAddress result{
        producer,
        part::FeatureCurveRoleKind::cap_side,
        std::move(adjacent)};
    CHECK(result.valid());
    return result;
}

struct Vocabulary final {
    part::FeatureId producer{featureId("1")};
    part::BodyStageRef stage{
        part::BodyStageKind::after_feature,
        producer};
    part::FeatureSurfaceAddress cap{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::profile_cap)};
    part::FeatureSurfaceAddress extent{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::extent_cap)};
    part::FeatureSurfaceAddress negative{
        surface(
            producer,
            part::FeatureSurfaceRoleKind::negative_cap)};
    part::FeatureCurveAddress first_curve{
        curve(producer, cap, extent)};
    part::FeatureCurveAddress second_curve{
        curve(producer, cap, negative)};
};

part::MaterialEdgeReference reference(
    const Vocabulary& v,
    const part::FeatureCurveAddress& address) {
    part::MaterialEdgeReference result{
        v.stage,
        address,
        part::SingularAtAuthoredStage{}};
    CHECK(result.valid());
    return result;
}

part::FeatureCurveResolution curveResolution(
    const part::FeatureCurveAddress& address,
    kernel::ReferenceStatus status,
    std::vector<kernel::RuntimeEdgeToken> edges) {
    part::FeatureCurveResolution result;
    result.address = address;
    result.status = status;
    result.curve_kind = kernel::CurveKind::line;
    result.current_edges = std::move(edges);
    result.candidate_edge_count =
        result.current_edges.size();
    if (status == kernel::ReferenceStatus::resolved) {
        result.strict_edge_status =
            result.current_edges.size() == 1U
                ? kernel::ReferenceStatus::resolved
                : kernel::ReferenceStatus::ambiguous;
    } else {
        result.strict_edge_status = status;
    }
    CHECK(result.valid());
    return result;
}

part::BodyEdgeTopologyRecord edgeRecord(
    kernel::RuntimeEdgeToken token,
    std::vector<part::FeatureCurveAddress> candidates,
    kernel::ReferenceStatus referenceability) {
    std::sort(candidates.begin(), candidates.end());
    part::BodyEdgeTopologyRecord result;
    result.runtime_token = token;
    result.accounting_class =
        part::TopologyAccountingClass::referenceable;
    result.referenceability = referenceability;
    result.curve_kind = kernel::CurveKind::line;
    result.curve_candidates = std::move(candidates);
    CHECK(result.valid());
    return result;
}

part::BodyStageTopologyCatalog oneCurveCatalog(
    const Vocabulary& v,
    kernel::ReferenceStatus status,
    std::vector<kernel::RuntimeEdgeToken> edges) {
    part::BodyStageTopologyCatalog catalog;
    catalog.stage = v.stage;
    catalog.curves.push_back(
        curveResolution(
            v.first_curve,
            status,
            edges));

    for (const auto token : edges) {
        catalog.edges.push_back(
            edgeRecord(
                token,
                {v.first_curve},
                edges.size() == 1U
                    ? kernel::ReferenceStatus::resolved
                    : kernel::ReferenceStatus::ambiguous));
    }
    CHECK(catalog.complete());
    return catalog;
}

part::BodyStageTopologyCatalog mergedCurveCatalog(
    const Vocabulary& v) {
    constexpr kernel::RuntimeEdgeToken merged{101U};

    part::BodyStageTopologyCatalog catalog;
    catalog.stage = v.stage;
    catalog.curves = {
        curveResolution(
            v.first_curve,
            kernel::ReferenceStatus::resolved,
            {merged}),
        curveResolution(
            v.second_curve,
            kernel::ReferenceStatus::resolved,
            {merged})};
    catalog.edges = {
        edgeRecord(
            merged,
            {v.first_curve, v.second_curve},
            kernel::ReferenceStatus::ambiguous)};
    CHECK(catalog.complete());
    return catalog;
}

template <typename Feature>
void expectResolved(
    const Feature& feature,
    const part::BodyStageTopologyCatalog& catalog,
    kernel::EdgeFeatureOperation operation) {
    const auto result =
        part::resolveKernelEdgeFeatureInput(
            feature,
            &catalog);
    CHECK(result.ok());
    CHECK(result.input);
    CHECK(result.input->operation == operation);
    CHECK(result.input->edges.size() == 1U);
    CHECK(
        result.input->edges.front() ==
        kernel::RuntimeEdgeToken{101U});
    CHECK(!result.failing_edge_input_index);
    CHECK(!result.reference_status);
}

template <typename Feature>
void expectBlocked(
    const Feature& feature,
    const part::BodyStageTopologyCatalog& catalog,
    part::EdgeFeatureKernelInputStatus status,
    kernel::ReferenceStatus reference_status) {
    const auto result =
        part::resolveKernelEdgeFeatureInput(
            feature,
            &catalog);
    CHECK(!result.ok());
    CHECK(!result.input);
    CHECK(result.status == status);
    CHECK(
        result.failing_edge_input_index ==
        std::optional<std::size_t>{0U});
    CHECK(
        result.reference_status ==
        std::optional<kernel::ReferenceStatus>{
            reference_status});
}

} // namespace

int main() {
    const Vocabulary v;
    const auto first = reference(v, v.first_curve);

    const part::FilletFeature fillet{
        {first},
        core::LengthValue{2.0}};
    const part::ChamferFeature chamfer{
        {first},
        core::LengthValue{1.0}};
    CHECK(part::filletFeatureStructurallyValid(fillet));
    CHECK(part::chamferFeatureStructurallyValid(chamfer));

    // Preserve: one semantic Curve remains one strict material Edge.
    const auto preserved =
        oneCurveCatalog(
            v,
            kernel::ReferenceStatus::resolved,
            {kernel::RuntimeEdgeToken{101U}});
    expectResolved(
        fillet,
        preserved,
        kernel::EdgeFeatureOperation::fillet);
    expectResolved(
        chamfer,
        preserved,
        kernel::EdgeFeatureOperation::chamfer);

    // Split: one durable singular Edge meaning now has multiple bounded
    // realizations. It is Ambiguous; no first/nearest/longest winner exists.
    const auto split =
        oneCurveCatalog(
            v,
            kernel::ReferenceStatus::resolved,
            {
                kernel::RuntimeEdgeToken{101U},
                kernel::RuntimeEdgeToken{102U}});
    expectBlocked(
        fillet,
        split,
        part::EdgeFeatureKernelInputStatus::ambiguous_edge,
        kernel::ReferenceStatus::ambiguous);
    expectBlocked(
        chamfer,
        split,
        part::EdgeFeatureKernelInputStatus::ambiguous_edge,
        kernel::ReferenceStatus::ambiguous);

    // Remove: semantic carrier is explicitly Missing.
    const auto removed =
        oneCurveCatalog(
            v,
            kernel::ReferenceStatus::missing,
            {});
    expectBlocked(
        fillet,
        removed,
        part::EdgeFeatureKernelInputStatus::missing_edge,
        kernel::ReferenceStatus::missing);
    expectBlocked(
        chamfer,
        removed,
        part::EdgeFeatureKernelInputStatus::missing_edge,
        kernel::ReferenceStatus::missing);

    // Unsupported remains distinct from Missing/Ambiguous for repair UI.
    const auto unsupported =
        oneCurveCatalog(
            v,
            kernel::ReferenceStatus::unsupported,
            {});
    expectBlocked(
        fillet,
        unsupported,
        part::EdgeFeatureKernelInputStatus::unsupported_edge,
        kernel::ReferenceStatus::unsupported);
    expectBlocked(
        chamfer,
        unsupported,
        part::EdgeFeatureKernelInputStatus::unsupported_edge,
        kernel::ReferenceStatus::unsupported);

    // Merge: two durable Curve meanings now share one runtime bounded Edge.
    // The record truthfully carries both semantic candidates, therefore each
    // strict singular input is Ambiguous rather than silently choosing one.
    auto merged_edges =
        std::vector<part::MaterialEdgeReference>{
            first,
            reference(v, v.second_curve)};
    std::sort(merged_edges.begin(), merged_edges.end());
    const part::FilletFeature merged_fillet{
        merged_edges,
        core::LengthValue{2.0}};
    const part::ChamferFeature merged_chamfer{
        merged_edges,
        core::LengthValue{1.0}};
    CHECK(
        part::filletFeatureStructurallyValid(
            merged_fillet));
    CHECK(
        part::chamferFeatureStructurallyValid(
            merged_chamfer));

    const auto merged = mergedCurveCatalog(v);
    expectBlocked(
        merged_fillet,
        merged,
        part::EdgeFeatureKernelInputStatus::ambiguous_edge,
        kernel::ReferenceStatus::ambiguous);
    expectBlocked(
        merged_chamfer,
        merged,
        part::EdgeFeatureKernelInputStatus::ambiguous_edge,
        kernel::ReferenceStatus::ambiguous);

    std::cout
        << "PM05E4_EDGE_LIFECYCLE_MATRIX_PASS"
        << " preserve=resolved"
        << " split=ambiguous"
        << " merge=ambiguous"
        << " remove=missing"
        << " unsupported=unsupported"
        << " geometry_fallback=0\n";
    return EXIT_SUCCESS;
}
