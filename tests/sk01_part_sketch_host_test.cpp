#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/part_sketch.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-01 Part Sketch host CHECK failed at line "
            << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr)     check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_sk01_host_" +
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
    CHECK(part::isSketchOriginPlane(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(part::isSketchOriginPlane(
        core::BuiltinReferenceRole::xz_plane));
    CHECK(part::isSketchOriginPlane(
        core::BuiltinReferenceRole::yz_plane));

    CHECK(!part::isSketchOriginPlane(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!part::isSketchOriginPlane(
        core::BuiltinReferenceRole::x_axis));
    CHECK(!part::isSketchOriginPlane(
        core::BuiltinReferenceRole::y_axis));
    CHECK(!part::isSketchOriginPlane(
        core::BuiltinReferenceRole::z_axis));

    for (const auto role : {
             core::BuiltinReferenceRole::xy_plane,
             core::BuiltinReferenceRole::xz_plane,
             core::BuiltinReferenceRole::yz_plane}) {
        const auto support =
            part::partSketchSupportForBuiltinPlane(role);
        CHECK(support.has_value());

        const auto placement =
            part::sketchPlacementForSupport(*support);
        CHECK(placement.has_value());
        CHECK(placement->valid());
        CHECK(part::sketchPlacementMatchesSupport(
            *placement,
            *support));
    }

    TempDirectory temp;
    const auto path =
        temp.path / "SketchHost.ss2part";

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    part::PartDocumentStore store;
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{
        path,
        std::move(document)};

    const auto initial_revision =
        session.document().revision().value();

    const auto invalid =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::x_axis});
    CHECK(!invalid.ok());
    CHECK(!invalid.changed);
    CHECK(!invalid.sketch_id.has_value());
    CHECK(
        invalid.diagnostic.code ==
        application::DocumentSessionErrorCode::
            invalid_command);
    CHECK(
        session.document().revision().value() ==
        initial_revision);
    CHECK(session.document().sketches().empty());
    CHECK(!session.needsSave());

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xz_plane});
    CHECK(created.ok());
    CHECK(created.changed);
    CHECK(created.sketch_id.has_value());
    CHECK(session.document().sketches().size() == 1U);
    CHECK(session.needsSave());
    CHECK(session.canUndo());
    CHECK(session.undoDepth() == 1U);

    const auto created_id =
        *created.sketch_id;
    const auto* sketch =
        session.document().findSketch(created_id);
    CHECK(sketch != nullptr);
    CHECK(
        sketch->support.builtin_plane ==
        core::BuiltinReferenceRole::xz_plane);
    CHECK(
        part::sketchPlacementMatchesSupport(
            sketch->placement,
            sketch->support));
    CHECK(sketch->visible);

    const auto expected_placement =
        sketch->placement;

    CHECK(session.undo().changed);
    CHECK(session.document().sketches().empty());
    CHECK(session.document().findSketch(created_id) == nullptr);
    CHECK(!session.needsSave());
    CHECK(session.canRedo());

    CHECK(session.redo().changed);
    CHECK(session.document().sketches().size() == 1U);
    const auto* redone =
        session.document().findSketch(created_id);
    CHECK(redone != nullptr);
    CHECK(redone->placement == expected_placement);
    CHECK(redone->support.builtin_plane ==
          core::BuiltinReferenceRole::xz_plane);

    const auto second =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xz_plane});
    CHECK(second.ok());
    CHECK(second.sketch_id.has_value());
    CHECK(*second.sketch_id != created_id);
    CHECK(session.document().sketches().size() == 2U);

    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(loaded.document->sketches().size() == 2U);

    const auto* loaded_first =
        loaded.document->findSketch(created_id);
    CHECK(loaded_first != nullptr);
    CHECK(
        loaded_first->support.builtin_plane ==
        core::BuiltinReferenceRole::xz_plane);
    CHECK(loaded_first->placement == expected_placement);
    CHECK(loaded_first->visible);

    return EXIT_SUCCESS;
}
