#pragma once

#include <simplesolid2/application/workspace_directory.hpp>

#include <QDialog>

#include <filesystem>
#include <optional>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QString;
class QTreeWidget;
class QTreeWidgetItem;

namespace simplesolid2::ui {

class WorkspaceLocationDialog final : public QDialog {
public:
    WorkspaceLocationDialog(
        std::filesystem::path workspace_root,
        QString document_kind,
        QString required_extension,
        QString suggested_filename,
        QWidget* parent = nullptr);

    [[nodiscard]] std::optional<std::filesystem::path>
    selectedRelativeFilePath() const;

private:
    void rebuildFolderTree(
        const std::filesystem::path& select_relative = {});
    void createFolder();
    void refreshValidation();
    void acceptSelection();

    [[nodiscard]] std::filesystem::path
    selectedDirectory() const;

    [[nodiscard]] std::optional<std::filesystem::path>
    candidateRelativeFilePath(
        QString* diagnostic = nullptr) const;

    std::filesystem::path workspace_root_;
    QString document_kind_;
    QString required_extension_;

    application::WorkspaceDirectoryService
        directory_service_;

    QTreeWidget* folder_tree_{};
    QLineEdit* folder_name_{};
    QPushButton* create_folder_button_{};
    QLineEdit* file_name_{};
    QLabel* preview_{};
    QLabel* diagnostic_{};
    QDialogButtonBox* buttons_{};
    QPushButton* create_document_button_{};
};

} // namespace simplesolid2::ui
