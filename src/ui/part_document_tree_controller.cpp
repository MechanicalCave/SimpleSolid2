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
#include <variant>

namespace simplesolid2::ui {
namespace {

constexpr int builtinReferenceRoleData = Qt::UserRole + 40;
constexpr int builtinReferenceVisibleData = Qt::UserRole + 41;
constexpr int sketchIdData = Qt::UserRole + 42;
constexpr int profileIdData = Qt::UserRole + 43;
constexpr int bodyIdData = Qt::UserRole + 44;
constexpr int featureIdData = Qt::UserRole + 45;
constexpr int datumIdData = Qt::UserRole + 46;
constexpr int referenceGeometryGroupData = Qt::UserRole + 47;
constexpr int axisIdData = Qt::UserRole + 48;

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


QString axisReferenceText(
    const part::AxisReference& reference) {
    if (const auto role =
            part::builtinOriginAxisForAxisReference(
                reference)) {
        switch (*role) {
        case core::BuiltinReferenceRole::x_axis:
            return QStringLiteral("Origin X Axis");
        case core::BuiltinReferenceRole::y_axis:
            return QStringLiteral("Origin Y Axis");
        case core::BuiltinReferenceRole::z_axis:
            return QStringLiteral("Origin Z Axis");
        default:
            break;
        }
    }
    if (const auto axis_id =
            part::authoredAxisIdForAxisReference(
                reference)) {
        return QStringLiteral("AxisId %1")
            .arg(
                fromUtf8(
                    axis_id->serialized()));
    }
    return QStringLiteral("<invalid AxisReference>");
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

QString axisStatusText(
    part::AxisEvaluationStatus status) {
    switch (status) {
    case part::AxisEvaluationStatus::resolved:
        return QStringLiteral("Resolved");
    case part::AxisEvaluationStatus::missing:
        return QStringLiteral("Missing");
    case part::AxisEvaluationStatus::ambiguous:
        return QStringLiteral("Ambiguous");
    case part::AxisEvaluationStatus::unsupported:
        return QStringLiteral("Unsupported");
    case part::AxisEvaluationStatus::blocked:
        return QStringLiteral("Blocked");
    }
    return QStringLiteral("Unknown");
}

QString axisDiagnosticText(
    part::AxisEvaluationDiagnostic diagnostic) {
    switch (diagnostic) {
    case part::AxisEvaluationDiagnostic::none:
        return QStringLiteral("None");
    case part::AxisEvaluationDiagnostic::invalid_reference:
        return QStringLiteral("Invalid reference");
    case part::AxisEvaluationDiagnostic::missing_axis:
        return QStringLiteral("Missing Axis");
    case part::AxisEvaluationDiagnostic::missing_sketch:
        return QStringLiteral("Missing source Sketch");
    case part::AxisEvaluationDiagnostic::missing_line:
        return QStringLiteral("Missing source Line");
    case part::AxisEvaluationDiagnostic::source_not_line:
        return QStringLiteral("Source is not a Line");
    case part::AxisEvaluationDiagnostic::sketch_support_missing:
        return QStringLiteral("Sketch support missing");
    case part::AxisEvaluationDiagnostic::sketch_support_ambiguous:
        return QStringLiteral("Sketch support ambiguous");
    case part::AxisEvaluationDiagnostic::sketch_support_unsupported:
        return QStringLiteral("Sketch support unsupported");
    case part::AxisEvaluationDiagnostic::sketch_support_blocked:
        return QStringLiteral("Sketch support blocked");
    case part::AxisEvaluationDiagnostic::stale_part_evaluation:
        return QStringLiteral("Stale Part evaluation");
    case part::AxisEvaluationDiagnostic::stale_datum_evaluation:
        return QStringLiteral("Stale Datum evaluation");
    case part::AxisEvaluationDiagnostic::support_stage_unavailable:
        return QStringLiteral("Support stage unavailable");
    case part::AxisEvaluationDiagnostic::invalid_frame:
        return QStringLiteral("Invalid frame");
    }
    return QStringLiteral("Unknown");
}

QString datumStatusText(
    part::DatumPlaneEvaluationStatus status) {
    switch (status) {
    case part::DatumPlaneEvaluationStatus::resolved:
        return QStringLiteral("Resolved");
    case part::DatumPlaneEvaluationStatus::missing:
        return QStringLiteral("Missing");
    case part::DatumPlaneEvaluationStatus::ambiguous:
        return QStringLiteral("Ambiguous");
    case part::DatumPlaneEvaluationStatus::unsupported:
        return QStringLiteral("Unsupported");
    case part::DatumPlaneEvaluationStatus::blocked:
        return QStringLiteral("Blocked");
    }
    return QStringLiteral("Unknown");
}

QString datumDiagnosticText(
    part::DatumPlaneEvaluationDiagnostic diagnostic) {
    switch (diagnostic) {
    case part::DatumPlaneEvaluationDiagnostic::none:
        return QStringLiteral("None");
    case part::DatumPlaneEvaluationDiagnostic::invalid_datum:
        return QStringLiteral("Invalid Datum");
    case part::DatumPlaneEvaluationDiagnostic::stale_part_evaluation:
        return QStringLiteral("Stale Part evaluation");
    case part::DatumPlaneEvaluationDiagnostic::body_stage_unavailable:
        return QStringLiteral("Body stage unavailable");
    case part::DatumPlaneEvaluationDiagnostic::missing_surface:
        return QStringLiteral("Missing Surface");
    case part::DatumPlaneEvaluationDiagnostic::ambiguous_surface:
        return QStringLiteral("Ambiguous Surface");
    case part::DatumPlaneEvaluationDiagnostic::unsupported_surface:
        return QStringLiteral("Unsupported Surface");
    case part::DatumPlaneEvaluationDiagnostic::unsupported_non_planar:
        return QStringLiteral("Non-planar Surface");
    case part::DatumPlaneEvaluationDiagnostic::missing_datum:
        return QStringLiteral("Missing Datum");
    case part::DatumPlaneEvaluationDiagnostic::cyclic_dependency:
        return QStringLiteral("Cyclic dependency");
    case part::DatumPlaneEvaluationDiagnostic::upstream_datum_unavailable:
        return QStringLiteral("Upstream Datum unavailable");
    case part::DatumPlaneEvaluationDiagnostic::invalid_frame:
        return QStringLiteral("Invalid frame");
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
    case part::FeatureEvaluationDiagnosticCode::profile_unavailable:
        return QStringLiteral("Profile unavailable");
    case part::FeatureEvaluationDiagnosticCode::unresolved_profile:
        return QStringLiteral("Unresolved Profile");
    case part::FeatureEvaluationDiagnosticCode::missing_axis:
        return QStringLiteral("Missing Axis");
    case part::FeatureEvaluationDiagnosticCode::axis_unavailable:
        return QStringLiteral("Axis unavailable");
    case part::FeatureEvaluationDiagnosticCode::axis_not_in_profile_plane:
        return QStringLiteral("Axis not in Profile plane");
    case part::FeatureEvaluationDiagnosticCode::profile_crosses_axis:
        return QStringLiteral("Profile crosses Axis");
    case part::FeatureEvaluationDiagnosticCode::sketch_support_missing:
        return QStringLiteral("Sketch support missing");
    case part::FeatureEvaluationDiagnosticCode::sketch_support_ambiguous:
        return QStringLiteral("Sketch support ambiguous");
    case part::FeatureEvaluationDiagnosticCode::sketch_support_unsupported:
        return QStringLiteral("Sketch support unsupported");
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
    case part::FeatureEvaluationDiagnosticCode::topology_integrity_failure:
        return QStringLiteral("Topology integrity failure");
    case part::FeatureEvaluationDiagnosticCode::edge_reference_missing:
        return QStringLiteral("Edge reference Missing");
    case part::FeatureEvaluationDiagnosticCode::edge_reference_ambiguous:
        return QStringLiteral("Edge reference Ambiguous");
    case part::FeatureEvaluationDiagnosticCode::edge_reference_unsupported:
        return QStringLiteral("Edge reference Unsupported");
    }
    return QStringLiteral("Unknown");
}

QString datumSourceText(
    const part::PlaneReference& source) {
    if (const auto origin =
            part::builtinOriginPlaneForPlaneReference(
                source)) {
        switch (*origin) {
        case core::BuiltinReferenceRole::xy_plane:
            return QStringLiteral("XY Plane");
        case core::BuiltinReferenceRole::xz_plane:
            return QStringLiteral("XZ Plane");
        case core::BuiltinReferenceRole::yz_plane:
            return QStringLiteral("YZ Plane");
        default:
            return QStringLiteral("<invalid Origin plane>");
        }
    }

    if (const auto* surface =
            part::bodyPlanarSurfaceForPlaneReference(
                source)) {
        return QStringLiteral("Body Surface @ Feature %1")
            .arg(
                fromUtf8(
                    surface->surface
                        .producer_feature_id
                        .serialized()));
    }

    if (const auto datum =
            part::datumPlaneIdForPlaneReference(
                source)) {
        return QStringLiteral("Datum Plane %1")
            .arg(fromUtf8(datum->serialized()));
    }

    return QStringLiteral("<invalid source>");
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

    change_sketch_support_action_ =
        new QAction(
            QStringLiteral("Change Sketch Support"),
            tree_);
    change_sketch_support_action_->setObjectName(
        QStringLiteral("changeSketchSupportAction"));

    edit_profile_action_ =
        new QAction(QStringLiteral("Edit Profile"), tree_);
    edit_profile_action_->setObjectName(
        QStringLiteral("editProfileAction"));

    edit_axis_action_ =
        new QAction(
            QStringLiteral("Edit Axis"),
            tree_);
    edit_axis_action_->setObjectName(
        QStringLiteral("editAxisAction"));

    delete_axis_action_ =
        new QAction(
            QStringLiteral("Delete Axis"),
            tree_);
    delete_axis_action_->setObjectName(
        QStringLiteral("deleteAxisAction"));

    edit_datum_action_ =
        new QAction(
            QStringLiteral("Edit Datum Plane"),
            tree_);
    edit_datum_action_->setObjectName(
        QStringLiteral("editDatumPlaneAction"));

    delete_datum_action_ =
        new QAction(
            QStringLiteral("Delete Datum Plane"),
            tree_);
    delete_datum_action_->setObjectName(
        QStringLiteral("deleteDatumPlaneAction"));

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
        edit_axis_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestAxisEdit(*item);
            }
        });

    QObject::connect(
        delete_axis_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestAxisDelete(*item);
            }
        });

    QObject::connect(
        edit_datum_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestDatumEdit(*item);
            }
        });

    QObject::connect(
        delete_datum_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestDatumDelete(*item);
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
        change_sketch_support_action_,
        &QAction::triggered,
        this,
        [this] {
            if (auto* item = tree_->currentItem();
                item != nullptr) {
                requestSketchSupportChange(*item);
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
                if (axisIdForItem(*item)) {
                    tree_->setCurrentItem(item);
                    requestAxisEdit(*item);
                    return true;
                }
                if (datumIdForItem(*item)) {
                    tree_->setCurrentItem(item);
                    requestDatumEdit(*item);
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
    datum_evaluations_.clear();
    axis_evaluations_.clear();
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

std::optional<sketch::SketchId>
PartDocumentTreeController::primarySketchId() const {
    if (tree_ == nullptr) {
        return std::nullopt;
    }

    const auto selected =
        tree_->selectedItems();
    if (selected.size() != 1U ||
        selected.front() == nullptr) {
        return std::nullopt;
    }
    return sketchIdForItem(
        *selected.front());
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

std::vector<part::AxisId>
PartDocumentTreeController::selectedAxisIds() const {
    std::vector<part::AxisId> ids;
    for (const auto* item : tree_->selectedItems()) {
        if (item == nullptr) continue;
        if (const auto id = axisIdForItem(*item)) {
            ids.push_back(*id);
        }
    }
    return ids;
}

std::optional<part::AxisId>
PartDocumentTreeController::primaryAxisId() const {
    const auto* current = tree_->currentItem();
    if (current != nullptr && current->isSelected()) {
        if (const auto id = axisIdForItem(*current)) {
            return id;
        }
    }
    const auto selected = selectedAxisIds();
    return selected.empty()
        ? std::nullopt
        : std::optional<part::AxisId>{selected.front()};
}

std::vector<part::DatumId>
PartDocumentTreeController::selectedDatumIds() const {
    std::vector<part::DatumId> ids;
    for (const auto* item : tree_->selectedItems()) {
        if (item == nullptr) continue;
        if (const auto id = datumIdForItem(*item)) {
            ids.push_back(*id);
        }
    }
    return ids;
}

std::optional<part::DatumId>
PartDocumentTreeController::primaryDatumId() const {
    const auto* current = tree_->currentItem();
    if (current != nullptr && current->isSelected()) {
        if (const auto id = datumIdForItem(*current)) {
            return id;
        }
    }
    const auto selected = selectedDatumIds();
    return selected.empty()
        ? std::nullopt
        : std::optional<part::DatumId>{
              selected.front()};
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


void PartDocumentTreeController::setAxisSelection(
    const std::vector<part::AxisId>& selected,
    std::optional<part::AxisId> primary) {
    const QSignalBlocker blocked{tree_};

    QTreeWidgetItem* first_selected = nullptr;
    QTreeWidgetItem* primary_item = nullptr;

    const auto visit =
        [&](auto&& self, QTreeWidgetItem* item) -> void {
            if (item == nullptr) return;
            if (const auto id = axisIdForItem(*item)) {
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

void PartDocumentTreeController::setDatumSelection(
    const std::vector<part::DatumId>& selected,
    std::optional<part::DatumId> primary) {
    const QSignalBlocker blocked{tree_};

    QTreeWidgetItem* first_selected = nullptr;
    QTreeWidgetItem* primary_item = nullptr;

    const auto visit =
        [&](auto&& self, QTreeWidgetItem* item) -> void {
            if (item == nullptr) return;
            if (const auto id = datumIdForItem(*item)) {
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
        feature_evaluations,
    std::vector<DatumTreeEvaluationEntry>
        datum_evaluations,
    std::vector<AxisTreeEvaluationEntry>
        axis_evaluations) {
    body_status_ = body_status;
    feature_evaluations_ =
        std::move(feature_evaluations);
    datum_evaluations_ =
        std::move(datum_evaluations);
    axis_evaluations_ =
        std::move(axis_evaluations);
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

bool PartDocumentTreeController::
selectionContainsOnlyAxisReferences() const {
    const auto selected = tree_->selectedItems();
    if (selected.empty()) return false;

    for (const auto* item : selected) {
        if (item == nullptr || !axisIdForItem(*item)) {
            return false;
        }
    }
    return true;
}

bool PartDocumentTreeController::
selectionContainsOnlyDatumReferences() const {
    const auto selected = tree_->selectedItems();
    if (selected.empty()) return false;

    for (const auto* item : selected) {
        if (item == nullptr ||
            (!datumIdForItem(*item) &&
             !isReferenceGeometryGroupItem(*item))) {
            return false;
        }
    }
    return true;
}

bool PartDocumentTreeController::
referenceGeometryGroupSelected() const {
    for (const auto* item : tree_->selectedItems()) {
        if (item != nullptr &&
            isReferenceGeometryGroupItem(*item)) {
            return true;
        }
    }
    return false;
}

std::vector<part::DatumId>
PartDocumentTreeController::
selectedDatumVisibilityTargets() const {
    std::vector<part::DatumId> targets;
    if (session_ == nullptr) {
        return targets;
    }

    if (referenceGeometryGroupSelected()) {
        targets.reserve(
            session_->document()
                .datumPlanes()
                .size());
        for (const auto& datum :
             session_->document().datumPlanes()) {
            targets.push_back(datum.id);
        }
        return targets;
    }

    targets = selectedDatumIds();
    std::sort(targets.begin(), targets.end());
    targets.erase(
        std::unique(
            targets.begin(),
            targets.end()),
        targets.end());
    return targets;
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
    std::vector<part::DatumId>
        previously_selected_datums;
    std::vector<part::AxisId>
        previously_selected_axes;
    bool reference_geometry_group_selected = false;
    std::vector<part::FeatureId>
        previously_selected_features;
    std::optional<part::BodyId>
        previously_selected_body;

    if (preserve_reference_selection) {
        previously_selected =
            selectedBuiltinReferences();
        previously_selected_profiles =
            selectedProfileIds();
        previously_selected_datums =
            selectedDatumIds();
        previously_selected_axes =
            selectedAxisIds();
        reference_geometry_group_selected =
            referenceGeometryGroupSelected();
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
        const auto* revolve =
            std::get_if<part::RevolveFeature>(
                &feature.definition);
        const auto* fillet =
            std::get_if<part::FilletFeature>(
                &feature.definition);
        const auto* chamfer =
            std::get_if<part::ChamferFeature>(
                &feature.definition);
        if (extrude == nullptr &&
            revolve == nullptr &&
            fillet == nullptr &&
            chamfer == nullptr) {
            continue;
        }

        const auto source_profile_id =
            part::sourceProfileId(feature);
        if ((extrude != nullptr ||
             revolve != nullptr) &&
            !source_profile_id) {
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

        const bool profile_feature =
            extrude != nullptr ||
            revolve != nullptr;
        const bool cut =
            extrude != nullptr
                ? extrude->operation ==
                      part::ExtrudeOperation::cut
                : revolve != nullptr
                    ? revolve->operation ==
                          part::RevolveOperation::cut
                    : false;
        const auto feature_kind =
            extrude != nullptr
                ? QStringLiteral("Extrude")
                : revolve != nullptr
                    ? QStringLiteral("Revolve")
                    : fillet != nullptr
                        ? QStringLiteral("Fillet")
                        : QStringLiteral("Chamfer");

        QString label =
            feature.name.empty()
                ? QStringLiteral("%1 %2")
                      .arg(
                          feature_kind,
                          QString::number(
                              static_cast<qulonglong>(
                                  feature_index)))
                : fromUtf8(feature.name);
        if (profile_feature) {
            label += cut
                         ? QStringLiteral(" — Cut")
                         : QStringLiteral(" — Add");
        } else {
            const auto edge_count =
                fillet != nullptr
                    ? fillet->edges.size()
                    : chamfer->edges.size();
            label += QStringLiteral(" — %1 Edge%2")
                         .arg(
                             static_cast<qulonglong>(
                                 edge_count))
                         .arg(
                             edge_count == 1U
                                 ? QString{}
                                 : QStringLiteral("s"));
        }
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
            source_profile_id
                ? session_->document().findProfile(
                      *source_profile_id)
                : nullptr;
        QString tooltip =
            QStringLiteral("FeatureId: ") +
            fromUtf8(
                feature.id.serialized()) +
            QStringLiteral("\nType: ") +
            feature_kind;
        if (source_profile_id) {
            tooltip +=
                QStringLiteral("\nSource ProfileId: ") +
                fromUtf8(
                    source_profile_id->serialized()) +
                QStringLiteral("\nSource SketchId: ") +
                (profile != nullptr
                     ? fromUtf8(
                           profile->source_sketch_id
                               .value())
                     : QStringLiteral("<missing>"));
        }
        if (revolve != nullptr) {
            tooltip +=
                QStringLiteral("\nAxis: ") +
                axisReferenceText(
                    revolve->axis);
        }
        if (fillet != nullptr) {
            tooltip +=
                QStringLiteral("\nExplicit Edges: %1\nRadius: %2 mm")
                    .arg(
                        static_cast<qulonglong>(
                            fillet->edges.size()))
                    .arg(
                        fillet->radius.millimetres,
                        0,
                        'g',
                        12);
        } else if (chamfer != nullptr) {
            tooltip +=
                QStringLiteral("\nExplicit Edges: %1\nDistance: %2 mm")
                    .arg(
                        static_cast<qulonglong>(
                            chamfer->edges.size()))
                    .arg(
                        chamfer->distance.millimetres,
                        0,
                        'g',
                        12);
        }
        tooltip +=
            QStringLiteral("\nStatus: ") +
            featureStatusText(status) +
            QStringLiteral("\nDiagnostic: ") +
            featureDiagnosticText(
                diagnostic);
        item->setToolTip(
            0,
            tooltip);

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

    auto* reference_geometry =
        new QTreeWidgetItem(
            root,
            QStringList{
                QStringLiteral("Reference Geometry")});
    reference_geometry->setData(
        0,
        referenceGeometryGroupData,
        true);
    reference_geometry->setToolTip(
        0,
        QStringLiteral(
            "Authored Datum reference geometry. Show/Hide on this group updates the visibility of all contained Datum objects in one transaction."));
    if (preserve_reference_selection &&
        reference_geometry_group_selected) {
        reference_geometry->setSelected(true);
    }

    std::size_t datum_index = 0U;
    for (const auto& datum :
         session_->document().datumPlanes()) {
        ++datum_index;
        auto* item =
            new QTreeWidgetItem(
                reference_geometry,
                QStringList{
                    QStringLiteral("Datum Plane %1")
                        .arg(
                            static_cast<qulonglong>(
                                datum_index))});
        item->setData(
            0,
            datumIdData,
            fromUtf8(datum.id.serialized()));

        const auto evaluation =
            std::find_if(
                datum_evaluations_.begin(),
                datum_evaluations_.end(),
                [&datum](
                    const DatumTreeEvaluationEntry& entry) {
                    return entry.datum_id == datum.id;
                });
        const bool evaluated =
            evaluation != datum_evaluations_.end();
        const bool resolved =
            evaluated &&
            evaluation->status ==
                part::DatumPlaneEvaluationStatus::
                    resolved;

        auto font = item->font(0);
        font.setItalic(!datum.visible);
        font.setBold(evaluated && !resolved);
        item->setFont(0, font);
        if (evaluated && !resolved) {
            item->setIcon(
                0,
                tree_->style()->standardIcon(
                    QStyle::SP_MessageBoxWarning));
        }

        item->setToolTip(
            0,
            QStringLiteral(
                "DatumId: %1\nSource: %2\nOffset: %3 mm\nVisibility: %4\nStatus: %5\nDiagnostic: %6")
                .arg(
                    fromUtf8(datum.id.serialized()),
                    datumSourceText(datum.source),
                    QString::number(
                        datum.offset.millimetres,
                        'g',
                        12),
                    datum.visible
                        ? QStringLiteral("Shown")
                        : QStringLiteral("Hidden"),
                    evaluated
                        ? datumStatusText(
                              evaluation->status)
                        : QStringLiteral(
                              "Not evaluated"),
                    evaluated
                        ? datumDiagnosticText(
                              evaluation->diagnostic)
                        : QStringLiteral("—")));

        if (preserve_reference_selection) {
            const bool was_selected =
                std::find(
                    previously_selected_datums.begin(),
                    previously_selected_datums.end(),
                    datum.id) !=
                previously_selected_datums.end();
            item->setSelected(was_selected);
        }
    }
    reference_geometry->setExpanded(true);

    if (!session_->document().sketches().empty() ||
        !session_->document().axes().empty()) {
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
            if (const auto origin_plane =
                    part::builtinOriginPlaneForSketchSupport(
                        sketch.support)) {
                switch (*origin_plane) {
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
            } else if (const auto* surface =
                           part::bodyPlanarSurfaceReference(
                               sketch.support)) {
                support =
                    QStringLiteral("Body Surface @ Feature %1")
                        .arg(
                            QString::fromStdString(
                                surface->stage
                                    .feature_id
                                    ->serialized()));
            } else if (const auto datum_id =
                           part::datumPlaneIdForSketchSupport(
                               sketch.support)) {
                support =
                    QStringLiteral("Datum Plane %1")
                        .arg(
                            fromUtf8(
                                datum_id->serialized()));
            } else {
                support = QStringLiteral("<invalid>");
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

            for (const auto& axis :
                 session_->document().axes()) {
                if (axis.source.sketch_id !=
                    sketch.id) {
                    continue;
                }

                const auto evaluation =
                    std::find_if(
                        axis_evaluations_.begin(),
                        axis_evaluations_.end(),
                        [&axis](
                            const AxisTreeEvaluationEntry& entry) {
                            return entry.axis_id == axis.id;
                        });
                const bool evaluated =
                    evaluation != axis_evaluations_.end();
                const bool resolved =
                    evaluated &&
                    evaluation->status ==
                        part::AxisEvaluationStatus::resolved;

                QString label =
                    axis.name.empty()
                        ? QStringLiteral("Axis %1")
                              .arg(fromUtf8(
                                  axis.id.serialized()))
                        : fromUtf8(axis.name);
                if (evaluated && !resolved) {
                    label += QStringLiteral(" [%1]")
                                 .arg(axisStatusText(
                                     evaluation->status));
                }

                auto* axis_item =
                    new QTreeWidgetItem(
                        item,
                        QStringList{label});
                axis_item->setData(
                    0,
                    axisIdData,
                    fromUtf8(axis.id.serialized()));

                auto axis_font =
                    axis_item->font(0);
                axis_font.setItalic(!axis.visible);
                axis_font.setBold(evaluated && !resolved);
                axis_item->setFont(0, axis_font);
                if (evaluated && !resolved) {
                    axis_item->setIcon(
                        0,
                        tree_->style()->standardIcon(
                            QStyle::SP_MessageBoxWarning));
                }

                axis_item->setToolTip(
                    0,
                    QStringLiteral(
                        "AxisId: %1\nSource SketchId: %2\nSource EntityId: %3\nVisibility: %4\nStatus: %5\nDiagnostic: %6")
                        .arg(
                            fromUtf8(axis.id.serialized()),
                            fromUtf8(axis.source.sketch_id.value()),
                            fromUtf8(axis.source.entity_id.serialized()),
                            axis.visible
                                ? QStringLiteral("Shown")
                                : QStringLiteral("Hidden"),
                            evaluated
                                ? axisStatusText(evaluation->status)
                                : QStringLiteral("Not evaluated"),
                            evaluated
                                ? axisDiagnosticText(
                                      evaluation->diagnostic)
                                : QStringLiteral("—")));

                if (preserve_reference_selection) {
                    const bool was_selected =
                        std::find(
                            previously_selected_axes.begin(),
                            previously_selected_axes.end(),
                            axis.id) !=
                        previously_selected_axes.end();
                    axis_item->setSelected(was_selected);
                }
            }

            if (item->childCount() > 0) {
                item->setExpanded(true);
            }
        }

        QTreeWidgetItem* missing_sources = nullptr;
        for (const auto& axis :
             session_->document().axes()) {
            const auto* source_sketch =
                session_->document().findSketch(
                    axis.source.sketch_id);
            if (source_sketch != nullptr) {
                continue;
            }

            if (missing_sources == nullptr) {
                missing_sources =
                    new QTreeWidgetItem(
                        sketches,
                        QStringList{
                            QStringLiteral(
                                "Missing source Sketches")});
                auto missing_font =
                    missing_sources->font(0);
                missing_font.setBold(true);
                missing_sources->setFont(
                    0,
                    missing_font);
                missing_sources->setIcon(
                    0,
                    tree_->style()->standardIcon(
                        QStyle::SP_MessageBoxWarning));
                missing_sources->setToolTip(
                    0,
                    QStringLiteral(
                        "Authored Axis objects whose source Sketch no longer exists. They remain repairable by Edit Axis."));
            }

            const auto evaluation =
                std::find_if(
                    axis_evaluations_.begin(),
                    axis_evaluations_.end(),
                    [&axis](
                        const AxisTreeEvaluationEntry& entry) {
                        return entry.axis_id == axis.id;
                    });
            const bool evaluated =
                evaluation != axis_evaluations_.end();

            QString label =
                axis.name.empty()
                    ? QStringLiteral("Axis %1")
                          .arg(fromUtf8(
                              axis.id.serialized()))
                    : fromUtf8(axis.name);
            label += QStringLiteral(
                " [Missing source Sketch]");

            auto* axis_item =
                new QTreeWidgetItem(
                    missing_sources,
                    QStringList{label});
            axis_item->setData(
                0,
                axisIdData,
                fromUtf8(axis.id.serialized()));

            auto axis_font =
                axis_item->font(0);
            axis_font.setItalic(!axis.visible);
            axis_font.setBold(true);
            axis_item->setFont(0, axis_font);
            axis_item->setIcon(
                0,
                tree_->style()->standardIcon(
                    QStyle::SP_MessageBoxWarning));

            axis_item->setToolTip(
                0,
                QStringLiteral(
                    "AxisId: %1\nMissing source SketchId: %2\nSource EntityId: %3\nVisibility: %4\nStatus: %5\nDiagnostic: %6\nUse Edit Axis to re-source this authored Axis.")
                    .arg(
                        fromUtf8(axis.id.serialized()),
                        fromUtf8(
                            axis.source.sketch_id.value()),
                        fromUtf8(
                            axis.source.entity_id.serialized()),
                        axis.visible
                            ? QStringLiteral("Shown")
                            : QStringLiteral("Hidden"),
                        evaluated
                            ? axisStatusText(
                                  evaluation->status)
                            : QStringLiteral(
                                  "Not evaluated"),
                        evaluated
                            ? axisDiagnosticText(
                                  evaluation->diagnostic)
                            : QStringLiteral("—")));

            if (preserve_reference_selection) {
                const bool was_selected =
                    std::find(
                        previously_selected_axes.begin(),
                        previously_selected_axes.end(),
                        axis.id) !=
                    previously_selected_axes.end();
                axis_item->setSelected(
                    was_selected);
            }
        }

        if (missing_sources != nullptr) {
            missing_sources->setExpanded(true);
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
    if (session_ == nullptr) {
        show_action_->setEnabled(false);
        hide_action_->setEnabled(false);
        return;
    }

    bool any_visible = false;
    bool any_hidden = false;

    if (selectionContainsOnlyBuiltinReferences()) {
        for (const auto role :
             selectedBuiltinReferences()) {
            if (session_->document()
                    .builtinReferenceVisible(role)) {
                any_visible = true;
            } else {
                any_hidden = true;
            }
        }
    } else if (
        selectionContainsOnlyAxisReferences()) {
        for (const auto id : selectedAxisIds()) {
            const auto* axis =
                session_->document().findAxis(id);
            if (axis == nullptr) {
                continue;
            }
            if (axis->visible) {
                any_visible = true;
            } else {
                any_hidden = true;
            }
        }
    } else if (
        selectionContainsOnlyDatumReferences()) {
        const auto targets =
            selectedDatumVisibilityTargets();
        for (const auto id : targets) {
            const auto* datum =
                session_->document()
                    .findDatumPlane(id);
            if (datum == nullptr) {
                continue;
            }
            if (datum->visible) {
                any_visible = true;
            } else {
                any_hidden = true;
            }
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

            edit_feature_action_->setText(
                std::holds_alternative<
                    part::RevolveFeature>(
                    feature->definition)
                    ? QStringLiteral("Edit Revolve")
                    : QStringLiteral("Edit Extrude"));
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
        if (axisIdForItem(*item)) {
            tree_->setCurrentItem(item);
            updateVisibilityActions();
            QMenu menu{tree_};
            menu.addAction(edit_axis_action_);
            if (show_action_->isEnabled() ||
                hide_action_->isEnabled()) {
                menu.addSeparator();
                menu.addAction(show_action_);
                menu.addAction(hide_action_);
            }
            menu.addSeparator();
            menu.addAction(delete_axis_action_);
            menu.exec(
                tree_->viewport()->mapToGlobal(
                    position));
            return;
        }
        if (datumIdForItem(*item)) {
            tree_->setCurrentItem(item);
            updateVisibilityActions();
            QMenu menu{tree_};
            menu.addAction(edit_datum_action_);
            if (show_action_->isEnabled() ||
                hide_action_->isEnabled()) {
                menu.addSeparator();
                menu.addAction(show_action_);
                menu.addAction(hide_action_);
            }
            menu.addSeparator();
            menu.addAction(delete_datum_action_);
            menu.exec(
                tree_->viewport()->mapToGlobal(
                    position));
            return;
        }
        if (isReferenceGeometryGroupItem(*item)) {
            tree_->setCurrentItem(item);
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
            menu.addAction(
                change_sketch_support_action_);
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
    if (session_ == nullptr) {
        return;
    }

    application::DocumentSessionResult result;
    bool attempted = false;

    if (selectionContainsOnlyBuiltinReferences()) {
        const auto roles =
            selectedBuiltinReferences();
        if (roles.empty()) return;
        attempted = true;
        result = session_->execute(
            application::
                SetBuiltinReferenceVisibilityCommand{
                    roles,
                    visible});
    } else if (
        selectionContainsOnlyAxisReferences()) {
        const auto targets =
            selectedAxisIds();
        if (targets.empty()) return;
        attempted = true;
        result = session_->execute(
            application::
                SetAxisVisibilityCommand{
                    targets,
                    session_->document().revision(),
                    visible});
    } else if (
        selectionContainsOnlyDatumReferences()) {
        const auto targets =
            selectedDatumVisibilityTargets();
        if (targets.empty()) return;
        attempted = true;
        result = session_->execute(
            application::
                SetDatumPlaneVisibilityCommand{
                    targets,
                    session_->document().revision(),
                    visible});
    }

    if (!attempted) {
        return;
    }

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

void PartDocumentTreeController::requestSketchSupportChange(
    const QTreeWidgetItem& item) {
    const auto sketch_id =
        sketchIdForItem(item);
    if (!sketch_id ||
        !sketch_support_change_handler_) {
        return;
    }
    sketch_support_change_handler_(
        *sketch_id);
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

void PartDocumentTreeController::requestAxisEdit(
    const QTreeWidgetItem& item) {
    const auto axis_id =
        axisIdForItem(item);
    if (!axis_id ||
        !axis_edit_handler_) {
        return;
    }
    axis_edit_handler_(*axis_id);
}

void PartDocumentTreeController::requestAxisDelete(
    const QTreeWidgetItem& item) {
    const auto axis_id =
        axisIdForItem(item);
    if (!axis_id ||
        !axis_delete_handler_) {
        return;
    }
    axis_delete_handler_(*axis_id);
}

void PartDocumentTreeController::requestDatumEdit(
    const QTreeWidgetItem& item) {
    const auto datum_id =
        datumIdForItem(item);
    if (!datum_id ||
        !datum_edit_handler_) {
        return;
    }
    datum_edit_handler_(*datum_id);
}

void PartDocumentTreeController::requestDatumDelete(
    const QTreeWidgetItem& item) {
    const auto datum_id =
        datumIdForItem(item);
    if (!datum_id ||
        !datum_delete_handler_) {
        return;
    }
    datum_delete_handler_(*datum_id);
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
    if (axis_selection_handler_) {
        axis_selection_handler_(
            selectedAxisIds(),
            primaryAxisId());
    }
    if (datum_selection_handler_) {
        datum_selection_handler_(
            selectedDatumIds(),
            primaryDatumId());
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


std::optional<part::AxisId>
PartDocumentTreeController::axisIdForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, axisIdData);
    if (!value.isValid()) {
        return std::nullopt;
    }
    const auto bytes =
        value.toString().toUtf8();
    return part::AxisId::parse(
        std::string_view{
            bytes.constData(),
            static_cast<std::size_t>(
                bytes.size())});
}

std::optional<part::DatumId>
PartDocumentTreeController::datumIdForItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(0, datumIdData);
    if (!value.isValid()) {
        return std::nullopt;
    }
    const auto bytes =
        value.toString().toUtf8();
    return part::DatumId::parse(
        std::string_view{
            bytes.constData(),
            static_cast<std::size_t>(
                bytes.size())});
}

bool PartDocumentTreeController::
isReferenceGeometryGroupItem(
    const QTreeWidgetItem& item) {
    const auto value =
        item.data(
            0,
            referenceGeometryGroupData);
    return value.isValid() &&
           value.toBool();
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
