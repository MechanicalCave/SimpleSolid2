#include "open_document_dialog.hpp"
#include "workspace_location_dialog.hpp"

#include <simplesolid2/core/document.hpp>

#include <QApplication>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTest>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace simplesolid2;

namespace {

bool expect(bool value, const char* expression, int line) {
    if (value) return true;
    std::cerr
        << "WB-01A document dialog CHECK failed at line "
        << line << ": " << expression << '\n';
    return false;
}

#define EXPECT(expr) \
    do { if (!expect(static_cast<bool>(expr), #expr, __LINE__)) return EXIT_FAILURE; } while(false)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_document_dialog_" +
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

QTreeWidgetItem* findItem(
    QTreeWidgetItem* root,
    const QString& text) {
    if (root == nullptr) return nullptr;
    if (root->text(0) == text) return root;

    for (int index = 0; index < root->childCount(); ++index) {
        if (auto* found = findItem(root->child(index), text)) {
            return found;
        }
    }
    return nullptr;
}

bool treeContains(
    QTreeWidget& tree,
    const QString& text) {
    for (int index = 0; index < tree.topLevelItemCount(); ++index) {
        if (findItem(tree.topLevelItem(index), text) != nullptr) {
            return true;
        }
    }
    return false;
}

QTreeWidgetItem* findTopLevelRow(
    QTreeWidget& tree,
    const QString& name) {
    for (int index = 0; index < tree.topLevelItemCount(); ++index) {
        auto* item = tree.topLevelItem(index);
        if (item != nullptr && item->text(1) == name) {
            return item;
        }
    }
    return nullptr;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace = temp.path / "Project";
    std::filesystem::create_directories(
        workspace / ".simplesolid" / "internal");
    std::filesystem::create_directories(
        workspace / "Parts");

    ui::WorkspaceLocationDialog location{
        workspace,
        QStringLiteral("Part"),
        QStringLiteral(".ss2part"),
        QStringLiteral("Part001.ss2part")};

    auto* folder_tree =
        location.findChild<QTreeWidget*>(
            QStringLiteral("workspaceFolderTree"));
    auto* folder_name =
        location.findChild<QLineEdit*>(
            QStringLiteral("newFolderNameEdit"));
    auto* create_folder =
        location.findChild<QPushButton*>(
            QStringLiteral("createWorkspaceFolderButton"));
    auto* file_name =
        location.findChild<QLineEdit*>(
            QStringLiteral("documentFileNameEdit"));
    auto* create_document =
        location.findChild<QPushButton*>(
            QStringLiteral("createDocumentButton"));

    EXPECT(folder_tree != nullptr);
    EXPECT(folder_name != nullptr);
    EXPECT(create_folder != nullptr);
    EXPECT(file_name != nullptr);
    EXPECT(create_document != nullptr);

    EXPECT(folder_tree->topLevelItemCount() == 1);
    EXPECT(treeContains(*folder_tree, QStringLiteral("Parts")));
    EXPECT(!treeContains(*folder_tree, QStringLiteral(".simplesolid")));

    auto* parts =
        findItem(
            folder_tree->topLevelItem(0),
            QStringLiteral("Parts"));
    EXPECT(parts != nullptr);

    folder_tree->setCurrentItem(parts);
    folder_name->setText(
        QString::fromUtf8("Łożyska"));
    QApplication::processEvents();
    EXPECT(create_folder->isEnabled());

    create_folder->click();
    QApplication::processEvents();

    EXPECT(std::filesystem::is_directory(
        workspace /
        "Parts" /
        std::filesystem::path{L"Łożyska"}));

    auto* created_folder =
        findItem(
            folder_tree->topLevelItem(0),
            QString::fromUtf8("Łożyska"));
    EXPECT(created_folder != nullptr);
    EXPECT(folder_tree->currentItem() == created_folder);

    file_name->setText(QStringLiteral("Shaft"));
    QApplication::processEvents();
    EXPECT(create_document->isEnabled());

    auto selected =
        location.selectedRelativeFilePath();
    EXPECT(selected.has_value());
    EXPECT(
        selected->extension() ==
        std::filesystem::path{".ss2part"});
    EXPECT(
        selected->filename() ==
        std::filesystem::path{"Shaft.ss2part"});

    file_name->setText(
        QStringLiteral("../Outside.ss2part"));
    QApplication::processEvents();
    EXPECT(!create_document->isEnabled());
    EXPECT(!location.selectedRelativeFilePath().has_value());

    file_name->setText(
        QStringLiteral("Wrong.step"));
    QApplication::processEvents();
    EXPECT(!create_document->isEnabled());

    const auto existing =
        workspace / "Parts" /
        std::filesystem::path{L"Łożyska"} /
        "Existing.ss2part";
    {
        std::ofstream output{existing, std::ios::binary};
        output << "existing";
    }

    file_name->setText(
        QStringLiteral("Existing"));
    QApplication::processEvents();
    EXPECT(!create_document->isEnabled());
    EXPECT(!location.selectedRelativeFilePath().has_value());

    file_name->setText(
        QStringLiteral("Fresh.ss2part"));
    QApplication::processEvents();
    EXPECT(create_document->isEnabled());

    const auto resolved_id = core::DocumentId::generate();
    const auto conflict_id = core::DocumentId::generate();

    std::vector<ui::OpenDocumentCandidate> candidates;
    candidates.push_back(
        ui::OpenDocumentCandidate{
            QStringLiteral("Part"),
            QStringLiteral("Shaft"),
            QStringLiteral("Parts/Shaft.ss2part"),
            QStringLiteral("Ready"),
            QStringLiteral("Resolved native Part"),
            resolved_id,
            true});
    candidates.push_back(
        ui::OpenDocumentCandidate{
            QStringLiteral("Part"),
            QStringLiteral("Conflict"),
            QStringLiteral(
                "A.ss2part\nB.ss2part"),
            QStringLiteral("Identity conflict"),
            QStringLiteral("Duplicate DocumentId"),
            conflict_id,
            false});
    candidates.push_back(
        ui::OpenDocumentCandidate{
            QStringLiteral("Part"),
            QStringLiteral("Broken"),
            QStringLiteral("Broken.ss2part"),
            QStringLiteral("Invalid Part"),
            QStringLiteral("Unsupported native file"),
            std::nullopt,
            false});

    ui::OpenDocumentDialog open{
        std::move(candidates)};

    auto* document_list =
        open.findChild<QTreeWidget*>(
            QStringLiteral("openDocumentList"));
    auto* open_button =
        open.findChild<QPushButton*>(
            QStringLiteral("openDocumentConfirmButton"));

    EXPECT(document_list != nullptr);
    EXPECT(open_button != nullptr);
    EXPECT(document_list->columnCount() == 4);
    EXPECT(document_list->topLevelItemCount() == 3);
    EXPECT(!open_button->isEnabled());

    auto* resolved =
        findTopLevelRow(
            *document_list,
            QStringLiteral("Shaft"));
    auto* conflict =
        findTopLevelRow(
            *document_list,
            QStringLiteral("Conflict"));
    auto* invalid =
        findTopLevelRow(
            *document_list,
            QStringLiteral("Broken"));

    EXPECT(resolved != nullptr);
    EXPECT(conflict != nullptr);
    EXPECT(invalid != nullptr);

    EXPECT(resolved->text(0) == QStringLiteral("Part"));
    EXPECT(resolved->text(3) == QStringLiteral("Ready"));
    EXPECT(
        (conflict->flags() & Qt::ItemIsEnabled) == 0);
    EXPECT(
        (invalid->flags() & Qt::ItemIsEnabled) == 0);

    document_list->setCurrentItem(resolved);
    resolved->setSelected(true);
    QApplication::processEvents();

    EXPECT(open_button->isEnabled());
    const auto selected_id =
        open.selectedDocumentId();
    EXPECT(selected_id.has_value());
    EXPECT(*selected_id == resolved_id);

    open_button->click();
    QApplication::processEvents();
    EXPECT(open.result() == QDialog::Accepted);

    return EXIT_SUCCESS;
}
