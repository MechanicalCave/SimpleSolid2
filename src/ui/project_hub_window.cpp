#include "project_hub_window.hpp"
#include "project_creation_dialog.hpp"
#include "project_hub_selection.hpp"
#include "cad_workbench.hpp"

#include <QAbstractItemView>
#include <QByteArray>
#include <QCloseEvent>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QStyle>
#include <QVBoxLayout>
#include <QWidget>

#include <string_view>
#include <utility>

namespace simplesolid2::ui {
namespace {

std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(value.size()));
}

std::filesystem::path toFilesystemPath(const QString& value) {
#if defined(_WIN32)
    return std::filesystem::path{value.toStdWString()};
#else
    const auto bytes = value.toUtf8();
    std::u8string utf8;
    utf8.reserve(static_cast<std::size_t>(bytes.size()));
    for (const unsigned char ch : bytes) {
        utf8.push_back(static_cast<char8_t>(ch));
    }
    return std::filesystem::path{utf8};
#endif
}

QString fromFilesystemPath(const std::filesystem::path& value) {
#if defined(_WIN32)
    return QString::fromStdWString(value.wstring());
#else
    const auto utf8 = value.generic_u8string();
    return QString::fromUtf8(
        reinterpret_cast<const char*>(utf8.data()),
        static_cast<qsizetype>(utf8.size()));
#endif
}

QString availabilityLabel(
    application::internal::RecentProjectAvailability availability) {
    using application::internal::RecentProjectAvailability;
    switch (availability) {
    case RecentProjectAvailability::available:
        return {};
    case RecentProjectAvailability::workspace_missing:
        return QStringLiteral("Workspace not found");
    case RecentProjectAvailability::project_invalid:
        return QStringLiteral("Invalid Project");
    case RecentProjectAvailability::identity_mismatch:
        return QStringLiteral("Project mismatch");
    }
    return QStringLiteral("Project unavailable");
}

} // namespace

ProjectHubWindow::ProjectHubWindow(
    std::filesystem::path recent_catalog_path,
    QWidget* parent)
    : QMainWindow{parent},
      controller_{std::move(recent_catalog_path)} {
    setWindowTitle(QStringLiteral("SimpleSolid 2.0"));
    resize(1280, 800);

    pages_ = new QStackedWidget(this);
    setCentralWidget(pages_);

    buildHubPage();
    buildWorkspacePage();
    pages_->setCurrentWidget(hub_page_);
    refreshRecent();
}

void ProjectHubWindow::buildHubPage() {
    hub_page_ = new QWidget(pages_);
    auto* root = new QVBoxLayout(hub_page_);

    auto* title = new QLabel(QStringLiteral("SimpleSolid 2.0"), hub_page_);
    auto font = title->font();
    font.setPointSize(font.pointSize() + 6);
    font.setBold(true);
    title->setFont(font);
    root->addWidget(title);

    auto* subtitle = new QLabel(
        QStringLiteral("Project Hub — create, open or reopen a Project."),
        hub_page_);
    root->addWidget(subtitle);

    auto* primary_actions = new QHBoxLayout;
    auto* create_button =
        new QPushButton(QStringLiteral("Create Project…"), hub_page_);
    auto* open_button =
        new QPushButton(QStringLiteral("Open Project…"), hub_page_);
    primary_actions->addWidget(create_button);
    primary_actions->addWidget(open_button);
    primary_actions->addStretch(1);
    root->addLayout(primary_actions);

    auto* recent_label = new QLabel(QStringLiteral("Recent Projects"), hub_page_);
    root->addWidget(recent_label);

    recent_list_ = new QListWidget(hub_page_);
    recent_list_->setObjectName(QStringLiteral("recentProjectsList"));
    recent_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    recent_list_->setAlternatingRowColors(true);
    root->addWidget(recent_list_, 1);

    auto* recent_actions = new QHBoxLayout;
    open_recent_button_ =
        new QPushButton(QStringLiteral("Open"), hub_page_);
    open_recent_button_->setObjectName(QStringLiteral("openRecentButton"));
    locate_recent_button_ =
        new QPushButton(QStringLiteral("Locate…"), hub_page_);
    locate_recent_button_->setObjectName(QStringLiteral("locateRecentButton"));
    remove_recent_button_ =
        new QPushButton(QStringLiteral("Remove from Recent"), hub_page_);
    remove_recent_button_->setObjectName(QStringLiteral("removeRecentButton"));
    open_recent_button_->setEnabled(false);
    locate_recent_button_->setEnabled(false);
    remove_recent_button_->setEnabled(false);
    recent_actions->addWidget(open_recent_button_);
    recent_actions->addWidget(locate_recent_button_);
    recent_actions->addWidget(remove_recent_button_);
    recent_actions->addStretch(1);
    root->addLayout(recent_actions);

    hub_status_ = new QLabel(hub_page_);
    hub_status_->setWordWrap(true);
    root->addWidget(hub_status_);

    QObject::connect(
        create_button,
        &QPushButton::clicked,
        this,
        [this] { createProject(); });
    QObject::connect(
        open_button,
        &QPushButton::clicked,
        this,
        [this] { openProject(); });
    QObject::connect(
        open_recent_button_,
        &QPushButton::clicked,
        this,
        [this] { openSelectedRecent(); });
    QObject::connect(
        locate_recent_button_,
        &QPushButton::clicked,
        this,
        [this] { locateSelectedRecent(); });
    QObject::connect(
        remove_recent_button_,
        &QPushButton::clicked,
        this,
        [this] { removeSelectedRecent(); });
    QObject::connect(
        recent_list_,
        &QListWidget::itemDoubleClicked,
        this,
        [this](QListWidgetItem*) { openSelectedRecent(); });
    QObject::connect(
        recent_list_,
        &QListWidget::itemSelectionChanged,
        this,
        [this] { syncRecentActionState(); });

    pages_->addWidget(hub_page_);
}

void ProjectHubWindow::buildWorkspacePage() {
    workspace_page_ = new QWidget(pages_);
    auto* root = new QVBoxLayout(workspace_page_);

    auto* title = new QLabel(QStringLiteral("Workspace"), workspace_page_);
    auto font = title->font();
    font.setPointSize(font.pointSize() + 6);
    font.setBold(true);
    title->setFont(font);
    root->addWidget(title);

    workspace_name_ = new QLabel(workspace_page_);
    workspace_id_ = new QLabel(workspace_page_);
    workspace_path_ = new QLabel(workspace_page_);
    workspace_path_->setWordWrap(true);

    root->addWidget(workspace_name_);
    root->addWidget(workspace_id_);
    root->addWidget(workspace_path_);

    cad_workbench_ = new CadWorkbench(workspace_page_);
    cad_workbench_->setObjectName(QStringLiteral("cadWorkbench"));
    root->addWidget(cad_workbench_, 1);

    auto* close_button =
        new QPushButton(QStringLiteral("Close Project"), workspace_page_);
    root->addWidget(close_button);

    QObject::connect(
        close_button,
        &QPushButton::clicked,
        this,
        [this] { closeProject(); });

    pages_->addWidget(workspace_page_);
}

void ProjectHubWindow::refreshRecent() {
    recent_list_->clear();
    recent_list_->setCurrentRow(-1);
    syncRecentActionState();

    const auto listed = controller_.recentProjectHubEntries();
    if (!listed.ok()) {
        hub_status_->setText(
            QStringLiteral("Recent Projects unavailable: ") +
            fromUtf8(listed.diagnostic.message));
        return;
    }

    for (const auto& view : listed.entries) {
        auto text =
            fromUtf8(view.recent.display_name) +
            QStringLiteral("\n") +
            fromFilesystemPath(view.recent.workspace_root);

        const auto status = availabilityLabel(view.availability);
        if (!status.isEmpty()) {
            text += QStringLiteral("\n⚠ ") + status;
        }

        auto* item = new QListWidgetItem(text, recent_list_);
        item->setData(
            Qt::UserRole,
            fromUtf8(view.recent.project_id));
        item->setData(
            internal::recentOpenableRole,
            view.openable());
        item->setData(
            internal::recentAvailabilityRole,
            static_cast<int>(view.availability));

        auto tooltip =
            QStringLiteral("ProjectId: ") + fromUtf8(view.recent.project_id);
        if (!view.diagnostic.empty()) {
            tooltip += QStringLiteral("\n") + fromUtf8(view.diagnostic);
        }
        item->setToolTip(tooltip);

        if (!view.openable()) {
            item->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
        }
    }

    recent_list_->clearSelection();
    recent_list_->setCurrentRow(-1);
    syncRecentActionState();

    if (listed.entries.empty()) {
        hub_status_->setText(QStringLiteral("No recent Projects."));
    } else {
        hub_status_->setText(
            QStringLiteral("%1 recent Project(s).")
                .arg(static_cast<qulonglong>(listed.entries.size())));
    }
}

void ProjectHubWindow::syncRecentActionState() {
    const bool selected = internal::hasSingleRecentSelection(*recent_list_);
    open_recent_button_->setEnabled(
        selected && internal::selectedRecentCanOpen(*recent_list_));
    locate_recent_button_->setEnabled(selected);
    remove_recent_button_->setEnabled(selected);
}

void ProjectHubWindow::createProject() {
    ProjectCreationDialog dialog{this};
    if (dialog.exec() != QDialog::Accepted) return;

    const auto result = controller_.createProjectInLocation(
        toFilesystemPath(dialog.location()),
        toFilesystemPath(dialog.projectFolder()),
        toUtf8(dialog.projectName()));
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::openProject() {
    const auto folder = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Open SimpleSolid Project"));
    if (folder.isEmpty()) return;

    const auto result =
        controller_.openProject(toFilesystemPath(folder));
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::openSelectedRecent() {
    if (!internal::selectedRecentCanOpen(*recent_list_)) return;

    const auto project_id = selectedProjectId();
    if (project_id.empty()) return;

    const auto result = controller_.openRecent(project_id);
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::locateSelectedRecent() {
    const auto project_id = selectedProjectId();
    if (project_id.empty()) return;

    const auto folder = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Locate Project Workspace"));
    if (folder.isEmpty()) return;

    const auto result = controller_.relocateAndOpenRecent(
        project_id,
        toFilesystemPath(folder));
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::removeSelectedRecent() {
    const auto project_id = selectedProjectId();
    if (project_id.empty()) return;

    if (QMessageBox::question(
            this,
            QStringLiteral("Remove from Recent"),
            QStringLiteral(
                "Remove this Project from Recent Projects?\n\n"
                "The Project files will not be modified.")) !=
        QMessageBox::Yes) {
        return;
    }

    const auto result = controller_.removeRecent(project_id);
    if (!result.ok()) {
        showFailure(result.diagnostic);
    }
    refreshRecent();
}

void ProjectHubWindow::closeProject() {
    if (!requestCloseProject()) return;

    pages_->setCurrentWidget(hub_page_);
    refreshRecent();
}

void ProjectHubWindow::enterWorkspace() {
    auto* session = controller_.activeSession();
    if (session == nullptr) {
        QMessageBox::critical(
            this,
            QStringLiteral("Project"),
            QStringLiteral("No active ProjectSession is available."));
        return;
    }

    workspace_name_->setText(
        QStringLiteral("Project: ") + fromUtf8(session->displayName()));
    workspace_id_->setText(
        QStringLiteral("ProjectId: ") + fromUtf8(session->projectId()));
    workspace_path_->setText(
        QStringLiteral("Workspace: ") +
        fromFilesystemPath(session->workspaceRoot()));

    cad_workbench_->setProjectSession(session);
    pages_->setCurrentWidget(workspace_page_);
}

bool ProjectHubWindow::requestCloseProject() {
    if (!controller_.hasActiveProject()) return true;

    const auto disposition = cad_workbench_->prepareProjectClose();
    if (disposition == ProjectCloseDisposition::cancel) {
        return false;
    }

    const bool discard =
        disposition == ProjectCloseDisposition::discard;
    if (!controller_.closeProject(discard)) {
        QMessageBox::warning(
            this,
            QStringLiteral("Close Project"),
            QStringLiteral(
                "The Project still contains unsaved Part changes and remains open."));
        return false;
    }

    cad_workbench_->clearProjectSession();
    return true;
}

void ProjectHubWindow::closeEvent(QCloseEvent* event) {
    if (requestCloseProject()) {
        event->accept();
    } else {
        event->ignore();
    }
}

std::string ProjectHubWindow::selectedProjectId() const {
    return internal::selectedRecentProjectId(*recent_list_);
}

void ProjectHubWindow::showFailure(
    const application::internal::ProjectHubDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);
    if (!diagnostic.path.empty()) {
        message += QStringLiteral("\n\nPath: ");
        message += fromFilesystemPath(diagnostic.path);
    }

    QMessageBox::warning(
        this,
        QStringLiteral("SimpleSolid Project"),
        message);
}

} // namespace simplesolid2::ui
