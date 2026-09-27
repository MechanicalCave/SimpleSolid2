#include "project_workspace_shell.hpp"

#include <QAbstractSpinBox>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QFrame>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QTextEdit>
#include <QVBoxLayout>

#include <algorithm>
#include <string>
#include <string_view>

namespace simplesolid2::ui {
namespace {

std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return std::string{
        bytes.constData(),
        static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(value.size()));
}

bool isPrintableText(const QString& text) {
    return !text.isEmpty() &&
           std::all_of(
               text.cbegin(),
               text.cend(),
               [](QChar ch) {
                   return ch.isPrint();
               });
}

} // namespace

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

    command_line_widget_ = new QFrame(this);
    command_line_widget_->setObjectName(
        QStringLiteral("cadCommandLine"));
    auto* command_layout =
        new QHBoxLayout(command_line_widget_);
    command_layout->setContentsMargins(6, 3, 6, 3);

    command_prompt_ =
        new QLabel(
            QStringLiteral("Command:"),
            command_line_widget_);
    command_prompt_->setObjectName(
        QStringLiteral("cadCommandPrompt"));
    command_layout->addWidget(command_prompt_);

    command_input_ =
        new QLineEdit(command_line_widget_);
    command_input_->setObjectName(
        QStringLiteral("cadCommandInput"));
    command_input_->setPlaceholderText(
        QStringLiteral("Type a CAD command or value"));
    command_layout->addWidget(command_input_, 1);

    command_diagnostic_ =
        new QLabel(command_line_widget_);
    command_diagnostic_->setObjectName(
        QStringLiteral("cadCommandDiagnostic"));
    command_diagnostic_->setWordWrap(false);
    command_diagnostic_->setTextFormat(Qt::PlainText);
    command_diagnostic_->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    // Reserve diagnostic space permanently so a rejected token cannot
    // resize the input field or reflow the Workbench vertically.
    command_diagnostic_->setFixedWidth(320);
    command_diagnostic_->setVisible(true);
    command_layout->addWidget(command_diagnostic_);

    // The Command Line is a stable one-row Workspace surface. Long
    // diagnostics are elided inside their reserved region instead of
    // increasing the shell height and shrinking the Viewer.
    command_line_widget_->setFixedHeight(
        command_input_->sizeHint().height() + 6);

    root->addWidget(command_line_widget_);

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

    QObject::connect(
        command_input_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text) {
            if (syncing_command_input_) {
                return;
            }
            cad_input_.setBuffer(toUtf8(text));
            refreshCadInputPresentation();
        });
    QObject::connect(
        command_input_,
        &QLineEdit::returnPressed,
        this,
        [this] {
            submitCadInputFromLineEdit();
        });

    if (qApp != nullptr) {
        qApp->installEventFilter(this);
    }
    refreshCadInputPresentation();
}

ProjectWorkspaceShell::~ProjectWorkspaceShell() {
    if (qApp != nullptr) {
        qApp->removeEventFilter(this);
    }
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

    cad_input_.detachEndpoint();
    refreshCadInputPresentation();

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
    cad_input_.detachEndpoint();
    refreshCadInputPresentation();

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
    refreshCadInputPresentation();
}

void ProjectWorkspaceShell::setCadInputEndpoint(
    application::ICadInputEndpoint* endpoint) {
    if (endpoint == nullptr) {
        cad_input_.detachEndpoint();
    } else {
        cad_input_.attachEndpoint(endpoint);
    }
    refreshCadInputPresentation();
}

void ProjectWorkspaceShell::refreshCadInputPresentation() {
    if (command_prompt_ != nullptr) {
        command_prompt_->setText(
            fromUtf8(cad_input_.prompt()));
    }

    syncCadInputLineEdit();

    if (command_diagnostic_ != nullptr) {
        const auto full_diagnostic =
            fromUtf8(cad_input_.diagnostic());
        command_diagnostic_->setToolTip(
            full_diagnostic);

        const int available_width =
            std::max(
                0,
                command_diagnostic_->width() - 4);
        command_diagnostic_->setText(
            command_diagnostic_->fontMetrics().
                elidedText(
                    full_diagnostic,
                    Qt::ElideRight,
                    available_width));
    }
}

const std::string&
ProjectWorkspaceShell::cadInputBuffer() const noexcept {
    return cad_input_.buffer();
}

bool ProjectWorkspaceShell::showingWorkspace() const noexcept {
    return content_->currentWidget() ==
           dashboard_;
}

bool ProjectWorkspaceShell::focusOwnsTextInput(
    QWidget* focus) const noexcept {
    if (focus == nullptr ||
        focus == command_input_) {
        return false;
    }

    if (qobject_cast<QLineEdit*>(focus) != nullptr ||
        qobject_cast<QPlainTextEdit*>(focus) != nullptr ||
        qobject_cast<QTextEdit*>(focus) != nullptr ||
        qobject_cast<QAbstractSpinBox*>(focus) != nullptr) {
        return true;
    }

    auto* combo = qobject_cast<QComboBox*>(focus);
    return combo != nullptr && combo->isEditable();
}

void ProjectWorkspaceShell::syncCadInputLineEdit() {
    if (command_input_ == nullptr) {
        return;
    }

    const auto current =
        fromUtf8(cad_input_.buffer());
    if (command_input_->text() == current) {
        return;
    }

    syncing_command_input_ = true;
    command_input_->setText(current);
    syncing_command_input_ = false;
}

void ProjectWorkspaceShell::submitCadInputFromLineEdit() {
    const auto result = cad_input_.submit();
    refreshCadInputPresentation();

    if (result.accepted &&
        document_workbench_ != nullptr) {
        document_workbench_->setFocus(
            Qt::OtherFocusReason);
    }
}

bool ProjectWorkspaceShell::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (event == nullptr ||
        event->type() != QEvent::KeyPress ||
        !isVisible()) {
        return QWidget::eventFilter(
            watched,
            event);
    }

    if (QApplication::activeModalWidget() != nullptr) {
        return QWidget::eventFilter(
            watched,
            event);
    }

    auto* key_event =
        static_cast<QKeyEvent*>(event);
    auto* focus = QApplication::focusWidget();

    if (focus == command_input_) {
        if (key_event->key() == Qt::Key_Escape) {
            cad_input_.clearBuffer();
            refreshCadInputPresentation();
            if (document_workbench_ != nullptr) {
                document_workbench_->setFocus(
                    Qt::OtherFocusReason);
            }
            return true;
        }
        return QWidget::eventFilter(
            watched,
            event);
    }

    if (focusOwnsTextInput(focus) ||
        !cad_input_.hasEndpoint()) {
        return QWidget::eventFilter(
            watched,
            event);
    }

    const auto modifiers = key_event->modifiers();
    if ((modifiers & Qt::ControlModifier) ||
        (modifiers & Qt::AltModifier) ||
        (modifiers & Qt::MetaModifier)) {
        return QWidget::eventFilter(
            watched,
            event);
    }

    if (key_event->key() == Qt::Key_Backspace) {
        if (cad_input_.backspace()) {
            refreshCadInputPresentation();
            return true;
        }
        return QWidget::eventFilter(
            watched,
            event);
    }

    if (key_event->key() == Qt::Key_Escape) {
        if (!cad_input_.buffer().empty()) {
            cad_input_.clearBuffer();
            refreshCadInputPresentation();
            return true;
        }
        return QWidget::eventFilter(
            watched,
            event);
    }

    if (key_event->key() == Qt::Key_Return ||
        key_event->key() == Qt::Key_Enter) {
        if (!cad_input_.buffer().empty()) {
            static_cast<void>(
                cad_input_.submit());
            refreshCadInputPresentation();
            return true;
        }
        return QWidget::eventFilter(
            watched,
            event);
    }

    if (key_event->key() == Qt::Key_Space &&
        cad_input_.buffer().empty()) {
        return QWidget::eventFilter(
            watched,
            event);
    }

    const auto text = key_event->text();
    if (isPrintableText(text)) {
        cad_input_.appendText(toUtf8(text));
        refreshCadInputPresentation();
        return true;
    }

    return QWidget::eventFilter(
        watched,
        event);
}

} // namespace simplesolid2::ui
