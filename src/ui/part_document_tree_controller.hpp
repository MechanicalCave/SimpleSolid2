#pragma once

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/axis_evaluation.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>

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

struct FeatureTreeEvaluationEntry final {
    part::FeatureId feature_id;
    part::FeatureEvaluationStatus status{
        part::FeatureEvaluationStatus::blocked};
    part::FeatureEvaluationDiagnosticCode diagnostic{
        part::FeatureEvaluationDiagnosticCode::none};
};

struct AxisTreeEvaluationEntry final {
    part::AxisId axis_id;
    part::AxisEvaluationStatus status{
        part::AxisEvaluationStatus::blocked};
    part::AxisEvaluationDiagnostic diagnostic{
        part::AxisEvaluationDiagnostic::
            invalid_reference};
};

struct DatumTreeEvaluationEntry final {
    part::DatumId datum_id;
    part::DatumPlaneEvaluationStatus status{
        part::DatumPlaneEvaluationStatus::blocked};
    part::DatumPlaneEvaluationDiagnostic diagnostic{
        part::DatumPlaneEvaluationDiagnostic::
            invalid_datum};
};

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
    using SketchSupportChangeHandler =
        std::function<void(
            const sketch::SketchId&)>;
    using ProfileSelectionHandler =
        std::function<void(
            const std::vector<part::ProfileId>&,
            std::optional<part::ProfileId>)>;
    using AxisSelectionHandler =
        std::function<void(
            const std::vector<part::AxisId>&,
            std::optional<part::AxisId>)>;
    using AxisEditHandler =
        std::function<void(part::AxisId)>;
    using AxisDeleteHandler =
        std::function<void(part::AxisId)>;
    using DatumSelectionHandler =
        std::function<void(
            const std::vector<part::DatumId>&,
            std::optional<part::DatumId>)>;
    using DatumEditHandler =
        std::function<void(part::DatumId)>;
    using DatumDeleteHandler =
        std::function<void(part::DatumId)>;
    using ProfileEditHandler =
        std::function<void(part::ProfileId)>;
    using FeatureSelectionHandler =
        std::function<void(
            const std::vector<part::FeatureId>&,
            std::optional<part::FeatureId>)>;
    using FeatureHoverHandler =
        std::function<void(
            std::optional<part::FeatureId>)>;
    using FeatureEditHandler =
        std::function<void(part::FeatureId)>;
    using FeatureSuppressionHandler =
        std::function<void(
            part::FeatureId,
            bool suppressed)>;
    using FeatureDeleteHandler =
        std::function<void(part::FeatureId)>;
    using BodySelectionHandler =
        std::function<void(
            std::optional<part::BodyId>)>;

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

    void setAxisSelection(
        const std::vector<part::AxisId>& selected,
        std::optional<part::AxisId> primary);

    void setDatumSelection(
        const std::vector<part::DatumId>& selected,
        std::optional<part::DatumId> primary);

    void setFeatureSelection(
        const std::vector<part::FeatureId>& selected,
        std::optional<part::FeatureId> primary);

    void setEvaluationSnapshot(
        part::BodyEvaluationStatus body_status,
        std::vector<FeatureTreeEvaluationEntry>
            feature_evaluations,
        std::vector<DatumTreeEvaluationEntry>
            datum_evaluations = {},
        std::vector<AxisTreeEvaluationEntry>
            axis_evaluations = {});

    void setSketchEditHandler(SketchEditHandler handler) {
        sketch_edit_handler_ = std::move(handler);
    }
    void setSketchSupportChangeHandler(
        SketchSupportChangeHandler handler) {
        sketch_support_change_handler_ =
            std::move(handler);
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
    void setAxisSelectionHandler(
        AxisSelectionHandler handler) {
        axis_selection_handler_ =
            std::move(handler);
    }
    void setAxisEditHandler(
        AxisEditHandler handler) {
        axis_edit_handler_ =
            std::move(handler);
    }
    void setAxisDeleteHandler(
        AxisDeleteHandler handler) {
        axis_delete_handler_ =
            std::move(handler);
    }
    void setDatumSelectionHandler(
        DatumSelectionHandler handler) {
        datum_selection_handler_ =
            std::move(handler);
    }
    void setDatumEditHandler(
        DatumEditHandler handler) {
        datum_edit_handler_ =
            std::move(handler);
    }
    void setDatumDeleteHandler(
        DatumDeleteHandler handler) {
        datum_delete_handler_ =
            std::move(handler);
    }
    void setFeatureSelectionHandler(
        FeatureSelectionHandler handler) {
        feature_selection_handler_ =
            std::move(handler);
    }
    void setFeatureHoverHandler(
        FeatureHoverHandler handler) {
        feature_hover_handler_ =
            std::move(handler);
    }
    void setFeatureEditHandler(
        FeatureEditHandler handler) {
        feature_edit_handler_ =
            std::move(handler);
    }
    void setFeatureSuppressionHandler(
        FeatureSuppressionHandler handler) {
        feature_suppression_handler_ =
            std::move(handler);
    }
    void setFeatureDeleteHandler(
        FeatureDeleteHandler handler) {
        feature_delete_handler_ =
            std::move(handler);
    }
    void setBodySelectionHandler(
        BodySelectionHandler handler) {
        body_selection_handler_ =
            std::move(handler);
    }

    [[nodiscard]] std::vector<core::BuiltinReferenceRole>
    selectedBuiltinReferences() const;

    [[nodiscard]] std::optional<core::BuiltinReferenceRole>
    primaryBuiltinReference() const;
    [[nodiscard]] std::optional<sketch::SketchId>
    primarySketchId() const;
    [[nodiscard]] std::vector<part::ProfileId>
    selectedProfileIds() const;
    [[nodiscard]] std::optional<part::ProfileId>
    primaryProfileId() const;
    [[nodiscard]] std::vector<part::AxisId>
    selectedAxisIds() const;
    [[nodiscard]] std::optional<part::AxisId>
    primaryAxisId() const;
    [[nodiscard]] std::vector<part::DatumId>
    selectedDatumIds() const;
    [[nodiscard]] std::optional<part::DatumId>
    primaryDatumId() const;
    [[nodiscard]] std::vector<part::FeatureId>
    selectedFeatureIds() const;
    [[nodiscard]] std::optional<part::FeatureId>
    primaryFeatureId() const;
    [[nodiscard]] std::optional<part::BodyId>
    selectedBodyId() const;

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
    void requestSketchSupportChange(
        const QTreeWidgetItem& item);
    void requestProfileEdit(const QTreeWidgetItem& item);
    void requestAxisEdit(const QTreeWidgetItem& item);
    void requestAxisDelete(const QTreeWidgetItem& item);
    void requestDatumEdit(const QTreeWidgetItem& item);
    void requestDatumDelete(const QTreeWidgetItem& item);
    void requestFeatureEdit(const QTreeWidgetItem& item);
    void notifySelectionChanged();

    [[nodiscard]] bool selectionContainsOnlyBuiltinReferences() const;
    [[nodiscard]] bool selectionContainsOnlyAxisReferences() const;
    [[nodiscard]] bool selectionContainsOnlyDatumReferences() const;
    [[nodiscard]] bool referenceGeometryGroupSelected() const;
    [[nodiscard]] std::vector<part::DatumId>
    selectedDatumVisibilityTargets() const;
    [[nodiscard]] static QString labelFor(
        core::BuiltinReferenceRole role);
    [[nodiscard]] static std::optional<core::BuiltinReferenceRole>
    roleForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<sketch::SketchId>
    sketchIdForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<part::ProfileId>
    profileIdForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<part::AxisId>
    axisIdForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<part::DatumId>
    datumIdForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static bool
    isReferenceGeometryGroupItem(
        const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<part::FeatureId>
    featureIdForItem(const QTreeWidgetItem& item);
    [[nodiscard]] static std::optional<part::BodyId>
    bodyIdForItem(const QTreeWidgetItem& item);

    QTreeWidget* tree_{};
    application::DocumentSession* session_{};
    QAction* show_action_{};
    QAction* hide_action_{};
    QAction* edit_sketch_action_{};
    QAction* change_sketch_support_action_{};
    QAction* edit_profile_action_{};
    QAction* edit_axis_action_{};
    QAction* delete_axis_action_{};
    QAction* edit_datum_action_{};
    QAction* delete_datum_action_{};
    QAction* edit_feature_action_{};
    QAction* suppress_feature_action_{};
    QAction* unsuppress_feature_action_{};
    QAction* delete_feature_action_{};
    ResultHandler result_handler_;
    SelectionHandler selection_handler_;
    SketchEditHandler sketch_edit_handler_;
    SketchSupportChangeHandler
        sketch_support_change_handler_;
    ProfileSelectionHandler profile_selection_handler_;
    ProfileEditHandler profile_edit_handler_;
    AxisSelectionHandler axis_selection_handler_;
    AxisEditHandler axis_edit_handler_;
    AxisDeleteHandler axis_delete_handler_;
    DatumSelectionHandler datum_selection_handler_;
    DatumEditHandler datum_edit_handler_;
    DatumDeleteHandler datum_delete_handler_;
    FeatureSelectionHandler feature_selection_handler_;
    FeatureHoverHandler feature_hover_handler_;
    FeatureEditHandler feature_edit_handler_;
    FeatureSuppressionHandler
        feature_suppression_handler_;
    FeatureDeleteHandler
        feature_delete_handler_;
    BodySelectionHandler body_selection_handler_;
    part::BodyEvaluationStatus body_status_{
        part::BodyEvaluationStatus::empty};
    std::vector<FeatureTreeEvaluationEntry>
        feature_evaluations_;
    std::vector<DatumTreeEvaluationEntry>
        datum_evaluations_;
    std::vector<AxisTreeEvaluationEntry>
        axis_evaluations_;
    std::optional<part::FeatureId>
        hovered_feature_id_;
};

} // namespace simplesolid2::ui
