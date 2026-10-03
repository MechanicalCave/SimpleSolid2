#pragma once

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/application/document_session.hpp>
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
class QWidget;

namespace simplesolid2::kernel {
class ISolidModelingKernel;
}

namespace simplesolid2::ui {

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
        return extrude_draft_.has_value();
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

    [[nodiscard]] bool startExtrudeFromSelectedProfile();
    [[nodiscard]] bool startExtrudeEdit(
        part::FeatureId feature_id);
    void cancelExtrude();
    [[nodiscard]] bool finishExtrude();
    void clearExtrudeRuntimeContext();
    void refreshExtrudePreview();
    void syncExtrudeUi();
    [[nodiscard]] bool setExtrudeDistance(
        core::LengthValue distance,
        std::optional<std::string_view> display_text =
            std::nullopt);
    [[nodiscard]] application::CadInputSubmitResult
    submitExtrudeCadInput(std::string_view text);

    void setSketchSelectionRole(
        sketch::EntityRole role);
    void startSketchTool();
    void cancelSketchTool();
    void requestEditSketch(
        const sketch::SketchId& sketch_id);
    void requestEditProfile(
        part::ProfileId profile_id);
    void tryCreateSketchFromSupport(
        std::optional<core::BuiltinReferenceRole> support);
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
    void refreshBodyProperties(
        part::BodyId body_id);
    void refreshFeatureProperties(
        part::FeatureId feature_id);
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
    QWidget* profile_properties_page_{};
    QWidget* body_properties_page_{};
    QWidget* feature_properties_page_{};
    QLabel* active_path_{};
    QLabel* active_id_{};
    QLabel* reference_name_{};
    QLabel* reference_kind_{};
    QLabel* reference_identity_{};
    QLabel* reference_visibility_{};
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
    std::optional<part::FeatureId>
        selected_feature_id_;
    std::optional<part::BodyId>
        selected_body_id_;
    QLineEdit* number_{};
    QLineEdit* title_{};
    QPlainTextEdit* description_{};
    QLineEdit* engineering_revision_{};
    QComboBox* length_unit_combo_{};
    QPushButton* apply_button_{};

    QLabel* body_identity_{};
    QLabel* body_status_{};
    QLabel* body_feature_count_{};

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
    QPushButton* feature_go_to_profile_button_{};
    QPushButton* feature_edit_button_{};

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
    QPushButton* extrude_button_{};
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

    QWidget* extrude_operations_widget_{};
    QPushButton* extrude_add_button_{};
    QPushButton* extrude_cut_button_{};
    QPushButton* extrude_one_side_button_{};
    QPushButton* extrude_midplane_button_{};
    QPushButton* extrude_reverse_button_{};
    QLineEdit* extrude_distance_edit_{};
    QLabel* extrude_result_label_{};
    QPushButton* extrude_finish_button_{};
    QPushButton* extrude_cancel_button_{};
    std::optional<application::ExtrudeDraft>
        extrude_draft_;
    std::optional<
        application::ExtrudeDraftEvaluationResult>
        extrude_evaluation_;
    bool extrude_distance_input_valid_{};
    bool syncing_extrude_ui_{};

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
