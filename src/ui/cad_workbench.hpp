#pragma once

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/edge_feature_draft.hpp>
#include <simplesolid2/application/revolve_draft.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>
#include <simplesolid2/viewer/camera_state.hpp>

#include "viewport_surface.hpp"

#include <QWidget>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class QCheckBox;
class QComboBox;
class QEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QString;
class QTreeWidget;
class QTimer;
class QWidget;

namespace simplesolid2::kernel {
class ISolidModelingKernel;
}

namespace simplesolid2::ui {

struct BodyTopologyInspection;

class CadWorkbenchShell;
class PartDocumentTreeController;
class PartSketchInteractionController;
class PartViewportController;
class ViewCubeWidget;

struct CadDynamicInputUiState final {
    std::string buffer;
    std::size_t focused_index{};
};

class CadWorkbench final : public QWidget,
                           public application::ICadInputEndpoint {
public:
    explicit CadWorkbench(QWidget* parent = nullptr);
    ~CadWorkbench() override;
    CadWorkbench(
        ViewportFactory viewport_factory,
        QWidget* parent = nullptr);
    CadWorkbench(
        ViewportFactory viewport_factory,
        kernel::ISolidModelingKernel*
            solid_modeling_kernel,
        QWidget* parent = nullptr);

    [[nodiscard]] bool activateDocument(
        application::DocumentSession* session,
        std::filesystem::path workspace_root);
    void deactivateDocument();
    void resetRuntimeState();
    void forgetDocumentRuntimeState(
        const core::DocumentId& document_id);

    [[nodiscard]] std::optional<core::DocumentId>
    activeDocumentId() const {
        if (document_session_ == nullptr) {
            return std::nullopt;
        }
        return document_session_->documentId();
    }

    using CloseDocumentHandler =
        std::function<void(
            const core::DocumentId&)>;
    void setCloseDocumentHandler(
        CloseDocumentHandler handler) {
        close_document_handler_ =
            std::move(handler);
    }

    using DocumentStateChangedHandler =
        std::function<void(
            const core::DocumentId&)>;
    void setDocumentStateChangedHandler(
        DocumentStateChangedHandler handler) {
        document_state_changed_handler_ =
            std::move(handler);
    }

    using CadInputContextChangedHandler =
        std::function<void()>;
    void setCadInputContextChangedHandler(
        CadInputContextChangedHandler handler) {
        cad_input_context_changed_handler_ =
            std::move(handler);
    }

    using CadInteractionSettingsProvider =
        std::function<
            application::CadInteractionSettings()>;
    using CadInteractionSettingsUpdater =
        std::function<
            bool(application::CadInteractionSettings)>;
    void setCadInteractionSettingsProvider(
        CadInteractionSettingsProvider provider);
    void setCadInteractionSettingsUpdater(
        CadInteractionSettingsUpdater updater) {
        cad_interaction_settings_updater_ =
            std::move(updater);
    }
    void refreshCadInteractionSettingsUi();

    using CadDynamicInputUiStateProvider =
        std::function<CadDynamicInputUiState()>;
    void setCadDynamicInputUiStateProvider(
        CadDynamicInputUiStateProvider provider) {
        cad_dynamic_input_ui_state_provider_ =
            std::move(provider);
        refreshCadDynamicInputOverlay();
    }
    void refreshCadDynamicInputOverlay();

    void requestUndo() { undo(); }
    void requestRedo() { redo(); }

    [[nodiscard]] std::string cadInputPrompt() const override;
    [[nodiscard]] application::CadInputContextGeneration
    cadInputContextGeneration() const noexcept override;
    [[nodiscard]] application::CadInputSubmitResult
    submitCadInput(
        std::string_view text,
        application::CadInputContextGeneration
            expected_context_generation) override;
    [[nodiscard]] bool
    acceptsEmptyCadInput() const noexcept override {
        return axis_draft_.has_value() ||
               datum_plane_draft_.has_value() ||
               extrude_draft_.has_value() ||
               revolve_draft_.has_value() ||
               fillet_draft_.has_value() ||
               chamfer_draft_.has_value() ||
               (sketch_support_pick_active_ &&
                pending_sketch_support_.has_value());
    }
    [[nodiscard]] std::vector<
        application::CadDynamicInputField>
    cadDynamicInputFields() const override;
    [[nodiscard]] application::CadInputSubmitResult
    lockCadDynamicInputField(
        std::size_t index,
        std::string_view text,
        application::CadInputContextGeneration
            expected_context_generation) override;
    [[nodiscard]] application::CadInputSubmitResult
    submitCadDynamicInputRequest(
        application::CadInputContextGeneration
            expected_context_generation) override;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUi();

    void applyProperties();
    void setPartLengthUnit(
        core::LengthUnit unit);
    void applyProfileProperties();
    void deleteSelectedProfile();

    [[nodiscard]] bool startAxisTool();
    [[nodiscard]] bool startAxisEdit(
        part::AxisId axis_id);
    void deleteAxis(part::AxisId axis_id);
    void cancelAxis();
    [[nodiscard]] bool finishAxis();
    void clearAxisRuntimeContext();
    void tryStageAxisFromSketchSelection();
    void stageAxisSource(
        part::SketchLineAxisSource source);
    void refreshAxisEvaluation();
    void syncAxisUi();
    [[nodiscard]] application::CadInputSubmitResult
    submitAxisCadInput(std::string_view text);

    [[nodiscard]] bool startDatumPlaneTool();
    [[nodiscard]] bool startDatumPlaneEdit(
        part::DatumId datum_id);
    void deleteDatumPlane(
        part::DatumId datum_id);
    void cancelDatumPlane();
    [[nodiscard]] bool finishDatumPlane();
    void clearDatumPlaneRuntimeContext();
    void stageDatumPlaneSource(
        part::PlaneReference source);
    void tryStageDatumPlaneFromSupport(
        std::optional<core::BuiltinReferenceRole> support);
    void tryStageDatumPlaneFromBodyTopology(
        const BodyTopologyInspection& inspection);
    void tryStageDatumPlaneFromDatum(
        std::optional<part::DatumId> datum_id);
    void refreshDatumPlaneEvaluation();
    void scheduleDatumPlaneEvaluation();
    void flushDatumPlaneEvaluation();
    void syncDatumPlaneUi();
    [[nodiscard]] application::CadInputSubmitResult
    submitDatumPlaneCadInput(
        std::string_view text);

    [[nodiscard]] bool startExtrudeTool();
    [[nodiscard]] bool startExtrudeFromSelectedProfile();
    void cancelExtrudeProfilePick();
    void tryCompleteExtrudeProfilePick();
    [[nodiscard]] bool startExtrudeEdit(
        part::FeatureId feature_id);
    void setFeatureSuppressed(
        part::FeatureId feature_id,
        bool suppressed);
    void deleteFeature(
        part::FeatureId feature_id);
    void cancelExtrude();
    [[nodiscard]] bool finishExtrude();
    void clearExtrudeRuntimeContext();
    void refreshExtrudePreview();
    void scheduleExtrudePreview();
    void flushExtrudePreview();
    void syncExtrudeUi();
    [[nodiscard]] bool setExtrudeDistance(
        core::LengthValue distance,
        std::optional<std::string_view> display_text =
            std::nullopt,
        bool refresh_now = true);
    [[nodiscard]] application::CadInputSubmitResult
    submitExtrudeCadInput(std::string_view text);

    [[nodiscard]] bool startRevolveTool();
    [[nodiscard]] bool startRevolveEdit(
        part::FeatureId feature_id);
    void tryStageRevolveProfile(
        std::optional<part::ProfileId> profile_id);
    void tryStageRevolveAxisFromBuiltin(
        std::optional<core::BuiltinReferenceRole> role);
    void tryStageRevolveAxisFromAuthored(
        std::optional<part::AxisId> axis_id);
    void cancelRevolve();
    [[nodiscard]] bool finishRevolve();
    void clearRevolveRuntimeContext();
    void refreshRevolvePreview();
    void scheduleRevolvePreview();
    void flushRevolvePreview();
    void syncRevolveUi();
    [[nodiscard]] bool setRevolveAngle(
        core::AngleValue angle,
        std::optional<std::string_view> display_text =
            std::nullopt,
        bool refresh_now = true);
    [[nodiscard]] application::CadInputSubmitResult
    submitRevolveCadInput(std::string_view text);


    [[nodiscard]] bool startFilletTool();
    [[nodiscard]] bool startChamferTool();
    void cancelEdgeFeature();
    [[nodiscard]] bool finishEdgeFeature();
    void clearEdgeFeatureRuntimeContext();
    void tryStageEdgeFeatureSelection();
    void refreshEdgeFeaturePreview();
    void scheduleEdgeFeaturePreview();
    void flushEdgeFeaturePreview();
    void syncEdgeFeatureUi();
    [[nodiscard]] bool setEdgeFeatureParameter(
        core::LengthValue parameter,
        std::optional<std::string_view> display_text =
            std::nullopt,
        bool refresh_now = true);
    [[nodiscard]] application::CadInputSubmitResult
    submitEdgeFeatureCadInput(std::string_view text);

    void setSketchSelectionRole(
        sketch::EntityRole role);
    void setSketchLineAxisDesignation(
        bool enabled);
    void startSketchTool();
    void startSketchResupport(
        const sketch::SketchId& sketch_id);
    void cancelSketchTool();
    void requestEditSketch(
        const sketch::SketchId& sketch_id);
    void requestEditProfile(
        part::ProfileId profile_id);
    void tryCreateSketchFromSupport(
        std::optional<core::BuiltinReferenceRole> support);
    void tryCreateSketchFromDatum(
        std::optional<part::DatumId> datum_id);
    void tryCreateSketchFromBodyTopology(
        const BodyTopologyInspection& inspection);
    void stageSketchSupport(
        part::PartSketchSupport support);
    [[nodiscard]] bool finishSketchSupport();
    void enterSketchEdit(
        const sketch::SketchId& sketch_id);
    void finishSketch();
    void activateSketchSelect();
    void activateSketchLine();
    void activateSketchCircle();
    void activateSketchArc();
    void activateSketchRectangle();
    void activateSketchTrim();
    void activateSketchExtend();
    void activateSketchExtendBoth();
    void activateSketchMove();
    void activateSketchCopy();
    void activateSketchRotate();
    void activateSketchScale();
    void activateSketchMirror();
    void activateSketchMeasure();
    void activateSketchMeasureBetween();
    void finishSketchLine();
    void cancelSketchLine();
    void deleteSketchSelection();
    void syncSketchInteractionUi();
    void clearSketchRuntimeContext();
    void reconcileSketchRuntimeContext();
    void undo();
    void redo();
    void save();
    void closeActiveDocument();

    void refreshActiveContext();
    void clearActiveContext();
    void captureActiveViewState();
    void restoreActiveViewState();
    void refreshPropertiesContext(
        std::optional<core::BuiltinReferenceRole> primary);
    void refreshProfileProperties(
        part::ProfileId profile_id);
    void refreshAxisProperties(
        part::AxisId axis_id);
    void refreshDatumProperties(
        part::DatumId datum_id);
    void refreshBodyProperties(
        part::BodyId body_id);
    void refreshFeatureProperties(
        part::FeatureId feature_id);
    void refreshTopologyProperties(
        const BodyTopologyInspection& inspection);
    void refreshPartFeatureEvaluationSnapshot();
    void navigateToProfile(
        part::ProfileId profile_id);
    void navigateToFeature(
        part::FeatureId feature_id);
    void syncActionState();
    void notifyDocumentStateChanged();
    void notifyCadInputContextChanged();
    [[nodiscard]] QString cadInputPromptText() const;

    [[nodiscard]] application::DocumentSession*
    activeDocumentSession() noexcept {
        return document_session_;
    }
    [[nodiscard]] const application::DocumentSession*
    activeDocumentSession() const noexcept {
        return document_session_;
    }

    void showFailure(
        const application::DocumentSessionDiagnostic& diagnostic);
    void setStatusText(const QString& message);

    struct FeatureEvaluationUiState final {
        part::FeatureId feature_id;
        part::FeatureEvaluationStatus status{
            part::FeatureEvaluationStatus::blocked};
        part::FeatureEvaluationDiagnosticCode diagnostic{
            part::FeatureEvaluationDiagnosticCode::none};
    };

    struct AxisEvaluationUiState final {
        part::AxisId axis_id;
        part::AxisEvaluationStatus status{
            part::AxisEvaluationStatus::blocked};
        part::AxisEvaluationDiagnostic diagnostic{
            part::AxisEvaluationDiagnostic::
                invalid_reference};
        std::optional<part::ResolvedAxisLine> line;
    };

    struct DatumEvaluationUiState final {
        part::DatumId datum_id;
        part::DatumPlaneEvaluationStatus status{
            part::DatumPlaneEvaluationStatus::blocked};
        part::DatumPlaneEvaluationDiagnostic diagnostic{
            part::DatumPlaneEvaluationDiagnostic::
                invalid_datum};
    };

    application::DocumentSession* document_session_{};
    std::filesystem::path workspace_root_;
    ViewportFactory viewport_factory_;
    kernel::ISolidModelingKernel*
        solid_modeling_kernel_{};
    viewer::IDocumentViewport* viewport_{};
    std::unordered_map<
        std::string,
        viewer::CameraState>
        document_view_states_;

    bool sketch_support_pick_active_{};
    std::optional<sketch::SketchId>
        sketch_resupport_target_;
    std::optional<part::PartSketchSupport>
        pending_sketch_support_;
    std::optional<core::DocumentRevision>
        pending_sketch_support_revision_;
    application::CadInputContextGeneration
        sketch_support_pick_generation_{};
    std::optional<core::DocumentId>
        sketch_edit_document_id_;
    std::optional<sketch::SketchId>
        active_sketch_id_;

    CloseDocumentHandler
        close_document_handler_;
    DocumentStateChangedHandler
        document_state_changed_handler_;
    CadInputContextChangedHandler
        cad_input_context_changed_handler_;
    CadInteractionSettingsProvider
        cad_interaction_settings_provider_;
    CadInteractionSettingsUpdater
        cad_interaction_settings_updater_;
    CadDynamicInputUiStateProvider
        cad_dynamic_input_ui_state_provider_;
    std::optional<viewer::ViewportPoint2>
        dynamic_input_anchor_;

    CadWorkbenchShell* shell_{};
    PartDocumentTreeController* tree_controller_{};
    PartViewportController* viewport_controller_{};
    std::unique_ptr<PartSketchInteractionController>
        sketch_interaction_controller_;

    QPushButton* undo_button_{};
    QPushButton* redo_button_{};
    QPushButton* save_button_{};
    QPushButton* close_document_button_{};

    QTreeWidget* document_tree_{};
    QWidget* editor_surface_{};
    QWidget* viewport_widget_{};

    QStackedWidget* properties_stack_{};
    QWidget* document_properties_page_{};
    QWidget* reference_properties_page_{};
    QWidget* axis_properties_page_{};
    QWidget* datum_properties_page_{};
    QWidget* profile_properties_page_{};
    QWidget* body_properties_page_{};
    QWidget* feature_properties_page_{};
    QWidget* topology_properties_page_{};
    QLabel* active_path_{};
    QLabel* active_id_{};
    QLabel* reference_name_{};
    QLabel* reference_kind_{};
    QLabel* reference_identity_{};
    QLabel* reference_visibility_{};
    QLabel* axis_name_{};
    QLabel* axis_identity_{};
    QLabel* axis_source_sketch_{};
    QLabel* axis_source_line_{};
    QLabel* axis_visibility_{};
    QLabel* axis_status_{};
    QLabel* axis_diagnostic_{};
    QLabel* axis_origin_{};
    QLabel* axis_direction_{};
    QPushButton* axis_edit_button_{};
    QPushButton* axis_delete_button_{};
    QLabel* datum_name_{};
    QLabel* datum_identity_{};
    QLabel* datum_constructor_{};
    QLabel* datum_source_{};
    QLabel* datum_offset_{};
    QLabel* datum_visibility_{};
    QLabel* datum_status_{};
    QLabel* datum_diagnostic_{};
    QPushButton* datum_edit_button_{};
    QPushButton* datum_delete_button_{};
    QLineEdit* profile_name_{};
    QLabel* profile_identity_{};
    QLabel* profile_source_{};
    QLabel* profile_status_{};
    QLabel* profile_diagnostic_{};
    QLabel* profile_area_{};
    QLabel* profile_perimeter_{};
    QLabel* profile_holes_{};
    QComboBox* profile_consuming_features_{};
    QPushButton* profile_go_to_feature_button_{};
    QCheckBox* profile_visible_{};
    QPushButton* apply_profile_button_{};
    QPushButton* delete_profile_button_{};
    std::optional<part::ProfileId>
        selected_profile_id_;
    std::optional<part::AxisId>
        selected_axis_id_;
    std::optional<part::DatumId>
        selected_datum_id_;
    std::optional<part::FeatureId>
        selected_feature_id_;
    std::optional<part::BodyId>
        selected_body_id_;
    std::optional<core::DocumentRevision>
        part_evaluation_revision_;
    std::optional<part::BodyEvaluationStatus>
        body_evaluation_status_;
    std::vector<FeatureEvaluationUiState>
        feature_evaluation_statuses_;
    std::vector<AxisEvaluationUiState>
        axis_evaluation_statuses_;
    std::vector<DatumEvaluationUiState>
        datum_evaluation_statuses_;
    QLineEdit* number_{};
    QLineEdit* title_{};
    QPlainTextEdit* description_{};
    QLineEdit* engineering_revision_{};
    QComboBox* length_unit_combo_{};
    QPushButton* apply_button_{};

    QLabel* body_identity_{};
    QLabel* body_status_{};
    QLabel* body_feature_count_{};
    QLabel* body_topology_counts_{};
    QLabel* body_topology_accounting_{};

    QLabel* feature_name_{};
    QLabel* feature_identity_{};
    QLabel* feature_status_{};
    QLabel* feature_diagnostic_{};
    QLabel* feature_operation_{};
    QLabel* feature_extent_{};
    QLabel* feature_distance_{};
    QLabel* feature_direction_{};
    QLabel* feature_source_profile_{};
    QLabel* feature_source_sketch_{};
    QLabel* feature_contribution_{};
    QLabel* feature_contribution_diagnostics_{};
    QPushButton* feature_go_to_profile_button_{};
    QPushButton* feature_edit_button_{};
    QPushButton* feature_suppress_button_{};
    QPushButton* feature_delete_button_{};

    QLabel* topology_kind_{};
    QLabel* topology_stage_{};
    QLabel* topology_presence_{};
    QLabel* topology_accounting_{};
    QLabel* topology_strict_reference_{};
    QLabel* topology_carrier_reference_{};
    QLabel* topology_carrier_{};
    QLabel* topology_carrier_type_{};
    QLabel* topology_producer_{};
    QLabel* topology_candidates_{};
    QLabel* topology_adjacency_{};
    QLabel* topology_sketch_support_{};
    QLabel* topology_geometry_{};

    QLabel* operations_placeholder_{};
    QWidget* precision_operations_widget_{};
    QLabel* precision_status_label_{};
    QPushButton* object_snap_toggle_button_{};
    QCheckBox* object_snap_endpoint_check_{};
    QCheckBox* object_snap_midpoint_check_{};
    QCheckBox* object_snap_center_check_{};
    QCheckBox* object_snap_quadrant_check_{};
    QCheckBox* object_snap_intersection_check_{};
    QCheckBox* object_snap_origin_check_{};
    QCheckBox* object_snap_perpendicular_check_{};
    QCheckBox* object_snap_tangent_check_{};
    QCheckBox* object_snap_nearest_check_{};
    QCheckBox* object_snap_extension_check_{};
    QComboBox* object_snap_override_combo_{};
    QPushButton* object_tracking_toggle_button_{};
    QPushButton* polar_toggle_button_{};
    QLineEdit* polar_step_edit_{};
    QComboBox* polar_reference_combo_{};
    QLineEdit* polar_additional_edit_{};
    QPushButton* polar_additional_add_button_{};
    QPushButton* polar_additional_clear_button_{};
    QLabel* polar_additional_label_{};
    QPushButton* dynamic_input_toggle_button_{};
    QLabel* circle_size_mode_label_{};
    QComboBox* circle_size_mode_combo_{};
    bool syncing_precision_ui_{};
    QPushButton* sketch_button_{};
    QPushButton* axis_button_{};
    QPushButton* datum_plane_button_{};
    QPushButton* extrude_button_{};
    QPushButton* revolve_button_{};
    QLabel* part_create_tools_label_{};
    QLabel* part_modify_tools_label_{};
    QPushButton* fillet_button_{};
    QPushButton* chamfer_button_{};
    QPushButton* select_sketch_button_{};
    QLabel* create_tools_label_{};
    QPushButton* line_sketch_button_{};
    QPushButton* circle_sketch_button_{};
    QPushButton* arc_sketch_button_{};
    QPushButton* rectangle_sketch_button_{};
    QPushButton* create_construction_button_{};
    QPushButton* rectangle_diagonals_button_{};
    QLabel* profile_tools_label_{};
    QPushButton* profile_sketch_button_{};
    QLabel* modify_tools_label_{};
    QPushButton* trim_sketch_button_{};
    QPushButton* extend_sketch_button_{};
    QPushButton* extend_both_sketch_button_{};
    QPushButton* move_sketch_button_{};
    QPushButton* copy_sketch_button_{};
    QPushButton* rotate_sketch_button_{};
    QPushButton* scale_sketch_button_{};
    QPushButton* mirror_sketch_button_{};
    QLabel* inspect_tools_label_{};
    QPushButton* measure_sketch_button_{};
    QPushButton* measure_between_button_{};
    QPushButton* cancel_sketch_button_{};
    QPushButton* finish_sketch_button_{};
    QPushButton* finish_line_button_{};
    QPushButton* cancel_line_button_{};
    QPushButton* delete_selection_button_{};
    QLabel* entity_role_label_{};
    QPushButton* regular_role_button_{};
    QPushButton* construction_role_button_{};
    QLabel* line_part_reference_label_{};
    QCheckBox* line_axis_designation_check_{};

    QWidget* axis_operations_widget_{};
    QLabel* axis_source_label_{};
    QLabel* axis_result_label_{};
    QPushButton* axis_finish_button_{};
    QPushButton* axis_cancel_button_{};
    std::optional<application::AxisDraft>
        axis_draft_;
    std::optional<
        application::AxisDraftEvaluationResult>
        axis_evaluation_;

    QWidget* datum_plane_operations_widget_{};
    QComboBox* datum_plane_constructor_combo_{};
    QLabel* datum_plane_source_label_{};
    QLineEdit* datum_plane_offset_edit_{};
    QPushButton* datum_plane_reverse_button_{};
    QTimer* datum_plane_preview_timer_{};
    QLabel* datum_plane_result_label_{};
    QPushButton* datum_plane_finish_button_{};
    QPushButton* datum_plane_cancel_button_{};
    std::optional<application::DatumPlaneDraft>
        datum_plane_draft_;
    std::optional<
        application::DatumPlaneDraftEvaluationResult>
        datum_plane_evaluation_;
    bool datum_plane_offset_input_valid_{true};
    bool syncing_datum_plane_ui_{};

    QWidget* extrude_operations_widget_{};
    QPushButton* extrude_add_button_{};
    QPushButton* extrude_cut_button_{};
    QPushButton* extrude_one_side_button_{};
    QPushButton* extrude_midplane_button_{};
    QPushButton* extrude_reverse_button_{};
    QLineEdit* extrude_distance_edit_{};
    QTimer* extrude_preview_timer_{};
    QLabel* extrude_result_label_{};
    QPushButton* extrude_finish_button_{};
    QPushButton* extrude_cancel_button_{};
    std::optional<application::ExtrudeDraft>
        extrude_draft_;
    std::optional<
        application::ExtrudeDraftEvaluationResult>
        extrude_evaluation_;
    bool extrude_profile_pick_active_{};
    application::CadInputContextGeneration
        extrude_profile_pick_generation_{};
    bool extrude_distance_input_valid_{};
    bool syncing_extrude_ui_{};

    QWidget* revolve_operations_widget_{};
    QLabel* revolve_profile_label_{};
    QLabel* revolve_axis_label_{};
    QPushButton* revolve_add_button_{};
    QPushButton* revolve_cut_button_{};
    QPushButton* revolve_one_side_button_{};
    QPushButton* revolve_midplane_button_{};
    QPushButton* revolve_reverse_button_{};
    QLineEdit* revolve_angle_edit_{};
    QTimer* revolve_preview_timer_{};
    QLabel* revolve_result_label_{};
    QPushButton* revolve_finish_button_{};
    QPushButton* revolve_cancel_button_{};
    std::optional<application::RevolveDraft>
        revolve_draft_;
    std::optional<
        application::RevolveDraftEvaluationResult>
        revolve_evaluation_;
    bool revolve_angle_input_valid_{true};
    bool syncing_revolve_ui_{};


    QWidget* edge_feature_operations_widget_{};
    QLabel* edge_feature_title_label_{};
    QLabel* edge_feature_selection_label_{};
    QPushButton* edge_feature_clear_button_{};
    QLabel* edge_feature_parameter_name_label_{};
    QLineEdit* edge_feature_parameter_edit_{};
    QTimer* edge_feature_preview_timer_{};
    QLabel* edge_feature_result_label_{};
    QPushButton* edge_feature_finish_button_{};
    QPushButton* edge_feature_cancel_button_{};
    std::optional<application::FilletDraft>
        fillet_draft_;
    std::optional<application::ChamferDraft>
        chamfer_draft_;
    std::optional<
        application::EdgeFeatureDraftEvaluationResult>
        edge_feature_evaluation_;
    bool edge_feature_parameter_input_valid_{true};
    bool syncing_edge_feature_ui_{};

    QWidget* profile_operations_widget_{};
    QPushButton* profile_add_area_button_{};
    QPushButton* profile_subtract_area_button_{};
    QPushButton* profile_show_islands_button_{};
    QPushButton* profile_highlight_hover_button_{};
    QPushButton* profile_show_boundaries_button_{};
    QPushButton* profile_show_problems_button_{};
    QLabel* profile_result_label_{};
    QPushButton* profile_finish_button_{};
    QPushButton* profile_cancel_button_{};

    QLabel* status_{};
};

} // namespace simplesolid2::ui
