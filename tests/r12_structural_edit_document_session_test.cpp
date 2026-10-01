#include <simplesolid2/application/document_session.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << '\n'; \
            return 1; \
        } \
    } while (false)

bool near(double first, double second) {
    return std::abs(first - second) <= 1.0e-10;
}

} // namespace

int main() {
    using namespace simplesolid2;

    // Circle -> Arc Trim is one atomic history entry, retires the Circle ID,
    // and Redo restores the exact same fresh Arc ID.
    {
        application::DocumentSession session{
            {},
            part::PartDocument::create(
                core::DocumentId::generate())};
        const auto sketch_created =
            session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
        CHECK(sketch_created.ok());
        CHECK(sketch_created.sketch_id.has_value());
        const auto sketch_id =
            *sketch_created.sketch_id;

        const auto circle =
            session.execute(
                application::AddSketchCircleCommand{
                    sketch_id,
                    {0.0, 0.0},
                    10.0,
                    sketch::EntityRole::construction});
        const auto cutter =
            session.execute(
                application::AddSketchLineCommand{
                    sketch_id,
                    {-20.0, 0.0},
                    {20.0, 0.0}});
        CHECK(circle.ok() && circle.entity_id);
        CHECK(cutter.ok() && cutter.entity_id);

        const auto undo_before =
            session.undoDepth();
        const auto trim_revision =
            session.document().revision();
        const auto trim =
            session.execute(
                application::TrimSketchCommand{
                    sketch_id,
                    trim_revision,
                    *circle.entity_id,
                    {*cutter.entity_id},
                    {0.0, -10.0}});
        CHECK(trim.ok());
        CHECK(trim.changed);
        CHECK(
            trim.edit_status ==
            sketch::StructuralEditStatus::ready);
        CHECK(trim.result_entity.has_value());
        const auto replacement =
            *trim.result_entity;
        CHECK(replacement != *circle.entity_id);
        CHECK(
            session.undoDepth() ==
            undo_before + 1U);

        const auto* after =
            session.document().findSketch(
                sketch_id);
        CHECK(after != nullptr);
        CHECK(
            after->model.findCircle(
                *circle.entity_id) == nullptr);
        const auto* arc =
            after->model.findArc(replacement);
        CHECK(arc != nullptr);
        CHECK(
            arc->role() ==
            sketch::EntityRole::construction);

        CHECK(session.undo().changed);
        const auto* undone =
            session.document().findSketch(
                sketch_id);
        CHECK(undone != nullptr);
        CHECK(
            undone->model.findCircle(
                *circle.entity_id) != nullptr);
        CHECK(
            undone->model.findArc(
                replacement) == nullptr);

        CHECK(session.redo().changed);
        const auto* redone =
            session.document().findSketch(
                sketch_id);
        CHECK(redone != nullptr);
        CHECK(
            redone->model.findCircle(
                *circle.entity_id) == nullptr);
        CHECK(
            redone->model.findArc(
                replacement) != nullptr);

        // A command captured before the Trim cannot mutate after Redo.
        const auto stale =
            session.execute(
                application::TrimSketchCommand{
                    sketch_id,
                    trim_revision,
                    replacement,
                    {*cutter.entity_id},
                    {0.0, 10.0}});
        CHECK(!stale.ok());
        CHECK(
            stale.diagnostic.code ==
            application::DocumentSessionErrorCode::
                revision_diverged);
    }

    // Extend Both commits two Line mutations in exactly one Undo step.
    {
        application::DocumentSession session{
            {},
            part::PartDocument::create(
                core::DocumentId::generate())};
        const auto sketch_created =
            session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::xy_plane});
        CHECK(sketch_created.ok());
        const auto sketch_id =
            *sketch_created.sketch_id;

        const auto first =
            session.execute(
                application::AddSketchLineCommand{
                    sketch_id,
                    {0.0, 0.0},
                    {1.0, 0.0}});
        const auto second =
            session.execute(
                application::AddSketchLineCommand{
                    sketch_id,
                    {3.0, 2.0},
                    {3.0, 1.0},
                    sketch::EntityRole::construction});
        CHECK(first.ok() && first.entity_id);
        CHECK(second.ok() && second.entity_id);

        const auto before_undo =
            session.undoDepth();
        const auto extended =
            session.execute(
                application::ExtendBothSketchLinesCommand{
                    sketch_id,
                    session.document().revision(),
                    *first.entity_id,
                    *second.entity_id});
        CHECK(extended.ok());
        CHECK(extended.changed);
        CHECK(
            session.undoDepth() ==
            before_undo + 1U);

        const auto* sketch_after =
            session.document().findSketch(sketch_id);
        CHECK(sketch_after != nullptr);
        const auto* first_after =
            sketch_after->model.findLine(
                *first.entity_id);
        const auto* second_after =
            sketch_after->model.findLine(
                *second.entity_id);
        CHECK(first_after != nullptr);
        CHECK(second_after != nullptr);
        CHECK(near(first_after->end().u, 3.0));
        CHECK(near(first_after->end().v, 0.0));
        CHECK(near(second_after->end().u, 3.0));
        CHECK(near(second_after->end().v, 0.0));

        CHECK(session.undo().changed);
        const auto* sketch_undo =
            session.document().findSketch(sketch_id);
        CHECK(sketch_undo != nullptr);
        CHECK(near(
            sketch_undo->model
                .findLine(*first.entity_id)
                ->end().u,
            1.0));
        CHECK(near(
            sketch_undo->model
                .findLine(*second.entity_id)
                ->end().v,
            1.0));
    }

    return EXIT_SUCCESS;
}
