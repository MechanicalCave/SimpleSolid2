#include "cad_workbench.hpp"
#include "part_viewer_projection.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QAbstractItemView>
#include <QAction>
#include <QApplication>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabBar>
#include <QTreeWidget>
#include <QWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

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
    explicit TestViewportWidget(
        QWidget* parent = nullptr)
        : QWidget{parent} {}

    [[nodiscard]] std::optional<
        viewer::CameraState>
    cameraState() const override {
        return state_;
    }

    bool setCameraState(
        const viewer::CameraState& state) override {
        if (!viewer::validateCameraState(state).valid) {
            return false;
        }
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
        for (const auto& item : scene.references) {
            if (!item.valid()) return false;
        }
        scene_ = scene;
        ++scene_set_count_;
        return true;
    }

    bool setReferenceGrid(
        const viewer::ReferenceGridPresentation& grid)
        override {
        if (!grid.valid()) return false;
        grid_ = grid;
        ++grid_set_count_;
        return true;
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler)
        override {
        selection_handler_ =
            std::move(handler);
    }

    void emitSelectionIntent(
        std::optional<viewer::PresentationToken> token,
        viewer::SelectionIntentMode mode =
            viewer::SelectionIntentMode::replace) {
        if (selection_handler_) {
            selection_handler_(
                viewer::SelectionIntent{
                    token,
                    mode});
        }
    }

    [[nodiscard]] const viewer::ReferenceScene&
    referenceScene() const noexcept {
        return scene_;
    }

    [[nodiscard]] const
    viewer::ReferenceGridPresentation&
    referenceGrid() const noexcept {
        return grid_;
    }

    [[nodiscard]] int fitAllCount() const noexcept {
        return fit_all_count_;
    }

    [[nodiscard]] int sceneSetCount() const noexcept {
        return scene_set_count_;
    }

    [[nodiscard]] int gridSetCount() const noexcept {
        return grid_set_count_;
    }

private:
    viewer::CameraState state_;
    viewer::ReferenceScene scene_;
    viewer::ReferenceGridPresentation grid_;
    viewer::SelectionIntentHandler selection_handler_;
    int fit_all_count_{};
    int scene_set_count_{};
    int grid_set_count_{};
};

const viewer::ReferencePresentation* findReference(
    const viewer::ReferenceScene& scene,
    viewer::PresentationToken token) {
    for (const auto& item : scene.references) {
        if (item.token == token) {
            return &item;
        }
    }
    return nullptr;
}


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
    auto* show_references =
        workbench.findChild<QAction*>(
            QStringLiteral("showBuiltinReferencesAction"));
    auto* hide_references =
        workbench.findChild<QAction*>(
            QStringLiteral("hideBuiltinReferencesAction"));
    auto* view_top =
        workbench.findChild<QAction*>(
            QStringLiteral("viewTopAction"));
    auto* view_isometric =
        workbench.findChild<QAction*>(
            QStringLiteral("viewIsometricAction"));
    auto* fit_all =
        workbench.findChild<QPushButton*>(
            QStringLiteral("fitAllButton"));
    auto* projection =
        workbench.findChild<QPushButton*>(
            QStringLiteral("projectionButton"));
    auto* selection_context =
        workbench.findChild<QLabel*>(
            QStringLiteral("selectionContext"));

    CHECK(tabs != nullptr);
    CHECK(tree != nullptr);
    CHECK(editor != nullptr);
    CHECK(operations != nullptr);
    CHECK(title != nullptr);
    CHECK(apply != nullptr);
    CHECK(undo != nullptr);
    CHECK(redo != nullptr);
    CHECK(save != nullptr);
    CHECK(show_references != nullptr);
    CHECK(hide_references != nullptr);
    CHECK(view_top != nullptr);
    CHECK(view_isometric != nullptr);
    CHECK(fit_all != nullptr);
    CHECK(projection != nullptr);
    CHECK(selection_context != nullptr);

    CHECK(tabs->count() == 2);
    CHECK(operations->text().contains(
        QStringLiteral("No modeling operations")));

    CHECK(workbench.activateDocument(first_id));
    CHECK(workbench.activeDocumentId().has_value());
    CHECK(*workbench.activeDocumentId() == first_id);
    CHECK(tabs->count() == 2);
    CHECK(title->text() == QStringLiteral("Drive Shaft"));
    CHECK(selection_context->text().contains(
        QStringLiteral("Document")));
    CHECK(viewport->referenceScene().references.size() == 7U);
    CHECK(viewport->referenceGrid().valid());
    CHECK(viewport->referenceGrid().visible);

    const auto x_axis_token =
        ui::presentationTokenFor(
            core::BuiltinReferenceRole::x_axis);
    const auto xy_plane_token =
        ui::presentationTokenFor(
            core::BuiltinReferenceRole::xy_plane);
    const auto origin_point_token =
        ui::presentationTokenFor(
            core::BuiltinReferenceRole::origin_point);

    const auto* initial_x_axis =
        findReference(
            viewport->referenceScene(),
            x_axis_token);
    const auto* initial_xy_plane =
        findReference(
            viewport->referenceScene(),
            xy_plane_token);
    CHECK(initial_x_axis != nullptr);
    CHECK(initial_xy_plane != nullptr);
    CHECK(initial_x_axis->visible);
    CHECK(!initial_xy_plane->visible);

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

    const auto view_revision_before =
        opened.session->documentSession(first_id)
            ->document().revision().value();

    view_top->trigger();
    CHECK(viewport->cameraState().has_value());
    CHECK(viewport->cameraState()->eye.z >
          viewport->cameraState()->target.z);

    view_isometric->trigger();
    CHECK(viewport->cameraState().has_value());

    const auto fit_before =
        viewport->fitAllCount();
    fit_all->click();
    CHECK(viewport->fitAllCount() ==
          fit_before + 1);

    const auto projection_before =
        viewport->cameraState()->projection;
    projection->click();
    CHECK(viewport->cameraState()->projection !=
          projection_before);

    CHECK(opened.session->documentSession(first_id)
              ->document().revision().value() ==
          view_revision_before);
    CHECK(!opened.session->documentSession(first_id)
               ->needsSave());

    auto* first_session_for_view =
        opened.session->documentSession(first_id);
    auto* second_session_for_view =
        opened.session->documentSession(second_id);
    CHECK(first_session_for_view != nullptr);
    CHECK(second_session_for_view != nullptr);
    CHECK(!first_session_for_view->needsSave());
    CHECK(!second_session_for_view->needsSave());

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
    QTreeWidgetItem* x_axis = nullptr;
    QTreeWidgetItem* origin_point = nullptr;
    for (int index = 0; index < origin->childCount(); ++index) {
        auto* item = origin->child(index);
        if (item->text(0) == QStringLiteral("XY Plane")) {
            xy_plane = item;
        } else if (
            item->text(0) == QStringLiteral("X Axis")) {
            x_axis = item;
        } else if (
            item->text(0) == QStringLiteral("Origin Point")) {
            origin_point = item;
        }
    }

    CHECK(xy_plane != nullptr);
    CHECK(x_axis != nullptr);
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

    const auto selection_revision =
        first_session->document().revision().value();

    tree->clearSelection();
    x_axis->setSelected(true);
    tree->setCurrentItem(
        x_axis,
        0,
        QItemSelectionModel::NoUpdate);
    QApplication::processEvents();

    CHECK(selection_context->text().contains(
        QStringLiteral("X Axis")));
    CHECK(!title->isEnabled());
    CHECK(!apply->isEnabled());

    const auto* tree_selected_x =
        findReference(
            viewport->referenceScene(),
            x_axis_token);
    CHECK(tree_selected_x != nullptr);
    CHECK(tree_selected_x->role ==
          viewer::PresentationRole::primary_selection);

    viewport->emitSelectionIntent(
        origin_point_token);
    QApplication::processEvents();

    CHECK(tree->selectedItems().size() == 1);
    CHECK(tree->currentItem() != nullptr);
    CHECK(tree->currentItem()->text(0) ==
          QStringLiteral("Origin Point"));
    CHECK(selection_context->text().contains(
        QStringLiteral("Origin Point")));

    viewport->emitSelectionIntent(
        x_axis_token,
        viewer::SelectionIntentMode::toggle);
    QApplication::processEvents();

    CHECK(tree->selectedItems().size() == 2);
    CHECK(tree->currentItem() != nullptr);
    CHECK(tree->currentItem()->text(0) ==
          QStringLiteral("X Axis"));

    const auto* viewport_primary_x =
        findReference(
            viewport->referenceScene(),
            x_axis_token);
    const auto* viewport_secondary_origin =
        findReference(
            viewport->referenceScene(),
            origin_point_token);
    CHECK(viewport_primary_x != nullptr);
    CHECK(viewport_secondary_origin != nullptr);
    CHECK(viewport_primary_x->role ==
          viewer::PresentationRole::primary_selection);
    CHECK(viewport_secondary_origin->role ==
          viewer::PresentationRole::secondary_selection);

    viewport->emitSelectionIntent(std::nullopt);
    QApplication::processEvents();

    CHECK(tree->selectedItems().size() == 1);
    CHECK(tree->currentItem() == root);
    CHECK(title->isEnabled());
    CHECK(apply->isEnabled());
    CHECK(first_session->document().revision().value() ==
          selection_revision);
    CHECK(!first_session->needsSave());

    tree->clearSelection();
    xy_plane->setSelected(true);
    origin_point->setSelected(true);
    CHECK(hide_references->isEnabled());
    CHECK(show_references->isEnabled());

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

    const auto* hidden_origin =
        findReference(
            viewport->referenceScene(),
            origin_point_token);
    CHECK(hidden_origin != nullptr);
    CHECK(!hidden_origin->visible);

    CHECK(undo->isEnabled());
    CHECK(save->isEnabled());

    undo->click();
    CHECK(first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!first_session->document().builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(!first_session->needsSave());

    const auto* restored_origin =
        findReference(
            viewport->referenceScene(),
            origin_point_token);
    CHECK(restored_origin != nullptr);
    CHECK(restored_origin->visible);

    viewport->emitSelectionIntent(std::nullopt);
    QApplication::processEvents();
    CHECK(title->isEnabled());

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

    workbench.clearProjectSession();
    CHECK(!workbench.activeDocumentId().has_value());
    CHECK(tabs->count() == 0);
    CHECK(!title->isEnabled());

    return EXIT_SUCCESS;
}
