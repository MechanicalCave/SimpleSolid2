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

    // Undo removes the copy but preserves the allocator high-water mark.
    const auto undo_a = session.undo();
    CHECK(undo_a.ok() && undo_a.changed);
    const auto* after_undo_a =
        session.document().findSketch(sketch_id);
    CHECK(after_undo_a != nullptr);
    for (const auto id : copied_a.entity_ids) {
        CHECK(!after_undo_a->model.contains(id));
    }

    // Redo restores the same copied identities, not newly allocated ones.
    const auto redo_a = session.redo();
    CHECK(redo_a.ok() && redo_a.changed);
    const auto* after_redo_a =
        session.document().findSketch(sketch_id);
    CHECK(after_redo_a != nullptr);
    for (const auto id : copied_a.entity_ids) {
        CHECK(after_redo_a->model.contains(id));
    }

    // Branch history after Undo: a new copy must not reuse undone IDs.
    CHECK(session.undo().ok());
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

    // Stale revision fails without authored or allocator mutation.
    const auto stale_state =
        session.document().state();
    const auto stale_undo =
        session.undoDepth();
    const auto stale =
        session.execute(
            application::DuplicateSketchGeometryCommand{
                sketch_id,
                revision_before_a,
                *placement_a});
    CHECK(!stale.ok());
    CHECK(
        stale.diagnostic.code ==
        application::DocumentSessionErrorCode::revision_diverged);
    CHECK(session.document().state() == stale_state);
    CHECK(session.undoDepth() == stale_undo);

    CHECK(session.save().ok());

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    application::DocumentSession reopened{
        path,
        std::move(*loaded.document)};

    const auto* reopened_sketch =
        reopened.document().findSketch(sketch_id);
    CHECK(reopened_sketch != nullptr);
    for (const auto id : source_ids) {
        CHECK(reopened_sketch->model.contains(id));
    }
    for (const auto id : copied_b.entity_ids) {
        CHECK(reopened_sketch->model.contains(id));
    }

    // Persisted next_entity_id must keep future COPY above all prior
    // committed/undone identities.
    const auto placement_c =
        sketch::translateSketchGeometry(
            source,
            {20.0, -4.0});
    CHECK(placement_c.has_value());
    const auto copied_c =
        reopened.execute(
            application::DuplicateSketchGeometryCommand{
                sketch_id,
                reopened.document().revision(),
                *placement_c});
    CHECK(copied_c.ok() && copied_c.changed);
    CHECK(disjoint(source_ids, copied_c.entity_ids));
    CHECK(disjoint(copied_a.entity_ids, copied_c.entity_ids));
    CHECK(disjoint(copied_b.entity_ids, copied_c.entity_ids));

    return EXIT_SUCCESS;
}
