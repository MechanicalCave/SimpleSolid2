#include "open_document_dialog.hpp"
#include "project_hub_window.hpp"
#include "workspace_location_dialog.hpp"

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/application/recent_project_store.hpp>

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QMetaObject>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QTimer>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

bool expect(
    bool value,
    const char* expression,
    int line) {
    if (value) return true;

    std::cerr
        << "WS-01 Workspace dialog integration CHECK failed at line "
        << line << ": " << expression << '\n';
    return false;
}

#define EXPECT(expr) \
    do { \
        if (!expect( \
                static_cast<bool>(expr), \
                #expr, \
                __LINE__)) { \
            return EXIT_FAILURE; \
        } \
    } while (false)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_ws01_dialog_" +
             std::to_string(
                 std::filesystem::
                     file_time_type::clock::now()
                         .time_since_epoch()
                         .count()));
        std::filesystem::create_directories(
            path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(
            path,
            ec);
    }
};

bool completeNewPartDialog(
    const QString& file_name) {
    auto* modal =
        QApplication::activeModalWidget();
    auto* dialog =
        dynamic_cast<
            ui::WorkspaceLocationDialog*>(
                modal);
    if (dialog == nullptr) {
        if (auto* unexpected =
                dynamic_cast<QDialog*>(
                    modal)) {
            unexpected->reject();
        }
        return false;
    }

    auto* file_edit =
        dialog->findChild<QLineEdit*>(
            QStringLiteral(
                "documentFileNameEdit"));
    auto* create =
        dialog->findChild<QPushButton*>(
            QStringLiteral(
                "createDocumentButton"));
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

bool completeOpenDialog(
    const QString& document_name) {
    auto* modal =
        QApplication::activeModalWidget();
    auto* dialog =
        dynamic_cast<
            ui::OpenDocumentDialog*>(
                modal);
    if (dialog == nullptr) {
        if (auto* unexpected =
                dynamic_cast<QDialog*>(
                    modal)) {
            unexpected->reject();
        }
        return false;
    }

    auto* list =
        dialog->findChild<QTreeWidget*>(
            QStringLiteral(
                "openDocumentList"));
    auto* open =
        dialog->findChild<QPushButton*>(
            QStringLiteral(
                "openDocumentConfirmButton"));
    if (list == nullptr ||
        open == nullptr) {
        dialog->reject();
        return false;
    }

    QTreeWidgetItem* target = nullptr;
    for (int index = 0;
         index < list->topLevelItemCount();
         ++index) {
        auto* item =
            list->topLevelItem(index);
        if (item != nullptr &&
            item->text(1) ==
                document_name) {
            target = item;
            break;
        }
    }

    if (target == nullptr ||
        (target->flags() &
         Qt::ItemIsEnabled) == 0) {
        dialog->reject();
        return false;
    }

    list->setCurrentItem(target);
    target->setSelected(true);
    QApplication::processEvents();

    if (!open->isEnabled()) {
        dialog->reject();
        return false;
    }

    open->click();
    return true;
}

bool seedProject(
    const std::filesystem::path& workspace,
    const std::filesystem::path& catalog) {
    std::filesystem::create_directories(
        workspace);

    application::
        ProjectWorkspaceMetadataService metadata;
    if (!metadata.initialize(
            workspace,
            "Workspace Navigation")
            .ok()) {
        return false;
    }

    auto opened =
        application::ProjectSession::open(
            workspace);
    if (!opened.ok()) {
        return false;
    }

    auto existing =
        opened.session->createPart(
            "Existing.ss2part");
    if (!existing.ok()) {
        return false;
    }

    core::DocumentProperties properties;
    properties.title = "Existing";
    if (!existing.session
             ->execute(
                 application::
                     SetDocumentPropertiesCommand{
                         properties})
             .changed) {
        return false;
    }
    if (!existing.session->save().ok()) {
        return false;
    }

    application::RecentProjectStore recent{
        catalog};
    return recent.recordOpened(
        *opened.session)
        .ok();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace =
        temp.path / "Project";
    const auto catalog =
        temp.path /
        "state" /
        "recent-projects-v1.txt";

    EXPECT(seedProject(workspace, catalog));

    ui::ProjectHubWindow window{catalog};
    window.show();
    QApplication::processEvents();

    auto* recent =
        window.findChild<QListWidget*>(
            QStringLiteral(
                "recentProjectsList"));
    auto* open_recent =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "openRecentButton"));
    EXPECT(recent != nullptr);
    EXPECT(open_recent != nullptr);
    EXPECT(recent->count() == 1);

    recent->item(0)->setSelected(true);
    QApplication::processEvents();
    EXPECT(open_recent->isEnabled());
    open_recent->click();
    QApplication::processEvents();

    auto* tabs =
        window.findChild<QTabBar*>(
            QStringLiteral("documentTabs"));
    auto* workspace_button =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "workspaceHomeButton"));
    auto* new_part =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "workspaceNewPartButton"));
    auto* open_document =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "workspaceOpenDocumentButton"));
    auto* refresh =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "workspaceRefreshButton"));
    auto* content =
        window.findChild<QStackedWidget*>(
            QStringLiteral(
                "workspaceContentHost"));
    auto* dashboard =
        window.findChild<QWidget*>(
            QStringLiteral(
                "workspaceDashboard"));
    auto* workbench =
        window.findChild<QWidget*>(
            QStringLiteral(
                "cadWorkbench"));
    auto* toolbar =
        window.findChild<QWidget*>(
            QStringLiteral(
                "projectToolbar"));

    EXPECT(tabs != nullptr);
    EXPECT(workspace_button != nullptr);
    EXPECT(new_part != nullptr);
    EXPECT(open_document != nullptr);
    EXPECT(refresh != nullptr);
    EXPECT(content != nullptr);
    EXPECT(dashboard != nullptr);
    EXPECT(workbench != nullptr);
    EXPECT(toolbar != nullptr);

    EXPECT(content->currentWidget() == dashboard);
    EXPECT(tabs->count() == 0);
    EXPECT(!tabs->isVisible());
    EXPECT(toolbar->isVisible());
    EXPECT(new_part->isVisible());
    EXPECT(open_document->isVisible());
    EXPECT(refresh->isVisible());

    bool new_dialog_ok = false;
    QTimer::singleShot(
        0,
        &window,
        [&] {
            new_dialog_ok =
                completeNewPartDialog(
                    QStringLiteral(
                        "CreatedByDialog.ss2part"));
        });
    new_part->click();
    EXPECT(new_dialog_ok);
    QApplication::processEvents();

    EXPECT(
        std::filesystem::is_regular_file(
            workspace /
            "CreatedByDialog.ss2part"));
    EXPECT(tabs->count() == 1);
    EXPECT(tabs->isVisible());
    EXPECT(content->currentWidget() == workbench);
    EXPECT(toolbar->isVisible());

    auto* title_edit =
        window.findChild<QLineEdit*>(
            QStringLiteral(
                "documentTitleEdit"));
    auto* apply_properties =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "applyDocumentPropertiesButton"));
    auto* save_document =
        window.findChild<QPushButton*>(
            QStringLiteral(
                "saveDocumentButton"));
    EXPECT(title_edit != nullptr);
    EXPECT(apply_properties != nullptr);
    EXPECT(save_document != nullptr);

    title_edit->setText(
        QStringLiteral("Created Dirty"));
    apply_properties->click();
    QApplication::processEvents();
    EXPECT(
        tabs->tabText(
            tabs->currentIndex())
            .endsWith(
                QStringLiteral(" *")));

    save_document->click();
    QApplication::processEvents();
    EXPECT(
        !tabs->tabText(
            tabs->currentIndex())
             .endsWith(
                 QStringLiteral(" *")));

    // Project actions remain available while a Part Workbench is active.
    bool second_dialog_ok = false;
    QTimer::singleShot(
        0,
        &window,
        [&] {
            second_dialog_ok =
                completeNewPartDialog(
                    QStringLiteral(
                        "SecondPart.ss2part"));
        });
    new_part->click();
    EXPECT(second_dialog_ok);
    QApplication::processEvents();

    EXPECT(tabs->count() == 2);
    EXPECT(content->currentWidget() == workbench);

    bool open_dialog_ok = false;
    QTimer::singleShot(
        0,
        &window,
        [&] {
            open_dialog_ok =
                completeOpenDialog(
                    QStringLiteral("Existing"));
        });
    open_document->click();
    EXPECT(open_dialog_ok);
    QApplication::processEvents();

    EXPECT(tabs->count() == 3);
    EXPECT(content->currentWidget() == workbench);

    // Reopening an already open Document reuses its canonical tab/session.
    bool reuse_dialog_ok = false;
    QTimer::singleShot(
        0,
        &window,
        [&] {
            reuse_dialog_ok =
                completeOpenDialog(
                    QStringLiteral("Existing"));
        });
    open_document->click();
    EXPECT(reuse_dialog_ok);
    QApplication::processEvents();
    EXPECT(tabs->count() == 3);

    // Closing an inactive Project-level tab closes only that
    // DocumentSession and leaves the active Workbench context intact.
    const int active_before_close =
        tabs->currentIndex();
    EXPECT(active_before_close >= 0);
    const int inactive_index =
        active_before_close == 0
            ? 1
            : 0;
    EXPECT(
        QMetaObject::invokeMethod(
            tabs,
            "tabCloseRequested",
            Qt::DirectConnection,
            Q_ARG(int, inactive_index)));
    QApplication::processEvents();

    EXPECT(tabs->count() == 2);
    EXPECT(content->currentWidget() == workbench);
    EXPECT(tabs->currentIndex() >= 0);

    // Workspace is explicit navigation, not a tab-selection
    // sentinel and not a close operation. QTabBar may keep the last
    // current tab while the authoritative navigation context is
    // Workspace.
    const int remembered_tab =
        tabs->currentIndex();
    EXPECT(remembered_tab >= 0);

    workspace_button->click();
    QApplication::processEvents();

    EXPECT(content->currentWidget() == dashboard);
    EXPECT(tabs->count() == 2);
    EXPECT(tabs->currentIndex() == remembered_tab);
    EXPECT(new_part->isVisible());
    EXPECT(open_document->isVisible());

    // A Project-level Document Tab returns to its Document Workbench.
    const int target_tab =
        remembered_tab == 0
            ? 1
            : 0;
    tabs->setCurrentIndex(target_tab);
    QApplication::processEvents();

    EXPECT(content->currentWidget() == workbench);
    EXPECT(tabs->count() == 2);

    return EXIT_SUCCESS;
}
