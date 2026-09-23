#include "project_hub_window.hpp"
#include "project_hub_selection.hpp"

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/application/recent_project_store.hpp>

#include <QApplication>
#include <QItemSelectionModel>
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
        std::cerr << "PH-02A selection UX CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_ph02a_selection_" +
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
    const auto created = metadata_service.initialize(workspace, "Selection Test");
    CHECK(created.ok());

    auto opened = ProjectSession::open(workspace);
    CHECK(opened.ok());
    CHECK(opened.session.has_value());

    RecentProjectStore recent{catalog};
    const auto recorded = recent.recordOpened(*opened.session);
    CHECK(recorded.ok());
}

void verifySelectionDrivesRecentActions(
    const std::filesystem::path& root) {
    const auto workspace = root / "Project";
    const auto catalog = root / "state" / "recent-projects-v1.txt";
    seedRecentProject(workspace, catalog);

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
    CHECK(list->selectedItems().empty());
    CHECK(list->currentItem() == nullptr);
    CHECK(internal::selectedRecentProjectId(*list).empty());
    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());

    auto* item = list->item(0);
    CHECK(item != nullptr);

    list->setCurrentItem(item, QItemSelectionModel::NoUpdate);
    QApplication::processEvents();

    CHECK(list->currentItem() == item);
    CHECK(list->selectedItems().empty());
    CHECK(internal::selectedRecentProjectId(*list).empty());
    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());

    item->setSelected(true);
    QApplication::processEvents();

    CHECK(list->selectedItems().size() == 1);
    CHECK(list->selectedItems().front() == item);
    CHECK(!internal::selectedRecentProjectId(*list).empty());
    CHECK(open->isEnabled());
    CHECK(locate->isEnabled());
    CHECK(remove->isEnabled());

    list->clearSelection();
    QApplication::processEvents();

    CHECK(list->selectedItems().empty());
    CHECK(list->currentItem() == item);
    CHECK(internal::selectedRecentProjectId(*list).empty());
    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    TempDirectory temp;
    verifySelectionDrivesRecentActions(temp.path);
    return EXIT_SUCCESS;
}
