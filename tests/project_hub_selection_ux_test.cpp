#include "project_hub_window.hpp"
#include "project_hub_selection.hpp"
#include "workspace_location_dialog.hpp"

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/application/recent_project_store.hpp>

#include <QApplication>
#include <QDialog>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>

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

bool completeNewPartDialog(
    const QString& file_name) {
    auto* modal =
        QApplication::activeModalWidget();
    auto* dialog =
        dynamic_cast<WorkspaceLocationDialog*>(
            modal);
    if (dialog == nullptr) {
        if (auto* unexpected =
                dynamic_cast<QDialog*>(modal)) {
            unexpected->reject();
        }
        return false;
    }

    auto* file_edit =
        dialog->findChild<QLineEdit*>(
            QStringLiteral("documentFileNameEdit"));
    auto* create =
        dialog->findChild<QPushButton*>(
            QStringLiteral("createDocumentButton"));
    if (file_edit == nullptr ||
        create == nullptr) {
        dialog->reject();
        return false;
    }

    file_edit->setText(file_name);
    QApplication::processEvents();
    if (!create->isEnabled()) {
        dialog->reject();
        return false;
    }

    create->click();
    return true;
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
    CHECK(simplesolid2::ui::internal::selectedRecentProjectId(*list).empty());
    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());

    auto* item = list->item(0);
    CHECK(item != nullptr);

    list->setCurrentItem(item, QItemSelectionModel::NoUpdate);
    QApplication::processEvents();

    CHECK(list->currentItem() == item);
    CHECK(list->selectedItems().empty());
    CHECK(simplesolid2::ui::internal::selectedRecentProjectId(*list).empty());
    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());

    item->setSelected(true);
    QApplication::processEvents();

    CHECK(list->selectedItems().size() == 1);
    CHECK(list->selectedItems().front() == item);
    CHECK(!simplesolid2::ui::internal::selectedRecentProjectId(*list).empty());
    CHECK(open->isEnabled());
    CHECK(locate->isEnabled());
    CHECK(remove->isEnabled());

    list->clearSelection();
    QApplication::processEvents();

    CHECK(list->selectedItems().empty());
    CHECK(list->currentItem() == item);
    CHECK(simplesolid2::ui::internal::selectedRecentProjectId(*list).empty());
    CHECK(!open->isEnabled());
    CHECK(!locate->isEnabled());
    CHECK(!remove->isEnabled());
}

void verifyNeutralWorkspaceAndCloseLifecycle(
    const std::filesystem::path& root) {
    std::filesystem::create_directories(root);

    const auto workspace =
        root / "Project";
    const auto catalog =
        root / "state" / "recent-projects-v1.txt";
    seedRecentProject(workspace, catalog);

    ProjectHubWindow window{catalog};
    window.show();
    QApplication::processEvents();

    auto* pages =
        window.findChild<QStackedWidget*>(
            "applicationPages");
    auto* recent =
        window.findChild<QListWidget*>(
            "recentProjectsList");
    auto* open_recent =
        window.findChild<QPushButton*>(
            "openRecentButton");

    CHECK(pages != nullptr);
    CHECK(recent != nullptr);
    CHECK(open_recent != nullptr);
    CHECK(recent->count() == 1);

    auto* recent_item = recent->item(0);
    CHECK(recent_item != nullptr);
    recent_item->setSelected(true);
    QApplication::processEvents();
    CHECK(open_recent->isEnabled());

    open_recent->click();
    QApplication::processEvents();

    auto* workspace_host =
        window.findChild<QStackedWidget*>(
            "workspaceContentHost");
    auto* dashboard =
        window.findChild<QWidget*>(
            "workspaceDashboard");
    auto* cad_workbench =
        window.findChild<QWidget*>(
            "cadWorkbench");
    auto* new_part =
        window.findChild<QPushButton*>(
            "workspaceNewPartButton");
    auto* close_document =
        window.findChild<QPushButton*>(
            "closeDocumentButton");
    auto* close_project =
        window.findChild<QPushButton*>(
            "closeProjectButton");
    auto* tabs =
        window.findChild<QTabBar*>(
            "documentTabs");

    CHECK(
        pages->currentWidget()->objectName() ==
        QStringLiteral("workspacePage"));
    CHECK(workspace_host != nullptr);
    CHECK(dashboard != nullptr);
    CHECK(cad_workbench != nullptr);
    CHECK(new_part != nullptr);
    CHECK(close_document != nullptr);
    CHECK(close_project != nullptr);
    CHECK(tabs != nullptr);
    CHECK(
        workspace_host->currentWidget() ==
        dashboard);
    CHECK(tabs->count() == 0);
    CHECK(!tabs->isVisible());

    bool first_dialog_ok = false;
    QTimer::singleShot(
        0,
        &window,
        [&] {
            first_dialog_ok =
                completeNewPartDialog(
                    QStringLiteral(
                        "WorkspacePart001.ss2part"));
        });
    new_part->click();
    CHECK(first_dialog_ok);
    QApplication::processEvents();

    CHECK(tabs->count() == 1);
    CHECK(tabs->isVisible());
    CHECK(
        workspace_host->currentWidget() ==
        cad_workbench);

    // Closing the last active Document must detach the Workbench
    // before ProjectSession destroys the DocumentSession, then
    // return to neutral Workspace.
    close_document->click();
    QApplication::processEvents();

    CHECK(tabs->count() == 0);
    CHECK(!tabs->isVisible());
    CHECK(
        workspace_host->currentWidget() ==
        dashboard);

    bool second_dialog_ok = false;
    QTimer::singleShot(
        0,
        &window,
        [&] {
            second_dialog_ok =
                completeNewPartDialog(
                    QStringLiteral(
                        "WorkspacePart002.ss2part"));
        });
    new_part->click();
    CHECK(second_dialog_ok);
    QApplication::processEvents();
    CHECK(tabs->count() == 1);
    CHECK(
        workspace_host->currentWidget() ==
        cad_workbench);

    // Project close has the same lifetime rule at the larger
    // ownership boundary: detach Workbench before ProjectSession
    // destruction.
    close_project->click();
    QApplication::processEvents();

    CHECK(
        pages->currentWidget()->objectName() ==
        QStringLiteral("projectHubPage"));
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};
    TempDirectory temp;
    verifySelectionDrivesRecentActions(
        temp.path / "selection");
    verifyNeutralWorkspaceAndCloseLifecycle(
        temp.path / "workspace");
    return EXIT_SUCCESS;
}
