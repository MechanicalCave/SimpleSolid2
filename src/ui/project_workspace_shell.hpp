#pragma once

#include <QWidget>

class QLabel;
class QString;
class QPushButton;
class QStackedWidget;
class QTabBar;
class QVBoxLayout;
class QWidget;

namespace simplesolid2::ui {

class ProjectWorkspaceShell final : public QWidget {
public:
    explicit ProjectWorkspaceShell(QWidget* parent = nullptr);

    void setProjectInfo(
        const QString& display_name,
        const QString& project_id,
        const QString& workspace_path);

    [[nodiscard]] QPushButton& workspaceButton() noexcept;
    [[nodiscard]] QPushButton& newPartButton() noexcept;
    [[nodiscard]] QPushButton& openDocumentButton() noexcept;
    [[nodiscard]] QPushButton& refreshButton() noexcept;
    [[nodiscard]] QPushButton& closeProjectButton() noexcept;
    [[nodiscard]] QTabBar& documentTabs() noexcept;

    [[nodiscard]] QWidget& dashboard() noexcept;
    void setDocumentWorkbench(QWidget* workbench);
    void showWorkspace();
    void showDocumentWorkbench();

    [[nodiscard]] bool showingWorkspace() const noexcept;

private:
    QLabel* project_name_{};
    QLabel* project_id_{};
    QLabel* workspace_path_{};

    QPushButton* workspace_button_{};
    QPushButton* new_part_button_{};
    QPushButton* open_document_button_{};
    QPushButton* refresh_button_{};
    QPushButton* close_project_button_{};

    QStackedWidget* content_{};
    QWidget* dashboard_{};
    QWidget* document_workbench_{};
    QTabBar* document_tabs_{};
};

} // namespace simplesolid2::ui
