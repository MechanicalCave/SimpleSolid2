#pragma once

#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstddef>
#include <span>
#include <vector>

namespace simplesolid2::sketch {

// Solver-neutral R13-A1 bridge adapted from SS1 ConstraintProblem /
// SolveResult / SolverAcceptancePolicy. No PlaneGCS, vendor dependency,
// algorithm or publication of solved geometry is owned by this API.
//
// The caller supplies fixed source lines from the exact current Part stage;
// Sketch has no authority to guess which lines are PG-01 projections.
struct LocalSolveLineInput final {
    EntityId id;
    Point2 start;
    Point2 end;
    bool fixed{false};
};

struct LocalSolveProblem final {
    SketchModelState authored;
    std::vector<LocalSolveLineInput> lines;
    // Complete authored records, including Fix for identity/diagnostics.
    std::vector<SketchConstraintState> constraints;
    std::vector<EntityId> fixed_lines;
    // Explicit equation inventory for the donor backend: five geometric
    // kinds only. Fix(Line) is encoded solely by fixed_lines.
    std::vector<SketchConstraintState> equations;
};

// Fail closed on corrupt semantic intent, duplicate/missing/non-Line fixed
// references, or a non-canonical Sketch. No mutation and no numerical solve.
[[nodiscard]] std::optional<LocalSolveProblem> prepareLocalSolveProblem(
    const SketchModelState& authored,
    std::span<const EntityId> fixed_lines = {});

// Exactly as in SS1, tolerances belong to the caller (Sketch model units and
// radians). Neither a hidden default epsilon nor a solver-success inference.
struct LocalSolverAcceptancePolicy final {
    double distance_tolerance{};
    double angular_tolerance_radians{};

    [[nodiscard]] bool valid() const noexcept;
};

enum class LocalResidualUnit {
    model_distance,
    radians
};

struct LocalConstraintResidual final {
    ConstraintId constraint_id;
    ConstraintKind kind{ConstraintKind::horizontal};
    LocalResidualUnit unit{LocalResidualUnit::model_distance};
    double residual{};
    bool accepted{false};
};

enum class LocalResidualStatus {
    residuals_satisfied, // residual check only, NOT solver convergence
    invalid_policy,
    invalid_candidate,
    stale_semantics,
    fixed_entity_changed,
    residual_violation
};

struct LocalResidualReport final {
    LocalResidualStatus status{LocalResidualStatus::invalid_candidate};
    std::vector<LocalConstraintResidual> constraints;
    double max_distance_residual{};
    double max_angular_residual_radians{};
};

// Review a disposable candidate produced by a future SS1-backed solver:
// same IDs, roles, topology, authored intent and cursor, no moved fixed line,
// finite non-degenerate geometry and every residual within caller-owned policy.
// This check NEVER certifies DoF/redundancy, backend convergence or permission
// to publish effective Sketch to Profile / Kernel / Part.
[[nodiscard]] LocalResidualReport reviewLocalSolveCandidate(
    const LocalSolveProblem& problem,
    const SketchModelState& candidate,
    const LocalSolverAcceptancePolicy& policy);

// Narrow SS1 ISketchSolver-style injection point. A real vendor-backed
// implementation still requires separate SHA/license/Windows approval.
enum class LocalBackendStatus {
    converged,
    conflict,
    failed,
    unsupported
};

// Backend-neutral diagnostics, mirroring the tested SS1 distinction between
// failure classification and the numerical candidate snapshot.
enum class LocalBackendDiagnosticKind {
    redundant_constraint,
    conflicting_constraint,
    unsupported_constraint,
    no_convergence,
    backend_failure,
    residual_violation
};

struct LocalBackendDiagnostic final {
    LocalBackendDiagnosticKind kind{LocalBackendDiagnosticKind::backend_failure};
    std::optional<ConstraintId> constraint;
    friend bool operator==(
        const LocalBackendDiagnostic&, const LocalBackendDiagnostic&) = default;
};

struct LocalBackendSolveResult final {
    LocalBackendStatus status{LocalBackendStatus::failed};
    std::optional<SketchModelState> candidate;
    std::vector<ConstraintId> conflicting_constraints;
    std::vector<LocalBackendDiagnostic> diagnostics;
    std::optional<std::size_t> degrees_of_freedom;
};

class ILocalSketchSolver {
public:
    virtual ~ILocalSketchSolver() = default;

    // Returns a disposable candidate, never mutates authoritative SketchModel.
    [[nodiscard]] virtual LocalBackendSolveResult solve(
        const LocalSolveProblem& problem) = 0;
};

enum class LocalSolveGatewayStatus {
    candidate_accepted, // ONLY an ephemeral valid candidate; no Part publication
    invalid_policy,
    invalid_problem,
    invalid_backend_response,
    redundant_constraint,
    conflict,
    unsupported,
    backend_failure,
    residual_violation,
    coincident_not_closed,
    invalid_candidate,
    stale_semantics,
    fixed_entity_changed
};

struct LocalSolveGatewayResult final {
    LocalSolveGatewayStatus status{LocalSolveGatewayStatus::invalid_problem};
    std::optional<SketchModelState> candidate;
    LocalResidualReport residuals;
    std::vector<ConstraintId> conflicting_constraints;
    std::vector<LocalBackendDiagnostic> diagnostics;
    std::optional<std::size_t> degrees_of_freedom;
};

// SS1 donor-aligned orchestration: only an explicit converged backend response
// plus independently checked unchanged semantics/fixed anchors and acceptable
// numeric residuals yields a disposable candidate. No hidden tolerance,
// no automatic publish to SketchModel, DocumentSession, Profile or Kernel.
[[nodiscard]] LocalSolveGatewayResult evaluateLocalSolve(
    const LocalSolveProblem& problem,
    const LocalSolverAcceptancePolicy& policy,
    ILocalSketchSolver& backend);

// R13-A2.0: numerical PRE-FLIGHT for one new explicit relation.
// Operates on a disposable current-stage Sketch snapshot and a caller-
// injected solver only. This API cannot commit/approve a DocumentRevision,
// Part incarnation, PG-01 linked reference, native package or UI transaction.
// An accepted preflight is NOT permission to reuse the provisional ID:
// authoritative DocumentSession must independently validate + solve the
// exact current stage in its OWN atomic revision-bound candidate transaction.
enum class LocalRelationPreflightStatus {
    invalid_intent,
    invalid_problem,
    solver_rejected,
    candidate_accepted
};

struct LocalRelationPreflightResult final {
    LocalRelationPreflightStatus status{
        LocalRelationPreflightStatus::invalid_intent};
    // Ephemeral identity from the disposable candidate; MUST NOT be
    // published, stored or supplied as an authoritative ConstraintId.
    std::optional<ConstraintId> provisional_constraint_id;
    std::optional<SketchModelState> proposed_authored;
    LocalSolveGatewayResult solve;
};

// Call with the exact stage-resolved Sketch snapshot. Fixed Line declarations
// come from the caller's current stage; this function does not infer PG-01,
// source identity or the right to author constraints on projected geometry.
// Existing Sketch-model duplicate/shape checks happen before any backend
// invocation; the current authoritative model remains byte-for-byte intact.
[[nodiscard]] LocalRelationPreflightResult preflightLocalRelation(
    const SketchModelState& current_stage,
    ConstraintKind kind,
    ConstraintReference first,
    ConstraintReference second,
    std::span<const EntityId> stage_fixed_lines,
    const LocalSolverAcceptancePolicy& policy,
    ILocalSketchSolver& backend);

} // namespace simplesolid2::sketch
