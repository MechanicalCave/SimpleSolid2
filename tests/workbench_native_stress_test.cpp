#include "cad_workbench.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QAction>
#include <QApplication>
#include <QItemSelectionModel>
#include <QPushButton>
#include <QSplitter>
#include <QToolButton>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_wb01a_native_stress_" +
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

bool expect(
    bool value,
    const char* expression,
    int line) {
    if (value) return true;

    std::cerr
        << "WB-01A native stress check failed at line "
        << line
        << ": "
        << expression
        << '\n';
    return false;
}

#define EXPECT(expr) \
    do { \
        if (!expect(static_cast<bool>(expr), #expr, __LINE__)) { \
            result = EXIT_FAILURE; \
            workbench.close(); \
            app.quit(); \
            return; \
        } \
    } while (false)

QTreeWidgetItem* findOriginReference(
    QTreeWidget& tree,
    const QString& label) {
    if (tree.topLevelItemCount() != 1) {
        return nullptr;
    }

    auto* root = tree.topLevelItem(0);
    if (root == nullptr || root->childCount() != 1) {
        return nullptr;
    }

    auto* origin = root->child(0);
    if (origin == nullptr) return nullptr;

    for (int index = 0;
         index < origin->childCount();
         ++index) {
        auto* item = origin->child(index);
        if (item != nullptr &&
            item->text(0) == label) {
            return item;
        }
    }

    return nullptr;
}

void selectReference(
    QTreeWidget& tree,
    QTreeWidgetItem& item) {
    tree.clearSelection();
    item.setSelected(true);
    tree.setCurrentItem(
        &item,
        0,
        QItemSelectionModel::NoUpdate);
    QApplication::processEvents();
}

void tracePhase(
    int iteration,
    const char* phase) {
    std::cerr
        << "WB-01A stress iteration "
        << iteration
        << " phase "
        << phase
        << std::endl;
}

void selectTwoReferences(
    QTreeWidget& tree,
    QTreeWidgetItem& first,
    QTreeWidgetItem& second) {
    tree.clearSelection();
    first.setSelected(true);
    second.setSelected(true);
    tree.setCurrentItem(
        &second,
        0,
        QItemSelectionModel::NoUpdate);
    QApplication::processEvents();
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TempDirectory temp;
    const auto workspace = temp.path / "Project";
    std::filesystem::create_directories(workspace);

    application::ProjectWorkspaceMetadataService metadata;
    if (!metadata.initialize(workspace, "Machine").ok()) {
        return EXIT_FAILURE;
    }

    auto opened = application::ProjectSession::open(workspace);
    if (!opened.ok()) {
        return EXIT_FAILURE;
    }

    auto created =
        opened.session->createPart("StressPart.ss2part");
    if (!created.ok()) {
        return EXIT_FAILURE;
    }

    const auto document_id =
        created.session->documentId();

    viewer_qt_occt::QtOcctViewerWidget* viewport = nullptr;

    ui::CadWorkbench workbench{
        [&viewport](QWidget* parent) {
            viewport =
                new viewer_qt_occt::QtOcctViewerWidget{
                    parent};
            return ui::ViewportSurface{
                viewport,
                viewport};
        }};

    workbench.resize(1200, 780);

    if (!workbench.activateDocument(
            opened.session->documentSession(document_id),
            workspace) ||
        viewport == nullptr) {
        return EXIT_FAILURE;
    }

    workbench.show();

    int result = EXIT_FAILURE;

    QTimer::singleShot(
        150,
        &app,
        [&] {
            auto* tree =
                workbench.findChild<QTreeWidget*>(
                    QStringLiteral("documentTree"));
            auto* hide_action =
                workbench.findChild<QAction*>(
                    QStringLiteral(
                        "hideBuiltinReferencesAction"));
            auto* show_action =
                workbench.findChild<QAction*>(
                    QStringLiteral(
                        "showBuiltinReferencesAction"));
            auto* undo =
                workbench.findChild<QPushButton*>(
                    QStringLiteral("undoDocumentButton"));
            auto* redo =
                workbench.findChild<QPushButton*>(
                    QStringLiteral("redoDocumentButton"));
            auto* splitter =
                workbench.findChild<QSplitter*>(
                    QStringLiteral("workbenchSplitter"));
            auto* editor_surface =
                workbench.findChild<QWidget*>(
                    QStringLiteral("editorSurface"));
            auto* view_cube =
                workbench.findChild<QWidget*>(
                    QStringLiteral("viewCubeWidget"));
            auto* compact_button =
                workbench.findChild<QToolButton*>(
                    QStringLiteral("viewCubeCompactButton"));

            EXPECT(tree != nullptr);
            EXPECT(hide_action != nullptr);
            EXPECT(show_action != nullptr);
            EXPECT(undo != nullptr);
            EXPECT(redo != nullptr);
            EXPECT(splitter != nullptr);
            EXPECT(editor_surface != nullptr);
            EXPECT(view_cube != nullptr);
            EXPECT(compact_button != nullptr);

            auto* session =
                opened.session->documentSession(
                    document_id);
            EXPECT(session != nullptr);

            EXPECT(viewport->setStandardView(
                viewer::StandardView::front));
            viewport->fitAll();
            QApplication::processEvents();

            constexpr int stress_iterations = 100;

            for (int iteration = 0;
                 iteration < stress_iterations;
                 ++iteration) {
                EXPECT(viewport->isVisible());
                EXPECT(viewport->width() > 20);
                EXPECT(viewport->height() > 20);

                const QPoint center{
                    viewport->width() / 2,
                    viewport->height() / 2};
                const QPoint empty{
                    viewport->width() - 6,
                    viewport->height() - 6};

                tracePhase(iteration, "left-center");
                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::NoModifier,
                    center,
                    1);
                QApplication::processEvents();

                tracePhase(iteration, "ctrl-left-center");
                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::ControlModifier,
                    center,
                    1);
                QApplication::processEvents();

                tracePhase(iteration, "left-empty");
                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::NoModifier,
                    empty,
                    1);
                QApplication::processEvents();

                tracePhase(iteration, "right-center");
                QTest::mouseClick(
                    viewport,
                    Qt::RightButton,
                    Qt::NoModifier,
                    center,
                    1);
                QApplication::processEvents();

                if ((iteration % 10) == 0) {
                    tracePhase(iteration, "narrow-editor");
                    workbench.resize(900, 720);
                    splitter->setSizes({180, 120, 560});
                    QApplication::processEvents();

                    EXPECT(editor_surface->rect().contains(
                        view_cube->geometry()));
                    if (editor_surface->width() < 180) {
                        EXPECT(compact_button->isVisible());
                    }

                    tracePhase(iteration, "restore-editor");
                    workbench.resize(1200, 780);
                    splitter->setSizes({220, 620, 300});
                    QApplication::processEvents();

                    EXPECT(editor_surface->rect().contains(
                        view_cube->geometry()));
                }

                tracePhase(iteration, "navigation");
                viewport->panByPixels(
                    (iteration % 3) - 1,
                    ((iteration + 1) % 3) - 1);
                viewport->orbitByRadians(
                    0.001 * static_cast<double>(
                        (iteration % 3) - 1),
                    0.001);
                viewport->zoomByFactor(
                    (iteration % 2) == 0
                        ? 1.002
                        : 0.998);
                QApplication::processEvents();

                auto* origin_point =
                    findOriginReference(
                        *tree,
                        QStringLiteral("Origin Point"));
                EXPECT(origin_point != nullptr);

                selectReference(
                    *tree,
                    *origin_point);
                EXPECT(tree->selectedItems().size() == 1);

                QTest::mouseClick(
                    viewport,
                    Qt::RightButton,
                    Qt::NoModifier,
                    center,
                    1);
                QApplication::processEvents();
                EXPECT(tree->selectedItems().size() == 1);
                EXPECT(tree->selectedItems().front()->text(0) ==
                       QStringLiteral("Origin Point"));

                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::NoModifier,
                    empty,
                    1);
                QApplication::processEvents();
                EXPECT(tree->selectedItems().empty());

                origin_point =
                    findOriginReference(
                        *tree,
                        QStringLiteral("Origin Point"));
                EXPECT(origin_point != nullptr);
                tracePhase(iteration, "tree-origin-right-noop");
                selectReference(
                    *tree,
                    *origin_point);
                EXPECT(hide_action->isEnabled());
                tracePhase(iteration, "hide-origin");
                hide_action->trigger();
                QApplication::processEvents();

                EXPECT(!session->document()
                    .builtinReferenceVisible(
                        core::BuiltinReferenceRole::
                            origin_point));

                if ((iteration % 10) == 0) {
                    EXPECT(undo->isEnabled());
                    tracePhase(iteration, "undo-origin-hide");
                    undo->click();
                    QApplication::processEvents();
                    EXPECT(session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                origin_point));

                    EXPECT(redo->isEnabled());
                    tracePhase(iteration, "redo-origin-hide");
                    redo->click();
                    QApplication::processEvents();
                    EXPECT(!session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                origin_point));
                }

                origin_point =
                    findOriginReference(
                        *tree,
                        QStringLiteral("Origin Point"));
                EXPECT(origin_point != nullptr);
                selectReference(
                    *tree,
                    *origin_point);
                EXPECT(show_action->isEnabled());
                tracePhase(iteration, "show-origin");
                show_action->trigger();
                QApplication::processEvents();

                EXPECT(session->document()
                    .builtinReferenceVisible(
                        core::BuiltinReferenceRole::
                            origin_point));

                if ((iteration % 5) == 0) {
                    origin_point =
                        findOriginReference(
                            *tree,
                            QStringLiteral("Origin Point"));
                    auto* xy_plane =
                        findOriginReference(
                            *tree,
                            QStringLiteral("XY Plane"));
                    EXPECT(origin_point != nullptr);
                    EXPECT(xy_plane != nullptr);

                    selectTwoReferences(
                        *tree,
                        *origin_point,
                        *xy_plane);
                    EXPECT(show_action->isEnabled());
                    tracePhase(iteration, "batch-show");
                    show_action->trigger();
                    QApplication::processEvents();

                    EXPECT(session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                origin_point));
                    EXPECT(session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                xy_plane));

                    origin_point =
                        findOriginReference(
                            *tree,
                            QStringLiteral("Origin Point"));
                    xy_plane =
                        findOriginReference(
                            *tree,
                            QStringLiteral("XY Plane"));
                    EXPECT(origin_point != nullptr);
                    EXPECT(xy_plane != nullptr);
                    selectTwoReferences(
                        *tree,
                        *origin_point,
                        *xy_plane);
                    EXPECT(hide_action->isEnabled());
                    tracePhase(iteration, "batch-hide");
                    hide_action->trigger();
                    QApplication::processEvents();

                    EXPECT(!session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                origin_point));
                    EXPECT(!session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                xy_plane));

                    origin_point =
                        findOriginReference(
                            *tree,
                            QStringLiteral("Origin Point"));
                    EXPECT(origin_point != nullptr);
                    selectReference(
                        *tree,
                        *origin_point);
                    EXPECT(show_action->isEnabled());
                    show_action->trigger();
                    QApplication::processEvents();

                    EXPECT(session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                origin_point));
                    EXPECT(!session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                xy_plane));
                }
            }

            EXPECT(session->document()
                .builtinReferenceVisible(
                    core::BuiltinReferenceRole::origin_point));
            EXPECT(!session->document()
                .builtinReferenceVisible(
                    core::BuiltinReferenceRole::xy_plane));
            EXPECT(!session->needsSave());

            result = EXIT_SUCCESS;
            workbench.close();
            app.quit();
        });

    app.exec();
    return result;
}
