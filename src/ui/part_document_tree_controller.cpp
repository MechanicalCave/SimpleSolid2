#include "part_document_tree_controller.hpp"

#include <QAbstractItemView>
#include <QAction>
#include <QEvent>
#include <QFont>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMouseEvent>
#include <QPoint>
#include <QSignalBlocker>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr int builtinReferenceRoleData = Qt::UserRole + 40;
constexpr int builtinReferenceVisibleData = Qt::UserRole + 41;
constexpr int sketchIdData = Qt::UserRole + 42;
constexpr int profileIdData = Qt::UserRole + 43;

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
    tree_->viewport()->installEventFilter(this);

    show_action_ =
        new QAction(QStringLiteral("Show"), tree_);
    show_action_->setObjectName(
        QStringLiteral("showBuiltinReferencesAction"));

    hide_action_ =
        new QAction(QStringLiteral("Hide"), tree_);
    hide_action_->setObjectName(
        QStringLiteral("hideBuiltinReferencesAction"));

    edit_sketch_action_ =
        new QAction(QStringLiteral("Edit Sketch"), tree_);
    edit_sketch_action_->setObjectName(
        QStringLiteral("editSketchAction"));

    edit_profile_action_ =
        new QAction(QStringLiteral("Edit Profile"), tree_);
    edit_profile_action_->setObjectName(
        QStringLiteral("editProfileAction"));

    QObject::connect(
        edit_profile_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestProfileEdit(*item);
            }
        });

    QObject::connect(
        edit_sketch_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestSketchEdit(*item);
            }
        });

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
        [this] {
            updateVisibilityActions();
            notifySelectionChanged();
        });

    QObject::connect(
        tree_,
        &QTreeWidget::currentItemChanged,
        this,
        [this](QTreeWidgetItem*, QTreeWidgetItem*) {
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

bool PartDocumentTreeController::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (watched == tree_->viewport() &&
        event != nullptr &&
        event->type() ==
            QEvent::MouseButtonDblClick) {
        auto* mouse_event =
            static_cast<QMouseEvent*>(event);

        if (mouse_event->button() ==
            Qt::LeftButton) {
            if (auto* item =
                    tree_->itemAt(
                        mouse_event->position()
                            .toPoint());
                item != nullptr) {
                if (profileIdForItem(*item)) {
                    tree_->setCurrentItem(item);
                    requestProfileEdit(*item);
                    return true;
                }
                if (sketchIdForItem(*item)) {
                    tree_->setCurrentItem(item);
                    requestSketchEdit(*item);
                    return true;
                }
            }
        }
    }

    return QObject::eventFilter(
        watched,
        event);
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

std::optional<core::BuiltinReferenceRole>
PartDocumentTreeController::primaryBuiltinReference() const {
    const auto* current = tree_->currentItem();
    if (current != nullptr && current->isSelected()) {
        if (const auto role = roleForItem(*current)) {
            return role;
        }
    }

    const auto selected = selectedBuiltinReferences();
    if (!selected.empty()) {
        return selected.front();
    }

    return std::nullopt;
}

std::vector<part::ProfileId>
PartDocumentTreeController::selectedProfileIds() const {
    std::vector<part::ProfileId> ids;
    for (const auto* item : tree_->selectedItems()) {
        if (item == nullptr) {
            continue;
        }
        if (const auto id = profileIdForItem(*item)) {
            ids.push_back(*id);
        }
    }
    return ids;
}

std::optional<part::ProfileId>
PartDocumentTreeController::primaryProfileId() const {
    const auto* current = tree_->currentItem();
    if (current != nullptr && current->isSelected()) {
        if (const auto id = profileIdForItem(*current)) {
            return id;
        }
    }

    const auto selected = selectedProfileIds();
    return selected.empty()
        ? std::nullopt
        : std::optional<part::ProfileId>{selected.front()};
}

void PartDocumentTreeController::setBuiltinReferenceSelection(
    const std::vector<core::BuiltinReferenceRole>& selected,
    std::optional<core::BuiltinReferenceRole> primary) {
    const QSignalBlocker blocked{tree_};

    QTreeWidgetItem* first_selected = nullptr;
    QTreeWidgetItem* primary_item = nullptr;

    const auto roots = tree_->findItems(
        QStringLiteral("Origin"),
        Qt::MatchExactly | Qt::MatchRecursive,
        0);

    for (auto* origin : roots) {
        if (origin == nullptr) continue;

        for (int index = 0; index < origin->childCount(); ++index) {
            auto* item = origin->child(index);
            if (item == nullptr) continue;

            const auto role = roleForItem(*item);
            if (!role) continue;

            const bool should_select =
                std::find(
                    selected.begin(),
                    selected.end(),
                    *role) != selected.end();
            item->setSelected(should_select);

            if (should_select && first_selected == nullptr) {
                first_selected = item;
            }
            if (should_select && primary && *primary == *role) {
                primary_item = item;
            }
        }
    }

    if (primary_item != nullptr) {
        tree_->setCurrentItem(
            primary_item,
            0,
            QItemSelectionModel::NoUpdate);
    } else if (first_selected != nullptr) {
        tree_->setCurrentItem(
            first_selected,
            0,
            QItemSelectionModel::NoUpdate);
    }

    updateVisibilityActions();
}

void PartDocumentTreeController::setProfileSelection(
    const std::vector<part::ProfileId>& selected,
    std::optional<part::ProfileId> primary) {
    const QSignalBlocker blocked{tree_};

    QTreeWidgetItem* first_selected = nullptr;
    QTreeWidgetItem* primary_item = nullptr;

    const auto visit =
        [&](auto&& self, QTreeWidgetItem* item) -> void {
            if (item == nullptr) return;

            if (const auto id = profileIdForItem(*item)) {
                const bool should_select =
                    std::find(
                        selected.begin(),
                        selected.end(),
                        *id) != selected.end();
                item->setSelected(should_select);
                if (should_select && first_selected == nullptr) {
                    first_selected = item;
                }
                if (should_select && primary && *primary == *id) {
                    primary_item = item;
                }
            }

            for (int index = 0; index < item->childCount(); ++index) {
                self(self, item->child(index));
            }
        };

    for (int index = 0; index < tree_->topLevelItemCount(); ++index) {
        visit(visit, tree_->topLevelItem(index));
    }

    if (primary_item != nullptr) {
        tree_->setCurrentItem(
            primary_item,
            0,
            QItemSelectionModel::NoUpdate);
    } else if (first_selected != nullptr) {
        tree_->setCurrentItem(
            first_selected,
            0,
            QItemSelectionModel::NoUpdate);
    }

    updateVisibilityActions();
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
    const QSignalBlocker blocked{tree_};

    std::vector<core::BuiltinReferenceRole>
        previously_selected;
    std::vector<part::ProfileId>
        previously_selected_profiles;

    if (preserve_reference_selection) {
        previously_selected =
            selectedBuiltinReferences();
        previously_selected_profiles =
            selectedProfileIds();
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

    if (!session_->document().sketches().empty()) {
        auto* sketches = new QTreeWidgetItem(
            root,
            QStringList{QStringLiteral("Sketches")});

        std::size_t sketch_index = 0U;
        for (const auto& sketch :
             session_->document().sketches()) {
            ++sketch_index;

            auto* item = new QTreeWidgetItem(
                sketches,
                QStringList{
                    QStringLiteral("Sketch %1")
                        .arg(
                            static_cast<qulonglong>(
                                sketch_index))});

            QString support;
            switch (sketch.support.builtin_plane) {
            case core::BuiltinReferenceRole::xy_plane:
                support = QStringLiteral("XY Plane");
                break;
            case core::BuiltinReferenceRole::xz_plane:
                support = QStringLiteral("XZ Plane");
                break;
            case core::BuiltinReferenceRole::yz_plane:
                support = QStringLiteral("YZ Plane");
                break;
            default:
                support = QStringLiteral("<invalid>");
                break;
            }

            item->setData(
                0,
                sketchIdData,
                fromUtf8(sketch.id.value()));

            item->setToolTip(
                0,
                QStringLiteral("SketchId: ") +
                    fromUtf8(sketch.id.value()) +
                    QStringLiteral("\nSupport: ") +
                    support);

            auto font = item->font(0);
            font.setItalic(!sketch.visible);
            item->setFont(0, font);

            for (const auto& profile :
                 session_->document().profiles()) {
                if (profile.source_sketch_id !=
                    sketch.id) {
                    continue;
                }

                const auto evaluation =
                    session_->document()
                        .evaluateProfile(profile.id);
                const bool valid =
                    evaluation &&
                    evaluation->valid();

                QString label =
                    profile.name.empty()
                        ? QStringLiteral("Profile %1")
                              .arg(fromUtf8(
                                  profile.id.serialized()))
                        : fromUtf8(profile.name);
                if (!valid) {
                    label += QStringLiteral(" [Invalid]");
                }

                auto* profile_item =
                    new QTreeWidgetItem(
                        item,
                        QStringList{label});
                profile_item->setData(
                    0,
                    profileIdData,
                    fromUtf8(
                        profile.id.serialized()));

                auto profile_font =
                    profile_item->font(0);
                profile_font.setItalic(!profile.visible);
                profile_item->setFont(
                    0,
                    profile_font);

                profile_item->setToolTip(
                    0,
                    QStringLiteral("ProfileId: ") +
                        fromUtf8(
                            profile.id.serialized()) +
                        QStringLiteral("\nSource SketchId: ") +
                        fromUtf8(
                            profile.source_sketch_id.value()) +
                        QStringLiteral("\nStatus: ") +
                        (valid
                             ? QStringLiteral("Valid")
                             : QStringLiteral("Invalid")));

                if (preserve_reference_selection) {
                    const bool was_selected =
                        std::find(
                            previously_selected_profiles
                                .begin(),
                            previously_selected_profiles
                                .end(),
                            profile.id) !=
                        previously_selected_profiles.end();
                    profile_item->setSelected(
                        was_selected);
                }
            }

            if (item->childCount() > 0) {
                item->setExpanded(true);
            }
        }

        sketches->setExpanded(true);
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
    if (session_ == nullptr ||
        !selectionContainsOnlyBuiltinReferences()) {
        show_action_->setEnabled(false);
        hide_action_->setEnabled(false);
        return;
    }

    bool any_visible = false;
    bool any_hidden = false;

    for (const auto role : selectedBuiltinReferences()) {
        if (session_->document().builtinReferenceVisible(role)) {
            any_visible = true;
        } else {
            any_hidden = true;
        }
    }

    show_action_->setEnabled(any_hidden);
    hide_action_->setEnabled(any_visible);
}

void PartDocumentTreeController::showContextMenu(
    const QPoint& position) {
    updateVisibilityActions();

    if (auto* item = tree_->itemAt(position);
        item != nullptr) {
        if (profileIdForItem(*item)) {
            tree_->setCurrentItem(item);
            QMenu menu{tree_};
            menu.addAction(edit_profile_action_);
            menu.exec(
                tree_->viewport()->mapToGlobal(
                    position));
            return;
        }
        if (sketchIdForItem(*item)) {
            tree_->setCurrentItem(item);
            QMenu menu{tree_};
            menu.addAction(edit_sketch_action_);
            menu.exec(
                tree_->viewport()->mapToGlobal(
                    position));
            return;
        }
    }

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

void PartDocumentTreeController::requestSketchEdit(
    const QTreeWidgetItem& item) {
    const auto sketch_id =
        sketchIdForItem(item);
    if (!sketch_id ||
        !sketch_edit_handler_) {
        return;
    }

    sketch_edit_handler_(*sketch_id);
}

void PartDocumentTreeController::requestProfileEdit(
    const QTreeWidgetItem& item) {
    const auto profile_id =
        profileIdForItem(item);
    if (!profile_id ||
        !profile_edit_handler_) {
        return;
    }

    profile_edit_handler_(*profile_id);
}

void PartDocumentTreeController::notifySelectionChanged() {
    if (selection_handler_) {
        selection_handler_(
            selectedBuiltinReferences(),
            primaryBuiltinReference());
    }
    if (profile_selection_handler_) {
        profile_selection_handler_(
            selectedProfileIds(),
            primaryProfileId());
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


std::optional<sketch::SketchId>
PartDocumentTreeController::sketchIdForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, sketchIdData);
    if (!value.isValid()) {
        return std::nullopt;
    }

    const auto bytes =
        value.toString().toUtf8();
    return sketch::SketchId::parse(
        std::string_view{
            bytes.constData(),
            static_cast<std::size_t>(
                bytes.size())});
}

std::optional<part::ProfileId>
PartDocumentTreeController::profileIdForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, profileIdData);
    if (!value.isValid()) {
        return std::nullopt;
    }

    const auto bytes =
        value.toString().toUtf8();
    return part::ProfileId::parse(
        std::string_view{
            bytes.constData(),
            static_cast<std::size_t>(
                bytes.size())});
}

} // namespace simplesolid2::ui
