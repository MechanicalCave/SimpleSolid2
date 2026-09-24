#include "open_document_dialog.hpp"

#include <QAbstractItemView>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr int documentIdRole =
    Qt::UserRole + 50;
constexpr int openableRole =
    Qt::UserRole + 51;

QString fromUtf8(
    std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(
            value.size()));
}

} // namespace

OpenDocumentDialog::OpenDocumentDialog(
    std::vector<OpenDocumentCandidate> candidates,
    QWidget* parent)
    : QDialog{parent},
      candidates_{std::move(candidates)} {
    setObjectName(
        QStringLiteral(
            "openDocumentDialog"));
    setWindowTitle(
        QStringLiteral("Open Document"));
    resize(760, 440);

    auto* root =
        new QVBoxLayout(this);

    auto* explanation = new QLabel(
        QStringLiteral(
            "Select a document. Invalid files and identity conflicts "
            "remain visible but cannot be opened."),
        this);
    explanation->setWordWrap(true);
    root->addWidget(explanation);

    documents_ =
        new QTreeWidget(this);
    documents_->setObjectName(
        QStringLiteral(
            "openDocumentList"));
    documents_->setColumnCount(4);
    documents_->setHeaderLabels({
        QStringLiteral("Kind"),
        QStringLiteral("Name"),
        QStringLiteral("Location"),
        QStringLiteral("Status")});
    documents_->setSelectionMode(
        QAbstractItemView::SingleSelection);
    documents_->setRootIsDecorated(false);
    documents_->setAlternatingRowColors(true);
    auto* header = documents_->header();
    header->setStretchLastSection(false);
    header->setSectionsMovable(false);
    header->setSectionResizeMode(
        QHeaderView::Interactive);
    header->resizeSection(0, 70);
    header->resizeSection(1, 150);
    header->resizeSection(2, 360);
    header->resizeSection(3, 140);
    root->addWidget(
        documents_,
        1);

    for (const auto& candidate :
         candidates_) {
        auto* item =
            new QTreeWidgetItem(
                documents_);
        item->setText(
            0,
            candidate.kind);
        item->setText(
            1,
            candidate.name);
        item->setText(
            2,
            candidate.location);
        item->setText(
            3,
            candidate.status);
        item->setToolTip(
            0,
            candidate.tooltip);
        item->setToolTip(
            1,
            candidate.tooltip);
        item->setToolTip(
            2,
            candidate.tooltip);
        item->setToolTip(
            3,
            candidate.tooltip);
        item->setData(
            0,
            openableRole,
            candidate.openable);

        if (candidate.document_id) {
            item->setData(
                0,
                documentIdRole,
                fromUtf8(
                    candidate.document_id
                        ->value()));
        }

        if (!candidate.openable) {
            item->setDisabled(true);
        }
    }

    buttons_ =
        new QDialogButtonBox(
            QDialogButtonBox::Open |
                QDialogButtonBox::Cancel,
            this);
    open_button_ =
        buttons_->button(
            QDialogButtonBox::Open);
    open_button_->setObjectName(
        QStringLiteral(
            "openDocumentConfirmButton"));
    open_button_->setEnabled(false);
    root->addWidget(buttons_);

    QObject::connect(
        documents_,
        &QTreeWidget::itemSelectionChanged,
        this,
        [this] {
            syncOpenEnabled();
        });
    QObject::connect(
        documents_,
        &QTreeWidget::itemDoubleClicked,
        this,
        [this](
            QTreeWidgetItem* item,
            int) {
            if (item != nullptr &&
                item->data(
                        0,
                        openableRole)
                    .toBool()) {
                acceptCurrent();
            }
        });
    QObject::connect(
        buttons_,
        &QDialogButtonBox::accepted,
        this,
        [this] {
            acceptCurrent();
        });
    QObject::connect(
        buttons_,
        &QDialogButtonBox::rejected,
        this,
        &QDialog::reject);
}

std::optional<core::DocumentId>
OpenDocumentDialog::
selectedDocumentId() const {
    const auto* item =
        documents_->currentItem();
    if (item == nullptr ||
        !item->data(
                 0,
                 openableRole)
             .toBool()) {
        return std::nullopt;
    }

    return core::DocumentId::parse(
        item->data(
                0,
                documentIdRole)
            .toString()
            .toUtf8()
            .toStdString());
}

void OpenDocumentDialog::
syncOpenEnabled() {
    const auto* item =
        documents_->currentItem();
    open_button_->setEnabled(
        item != nullptr &&
        item->data(
                0,
                openableRole)
            .toBool());
}

void OpenDocumentDialog::
acceptCurrent() {
    if (!selectedDocumentId()) {
        syncOpenEnabled();
        return;
    }

    accept();
}

} // namespace simplesolid2::ui
