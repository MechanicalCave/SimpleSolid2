#include <simplesolid2/sketch/local_solver.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <limits>
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
    case ConstraintKind::fix:
        // Fix is an immutable-parameter declaration. It must never reach
        // scalar equation residual evaluation; fixed-line equality is
        // checked separately and exactly.
        return std::nullopt;
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

// Canonicalize ONLY explicitly authored Coincident endpoint groups after
// a backend reports convergence and all pre-normalization residuals pass.
// The result is a disposable solved candidate, never an authored edit.
//
// One canonical point per connected component prevents a chain of locally
// accepted residuals from drifting beyond the caller's distance tolerance.
// A Fix/PG-01 point wins over a free point, and fixed geometry never moves.
// Group representatives are chosen by semantic EntityId + endpoint role,
// not solver iteration order or relation insertion order.
[[nodiscard]] std::optional<SketchModelState> closeSolvedCoincidences(
    const LocalSolveProblem& problem,
    const SketchModelState& candidate,
    const LocalSolverAcceptancePolicy& policy) {
    using EndpointKey = std::pair<EntityId, ConstraintElement>;
    std::map<EndpointKey, std::size_t> index;
    std::vector<ConstraintReference> references;
    std::vector<std::size_t> parent;

    const auto add = [&index, &references, &parent](
        ConstraintReference reference) -> std::size_t {
        const EndpointKey key{reference.entity, reference.element};
        const auto found = index.find(key);
        if (found != index.end()) return found->second;
        const std::size_t value = references.size();
        index.emplace(key, value);
        references.push_back(reference);
        parent.push_back(value);
        return value;
    };
    const auto root = [&parent](std::size_t value) {
        while (parent[value] != value) {
            parent[value] = parent[parent[value]];
            value = parent[value];
        }
        return value;
    };
    for (const auto& equation : problem.equations) {
        if (equation.kind != ConstraintKind::coincident) continue;
        const auto a = root(add(equation.first));
        const auto b = root(add(equation.second));
        if (a != b) parent[std::max(a, b)] = std::min(a, b);
    }

    SketchModelState closed = candidate;
    if (references.empty()) return closed;

    std::vector<std::vector<std::size_t>> groups(references.size());
    for (std::size_t i = 0; i < references.size(); ++i) {
        groups[root(i)].push_back(i);
    }
    const auto fixed = [&problem, &references](std::size_t i) {
        return std::find(problem.fixed_lines.begin(),
                         problem.fixed_lines.end(),
                         references[i].entity) != problem.fixed_lines.end();
    };
    const auto key = [&references](std::size_t i) {
        return EndpointKey{references[i].entity, references[i].element};
    };

    for (const auto& group : groups) {
        if (group.empty()) continue;
        std::size_t chosen = group.front();
        for (const auto i : group) {
            if ((fixed(i) && !fixed(chosen)) ||
                (fixed(i) == fixed(chosen) && key(i) < key(chosen))) {
                chosen = i;
            }
        }
        const auto anchor = fixed(chosen)
            ? endpoint(problem.authored, references[chosen])
            : endpoint(candidate, references[chosen]);
        if (!anchor || !anchor->finite()) return std::nullopt;

        for (const auto i : group) {
            const auto before = endpoint(candidate, references[i]);
            if (!before) return std::nullopt;
            const double distance = std::hypot(
                before->u - anchor->u, before->v - anchor->v);
            if (!std::isfinite(distance) ||
                distance > policy.distance_tolerance) {
                return std::nullopt;
            }
            if (fixed(i)) {
                if (*before != *anchor) return std::nullopt;
                continue;
            }
            const auto found = std::find_if(
                closed.lines.begin(), closed.lines.end(),
                [&reference = references[i]](const SketchLineState& line) {
                    return line.id == reference.entity;
                });
            if (found == closed.lines.end()) return std::nullopt;
            if (references[i].element == ConstraintElement::line_start) {
                found->start = *anchor;
            } else if (references[i].element == ConstraintElement::line_end) {
                found->end = *anchor;
            } else {
                return std::nullopt;
            }
        }
    }
    return closed;
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
    result.equations.reserve(authored.constraints.size());
    for (const auto& relation : authored.constraints) {
        if (relation.kind != ConstraintKind::fix) {
            result.equations.push_back(relation);
        }
    }
    result.fixed_lines.assign(fixed_lines.begin(), fixed_lines.end());
    // A user-authored Fix(Line) is as strong as a fixed PG-01 source,
    // without requiring any Body projection. Preserve both forms of
    // grounding in the same canonical frozen input, without duplicates.
    for (const auto& relation : authored.constraints) {
        if (relation.kind != ConstraintKind::fix) continue;
        if (seen.insert(relation.first.entity).second) {
            result.fixed_lines.push_back(relation.first.entity);
        }
    }
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
    const auto canonical = prepareLocalSolveProblem(
        problem.authored, std::span<const EntityId>{problem.fixed_lines});
    if (!canonical ||
        canonical->constraints != problem.constraints ||
        canonical->equations != problem.equations ||
        canonical->fixed_lines != problem.fixed_lines ||
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
    for (const auto& constraint : problem.equations) {
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
    // inventory, reference geometry or fixed-line declaration. Keep the
    // same exact comparison after the backend call: the solver receives
    // a const view of our mutable temporary, and a faulty native adapter
    // must never rewrite the very equations used to review its answer.
    const auto canonical = prepareLocalSolveProblem(
        problem.authored, std::span<const EntityId>{problem.fixed_lines});
    if (!canonical) {
        result.status = LocalSolveGatewayStatus::invalid_problem;
        return result;
    }
    // Independent entry-time authority. Comparing two *live* DTOs after
    // solve is insufficient: a reentrant adapter can mutate both the
    // caller problem and its canonical temporary identically. This
    // snapshot is never exposed to the backend.
    const LocalSolveProblem original = *canonical;
    const auto matchesCaller = [&problem, &canonical, &original]() {
        if (canonical->authored != original.authored ||
            problem.authored != original.authored ||
            canonical->constraints != original.constraints ||
            problem.constraints != original.constraints ||
            canonical->equations != original.equations ||
            problem.equations != original.equations ||
            canonical->fixed_lines != original.fixed_lines ||
            problem.fixed_lines != original.fixed_lines ||
            canonical->lines.size() != original.lines.size() ||
            problem.lines.size() != original.lines.size()) {
            return false;
        }
        for (std::size_t i = 0; i < original.lines.size(); ++i) {
            const auto& expected = original.lines[i];
            const auto& received = canonical->lines[i];
            const auto& current = problem.lines[i];
            const auto matches = [&expected](const LocalSolveLineInput& line) {
                return expected.id == line.id &&
                       expected.start == line.start &&
                       expected.end == line.end &&
                       expected.fixed == line.fixed;
            };
            if (!matches(received) || !matches(current)) {
                return false;
            }
        }
        return true;
    };
    if (!matchesCaller()) {
        result.status = LocalSolveGatewayStatus::invalid_problem;
        return result;
    }

    LocalBackendSolveResult backend_result;
    try {
        backend_result = backend.solve(*canonical);
    } catch (...) {
        result.status = LocalSolveGatewayStatus::backend_failure;
        return result;
    }
    if (!matchesCaller()) {
        result.status = LocalSolveGatewayStatus::invalid_backend_response;
        return result;
    }

    // An externally supplied DoF is diagnostic evidence, not solver
    // authority. It cannot exceed the number of mutable scalar inputs:
    // a free Line has exactly four U/V endpoint coordinates; a Fix or
    // stage-fixed Line has zero. This is an upper-bound sanity check,
    // not a claim that DoF rank/redundancy is independently computed.
    if (backend_result.degrees_of_freedom) {
        const auto free_lines = static_cast<std::size_t>(
            std::count_if(canonical->lines.cbegin(),
                          canonical->lines.cend(),
                          [](const LocalSolveLineInput& line) {
                              return !line.fixed;
                          }));
        if (free_lines > std::numeric_limits<std::size_t>::max() / 4U ||
            *backend_result.degrees_of_freedom > 4U * free_lines) {
            result.status = LocalSolveGatewayStatus::invalid_backend_response;
            return result;
        }
    }

    // Fix(Line) is a fixed-parameter mask, never a backend equation.
    // Check equation-bound conflict provenance against the immutable,
    // entry-time canonical inventory, not merely any authored ConstraintId.
    const auto isEquation = [&original](ConstraintId id) {
        return std::any_of(
            original.equations.cbegin(), original.equations.cend(),
            [id](const SketchConstraintState& relation) {
                return relation.id == id;
            });
    };
    std::set<ConstraintId> seen;
    for (const auto id : backend_result.conflicting_constraints) {
        if (!id.valid() || !isEquation(id) || !seen.insert(id).second) {
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
    std::set<ConstraintId> diagnosed_conflicts;
    for (const auto& diagnostic : backend_result.diagnostics) {
        if (diagnostic.constraint) {
            const auto id = *diagnostic.constraint;
            const auto found = std::find_if(
                problem.constraints.cbegin(), problem.constraints.cend(),
                [id](const SketchConstraintState& c) { return c.id == id; });
            // Redundancy/conflict diagnostics must name an actual
            // equation. A Fix ID can exist in the Sketch inventory but
            // has no algebraic equation to diagnose or highlight.
            const bool equation_diagnostic =
                diagnostic.kind ==
                    LocalBackendDiagnosticKind::redundant_constraint ||
                diagnostic.kind ==
                    LocalBackendDiagnosticKind::conflicting_constraint;
            if (!id.valid() || found == problem.constraints.cend() ||
                (equation_diagnostic && !isEquation(id)) ||
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
            if (compatible) {
                diagnosed_conflicts.insert(*diagnostic.constraint);
            }
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
    // Detailed conflict diagnostics identify the exact same semantic
    // owners as the conflict-ID vector consumed by viewer/UI highlights.
    // An older backend may return only the conflict vector, but if it
    // supplies detailed conflict IDs their inventory must be complete
    // and identical. Never attribute one conflict to another constraint.
    if (!diagnosed_conflicts.empty() &&
        diagnosed_conflicts != seen) {
        result.status = LocalSolveGatewayStatus::invalid_backend_response;
        return result;
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
        case LocalResidualStatus::residuals_satisfied: {
            // The caller-owned tolerance may admit a tiny/1e-12
            // Coincident residual, but Profile must never consume a seam.
            // Close ONLY explicit endpoint groups in derived candidate
            // geometry; keep Fix/PG-01 anchors and all authored seeds exact.
            const auto closed = closeSolvedCoincidences(
                *canonical, *backend_result.candidate, policy);
            if (!closed) {
                result.status = LocalSolveGatewayStatus::coincident_not_closed;
                for (const auto& equation : canonical->equations) {
                    if (equation.kind == ConstraintKind::coincident) {
                        result.diagnostics.push_back(LocalBackendDiagnostic{
                            LocalBackendDiagnosticKind::residual_violation,
                            equation.id});
                    }
                }
                return result;
            }
            // Canonicalization can change other relations or degenerate a
            // Line: review the entire candidate again, not just the seam.
            result.residuals = reviewLocalSolveCandidate(
                *canonical, *closed, policy);
            if (result.residuals.status !=
                LocalResidualStatus::residuals_satisfied) {
                result.status = LocalSolveGatewayStatus::invalid_candidate;
                if (result.residuals.status ==
                    LocalResidualStatus::residual_violation) {
                    result.status = LocalSolveGatewayStatus::residual_violation;
                    for (const auto& value : result.residuals.constraints) {
                        if (!value.accepted) {
                            result.diagnostics.push_back(LocalBackendDiagnostic{
                                LocalBackendDiagnosticKind::residual_violation,
                                value.constraint_id});
                        }
                    }
                }
                return result;
            }
            result.status = LocalSolveGatewayStatus::candidate_accepted;
            // A COPY of the normalized, stage-disposable candidate only.
            result.candidate = std::move(*closed);
            result.degrees_of_freedom = backend_result.degrees_of_freedom;
            return result;
        }
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

namespace {

// An A2 create command must not depend solely on a backend diagnosing
// elementary redundant equations: a well-formed Converged response can
// omit redundancy diagnostics. This detects ONLY logically implied NEW
// relations, never rewrites an existing Sketch or the generic solver.
//
// Parallel/Perpendicular form an undirected parity graph (0 = parallel,
// 1 = perpendicular). Horizontal is parallel to the single horizontal
// reference axis; Vertical is perpendicular to it. The graph must be
// derived from authored constraints, NOT coincident seed coordinates.
[[nodiscard]] bool authoredRelationsImply(
    const SketchModelState& authored,
    ConstraintKind kind,
    ConstraintReference first,
    ConstraintReference second) {
    if (kind == ConstraintKind::coincident) {
        // Equality is transitive over exact SEMANTIC endpoints, not whole
        // Line identity or neighboring/nearby model-space coordinates.
        std::vector<ConstraintReference> reachable{first};
        for (std::size_t i = 0; i < reachable.size(); ++i) {
            for (const auto& relation : authored.constraints) {
                if (relation.kind != ConstraintKind::coincident) continue;
                ConstraintReference other;
                if (relation.first == reachable[i]) {
                    other = relation.second;
                } else if (relation.second == reachable[i]) {
                    other = relation.first;
                } else {
                    continue;
                }
                if (other == second) return true;
                if (std::find(reachable.cbegin(), reachable.cend(), other)
                    == reachable.cend()) {
                    reachable.push_back(other);
                }
            }
        }
        return false;
    }

    if (kind != ConstraintKind::horizontal &&
        kind != ConstraintKind::vertical &&
        kind != ConstraintKind::parallel &&
        kind != ConstraintKind::perpendicular) {
        return false;
    }

    std::map<EntityId, std::size_t> indices;
    for (std::size_t i = 0; i < authored.lines.size(); ++i) {
        indices.emplace(authored.lines[i].id, i);
    }
    const auto first_node = indices.find(first.entity);
    if (first_node == indices.end()) return false;
    const std::size_t axis = authored.lines.size();
    const bool unary = kind == ConstraintKind::horizontal ||
                       kind == ConstraintKind::vertical;
    std::size_t destination = axis;
    if (!unary) {
        const auto second_node = indices.find(second.entity);
        if (second_node == indices.end()) return false;
        destination = second_node->second;
    }
    const int required = (kind == ConstraintKind::vertical ||
                          kind == ConstraintKind::perpendicular) ? 1 : 0;

    using Edge = std::pair<std::size_t, int>;
    std::vector<std::vector<Edge>> connections(axis + 1U);
    const auto connect = [&connections](
        std::size_t a, std::size_t b, int parity) {
        connections[a].push_back({b, parity});
        connections[b].push_back({a, parity});
    };
    for (const auto& relation : authored.constraints) {
        if (relation.kind == ConstraintKind::fix ||
            relation.kind == ConstraintKind::coincident) {
            continue;
        }
        const auto a = indices.find(relation.first.entity);
        if (a == indices.end()) return false;
        switch (relation.kind) {
        case ConstraintKind::horizontal:
            connect(a->second, axis, 0);
            break;
        case ConstraintKind::vertical:
            connect(a->second, axis, 1);
            break;
        case ConstraintKind::parallel:
        case ConstraintKind::perpendicular: {
            const auto b = indices.find(relation.second.entity);
            if (b == indices.end()) return false;
            connect(a->second, b->second,
                    relation.kind == ConstraintKind::perpendicular ? 1 : 0);
            break;
        }
        case ConstraintKind::coincident:
        case ConstraintKind::fix:
            break;
        }
    }

    // Find an already-authored implication by parity reachability.
    // Opposite parity is a potential conflict, left for the complete
    // solver/diagnostic gate instead of mislabeling it as redundancy.
    std::vector<int> parity(axis + 1U, -1);
    std::vector<std::size_t> pending{first_node->second};
    parity[first_node->second] = 0;
    for (std::size_t i = 0; i < pending.size(); ++i) {
        const auto current = pending[i];
        for (const auto [neighbor, edge_parity] : connections[current]) {
            const int expected = parity[current] ^ edge_parity;
            if (parity[neighbor] != -1) {
                // An inconsistent authored cycle has no single valid
                // implication. Leave the full conflicting system to
                // the numeric solver instead of picking an arbitrary
                // BFS path and falsely declaring this request redundant.
                if (parity[neighbor] != expected) return false;
                continue;
            }
            parity[neighbor] = expected;
            pending.push_back(neighbor);
        }
    }
    return parity[destination] == required;
}

} // namespace

LocalRelationPreflightResult preflightLocalRelation(
    const SketchModelState& current_stage,
    ConstraintKind kind,
    ConstraintReference first,
    ConstraintReference second,
    std::span<const EntityId> stage_fixed_lines,
    const LocalSolverAcceptancePolicy& policy,
    ILocalSketchSolver& backend) {
    LocalRelationPreflightResult result;

    // Restore a fresh copy so every rejected draft preserves the original
    // cursor, semantic refs, geometry and Part-authoring state.
    auto disposable = SketchModel::restore(current_stage);
    if (!disposable) return result;

    // Fail before allocating even a provisional ID or invoking an
    // untrustworthy backend if authored semantics already imply intent.
    if (authoredRelationsImply(current_stage, kind, first, second)) {
        return result;
    }

    // Fix is a position mask, not a sixth scalar equation. A fresh
    // geometric relation whose every participant is ALREADY authored
    // fixed cannot independently control any geometry. Reject this
    // redundant proposal BEFORE the native adapter: existing older
    // equations are allowed to survive a later explicit Fix transaction.
    // Do not conflate externally stage-fixed PG-01 with authored Fix.
    if (kind != ConstraintKind::fix &&
        disposable->isFixedLine(first.entity) &&
        (!second.entity.valid() ||
         disposable->isFixedLine(second.entity))) {
        return result;
    }

    const auto provisional_id =
        disposable->addConstraint(kind, first, second);
    if (!provisional_id) return result;

    // A malicious/invalid fixed-reference set fails closed before solve.
    const auto draft = disposable->state();
    const auto prepared = prepareLocalSolveProblem(
        draft, stage_fixed_lines);
    if (!prepared) {
        result.status = LocalRelationPreflightStatus::invalid_problem;
        return result;
    }

    result.solve = evaluateLocalSolve(*prepared, policy, backend);
    // A rejected numerical proposal must not expose any authored draft
    // or provisional identity that a future caller could accidentally
    // publish. Diagnostics remain available separately in result.solve.
    if (result.solve.status ==
            LocalSolveGatewayStatus::candidate_accepted &&
        result.solve.candidate.has_value()) {
        result.status = LocalRelationPreflightStatus::candidate_accepted;
        result.provisional_constraint_id = *provisional_id;
        result.proposed_authored = std::move(draft);
    } else {
        result.status = LocalRelationPreflightStatus::solver_rejected;
    }
    // No DocumentSession command, no model write and no revision/lease
    // capture occurs here. A real writer MUST solve the authoritative
    // revision again within a single guarded Part transaction.
    return result;
}

} // namespace simplesolid2::sketch
