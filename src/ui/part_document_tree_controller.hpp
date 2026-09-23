#pragma once

#include <simplesolid2/application/document_session.hpp>

#include <QObject>

#include <functional>
#include <optional>
#include <utility>
#include <vector>

class QAction;
class QPoint;
class QString;
class QTreeWidget;
class QTreeWidgetItem;

namespace simplesolid2::ui {

class PartDocumentTreeController final : public QObject {
public:
    using ResultHandler = std::function<void(
        const application::DocumentSessionResult&,
        bool visible)>;

    using SelectionHandler = std::function<void(
        const std::vector<core::BuiltinReferenceRole>&,
        std::optional<core::BuiltinReferenceRole>)>;

    PartDocumentTreeController(
        QTreeWidget& tree,
        QObject* parent = nullptr);

    void setDocumentSession(
        application::DocumentSession* session);
    void clear();

    void setResultHandler(ResultHandler handler) {
        result_handler_ = std::move(handler);
    }

    void setSelectionHandler(SelectionHandler handler) {
        selection_handler_ = std::move(handler);
    }

    void setBuiltinReferenceSelection(
        const std::vector<core::BuiltinReferenceRole>& selected,
        std::optional<core::BuiltinReferenceRole> primary);

    [[nodiscard]] std::vector<core::BuiltinReferenceRole>
    selectedBuiltinReferences() const;

    [[nodiscard]] std::optional<core::BuiltinReferenceRole>
    primaryBuiltinReference() const;

private:
    void rebuild(bool preserve_reference_selection);
    void updateVisibilityActions();
    void showContextMenu(const QPoint& position);
    void applySelectedVisibility(bool visible);
    void notifySelectionChanged();

    [[nodiscard]] bool selectionContainsOnlyBuiltinReferences() const;
    [[nodiscard]] static QString labelFor(
        core::BuiltinReferenceRole role);
    [[nodiscard]] static std::optional<core::BuiltinReferenceRole>
    roleForItem(const QTreeWidgetItem& item);

    QTreeWidget* tree_{};
    application::DocumentSession* session_{};
    QAction* show_action_{};
    QAction* hide_action_{};
    ResultHandler result_handler_;
    SelectionHandler selection_handler_;
};

} // namespace simplesolid2::ui
