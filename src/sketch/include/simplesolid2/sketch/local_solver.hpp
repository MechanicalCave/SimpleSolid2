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
    std::vector<SketchConstraintState> constraints;
    std::vector<EntityId> fixed_lines;
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

} // namespace simplesolid2::sketch
