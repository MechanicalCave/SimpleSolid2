#include "project_hub_controller.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 dirty close CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;
    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_dirty_close_" +
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

} // namespace

int main() {
    TempDirectory temp;
    const auto workspace = temp.path / "Project";
    std::filesystem::create_directories(workspace);

    application::ProjectWorkspaceMetadataService metadata;
    CHECK(metadata.initialize(workspace, "Machine").ok());

    application::internal::ProjectHubController controller{
        temp.path / "recent-projects-v1.txt"};
    CHECK(controller.openProject(workspace).ok());

    auto* project = controller.activeSession();
    CHECK(project != nullptr);

    auto created = project->createPart("Part001.ss2part");
    CHECK(created.ok());
    const auto id = created.session->documentId();

    core::DocumentProperties properties;
    properties.title = "Unsaved title";
    CHECK(created.session
              ->execute(application::SetDocumentPropertiesCommand{properties})
              .changed);
    CHECK(project->hasDirtyDocuments());

    CHECK(!controller.closeProject());
    CHECK(controller.hasActiveProject());

    const auto save_all = project->saveAllDirtyDocuments();
    CHECK(save_all.ok());
    CHECK(!project->hasDirtyDocuments());
    CHECK(controller.closeProject());
    CHECK(!controller.hasActiveProject());

    CHECK(controller.openProject(workspace).ok());
    project = controller.activeSession();
    CHECK(project != nullptr);

    auto reopened = project->openDocument(id);
    CHECK(reopened.ok());
    CHECK(reopened.session->document().properties().title == "Unsaved title");

    properties.title = "Discard me";
    CHECK(reopened.session
              ->execute(application::SetDocumentPropertiesCommand{properties})
              .changed);
    CHECK(project->hasDirtyDocuments());

    CHECK(controller.closeProject(true));
    CHECK(!controller.hasActiveProject());

    CHECK(controller.openProject(workspace).ok());
    project = controller.activeSession();
    CHECK(project != nullptr);
    auto after_discard = project->openDocument(id);
    CHECK(after_discard.ok());
    CHECK(after_discard.session->document().properties().title == "Unsaved title");

    return EXIT_SUCCESS;
}
