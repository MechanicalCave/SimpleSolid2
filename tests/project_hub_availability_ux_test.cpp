#include "project_hub_selection.hpp"
#include "project_hub_window.hpp"

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/application/recent_project_store.hpp>

#include <QApplication>
#include <QListWidget>
#include <QPushButton>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2::application;
using namespace simplesolid2::ui;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PH-02B availability UX CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph02b_availability_ux_" +
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

void seedRecentProject(
    const std::filesystem::path& workspace,
    const std::filesystem::path& catalog) {
    std::filesystem::create_directories(workspace);

    ProjectWorkspaceMetadataService metadata_service;
    const auto created =
        metadata_service.initialize(workspace, "Moved Project");
    CHECK(created.ok());

    auto opened = ProjectSession::open(workspace);
    CHECK(opened.ok());
    CHECK(opened.session.has_value());

    RecentProjectStore recent{catalog};
    CHECK(recent.recordOpened(*opened.session).ok());
}

void verifyMissingWorkspacePresentation(
    const std::filesystem::path& root) {
    const auto workspace = root / "Original";
    const auto moved = root / "Moved";
    const auto catalog = root / "state" / "recent-projects-v1.txt";

    seedRecentProject(workspace, catalog);
    const auto remembered_workspace =
        std::filesystem::weakly_canonical(workspace);
    std::filesystem::rename(workspace, moved);

    ProjectHubWindow window{catalog};
    window.show();
    QApplication::processEvents();

    auto* list =
        window.findChild<QListWidget*>("recentProjectsList");
    auto* open =
        window.findChild<QPushButton*>("openRecentButton");
    auto* locate =
        window.findChild<QPushButton*>("locateRecentButton");
    auto* remove =
        window.findChild<QPushButton*>("removeRecentButton");

    CHECK(list != nullptr);
    CHECK(open != nullptr);
    CHECK(locate != nullptr);
    CHECK(remove != nullptr);
    CHECK(list->count() == 1);

    auto* item = list->item(0);
    CHECK(item != nullptr);
    CHECK(item->text().contains(QStringLiteral("Workspace not found")));
    CHECK(item->toolTip().contains(QStringLiteral("Locate")));
    CHECK(item->data(internal::recentOpenableRole).toBool() == false);
    CHECK(item->data(internal::recentAvailabilityRole).toInt() ==
          static_cast<int>(
              simplesolid2::application::internal::
                  RecentProjectAvailability::workspace_missing));

    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());

    item->setSelected(true);
    QApplication::processEvents();

    CHECK(!open->isEnabled());
    CHECK(locate->isEnabled());
    CHECK(remove->isEnabled());
    CHECK(!internal::selectedRecentCanOpen(*list));

    RecentProjectStore recent{catalog};
    const auto persisted = recent.list();
    CHECK(persisted.ok());
    CHECK(persisted.entries.size() == 1U);
    CHECK(persisted.entries[0].workspace_root == remembered_workspace);
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    TempDirectory temp;
    verifyMissingWorkspacePresentation(temp.path);
    return EXIT_SUCCESS;
}
