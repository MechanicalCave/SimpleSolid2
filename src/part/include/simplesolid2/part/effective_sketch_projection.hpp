#pragma once

#include <simplesolid2/kernel/edge_projection.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/local_solver.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::part {

// A source-controlled target that cannot be reprojected is deliberately
// removed only from the temporary evaluated Sketch, never from authored state.
enum class ProjectedSketchSourceStatus {
    resolved,
    missing_stage,
    ambiguous_source,
    unsupported_source,
    unsupported_projection,
    degenerate_projection,
    provider_failure,
    invalid_support,
    invalid_dependency,
    changed_curve_kind,
};

struct ProjectedSketchOutcome final {
    sketch::EntityId target_entity;
    ProjectedSketchSourceStatus status{
        ProjectedSketchSourceStatus::missing_stage};

    [[nodiscard]] bool resolved() const noexcept {
        return status == ProjectedSketchSourceStatus::resolved;
    }
};

struct EffectiveSketchProjection final {
    // A private, disposable snapshot. Original EntityId and role are kept.
    sketch::SketchModel model;
    std::vector<ProjectedSketchOutcome> outcomes;

    [[nodiscard]] bool allResolved() const noexcept {
        for (const auto& item : outcomes) {
            if (!item.resolved()) {
                return false;
            }
        }
        return true;
    }
};

// Canonical shared source resolution for evaluating an existing binding
// and atomically authoring a new one. No geometry proximity or token reuse.
struct StrictProjectedEdgeGeometry final {
    ProjectedSketchSourceStatus status{
        ProjectedSketchSourceStatus::missing_stage};
    std::optional<kernel::Curve2> curve;

    [[nodiscard]] bool resolved() const noexcept {
        return status == ProjectedSketchSourceStatus::resolved &&
               curve.has_value();
    }
};

[[nodiscard]] std::optional<kernel::Frame3>
resolveCurrentProjectionSketchFrame(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix);

[[nodiscard]] StrictProjectedEdgeGeometry
projectStrictMaterialEdge(
    const PartDocument& document,
    const MaterialEdgeReference& source,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    const kernel::Frame3& frame,
    std::optional<FeatureId> consuming_feature = std::nullopt);

// Pure projection: no PartDocument mutation, authored seed updates, runtime
// token persistence, global graph or implicit source rebinding. Caller supplies
// the exact same-revision evaluated Feature prefix. If consumer is present,
// sources must strictly precede the consuming Feature in authored order.
// B3 must consume this derived model instead of raw Sketch authored seeds.
[[nodiscard]] std::optional<EffectiveSketchProjection>
evaluateEffectiveSketchProjection(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    std::optional<FeatureId> consuming_feature = std::nullopt);

// Stage- and owner-bound Part→SS2 solver bridge. Runs the existing PG-01
// projection resolver first and uses ONLY its current ephemeral geometry.
// Every resolved projected Line is fixed in the resulting solver problem;
// a missing or unsupported projection must fail, never use authored seeds.
// This prepares an input problem only. It does NOT solve or publish geometry.
[[nodiscard]] std::optional<sketch::LocalSolveProblem>
prepareStageLocalSolveProblem(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    std::optional<FeatureId> consuming_feature = std::nullopt);

// A disposable numerical result can be handed between Part consumers only
// with an explicit immutable owner/stage label. It is NOT a published Profile,
// OCCT input, persistence record or authorization to reuse a later revision.
struct StageLocalSolveEvaluation final {
    core::DocumentId source_document_id;
    core::DocumentRevision source_revision;
    sketch::SketchId source_sketch_id;
    std::optional<FeatureId> consuming_feature;
    sketch::LocalSolveGatewayResult solution;
};

// Compose the existing strict same-stage PG-01/Sketch preparation with the
// solver-neutral gateway. Never invoke backend for absent/stale owner, stage,
// sketch or invalid caller policy. Recheck revision AFTER backend invocation,
// which may re-enter a DocumentSession; stale numerical answers are discarded.
// Returns nullopt for invalid/stale Part context. Backend conflicts/failure
// remain typed in solution, never promoted to accepted geometry.
// No mutation, profile publication, vendor import or global solve cache.
[[nodiscard]] std::optional<StageLocalSolveEvaluation>
evaluateStageLocalSolve(
    const PartDocument& document,
    const sketch::SketchId& sketch_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    const sketch::LocalSolverAcceptancePolicy& policy,
    sketch::ILocalSketchSolver& backend,
    std::optional<FeatureId> consuming_feature = std::nullopt);


// The ONLY stage-authorized gateway from a constrained Sketch into the
// existing ProfileRegionIntent -> shared 2D -> Kernel profile machinery.
// Internally prepares the actual current PG-01 stage, invokes and accepts the
// solver, and immediately resolves the derived geometry for this Profile.
// No caller-supplied arbitrary SketchModel can bypass the legacy safety fence.
// Does NOT publish a Profile, evaluate a Body/OCCT feature, or persist geometry.
[[nodiscard]] ProfileKernelInputResult
resolveStageSolvedProfileKernelInput(
    const PartDocument& document,
    ProfileId profile_id,
    const PartEvaluation& evaluated_prefix,
    kernel::IEdgeProjectionQuery& query,
    const sketch::LocalSolverAcceptancePolicy& policy,
    sketch::ILocalSketchSolver& backend,
    const BodyStageTopologyCatalog* support_topology = nullptr,
    const DatumEvaluation* datum_evaluation = nullptr,
    std::optional<FeatureId> consuming_feature = std::nullopt);

} // namespace simplesolid2::part
