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

struct AuthorableEdge final {
    kernel::RuntimeEdgeToken token;
    part::MaterialEdgeReference reference;
};

std::vector<AuthorableEdge> authorableEdges(
    const part::BodyStageTopologyCatalog& catalog) {
    std::vector<AuthorableEdge> result;
    for (const auto& edge : catalog.edges) {
        if (edge.accounting_class !=
                part::TopologyAccountingClass::referenceable ||
            edge.referenceability !=
                kernel::ReferenceStatus::resolved ||
            edge.periodic_seam ||
            edge.representation_partition) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                edge.runtime_token);
        if (!authored.ok()) {
            continue;
        }
        result.push_back(
            {edge.runtime_token, *authored.reference});
    }
    std::sort(
        result.begin(),
        result.end(),
        [](const auto& first, const auto& second) {
            return first.reference < second.reference;
        });
    return result;
}

bool shareVertex(
    const part::BodyStageTopologyCatalog& catalog,
    kernel::RuntimeEdgeToken first,
    kernel::RuntimeEdgeToken second) {
    for (const auto& vertex : catalog.vertices) {
        const auto& incident =
            vertex.incident_material_edges;
        if (std::find(
                incident.begin(),
                incident.end(),
                first) != incident.end() &&
            std::find(
                incident.begin(),
                incident.end(),
                second) != incident.end()) {
            return true;
        }
    }
    return false;
}

std::vector<std::vector<part::MaterialEdgeReference>>
trihedralSets(
    const part::BodyStageTopologyCatalog& catalog) {
    std::vector<std::vector<part::MaterialEdgeReference>> result;
    for (const auto& vertex : catalog.vertices) {
        if (vertex.referenceability !=
                kernel::ReferenceStatus::resolved ||
            vertex.incident_material_edges.size() != 3U) {
            continue;
        }
        std::vector<part::MaterialEdgeReference> refs;
        for (const auto token :
             vertex.incident_material_edges) {
            const auto authored =
                part::authorMaterialEdgeReference(
                    catalog,
                    token);
            if (!authored.ok()) {
                refs.clear();
                break;
            }
            refs.push_back(*authored.reference);
        }
        if (refs.size() != 3U) {
            continue;
        }
        std::sort(refs.begin(), refs.end());
        if (std::adjacent_find(
                refs.begin(),
                refs.end()) != refs.end()) {
            continue;
        }
        if (std::find(
                result.begin(),
                result.end(),
                refs) == result.end()) {
            result.push_back(std::move(refs));
        }
    }
    return result;
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
    std::size_t edge_feature_boundary_other_count{};
    std::size_t edge_feature_boundary_line_count{};
    std::size_t edge_feature_boundary_circle_count{};
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

        std::vector<kernel::RuntimeSurfaceToken>
            generated_surface_tokens;
        generated_surface_tokens.reserve(
            provider.edge_feature_surfaces.size());
        for (const auto& surface :
             provider.edge_feature_surfaces) {
            generated_surface_tokens.push_back(
                surface.runtime_token);
        }

        for (const auto& observation :
             provider.current_edge_semantics) {
            const bool touches_edge_feature_surface =
                std::any_of(
                    observation.adjacent_surfaces.begin(),
                    observation.adjacent_surfaces.end(),
                    [&generated_surface_tokens](
                        kernel::RuntimeSurfaceToken token) {
                        return std::find(
                                   generated_surface_tokens.begin(),
                                   generated_surface_tokens.end(),
                                   token) !=
                               generated_surface_tokens.end();
                    });
            if (!touches_edge_feature_surface) {
                continue;
            }
            switch (observation.provider_curve_kind) {
            case kernel::CurveKind::line:
                ++result.edge_feature_boundary_line_count;
                break;
            case kernel::CurveKind::circle:
                ++result.edge_feature_boundary_circle_count;
                break;
            case kernel::CurveKind::other:
                ++result.edge_feature_boundary_other_count;
                break;
            }
        }

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
        << " edge_boundary_line="
        << result.edge_feature_boundary_line_count
        << " edge_boundary_circle="
        << result.edge_feature_boundary_circle_count
        << " edge_boundary_other="
        << result.edge_feature_boundary_other_count
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

    const auto all_edges =
        authorableEdges(
            *base_eval.features[0].result_topology);
    CHECK(all_edges.size() == 12U);

    std::size_t pair_count = 0U;
    std::size_t adjacent_pair_count = 0U;
    std::size_t fillet_pair_successes = 0U;
    std::size_t chamfer_pair_successes = 0U;
    std::size_t adjacent_fillet_failures = 0U;

    for (std::size_t first = 0U;
         first + 1U < all_edges.size();
         ++first) {
        for (std::size_t second = first + 1U;
             second < all_edges.size();
             ++second) {
            ++pair_count;
            const bool adjacent_pair =
                shareVertex(
                    *base_eval.features[0].result_topology,
                    all_edges[first].token,
                    all_edges[second].token);
            if (adjacent_pair) {
                ++adjacent_pair_count;
            }

            const std::vector<part::MaterialEdgeReference>
                pair_refs{
                    all_edges[first].reference,
                    all_edges[second].reference};

            const auto fillet_pair =
                runCase(
                    base,
                    base_eval.features[0],
                    pair_refs,
                    kernel::EdgeFeatureOperation::fillet,
                    kernel);
            const bool fillet_ok =
                fillet_pair.product_status ==
                    part::FeatureEvaluationStatus::up_to_date &&
                fillet_pair.product_topology_complete;
            if (fillet_ok) {
                ++fillet_pair_successes;
            } else {
                if (adjacent_pair) {
                    ++adjacent_fillet_failures;
                }
                std::cerr
                    << "PM05F_CUBE_PAIR_FAIL"
                    << " op=fillet"
                    << " i=" << first
                    << " j=" << second
                    << " token_i="
                    << all_edges[first].token.value
                    << " token_j="
                    << all_edges[second].token.value
                    << " adjacent="
                    << (adjacent_pair ? 1 : 0)
                    << " input_resolved="
                    << (fillet_pair.input_resolved ? 1 : 0)
                    << " provider_status="
                    << static_cast<int>(
                           fillet_pair.provider_status)
                    << " provider_ok="
                    << (fillet_pair.provider_ok ? 1 : 0)
                    << " provider_contour_edges="
                    << fillet_pair.provider_contour_count
                    << " exact_membership="
                    << (fillet_pair.exact_membership ? 1 : 0)
                    << " edge_boundary_line="
                    << fillet_pair.edge_feature_boundary_line_count
                    << " edge_boundary_circle="
                    << fillet_pair.edge_feature_boundary_circle_count
                    << " edge_boundary_other="
                    << fillet_pair.edge_feature_boundary_other_count
                    << " product_status="
                    << static_cast<int>(
                           fillet_pair.product_status)
                    << " product_diagnostic="
                    << static_cast<int>(
                           fillet_pair.product_diagnostic)
                    << " product_kernel_status="
                    << (fillet_pair.product_kernel_status
                            ? static_cast<int>(
                                  *fillet_pair
                                       .product_kernel_status)
                            : -1)
                    << '\n';
            }

            const auto chamfer_pair =
                runCase(
                    base,
                    base_eval.features[0],
                    pair_refs,
                    kernel::EdgeFeatureOperation::chamfer,
                    kernel);
            const bool chamfer_ok =
                chamfer_pair.product_status ==
                    part::FeatureEvaluationStatus::up_to_date &&
                chamfer_pair.product_topology_complete;
            if (chamfer_ok) {
                ++chamfer_pair_successes;
            } else {
                std::cerr
                    << "PM05F_CUBE_PAIR_FAIL"
                    << " op=chamfer"
                    << " i=" << first
                    << " j=" << second
                    << " token_i="
                    << all_edges[first].token.value
                    << " token_j="
                    << all_edges[second].token.value
                    << " adjacent="
                    << (adjacent_pair ? 1 : 0)
                    << " provider_status="
                    << static_cast<int>(
                           chamfer_pair.provider_status)
                    << " product_status="
                    << static_cast<int>(
                           chamfer_pair.product_status)
                    << " product_diagnostic="
                    << static_cast<int>(
                           chamfer_pair.product_diagnostic)
                    << '\n';
            }
        }
    }

    const auto triples =
        trihedralSets(
            *base_eval.features[0].result_topology);
    CHECK(triples.size() == 8U);
    std::size_t fillet_triple_successes = 0U;
    std::size_t chamfer_triple_successes = 0U;
    for (std::size_t index = 0U;
         index < triples.size();
         ++index) {
        const auto fillet_triple =
            runCase(
                base,
                base_eval.features[0],
                triples[index],
                kernel::EdgeFeatureOperation::fillet,
                kernel);
        if (fillet_triple.product_status ==
                part::FeatureEvaluationStatus::up_to_date &&
            fillet_triple.product_topology_complete) {
            ++fillet_triple_successes;
        } else {
            std::cerr
                << "PM05F_CUBE_TRIPLE_FAIL"
                << " op=fillet"
                << " triple=" << index
                << " provider_status="
                << static_cast<int>(
                       fillet_triple.provider_status)
                << " provider_ok="
                << (fillet_triple.provider_ok ? 1 : 0)
                << " product_status="
                << static_cast<int>(
                       fillet_triple.product_status)
                << " product_diagnostic="
                << static_cast<int>(
                       fillet_triple.product_diagnostic)
                << '\n';
        }

        const auto chamfer_triple =
            runCase(
                base,
                base_eval.features[0],
                triples[index],
                kernel::EdgeFeatureOperation::chamfer,
                kernel);
        if (chamfer_triple.product_status ==
                part::FeatureEvaluationStatus::up_to_date &&
            chamfer_triple.product_topology_complete) {
            ++chamfer_triple_successes;
        } else {
            std::cerr
                << "PM05F_CUBE_TRIPLE_FAIL"
                << " op=chamfer"
                << " triple=" << index
                << " provider_status="
                << static_cast<int>(
                       chamfer_triple.provider_status)
                << " product_status="
                << static_cast<int>(
                       chamfer_triple.product_status)
                << " product_diagnostic="
                << static_cast<int>(
                       chamfer_triple.product_diagnostic)
                << '\n';
        }
    }

    std::cout
        << "PM05F_FILLET_CUBE_MATRIX"
        << " edges=" << all_edges.size()
        << " pairs=" << pair_count
        << " adjacent_pairs=" << adjacent_pair_count
        << " fillet_pair_successes="
        << fillet_pair_successes
        << " chamfer_pair_successes="
        << chamfer_pair_successes
        << " adjacent_fillet_failures="
        << adjacent_fillet_failures
        << " trihedral_sets=" << triples.size()
        << " fillet_triple_successes="
        << fillet_triple_successes
        << " chamfer_triple_successes="
        << chamfer_triple_successes
        << '\n';

    // A 40x30x20 box with 2 mm Fillet / 1.5 mm Chamfer has ample geometric
    // clearance. Every two-Edge set and every trihedral corner is expected to
    // be constructible. Any failure here is a deterministic product defect,
    // not an excessive-parameter case.
    CHECK(pair_count == 66U);
    CHECK(adjacent_pair_count == 24U);
    CHECK(fillet_pair_successes == pair_count);
    CHECK(chamfer_pair_successes == pair_count);
    CHECK(adjacent_fillet_failures == 0U);
    CHECK(fillet_triple_successes == triples.size());
    CHECK(chamfer_triple_successes == triples.size());

    std::cout
        << "PM05F_FILLET_DIAGNOSTIC_PASS"
        << " raw_adjacent_build=1"
        << " raw_trihedral_build=1"
        << " exhaustive_cube_matrix=1"
        << '\n';
    return EXIT_SUCCESS;
}
