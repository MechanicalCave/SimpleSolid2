#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/edge_feature_evidence.hpp>
#include <simplesolid2/kernel_occt/edge_feature_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05F Fillet diagnostics CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::PartDocument makeBasePart() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch_created.ok() && sketch_created.sketch_id);

    CHECK(
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false})
            .ok());

    const auto* sketch =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto regions =
        sketch::analyzeRegions(sketch->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    auto state = session.document().state();
    const auto base_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id);
    state.body.features.push_back(
        part::PartFeature{
            *base_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

std::vector<part::MaterialEdgeReference>
trihedralReferences(
    const part::BodyStageTopologyCatalog& catalog) {
    for (const auto& vertex : catalog.vertices) {
        if (vertex.referenceability !=
                kernel::ReferenceStatus::resolved ||
            vertex.point_candidates.size() != 1U ||
            vertex.incident_material_edges.size() < 3U) {
            continue;
        }

        std::vector<part::MaterialEdgeReference> result;
        for (std::size_t index = 0U; index < 3U; ++index) {
            const auto authored =
                part::authorMaterialEdgeReference(
                    catalog,
                    vertex.incident_material_edges[index]);
            if (!authored.ok()) {
                result.clear();
                break;
            }
            result.push_back(*authored.reference);
        }
        if (result.size() != 3U) {
            continue;
        }
        std::sort(result.begin(), result.end());
        if (std::adjacent_find(
                result.begin(),
                result.end()) == result.end()) {
            return result;
        }
    }
    return {};
}

part::PartDocument withEdgeFeature(
    const part::PartDocument& source,
    std::vector<part::MaterialEdgeReference> edges,
    kernel::EdgeFeatureOperation operation,
    double parameter) {
    std::sort(edges.begin(), edges.end());

    auto state = source.state();
    const auto id =
        state.body.next_feature_id.allocate();
    CHECK(id);

    part::PartFeature feature;
    feature.id = *id;
    feature.name =
        operation == kernel::EdgeFeatureOperation::fillet
            ? "FilletDiagnostic"
            : "ChamferDiagnostic";
    feature.suppressed = false;
    if (operation == kernel::EdgeFeatureOperation::fillet) {
        feature.definition =
            part::FilletFeature{
                std::move(edges),
                core::LengthValue{parameter}};
    } else {
        feature.definition =
            part::ChamferFeature{
                std::move(edges),
                core::LengthValue{parameter}};
    }
    state.body.features.push_back(std::move(feature));

    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

struct CaseResult final {
    bool input_resolved{false};
    kernel::SolidModelingStatus provider_status{
        kernel::SolidModelingStatus::provider_failure};
    std::size_t requested_count{};
    std::size_t provider_contour_count{};
    bool exact_membership{false};
    bool provider_ok{false};
    bool provider_brep_valid{false};
    std::size_t provider_solid_count{};
    std::size_t generated_surface_count{};
    part::FeatureEvaluationStatus product_status{
        part::FeatureEvaluationStatus::blocked};
    part::FeatureEvaluationDiagnosticCode product_diagnostic{
        part::FeatureEvaluationDiagnosticCode::none};
    std::optional<kernel::SolidModelingStatus>
        product_kernel_status;
    bool product_topology_complete{false};
};

CaseResult runCase(
    const part::PartDocument& base,
    const part::FeatureEvaluation& upstream,
    std::vector<part::MaterialEdgeReference> edges,
    kernel::EdgeFeatureOperation operation,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    CaseResult result;
    result.requested_count = edges.size();
    std::sort(edges.begin(), edges.end());

    std::optional<kernel::EdgeFeatureInput> input;
    if (operation == kernel::EdgeFeatureOperation::fillet) {
        const part::FilletFeature definition{
            edges,
            core::LengthValue{2.0}};
        const auto resolved =
            part::resolveKernelEdgeFeatureInput(
                definition,
                upstream.result_topology
                    ? &*upstream.result_topology
                    : nullptr);
        result.input_resolved = resolved.ok();
        input = resolved.input;
    } else {
        const part::ChamferFeature definition{
            edges,
            core::LengthValue{1.5}};
        const auto resolved =
            part::resolveKernelEdgeFeatureInput(
                definition,
                upstream.result_topology
                    ? &*upstream.result_topology
                    : nullptr);
        result.input_resolved = resolved.ok();
        input = resolved.input;
    }

    if (input) {
        const auto provider =
            kernel.edgeFeature(
                *input,
                upstream.result_solid);
        result.provider_status = provider.status;
        result.provider_ok = provider.ok();
        result.provider_brep_valid = provider.brep_valid;
        result.provider_solid_count = provider.solid_count;
        result.generated_surface_count =
            provider.edge_feature_surfaces.size();
        if (provider.edge_feature_input_membership) {
            result.provider_contour_count =
                provider.edge_feature_input_membership
                    ->provider_contour_edges.size();
            result.exact_membership =
                provider.edge_feature_input_membership
                    ->exactFor(input->edges);
        }
    }

    auto document =
        withEdgeFeature(
            base,
            edges,
            operation,
            operation == kernel::EdgeFeatureOperation::fillet
                ? 2.0
                : 1.5);
    const auto evaluation =
        part::evaluatePart(
            document,
            kernel);
    CHECK(evaluation.features.size() == 2U);
    const auto& feature = evaluation.features[1];
    result.product_status = feature.status;
    result.product_diagnostic = feature.diagnostic;
    result.product_kernel_status = feature.kernel_status;
    result.product_topology_complete =
        feature.result_topology &&
        feature.result_topology->complete();
    return result;
}

std::string_view operationName(
    kernel::EdgeFeatureOperation operation) {
    return operation == kernel::EdgeFeatureOperation::fillet
        ? "fillet"
        : "chamfer";
}

void printCase(
    std::string_view name,
    kernel::EdgeFeatureOperation operation,
    const CaseResult& result) {
    std::cout
        << "PM05F_FILLET_DIAG"
        << " case=" << name
        << " op=" << operationName(operation)
        << " requested=" << result.requested_count
        << " input_resolved=" << (result.input_resolved ? 1 : 0)
        << " provider_status="
        << static_cast<int>(result.provider_status)
        << " provider_ok=" << (result.provider_ok ? 1 : 0)
        << " provider_contour_edges="
        << result.provider_contour_count
        << " exact_membership="
        << (result.exact_membership ? 1 : 0)
        << " brep_valid="
        << (result.provider_brep_valid ? 1 : 0)
        << " solid_count=" << result.provider_solid_count
        << " generated_surfaces="
        << result.generated_surface_count
        << " product_status="
        << static_cast<int>(result.product_status)
        << " product_diagnostic="
        << static_cast<int>(result.product_diagnostic)
        << " product_kernel_status="
        << (result.product_kernel_status
                ? static_cast<int>(*result.product_kernel_status)
                : -1)
        << " product_topology_complete="
        << (result.product_topology_complete ? 1 : 0)
        << '\n';
}

} // namespace

int main() {
    // Raw-OCCT control: PM-05A already established that adjacent-pair and
    // trihedral Fillets are constructible on the same 40x30x20 box.
    const auto raw_adjacent =
        kernel_occt::buildEdgeFeatureProviderEvidence(
            kernel::EdgeFeatureEvidenceOperation::fillet,
            kernel::EdgeFeatureProbeScenario::adjacent_pair);
    const auto raw_trihedral =
        kernel_occt::buildEdgeFeatureProviderEvidence(
            kernel::EdgeFeatureEvidenceOperation::fillet,
            kernel::EdgeFeatureProbeScenario::trihedral_corner);
    CHECK(raw_adjacent.sourceReady());
    CHECK(raw_adjacent.build_succeeded);
    CHECK(raw_trihedral.sourceReady());
    CHECK(raw_trihedral.build_succeeded);

    kernel_occt::OcctSolidModelingKernel kernel;
    const auto base = makeBasePart();
    const auto base_eval =
        part::evaluatePart(base, kernel);
    CHECK(
        base_eval.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(base_eval.features.size() == 1U);
    CHECK(base_eval.features[0].result_solid);
    CHECK(base_eval.features[0].result_topology);
    CHECK(base_eval.features[0].result_topology->complete());

    const auto trihedral =
        trihedralReferences(
            *base_eval.features[0].result_topology);
    CHECK(trihedral.size() == 3U);

    const std::vector<part::MaterialEdgeReference> one{
        trihedral[0]};
    const std::vector<part::MaterialEdgeReference> adjacent{
        trihedral[0],
        trihedral[1]};

    const auto fillet_one =
        runCase(
            base,
            base_eval.features[0],
            one,
            kernel::EdgeFeatureOperation::fillet,
            kernel);
    const auto fillet_two =
        runCase(
            base,
            base_eval.features[0],
            adjacent,
            kernel::EdgeFeatureOperation::fillet,
            kernel);
    const auto fillet_three =
        runCase(
            base,
            base_eval.features[0],
            trihedral,
            kernel::EdgeFeatureOperation::fillet,
            kernel);
    const auto chamfer_two =
        runCase(
            base,
            base_eval.features[0],
            adjacent,
            kernel::EdgeFeatureOperation::chamfer,
            kernel);
    const auto chamfer_three =
        runCase(
            base,
            base_eval.features[0],
            trihedral,
            kernel::EdgeFeatureOperation::chamfer,
            kernel);

    printCase(
        "one_edge",
        kernel::EdgeFeatureOperation::fillet,
        fillet_one);
    printCase(
        "adjacent_pair",
        kernel::EdgeFeatureOperation::fillet,
        fillet_two);
    printCase(
        "trihedral",
        kernel::EdgeFeatureOperation::fillet,
        fillet_three);
    printCase(
        "adjacent_pair",
        kernel::EdgeFeatureOperation::chamfer,
        chamfer_two);
    printCase(
        "trihedral",
        kernel::EdgeFeatureOperation::chamfer,
        chamfer_three);

    // Controls already accepted by PM-05C2b. The diagnostic deliberately
    // does not require adjacent-pair Fillet product success yet: that is the
    // Owner-reported acceptance blocker being classified.
    CHECK(fillet_one.input_resolved);
    CHECK(fillet_one.provider_ok);
    CHECK(
        fillet_one.product_status ==
        part::FeatureEvaluationStatus::up_to_date);

    CHECK(fillet_two.input_resolved);

    CHECK(fillet_three.input_resolved);
    CHECK(fillet_three.provider_ok);
    CHECK(
        fillet_three.product_status ==
        part::FeatureEvaluationStatus::up_to_date);

    CHECK(chamfer_two.input_resolved);
    CHECK(chamfer_two.provider_ok);
    CHECK(
        chamfer_two.product_status ==
        part::FeatureEvaluationStatus::up_to_date);

    CHECK(chamfer_three.input_resolved);
    CHECK(chamfer_three.provider_ok);
    CHECK(
        chamfer_three.product_status ==
        part::FeatureEvaluationStatus::up_to_date);

    std::cout
        << "PM05F_FILLET_DIAGNOSTIC_PASS"
        << " raw_adjacent_build=1"
        << " raw_trihedral_build=1"
        << " adjacent_product_ok="
        << (fillet_two.product_status ==
                    part::FeatureEvaluationStatus::up_to_date
                ? 1
                : 0)
        << '\n';
    return EXIT_SUCCESS;
}
