#include "cad_workbench.hpp"

#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/viewer_qt_occt/qt_occt_viewer_widget.hpp>

#include <QAction>
#include <QApplication>
#include <QItemSelectionModel>
#include <QPushButton>
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
    workbench.setProjectSession(&*opened.session);

    if (!workbench.activateDocument(document_id) ||
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

            EXPECT(tree != nullptr);
            EXPECT(hide_action != nullptr);
            EXPECT(show_action != nullptr);
            EXPECT(undo != nullptr);
            EXPECT(redo != nullptr);

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

                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::NoModifier,
                    center,
                    1);
                QApplication::processEvents();

                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::ControlModifier,
                    center,
                    1);
                QApplication::processEvents();

                QTest::mouseClick(
                    viewport,
                    Qt::LeftButton,
                    Qt::NoModifier,
                    empty,
                    1);
                QApplication::processEvents();

                QTest::mouseClick(
                    viewport,
                    Qt::RightButton,
                    Qt::NoModifier,
                    center,
                    1);
                QApplication::processEvents();

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
                EXPECT(hide_action->isEnabled());
                hide_action->trigger();
                QApplication::processEvents();

                EXPECT(!session->document()
                    .builtinReferenceVisible(
                        core::BuiltinReferenceRole::
                            origin_point));

                if ((iteration % 10) == 0) {
                    EXPECT(undo->isEnabled());
                    undo->click();
                    QApplication::processEvents();
                    EXPECT(session->document()
                        .builtinReferenceVisible(
                            core::BuiltinReferenceRole::
                                origin_point));

                    EXPECT(redo->isEnabled());
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
