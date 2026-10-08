#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/edge_feature_draft.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <chrono>
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

Fixture makeCapsuleCutPart(double cut_depth = 20.0) {
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
                    core::LengthValue{cut_depth},
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

    // PM-05F R2-D: production mixed Line/Circle capsule cut and its
    // four-Edge Fillet must produce a presentation-only exact local delta.
    // Neither preview mesh may be substituted with the final full Body.
    CHECK(cut.result_solid != nullptr);
    CHECK(direct_fillet.result_solid != nullptr);
    const auto delta_started =
        std::chrono::steady_clock::now();
    const auto complex_delta =
        kernel.materialDifferencePreview(
            cut.result_solid,
            direct_fillet.result_solid);
    const auto delta_ms =
        std::chrono::duration_cast<
            std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - delta_started)
            .count();
    CHECK(complex_delta.ok());
    CHECK(complex_delta.removed || complex_delta.added);
    CHECK(!complex_delta.removed ||
          complex_delta.removed->valid());
    CHECK(!complex_delta.added ||
          complex_delta.added->valid());
    const auto unchanged_delta =
        kernel.materialDifferencePreview(
            cut.result_solid,
            cut.result_solid);
    CHECK(!unchanged_delta.ok());
    CHECK(!unchanged_delta.removed);
    CHECK(!unchanged_delta.added);
    std::cout
        << "PM05F_R2_COMPLEX_EXACT_DELTA"
        << " removed=" << (complex_delta.removed ? 1 : 0)
        << " added=" << (complex_delta.added ? 1 : 0)
        << " elapsed_ms=" << delta_ms << '\n';

    // PM-05F R2-D concave pocket: unlike the opening rim (a
    // cross-producer Boolean Intersection), the bottom wall/floor seam is
    // the Cut generator's own cap/side Curve. Select it via exact semantic
    // producer + Surface-pair relation, never by XYZ or provider ordering.
    {
        const auto pocket =
            makeCapsuleCutPart(10.0);
        const auto evaluated_pocket =
            part::evaluatePart(pocket.document, kernel);
        CHECK(evaluated_pocket.body_status ==
              part::BodyEvaluationStatus::up_to_date);
        CHECK(evaluated_pocket.features.size() == 2U);
        const auto& pocket_cut =
            evaluated_pocket.features.back();
        CHECK(pocket_cut.result_solid != nullptr);
        CHECK(pocket_cut.result_topology);
        CHECK(pocket_cut.result_topology->complete());

        std::vector<part::MaterialEdgeReference> floor_edges;
        std::size_t observed_cap_side = 0U;
        for (const auto& curve :
             pocket_cut.produced_curves) {
            if (curve.address.producer_feature_id !=
                    pocket.cut_id ||
                curve.address.role !=
                    part::FeatureCurveRoleKind::cap_side) {
                continue;
            }
            ++observed_cap_side;
            if (curve.strict_edge_status !=
                    kernel::ReferenceStatus::resolved ||
                curve.current_edges.size() != 1U) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    *pocket_cut.result_topology,
                    curve.current_edges.front());
            if (authored.ok()) {
                floor_edges.push_back(*authored.reference);
            }
        }
        std::sort(floor_edges.begin(), floor_edges.end());
        floor_edges.erase(
            std::unique(
                floor_edges.begin(), floor_edges.end()),
            floor_edges.end());

        std::cout
            << "PM05F_R2_POCKET_FLOOR_AUTHORED"
            << " cap_side=" << observed_cap_side
            << " authorable=" << floor_edges.size()
            << '\n';
        CHECK(floor_edges.size() == 4U);
        const auto candidate =
            appendEdgeFeature(
                pocket.document,
                floor_edges,
                kernel::EdgeFeatureOperation::fillet,
                1.0);
        const auto after =
            part::evaluatePart(candidate, kernel);
        CHECK(after.features.size() == 3U);
        const auto& floor_fillet =
            after.features.back();
        CHECK(floor_fillet.status ==
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(floor_fillet.result_solid != nullptr);
        const auto delta =
            kernel.materialDifferencePreview(
                pocket_cut.result_solid,
                floor_fillet.result_solid);
        CHECK(delta.ok());
        CHECK(delta.added.has_value());
        CHECK(delta.added->valid());
        std::cout
            << "PM05F_R2_CONCAVE_FLOOR_DELTA"
            << " removed=" << (delta.removed ? 1 : 0)
            << " added=" << (delta.added ? 1 : 0)
            << '\n';

        // Owner R2-A PASS; next P0: adding one exterior convex Edge
        // to an already selected concave pocket-floor Edge must either
        // evaluate the complete explicit pair or fail in a controlled
        // way. In particular it may never terminate the process during
        // synchronous draft re-evaluation or exact material preview.
        // Both inputs are authored from the same, current Cut stage.
        const auto& pocket_catalog =
            *pocket_cut.result_topology;
        std::vector<part::MaterialEdgeReference>
            exterior_edges;
        for (const auto& edge : pocket_catalog.edges) {
            if (edge.accounting_class !=
                    part::TopologyAccountingClass::
                        referenceable ||
                edge.referenceability !=
                    kernel::ReferenceStatus::resolved ||
                edge.curve_kind !=
                    kernel::CurveKind::line ||
                edge.periodic_seam ||
                edge.representation_partition) {
                continue;
            }
            const bool from_cut =
                std::any_of(
                    edge.curve_candidates.begin(),
                    edge.curve_candidates.end(),
                    [&pocket](const auto& address) {
                        return address.producer_feature_id ==
                               pocket.cut_id;
                    });
            if (from_cut) {
                continue;
            }
            const auto authored =
                part::authorMaterialEdgeReference(
                    pocket_catalog, edge.runtime_token);
            if (authored.ok()) {
                exterior_edges.push_back(
                    *authored.reference);
            }
        }
        std::sort(
            exterior_edges.begin(),
            exterior_edges.end());
        exterior_edges.erase(
            std::unique(
                exterior_edges.begin(),
                exterior_edges.end()),
            exterior_edges.end());
        CHECK(!exterior_edges.empty());

        auto restored_for_draft =
            part::PartDocument::restore(
                pocket.document.documentId(),
                pocket.document.state(),
                pocket.document.revision());
        CHECK(restored_for_draft.ok());
        application::DocumentSession preview_session{
            {}, std::move(*restored_for_draft.document)};
        const auto source_revision =
            preview_session.document().revision();
        const auto source_feature_count =
            preview_session.document().body().features.size();

        const auto concave =
            floor_edges.front();
        const auto convex =
            exterior_edges.front();
        CHECK(concave != convex);
        for (const auto operation : {
                 kernel::EdgeFeatureOperation::fillet,
                 kernel::EdgeFeatureOperation::chamfer}) {
            const bool fillet =
                operation ==
                kernel::EdgeFeatureOperation::fillet;
            std::cerr
                << "PM05F_R2_MIXED_P0_STAGE"
                << " operation=" << (fillet ? "FILLET" : "CHAMFER")
                << " step=concave_only"
                << std::endl;
            if (fillet) {
                auto draft =
                    application::FilletDraft::beginCreate(
                        preview_session,
                        {concave});
                CHECK(draft);
                CHECK(draft->setRadius(core::LengthValue{1.0}));
                const auto solo =
                    preview_session.evaluateFilletDraft(
                        *draft, kernel);
                std::cerr
                    << "PM05F_R2_MIXED_P0_SOLO"
                    << " status=" << static_cast<int>(solo.status)
                    << " target=" << (solo.target_status
                        ? static_cast<int>(*solo.target_status)
                        : -1)
                    << std::endl;
                CHECK(draft->setEdges({concave, convex}));
                std::cerr
                    << "PM05F_R2_MIXED_P0_STAGE"
                    << " operation=FILLET step=mixed"
                    << std::endl;
                const auto mixed =
                    preview_session.evaluateFilletDraft(
                        *draft, kernel);
                std::cerr
                    << "PM05F_R2_MIXED_P0_RESULT"
                    << " operation=FILLET"
                    << " status=" << static_cast<int>(mixed.status)
                    << " target=" << (mixed.target_status
                        ? static_cast<int>(*mixed.target_status)
                        : -1)
                    << std::endl;
                CHECK(mixed.status ==
                      application::EdgeFeatureDraftEvaluationStatus::ok ||
                      mixed.status ==
                      application::EdgeFeatureDraftEvaluationStatus::
                          target_failed);
            } else {
                auto draft =
                    application::ChamferDraft::beginCreate(
                        preview_session,
                        {concave});
                CHECK(draft);
                CHECK(draft->setDistance(core::LengthValue{1.0}));
                const auto solo =
                    preview_session.evaluateChamferDraft(
                        *draft, kernel);
                std::cerr
                    << "PM05F_R2_MIXED_P0_SOLO"
                    << " status=" << static_cast<int>(solo.status)
                    << " target=" << (solo.target_status
                        ? static_cast<int>(*solo.target_status)
                        : -1)
                    << std::endl;
                CHECK(draft->setEdges({concave, convex}));
                std::cerr
                    << "PM05F_R2_MIXED_P0_STAGE"
                    << " operation=CHAMFER step=mixed"
                    << std::endl;
                const auto mixed =
                    preview_session.evaluateChamferDraft(
                        *draft, kernel);
                std::cerr
                    << "PM05F_R2_MIXED_P0_RESULT"
                    << " operation=CHAMFER"
                    << " status=" << static_cast<int>(mixed.status)
                    << " target=" << (mixed.target_status
                        ? static_cast<int>(*mixed.target_status)
                        : -1)
                    << std::endl;
                CHECK(mixed.status ==
                      application::EdgeFeatureDraftEvaluationStatus::ok ||
                      mixed.status ==
                      application::EdgeFeatureDraftEvaluationStatus::
                          target_failed);
            }
            CHECK(preview_session.document().revision() ==
                  source_revision);
            CHECK(preview_session.document().body()
                      .features.size() == source_feature_count);
        }
        std::cout
            << "PM05F_R2_MIXED_P0_DRAFT_COMPLETES"
            << " fillet=1 chamfer=1"
            << " mutation=0\n";
    }

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

    // The unrelated exterior Chamfer must not erase or reclassify the mixed
    // capsule Curve meanings. Prove the exact semantic Cut relations survive
    // in the adjacent stage as two lines + two circles before re-authoring.
    std::size_t continued_lines = 0U;
    std::size_t continued_circles = 0U;
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
        CHECK(
            found != chamfer_evaluation
                         .current_curve_references.end());
        CHECK(
            found->status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            found->strict_edge_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(found->current_edges.size() == 1U);
        if (found->curve_kind ==
            kernel::CurveKind::line) {
            ++continued_lines;
        } else if (
            found->curve_kind ==
            kernel::CurveKind::circle) {
            ++continued_circles;
        } else {
            CHECK(false);
        }
    }
    CHECK(continued_lines == 2U);
    CHECK(continued_circles == 2U);

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
                << address.producer_feature_id.serialized()
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
        << " continued_lines=2"
        << " continued_circles=2"
        << " xyz_identity=0\n";
    return EXIT_SUCCESS;
}
