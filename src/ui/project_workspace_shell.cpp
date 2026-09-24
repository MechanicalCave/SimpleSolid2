#include "project_workspace_shell.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QVBoxLayout>

namespace simplesolid2::ui {

ProjectWorkspaceShell::ProjectWorkspaceShell(
    QWidget* parent)
    : QWidget{parent} {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(8, 8, 8, 8);
    root->setSpacing(6);

    auto* title =
        new QLabel(
            QStringLiteral("Workspace"),
            this);
    auto title_font = title->font();
    title_font.setPointSize(
        title_font.pointSize() + 6);
    title_font.setBold(true);
    title->setFont(title_font);
    root->addWidget(title);

    project_name_ = new QLabel(this);
    project_name_->setObjectName(
        QStringLiteral("workspaceProjectName"));
    project_id_ = new QLabel(this);
    project_id_->setObjectName(
        QStringLiteral("workspaceProjectId"));
    workspace_path_ = new QLabel(this);
    workspace_path_->setObjectName(
        QStringLiteral("workspaceProjectPath"));
    workspace_path_->setWordWrap(true);

    root->addWidget(project_name_);
    root->addWidget(project_id_);
    root->addWidget(workspace_path_);

    auto* toolbar_frame = new QFrame(this);
    toolbar_frame->setObjectName(
        QStringLiteral("projectToolbar"));
    toolbar_frame->setFrameShape(
        QFrame::StyledPanel);
    auto* toolbar =
        new QHBoxLayout(toolbar_frame);
    toolbar->setContentsMargins(6, 4, 6, 4);

    workspace_button_ =
        new QPushButton(
            QStringLiteral("Workspace"),
            toolbar_frame);
    workspace_button_->setObjectName(
        QStringLiteral("workspaceHomeButton"));
    workspace_button_->setCheckable(true);
    toolbar->addWidget(workspace_button_);

    new_part_button_ =
        new QPushButton(
            QStringLiteral("New Part…"),
            toolbar_frame);
    new_part_button_->setObjectName(
        QStringLiteral("workspaceNewPartButton"));
    toolbar->addWidget(new_part_button_);

    open_document_button_ =
        new QPushButton(
            QStringLiteral("Open…"),
            toolbar_frame);
    open_document_button_->setObjectName(
        QStringLiteral("workspaceOpenDocumentButton"));
    toolbar->addWidget(open_document_button_);

    refresh_button_ =
        new QPushButton(
            QStringLiteral("Refresh"),
            toolbar_frame);
    refresh_button_->setObjectName(
        QStringLiteral("workspaceRefreshButton"));
    toolbar->addWidget(refresh_button_);

    toolbar->addStretch(1);

    close_project_button_ =
        new QPushButton(
            QStringLiteral("Close Project"),
            toolbar_frame);
    close_project_button_->setObjectName(
        QStringLiteral("closeProjectButton"));
    toolbar->addWidget(close_project_button_);

    root->addWidget(toolbar_frame);

    content_ = new QStackedWidget(this);
    content_->setObjectName(
        QStringLiteral("workspaceContentHost"));

    dashboard_ = new QWidget(content_);
    dashboard_->setObjectName(
        QStringLiteral("workspaceDashboard"));
    auto* dashboard_layout =
        new QVBoxLayout(dashboard_);
    dashboard_layout->addStretch(1);

    auto* dashboard_title =
        new QLabel(
            QStringLiteral("Project Workspace"),
            dashboard_);
    auto dashboard_font =
        dashboard_title->font();
    dashboard_font.setPointSize(
        dashboard_font.pointSize() + 3);
    dashboard_font.setBold(true);
    dashboard_title->setFont(
        dashboard_font);
    dashboard_title->setAlignment(
        Qt::AlignCenter);
    dashboard_layout->addWidget(
        dashboard_title);

    auto* dashboard_hint =
        new QLabel(
            QStringLiteral(
                "Project information, configuration and Project tools will live here."),
            dashboard_);
    dashboard_hint->setObjectName(
        QStringLiteral("workspaceDashboardHint"));
    dashboard_hint->setAlignment(
        Qt::AlignCenter);
    dashboard_hint->setWordWrap(true);
    dashboard_layout->addWidget(
        dashboard_hint);
    dashboard_layout->addStretch(2);

    content_->addWidget(dashboard_);
    content_->setCurrentWidget(dashboard_);
    root->addWidget(content_, 1);

    document_tabs_ = new QTabBar(this);
    document_tabs_->setObjectName(
        QStringLiteral("documentTabs"));
    document_tabs_->setDocumentMode(true);
    document_tabs_->setExpanding(false);
    document_tabs_->setMovable(true);
    document_tabs_->setTabsClosable(true);
    document_tabs_->setUsesScrollButtons(true);
    document_tabs_->setVisible(false);
    root->addWidget(document_tabs_);
}

void ProjectWorkspaceShell::setProjectInfo(
    const QString& display_name,
    const QString& project_id,
    const QString& workspace_path) {
    project_name_->setText(
        QStringLiteral("Project: ") +
        display_name);
    project_id_->setText(
        QStringLiteral("ProjectId: ") +
        project_id);
    workspace_path_->setText(
        QStringLiteral("Workspace: ") +
        workspace_path);
}

QPushButton&
ProjectWorkspaceShell::workspaceButton() noexcept {
    return *workspace_button_;
}

QPushButton&
ProjectWorkspaceShell::newPartButton() noexcept {
    return *new_part_button_;
}

QPushButton&
ProjectWorkspaceShell::openDocumentButton() noexcept {
    return *open_document_button_;
}

QPushButton&
ProjectWorkspaceShell::refreshButton() noexcept {
    return *refresh_button_;
}

QPushButton&
ProjectWorkspaceShell::closeProjectButton() noexcept {
    return *close_project_button_;
}

QTabBar&
ProjectWorkspaceShell::documentTabs() noexcept {
    return *document_tabs_;
}

QWidget&
ProjectWorkspaceShell::dashboard() noexcept {
    return *dashboard_;
}

void ProjectWorkspaceShell::setDocumentWorkbench(
    QWidget* workbench) {
    if (document_workbench_ == workbench) {
        return;
    }

    if (document_workbench_ != nullptr) {
        content_->removeWidget(
            document_workbench_);
        document_workbench_->setParent(nullptr);
    }

    document_workbench_ = workbench;
    if (document_workbench_ != nullptr) {
        if (document_workbench_->parentWidget() !=
            content_) {
            document_workbench_->setParent(
                content_);
        }
        content_->addWidget(
            document_workbench_);
    }
}

void ProjectWorkspaceShell::showWorkspace() {
    content_->setCurrentWidget(
        dashboard_);
    workspace_button_->setEnabled(true);
    workspace_button_->setChecked(true);
}

void ProjectWorkspaceShell::showDocumentWorkbench() {
    if (document_workbench_ == nullptr) {
        showWorkspace();
        return;
    }

    content_->setCurrentWidget(
        document_workbench_);
    workspace_button_->setEnabled(true);
    workspace_button_->setChecked(false);
}

bool ProjectWorkspaceShell::showingWorkspace() const noexcept {
    return content_->currentWidget() ==
           dashboard_;
}

} // namespace simplesolid2::ui
