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

    using SketchEditHandler = std::function<void(
        const sketch::SketchId&)>;
    using ProfileSelectionHandler =
        std::function<void(
            const std::vector<part::ProfileId>&,
            std::optional<part::ProfileId>)>;
    using ProfileEditHandler =
        std::function<void(part::ProfileId)>;

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

    void setProfileSelection(
        const std::vector<part::ProfileId>& selected,
        std::optional<part::ProfileId> primary);

    void setSketchEditHandler(SketchEditHandler handler) {
        sketch_edit_handler_ = std::move(handler);
    }
    void setProfileSelectionHandler(
        ProfileSelectionHandler handler) {
        profile_selection_handler_ =
            std::move(handler);
    }
    void setProfileEditHandler(
        ProfileEditHandler handler) {
        profile_edit_handler_ =
            std::move(handler);
    }

    [[nodiscard]] std::vector<core::BuiltinReferenceRole>
    selectedBuiltinReferences() const;

    [[nodiscard]] std::optional<core::BuiltinReferenceRole>
    primaryBuiltinReference() const;
    [[nodiscard]] std::vector<part::ProfileId>
    selectedProfileIds() const;
    [[nodiscard]] std::optional<part::ProfileId>
    primaryProfileId() const;

protected:
    bool eventFilter(
        QObject* watched,
        QEvent* event) override;

private:
    void rebuild(bool preserve_reference_selection);
    void updateVisibilityActions();
    void showContextMenu(const QPoint& position);
    void applySelectedVisibility(bool visible);
    void requestSketchEdit(const QTreeWidgetItem& item);
    void requestProfileEdit(const QTreeWidgetItem& item);
    void notifySelectionChanged();

    [[nodiscard]] bool selectionContainsOnlyBuiltinReferences() const;
    [[nodiscard]] static QString labelFor(
        core::BuiltinReferenceRole role);
    [[nodiscard]] static std::optional<core::BuiltinReferenceRole>
    roleForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<sketch::SketchId>
    sketchIdForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<part::ProfileId>
    profileIdForItem(const QTreeWidgetItem& item);

    QTreeWidget* tree_{};
    application::DocumentSession* session_{};
    QAction* show_action_{};
    QAction* hide_action_{};
    QAction* edit_sketch_action_{};
    QAction* edit_profile_action_{};
    ResultHandler result_handler_;
    SelectionHandler selection_handler_;
    SketchEditHandler sketch_edit_handler_;
    ProfileSelectionHandler profile_selection_handler_;
    ProfileEditHandler profile_edit_handler_;
};

} // namespace simplesolid2::ui
