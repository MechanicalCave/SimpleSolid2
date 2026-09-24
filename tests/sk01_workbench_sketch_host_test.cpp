#include "cad_workbench.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <QAction>
#include <QApplication>
#include <QLabel>
#include <QPushButton>
#include <QTreeWidget>
#include <QWidget>
#include <QTest>

#include <cstdlib>
#include <filesystem>
#include <iostream>
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
        selection_handler_ =
            std::move(handler);
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
    viewer::PresentationSelection selection_;
    viewer::SelectionIntentHandler
        selection_handler_;
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
    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            viewport =
                new TestViewportWidget{parent};
            return ui::ViewportSurface{
                viewport,
                viewport};
        }};

    workbench.setProjectSession(
        &*opened.session);
    CHECK(
        workbench.activateOpenDocument(
            document_id));
    workbench.show();
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
    auto* undo_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("undoDocumentButton"));
    auto* redo_button =
        workbench.findChild<QPushButton*>(
            QStringLiteral("redoDocumentButton"));
    auto* tree =
        workbench.findChild<QTreeWidget*>(
            QStringLiteral("documentTree"));
    auto* top_action =
        workbench.findChild<QAction*>(
            QStringLiteral("viewCubeTopAction"));
    auto* edit_sketch_action =
        workbench.findChild<QAction*>(
            QStringLiteral("editSketchAction"));
    auto* operations_content =
        workbench.findChild<QWidget*>(
            QStringLiteral("partOperationsContent"));
    auto* editor_host =
        workbench.findChild<QWidget*>(
            QStringLiteral("editorSurfaceHost"));
    auto* operations_label =
        workbench.findChild<QLabel*>(
            QStringLiteral("operationsPlaceholder"));

    CHECK(sketch_button != nullptr);
    CHECK(cancel_button != nullptr);
    CHECK(finish_button != nullptr);
    CHECK(undo_button != nullptr);
    CHECK(redo_button != nullptr);
    CHECK(tree != nullptr);
    CHECK(top_action != nullptr);
    CHECK(edit_sketch_action != nullptr);
    CHECK(operations_content != nullptr);
    CHECK(editor_host != nullptr);
    CHECK(operations_label != nullptr);
    CHECK(editor_host->isAncestorOf(sketch_button));
    CHECK(!operations_content->isAncestorOf(sketch_button));
    CHECK(sketch_button->isEnabled());
    CHECK(cancel_button->isHidden());
    CHECK(finish_button->isHidden());
    CHECK(
        operations_label->text() ==
        QStringLiteral("No active tool."));

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

    top_action->trigger();
    QApplication::processEvents();

    CHECK(
        session->document().state() ==
        authored_after_create);
    CHECK(
        session->document().revision() ==
        revision_after_create);
    CHECK(session->needsSave());

    finish_button->click();
    QApplication::processEvents();

    CHECK(finish_button->isHidden());
    CHECK(cancel_button->isHidden());
    CHECK(sketch_button->isEnabled());
    CHECK(
        operations_label->text() ==
        QStringLiteral("No active tool."));
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

    CHECK(session->save().ok());
    CHECK(!session->needsSave());

    workbench.clearProjectSession();

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
