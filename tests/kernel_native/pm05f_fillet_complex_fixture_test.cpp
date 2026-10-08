#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05F complex Fillet fixture CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct Fixture final {
    part::PartDocument document;
    part::FeatureId cut_id;
};

std::optional<part::ProfileId> createProfile(
    application::DocumentSession& session,
    sketch::SketchId sketch_id) {
    const auto* sketch =
        session.document().findSketch(sketch_id);
    if (sketch == nullptr) {
        return std::nullopt;
    }
    const auto regions =
        sketch::analyzeRegions(sketch->model);
    if (!regions.complete() ||
        regions.regions.size() != 1U) {
        return std::nullopt;
    }
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    if (!intent) {
        return std::nullopt;
    }
    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    return profile.ok()
        ? profile.profile_id
        : std::nullopt;
}

Fixture makeCapsuleCutPart() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(base_sketch.ok() && base_sketch.sketch_id);
    CHECK(
        session.execute(
            application::AddSketchRectangleCommand{
                *base_sketch.sketch_id,
                session.document().revision(),
                {-30.0, -20.0},
                {30.0, 20.0},
                sketch::EntityRole::regular,
                false})
            .ok());
    const auto base_profile =
        createProfile(
            session,
            *base_sketch.sketch_id);
    CHECK(base_profile);

    const auto cut_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(cut_sketch.ok() && cut_sketch.sketch_id);

    CHECK(
        session.execute(
            application::AddSketchLineCommand{
                *cut_sketch.sketch_id,
                {-10.0, -5.0},
                {10.0, -5.0},
                sketch::EntityRole::regular})
            .ok());
    CHECK(
        session.execute(
            application::AddSketchArcCommand{
                *cut_sketch.sketch_id,
                {10.0, 0.0},
                5.0,
                -std::numbers::pi / 2.0,
                std::numbers::pi,
                sketch::EntityRole::regular})
            .ok());
    CHECK(
        session.execute(
            application::AddSketchLineCommand{
                *cut_sketch.sketch_id,
                {10.0, 5.0},
                {-10.0, 5.0},
                sketch::EntityRole::regular})
            .ok());
    CHECK(
        session.execute(
            application::AddSketchArcCommand{
                *cut_sketch.sketch_id,
                {-10.0, 0.0},
                5.0,
                std::numbers::pi / 2.0,
                std::numbers::pi,
                sketch::EntityRole::regular})
            .ok());

    const auto cut_profile =
        createProfile(
            session,
            *cut_sketch.sketch_id);
    CHECK(cut_profile);

    auto state = session.document().state();
    const auto base_id =
        state.body.next_feature_id.allocate();
    const auto cut_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id && cut_id);

    state.body.features.push_back(
        part::PartFeature{
            *base_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *base_profile,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false}}});
    state.body.features.push_back(
        part::PartFeature{
            *cut_id,
            "CapsuleCut",
            false,
            part::ExtrudeFeature{
                *cut_profile,
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return {
        std::move(*restored.document),
        *cut_id};
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

struct LoopEdge final {
    kernel::RuntimeEdgeToken token;
    part::FeatureCurveAddress address;
    part::MaterialEdgeReference reference;
    kernel::CurveKind kind{
        kernel::CurveKind::other};
};

using MixedLoop = std::vector<LoopEdge>;

std::vector<MixedLoop> mixedBooleanLoops(
    const part::FeatureEvaluation& feature) {
    CHECK(feature.result_topology);
    const auto& catalog =
        *feature.result_topology;

    std::vector<LoopEdge> candidates;
    std::size_t line_count = 0U;
    std::size_t circle_count = 0U;
    for (const auto& curve :
         feature.produced_curves) {
        if (curve.address.producer_feature_id !=
                feature.feature_id ||
            curve.address.role !=
                part::FeatureCurveRoleKind::
                    boolean_intersection ||
            curve.status !=
                kernel::ReferenceStatus::resolved ||
            curve.strict_edge_status !=
                kernel::ReferenceStatus::resolved ||
            curve.current_edges.size() != 1U) {
            continue;
        }
        if (curve.curve_kind !=
                kernel::CurveKind::line &&
            curve.curve_kind !=
                kernel::CurveKind::circle) {
            continue;
        }

        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                curve.current_edges.front());
        if (!authored.ok()) {
            continue;
        }
        candidates.push_back(
            {
                curve.current_edges.front(),
                curve.address,
                *authored.reference,
                curve.curve_kind,
            });
        if (curve.curve_kind ==
            kernel::CurveKind::line) {
            ++line_count;
        } else {
            ++circle_count;
        }
    }

    const auto unsupported =
        static_cast<std::size_t>(
            std::count_if(
                catalog.edges.begin(),
                catalog.edges.end(),
                [](const auto& edge) {
                    return edge.accounting_class ==
                           part::TopologyAccountingClass::
                               semantically_unsupported;
                }));

    std::cerr
        << "PM05F_COMPLEX_CUT_CURVES"
        << " boolean_lines=" << line_count
        << " boolean_circles=" << circle_count
        << " unsupported_edges=" << unsupported
        << " candidates=" << candidates.size()
        << '\n';

    std::vector<bool> visited(
        candidates.size(),
        false);
    std::vector<MixedLoop> loops;
    for (std::size_t seed = 0U;
         seed < candidates.size();
         ++seed) {
        if (visited[seed]) {
            continue;
        }
        std::vector<std::size_t> pending{seed};
        visited[seed] = true;
        MixedLoop component;

        while (!pending.empty()) {
            const auto current =
                pending.back();
            pending.pop_back();
            component.push_back(
                candidates[current]);

            for (std::size_t other = 0U;
                 other < candidates.size();
                 ++other) {
                if (visited[other] ||
                    !shareVertex(
                        catalog,
                        candidates[current].token,
                        candidates[other].token)) {
                    continue;
                }
                visited[other] = true;
                pending.push_back(other);
            }
        }

        if (component.size() != 4U) {
            continue;
        }

        std::size_t lines = 0U;
        std::size_t circles = 0U;
        bool cycle = true;
        for (std::size_t index = 0U;
             index < component.size();
             ++index) {
            if (component[index].kind ==
                kernel::CurveKind::line) {
                ++lines;
            } else if (
                component[index].kind ==
                kernel::CurveKind::circle) {
                ++circles;
            }

            std::size_t degree = 0U;
            for (std::size_t other = 0U;
                 other < component.size();
                 ++other) {
                if (index != other &&
                    shareVertex(
                        catalog,
                        component[index].token,
                        component[other].token)) {
                    ++degree;
                }
            }
            cycle = cycle && degree == 2U;
        }

        if (!cycle ||
            lines != 2U ||
            circles != 2U) {
            continue;
        }

        std::sort(
            component.begin(),
            component.end(),
            [](const auto& first, const auto& second) {
                return first.reference <
                       second.reference;
            });
        loops.push_back(
            std::move(component));
    }

    std::sort(
        loops.begin(),
        loops.end(),
        [](const auto& first, const auto& second) {
            std::vector<part::MaterialEdgeReference>
                first_refs;
            std::vector<part::MaterialEdgeReference>
                second_refs;
            for (const auto& item : first) {
                first_refs.push_back(item.reference);
            }
            for (const auto& item : second) {
                second_refs.push_back(item.reference);
            }
            return first_refs < second_refs;
        });

    return loops;
}

part::PartDocument appendEdgeFeature(
    const part::PartDocument& source,
    std::vector<part::MaterialEdgeReference> edges,
    kernel::EdgeFeatureOperation operation,
    double parameter) {
    std::sort(edges.begin(), edges.end());

    auto state = source.state();
    const auto id =
        state.body.next_feature_id.allocate();
    CHECK(id);
    if (operation ==
        kernel::EdgeFeatureOperation::fillet) {
        state.body.features.push_back(
            part::PartFeature{
                *id,
                "FilletComplex",
                false,
                part::FilletFeature{
                    std::move(edges),
                    core::LengthValue{parameter}}});
    } else {
        state.body.features.push_back(
            part::PartFeature{
                *id,
                "ChamferComplex",
                false,
                part::ChamferFeature{
                    std::move(edges),
                    core::LengthValue{parameter}}});
    }

    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

std::vector<part::MaterialEdgeReference>
references(const MixedLoop& loop) {
    std::vector<part::MaterialEdgeReference>
        result;
    for (const auto& item : loop) {
        result.push_back(item.reference);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<part::FeatureCurveAddress>
addresses(const MixedLoop& loop) {
    std::vector<part::FeatureCurveAddress>
        result;
    for (const auto& item : loop) {
        result.push_back(item.address);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::optional<part::MaterialEdgeReference>
disjointChamferEdge(
    const part::BodyStageTopologyCatalog& catalog,
    const MixedLoop& loop) {
    std::vector<part::MaterialEdgeReference>
        candidates;
    for (const auto& edge : catalog.edges) {
        if (edge.accounting_class !=
                part::TopologyAccountingClass::
                    referenceable ||
            edge.referenceability !=
                kernel::ReferenceStatus::resolved) {
            continue;
        }

        bool local = false;
        for (const auto& loop_edge : loop) {
            if (edge.runtime_token ==
                    loop_edge.token ||
                shareVertex(
                    catalog,
                    edge.runtime_token,
                    loop_edge.token)) {
                local = true;
                break;
            }
        }
        if (local ||
            edge.curve_kind != kernel::CurveKind::line) {
            continue;
        }

        // The old-project analogue chamfers an exterior box Edge, not the
        // opposite capsule rim. Exclude every Curve meaning produced by the
        // Cut Feature that produced the mixed loop; inherited base-box line
        // meaning must remain.
        const auto cut_feature_id =
            loop.front().address.producer_feature_id;
        const bool produced_by_cut =
            std::any_of(
                edge.curve_candidates.begin(),
                edge.curve_candidates.end(),
                [cut_feature_id](const auto& address) {
                    return address.producer_feature_id ==
                           cut_feature_id;
                });
        if (produced_by_cut) {
            continue;
        }

        const auto authored =
            part::authorMaterialEdgeReference(
                catalog,
                edge.runtime_token);
        if (authored.ok()) {
            candidates.push_back(
                *authored.reference);
        }
    }

    if (candidates.empty()) {
        return std::nullopt;
    }
    std::sort(
        candidates.begin(),
        candidates.end());
    return candidates.front();
}

std::optional<std::vector<part::MaterialEdgeReference>>
reauthorCurveAddresses(
    const part::BodyStageTopologyCatalog& catalog,
    const std::vector<part::FeatureCurveAddress>&
        wanted) {
    std::vector<part::MaterialEdgeReference>
        result;
    for (const auto& address : wanted) {
        std::vector<part::MaterialEdgeReference>
            matches;
        for (const auto& edge : catalog.edges) {
            if (std::find(
                    edge.curve_candidates.begin(),
                    edge.curve_candidates.end(),
                    address) ==
                edge.curve_candidates.end()) {
                continue;
            }

            const auto authored =
                part::authorMaterialEdgeReference(
                    catalog,
                    edge.runtime_token);
            if (authored.ok()) {
                matches.push_back(
                    *authored.reference);
            }
        }
        std::sort(
            matches.begin(),
            matches.end());
        matches.erase(
            std::unique(
                matches.begin(),
                matches.end()),
            matches.end());
        if (matches.size() != 1U) {
            return std::nullopt;
        }
        result.push_back(
            matches.front());
    }

    std::sort(result.begin(), result.end());
    if (std::adjacent_find(
            result.begin(),
            result.end()) !=
        result.end()) {
        return std::nullopt;
    }
    return result;
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;

    const auto fixture =
        makeCapsuleCutPart();
    const auto cut_evaluation =
        part::evaluatePart(
            fixture.document,
            kernel);
    CHECK(
        cut_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(cut_evaluation.features.size() == 2U);
    const auto& cut =
        cut_evaluation.features[1];
    CHECK(cut.feature_id == fixture.cut_id);
    CHECK(
        cut.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(cut.result_topology);
    CHECK(cut.result_topology->complete());

    const auto loops =
        mixedBooleanLoops(cut);

    // The capsule-through-box Cut has two material boundary loops (top and
    // bottom). Each loop is exactly two Line branches and two Circle branches.
    // Failure here classifies a semantic Curve/accounting defect before any
    // Fillet provider call is made.
    CHECK(loops.size() == 2U);

    const auto& loop = loops.front();
    const auto loop_refs =
        references(loop);
    CHECK(loop_refs.size() == 4U);

    // Direct complex production proof: a complete explicit mixed Line/Circle
    // loop is one four-Edge Fillet feature.
    const auto direct_document =
        appendEdgeFeature(
            fixture.document,
            loop_refs,
            kernel::EdgeFeatureOperation::fillet,
            2.0);
    const auto direct_evaluation =
        part::evaluatePart(
            direct_document,
            kernel);
    CHECK(direct_evaluation.features.size() == 3U);
    const auto& direct_fillet =
        direct_evaluation.features[2];
    CHECK(
        direct_fillet.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(direct_fillet.result_topology);
    CHECK(direct_fillet.result_topology->complete());

    // Old-project analogue: first Chamfer an unrelated exterior Edge, then
    // re-author the same Cut Curve meanings at the new stage and Fillet the
    // four-Edge capsule loop. No XYZ/fingerprint/provider ordinal is used.
    const auto chamfer_edge =
        disjointChamferEdge(
            *cut.result_topology,
            loop);
    CHECK(chamfer_edge);

    const auto chamfer_document =
        appendEdgeFeature(
            fixture.document,
            {*chamfer_edge},
            kernel::EdgeFeatureOperation::chamfer,
            2.0);
    const auto chamfer_evaluation =
        part::evaluatePart(
            chamfer_document,
            kernel);
    CHECK(chamfer_evaluation.features.size() == 3U);
    const auto& chamfer =
        chamfer_evaluation.features[2];
    CHECK(
        chamfer.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(chamfer.result_topology);
    CHECK(chamfer.result_topology->complete());

    const auto wanted_addresses =
        addresses(loop);
    const auto reauthored =
        reauthorCurveAddresses(
            *chamfer.result_topology,
            wanted_addresses);
    if (!reauthored) {
        for (const auto& address : wanted_addresses) {
            const auto found =
                std::find_if(
                    chamfer_evaluation
                        .current_curve_references.begin(),
                    chamfer_evaluation
                        .current_curve_references.end(),
                    [&address](const auto& reference) {
                        return reference.address == address;
                    });
            std::cerr
                << "PM05F_COMPLEX_REAUTHOR"
                << " role="
                << static_cast<int>(address.role)
                << " producer="
                << address.producer_feature_id.value()
                << " found="
                << (found != chamfer_evaluation
                                  .current_curve_references.end()
                        ? 1
                        : 0);
            if (found != chamfer_evaluation
                             .current_curve_references.end()) {
                std::cerr
                    << " status="
                    << static_cast<int>(found->status)
                    << " strict="
                    << static_cast<int>(
                           found->strict_edge_status)
                    << " candidates="
                    << found->candidate_edge_count
                    << " current_edges="
                    << found->current_edges.size()
                    << " kind="
                    << static_cast<int>(
                           found->curve_kind);
            }
            std::cerr << '\n';
        }
    }
    CHECK(reauthored);
    CHECK(reauthored->size() == 4U);

    const auto chained_document =
        appendEdgeFeature(
            chamfer_document,
            *reauthored,
            kernel::EdgeFeatureOperation::fillet,
            2.0);
    const auto chained_evaluation =
        part::evaluatePart(
            chained_document,
            kernel);
    CHECK(chained_evaluation.features.size() == 4U);
    const auto& chained_fillet =
        chained_evaluation.features[3];

    std::cerr
        << "PM05F_COMPLEX_FILLET"
        << " loops=" << loops.size()
        << " loop_edges=" << loop.size()
        << " direct_status="
        << static_cast<int>(
               direct_fillet.status)
        << " chained_status="
        << static_cast<int>(
               chained_fillet.status)
        << " chained_diagnostic="
        << static_cast<int>(
               chained_fillet.diagnostic)
        << " chained_kernel_status="
        << (chained_fillet.kernel_status
                ? static_cast<int>(
                      *chained_fillet.kernel_status)
                : -1)
        << '\n';

    CHECK(
        chained_fillet.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(chained_fillet.result_topology);
    CHECK(chained_fillet.result_topology->complete());

    std::cout
        << "PM05F_COMPLEX_FILLET_FIXTURE_PASS"
        << " capsule_loops=2"
        << " mixed_edges=4"
        << " chamfer_then_fillet=1"
        << " xyz_identity=0\n";
    return EXIT_SUCCESS;
}
