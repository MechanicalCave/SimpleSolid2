#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <variant>
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

    // Enumerate current bounded material Edges, not only Curve families whose
    // strict realization is globally singular. A multi-branch generated
    // Curve may still contain individually authorable bounded Edges through
    // BetweenSemanticPoints discrimination; that is production
    // MaterialEdgeReference semantics, not a test fallback.
    for (const auto& edge :
         feature.result_topology->edges) {
        const bool generated_boundary =
            std::any_of(
                edge.curve_candidates.begin(),
                edge.curve_candidates.end(),
                [&feature](const auto& address) {
                    return address.producer_feature_id ==
                               feature.feature_id &&
                           address.role ==
                               part::FeatureCurveRoleKind::
                                   edge_feature_boundary;
                });
        if (!generated_boundary) {
            continue;
        }

        const auto authored =
            part::authorMaterialEdgeReference(
                *feature.result_topology,
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

    std::size_t first_successes = 0U;
    std::size_t generated_candidates = 0U;
    std::size_t contour_authoring_successes = 0U;
    std::size_t second_evaluation_successes = 0U;
    std::optional<part::FeatureEvaluationStatus>
        first_second_status;
    std::optional<part::FeatureEvaluationDiagnosticCode>
        first_second_diagnostic;
    std::optional<kernel::SolidModelingStatus>
        first_second_kernel_status;
    std::optional<kernel::ReferenceStatus>
        first_second_reference_status;
    std::optional<std::size_t>
        first_second_surface_count;
    std::optional<std::size_t>
        first_second_curve_count;
    std::optional<std::size_t>
        first_second_point_count;
    std::optional<bool>
        first_second_result_topology;

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
        ++first_successes;

        const auto generated =
            generatedBoundaryReferences(
                *first_target);
        generated_candidates += generated.size();
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
            ++contour_authoring_successes;

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
                ++second_evaluation_successes;
                return ChainPlan{
                    std::move(first_edges),
                    *second_edges,
                    first.feature_id,
                    second.feature_id};
            }

            if (!first_second_status &&
                second_target != nullptr) {
                first_second_status =
                    second_target->status;
                first_second_diagnostic =
                    second_target->diagnostic;
                first_second_kernel_status =
                    second_target->kernel_status;
                first_second_reference_status =
                    second_target->edge_reference_status;
                first_second_surface_count =
                    second_target->produced_surfaces.size();
                first_second_curve_count =
                    second_target->produced_curves.size();
                first_second_point_count =
                    second_target->produced_points.size();
                first_second_result_topology =
                    second_target->result_topology.has_value();
            }
        }
    }

    const auto opName =
        [](kernel::EdgeFeatureOperation op) {
            return op == kernel::EdgeFeatureOperation::fillet
                ? "fillet"
                : "chamfer";
        };
    std::cerr
        << "PM05E3_CHAIN_SEARCH_FAIL"
        << " first=" << opName(first_operation)
        << " second=" << opName(second_operation)
        << " seeds=" << seeds.size()
        << " first_successes=" << first_successes
        << " generated_candidates=" << generated_candidates
        << " contour_authoring_successes="
        << contour_authoring_successes
        << " second_evaluation_successes="
        << second_evaluation_successes
        << " first_second_status="
        << (first_second_status
                ? static_cast<int>(*first_second_status)
                : -1)
        << " first_second_diagnostic="
        << (first_second_diagnostic
                ? static_cast<int>(*first_second_diagnostic)
                : -1)
        << " first_second_kernel_status="
        << (first_second_kernel_status
                ? static_cast<int>(*first_second_kernel_status)
                : -1)
        << " first_second_reference_status="
        << (first_second_reference_status
                ? static_cast<int>(*first_second_reference_status)
                : -1)
        << " first_second_surfaces="
        << (first_second_surface_count
                ? static_cast<long long>(*first_second_surface_count)
                : -1)
        << " first_second_curves="
        << (first_second_curve_count
                ? static_cast<long long>(*first_second_curve_count)
                : -1)
        << " first_second_points="
        << (first_second_point_count
                ? static_cast<long long>(*first_second_point_count)
                : -1)
        << " first_second_result_topology="
        << (first_second_result_topology
                ? (*first_second_result_topology ? 1 : 0)
                : -1)
        << '\n';
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


void verifyPg01bAtomicProjectedEdges() {
    TempDirectory temp;
    const auto path = temp.path / "Pg01bProjectedBatch.ss2part";
    kernel_occt::OcctSolidModelingKernel kernel;
    auto fixture = makeBaseSession(path, kernel);
    const auto prefix = part::evaluatePart(
        fixture.session.document(), kernel);
    const auto* stage = prefix.findFeature(fixture.base_id);
    CHECK(stage && stage->result_topology && stage->result_solid);
    const kernel::Frame3 xy{};
    std::vector<part::MaterialEdgeReference> sources;
    std::optional<part::MaterialEdgeReference>
        projected_point_source;
    for (const auto& edge : stage->result_topology->edges) {
        const auto ref = part::authorMaterialEdgeReference(
            *stage->result_topology, edge.runtime_token);
        if (!ref.ok()) {
            continue;
        }
        const auto scoped = kernel.bindEdgeToBody(
            stage->result_solid, edge.runtime_token);
        if (!scoped) {
            continue;
        }
        const auto projected = kernel.projectEdgeToPlane(
            stage->result_solid, *scoped, xy);
        if (projected.status ==
                kernel::EdgeProjectionStatus::degenerate_projection &&
            !projected_point_source) {
            projected_point_source = *ref.reference;
        }
        if (projected.ok() &&
            std::holds_alternative<kernel::Line2>(*projected.curve) &&
            std::find(sources.begin(), sources.end(), *ref.reference)
                == sources.end() &&
            sources.size() < 2U) {
            sources.push_back(*ref.reference);
        }
        if (sources.size() == 2U && projected_point_source) {
            break;
        }
    }
    CHECK(sources.size() == 2U);
    CHECK(projected_point_source);
    const auto new_sketch = fixture.session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(new_sketch.ok() && new_sketch.sketch_id);
    const auto id = *new_sketch.sketch_id;
    const auto old_revision = fixture.session.document().revision();
    // First exact Edge materializes in pending state, but the second
    // strict source collapses into a Point in XY. The entire two-source
    // request must fail with no partial authored target/EntityId.
    const auto before_bad = fixture.session.document().state();
    const auto bad_batch = fixture.session.execute(
        application::CreateProjectedSketchEdgesCommand{
            id, old_revision,
            {sources.front(), *projected_point_source},
            sketch::EntityRole::regular}, kernel);
    CHECK(!bad_batch.ok() && !bad_batch.changed);
    CHECK(bad_batch.failing_source_index &&
          *bad_batch.failing_source_index == 1U);
    CHECK(bad_batch.source_status &&
          *bad_batch.source_status ==
              part::ProjectedSketchSourceStatus::degenerate_projection);
    CHECK(fixture.session.document().state() == before_bad);

    const auto batch = fixture.session.execute(
        application::CreateProjectedSketchEdgesCommand{
            id, old_revision, sources,
            sketch::EntityRole::construction}, kernel);
    CHECK(batch.ok());
    CHECK(batch.entity_ids.size() == 2U);
    const auto* sketch = fixture.session.document().findSketch(id);
    CHECK(sketch && sketch->projection_bindings.size() == 2U);
    CHECK(sketch->model.entityCount() == 2U);
    for (const auto entity : batch.entity_ids) {
        const auto* line = sketch->model.findLine(entity);
        CHECK(line && line->role() == sketch::EntityRole::construction);
    }
    const auto before_duplicate =
        fixture.session.document().state();
    const auto geometry =
        sketch::captureSketchTransformGeometry(
            authored->model, {batch.entity_ids.front()});
    CHECK(geometry && !geometry->empty());
    const auto stale_copy = fixture.session.execute(
        application::DuplicateSketchGeometryCommand{
            id, fixture.session.document().revision(),
            *geometry});
    CHECK(!stale_copy.ok() && !stale_copy.changed);
    CHECK(fixture.session.document().state() ==
          before_duplicate);

    const auto after = fixture.session.document().state();
    const auto dup = fixture.session.execute(
        application::CreateProjectedSketchEdgesCommand{
            id, fixture.session.document().revision(),
            {sources[0], sources[0]},
            sketch::EntityRole::regular}, kernel);
    CHECK(!dup.ok() && !dup.changed);
    CHECK(dup.failing_source_index);
    CHECK(fixture.session.document().state() == after);
    const auto stale = fixture.session.execute(
        application::CreateProjectedSketchEdgesCommand{
            id, old_revision, sources,
            sketch::EntityRole::regular}, kernel);
    CHECK(!stale.ok() && !stale.changed);
    CHECK(fixture.session.document().state() == after);
    const auto incomplete = fixture.session.execute(
        application::CreateProjectedSketchEdgesCommand{
            id, fixture.session.document().revision(),
            {part::MaterialEdgeReference{}},
            sketch::EntityRole::regular}, kernel);
    CHECK(!incomplete.ok() && !incomplete.changed);
    CHECK(fixture.session.document().state() == after);

    // One batch is one Undo step, preserving both IDs under Redo.
    CHECK(fixture.session.undo().changed);
    const auto* undone = fixture.session.document().findSketch(id);
    CHECK(undone && undone->projection_bindings.empty());
    CHECK(undone->model.entityCount() == 0U);
    CHECK(fixture.session.redo().changed);
    const auto* redone = fixture.session.document().findSketch(id);
    CHECK(redone && redone->projection_bindings.size() == 2U);
    CHECK(redone->model.contains(batch.entity_ids[0]));
    CHECK(redone->model.contains(batch.entity_ids[1]));
    CHECK(fixture.session.save().ok());

    part::PartDocumentStore store;
    const auto loaded = store.load(path);
    CHECK(loaded.ok() && loaded.document);
    const auto* cold_sketch = loaded.document->findSketch(id);
    CHECK(cold_sketch && cold_sketch->projection_bindings.size() == 2U);
    CHECK(cold_sketch->model.contains(batch.entity_ids[0]));
    CHECK(cold_sketch->model.contains(batch.entity_ids[1]));
    kernel_occt::OcctSolidModelingKernel cold;
    const auto current = part::evaluatePart(
        *loaded.document, cold);
    const auto effective = part::evaluateEffectiveSketchProjection(
        *loaded.document, id, current, cold);
    CHECK(effective && effective->allResolved());
    std::cout
        << "PG01B_B4_ATOMIC_EDGE_BATCH_PASS"
        << " edges=2 undo_steps=1"
        << " cold_reproject=1\n";
}

void verifyPg01bNativeDerivedProfileExtrude() {
    TempDirectory temp;
    const auto path =
        temp.path / "Pg01bNativeLinkedProfile.ss2part";
    kernel_occt::OcctSolidModelingKernel kernel;
    auto fixture = makeBaseSession(path, kernel);
    const auto prefix =
        part::evaluatePart(
            fixture.session.document(), kernel);
    CHECK(prefix.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    const auto* stage =
        prefix.findFeature(fixture.base_id);
    CHECK(stage && stage->result_topology &&
          stage->result_solid);
    const auto close = [](double a, double b) {
        return std::abs(a - b) < 1.0e-7;
    };
    const kernel::Frame3 xy{};
    std::optional<part::MaterialEdgeReference> selected;
    for (const auto& edge :
         stage->result_topology->edges) {
        if (selected) {
            break;
        }
        const auto semantic =
            part::authorMaterialEdgeReference(
                *stage->result_topology,
                edge.runtime_token);
        if (!semantic.ok()) {
            continue;
        }
        const auto bound = kernel.bindEdgeToBody(
            stage->result_solid,
            edge.runtime_token);
        if (!bound) {
            continue;
        }
        const auto projected =
            kernel.projectEdgeToPlane(
                stage->result_solid, *bound, xy);
        if (!projected.ok()) {
            continue;
        }
        const auto* line =
            std::get_if<kernel::Line2>(
                &*projected.curve);
        if (!line) {
            continue;
        }
        const auto min_u =
            std::min(line->start.u, line->end.u);
        const auto max_u =
            std::max(line->start.u, line->end.u);
        if (close(min_u, 0.0) &&
            close(max_u, 40.0) &&
            close(line->start.v, 0.0) &&
            close(line->end.v, 0.0)) {
            selected = *semantic.reference;
        }
    }
    CHECK(selected && selected->valid());
    CHECK(selected->stage.feature_id == fixture.base_id);

    const auto sk = fixture.session.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(sk.ok() && sk.sketch_id);
    const auto id = *sk.sketch_id;
    const auto sourceLine = fixture.session.execute(
        application::AddSketchLineCommand{
            id, {0.0, 0.0}, {40.0, 0.0},
            sketch::EntityRole::regular});
    CHECK(sourceLine.ok() && sourceLine.entity_id);
    CHECK(fixture.session.execute(
        application::AddSketchLineCommand{
            id, {40.0, 0.0}, {40.0, 10.0},
            sketch::EntityRole::regular}).ok());
    CHECK(fixture.session.execute(
        application::AddSketchLineCommand{
            id, {40.0, 10.0}, {0.0, 10.0},
            sketch::EntityRole::regular}).ok());
    CHECK(fixture.session.execute(
        application::AddSketchLineCommand{
            id, {0.0, 10.0}, {0.0, 0.0},
            sketch::EntityRole::regular}).ok());
    const auto* sketch =
        fixture.session.document().findSketch(id);
    CHECK(sketch != nullptr);
    const auto regions =
        sketch::analyzeRegions(sketch->model);
    CHECK(regions.complete() &&
          regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent);
    const auto profile =
        fixture.session.execute(
            application::CreateProfileCommand{
                id,
                fixture.session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto cut =
        fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                fixture.session.document().revision(),
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{8.0}, false},
                "PG-01B Native Linked Cut"},
            kernel);
    CHECK(cut.ok() && cut.feature_id);

    // Deliberately store an OPEN authored seed while retaining the old
    // ProfileId and one linked EntityId. A stale-seed evaluator cannot
    // produce the current solid; correct stage-scoped projection can.
    auto state = fixture.session.document().state();
    auto target = std::find_if(
        state.sketches.begin(), state.sketches.end(),
        [&id](const part::PartSketch& x) {
            return x.id == id;
        });
    CHECK(target != state.sketches.end());
    CHECK(target->model.updateLine(
        *sourceLine.entity_id,
        {0.0, -8.0}, {40.0, -8.0}));
    target->projection_bindings.push_back({
        *sourceLine.entity_id, *selected});
    auto restored = part::PartDocument::restore(
        fixture.session.document().documentId(),
        std::move(state));
    CHECK(restored.ok());
    CHECK(!restored.document->evaluateProfile(
        *profile.profile_id));

    kernel_occt::OcctSolidModelingKernel cold;
    const auto evaluated =
        part::evaluatePart(*restored.document, cold);
    CHECK(evaluated.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    const auto* current =
        evaluated.findFeature(*cut.feature_id);
    CHECK(current && current->status ==
          part::FeatureEvaluationStatus::up_to_date);
    const auto effective =
        part::evaluateEffectiveSketchProjection(
            *restored.document, id,
            evaluated, cold,
            *cut.feature_id);
    CHECK(effective && effective->allResolved());
    const auto* line =
        effective->model.findLine(
            *sourceLine.entity_id);
    CHECK(line);
    CHECK(close(line->start().v, 0.0));
    CHECK(close(line->end().v, 0.0));
    CHECK(restored.document->findSketch(id)
              ->model.findLine(*sourceLine.entity_id)
              ->start().v == -8.0);

    // B3 draft regression: Finish already uses the effective source.
    // The live Edit Extrude preview must use it too, not the OPEN
    // authored line at v=-8 which cannot produce this Profile.
    auto preview_copy = part::PartDocument::restore(
        restored.document->documentId(),
        restored.document->state());
    CHECK(preview_copy.ok());
    application::DocumentSession preview_session{
        {}, std::move(*preview_copy.document)};
    const auto edit_draft =
        application::ExtrudeDraft::beginEdit(
            preview_session, *cut.feature_id);
    CHECK(edit_draft);
    const auto edit_preview =
        preview_session.evaluateExtrudeDraft(
            *edit_draft, kernel);
    CHECK(edit_preview.committable());
    CHECK(edit_preview.body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(edit_preview.previewSolidAvailable());
    CHECK(preview_session.document().findSketch(id)
              ->model.findLine(*sourceLine.entity_id)
              ->start().v == -8.0);

    // Break Link must freeze the exact CURRENT evaluation, not the
    // broken (-8 mm) persisted seed. The same EntityId survives and
    // one Undo restores the original durable semantic source.
    auto break_copy = part::PartDocument::restore(
        restored.document->documentId(),
        restored.document->state());
    CHECK(break_copy.ok());
    application::DocumentSession detached{
        {}, std::move(*break_copy.document)};
    const auto broken_revision =
        detached.document().revision();
    const auto broken_link =
        detached.execute(
            application::BreakProjectedEdgeLinkCommand{
                id, *sourceLine.entity_id,
                broken_revision},
            cold);
    CHECK(broken_link.ok());
    CHECK(detached.document().findSketch(id)
              ->projection_bindings.empty());
    CHECK(close(detached.document().findSketch(id)
              ->model.findLine(*sourceLine.entity_id)
              ->start().v, 0.0));
    CHECK(detached.undo().changed);
    CHECK(detached.document().findSketch(id)
              ->projection_bindings.size() == 1U);
    CHECK(detached.document().findSketch(id)
              ->model.findLine(*sourceLine.entity_id)
              ->start().v == -8.0);
    CHECK(detached.redo().changed);
    CHECK(detached.document().findSketch(id)
              ->projection_bindings.empty());
    const auto not_linked =
        detached.execute(
            application::BreakProjectedEdgeLinkCommand{
                id, *sourceLine.entity_id,
                detached.document().revision()},
            cold);
    CHECK(!not_linked.ok() && !not_linked.changed);

    auto suppressed = restored.document->state();
    suppressed.body.features.front().suppressed = true;
    auto unavailable = part::PartDocument::restore(
        restored.document->documentId(),
        std::move(suppressed));
    CHECK(unavailable.ok());
    kernel_occt::OcctSolidModelingKernel no_source;
    const auto blocked =
        part::evaluatePart(
            *unavailable.document, no_source);
    CHECK(blocked.body_status ==
          part::BodyEvaluationStatus::unavailable);
    CHECK(!blocked.body_solid);

    // B4 broken binding cannot Break Link using the old authored seed or
    // last-good provider result. Failure must be mutation-free.
    auto broken_copy = part::PartDocument::restore(
        unavailable.document->documentId(),
        unavailable.document->state());
    CHECK(broken_copy.ok());
    application::DocumentSession broken_session{
        {}, std::move(*broken_copy.document)};
    const auto broken_before = broken_session.document().state();
    const auto cannot_detach = broken_session.execute(
        application::BreakProjectedEdgeLinkCommand{
            id, *sourceLine.entity_id,
            broken_session.document().revision()},
        no_source);
    CHECK(!cannot_detach.ok() && !cannot_detach.changed);
    CHECK(cannot_detach.status ==
          application::BreakProjectedEdgeLinkStatus::source_unavailable);
    CHECK(broken_session.document().state() == broken_before);
    CHECK(broken_session.document().findSketch(id)
              ->projection_bindings.size() == 1U);

    std::cout
        << "PG01B_B3_NATIVE_LINKED_CUT_PASS"
        << " current_occt_edge=1"
        << " authored_seed_stale=1"
        << " same_profile_id=1"
        << " missing_source_fail_closed=1"
        << " break_link_undo_redo=1"
        << " broken_detach_denied=1\n";
}

} // namespace

int main() {
    verifyPg01bAtomicProjectedEdges();
    verifyPg01bNativeDerivedProfileExtrude();
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
