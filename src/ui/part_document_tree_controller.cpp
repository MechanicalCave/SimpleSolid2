#include "part_document_tree_controller.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QFont>
#include <QItemSelectionModel>
#include <QMenu>
#include <QPoint>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr int selectionTargetKindData =
    Qt::UserRole + 39;
constexpr int builtinReferenceRoleData =
    Qt::UserRole + 40;
constexpr int builtinReferenceVisibleData =
    Qt::UserRole + 41;

constexpr std::array<core::BuiltinReferenceRole, 7>
tree_reference_order{
    core::BuiltinReferenceRole::xy_plane,
    core::BuiltinReferenceRole::xz_plane,
    core::BuiltinReferenceRole::yz_plane,
    core::BuiltinReferenceRole::x_axis,
    core::BuiltinReferenceRole::y_axis,
    core::BuiltinReferenceRole::z_axis,
    core::BuiltinReferenceRole::origin_point,
};

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(value.size()));
}

QString displayName(
    const application::DocumentSession& session) {
    const auto& title =
        session.document().properties().title;
    if (!title.empty()) {
        return fromUtf8(title);
    }

#if defined(_WIN32)
    auto fallback =
        QString::fromStdWString(
            session.path().stem().wstring());
#else
    const auto utf8 =
        session.path().stem().generic_u8string();
    auto fallback = QString::fromUtf8(
        reinterpret_cast<const char*>(utf8.data()),
        static_cast<qsizetype>(utf8.size()));
#endif

    if (fallback.isEmpty()) {
        fallback =
            QStringLiteral("<untitled Part>");
    }
    return fallback;
}

void setTargetKind(
    QTreeWidgetItem& item,
    DocumentSelectionTargetKind kind) {
    item.setData(
        0,
        selectionTargetKindData,
        static_cast<int>(kind));
}

} // namespace

PartDocumentTreeController::PartDocumentTreeController(
    QTreeWidget& tree,
    QObject* parent)
    : QObject{parent},
      tree_{&tree} {
    tree_->setSelectionMode(
        QAbstractItemView::ExtendedSelection);
    tree_->setContextMenuPolicy(
        Qt::CustomContextMenu);

    show_action_ =
        new QAction(QStringLiteral("Show"), tree_);
    show_action_->setObjectName(
        QStringLiteral(
            "showBuiltinReferencesAction"));

    hide_action_ =
        new QAction(QStringLiteral("Hide"), tree_);
    hide_action_->setObjectName(
        QStringLiteral(
            "hideBuiltinReferencesAction"));

    QObject::connect(
        show_action_,
        &QAction::triggered,
        this,
        [this] {
            applySelectedVisibility(true);
        });

    QObject::connect(
        hide_action_,
        &QAction::triggered,
        this,
        [this] {
            applySelectedVisibility(false);
        });

    QObject::connect(
        tree_,
        &QTreeWidget::itemSelectionChanged,
        this,
        [this] {
            updateVisibilityActions();
            notifySelectionChanged();
        });

    QObject::connect(
        tree_,
        &QTreeWidget::currentItemChanged,
        this,
        [this](
            QTreeWidgetItem*,
            QTreeWidgetItem*) {
            notifySelectionChanged();
        });

    QObject::connect(
        tree_,
        &QTreeWidget::customContextMenuRequested,
        this,
        [this](const QPoint& position) {
            showContextMenu(position);
        });

    updateVisibilityActions();
}

void PartDocumentTreeController::setDocumentSession(
    application::DocumentSession* session) {
    const bool same_session =
        session_ == session;
    session_ = session;
    rebuild(same_session);
}

void PartDocumentTreeController::clear() {
    session_ = nullptr;
    applying_selection_ = true;
    tree_->clear();
    applying_selection_ = false;
    updateVisibilityActions();
}

DocumentSelectionState
PartDocumentTreeController::selectionState() const {
    DocumentSelectionState state;

    for (const auto* item :
         tree_->selectedItems()) {
        if (item == nullptr) continue;
        const auto target =
            targetForItem(*item);
        if (target) {
            state.selected.push_back(*target);
        }
    }

    if (const auto* current =
            tree_->currentItem()) {
        const auto target =
            targetForItem(*current);
        if (target &&
            state.contains(*target)) {
            state.primary = *target;
        }
    }

    if (!state.primary &&
        !state.selected.empty()) {
        state.primary =
            state.selected.front();
    }

    return state;
}

void PartDocumentTreeController::setSelectionState(
    const DocumentSelectionState& state) {
    if (!state.valid()) return;

    applying_selection_ = true;
    tree_->clearSelection();

    for (const auto& target :
         state.selected) {
        if (auto* item =
                itemForTarget(target)) {
            item->setSelected(true);
        }
    }

    if (state.primary) {
        if (auto* item =
                itemForTarget(*state.primary)) {
            tree_->setCurrentItem(
                item,
                0,
                QItemSelectionModel::NoUpdate);
        }
    } else {
        tree_->setCurrentItem(
            nullptr,
            0,
            QItemSelectionModel::NoUpdate);
    }

    applying_selection_ = false;
    updateVisibilityActions();
}

std::vector<core::BuiltinReferenceRole>
PartDocumentTreeController::
selectedBuiltinReferences() const {
    std::vector<core::BuiltinReferenceRole>
        roles;

    for (const auto* item :
         tree_->selectedItems()) {
        if (item == nullptr) continue;

        const auto target =
            targetForItem(*item);
        if (target &&
            target->kind ==
                DocumentSelectionTargetKind::
                    builtin_reference) {
            roles.push_back(
                target->builtin_reference);
        }
    }

    return roles;
}

bool PartDocumentTreeController::
selectionContainsOnlyBuiltinReferences() const {
    const auto selected =
        tree_->selectedItems();
    if (selected.empty()) return false;

    for (const auto* item : selected) {
        if (item == nullptr) return false;

        const auto target =
            targetForItem(*item);
        if (!target ||
            target->kind !=
                DocumentSelectionTargetKind::
                    builtin_reference) {
            return false;
        }
    }

    return true;
}

void PartDocumentTreeController::rebuild(
    bool preserve_selection) {
    const auto previous =
        preserve_selection
            ? selectionState()
            : DocumentSelectionState{};

    applying_selection_ = true;
    tree_->clear();

    if (session_ == nullptr) {
        applying_selection_ = false;
        updateVisibilityActions();
        return;
    }

    auto* root = new QTreeWidgetItem(
        tree_,
        QStringList{
            displayName(*session_)});
    setTargetKind(
        *root,
        DocumentSelectionTargetKind::
            document_root);

    auto* origin = new QTreeWidgetItem(
        root,
        QStringList{
            QStringLiteral("Origin")});

    for (const auto role :
         tree_reference_order) {
        auto* item = new QTreeWidgetItem(
            origin,
            QStringList{labelFor(role)});

        setTargetKind(
            *item,
            DocumentSelectionTargetKind::
                builtin_reference);

        item->setData(
            0,
            builtinReferenceRoleData,
            static_cast<int>(role));

        const bool visible =
            session_->document()
                .builtinReferenceVisible(role);

        item->setData(
            0,
            builtinReferenceVisibleData,
            visible);

        auto font = item->font(0);
        font.setItalic(!visible);
        item->setFont(0, font);

        item->setToolTip(
            0,
            visible
                ? QStringLiteral("Shown")
                : QStringLiteral("Hidden"));
    }

    root->setExpanded(true);
    origin->setExpanded(true);
    applying_selection_ = false;

    if (preserve_selection &&
        previous.valid() &&
        !previous.selected.empty()) {
        setSelectionState(previous);
    } else {
        setSelectionState(
            DocumentSelectionState::
                documentRootOnly());
    }

    updateVisibilityActions();
}

void PartDocumentTreeController::
updateVisibilityActions() {
    if (session_ == nullptr ||
        !selectionContainsOnlyBuiltinReferences()) {
        show_action_->setEnabled(false);
        hide_action_->setEnabled(false);
        return;
    }

    bool any_visible = false;
    bool any_hidden = false;

    for (const auto role :
         selectedBuiltinReferences()) {
        if (session_->document()
                .builtinReferenceVisible(role)) {
            any_visible = true;
        } else {
            any_hidden = true;
        }
    }

    show_action_->setEnabled(any_hidden);
    hide_action_->setEnabled(any_visible);
}

void PartDocumentTreeController::
notifySelectionChanged() {
    if (applying_selection_ ||
        !selection_handler_) {
        return;
    }

    const auto state = selectionState();
    if (!state.valid()) return;
    selection_handler_(state);
}

void PartDocumentTreeController::showContextMenu(
    const QPoint& position) {
    updateVisibilityActions();

    if (!show_action_->isEnabled() &&
        !hide_action_->isEnabled()) {
        return;
    }

    QMenu menu{tree_};
    menu.addAction(show_action_);
    menu.addAction(hide_action_);
    menu.exec(
        tree_->viewport()->mapToGlobal(
            position));
}

void PartDocumentTreeController::
applySelectedVisibility(bool visible) {
    if (session_ == nullptr ||
        !selectionContainsOnlyBuiltinReferences()) {
        return;
    }

    const auto roles =
        selectedBuiltinReferences();
    if (roles.empty()) return;

    const auto result =
        session_->execute(
            application::
                SetBuiltinReferenceVisibilityCommand{
                    roles,
                    visible});

    if (result.ok() && result.changed) {
        rebuild(true);
    }

    if (result_handler_) {
        result_handler_(result, visible);
    }
}

QString PartDocumentTreeController::labelFor(
    core::BuiltinReferenceRole role) {
    switch (role) {
    case core::BuiltinReferenceRole::
        origin_point:
        return QStringLiteral(
            "Origin Point");
    case core::BuiltinReferenceRole::x_axis:
        return QStringLiteral("X Axis");
    case core::BuiltinReferenceRole::y_axis:
        return QStringLiteral("Y Axis");
    case core::BuiltinReferenceRole::z_axis:
        return QStringLiteral("Z Axis");
    case core::BuiltinReferenceRole::xy_plane:
        return QStringLiteral("XY Plane");
    case core::BuiltinReferenceRole::xz_plane:
        return QStringLiteral("XZ Plane");
    case core::BuiltinReferenceRole::yz_plane:
        return QStringLiteral("YZ Plane");
    }

    return QStringLiteral(
        "<invalid reference>");
}

std::optional<DocumentSelectionTarget>
PartDocumentTreeController::targetForItem(
    const QTreeWidgetItem& item) {
    const auto kind_value =
        item.data(
            0,
            selectionTargetKindData);
    if (!kind_value.isValid()) {
        return std::nullopt;
    }

    const auto kind =
        static_cast<
            DocumentSelectionTargetKind>(
                kind_value.toInt());

    if (kind ==
        DocumentSelectionTargetKind::
            document_root) {
        return DocumentSelectionTarget::
            documentRoot();
    }

    if (kind !=
        DocumentSelectionTargetKind::
            builtin_reference) {
        return std::nullopt;
    }

    const auto role_value =
        item.data(
            0,
            builtinReferenceRoleData);
    if (!role_value.isValid()) {
        return std::nullopt;
    }

    const auto role =
        static_cast<
            core::BuiltinReferenceRole>(
                role_value.toInt());

    if (!core::isBuiltinReferenceRole(
            role)) {
        return std::nullopt;
    }

    return DocumentSelectionTarget::
        builtinReference(role);
}

QTreeWidgetItem*
PartDocumentTreeController::itemForTarget(
    const DocumentSelectionTarget& target) const {
    if (!target.valid() ||
        tree_->topLevelItemCount() == 0) {
        return nullptr;
    }

    auto* root =
        tree_->topLevelItem(0);
    if (root == nullptr) return nullptr;

    if (target.kind ==
        DocumentSelectionTargetKind::
            document_root) {
        return root;
    }

    if (root->childCount() == 0) {
        return nullptr;
    }

    auto* origin = root->child(0);
    if (origin == nullptr) return nullptr;

    for (int index = 0;
         index < origin->childCount();
         ++index) {
        auto* child =
            origin->child(index);
        if (child == nullptr) continue;

        const auto child_target =
            targetForItem(*child);
        if (child_target &&
            *child_target == target) {
            return child;
        }
    }

    return nullptr;
}

} // namespace simplesolid2::ui
