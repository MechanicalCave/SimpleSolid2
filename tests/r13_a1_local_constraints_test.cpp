#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>
#include <simplesolid2/sketch/local_solver.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <array>
#include <limits>

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

    std::cout << "R13-A1 Horizontal authored-state test PASS\n";
}
