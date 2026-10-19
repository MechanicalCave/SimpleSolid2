#include <simplesolid2/sketch/local_solver.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <utility>

namespace simplesolid2::sketch {
namespace {

[[nodiscard]] const SketchLineState* findLineState(
    const SketchModelState& state, EntityId id) noexcept {
    const auto it = std::find_if(
        state.lines.cbegin(), state.lines.cend(),
        [id](const SketchLineState& line) { return line.id == id; });
    return it == state.lines.cend() ? nullptr : &*it;
}

[[nodiscard]] std::optional<Point2> endpoint(
    const SketchModelState& state,
    ConstraintReference ref) noexcept {
    const auto* line = findLineState(state, ref.entity);
    if (!line) return std::nullopt;
    switch (ref.element) {
    case ConstraintElement::line_start:
        return line->start;
    case ConstraintElement::line_end:
        return line->end;
    default:
        return std::nullopt;
    }
}

[[nodiscard]] std::optional<double> lineAngle(
    const SketchModelState& state,
    EntityId id) noexcept {
    const auto* line = findLineState(state, id);
    if (!line) return std::nullopt;
    const double du = line->end.u - line->start.u;
    const double dv = line->end.v - line->start.v;
    if (!std::isfinite(du) || !std::isfinite(dv)) {
        return std::nullopt;
    }
    const double angle = std::atan2(dv, du);
    return std::isfinite(angle)
        ? std::optional<double>{angle} : std::nullopt;
}

struct ResidualValue final {
    LocalResidualUnit unit;
    double amount;
};

[[nodiscard]] std::optional<ResidualValue> residual(
    const SketchModelState& state,
    const SketchConstraintState& constraint) noexcept {
    switch (constraint.kind) {
    case ConstraintKind::horizontal:
    case ConstraintKind::vertical: {
        const auto* line = findLineState(state, constraint.first.entity);
        if (!line) return std::nullopt;
        const auto value = constraint.kind == ConstraintKind::horizontal
            ? std::abs(line->end.v - line->start.v)
            : std::abs(line->end.u - line->start.u);
        return std::isfinite(value)
            ? std::optional<ResidualValue>{
                ResidualValue{LocalResidualUnit::model_distance, value}}
            : std::nullopt;
    }
    case ConstraintKind::coincident: {
        const auto first = endpoint(state, constraint.first);
        const auto second = endpoint(state, constraint.second);
        if (!first || !second) return std::nullopt;
        const double du = first->u - second->u;
        const double dv = first->v - second->v;
        const double value = std::hypot(du, dv);
        return std::isfinite(value)
            ? std::optional<ResidualValue>{
                ResidualValue{LocalResidualUnit::model_distance, value}}
            : std::nullopt;
    }
    case ConstraintKind::parallel:
    case ConstraintKind::perpendicular: {
        const auto first = lineAngle(state, constraint.first.entity);
        const auto second = lineAngle(state, constraint.second.entity);
        if (!first || !second) return std::nullopt;
        const double target = constraint.kind == ConstraintKind::parallel
            ? 0.0 : 0.5 * std::numbers::pi_v<double>;
        // Undirected Line orientation: modulo PI, never modulo full turn.
        const double value = std::abs(std::remainder(
            *first - *second - target, std::numbers::pi_v<double>));
        return std::isfinite(value)
            ? std::optional<ResidualValue>{
                ResidualValue{LocalResidualUnit::radians, value}}
            : std::nullopt;
    }
    }
    return std::nullopt;
}

[[nodiscard]] bool sameNonGeometryState(
    const SketchModelState& original,
    const SketchModelState& candidate) noexcept {
    if (original.next_entity_id != candidate.next_entity_id ||
        original.next_constraint_id != candidate.next_constraint_id ||
        original.constraints != candidate.constraints ||
        original.circles != candidate.circles ||
        original.arcs != candidate.arcs ||
        original.lines.size() != candidate.lines.size()) {
        return false;
    }
    for (std::size_t i = 0; i < original.lines.size(); ++i) {
        if (original.lines[i].id != candidate.lines[i].id ||
            original.lines[i].role != candidate.lines[i].role) {
            return false;
        }
    }
    return true;
}

} // namespace

std::optional<LocalSolveProblem> prepareLocalSolveProblem(
    const SketchModelState& authored,
    std::span<const EntityId> fixed_lines) {
    const auto model = SketchModel::restore(authored);
    if (!model) return std::nullopt;

    std::set<EntityId> seen;
    for (const auto id : fixed_lines) {
        if (!id.valid() || !seen.insert(id).second ||
            model->findLine(id) == nullptr) {
            return std::nullopt;
        }
    }

    LocalSolveProblem result;
    result.authored = authored;
    result.constraints = authored.constraints;
    result.fixed_lines.assign(fixed_lines.begin(), fixed_lines.end());
    result.lines.reserve(authored.lines.size());
    for (const auto& line : authored.lines) {
        result.lines.push_back(LocalSolveLineInput{
            line.id, line.start, line.end, seen.contains(line.id)});
    }
    return result;
}

bool LocalSolverAcceptancePolicy::valid() const noexcept {
    return std::isfinite(distance_tolerance) &&
           distance_tolerance > 0.0 &&
           std::isfinite(angular_tolerance_radians) &&
           angular_tolerance_radians > 0.0;
}

LocalResidualReport reviewLocalSolveCandidate(
    const LocalSolveProblem& problem,
    const SketchModelState& candidate,
    const LocalSolverAcceptancePolicy& policy) {
    LocalResidualReport result;
    if (!policy.valid()) {
        result.status = LocalResidualStatus::invalid_policy;
        return result;
    }
    if (!SketchModel::restore(candidate)) {
        result.status = LocalResidualStatus::invalid_candidate;
        return result;
    }
    if (!SketchModel::restore(problem.authored) ||
        problem.constraints != problem.authored.constraints ||
        problem.lines.size() != problem.authored.lines.size() ||
        !sameNonGeometryState(problem.authored, candidate)) {
        result.status = LocalResidualStatus::stale_semantics;
        return result;
    }

    // Public DTOs are validated even when not created by our builder.
    std::set<EntityId> fixed;
    for (const auto id : problem.fixed_lines) {
        if (!fixed.insert(id).second ||
            findLineState(problem.authored, id) == nullptr) {
            result.status = LocalResidualStatus::stale_semantics;
            return result;
        }
    }
    for (std::size_t i = 0; i < problem.lines.size(); ++i) {
        const auto& actual = problem.authored.lines[i];
        const auto& declared = problem.lines[i];
        if (declared.id != actual.id || declared.start != actual.start ||
            declared.end != actual.end ||
            declared.fixed != fixed.contains(declared.id)) {
            result.status = LocalResidualStatus::stale_semantics;
            return result;
        }
    }

    for (const auto id : problem.fixed_lines) {
        const auto* original = findLineState(problem.authored, id);
        const auto* proposed = findLineState(candidate, id);
        if (!original || !proposed ||
            original->start != proposed->start ||
            original->end != proposed->end) {
            result.status = LocalResidualStatus::fixed_entity_changed;
            return result;
        }
    }

    bool all_accepted = true;
    result.constraints.reserve(problem.constraints.size());
    for (const auto& constraint : problem.constraints) {
        const auto value = residual(candidate, constraint);
        if (!value) {
            result.status = LocalResidualStatus::invalid_candidate;
            return result;
        }
        const auto threshold = value->unit == LocalResidualUnit::radians
            ? policy.angular_tolerance_radians
            : policy.distance_tolerance;
        const bool accepted = value->amount <= threshold;
        all_accepted = all_accepted && accepted;
        if (value->unit == LocalResidualUnit::radians) {
            result.max_angular_residual_radians = std::max(
                result.max_angular_residual_radians, value->amount);
        } else {
            result.max_distance_residual = std::max(
                result.max_distance_residual, value->amount);
        }
        result.constraints.push_back(LocalConstraintResidual{
            constraint.id, constraint.kind, value->unit,
            value->amount, accepted});
    }
    result.status = all_accepted
        ? LocalResidualStatus::residuals_satisfied
        : LocalResidualStatus::residual_violation;
    return result;
}

LocalSolveGatewayResult evaluateLocalSolve(
    const LocalSolveProblem& problem,
    const LocalSolverAcceptancePolicy& policy,
    ILocalSketchSolver& backend) {
    LocalSolveGatewayResult result;
    if (!policy.valid()) {
        result.status = LocalSolveGatewayStatus::invalid_policy;
        return result;
    }

    // Callers cannot fabricate or alter the prepared bridge's constraint
    // inventory, reference geometry or fixed-line declaration.
    const auto canonical = prepareLocalSolveProblem(
        problem.authored, std::span<const EntityId>{problem.fixed_lines});
    if (!canonical ||
        canonical->constraints != problem.constraints ||
        canonical->fixed_lines != problem.fixed_lines ||
        canonical->lines.size() != problem.lines.size()) {
        result.status = LocalSolveGatewayStatus::invalid_problem;
        return result;
    }
    for (std::size_t i = 0; i < problem.lines.size(); ++i) {
        const auto& a = canonical->lines[i];
        const auto& b = problem.lines[i];
        if (a.id != b.id || a.start != b.start || a.end != b.end ||
            a.fixed != b.fixed) {
            result.status = LocalSolveGatewayStatus::invalid_problem;
            return result;
        }
    }

    LocalBackendSolveResult backend_result;
    try {
        backend_result = backend.solve(*canonical);
    } catch (...) {
        result.status = LocalSolveGatewayStatus::backend_failure;
        return result;
    }

    // Backend diagnostics cannot invent new ConstraintIds or repeat IDs.
    std::set<ConstraintId> seen;
    for (const auto id : backend_result.conflicting_constraints) {
        const auto it = std::find_if(
            problem.constraints.cbegin(), problem.constraints.cend(),
            [id](const SketchConstraintState& c) { return c.id == id; });
        if (!id.valid() || it == problem.constraints.cend() ||
            !seen.insert(id).second) {
            result.status = LocalSolveGatewayStatus::invalid_backend_response;
            return result;
        }
    }

    // A backend must not smuggle an evaluated candidate into a conflict,
    // unsupported or failed response; these are diagnostic-only outcomes.
    if ((backend_result.status != LocalBackendStatus::converged &&
         backend_result.candidate.has_value()) ||
        (backend_result.status != LocalBackendStatus::conflict &&
         !backend_result.conflicting_constraints.empty())) {
        result.status = LocalSolveGatewayStatus::invalid_backend_response;
        return result;
    }

    bool redundant = false;
    std::set<ConstraintId> diagnostic_ids;
    for (const auto& diagnostic : backend_result.diagnostics) {
        if (diagnostic.constraint) {
            const auto id = *diagnostic.constraint;
            const auto found = std::find_if(
                problem.constraints.cbegin(), problem.constraints.cend(),
                [id](const SketchConstraintState& c) { return c.id == id; });
            if (!id.valid() || found == problem.constraints.cend() ||
                !diagnostic_ids.insert(id).second) {
                result.status = LocalSolveGatewayStatus::invalid_backend_response;
                return result;
            }
        }

        // A successful numeric answer may report redundancy but never a
        // conflict or failure. Redundant intent is rejected atomically,
        // rather than silently publishing an underdetermined solution.
        bool compatible = false;
        switch (diagnostic.kind) {
        case LocalBackendDiagnosticKind::redundant_constraint:
            redundant = true;
            compatible = backend_result.status == LocalBackendStatus::converged &&
                         diagnostic.constraint.has_value();
            break;
        case LocalBackendDiagnosticKind::conflicting_constraint:
            compatible = backend_result.status == LocalBackendStatus::conflict &&
                         diagnostic.constraint.has_value();
            break;
        case LocalBackendDiagnosticKind::unsupported_constraint:
            compatible = backend_result.status == LocalBackendStatus::unsupported;
            break;
        case LocalBackendDiagnosticKind::no_convergence:
        case LocalBackendDiagnosticKind::backend_failure:
        case LocalBackendDiagnosticKind::residual_violation:
            compatible = backend_result.status == LocalBackendStatus::failed;
            break;
        }
        if (!compatible) {
            result.status = LocalSolveGatewayStatus::invalid_backend_response;
            return result;
        }
    }
    result.diagnostics = backend_result.diagnostics;

    switch (backend_result.status) {
    case LocalBackendStatus::conflict:
        result.status = LocalSolveGatewayStatus::conflict;
        result.conflicting_constraints =
            std::move(backend_result.conflicting_constraints);
        return result;
    case LocalBackendStatus::unsupported:
        if (!backend_result.conflicting_constraints.empty()) break;
        result.status = LocalSolveGatewayStatus::unsupported;
        return result;
    case LocalBackendStatus::failed:
        if (!backend_result.conflicting_constraints.empty()) break;
        result.status = LocalSolveGatewayStatus::backend_failure;
        return result;
    case LocalBackendStatus::converged:
        if (!backend_result.candidate ||
            !backend_result.conflicting_constraints.empty()) {
            break;
        }
        if (redundant) {
            result.status = LocalSolveGatewayStatus::redundant_constraint;
            return result;
        }
        result.residuals = reviewLocalSolveCandidate(
            *canonical, *backend_result.candidate, policy);
        switch (result.residuals.status) {
        case LocalResidualStatus::residuals_satisfied:
            result.status = LocalSolveGatewayStatus::candidate_accepted;
            // A COPY, not a live mutable reference to authored state.
            result.candidate = std::move(backend_result.candidate);
            result.degrees_of_freedom = backend_result.degrees_of_freedom;
            return result;
        case LocalResidualStatus::residual_violation:
            result.status = LocalSolveGatewayStatus::residual_violation;
            for (const auto& value : result.residuals.constraints) {
                if (!value.accepted) {
                    result.diagnostics.push_back(LocalBackendDiagnostic{
                        LocalBackendDiagnosticKind::residual_violation,
                        value.constraint_id});
                }
            }
            return result;
        case LocalResidualStatus::invalid_candidate:
            result.status = LocalSolveGatewayStatus::invalid_candidate;
            return result;
        case LocalResidualStatus::stale_semantics:
            result.status = LocalSolveGatewayStatus::stale_semantics;
            return result;
        case LocalResidualStatus::fixed_entity_changed:
            result.status = LocalSolveGatewayStatus::fixed_entity_changed;
            return result;
        case LocalResidualStatus::invalid_policy:
            result.status = LocalSolveGatewayStatus::invalid_policy;
            return result;
        }
        break;
    }
    result.status = LocalSolveGatewayStatus::invalid_backend_response;
    return result;
}

} // namespace simplesolid2::sketch
