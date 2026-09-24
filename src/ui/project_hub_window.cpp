#include "project_hub_window.hpp"

#include "cad_workbench.hpp"
#include "open_document_dialog.hpp"
#include "project_creation_dialog.hpp"
#include "project_hub_selection.hpp"
#include "project_workspace_shell.hpp"
#include "workspace_location_dialog.hpp"

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
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QString>
#include <QStyle>
#include <QTabBar>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace simplesolid2::ui {
namespace {

std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return std::string{
        bytes.constData(),
        static_cast<std::size_t>(
            bytes.size())};
}

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(
            value.size()));
}

std::filesystem::path toFilesystemPath(
    const QString& value) {
#if defined(_WIN32)
    return std::filesystem::path{
        value.toStdWString()};
#else
    const auto bytes = value.toUtf8();
    std::u8string utf8;
    utf8.reserve(
        static_cast<std::size_t>(
            bytes.size()));
    for (const unsigned char ch : bytes) {
        utf8.push_back(
            static_cast<char8_t>(ch));
    }
    return std::filesystem::path{utf8};
#endif
}

QString fromFilesystemPath(
    const std::filesystem::path& value) {
#if defined(_WIN32)
    return QString::fromStdWString(
        value.wstring());
#else
    const auto utf8 =
        value.generic_u8string();
    return QString::fromUtf8(
        reinterpret_cast<const char*>(
            utf8.data()),
        static_cast<qsizetype>(
            utf8.size()));
#endif
}

QString availabilityLabel(
    application::internal::
        RecentProjectAvailability availability) {
    using application::internal::
        RecentProjectAvailability;

    switch (availability) {
    case RecentProjectAvailability::available:
        return {};
    case RecentProjectAvailability::
        workspace_missing:
        return QStringLiteral(
            "Workspace not found");
    case RecentProjectAvailability::
        project_invalid:
        return QStringLiteral(
            "Invalid Project");
    case RecentProjectAvailability::
        identity_mismatch:
        return QStringLiteral(
            "Project mismatch");
    }

    return QStringLiteral(
        "Project unavailable");
}

QString documentIndexStateLabel(
    application::DocumentIndexState state) {
    switch (state) {
    case application::DocumentIndexState::resolved:
        return QStringLiteral("Ready");
    case application::DocumentIndexState::
        identity_conflict:
        return QStringLiteral(
            "Identity conflict");
    case application::DocumentIndexState::invalid:
        return QStringLiteral(
            "Invalid Part");
    }

    return QStringLiteral("Unavailable");
}

QString documentCandidateName(
    const application::DocumentIndexEntry& entry) {
    if (!entry.title.empty()) {
        return fromUtf8(entry.title);
    }

    if (!entry.relative_paths.empty()) {
        auto name = fromFilesystemPath(
            entry.relative_paths.front().stem());
        if (!name.isEmpty()) {
            return name;
        }
    }

    return QStringLiteral("<untitled>");
}

QString documentCandidateLocations(
    const application::DocumentIndexEntry& entry) {
    QString result;
    for (const auto& path :
         entry.relative_paths) {
        if (!result.isEmpty()) {
            result += QStringLiteral("\n");
        }
        result +=
            fromFilesystemPath(path);
    }
    return result;
}

std::vector<OpenDocumentCandidate>
openDocumentCandidates(
    const application::DocumentWorkspaceIndex&
        index) {
    std::vector<OpenDocumentCandidate> result;
    result.reserve(
        index.entries().size());

    for (const auto& entry :
         index.entries()) {
        const bool openable =
            entry.state ==
                application::DocumentIndexState::
                    resolved &&
            entry.document_id.has_value();

        QString tooltip;
        if (entry.document_id) {
            tooltip =
                QStringLiteral("DocumentId: ") +
                fromUtf8(
                    entry.document_id->value());
        }
        if (!entry.diagnostic.empty()) {
            if (!tooltip.isEmpty()) {
                tooltip +=
                    QStringLiteral("\n");
            }
            tooltip +=
                fromUtf8(entry.diagnostic);
        }

        result.push_back(
            OpenDocumentCandidate{
                QStringLiteral("Part"),
                documentCandidateName(entry),
                documentCandidateLocations(entry),
                documentIndexStateLabel(
                    entry.state),
                std::move(tooltip),
                entry.document_id,
                openable});
    }

    return result;
}

QString partDisplayName(
    const application::DocumentSession& session) {
    const auto& title =
        session.document()
            .properties()
            .title;
    if (!title.empty()) {
        return fromUtf8(title);
    }

    auto fallback =
        fromFilesystemPath(
            session.path().stem());
    if (fallback.isEmpty()) {
        fallback =
            QStringLiteral(
                "<untitled Part>");
    }
    return fallback;
}

} // namespace

ProjectHubWindow::ProjectHubWindow(
    std::filesystem::path recent_catalog_path,
    QWidget* parent)
    : ProjectHubWindow{
          std::move(recent_catalog_path),
          ViewportFactory{},
          parent} {}

ProjectHubWindow::ProjectHubWindow(
    std::filesystem::path recent_catalog_path,
    ViewportFactory viewport_factory,
    QWidget* parent)
    : QMainWindow{parent},
      controller_{
          std::move(recent_catalog_path)},
      viewport_factory_{
          std::move(viewport_factory)} {
    setWindowTitle(
        QStringLiteral("SimpleSolid 2.0"));
    resize(1280, 800);

    pages_ = new QStackedWidget(this);
    pages_->setObjectName(
        QStringLiteral("applicationPages"));
    setCentralWidget(pages_);

    buildHubPage();
    buildWorkspacePage();

    pages_->setCurrentWidget(
        hub_page_);
    refreshRecent();
}

void ProjectHubWindow::buildHubPage() {
    hub_page_ = new QWidget(pages_);
    hub_page_->setObjectName(
        QStringLiteral("projectHubPage"));

    auto* root =
        new QVBoxLayout(hub_page_);

    auto* title =
        new QLabel(
            QStringLiteral(
                "SimpleSolid 2.0"),
            hub_page_);
    auto font = title->font();
    font.setPointSize(
        font.pointSize() + 6);
    font.setBold(true);
    title->setFont(font);
    root->addWidget(title);

    auto* subtitle =
        new QLabel(
            QStringLiteral(
                "Project Hub — create, open or reopen a Project."),
            hub_page_);
    root->addWidget(subtitle);

    auto* primary_actions =
        new QHBoxLayout;
    auto* create_button =
        new QPushButton(
            QStringLiteral(
                "Create Project…"),
            hub_page_);
    auto* open_button =
        new QPushButton(
            QStringLiteral(
                "Open Project…"),
            hub_page_);
    primary_actions->addWidget(
        create_button);
    primary_actions->addWidget(
        open_button);
    primary_actions->addStretch(1);
    root->addLayout(primary_actions);

    auto* recent_label =
        new QLabel(
            QStringLiteral(
                "Recent Projects"),
            hub_page_);
    root->addWidget(recent_label);

    recent_list_ =
        new QListWidget(hub_page_);
    recent_list_->setObjectName(
        QStringLiteral(
            "recentProjectsList"));
    recent_list_->setSelectionMode(
        QAbstractItemView::
            SingleSelection);
    recent_list_->setAlternatingRowColors(
        true);
    root->addWidget(recent_list_, 1);

    auto* recent_actions =
        new QHBoxLayout;

    open_recent_button_ =
        new QPushButton(
            QStringLiteral("Open"),
            hub_page_);
    open_recent_button_->setObjectName(
        QStringLiteral(
            "openRecentButton"));

    locate_recent_button_ =
        new QPushButton(
            QStringLiteral("Locate…"),
            hub_page_);
    locate_recent_button_->setObjectName(
        QStringLiteral(
            "locateRecentButton"));

    remove_recent_button_ =
        new QPushButton(
            QStringLiteral(
                "Remove from Recent"),
            hub_page_);
    remove_recent_button_->setObjectName(
        QStringLiteral(
            "removeRecentButton"));

    open_recent_button_->setEnabled(false);
    locate_recent_button_->setEnabled(false);
    remove_recent_button_->setEnabled(false);

    recent_actions->addWidget(
        open_recent_button_);
    recent_actions->addWidget(
        locate_recent_button_);
    recent_actions->addWidget(
        remove_recent_button_);
    recent_actions->addStretch(1);
    root->addLayout(recent_actions);

    hub_status_ =
        new QLabel(hub_page_);
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
        [this] {
            openSelectedRecent();
        });
    QObject::connect(
        locate_recent_button_,
        &QPushButton::clicked,
        this,
        [this] {
            locateSelectedRecent();
        });
    QObject::connect(
        remove_recent_button_,
        &QPushButton::clicked,
        this,
        [this] {
            removeSelectedRecent();
        });
    QObject::connect(
        recent_list_,
        &QListWidget::itemDoubleClicked,
        this,
        [this](QListWidgetItem*) {
            openSelectedRecent();
        });
    QObject::connect(
        recent_list_,
        &QListWidget::itemSelectionChanged,
        this,
        [this] {
            syncRecentActionState();
        });

    pages_->addWidget(hub_page_);
}

void ProjectHubWindow::buildWorkspacePage() {
    workspace_page_ =
        new QWidget(pages_);
    workspace_page_->setObjectName(
        QStringLiteral("workspacePage"));

    auto* root =
        new QVBoxLayout(workspace_page_);
    root->setContentsMargins(0, 0, 0, 0);

    workspace_shell_ =
        new ProjectWorkspaceShell(
            workspace_page_);
    workspace_shell_->setObjectName(
        QStringLiteral(
            "projectWorkspaceShell"));
    root->addWidget(
        workspace_shell_,
        1);

    cad_workbench_ =
        new CadWorkbench(
            viewport_factory_,
            workspace_shell_);
    cad_workbench_->setObjectName(
        QStringLiteral("cadWorkbench"));

    workspace_shell_->setDocumentWorkbench(
        cad_workbench_);

    cad_workbench_->setCloseDocumentHandler(
        [this](
            const core::DocumentId&
                document_id) {
            closeDocument(document_id);
        });

    cad_workbench_->
        setDocumentStateChangedHandler(
            [this](
                const core::DocumentId&
                    document_id) {
                updateDocumentTab(
                    document_id);
            });

    QObject::connect(
        &workspace_shell_->
            workspaceButton(),
        &QPushButton::clicked,
        this,
        [this] {
            navigateToWorkspace();
        });
    QObject::connect(
        &workspace_shell_->
            newPartButton(),
        &QPushButton::clicked,
        this,
        [this] { createPart(); });
    QObject::connect(
        &workspace_shell_->
            openDocumentButton(),
        &QPushButton::clicked,
        this,
        [this] { openDocument(); });
    QObject::connect(
        &workspace_shell_->
            refreshButton(),
        &QPushButton::clicked,
        this,
        [this] {
            refreshWorkspaceDocuments();
        });
    QObject::connect(
        &workspace_shell_->
            closeProjectButton(),
        &QPushButton::clicked,
        this,
        [this] { closeProject(); });

    QObject::connect(
        &workspace_shell_->
            documentTabs(),
        &QTabBar::currentChanged,
        this,
        [this](int index) {
            if (index < 0) {
                return;
            }

            if (const auto id =
                    tabDocumentId(index)) {
                static_cast<void>(
                    navigateToDocument(
                        *id));
            }
        });

    QObject::connect(
        &workspace_shell_->
            documentTabs(),
        &QTabBar::tabCloseRequested,
        this,
        [this](int index) {
            closeDocumentTab(index);
        });

    workspace_shell_->showWorkspace();
    pages_->addWidget(
        workspace_page_);
}

void ProjectHubWindow::refreshRecent() {
    recent_list_->clear();
    recent_list_->setCurrentRow(-1);
    syncRecentActionState();

    const auto listed =
        controller_.
            recentProjectHubEntries();
    if (!listed.ok()) {
        hub_status_->setText(
            QStringLiteral(
                "Recent Projects unavailable: ") +
            fromUtf8(
                listed.diagnostic.message));
        return;
    }

    for (const auto& view :
         listed.entries) {
        auto label =
            fromUtf8(
                view.recent.display_name) +
            QStringLiteral("\n") +
            fromFilesystemPath(
                view.recent.workspace_root);

        const auto status =
            availabilityLabel(
                view.availability);
        if (!status.isEmpty()) {
            label +=
                QStringLiteral("\n⚠ ") +
                status;
        }

        auto* item =
            new QListWidgetItem(
                label,
                recent_list_);
        item->setData(
            Qt::UserRole,
            fromUtf8(
                view.recent.project_id));
        item->setData(
            internal::recentOpenableRole,
            view.openable());
        item->setData(
            internal::
                recentAvailabilityRole,
            static_cast<int>(
                view.availability));

        auto tooltip =
            QStringLiteral(
                "ProjectId: ") +
            fromUtf8(
                view.recent.project_id);
        if (!view.diagnostic.empty()) {
            tooltip +=
                QStringLiteral("\n") +
                fromUtf8(
                    view.diagnostic);
        }
        item->setToolTip(tooltip);

        if (!view.openable()) {
            item->setIcon(
                style()->standardIcon(
                    QStyle::
                        SP_MessageBoxWarning));
        }
    }

    recent_list_->clearSelection();
    recent_list_->setCurrentRow(-1);
    syncRecentActionState();

    if (listed.entries.empty()) {
        hub_status_->setText(
            QStringLiteral(
                "No recent Projects."));
    } else {
        hub_status_->setText(
            QStringLiteral(
                "%1 recent Project(s).")
                .arg(
                    static_cast<
                        qulonglong>(
                        listed.entries
                            .size())));
    }
}

void ProjectHubWindow::
syncRecentActionState() {
    const bool selected =
        internal::
            hasSingleRecentSelection(
                *recent_list_);
    open_recent_button_->setEnabled(
        selected &&
        internal::
            selectedRecentCanOpen(
                *recent_list_));
    locate_recent_button_->
        setEnabled(selected);
    remove_recent_button_->
        setEnabled(selected);
}

void ProjectHubWindow::createProject() {
    ProjectCreationDialog dialog{this};
    if (dialog.exec() !=
        QDialog::Accepted) {
        return;
    }

    const auto result =
        controller_.
            createProjectInLocation(
                toFilesystemPath(
                    dialog.location()),
                toFilesystemPath(
                    dialog.projectFolder()),
                toUtf8(
                    dialog.projectName()));
    if (!result.ok()) {
        showFailure(
            result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::openProject() {
    const auto folder =
        QFileDialog::
            getExistingDirectory(
                this,
                QStringLiteral(
                    "Open SimpleSolid Project"));
    if (folder.isEmpty()) {
        return;
    }

    const auto result =
        controller_.openProject(
            toFilesystemPath(folder));
    if (!result.ok()) {
        showFailure(
            result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::
openSelectedRecent() {
    if (!internal::
            selectedRecentCanOpen(
                *recent_list_)) {
        return;
    }

    const auto project_id =
        selectedProjectId();
    if (project_id.empty()) {
        return;
    }

    const auto result =
        controller_.openRecent(
            project_id);
    if (!result.ok()) {
        showFailure(
            result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::
locateSelectedRecent() {
    const auto project_id =
        selectedProjectId();
    if (project_id.empty()) {
        return;
    }

    const auto folder =
        QFileDialog::
            getExistingDirectory(
                this,
                QStringLiteral(
                    "Locate Project Workspace"));
    if (folder.isEmpty()) {
        return;
    }

    const auto result =
        controller_.
            relocateAndOpenRecent(
                project_id,
                toFilesystemPath(
                    folder));
    if (!result.ok()) {
        showFailure(
            result.diagnostic);
        refreshRecent();
        return;
    }

    enterWorkspace();
}

void ProjectHubWindow::
removeSelectedRecent() {
    const auto project_id =
        selectedProjectId();
    if (project_id.empty()) {
        return;
    }

    if (QMessageBox::question(
            this,
            QStringLiteral(
                "Remove from Recent"),
            QStringLiteral(
                "Remove this Project from Recent Projects?\n\n"
                "The Project files will not be modified.")) !=
        QMessageBox::Yes) {
        return;
    }

    const auto result =
        controller_.removeRecent(
            project_id);
    if (!result.ok()) {
        showFailure(
            result.diagnostic);
    }

    refreshRecent();
}

void ProjectHubWindow::closeProject() {
    if (!requestCloseProject()) {
        return;
    }

    pages_->setCurrentWidget(
        hub_page_);
    refreshRecent();
}

void ProjectHubWindow::enterWorkspace() {
    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        QMessageBox::critical(
            this,
            QStringLiteral("Project"),
            QStringLiteral(
                "No active ProjectSession is available."));
        return;
    }

    workspace_shell_->setProjectInfo(
        fromUtf8(
            session->displayName()),
        fromUtf8(
            session->projectId()),
        fromFilesystemPath(
            session->workspaceRoot()));

    cad_workbench_->
        setProjectSession(session);
    syncOpenDocumentTabs();
    navigateToWorkspace();

    pages_->setCurrentWidget(
        workspace_page_);
}

void ProjectHubWindow::
navigateToWorkspace() {
    if (!controller_.hasActiveProject()) {
        return;
    }

    cad_workbench_->deactivateDocument();

    navigation_.kind =
        ProjectNavigationKind::workspace;
    navigation_.document_id.reset();

    {
        const QSignalBlocker blocked{
            workspace_shell_->
                documentTabs()};
        workspace_shell_->
            documentTabs()
            .setCurrentIndex(-1);
    }

    workspace_shell_->showWorkspace();
}

bool ProjectHubWindow::
navigateToDocument(
    const core::DocumentId& document_id) {
    auto* session =
        controller_.activeSession();
    if (session == nullptr ||
        session->documentSession(
            document_id) == nullptr) {
        return false;
    }

    ensureDocumentTab(document_id);

    if (!cad_workbench_->
            activateOpenDocument(
                document_id)) {
        return false;
    }

    navigation_.kind =
        ProjectNavigationKind::document;
    navigation_.document_id =
        document_id;

    const int index =
        tabIndexFor(document_id);
    if (index >= 0) {
        const QSignalBlocker blocked{
            workspace_shell_->
                documentTabs()};
        workspace_shell_->
            documentTabs()
            .setCurrentIndex(index);
    }

    workspace_shell_->
        showDocumentWorkbench();
    updateDocumentTab(document_id);
    return true;
}

void ProjectHubWindow::createPart() {
    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return;
    }

    WorkspaceLocationDialog dialog{
        session->workspaceRoot(),
        QStringLiteral("Part"),
        QStringLiteral(".ss2part"),
        fromFilesystemPath(
            defaultPartPath()
                .filename()),
        workspace_shell_};

    if (dialog.exec() !=
        QDialog::Accepted) {
        return;
    }

    const auto relative =
        dialog.selectedRelativeFilePath();
    if (!relative) {
        return;
    }

    auto created =
        session->createPart(
            *relative);
    if (!created.ok()) {
        showFailure(
            created.diagnostic);
        return;
    }

    ensureDocumentTab(
        created.session->
            documentId());
    refreshWorkspaceDocuments();

    static_cast<void>(
        navigateToDocument(
            created.session->
                documentId()));
}

void ProjectHubWindow::openDocument() {
    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return;
    }

    const auto refreshed =
        session->refreshDocuments();
    if (!refreshed.ok()) {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Document discovery"),
            fromUtf8(
                refreshed.diagnostic
                    .message));
        return;
    }

    OpenDocumentDialog dialog{
        openDocumentCandidates(
            session->documentIndex()),
        workspace_shell_};

    if (dialog.exec() !=
        QDialog::Accepted) {
        return;
    }

    const auto document_id =
        dialog.selectedDocumentId();
    if (!document_id) {
        return;
    }

    auto opened =
        session->openDocument(
            *document_id);
    if (!opened.ok()) {
        showFailure(
            opened.diagnostic);
        return;
    }

    ensureDocumentTab(
        opened.session->
            documentId());

    static_cast<void>(
        navigateToDocument(
            opened.session->
                documentId()));
}

void ProjectHubWindow::
refreshWorkspaceDocuments() {
    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return;
    }

    const auto refreshed =
        session->refreshDocuments();
    if (!refreshed.ok()) {
        QMessageBox::warning(
            this,
            QStringLiteral(
                "Document discovery"),
            fromUtf8(
                refreshed.diagnostic
                    .message));
        return;
    }

    for (const auto& id :
         session->openDocumentIds()) {
        updateDocumentTab(id);
    }
}

void ProjectHubWindow::
syncOpenDocumentTabs() {
    auto& tabs =
        workspace_shell_->
            documentTabs();
    const QSignalBlocker blocked{tabs};

    while (tabs.count() > 0) {
        tabs.removeTab(0);
    }

    auto* session =
        controller_.activeSession();
    if (session != nullptr) {
        for (const auto& id :
             session->openDocumentIds()) {
            ensureDocumentTab(id);
        }
    }

    tabs.setCurrentIndex(-1);
    tabs.setVisible(
        tabs.count() > 0);
}

void ProjectHubWindow::
ensureDocumentTab(
    const core::DocumentId& document_id) {
    if (tabIndexFor(document_id) >= 0) {
        updateDocumentTab(
            document_id);
        return;
    }

    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return;
    }

    auto* document_session =
        session->documentSession(
            document_id);
    if (document_session == nullptr) {
        return;
    }

    auto& tabs =
        workspace_shell_->
            documentTabs();
    const int index =
        tabs.addTab(
            partDisplayName(
                *document_session));
    tabs.setTabData(
        index,
        fromUtf8(
            document_id.value()));
    tabs.setVisible(true);

    updateDocumentTab(
        document_id);
}

void ProjectHubWindow::
updateDocumentTab(
    const core::DocumentId& document_id) {
    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return;
    }

    const int index =
        tabIndexFor(document_id);
    if (index < 0) {
        return;
    }

    const auto* document_session =
        session->documentSession(
            document_id);
    if (document_session == nullptr) {
        return;
    }

    auto label =
        partDisplayName(
            *document_session);
    if (document_session->needsSave()) {
        label += QStringLiteral(" *");
    }

    auto& tabs =
        workspace_shell_->
            documentTabs();
    tabs.setTabText(
        index,
        label);
    tabs.setTabToolTip(
        index,
        QStringLiteral("DocumentId: ") +
            fromUtf8(
                document_id.value()) +
            QStringLiteral("\nPath: ") +
            fromFilesystemPath(
                document_session->path()));
}

void ProjectHubWindow::
closeDocumentTab(int index) {
    const auto id =
        tabDocumentId(index);
    if (id) {
        closeDocument(*id);
    }
}

void ProjectHubWindow::closeDocument(
    const core::DocumentId& document_id) {
    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return;
    }

    auto* document_session =
        session->documentSession(
            document_id);
    if (document_session == nullptr) {
        return;
    }

    bool discard = false;
    if (document_session->needsSave()) {
        QMessageBox box{
            QMessageBox::Warning,
            QStringLiteral("Unsaved Part"),
            QStringLiteral(
                "This Part contains unsaved authored changes.\n"
                "Save them before closing?"),
            QMessageBox::NoButton,
            workspace_shell_};

        auto* save_button =
            box.addButton(
                QStringLiteral("Save"),
                QMessageBox::AcceptRole);
        auto* discard_button =
            box.addButton(
                QStringLiteral("Discard"),
                QMessageBox::DestructiveRole);
        auto* cancel_button =
            box.addButton(
                QStringLiteral("Cancel"),
                QMessageBox::RejectRole);

        box.exec();

        if (box.clickedButton() ==
                cancel_button ||
            box.clickedButton() ==
                nullptr) {
            return;
        }

        if (box.clickedButton() ==
            save_button) {
            const auto saved =
                document_session->save();
            if (!saved.ok()) {
                showFailure(
                    saved.diagnostic);
                return;
            }
            updateDocumentTab(
                document_id);
        } else if (
            box.clickedButton() ==
            discard_button) {
            discard = true;
        }
    }

    const bool closing_active =
        navigation_.kind ==
            ProjectNavigationKind::
                document &&
        navigation_.document_id &&
        *navigation_.document_id ==
            document_id;

    const int closing_index =
        tabIndexFor(document_id);

    // The Workbench holds non-owning pointers into the
    // DocumentSession. Drop those runtime bindings before
    // ProjectSession erases the session.
    cad_workbench_->
        forgetDocumentRuntimeState(
            document_id);

    if (!session->closeDocument(
            document_id,
            discard)) {
        if (closing_active) {
            static_cast<void>(
                navigateToDocument(
                    document_id));
        }
        return;
    }

    auto& tabs =
        workspace_shell_->
            documentTabs();

    std::optional<core::DocumentId>
        next_document;

    {
        const QSignalBlocker blocked{
            tabs};

        if (closing_index >= 0 &&
            closing_index <
                tabs.count()) {
            tabs.removeTab(
                closing_index);
        }

        if (closing_active &&
            tabs.count() > 0) {
            const int next_index =
                std::min(
                    closing_index,
                    tabs.count() - 1);
            next_document =
                tabDocumentId(
                    std::max(
                        next_index,
                        0));
        }

        if (!closing_active &&
            navigation_.kind ==
                ProjectNavigationKind::
                    document &&
            navigation_.document_id) {
            const int active_index =
                tabIndexFor(
                    *navigation_.
                        document_id);
            tabs.setCurrentIndex(
                active_index);
        } else {
            tabs.setCurrentIndex(-1);
        }

        tabs.setVisible(
            tabs.count() > 0);
    }

    if (closing_active) {
        navigation_.kind =
            ProjectNavigationKind::
                workspace;
        navigation_.document_id.reset();

        if (next_document) {
            static_cast<void>(
                navigateToDocument(
                    *next_document));
        } else {
            navigateToWorkspace();
        }
    }
}

int ProjectHubWindow::tabIndexFor(
    const core::DocumentId& document_id) const {
    if (workspace_shell_ == nullptr) {
        return -1;
    }

    const auto id =
        fromUtf8(
            document_id.value());
    const auto& tabs =
        workspace_shell_->
            documentTabs();

    for (int index = 0;
         index < tabs.count();
         ++index) {
        if (tabs.tabData(index)
                .toString() == id) {
            return index;
        }
    }

    return -1;
}

std::optional<core::DocumentId>
ProjectHubWindow::tabDocumentId(
    int index) const {
    if (workspace_shell_ == nullptr) {
        return std::nullopt;
    }

    const auto& tabs =
        workspace_shell_->
            documentTabs();
    if (index < 0 ||
        index >= tabs.count()) {
        return std::nullopt;
    }

    return core::DocumentId::parse(
        toUtf8(
            tabs.tabData(index)
                .toString()));
}

std::filesystem::path
ProjectHubWindow::defaultPartPath() const {
    const auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return "Part001.ss2part";
    }

    for (unsigned index = 1U;
         index <= 9999U;
         ++index) {
        std::ostringstream name;
        name << "Part"
             << std::setw(3)
             << std::setfill('0')
             << index
             << ".ss2part";

        const auto relative =
            std::filesystem::path{
                name.str()};
        std::error_code ec;
        if (!std::filesystem::exists(
                session->workspaceRoot() /
                    relative,
                ec) &&
            !ec) {
            return relative;
        }
    }

    return "Part.ss2part";
}

bool ProjectHubWindow::
requestCloseProject() {
    if (!controller_.hasActiveProject()) {
        return true;
    }

    auto* session =
        controller_.activeSession();
    if (session == nullptr) {
        return true;
    }

    bool discard = false;

    if (session->hasDirtyDocuments()) {
        QMessageBox box{
            QMessageBox::Warning,
            QStringLiteral(
                "Unsaved Parts"),
            QStringLiteral(
                "One or more open Parts contain unsaved authored changes.\n"
                "Save all changes before closing the Project?"),
            QMessageBox::NoButton,
            workspace_shell_};

        auto* save_button =
            box.addButton(
                QStringLiteral(
                    "Save All"),
                QMessageBox::AcceptRole);
        auto* discard_button =
            box.addButton(
                QStringLiteral(
                    "Discard"),
                QMessageBox::DestructiveRole);
        auto* cancel_button =
            box.addButton(
                QStringLiteral(
                    "Cancel"),
                QMessageBox::RejectRole);

        box.exec();

        if (box.clickedButton() ==
                cancel_button ||
            box.clickedButton() ==
                nullptr) {
            return false;
        }

        if (box.clickedButton() ==
            save_button) {
            const auto saved =
                session->
                    saveAllDirtyDocuments();
            if (!saved.ok()) {
                showFailure(
                    saved.diagnostic);
                return false;
            }

            for (const auto& id :
                 session->
                     openDocumentIds()) {
                updateDocumentTab(id);
            }
        } else if (
            box.clickedButton() ==
            discard_button) {
            discard = true;
        }
    }

    const auto previous_navigation =
        navigation_;

    // Detach the active Workbench before ProjectHubController
    // destroys the owning ProjectSession.
    cad_workbench_->
        clearProjectSession();

    if (!controller_.closeProject(
            discard)) {
        cad_workbench_->
            setProjectSession(
                session);
        syncOpenDocumentTabs();

        if (previous_navigation.kind ==
                ProjectNavigationKind::
                    document &&
            previous_navigation.document_id) {
            static_cast<void>(
                navigateToDocument(
                    *previous_navigation.
                        document_id));
        } else {
            navigateToWorkspace();
        }

        QMessageBox::warning(
            this,
            QStringLiteral(
                "Close Project"),
            QStringLiteral(
                "The Project still contains unsaved Part changes and remains open."));
        return false;
    }

    navigation_.kind =
        ProjectNavigationKind::
            workspace;
    navigation_.document_id.reset();

    auto& tabs =
        workspace_shell_->
            documentTabs();
    {
        const QSignalBlocker blocked{
            tabs};
        while (tabs.count() > 0) {
            tabs.removeTab(0);
        }
        tabs.setCurrentIndex(-1);
    }
    tabs.setVisible(false);
    workspace_shell_->showWorkspace();

    return true;
}

void ProjectHubWindow::closeEvent(
    QCloseEvent* event) {
    if (requestCloseProject()) {
        event->accept();
    } else {
        event->ignore();
    }
}

std::string
ProjectHubWindow::selectedProjectId()
    const {
    return internal::
        selectedRecentProjectId(
            *recent_list_);
}

void ProjectHubWindow::showFailure(
    const application::internal::
        ProjectHubDiagnostic& diagnostic) {
    auto message =
        fromUtf8(
            diagnostic.message);
    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral(
                "\n\nPath: ");
        message +=
            fromFilesystemPath(
                diagnostic.path);
    }

    QMessageBox::warning(
        this,
        QStringLiteral(
            "SimpleSolid Project"),
        message);
}

void ProjectHubWindow::showFailure(
    const application::
        ProjectDocumentDiagnostic&
            diagnostic) {
    auto message =
        fromUtf8(
            diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral(
                "\n\nPath: ");
        message +=
            fromFilesystemPath(
                diagnostic.path);
    }

    QMessageBox::warning(
        workspace_shell_,
        QStringLiteral(
            "SimpleSolid Document"),
        message);
}

void ProjectHubWindow::showFailure(
    const application::
        DocumentSessionDiagnostic&
            diagnostic) {
    auto message =
        fromUtf8(
            diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral(
                "\n\nPath: ");
        message +=
            fromFilesystemPath(
                diagnostic.path);
    }

    QMessageBox::warning(
        workspace_shell_,
        QStringLiteral(
            "SimpleSolid Document"),
        message);
}

} // namespace simplesolid2::ui
