#include "cad_workbench.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTabBar>
#include <QTreeWidget>
#include <QWidget>

#include <cstdlib>
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
        std::abort();
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

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        if (!scene.valid()) return false;
        scene_ = scene;
        return true;
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        if (!selection.valid()) return false;
        selection_ = selection;
        return true;
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler_ = std::move(handler);
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
    viewer::PresentationSelection selection_;
    viewer::SelectionIntentHandler selection_handler_;
    int fit_all_count_{};
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
    workbench.setProjectSession(&*opened.session);
    CHECK(viewport != nullptr);

    auto* tabs =
        workbench.findChild<QTabBar*>(
            QStringLiteral("documentTabs"));
    auto* tree =
        workbench.findChild<QTreeWidget*>(
            QStringLiteral("documentTree"));
    auto* editor =
        workbench.findChild<QWidget*>(
            QStringLiteral("editorSurface"));
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
    auto* projection_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("viewCubeProjectionButton"));
    auto* fit_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("viewCubeFitButton"));
    auto* properties_stack =
        workbench.findChild<QStackedWidget*>(
            QStringLiteral("propertiesContextStack"));
    auto* reference_name =
        workbench.findChild<QLabel*>(
            QStringLiteral("referencePropertyName"));
    auto* reference_visibility =
        workbench.findChild<QLabel*>(
            QStringLiteral("referencePropertyVisibility"));

    auto* top_view_action =
        workbench.findChild<QAction*>(
            QStringLiteral("viewCubeTopAction"));

    CHECK(tabs != nullptr);
    CHECK(tree != nullptr);
    CHECK(editor != nullptr);
    CHECK(operations != nullptr);
    CHECK(title != nullptr);
    CHECK(apply != nullptr);
    CHECK(undo != nullptr);
    CHECK(redo != nullptr);
    CHECK(save != nullptr);
    CHECK(close_document != nullptr);
    CHECK(show_references != nullptr);
    CHECK(hide_references != nullptr);
    CHECK(projection_button != nullptr);
    CHECK(fit_button != nullptr);
    CHECK(properties_stack != nullptr);
    CHECK(reference_name != nullptr);
    CHECK(reference_visibility != nullptr);
    CHECK(top_view_action != nullptr);

    CHECK(tabs->count() == 2);
    CHECK(operations->text() ==
          QStringLiteral("No active tool."));

    CHECK(workbench.activateDocument(first_id));
    CHECK(workbench.activeDocumentId().has_value());
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(tabs->count() == 2);
    CHECK(title->text() == QStringLiteral("Drive Shaft"));

    viewer::CameraState first_camera;
    first_camera.eye = {20.0, -10.0, 15.0};
    first_camera.target = {1.0, 2.0, 3.0};
    first_camera.up = {0.0, 0.0, 1.0};
    first_camera.projection =
        viewer::CameraProjection::perspective;
    first_camera.scale = 42.0;
    CHECK(viewport->setCameraState(first_camera));

    CHECK(workbench.activateDocument(second_id));
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

    CHECK(workbench.activateDocument(first_id));
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == first_camera);

    CHECK(workbench.activateDocument(second_id));
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == second_camera);

    CHECK(workbench.activateDocument(first_id));
    CHECK(viewport->cameraState().has_value());
    CHECK(*viewport->cameraState() == first_camera);

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

    top_view_action->trigger();
    CHECK(viewport->cameraState().has_value());
    CHECK(viewport->cameraState()->eye.z >
          viewport->cameraState()->target.z);
    CHECK(viewport->cameraState()->projection ==
          viewer::CameraProjection::perspective);
    CHECK(first_session_for_view->document().revision().value() ==
          navigation_revision);
    CHECK(!first_session_for_view->needsSave());

    projection_button->click();
    CHECK(viewport->cameraState()->projection ==
          viewer::CameraProjection::orthographic);
    CHECK(first_session_for_view->document().revision().value() ==
          navigation_revision);
    CHECK(!first_session_for_view->needsSave());

    const auto fit_before = viewport->fitAllCount();
    fit_button->click();
    CHECK(viewport->fitAllCount() == fit_before + 1);
    CHECK(first_session_for_view->document().revision().value() ==
          navigation_revision);
    CHECK(!first_session_for_view->needsSave());

    projection_button->click();
    CHECK(viewport->cameraState()->projection ==
          viewer::CameraProjection::perspective);

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
    CHECK(root->childCount() == 1);

    auto* origin = root->child(0);
    CHECK(origin != nullptr);
    CHECK(origin->text(0) == QStringLiteral("Origin"));
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

    title->setText(QStringLiteral("Drive Shaft Rev"));
    apply->click();

    CHECK(first_session->needsSave());
    CHECK(first_session->document().properties().title ==
          "Drive Shaft Rev");
    CHECK(undo->isEnabled());
    CHECK(save->isEnabled());

    undo->click();
    CHECK(first_session->document().properties().title ==
          "Drive Shaft");
    CHECK(!first_session->needsSave());
    CHECK(redo->isEnabled());

    redo->click();
    CHECK(first_session->document().properties().title ==
          "Drive Shaft Rev");
    CHECK(first_session->needsSave());

    save->click();
    CHECK(!first_session->needsSave());

    CHECK(workbench.activateDocument(second_id));
    CHECK(*workbench.activeDocumentId() == second_id);
    CHECK(title->text() == QStringLiteral("Housing"));
    CHECK(tree->topLevelItem(0)->text(0) ==
          QStringLiteral("Housing"));

    CHECK(workbench.activateDocument(first_id));
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(tabs->count() == 2);
    CHECK(title->text() == QStringLiteral("Drive Shaft Rev"));

    // SK-01A P0 regression: closing the active Part must detach
    // Workbench/Viewer non-owning pointers before ProjectSession
    // erases the owning DocumentSession.
    close_document->click();
    QApplication::processEvents();
    CHECK(
        opened.session->documentSession(first_id) ==
        nullptr);
    CHECK(workbench.activeDocumentId().has_value());
    CHECK(*workbench.activeDocumentId() == second_id);
    CHECK(tabs->count() == 1);
    CHECK(title->text() == QStringLiteral("Housing"));

    workbench.clearProjectSession();
    CHECK(!workbench.activeDocumentId().has_value());
    CHECK(tabs->count() == 0);
    CHECK(!title->isEnabled());

    return EXIT_SUCCESS;
}
