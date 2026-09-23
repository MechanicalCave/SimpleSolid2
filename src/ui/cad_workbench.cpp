#include "cad_workbench.hpp"
#include "cad_workbench_shell.hpp"
#include "part_document_tree_controller.hpp"

#include <QAbstractItemView>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStyle>
#include <QTabBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr int documentIdRole = Qt::UserRole + 20;
constexpr int documentOpenableRole = Qt::UserRole + 21;

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

QString documentIndexStateLabel(application::DocumentIndexState state) {
    switch (state) {
    case application::DocumentIndexState::resolved:
        return QStringLiteral("Ready");
    case application::DocumentIndexState::identity_conflict:
        return QStringLiteral("Identity conflict");
    case application::DocumentIndexState::invalid:
        return QStringLiteral("Invalid Part");
    }
    return QStringLiteral("Unavailable");
}

QString partDisplayName(const application::DocumentSession& session) {
    const auto& title = session.document().properties().title;
    if (!title.empty()) {
        return fromUtf8(title);
    }

    const auto stem = session.path().stem();
    auto fallback = fromFilesystemPath(stem);
    if (fallback.isEmpty()) {
        fallback = QStringLiteral("<untitled Part>");
    }
    return fallback;
}

} // namespace

CadWorkbench::CadWorkbench(QWidget* parent)
    : QWidget{parent} {
    buildUi();
    clearProjectSession();
}

void CadWorkbench::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    shell_ = new CadWorkbenchShell(this);
    shell_->setObjectName(QStringLiteral("cadWorkbenchShell"));
    root->addWidget(shell_, 1);

    auto& lifecycle_actions = shell_->documentActionsLayout();

    new_part_button_ =
        new QPushButton(QStringLiteral("New Part…"), shell_);
    new_part_button_->setObjectName(QStringLiteral("newPartButton"));

    open_part_button_ =
        new QPushButton(QStringLiteral("Open Part…"), shell_);
    open_part_button_->setObjectName(QStringLiteral("openPartButton"));

    refresh_button_ =
        new QPushButton(QStringLiteral("Refresh"), shell_);
    refresh_button_->setObjectName(
        QStringLiteral("refreshDocumentsButton"));

    undo_button_ =
        new QPushButton(QStringLiteral("Undo"), shell_);
    undo_button_->setObjectName(QStringLiteral("undoDocumentButton"));

    redo_button_ =
        new QPushButton(QStringLiteral("Redo"), shell_);
    redo_button_->setObjectName(QStringLiteral("redoDocumentButton"));

    save_button_ =
        new QPushButton(QStringLiteral("Save"), shell_);
    save_button_->setObjectName(QStringLiteral("saveDocumentButton"));

    close_document_button_ =
        new QPushButton(QStringLiteral("Close"), shell_);
    close_document_button_->setObjectName(
        QStringLiteral("closeDocumentButton"));

    lifecycle_actions.addWidget(new_part_button_);
    lifecycle_actions.addWidget(open_part_button_);
    lifecycle_actions.addWidget(refresh_button_);
    lifecycle_actions.addStretch(1);
    lifecycle_actions.addWidget(undo_button_);
    lifecycle_actions.addWidget(redo_button_);
    lifecycle_actions.addWidget(save_button_);
    lifecycle_actions.addWidget(close_document_button_);

    document_tree_ = &shell_->documentTree();
    document_tabs_ = &shell_->documentTabs();
    status_ = &shell_->statusLabel();

    tree_controller_ =
        new PartDocumentTreeController(*document_tree_, this);
    tree_controller_->setResultHandler(
        [this](
            const application::DocumentSessionResult& result,
            bool visible) {
            if (!result.ok()) {
                showFailure(result.diagnostic);
                refreshActiveContext();
                return;
            }

            refreshActiveContext();
            status_->setText(
                result.changed
                    ? (visible
                           ? QStringLiteral(
                                 "Selected Origin references shown.")
                           : QStringLiteral(
                                 "Selected Origin references hidden."))
                    : QStringLiteral(
                          "No Origin visibility change."));
        });

    auto* editor_frame = new QFrame(shell_);
    editor_frame->setObjectName(QStringLiteral("editorSurface"));
    editor_frame->setFrameShape(QFrame::StyledPanel);
    auto* editor_layout = new QVBoxLayout(editor_frame);

    auto* editor_label = new QLabel(
        QStringLiteral(
            "3D Document View\n"
            "Viewer presentation is connected in a later WB-01 slice."),
        editor_frame);
    editor_label->setObjectName(
        QStringLiteral("editorSurfacePlaceholder"));
    editor_label->setAlignment(Qt::AlignCenter);
    editor_layout->addWidget(editor_label, 1);

    editor_surface_ = editor_frame;
    shell_->setEditorSurface(editor_surface_);

    auto* properties_content = new QWidget(shell_);
    properties_content->setObjectName(
        QStringLiteral("partPropertiesContent"));
    auto* properties_root = new QVBoxLayout(properties_content);
    properties_root->setContentsMargins(0, 0, 0, 0);

    active_path_ = new QLabel(properties_content);
    active_path_->setObjectName(QStringLiteral("activeDocumentPath"));
    active_path_->setWordWrap(true);

    active_id_ = new QLabel(properties_content);
    active_id_->setObjectName(QStringLiteral("activeDocumentId"));
    active_id_->setWordWrap(true);

    properties_root->addWidget(active_path_);
    properties_root->addWidget(active_id_);

    auto* form = new QFormLayout;

    number_ = new QLineEdit(properties_content);
    number_->setObjectName(QStringLiteral("documentNumberEdit"));

    title_ = new QLineEdit(properties_content);
    title_->setObjectName(QStringLiteral("documentTitleEdit"));

    description_ = new QPlainTextEdit(properties_content);
    description_->setObjectName(
        QStringLiteral("documentDescriptionEdit"));
    description_->setMaximumHeight(90);

    engineering_revision_ = new QLineEdit(properties_content);
    engineering_revision_->setObjectName(
        QStringLiteral("documentEngineeringRevisionEdit"));

    form->addRow(QStringLiteral("Number"), number_);
    form->addRow(QStringLiteral("Title"), title_);
    form->addRow(QStringLiteral("Description"), description_);
    form->addRow(
        QStringLiteral("Engineering revision"),
        engineering_revision_);
    properties_root->addLayout(form);

    apply_button_ = new QPushButton(
        QStringLiteral("Apply Properties"),
        properties_content);
    apply_button_->setObjectName(
        QStringLiteral("applyDocumentPropertiesButton"));
    properties_root->addWidget(apply_button_);
    properties_root->addStretch(1);

    shell_->setPropertiesContent(properties_content);

    auto* operations_content = new QWidget(shell_);
    operations_content->setObjectName(
        QStringLiteral("partOperationsContent"));
    auto* operations_layout = new QVBoxLayout(operations_content);
    operations_layout->setContentsMargins(0, 0, 0, 0);

    operations_placeholder_ = new QLabel(
        QStringLiteral(
            "No modeling operations are available in this scope."),
        operations_content);
    operations_placeholder_->setObjectName(
        QStringLiteral("operationsPlaceholder"));
    operations_placeholder_->setWordWrap(true);
    operations_layout->addWidget(operations_placeholder_);
    operations_layout->addStretch(1);

    shell_->setOperationsContent(operations_content);

    QObject::connect(
        new_part_button_,
        &QPushButton::clicked,
        this,
        [this] { newPart(); });
    QObject::connect(
        open_part_button_,
        &QPushButton::clicked,
        this,
        [this] { openPart(); });
    QObject::connect(
        refresh_button_,
        &QPushButton::clicked,
        this,
        [this] { refreshWorkspaceIndex(); });
    QObject::connect(
        undo_button_,
        &QPushButton::clicked,
        this,
        [this] { undo(); });
    QObject::connect(
        redo_button_,
        &QPushButton::clicked,
        this,
        [this] { redo(); });
    QObject::connect(
        save_button_,
        &QPushButton::clicked,
        this,
        [this] { save(); });
    QObject::connect(
        close_document_button_,
        &QPushButton::clicked,
        this,
        [this] { closeActiveDocument(); });
    QObject::connect(
        apply_button_,
        &QPushButton::clicked,
        this,
        [this] { applyProperties(); });
    QObject::connect(
        document_tabs_,
        &QTabBar::currentChanged,
        this,
        [this](int index) { activateTab(index); });
    QObject::connect(
        document_tabs_,
        &QTabBar::tabCloseRequested,
        this,
        [this](int index) { closeTab(index); });
}

void CadWorkbench::setProjectSession(
    application::ProjectSession* session) {
    session_ = session;
    active_document_id_.reset();
    syncOpenTabs();
    refreshWorkspaceIndex();

    if (document_tabs_->count() > 0) {
        activateTab(document_tabs_->currentIndex());
    } else {
        clearActiveContext();
        status_->setText(
            QStringLiteral(
                "Project open. Create or open a Part Document."));
    }
}

void CadWorkbench::clearProjectSession() {
    session_ = nullptr;
    active_document_id_.reset();

    {
        const QSignalBlocker blocked{document_tabs_};
        while (document_tabs_->count() > 0) {
            document_tabs_->removeTab(0);
        }
        document_tabs_->setCurrentIndex(-1);
    }

    new_part_button_->setEnabled(false);
    open_part_button_->setEnabled(false);
    refresh_button_->setEnabled(false);
    clearActiveContext();
    status_->setText(QStringLiteral("No Project is open."));
}

void CadWorkbench::refreshWorkspaceIndex() {
    if (session_ == nullptr) {
        status_->setText(QStringLiteral("No Project is open."));
        return;
    }

    new_part_button_->setEnabled(true);
    open_part_button_->setEnabled(true);
    refresh_button_->setEnabled(true);

    const auto refreshed = session_->refreshDocuments();
    if (!refreshed.ok()) {
        status_->setText(
            QStringLiteral("Document discovery warning: ") +
            fromUtf8(refreshed.diagnostic.message));
        return;
    }

    status_->setText(
        QStringLiteral("%1 native Part entr%2 available in Workspace.")
            .arg(
                static_cast<qulonglong>(
                    session_->documentIndex().entries().size()))
            .arg(
                session_->documentIndex().entries().size() == 1U
                    ? QStringLiteral("y")
                    : QStringLiteral("ies")));

    for (const auto& id : session_->openDocumentIds()) {
        updateTabPresentation(id);
    }
}

void CadWorkbench::syncOpenTabs() {
    const QSignalBlocker blocked{document_tabs_};

    while (document_tabs_->count() > 0) {
        document_tabs_->removeTab(0);
    }

    if (session_ == nullptr) {
        document_tabs_->setCurrentIndex(-1);
        return;
    }

    for (const auto& id : session_->openDocumentIds()) {
        ensureDocumentTab(id);
    }

    document_tabs_->setCurrentIndex(
        document_tabs_->count() > 0 ? 0 : -1);
}

void CadWorkbench::ensureDocumentTab(
    const core::DocumentId& document_id) {
    if (session_ == nullptr || tabIndexFor(document_id) >= 0) return;

    auto* document_session = session_->documentSession(document_id);
    if (document_session == nullptr) return;

    const int index = document_tabs_->addTab(
        partDisplayName(*document_session));
    document_tabs_->setTabData(
        index,
        fromUtf8(document_id.value()));
    updateTabPresentation(document_id);
}

bool CadWorkbench::activateDocument(
    const core::DocumentId& document_id) {
    if (session_ == nullptr) return false;

    auto opened = session_->openDocument(document_id);
    if (!opened.ok()) {
        showFailure(opened.diagnostic);
        return false;
    }

    ensureDocumentTab(document_id);
    const int index = tabIndexFor(document_id);
    if (index < 0) return false;

    document_tabs_->setCurrentIndex(index);
    if (document_tabs_->currentIndex() == index) {
        activateTab(index);
    }

    status_->setText(
        opened.reused_session
            ? QStringLiteral(
                  "Part was already open; activated existing DocumentSession.")
            : QStringLiteral("Part opened."));
    return true;
}

void CadWorkbench::activateTab(int index) {
    const auto id = tabDocumentId(index);
    if (!id || session_ == nullptr ||
        session_->documentSession(*id) == nullptr) {
        active_document_id_.reset();
        clearActiveContext();
        return;
    }

    active_document_id_ = *id;
    refreshActiveContext();
}

int CadWorkbench::tabIndexFor(
    const core::DocumentId& document_id) const {
    const auto id = fromUtf8(document_id.value());
    for (int index = 0; index < document_tabs_->count(); ++index) {
        if (document_tabs_->tabData(index).toString() == id) {
            return index;
        }
    }
    return -1;
}

std::optional<core::DocumentId>
CadWorkbench::tabDocumentId(int index) const {
    if (index < 0 || index >= document_tabs_->count()) {
        return std::nullopt;
    }

    const auto encoded =
        toUtf8(document_tabs_->tabData(index).toString());
    return core::DocumentId::parse(encoded);
}

std::filesystem::path CadWorkbench::defaultPartPath() const {
    if (session_ == nullptr) return "Part001.ss2part";

    for (unsigned index = 1U; index <= 9999U; ++index) {
        std::ostringstream name;
        name << "Part" << std::setw(3) << std::setfill('0')
             << index << ".ss2part";

        const auto relative = std::filesystem::path{name.str()};
        std::error_code ec;
        if (!std::filesystem::exists(
                session_->workspaceRoot() / relative,
                ec) &&
            !ec) {
            return relative;
        }
    }

    return "Part.ss2part";
}

void CadWorkbench::newPart() {
    if (session_ == nullptr) return;

    bool accepted = false;
    const auto entered = QInputDialog::getText(
        this,
        QStringLiteral("New Part"),
        QStringLiteral(
            "Workspace-relative file path (.ss2part).\n"
            "Existing parent folders may be used, for example "
            "Parts/Shaft.ss2part:"),
        QLineEdit::Normal,
        fromFilesystemPath(defaultPartPath()),
        &accepted);

    if (!accepted || entered.trimmed().isEmpty()) return;

    auto relative = toFilesystemPath(entered.trimmed());
    if (!relative.has_extension()) {
        relative += ".ss2part";
    }

    auto created = session_->createPart(relative);
    if (!created.ok()) {
        showFailure(created.diagnostic);
        refreshWorkspaceIndex();
        return;
    }

    ensureDocumentTab(created.session->documentId());
    const int index =
        tabIndexFor(created.session->documentId());
    document_tabs_->setCurrentIndex(index);
    activateTab(index);
    refreshWorkspaceIndex();
    status_->setText(QStringLiteral("New Part created and saved."));
}

void CadWorkbench::openPart() {
    if (session_ == nullptr) return;

    const auto refreshed = session_->refreshDocuments();
    if (!refreshed.ok()) {
        status_->setText(
            QStringLiteral("Document discovery warning: ") +
            fromUtf8(refreshed.diagnostic.message));
    }

    QDialog dialog{this};
    dialog.setWindowTitle(QStringLiteral("Open Part"));
    dialog.resize(620, 420);

    auto* layout = new QVBoxLayout(&dialog);
    auto* explanation = new QLabel(
        QStringLiteral(
            "Select a resolved native Part Document. "
            "Identity conflicts and invalid files remain visible "
            "but cannot be opened."),
        &dialog);
    explanation->setWordWrap(true);
    layout->addWidget(explanation);

    auto* list = new QListWidget(&dialog);
    list->setObjectName(QStringLiteral("openPartDocumentList"));
    list->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(list, 1);

    for (const auto& entry : session_->documentIndex().entries()) {
        QString text;
        if (entry.state == application::DocumentIndexState::resolved) {
            text = entry.title.empty()
                ? QStringLiteral("<untitled>")
                : fromUtf8(entry.title);
        } else {
            text =
                QStringLiteral("⚠ ") +
                documentIndexStateLabel(entry.state);
        }

        for (const auto& path : entry.relative_paths) {
            text += QStringLiteral("\n") +
                    fromFilesystemPath(path);
        }

        auto* item = new QListWidgetItem(text, list);
        const bool openable =
            entry.state == application::DocumentIndexState::resolved &&
            entry.document_id.has_value();

        item->setData(documentOpenableRole, openable);
        if (entry.document_id) {
            item->setData(
                documentIdRole,
                fromUtf8(entry.document_id->value()));
        }

        QString tooltip;
        if (entry.document_id) {
            tooltip =
                QStringLiteral("DocumentId: ") +
                fromUtf8(entry.document_id->value());
        }
        if (!entry.diagnostic.empty()) {
            if (!tooltip.isEmpty()) {
                tooltip += QStringLiteral("\n");
            }
            tooltip += fromUtf8(entry.diagnostic);
        }
        item->setToolTip(tooltip);

        if (!openable) {
            item->setIcon(
                style()->standardIcon(QStyle::SP_MessageBoxWarning));
        }
    }

    auto* buttons = new QDialogButtonBox(
        QDialogButtonBox::Open | QDialogButtonBox::Cancel,
        &dialog);
    auto* open_button = buttons->button(QDialogButtonBox::Open);
    open_button->setEnabled(false);
    layout->addWidget(buttons);

    const auto sync_open_enabled = [list, open_button] {
        const auto* item = list->currentItem();
        open_button->setEnabled(
            item != nullptr &&
            item->data(documentOpenableRole).toBool());
    };

    QObject::connect(
        list,
        &QListWidget::itemSelectionChanged,
        &dialog,
        sync_open_enabled);
    QObject::connect(
        buttons,
        &QDialogButtonBox::accepted,
        &dialog,
        [&dialog, list] {
            const auto* item = list->currentItem();
            if (item != nullptr &&
                item->data(documentOpenableRole).toBool()) {
                dialog.accept();
            }
        });
    QObject::connect(
        buttons,
        &QDialogButtonBox::rejected,
        &dialog,
        &QDialog::reject);
    QObject::connect(
        list,
        &QListWidget::itemDoubleClicked,
        &dialog,
        [&dialog](QListWidgetItem* item) {
            if (item != nullptr &&
                item->data(documentOpenableRole).toBool()) {
                dialog.accept();
            }
        });

    if (dialog.exec() != QDialog::Accepted) return;

    const auto* item = list->currentItem();
    if (item == nullptr) return;

    const auto parsed = core::DocumentId::parse(
        toUtf8(item->data(documentIdRole).toString()));
    if (!parsed) return;

    static_cast<void>(activateDocument(*parsed));
}

application::DocumentSession*
CadWorkbench::activeDocumentSession() noexcept {
    if (session_ == nullptr || !active_document_id_) return nullptr;
    return session_->documentSession(*active_document_id_);
}

const application::DocumentSession*
CadWorkbench::activeDocumentSession() const noexcept {
    if (session_ == nullptr || !active_document_id_) return nullptr;
    return session_->documentSession(*active_document_id_);
}

void CadWorkbench::applyProperties() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    core::DocumentProperties properties;
    properties.number = toUtf8(number_->text());
    properties.title = toUtf8(title_->text());
    properties.description =
        toUtf8(description_->toPlainText());
    properties.engineering_revision =
        toUtf8(engineering_revision_->text());

    const auto result = document_session->execute(
        application::SetDocumentPropertiesCommand{
            std::move(properties)});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }

    refreshActiveContext();
    status_->setText(
        result.changed
            ? QStringLiteral(
                  "Properties changed — save is required.")
            : QStringLiteral("No authored property change."));
}

void CadWorkbench::undo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result = document_session->undo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    status_->setText(
        result.changed
            ? QStringLiteral("Undo applied.")
            : QStringLiteral("Nothing to undo."));
}

void CadWorkbench::redo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result = document_session->redo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    status_->setText(
        result.changed
            ? QStringLiteral("Redo applied.")
            : QStringLiteral("Nothing to redo."));
}

void CadWorkbench::save() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result = document_session->save();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshWorkspaceIndex();
    refreshActiveContext();
    status_->setText(QStringLiteral("Part saved."));
}

void CadWorkbench::closeActiveDocument() {
    if (!active_document_id_) return;
    const int index = tabIndexFor(*active_document_id_);
    if (index >= 0) {
        closeTab(index);
    }
}

void CadWorkbench::closeTab(int index) {
    if (session_ == nullptr) return;

    const auto id = tabDocumentId(index);
    if (!id) return;

    auto* document_session = session_->documentSession(*id);
    if (document_session == nullptr) {
        document_tabs_->removeTab(index);
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
            this};

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

        if (box.clickedButton() == cancel_button ||
            box.clickedButton() == nullptr) {
            return;
        }

        if (box.clickedButton() == save_button) {
            const auto saved = document_session->save();
            if (!saved.ok()) {
                showFailure(saved.diagnostic);
                return;
            }
        } else if (box.clickedButton() == discard_button) {
            discard = true;
        }
    }

    if (!session_->closeDocument(*id, discard)) {
        status_->setText(
            QStringLiteral(
                "Part remains open because it still has unsaved changes."));
        return;
    }

    const bool closing_active =
        active_document_id_.has_value() &&
        *active_document_id_ == *id;

    int next_index = -1;
    {
        const QSignalBlocker blocked{document_tabs_};
        document_tabs_->removeTab(index);
        if (document_tabs_->count() > 0) {
            next_index = std::min(
                index,
                document_tabs_->count() - 1);
            document_tabs_->setCurrentIndex(next_index);
        } else {
            document_tabs_->setCurrentIndex(-1);
        }
    }

    if (closing_active) {
        active_document_id_.reset();
        if (next_index >= 0) {
            activateTab(next_index);
        } else {
            clearActiveContext();
        }
    }

    status_->setText(QStringLiteral("Part closed."));
}

void CadWorkbench::refreshActiveContext() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) {
        clearActiveContext();
        return;
    }

    const auto& properties =
        document_session->document().properties();

    number_->setText(fromUtf8(properties.number));
    title_->setText(fromUtf8(properties.title));
    description_->setPlainText(
        fromUtf8(properties.description));
    engineering_revision_->setText(
        fromUtf8(properties.engineering_revision));

    std::error_code ec;
    const auto relative = std::filesystem::relative(
        document_session->path(),
        session_->workspaceRoot(),
        ec);

    active_path_->setText(
        QStringLiteral("Path: ") +
        fromFilesystemPath(
            ec ? document_session->path() : relative));

    active_id_->setText(
        QStringLiteral("DocumentId: ") +
        fromUtf8(document_session->documentId().value()));

    number_->setEnabled(true);
    title_->setEnabled(true);
    description_->setEnabled(true);
    engineering_revision_->setEnabled(true);

    tree_controller_->setDocumentSession(document_session);
    updateTabPresentation(document_session->documentId());
    syncActionState();
}

void CadWorkbench::clearActiveContext() {
    active_path_->setText(QStringLiteral("No Part is open."));
    active_id_->clear();

    number_->clear();
    title_->clear();
    description_->clear();
    engineering_revision_->clear();

    number_->setEnabled(false);
    title_->setEnabled(false);
    description_->setEnabled(false);
    engineering_revision_->setEnabled(false);

    tree_controller_->clear();
    syncActionState();
}

void CadWorkbench::syncActionState() {
    const auto* document_session = activeDocumentSession();
    const bool active = document_session != nullptr;

    apply_button_->setEnabled(active);
    undo_button_->setEnabled(
        active && document_session->canUndo());
    redo_button_->setEnabled(
        active && document_session->canRedo());
    save_button_->setEnabled(
        active && document_session->needsSave());
    close_document_button_->setEnabled(active);

    new_part_button_->setEnabled(session_ != nullptr);
    open_part_button_->setEnabled(session_ != nullptr);
    refresh_button_->setEnabled(session_ != nullptr);
}

void CadWorkbench::updateTabPresentation(
    const core::DocumentId& document_id) {
    if (session_ == nullptr) return;

    const int index = tabIndexFor(document_id);
    if (index < 0) return;

    const auto* document_session =
        session_->documentSession(document_id);
    if (document_session == nullptr) return;

    auto label = partDisplayName(*document_session);
    if (document_session->needsSave()) {
        label += QStringLiteral(" *");
    }

    document_tabs_->setTabText(index, label);
    document_tabs_->setTabToolTip(
        index,
        QStringLiteral("DocumentId: ") +
            fromUtf8(document_id.value()) +
            QStringLiteral("\nPath: ") +
            fromFilesystemPath(document_session->path()));
}

ProjectCloseDisposition CadWorkbench::prepareProjectClose() {
    if (session_ == nullptr ||
        !session_->hasDirtyDocuments()) {
        return ProjectCloseDisposition::clean;
    }

    QMessageBox box{
        QMessageBox::Warning,
        QStringLiteral("Unsaved Parts"),
        QStringLiteral(
            "One or more open Parts contain unsaved authored changes.\n"
            "Save all changes before closing the Project?"),
        QMessageBox::NoButton,
        this};

    auto* save_button =
        box.addButton(
            QStringLiteral("Save All"),
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

    if (box.clickedButton() == cancel_button ||
        box.clickedButton() == nullptr) {
        return ProjectCloseDisposition::cancel;
    }

    if (box.clickedButton() == discard_button) {
        return ProjectCloseDisposition::discard;
    }

    if (box.clickedButton() == save_button) {
        const auto saved =
            session_->saveAllDirtyDocuments();
        if (!saved.ok()) {
            showFailure(saved.diagnostic);
            return ProjectCloseDisposition::cancel;
        }

        for (const auto& id : session_->openDocumentIds()) {
            updateTabPresentation(id);
        }
        syncActionState();
        return ProjectCloseDisposition::clean;
    }

    return ProjectCloseDisposition::cancel;
}

void CadWorkbench::showFailure(
    const application::ProjectDocumentDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral("\n\nPath: ") +
            fromFilesystemPath(diagnostic.path);
    }

    if (!diagnostic.candidates.empty()) {
        message += QStringLiteral(
            "\n\nConflicting locations:");
        for (const auto& path : diagnostic.candidates) {
            message +=
                QStringLiteral("\n• ") +
                fromFilesystemPath(path);
        }
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
}

void CadWorkbench::showFailure(
    const application::DocumentSessionDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral("\n\nPath: ") +
            fromFilesystemPath(diagnostic.path);
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
}

} // namespace simplesolid2::ui
