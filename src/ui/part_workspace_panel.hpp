#pragma once

#include <simplesolid2/application/project_session.hpp>

#include <QWidget>

#include <filesystem>
#include <optional>

class QLabel;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;

namespace simplesolid2::ui {

enum class ProjectCloseDisposition {
    clean,
    discard,
    cancel,
};

class PartWorkspacePanel final : public QWidget {
public:
    explicit PartWorkspacePanel(QWidget* parent = nullptr);

    void setProjectSession(application::ProjectSession* session);
    void clearProjectSession();

    [[nodiscard]] ProjectCloseDisposition prepareProjectClose();

private:
    void buildUi();
    void refreshDocuments();
    void syncDocumentActionState();
    void newPart();
    void openSelectedDocument();
    void applyProperties();
    void undo();
    void redo();
    void save();
    void closeActiveDocument();
    void loadActiveDocument();
    void clearActiveDocument();
    void syncActiveActions();

    [[nodiscard]] std::optional<core::DocumentId> selectedDocumentId() const;
    [[nodiscard]] application::DocumentSession* activeDocumentSession() noexcept;
    [[nodiscard]] std::filesystem::path defaultPartPath() const;

    void showFailure(const application::ProjectDocumentDiagnostic& diagnostic);
    void showFailure(const application::DocumentSessionDiagnostic& diagnostic);

    application::ProjectSession* session_{};
    std::optional<core::DocumentId> active_document_id_;

    QListWidget* document_list_{};
    QPushButton* new_part_button_{};
    QPushButton* refresh_button_{};
    QPushButton* open_button_{};
    QLabel* list_status_{};

    QLabel* active_path_{};
    QLabel* active_id_{};
    QLineEdit* number_{};
    QLineEdit* title_{};
    QPlainTextEdit* description_{};
    QLineEdit* engineering_revision_{};
    QPushButton* apply_button_{};
    QPushButton* undo_button_{};
    QPushButton* redo_button_{};
    QPushButton* save_button_{};
    QPushButton* close_button_{};
    QLabel* active_status_{};
};

} // namespace simplesolid2::ui
