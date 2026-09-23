#include "part_workspace_panel.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QApplication>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 workspace panel CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;
    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_part_panel_" +
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
    auto created = opened.session->createPart("Part001.ss2part");
    CHECK(created.ok());
    const auto id = created.session->documentId();
    CHECK(opened.session->closeDocument(id));

    ui::PartWorkspacePanel panel;
    panel.setProjectSession(&*opened.session);

    auto* list = panel.findChild<QListWidget*>(QStringLiteral("documentList"));
    auto* open =
        panel.findChild<QPushButton*>(QStringLiteral("openDocumentButton"));
    auto* title =
        panel.findChild<QLineEdit*>(QStringLiteral("documentTitleEdit"));
    auto* apply = panel.findChild<QPushButton*>(
        QStringLiteral("applyDocumentPropertiesButton"));
    auto* undo =
        panel.findChild<QPushButton*>(QStringLiteral("undoDocumentButton"));
    auto* redo =
        panel.findChild<QPushButton*>(QStringLiteral("redoDocumentButton"));
    auto* save =
        panel.findChild<QPushButton*>(QStringLiteral("saveDocumentButton"));
    auto* close =
        panel.findChild<QPushButton*>(QStringLiteral("closeDocumentButton"));

    CHECK(list != nullptr);
    CHECK(open != nullptr);
    CHECK(title != nullptr);
    CHECK(apply != nullptr);
    CHECK(undo != nullptr);
    CHECK(redo != nullptr);
    CHECK(save != nullptr);
    CHECK(close != nullptr);

    CHECK(list->count() == 1);
    CHECK(!open->isEnabled());
    CHECK(!title->isEnabled());

    list->setCurrentRow(0);
    CHECK(open->isEnabled());
    open->click();

    CHECK(title->isEnabled());
    CHECK(opened.session->documentSession(id) != nullptr);
    CHECK(!opened.session->documentSession(id)->needsSave());

    title->setText(QStringLiteral("Drive Shaft"));
    apply->click();
    CHECK(opened.session->documentSession(id)->needsSave());
    CHECK(opened.session->documentSession(id)->document().properties().title ==
          "Drive Shaft");
    CHECK(undo->isEnabled());

    undo->click();
    CHECK(opened.session->documentSession(id)->document().properties().title.empty());
    CHECK(redo->isEnabled());

    redo->click();
    CHECK(opened.session->documentSession(id)->document().properties().title ==
          "Drive Shaft");
    CHECK(save->isEnabled());

    save->click();
    CHECK(!opened.session->documentSession(id)->needsSave());
    CHECK(!save->isEnabled());

    close->click();
    CHECK(opened.session->documentSession(id) == nullptr);
    CHECK(!title->isEnabled());

    return EXIT_SUCCESS;
}
