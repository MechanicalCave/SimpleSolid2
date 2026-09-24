#include "cad_workbench.hpp"
#include "open_document_dialog.hpp"
#include "workspace_location_dialog.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QApplication>
#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QTabBar>
#include <QTimer>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

bool expect(bool value, const char* expression, int line) {
    if (value) return true;
    std::cerr
        << "WB-01A Workbench dialog integration CHECK failed at line "
        << line << ": " << expression << '\n';
    return false;
}

#define EXPECT(expr) \
    do { if (!expect(static_cast<bool>(expr), #expr, __LINE__)) return EXIT_FAILURE; } while(false)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_workbench_document_dialog_" +
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

bool completeNewPartDialog() {
    auto* modal =
        QApplication::activeModalWidget();
    auto* dialog =
        dynamic_cast<ui::WorkspaceLocationDialog*>(
            modal);
    if (dialog == nullptr) {
        if (auto* unexpected =
                dynamic_cast<QDialog*>(modal)) {
            unexpected->reject();
        }
        return false;
    }

    auto* file_name =
        dialog->findChild<QLineEdit*>(
            QStringLiteral("documentFileNameEdit"));
    auto* create =
        dialog->findChild<QPushButton*>(
            QStringLiteral("createDocumentButton"));
    if (file_name == nullptr || create == nullptr) {
        dialog->reject();
        return false;
    }

    file_name->setText(
        QStringLiteral("CreatedByDialog.ss2part"));
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
        dynamic_cast<ui::OpenDocumentDialog*>(
            modal);
    if (dialog == nullptr) {
        if (auto* unexpected =
                dynamic_cast<QDialog*>(modal)) {
            unexpected->reject();
        }
        return false;
    }

    auto* list =
        dialog->findChild<QTreeWidget*>(
            QStringLiteral("openDocumentList"));
    auto* open =
        dialog->findChild<QPushButton*>(
            QStringLiteral("openDocumentConfirmButton"));
    if (list == nullptr || open == nullptr) {
        dialog->reject();
        return false;
    }

    QTreeWidgetItem* target = nullptr;
    for (int index = 0;
         index < list->topLevelItemCount();
         ++index) {
        auto* item = list->topLevelItem(index);
        if (item != nullptr &&
            item->text(1) == document_name) {
            target = item;
            break;
        }
    }

    if (target == nullptr ||
        (target->flags() & Qt::ItemIsEnabled) == 0) {
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

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace =
        temp.path / "Project";
    std::filesystem::create_directories(
        workspace);

    application::ProjectWorkspaceMetadataService metadata;
    EXPECT(
        metadata.initialize(
                    workspace,
                    "Machine")
            .ok());

    auto opened =
        application::ProjectSession::open(
            workspace);
    EXPECT(opened.ok());

    auto existing =
        opened.session->createPart(
            "Existing.ss2part");
    EXPECT(existing.ok());
    const auto existing_id =
        existing.session->documentId();
    EXPECT(
        opened.session->closeDocument(
            existing_id));

    ui::CadWorkbench workbench;
    workbench.setProjectSession(
        &*opened.session);

    auto* tabs =
        workbench.findChild<QTabBar*>(
            QStringLiteral("documentTabs"));

    EXPECT(tabs != nullptr);

    bool new_dialog_ok = false;
    QTimer::singleShot(
        0,
        &app,
        [&] {
            new_dialog_ok =
                completeNewPartDialog();
        });

    EXPECT(workbench.createPartInteractive(&workbench));

    EXPECT(new_dialog_ok);
    EXPECT(
        std::filesystem::is_regular_file(
            workspace /
            "CreatedByDialog.ss2part"));
    EXPECT(
        opened.session
            ->openDocumentIds()
            .size() == 1U);
    EXPECT(tabs->count() == 1);

    bool open_dialog_ok = false;
    QTimer::singleShot(
        0,
        &app,
        [&] {
            open_dialog_ok =
                completeOpenDialog(
                    QStringLiteral("Existing"));
        });

    EXPECT(workbench.openDocumentInteractive(&workbench));

    EXPECT(open_dialog_ok);
    EXPECT(
        opened.session->documentSession(
            existing_id) != nullptr);
    EXPECT(
        opened.session
            ->openDocumentIds()
            .size() == 2U);
    EXPECT(tabs->count() == 2);

    bool reuse_dialog_ok = false;
    QTimer::singleShot(
        0,
        &app,
        [&] {
            reuse_dialog_ok =
                completeOpenDialog(
                    QStringLiteral("Existing"));
        });

    open_document->click();

    EXPECT(reuse_dialog_ok);
    EXPECT(
        opened.session
            ->openDocumentIds()
            .size() == 2U);
    EXPECT(tabs->count() == 2);
    EXPECT(
        workbench.activeDocumentId()
            .has_value());
    EXPECT(
        *workbench.activeDocumentId() ==
        existing_id);

    return EXIT_SUCCESS;
}
