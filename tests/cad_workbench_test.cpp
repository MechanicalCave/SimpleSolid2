#include "cad_workbench.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabBar>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 CAD Workbench CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_cad_workbench_" +
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

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace = temp.path / "Project";
    std::filesystem::create_directories(workspace);

    application::ProjectWorkspaceMetadataService metadata;
    CHECK(metadata.initialize(workspace, "Machine").ok());

    auto opened = application::ProjectSession::open(workspace);
    CHECK(opened.ok());

    auto first = opened.session->createPart("Part001.ss2part");
    CHECK(first.ok());
    const auto first_id = first.session->documentId();

    core::DocumentProperties first_properties;
    first_properties.title = "Drive Shaft";
    CHECK(first.session
              ->execute(
                  application::SetDocumentPropertiesCommand{
                      first_properties})
              .changed);
    CHECK(first.session->save().ok());

    auto second = opened.session->createPart("Part002.ss2part");
    CHECK(second.ok());
    const auto second_id = second.session->documentId();

    core::DocumentProperties second_properties;
    second_properties.title = "Housing";
    CHECK(second.session
              ->execute(
                  application::SetDocumentPropertiesCommand{
                      second_properties})
              .changed);
    CHECK(second.session->save().ok());

    CHECK(opened.session->closeDocument(first_id));
    CHECK(opened.session->closeDocument(second_id));

    auto reopened_first =
        opened.session->openDocument(first_id);
    auto reopened_second =
        opened.session->openDocument(second_id);
    CHECK(reopened_first.ok());
    CHECK(reopened_second.ok());
    CHECK(opened.session->openDocumentIds().size() == 2U);

    ui::CadWorkbench workbench;
    workbench.setProjectSession(&*opened.session);

    auto* tabs =
        workbench.findChild<QTabBar*>(
            QStringLiteral("documentTabs"));
    auto* tree =
        workbench.findChild<QTreeWidget*>(
            QStringLiteral("documentTree"));
    auto* editor =
        workbench.findChild<QWidget*>(
            QStringLiteral("editorSurface"));
    auto* operations =
        workbench.findChild<QLabel*>(
            QStringLiteral("operationsPlaceholder"));
    auto* title =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("documentTitleEdit"));
    auto* apply =
        workbench.findChild<QPushButton*>(
            QStringLiteral("applyDocumentPropertiesButton"));
    auto* undo =
        workbench.findChild<QPushButton*>(
            QStringLiteral("undoDocumentButton"));
    auto* redo =
        workbench.findChild<QPushButton*>(
            QStringLiteral("redoDocumentButton"));
    auto* save =
        workbench.findChild<QPushButton*>(
            QStringLiteral("saveDocumentButton"));
    auto* show_references =
        workbench.findChild<QAction*>(
            QStringLiteral("showBuiltinReferencesAction"));
    auto* hide_references =
        workbench.findChild<QAction*>(
            QStringLiteral("hideBuiltinReferencesAction"));

    CHECK(tabs != nullptr);
    CHECK(tree != nullptr);
    CHECK(editor != nullptr);
    CHECK(operations != nullptr);
    CHECK(title != nullptr);
    CHECK(apply != nullptr);
    CHECK(undo != nullptr);
    CHECK(redo != nullptr);
    CHECK(save != nullptr);
    CHECK(show_references != nullptr);
    CHECK(hide_references != nullptr);

    CHECK(tabs->count() == 2);
    CHECK(operations->text().contains(
        QStringLiteral("No modeling operations")));

    CHECK(workbench.activateDocument(first_id));
    CHECK(workbench.activeDocumentId().has_value());
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(tabs->count() == 2);
    CHECK(title->text() == QStringLiteral("Drive Shaft"));

    CHECK(tree->topLevelItemCount() == 1);
    auto* root = tree->topLevelItem(0);
    CHECK(root != nullptr);
    CHECK(root->text(0) == QStringLiteral("Drive Shaft"));
    CHECK(root->childCount() == 1);

    auto* origin = root->child(0);
    CHECK(origin != nullptr);
    CHECK(origin->text(0) == QStringLiteral("Origin"));
    CHECK(origin->childCount() == 7);
    CHECK(tree->selectionMode() ==
          QAbstractItemView::ExtendedSelection);

    QTreeWidgetItem* xy_plane = nullptr;
    QTreeWidgetItem* origin_point = nullptr;
    for (int index = 0; index < origin->childCount(); ++index) {
        auto* item = origin->child(index);
        if (item->text(0) == QStringLiteral("XY Plane")) {
            xy_plane = item;
        } else if (
            item->text(0) == QStringLiteral("Origin Point")) {
            origin_point = item;
        }
    }

    CHECK(xy_plane != nullptr);
    CHECK(origin_point != nullptr);
    CHECK(xy_plane->font(0).italic());
    CHECK(!origin_point->font(0).italic());

    auto* first_session =
        opened.session->documentSession(first_id);
    CHECK(first_session != nullptr);
    CHECK(!first_session->canUndo());
    CHECK(!first_session->needsSave());

    tree->clearSelection();
    xy_plane->setSelected(true);
    origin_point->setSelected(true);
    CHECK(hide_references->isEnabled());
    CHECK(show_references->isEnabled());

    const auto before_visibility_revision =
        first_session->document().revision().value();
    hide_references->trigger();

    CHECK(first_session->document().revision().value() ==
          before_visibility_revision + 1U);
    CHECK(first_session->undoDepth() == 1U);
    CHECK(first_session->needsSave());
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(undo->isEnabled());
    CHECK(save->isEnabled());

    undo->click();
    CHECK(first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(!first_session->needsSave());

    title->setText(QStringLiteral("Drive Shaft Rev"));
    apply->click();

    CHECK(first_session->needsSave());
    CHECK(first_session->document().properties().title ==
          "Drive Shaft Rev");
    CHECK(undo->isEnabled());
    CHECK(save->isEnabled());

    undo->click();
    CHECK(first_session->document().properties().title ==
          "Drive Shaft");
    CHECK(!first_session->needsSave());
    CHECK(redo->isEnabled());

    redo->click();
    CHECK(first_session->document().properties().title ==
          "Drive Shaft Rev");
    CHECK(first_session->needsSave());

    save->click();
    CHECK(!first_session->needsSave());

    CHECK(workbench.activateDocument(second_id));
    CHECK(*workbench.activeDocumentId() == second_id);
    CHECK(title->text() == QStringLiteral("Housing"));
    CHECK(tree->topLevelItem(0)->text(0) ==
          QStringLiteral("Housing"));

    CHECK(workbench.activateDocument(first_id));
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(tabs->count() == 2);
    CHECK(title->text() == QStringLiteral("Drive Shaft Rev"));

    workbench.clearProjectSession();
    CHECK(!workbench.activeDocumentId().has_value());
    CHECK(tabs->count() == 0);
    CHECK(!title->isEnabled());

    return EXIT_SUCCESS;
}
