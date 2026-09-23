#include "project_creation_dialog.hpp"

#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

namespace simplesolid2::ui {

ProjectCreationDialog::ProjectCreationDialog(QWidget* parent)
    : QDialog{parent} {
    setWindowTitle(QStringLiteral("Create Project"));
    setModal(true);
    resize(620, 220);

    auto* root = new QVBoxLayout(this);
    auto* form = new QFormLayout;

    project_name_ = new QLineEdit(this);
    project_name_->setObjectName(QStringLiteral("projectNameEdit"));
    project_name_->setPlaceholderText(QStringLiteral("e.g. Hydraulic Press"));

    auto* location_row = new QWidget(this);
    auto* location_layout = new QHBoxLayout(location_row);
    location_layout->setContentsMargins(0, 0, 0, 0);
    location_ = new QLineEdit(location_row);
    location_->setObjectName(QStringLiteral("projectLocationEdit"));
    location_->setPlaceholderText(
        QStringLiteral("Parent folder for SimpleSolid Projects"));
    browse_button_ =
        new QPushButton(QStringLiteral("Browse…"), location_row);
    browse_button_->setObjectName(QStringLiteral("browseProjectLocationButton"));
    location_layout->addWidget(location_, 1);
    location_layout->addWidget(browse_button_);

    project_folder_ = new QLineEdit(this);
    project_folder_->setObjectName(QStringLiteral("projectFolderEdit"));
    project_folder_->setPlaceholderText(QStringLiteral("Project folder"));

    final_path_ = new QLabel(this);
    final_path_->setObjectName(QStringLiteral("projectFinalPathLabel"));
    final_path_->setWordWrap(true);
    final_path_->setTextInteractionFlags(Qt::TextSelectableByMouse);

    form->addRow(QStringLiteral("Project name:"), project_name_);
    form->addRow(QStringLiteral("Location:"), location_row);
    form->addRow(QStringLiteral("Project folder:"), project_folder_);
    form->addRow(QStringLiteral("Final path:"), final_path_);

    root->addLayout(form);

    auto* note = new QLabel(
        QStringLiteral(
            "SimpleSolid will create a new Workspace folder at the final path. "
            "An existing target folder will not be adopted or overwritten."),
        this);
    note->setWordWrap(true);
    root->addWidget(note);

    buttons_ = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
        this);
    buttons_->setObjectName(QStringLiteral("projectCreationButtons"));
    if (auto* create = buttons_->button(QDialogButtonBox::Ok)) {
        create->setText(QStringLiteral("Create"));
        create->setObjectName(QStringLiteral("createProjectConfirmButton"));
    }
    root->addWidget(buttons_);

    QObject::connect(
        project_name_,
        &QLineEdit::textChanged,
        this,
        [this](const QString& text) {
            if (folder_follows_name_) {
                project_folder_->setText(text.trimmed());
            }
            updateDerivedState();
        });

    QObject::connect(
        project_folder_,
        &QLineEdit::textEdited,
        this,
        [this](const QString&) {
            folder_follows_name_ = false;
            updateDerivedState();
        });

    QObject::connect(
        project_folder_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { updateDerivedState(); });

    QObject::connect(
        location_,
        &QLineEdit::textChanged,
        this,
        [this](const QString&) { updateDerivedState(); });

    QObject::connect(
        browse_button_,
        &QPushButton::clicked,
        this,
        [this] { browseLocation(); });

    QObject::connect(
        buttons_,
        &QDialogButtonBox::accepted,
        this,
        &QDialog::accept);
    QObject::connect(
        buttons_,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject);

    updateDerivedState();
}

QString ProjectCreationDialog::projectName() const {
    return project_name_->text().trimmed();
}

QString ProjectCreationDialog::location() const {
    return location_->text().trimmed();
}

QString ProjectCreationDialog::projectFolder() const {
    return project_folder_->text().trimmed();
}

void ProjectCreationDialog::browseLocation() {
    const auto selected = QFileDialog::getExistingDirectory(
        this,
        QStringLiteral("Select Projects Location"),
        location_->text());
    if (selected.isEmpty()) return;
    location_->setText(QDir::toNativeSeparators(selected));
}

void ProjectCreationDialog::updateDerivedState() {
    const auto name = projectName();
    const auto location = this->location();
    const auto folder = projectFolder();

    QString final_path;
    if (!location.isEmpty() && !folder.isEmpty()) {
        final_path = QDir::toNativeSeparators(
            QDir{location}.filePath(folder));
    }
    final_path_->setText(final_path);

    if (auto* create = buttons_->button(QDialogButtonBox::Ok)) {
        create->setEnabled(
            !name.isEmpty() &&
            !location.isEmpty() &&
            !folder.isEmpty());
    }
}

} // namespace simplesolid2::ui
