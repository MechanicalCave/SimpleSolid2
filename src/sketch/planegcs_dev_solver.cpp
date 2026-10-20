#include <simplesolid2/sketch/planegcs_dev_solver.hpp>
#include "src/wrapper.h" // original externally provisioned PlaneGCS 0.8.0

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace simplesolid2::sketch {
namespace {
struct NativeLine { int start{}, end{}, line{}; };
bool finite(Point2 p) { return std::isfinite(p.u) && std::isfinite(p.v); }
bool validLine(const LocalSolveLineInput& l) {
    const double d = std::hypot(l.end.u-l.start.u, l.end.v-l.start.v);
    return l.id.valid() && finite(l.start) && finite(l.end) &&
        std::isfinite(d) && d > 0.0;
}
std::optional<int> endpoint(const NativeLine& line, ConstraintElement e) {
    if (e == ConstraintElement::line_start) return line.start;
    if (e == ConstraintElement::line_end) return line.end;
    return std::nullopt;
}
LocalBackendSolveResult failed() {
    return {LocalBackendStatus::failed, std::nullopt, {},
        {{LocalBackendDiagnosticKind::backend_failure, std::nullopt}},
        std::nullopt};
}
} // namespace

LocalBackendSolveResult PlaneGcsDevSolver::solve(
    const LocalSolveProblem& problem) {
    LocalBackendSolveResult out;
    try {
        std::map<EntityId, const LocalSolveLineInput*> inputs;
        for (const auto& line : problem.lines) {
            if (!validLine(line) || !inputs.emplace(line.id, &line).second) {
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
        }
        std::set<EntityId> fixed;
        for (const auto id : problem.fixed_lines) {
            if (!id.valid() || !fixed.insert(id).second ||
                !inputs.contains(id)) {
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
        }
        for (const auto& line : problem.lines) {
            if (line.fixed != fixed.contains(line.id)) {
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
        }
        std::set<ConstraintId> equations;
        for (const auto& relation : problem.equations) {
            if (!relation.id.valid() ||
                !equations.insert(relation.id).second ||
                !inputs.contains(relation.first.entity) ||
                relation.kind == ConstraintKind::fix) {
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
            const bool binary = relation.kind == ConstraintKind::coincident ||
                relation.kind == ConstraintKind::parallel ||
                relation.kind == ConstraintKind::perpendicular;
            if (binary && !inputs.contains(relation.second.entity)) {
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
            const auto validEnd = [](ConstraintElement e) {
                return e == ConstraintElement::line_start ||
                    e == ConstraintElement::line_end;
            };
            if (relation.kind == ConstraintKind::coincident) {
                if (!validEnd(relation.first.element) ||
                    !validEnd(relation.second.element)) {
                    out.status = LocalBackendStatus::unsupported;
                    return out;
                }
            } else if (relation.first.element != ConstraintElement::line ||
                       (binary &&
                        relation.second.element != ConstraintElement::line)) {
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
        }

        SketchSolver native;
        std::map<EntityId, NativeLine> handles;
        for (const auto& line : problem.lines) {
            const int a = native.add_point(line.start.u, line.start.v);
            const int b = native.add_point(line.end.u, line.end.v);
            const int segment = native.add_line(a, b);
            handles.emplace(line.id, NativeLine{a,b,segment});
            if (line.fixed) {
                for (int id : {a,b}) {
                    const auto [x,y] = native.get_point_param_ids(id);
                    native.set_param_fixed(x,true);
                    native.set_param_fixed(y,true);
                }
            }
        }
        std::map<int,ConstraintId> tagToId;
        for (const auto& relation : problem.equations) {
            const auto& a = handles.at(relation.first.entity);
            int tag = 0;
            switch (relation.kind) {
            case ConstraintKind::horizontal:
                tag = native.horizontal_line(a.line); break;
            case ConstraintKind::vertical:
                tag = native.vertical_line(a.line); break;
            case ConstraintKind::parallel:
                tag = native.parallel(
                    a.line, handles.at(relation.second.entity).line); break;
            case ConstraintKind::perpendicular:
                tag = native.perpendicular(
                    a.line, handles.at(relation.second.entity).line); break;
            case ConstraintKind::coincident: {
                const auto& b = handles.at(relation.second.entity);
                const auto p = endpoint(a, relation.first.element);
                const auto q = endpoint(b, relation.second.element);
                if (!p || !q) {
                    out.status = LocalBackendStatus::unsupported;
                    return out;
                }
                tag = native.coincident(*p,*q);
                break;
            }
            case ConstraintKind::fix:  // Fix is a mask, never an equation.
                out.status = LocalBackendStatus::unsupported;
                return out;
            }
            if (tag <= 0 || !tagToId.emplace(tag,relation.id).second) {
                return failed();
            }
        }
        const auto diagnosis = native.diagnose();
        if (diagnosis.dof >= 0) {
            out.degrees_of_freedom =
                static_cast<std::size_t>(diagnosis.dof);
        }
        std::set<ConstraintId> diagnosed;
        const auto append = [&](const std::vector<int>& tags,
                                LocalBackendDiagnosticKind kind) {
            for (const int tag : tags) {
                const auto it = tagToId.find(tag);
                if (it == tagToId.end()) return false;

                // Upstream diagnose() may put a constraint in both its
                // conflict and redundancy categories. Conflict must win:
                // failing generically would discard the exact semantic
                // ConstraintId that the Sketch UI needs to diagnose it.
                // Treat a repeated lower-priority redundancy as already
                // reported; never invent or duplicate an SS2 diagnostic.
                if (!diagnosed.insert(it->second).second) {
                    if (kind == LocalBackendDiagnosticKind::redundant_constraint) {
                        continue;
                    }
                    return false;
                }
                out.diagnostics.push_back({kind,it->second});
                if (kind == LocalBackendDiagnosticKind::conflicting_constraint) {
                    out.conflicting_constraints.push_back(it->second);
                }
            }
            return true;
        };
        if (!append(diagnosis.conflicting,
                    LocalBackendDiagnosticKind::conflicting_constraint) ||
            !append(diagnosis.redundant,
                    LocalBackendDiagnosticKind::redundant_constraint) ||
            !append(diagnosis.partially_redundant,
                    LocalBackendDiagnosticKind::redundant_constraint)) {
            return failed();
        }
        if (!out.conflicting_constraints.empty()) {
            out.diagnostics.erase(std::remove_if(
                out.diagnostics.begin(),out.diagnostics.end(),
                [](const LocalBackendDiagnostic& d) {
                    return d.kind !=
                        LocalBackendDiagnosticKind::conflicting_constraint;
                }),out.diagnostics.end());
            out.status = LocalBackendStatus::conflict;
            return out;
        }
        const auto status = native.solve();
        if (status != GCS::Success && status != GCS::Converged) {
            out.status = LocalBackendStatus::failed;
            out.diagnostics = {
                {LocalBackendDiagnosticKind::no_convergence,std::nullopt}};
            return out;
        }
        auto candidate = problem.authored;
        for (auto& line : candidate.lines) {
            const auto it = handles.find(line.id);
            const auto input = inputs.find(line.id);
            if (it == handles.end() || input == inputs.end()) {
                return failed();
            }
            const auto a = native.get_point(it->second.start);
            const auto b = native.get_point(it->second.end);
            const Point2 start{a.first,a.second},end{b.first,b.second};
            const double length =
                std::hypot(end.u-start.u,end.v-start.v);
            if (!finite(start) || !finite(end) ||
                !std::isfinite(length) || length <= 0.0) return failed();
            if (input->second->fixed &&
                (start != input->second->start ||
                 end != input->second->end)) return failed();
            line.start = start;
            line.end = end;
        }
        out.candidate = std::move(candidate);
        out.status = LocalBackendStatus::converged;
        // The SS2 gateway must independently check residuals, Fix/PG-01,
        // semantic identity and exact Coincident closure before acceptance.
        return out;
    } catch (...) {
        return failed();
    }
}
} // namespace simplesolid2::sketch
