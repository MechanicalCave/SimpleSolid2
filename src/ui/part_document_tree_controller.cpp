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
#include <QStyle>
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
constexpr int bodyIdData = Qt::UserRole + 44;
constexpr int featureIdData = Qt::UserRole + 45;

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


QString featureStatusText(
    part::FeatureEvaluationStatus status) {
    switch (status) {
    case part::FeatureEvaluationStatus::up_to_date:
        return QStringLiteral("UpToDate");
    case part::FeatureEvaluationStatus::failed:
        return QStringLiteral("Failed");
    case part::FeatureEvaluationStatus::blocked:
        return QStringLiteral("Blocked");
    case part::FeatureEvaluationStatus::suppressed:
        return QStringLiteral("Suppressed");
    }
    return QStringLiteral("Unknown");
}

QString bodyStatusText(
    part::BodyEvaluationStatus status) {
    switch (status) {
    case part::BodyEvaluationStatus::empty:
        return QStringLiteral("Empty");
    case part::BodyEvaluationStatus::up_to_date:
        return QStringLiteral("UpToDate");
    case part::BodyEvaluationStatus::unavailable:
        return QStringLiteral("Unavailable");
    }
    return QStringLiteral("Unknown");
}

QString featureDiagnosticText(
    part::FeatureEvaluationDiagnosticCode diagnostic) {
    switch (diagnostic) {
    case part::FeatureEvaluationDiagnosticCode::none:
        return QStringLiteral("None");
    case part::FeatureEvaluationDiagnosticCode::missing_profile:
        return QStringLiteral("Missing Profile");
    case part::FeatureEvaluationDiagnosticCode::unresolved_profile:
        return QStringLiteral("Unresolved Profile");
    case part::FeatureEvaluationDiagnosticCode::missing_upstream_body:
        return QStringLiteral("Missing upstream Body");
    case part::FeatureEvaluationDiagnosticCode::upstream_unavailable:
        return QStringLiteral("Upstream unavailable");
    case part::FeatureEvaluationDiagnosticCode::kernel_invalid_input:
        return QStringLiteral("Kernel invalid input");
    case part::FeatureEvaluationDiagnosticCode::kernel_provider_mismatch:
        return QStringLiteral("Kernel provider mismatch");
    case part::FeatureEvaluationDiagnosticCode::kernel_provider_failure:
        return QStringLiteral("Kernel provider failure");
    case part::FeatureEvaluationDiagnosticCode::invalid_brep:
        return QStringLiteral("Invalid B-Rep");
    case part::FeatureEvaluationDiagnosticCode::detached_add:
        return QStringLiteral("Detached Add");
    case part::FeatureEvaluationDiagnosticCode::no_effect:
        return QStringLiteral("No effect");
    case part::FeatureEvaluationDiagnosticCode::empty_result:
        return QStringLiteral("Empty result");
    case part::FeatureEvaluationDiagnosticCode::multi_solid:
        return QStringLiteral("Multi-solid result");
    }
    return QStringLiteral("Unknown");
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
    tree_->viewport()->setMouseTracking(true);
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

    edit_feature_action_ =
        new QAction(
            QStringLiteral("Edit Extrude"),
            tree_);
    edit_feature_action_->setObjectName(
        QStringLiteral("editExtrudeFeatureAction"));

    suppress_feature_action_ =
        new QAction(
            QStringLiteral("Suppress Feature"),
            tree_);
    suppress_feature_action_->setObjectName(
        QStringLiteral("suppressFeatureAction"));
    unsuppress_feature_action_ =
        new QAction(
            QStringLiteral("Unsuppress Feature"),
            tree_);
    unsuppress_feature_action_->setObjectName(
        QStringLiteral("unsuppressFeatureAction"));
    delete_feature_action_ =
        new QAction(
            QStringLiteral("Delete Feature"),
            tree_);
    delete_feature_action_->setObjectName(
        QStringLiteral("deleteFeatureAction"));

    QObject::connect(
        suppress_feature_action_,
        &QAction::triggered,
        this,
        [this] {
            const auto id =
                primaryFeatureId();
            if (id &&
                feature_suppression_handler_) {
                feature_suppression_handler_(
                    *id,
                    true);
            }
        });
    QObject::connect(
        unsuppress_feature_action_,
        &QAction::triggered,
        this,
        [this] {
            const auto id =
                primaryFeatureId();
            if (id &&
                feature_suppression_handler_) {
                feature_suppression_handler_(
                    *id,
                    false);
            }
        });
    QObject::connect(
        delete_feature_action_,
        &QAction::triggered,
        this,
        [this] {
            const auto id =
                primaryFeatureId();
            if (id &&
                feature_delete_handler_) {
                feature_delete_handler_(
                    *id);
            }
        });

    QObject::connect(
        edit_feature_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestFeatureEdit(*item);
            }
        });

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
    const auto publish_feature_hover =
        [this](std::optional<part::FeatureId> feature_id) {
            if (hovered_feature_id_ == feature_id) {
                return;
            }
            hovered_feature_id_ =
                std::move(feature_id);
            if (feature_hover_handler_) {
                feature_hover_handler_(
                    hovered_feature_id_);
            }
        };

    if (watched == tree_->viewport() &&
        event != nullptr &&
        event->type() == QEvent::MouseMove) {
        auto* mouse_event =
            static_cast<QMouseEvent*>(event);
        auto* item =
            tree_->itemAt(
                mouse_event->position().toPoint());
        publish_feature_hover(
            item != nullptr
                ? featureIdForItem(*item)
                : std::nullopt);
    }

    if (watched == tree_->viewport() &&
        event != nullptr &&
        event->type() == QEvent::Leave) {
        publish_feature_hover(std::nullopt);
    }
    if (watched == tree_->viewport() &&
        event != nullptr &&
        event->type() ==
            QEvent::MouseButtonPress) {
        auto* mouse_event =
            static_cast<QMouseEvent*>(event);

        if (mouse_event->button() ==
                Qt::LeftButton &&
            tree_->itemAt(
                mouse_event->position().toPoint()) ==
                nullptr) {
            tree_->clearSelection();
            tree_->setCurrentItem(nullptr);
            return true;
        }
    }

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
                if (featureIdForItem(*item)) {
                    tree_->setCurrentItem(item);
                    requestFeatureEdit(*item);
                    return true;
                }
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
    if (hovered_feature_id_) {
        hovered_feature_id_.reset();
        if (feature_hover_handler_) {
            feature_hover_handler_(std::nullopt);
        }
    }
    session_ = nullptr;
    feature_evaluations_.clear();
    body_status_ =
        part::BodyEvaluationStatus::empty;
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


std::vector<part::FeatureId>
PartDocumentTreeController::selectedFeatureIds() const {
    std::vector<part::FeatureId> ids;
    for (const auto* item : tree_->selectedItems()) {
        if (item == nullptr) continue;
        if (const auto id = featureIdForItem(*item)) {
            ids.push_back(*id);
        }
    }
    return ids;
}

std::optional<part::FeatureId>
PartDocumentTreeController::primaryFeatureId() const {
    const auto* current = tree_->currentItem();
    if (current != nullptr && current->isSelected()) {
        if (const auto id = featureIdForItem(*current)) {
            return id;
        }
    }
    const auto selected = selectedFeatureIds();
    return selected.empty()
        ? std::nullopt
        : std::optional<part::FeatureId>{
              selected.front()};
}

std::optional<part::BodyId>
PartDocumentTreeController::selectedBodyId() const {
    const auto* current = tree_->currentItem();
    if (current != nullptr && current->isSelected()) {
        if (const auto id = bodyIdForItem(*current)) {
            return id;
        }
    }
    for (const auto* item : tree_->selectedItems()) {
        if (item == nullptr) continue;
        if (const auto id = bodyIdForItem(*item)) {
            return id;
        }
    }
    return std::nullopt;
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


void PartDocumentTreeController::setFeatureSelection(
    const std::vector<part::FeatureId>& selected,
    std::optional<part::FeatureId> primary) {
    const QSignalBlocker blocked{tree_};

    QTreeWidgetItem* first_selected = nullptr;
    QTreeWidgetItem* primary_item = nullptr;

    const auto visit =
        [&](auto&& self, QTreeWidgetItem* item) -> void {
            if (item == nullptr) return;
            if (const auto id = featureIdForItem(*item)) {
                const bool should_select =
                    std::find(
                        selected.begin(),
                        selected.end(),
                        *id) != selected.end();
                item->setSelected(should_select);
                if (should_select &&
                    first_selected == nullptr) {
                    first_selected = item;
                }
                if (should_select &&
                    primary &&
                    *primary == *id) {
                    primary_item = item;
                }
            }
            for (int index = 0;
                 index < item->childCount();
                 ++index) {
                self(self, item->child(index));
            }
        };

    for (int index = 0;
         index < tree_->topLevelItemCount();
         ++index) {
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

void PartDocumentTreeController::setEvaluationSnapshot(
    part::BodyEvaluationStatus body_status,
    std::vector<FeatureTreeEvaluationEntry>
        feature_evaluations) {
    body_status_ = body_status;
    feature_evaluations_ =
        std::move(feature_evaluations);
    rebuild(true);
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
    if (hovered_feature_id_) {
        hovered_feature_id_.reset();
        if (feature_hover_handler_) {
            feature_hover_handler_(std::nullopt);
        }
    }

    const QSignalBlocker blocked{tree_};

    std::vector<core::BuiltinReferenceRole>
        previously_selected;
    std::vector<part::ProfileId>
        previously_selected_profiles;
    std::vector<part::FeatureId>
        previously_selected_features;
    std::optional<part::BodyId>
        previously_selected_body;

    if (preserve_reference_selection) {
        previously_selected =
            selectedBuiltinReferences();
        previously_selected_profiles =
            selectedProfileIds();
        previously_selected_features =
            selectedFeatureIds();
        previously_selected_body =
            selectedBodyId();
    }

    tree_->clear();

    if (session_ == nullptr) {
        updateVisibilityActions();
        return;
    }

    auto* root = new QTreeWidgetItem(
        tree_,
        QStringList{displayName(*session_)});


    auto* body = new QTreeWidgetItem(
        root,
        QStringList{
            QStringLiteral("Body [%1]")
                .arg(bodyStatusText(
                    body_status_))});
    body->setData(
        0,
        bodyIdData,
        fromUtf8(
            session_->document()
                .body().id.serialized()));
    body->setToolTip(
        0,
        QStringLiteral("BodyId: ") +
            fromUtf8(
                session_->document()
                    .body().id.serialized()) +
            QStringLiteral("\nStatus: ") +
            bodyStatusText(body_status_) +
            QStringLiteral("\nOrdered Features: ") +
            QString::number(
                static_cast<qulonglong>(
                    session_->document()
                        .body().features.size())));

    {
        auto body_font = body->font(0);
        const bool unavailable =
            body_status_ ==
            part::BodyEvaluationStatus::unavailable;
        body_font.setBold(unavailable);
        body->setFont(0, body_font);
        if (unavailable) {
            body->setIcon(
                0,
                tree_->style()->standardIcon(
                    QStyle::SP_MessageBoxWarning));
        }
    }

    if (preserve_reference_selection &&
        previously_selected_body &&
        *previously_selected_body ==
            session_->document().body().id) {
        body->setSelected(true);
    }

    std::size_t feature_index = 0U;
    for (const auto& feature :
         session_->document().body().features) {
        ++feature_index;
        const auto* extrude =
            std::get_if<part::ExtrudeFeature>(
                &feature.definition);
        if (extrude == nullptr) {
            continue;
        }

        const auto evaluation =
            std::find_if(
                feature_evaluations_.begin(),
                feature_evaluations_.end(),
                [&feature](
                    const FeatureTreeEvaluationEntry&
                        entry) {
                    return entry.feature_id ==
                           feature.id;
                });

        const auto status =
            evaluation !=
                    feature_evaluations_.end()
                ? evaluation->status
                : (feature.suppressed
                       ? part::FeatureEvaluationStatus::
                             suppressed
                       : part::FeatureEvaluationStatus::
                             blocked);
        const auto diagnostic =
            evaluation !=
                    feature_evaluations_.end()
                ? evaluation->diagnostic
                : part::FeatureEvaluationDiagnosticCode::
                      none;

        QString label =
            feature.name.empty()
                ? QStringLiteral("Extrude %1")
                      .arg(
                          static_cast<qulonglong>(
                              feature_index))
                : fromUtf8(feature.name);
        label += extrude->operation ==
                         part::ExtrudeOperation::cut
                     ? QStringLiteral(" — Cut")
                     : QStringLiteral(" — Add");
        label += QStringLiteral(" [%1]")
                     .arg(
                         featureStatusText(status));

        auto* item = new QTreeWidgetItem(
            body,
            QStringList{label});
        item->setData(
            0,
            featureIdData,
            fromUtf8(
                feature.id.serialized()));

        const auto* profile =
            session_->document().findProfile(
                extrude->profile_id);
        item->setToolTip(
            0,
            QStringLiteral("FeatureId: ") +
                fromUtf8(
                    feature.id.serialized()) +
                QStringLiteral("\nSource ProfileId: ") +
                fromUtf8(
                    extrude->profile_id
                        .serialized()) +
                QStringLiteral("\nSource SketchId: ") +
                (profile != nullptr
                     ? fromUtf8(
                           profile->source_sketch_id
                               .value())
                     : QStringLiteral("<missing>")) +
                QStringLiteral("\nStatus: ") +
                featureStatusText(status) +
                QStringLiteral("\nDiagnostic: ") +
                featureDiagnosticText(
                    diagnostic));

        auto font = item->font(0);
        font.setItalic(feature.suppressed);
        const bool warning =
            status ==
                part::FeatureEvaluationStatus::failed ||
            status ==
                part::FeatureEvaluationStatus::blocked;
        font.setBold(warning);
        item->setFont(0, font);
        if (warning) {
            item->setIcon(
                0,
                tree_->style()->standardIcon(
                    QStyle::SP_MessageBoxWarning));
        }

        if (preserve_reference_selection) {
            const bool was_selected =
                std::find(
                    previously_selected_features
                        .begin(),
                    previously_selected_features
                        .end(),
                    feature.id) !=
                previously_selected_features
                    .end();
            item->setSelected(was_selected);
        }
    }
    body->setExpanded(true);

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
                profile_font.setItalic(
                    !session_->document()
                         .profilePresentationVisible(
                             profile.id));
                profile_font.setBold(!valid);
                profile_item->setFont(
                    0,
                    profile_font);
                if (!valid) {
                    profile_item->setIcon(
                        0,
                        tree_->style()->standardIcon(
                            QStyle::SP_MessageBoxWarning));
                }

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

    // Source authoring appears before derived modeling history.
    // Body remains a direct child of Part, but is positioned after Origin
    // and Sketches so the Tree reads from inputs to modeled result.
    root->removeChild(body);
    root->addChild(body);

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
        if (const auto feature_id =
                featureIdForItem(*item)) {
            tree_->setCurrentItem(item);
            const auto* feature =
                session_ != nullptr
                    ? session_->document()
                          .findFeature(
                              *feature_id)
                    : nullptr;
            if (feature == nullptr) {
                return;
            }

            edit_feature_action_->setEnabled(
                !feature->suppressed);
            suppress_feature_action_->setEnabled(
                !feature->suppressed);
            unsuppress_feature_action_->setEnabled(
                feature->suppressed);
            delete_feature_action_->setEnabled(
                true);

            QMenu menu{tree_};
            menu.addAction(edit_feature_action_);
            menu.addSeparator();
            menu.addAction(
                feature->suppressed
                    ? unsuppress_feature_action_
                    : suppress_feature_action_);
            menu.addAction(delete_feature_action_);
            menu.exec(
                tree_->viewport()->mapToGlobal(
                    position));
            return;
        }
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


void PartDocumentTreeController::requestFeatureEdit(
    const QTreeWidgetItem& item) {
    const auto feature_id =
        featureIdForItem(item);
    if (!feature_id ||
        !feature_edit_handler_) {
        return;
    }
    feature_edit_handler_(*feature_id);
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
    if (feature_selection_handler_) {
        feature_selection_handler_(
            selectedFeatureIds(),
            primaryFeatureId());
    }
    if (body_selection_handler_) {
        body_selection_handler_(
            selectedBodyId());
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


std::optional<part::FeatureId>
PartDocumentTreeController::featureIdForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, featureIdData);
    if (!value.isValid()) {
        return std::nullopt;
    }
    const auto bytes =
        value.toString().toUtf8();
    return part::FeatureId::parse(
        std::string_view{
            bytes.constData(),
            static_cast<std::size_t>(
                bytes.size())});
}

std::optional<part::BodyId>
PartDocumentTreeController::bodyIdForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, bodyIdData);
    if (!value.isValid()) {
        return std::nullopt;
    }
    const auto bytes =
        value.toString().toUtf8();
    return part::BodyId::parse(
        std::string_view{
            bytes.constData(),
            static_cast<std::size_t>(
                bytes.size())});
}

} // namespace simplesolid2::ui
