#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#if defined(_WIN32)
#define NOMINMAX
#include <windows.h>
#ifdef DocumentProperties
#undef DocumentProperties
#endif
#endif

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 DocumentSession CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

std::string readText(const std::filesystem::path& path) {
    std::ifstream in{path, std::ios::binary};
    CHECK(static_cast<bool>(in));
    return std::string{
        std::istreambuf_iterator<char>{in},
        std::istreambuf_iterator<char>{}};
}

struct TempDirectory final {
    std::filesystem::path path;
    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_document_session_" +
                std::to_string(
                    std::filesystem::file_time_type::clock::now()
                        .time_since_epoch().count()));
        std::filesystem::create_directories(path);
    }
    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};
}

int main() {
    TempDirectory temp;
    const auto path = temp.path / "Part001.ss2part";
    auto document = part::PartDocument::create(core::DocumentId::generate());
    part::PartDocumentStore store;
    const auto created =
        store.createNew(path, document);
    CHECK(created.ok());

    application::DocumentSession session{
        path,
        std::move(document),
        *created.checkpoint};
    CHECK(!session.needsSave());
    CHECK(!session.canUndo());
    CHECK(!session.canRedo());

    CHECK(session.document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!session.document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));

    const auto visibility_no_op_revision =
        session.document().revision().value();
    const auto visibility_no_op = session.execute(
        application::SetBuiltinReferenceVisibilityCommand{
            {core::BuiltinReferenceRole::xy_plane},
            false});
    CHECK(visibility_no_op.ok());
    CHECK(!visibility_no_op.changed);
    CHECK(session.document().revision().value() ==
          visibility_no_op_revision);
    CHECK(session.undoDepth() == 0U);

    const auto invalid_revision = session.document().revision().value();
    const auto invalid_visibility = session.execute(
        application::SetBuiltinReferenceVisibilityCommand{
            {static_cast<core::BuiltinReferenceRole>(255U)},
            false});
    CHECK(!invalid_visibility.ok());
    CHECK(invalid_visibility.diagnostic.code ==
          application::DocumentSessionErrorCode::invalid_command);
    CHECK(session.document().revision().value() == invalid_revision);
    CHECK(session.undoDepth() == 0U);

    const auto before_visibility =
        session.document().revision().value();
    const auto hide_mixed = session.execute(
        application::SetBuiltinReferenceVisibilityCommand{
            {
                core::BuiltinReferenceRole::origin_point,
                core::BuiltinReferenceRole::xy_plane,
            },
            false});
    CHECK(hide_mixed.ok());
    CHECK(hide_mixed.changed);
    CHECK(session.document().revision().value() ==
          before_visibility + 1U);
    CHECK(session.undoDepth() == 1U);
    CHECK(session.needsSave());
    CHECK(!session.document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!session.document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));

    const auto before_visibility_undo =
        session.document().revision().value();
    CHECK(session.undo().changed);
    CHECK(session.document().revision().value() ==
          before_visibility_undo + 1U);
    CHECK(!session.needsSave());
    CHECK(session.document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!session.document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(session.canRedo());

    core::DocumentProperties properties;
    properties.title = "Drive Shaft";
    const auto first_revision = session.document().revision().value();
    auto executed = session.execute(
        application::SetDocumentPropertiesCommand{properties});
    CHECK(executed.ok());
    CHECK(executed.changed);
    CHECK(session.document().revision().value() == first_revision + 1U);
    CHECK(session.needsSave());
    CHECK(session.canUndo());
    CHECK(!session.canRedo());

    const auto no_op_revision = session.document().revision().value();
    const auto no_op = session.execute(
        application::SetDocumentPropertiesCommand{properties});
    CHECK(no_op.ok());
    CHECK(!no_op.changed);
    CHECK(session.document().revision().value() == no_op_revision);
    CHECK(session.undoDepth() == 1U);

    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    properties.number = "12-04-117";
    CHECK(session.execute(
              application::SetDocumentPropertiesCommand{properties})
              .changed);
    CHECK(session.needsSave());
    const auto before_undo = session.document().revision().value();

    CHECK(session.undo().changed);
    CHECK(session.document().revision().value() == before_undo + 1U);
    CHECK(!session.needsSave());
    CHECK(session.canRedo());

    const auto before_redo = session.document().revision().value();
    CHECK(session.redo().changed);
    CHECK(session.document().revision().value() == before_redo + 1U);
    CHECK(session.needsSave());

    CHECK(session.undo().changed);
    CHECK(!session.needsSave());

    {
        auto branch_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession branch_session{
            {},
            std::move(branch_document)};

        core::DocumentProperties branch_properties;
        branch_properties.title = "A";
        CHECK(branch_session.execute(
                  application::SetDocumentPropertiesCommand{
                      branch_properties})
                  .changed);
        branch_properties.title = "B";
        CHECK(branch_session.execute(
                  application::SetDocumentPropertiesCommand{
                      branch_properties})
                  .changed);
        branch_properties.title = "C";
        CHECK(branch_session.execute(
                  application::SetDocumentPropertiesCommand{
                      branch_properties})
                  .changed);
        CHECK(branch_session.undoDepth() == 3U);
        CHECK(branch_session.redoDepth() == 0U);

        CHECK(branch_session.undo().changed);
        CHECK(
            branch_session.document().properties().title ==
            "B");
        CHECK(branch_session.undoDepth() == 2U);
        CHECK(branch_session.redoDepth() == 1U);

        const auto branch_revision =
            branch_session.document().revision();
        const auto branch_no_op =
            branch_session.execute(
                application::SetDocumentPropertiesCommand{
                    branch_session.document().properties()});
        CHECK(branch_no_op.ok());
        CHECK(!branch_no_op.changed);
        CHECK(
            branch_session.document().revision() ==
            branch_revision);
        CHECK(branch_session.undoDepth() == 2U);
        CHECK(branch_session.redoDepth() == 1U);

        const auto rejected_with_redo =
            branch_session.execute(
                application::SetBuiltinReferenceVisibilityCommand{
                    {
                        static_cast<
                            core::BuiltinReferenceRole>(255U),
                    },
                    false});
        CHECK(!rejected_with_redo.ok());
        CHECK(
            rejected_with_redo.diagnostic.code ==
            application::DocumentSessionErrorCode::
                invalid_command);
        CHECK(branch_session.undoDepth() == 2U);
        CHECK(branch_session.redoDepth() == 1U);
        CHECK(
            branch_session.document().properties().title ==
            "B");

        branch_properties =
            branch_session.document().properties();
        branch_properties.title = "D";
        const auto branched =
            branch_session.execute(
                application::SetDocumentPropertiesCommand{
                    branch_properties});
        CHECK(branched.ok());
        CHECK(branched.changed);
        CHECK(branch_session.undoDepth() == 3U);
        CHECK(branch_session.redoDepth() == 0U);
        CHECK(
            branch_session.document().properties().title ==
            "D");

        CHECK(branch_session.undo().changed);
        CHECK(
            branch_session.document().properties().title ==
            "B");
        CHECK(branch_session.redoDepth() == 1U);

        CHECK(branch_session.redo().changed);
        CHECK(
            branch_session.document().properties().title ==
            "D");
        CHECK(branch_session.redoDepth() == 0U);
        CHECK(!branch_session.redo().changed);
    }

    {
        const auto exhausted_id =
            core::DocumentId::generate();
        auto exhausted_seed =
            part::PartDocument::create(
                exhausted_id);
        auto exhausted_restore =
            part::PartDocument::restore(
                exhausted_id,
                exhausted_seed.state(),
                core::DocumentRevision{
                    std::numeric_limits<
                        std::uint64_t>::max()});
        CHECK(exhausted_restore.ok());

        application::DocumentSession exhausted_session{
            {},
            std::move(*exhausted_restore.document)};
        const auto exhausted_before =
            exhausted_session.document().state();
        const auto exhausted_revision =
            exhausted_session.document().revision();

        core::DocumentProperties exhausted_properties;
        exhausted_properties.title =
            "Must not commit at max revision";
        const auto exhausted =
            exhausted_session.execute(
                application::SetDocumentPropertiesCommand{
                    exhausted_properties});
        CHECK(!exhausted.ok());
        CHECK(
            exhausted.diagnostic.code ==
            application::DocumentSessionErrorCode::
                transaction_failure);
        CHECK(
            exhausted.diagnostic.commit_code ==
            part::PartCommitErrorCode::
                revision_exhausted);
        CHECK(
            exhausted_session.document().state() ==
            exhausted_before);
        CHECK(
            exhausted_session.document().revision() ==
            exhausted_revision);
        CHECK(exhausted_session.undoDepth() == 0U);
        CHECK(exhausted_session.redoDepth() == 0U);
    }

    {
        auto rectangle_document =
            part::PartDocument::create(
                core::DocumentId::generate());
        application::DocumentSession
            rectangle_session{
                {},
                std::move(rectangle_document)};

        const auto sketch_created =
            rectangle_session.execute(
                application::CreatePartSketchCommand{
                    core::BuiltinReferenceRole::
                        xy_plane});
        CHECK(
            sketch_created.ok() &&
            sketch_created.sketch_id.has_value());
        const auto rectangle_sketch_id =
            *sketch_created.sketch_id;

        const auto before_rectangle_revision =
            rectangle_session.document().revision();
        const auto before_rectangle_undo =
            rectangle_session.undoDepth();

        const auto rectangle =
            rectangle_session.execute(
                application::AddSketchRectangleCommand{
                    rectangle_sketch_id,
                    before_rectangle_revision,
                    {0.0, 0.0},
                    {10.0, 5.0},
                    sketch::EntityRole::regular,
                    true});
        CHECK(rectangle.ok());
        CHECK(rectangle.changed);
        CHECK(rectangle.entity_ids.size() == 6U);
        CHECK(
            rectangle_session.document().revision().value() ==
            before_rectangle_revision.value() + 1U);
        CHECK(
            rectangle_session.undoDepth() ==
            before_rectangle_undo + 1U);

        const auto* rectangle_sketch =
            rectangle_session.document()
                .findSketch(rectangle_sketch_id);
        CHECK(rectangle_sketch != nullptr);
        CHECK(
            rectangle_sketch->model.entityCount() ==
            6U);

        for (std::size_t index = 0U;
             index < 4U;
             ++index) {
            const auto* line =
                rectangle_sketch->model.findLine(
                    rectangle.entity_ids[index]);
            CHECK(line != nullptr);
            CHECK(
                line->role() ==
                sketch::EntityRole::regular);
        }
        for (std::size_t index = 4U;
             index < 6U;
             ++index) {
            const auto* line =
                rectangle_sketch->model.findLine(
                    rectangle.entity_ids[index]);
            CHECK(line != nullptr);
            CHECK(
                line->role() ==
                sketch::EntityRole::construction);
        }

        const auto* first_diagonal =
            rectangle_sketch->model.findLine(
                rectangle.entity_ids[4]);
        const auto* second_diagonal =
            rectangle_sketch->model.findLine(
                rectangle.entity_ids[5]);
        CHECK(first_diagonal != nullptr);
        CHECK(second_diagonal != nullptr);
        CHECK(
            first_diagonal->start() ==
            sketch::Point2{0.0, 0.0});
        CHECK(
            first_diagonal->end() ==
            sketch::Point2{10.0, 5.0});
        CHECK(
            second_diagonal->start() ==
            sketch::Point2{10.0, 0.0});
        CHECK(
            second_diagonal->end() ==
            sketch::Point2{0.0, 5.0});

        const auto rectangle_ids =
            rectangle.entity_ids;
        CHECK(rectangle_session.undo().changed);
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount() == 0U);
        CHECK(rectangle_session.redo().changed);
        const auto* redone_sketch =
            rectangle_session.document()
                .findSketch(rectangle_sketch_id);
        CHECK(redone_sketch != nullptr);
        for (const auto id : rectangle_ids) {
            CHECK(
                redone_sketch->model.findLine(id) !=
                nullptr);
        }

        const auto construction_line =
            rectangle_session.execute(
                application::AddSketchLineCommand{
                    rectangle_sketch_id,
                    {20.0, 0.0},
                    {21.0, 0.0},
                    sketch::EntityRole::construction});
        CHECK(
            construction_line.ok() &&
            construction_line.entity_id.has_value());
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.findLine(
                    *construction_line.entity_id)
                ->role() ==
            sketch::EntityRole::construction);

        const auto construction_circle =
            rectangle_session.execute(
                application::AddSketchCircleCommand{
                    rectangle_sketch_id,
                    {24.0, 2.0},
                    1.0,
                    sketch::EntityRole::construction});
        CHECK(
            construction_circle.ok() &&
            construction_circle.entity_id.has_value());
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.findCircle(
                    *construction_circle.entity_id)
                ->role() ==
            sketch::EntityRole::construction);

        const auto construction_arc =
            rectangle_session.execute(
                application::AddSketchArcCommand{
                    rectangle_sketch_id,
                    {28.0, 2.0},
                    1.0,
                    0.0,
                    1.0,
                    sketch::EntityRole::construction});
        CHECK(
            construction_arc.ok() &&
            construction_arc.entity_id.has_value());
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.findArc(
                    *construction_arc.entity_id)
                ->role() ==
            sketch::EntityRole::construction);

        const auto stale_revision =
            rectangle_session.document().revision();
        const auto advance =
            rectangle_session.execute(
                application::AddSketchLineCommand{
                    rectangle_sketch_id,
                    {30.0, 0.0},
                    {31.0, 0.0},
                    sketch::EntityRole::regular});
        CHECK(advance.ok() && advance.changed);

        const auto count_before_stale =
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount();
        const auto undo_before_stale =
            rectangle_session.undoDepth();
        const auto revision_before_stale =
            rectangle_session.document().revision();
        const auto stale_rectangle =
            rectangle_session.execute(
                application::AddSketchRectangleCommand{
                    rectangle_sketch_id,
                    stale_revision,
                    {40.0, 0.0},
                    {45.0, 5.0},
                    sketch::EntityRole::regular,
                    false});
        CHECK(!stale_rectangle.ok());
        CHECK(
            stale_rectangle.diagnostic.code ==
            application::DocumentSessionErrorCode::
                revision_diverged);
        CHECK(
            rectangle_session.document().revision() ==
            revision_before_stale);
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount() ==
            count_before_stale);
        CHECK(
            rectangle_session.undoDepth() ==
            undo_before_stale);

        const auto before_degenerate_revision =
            rectangle_session.document().revision();
        const auto before_degenerate_undo =
            rectangle_session.undoDepth();
        const auto count_before_degenerate =
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount();
        const auto degenerate =
            rectangle_session.execute(
                application::AddSketchRectangleCommand{
                    rectangle_sketch_id,
                    before_degenerate_revision,
                    {50.0, 0.0},
                    {50.0, 5.0},
                    sketch::EntityRole::regular,
                    false});
        CHECK(!degenerate.ok());
        CHECK(
            degenerate.diagnostic.code ==
            application::DocumentSessionErrorCode::
                invalid_command);
        CHECK(
            rectangle_session.document().revision() ==
            before_degenerate_revision);
        CHECK(
            rectangle_session.undoDepth() ==
            before_degenerate_undo);
        CHECK(
            rectangle_session.document()
                .findSketch(rectangle_sketch_id)
                ->model.entityCount() ==
            count_before_degenerate);

        const auto construction_rectangle =
            rectangle_session.execute(
                application::AddSketchRectangleCommand{
                    rectangle_sketch_id,
                    rectangle_session.document()
                        .revision(),
                    {60.0, 0.0},
                    {65.0, 5.0},
                    sketch::EntityRole::construction,
                    true});
        CHECK(construction_rectangle.ok());
        CHECK(
            construction_rectangle.entity_ids.size() ==
            6U);
        for (const auto id :
             construction_rectangle.entity_ids) {
            const auto* line =
                rectangle_session.document()
                    .findSketch(rectangle_sketch_id)
                    ->model.findLine(id);
            CHECK(line != nullptr);
            CHECK(
                line->role() ==
                sketch::EntityRole::construction);
        }
    }

#if defined(_WIN32)
    const auto durable_before_failed_save = readText(path);

    properties = session.document().properties();
    properties.description = "This change must remain in memory after save failure";
    CHECK(session.execute(
              application::SetDocumentPropertiesCommand{properties})
              .changed);
    CHECK(session.needsSave());
    const auto failed_save_state =
        session.document().state();
    const auto failed_save_revision =
        session.document().revision();
    const auto failed_save_undo =
        session.undoDepth();
    const auto failed_save_redo =
        session.redoDepth();
    const auto failed_save_checkpoint =
        session.fileCheckpoint();
    CHECK(failed_save_checkpoint.has_value());

    HANDLE locked = ::CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);
    CHECK(locked != INVALID_HANDLE_VALUE);

    const auto failed_save = session.save();
    CHECK(!failed_save.ok());
    CHECK(failed_save.diagnostic.code ==
          application::DocumentSessionErrorCode::persistence_failure);
    CHECK(session.needsSave());
    CHECK(
        session.document().state() ==
        failed_save_state);
    CHECK(
        session.document().revision() ==
        failed_save_revision);
    CHECK(
        session.undoDepth() ==
        failed_save_undo);
    CHECK(
        session.redoDepth() ==
        failed_save_redo);
    CHECK(
        session.fileCheckpoint() ==
        failed_save_checkpoint);

    CHECK(::CloseHandle(locked) != 0);
    CHECK(readText(path) == durable_before_failed_save);

    auto persisted_after_failure = store.load(path);
    CHECK(persisted_after_failure.ok());
    CHECK(persisted_after_failure.document->properties().description.empty());
    CHECK(session.document().properties().description ==
          "This change must remain in memory after save failure");
#endif

    return EXIT_SUCCESS;
}
