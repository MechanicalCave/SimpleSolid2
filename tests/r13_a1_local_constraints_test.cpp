#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>
#include <simplesolid2/sketch/local_solver.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <array>
#include <limits>
#include <stdexcept>

namespace sk = simplesolid2::sketch;

namespace {
void check(bool condition, const char* expression, int line) {
    if (!condition) {
        std::cerr << "R13-A1 Horizontal CHECK failed at " << line
                  << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(x) check(static_cast<bool>(x), #x, __LINE__)
} // namespace

int main() {
    CHECK(!sk::ConstraintId::parse("0"));
    CHECK(!sk::ConstraintId::parse("01"));
    CHECK(!sk::ConstraintId::parse("-1"));
    CHECK(!sk::ConstraintId::parse("18446744073709551616"));

    sk::SketchModel model;
    const auto line = model.addLine({1.0, 2.0}, {5.0, 4.0});
    const auto seed = model.state();
    CHECK(seed.constraints.empty());
    CHECK(model.constraintCount() == 0U);

    // Invalid and duplicate requests must not consume an ID or mutate state.
    const auto cursor0 = model.constraintIdCursor();
    CHECK(!model.addHorizontal({}));
    CHECK(model.constraintIdCursor() == cursor0);
    const auto id = model.addHorizontal(line);
    CHECK(id && id->valid() && id->serialized() == "1");
    CHECK(sk::ConstraintId::parse(id->serialized()) == id);
    CHECK(model.constraintCount() == 1U);
    CHECK(model.findConstraint(*id) != nullptr);
    CHECK(!model.addHorizontal(line));
    CHECK(model.constraintCount() == 1U);
    CHECK(model.state().constraints.front().kind == sk::ConstraintKind::horizontal);
    CHECK(model.state().constraints.front().first.entity == line);
    CHECK(model.state().constraints.front().first.element == sk::ConstraintElement::line);
    CHECK(model.state().constraints.front().second == sk::ConstraintReference{});
    CHECK(model.state().next_constraint_id.serialized() == "2");

    // Creating a relation is authored design intent, not a geometry rewrite.
    CHECK(model.findLine(line)->start() == seed.lines.front().start);
    CHECK(model.findLine(line)->end() == seed.lines.front().end);
    CHECK(model.state() != seed);

    // State copy / restore carries exact IDs and semantics; cursor is monotonic.
    const auto snapshot = model.state();
    auto restored = sk::SketchModel::restore(snapshot);
    CHECK(restored.has_value());
    CHECK(restored->state() == snapshot);
    CHECK(*restored == model);
    CHECK(restored->eraseConstraint(*id));
    CHECK(!restored->eraseConstraint(*id));
    CHECK(restored->findConstraint(*id) == nullptr);
    CHECK(restored->constraintIdCursor().serialized() == "2");
    const auto id2 = restored->addHorizontal(line);
    CHECK(id2 && id2->serialized() == "2");
    CHECK(model.findConstraint(*id) != nullptr);

    // Deleted geometry cascades local semantic dependants without reusing IDs.
    CHECK(model.erase(line));
    CHECK(model.constraintCount() == 0U);
    CHECK(model.findConstraint(*id) == nullptr);
    CHECK(model.constraintIdCursor().serialized() == "2");
    const auto newLine = model.addLine({0.0, 0.0}, {3.0, 1.0});
    const auto newConstraint = model.addHorizontal(newLine);
    CHECK(newConstraint && newConstraint->serialized() == "2");

    // Fail-closed restoration: missing target, duplicate ID, bad role, bad cursor.
    auto broken = snapshot;
    broken.constraints.front().first.entity = *sk::EntityId::parse("9999");
    CHECK(!sk::SketchModel::restore(broken));
    broken = snapshot;
    broken.constraints.push_back(broken.constraints.front());
    CHECK(!sk::SketchModel::restore(broken));
    broken = snapshot;
    broken.constraints.front().first.element = sk::ConstraintElement::line_start;
    CHECK(!sk::SketchModel::restore(broken));
    broken = snapshot;
    broken.next_constraint_id = *sk::ConstraintIdCursor::parse("1");
    CHECK(!sk::SketchModel::restore(broken));

    // Native DocumentSession: one atomic command, revision guard and exact Undo/Redo.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto lineMade = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {2.0, 3.0}, {7.0, 5.0}});
        CHECK(lineMade.ok() && lineMade.entity_id);
        const auto rev = session.document().revision();
        const auto historyBefore = session.undoDepth();
        const auto created = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, rev, *lineMade.entity_id});
        CHECK(created.ok() && created.changed && created.constraint_id);
        CHECK(created.constraint_id->serialized() == "1");
        CHECK(session.undoDepth() == historyBefore + 1U);
        CHECK(session.needsSave());
        // Strict v20 native store does not yet carry authored constraints.
        // Save must fail closed; a future A1 successor schema will enable it.
        part::PartDocumentStore store;
        const auto blockedSave = store.createNew(
            std::filesystem::temp_directory_path() /
                "r13a1_unpersistable_constraints.ss2part",
            session.document());
        CHECK(!blockedSave.ok());
        CHECK(blockedSave.diagnostic.code == part::PartStoreErrorCode::unsupported_schema);
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .findConstraint(*created.constraint_id) != nullptr);

        const auto duplicate = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *lineMade.entity_id});
        CHECK(!duplicate.ok() && !duplicate.changed);
        CHECK(duplicate.diagnostic.code == app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.undoDepth() == historyBefore + 1U);

        const auto stale = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, rev, *lineMade.entity_id});
        CHECK(!stale.ok());
        CHECK(stale.diagnostic.code == app::DocumentSessionErrorCode::revision_diverged);

        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 0U);
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .findConstraint(*created.constraint_id) != nullptr);

        CHECK(session.undo().changed);
        // A new branch after Undo must not recycle even a retired local ConstraintId.
        const auto forked = session.execute(app::AddHorizontalSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *lineMade.entity_id});
        CHECK(forked.ok() && forked.changed && forked.constraint_id);
        CHECK(forked.constraint_id->serialized() == "2");
        CHECK(!session.canRedo());

        const auto deleted = session.execute(app::EraseSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *forked.constraint_id});
        CHECK(deleted.ok() && deleted.changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 0U);
        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .findConstraint(*forked.constraint_id) != nullptr);
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 0U);
    }

    // A1.1 has no accepted solver yet: an authored Horizontal on a
    // geometrically closed rectangle MUST NOT publish seed geometry to
    // Profile/Kernel as if it had passed a constraint solve.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto rectangle = session.execute(
            app::AddSketchRectangleCommand{
                *made.sketch_id, session.document().revision(),
                {0.0, 0.0}, {10.0, 8.0}, sk::EntityRole::regular, false});
        CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
        const auto* sketchBefore = session.document().findSketch(*made.sketch_id);
        CHECK(sketchBefore != nullptr);
        const auto regions = sk::analyzeRegions(sketchBefore->model);
        CHECK(regions.complete() && regions.regions.size() == 1U);
        const auto intent = part::makeProfileRegionIntent(regions.regions.front());
        CHECK(intent.has_value());
        const auto profile = session.execute(
            app::CreateProfileCommand{
                *made.sketch_id, session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id);
        CHECK(session.document().evaluateProfile(*profile.profile_id)->valid());
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).ok());

        const auto linked = session.execute(
            app::AddHorizontalSketchConstraintCommand{
                *made.sketch_id, session.document().revision(),
                rectangle.entity_ids.front()});
        CHECK(linked.ok() && linked.changed && linked.constraint_id);
        CHECK(!session.document().evaluateProfile(*profile.profile_id).has_value());
        const auto unresolved = part::resolveKernelProfileInput(
            session.document(), *profile.profile_id);
        CHECK(!unresolved.ok());
        CHECK(unresolved.status == part::ProfileKernelInputStatus::unresolved_profile);
        CHECK(session.document().findProfile(*profile.profile_id) != nullptr);

        // Undo revives the previously valid Profile; Redo blocks it again
        // without mutating the authored Profile or Sketch line coordinates.
        CHECK(session.undo().changed);
        CHECK(session.document().evaluateProfile(*profile.profile_id)->valid());
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).ok());
        CHECK(session.redo().changed);
        CHECK(!session.document().evaluateProfile(*profile.profile_id).has_value());
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).status ==
            part::ProfileKernelInputStatus::unresolved_profile);
    }


    // R13-A1: explicit five-relation authored semantics (not numeric solve).
    {
        sk::SketchModel relations;
        const auto a = relations.addLine({0.0, 0.0}, {2.0, 3.0});
        const auto b = relations.addLine({4.0, 5.0}, {6.0, 7.0});
        const auto c = relations.addLine({1.0, 1.0}, {4.0, 4.0});
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        const sk::ConstraintReference la{a, E::line};
        const sk::ConstraintReference lb{b, E::line};
        const sk::ConstraintReference lc{c, E::line};
        const sk::ConstraintReference as{a, E::line_start};
        const sk::ConstraintReference ae{a, E::line_end};
        const sk::ConstraintReference bs{b, E::line_start};
        const auto horizontal = relations.addConstraint(K::horizontal, la);
        const auto vertical = relations.addConstraint(K::vertical, lc);
        const auto coincident = relations.addConstraint(K::coincident, ae, bs);
        const auto parallel = relations.addConstraint(K::parallel, la, lb);
        const auto perpendicular = relations.addConstraint(K::perpendicular, lb, lc);
        CHECK(horizontal && vertical && coincident && parallel && perpendicular);
        CHECK(horizontal->serialized() == "1");
        CHECK(vertical->serialized() == "2");
        CHECK(coincident->serialized() == "3");
        CHECK(parallel->serialized() == "4");
        CHECK(perpendicular->serialized() == "5");
        const auto beforeInvalid = relations.state();
        CHECK(!relations.addConstraint(K::horizontal, la));
        CHECK(!relations.addConstraint(K::vertical, lc)); // duplicate
        CHECK(!relations.addConstraint(K::vertical, la)); // conflicts with Horizontal(a)
        CHECK(!relations.addConstraint(K::coincident, bs, ae));
        CHECK(!relations.addConstraint(K::parallel, lb, la));
        CHECK(!relations.addConstraint(K::perpendicular, lc, lb));
        CHECK(!relations.addConstraint(K::perpendicular, la, lb)); // conflicts with Parallel(a,b)
        CHECK(!relations.addConstraint(K::parallel, lb, lc)); // conflicts with Perpendicular(b,c)
        CHECK(!relations.addConstraint(K::coincident, as, ae)); // same Line
        CHECK(!relations.addConstraint(K::coincident, as, as));
        CHECK(!relations.addConstraint(K::parallel, la, la));
        CHECK(!relations.addConstraint(K::perpendicular, la, la));
        CHECK(!relations.addConstraint(K::vertical, as));
        CHECK(!relations.addConstraint(K::horizontal, la, lb));
        CHECK(!relations.addConstraint(K::coincident, la, bs));
        CHECK(!relations.addConstraint(K::parallel, as, lb));
        CHECK(!relations.addConstraint(K::perpendicular, la, bs));
        CHECK(!relations.addConstraint(static_cast<K>(255), la));
        CHECK(!relations.addConstraint(K::vertical, { *sk::EntityId::parse("9999"), E::line }));
        CHECK(relations.state() == beforeInvalid);
        auto roundTrip = sk::SketchModel::restore(beforeInvalid);
        CHECK(roundTrip && roundTrip->state() == beforeInvalid);
        const sk::Point2 originalStart{0.0, 0.0};
        CHECK(relations.findLine(a)->start() == originalStart);
        auto invalidState = beforeInvalid;
        invalidState.constraints.push_back(invalidState.constraints[3]);
        invalidState.constraints.back().id = *sk::ConstraintId::parse("6");
        invalidState.next_constraint_id = *sk::ConstraintIdCursor::parse("7");
        invalidState.constraints.back().first = lb;
        invalidState.constraints.back().second = la;
        CHECK(!sk::SketchModel::restore(invalidState));
        invalidState = beforeInvalid;
        invalidState.constraints[2].first = la;
        CHECK(!sk::SketchModel::restore(invalidState));
        invalidState = beforeInvalid;
        invalidState.constraints[4].second.entity = *sk::EntityId::parse("9999");
        CHECK(!sk::SketchModel::restore(invalidState));
        invalidState = beforeInvalid;
        invalidState.constraints[1].kind = static_cast<K>(255);
        CHECK(!sk::SketchModel::restore(invalidState));
        CHECK(relations.erase(b));
        CHECK(relations.constraintCount() == 2U); // Horizontal(a), Vertical(a)
        CHECK(relations.findConstraint(*horizontal) != nullptr);
        CHECK(relations.findConstraint(*vertical) != nullptr);
        CHECK(relations.findConstraint(*coincident) == nullptr);
        CHECK(relations.findConstraint(*parallel) == nullptr);
        CHECK(relations.findConstraint(*perpendicular) == nullptr);
        CHECK(relations.constraintIdCursor().serialized() == "6");
        const auto newParallel = relations.addConstraint(K::parallel, la, lc);
        CHECK(newParallel && newParallel->serialized() == "6");
        CHECK(relations.state().constraints.size() == 3U);
        // Current solved geometry is not published for any of these intents.
    }

    // All five manual relation kinds are wired to the real revision-bound
    // DocumentSession transaction path and undoable as one command each.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto first = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {1.0, 2.0}, {5.0, 6.0}});
        const auto second = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {6.0, 1.0}, {8.0, 2.0}});
        const auto third = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {10.0, 2.0}, {12.0, 4.0}});
        CHECK(first.ok() && first.entity_id && second.ok() && second.entity_id);
        CHECK(third.ok() && third.entity_id);
        const sk::ConstraintReference firstLine{*first.entity_id, E::line};
        const sk::ConstraintReference secondLine{*second.entity_id, E::line};
        const sk::ConstraintReference thirdLine{*third.entity_id, E::line};
        const sk::ConstraintReference firstEnd{*first.entity_id, E::line_end};
        const sk::ConstraintReference secondStart{*second.entity_id, E::line_start};
        const auto oldRevision = session.document().revision();
        const auto add = [&](K kind, sk::ConstraintReference lhs,
                             sk::ConstraintReference rhs = {}) {
            return session.execute(app::AddSketchRelationCommand{
                *made.sketch_id, session.document().revision(), kind, lhs, rhs});
        };
        CHECK(add(K::horizontal, firstLine).changed);
        CHECK(add(K::vertical, secondLine).changed);
        const auto coincident = add(K::coincident, firstEnd, secondStart);
        CHECK(coincident.changed && coincident.constraint_id);
        CHECK(add(K::parallel, firstLine, secondLine).changed);
        CHECK(add(K::perpendicular, secondLine, thirdLine).changed);
        const auto* hosted = session.document().findSketch(*made.sketch_id);
        CHECK(hosted && hosted->model.constraintCount() == 5U);
        const auto undoDepth = session.undoDepth();
        const auto duplicate = add(K::coincident, secondStart, firstEnd);
        CHECK(!duplicate.changed && !duplicate.ok());
        CHECK(session.undoDepth() == undoDepth);
        CHECK(!add(K::vertical, firstLine).ok()); // conflicts with Horizontal
        CHECK(!add(K::perpendicular, firstLine, secondLine).ok()); // conflicts with Parallel
        CHECK(session.undoDepth() == undoDepth);
        const auto stale = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, oldRevision,
            K::parallel, firstLine, secondLine});
        CHECK(!stale.ok());
        CHECK(stale.diagnostic.code == app::DocumentSessionErrorCode::revision_diverged);
        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 4U);
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 5U);
        const auto deleted = session.execute(app::EraseSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *coincident.constraint_id});
        CHECK(deleted.ok() && deleted.changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 4U);
        CHECK(session.undo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->model.constraintCount() == 5U);
    }


    // R13-A1.2 RED: donor-aligned, pure numeric acceptance bridge.
    // It checks candidate geometry; it does not solve equations or publish Part.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel authored;
        const auto a = authored.addLine({0.0, 0.0}, {4.0, 0.0});
        const auto b = authored.addLine({4.0, 0.0}, {4.0, 3.0});
        const auto c = authored.addLine({1.0, 2.0}, {3.0, 2.0});
        CHECK(authored.addConstraint(K::horizontal, {a, E::line}));
        CHECK(authored.addConstraint(K::vertical, {b, E::line}));
        CHECK(authored.addConstraint(K::coincident,
            {a, E::line_end}, {b, E::line_start}));
        CHECK(authored.addConstraint(K::parallel,
            {a, E::line}, {c, E::line}));
        CHECK(authored.addConstraint(K::perpendicular,
            {b, E::line}, {c, E::line}));

        const std::array<sk::EntityId, 1> fixed{b};
        const auto authoredState = authored.state();
        const auto problem = sk::prepareLocalSolveProblem(authoredState, fixed);
        CHECK(problem.has_value());
        CHECK(problem->authored == authoredState);
        CHECK(problem->lines.size() == 3U);
        CHECK(problem->constraints.size() == 5U);
        CHECK(problem->fixed_lines.size() == 1U);
        CHECK(problem->fixed_lines.front() == b);
        CHECK(problem->lines[1].fixed);
        CHECK(!problem->lines[0].fixed);
        CHECK(!problem->lines[2].fixed);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-7, 1.0e-6};
        CHECK(policy.valid());
        const auto good = sk::reviewLocalSolveCandidate(
            *problem, authoredState, policy);
        CHECK(good.status == sk::LocalResidualStatus::residuals_satisfied);
        CHECK(good.constraints.size() == 5U);
        for (const auto& result : good.constraints) {
            CHECK(result.constraint_id.valid());
            CHECK(result.accepted);
            CHECK(result.residual == 0.0);
        }
        CHECK(good.constraints[0].unit == sk::LocalResidualUnit::model_distance);
        CHECK(good.constraints[1].unit == sk::LocalResidualUnit::model_distance);
        CHECK(good.constraints[2].unit == sk::LocalResidualUnit::model_distance);
        CHECK(good.constraints[3].unit == sk::LocalResidualUnit::radians);
        CHECK(good.constraints[4].unit == sk::LocalResidualUnit::radians);

        auto badGeometry = authoredState;
        badGeometry.lines[0].end.v = 0.1; // horizontal, coincident, parallel fail
        const auto bad = sk::reviewLocalSolveCandidate(
            *problem, badGeometry, policy);
        CHECK(bad.status == sk::LocalResidualStatus::residual_violation);
        CHECK(!bad.constraints[0].accepted);
        CHECK(!bad.constraints[2].accepted);
        CHECK(!bad.constraints[3].accepted);
        CHECK(authored.state() == authoredState); // no authored mutation

        auto fixedChanged = authoredState;
        fixedChanged.lines[1].start.u += 1.0e-8;
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, fixedChanged, policy).status ==
            sk::LocalResidualStatus::fixed_entity_changed);

        auto corrupt = authoredState;
        corrupt.constraints.clear();
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, corrupt, policy).status ==
            sk::LocalResidualStatus::stale_semantics);
        corrupt = authoredState;
        corrupt.lines[0].role = sk::EntityRole::construction;
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, corrupt, policy).status ==
            sk::LocalResidualStatus::stale_semantics);
        corrupt = authoredState;
        corrupt.lines[0].end.u = std::numeric_limits<double>::infinity();
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, corrupt, policy).status ==
            sk::LocalResidualStatus::invalid_candidate);

        CHECK((!sk::LocalSolverAcceptancePolicy{0.0, 1.0e-6}.valid()));
        CHECK((!sk::LocalSolverAcceptancePolicy{1.0e-7, 0.0}.valid()));
        CHECK((!sk::LocalSolverAcceptancePolicy{
            std::numeric_limits<double>::infinity(), 1.0e-6}.valid()));
        CHECK(sk::reviewLocalSolveCandidate(
            *problem, authoredState,
            sk::LocalSolverAcceptancePolicy{0.0, 1.0e-6}).status ==
            sk::LocalResidualStatus::invalid_policy);

        const std::array<sk::EntityId, 1> missing{
            *sk::EntityId::parse("9999")};
        const std::array<sk::EntityId, 2> repeated{b, b};
        CHECK(!sk::prepareLocalSolveProblem(authoredState, missing));
        CHECK(!sk::prepareLocalSolveProblem(authoredState, repeated));
        CHECK(!sk::prepareLocalSolveProblem(
            sk::SketchModelState{}, fixed));
    }

    // SS1-style backend gateway RED: do not mistake a backend's declared
    // convergence for a trustworthy solved Sketch or Part authority.
    // Scripted backend tests the boundary; it is NOT a production solver.
    {
        struct ScriptedBackend final : sk::ILocalSketchSolver {
            sk::LocalBackendStatus status{sk::LocalBackendStatus::converged};
            std::optional<sk::SketchModelState> candidate;
            std::vector<sk::ConstraintId> conflicts;
            std::vector<sk::LocalBackendDiagnostic> diagnostics;
            std::optional<std::size_t> degrees_of_freedom;
            bool throw_on_solve{false};
            unsigned calls{};

            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem&) override {
                ++calls;
                if (throw_on_solve) {
                    throw std::runtime_error{"synthetic backend failure"};
                }
                return {status, candidate, conflicts, diagnostics, degrees_of_freedom};
            }
        };

        sk::SketchModel seed;
        const auto line = seed.addLine({0.0, 0.0}, {10.0, 0.0});
        const auto relation = seed.addHorizontal(line);
        CHECK(relation);
        const auto state = seed.state();
        const auto input = sk::prepareLocalSolveProblem(state);
        CHECK(input);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-6};
        ScriptedBackend backend;
        backend.candidate = state;
        backend.degrees_of_freedom = 3U; // underconstrained is legal
        const auto accepted = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(accepted.status == sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(accepted.candidate && *accepted.candidate == state);
        CHECK(accepted.residuals.status == sk::LocalResidualStatus::residuals_satisfied);
        CHECK(accepted.degrees_of_freedom == 3U);
        CHECK(accepted.diagnostics.empty());
        CHECK(backend.calls == 1U);
        CHECK(seed.state() == state);

        backend.candidate->lines.front().end.v = 0.25;
        const auto residualRejected = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(residualRejected.status == sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!residualRejected.candidate);
        CHECK(residualRejected.residuals.status == sk::LocalResidualStatus::residual_violation);
        CHECK(!residualRejected.diagnostics.empty());
        CHECK(residualRejected.diagnostics.front().kind ==
            sk::LocalBackendDiagnosticKind::residual_violation);
        CHECK(residualRejected.diagnostics.front().constraint == relation);

        backend.candidate = state;
        backend.diagnostics = {{sk::LocalBackendDiagnosticKind::redundant_constraint, relation}};
        const auto redundancy = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(redundancy.status == sk::LocalSolveGatewayStatus::redundant_constraint);
        CHECK(!redundancy.candidate);
        CHECK(redundancy.diagnostics == backend.diagnostics);
        backend.diagnostics.clear();

        backend.diagnostics = {{sk::LocalBackendDiagnosticKind::conflicting_constraint, relation}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();
        backend.diagnostics = {{sk::LocalBackendDiagnosticKind::redundant_constraint,
                               *sk::ConstraintId::parse("9999")}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();
        backend.diagnostics = {{static_cast<sk::LocalBackendDiagnosticKind>(255), relation}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();
        backend.diagnostics = {
            {sk::LocalBackendDiagnosticKind::redundant_constraint, relation},
            {sk::LocalBackendDiagnosticKind::redundant_constraint, relation}};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.diagnostics.clear();

        backend.status = sk::LocalBackendStatus::conflict;
        backend.candidate.reset();
        backend.conflicts = {*relation};
        const auto conflict = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(conflict.status == sk::LocalSolveGatewayStatus::conflict);
        CHECK(!conflict.candidate);
        CHECK(conflict.conflicting_constraints == backend.conflicts);

        backend.conflicts = {*sk::ConstraintId::parse("9999")};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.conflicts.clear();
        backend.candidate = state;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.candidate.reset();
        backend.status = sk::LocalBackendStatus::unsupported;
        const auto unsupported = sk::evaluateLocalSolve(*input, policy, backend);
        CHECK(unsupported.status == sk::LocalSolveGatewayStatus::unsupported);
        CHECK(!unsupported.candidate);
        backend.candidate = state;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.candidate.reset();
        backend.status = sk::LocalBackendStatus::failed;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::backend_failure);

        backend.status = sk::LocalBackendStatus::converged;
        backend.candidate.reset();
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.candidate = state;
        backend.conflicts = {*relation};
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_backend_response);
        backend.conflicts.clear();

        backend.throw_on_solve = true;
        CHECK(sk::evaluateLocalSolve(*input, policy, backend).status ==
              sk::LocalSolveGatewayStatus::backend_failure);
        backend.throw_on_solve = false;
        const auto calls = backend.calls;
        CHECK(sk::evaluateLocalSolve(
            *input, sk::LocalSolverAcceptancePolicy{0.0, 1.0e-6},
            backend).status == sk::LocalSolveGatewayStatus::invalid_policy);
        CHECK(backend.calls == calls);

        auto spoofed = *input;
        spoofed.constraints.clear();
        CHECK(sk::evaluateLocalSolve(spoofed, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_problem);
        CHECK(backend.calls == calls);
        spoofed = *input;
        spoofed.equations.clear();
        CHECK(sk::evaluateLocalSolve(spoofed, policy, backend).status ==
              sk::LocalSolveGatewayStatus::invalid_problem);
        CHECK(backend.calls == calls);

        // A candidate can meet a numerical Coincident tolerance and still
        // contain an open endpoint seam. Profile may consume ONLY exact
        // evaluated equality, never a tolerance-welded fiction.
        sk::SketchModel seamModel;
        const auto s1 = seamModel.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto s2 = seamModel.addLine({2.0, 0.0}, {2.0, 4.0});
        const auto seamId = seamModel.addConstraint(
            sk::ConstraintKind::coincident,
            {s1, sk::ConstraintElement::line_end},
            {s2, sk::ConstraintElement::line_start});
        CHECK(seamId);
        const auto seamAuthor = seamModel.state();
        const auto seamProblem = sk::prepareLocalSolveProblem(seamAuthor);
        CHECK(seamProblem);
        ScriptedBackend seamBackend;
        seamBackend.candidate = seamAuthor;
        seamBackend.candidate->lines[1].start.u += 1.0e-10;
        const auto seamNear = sk::evaluateLocalSolve(
            *seamProblem, policy, seamBackend);
        CHECK(seamNear.residuals.status ==
            sk::LocalResidualStatus::residuals_satisfied);
        CHECK(seamNear.status ==
            sk::LocalSolveGatewayStatus::coincident_not_closed);
        CHECK(!seamNear.candidate);
        CHECK(seamNear.diagnostics.size() == 1U);
        CHECK(seamNear.diagnostics.front().constraint == seamId);
        CHECK(seamModel.state() == seamAuthor);

        // An exactly equal disposable candidate may cross the gateway,
        // but it still cannot publish to Part until stage/revision checks.
        seamBackend.candidate = seamAuthor;
        const auto exactlyClosed = sk::evaluateLocalSolve(
            *seamProblem, policy, seamBackend);
        CHECK(exactlyClosed.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(exactlyClosed.candidate);
    }


    // R13-A1.2 Part-owned native bridge: a solver problem must come
    // from the exact PartDocument owner/revision and current stage;
    // PG-01 projection must never fall back to stale authored seeds.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        namespace kernel = simplesolid2::kernel;
        struct RefusingProjectionProvider final : kernel::IEdgeProjectionQuery {
            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle,
                kernel::RuntimeEdgeToken) noexcept override {
                return std::nullopt;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle,
                const kernel::ScopedProjectionEdge&,
                const kernel::Frame3&) noexcept override {
                return {};
            }
        } provider;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto line = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {1.0, 2.0}, {3.0, 2.0}});
        CHECK(line.ok() && line.entity_id);
        const auto relation = session.execute(
            app::AddHorizontalSketchConstraintCommand{
                *made.sketch_id, session.document().revision(), *line.entity_id});
        CHECK(relation.ok() && relation.constraint_id);

        part::PartEvaluation prefix;
        prefix.source_document_id = session.document().documentId();
        prefix.source_revision = session.document().revision();
        const auto input = part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider);
        CHECK(input && input->constraints.size() == 1U);
        CHECK(input->authored == session.document().findSketch(*made.sketch_id)->model.state());
        CHECK(input->fixed_lines.empty()); // no projected bindings on this Sketch

        auto invalidOwner = prefix;
        invalidOwner.source_document_id = core::DocumentId::generate();
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, invalidOwner, provider));
        auto invalidRevision = prefix;
        invalidRevision.source_revision = {};
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, invalidRevision, provider));
        invalidOwner = prefix;
        invalidOwner.source_document_id.reset();
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, invalidOwner, provider));

        // Subsequent real Part mutation retires the previous prefix.
        const auto added = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {10.0, 1.0}, {12.0, 1.0}});
        CHECK(added.ok());
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider));
        prefix.source_revision = session.document().revision();
        const auto updated = part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider);
        CHECK(updated && updated->lines.size() == 2U);
        CHECK(updated->constraints.size() == 1U);
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), sk::SketchId::generate(), prefix, provider));

        // Manual Fix needs NO projected edge or upstream Body at all.
        // The same production Part bridge must supply a frozen Line.
        const auto fix = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, session.document().revision(),
            sk::ConstraintKind::fix,
            {*line.entity_id, sk::ConstraintElement::line}, {}});
        CHECK(fix.ok() && fix.constraint_id);
        CHECK(!part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider));
        prefix.source_revision = session.document().revision();
        const auto withFix = part::prepareStageLocalSolveProblem(
            session.document(), *made.sketch_id, prefix, provider);
        CHECK(withFix && withFix->fixed_lines.size() == 1U);
        CHECK(withFix->fixed_lines.front() == *line.entity_id);
        CHECK(withFix->lines.front().fixed);
        CHECK(!withFix->lines.back().fixed);
        CHECK(withFix->constraints.size() == 2U);
        CHECK(withFix->constraints.back().kind == sk::ConstraintKind::fix);
        CHECK(withFix->equations.size() == 1U);
        CHECK(withFix->equations.front().kind == sk::ConstraintKind::horizontal);
    }

    // Owner-approved sixth relation: Fix(Line) locks both endpoints in the
    // authored Sketch. No projected Body, implicit origin anchor or solver
    // is needed to ground ordinary sketch lines.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel model;
        const auto locked = model.addLine({1.0, 3.0}, {5.0, 7.0});
        const auto freeLine = model.addLine({0.0, 1.0}, {8.0, 2.0});
        const auto untouched = model.state();
        CHECK(!model.isFixedLine(locked));
        const auto fix = model.addConstraint(K::fix, {locked, E::line});
        CHECK(fix && fix->serialized() == "1");
        CHECK(model.isFixedLine(locked));
        CHECK(!model.isFixedLine(freeLine));
        const auto frozen = model.state();
        CHECK(frozen.constraints.size() == 1U);
        CHECK(frozen.constraints.front().kind == K::fix);
        CHECK(frozen.lines == untouched.lines);
        CHECK(!model.addConstraint(K::fix, {locked, E::line}));
        CHECK(!model.addConstraint(K::fix, {locked, E::line_start}));
        CHECK(!model.addConstraint(K::fix, {locked, E::line_end}));
        CHECK(!model.addConstraint(K::fix, {locked, E::line},
                                   {freeLine, E::line}));
        CHECK(model.state() == frozen); // no duplicate ID consumed
        CHECK(!model.updateLine(locked, {8.0, 9.0}, {5.0, 7.0}));
        CHECK(!model.updateLine(locked, {1.0, 3.0}, {5.0, 7.0}));
        CHECK(model.findLine(locked)->start() == frozen.lines[0].start);
        CHECK(model.findLine(locked)->end() == frozen.lines[0].end);
        CHECK(model.updateLine(freeLine, {0.0, 2.0}, {8.0, 3.0}));

        const auto solvedInput = sk::prepareLocalSolveProblem(model.state());
        CHECK(solvedInput && solvedInput->lines.size() == 2U);
        CHECK(solvedInput->constraints.size() == 1U);
        CHECK(solvedInput->equations.empty()); // Fix is not a solver equation
        CHECK(solvedInput->fixed_lines.size() == 1U);
        CHECK(solvedInput->fixed_lines.front() == locked);
        CHECK(solvedInput->lines.front().fixed);
        CHECK(!solvedInput->lines.back().fixed);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-7, 1.0e-6};
        CHECK(sk::reviewLocalSolveCandidate(
            *solvedInput, model.state(), policy).status ==
            sk::LocalResidualStatus::residuals_satisfied);
        auto movedFixed = model.state();
        movedFixed.lines.front().start.u += 1.0e-9;
        CHECK(sk::reviewLocalSolveCandidate(
            *solvedInput, movedFixed, policy).status ==
            sk::LocalResidualStatus::fixed_entity_changed);
        auto restored = sk::SketchModel::restore(model.state());
        CHECK(restored && restored->isFixedLine(locked));
        CHECK(!restored->updateLine(locked, {7.0, 7.0}, {9.0, 9.0}));
        CHECK(restored->eraseConstraint(*fix));
        CHECK(!restored->isFixedLine(locked));
        CHECK(restored->updateLine(locked, {7.0, 7.0}, {9.0, 9.0}));
        CHECK(model.erase(locked)); // dependent Fix is removed with line
        CHECK(model.constraintCount() == 0U);
        CHECK(!model.isFixedLine(locked));
    }

    // Contradictions between immutable Fix coordinates and new geometric
    // intent must reject before allocating an ID or calling a solver.
    // Restore also rejects malformed/impossible authored combinations.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel blocked;
        const auto diagonal = blocked.addLine({0.0, 0.0}, {3.0, 4.0});
        const auto parallel = blocked.addLine({1.0, 1.0}, {7.0, 8.0});
        const auto offset = blocked.addLine({0.0, 1.0}, {3.0, 5.0});
        const auto af = blocked.addConstraint(K::fix, {diagonal, E::line});
        CHECK(af);
        const auto preserved = blocked.state();
        CHECK(!blocked.addConstraint(K::horizontal, {diagonal, E::line}));
        CHECK(!blocked.addConstraint(K::vertical, {diagonal, E::line}));
        CHECK(blocked.state() == preserved);
        CHECK(blocked.constraintIdCursor().serialized() == "2");
        CHECK(blocked.addConstraint(K::fix, {parallel, E::line}));
        const auto twoFixed = blocked.state();
        CHECK(!blocked.addConstraint(
            K::parallel, {diagonal, E::line}, {parallel, E::line}));
        CHECK(!blocked.addConstraint(
            K::perpendicular, {diagonal, E::line}, {parallel, E::line}));
        CHECK(!blocked.addConstraint(
            K::coincident, {diagonal, E::line_start}, {parallel, E::line_start}));
        CHECK(blocked.state() == twoFixed);
        auto corrupt = twoFixed;
        corrupt.constraints.push_back(sk::SketchConstraintState{
            *sk::ConstraintId::parse("3"), K::horizontal,
            {diagonal, E::line}, {}});
        corrupt.next_constraint_id = *sk::ConstraintIdCursor::parse("4");
        CHECK(!sk::SketchModel::restore(corrupt));
        corrupt = twoFixed;
        corrupt.constraints.push_back(sk::SketchConstraintState{
            *sk::ConstraintId::parse("3"), K::parallel,
            {diagonal, E::line}, {parallel, E::line}});
        corrupt.next_constraint_id = *sk::ConstraintIdCursor::parse("4");
        CHECK(!sk::SketchModel::restore(corrupt));
        CHECK(!blocked.isFixedLine(offset));
        const auto earlierIntent = blocked.addConstraint(
            K::horizontal, {offset, E::line});
        CHECK(earlierIntent); // free Line can be solved later
        const auto beforeImpossibleFix = blocked.state();
        CHECK(!blocked.addConstraint(K::fix, {offset, E::line}));
        CHECK(blocked.state() == beforeImpossibleFix);
    }

    // Do not confuse a genuinely compatible set of fixed references
    // with a contradiction; the backend still owns DoF/redundancy.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel model;
        const auto horizontal = model.addLine({0.0, 0.0}, {4.0, 0.0});
        const auto vertical = model.addLine({4.0, 0.0}, {4.0, 5.0});
        CHECK(model.addConstraint(K::fix, {horizontal, E::line}));
        CHECK(model.addConstraint(K::fix, {vertical, E::line}));
        CHECK(model.addConstraint(K::horizontal, {horizontal, E::line}));
        CHECK(model.addConstraint(K::vertical, {vertical, E::line}));
        CHECK(model.addConstraint(K::coincident,
            {horizontal, E::line_end}, {vertical, E::line_start}));
        CHECK(model.addConstraint(K::perpendicular,
            {horizontal, E::line}, {vertical, E::line}));
        const auto current = model.state();
        const auto restored = sk::SketchModel::restore(current);
        CHECK(restored && restored->state() == current);
        const auto prepared = sk::prepareLocalSolveProblem(current);
        CHECK(prepared && prepared->fixed_lines.size() == 2U);
        CHECK(prepared->lines[0].fixed && prepared->lines[1].fixed);
        const sk::LocalSolverAcceptancePolicy policy{1.0e-8, 1.0e-6};
        const auto checked = sk::reviewLocalSolveCandidate(*prepared, current, policy);
        CHECK(checked.status == sk::LocalResidualStatus::residuals_satisfied);
    }

    // Fix is an ordinary undoable revision-bound DocumentSession relation;
    // attempts to mutate it must fail atomically, without dirty/history
    // changes, and become possible only after explicit Unfix.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto line = session.execute(
            app::AddSketchLineCommand{*made.sketch_id, {2.0, 3.0}, {7.0, 8.0}});
        CHECK(line.ok() && line.entity_id);
        const auto initialRevision = session.document().revision();
        const auto initialDepth = session.undoDepth();
        const auto fixed = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, initialRevision, K::fix,
            {*line.entity_id, E::line}, {}});
        CHECK(fixed.ok() && fixed.changed && fixed.constraint_id);
        CHECK(session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.undoDepth() == initialDepth + 1U);
        CHECK(session.needsSave());
        const auto depth = session.undoDepth();
        const auto afterFixRevision = session.document().revision();
        const auto contradictory = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, afterFixRevision, K::horizontal,
            {*line.entity_id, E::line}, {}});
        CHECK(!contradictory.ok() && !contradictory.changed);
        CHECK(contradictory.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.document().revision() == afterFixRevision);
        CHECK(session.undoDepth() == depth);
        const auto attempted = session.execute(app::UpdateSketchGeometryCommand{
            *made.sketch_id, afterFixRevision,
            {{*line.entity_id, {2.0, 3.0}, {10.0, 8.0}}}, {}, {}});
        CHECK(!attempted.ok() && !attempted.changed);
        CHECK(attempted.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.document().revision() == afterFixRevision);
        CHECK(session.undoDepth() == depth);
        const auto other = session.execute(app::AddSketchLineCommand{
            *made.sketch_id, {3.0, 0.0}, {3.0, 12.0}});
        CHECK(other.ok() && other.entity_id);
        const auto fixedDepth = session.undoDepth();
        const auto fixedRevision = session.document().revision();
        const auto trimBlocked = session.execute(app::TrimSketchCommand{
            *made.sketch_id, fixedRevision, *line.entity_id,
            {*other.entity_id}, {4.0, 5.0}});
        CHECK(!trimBlocked.changed && !trimBlocked.ok());
        CHECK(trimBlocked.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        const auto extendBlocked = session.execute(app::ExtendSketchCommand{
            *made.sketch_id, fixedRevision, *line.entity_id,
            {*other.entity_id}, sk::StructuralEndpointRole::end});
        CHECK(!extendBlocked.changed);
        CHECK(extendBlocked.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        const auto extendBothBlocked =
            session.execute(app::ExtendBothSketchLinesCommand{
                *made.sketch_id, fixedRevision,
                *line.entity_id, *other.entity_id});
        CHECK(!extendBothBlocked.changed);
        CHECK(extendBothBlocked.diagnostic.code ==
            app::DocumentSessionErrorCode::invalid_command);
        CHECK(session.undoDepth() == fixedDepth);
        CHECK(session.document().revision() == fixedRevision);
        CHECK(!session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, initialRevision, K::fix,
            {*line.entity_id, E::line}, {}}).ok());
        CHECK(session.undo().changed); // retire unrelated second Line first
        CHECK(session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.undo().changed); // then undo Fix itself
        CHECK(!session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.redo().changed);
        CHECK(session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.execute(app::EraseSketchConstraintCommand{
            *made.sketch_id, session.document().revision(), *fixed.constraint_id}).changed);
        CHECK(!session.document().findSketch(*made.sketch_id)->
            model.isFixedLine(*line.entity_id));
        CHECK(session.execute(app::UpdateSketchGeometryCommand{
            *made.sketch_id, session.document().revision(),
            {{*line.entity_id, {2.0, 3.0}, {10.0, 8.0}}}, {}, {}}).changed);
    }

    std::cout << "R13-A1 Horizontal authored-state test PASS\n";
}
