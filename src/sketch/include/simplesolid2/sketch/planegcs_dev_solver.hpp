#pragma once
#include <simplesolid2/sketch/local_solver.hpp>

namespace simplesolid2::sketch {
// Original external PlaneGCS 0.8.0, DEV opt-in only. No Part publication.
class PlaneGcsDevSolver final : public ILocalSketchSolver {
public:
    [[nodiscard]] LocalBackendSolveResult solve(
        const LocalSolveProblem& problem) override;
};
} // namespace simplesolid2::sketch
