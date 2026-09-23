#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>

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
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{path, std::move(document)};
    CHECK(!session.needsSave());
    CHECK(!session.canUndo());
    CHECK(!session.canRedo());

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

    return EXIT_SUCCESS;
}
