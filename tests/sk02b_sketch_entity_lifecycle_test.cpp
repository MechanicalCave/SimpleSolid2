#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-02B entity lifecycle CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_sk02b_lifecycle_" +
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

} // namespace

int main() {
    CHECK(!sketch::EntityId::parse("").has_value());
    CHECK(!sketch::EntityId::parse("0").has_value());
    CHECK(!sketch::EntityId::parse("01").has_value());
    CHECK(!sketch::EntityId::parse("+1").has_value());
    CHECK(!sketch::EntityId::parse("18446744073709551616").has_value());
    CHECK(sketch::EntityId::parse("1").has_value());

    CHECK(!sketch::EntityIdCursor::parse("").has_value());
    CHECK(!sketch::EntityIdCursor::parse("0").has_value());
    CHECK(!sketch::EntityIdCursor::parse("01").has_value());
    CHECK(sketch::EntityIdCursor::parse("1").has_value());

    sketch::SketchModel neutral_model;
    const auto neutral_id =
        neutral_model.addLine(
            sketch::Point2{0.0, 0.0},
            sketch::Point2{1.0, 0.0});
    CHECK(neutral_id.serialized() == "1");
    CHECK(
        neutral_model.entityIdCursor().serialized() ==
        "2");

    const auto captured = neutral_model.state();
    auto restored =
        sketch::SketchModel::restore(captured);
    CHECK(restored.has_value());
    CHECK(*restored == neutral_model);
    CHECK(
        restored->entityIdCursor() ==
        neutral_model.entityIdCursor());

    auto duplicate_state = captured;
    duplicate_state.lines.push_back(
        duplicate_state.lines.front());
    CHECK(
        !sketch::SketchModel::restore(
             std::move(duplicate_state))
             .has_value());

    auto non_finite_state = captured;
    non_finite_state.lines.front().start.u =
        std::numeric_limits<double>::infinity();
    CHECK(
        !sketch::SketchModel::restore(
             std::move(non_finite_state))
             .has_value());

    TempDirectory temp;
    const auto path =
        temp.path / "Lifecycle.ss2part";

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    part::PartDocumentStore store;
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{
        path,
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok());
    CHECK(created.sketch_id.has_value());
    const auto sketch_id = *created.sketch_id;

    CHECK(session.save().ok());
    CHECK(!session.needsSave());
    const auto baseline_undo_depth =
        session.undoDepth();

    const auto invalid_revision =
        session.document().revision();
    const auto invalid_depth =
        session.undoDepth();

    const auto invalid_zero =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{5.0, 5.0},
                sketch::Point2{5.0, 5.0}});
    CHECK(!invalid_zero.ok());
    CHECK(!invalid_zero.changed);
    CHECK(!invalid_zero.entity_id.has_value());
    CHECK(
        session.document().revision() ==
        invalid_revision);
    CHECK(session.undoDepth() == invalid_depth);
    CHECK(!session.needsSave());

    const auto nan =
        std::numeric_limits<double>::quiet_NaN();
    const auto invalid_nan =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{nan, 0.0},
                sketch::Point2{1.0, 0.0}});
    CHECK(!invalid_nan.ok());
    CHECK(session.undoDepth() == invalid_depth);
    CHECK(!session.needsSave());

    const auto unknown_sketch =
        sketch::SketchId::generate();
    const auto invalid_target =
        session.execute(
            application::AddSketchLineCommand{
                unknown_sketch,
                sketch::Point2{0.0, 0.0},
                sketch::Point2{1.0, 0.0}});
    CHECK(!invalid_target.ok());
    CHECK(session.undoDepth() == invalid_depth);
    CHECK(!session.needsSave());

    const auto first =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{0.0, 0.0},
                sketch::Point2{10.0, 0.0}});
    CHECK(first.ok());
    CHECK(first.changed);
    CHECK(first.entity_id.has_value());
    CHECK(first.entity_id->serialized() == "1");
    CHECK(
        session.undoDepth() ==
        baseline_undo_depth + 1U);
    CHECK(session.needsSave());

    CHECK(session.undo().changed);
    CHECK(!session.needsSave());
    const auto* after_undo =
        session.document().findSketch(sketch_id);
    CHECK(after_undo != nullptr);
    CHECK(after_undo->model.entityCount() == 0U);

    const auto branched =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{0.0, 1.0},
                sketch::Point2{10.0, 1.0}});
    CHECK(branched.ok());
    CHECK(branched.entity_id.has_value());
    CHECK(*branched.entity_id != *first.entity_id);
    CHECK(branched.entity_id->serialized() == "2");
    CHECK(!session.canRedo());

    CHECK(session.undo().changed);
    CHECK(!session.needsSave());
    CHECK(session.redo().changed);
    CHECK(session.needsSave());

    const auto* redone =
        session.document().findSketch(sketch_id);
    CHECK(redone != nullptr);
    const auto* redone_line =
        redone->model.findLine(*branched.entity_id);
    CHECK(redone_line != nullptr);
    CHECK(redone_line->id() == *branched.entity_id);
    CHECK(
        (redone_line->start() ==
         sketch::Point2{0.0, 1.0}));
    CHECK(
        (redone_line->end() ==
         sketch::Point2{10.0, 1.0}));

    const auto erase_depth =
        session.undoDepth();
    const auto erased =
        session.execute(
            application::EraseSketchEntityCommand{
                sketch_id,
                *branched.entity_id});
    CHECK(erased.ok());
    CHECK(erased.changed);
    CHECK(session.undoDepth() == erase_depth + 1U);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model
            .findLine(*branched.entity_id) ==
        nullptr);

    CHECK(session.undo().changed);
    CHECK(session.canRedo());
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model
            .findLine(*branched.entity_id) !=
        nullptr);

    const auto before_failed_erase_depth =
        session.undoDepth();
    const auto failed_erase =
        session.execute(
            application::EraseSketchEntityCommand{
                sketch_id,
                sketch::EntityId{}});
    CHECK(!failed_erase.ok());
    CHECK(
        session.undoDepth() ==
        before_failed_erase_depth);
    CHECK(session.canRedo());

    CHECK(session.redo().changed);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model
            .findLine(*branched.entity_id) ==
        nullptr);

    return EXIT_SUCCESS;
}
