#include <simplesolid2/application/document_session.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-05A Line geometry command CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

application::SketchLineGeometryUpdate update(
    sketch::EntityId id,
    sketch::Point2 start,
    sketch::Point2 end) {
    return {id, start, end};
}

} // namespace

int main() {
    auto document =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{"sk05a-line-geometry.ss2part"},
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok() && created.changed);
    CHECK(created.sketch_id.has_value());
    const auto sketch_id = *created.sketch_id;

    const auto first =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 0.0},
                {10.0, 0.0}});
    const auto second =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 10.0},
                {10.0, 10.0}});
    CHECK(first.ok() && first.entity_id.has_value());
    CHECK(second.ok() && second.entity_id.has_value());

    const auto id1 = *first.entity_id;
    const auto id2 = *second.entity_id;
    const auto baseline_undo = session.undoDepth();
    const auto before_revision = session.document().revision();

    const auto moved =
        session.execute(
            application::UpdateSketchLinesCommand{
                sketch_id,
                before_revision,
                {
                    update(
                        id1,
                        {2.0, 3.0},
                        {12.0, 3.0}),
                    update(
                        id2,
                        {2.0, 13.0},
                        {12.0, 13.0}),
                }});
    CHECK(moved.ok() && moved.changed);
    CHECK(session.undoDepth() == baseline_undo + 1U);

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(hosted->model.findLine(id1) != nullptr);
    CHECK(hosted->model.findLine(id2) != nullptr);
    CHECK(hosted->model.findLine(id1)->start() ==
          sketch::Point2{2.0, 3.0});
    CHECK(hosted->model.findLine(id2)->end() ==
          sketch::Point2{12.0, 13.0});

    const auto no_op_revision =
        session.document().revision();
    const auto no_op_undo =
        session.undoDepth();
    const auto no_op =
        session.execute(
            application::UpdateSketchLinesCommand{
                sketch_id,
                no_op_revision,
                {
                    update(
                        id1,
                        {2.0, 3.0},
                        {12.0, 3.0}),
                    update(
                        id2,
                        {2.0, 13.0},
                        {12.0, 13.0}),
                }});
    CHECK(no_op.ok());
    CHECK(!no_op.changed);
    CHECK(session.undoDepth() == no_op_undo);
    CHECK(session.document().revision() == no_op_revision);

    const auto before_invalid_state =
        session.document().state();
    const auto before_invalid_revision =
        session.document().revision();
    const auto invalid =
        session.execute(
            application::UpdateSketchLinesCommand{
                sketch_id,
                before_invalid_revision,
                {
                    update(
                        id1,
                        {4.0, 5.0},
                        {14.0, 5.0}),
                    update(
                        sketch::EntityId{},
                        {1.0, 1.0},
                        {2.0, 2.0}),
                }});
    CHECK(!invalid.ok());
    CHECK(!invalid.changed);
    CHECK(session.document().state() == before_invalid_state);
    CHECK(session.document().revision() == before_invalid_revision);

    const auto stale_revision =
        session.document().revision();
    const auto add_third =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {20.0, 0.0},
                {30.0, 0.0}});
    CHECK(add_third.ok() && add_third.changed);

    const auto state_before_stale =
        session.document().state();
    const auto stale =
        session.execute(
            application::UpdateSketchLinesCommand{
                sketch_id,
                stale_revision,
                {
                    update(
                        id1,
                        {100.0, 100.0},
                        {110.0, 100.0}),
                }});
    CHECK(!stale.ok());
    CHECK(
        stale.diagnostic.code ==
        application::DocumentSessionErrorCode::
            revision_diverged);
    CHECK(session.document().state() == state_before_stale);

    const auto before_history =
        session.document().state();
    const auto history_revision =
        session.document().revision();
    const auto reshape =
        session.execute(
            application::UpdateSketchLinesCommand{
                sketch_id,
                history_revision,
                {
                    update(
                        id1,
                        {-5.0, 3.0},
                        {12.0, 3.0}),
                }});
    CHECK(reshape.ok() && reshape.changed);
    CHECK(session.undo().changed);
    CHECK(session.document().state() == before_history);
    CHECK(session.redo().changed);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(id1)
            ->start() ==
        sketch::Point2{-5.0, 3.0});

    return EXIT_SUCCESS;
}
