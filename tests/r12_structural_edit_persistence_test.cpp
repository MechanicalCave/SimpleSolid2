#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << '\n'; \
            return 1; \
        } \
    } while (false)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_r12_structural_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

bool near(double first, double second) {
    return std::abs(first - second) <= 1.0e-10;
}

} // namespace

int main() {
    using namespace simplesolid2;

    TempDirectory temp;
    const auto path =
        temp.path / "R12StructuralEdit.ss2part";

    part::PartDocumentStore store;
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    const auto published =
        store.createNew(path, document);
    CHECK(published.ok());

    application::DocumentSession session{
        path,
        std::move(document),
        *published.checkpoint};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                {0.0, 0.0},
                10.0});
    const auto cutter =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {-20.0, 0.0},
                {20.0, 0.0},
                sketch::EntityRole::construction});
    CHECK(circle.ok() && circle.entity_id);
    CHECK(cutter.ok() && cutter.entity_id);

    const auto trimmed =
        session.execute(
            application::TrimSketchCommand{
                sketch_id,
                session.document().revision(),
                *circle.entity_id,
                {*cutter.entity_id},
                {0.0, -10.0}});
    CHECK(trimmed.ok() && trimmed.changed);
    CHECK(trimmed.result_entity.has_value());
    const auto replacement =
        *trimmed.result_entity;
    CHECK(replacement != *circle.entity_id);

    const auto* before_save =
        session.document().findSketch(sketch_id);
    CHECK(before_save != nullptr);
    CHECK(
        before_save->model.findCircle(
            *circle.entity_id) == nullptr);
    const auto* before_arc =
        before_save->model.findArc(replacement);
    CHECK(before_arc != nullptr);
    const auto cursor_after_trim =
        before_save->model.entityIdCursor();
    const auto saved_arc = *before_arc;

    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    const auto* loaded_sketch =
        loaded.document->findSketch(sketch_id);
    CHECK(loaded_sketch != nullptr);
    CHECK(
        loaded_sketch->model.findCircle(
            *circle.entity_id) == nullptr);
    const auto* loaded_arc =
        loaded_sketch->model.findArc(replacement);
    CHECK(loaded_arc != nullptr);
    CHECK(
        loaded_sketch->model.entityIdCursor() ==
        cursor_after_trim);
    CHECK(loaded_arc->id() == saved_arc.id());
    CHECK(loaded_arc->role() == saved_arc.role());
    CHECK(loaded_arc->center() == saved_arc.center());
    CHECK(near(
        loaded_arc->radius(),
        saved_arc.radius()));
    CHECK(near(
        loaded_arc->startAngle(),
        saved_arc.startAngle()));
    CHECK(near(
        loaded_arc->sweepAngle(),
        saved_arc.sweepAngle()));

    application::DocumentSession reopened{
        path,
        std::move(*loaded.document),
        *loaded.checkpoint};

    const auto fresh =
        reopened.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {20.0, 20.0},
                {30.0, 20.0}});
    CHECK(fresh.ok() && fresh.entity_id);
    CHECK(*fresh.entity_id != *circle.entity_id);
    CHECK(*fresh.entity_id != *cutter.entity_id);
    CHECK(*fresh.entity_id != replacement);
    CHECK(replacement < *fresh.entity_id);

    const auto* reopened_sketch =
        reopened.document().findSketch(sketch_id);
    CHECK(reopened_sketch != nullptr);
    CHECK(
        reopened_sketch->model.entityIdCursor() >
        cursor_after_trim);

    CHECK(reopened.save().ok());
    auto loaded_again = store.load(path);
    CHECK(loaded_again.ok());
    const auto* final_sketch =
        loaded_again.document->findSketch(sketch_id);
    CHECK(final_sketch != nullptr);
    CHECK(
        final_sketch->model.findArc(replacement) !=
        nullptr);
    CHECK(
        final_sketch->model.findLine(
            *fresh.entity_id) != nullptr);

    return EXIT_SUCCESS;
}
