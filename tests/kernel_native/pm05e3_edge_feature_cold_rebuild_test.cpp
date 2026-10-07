#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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
            << "PM-05E3 cold rebuild CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm05e3_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

struct SessionFixture final {
    application::DocumentSession session;
    part::FeatureId base_id;
};

SessionFixture makeBaseSession(
    const std::filesystem::path& path,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    part::PartDocumentStore store;
    const auto created =
        store.createNew(path, document);
    CHECK(created.ok());

    application::DocumentSession session{
        path,
        std::move(document),
        *created.checkpoint};

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

    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false},
                "Base"},
            kernel);
    CHECK(base.ok() && base.feature_id);

    return {
        std::move(session),
        *base.feature_id};
}

std::vector<part::MaterialEdgeReference>
singleEdgeCandidates(
    const part::BodyStageTopologyCatalog& catalog) {
    std::vector<part::MaterialEdgeReference> result;
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
        if (authored.ok()) {
            result.push_back(*authored.reference);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(
        std::unique(result.begin(), result.end()),
        result.end());
    return result;
}

std::vector<part::MaterialEdgeReference>
generatedBoundaryReferences(
    const part::FeatureEvaluation& feature) {
    std::vector<part::MaterialEdgeReference> result;
    if (!feature.result_topology) {
        return result;
    }
    for (const auto& curve :
         feature.produced_curves) {
        if (curve.address.role !=
                part::FeatureCurveRoleKind::
                    edge_feature_boundary ||
            curve.status !=
                kernel::ReferenceStatus::resolved ||
            curve.strict_edge_status !=
                kernel::ReferenceStatus::resolved ||
            curve.current_edges.size() != 1U) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                *feature.result_topology,
                curve.current_edges.front());
        if (authored.ok()) {
            result.push_back(*authored.reference);
        }
    }
    std::sort(result.begin(), result.end());
    result.erase(
        std::unique(result.begin(), result.end()),
        result.end());
    return result;
}

std::optional<std::vector<part::MaterialEdgeReference>>
explicitContourReferences(
    const part::FeatureEvaluation& upstream_feature,
    const part::MaterialEdgeReference& seed,
    kernel::EdgeFeatureOperation operation,
    double parameter,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    if (!upstream_feature.result_topology ||
        !upstream_feature.result_solid) {
        return std::nullopt;
    }

    const auto seed_resolution =
        part::resolveMaterialEdgeReference(
            seed,
            *upstream_feature.result_topology);
    if (!seed_resolution ||
        !seed_resolution->resolved()) {
        return std::nullopt;
    }

    const kernel::EdgeFeatureInput probe{
        operation,
        {seed_resolution->current_edges.front()},
        parameter};
    CHECK(probe.valid());

    const auto probe_result =
        kernel.edgeFeature(
            probe,
            upstream_feature.result_solid);
    if (!probe_result.edge_feature_input_membership ||
        !probe_result.edge_feature_input_membership
             ->valid()) {
        return std::nullopt;
    }

    std::vector<part::MaterialEdgeReference> result;
    for (const auto token :
         probe_result.edge_feature_input_membership
             ->provider_contour_edges) {
        const auto authored =
            part::authorMaterialEdgeReference(
                *upstream_feature.result_topology,
                token);
        if (!authored.ok()) {
            return std::nullopt;
        }
        result.push_back(*authored.reference);
    }
    std::sort(result.begin(), result.end());
    result.erase(
        std::unique(result.begin(), result.end()),
        result.end());
    return result.empty()
        ? std::nullopt
        : std::optional<
              std::vector<part::MaterialEdgeReference>>{
              std::move(result)};
}

struct AppendResult final {
    part::PartDocument document;
    part::FeatureId feature_id;
};

AppendResult appendEdgeFeature(
    const part::PartDocument& source,
    std::vector<part::MaterialEdgeReference> edges,
    kernel::EdgeFeatureOperation operation,
    double parameter) {
    auto state = source.state();
    const auto id =
        state.body.next_feature_id.allocate();
    CHECK(id);

    part::PartFeature feature;
    feature.id = *id;
    feature.name =
        operation == kernel::EdgeFeatureOperation::fillet
            ? "FilletProbe"
            : "ChamferProbe";
    feature.suppressed = false;
    if (operation ==
        kernel::EdgeFeatureOperation::fillet) {
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
    state.body.features.push_back(
        std::move(feature));

    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return {
        std::move(*restored.document),
        *id};
}

struct ChainPlan final {
    std::vector<part::MaterialEdgeReference>
        first_edges;
    std::vector<part::MaterialEdgeReference>
        second_edges;
    part::FeatureId first_id;
    part::FeatureId second_id;
};

std::optional<ChainPlan> findChainPlan(
    const part::PartDocument& base,
    kernel::EdgeFeatureOperation first_operation,
    kernel::EdgeFeatureOperation second_operation,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto base_evaluation =
        part::evaluatePart(base, kernel);
    if (base_evaluation.body_status !=
            part::BodyEvaluationStatus::up_to_date ||
        base_evaluation.features.empty() ||
        !base_evaluation.features.back()
             .result_topology) {
        return std::nullopt;
    }

    // Reuse the Owner-accepted PM-05A chaining evidence parameters:
    // normal first operation = 2.0 mm, generated-boundary second = 0.75 mm.
    constexpr double first_parameter = 2.0;
    constexpr double second_parameter = 0.75;
    const auto seeds =
        singleEdgeCandidates(
            *base_evaluation.features.back()
                 .result_topology);

    for (const auto& seed : seeds) {
        // Match the accepted PM-05A / PM-05C2b chaining path exactly:
        // the first Feature consumes one explicit authored source Edge.
        // Provider-contour expansion is only evidence extraction for the
        // second operation and never a production fallback.
        std::vector<part::MaterialEdgeReference>
            first_edges{seed};

        auto first =
            appendEdgeFeature(
                base,
                first_edges,
                first_operation,
                first_parameter);
        const auto first_evaluation =
            part::evaluatePart(
                first.document,
                kernel);
        const auto* first_target =
            first_evaluation.findFeature(
                first.feature_id);
        if (first_evaluation.body_status !=
                part::BodyEvaluationStatus::up_to_date ||
            first_target == nullptr ||
            first_target->status !=
                part::FeatureEvaluationStatus::up_to_date) {
            continue;
        }

        const auto generated =
            generatedBoundaryReferences(
                *first_target);
        for (const auto& generated_seed :
             generated) {
            const auto second_edges =
                explicitContourReferences(
                    *first_target,
                    generated_seed,
                    second_operation,
                    second_parameter,
                    kernel);
            if (!second_edges) {
                continue;
            }

            auto second =
                appendEdgeFeature(
                    first.document,
                    *second_edges,
                    second_operation,
                    second_parameter);
            const auto second_evaluation =
                part::evaluatePart(
                    second.document,
                    kernel);
            const auto* second_target =
                second_evaluation.findFeature(
                    second.feature_id);
            if (second_evaluation.body_status ==
                    part::BodyEvaluationStatus::
                        up_to_date &&
                second_target != nullptr &&
                second_target->status ==
                    part::FeatureEvaluationStatus::
                        up_to_date) {
                return ChainPlan{
                    std::move(first_edges),
                    *second_edges,
                    first.feature_id,
                    second.feature_id};
            }
        }
    }
    return std::nullopt;
}

struct ChainResult final {
    part::FeatureId base_id;
    part::FeatureId first_id;
    part::FeatureId second_id;
    part::PartAuthoredState saved_state;
};

ChainResult authorAndSaveChain(
    const std::filesystem::path& path,
    kernel::EdgeFeatureOperation first_operation,
    kernel::EdgeFeatureOperation second_operation) {
    kernel_occt::OcctSolidModelingKernel kernel;
    auto fixture =
        makeBaseSession(path, kernel);

    const auto plan =
        findChainPlan(
            fixture.session.document(),
            first_operation,
            second_operation,
            kernel);
    CHECK(plan);

    // Reuse the Owner-accepted PM-05A chaining evidence parameters:
    // normal first operation = 2.0 mm, generated-boundary second = 0.75 mm.
    constexpr double first_parameter = 2.0;
    constexpr double second_parameter = 0.75;

    part::FeatureId first_id;
    if (first_operation ==
        kernel::EdgeFeatureOperation::fillet) {
        const auto first =
            fixture.session.execute(
                application::CreateFilletFeatureCommand{
                    plan->first_edges,
                    fixture.session.document().revision(),
                    core::LengthValue{first_parameter},
                    "Fillet001"},
                kernel);
        CHECK(first.ok() && first.feature_id);
        first_id = *first.feature_id;
    } else {
        const auto first =
            fixture.session.execute(
                application::CreateChamferFeatureCommand{
                    plan->first_edges,
                    fixture.session.document().revision(),
                    core::LengthValue{first_parameter},
                    "Chamfer001"},
                kernel);
        CHECK(first.ok() && first.feature_id);
        first_id = *first.feature_id;
    }
    CHECK(first_id == plan->first_id);

    part::FeatureId second_id;
    if (second_operation ==
        kernel::EdgeFeatureOperation::fillet) {
        const auto second =
            fixture.session.execute(
                application::CreateFilletFeatureCommand{
                    plan->second_edges,
                    fixture.session.document().revision(),
                    core::LengthValue{second_parameter},
                    "Fillet002"},
                kernel);
        CHECK(second.ok() && second.feature_id);
        second_id = *second.feature_id;
    } else {
        const auto second =
            fixture.session.execute(
                application::CreateChamferFeatureCommand{
                    plan->second_edges,
                    fixture.session.document().revision(),
                    core::LengthValue{second_parameter},
                    "Chamfer002"},
                kernel);
        CHECK(second.ok() && second.feature_id);
        second_id = *second.feature_id;
    }
    CHECK(second_id == plan->second_id);

    const auto before_save =
        fixture.session.document().state();
    CHECK(fixture.session.save().ok());
    CHECK(!fixture.session.needsSave());

    return {
        fixture.base_id,
        first_id,
        second_id,
        before_save};
}

void checkColdChain(
    const std::filesystem::path& path,
    const ChainResult& authored,
    kernel::EdgeFeatureOperation first_operation,
    kernel::EdgeFeatureOperation second_operation,
    bool prove_blocked_persistence) {
    part::PartDocumentStore store;
    auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->state() ==
        authored.saved_state);

    const auto* first =
        loaded.document->findFeature(
            authored.first_id);
    const auto* second =
        loaded.document->findFeature(
            authored.second_id);
    CHECK(first != nullptr);
    CHECK(second != nullptr);

    if (first_operation ==
        kernel::EdgeFeatureOperation::fillet) {
        CHECK(
            std::get_if<part::FilletFeature>(
                &first->definition) != nullptr);
    } else {
        CHECK(
            std::get_if<part::ChamferFeature>(
                &first->definition) != nullptr);
    }
    if (second_operation ==
        kernel::EdgeFeatureOperation::fillet) {
        CHECK(
            std::get_if<part::FilletFeature>(
                &second->definition) != nullptr);
    } else {
        CHECK(
            std::get_if<part::ChamferFeature>(
                &second->definition) != nullptr);
    }

    kernel_occt::OcctSolidModelingKernel cold_kernel;
    const auto cold =
        part::evaluatePart(
            *loaded.document,
            cold_kernel);
    CHECK(
        cold.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(
        cold.findFeature(authored.base_id)->status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        cold.findFeature(authored.first_id)->status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        cold.findFeature(authored.second_id)->status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(cold.current_topology);
    CHECK(cold.current_topology->complete());

    if (!prove_blocked_persistence) {
        return;
    }

    application::DocumentSession reopened{
        path,
        std::move(*loaded.document),
        *loaded.checkpoint};

    const auto* first_before =
        reopened.document().findFeature(
            authored.first_id);
    CHECK(first_before != nullptr);
    const auto first_definition_before =
        first_before->definition;

    CHECK(
        reopened.execute(
            application::DeleteFeatureCommand{
                authored.base_id,
                reopened.document().revision()})
            .ok());
    CHECK(reopened.save().ok());

    auto blocked_load = store.load(path);
    CHECK(blocked_load.ok());
    CHECK(
        blocked_load.document->findFeature(
            authored.base_id) == nullptr);
    const auto* retained_first =
        blocked_load.document->findFeature(
            authored.first_id);
    CHECK(retained_first != nullptr);
    CHECK(
        retained_first->definition ==
        first_definition_before);

    kernel_occt::OcctSolidModelingKernel blocked_kernel;
    const auto blocked =
        part::evaluatePart(
            *blocked_load.document,
            blocked_kernel);
    CHECK(
        blocked.body_status ==
        part::BodyEvaluationStatus::unavailable);
    const auto* blocked_first =
        blocked.findFeature(authored.first_id);
    CHECK(blocked_first != nullptr);
    CHECK(
        blocked_first->status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(
        blocked_first->result_solid == nullptr);
}

} // namespace

int main() {
    TempDirectory temp;

    const auto fillet_then_chamfer_path =
        temp.path / "FilletThenChamfer.ss2part";
    const auto f_c =
        authorAndSaveChain(
            fillet_then_chamfer_path,
            kernel::EdgeFeatureOperation::fillet,
            kernel::EdgeFeatureOperation::chamfer);
    checkColdChain(
        fillet_then_chamfer_path,
        f_c,
        kernel::EdgeFeatureOperation::fillet,
        kernel::EdgeFeatureOperation::chamfer,
        true);

    const auto chamfer_then_fillet_path =
        temp.path / "ChamferThenFillet.ss2part";
    const auto c_f =
        authorAndSaveChain(
            chamfer_then_fillet_path,
            kernel::EdgeFeatureOperation::chamfer,
            kernel::EdgeFeatureOperation::fillet);
    checkColdChain(
        chamfer_then_fillet_path,
        c_f,
        kernel::EdgeFeatureOperation::chamfer,
        kernel::EdgeFeatureOperation::fillet,
        false);

    std::cout
        << "PM05E3_EDGE_FEATURE_COLD_REBUILD_PASS"
        << " fillet_chamfer=1"
        << " chamfer_fillet=1"
        << " cold_rebuild=1"
        << " blocked_intent_reopen=1"
        << " runtime_identity_persisted=0\n";
    return EXIT_SUCCESS;
}
