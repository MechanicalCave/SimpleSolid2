#pragma once

#include "project_hub_controller.hpp"
#include "viewport_surface.hpp"

#include <simplesolid2/core/document.hpp>

#include <QMainWindow>

#include <filesystem>
#include <optional>
#include <string>

class QCloseEvent;
class QLabel;
class QListWidget;
class QStackedWidget;
class QWidget;

namespace simplesolid2::ui {

class CadWorkbench;
class ProjectWorkspaceShell;

enum class ProjectNavigationKind {
    workspace,
    document,
};

struct ProjectNavigationContext final {
    ProjectNavigationKind kind{
        ProjectNavigationKind::workspace};
    std::optional<core::DocumentId>
        document_id;
};

class ProjectHubWindow final : public QMainWindow {
public:
    explicit ProjectHubWindow(
        std::filesystem::path recent_catalog_path,
        QWidget* parent = nullptr);

    ProjectHubWindow(
        std::filesystem::path recent_catalog_path,
        ViewportFactory viewport_factory,
        QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void buildHubPage();
    void buildWorkspacePage();
    void refreshRecent();
    void syncRecentActionState();

    void createProject();
    void openProject();
    void openSelectedRecent();
    void locateSelectedRecent();
    void removeSelectedRecent();
    void closeProject();
    void enterWorkspace();

    void navigateToWorkspace();
    [[nodiscard]] bool navigateToDocument(
        const core::DocumentId& document_id);

    void createPart();
    void openDocument();
    void refreshWorkspaceDocuments();

    void syncOpenDocumentTabs();
    void ensureDocumentTab(
        const core::DocumentId& document_id);
    void updateDocumentTab(
        const core::DocumentId& document_id);
    void closeDocumentTab(int index);
    void closeDocument(
        const core::DocumentId& document_id);

    [[nodiscard]] int tabIndexFor(
        const core::DocumentId& document_id) const;
    [[nodiscard]] std::optional<core::DocumentId>
    tabDocumentId(int index) const;
    [[nodiscard]] std::filesystem::path
    defaultPartPath() const;

    [[nodiscard]] bool requestCloseProject();
    [[nodiscard]] std::string selectedProjectId() const;

    void showFailure(
        const application::internal::ProjectHubDiagnostic& diagnostic);
    void showFailure(
        const application::ProjectDocumentDiagnostic& diagnostic);
    void showFailure(
        const application::DocumentSessionDiagnostic& diagnostic);

    application::internal::ProjectHubController controller_;
    ViewportFactory viewport_factory_;

    QStackedWidget* pages_{};
    QWidget* hub_page_{};
    QWidget* workspace_page_{};

    QListWidget* recent_list_{};
    QPushButton* open_recent_button_{};
    QPushButton* locate_recent_button_{};
    QPushButton* remove_recent_button_{};
    QLabel* hub_status_{};

    ProjectWorkspaceShell* workspace_shell_{};
    CadWorkbench* cad_workbench_{};
    ProjectNavigationContext
        navigation_;
};

} // namespace simplesolid2::ui
