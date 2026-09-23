#pragma once

#include <simplesolid2/application/project_session.hpp>

#include <QWidget>

#include <filesystem>
#include <optional>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabBar;
class QTreeWidget;
class QWidget;

namespace simplesolid2::ui {

enum class ProjectCloseDisposition {
    clean,
    discard,
    cancel,
};

class CadWorkbench final : public QWidget {
public:
    explicit CadWorkbench(QWidget* parent = nullptr);

    void setProjectSession(application::ProjectSession* session);
    void clearProjectSession();

    [[nodiscard]] ProjectCloseDisposition prepareProjectClose();

    [[nodiscard]] std::optional<core::DocumentId> activeDocumentId() const {
        return active_document_id_;
    }

    [[nodiscard]] bool activateDocument(const core::DocumentId& document_id);

private:
    void buildUi();
    void refreshWorkspaceIndex();
    void syncOpenTabs();
    void ensureDocumentTab(const core::DocumentId& document_id);
    void activateTab(int index);
    void closeTab(int index);

    void newPart();
    void openPart();
    void applyProperties();
    void undo();
    void redo();
    void save();
    void closeActiveDocument();

    void refreshActiveContext();
    void clearActiveContext();
    void rebuildDocumentTree();
    void syncActionState();
    void updateTabPresentation(const core::DocumentId& document_id);

    [[nodiscard]] application::DocumentSession* activeDocumentSession() noexcept;
    [[nodiscard]] const application::DocumentSession* activeDocumentSession() const noexcept;
    [[nodiscard]] std::filesystem::path defaultPartPath() const;
    [[nodiscard]] int tabIndexFor(const core::DocumentId& document_id) const;
    [[nodiscard]] std::optional<core::DocumentId> tabDocumentId(int index) const;

    void showFailure(const application::ProjectDocumentDiagnostic& diagnostic);
    void showFailure(const application::DocumentSessionDiagnostic& diagnostic);

    application::ProjectSession* session_{};
    std::optional<core::DocumentId> active_document_id_;

    QPushButton* new_part_button_{};
    QPushButton* open_part_button_{};
    QPushButton* refresh_button_{};
    QPushButton* undo_button_{};
    QPushButton* redo_button_{};
    QPushButton* save_button_{};
    QPushButton* close_document_button_{};

    QTreeWidget* document_tree_{};
    QWidget* editor_surface_{};

    QLabel* active_path_{};
    QLabel* active_id_{};
    QLineEdit* number_{};
    QLineEdit* title_{};
    QPlainTextEdit* description_{};
    QLineEdit* engineering_revision_{};
    QPushButton* apply_button_{};

    QLabel* operations_placeholder_{};

    QTabBar* document_tabs_{};
    QLabel* status_{};
};

} // namespace simplesolid2::ui
