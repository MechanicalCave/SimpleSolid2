#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 lifecycle CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;
    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_part_lifecycle_" +
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
    const auto workspace = temp.path / "Machine";
    std::filesystem::create_directories(workspace / "Parts");

    application::ProjectWorkspaceMetadataService metadata;
    CHECK(metadata.initialize(workspace, "Packaging Machine").ok());

    auto opened = application::ProjectSession::open(workspace);
    CHECK(opened.ok());

    auto created = opened.session->createPart(
        std::filesystem::path{"Parts"} / "Part001.ss2part");
    CHECK(created.ok());
    CHECK(!created.reused_session);
    const auto document_id = created.session->documentId();
    const std::string serialized_id{document_id.value()};
    CHECK(!created.session->needsSave());

    core::DocumentProperties properties;
    properties.number = "12-04-117";
    properties.title = "Drive Shaft";
    properties.description = "Main gearbox shaft";
    properties.engineering_revision = "A";
    CHECK(created.session
              ->execute(application::SetDocumentPropertiesCommand{properties})
              .changed);
    CHECK(created.session->needsSave());
    CHECK(created.session->save().ok());
    CHECK(!created.session->needsSave());

    auto reopened_same = opened.session->openDocument(document_id);
    CHECK(reopened_same.ok());
    CHECK(reopened_same.reused_session);
    CHECK(reopened_same.session == created.session);

    CHECK(opened.session->closeDocument(document_id));
    CHECK(opened.session->documentSession(document_id) == nullptr);

    auto reopened_document = opened.session->openDocument(document_id);
    CHECK(reopened_document.ok());
    CHECK(!reopened_document.reused_session);
    CHECK(reopened_document.session->document().properties() == properties);
    CHECK(!reopened_document.session->canUndo());
    CHECK(!reopened_document.session->canRedo());

    CHECK(opened.session->closeDocument(document_id));

    const auto old_path = workspace / "Parts" / "Part001.ss2part";
    const auto moved_path = workspace / "Parts" / "Shaft.ss2part";
    std::filesystem::rename(old_path, moved_path);
    CHECK(opened.session->refreshDocuments().ok());

    const auto resolution =
        opened.session->documentIndex().resolve(document_id);
    CHECK(resolution.ok());
    CHECK(resolution.absolute_path == std::filesystem::weakly_canonical(moved_path));

    opened.session.reset();

    auto after_restart = application::ProjectSession::open(workspace);
    CHECK(after_restart.ok());
    const auto reparsed_id = core::DocumentId::parse(serialized_id);
    CHECK(reparsed_id.has_value());
    auto after_restart_document =
        after_restart.session->openDocument(*reparsed_id);
    CHECK(after_restart_document.ok());
    CHECK(after_restart_document.session->documentId() == *reparsed_id);
    CHECK(after_restart_document.session->document().properties() == properties);
    CHECK(!after_restart_document.session->canUndo());

    CHECK(after_restart.session->closeDocument(*reparsed_id));

    const auto copied_path = workspace / "Parts" / "Shaft backup.ss2part";
    std::filesystem::copy_file(moved_path, copied_path);
    CHECK(after_restart.session->refreshDocuments().ok());

    const auto conflict =
        after_restart.session->documentIndex().resolve(*reparsed_id);
    CHECK(conflict.state ==
          application::DocumentResolutionState::identity_conflict);
    CHECK(conflict.relative_paths.size() == 2U);

    const auto blocked = after_restart.session->openDocument(*reparsed_id);
    CHECK(!blocked.ok());
    CHECK(blocked.diagnostic.code ==
          application::ProjectDocumentErrorCode::identity_conflict);
    CHECK(blocked.diagnostic.candidates.size() == 2U);

    return EXIT_SUCCESS;
}
