#include "cad_workbench.hpp"
#include "project_workspace_shell.hpp"

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QAction>
#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>
#include <QTest>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numbers>
#include <optional>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-01 Workbench Sketch CHECK failed at line "
            << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr)     check(static_cast<bool>(expr), #expr, __LINE__)

class TestViewportWidget final
    : public QWidget,
      public viewer::IDocumentViewport {
public:
    explicit TestViewportWidget(QWidget* parent = nullptr)
        : QWidget{parent} {}

    [[nodiscard]] std::optional<viewer::CameraState>
    cameraState() const override {
        return state_;
    }

    bool setCameraState(
        const viewer::CameraState& state) override {
        state_ = state;
        return true;
    }

    bool setStandardView(
        viewer::StandardView view) override {
        const auto next =
            viewer::cameraForStandardView(
                state_,
                view);
        if (!next) return false;
        state_ = *next;
        last_standard_view_ = view;
        return true;
    }

    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                state_,
                projection);
        if (!next) return false;
        state_ = *next;
        return true;
    }

    void fitAll() override {
        ++fit_all_count_;
    }

    void setNavigationCubeActionHandler(
        viewer::NavigationCubeActionHandler handler) override {
        navigation_cube_handler_ =
            std::move(handler);
    }

    bool animateCameraState(
        const viewer::CameraState& state,
        double,
        bool fit_all) override {
        if (!setCameraState(state)) {
            return false;
        }
        if (fit_all) {
            fitAll();
        }
        return true;
    }

    void emitNavigationCubeAction(
        const viewer::NavigationCubeAction& action) {
        CHECK(static_cast<bool>(
            navigation_cube_handler_));
        navigation_cube_handler_(action);
    }

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        if (!scene.valid()) return false;
        scene_ = scene;
        return true;
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        if (!scene.valid()) return false;
        sketch_scene_ = scene;
        return true;
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        if (!scene.valid()) return false;
        sketch_preview_scene_ = scene;
        return true;
    }

    bool setSketchGripScene(
        const viewer::SketchGripScene& scene) override {
        if (!scene.valid()) return false;
        grip_scene_ = scene;
        return true;
    }

    bool setSketchInteractionPresentation(
        const viewer::SketchInteractionPresentation& presentation) override {
        if (!presentation.valid()) return false;
        interaction_presentation_ = presentation;
        return true;
    }

    viewer::SketchGripQueryResult querySketchGrip(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return grip_query_;
    }

    bool setSketchMeasureMarkerScene(
        const viewer::SketchMeasureMarkerScene& scene) override {
        if (!scene.valid()) return false;
        measure_marker_scene_ = scene;
        return true;
    }

    viewer::SketchMeasureMarkerQueryResult
    querySketchMeasureMarkers(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return measure_marker_query_;
    }

    bool setSketchMeasureCueScene(
        const viewer::SketchMeasureCueScene& scene) override {
        if (!scene.valid()) return false;
        measure_cue_scene_ = scene;
        return true;
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        if (!selection.valid()) return false;
        selection_ = selection;
        return true;
    }

    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return point_query_;
    }

    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule) override {
        return viewer::SketchRectangleQueryResult{
            rectangle.valid(),
            {}};
    }

    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override {
        return overlay.valid();
    }

    void clearSketchSelectionBoxOverlay() override {}

    bool setSketchDynamicInputOverlay(
        const viewer::SketchDynamicInputOverlay& overlay) override {
        if (!overlay.valid()) return false;
        dynamic_input_overlay_ = overlay;
        return true;
    }

    void clearSketchDynamicInputOverlay() override {
        dynamic_input_overlay_.reset();
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler_ =
            std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_pointer_handler_ =
            std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing) override {
        routing_ = routing;
    }

    void setCursorMode(
        viewer::ViewportCursorMode mode) override {
        cursor_mode_ = mode;
    }

    void emitSelectionIntent(
        viewer::PresentationToken token,
        viewer::SelectionIntentMode mode =
            viewer::SelectionIntentMode::replace) {
        CHECK(static_cast<bool>(
            selection_handler_));
        selection_handler_(
            viewer::SelectionIntent{
                token,
                mode});
    }

    void setSketchPointHit(
        std::optional<viewer::PresentationToken> token) {
        point_query_ = {true, token};
    }

    void setSketchGripHit(
        std::optional<viewer::SketchGripKey> grip) {
        grip_query_ = {true, grip};
    }

    void setSketchMeasureMarkerHits(
        std::vector<viewer::SketchMeasureMarkerKey> markers) {
        measure_marker_query_ = {
            true,
            std::move(markers)};
    }

    [[nodiscard]] const viewer::SketchMeasureMarkerScene&
    measureMarkerScene() const noexcept {
        return measure_marker_scene_;
    }

    [[nodiscard]] const viewer::SketchMeasureCueScene&
    measureCueScene() const noexcept {
        return measure_cue_scene_;
    }

    void emitSketchPointerXZ(
        viewer::SpatialPointerPhase phase,
        double sx,
        double sy,
        double u,
        double v,
        bool control = false) {
        CHECK(static_cast<bool>(
            spatial_pointer_handler_));
        spatial_pointer_handler_(
            viewer::SpatialPointerEvent{
                phase,
                {sx, sy},
                {{u, 1.0, v}, {0.0, -1.0, 0.0}},
                {control}});
    }

    [[nodiscard]] const viewer::SketchScene&
    sketchScene() const noexcept {
        return sketch_scene_;
    }

    [[nodiscard]] const viewer::SketchPreviewScene&
    sketchPreviewScene() const noexcept {
        return sketch_preview_scene_;
    }

    [[nodiscard]] const std::optional<viewer::SketchDynamicInputOverlay>&
    dynamicInputOverlay() const noexcept {
        return dynamic_input_overlay_;
    }

    [[nodiscard]] const viewer::SketchInteractionPresentation&
    interactionPresentation() const noexcept {
        return interaction_presentation_;
    }

    [[nodiscard]] const viewer::ReferenceScene&
    scene() const noexcept {
        return scene_;
    }

    [[nodiscard]] int fitAllCount() const noexcept {
        return fit_all_count_;
    }

    [[nodiscard]] std::optional<viewer::StandardView>
    lastStandardView() const noexcept {
        return last_standard_view_;
    }

private:
    viewer::CameraState state_;
    viewer::ReferenceScene scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene sketch_preview_scene_;
    std::optional<viewer::SketchDynamicInputOverlay>
        dynamic_input_overlay_;
    viewer::SketchGripScene grip_scene_;
    viewer::SketchInteractionPresentation
        interaction_presentation_;
    viewer::SketchPointQueryResult point_query_{
        true,
        std::nullopt};
    viewer::SketchGripQueryResult grip_query_{
        true,
        std::nullopt};
    viewer::SketchMeasureMarkerScene
        measure_marker_scene_;
    viewer::SketchMeasureMarkerQueryResult
        measure_marker_query_{true, {}};
    viewer::SketchMeasureCueScene
        measure_cue_scene_;
    viewer::PresentationSelection selection_;
    viewer::SpatialPointerHandler spatial_pointer_handler_;
    viewer::PrimaryPointerRouting routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
    viewer::SelectionIntentHandler
        selection_handler_;
    viewer::NavigationCubeActionHandler
        navigation_cube_handler_;
    int fit_all_count_{};
    std::optional<viewer::StandardView>
        last_standard_view_;
};

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_sk01_workbench_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

bool vectorEquals(
    const viewer::Vec3& value,
    double x,
    double y,
    double z) {
    return value.x == x &&
           value.y == y &&
           value.z == z;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace =
        temp.path / "Project";
    std::filesystem::create_directories(
        workspace);

    application::ProjectWorkspaceMetadataService
        metadata;
    CHECK(
        metadata.initialize(
            workspace,
            "SketchHost")
            .ok());

    auto opened =
        application::ProjectSession::open(
            workspace);
    CHECK(opened.ok());

    auto created_part =
        opened.session->createPart(
            "Part001.ss2part");
    CHECK(created_part.ok());
    const auto document_id =
        created_part.session->documentId();

    TestViewportWidget* viewport = nullptr;
    ui::ProjectWorkspaceShell workspace_shell;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            viewport =
                new TestViewportWidget{parent};
            return ui::ViewportSurface{
                viewport,
                viewport};
        },
        &workspace_shell};
    workspace_shell.setDocumentWorkbench(
        &workbench);
    workbench.setCadInputContextChangedHandler(
        [&workspace_shell] {
            workspace_shell.refreshCadInputPresentation();
        });
    workbench.setCadInteractionSettingsProvider(
        [&workspace_shell] {
            return workspace_shell.cadInteractionSettings();
        });
    workbench.setCadInteractionSettingsUpdater(
        [&workspace_shell](
            application::CadInteractionSettings settings) {
            return workspace_shell.setCadInteractionSettings(
                std::move(settings));
        });
    workspace_shell.setCadInteractionSettingsChangedHandler(
        [&workbench] {
            workbench.refreshCadInteractionSettingsUi();
        });

    workbench.setCadDynamicInputUiStateProvider(
        [&workspace_shell] {
            return ui::CadDynamicInputUiState{
                workspace_shell.cadInputBuffer(),
                workspace_shell.cadDynamicInputFieldIndex()};
        });
    workspace_shell.setCadInputPresentationChangedHandler(
        [&workbench] {
            workbench.refreshCadDynamicInputOverlay();
        });

    CHECK(
        workbench.activateDocument(
            opened.session->documentSession(
                document_id),
            workspace));
    workspace_shell.setCadInputEndpoint(
        &workbench);
    workspace_shell.showDocumentWorkbench();
    workspace_shell.show();
    QApplication::processEvents();
    CHECK(viewport != nullptr);

    auto* sketch_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchToolButton"));
    auto* cancel_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("cancelSketchButton"));
    auto* finish_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("finishSketchButton"));
    auto* finish_line_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("finishSketchLineButton"));
    auto* undo_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("undoDocumentButton"));
    auto* redo_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("redoDocumentButton"));
    auto* tree =
        workbench.findChild<QTreeWidget*>(
            QStringLiteral("documentTree"));
    auto* edit_sketch_action =
        workbench.findChild<QAction*>(
            QStringLiteral("editSketchAction"));
    auto* edit_profile_action =
        workbench.findChild<QAction*>(
            QStringLiteral("editProfileAction"));
    auto* profile_properties_page =
        workbench.findChild<QWidget*>(
            QStringLiteral("profilePropertiesPage"));
    auto* profile_name_edit =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("profilePropertyName"));
    auto* profile_visible_check =
        workbench.findChild<QCheckBox*>(
            QStringLiteral("profilePropertyVisible"));
    auto* profile_source_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("profilePropertySourceSketch"));
    auto* profile_status_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("profilePropertyStatus"));
    auto* profile_diagnostic_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("profilePropertyDiagnostic"));
    auto* profile_area_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("profilePropertyArea"));
    auto* profile_perimeter_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("profilePropertyPerimeter"));
    auto* profile_holes_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("profilePropertyHoles"));
    auto* delete_profile_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("deleteProfileButton"));
    auto* apply_profile_properties =
        workbench.findChild<QPushButton*>(
            QStringLiteral("applyProfilePropertiesButton"));
    auto* operations_content =
        workbench.findChild<QWidget*>(
            QStringLiteral("partOperationsContent"));
    auto* editor_host =
        workbench.findChild<QWidget*>(
            QStringLiteral("editorSurfaceHost"));
    auto* operations_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("operationsPlaceholder"));
    auto* line_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("lineSketchToolButton"));
    auto* circle_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("circleSketchToolButton"));
    auto* arc_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("arcSketchToolButton"));
    auto* rectangle_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("rectangleSketchToolButton"));
    auto* creation_construction_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchCreationConstructionButton"));
    auto* rectangle_diagonals_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchRectangleDiagonalsButton"));
    auto* profile_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileSketchToolButton"));
    auto* regular_role_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchRegularRoleButton"));
    auto* construction_role_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchConstructionRoleButton"));
    auto* profile_operations =
        workbench.findChild<QWidget*>(
            QStringLiteral("profileOperationsWidget"));
    auto* profile_add_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileAddAreaButton"));
    auto* profile_subtract_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileSubtractAreaButton"));
    auto* profile_detect_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileDetectIslandsButton"));
    auto* profile_highlight_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileHighlightHoverButton"));
    auto* profile_boundaries_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileShowBoundariesButton"));
    auto* profile_problems_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileShowProblemsButton"));
    auto* profile_find_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileFindRegionsButton"));
    auto* profile_finish_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileFinishButton"));
    auto* profile_cancel_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("profileCancelButton"));
    auto* move_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("moveSketchToolButton"));
    auto* copy_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("copySketchToolButton"));
    auto* create_tools_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("sketchCreateToolsLabel"));
    auto* modify_tools_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("sketchModifyToolsLabel"));
    auto* rotate_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("rotateSketchToolButton"));
    auto* scale_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("scaleSketchToolButton"));
    auto* mirror_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("mirrorSketchToolButton"));
    auto* inspect_tools_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("sketchInspectToolsLabel"));
    auto* measure_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("measureSketchToolButton"));
    auto* measure_between_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("measureBetweenButton"));
    auto* command_input =
        workspace_shell.findChild<QLineEdit*>(
            QStringLiteral("cadCommandInput"));
    auto* command_prompt =
        workspace_shell.findChild<QLabel*>(
            QStringLiteral("cadCommandPrompt"));
    auto* length_unit_combo =
        workbench.findChild<QComboBox*>(
            QStringLiteral("partLengthUnitCombo"));
    auto* precision_widget =
        workbench.findChild<QWidget*>(
            QStringLiteral("precisionOperationsWidget"));
    auto* precision_status =
        workbench.findChild<QLabel*>(
            QStringLiteral("precisionCadAidStatus"));
    auto* polar_toggle =
        workbench.findChild<QPushButton*>(
            QStringLiteral("polarToggleButton"));
    auto* dyn_toggle =
        workbench.findChild<QPushButton*>(
            QStringLiteral("dynamicInputToggleButton"));
    auto* polar_step =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("polarStepEdit"));
    auto* polar_reference =
        workbench.findChild<QComboBox*>(
            QStringLiteral("polarReferenceCombo"));
    auto* polar_additional =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("polarAdditionalAngleEdit"));
    auto* polar_add_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("polarAdditionalAngleAddButton"));
    auto* polar_clear_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("polarAdditionalAnglesClearButton"));
    auto* polar_additional_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("polarAdditionalAnglesLabel"));
    auto* circle_size_mode =
        workbench.findChild<QComboBox*>(
            QStringLiteral("circleSizeModeCombo"));

    CHECK(sketch_button != nullptr);
    CHECK(cancel_button != nullptr);
    CHECK(finish_button != nullptr);
    CHECK(finish_line_button != nullptr);
    CHECK(undo_button != nullptr);
    CHECK(redo_button != nullptr);
    CHECK(tree != nullptr);
    CHECK(edit_sketch_action != nullptr);
    CHECK(edit_profile_action != nullptr);
    CHECK(profile_properties_page != nullptr);
    CHECK(profile_name_edit != nullptr);
    CHECK(profile_visible_check != nullptr);
    CHECK(profile_source_label != nullptr);
    CHECK(profile_status_label != nullptr);
    CHECK(profile_diagnostic_label != nullptr);
    CHECK(profile_area_label != nullptr);
    CHECK(profile_perimeter_label != nullptr);
    CHECK(profile_holes_label != nullptr);
    CHECK(delete_profile_button != nullptr);
    CHECK(apply_profile_properties != nullptr);
    CHECK(operations_content != nullptr);
    CHECK(editor_host != nullptr);
    CHECK(operations_label != nullptr);
    CHECK(line_button != nullptr);
    CHECK(circle_button != nullptr);
    CHECK(arc_button != nullptr);
    CHECK(rectangle_button != nullptr);
    CHECK(creation_construction_button != nullptr);
    CHECK(rectangle_diagonals_button != nullptr);
    CHECK(profile_button != nullptr);
    CHECK(regular_role_button != nullptr);
    CHECK(construction_role_button != nullptr);
    CHECK(profile_operations != nullptr);
    CHECK(profile_add_button != nullptr);
    CHECK(profile_subtract_button != nullptr);
    CHECK(profile_detect_button != nullptr);
    CHECK(profile_highlight_button != nullptr);
    CHECK(profile_boundaries_button != nullptr);
    CHECK(profile_problems_button != nullptr);
    CHECK(profile_find_button != nullptr);
    CHECK(profile_finish_button != nullptr);
    CHECK(profile_cancel_button != nullptr);
    CHECK(move_button != nullptr);
    CHECK(copy_button != nullptr);
    CHECK(create_tools_label != nullptr);
    CHECK(modify_tools_label != nullptr);
    CHECK(rotate_button != nullptr);
    CHECK(scale_button != nullptr);
    CHECK(mirror_button != nullptr);
    CHECK(inspect_tools_label != nullptr);
    CHECK(measure_button != nullptr);
    CHECK(measure_between_button != nullptr);
    CHECK(command_input != nullptr);
    CHECK(command_prompt != nullptr);
    CHECK(length_unit_combo != nullptr);
    CHECK(precision_widget != nullptr);
    CHECK(precision_status != nullptr);
    CHECK(polar_toggle != nullptr);
    CHECK(dyn_toggle != nullptr);
    CHECK(polar_step != nullptr);
    CHECK(polar_reference != nullptr);
    CHECK(polar_additional != nullptr);
    CHECK(polar_add_button != nullptr);
    CHECK(polar_clear_button != nullptr);
    CHECK(polar_additional_label != nullptr);
    CHECK(circle_size_mode != nullptr);
    CHECK(editor_host->isAncestorOf(sketch_button));
    CHECK(!operations_content->isAncestorOf(sketch_button));
    CHECK(sketch_button->isEnabled());
    CHECK(cancel_button->isHidden());
    CHECK(finish_button->isHidden());
    CHECK(measure_between_button->isHidden());
    CHECK(
        operations_label->text() ==
        QStringLiteral("Part modeling context."));
    CHECK(
        length_unit_combo->currentText() ==
        QStringLiteral("mm"));
    CHECK(precision_widget->isHidden());

    auto* session =
        opened.session->documentSession(
            document_id);
    CHECK(session != nullptr);
    CHECK(session->document().sketches().empty());
    CHECK(!session->needsSave());

    sketch_button->click();
    CHECK(!cancel_button->isHidden());
    CHECK(finish_button->isHidden());
    CHECK(!sketch_button->isEnabled());

    const viewer::PresentationToken
        x_axis_token{0x102U};
    viewport->emitSelectionIntent(
        x_axis_token);
    QApplication::processEvents();

    CHECK(session->document().sketches().empty());
    CHECK(finish_button->isHidden());
    CHECK(!cancel_button->isHidden());

    const viewer::PresentationToken
        xz_plane_token{0x106U};
    viewport->emitSelectionIntent(
        xz_plane_token);
    QApplication::processEvents();

    CHECK(session->document().sketches().size() == 1U);
    CHECK(session->needsSave());
    CHECK(!finish_button->isHidden());
    CHECK(finish_button->isEnabled());
    CHECK(cancel_button->isHidden());
    CHECK(!sketch_button->isEnabled());

    const auto sketch_id =
        session->document().sketches().front().id;
    CHECK(
        session->document()
            .sketches()
            .front()
            .support
            .builtin_plane ==
        core::BuiltinReferenceRole::xz_plane);

    CHECK(viewport->scene().grid.has_value());
    CHECK(
        vectorEquals(
            viewport->scene().grid->u_axis,
            1.0, 0.0, 0.0));
    CHECK(
        vectorEquals(
            viewport->scene().grid->v_axis,
            0.0, 0.0, 1.0));
    CHECK(
        viewport->lastStandardView() ==
        viewer::StandardView::front);
    CHECK(viewport->fitAllCount() > 0);

    CHECK(!precision_widget->isHidden());
    CHECK(polar_toggle->isChecked());
    CHECK(!dyn_toggle->isChecked());
    CHECK(
        precision_status->text() ==
        QStringLiteral(
            "POLAR ON   360/8 = 45°   ABS   DYN OFF"));

    const auto precision_state_before =
        session->document().state();
    const auto precision_revision_before =
        session->document().revision();
    const auto precision_undo_before =
        session->undoDepth();

    polar_toggle->click();
    dyn_toggle->click();
    QApplication::processEvents();
    CHECK(!workspace_shell.cadInteractionSettings().polar.enabled);
    CHECK(
        workspace_shell.cadInteractionSettings().
            dynamic_input_enabled);
    CHECK(!polar_toggle->isChecked());
    CHECK(dyn_toggle->isChecked());
    CHECK(
        session->document().state() ==
        precision_state_before);
    CHECK(
        session->document().revision() ==
        precision_revision_before);
    CHECK(
        session->undoDepth() ==
        precision_undo_before);

    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClick(viewport, Qt::Key_F10);
    QTest::keyClick(viewport, Qt::Key_F12);
    QApplication::processEvents();
    CHECK(workspace_shell.cadInteractionSettings().polar.enabled);
    CHECK(
        !workspace_shell.cadInteractionSettings().
            dynamic_input_enabled);
    CHECK(polar_toggle->isChecked());
    CHECK(!dyn_toggle->isChecked());
    CHECK(
        precision_status->text() ==
        QStringLiteral(
            "POLAR ON   360/8 = 45°   ABS   DYN OFF"));
    CHECK(
        polar_reference->currentText() ==
        QStringLiteral("Absolute"));
    CHECK(
        polar_additional_label->text() ==
        QStringLiteral("none"));

    polar_step->setFocus(Qt::OtherFocusReason);
    polar_step->setText(
        QStringLiteral("360/12"));
    QTest::keyClick(
        polar_step,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(
        std::abs(
            workspace_shell.cadInteractionSettings()
                    .polar.primary_spacing -
            std::numbers::pi_v<double> / 6.0) <
        1.0e-12);
    CHECK(
        precision_status->text() ==
        QStringLiteral(
            "POLAR ON   360/12 = 30°   ABS   DYN OFF"));

    polar_reference->setCurrentIndex(1);
    QApplication::processEvents();
    CHECK(
        workspace_shell.cadInteractionSettings()
                .polar.reference_mode ==
        application::PolarReferenceMode::relative);
    CHECK(
        precision_status->text() ==
        QStringLiteral(
            "POLAR ON   360/12 = 30°   REL   DYN OFF"));

    polar_additional->setText(
        QStringLiteral("17"));
    polar_add_button->click();
    polar_additional->setText(
        QStringLiteral("30deg"));
    polar_add_button->click();
    QApplication::processEvents();
    CHECK(
        workspace_shell.cadInteractionSettings()
            .polar.additional_angles.size() == 2U);
    CHECK(
        polar_additional_label->text() ==
        QStringLiteral("17°, 30°"));

    const auto settings_before_invalid_step =
        workspace_shell.cadInteractionSettings();
    polar_step->setText(
        QStringLiteral("0"));
    QTest::keyClick(
        polar_step,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(
        workspace_shell.cadInteractionSettings() ==
        settings_before_invalid_step);

    polar_clear_button->click();
    polar_reference->setCurrentIndex(0);
    polar_step->setText(
        QStringLiteral("45"));
    QTest::keyClick(
        polar_step,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(
        workspace_shell.cadInteractionSettings()
            .polar.additional_angles.empty());
    CHECK(
        workspace_shell.cadInteractionSettings()
                .polar.reference_mode ==
        application::PolarReferenceMode::absolute);
    CHECK(
        std::abs(
            workspace_shell.cadInteractionSettings()
                    .polar.primary_spacing -
            std::numbers::pi_v<double> / 4.0) <
        1.0e-12);
    CHECK(
        session->document().state() ==
        precision_state_before);
    CHECK(
        session->document().revision() ==
        precision_revision_before);
    CHECK(
        session->undoDepth() ==
        precision_undo_before);

    // SK-07B: Sketch tools are visibly grouped and the new
    // transform adapters enter the same command-first collection stage.
    CHECK(!create_tools_label->isHidden());
    CHECK(!modify_tools_label->isHidden());
    CHECK(
        create_tools_label->text() ==
        QStringLiteral("Create:"));
    CHECK(
        modify_tools_label->text() ==
        QStringLiteral("Modify:"));
    CHECK(!inspect_tools_label->isHidden());
    CHECK(
        inspect_tools_label->text() ==
        QStringLiteral("Inspect:"));
    CHECK(!measure_button->isHidden());
    CHECK(!rotate_button->isHidden());
    CHECK(!scale_button->isHidden());
    CHECK(!mirror_button->isHidden());

    // R9 Owner manual-review correction: the top Create strip contains
    // tools only. Runtime creation options live in the right Operations
    // panel for the active creation tool.
    CHECK(!rectangle_button->isHidden());
    CHECK(creation_construction_button->isHidden());
    CHECK(rectangle_diagonals_button->isHidden());
    CHECK(
        creation_construction_button->parentWidget() ==
        operations_content);
    CHECK(
        rectangle_diagonals_button->parentWidget() ==
        operations_content);
    CHECK(!creation_construction_button->isChecked());
    CHECK(!rectangle_diagonals_button->isChecked());
    CHECK(construction_role_button->isHidden());

    circle_button->click();
    QApplication::processEvents();
    CHECK(!circle_size_mode->isHidden());
    CHECK(!circle_size_mode->isEnabled());
    CHECK(
        circle_size_mode->currentText() ==
        QStringLiteral("Diameter"));

    command_input->setText(
        QStringLiteral("0;0"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(circle_size_mode->isEnabled());
    circle_size_mode->setCurrentIndex(1);
    QApplication::processEvents();
    CHECK(
        circle_size_mode->currentText() ==
        QStringLiteral("Radius"));
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Circle — Radius mode [R]; specify size"));
    circle_size_mode->setCurrentIndex(0);
    QApplication::processEvents();
    CHECK(
        circle_size_mode->currentText() ==
        QStringLiteral("Diameter"));
    CHECK(
        session->document().revision() ==
        precision_revision_before);
    CHECK(
        session->undoDepth() ==
        precision_undo_before);

    rectangle_button->click();
    QApplication::processEvents();
    CHECK(rectangle_button->isChecked());
    CHECK(!creation_construction_button->isHidden());
    CHECK(!rectangle_diagonals_button->isHidden());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Rectangle — Specify first corner; Role: Regular; Draw Diagonals: Off"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: RECTANGLE — Specify first corner"));

    const auto r9_option_state_before =
        session->document().state();
    const auto r9_option_revision_before =
        session->document().revision();
    const auto r9_option_undo_before =
        session->undoDepth();

    creation_construction_button->click();
    rectangle_diagonals_button->click();
    QApplication::processEvents();
    CHECK(creation_construction_button->isChecked());
    CHECK(rectangle_diagonals_button->isChecked());
    CHECK(
        session->document().state() ==
        r9_option_state_before);
    CHECK(
        session->document().revision() ==
        r9_option_revision_before);
    CHECK(
        session->undoDepth() ==
        r9_option_undo_before);

    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Rectangle — Specify first corner; Role: Construction; Draw Diagonals: On"));

    // Runtime options can be returned to their defaults without authored
    // mutation; later tests in this host continue from Regular/OFF.
    creation_construction_button->click();
    rectangle_diagonals_button->click();
    QApplication::processEvents();
    CHECK(!creation_construction_button->isChecked());
    CHECK(!rectangle_diagonals_button->isChecked());
    CHECK(
        session->document().state() ==
        r9_option_state_before);
    CHECK(
        session->document().revision() ==
        r9_option_revision_before);
    CHECK(
        session->undoDepth() ==
        r9_option_undo_before);

    // Package F: toolbar, Operations and Command Line are adapters to one
    // controller-owned transient Profile session.
    CHECK(!profile_button->isHidden());
    CHECK(profile_operations->isHidden());

    const auto profile_ui_state_before =
        session->document().state();
    const auto profile_ui_revision_before =
        session->document().revision();
    const auto profile_ui_undo_before =
        session->undoDepth();

    profile_button->click();
    QApplication::processEvents();
    CHECK(profile_button->isChecked());
    CHECK(!profile_operations->isHidden());
    CHECK(profile_add_button->isChecked());
    CHECK(!profile_subtract_button->isChecked());
    CHECK(profile_detect_button->isChecked());
    CHECK(profile_highlight_button->isChecked());
    CHECK(!profile_boundaries_button->isChecked());
    CHECK(profile_problems_button->isChecked());
    CHECK(profile_finish_button->isEnabled() == false);
    CHECK(
        operations_label->text() ==
        QStringLiteral("Profile — Create"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: PROFILE — Add Area — Hover/click bounded region"));

    command_input->setText(
        QStringLiteral("SUBTRACT"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(profile_subtract_button->isChecked());
    CHECK(!profile_add_button->isChecked());
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: PROFILE — Subtract Area — Hover/click bounded region"));

    profile_add_button->click();
    QApplication::processEvents();
    CHECK(profile_add_button->isChecked());
    CHECK(!profile_subtract_button->isChecked());

    profile_boundaries_button->click();
    QApplication::processEvents();
    CHECK(profile_boundaries_button->isChecked());

    command_input->setText(
        QStringLiteral("BOUNDARIES OFF"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(!profile_boundaries_button->isChecked());

    profile_detect_button->click();
    QApplication::processEvents();
    CHECK(!profile_detect_button->isChecked());
    command_input->setText(
        QStringLiteral("ISLANDS ON"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(profile_detect_button->isChecked());

    profile_find_button->click();
    QApplication::processEvents();

    profile_cancel_button->click();
    QApplication::processEvents();
    CHECK(!profile_button->isChecked());
    CHECK(profile_operations->isHidden());
    CHECK(
        command_prompt->text() ==
        QStringLiteral("Command: SELECT"));

    CHECK(
        session->document().state() ==
        profile_ui_state_before);
    CHECK(
        session->document().revision() ==
        profile_ui_revision_before);
    CHECK(
        session->undoDepth() ==
        profile_ui_undo_before);

    command_input->setText(
        QStringLiteral("PROFILE"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(profile_button->isChecked());
    CHECK(!profile_operations->isHidden());
    CHECK(profile_add_button->isChecked());

    command_input->setText(
        QStringLiteral("CANCEL"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(!profile_button->isChecked());
    CHECK(profile_operations->isHidden());

    // Package F: authored Profiles are semantic Tree children of their
    // source Sketch. Tree selection carries ProfileId into Properties and
    // EDITPROFILE; labels/status are derived from current evaluation.
    const auto profile_tree_undo_baseline =
        session->undoDepth();

    const auto tree_circle =
        session->execute(
            application::AddSketchCircleCommand{
                sketch_id,
                sketch::Point2{50.0, 50.0},
                5.0});
    CHECK(tree_circle.ok());
    CHECK(tree_circle.entity_id.has_value());

    const auto* tree_profile_source =
        session->document()
            .findSketch(sketch_id);
    CHECK(tree_profile_source != nullptr);
    const auto tree_profile_analysis =
        sketch::analyzeRegions(
            tree_profile_source->model);
    const auto tree_profile_pick =
        sketch::pickRegion(
            tree_profile_source->model,
            tree_profile_analysis,
            sketch::Point2{50.0, 50.0});
    CHECK(tree_profile_pick.region_index.has_value());
    const auto tree_profile_region =
        std::find_if(
            tree_profile_analysis.regions.begin(),
            tree_profile_analysis.regions.end(),
            [&tree_profile_pick](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *tree_profile_pick.region_index;
            });
    CHECK(
        tree_profile_region !=
        tree_profile_analysis.regions.end());
    const auto tree_profile_intent =
        part::makeProfileRegionIntent(
            *tree_profile_region);
    CHECK(tree_profile_intent.has_value());

    const auto tree_profile_created =
        session->execute(
            application::CreateProfileCommand{
                sketch_id,
                session->document().revision(),
                *tree_profile_intent});
    CHECK(tree_profile_created.ok());
    CHECK(tree_profile_created.profile_id.has_value());
    const auto tree_profile_id =
        *tree_profile_created.profile_id;

    CHECK(
        workbench.activateDocument(
            session,
            workspace));
    QApplication::processEvents();

    auto profile_items =
        tree->findItems(
            QStringLiteral("Profile001"),
            Qt::MatchExactly |
                Qt::MatchRecursive,
            0);
    CHECK(profile_items.size() == 1);
    auto* profile_item =
        profile_items.front();
    CHECK(profile_item != nullptr);
    CHECK(profile_item->parent() != nullptr);
    CHECK(
        profile_item->parent()->text(0) ==
        QStringLiteral("Sketch 1"));

    tree->clearSelection();
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);
    QApplication::processEvents();

    CHECK(!profile_properties_page->isHidden());
    CHECK(
        profile_name_edit->text() ==
        QStringLiteral("Profile001"));
    CHECK(profile_visible_check->isChecked());
    CHECK(
        profile_source_label->text() ==
        QString::fromUtf8(
            sketch_id.value().data(),
            static_cast<qsizetype>(
                sketch_id.value().size())));
    CHECK(
        profile_status_label->text() ==
        QStringLiteral("Valid"));
    CHECK(
        profile_diagnostic_label->text() ==
        QStringLiteral("—"));
    CHECK(
        profile_area_label->text() !=
        QStringLiteral("—"));
    CHECK(
        profile_perimeter_label->text() !=
        QStringLiteral("—"));
    CHECK(
        profile_holes_label->text() ==
        QStringLiteral("0"));

    command_input->setText(
        QStringLiteral("EDITPROFILE"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(profile_button->isChecked());
    CHECK(!profile_operations->isHidden());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Profile — Edit — Profile001"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: EDITPROFILE — Add Area — Hover/click bounded region"));
    profile_cancel_button->click();
    QApplication::processEvents();

    profile_name_edit->setText(
        QStringLiteral("Main Profile"));
    profile_visible_check->setChecked(false);
    apply_profile_properties->click();
    QApplication::processEvents();

    const auto* renamed_profile =
        session->document()
            .findProfile(tree_profile_id);
    CHECK(renamed_profile != nullptr);
    CHECK(
        renamed_profile->name ==
        "Main Profile");
    CHECK(!renamed_profile->visible);

    profile_items =
        tree->findItems(
            QStringLiteral("Main Profile"),
            Qt::MatchExactly |
                Qt::MatchRecursive,
            0);
    CHECK(profile_items.size() == 1);
    profile_item = profile_items.front();
    CHECK(profile_item->font(0).italic());

    // Construction is a user-visible authored role, not a test-only
    // mutation. Select the source Circle normally and switch its role from
    // the Operations surface.
    CHECK(!viewport->sketchScene().curves.empty());
    viewport->setSketchGripHit(std::nullopt);
    viewport->setSketchPointHit(
        viewport->sketchScene().curves.front().token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        120.0, 120.0,
        50.0, 50.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        120.0, 120.0,
        50.0, 50.0);
    QApplication::processEvents();
    CHECK(!construction_role_button->isHidden());
    CHECK(regular_role_button->isChecked());
    construction_role_button->click();
    QApplication::processEvents();
    CHECK(construction_role_button->isChecked());
    CHECK(
        std::any_of(
            viewport->sketchScene().curves.begin(),
            viewport->sketchScene().curves.end(),
            [](const viewer::SketchCurvePresentation& curve) {
                return curve.construction;
            }));
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.findCircle(*tree_circle.entity_id)
            ->role() ==
        sketch::EntityRole::construction);

    // R8A: Construction remains inspectable and Measure is read-only.
    const auto construction_measure_state =
        session->document().state();
    const auto construction_measure_revision =
        session->document().revision();
    const auto construction_measure_undo =
        session->undoDepth();
    measure_button->click();
    QApplication::processEvents();
    CHECK(measure_button->isChecked());
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Measure — Circle [")));
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Role: Construction")));
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Radius:")));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE — Click Line/Circle/Arc; BETWEEN for relational; Esc ends"));
    CHECK(
        session->document().state() ==
        construction_measure_state);
    CHECK(
        session->document().revision() ==
        construction_measure_revision);
    CHECK(
        session->undoDepth() ==
        construction_measure_undo);
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!measure_button->isChecked());

    profile_items =
        tree->findItems(
            QStringLiteral("Main Profile [Invalid]"),
            Qt::MatchExactly |
                Qt::MatchRecursive,
            0);
    CHECK(profile_items.size() == 1);
    profile_item = profile_items.front();
    tree->clearSelection();
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);
    QApplication::processEvents();
    CHECK(
        profile_status_label->text() ==
        QStringLiteral("Invalid"));
    CHECK(
        profile_diagnostic_label->text() !=
        QStringLiteral("—"));
    CHECK(
        profile_area_label->text() ==
        QStringLiteral("—"));
    CHECK(
        profile_perimeter_label->text() ==
        QStringLiteral("—"));
    CHECK(
        profile_holes_label->text() ==
        QStringLiteral("—"));

    viewport->setSketchPointHit(
        viewport->sketchScene().curves.front().token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        120.0, 120.0,
        50.0, 50.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        120.0, 120.0,
        50.0, 50.0);
    QApplication::processEvents();
    regular_role_button->click();
    QApplication::processEvents();
    CHECK(regular_role_button->isChecked());
    CHECK(
        std::none_of(
            viewport->sketchScene().curves.begin(),
            viewport->sketchScene().curves.end(),
            [](const viewer::SketchCurvePresentation& curve) {
                return curve.construction;
            }));
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.findCircle(*tree_circle.entity_id)
            ->role() ==
        sketch::EntityRole::regular);
    CHECK(
        tree->findItems(
                QStringLiteral("Main Profile"),
                Qt::MatchExactly |
                    Qt::MatchRecursive,
                0)
            .size() == 1);

    // Profile deletion acts on Profile identity only. Source Sketch geometry
    // remains authored, and Undo restores the same ProfileId.
    profile_items =
        tree->findItems(
            QStringLiteral("Main Profile"),
            Qt::MatchExactly |
                Qt::MatchRecursive,
            0);
    CHECK(profile_items.size() == 1);
    profile_item = profile_items.front();
    tree->clearSelection();
    profile_item->setSelected(true);
    tree->setCurrentItem(profile_item);
    QApplication::processEvents();

    const auto entities_before_profile_delete =
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount();
    CHECK(!profile_properties_page->isHidden());
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClick(viewport, Qt::Key_Delete);
    QApplication::processEvents();
    CHECK(
        session->document()
            .findProfile(tree_profile_id) == nullptr);
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() ==
        entities_before_profile_delete);

    undo_button->click();
    QApplication::processEvents();
    CHECK(
        session->document()
            .findProfile(tree_profile_id) != nullptr);
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() ==
        entities_before_profile_delete);

    while (session->undoDepth() >
           profile_tree_undo_baseline) {
        const auto undone =
            session->undo();
        CHECK(undone.ok());
        CHECK(undone.changed);
    }
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 0U);
    CHECK(session->document().profiles().empty());
    CHECK(
        workbench.activateDocument(
            session,
            workspace));
    QApplication::processEvents();

    // SK-07D: a fresh Sketch edit session has no repeat target.
    QTest::keyClick(viewport, Qt::Key_Return);
    QApplication::processEvents();
    CHECK(!line_button->isChecked());
    CHECK(!circle_button->isChecked());
    CHECK(!arc_button->isChecked());
    CHECK(!move_button->isChecked());
    CHECK(!copy_button->isChecked());

    // SK-07C: toolbar and Command Line COPY are adapters to the
    // same command-first semantic COPY state.
    CHECK(!copy_button->isHidden());
    copy_button->click();
    QApplication::processEvents();
    CHECK(copy_button->isChecked());
    CHECK(QApplication::focusWidget() == viewport);
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Copy — Select objects; Enter/Space/RMB to continue"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: COPY — Select objects; Enter/Space/RMB to continue"));
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!copy_button->isChecked());

    command_input->setText(
        QStringLiteral("COPY"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(copy_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Copy — Select objects; Enter/Space/RMB to continue"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: COPY — Select objects; Enter/Space/RMB to continue"));
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!copy_button->isChecked());

    rotate_button->click();
    QApplication::processEvents();
    CHECK(rotate_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Rotate — Select objects; Enter/Space/RMB to continue"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: ROTATE — Select objects; Enter/Space/RMB to continue"));
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!rotate_button->isChecked());

    command_input->setText(
        QStringLiteral("SCALE"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(scale_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Scale — Select objects; Enter/Space/RMB to continue"));
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!scale_button->isChecked());

    command_input->setText(
        QStringLiteral("MIRROR"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(mirror_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Mirror — Select objects; Enter/Space/RMB to continue"));
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!mirror_button->isChecked());

    // SK-07A: toolbar and Command Line are adapters to the same
    // command-first MOVE state when the Sketch selection is empty.
    CHECK(!move_button->isHidden());
    move_button->click();
    QApplication::processEvents();
    CHECK(move_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Move — Select objects; Enter/Space/RMB to continue"));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MOVE — Select objects; Enter/Space/RMB to continue"));

    QTest::keyClick(
        viewport,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!move_button->isChecked());

    command_input->clear();
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("MOVE"));
    QApplication::processEvents();
    CHECK(
        command_input->text() ==
        QStringLiteral("MOVE"));
    CHECK(QApplication::focusWidget() == viewport);
    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(move_button->isChecked());
    CHECK(command_input->text().isEmpty());
    CHECK(QApplication::focusWidget() == viewport);
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Move — Select objects; Enter/Space/RMB to continue"));

    command_input->setText(
        QStringLiteral("A"));
    command_input->setFocus();
    QTest::keyClick(
        command_input,
        Qt::Key_Space);
    QApplication::processEvents();
    CHECK(
        command_input->text() ==
        QStringLiteral("A "));
    CHECK(move_button->isChecked());

    // Escape in text focus clears text/focus only; viewport Escape
    // then cancels the active MOVE and returns to Select.
    QTest::keyClick(
        command_input,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(command_input->text().isEmpty());
    CHECK(move_button->isChecked());
    QTest::keyClick(
        viewport,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!move_button->isChecked());

    // SK-07D: MOVE remains the last repeatable command after Esc.
    // Viewport Enter and Space repeat it from ordinary Select.
    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(move_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Move — Select objects; Enter/Space/RMB to continue"));

    // Space while MOVE is already active retains transform-selection
    // precedence and must not start a second repeated command.
    QTest::keyClick(
        viewport,
        Qt::Key_Space);
    QApplication::processEvents();
    CHECK(move_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Move — Select objects; Enter/Space/RMB to continue"));

    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!move_button->isChecked());

    QTest::keyClick(
        viewport,
        Qt::Key_Space);
    QApplication::processEvents();
    CHECK(move_button->isChecked());
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!move_button->isChecked());

    // Explicit Line activation replaces MOVE as the repeat target.
    line_button->click();
    QApplication::processEvents();
    CHECK(line_button->isChecked());
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!line_button->isChecked());

    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(line_button->isChecked());
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!line_button->isChecked());

    // Text-entry focus keeps text semantics; Space and empty Return do
    // not invoke viewport Repeat Last Command.
    command_input->clear();
    command_input->setFocus();
    QTest::keyClick(
        command_input,
        Qt::Key_Space);
    QApplication::processEvents();
    CHECK(
        command_input->text() ==
        QStringLiteral(" "));
    CHECK(!line_button->isChecked());

    command_input->clear();
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(!line_button->isChecked());

    if (viewport != nullptr) {
        viewport->setFocus(Qt::OtherFocusReason);
    }

    // SK-07E: viewport Space cycles the active grip EditMode and has
    // precedence over SK-07D Repeat Last Command. Build one temporary
    // Line through the normal UI so the Workbench key route is exercised.
    line_button->click();
    QApplication::processEvents();
    CHECK(line_button->isChecked());

    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        140.0, 100.0,
        1.0, 0.0);

    // R10 integration regression: DYN ON must project the real semantic
    // fields next to the current pointer, and the one Workspace CAD token
    // must appear in the focused field instead of living only in Command Line.
    dyn_toggle->click();
    QApplication::processEvents();
    CHECK(viewport->dynamicInputOverlay().has_value());
    CHECK(viewport->dynamicInputOverlay()->fields.size() == 4U);
    CHECK(
        viewport->dynamicInputOverlay()->fields[0].label ==
        "Distance");
    CHECK(viewport->dynamicInputOverlay()->anchor.x == 140.0);
    CHECK(viewport->dynamicInputOverlay()->anchor.y == 100.0);

    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(viewport, QStringLiteral("12"));
    QApplication::processEvents();
    CHECK(command_input->text() == QStringLiteral("12"));
    CHECK(viewport->dynamicInputOverlay().has_value());
    CHECK(
        viewport->dynamicInputOverlay()->fields[0].display_value ==
        "12");
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(command_input->text().isEmpty());

    dyn_toggle->click();
    QApplication::processEvents();
    CHECK(!viewport->dynamicInputOverlay().has_value());

    // SK-07F/WB-02: an active semantic PointRequest owns Command
    // Line submission before top-level command activation. Rejection
    // keeps LINE authoritative but Enter consumes the submitted token.
    command_input->setText(QStringLiteral("MOVE"));
    QTest::keyClick(command_input, Qt::Key_Return);
    QApplication::processEvents();
    CHECK(line_button->isChecked());
    CHECK(!move_button->isChecked());
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 0U);
    CHECK(command_input->text().isEmpty());

    // AUDIT-01 A1: a token is bound to the semantic request, not
    // merely the CadWorkbench object. Pointer movement within the same
    // PointRequest preserves it; replacing the request clears it.
    command_input->clear();
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("12"));
    CHECK(command_input->text() == QStringLiteral("12"));
    const auto request_guard_revision =
        session->document().revision();
    const auto request_guard_undo =
        session->undoDepth();

    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        160.0, 100.0,
        2.0, 0.0);
    QApplication::processEvents();
    CHECK(command_input->text() == QStringLiteral("12"));

    finish_line_button->click();
    QApplication::processEvents();
    CHECK(command_input->text().isEmpty());
    CHECK(!line_button->isChecked());
    CHECK(
        session->document().revision() ==
        request_guard_revision);
    CHECK(session->undoDepth() == request_guard_undo);

    // Return to LINE for the existing Direct Distance scenarios.
    line_button->click();
    QApplication::processEvents();
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        140.0, 100.0,
        1.0, 0.0);

    // Current-locale decimal separator and '.' both feed the same
    // Direct Distance path. Keep two segments temporarily so both
    // forms are exercised without changing the later Sketch workflow.
    const QLocale previous_locale = QLocale{};
    QLocale::setDefault(QLocale{QStringLiteral("pl_PL")});
    command_input->clear();
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("10,5"));
    CHECK(
        command_input->text() ==
        QStringLiteral("10,5"));
    CHECK(QApplication::focusWidget() == viewport);
    QTest::keyClick(viewport, Qt::Key_Return);
    QApplication::processEvents();
    QLocale::setDefault(previous_locale);
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 1U);

    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        140.0, 120.0,
        10.5, 1.0);
    command_input->setText(QStringLiteral("5.5"));
    QTest::keyClick(command_input, Qt::Key_Return);
    QApplication::processEvents();
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 2U);

    finish_line_button->click();
    QApplication::processEvents();
    CHECK(!line_button->isChecked());
    CHECK(!viewport->sketchScene().lines.empty());

    auto line_token =
        viewport->sketchScene().lines.front().token;
    viewport->setSketchPointHit(line_token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        120.0, 100.0,
        5.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        120.0, 100.0,
        5.0, 0.0);
    QApplication::processEvents();
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // R8A toolbar and Command Line are adapters to the same read-only
    // Measure state and do not replace normal selection or history.
    const auto measure_ui_state =
        session->document().state();
    const auto measure_ui_revision =
        session->document().revision();
    const auto measure_ui_undo =
        session->undoDepth();

    measure_button->click();
    QApplication::processEvents();
    CHECK(measure_button->isChecked());
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Measure — Line [")));
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Length:")));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE — Click Line/Circle/Arc; BETWEEN for relational; Esc ends"));
    CHECK(session->document().state() == measure_ui_state);
    CHECK(session->document().revision() == measure_ui_revision);
    CHECK(session->undoDepth() == measure_ui_undo);
    CHECK(!measure_between_button->isHidden());
    CHECK(!measure_between_button->isChecked());

    // R8B Operations action enters the same semantic Between state used by
    // Command Line. Runtime marker/cue presentation remains read-only.
    measure_between_button->click();
    QApplication::processEvents();
    CHECK(measure_between_button->isChecked());
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Measure Between")));
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Target A: Choose")));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE BETWEEN — Choose Target A; Esc returns to Measure"));
    CHECK(!viewport->measureMarkerScene().markers.empty());

    const auto point_marker =
        viewport->measureMarkerScene().markers.front().key;
    viewport->setSketchMeasureMarkerHits(
        {point_marker});
    viewport->setSketchPointHit(
        viewport->sketchScene().lines.back().token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        120.0, 100.0,
        0.0, 0.0);
    QApplication::processEvents();

    CHECK(
        operations_label->text().contains(
            QStringLiteral("Target A: Point [")));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE BETWEEN — Choose Target B; Esc returns to Measure"));
    CHECK(
        viewport->measureMarkerScene().selected.size() == 1U);

    viewport->setSketchMeasureMarkerHits({});
    viewport->setSketchPointHit(
        viewport->sketchScene().lines.back().token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        140.0, 120.0,
        10.5, 1.0);
    QApplication::processEvents();

    CHECK(
        operations_label->text().contains(
            QStringLiteral("Perpendicular distance:")));
    CHECK(
        operations_label->text().contains(
            QStringLiteral(
                "Line semantics: infinite supporting line")));
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE BETWEEN — Result shown; choose next Target A; Esc returns to Measure"));
    CHECK(!viewport->measureCueScene().empty());
    CHECK(session->document().state() == measure_ui_state);
    CHECK(session->document().revision() == measure_ui_revision);
    CHECK(session->undoDepth() == measure_ui_undo);

    // First Esc returns only to ordinary Measure and clears R8B presentation.
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(measure_button->isChecked());
    CHECK(!measure_between_button->isChecked());
    CHECK(viewport->measureMarkerScene().empty());
    CHECK(viewport->measureCueScene().empty());
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE — Click Line/Circle/Arc; BETWEEN for relational; Esc ends"));

    // Command Line BETWEEN re-enters the same tool-local semantic state.
    command_input->setText(QStringLiteral("BETWEEN"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(measure_between_button->isChecked());
    CHECK(command_input->text().isEmpty());
    CHECK(
        command_prompt->text() ==
        QStringLiteral(
            "Command: MEASURE BETWEEN — Choose Target A; Esc returns to Measure"));

    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(measure_button->isChecked());
    CHECK(!measure_between_button->isChecked());

    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!measure_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    command_input->setText(
        QStringLiteral("MEASURE"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(measure_button->isChecked());
    CHECK(command_input->text().isEmpty());
    CHECK(
        operations_label->text().contains(
            QStringLiteral("Measure — Line [")));
    CHECK(session->document().state() == measure_ui_state);
    CHECK(session->document().revision() == measure_ui_revision);
    CHECK(session->undoDepth() == measure_ui_undo);
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!measure_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // R10 Measure presentation follows the durable Part display/input
    // unit while the measured Sketch geometry remains unchanged.
    const auto measured_geometry_before_unit_switch =
        session->document()
            .findSketch(sketch_id)
            ->model.state();
    const auto measure_unit_revision_before =
        session->document().revision();
    const auto measure_unit_undo_before =
        session->undoDepth();

    command_input->setText(
        QStringLiteral("MEASURE"));
    QTest::keyClick(
        command_input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(measure_button->isChecked());
    CHECK(
        operations_label->text().contains(
            QStringLiteral(
                "Length: 10.5 mm")));

    length_unit_combo->setCurrentIndex(3);
    QApplication::processEvents();
    CHECK(
        session->document().lengthUnit() ==
        core::LengthUnit::inch);
    CHECK(measure_button->isChecked());
    CHECK(
        operations_label->text().contains(
            QStringLiteral(
                "Length: 0.413385826772 in")));
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.state() ==
        measured_geometry_before_unit_switch);
    CHECK(
        session->document().revision() !=
        measure_unit_revision_before);
    CHECK(
        session->undoDepth() ==
        measure_unit_undo_before + 1U);

    length_unit_combo->setCurrentIndex(0);
    QApplication::processEvents();
    CHECK(
        session->document().lengthUnit() ==
        core::LengthUnit::millimetre);
    CHECK(measure_button->isChecked());
    CHECK(
        operations_label->text().contains(
            QStringLiteral(
                "Length: 10.5 mm")));
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.state() ==
        measured_geometry_before_unit_switch);
    CHECK(
        session->undoDepth() ==
        measure_unit_undo_before + 2U);

    QTest::keyClick(
        viewport,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!measure_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // AUDIT-01 A2: Delete has CAD-input precedence while a live
    // token exists and must not fall through to semantic geometry
    // deletion. The append-only viewport token itself is unchanged.
    const auto delete_guard_state =
        session->document().state();
    const auto delete_guard_revision =
        session->document().revision();
    const auto delete_guard_undo =
        session->undoDepth();
    const auto delete_guard_redo =
        session->redoDepth();
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("TE"));
    CHECK(command_input->text() == QStringLiteral("TE"));
    QTest::keyClick(
        viewport,
        Qt::Key_Delete);
    QApplication::processEvents();
    CHECK(command_input->text() == QStringLiteral("TE"));
    CHECK(session->document().state() == delete_guard_state);
    CHECK(
        session->document().revision() ==
        delete_guard_revision);
    CHECK(session->undoDepth() == delete_guard_undo);
    CHECK(session->redoDepth() == delete_guard_redo);
    QTest::keyClick(
        viewport,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(command_input->text().isEmpty());
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // WB-02: normal MOVE consumes the same global keyboard-first
    // buffer. Restore the authored fixture after the commit so later
    // grip/history assertions keep their original baseline.
    const auto keyboard_baseline =
        session->document().state();
    const auto keyboard_undo =
        session->undoDepth();

    move_button->click();
    QApplication::processEvents();
    CHECK(move_button->isChecked());
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        120.0, 100.0,
        1.0, 0.0);
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("5"));
    CHECK(
        command_input->text() ==
        QStringLiteral("5"));
    CHECK(QApplication::focusWidget() == viewport);
    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(!move_button->isChecked());
    CHECK(command_input->text().isEmpty());
    CHECK(session->undoDepth() == keyboard_undo + 1U);

    undo_button->click();
    QApplication::processEvents();
    CHECK(session->document().state() == keyboard_baseline);
    CHECK(session->undoDepth() == keyboard_undo);
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // WB-02: repeated COPY placement is also keyboard-first. COPY
    // remains active after accepted placement; Esc ends the transient
    // session and Undo removes only that placement.
    copy_button->click();
    QApplication::processEvents();
    CHECK(copy_button->isChecked());
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        100.0, 120.0,
        0.0, 1.0);
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("5"));
    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(copy_button->isChecked());
    CHECK(command_input->text().isEmpty());
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 3U);
    CHECK(session->undoDepth() == keyboard_undo + 1U);

    QTest::keyClick(
        viewport,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(!copy_button->isChecked());
    undo_button->click();
    QApplication::processEvents();
    CHECK(session->document().state() == keyboard_baseline);
    CHECK(session->undoDepth() == keyboard_undo);
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 2U);
    CHECK(!viewport->sketchScene().lines.empty());
    line_token =
        viewport->sketchScene().lines.front().token;

    // History restoration is authored-state authority, not selection
    // presentation authority. Re-establish the semantic selection
    // explicitly before testing grip input.
    viewport->setSketchGripHit(std::nullopt);
    viewport->setSketchPointHit(line_token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        120.0, 100.0,
        5.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        120.0, 100.0,
        5.0, 0.0);
    QApplication::processEvents();
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // WB-02: grip Reshape uses the global keyboard buffer and the
    // existing SK-07F semantic Direct Distance resolver.
    viewport->setSketchGripHit(
        viewer::SketchGripKey{
            line_token,
            viewer::SketchGripRole::line_start});
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        110.0, 100.0,
        1.0, 0.0);
    QApplication::processEvents();
    CHECK(
        viewport->interactionPresentation().
            active_grip.has_value());

    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("5"));
    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(
        !viewport->interactionPresentation().
            active_grip.has_value());
    CHECK(command_input->text().isEmpty());
    CHECK(session->undoDepth() == keyboard_undo + 1U);

    undo_button->click();
    QApplication::processEvents();
    CHECK(session->document().state() == keyboard_baseline);
    CHECK(session->undoDepth() == keyboard_undo);
    CHECK(!viewport->sketchScene().lines.empty());
    line_token =
        viewport->sketchScene().lines.front().token;

    viewport->setSketchGripHit(std::nullopt);
    viewport->setSketchPointHit(line_token);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        120.0, 100.0,
        5.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        120.0, 100.0,
        5.0, 0.0);
    QApplication::processEvents();
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));

    // Establish the direction in Reshape, then Space-switch to Move.
    // The same pointer candidate must survive the mode cycle and the
    // numeric value must still arrive through the global buffer.
    viewport->setSketchGripHit(
        viewer::SketchGripKey{
            line_token,
            viewer::SketchGripRole::line_start});
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        110.0, 100.0,
        1.0, 0.0);
    QTest::keyClick(
        viewport,
        Qt::Key_Space);
    QApplication::processEvents();
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Grip — Move; Space cycles mode; Enter/LMB commits; Esc cancels"));

    QTest::keyClicks(
        viewport,
        QStringLiteral("5"));
    QTest::keyClick(
        viewport,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(
        !viewport->interactionPresentation().
            active_grip.has_value());
    CHECK(command_input->text().isEmpty());
    CHECK(session->undoDepth() == keyboard_undo + 1U);

    undo_button->click();
    QApplication::processEvents();
    CHECK(session->document().state() == keyboard_baseline);
    CHECK(session->undoDepth() == keyboard_undo);
    CHECK(!viewport->sketchScene().lines.empty());
    line_token =
        viewport->sketchScene().lines.front().token;

    // Re-enter an uncommitted grip session for the existing SK-07E
    // cycle/no-mutation regression below.
    viewport->setSketchGripHit(
        viewer::SketchGripKey{
            line_token,
            viewer::SketchGripRole::line_start});
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_press,
        100.0, 100.0,
        0.0, 0.0);
    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::primary_release,
        100.0, 100.0,
        0.0, 0.0);
    QApplication::processEvents();
    CHECK(
        viewport->interactionPresentation().
            active_grip.has_value());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Grip — Reshape; Space cycles mode; Enter/LMB commits; Esc cancels"));

    viewport->emitSketchPointerXZ(
        viewer::SpatialPointerPhase::move,
        110.0, 110.0,
        2.0, 3.0);
    QApplication::processEvents();

    const auto cycle_state =
        session->document().state();
    const auto cycle_revision =
        session->document().revision();
    const auto cycle_undo =
        session->undoDepth();

    QTest::keyClick(viewport, Qt::Key_Space);
    QApplication::processEvents();
    CHECK(!line_button->isChecked());
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Grip — Move; Space cycles mode; Enter/LMB commits; Esc cancels"));
    CHECK(session->document().state() == cycle_state);
    CHECK(session->document().revision() == cycle_revision);
    CHECK(session->undoDepth() == cycle_undo);

    QTest::keyClick(viewport, Qt::Key_Space);
    QApplication::processEvents();
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Grip — Rotate; Space cycles mode; Enter/LMB commits; Esc cancels"));

    command_input->clear();
    command_input->setFocus();
    QTest::keyClick(command_input, Qt::Key_Space);
    QApplication::processEvents();
    CHECK(
        command_input->text() ==
        QStringLiteral(" "));
    CHECK(
        operations_label->text() ==
        QStringLiteral(
            "Grip — Rotate; Space cycles mode; Enter/LMB commits; Esc cancels"));

    QTest::keyClick(command_input, Qt::Key_Escape);
    QApplication::processEvents();
    QTest::keyClick(viewport, Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(
        operations_label->text() ==
        QStringLiteral("Select — 1 entity selected"));
    CHECK(session->document().state() == cycle_state);
    CHECK(session->document().revision() == cycle_revision);
    CHECK(session->undoDepth() == cycle_undo);

    // Retire the two R10 Measure unit-presentation commands before
    // the original SK-07F history cleanup. This restores the pre-check
    // authored Part unit and keeps the later Line Undo expectations intact.
    undo_button->click();
    QApplication::processEvents();
    CHECK(
        session->document().lengthUnit() ==
        core::LengthUnit::inch);
    undo_button->click();
    QApplication::processEvents();
    CHECK(
        session->document().lengthUnit() ==
        core::LengthUnit::millimetre);
    CHECK(
        length_unit_combo->currentText() ==
        QStringLiteral("mm"));

    // Remove the two temporary SK-07F Line segments so later history
    // assertions retain their original pre-SK-07E shape.
    undo_button->click();
    QApplication::processEvents();
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 1U);
    undo_button->click();
    QApplication::processEvents();
    CHECK(
        session->document()
            .findSketch(sketch_id)
            ->model.entityCount() == 0U);
    viewport->setSketchPointHit(std::nullopt);
    viewport->setSketchGripHit(std::nullopt);

    auto* root =
        tree->topLevelItem(0);
    CHECK(root != nullptr);
    CHECK(root->childCount() == 2);
    CHECK(
        root->child(1)->text(0) ==
        QStringLiteral("Sketches"));
    CHECK(root->child(1)->childCount() == 1);
    CHECK(
        root->child(1)->child(0)->text(0) ==
        QStringLiteral("Sketch 1"));

    const auto authored_after_create =
        session->document().state();
    const auto revision_after_create =
        session->document().revision();

    viewport->emitNavigationCubeAction(
        viewer::NavigationCubeAction::orientTo(
            viewer::NavigationCubeTarget::top));
    QApplication::processEvents();

    CHECK(
        viewport->cameraState()->projection ==
        viewer::CameraProjection::orthographic);
    CHECK(
        session->document().state() ==
        authored_after_create);
    CHECK(
        session->document().revision() ==
        revision_after_create);
    CHECK(session->needsSave());

    // AUDIT-01 A1: Finish Sketch replaces the semantic editing
    // context even though the same CadWorkbench endpoint object remains.
    viewport->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        viewport,
        QStringLiteral("STALE"));
    CHECK(command_input->text() == QStringLiteral("STALE"));
    const auto finish_guard_state =
        session->document().state();
    const auto finish_guard_revision =
        session->document().revision();
    const auto finish_guard_undo =
        session->undoDepth();

    finish_button->click();
    QApplication::processEvents();

    CHECK(command_input->text().isEmpty());
    CHECK(session->document().state() == finish_guard_state);
    CHECK(
        session->document().revision() ==
        finish_guard_revision);
    CHECK(session->undoDepth() == finish_guard_undo);

    CHECK(finish_button->isHidden());
    CHECK(cancel_button->isHidden());
    CHECK(sketch_button->isEnabled());
    CHECK(
        operations_label->text() ==
        QStringLiteral("Part modeling context."));
    CHECK(session->document().sketches().size() == 1U);
    CHECK(viewport->scene().grid.has_value());
    CHECK(
        vectorEquals(
            viewport->scene().grid->u_axis,
            1.0, 0.0, 0.0));
    CHECK(
        vectorEquals(
            viewport->scene().grid->v_axis,
            0.0, 1.0, 0.0));

    auto* sketches_node = root->child(1);
    CHECK(sketches_node != nullptr);
    auto* sketch_item = sketches_node->child(0);
    CHECK(sketch_item != nullptr);

    const auto before_reedit_state =
        session->document().state();
    const auto before_reedit_revision =
        session->document().revision();

    tree->setCurrentItem(sketch_item);
    edit_sketch_action->trigger();
    QApplication::processEvents();

    CHECK(!finish_button->isHidden());
    CHECK(
        session->document().state() ==
        before_reedit_state);
    CHECK(
        session->document().revision() ==
        before_reedit_revision);

    finish_button->click();
    QApplication::processEvents();
    CHECK(finish_button->isHidden());

    tree->scrollToItem(sketch_item);
    QApplication::processEvents();
    const auto sketch_rect =
        tree->visualItemRect(sketch_item);
    CHECK(sketch_rect.isValid());
    QTest::mouseDClick(
        tree->viewport(),
        Qt::LeftButton,
        Qt::NoModifier,
        sketch_rect.center());
    QApplication::processEvents();

    CHECK(!finish_button->isHidden());
    CHECK(
        session->document().state() ==
        before_reedit_state);
    CHECK(
        session->document().revision() ==
        before_reedit_revision);

    finish_button->click();
    QApplication::processEvents();
    CHECK(finish_button->isHidden());

    undo_button->click();
    QApplication::processEvents();
    CHECK(session->document().sketches().empty());
    CHECK(redo_button->isEnabled());

    redo_button->click();
    QApplication::processEvents();
    CHECK(session->document().sketches().size() == 1U);
    CHECK(
        session->document().sketches().front().id ==
        sketch_id);
    CHECK(finish_button->isHidden());

    sketch_button->click();
    const viewer::PresentationToken
        yz_plane_token{0x107U};
    viewport->emitSelectionIntent(
        yz_plane_token);
    QApplication::processEvents();

    CHECK(session->document().sketches().size() == 2U);
    CHECK(finish_button->isEnabled());
    CHECK(
        viewport->lastStandardView() ==
        viewer::StandardView::right);

    undo_button->click();
    QApplication::processEvents();

    CHECK(session->document().sketches().size() == 1U);
    CHECK(finish_button->isHidden());
    CHECK(sketch_button->isEnabled());
    CHECK(
        session->document().sketches().front().id ==
        sketch_id);

    sketch_button->click();
    const viewer::PresentationToken
        xy_plane_token{0x105U};
    viewport->emitSelectionIntent(
        xy_plane_token);
    QApplication::processEvents();

    CHECK(session->document().sketches().size() == 2U);
    CHECK(finish_button->isEnabled());
    CHECK(
        viewport->lastStandardView() ==
        viewer::StandardView::top);
    CHECK(viewport->scene().grid.has_value());
    CHECK(
        vectorEquals(
            viewport->scene().grid->u_axis,
            1.0, 0.0, 0.0));
    CHECK(
        vectorEquals(
            viewport->scene().grid->v_axis,
            0.0, 1.0, 0.0));

    undo_button->click();
    QApplication::processEvents();
    CHECK(session->document().sketches().size() == 1U);
    CHECK(finish_button->isHidden());

    const auto unit_revision_before =
        session->document().revision();
    const auto unit_undo_before =
        session->undoDepth();
    length_unit_combo->setCurrentIndex(3);
    QApplication::processEvents();
    CHECK(
        session->document().lengthUnit() ==
        core::LengthUnit::inch);
    CHECK(
        session->document().revision() !=
        unit_revision_before);
    CHECK(
        session->undoDepth() ==
        unit_undo_before + 1U);
    undo_button->click();
    QApplication::processEvents();
    CHECK(
        session->document().lengthUnit() ==
        core::LengthUnit::millimetre);
    CHECK(
        length_unit_combo->currentText() ==
        QStringLiteral("mm"));

    CHECK(session->save().ok());
    CHECK(!session->needsSave());

    workbench.deactivateDocument();

    CHECK(
        opened.session->closeDocument(
            document_id));

    auto reopened =
        opened.session->openDocument(
            document_id);
    CHECK(reopened.ok());
    CHECK(
        reopened.session->document()
            .sketches()
            .size() == 1U);
    CHECK(
        reopened.session->document()
            .sketches()
            .front()
            .id ==
        sketch_id);
    CHECK(
        reopened.session->document()
            .sketches()
            .front()
            .support
            .builtin_plane ==
        core::BuiltinReferenceRole::xz_plane);

    return EXIT_SUCCESS;
}
