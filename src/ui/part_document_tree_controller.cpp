#include "part_document_tree_controller.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QFont>
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

constexpr int builtinReferenceRoleData = Qt::UserRole + 40;
constexpr int builtinReferenceVisibleData = Qt::UserRole + 41;

constexpr std::array<core::BuiltinReferenceRole, 7> tree_reference_order{
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
        QString::fromStdWString(session.path().stem().wstring());
#else
    const auto utf8 = session.path().stem().generic_u8string();
    auto fallback = QString::fromUtf8(
        reinterpret_cast<const char*>(utf8.data()),
        static_cast<qsizetype>(utf8.size()));
#endif

    if (fallback.isEmpty()) {
        fallback = QStringLiteral("<untitled Part>");
    }
    return fallback;
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
        QStringLiteral("showBuiltinReferencesAction"));

    hide_action_ =
        new QAction(QStringLiteral("Hide"), tree_);
    hide_action_->setObjectName(
        QStringLiteral("hideBuiltinReferencesAction"));

    QObject::connect(
        show_action_,
        &QAction::triggered,
        this,
        [this] { applySelectedVisibility(true); });

    QObject::connect(
        hide_action_,
        &QAction::triggered,
        this,
        [this] { applySelectedVisibility(false); });

    QObject::connect(
        tree_,
        &QTreeWidget::itemSelectionChanged,
        this,
        [this] { updateVisibilityActions(); });

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
    const bool same_session = session_ == session;
    session_ = session;
    rebuild(same_session);
}

void PartDocumentTreeController::clear() {
    session_ = nullptr;
    tree_->clear();
    updateVisibilityActions();
}

std::vector<core::BuiltinReferenceRole>
PartDocumentTreeController::selectedBuiltinReferences() const {
    std::vector<core::BuiltinReferenceRole> roles;

    for (const auto* item : tree_->selectedItems()) {
        if (item == nullptr) continue;

        const auto role = roleForItem(*item);
        if (role) {
            roles.push_back(*role);
        }
    }

    return roles;
}

bool PartDocumentTreeController::
selectionContainsOnlyBuiltinReferences() const {
    const auto selected = tree_->selectedItems();
    if (selected.empty()) return false;

    for (const auto* item : selected) {
        if (item == nullptr || !roleForItem(*item)) {
            return false;
        }
    }
    return true;
}

void PartDocumentTreeController::rebuild(
    bool preserve_reference_selection) {
    std::vector<core::BuiltinReferenceRole>
        previously_selected;

    if (preserve_reference_selection) {
        previously_selected =
            selectedBuiltinReferences();
    }

    tree_->clear();

    if (session_ == nullptr) {
        updateVisibilityActions();
        return;
    }

    auto* root = new QTreeWidgetItem(
        tree_,
        QStringList{displayName(*session_)});

    auto* origin = new QTreeWidgetItem(
        root,
        QStringList{QStringLiteral("Origin")});

    for (const auto role : tree_reference_order) {
        auto* item = new QTreeWidgetItem(
            origin,
            QStringList{labelFor(role)});

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

        if (preserve_reference_selection) {
            const bool was_selected =
                std::find(
                    previously_selected.begin(),
                    previously_selected.end(),
                    role) != previously_selected.end();
            item->setSelected(was_selected);
        }
    }

    root->setExpanded(true);
    origin->setExpanded(true);

    if (!preserve_reference_selection ||
        tree_->selectedItems().empty()) {
        tree_->setCurrentItem(root);
    }

    updateVisibilityActions();
}

void PartDocumentTreeController::updateVisibilityActions() {
    const bool applicable =
        session_ != nullptr &&
        selectionContainsOnlyBuiltinReferences();

    show_action_->setEnabled(applicable);
    hide_action_->setEnabled(applicable);
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
    menu.exec(tree_->viewport()->mapToGlobal(position));
}

void PartDocumentTreeController::applySelectedVisibility(
    bool visible) {
    if (session_ == nullptr ||
        !selectionContainsOnlyBuiltinReferences()) {
        return;
    }

    const auto roles =
        selectedBuiltinReferences();
    if (roles.empty()) return;

    const auto result = session_->execute(
        application::SetBuiltinReferenceVisibilityCommand{
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
    case core::BuiltinReferenceRole::origin_point:
        return QStringLiteral("Origin Point");
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

    return QStringLiteral("<invalid reference>");
}

std::optional<core::BuiltinReferenceRole>
PartDocumentTreeController::roleForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, builtinReferenceRoleData);

    if (!value.isValid()) {
        return std::nullopt;
    }

    const auto role =
        static_cast<core::BuiltinReferenceRole>(
            value.toInt());

    if (!core::isBuiltinReferenceRole(role)) {
        return std::nullopt;
    }

    return role;
}

} // namespace simplesolid2::ui
