#include "workspace_location_dialog.hpp"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <map>
#include <string>
#include <system_error>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr int relativePathRole = Qt::UserRole + 40;

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
    for (const unsigned char value : bytes) {
        utf8.push_back(
            static_cast<char8_t>(value));
    }
    return std::filesystem::path{utf8};
#endif
}

QString normalizedExtension(
    QString extension) {
    extension = extension.trimmed();
    if (!extension.isEmpty() &&
        !extension.startsWith(
            QLatin1Char('.'))) {
        extension.prepend(
            QLatin1Char('.'));
    }
    return extension;
}

bool sameExtension(
    const QString& left,
    const QString& right) {
    return QString::compare(
               left,
               right,
               Qt::CaseInsensitive) == 0;
}

} // namespace

WorkspaceLocationDialog::WorkspaceLocationDialog(
    std::filesystem::path workspace_root,
    QString document_kind,
    QString required_extension,
    QString suggested_filename,
    QWidget* parent)
    : QDialog{parent},
      workspace_root_{
          std::move(workspace_root)},
      document_kind_{
          std::move(document_kind)},
      required_extension_{
          normalizedExtension(
              std::move(required_extension))} {
    setObjectName(
        QStringLiteral(
            "workspaceLocationDialog"));
    setWindowTitle(
        QStringLiteral("New ") +
        document_kind_);
    resize(660, 500);

    auto* root = new QVBoxLayout(this);

    auto* explanation = new QLabel(
        QStringLiteral(
            "Choose a folder inside the current Workspace. "
            "Project metadata is not a document location."),
        this);
    explanation->setWordWrap(true);
    root->addWidget(explanation);

    auto* workspace_label = new QLabel(
        QStringLiteral("Workspace: ") +
            fromFilesystemPath(
                workspace_root_),
        this);
    workspace_label->setObjectName(
        QStringLiteral(
            "workspaceRootLabel"));
    workspace_label->setWordWrap(true);
    root->addWidget(workspace_label);

    folder_tree_ = new QTreeWidget(this);
    folder_tree_->setObjectName(
        QStringLiteral(
            "workspaceFolderTree"));
    folder_tree_->setHeaderLabel(
        QStringLiteral("Folders"));
    folder_tree_->setSelectionMode(
        QAbstractItemView::SingleSelection);
    root->addWidget(folder_tree_, 1);

    auto* folder_controls =
        new QHBoxLayout;

    folder_name_ = new QLineEdit(this);
    folder_name_->setObjectName(
        QStringLiteral(
            "newFolderNameEdit"));
    folder_name_->setPlaceholderText(
        QStringLiteral(
            "New folder name"));

    create_folder_button_ =
        new QPushButton(
            QStringLiteral("Create Folder"),
            this);
    create_folder_button_->setObjectName(
        QStringLiteral(
            "createWorkspaceFolderButton"));

    folder_controls->addWidget(
        folder_name_,
        1);
    folder_controls->addWidget(
        create_folder_button_);
    root->addLayout(folder_controls);

    auto* form = new QFormLayout;

    file_name_ = new QLineEdit(
        suggested_filename,
        this);
    file_name_->setObjectName(
        QStringLiteral(
            "documentFileNameEdit"));
    form->addRow(
        QStringLiteral("File name"),
        file_name_);

    auto* kind = new QLabel(
        document_kind_,
        this);
    kind->setObjectName(
        QStringLiteral(
            "newDocumentKindLabel"));
    form->addRow(
        QStringLiteral("Document type"),
        kind);

    preview_ = new QLabel(this);
    preview_->setObjectName(
        QStringLiteral(
            "documentTargetPreview"));
    preview_->setWordWrap(true);
    form->addRow(
        QStringLiteral("Location"),
        preview_);

    root->addLayout(form);

    diagnostic_ = new QLabel(this);
    diagnostic_->setObjectName(
        QStringLiteral(
            "workspaceLocationDiagnostic"));
    diagnostic_->setWordWrap(true);
    root->addWidget(diagnostic_);

    buttons_ = new QDialogButtonBox(
        QDialogButtonBox::Ok |
            QDialogButtonBox::Cancel,
        this);
    create_document_button_ =
        buttons_->button(
            QDialogButtonBox::Ok);
    create_document_button_->setText(
        QStringLiteral("Create"));
    create_document_button_->setObjectName(
        QStringLiteral(
            "createDocumentButton"));
    root->addWidget(buttons_);

    QObject::connect(
        folder_tree_,
        &QTreeWidget::itemSelectionChanged,
        this,
        [this] { refreshValidation(); });
    QObject::connect(
        file_name_,
        &QLineEdit::textChanged,
        this,
        [this] { refreshValidation(); });
    QObject::connect(
        folder_name_,
        &QLineEdit::textChanged,
        this,
        [this] {
            create_folder_button_->setEnabled(
                !folder_name_->text()
                     .trimmed()
                     .isEmpty());
        });
    QObject::connect(
        create_folder_button_,
        &QPushButton::clicked,
        this,
        [this] { createFolder(); });
    QObject::connect(
        buttons_,
        &QDialogButtonBox::accepted,
        this,
        [this] { acceptSelection(); });
    QObject::connect(
        buttons_,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject);

    create_folder_button_->setEnabled(
        false);

    rebuildFolderTree();
    refreshValidation();
}

std::optional<std::filesystem::path>
WorkspaceLocationDialog::
selectedRelativeFilePath() const {
    return candidateRelativeFilePath();
}

std::filesystem::path
WorkspaceLocationDialog::
selectedDirectory() const {
    const auto* item =
        folder_tree_->currentItem();
    if (item == nullptr) return {};

    return toFilesystemPath(
        item->data(
                0,
                relativePathRole)
            .toString());
}

std::optional<std::filesystem::path>
WorkspaceLocationDialog::
candidateRelativeFilePath(
    QString* diagnostic) const {
    const auto entered =
        file_name_->text().trimmed();

    if (entered.isEmpty()) {
        if (diagnostic != nullptr) {
            *diagnostic =
                QStringLiteral(
                    "Enter a document file name.");
        }
        return std::nullopt;
    }

    auto file_name =
        toFilesystemPath(entered);
    if (file_name.is_absolute() ||
        file_name.has_root_path() ||
        file_name.has_parent_path() ||
        file_name.filename() != file_name ||
        file_name == "." ||
        file_name == "..") {
        if (diagnostic != nullptr) {
            *diagnostic =
                QStringLiteral(
                    "File name must be one file name, not a path.");
        }
        return std::nullopt;
    }

    const auto current_extension =
        fromFilesystemPath(
            file_name.extension());

    if (current_extension.isEmpty()) {
        file_name +=
            toFilesystemPath(
                required_extension_);
    } else if (!sameExtension(
                   current_extension,
                   required_extension_)) {
        if (diagnostic != nullptr) {
            *diagnostic =
                QStringLiteral(
                    "Expected native extension ") +
                required_extension_ +
                QStringLiteral(".");
        }
        return std::nullopt;
    }

    auto relative =
        selectedDirectory() /
        file_name;
    relative =
        relative.lexically_normal();

    std::error_code ec;
    const auto target =
        workspace_root_ /
        relative;
    if (std::filesystem::exists(
            target,
            ec)) {
        if (diagnostic != nullptr) {
            *diagnostic =
                QStringLiteral(
                    "A file or folder already exists at this location.");
        }
        return std::nullopt;
    }
    if (ec) {
        if (diagnostic != nullptr) {
            *diagnostic =
                QStringLiteral(
                    "Unable to inspect the target location.");
        }
        return std::nullopt;
    }

    return relative;
}

void WorkspaceLocationDialog::
rebuildFolderTree(
    const std::filesystem::path&
        select_relative) {
    const auto listed =
        directory_service_
            .listDirectories(
                workspace_root_);

    folder_tree_->clear();

    if (!listed.ok()) {
        diagnostic_->setText(
            QString::fromUtf8(
                listed.diagnostic
                    .message.c_str()));
        create_document_button_
            ->setEnabled(false);
        create_folder_button_
            ->setEnabled(false);
        return;
    }

    std::map<
        std::string,
        QTreeWidgetItem*> items;

    auto* root_item =
        new QTreeWidgetItem(
            folder_tree_);
    auto root_name =
        fromFilesystemPath(
            workspace_root_.filename());
    if (root_name.isEmpty()) {
        root_name =
            QStringLiteral("Workspace");
    }
    root_item->setText(
        0,
        root_name);
    root_item->setData(
        0,
        relativePathRole,
        QString{});
    items.emplace(
        std::string{},
        root_item);

    QTreeWidgetItem* selected_item =
        select_relative.empty()
            ? root_item
            : nullptr;

    for (const auto& relative :
         listed.directories) {
        if (relative.empty()) continue;

        const auto parent =
            relative.parent_path();
        const auto parent_key =
            parent.empty()
                ? std::string{}
                : parent.generic_string();

        const auto found =
            items.find(parent_key);
        if (found == items.end()) {
            continue;
        }

        auto* item =
            new QTreeWidgetItem(
                found->second);
        item->setText(
            0,
            fromFilesystemPath(
                relative.filename()));
        item->setData(
            0,
            relativePathRole,
            fromFilesystemPath(
                relative));
        items.emplace(
            relative.generic_string(),
            item);

        if (relative ==
            select_relative
                .lexically_normal()) {
            selected_item = item;
        }
    }

    root_item->setExpanded(true);
    folder_tree_->setCurrentItem(
        selected_item != nullptr
            ? selected_item
            : root_item);
}

void WorkspaceLocationDialog::
createFolder() {
    const auto name =
        folder_name_->text()
            .trimmed();
    if (name.isEmpty()) return;

    const auto result =
        directory_service_
            .createDirectory(
                workspace_root_,
                selectedDirectory(),
                name.toUtf8()
                    .toStdString());

    if (!result.ok()) {
        diagnostic_->setText(
            QString::fromUtf8(
                result.diagnostic
                    .message.c_str()));
        return;
    }

    folder_name_->clear();
    rebuildFolderTree(
        result.relative_path);
    refreshValidation();
}

void WorkspaceLocationDialog::
refreshValidation() {
    QString diagnostic;
    const auto candidate =
        candidateRelativeFilePath(
            &diagnostic);

    if (candidate) {
        preview_->setText(
            fromFilesystemPath(
                *candidate));
        diagnostic_->clear();
        create_document_button_
            ->setEnabled(true);
    } else {
        preview_->setText(
            QStringLiteral("—"));
        diagnostic_->setText(
            diagnostic);
        create_document_button_
            ->setEnabled(false);
    }
}

void WorkspaceLocationDialog::
acceptSelection() {
    QString diagnostic;
    if (!candidateRelativeFilePath(
            &diagnostic)) {
        diagnostic_->setText(
            diagnostic);
        create_document_button_
            ->setEnabled(false);
        return;
    }

    accept();
}

} // namespace simplesolid2::ui
