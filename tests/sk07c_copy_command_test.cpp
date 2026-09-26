#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-07C COPY command CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) \
    check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("ss2-sk07c-copy-command-" +
             std::string{
                 core::DocumentId::generate().value()});
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

sketch::SketchTransformGeometry capture(
    const application::DocumentSession& session,
    const sketch::SketchId& sketch_id,
    const std::vector<sketch::EntityId>& ids) {
    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    const auto geometry =
        sketch::captureSketchTransformGeometry(
            hosted->model,
            ids);
    CHECK(geometry.has_value());
    return *geometry;
}

bool disjoint(
    const std::vector<sketch::EntityId>& left,
    const std::vector<sketch::EntityId>& right) {
    std::set<sketch::EntityId> seen{
        left.begin(),
        left.end()};
    return std::none_of(
        right.begin(),
        right.end(),
        [&seen](sketch::EntityId id) {
            return seen.contains(id);
        });
}

} // namespace

int main() {
    constexpr double pi =
        std::numbers::pi_v<double>;

    TempDirectory temp;
    const auto path =
        temp.path / "sk07c-copy.ss2part";

    part::PartDocumentStore store;
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{
        path,
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok() && created.sketch_id);
    const auto sketch_id = *created.sketch_id;

    const auto line =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {1.0, 0.0},
                {3.0, 0.0}});
    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {2.0, 1.0},
                2.0});
    const auto arc =
        session.execute(
            application::AddSketchArcCommand{
                sketch_id,
                {1.0, 2.0},
                4.0,
                pi * 0.25,
                pi * 0.75});
    CHECK(line.ok() && line.entity_id);
    CHECK(circle.ok() && circle.entity_id);
    CHECK(arc.ok() && arc.entity_id);

    const std::vector<sketch::EntityId> source_ids{
        *line.entity_id,
        *circle.entity_id,
        *arc.entity_id};
    const auto source =
        capture(session, sketch_id, source_ids);

    // Establish a clean persisted baseline before COPY so allocator-only
    // high-water changes after Undo are observable through needsSave().
    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    const auto placement_a =
        sketch::translateSketchGeometry(
            source,
            {10.0, 5.0});
    CHECK(placement_a.has_value());

    const auto undo_before_a =
        session.undoDepth();
    const auto revision_before_a =
        session.document().revision();
    const auto copied_a =
        session.execute(
            application::DuplicateSketchGeometryCommand{
                sketch_id,
                revision_before_a,
                *placement_a});
    CHECK(copied_a.ok() && copied_a.changed);
    CHECK(copied_a.entity_ids.size() == 3U);
    CHECK(disjoint(source_ids, copied_a.entity_ids));
    CHECK(session.undoDepth() == undo_before_a + 1U);
    CHECK(
        session.document().revision() ==
        *revision_before_a.next());
    CHECK(session.needsSave());

    const auto* after_a =
        session.document().findSketch(sketch_id);
    CHECK(after_a != nullptr);
    for (const auto id : source_ids) {
        CHECK(after_a->model.contains(id));
    }
    for (const auto id : copied_a.entity_ids) {
        CHECK(after_a->model.contains(id));
    }
    CHECK(capture(session, sketch_id, source_ids) == source);

    // Undo removes A, but the high-water identity cursor remains advanced
    // and therefore the session still requires Save even though authored
    // geometry matches the persisted baseline.
    const auto undo_a = session.undo();
    CHECK(undo_a.ok() && undo_a.changed);
    const auto* after_undo_a =
        session.document().findSketch(sketch_id);
    CHECK(after_undo_a != nullptr);
    for (const auto id : copied_a.entity_ids) {
        CHECK(!after_undo_a->model.contains(id));
    }
    CHECK(session.needsSave());

    // Redo restores the exact copied identities.
    const auto redo_a = session.redo();
    CHECK(redo_a.ok() && redo_a.changed);
    const auto* after_redo_a =
        session.document().findSketch(sketch_id);
    CHECK(after_redo_a != nullptr);
    for (const auto id : copied_a.entity_ids) {
        CHECK(after_redo_a->model.contains(id));
    }

    // Branch from Undo: B must not reuse A's identities and must clear
    // the abandoned Redo branch.
    CHECK(session.undo().ok());
    CHECK(session.needsSave());
    const auto placement_b =
        sketch::translateSketchGeometry(
            source,
            {-7.0, 3.0});
    CHECK(placement_b.has_value());
    const auto copied_b =
        session.execute(
            application::DuplicateSketchGeometryCommand{
                sketch_id,
                session.document().revision(),
                *placement_b});
    CHECK(copied_b.ok() && copied_b.changed);
    CHECK(copied_b.entity_ids.size() == 3U);
    CHECK(disjoint(source_ids, copied_b.entity_ids));
    CHECK(disjoint(copied_a.entity_ids, copied_b.entity_ids));
    CHECK(session.redoDepth() == 0U);

    // Undo B as well. Geometry is back at the persisted baseline, but both
    // committed identity ranges A and B must remain durably consumed.
    CHECK(session.undo().ok());
    CHECK(
        session.document().findSketch(sketch_id)->
            model.entityCount() == 3U);
    CHECK(session.needsSave());

    // Saving this allocator-only difference is the key SK-07C lifecycle
    // boundary. Reopen must retain the high-water cursor without schema
    // migration.
    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    auto loaded_after_undo = store.load(path);
    CHECK(loaded_after_undo.ok());
    application::DocumentSession reopened{
        path,
        std::move(*loaded_after_undo.document)};

    const auto* reopened_baseline =
        reopened.document().findSketch(sketch_id);
    CHECK(reopened_baseline != nullptr);
    CHECK(reopened_baseline->model.entityCount() == 3U);
    for (const auto id : source_ids) {
        CHECK(reopened_baseline->model.contains(id));
    }

    const auto placement_c =
        sketch::translateSketchGeometry(
            source,
            {20.0, -4.0});
    CHECK(placement_c.has_value());
    const auto revision_before_c =
        reopened.document().revision();
    const auto copied_c =
        reopened.execute(
            application::DuplicateSketchGeometryCommand{
                sketch_id,
                revision_before_c,
                *placement_c});
    CHECK(copied_c.ok() && copied_c.changed);
    CHECK(copied_c.entity_ids.size() == 3U);
    CHECK(disjoint(source_ids, copied_c.entity_ids));
    CHECK(disjoint(copied_a.entity_ids, copied_c.entity_ids));
    CHECK(disjoint(copied_b.entity_ids, copied_c.entity_ids));

    // A stale command must not allocate another identity range.
    const auto stale_state =
        reopened.document().state();
    const auto stale_undo =
        reopened.undoDepth();
    const auto stale =
        reopened.execute(
            application::DuplicateSketchGeometryCommand{
                sketch_id,
                revision_before_c,
                *placement_a});
    CHECK(!stale.ok());
    CHECK(
        stale.diagnostic.code ==
        application::DocumentSessionErrorCode::revision_diverged);
    CHECK(reopened.document().state() == stale_state);
    CHECK(reopened.undoDepth() == stale_undo);

    CHECK(reopened.save().ok());

    auto loaded_final = store.load(path);
    CHECK(loaded_final.ok());
    const auto* final_sketch =
        loaded_final.document->findSketch(sketch_id);
    CHECK(final_sketch != nullptr);
    for (const auto id : source_ids) {
        CHECK(final_sketch->model.contains(id));
    }
    for (const auto id : copied_c.entity_ids) {
        CHECK(final_sketch->model.contains(id));
    }

    return EXIT_SUCCESS;
}
