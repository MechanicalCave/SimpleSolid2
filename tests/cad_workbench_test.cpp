#include "cad_workbench.hpp"

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QStackedWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QWidget>

#include <cstdlib>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 CAD Workbench CHECK failed at line "
                  << line << ": " << expression << '\n';
        // The self-hosted Windows runner is interactive. MSVC Debug
        // abort() opens a modal CRT dialog and turns a normal assertion
        // failure into a blocking desktop prompt. Exit with failure instead
        // so CTest reports the CHECK immediately without user intervention.
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

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
            viewer::cameraForStandardView(state_, view);
        if (!next) return false;
        state_ = *next;
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
        if (fail_sketch_scene_) return false;
        if (!scene.valid()) return false;
        sketch_scene_ = scene;
        return true;
    }

    void setFailSketchScene(bool fail) noexcept {
        fail_sketch_scene_ = fail;
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        if (!scene.valid()) return false;
        sketch_preview_scene_ = scene;
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
        return viewer::SketchPointQueryResult{
            point.valid(),
            std::nullopt};
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

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler_ = std::move(handler);
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
        viewer::SelectionIntentMode mode) {
        if (selection_handler_) {
            selection_handler_(
                viewer::SelectionIntent{token, mode});
        }
    }

    [[nodiscard]] int fitAllCount() const noexcept {
        return fit_all_count_;
    }

    [[nodiscard]] const viewer::ReferenceScene& scene() const noexcept {
        return scene_;
    }

    [[nodiscard]] const viewer::PresentationSelection&
    selection() const noexcept {
        return selection_;
    }

private:
    viewer::CameraState state_;
    viewer::ReferenceScene scene_;
    viewer::SketchScene sketch_scene_;
    viewer::SketchPreviewScene sketch_preview_scene_;
    viewer::PresentationSelection selection_;
    viewer::SpatialPointerHandler spatial_pointer_handler_;
    viewer::PrimaryPointerRouting routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
    viewer::SelectionIntentHandler selection_handler_;
    viewer::NavigationCubeActionHandler
        navigation_cube_handler_;
    int fit_all_count_{};
    bool fail_sketch_scene_{};
};

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_cad_workbench_" +
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

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace = temp.path / "Project";
    std::filesystem::create_directories(workspace);

    application::ProjectWorkspaceMetadataService metadata;
    CHECK(metadata.initialize(workspace, "Machine").ok());

    auto opened = application::ProjectSession::open(workspace);
    CHECK(opened.ok());

    auto first = opened.session->createPart("Part001.ss2part");
    CHECK(first.ok());
    const auto first_id = first.session->documentId();

    core::DocumentProperties first_properties;
    first_properties.title = "Drive Shaft";
    CHECK(first.session
              ->execute(
                  application::SetDocumentPropertiesCommand{
                      first_properties})
              .changed);
    CHECK(first.session->save().ok());

    auto second = opened.session->createPart("Part002.ss2part");
    CHECK(second.ok());
    const auto second_id = second.session->documentId();

    core::DocumentProperties second_properties;
    second_properties.title = "Housing";
    CHECK(second.session
              ->execute(
                  application::SetDocumentPropertiesCommand{
                      second_properties})
              .changed);
    CHECK(second.session->save().ok());

    CHECK(opened.session->closeDocument(first_id));
    CHECK(opened.session->closeDocument(second_id));

    auto reopened_first =
        opened.session->openDocument(first_id);
    auto reopened_second =
        opened.session->openDocument(second_id);
    CHECK(reopened_first.ok());
    CHECK(reopened_second.ok());
    CHECK(opened.session->openDocumentIds().size() == 2U);

    TestViewportWidget* viewport = nullptr;
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            viewport =
                new TestViewportWidget{parent};
            return ui::ViewportSurface{
                viewport,
                viewport};
        }};
    CHECK(viewport != nullptr);

    std::optional<core::DocumentId>
        close_requested;
    workbench.setCloseDocumentHandler(
        [&close_requested](
            const core::DocumentId& id) {
            close_requested = id;
        });

    auto* tree =
        workbench.findChild<QTreeWidget*>(
            QStringLiteral("documentTree"));
    auto* editor =
        workbench.findChild<QWidget*>(
            QStringLiteral("editorSurface"));
    auto* editor_host =
        workbench.findChild<QWidget*>(
            QStringLiteral("editorSurfaceHost"));
    auto* document_top_row =
        workbench.findChild<QWidget*>(
            QStringLiteral("documentTopRow"));
    auto* tree_panel =
        workbench.findChild<QWidget*>(
            QStringLiteral("documentTreePanel"));
    auto* right_panel =
        workbench.findChild<QWidget*>(
            QStringLiteral("workbenchRightPanel"));
    auto* splitter =
        workbench.findChild<QSplitter*>(
            QStringLiteral("workbenchSplitter"));
    auto* tree_recovery =
        workbench.findChild<QToolButton*>(
            QStringLiteral("workbenchTreeRecoveryButton"));
    auto* right_recovery =
        workbench.findChild<QToolButton*>(
            QStringLiteral("workbenchRightPanelRecoveryButton"));
    auto* sketch_tool =
        workbench.findChild<QPushButton*>(
            QStringLiteral("sketchToolButton"));
    auto* operations =
        workbench.findChild<QLabel*>(
            QStringLiteral("operationsPlaceholder"));
    auto* title =
        workbench.findChild<QLineEdit*>(
            QStringLiteral("documentTitleEdit"));
    auto* apply =
        workbench.findChild<QPushButton*>(
            QStringLiteral("applyDocumentPropertiesButton"));
    auto* undo =
        workbench.findChild<QPushButton*>(
            QStringLiteral("undoDocumentButton"));
    auto* redo =
        workbench.findChild<QPushButton*>(
            QStringLiteral("redoDocumentButton"));
    auto* save =
        workbench.findChild<QPushButton*>(
            QStringLiteral("saveDocumentButton"));
    auto* close_document =
        workbench.findChild<QPushButton*>(
            QStringLiteral("closeDocumentButton"));
    auto* show_references =
        workbench.findChild<QAction*>(
            QStringLiteral("showBuiltinReferencesAction"));
    auto* hide_references =
        workbench.findChild<QAction*>(
            QStringLiteral("hideBuiltinReferencesAction"));
    auto* change_sketch_support =
        workbench.findChild<QAction*>(
            QStringLiteral("changeSketchSupportAction"));
    auto* properties_stack =
        workbench.findChild<QStackedWidget*>(
            QStringLiteral("propertiesContextStack"));
    auto* reference_name =
        workbench.findChild<QLabel*>(
            QStringLiteral("referencePropertyName"));
    auto* reference_visibility =
        workbench.findChild<QLabel*>(
            QStringLiteral("referencePropertyVisibility"));
    auto* status =
        workbench.findChild<QLabel*>(
            QStringLiteral("workbenchStatus"));

    CHECK(tree != nullptr);
    CHECK(editor != nullptr);
    CHECK(editor_host != nullptr);
    CHECK(document_top_row != nullptr);
    CHECK(tree_panel != nullptr);
    CHECK(right_panel != nullptr);
    CHECK(splitter != nullptr);
    CHECK(tree_recovery != nullptr);
    CHECK(right_recovery != nullptr);
    CHECK(sketch_tool != nullptr);
    CHECK(operations != nullptr);
    CHECK(title != nullptr);
    CHECK(apply != nullptr);
    CHECK(undo != nullptr);
    CHECK(redo != nullptr);
    CHECK(save != nullptr);
    CHECK(change_sketch_support != nullptr);
    CHECK(close_document != nullptr);
    CHECK(show_references != nullptr);
    CHECK(hide_references != nullptr);
    CHECK(properties_stack != nullptr);
    CHECK(reference_name != nullptr);
    CHECK(reference_visibility != nullptr);
    CHECK(status != nullptr);

    workbench.resize(1400, 800);
    workbench.show();
    QApplication::processEvents();

    CHECK(document_top_row->isAncestorOf(sketch_tool));
    CHECK(document_top_row->isAncestorOf(undo));

    const auto sketch_top =
        sketch_tool->mapTo(
            document_top_row,
            QPoint{0, 0}).y();
    const auto undo_top =
        undo->mapTo(
            document_top_row,
            QPoint{0, 0}).y();
    CHECK(std::abs(sketch_top - undo_top) <= 4);

    CHECK(tree_panel->width() >= 110);
    CHECK(tree_panel->width() <= 210);
    CHECK(right_panel->width() >= 210);
    CHECK(right_panel->width() <= 340);
    CHECK(editor_host->width() > tree_panel->width());
    CHECK(editor_host->width() > right_panel->width());

    // UI-01 G2 native baseline characterization (expected RED until the
    // accepted Properties / Operations right-hand layout is implemented).
    // Must be two independently vertically scrollable panes with a
    // user-adjustable vertical splitter, not one giant clipped GroupBox.
    auto* right_vertical_splitter =
        workbench.findChild<QSplitter*>(
            QStringLiteral("workbenchRightVerticalSplitter"));
    CHECK(right_vertical_splitter != nullptr);
    CHECK(right_vertical_splitter->orientation() == Qt::Vertical);
    auto* properties_scroll =
        workbench.findChild<QScrollArea*>(
            QStringLiteral("workbenchPropertiesScroll"));
    auto* operations_scroll =
        workbench.findChild<QScrollArea*>(
            QStringLiteral("workbenchOperationsScroll"));
    CHECK(properties_scroll != nullptr);
    CHECK(operations_scroll != nullptr);
    CHECK(properties_scroll != operations_scroll);
    CHECK(properties_scroll->verticalScrollBarPolicy() ==
          Qt::ScrollBarAlwaysOn);
    CHECK(operations_scroll->verticalScrollBarPolicy() ==
          Qt::ScrollBarAlwaysOn);
    const auto panel_heights = right_vertical_splitter->sizes();
    CHECK(panel_heights.size() == 2);
    CHECK(panel_heights[0] > 0 && panel_heights[1] > 0);
    const double initial_properties_fraction =
        static_cast<double>(panel_heights[0]) /
        static_cast<double>(panel_heights[0] + panel_heights[1]);
    CHECK(std::abs(initial_properties_fraction - 0.4) < 0.12);
    std::cout << "UI01_G2_RIGHT_PANELS_NATIVE_LAYOUT_PASS" << std::endl;

    // G2 independent vertical scroll under controlled long-form pressure:
    // grow the real existing form hosts (test-only UI sizing), never author
    // Part properties, edit a Feature, or synthesize a CAD command.
    auto* properties_form = properties_scroll->widget();
    auto* operations_form = operations_scroll->widget();
    CHECK(properties_form && operations_form);
    auto* properties_bar = properties_scroll->verticalScrollBar();
    auto* operations_bar = operations_scroll->verticalScrollBar();
    CHECK(properties_bar && operations_bar);
    const int original_properties_min = properties_form->minimumHeight();
    const int original_operations_min = operations_form->minimumHeight();
    properties_form->setMinimumHeight(
        properties_scroll->viewport()->height() + 500);
    operations_form->setMinimumHeight(
        operations_scroll->viewport()->height() + 500);
    QApplication::processEvents();
    CHECK(properties_bar->maximum() > 0);
    CHECK(operations_bar->maximum() > 0);
    properties_bar->setValue(0);
    operations_bar->setValue(0);
    properties_bar->setValue(properties_bar->maximum());
    CHECK(properties_bar->value() == properties_bar->maximum());
    CHECK(operations_bar->value() == 0);
    operations_bar->setValue(operations_bar->maximum());
    CHECK(operations_bar->value() == operations_bar->maximum());
    CHECK(properties_bar->value() == properties_bar->maximum());
    properties_form->setMinimumHeight(original_properties_min);
    operations_form->setMinimumHeight(original_operations_min);
    properties_bar->setValue(0);
    operations_bar->setValue(0);
    QApplication::processEvents();
    std::cout << "UI01_G2_INDEPENDENT_SCROLL_NATIVE_PASS" << std::endl;

    // G2 second characterization: width alone is never permission to
    // collapse Properties/Operations; insufficient *height* is.
    auto* short_tabs = workbench.findChild<QTabWidget*>(
        QStringLiteral("workbenchRightShortTabs"));
    CHECK(short_tabs != nullptr);
    CHECK(short_tabs->isHidden());
    CHECK(!right_vertical_splitter->isHidden());
    // User-adjusted ratio must survive switching to short-height tabs and
    // back; even repeated viewport-only reflow must not reset it to 40/60.
    right_vertical_splitter->setSizes({600, 400});
    QApplication::processEvents();
    const auto adjusted_heights = right_vertical_splitter->sizes();
    CHECK(adjusted_heights.size() == 2);
    const double adjusted_fraction =
        static_cast<double>(adjusted_heights[0]) /
        static_cast<double>(
            adjusted_heights[0] + adjusted_heights[1]);
    CHECK(adjusted_fraction > 0.5);
    workbench.resize(1400, 440);
    QApplication::processEvents();
    CHECK(!short_tabs->isHidden());
    CHECK(right_vertical_splitter->isHidden());
    CHECK(short_tabs->count() == 2);
    CHECK(short_tabs->tabText(0) == QStringLiteral("Properties"));
    CHECK(short_tabs->tabText(1) == QStringLiteral("Operations"));
    short_tabs->setCurrentIndex(1);
    QApplication::processEvents();
    CHECK(short_tabs->currentIndex() == 1);
    workbench.resize(1400, 800);
    QApplication::processEvents();
    CHECK(short_tabs->isHidden());
    CHECK(!right_vertical_splitter->isHidden());
    const auto restored_tab_heights = right_vertical_splitter->sizes();
    CHECK(restored_tab_heights.size() == 2);
    CHECK(std::abs(
        static_cast<double>(restored_tab_heights[0]) /
            static_cast<double>(
                restored_tab_heights[0] + restored_tab_heights[1]) -
            adjusted_fraction) < 0.1);
    right_vertical_splitter->setSizes({400, 600});
    QApplication::processEvents();
    std::cout << "UI01_G2_TAB_RATIO_RESTORATION_PASS" << std::endl;

    // G2 font/DPI pressure: effective pane capacity depends on actual
    // QFontMetrics, not only a fixed number of physical window pixels.
    // This exercises 200%-sized Qt fonts, NOT a claim of Windows monitor
    // DPI parity. Reflow must not use viewport width as a proxy.
    auto* shell_surface = workbench.findChild<QWidget*>(
        QStringLiteral("cadWorkbenchShell"));
    CHECK(shell_surface != nullptr);
    workbench.resize(1400, 620);
    QApplication::processEvents();
    CHECK(short_tabs->isHidden());
    const QFont base_font = shell_surface->font();
    QFont large_font = base_font;
    const double original_point_size =
        base_font.pointSizeF() > 0.0
            ? base_font.pointSizeF() : 10.0;
    large_font.setPointSizeF(original_point_size * 2.0);
    shell_surface->setFont(large_font);
    workbench.resize(1400, 619);
    QApplication::processEvents();
    CHECK(!short_tabs->isHidden());
    CHECK(right_vertical_splitter->isHidden());
    shell_surface->setFont(base_font);
    workbench.resize(1400, 800);
    QApplication::processEvents();
    CHECK(short_tabs->isHidden());
    CHECK(!right_vertical_splitter->isHidden());
    std::cout << "UI01_G2_FONT_AWARE_SHORT_HEIGHT_PASS" << std::endl;

    // G2 completion actions must remain anchored below the Operations
    // scroll viewport. Test actual existing button identity, not a copy
    // that might silently invoke different domain behavior.
    auto* operations_footer =
        workbench.findChild<QWidget*>(
            QStringLiteral("workbenchOperationsFooter"));
    CHECK(operations_footer != nullptr);
    auto* extrude_finish =
        workbench.findChild<QPushButton*>(
            QStringLiteral("extrudeFinishButton"));
    auto* extrude_cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral("extrudeCancelButton"));
    CHECK(extrude_finish && extrude_cancel);
    CHECK(operations_footer->isAncestorOf(extrude_finish));
    CHECK(operations_footer->isAncestorOf(extrude_cancel));
    CHECK(!operations_scroll->isAncestorOf(extrude_finish));
    CHECK(!operations_scroll->isAncestorOf(extrude_cancel));

    const auto initial_side_widths = splitter->sizes();
    CHECK(initial_side_widths.size() == 3);
    workbench.resize(1800, 800);
    QApplication::processEvents();
    const auto wider_side_widths = splitter->sizes();
    CHECK(wider_side_widths.size() == 3);
    CHECK(
        wider_side_widths[1] >
        initial_side_widths[1]);
    CHECK(
        std::abs(
            wider_side_widths[0] -
            initial_side_widths[0]) <= 30);
    CHECK(
        std::abs(
            wider_side_widths[2] -
            initial_side_widths[2]) <= 30);

    // Compact keeps all three surfaces but deliberately gives the center
    // Editor priority. Narrow collapses side surfaces and exposes bounded,
    // local recovery controls rather than introducing a docking framework.
    workbench.resize(980, 800);
    QApplication::processEvents();
    CHECK(!tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(tree_recovery->isHidden());
    CHECK(right_recovery->isHidden());
    const auto compact_widths = splitter->sizes();
    CHECK(compact_widths.size() == 3);
    CHECK(compact_widths[0] <= 170);
    CHECK(compact_widths[2] <= 280);
    CHECK(compact_widths[1] > compact_widths[0]);
    CHECK(compact_widths[1] > compact_widths[2]);

    // Breakpoint hysteresis: compact does not enter narrow until the lower
    // threshold is crossed, and narrow does not leave until the distinct
    // upper threshold is crossed.
    workbench.resize(790, 800);
    QApplication::processEvents();
    CHECK(!tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(tree_recovery->isHidden());
    CHECK(right_recovery->isHidden());

    workbench.resize(750, 800);
    QApplication::processEvents();
    CHECK(tree_panel->isHidden());
    CHECK(right_panel->isHidden());
    CHECK(!tree_recovery->isHidden());
    CHECK(!right_recovery->isHidden());

    workbench.resize(790, 800);
    QApplication::processEvents();
    CHECK(tree_panel->isHidden());
    CHECK(right_panel->isHidden());
    CHECK(!tree_recovery->isHidden());
    CHECK(!right_recovery->isHidden());

    workbench.resize(830, 800);
    QApplication::processEvents();
    CHECK(!tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(tree_recovery->isHidden());
    CHECK(right_recovery->isHidden());

    workbench.resize(720, 800);
    QApplication::processEvents();
    CHECK(tree_panel->isHidden());
    CHECK(right_panel->isHidden());
    CHECK(!tree_recovery->isHidden());
    CHECK(!right_recovery->isHidden());
    CHECK(!editor_host->isHidden());
    CHECK(
        tree_recovery->toolTip() ==
        QStringLiteral("Show Document Tree"));
    CHECK(
        right_recovery->toolTip() ==
        QStringLiteral("Show Properties and Operations"));

    tree_recovery->click();
    QApplication::processEvents();
    CHECK(!tree_panel->isHidden());
    CHECK(right_panel->isHidden());
    CHECK(tree_recovery->isChecked());
    CHECK(!right_recovery->isChecked());
    CHECK(
        tree_recovery->toolTip() ==
        QStringLiteral("Hide Document Tree"));
    CHECK(
        right_recovery->toolTip() ==
        QStringLiteral("Show Properties and Operations"));

    right_recovery->click();
    QApplication::processEvents();
    CHECK(tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(!tree_recovery->isChecked());
    CHECK(right_recovery->isChecked());
    CHECK(
        tree_recovery->toolTip() ==
        QStringLiteral("Show Document Tree"));
    CHECK(
        right_recovery->toolTip() ==
        QStringLiteral("Hide Properties and Operations"));

    right_recovery->click();
    QApplication::processEvents();
    CHECK(tree_panel->isHidden());
    CHECK(right_panel->isHidden());

    workbench.resize(1400, 800);
    QApplication::processEvents();
    CHECK(!tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(tree_recovery->isHidden());
    CHECK(right_recovery->isHidden());

    CHECK(operations->text() ==
          QStringLiteral("Part modeling context."));

    CHECK(workbench.activateDocument(
        opened.session->documentSession(first_id),
        workspace));
    CHECK(workbench.activeDocumentId().has_value());
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(title->text() == QStringLiteral("Drive Shaft"));
    CHECK(save->isEnabled());

    auto* responsive_session =
        opened.session->documentSession(first_id);
    CHECK(responsive_session != nullptr);
    const auto responsive_revision =
        responsive_session->document().revision();
    const auto responsive_undo =
        responsive_session->undoDepth();
    const bool responsive_dirty =
        responsive_session->needsSave();

    // G2: a real active operation in a short-height window must bring
    // Operations forward in one step. The existing Cancel Sketch support
    // button remains in the fixed footer; cancelling is non-authoring.
    auto* sketch_support_cancel =
        workbench.findChild<QPushButton*>(
            QStringLiteral("cancelSketchButton"));
    CHECK(sketch_support_cancel != nullptr);
    CHECK(operations_footer->isAncestorOf(sketch_support_cancel));
    workbench.resize(1400, 440);
    QApplication::processEvents();
    CHECK(!short_tabs->isHidden());
    CHECK(short_tabs->currentIndex() == 0);
    CHECK(sketch_tool->isEnabled());
    sketch_tool->click();
    QApplication::processEvents();
    CHECK(short_tabs->currentIndex() == 1);
    CHECK(operations_footer->isVisible());
    CHECK(sketch_support_cancel->isVisible());
    auto* viewport_mode_cue = workbench.findChild<QLabel*>(
        QStringLiteral("workbenchViewportModeCue"));
    auto* operations_mode_cue = workbench.findChild<QLabel*>(
        QStringLiteral("workbenchOperationsModeCue"));
    CHECK(viewport_mode_cue && operations_mode_cue);
    CHECK(!viewport_mode_cue->text().isEmpty());
    CHECK(operations_mode_cue->text() == viewport_mode_cue->text());
    sketch_support_cancel->click();
    QApplication::processEvents();
    CHECK(operations_footer->isHidden());
    CHECK(responsive_session->document().revision() ==
          responsive_revision);
    CHECK(responsive_session->undoDepth() == responsive_undo);
    CHECK(responsive_session->needsSave() == responsive_dirty);
    workbench.resize(1400, 800);
    QApplication::processEvents();
    CHECK(short_tabs->isHidden());
    CHECK(!right_vertical_splitter->isHidden());
    std::cout << "UI01_G2_SHORT_HEIGHT_ACTIVE_OPERATIONS_PASS"
              << std::endl;

    // A focused editor retains keyboard ownership when narrow reflow occurs;
    // the owning right panel remains recovered instead of disappearing.
    title->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();
    CHECK(title->hasFocus());
    workbench.resize(720, 800);
    QApplication::processEvents();
    CHECK(tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(title->hasFocus());
    CHECK(
        responsive_session->document().revision() ==
        responsive_revision);
    CHECK(responsive_session->undoDepth() ==
          responsive_undo);
    CHECK(responsive_session->needsSave() ==
          responsive_dirty);

    workbench.resize(1400, 800);
    QApplication::processEvents();
    CHECK(!tree_panel->isHidden());
    CHECK(!right_panel->isHidden());
    CHECK(
        responsive_session->document().revision() ==
        responsive_revision);
    CHECK(responsive_session->undoDepth() ==
          responsive_undo);
    CHECK(responsive_session->needsSave() ==
          responsive_dirty);

    viewer::CameraState first_camera;
    first_camera.eye = {20.0, -10.0, 15.0};
    first_camera.target = {1.0, 2.0, 3.0};
    first_camera.up = {0.0, 0.0, 1.0};
    first_camera.projection =
        viewer::CameraProjection::perspective;
    first_camera.scale = 42.0;
    CHECK(viewport->setCameraState(first_camera));

    CHECK(workbench.activateDocument(
        opened.session->documentSession(second_id),
        workspace));
    CHECK(*workbench.activeDocumentId() == second_id);
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == viewer::CameraState{});

    viewer::CameraState second_camera;
    second_camera.eye = {-12.0, -18.0, 9.0};
    second_camera.target = {0.0, 0.0, 0.0};
    second_camera.up = {0.0, 0.0, 1.0};
    second_camera.projection =
        viewer::CameraProjection::orthographic;
    second_camera.scale = 88.0;
    CHECK(viewport->setCameraState(second_camera));

    CHECK(workbench.activateDocument(
        opened.session->documentSession(first_id),
        workspace));
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == first_camera);

    CHECK(workbench.activateDocument(
        opened.session->documentSession(second_id),
        workspace));
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == second_camera);

    CHECK(workbench.activateDocument(
        opened.session->documentSession(first_id),
        workspace));
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == first_camera);

    // G2 per-Document proportion is runtime-only and independent of the
    // camera/geometry state already tested above. A user-adjusted split
    // belongs to the active Document for this application session.
    right_vertical_splitter->setSizes({160, 500});
    QApplication::processEvents();
    const auto first_ui_sizes = right_vertical_splitter->sizes();
    CHECK(first_ui_sizes.size() == 2);
    CHECK(first_ui_sizes[0] > 0 && first_ui_sizes[1] > 0);
    const double first_ui_fraction =
        static_cast<double>(first_ui_sizes[0]) /
        static_cast<double>(
            first_ui_sizes[0] + first_ui_sizes[1]);
    CHECK(first_ui_fraction < 0.35);
    const auto first_ui_revision =
        opened.session->documentSession(first_id)
            ->document().revision();
    const auto first_ui_undo =
        opened.session->documentSession(first_id)->undoDepth();
    const auto first_ui_dirty =
        opened.session->documentSession(first_id)->needsSave();
    CHECK(workbench.activateDocument(
        opened.session->documentSession(second_id), workspace));
    QApplication::processEvents();
    const auto second_ui_sizes = right_vertical_splitter->sizes();
    CHECK(second_ui_sizes.size() == 2);
    CHECK(second_ui_sizes[0] > 0 && second_ui_sizes[1] > 0);
    const double second_ui_fraction =
        static_cast<double>(second_ui_sizes[0]) /
        static_cast<double>(
            second_ui_sizes[0] + second_ui_sizes[1]);
    CHECK(std::abs(second_ui_fraction - 0.4) < 0.1);
    CHECK(workbench.activateDocument(
        opened.session->documentSession(first_id), workspace));
    QApplication::processEvents();
    const auto restored_ui_sizes = right_vertical_splitter->sizes();
    CHECK(restored_ui_sizes.size() == 2);
    CHECK(restored_ui_sizes[0] > 0 && restored_ui_sizes[1] > 0);
    const double restored_ui_fraction =
        static_cast<double>(restored_ui_sizes[0]) /
        static_cast<double>(
            restored_ui_sizes[0] + restored_ui_sizes[1]);
    CHECK(std::abs(restored_ui_fraction - first_ui_fraction) < 0.1);
    CHECK(opened.session->documentSession(first_id)
              ->document().revision() == first_ui_revision);
    CHECK(opened.session->documentSession(first_id)->undoDepth() ==
          first_ui_undo);
    CHECK(opened.session->documentSession(first_id)->needsSave() ==
          first_ui_dirty);
    std::cout << "UI01_G2_PER_DOCUMENT_RATIO_NATIVE_PASS" << std::endl;

    auto* first_session_for_view =
        opened.session->documentSession(first_id);
    auto* second_session_for_view =
        opened.session->documentSession(second_id);
    CHECK(first_session_for_view != nullptr);
    CHECK(second_session_for_view != nullptr);
    CHECK(!first_session_for_view->needsSave());
    CHECK(!second_session_for_view->needsSave());

    const auto navigation_revision =
        first_session_for_view->document().revision().value();

    viewport->emitNavigationCubeAction(
        viewer::NavigationCubeAction::orientTo(
            viewer::NavigationCubeTarget::top));
    CHECK(viewport->cameraState().has_value());
    CHECK(viewport->cameraState()->eye.z >
          viewport->cameraState()->target.z);
    CHECK(viewport->cameraState()->projection ==
          viewer::CameraProjection::perspective);
    CHECK(first_session_for_view->document().revision().value() ==
          navigation_revision);
    CHECK(!first_session_for_view->needsSave());

    const auto fit_before = viewport->fitAllCount();
    viewport->emitNavigationCubeAction(
        {viewer::NavigationCubeActionKind::home, {}});
    CHECK(viewport->fitAllCount() == fit_before + 1);
    CHECK(viewport->cameraState()->projection ==
          viewer::CameraProjection::perspective);
    CHECK(first_session_for_view->document().revision().value() ==
          navigation_revision);
    CHECK(!first_session_for_view->needsSave());

    const auto before_projection_toggle =
        *viewport->cameraState();
    viewport->emitNavigationCubeAction(
        {viewer::NavigationCubeActionKind::
             toggle_projection,
         {}});
    CHECK(
        viewport->cameraState()->projection ==
        viewer::CameraProjection::orthographic);
    CHECK(
        viewport->cameraState()->eye ==
        before_projection_toggle.eye);
    CHECK(
        viewport->cameraState()->target ==
        before_projection_toggle.target);
    CHECK(
        viewport->cameraState()->up ==
        before_projection_toggle.up);
    CHECK(first_session_for_view->document().revision().value() ==
          navigation_revision);
    CHECK(!first_session_for_view->needsSave());

    CHECK(properties_stack->currentWidget()->objectName() ==
          QStringLiteral("documentPropertiesPage"));

    CHECK(viewport->scene().valid());
    CHECK(viewport->scene().grid.has_value());
    CHECK(viewport->scene().grid->visible);
    CHECK(viewport->scene().references.size() == 7U);

    auto visibleReference = [&](
        viewer::PresentationToken token) {
        for (const auto& reference : viewport->scene().references) {
            if (reference.token == token) {
                return reference.visible;
            }
        }
        return false;
    };

    const viewer::PresentationToken origin_point_token{0x101U};
    const viewer::PresentationToken xy_plane_token{0x105U};
    CHECK(visibleReference(origin_point_token));
    CHECK(!visibleReference(xy_plane_token));

    CHECK(tree->topLevelItemCount() == 1);
    auto* root = tree->topLevelItem(0);
    CHECK(root != nullptr);
    CHECK(root->text(0) == QStringLiteral("Drive Shaft"));
    CHECK(root->childCount() == 3);
    CHECK(
        root->child(0)->text(0) ==
        QStringLiteral("Origin"));
    CHECK(
        root->child(1)->text(0) ==
        QStringLiteral("Reference Geometry"));

    QTreeWidgetItem* origin = nullptr;
    QTreeWidgetItem* reference_geometry = nullptr;
    QTreeWidgetItem* body = nullptr;
    for (int index = 0;
         index < root->childCount();
         ++index) {
        auto* item = root->child(index);
        if (item == nullptr) continue;
        if (item->text(0).startsWith(
                QStringLiteral("Body ["))) {
            body = item;
        } else if (
            item->text(0) ==
            QStringLiteral("Origin")) {
            origin = item;
        } else if (
            item->text(0) ==
            QStringLiteral("Reference Geometry")) {
            reference_geometry = item;
        }
    }
    CHECK(body != nullptr);
    CHECK(body->text(0) ==
          QStringLiteral("Body [Empty]"));
    CHECK(origin != nullptr);
    CHECK(reference_geometry != nullptr);
    CHECK(reference_geometry->childCount() == 0);
    CHECK(origin->childCount() == 7);
    CHECK(tree->selectionMode() ==
          QAbstractItemView::ExtendedSelection);

    QTreeWidgetItem* xy_plane = nullptr;
    QTreeWidgetItem* origin_point = nullptr;
    for (int index = 0; index < origin->childCount(); ++index) {
        auto* item = origin->child(index);
        if (item->text(0) == QStringLiteral("XY Plane")) {
            xy_plane = item;
        } else if (
            item->text(0) == QStringLiteral("Origin Point")) {
            origin_point = item;
        }
    }

    CHECK(xy_plane != nullptr);
    CHECK(origin_point != nullptr);
    CHECK(xy_plane->font(0).italic());
    CHECK(!origin_point->font(0).italic());

    tree->clearSelection();
    root->setSelected(true);
    xy_plane->setSelected(true);
    CHECK(!hide_references->isEnabled());
    CHECK(!show_references->isEnabled());

    auto* first_session =
        opened.session->documentSession(first_id);
    CHECK(first_session != nullptr);
    CHECK(!first_session->canUndo());
    CHECK(!first_session->needsSave());

    tree->clearSelection();
    xy_plane->setSelected(true);
    origin_point->setSelected(true);
    tree->setCurrentItem(
        origin_point,
        0,
        QItemSelectionModel::NoUpdate);
    QApplication::processEvents();

    CHECK(hide_references->isEnabled());
    CHECK(show_references->isEnabled());
    CHECK(viewport->selection().selected.size() == 2U);
    CHECK(viewport->selection().primary.has_value());
    CHECK(*viewport->selection().primary == origin_point_token);

    viewport->emitSelectionIntent(
        xy_plane_token,
        viewer::SelectionIntentMode::replace);
    QApplication::processEvents();
    CHECK(tree->selectedItems().size() == 1);
    CHECK(tree->selectedItems().front() == xy_plane);
    CHECK(viewport->selection().selected.size() == 1U);
    CHECK(viewport->selection().primary.has_value());
    CHECK(*viewport->selection().primary == xy_plane_token);
    CHECK(properties_stack->currentWidget()->objectName() ==
          QStringLiteral("referencePropertiesPage"));
    CHECK(reference_name->text() ==
          QStringLiteral("XY Plane"));
    CHECK(reference_visibility->text() ==
          QStringLiteral("Hidden"));

    viewport->emitSelectionIntent(
        origin_point_token,
        viewer::SelectionIntentMode::toggle);
    QApplication::processEvents();
    CHECK(tree->selectedItems().size() == 2);
    CHECK(viewport->selection().selected.size() == 2U);
    CHECK(viewport->selection().primary.has_value());
    CHECK(*viewport->selection().primary == origin_point_token);

    viewport->emitSelectionIntent(
        {},
        viewer::SelectionIntentMode::clear);
    QApplication::processEvents();
    CHECK(tree->selectedItems().empty());
    CHECK(viewport->selection().selected.empty());
    CHECK(!viewport->selection().primary.has_value());

    viewport->emitSelectionIntent(
        xy_plane_token,
        viewer::SelectionIntentMode::replace);
    viewport->emitSelectionIntent(
        origin_point_token,
        viewer::SelectionIntentMode::toggle);
    QApplication::processEvents();
    CHECK(tree->selectedItems().size() == 2);
    CHECK(viewport->selection().selected.size() == 2U);
    CHECK(viewport->selection().primary.has_value());
    CHECK(*viewport->selection().primary == origin_point_token);
    CHECK(properties_stack->currentWidget()->objectName() ==
          QStringLiteral("referencePropertiesPage"));
    CHECK(reference_name->text() ==
          QStringLiteral("Origin Point"));
    CHECK(reference_visibility->text() ==
          QStringLiteral("Shown"));

    const auto before_visibility_revision =
        first_session->document().revision().value();
    hide_references->trigger();

    CHECK(first_session->document().revision().value() ==
          before_visibility_revision + 1U);
    CHECK(first_session->undoDepth() == 1U);
    CHECK(first_session->needsSave());
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(!visibleReference(origin_point_token));
    CHECK(!visibleReference(xy_plane_token));
    CHECK(reference_visibility->text() ==
          QStringLiteral("Hidden"));
    CHECK(undo->isEnabled());
    CHECK(save->isEnabled());

    undo->click();
    CHECK(first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(visibleReference(origin_point_token));
    CHECK(!visibleReference(xy_plane_token));
    CHECK(reference_visibility->text() ==
          QStringLiteral("Shown"));
    CHECK(!first_session->needsSave());

    tree->clearSelection();
    QApplication::processEvents();
    CHECK(properties_stack->currentWidget()->objectName() ==
          QStringLiteral("documentPropertiesPage"));

    const auto before_failed_presentation_revision =
        first_session->document().revision();
    const auto before_failed_presentation_undo =
        first_session->undoDepth();

    viewport->setFailSketchScene(true);
    title->setText(QStringLiteral("Drive Shaft Rev"));
    apply->click();

    CHECK(first_session->needsSave());
    CHECK(first_session->document().properties().title ==
          "Drive Shaft Rev");
    CHECK(
        first_session->document().revision() ==
        *before_failed_presentation_revision.next());
    CHECK(
        first_session->undoDepth() ==
        before_failed_presentation_undo + 1U);
    CHECK(undo->isEnabled());
    CHECK(save->isEnabled());
    CHECK(
        status->text().contains(
            QStringLiteral(
                "3D presentation update failed")));

    const auto committed_revision =
        first_session->document().revision();
    const auto committed_undo =
        first_session->undoDepth();
    const auto committed_dirty =
        first_session->needsSave();

    // The next normal full refresh retries from authored state and clears
    // the runtime-only degraded presentation diagnostic.
    viewport->setFailSketchScene(false);
    CHECK(workbench.activateDocument(
        first_session,
        workspace));
    CHECK(
        !status->text().contains(
            QStringLiteral(
                "3D presentation update failed")));
    CHECK(
        first_session->document().revision() ==
        committed_revision);
    CHECK(first_session->undoDepth() ==
          committed_undo);
    CHECK(first_session->needsSave() ==
          committed_dirty);

    undo->click();
    CHECK(first_session->document().properties().title ==
          "Drive Shaft");
    CHECK(!first_session->needsSave());
    CHECK(redo->isEnabled());
    CHECK(save->isEnabled());

    redo->click();
    CHECK(first_session->document().properties().title ==
          "Drive Shaft Rev");
    CHECK(first_session->needsSave());

    save->click();
    CHECK(!first_session->needsSave());
    CHECK(save->isEnabled());

    CHECK(workbench.activateDocument(
        opened.session->documentSession(second_id),
        workspace));
    CHECK(*workbench.activeDocumentId() == second_id);
    CHECK(title->text() == QStringLiteral("Housing"));
    CHECK(tree->topLevelItem(0)->text(0) ==
          QStringLiteral("Housing"));

    CHECK(workbench.activateDocument(
        opened.session->documentSession(first_id),
        workspace));
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(title->text() == QStringLiteral("Drive Shaft Rev"));

    // WS-01 boundary: Part Workbench requests close by stable
    // DocumentId but does not erase Project-owned DocumentSessions.
    close_document->click();
    QApplication::processEvents();
    CHECK(close_requested.has_value());
    CHECK(*close_requested == first_id);
    CHECK(
        opened.session->documentSession(first_id) !=
        nullptr);
    CHECK(workbench.activeDocumentId().has_value());
    CHECK(*workbench.activeDocumentId() == first_id);

    const auto first_revision_before_workspace =
        first_session->document().revision();
    const bool first_dirty_before_workspace =
        first_session->needsSave();

    workbench.deactivateDocument();
    CHECK(!workbench.activeDocumentId().has_value());
    CHECK(!title->isEnabled());
    CHECK(
        opened.session->documentSession(first_id) !=
        nullptr);
    CHECK(
        first_session->document().revision() ==
        first_revision_before_workspace);
    CHECK(
        first_session->needsSave() ==
        first_dirty_before_workspace);

    CHECK(workbench.activateDocument(
        opened.session->documentSession(second_id),
        workspace));
    CHECK(*workbench.activeDocumentId() == second_id);
    CHECK(title->text() == QStringLiteral("Housing"));

    workbench.deactivateDocument();
    CHECK(!workbench.activeDocumentId().has_value());
    CHECK(!title->isEnabled());

    return EXIT_SUCCESS;
}
