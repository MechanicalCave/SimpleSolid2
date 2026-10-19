#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

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

    std::cout << "R13-A1 Horizontal authored-state test PASS\n";
}
