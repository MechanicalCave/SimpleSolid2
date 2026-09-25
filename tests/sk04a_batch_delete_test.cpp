#include <simplesolid2/application/document_session.hpp>

#include <cstdlib>
#include <iostream>
#include <vector>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04A batch Delete CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

struct SessionSnapshot final {
    part::PartAuthoredState state;
    core::DocumentRevision revision;
    std::size_t undo_depth{};
    std::size_t redo_depth{};
    bool dirty{};
};

SessionSnapshot snapshot(
    const application::DocumentSession& session) {
    return SessionSnapshot{
        session.document().state(),
        session.document().revision(),
        session.undoDepth(),
        session.redoDepth(),
        session.needsSave()};
}

void checkUnchanged(
    const application::DocumentSession& session,
    const SessionSnapshot& before) {
    CHECK(session.document().state() == before.state);
    CHECK(
        session.document().revision() ==
        before.revision);
    CHECK(session.undoDepth() == before.undo_depth);
    CHECK(session.redoDepth() == before.redo_depth);
    CHECK(session.needsSave() == before.dirty);
}

const sketch::Line* line(
    const application::DocumentSession& session,
    const sketch::SketchId& sketch_id,
    sketch::EntityId entity_id) {
    const auto* hosted =
        session.document().findSketch(sketch_id);
    if (hosted == nullptr) {
        return nullptr;
    }
    return hosted->model.findLine(entity_id);
}

} // namespace

int main() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());

    application::DocumentSession session{
        "sk04a-batch-delete.ss2part",
        std::move(document)};

    const auto first_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    const auto second_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xz_plane});
    CHECK(first_sketch.ok());
    CHECK(second_sketch.ok());
    CHECK(first_sketch.sketch_id.has_value());
    CHECK(second_sketch.sketch_id.has_value());

    const auto first_id =
        session.execute(
            application::AddSketchLineCommand{
                *first_sketch.sketch_id,
                sketch::Point2{0.0, 0.0},
                sketch::Point2{10.0, 0.0}});
    const auto second_id =
        session.execute(
            application::AddSketchLineCommand{
                *first_sketch.sketch_id,
                sketch::Point2{10.0, 0.0},
                sketch::Point2{10.0, 10.0}});
    const auto third_id =
        session.execute(
            application::AddSketchLineCommand{
                *first_sketch.sketch_id,
                sketch::Point2{10.0, 10.0},
                sketch::Point2{0.0, 10.0}});
    const auto other_sketch_id =
        session.execute(
            application::AddSketchLineCommand{
                *second_sketch.sketch_id,
                sketch::Point2{0.0, 0.0},
                sketch::Point2{0.0, 5.0}});

    CHECK(first_id.ok() && first_id.entity_id.has_value());
    CHECK(second_id.ok() && second_id.entity_id.has_value());
    CHECK(third_id.ok() && third_id.entity_id.has_value());
    CHECK(
        other_sketch_id.ok() &&
        other_sketch_id.entity_id.has_value());

    // EntityId is model-local: both fresh Sketches begin at the same
    // value, but deleting from one Sketch must not affect the other.
    CHECK(
        *first_id.entity_id ==
        *other_sketch_id.entity_id);

    const auto original_first =
        *line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id);
    const auto original_second =
        *line(
            session,
            *first_sketch.sketch_id,
            *second_id.entity_id);

    const auto before_multi_revision =
        session.document().revision();
    const auto before_multi_depth =
        session.undoDepth();

    const auto multi =
        session.execute(
            application::EraseSketchEntitiesCommand{
                *first_sketch.sketch_id,
                {
                    *first_id.entity_id,
                    *second_id.entity_id,
                }});

    CHECK(multi.ok());
    CHECK(multi.changed);
    CHECK(
        session.document().revision().value() ==
        before_multi_revision.value() + 1U);
    CHECK(
        session.undoDepth() ==
        before_multi_depth + 1U);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id) == nullptr);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *second_id.entity_id) == nullptr);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *third_id.entity_id) != nullptr);
    CHECK(
        line(
            session,
            *second_sketch.sketch_id,
            *other_sketch_id.entity_id) != nullptr);

    CHECK(session.undo().changed);
    const auto* restored_first =
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id);
    const auto* restored_second =
        line(
            session,
            *first_sketch.sketch_id,
            *second_id.entity_id);
    CHECK(restored_first != nullptr);
    CHECK(restored_second != nullptr);
    CHECK(restored_first->id() == original_first.id());
    CHECK(restored_second->id() == original_second.id());
    CHECK(restored_first->start() == original_first.start());
    CHECK(restored_first->end() == original_first.end());
    CHECK(restored_second->start() == original_second.start());
    CHECK(restored_second->end() == original_second.end());

    CHECK(session.redo().changed);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id) == nullptr);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *second_id.entity_id) == nullptr);

    CHECK(session.undo().changed);

    const auto before_single_revision =
        session.document().revision();
    const auto before_single_depth =
        session.undoDepth();
    const auto single =
        session.execute(
            application::EraseSketchEntitiesCommand{
                *first_sketch.sketch_id,
                {*third_id.entity_id}});
    CHECK(single.ok());
    CHECK(single.changed);
    CHECK(
        session.document().revision().value() ==
        before_single_revision.value() + 1U);
    CHECK(
        session.undoDepth() ==
        before_single_depth + 1U);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *third_id.entity_id) == nullptr);
    CHECK(session.undo().changed);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *third_id.entity_id) != nullptr);

    const auto empty_before = snapshot(session);
    const auto empty =
        session.execute(
            application::EraseSketchEntitiesCommand{
                *first_sketch.sketch_id,
                {}});
    CHECK(!empty.ok());
    CHECK(!empty.changed);
    checkUnchanged(session, empty_before);

    const auto duplicate_before = snapshot(session);
    const auto duplicate =
        session.execute(
            application::EraseSketchEntitiesCommand{
                *first_sketch.sketch_id,
                {
                    *first_id.entity_id,
                    *first_id.entity_id,
                }});
    CHECK(!duplicate.ok());
    CHECK(!duplicate.changed);
    checkUnchanged(session, duplicate_before);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id) != nullptr);

    const auto invalid_before = snapshot(session);
    const auto invalid =
        session.execute(
            application::EraseSketchEntitiesCommand{
                *first_sketch.sketch_id,
                {
                    *first_id.entity_id,
                    sketch::EntityId{},
                }});
    CHECK(!invalid.ok());
    CHECK(!invalid.changed);
    checkUnchanged(session, invalid_before);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id) != nullptr);

    const auto unknown_id =
        sketch::EntityId::parse("999");
    CHECK(unknown_id.has_value());

    const auto stale_before = snapshot(session);
    const auto stale =
        session.execute(
            application::EraseSketchEntitiesCommand{
                *first_sketch.sketch_id,
                {
                    *first_id.entity_id,
                    *unknown_id,
                }});
    CHECK(!stale.ok());
    CHECK(!stale.changed);
    checkUnchanged(session, stale_before);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id) != nullptr);

    const auto unknown_sketch_before =
        snapshot(session);
    const auto unknown_sketch =
        session.execute(
            application::EraseSketchEntitiesCommand{
                sketch::SketchId::generate(),
                {*first_id.entity_id}});
    CHECK(!unknown_sketch.ok());
    CHECK(!unknown_sketch.changed);
    checkUnchanged(
        session,
        unknown_sketch_before);

    // Existing single-erase command remains valid.
    const auto single_compat_before =
        session.undoDepth();
    const auto single_compat =
        session.execute(
            application::EraseSketchEntityCommand{
                *first_sketch.sketch_id,
                *first_id.entity_id});
    CHECK(single_compat.ok());
    CHECK(single_compat.changed);
    CHECK(
        session.undoDepth() ==
        single_compat_before + 1U);
    CHECK(
        line(
            session,
            *first_sketch.sketch_id,
            *first_id.entity_id) == nullptr);

    return EXIT_SUCCESS;
}
