#pragma once

#include "document_selection.hpp"

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
        const DocumentSelectionState&)>;

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

    [[nodiscard]] DocumentSelectionState
    selectionState() const;

    void setSelectionState(
        const DocumentSelectionState& state);

    [[nodiscard]] std::vector<core::BuiltinReferenceRole>
    selectedBuiltinReferences() const;

private:
    void rebuild(bool preserve_selection);
    void updateVisibilityActions();
    void notifySelectionChanged();
    void showContextMenu(const QPoint& position);
    void applySelectedVisibility(bool visible);

    [[nodiscard]] bool
    selectionContainsOnlyBuiltinReferences() const;

    [[nodiscard]] static QString labelFor(
        core::BuiltinReferenceRole role);

    [[nodiscard]] static std::optional<
        DocumentSelectionTarget>
    targetForItem(const QTreeWidgetItem& item);

    [[nodiscard]] QTreeWidgetItem* itemForTarget(
        const DocumentSelectionTarget& target) const;

    QTreeWidget* tree_{};
    application::DocumentSession* session_{};
    QAction* show_action_{};
    QAction* hide_action_{};
    ResultHandler result_handler_;
    SelectionHandler selection_handler_;
    bool applying_selection_{false};
};

} // namespace simplesolid2::ui
