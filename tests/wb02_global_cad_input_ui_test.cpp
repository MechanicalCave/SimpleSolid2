#include "project_workspace_shell.hpp"

#include <simplesolid2/application/cad_input.hpp>

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << "\n"; \
            return EXIT_FAILURE; \
        } \
    } while (false)

class FakeEndpoint final
    : public simplesolid2::application::ICadInputEndpoint {
public:
    std::string last;
    std::string rejection{
        "Rejected by fake endpoint."};
    bool accept{true};

    [[nodiscard]] std::string
    cadInputPrompt() const override {
        return "Command: FAKE";
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    submitCadInput(std::string_view text) override {
        last.assign(text);
        return {
            accept,
            accept ? std::string{} :
                     rejection};
    }
};

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    simplesolid2::ui::ProjectWorkspaceShell shell;
    QWidget workbench;
    workbench.setFocusPolicy(Qt::StrongFocus);
    auto* layout = new QVBoxLayout(&workbench);

    auto* cad_surface = new QWidget(&workbench);
    cad_surface->setObjectName(
        QStringLiteral("testCadSurface"));
    cad_surface->setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(cad_surface);

    auto* ordinary_editor =
        new QLineEdit(&workbench);
    ordinary_editor->setObjectName(
        QStringLiteral("ordinaryEditor"));
    layout->addWidget(ordinary_editor);

    workbench.setFocusProxy(cad_surface);
    shell.setDocumentWorkbench(&workbench);

    FakeEndpoint first;
    FakeEndpoint second;
    shell.setCadInputEndpoint(&first);
    shell.showDocumentWorkbench();
    shell.show();
    QApplication::processEvents();

    auto* input =
        shell.findChild<QLineEdit*>(
            QStringLiteral("cadCommandInput"));
    auto* command_line =
        shell.findChild<QWidget*>(
            QStringLiteral("cadCommandLine"));
    auto* diagnostic =
        shell.findChild<QLabel*>(
            QStringLiteral("cadCommandDiagnostic"));
    CHECK(input != nullptr);
    CHECK(command_line != nullptr);
    CHECK(diagnostic != nullptr);
    CHECK(!diagnostic->wordWrap());
    CHECK(
        command_line->minimumHeight() ==
        command_line->maximumHeight());
    CHECK(
        diagnostic->minimumWidth() ==
        diagnostic->maximumWidth());

    const int command_height =
        command_line->height();
    const int input_width =
        input->width();
    const int diagnostic_width =
        diagnostic->width();

    cad_surface->setFocus(Qt::OtherFocusReason);
    CHECK(QApplication::focusWidget() == cad_surface);

    QTest::keyClicks(
        cad_surface,
        QStringLiteral("50"));
    QApplication::processEvents();
    CHECK(input->text() == QStringLiteral("50"));
    CHECK(QApplication::focusWidget() == cad_surface);

    QTest::keyClick(
        cad_surface,
        Qt::Key_Backspace);
    CHECK(input->text() == QStringLiteral("5"));
    QTest::keyClicks(
        cad_surface,
        QStringLiteral("0"));
    QTest::keyClick(
        cad_surface,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(first.last == "50");
    CHECK(input->text().isEmpty());
    CHECK(QApplication::focusWidget() == cad_surface);

    input->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        input,
        QStringLiteral("MOVE"));
    QTest::keyClick(
        input,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(first.last == "MOVE");
    CHECK(input->text().isEmpty());
    CHECK(QApplication::focusWidget() == cad_surface);

    ordinary_editor->clear();
    ordinary_editor->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        ordinary_editor,
        QStringLiteral("PartName50"));
    QApplication::processEvents();
    CHECK(
        ordinary_editor->text() ==
        QStringLiteral("PartName50"));
    CHECK(input->text().isEmpty());

    cad_surface->setFocus(Qt::OtherFocusReason);
    QTest::keyClick(
        cad_surface,
        Qt::Key_S,
        Qt::ControlModifier);
    QApplication::processEvents();
    CHECK(input->text().isEmpty());

    first.accept = false;
    first.rejection =
        "This is an intentionally very long rejected CAD input diagnostic "
        "that must stay inside one reserved line without resizing the "
        "Workspace Command Line or the Viewer above it.";
    QTest::keyClicks(
        cad_surface,
        QStringLiteral("BAD"));
    QTest::keyClick(
        cad_surface,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(first.last == "BAD");
    CHECK(input->text().isEmpty());
    CHECK(!diagnostic->text().isEmpty());
    CHECK(
        diagnostic->toolTip() ==
        QString::fromStdString(first.rejection));
    CHECK(command_line->height() == command_height);
    CHECK(input->width() == input_width);
    CHECK(diagnostic->width() == diagnostic_width);
    CHECK(QApplication::focusWidget() == cad_surface);

    // Editing before Enter remains possible; Esc clears a live token
    // without cancelling the underlying fake CAD context.
    QTest::keyClicks(
        cad_surface,
        QStringLiteral("TEMP"));
    CHECK(input->text() == QStringLiteral("TEMP"));
    QTest::keyClick(
        cad_surface,
        Qt::Key_Escape);
    QApplication::processEvents();
    CHECK(input->text().isEmpty());
    CHECK(QApplication::focusWidget() == cad_surface);

    QTest::keyClicks(
        cad_surface,
        QStringLiteral("STALE"));
    CHECK(!input->text().isEmpty());
    shell.setCadInputEndpoint(&second);
    CHECK(input->text().isEmpty());

    QTest::keyClicks(
        cad_surface,
        QStringLiteral("NEXT"));
    QTest::keyClick(
        cad_surface,
        Qt::Key_Return);
    CHECK(second.last == "NEXT");
    CHECK(first.last == "BAD");

    shell.showWorkspace();
    CHECK(input->text().isEmpty());
    cad_surface->setFocus(Qt::OtherFocusReason);
    QTest::keyClicks(
        cad_surface,
        QStringLiteral("HIDDEN"));
    QApplication::processEvents();
    CHECK(input->text().isEmpty());
    CHECK(second.last == "NEXT");

    return EXIT_SUCCESS;
}
