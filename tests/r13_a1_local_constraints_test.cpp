#if defined(SS2_R13_A1_OCCT_EVIDENCE)
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <numbers>
#endif

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>
#include <simplesolid2/sketch/local_solver.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <array>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <set>
#include <utility>

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

#if !defined(SS2_R13_A1_OCCT_EVIDENCE)
int main() {
    // Native MSVC RED: a caller-owned solver may drive the actual Part
    // feature scheduler, without changing the legacy evaluatePart API.
    CHECK(&simplesolid2::part::evaluatePartWithLocalSolve != nullptr);
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

        // RED A1.3: a REAL Part-owned same-stage solver gateway must let
        // the EXISTING region-intent/Kernel Profile resolver consume its
        // accepted, derived geometry; no second Profile implementation.
        struct EmptyProjectionQuery final : simplesolid2::kernel::IEdgeProjectionQuery {
            std::optional<simplesolid2::kernel::ScopedProjectionEdge>
            bindEdgeToBody(
                simplesolid2::kernel::RuntimeSolidHandle,
                simplesolid2::kernel::RuntimeEdgeToken) noexcept override {
                return std::nullopt;
            }
            simplesolid2::kernel::EdgeProjectionResult
            projectEdgeToPlane(
                simplesolid2::kernel::RuntimeSolidHandle,
                const simplesolid2::kernel::ScopedProjectionEdge&,
                const simplesolid2::kernel::Frame3&) noexcept override {
                return {};
            }
        } query;
        struct ProfileSolver final : sk::ILocalSketchSolver {
            unsigned calls{};
            bool fail{false};
            double translation_u{};
            std::optional<app::DocumentSession*> edit_session;
            std::optional<sk::SketchId> edit_sketch;
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                if (edit_session && edit_sketch) {
                    const auto edit = (*edit_session)->execute(
                        app::AddSketchLineCommand{
                            *edit_sketch, {20.0, 20.0}, {21.0, 21.0}});
                    CHECK(edit.ok() && edit.changed);
                }
                if (fail) {
                    return {sk::LocalBackendStatus::failed,
                        std::nullopt, {}, {}, std::nullopt};
                }
                // Scripted native test double: deliberately return a
                // different, still-valid solved rectangle so that the
                // Kernel Profile proves it consumes DERIVED coordinates.
                auto geometry = problem.authored;
                for (auto& line : geometry.lines) {
                    line.start.u += translation_u;
                    line.end.u += translation_u;
                }
                return {sk::LocalBackendStatus::converged,
                    std::move(geometry), {}, {}, 1U};
            }
        } backend;
        const sk::LocalSolverAcceptancePolicy accurate{1.0e-12, 1.0e-12};
        part::PartEvaluation prefix;
        prefix.source_document_id = session.document().documentId();
        prefix.source_revision = session.document().revision();
        const auto proved = part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend);
        CHECK(proved.ok());
        CHECK(proved.input->outer.boundary.size() == 4U);
        CHECK(proved.input->holes.empty());
        CHECK(backend.calls == 1U);

        // Same durable ProfileIntent, but solver-produced coordinates
        // differ from the authored seed by 2 mm. This is the existing
        // Profile/Kernel adapter, not a second region resolver.
        backend.translation_u = 2.0;
        const auto shifted = part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend);
        CHECK(shifted.ok());
        CHECK(backend.calls == 2U);
        CHECK(shifted.input != proved.input);
        CHECK(shifted.input->outer.boundary.size() ==
            proved.input->outer.boundary.size());
        for (std::size_t i = 0; i < proved.input->outer.boundary.size(); ++i) {
            const auto& before = std::get<simplesolid2::kernel::Line2>(
                proved.input->outer.boundary[i].curve);
            const auto& after = std::get<simplesolid2::kernel::Line2>(
                shifted.input->outer.boundary[i].curve);
            CHECK(after.start.u == before.start.u + 2.0);
            CHECK(after.end.u == before.end.u + 2.0);
            CHECK(after.start.v == before.start.v);
            CHECK(after.end.v == before.end.v);
        }
        backend.translation_u = 0.0;
        CHECK(!session.document().evaluateProfile(*profile.profile_id));
        // Existing untrusted entrypoint remains strictly fenced even if a
        // caller passes its own arbitrary model that looks solved.
        const auto untrusted = sk::SketchModel::restore(
            session.document().findSketch(*made.sketch_id)->model.state());
        CHECK(untrusted);
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id,
            nullptr, nullptr, &*untrusted).status ==
            part::ProfileKernelInputStatus::unresolved_profile);
        auto foreign = prefix;
        foreign.source_document_id = core::DocumentId::generate();
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, foreign, query,
            accurate, backend).ok());
        CHECK(backend.calls == 2U);
        backend.fail = true;
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend).ok());
        CHECK(backend.calls == 3U);
        backend.fail = false;
        backend.edit_session = &session;
        backend.edit_sketch = *made.sketch_id;
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend).ok());
        CHECK(backend.calls == 4U);
        CHECK(prefix.source_revision != session.document().revision());
        CHECK(!part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix, query,
            accurate, backend).ok());
        CHECK(backend.calls == 4U);
        // First undo only the reentrant backend test edit; the next Undo
        // must still target the original Horizontal relation.
        CHECK(session.undo().changed);

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


    // R13-A1: the production Part -> accepted numerical candidate ->
    // EXISTING ProfileRegionIntent/Kernel bridge must close an explicit
    // Coincident at 1e-12 precision BEFORE region and wire conversion.
    // This test uses a scripted backend, not a native PlaneGCS solver.
    {
        namespace app = simplesolid2::application;
        namespace part = simplesolid2::part;
        namespace core = simplesolid2::core;
        namespace kernel = simplesolid2::kernel;
        struct NoProjection final : kernel::IEdgeProjectionQuery {
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
        } projection;
        struct SeamBackend final : sk::ILocalSketchSolver {
            double offset{};
            unsigned calls{};
            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                auto candidate = problem.authored;
                CHECK(candidate.lines.size() == 4U);
                candidate.lines[1].start.u += offset;
                return {sk::LocalBackendStatus::converged,
                        std::move(candidate), {}, {}, 3U};
            }
        } backend;
        app::DocumentSession session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto made = session.execute(
            app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto rectangle = session.execute(
            app::AddSketchRectangleCommand{
                *made.sketch_id, session.document().revision(),
                {0.0, 0.0}, {10.0, 8.0},
                sk::EntityRole::regular, false});
        CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
        const auto before = session.document()
            .findSketch(*made.sketch_id)->model.state();
        CHECK(before.lines.size() == 4U);
        CHECK(before.lines[0].end == before.lines[1].start);
        const auto regions = sk::analyzeRegions(
            session.document().findSketch(*made.sketch_id)->model);
        CHECK(regions.complete() && regions.regions.size() == 1U);
        const auto intent = part::makeProfileRegionIntent(
            regions.regions.front());
        CHECK(intent);
        const auto profile = session.execute(
            app::CreateProfileCommand{
                *made.sketch_id, session.document().revision(), *intent});
        CHECK(profile.ok() && profile.profile_id);
        const auto relation = session.execute(app::AddSketchRelationCommand{
            *made.sketch_id, session.document().revision(),
            sk::ConstraintKind::coincident,
            {before.lines[0].id, sk::ConstraintElement::line_end},
            {before.lines[1].id, sk::ConstraintElement::line_start}});
        CHECK(relation.ok() && relation.constraint_id);
        CHECK(!session.document().evaluateProfile(*profile.profile_id));
        CHECK(part::resolveKernelProfileInput(
            session.document(), *profile.profile_id).status ==
            part::ProfileKernelInputStatus::unresolved_profile);

        const sk::LocalSolverAcceptancePolicy precise{1.0e-12, 1.0e-12};
        part::PartEvaluation prefix;
        prefix.source_document_id = session.document().documentId();
        prefix.source_revision = session.document().revision();
        backend.offset = 5.0e-13;
        const auto stage = part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix,
            projection, precise, backend);
        CHECK(stage);
        CHECK(stage->solution.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(stage->solution.candidate);
        CHECK(stage->solution.candidate->lines[0].end ==
            stage->solution.candidate->lines[1].start);
        CHECK(stage->solution.candidate->lines[1].start ==
            before.lines[1].start);
        const auto acceptedProfile =
            part::resolveStageSolvedProfileKernelInput(
                session.document(), *profile.profile_id, prefix,
                projection, precise, backend);
        CHECK(acceptedProfile.ok());
        CHECK(acceptedProfile.input->outer.boundary.size() == 4U);
        CHECK(acceptedProfile.input->holes.empty());
        // The existing Kernel adapter must carry the EXACT same canonical
        // shared point, not reintroduce an epsilon-sized seam in the OCCT
        // wire's source curves. Use semantic provenance, not loop order.
        const kernel::Line2* first_curve = nullptr;
        const kernel::Line2* second_curve = nullptr;
        for (const auto& use : acceptedProfile.input->outer.boundary) {
            if (use.provenance.source_entity ==
                before.lines[0].id.serialized()) {
                first_curve = std::get_if<kernel::Line2>(&use.curve);
            }
            if (use.provenance.source_entity ==
                before.lines[1].id.serialized()) {
                second_curve = std::get_if<kernel::Line2>(&use.curve);
            }
        }
        CHECK(first_curve != nullptr && second_curve != nullptr);
        CHECK(first_curve->end == second_curve->start);
        CHECK(first_curve->end.u == before.lines[0].end.u);
        CHECK(first_curve->end.v == before.lines[0].end.v);
        CHECK(backend.calls == 2U);
        // The observed Profile remains authored, with stable identity.
        CHECK(session.document().findProfile(*profile.profile_id));
        CHECK(session.document().findSketch(*made.sketch_id)->model
            .state().lines == before.lines);

        // Out-of-policy local Coincident gaps must never be promoted into
        // a repaired Profile via the new staged wrapper.
        backend.offset = 5.0e-12;
        const auto refused = part::resolveStageSolvedProfileKernelInput(
            session.document(), *profile.profile_id, prefix,
            projection, precise, backend);
        CHECK(!refused.ok());
        CHECK(refused.status ==
            part::ProfileKernelInputStatus::unresolved_profile);
        CHECK(backend.calls == 3U);
        CHECK(session.document().findProfile(*profile.profile_id));
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

        // Explicit Coincident uses a caller-owned 1e-12 model-distance
        // policy. A numerically converged seam must have one EXACT shared
        // evaluated vertex, not two almost-equal Profile coordinates.
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
        const sk::LocalSolverAcceptancePolicy precise{1.0e-12, 1.0e-12};
        ScriptedBackend seamBackend;
        seamBackend.candidate = seamAuthor;
        seamBackend.candidate->lines[1].start.u += 5.0e-13;
        const auto seamNear = sk::evaluateLocalSolve(
            *seamProblem, precise, seamBackend);
        CHECK(seamNear.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(seamNear.candidate);
        CHECK(seamNear.candidate->lines[0].end ==
            seamNear.candidate->lines[1].start);
        CHECK(seamNear.candidate->lines[0].end == seamAuthor.lines[0].end);
        CHECK(seamModel.state() == seamAuthor); // authored never welded

        // Real gaps outside the caller-owned precision remain open.
        seamBackend.candidate = seamAuthor;
        seamBackend.candidate->lines[1].start.u += 5.0e-12;
        const auto seamTooWide = sk::evaluateLocalSolve(
            *seamProblem, precise, seamBackend);
        CHECK(seamTooWide.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!seamTooWide.candidate);

        // An exactly equal disposable candidate remains valid.
        seamBackend.candidate = seamAuthor;
        const auto exactlyClosed = sk::evaluateLocalSolve(
            *seamProblem, precise, seamBackend);
        CHECK(exactlyClosed.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(exactlyClosed.candidate);

        // Fixed sources are authoritative, even when the free seed is
        // only numerically Coincident. Never move a Fix/PG-01 anchor.
        sk::SketchModel anchored;
        const auto anchor = anchored.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto free = anchored.addLine({2.0 + 5.0e-13, 0.0}, {2.0, 4.0});
        CHECK(anchored.addConstraint(sk::ConstraintKind::fix,
            {anchor, sk::ConstraintElement::line}));
        CHECK(anchored.addConstraint(sk::ConstraintKind::coincident,
            {anchor, sk::ConstraintElement::line_end},
            {free, sk::ConstraintElement::line_start}));
        const auto anchoredState = anchored.state();
        const auto anchoredInput = sk::prepareLocalSolveProblem(anchoredState);
        CHECK(anchoredInput);
        ScriptedBackend anchoredBackend;
        anchoredBackend.candidate = anchoredState;
        const auto anchoredResult = sk::evaluateLocalSolve(
            *anchoredInput, precise, anchoredBackend);
        CHECK(anchoredResult.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(anchoredResult.candidate);
        CHECK(anchoredResult.candidate->lines[0] == anchoredState.lines[0]);
        CHECK(anchoredResult.candidate->lines[1].start ==
            anchoredState.lines[0].end);
        CHECK(anchored.state() == anchoredState);

        // A multi-relation chain cannot accumulate pairwise tolerances
        // into a larger displacement from the canonical vertex.
        sk::SketchModel chained;
        const auto ca = chained.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto cb = chained.addLine({2.0 + 8.0e-13, 0.0}, {3.0, 1.0});
        const auto cc = chained.addLine({2.0 + 1.6e-12, 0.0}, {4.0, 2.0});
        CHECK(chained.addConstraint(sk::ConstraintKind::coincident,
            {ca, sk::ConstraintElement::line_end},
            {cb, sk::ConstraintElement::line_start}));
        CHECK(chained.addConstraint(sk::ConstraintKind::coincident,
            {cb, sk::ConstraintElement::line_start},
            {cc, sk::ConstraintElement::line_start}));
        const auto chainedState = chained.state();
        const auto chainedInput = sk::prepareLocalSolveProblem(chainedState);
        CHECK(chainedInput);
        ScriptedBackend chainedBackend;
        chainedBackend.candidate = chainedState;
        const auto chainedRejected = sk::evaluateLocalSolve(
            *chainedInput, precise, chainedBackend);
        CHECK(chainedRejected.status ==
            sk::LocalSolveGatewayStatus::coincident_not_closed);
        CHECK(!chainedRejected.candidate);
        CHECK(chained.state() == chainedState);

        // No authored Coincident means no implicit global gap welding,
        // even though the gap is inside the same numerical error budget.
        sk::SketchModel separate;
        const auto sep1 = separate.addLine({0.0, 0.0}, {2.0, 0.0});
        separate.addLine({2.0 + 5.0e-13, 0.0}, {3.0, 1.0});
        CHECK(separate.addHorizontal(sep1));
        const auto separateState = separate.state();
        const auto separateInput = sk::prepareLocalSolveProblem(separateState);
        CHECK(separateInput);
        ScriptedBackend separateBackend;
        separateBackend.candidate = separateState;
        const auto separateResult = sk::evaluateLocalSolve(
            *separateInput, precise, separateBackend);
        CHECK(separateResult.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(separateResult.candidate);
        CHECK(separateResult.candidate->lines[0].end !=
            separateResult.candidate->lines[1].start);

        // A local snapping decision must not break a previously accepted
        // Vertical residual: the ENTIRE candidate is checked again.
        sk::SketchModel coupled;
        const auto cs = coupled.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto ct = coupled.addLine(
            {2.0 + 5.0e-13, 0.0}, {2.0 + 1.2e-12, 4.0});
        CHECK(coupled.addConstraint(sk::ConstraintKind::coincident,
            {cs, sk::ConstraintElement::line_end},
            {ct, sk::ConstraintElement::line_start}));
        CHECK(coupled.addConstraint(sk::ConstraintKind::vertical,
            {ct, sk::ConstraintElement::line}));
        const auto coupledState = coupled.state();
        const auto coupledInput = sk::prepareLocalSolveProblem(coupledState);
        CHECK(coupledInput);
        ScriptedBackend coupledBackend;
        coupledBackend.candidate = coupledState;
        const auto coupledResult = sk::evaluateLocalSolve(
            *coupledInput, precise, coupledBackend);
        CHECK(coupledResult.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!coupledResult.candidate);
        CHECK(coupled.state() == coupledState);

        // Same authored semantic constraints in different record order
        // must produce the SAME evaluated vertex, not a history-dependent
        // sequence of pairwise 1e-12 tolerance welds.
        sk::SketchModel ordered;
        const auto oa = ordered.addLine({0.0, 0.0}, {10.0, 0.0});
        const auto ob = ordered.addLine({10.0 + 4.0e-13, 0.0}, {11.0, 1.0});
        const auto oc = ordered.addLine({10.0 + 8.0e-13, 0.0}, {12.0, 2.0});
        CHECK(ordered.addConstraint(sk::ConstraintKind::coincident,
            {oa, sk::ConstraintElement::line_end},
            {ob, sk::ConstraintElement::line_start}));
        CHECK(ordered.addConstraint(sk::ConstraintKind::coincident,
            {ob, sk::ConstraintElement::line_start},
            {oc, sk::ConstraintElement::line_start}));
        const auto orderedState = ordered.state();
        auto reversedState = orderedState;
        std::reverse(reversedState.constraints.begin(),
                     reversedState.constraints.end());
        CHECK(sk::SketchModel::restore(reversedState));
        const auto inputOrdered = sk::prepareLocalSolveProblem(orderedState);
        const auto inputReversed = sk::prepareLocalSolveProblem(reversedState);
        CHECK(inputOrdered && inputReversed);
        ScriptedBackend orderedBackend;
        ScriptedBackend reversedBackend;
        orderedBackend.candidate = orderedState;
        reversedBackend.candidate = reversedState;
        const auto solvedOrdered = sk::evaluateLocalSolve(
            *inputOrdered, precise, orderedBackend);
        const auto solvedReversed = sk::evaluateLocalSolve(
            *inputReversed, precise, reversedBackend);
        CHECK(solvedOrdered.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(solvedReversed.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(solvedOrdered.candidate && solvedReversed.candidate);
        CHECK(solvedOrdered.candidate->lines ==
            solvedReversed.candidate->lines);
        CHECK(solvedOrdered.candidate->lines[0].end ==
            solvedOrdered.candidate->lines[1].start);
        CHECK(solvedOrdered.candidate->lines[1].start ==
            solvedOrdered.candidate->lines[2].start);
        CHECK(ordered.state() == orderedState);

        // A stage may supply two independently frozen projected Lines.
        // A near-but-unequal authored Coincident cannot move EITHER
        // projected source even when both residues fit the numeric budget.
        sk::SketchModel twoProjected;
        const auto p1 = twoProjected.addLine({0.0, 0.0}, {2.0, 0.0});
        const auto p2 = twoProjected.addLine(
            {2.0 + 5.0e-13, 0.0}, {2.0 + 5.0e-13, 3.0});
        CHECK(twoProjected.addConstraint(sk::ConstraintKind::coincident,
            {p1, sk::ConstraintElement::line_end},
            {p2, sk::ConstraintElement::line_start}));
        const auto twoProjectedState = twoProjected.state();
        const std::array<sk::EntityId, 2> projectedFixed{p1, p2};
        const auto twoProjectedInput = sk::prepareLocalSolveProblem(
            twoProjectedState, projectedFixed);
        CHECK(twoProjectedInput);
        ScriptedBackend twoProjectedBackend;
        twoProjectedBackend.candidate = twoProjectedState;
        const auto incompatibleFixed = sk::evaluateLocalSolve(
            *twoProjectedInput, precise, twoProjectedBackend);
        CHECK(incompatibleFixed.status ==
            sk::LocalSolveGatewayStatus::coincident_not_closed);
        CHECK(!incompatibleFixed.candidate);
        CHECK(twoProjected.state() == twoProjectedState);

        // At 100 metres one binary64 ULP is > 1e-12 mm. Reject a
        // representably separate candidate under a strict 1e-12 budget;
        // do not pretend that absolute target is attainable at any scale.
        // A distinct, explicit caller-owned budget may accept one ULP.
        const double large = 100000.0;
        const double oneUlp = std::nextafter(
            large, std::numeric_limits<double>::infinity()) - large;
        CHECK(oneUlp > precise.distance_tolerance);
        sk::SketchModel largeModel;
        const auto largeA = largeModel.addLine(
            {large - 1.0, 0.0}, {large, 0.0});
        const auto largeB = largeModel.addLine(
            {large + oneUlp, 0.0}, {large + 1.0, 1.0});
        CHECK(largeModel.addConstraint(sk::ConstraintKind::coincident,
            {largeA, sk::ConstraintElement::line_end},
            {largeB, sk::ConstraintElement::line_start}));
        const auto largeAuthored = largeModel.state();
        const auto largeInput = sk::prepareLocalSolveProblem(largeAuthored);
        CHECK(largeInput);
        ScriptedBackend largeBackend;
        largeBackend.candidate = largeAuthored;
        const auto strictLarge = sk::evaluateLocalSolve(
            *largeInput, precise, largeBackend);
        CHECK(strictLarge.status ==
            sk::LocalSolveGatewayStatus::residual_violation);
        CHECK(!strictLarge.candidate);
        const sk::LocalSolverAcceptancePolicy explicitOneUlp{
            2.0 * oneUlp, precise.angular_tolerance_radians};
        const auto calibratedLarge = sk::evaluateLocalSolve(
            *largeInput, explicitOneUlp, largeBackend);
        CHECK(calibratedLarge.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(calibratedLarge.candidate);
        CHECK(calibratedLarge.candidate->lines[0].end ==
            calibratedLarge.candidate->lines[1].start);
        CHECK(largeModel.state() == largeAuthored);

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

        // A1.2 Part-owned, current-stage evaluation gateway: never call
        // a backend for stale/foreign Part evidence and never advertise
        // a candidate whose source revision changed DURING solve.
        struct StageBackend final : sk::ILocalSketchSolver {
            unsigned calls{};
            app::DocumentSession* mutate_session{};
            std::optional<sk::SketchId> mutate_sketch;

            sk::LocalBackendSolveResult solve(
                const sk::LocalSolveProblem& problem) override {
                ++calls;
                if (mutate_session && mutate_sketch) {
                    const auto edit = mutate_session->execute(
                        app::AddSketchLineCommand{
                            *mutate_sketch, {15.0, 1.0}, {17.0, 1.0}});
                    CHECK(edit.ok() && edit.changed);
                }
                return {sk::LocalBackendStatus::converged,
                    problem.authored, {}, {}, 2U};
            }
        } stageBackend;

        const sk::LocalSolverAcceptancePolicy tight{1.0e-12, 1.0e-12};
        const auto stageSolved = part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            tight, stageBackend);
        CHECK(stageSolved);
        CHECK(stageSolved->source_document_id ==
              session.document().documentId());
        CHECK(stageSolved->source_revision == session.document().revision());
        CHECK(stageSolved->source_sketch_id == *made.sketch_id);
        CHECK(!stageSolved->consuming_feature);
        CHECK(stageSolved->solution.status ==
            sk::LocalSolveGatewayStatus::candidate_accepted);
        CHECK(stageSolved->solution.candidate);
        CHECK(stageSolved->solution.candidate->lines.size() == 2U);
        CHECK(stageSolved->solution.candidate->lines.front() ==
            withFix->authored.lines.front());
        CHECK(stageBackend.calls == 1U);

        auto wrongStage = prefix;
        wrongStage.source_document_id = core::DocumentId::generate();
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, wrongStage, provider,
            tight, stageBackend));
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), sk::SketchId::generate(), prefix, provider,
            tight, stageBackend));
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            sk::LocalSolverAcceptancePolicy{0.0, tight.angular_tolerance_radians},
            stageBackend));
        CHECK(stageBackend.calls == 1U);

        // A real DocumentSession mutation during a reentrant adapter call
        // retires this stage even if that backend declares convergence.
        stageBackend.mutate_session = &session;
        stageBackend.mutate_sketch = *made.sketch_id;
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            tight, stageBackend));
        CHECK(stageBackend.calls == 2U);
        CHECK(session.document().revision() != prefix.source_revision);
        CHECK(!part::evaluateStageLocalSolve(
            session.document(), *made.sketch_id, prefix, provider,
            tight, stageBackend));
        CHECK(stageBackend.calls == 2U);
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
        const auto fixedOnlyReview = sk::reviewLocalSolveCandidate(
            *solvedInput, model.state(), policy);
        CHECK(fixedOnlyReview.status ==
            sk::LocalResidualStatus::residuals_satisfied);
        CHECK(fixedOnlyReview.constraints.empty()); // Fix has no equation residual
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

    // Exact fixed geometry must not become a false conflict because
    // atan2/remainder accumulates a binary64 angle-roundoff. These
    // non-axis-aligned integer directions have exact zero dot/cross.
    {
        using K = sk::ConstraintKind;
        using E = sk::ConstraintElement;
        sk::SketchModel model;
        const auto first = model.addLine({0.0, 0.0}, {-20.0, 1.0});
        const auto perpendicular = model.addLine({0.0, 0.0}, {3.0, 60.0});
        const auto parallel = model.addLine({0.0, 0.0}, {-60.0, 3.0});
        const auto nearPerpendicular = model.addLine({0.0, 0.0}, {3.0, 59.0});
        CHECK(model.addConstraint(K::fix, {first, E::line}));
        CHECK(model.addConstraint(K::fix, {perpendicular, E::line}));
        CHECK(model.addConstraint(K::fix, {parallel, E::line}));
        CHECK(model.addConstraint(K::fix, {nearPerpendicular, E::line}));
        CHECK(model.addConstraint(K::perpendicular,
            {first, E::line}, {perpendicular, E::line}));
        CHECK(model.addConstraint(K::parallel,
            {first, E::line}, {parallel, E::line}));
        const auto accepted = model.state();
        CHECK(!model.addConstraint(K::perpendicular,
            {first, E::line}, {nearPerpendicular, E::line}));
        CHECK(model.state() == accepted);
        const auto restored = sk::SketchModel::restore(accepted);
        CHECK(restored && restored->state() == accepted);
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

#else

// Real Windows kernel-native evidence: existing authored ProfileRegionIntent,
// actual Part-stage numerical acceptance and the REAL OCCT solid provider.
// A scripted backend stands in for the still-unlicensed PlaneGCS solver.
// This test does exercise the production Feature scheduler with an explicit
// TEST-ONLY backend. It does not install PlaneGCS, alter default evaluation,
// write a native file or provide a vNext persistence proof.
int main() {
    namespace app = simplesolid2::application;
    namespace part = simplesolid2::part;
    namespace core = simplesolid2::core;
    namespace kernel = simplesolid2::kernel;
    namespace occt = simplesolid2::kernel_occt;

    struct ControlledSolver final : sk::ILocalSketchSolver {
        double seam_offset{5.0e-13};
        double translated_u{};
        unsigned calls{};

        sk::LocalBackendSolveResult solve(
            const sk::LocalSolveProblem& problem) override {
            ++calls;
            auto solved = problem.authored;
            CHECK(solved.lines.size() == 4U);
            // The explicit Coincident tolerates the residual, but
            // the downstream Profile/OCCT needs ONE exact vertex.
            // The opt-in Feature scheduler must ALSO consume material
            // solver changes, not just accept the old authored seed.
            for (auto& line : solved.lines) {
                line.start.u += translated_u;
                line.end.u += translated_u;
            }
            solved.lines[1].start.u += seam_offset;
            return {sk::LocalBackendStatus::converged,
                    std::move(solved), {}, {}, 3U};
        }
    } backend;
    occt::OcctSolidModelingKernel provider;
    app::DocumentSession session{
        {}, part::PartDocument::create(core::DocumentId::generate())};
    const auto sketch = session.execute(
        app::CreatePartSketchCommand{core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);

    // All x > 0 permits both a normal OCCT Extrude and full Revolve
    // around the unmodified global Y-axis (an annular solid).
    const auto rectangle = session.execute(app::AddSketchRectangleCommand{
        *sketch.sketch_id, session.document().revision(),
        {5.0, 0.0}, {15.0, 8.0}, sk::EntityRole::regular, false});
    CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);

    const auto* original = session.document().findSketch(*sketch.sketch_id);
    CHECK(original != nullptr);
    const auto seed = original->model.state();
    CHECK(seed.lines.size() == 4U);
    CHECK(seed.lines[0].end == seed.lines[1].start);
    const auto analysis = sk::analyzeRegions(original->model);
    CHECK(analysis.complete() && analysis.regions.size() == 1U);
    const auto intent = part::makeProfileRegionIntent(analysis.regions.front());
    CHECK(intent);
    const auto profile = session.execute(app::CreateProfileCommand{
        *sketch.sketch_id, session.document().revision(), *intent});
    CHECK(profile.ok() && profile.profile_id);
    const auto coincide = session.execute(app::AddSketchRelationCommand{
        *sketch.sketch_id, session.document().revision(),
        sk::ConstraintKind::coincident,
        {seed.lines[0].id, sk::ConstraintElement::line_end},
        {seed.lines[1].id, sk::ConstraintElement::line_start}});
    CHECK(coincide.ok() && coincide.constraint_id);

    CHECK(!session.document().evaluateProfile(*profile.profile_id));
    CHECK(part::resolveKernelProfileInput(
        session.document(), *profile.profile_id).status ==
        part::ProfileKernelInputStatus::unresolved_profile);

    part::PartEvaluation current;
    current.source_document_id = session.document().documentId();
    current.source_revision = session.document().revision();
    const sk::LocalSolverAcceptancePolicy policy{1.0e-12, 1.0e-12};
    const auto solved = part::resolveStageSolvedProfileKernelInput(
        session.document(), *profile.profile_id, current,
        provider, policy, backend);
    CHECK(solved.ok() && solved.input);
    CHECK(backend.calls == 1U);
    CHECK(solved.input->outer.boundary.size() == 4U);

    // Verify genuine Profile→OCCT Face construction before Extrude.
    const auto face = occt::buildProfileFaceEvidence(*solved.input);
    CHECK(face.brep_valid);
    CHECK(face.solid_count == 0U); // planar Face, not already a Body

    kernel::LinearExtrudeInput extrude_input;
    extrude_input.profile = *solved.input;
    extrude_input.start_offset_mm = 0.0;
    extrude_input.end_offset_mm = 10.0;
    CHECK(extrude_input.valid());
    const auto extruded = provider.extrude(extrude_input);
    CHECK(extruded.ok());
    CHECK(extruded.solid_count == 1U);
    CHECK(extruded.face_count >= 6U);

    // Provenance survives the actual OCCT prism, not just the neutral
    // Profile adapter: each authored boundary yields ONE resolvable side.
    const auto extrude_evidence =
        occt::buildProfileExtrudeEvidence(*solved.input, 10.0);
    CHECK(extrude_evidence.ok());
    CHECK(extrude_evidence.shape.brep_valid);
    CHECK(extrude_evidence.shape.solid_count == 1U);
    CHECK(extrude_evidence.sides.size() == 4U);
    std::set<std::string> expected_sources;
    for (const auto& line : seed.lines) {
        CHECK(expected_sources.insert(line.id.serialized()).second);
    }
    std::set<std::string> extruded_sources;
    for (const auto& side : extrude_evidence.sides) {
        CHECK(side.status == kernel::ReferenceStatus::resolved);
        CHECK(side.candidate_face_count == 1U);
        CHECK(side.provenance);
        CHECK(!side.provenance->hole);
        CHECK(extruded_sources.insert(
            side.provenance->source_entity).second);
    }
    CHECK(extruded_sources == expected_sources);

    kernel::AngularRevolveInput revolve_input;
    revolve_input.profile = *solved.input;
    revolve_input.axis.origin = {0.0, 0.0, 0.0};
    revolve_input.axis.direction = {0.0, 1.0, 0.0};
    revolve_input.start_angle_radians = 0.0;
    revolve_input.end_angle_radians =
        2.0 * std::numbers::pi_v<double>;
    CHECK(revolve_input.valid());
    const auto revolved = provider.revolve(revolve_input);
    CHECK(revolved.ok());
    CHECK(revolved.solid_count == 1U);
    CHECK(revolved.brep_valid);

    // The *existing* OCCT revolve evidence must map each semantic edge
    // back to the same durable EntityId. Native periodic seams are not
    // substitute authored edges and may not be silently used as such.
    const auto revolve_evidence = occt::buildProfileFullRevolveEvidence(
        *solved.input, {0.0, 0.0}, {0.0, 1.0});
    CHECK(revolve_evidence.ok());
    CHECK(revolve_evidence.shape.brep_valid);
    CHECK(revolve_evidence.shape.solid_count == 1U);
    CHECK(revolve_evidence.boundary_faces.size() == 4U);
    std::set<std::string> revolved_sources;
    for (const auto& boundary : revolve_evidence.boundary_faces) {
        CHECK(boundary.status == kernel::ReferenceStatus::resolved);
        CHECK(boundary.candidate_face_count == 1U);
        CHECK(revolved_sources.insert(
            boundary.provenance.source_entity).second);
    }
    CHECK(revolved_sources == expected_sources);

    // The numerical bridge alone authorizes the provider calls above.
    // A representably open seam outside 1e-12 must never reach OCCT.
    backend.seam_offset = 5.0e-12;
    const auto rejected = part::resolveStageSolvedProfileKernelInput(
        session.document(), *profile.profile_id, current,
        provider, policy, backend);
    CHECK(rejected.status ==
        part::ProfileKernelInputStatus::unresolved_profile);
    CHECK(!rejected.input);
    CHECK(backend.calls == 2U);
    CHECK(session.document().findProfile(*profile.profile_id));
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state().lines == seed.lines);

    // A refused candidate must not poison either the authored intent or
    // the old Profile/OCCT path. Undo the ONLY new Coincident relation;
    // the exact same ProfileId must immediately work through the original
    // public resolver and produce a genuine native solid again.
    CHECK(session.undo().changed);
    const auto* unconstrained =
        session.document().findProfile(*profile.profile_id);
    CHECK(unconstrained != nullptr);
    CHECK(session.document().findSketch(*sketch.sketch_id)
        ->model.state().constraints.empty());
    const auto original_profile =
        part::resolveKernelProfileInput(session.document(),
                                       *profile.profile_id);
    CHECK(original_profile.ok());
    kernel::LinearExtrudeInput after_undo = extrude_input;
    after_undo.profile = *original_profile.input;
    const auto restored_solid = provider.extrude(after_undo);
    CHECK(restored_solid.ok());
    CHECK(restored_solid.brep_valid);
    CHECK(restored_solid.solid_count == 1U);

    // Redo restores the exact authored relation identity and fail-closed
    // unsolved Profile state; no stale OCCT or solver result is reused.
    CHECK(session.redo().changed);
    CHECK(session.document().findProfile(*profile.profile_id) != nullptr);
    const auto after_redo =
        session.document().findSketch(*sketch.sketch_id)->model.state();
    CHECK(after_redo.lines == seed.lines);
    CHECK(after_redo.constraints.size() == 1U);
    CHECK(after_redo.constraints.front().id == *coincide.constraint_id);
    CHECK(part::resolveKernelProfileInput(
        session.document(), *profile.profile_id).status ==
        part::ProfileKernelInputStatus::unresolved_profile);
    CHECK(!session.document().evaluateProfile(*profile.profile_id));
    CHECK(backend.calls == 2U);

    // A1.3 RED->GREEN: from SAME persisted ProfileIntent/ConstraintId into the
    // REAL existing Part Feature scheduler (not a naked OCCT adapter).
    // The original product evaluatePart must remain fail-closed until a
    // native numeric solver is explicitly supplied.
    auto staged = session.document().state();
    const auto feature_id = staged.body.next_feature_id.allocate();
    CHECK(feature_id);
    staged.body.features.push_back(part::PartFeature{
        *feature_id,
        "R13StageExtrude",
        false,
        part::ExtrudeFeature{
            *profile.profile_id,
            part::ExtrudeOperation::add,
            part::OneSidedExtrudeExtent{
                core::LengthValue{10.0}, false}}});
    auto owning = part::PartDocument::restore(
        session.document().documentId(),
        std::move(staged), session.document().revision());
    CHECK(owning.ok());
    CHECK(owning.document);
    const auto legacy_body = part::evaluatePart(*owning.document, provider);
    CHECK(legacy_body.features.size() == 1U);
    CHECK(legacy_body.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    backend.seam_offset = 5.0e-13;
    const auto actual_body = part::evaluatePartWithLocalSolve(
        *owning.document, provider, policy, backend);
    CHECK(actual_body.source_document_id == owning.document->documentId());
    CHECK(actual_body.source_revision == owning.document->revision());
    CHECK(actual_body.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(actual_body.features.size() == 1U);
    CHECK(actual_body.features.front().feature_id == *feature_id);
    CHECK(actual_body.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(actual_body.features.front().result_topology);
    CHECK(actual_body.features.front().result_topology->complete());
    CHECK(actual_body.body_solid != nullptr);
    CHECK(actual_body.current_topology);
    CHECK(actual_body.current_topology->complete());
    CHECK(backend.calls == 3U);

    // Nontrivial derived geometry must reach the actual runtime OCCT
    // solid generated by the SAME Part Feature scheduler. A caller-owned
    // backend shifts all 4 Line endpoints by +2 mm in Sketch U; the
    // Feature Body X bounds (not merely neutral Profile input) must shift
    // by exactly that amount within mesh numeric accuracy.
    const auto original_mesh = provider.presentationMesh(
        actual_body.body_solid);
    CHECK(original_mesh.ok());
    const auto x_bounds = [](const kernel::SolidPresentationMesh& mesh) {
        double low = std::numeric_limits<double>::infinity();
        double high = -std::numeric_limits<double>::infinity();
        for (const auto& triangle : mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                low = std::min(low, point.x);
                high = std::max(high, point.x);
            }
        }
        return std::pair{low, high};
    };
    const auto original_bounds = x_bounds(original_mesh.mesh);
    CHECK(std::isfinite(original_bounds.first));
    CHECK(std::isfinite(original_bounds.second));
    CHECK(std::abs(original_bounds.first - 5.0) < 0.01);
    CHECK(std::abs(original_bounds.second - 15.0) < 0.01);
    backend.translated_u = 2.0;
    const auto translated_body = part::evaluatePartWithLocalSolve(
        *owning.document, provider, policy, backend);
    CHECK(translated_body.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(translated_body.features.size() == 1U);
    CHECK(translated_body.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(translated_body.body_solid != nullptr);
    const auto translated_mesh = provider.presentationMesh(
        translated_body.body_solid);
    CHECK(translated_mesh.ok());
    const auto translated_bounds = x_bounds(translated_mesh.mesh);
    CHECK(std::abs(translated_bounds.first -
        original_bounds.first - 2.0) < 0.01);
    CHECK(std::abs(translated_bounds.second -
        original_bounds.second - 2.0) < 0.01);
    CHECK(backend.calls == 4U);
    backend.translated_u = 0.0;
    CHECK(owning.document->findProfile(*profile.profile_id));
    CHECK(owning.document->findSketch(*sketch.sketch_id)->model
        .state().constraints.front().id == *coincide.constraint_id);

    // Violated residual and obsolete revision cannot reach Feature/OCCT.
    backend.seam_offset = 5.0e-12;
    const auto rejected_body = part::evaluatePartWithLocalSolve(
        *owning.document, provider, policy, backend);
    CHECK(rejected_body.features.size() == 1U);
    CHECK(rejected_body.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(rejected_body.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(backend.calls == 5U);

    // The SAME Part Feature scheduler and SAME region resolver must also
    // execute a REAL OCCT full Revolve. The built-in Y axis lies outside
    // the 5..15 mm rectangle, so this is an annular, one-solid body.
    auto revolve_state = session.document().state();
    const auto revolve_id = revolve_state.body.next_feature_id.allocate();
    CHECK(revolve_id);
    revolve_state.body.features.push_back(part::PartFeature{
        *revolve_id,
        "R13StageRevolve",
        false,
        part::RevolveFeature{
            *profile.profile_id,
            part::AxisReference{
                part::BuiltinOriginAxisReference{
                    core::BuiltinReferenceRole::y_axis}},
            part::RevolveOperation::add,
            part::OneSidedRevolveExtent{
                core::AngleValue{2.0 * std::numbers::pi_v<double>},
                false}}});
    const auto revolve_document = part::PartDocument::restore(
        session.document().documentId(),
        std::move(revolve_state), session.document().revision());
    CHECK(revolve_document.ok() && revolve_document.document);
    const auto legacy_revolve =
        part::evaluatePart(*revolve_document.document, provider);
    CHECK(legacy_revolve.features.size() == 1U);
    CHECK(legacy_revolve.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    backend.seam_offset = 5.0e-13;
    const auto evaluated_revolve = part::evaluatePartWithLocalSolve(
        *revolve_document.document, provider, policy, backend);
    CHECK(evaluated_revolve.source_revision ==
        revolve_document.document->revision());
    CHECK(evaluated_revolve.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluated_revolve.features.size() == 1U);
    CHECK(evaluated_revolve.features.front().feature_id == *revolve_id);
    CHECK(evaluated_revolve.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(evaluated_revolve.features.front().result_topology);
    CHECK(evaluated_revolve.features.front().result_topology->complete());
    CHECK(evaluated_revolve.body_solid != nullptr);
    CHECK(evaluated_revolve.current_topology);
    CHECK(evaluated_revolve.current_topology->complete());
    CHECK(backend.calls == 6U);

    // Extrude's +2 mm X shift alone is insufficient: a full Revolve
    // around the built-in Y axis must change BOTH the annulus's inner
    // AND outer radii in the actual OCCT Body, not just report success.
    // Sample the provider's runtime presentation only as an independent
    // output-location witness; triangulation is NOT CAD identity and its
    // looser measurement tolerance never modifies the 1e-12 solver
    // acceptance budget or the exact Coincident vertex.
    const auto radial_bounds = [](const kernel::SolidPresentationMesh& mesh) {
        double inner = std::numeric_limits<double>::infinity();
        double outer = -std::numeric_limits<double>::infinity();
        for (const auto& triangle : mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                const double radius = std::hypot(point.x, point.z);
                inner = std::min(inner, radius);
                outer = std::max(outer, radius);
            }
        }
        return std::pair{inner, outer};
    };
    const auto base_revolve_mesh =
        provider.presentationMesh(evaluated_revolve.body_solid);
    CHECK(base_revolve_mesh.ok());
    const auto initial_radii = radial_bounds(base_revolve_mesh.mesh);
    CHECK(std::isfinite(initial_radii.first));
    CHECK(std::isfinite(initial_radii.second));
    CHECK(std::abs(initial_radii.first - 5.0) < 0.1);
    CHECK(std::abs(initial_radii.second - 15.0) < 0.1);

    backend.translated_u = 2.0;
    const auto shifted_revolve = part::evaluatePartWithLocalSolve(
        *revolve_document.document, provider, policy, backend);
    CHECK(shifted_revolve.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(shifted_revolve.features.size() == 1U);
    CHECK(shifted_revolve.features.front().feature_id == *revolve_id);
    CHECK(shifted_revolve.features.front().status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(shifted_revolve.body_solid != nullptr);
    CHECK(shifted_revolve.current_topology);
    CHECK(shifted_revolve.current_topology->complete());
    const auto shifted_revolve_mesh =
        provider.presentationMesh(shifted_revolve.body_solid);
    CHECK(shifted_revolve_mesh.ok());
    const auto derived_radii = radial_bounds(shifted_revolve_mesh.mesh);
    CHECK(std::abs(derived_radii.first -
        initial_radii.first - 2.0) < 0.1);
    CHECK(std::abs(derived_radii.second -
        initial_radii.second - 2.0) < 0.1);
    CHECK(backend.calls == 7U);
    CHECK(revolve_document.document->findSketch(*sketch.sketch_id)
        ->model.state().lines == seed.lines);
    backend.translated_u = 0.0;
    backend.seam_offset = 5.0e-12;
    const auto rejected_revolve = part::evaluatePartWithLocalSolve(
        *revolve_document.document, provider, policy, backend);
    CHECK(rejected_revolve.features.size() == 1U);
    CHECK(rejected_revolve.features.front().status ==
        part::FeatureEvaluationStatus::blocked);
    CHECK(rejected_revolve.body_status !=
        part::BodyEvaluationStatus::up_to_date);
    CHECK(backend.calls == 8U);

    // Two scheduled Feature stages in ONE native Part evaluation:
    // first Extrude uses an ordinary unconstrained Profile; the second
    // is a constrained inner rectangle cut through the live upstream
    // OCCT solid. Only the second stage is authorized to call the
    // numerical backend. Rejecting that solve may not publish the
    // downstream Cut, erase the valid prefix or mutate either Sketch.
    {
        app::DocumentSession chain_session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto parent_sketch = chain_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(parent_sketch.ok() && parent_sketch.sketch_id);
        const auto parent_rectangle = chain_session.execute(
            app::AddSketchRectangleCommand{
                *parent_sketch.sketch_id,
                chain_session.document().revision(),
                {5.0, 0.0}, {15.0, 8.0},
                sk::EntityRole::regular, false});
        CHECK(parent_rectangle.ok());
        const auto parent_regions = sk::analyzeRegions(
            chain_session.document().findSketch(
                *parent_sketch.sketch_id)->model);
        CHECK(parent_regions.complete());
        CHECK(parent_regions.regions.size() == 1U);
        const auto parent_intent =
            part::makeProfileRegionIntent(parent_regions.regions.front());
        CHECK(parent_intent);
        const auto parent_profile = chain_session.execute(
            app::CreateProfileCommand{
                *parent_sketch.sketch_id,
                chain_session.document().revision(), *parent_intent});
        CHECK(parent_profile.ok() && parent_profile.profile_id);

        const auto cut_sketch = chain_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(cut_sketch.ok() && cut_sketch.sketch_id);
        const auto cut_rectangle = chain_session.execute(
            app::AddSketchRectangleCommand{
                *cut_sketch.sketch_id,
                chain_session.document().revision(),
                {7.0, 2.0}, {9.0, 6.0},
                sk::EntityRole::regular, false});
        CHECK(cut_rectangle.ok());
        const auto cut_model = chain_session.document()
            .findSketch(*cut_sketch.sketch_id)->model.state();
        CHECK(cut_model.lines.size() == 4U);
        const auto cut_regions = sk::analyzeRegions(
            chain_session.document().findSketch(
                *cut_sketch.sketch_id)->model);
        CHECK(cut_regions.complete());
        CHECK(cut_regions.regions.size() == 1U);
        const auto cut_intent =
            part::makeProfileRegionIntent(cut_regions.regions.front());
        CHECK(cut_intent);
        const auto cut_profile = chain_session.execute(
            app::CreateProfileCommand{
                *cut_sketch.sketch_id,
                chain_session.document().revision(), *cut_intent});
        CHECK(cut_profile.ok() && cut_profile.profile_id);
        const auto cut_constraint = chain_session.execute(
            app::AddSketchRelationCommand{
                *cut_sketch.sketch_id,
                chain_session.document().revision(),
                sk::ConstraintKind::coincident,
                {cut_model.lines[0].id, sk::ConstraintElement::line_end},
                {cut_model.lines[1].id, sk::ConstraintElement::line_start}});
        CHECK(cut_constraint.ok() && cut_constraint.constraint_id);

        auto chained_state = chain_session.document().state();
        const auto parent_feature_id =
            chained_state.body.next_feature_id.allocate();
        const auto cut_feature_id =
            chained_state.body.next_feature_id.allocate();
        CHECK(parent_feature_id && cut_feature_id);
        chained_state.body.features.push_back(part::PartFeature{
            *parent_feature_id, "NativePrefixExtrude", false,
            part::ExtrudeFeature{
                *parent_profile.profile_id, part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0}, false}}});
        chained_state.body.features.push_back(part::PartFeature{
            *cut_feature_id, "NativeStageCut", false,
            part::ExtrudeFeature{
                *cut_profile.profile_id, part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0}, false}}});
        const auto chained_document = part::PartDocument::restore(
            chain_session.document().documentId(),
            std::move(chained_state),
            chain_session.document().revision());
        CHECK(chained_document.ok() && chained_document.document);
        const auto& authored_chain = *chained_document.document;
        const auto legacy_chain = part::evaluatePart(authored_chain, provider);
        CHECK(legacy_chain.features.size() == 2U);
        CHECK(legacy_chain.features[0].feature_id == *parent_feature_id);
        CHECK(legacy_chain.features[0].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(legacy_chain.features[1].feature_id == *cut_feature_id);
        CHECK(legacy_chain.features[1].status ==
            part::FeatureEvaluationStatus::blocked);
        CHECK(legacy_chain.body_status !=
            part::BodyEvaluationStatus::up_to_date);
        CHECK(legacy_chain.resolved_prefix_solid != nullptr);
        CHECK(legacy_chain.resolved_prefix_topology);
        CHECK(legacy_chain.resolved_prefix_topology->complete());

        ControlledSolver chain_backend;
        const auto solved_chain = part::evaluatePartWithLocalSolve(
            authored_chain, provider, policy, chain_backend);
        CHECK(solved_chain.source_document_id == authored_chain.documentId());
        CHECK(solved_chain.source_revision == authored_chain.revision());
        CHECK(solved_chain.body_status ==
            part::BodyEvaluationStatus::up_to_date);
        CHECK(solved_chain.features.size() == 2U);
        CHECK(solved_chain.features[0].feature_id == *parent_feature_id);
        CHECK(solved_chain.features[0].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(solved_chain.features[1].feature_id == *cut_feature_id);
        CHECK(solved_chain.features[1].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(solved_chain.features[1].result_topology);
        CHECK(solved_chain.features[1].result_topology->complete());
        CHECK(solved_chain.current_topology);
        CHECK(solved_chain.current_topology->complete());
        CHECK(solved_chain.body_solid != nullptr);
        CHECK(chain_backend.calls == 1U);
        const auto cut_mesh = provider.presentationMesh(
            solved_chain.body_solid);
        CHECK(cut_mesh.ok());
        bool has_cut_inner_corner = false;
        for (const auto& triangle : cut_mesh.mesh.triangles) {
            for (const auto& point :
                 {triangle.first, triangle.second, triangle.third}) {
                has_cut_inner_corner |=
                    std::abs(point.x - 7.0) < 0.01 &&
                    std::abs(point.y - 2.0) < 0.01;
            }
        }
        CHECK(has_cut_inner_corner);

        chain_backend.seam_offset = 5.0e-12;
        const auto bad_chain = part::evaluatePartWithLocalSolve(
            authored_chain, provider, policy, chain_backend);
        CHECK(bad_chain.features.size() == 2U);
        CHECK(bad_chain.features[0].status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(bad_chain.features[1].status ==
            part::FeatureEvaluationStatus::blocked);
        CHECK(bad_chain.body_status !=
            part::BodyEvaluationStatus::up_to_date);
        CHECK(bad_chain.body_solid == nullptr);
        CHECK(bad_chain.current_topology == std::nullopt);
        CHECK(bad_chain.resolved_prefix_solid != nullptr);
        CHECK(bad_chain.resolved_prefix_topology);
        CHECK(bad_chain.resolved_prefix_topology->complete());
        CHECK(bad_chain.resolved_prefix_topology->stage.feature_id ==
            *parent_feature_id);
        CHECK(chain_backend.calls == 2U);
        CHECK(authored_chain.findSketch(*cut_sketch.sketch_id)
            ->model.state().lines == cut_model.lines);
        CHECK(authored_chain.findSketch(*cut_sketch.sketch_id)
            ->model.state().constraints.size() == 1U);
        CHECK(authored_chain.findSketch(*cut_sketch.sketch_id)
            ->model.state().constraints.front().id ==
            *cut_constraint.constraint_id);
        CHECK(authored_chain.findProfile(*parent_profile.profile_id));
        CHECK(authored_chain.findProfile(*cut_profile.profile_id));

        // A legitimate PG-01 projected Line triggers an external provider call
    // *inside* prepareStageLocalSolveProblem. Reentering DocumentSession
    // through that provider must not leave any borrowed authored pointers
    // dangling or allow an obsolete problem to escape standalone prepare.
    {
        auto cloned = part::PartDocument::restore(
            authored_chain.documentId(), authored_chain.state(),
            authored_chain.revision());
        CHECK(cloned.ok() && cloned.document);
        app::DocumentSession projected_session{
            {}, std::move(*cloned.document)};
        const auto projected_sketch = projected_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(projected_sketch.ok() && projected_sketch.sketch_id);
        auto current_prefix = part::evaluatePart(
            projected_session.document(), provider);
        CHECK(current_prefix.features.size() == 2U);
        CHECK(current_prefix.features.front().status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(current_prefix.features.front().result_solid);
        CHECK(current_prefix.features.front().result_topology);
        const auto frame = part::resolveCurrentProjectionSketchFrame(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix);
        CHECK(frame);

        std::optional<part::MaterialEdgeReference> source;
        for (const auto& edge :
             current_prefix.features.front().result_topology->edges) {
            const auto material = part::authorMaterialEdgeReference(
                *current_prefix.features.front().result_topology,
                edge.runtime_token);
            if (!material.ok()) continue;
            const auto bound = provider.bindEdgeToBody(
                current_prefix.features.front().result_solid,
                edge.runtime_token);
            if (!bound) continue;
            const auto projected_edge = provider.projectEdgeToPlane(
                current_prefix.features.front().result_solid,
                *bound, *frame);
            if (projected_edge.ok() &&
                std::holds_alternative<kernel::Line2>(
                    *projected_edge.curve)) {
                source = *material.reference;
                break;
            }
        }
        CHECK(source);
        const auto linked = projected_session.execute(
            app::CreateProjectedSketchEdgesCommand{
                *projected_sketch.sketch_id,
                projected_session.document().revision(), {*source},
                sk::EntityRole::regular},
            provider);
        CHECK(linked.ok());
        CHECK(linked.entity_ids.size() == 1U);
        current_prefix = part::evaluatePart(
            projected_session.document(), provider);
        const auto valid_problem = part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, provider);
        CHECK(valid_problem);
        CHECK(valid_problem->fixed_lines.size() == 1U);
        CHECK(valid_problem->fixed_lines.front() == linked.entity_ids.front());

        struct ReentrantQuery final : kernel::IEdgeProjectionQuery {
            occt::OcctSolidModelingKernel& native;
            app::DocumentSession& session;
            sk::SketchId sketch_id;
            unsigned bindings{};
            unsigned projections{};
            bool mutated{};
            bool mutate_on_bind{};

            ReentrantQuery(
                occt::OcctSolidModelingKernel& native_provider,
                app::DocumentSession& owner,
                sk::SketchId target_id)
                : native{native_provider}, session{owner},
                  sketch_id{target_id} {}

            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle body,
                kernel::RuntimeEdgeToken edge) noexcept override {
                ++bindings;
                auto bound = native.bindEdgeToBody(
                    std::move(body), edge);
                if (mutate_on_bind && bound && bound->valid()) {
                    const auto edit = session.execute(
                        app::AddSketchLineCommand{
                            sketch_id, {27.0, 2.0}, {31.0, 2.0}});
                    CHECK(edit.ok() && edit.changed);
                    mutated = true;
                }
                return bound;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle body,
                const kernel::ScopedProjectionEdge& bound,
                const kernel::Frame3& target) noexcept override {
                ++projections;
                auto value = native.projectEdgeToPlane(
                    std::move(body), bound, target);
                if (value.ok()) {
                    const auto edit = session.execute(
                        app::AddSketchLineCommand{
                            sketch_id, {21.0, 2.0}, {24.0, 2.0}});
                    CHECK(edit.ok() && edit.changed);
                    mutated = true;
                }
                return value;
            }
        } reentrant{provider, projected_session,
                    *projected_sketch.sketch_id};

        CHECK(!part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, reentrant));
        CHECK(reentrant.projections == 1U);
        CHECK(reentrant.mutated);
        CHECK(current_prefix.source_revision !=
            projected_session.document().revision());
        CHECK(!part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, provider));
        current_prefix = part::evaluatePart(
            projected_session.document(), provider);
        const auto restored = part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, provider);
        CHECK(restored && restored->lines.size() == 2U);
        CHECK(restored->fixed_lines.size() == 1U);

        // A mutation while BINDING the real native Edge must abort
        // BEFORE the second projection callback. Even a valid scoped
        // OCCT edge cannot authorize use of an obsolete Part revision.
        ReentrantQuery bind_reentrant{
            provider, projected_session, *projected_sketch.sketch_id};
        bind_reentrant.mutate_on_bind = true;
        CHECK(!part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            current_prefix, bind_reentrant));
        CHECK(bind_reentrant.bindings == 1U);
        CHECK(bind_reentrant.projections == 0U);
        CHECK(bind_reentrant.mutated);
        CHECK(current_prefix.source_revision !=
            projected_session.document().revision());
        const auto after_binding_edit = part::evaluatePart(
            projected_session.document(), provider);
        const auto accepted_again = part::prepareStageLocalSolveProblem(
            projected_session.document(), *projected_sketch.sketch_id,
            after_binding_edit, provider);
        CHECK(accepted_again && accepted_again->lines.size() == 3U);
        CHECK(accepted_again->fixed_lines.size() == 1U);

        // The DOCUMENT SESSION command is a different publication
        // boundary from prepareStageLocalSolveProblem. An older outer
        // Project Edge request must not overwrite the newer document
        // committed by an adversarial callback during the native query.
        const auto command_sketch = projected_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(command_sketch.ok() && command_sketch.sketch_id);
        const app::CreateProjectedSketchEdgesCommand request{
            *command_sketch.sketch_id,
            projected_session.document().revision(), {*source},
            sk::EntityRole::regular};

        struct CommandReentrantKernel final
            : kernel::ISolidModelingKernel,
              kernel::IEdgeProjectionQuery {
            enum class Hook { bind, project };

            occt::OcctSolidModelingKernel& native;
            app::DocumentSession& session;
            sk::SketchId sketch_id;
            Hook hook;
            unsigned binds{};
            unsigned projections{};
            bool mutated{};

            CommandReentrantKernel(
                occt::OcctSolidModelingKernel& provider,
                app::DocumentSession& owner,
                sk::SketchId target,
                Hook phase)
                : native{provider}, session{owner},
                  sketch_id{target}, hook{phase} {}

            kernel::SolidModelingResult extrude(
                const kernel::LinearExtrudeInput& input,
                kernel::RuntimeSolidHandle upstream = {}) noexcept override {
                return native.extrude(input, std::move(upstream));
            }
            kernel::SolidModelingResult revolve(
                const kernel::AngularRevolveInput& input,
                kernel::RuntimeSolidHandle upstream = {}) noexcept override {
                return native.revolve(input, std::move(upstream));
            }
            kernel::SolidModelingResult edgeFeature(
                const kernel::EdgeFeatureInput& input,
                kernel::RuntimeSolidHandle upstream = {}) noexcept override {
                return native.edgeFeature(input, std::move(upstream));
            }
            kernel::BodyPresentationResult bodyPresentation(
                kernel::RuntimeSolidHandle body) noexcept override {
                return native.bodyPresentation(std::move(body));
            }
            kernel::SolidPresentationResult presentationMesh(
                kernel::RuntimeSolidHandle body) noexcept override {
                return native.presentationMesh(std::move(body));
            }
            void editNested() noexcept {
                CHECK(!mutated);
                const auto edit = session.execute(
                    app::AddSketchLineCommand{
                        sketch_id, {27.0, 3.0}, {30.0, 3.0}});
                CHECK(edit.ok() && edit.changed);
                mutated = true;
            }
            std::optional<kernel::ScopedProjectionEdge> bindEdgeToBody(
                kernel::RuntimeSolidHandle body,
                kernel::RuntimeEdgeToken edge) noexcept override {
                ++binds;
                auto bound = native.bindEdgeToBody(
                    std::move(body), edge);
                if (hook == Hook::bind && bound && bound->valid()) {
                    editNested();
                }
                return bound;
            }
            kernel::EdgeProjectionResult projectEdgeToPlane(
                kernel::RuntimeSolidHandle body,
                const kernel::ScopedProjectionEdge& bound,
                const kernel::Frame3& target) noexcept override {
                ++projections;
                auto result = native.projectEdgeToPlane(
                    std::move(body), bound, target);
                if (hook == Hook::project && result.ok()) {
                    editNested();
                }
                return result;
            }
        };

        // First, reenter at the scoped binding step. The second provider
        // hook must never run. The nested edit survives; the old outer
        // command must not create a binding or consume a second Undo.
        const auto baseline_undo = projected_session.undoDepth();
        CommandReentrantKernel bind_command{
            provider, projected_session, *command_sketch.sketch_id,
            CommandReentrantKernel::Hook::bind};
        const auto stale_bind = projected_session.execute(
            request, bind_command);
        CHECK(!stale_bind.ok() && !stale_bind.changed);
        CHECK(stale_bind.diagnostic.code ==
            app::DocumentSessionErrorCode::revision_diverged);
        CHECK(bind_command.mutated && bind_command.binds == 1U);
        CHECK(bind_command.projections == 0U);
        CHECK(projected_session.undoDepth() == baseline_undo + 1U);
        const auto* first_after =
            projected_session.document().findSketch(
                *command_sketch.sketch_id);
        CHECK(first_after != nullptr);
        CHECK(first_after->model.state().lines.size() == 1U);
        CHECK(first_after->projection_bindings.empty());

        // The next valid request uses the newly committed revision.
        // Reenter DURING the geometric projection this time; again no
        // obsolete outer commit, no lost nested edit or invented history.
        const app::CreateProjectedSketchEdgesCommand next_request{
            *command_sketch.sketch_id,
            projected_session.document().revision(), {*source},
            sk::EntityRole::regular};
        const auto second_undo = projected_session.undoDepth();
        CommandReentrantKernel project_command{
            provider, projected_session, *command_sketch.sketch_id,
            CommandReentrantKernel::Hook::project};
        const auto stale_project = projected_session.execute(
            next_request, project_command);
        CHECK(!stale_project.ok() && !stale_project.changed);
        CHECK(stale_project.diagnostic.code ==
            app::DocumentSessionErrorCode::revision_diverged);
        CHECK(project_command.mutated &&
            project_command.binds == 1U &&
            project_command.projections == 1U);
        CHECK(projected_session.undoDepth() == second_undo + 1U);
        const auto* second_after =
            projected_session.document().findSketch(
                *command_sketch.sketch_id);
        CHECK(second_after != nullptr);
        CHECK(second_after->model.state().lines.size() == 2U);
        CHECK(second_after->projection_bindings.empty());

        // A normal, same-revision native OCCT call must still commit
        // exactly one target/binding as one Undoable transaction.
        const auto clean_revision = projected_session.document().revision();
        const auto clean_undo = projected_session.undoDepth();
        const auto clean_project = projected_session.execute(
            app::CreateProjectedSketchEdgesCommand{
                *command_sketch.sketch_id, clean_revision, {*source},
                sk::EntityRole::regular},
            provider);
        CHECK(clean_project.ok() && clean_project.changed);
        CHECK(clean_project.entity_ids.size() == 1U);
        CHECK(projected_session.undoDepth() == clean_undo + 1U);
        const auto* clean_after =
            projected_session.document().findSketch(
                *command_sketch.sketch_id);
        CHECK(clean_after != nullptr);
        CHECK(clean_after->model.state().lines.size() == 3U);
        CHECK(clean_after->projection_bindings.size() == 1U);
        CHECK(clean_after->projection_bindings.front().target_entity ==
            clean_project.entity_ids.front());

        // Break Link is ALSO a revision-bound provider command. The
        // recompute may reenter through native BIND or PROJECT. Its older
        // outer transaction must not detach the live link, erase a nested
        // Line or consume another Undo slot. Both hooks are exercised on
        // real OCCT-backed geometry, not artificial pick tokens.
        const auto linked_id = clean_project.entity_ids.front();
        const auto check_stale_break = [&](CommandReentrantKernel::Hook hook) {
            const auto expected = projected_session.document().revision();
            const auto undo_before = projected_session.undoDepth();
            const auto* before = projected_session.document().findSketch(
                *command_sketch.sketch_id);
            CHECK(before != nullptr);
            const auto line_count = before->model.state().lines.size();
            CommandReentrantKernel reentry{
                provider, projected_session, *command_sketch.sketch_id,
                hook};
            const auto broken = projected_session.execute(
                app::BreakProjectedEdgeLinksCommand{
                    *command_sketch.sketch_id, {linked_id}, expected},
                reentry);
            CHECK(!broken.ok() && !broken.changed);
            CHECK(broken.status ==
                app::BreakProjectedEdgeLinkStatus::stale_revision);
            CHECK(reentry.mutated && reentry.binds == 1U);
            CHECK(reentry.projections ==
                (hook == CommandReentrantKernel::Hook::bind ? 0U : 1U));
            CHECK(projected_session.undoDepth() == undo_before + 1U);
            const auto* live = projected_session.document().findSketch(
                *command_sketch.sketch_id);
            CHECK(live != nullptr);
            CHECK(live->model.state().lines.size() == line_count + 1U);
            CHECK(live->projection_bindings.size() == 1U);
            CHECK(live->projection_bindings.front().target_entity == linked_id);
        };
        check_stale_break(CommandReentrantKernel::Hook::bind);
        check_stale_break(CommandReentrantKernel::Hook::project);

        // A fresh ordinary native Break Link remains exactly one atomic
        // Undoable edit. It retains the latest resolved local Line/EntityId
        // while removing only its external PG-01 dependency.
        const auto final_undo = projected_session.undoDepth();
        const auto* before_break = projected_session.document().findSketch(
            *command_sketch.sketch_id);
        CHECK(before_break != nullptr);
        const auto final_lines = before_break->model.state().lines.size();
        const auto normal_break = projected_session.execute(
            app::BreakProjectedEdgeLinksCommand{
                *command_sketch.sketch_id, {linked_id},
                projected_session.document().revision()},
            provider);
        CHECK(normal_break.ok() && normal_break.changed);
        CHECK(projected_session.undoDepth() == final_undo + 1U);
        const auto* detached = projected_session.document().findSketch(
            *command_sketch.sketch_id);
        CHECK(detached != nullptr);
        CHECK(detached->projection_bindings.empty());
        CHECK(detached->model.findLine(linked_id) != nullptr);
        CHECK(detached->model.state().lines.size() == final_lines);

        // PG-01 Create/Edit Profile are revision-bound semantic Commands
        // too. A provider callback may reenter the session after the
        // commands captured their prospective PartAuthoredState. A stale
        // original command must return revision_diverged, not silently
        // report an unrelated broken RegionIntent or publish obsolete
        // Profile state over the genuine nested edit.
        const auto profile_sketch = projected_session.execute(
            app::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(profile_sketch.ok() && profile_sketch.sketch_id);
        const auto profile_linked = projected_session.execute(
            app::CreateProjectedSketchEdgesCommand{
                *profile_sketch.sketch_id,
                projected_session.document().revision(), {*source},
                sk::EntityRole::construction},
            provider);
        CHECK(profile_linked.ok());
        CHECK(profile_linked.entity_ids.size() == 1U);
        const auto rectangle = projected_session.execute(
            app::AddSketchRectangleCommand{
                *profile_sketch.sketch_id,
                projected_session.document().revision(),
                {50.0, 20.0}, {56.0, 26.0},
                sk::EntityRole::regular, false});
        CHECK(rectangle.ok());
        const auto* profile_model = projected_session.document()
            .findSketch(*profile_sketch.sketch_id);
        CHECK(profile_model != nullptr);
        const auto profile_regions = sk::analyzeRegions(profile_model->model);
        CHECK(profile_regions.complete());
        CHECK(profile_regions.regions.size() == 1U);
        const auto profile_intent = part::makeProfileRegionIntent(
            profile_regions.regions.front());
        CHECK(profile_intent);

        const auto check_stale_profile_create = [&](CommandReentrantKernel::Hook hook) {
            const auto original_revision = projected_session.document().revision();
            const auto undo_before = projected_session.undoDepth();
            const auto profile_count = projected_session.document().state().profiles.size();
            const auto before_count = projected_session.document()
                .findSketch(*profile_sketch.sketch_id)->model.state().lines.size();
            CommandReentrantKernel reentry{
                provider, projected_session, *profile_sketch.sketch_id, hook};
            const auto created = projected_session.execute(
                app::CreateProfileCommand{
                    *profile_sketch.sketch_id, original_revision,
                    *profile_intent},
                reentry);
            CHECK(!created.ok() && !created.changed);
            CHECK(created.diagnostic.code ==
                app::DocumentSessionErrorCode::revision_diverged);
            CHECK(reentry.mutated && reentry.binds == 1U);
            CHECK(reentry.projections ==
                (hook == CommandReentrantKernel::Hook::bind ? 0U : 1U));
            CHECK(projected_session.undoDepth() == undo_before + 1U);
            CHECK(projected_session.document().state().profiles.size() ==
                profile_count);
            const auto* live = projected_session.document().findSketch(
                *profile_sketch.sketch_id);
            CHECK(live != nullptr);
            CHECK(live->model.state().lines.size() == before_count + 1U);
            CHECK(live->projection_bindings.size() == 1U);
        };
        check_stale_profile_create(CommandReentrantKernel::Hook::bind);
        check_stale_profile_create(CommandReentrantKernel::Hook::project);

        const auto good_profile = projected_session.execute(
            app::CreateProfileCommand{
                *profile_sketch.sketch_id,
                projected_session.document().revision(), *profile_intent},
            provider);
        CHECK(good_profile.ok() && good_profile.profile_id);
        CHECK(projected_session.document().findProfile(*good_profile.profile_id));

        const auto check_stale_profile_edit = [&](CommandReentrantKernel::Hook hook) {
            const auto original_revision = projected_session.document().revision();
            const auto undo_before = projected_session.undoDepth();
            const auto before_count = projected_session.document()
                .findSketch(*profile_sketch.sketch_id)->model.state().lines.size();
            const auto before_intent = projected_session.document()
                .findProfile(*good_profile.profile_id)->region_intent;
            CommandReentrantKernel reentry{
                provider, projected_session, *profile_sketch.sketch_id, hook};
            const auto changed = projected_session.execute(
                app::ReplaceProfileRegionIntentCommand{
                    *good_profile.profile_id, original_revision,
                    *profile_intent},
                reentry);
            CHECK(!changed.ok() && !changed.changed);
            CHECK(changed.diagnostic.code ==
                app::DocumentSessionErrorCode::revision_diverged);
            CHECK(reentry.mutated && reentry.binds == 1U);
            CHECK(reentry.projections ==
                (hook == CommandReentrantKernel::Hook::bind ? 0U : 1U));
            CHECK(projected_session.undoDepth() == undo_before + 1U);
            const auto* live = projected_session.document().findSketch(
                *profile_sketch.sketch_id);
            CHECK(live != nullptr);
            CHECK(live->model.state().lines.size() == before_count + 1U);
            CHECK(projected_session.document().findProfile(
                *good_profile.profile_id)->region_intent == before_intent);
        };
        check_stale_profile_edit(CommandReentrantKernel::Hook::bind);
        check_stale_profile_edit(CommandReentrantKernel::Hook::project);

        // Positive same-revision Edit Profile remains a no-op when the
        // authored RegionIntent is unchanged; no spurious Undo/dirty write.
        const auto fresh_revision = projected_session.document().revision();
        const auto edit_undo = projected_session.undoDepth();
        const auto clean_edit = projected_session.execute(
            app::ReplaceProfileRegionIntentCommand{
                *good_profile.profile_id, fresh_revision,
                *profile_intent},
            provider);
        CHECK(clean_edit.ok() && !clean_edit.changed);
        CHECK(projected_session.document().revision() == fresh_revision);
        CHECK(projected_session.undoDepth() == edit_undo);
        }
    }

    std::cout << "R13_A1_OCCT_PASS exact_seam=1"
              << " extrude_solid=1 revolve_solid=1"
              << " stale_profile_publication=0"
              << " derived_extrude_x_shift=2mm"
              << " derived_revolve_radii_shift=2mm"
              << " stage_prefix_cut=1"
              << " pg01_reentrant_stale=0"
              << " pg01_reentrant_bind_stale=0"
              << " pg01_command_atomic_reentry=1"
              << " pg01_break_atomic_reentry=1"
              << " pg01_profile_command_reentry=1\n";
    return EXIT_SUCCESS;
}
#endif
