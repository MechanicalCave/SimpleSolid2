#include "part_workspace_panel.hpp"

#include <QAbstractItemView>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QString>
#include <QStyle>
#include <QVBoxLayout>

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
    return std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
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

QString stateLabel(application::DocumentIndexState state) {
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

} // namespace

PartWorkspacePanel::PartWorkspacePanel(QWidget* parent)
    : QWidget{parent} {
    buildUi();
    clearProjectSession();
}

void PartWorkspacePanel::buildUi() {
    auto* root = new QHBoxLayout(this);

    auto* documents_group = new QGroupBox(QStringLiteral("Part Documents"), this);
    auto* documents_layout = new QVBoxLayout(documents_group);

    document_list_ = new QListWidget(documents_group);
    document_list_->setObjectName(QStringLiteral("documentList"));
    document_list_->setSelectionMode(QAbstractItemView::SingleSelection);
    document_list_->setAlternatingRowColors(true);
    documents_layout->addWidget(document_list_, 1);

    auto* document_actions = new QHBoxLayout;
    new_part_button_ = new QPushButton(QStringLiteral("New Part…"), documents_group);
    new_part_button_->setObjectName(QStringLiteral("newPartButton"));
    refresh_button_ = new QPushButton(QStringLiteral("Refresh"), documents_group);
    refresh_button_->setObjectName(QStringLiteral("refreshDocumentsButton"));
    open_button_ = new QPushButton(QStringLiteral("Open"), documents_group);
    open_button_->setObjectName(QStringLiteral("openDocumentButton"));
    document_actions->addWidget(new_part_button_);
    document_actions->addWidget(refresh_button_);
    document_actions->addWidget(open_button_);
    documents_layout->addLayout(document_actions);

    list_status_ = new QLabel(documents_group);
    list_status_->setObjectName(QStringLiteral("documentListStatus"));
    list_status_->setWordWrap(true);
    documents_layout->addWidget(list_status_);

    root->addWidget(documents_group, 1);

    auto* properties_group =
        new QGroupBox(QStringLiteral("Document Properties"), this);
    auto* properties_root = new QVBoxLayout(properties_group);

    active_path_ = new QLabel(properties_group);
    active_path_->setObjectName(QStringLiteral("activeDocumentPath"));
    active_path_->setWordWrap(true);
    active_id_ = new QLabel(properties_group);
    active_id_->setObjectName(QStringLiteral("activeDocumentId"));
    active_id_->setWordWrap(true);
    properties_root->addWidget(active_path_);
    properties_root->addWidget(active_id_);

    auto* form = new QFormLayout;
    number_ = new QLineEdit(properties_group);
    number_->setObjectName(QStringLiteral("documentNumberEdit"));
    title_ = new QLineEdit(properties_group);
    title_->setObjectName(QStringLiteral("documentTitleEdit"));
    description_ = new QPlainTextEdit(properties_group);
    description_->setObjectName(QStringLiteral("documentDescriptionEdit"));
    description_->setMaximumHeight(100);
    engineering_revision_ = new QLineEdit(properties_group);
    engineering_revision_->setObjectName(
        QStringLiteral("documentEngineeringRevisionEdit"));

    form->addRow(QStringLiteral("Number"), number_);
    form->addRow(QStringLiteral("Title"), title_);
    form->addRow(QStringLiteral("Description"), description_);
    form->addRow(QStringLiteral("Engineering revision"), engineering_revision_);
    properties_root->addLayout(form);

    apply_button_ =
        new QPushButton(QStringLiteral("Apply Properties"), properties_group);
    apply_button_->setObjectName(QStringLiteral("applyDocumentPropertiesButton"));
    properties_root->addWidget(apply_button_);

    auto* history_actions = new QHBoxLayout;
    undo_button_ = new QPushButton(QStringLiteral("Undo"), properties_group);
    undo_button_->setObjectName(QStringLiteral("undoDocumentButton"));
    redo_button_ = new QPushButton(QStringLiteral("Redo"), properties_group);
    redo_button_->setObjectName(QStringLiteral("redoDocumentButton"));
    save_button_ = new QPushButton(QStringLiteral("Save"), properties_group);
    save_button_->setObjectName(QStringLiteral("saveDocumentButton"));
    close_button_ = new QPushButton(QStringLiteral("Close Part"), properties_group);
    close_button_->setObjectName(QStringLiteral("closeDocumentButton"));
    history_actions->addWidget(undo_button_);
    history_actions->addWidget(redo_button_);
    history_actions->addWidget(save_button_);
    history_actions->addWidget(close_button_);
    properties_root->addLayout(history_actions);

    active_status_ = new QLabel(properties_group);
    active_status_->setObjectName(QStringLiteral("activeDocumentStatus"));
    active_status_->setWordWrap(true);
    properties_root->addWidget(active_status_);
    properties_root->addStretch(1);

    root->addWidget(properties_group, 1);

    QObject::connect(
        new_part_button_, &QPushButton::clicked, this, [this] { newPart(); });
    QObject::connect(
        refresh_button_, &QPushButton::clicked, this, [this] { refreshDocuments(); });
    QObject::connect(
        open_button_, &QPushButton::clicked, this, [this] { openSelectedDocument(); });
    QObject::connect(
        document_list_,
        &QListWidget::itemDoubleClicked,
        this,
        [this](QListWidgetItem*) { openSelectedDocument(); });
    QObject::connect(
        document_list_,
        &QListWidget::itemSelectionChanged,
        this,
        [this] { syncDocumentActionState(); });
    QObject::connect(
        apply_button_, &QPushButton::clicked, this, [this] { applyProperties(); });
    QObject::connect(
        undo_button_, &QPushButton::clicked, this, [this] { undo(); });
    QObject::connect(
        redo_button_, &QPushButton::clicked, this, [this] { redo(); });
    QObject::connect(
        save_button_, &QPushButton::clicked, this, [this] { save(); });
    QObject::connect(
        close_button_, &QPushButton::clicked, this, [this] { closeActiveDocument(); });
}

void PartWorkspacePanel::setProjectSession(application::ProjectSession* session) {
    session_ = session;
    active_document_id_.reset();
    clearActiveDocument();
    refreshDocuments();
}

void PartWorkspacePanel::clearProjectSession() {
    session_ = nullptr;
    active_document_id_.reset();
    document_list_->clear();
    list_status_->setText(QStringLiteral("No Project is open."));
    new_part_button_->setEnabled(false);
    refresh_button_->setEnabled(false);
    open_button_->setEnabled(false);
    clearActiveDocument();
}

void PartWorkspacePanel::refreshDocuments() {
    document_list_->clear();
    open_button_->setEnabled(false);

    if (session_ == nullptr) {
        list_status_->setText(QStringLiteral("No Project is open."));
        return;
    }

    new_part_button_->setEnabled(true);
    refresh_button_->setEnabled(true);

    const auto refreshed = session_->refreshDocuments();
    if (!refreshed.ok()) {
        list_status_->setText(
            QStringLiteral("Document discovery warning: ") +
            fromUtf8(refreshed.diagnostic.message));
    }

    const auto& entries = session_->documentIndex().entries();
    for (const auto& entry : entries) {
        QString text;
        if (entry.state == application::DocumentIndexState::resolved) {
            const auto title =
                entry.title.empty() ? QStringLiteral("<untitled>")
                                    : fromUtf8(entry.title);
            text = title;
            if (!entry.relative_paths.empty()) {
                text += QStringLiteral("\n") +
                        fromFilesystemPath(entry.relative_paths.front());
            }
        } else {
            text = QStringLiteral("⚠ ") + stateLabel(entry.state);
            for (const auto& path : entry.relative_paths) {
                text += QStringLiteral("\n") + fromFilesystemPath(path);
            }
        }

        auto* item = new QListWidgetItem(text, document_list_);
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
            tooltip = QStringLiteral("DocumentId: ") +
                      fromUtf8(entry.document_id->value());
        }
        if (!entry.diagnostic.empty()) {
            if (!tooltip.isEmpty()) tooltip += QStringLiteral("\n");
            tooltip += fromUtf8(entry.diagnostic);
        }
        item->setToolTip(tooltip);

        if (!openable) {
            item->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
        }
    }

    if (entries.empty()) {
        list_status_->setText(QStringLiteral("No native Part Documents found."));
    } else if (refreshed.ok()) {
        list_status_->setText(
            QStringLiteral("%1 native Part entr%2.")
                .arg(static_cast<qulonglong>(entries.size()))
                .arg(entries.size() == 1U ? QStringLiteral("y")
                                         : QStringLiteral("ies")));
    }

    syncDocumentActionState();
    syncActiveActions();
}

void PartWorkspacePanel::syncDocumentActionState() {
    const auto* item = document_list_->currentItem();
    open_button_->setEnabled(
        session_ != nullptr &&
        item != nullptr &&
        item->data(documentOpenableRole).toBool());
}

std::filesystem::path PartWorkspacePanel::defaultPartPath() const {
    if (session_ == nullptr) return "Part001.ss2part";

    for (unsigned index = 1U; index <= 9999U; ++index) {
        std::ostringstream name;
        name << "Part" << std::setw(3) << std::setfill('0') << index
             << ".ss2part";
        const auto relative = std::filesystem::path{name.str()};
        std::error_code ec;
        if (!std::filesystem::exists(session_->workspaceRoot() / relative, ec) &&
            !ec) {
            return relative;
        }
    }
    return "Part.ss2part";
}

void PartWorkspacePanel::newPart() {
    if (session_ == nullptr) return;

    bool accepted = false;
    const auto entered = QInputDialog::getText(
        this,
        QStringLiteral("New Part"),
        QStringLiteral(
            "Workspace-relative file path (.ss2part).\n"
            "Existing parent folders may be used, for example Parts/Shaft.ss2part:"),
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
        refreshDocuments();
        return;
    }

    active_document_id_ = created.session->documentId();
    refreshDocuments();
    loadActiveDocument();
    active_status_->setText(QStringLiteral("New Part created and saved."));
}

std::optional<core::DocumentId> PartWorkspacePanel::selectedDocumentId() const {
    const auto* item = document_list_->currentItem();
    if (item == nullptr || !item->data(documentOpenableRole).toBool()) {
        return std::nullopt;
    }

    const auto value = item->data(documentIdRole).toString();
    const auto utf8 = toUtf8(value);
    return core::DocumentId::parse(utf8);
}

void PartWorkspacePanel::openSelectedDocument() {
    if (session_ == nullptr) return;
    const auto id = selectedDocumentId();
    if (!id) return;

    auto opened = session_->openDocument(*id);
    if (!opened.ok()) {
        showFailure(opened.diagnostic);
        refreshDocuments();
        return;
    }

    active_document_id_ = opened.session->documentId();
    loadActiveDocument();
    active_status_->setText(
        opened.reused_session
            ? QStringLiteral("Part was already open; using the existing session.")
            : QStringLiteral("Part opened."));
}

application::DocumentSession* PartWorkspacePanel::activeDocumentSession() noexcept {
    if (session_ == nullptr || !active_document_id_) return nullptr;
    return session_->documentSession(*active_document_id_);
}

void PartWorkspacePanel::applyProperties() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    core::DocumentProperties properties;
    properties.number = toUtf8(number_->text());
    properties.title = toUtf8(title_->text());
    properties.description = toUtf8(description_->toPlainText());
    properties.engineering_revision = toUtf8(engineering_revision_->text());

    const auto result = document_session->execute(
        application::SetDocumentPropertiesCommand{std::move(properties)});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        loadActiveDocument();
        return;
    }

    active_status_->setText(
        result.changed
            ? QStringLiteral("Properties changed — save is required.")
            : QStringLiteral("No authored property change."));
    syncActiveActions();
}

void PartWorkspacePanel::undo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;
    const auto result = document_session->undo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }
    loadActiveDocument();
    active_status_->setText(
        result.changed ? QStringLiteral("Undo applied.")
                       : QStringLiteral("Nothing to undo."));
}

void PartWorkspacePanel::redo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;
    const auto result = document_session->redo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }
    loadActiveDocument();
    active_status_->setText(
        result.changed ? QStringLiteral("Redo applied.")
                       : QStringLiteral("Nothing to redo."));
}

void PartWorkspacePanel::save() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result = document_session->save();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    active_status_->setText(QStringLiteral("Part saved."));
    refreshDocuments();
    loadActiveDocument();
}

void PartWorkspacePanel::closeActiveDocument() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr || !active_document_id_ || session_ == nullptr) {
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
            box.addButton(QStringLiteral("Save"), QMessageBox::AcceptRole);
        auto* discard_button =
            box.addButton(QStringLiteral("Discard"), QMessageBox::DestructiveRole);
        auto* cancel_button =
            box.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
        box.exec();

        if (box.clickedButton() == cancel_button || box.clickedButton() == nullptr) {
            return;
        }
        if (box.clickedButton() == save_button) {
            const auto result = document_session->save();
            if (!result.ok()) {
                showFailure(result.diagnostic);
                return;
            }
        } else if (box.clickedButton() == discard_button) {
            discard = true;
        }
    }

    if (!session_->closeDocument(*active_document_id_, discard)) {
        active_status_->setText(
            QStringLiteral("Part remains open because it still has unsaved changes."));
        return;
    }

    active_document_id_.reset();
    clearActiveDocument();
    refreshDocuments();
}

void PartWorkspacePanel::loadActiveDocument() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) {
        clearActiveDocument();
        return;
    }

    const auto& properties = document_session->document().properties();
    number_->setText(fromUtf8(properties.number));
    title_->setText(fromUtf8(properties.title));
    description_->setPlainText(fromUtf8(properties.description));
    engineering_revision_->setText(fromUtf8(properties.engineering_revision));

    std::error_code ec;
    auto relative = std::filesystem::relative(
        document_session->path(),
        session_->workspaceRoot(),
        ec);
    active_path_->setText(
        QStringLiteral("Path: ") +
        fromFilesystemPath(ec ? document_session->path() : relative));
    active_id_->setText(
        QStringLiteral("DocumentId: ") +
        fromUtf8(document_session->documentId().value()));

    number_->setEnabled(true);
    title_->setEnabled(true);
    description_->setEnabled(true);
    engineering_revision_->setEnabled(true);
    syncActiveActions();
}

void PartWorkspacePanel::clearActiveDocument() {
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
    apply_button_->setEnabled(false);
    undo_button_->setEnabled(false);
    redo_button_->setEnabled(false);
    save_button_->setEnabled(false);
    close_button_->setEnabled(false);
    active_status_->clear();
}

void PartWorkspacePanel::syncActiveActions() {
    auto* document_session = activeDocumentSession();
    const bool active = document_session != nullptr;
    apply_button_->setEnabled(active);
    undo_button_->setEnabled(active && document_session->canUndo());
    redo_button_->setEnabled(active && document_session->canRedo());
    save_button_->setEnabled(active && document_session->needsSave());
    close_button_->setEnabled(active);
}

ProjectCloseDisposition PartWorkspacePanel::prepareProjectClose() {
    if (session_ == nullptr || !session_->hasDirtyDocuments()) {
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
        box.addButton(QStringLiteral("Save All"), QMessageBox::AcceptRole);
    auto* discard_button =
        box.addButton(QStringLiteral("Discard"), QMessageBox::DestructiveRole);
    auto* cancel_button =
        box.addButton(QStringLiteral("Cancel"), QMessageBox::RejectRole);
    box.exec();

    if (box.clickedButton() == cancel_button || box.clickedButton() == nullptr) {
        return ProjectCloseDisposition::cancel;
    }
    if (box.clickedButton() == discard_button) {
        return ProjectCloseDisposition::discard;
    }
    if (box.clickedButton() == save_button) {
        const auto saved = session_->saveAllDirtyDocuments();
        if (!saved.ok()) {
            showFailure(saved.diagnostic);
            return ProjectCloseDisposition::cancel;
        }
        return ProjectCloseDisposition::clean;
    }

    return ProjectCloseDisposition::cancel;
}

void PartWorkspacePanel::showFailure(
    const application::ProjectDocumentDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);
    if (!diagnostic.path.empty()) {
        message += QStringLiteral("\n\nPath: ") +
                   fromFilesystemPath(diagnostic.path);
    }
    if (!diagnostic.candidates.empty()) {
        message += QStringLiteral("\n\nConflicting locations:");
        for (const auto& path : diagnostic.candidates) {
            message += QStringLiteral("\n• ") + fromFilesystemPath(path);
        }
    }
    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
}

void PartWorkspacePanel::showFailure(
    const application::DocumentSessionDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);
    if (!diagnostic.path.empty()) {
        message += QStringLiteral("\n\nPath: ") +
                   fromFilesystemPath(diagnostic.path);
    }
    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
}

} // namespace simplesolid2::ui
