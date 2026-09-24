#pragma once

#include <simplesolid2/core/document.hpp>

#include <QDialog>
#include <QString>

#include <optional>
#include <vector>

class QDialogButtonBox;
class QPushButton;
class QTreeWidget;

namespace simplesolid2::ui {

struct OpenDocumentCandidate final {
    QString kind;
    QString name;
    QString location;
    QString status;
    QString tooltip;
    std::optional<core::DocumentId> document_id;
    bool openable{};
};

class OpenDocumentDialog final : public QDialog {
public:
    explicit OpenDocumentDialog(
        std::vector<OpenDocumentCandidate> candidates,
        QWidget* parent = nullptr);

    [[nodiscard]] std::optional<core::DocumentId>
    selectedDocumentId() const;

private:
    void syncOpenEnabled();
    void acceptCurrent();

    std::vector<OpenDocumentCandidate> candidates_;
    QTreeWidget* documents_{};
    QDialogButtonBox* buttons_{};
    QPushButton* open_button_{};
};

} // namespace simplesolid2::ui
