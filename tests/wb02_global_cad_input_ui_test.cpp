#include "project_workspace_shell.hpp"

#include <simplesolid2/application/cad_input.hpp>

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>
#include <QVBoxLayout>
#include <QWidget>

#include <cstdlib>
#include <iostream>
#include <memory>
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
    bool expose_dynamic_fields{true};
    std::size_t locked_field{};
    std::string locked_token;
    unsigned dynamic_submit_count{};
    simplesolid2::application::CadInputContextGeneration
        generation{1U};

    [[nodiscard]]
    simplesolid2::application::CadInputContextGeneration
    cadInputContextGeneration() const noexcept override {
        return generation;
    }

    [[nodiscard]] std::string
    cadInputPrompt() const override {
        return "Command: FAKE";
    }

    [[nodiscard]]
    std::vector<
        simplesolid2::application::CadDynamicInputField>
    cadDynamicInputFields() const override {
        if (!expose_dynamic_fields) {
            return {};
        }
        using Semantic =
            simplesolid2::application::
                CadDynamicInputFieldSemantic;
        return {
            {Semantic::distance, "Distance"},
            {Semantic::angle, "Angle"},
        };
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    lockCadDynamicInputField(
        std::size_t index,
        std::string_view text,
        simplesolid2::application::CadInputContextGeneration
            expected_context_generation) override {
        if (expected_context_generation != generation) {
            return {false, "Stale fake DYN context."};
        }
        locked_field = index;
        locked_token.assign(text);
        return {true, {}};
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    submitCadDynamicInputRequest(
        simplesolid2::application::CadInputContextGeneration
            expected_context_generation) override {
        if (expected_context_generation != generation) {
            return {false, "Stale fake DYN submit context."};
        }
        ++dynamic_submit_count;
        return {true, {}};
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    submitCadInput(
        std::string_view text,
        simplesolid2::application::CadInputContextGeneration
            expected_context_generation) override {
        if (expected_context_generation != generation) {
            return {false, "Stale fake context."};
        }
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

    // R11 user preference persistence is application/user-level and stores
    // only OSNAP/OTRACK choices. The test uses a private INI backend so the
    // self-hosted runner's real user settings are never read or changed.
    QTemporaryDir preference_root;
    CHECK(preference_root.isValid());
    const auto preference_path =
        preference_root.filePath(
            QStringLiteral("cad-preferences.ini"));

    {
        auto store =
            std::make_unique<QSettings>(
                preference_path,
                QSettings::IniFormat);
        simplesolid2::ui::ProjectWorkspaceShell
            preference_writer{
                std::move(store)};

        auto settings =
            preference_writer.cadInteractionSettings();
        settings.object_snap.master_enabled = false;
        settings.object_snap.endpoint = false;
        settings.object_snap.midpoint = false;
        settings.object_snap.center = true;
        settings.object_snap.quadrant = false;
        settings.object_snap.intersection = true;
        settings.object_snap.origin = false;
        settings.object_snap.perpendicular = true;
        settings.object_snap.tangent = true;
        settings.object_snap.nearest = true;
        settings.object_snap.extension = true;
        settings.object_snap.object_tracking_enabled = true;

        // These R10 runtime aids intentionally do not belong to the R11
        // persisted OSNAP/OTRACK preference payload.
        settings.polar.enabled = false;
        settings.dynamic_input_enabled = true;
        CHECK(
            preference_writer.setCadInteractionSettings(
                settings));
    }

    {
        auto store =
            std::make_unique<QSettings>(
                preference_path,
                QSettings::IniFormat);
        simplesolid2::ui::ProjectWorkspaceShell
            preference_reader{
                std::move(store)};
        const auto restored =
            preference_reader.cadInteractionSettings();

        CHECK(!restored.object_snap.master_enabled);
        CHECK(!restored.object_snap.endpoint);
        CHECK(!restored.object_snap.midpoint);
        CHECK(restored.object_snap.center);
        CHECK(!restored.object_snap.quadrant);
        CHECK(restored.object_snap.intersection);
        CHECK(!restored.object_snap.origin);
        CHECK(restored.object_snap.perpendicular);
        CHECK(restored.object_snap.tangent);
        CHECK(restored.object_snap.nearest);
        CHECK(restored.object_snap.extension);
        CHECK(
            restored.object_snap.
                object_tracking_enabled);

        CHECK(restored.polar.enabled);
        CHECK(!restored.dynamic_input_enabled);
    }

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

    std::size_t undo_shortcut_count{};
    std::size_t redo_shortcut_count{};
    shell.setDocumentHistoryHandlers(
        [&undo_shortcut_count] {
            ++undo_shortcut_count;
        },
        [&redo_shortcut_count] {
            ++redo_shortcut_count;
        });

    FakeEndpoint first;
    FakeEndpoint second;
    shell.setCadInputEndpoint(&first);
    shell.showDocumentWorkbench();
    shell.show();
    shell.activateWindow();
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

    QTest::keyClick(
        cad_surface,
        Qt::Key_Z,
        Qt::ControlModifier);
    QTest::keyClick(
        cad_surface,
        Qt::Key_Y,
        Qt::ControlModifier);
    QApplication::processEvents();
    CHECK(undo_shortcut_count == 1U);
    CHECK(redo_shortcut_count == 1U);
    CHECK(input->text().isEmpty());

    CHECK(shell.cadInteractionSettings().polar.enabled);
    CHECK(
        !shell.cadInteractionSettings().
            dynamic_input_enabled);
    QTest::keyClick(
        cad_surface,
        Qt::Key_F10);
    QApplication::processEvents();
    CHECK(!shell.cadInteractionSettings().polar.enabled);
    QTest::keyClick(
        cad_surface,
        Qt::Key_F12);
    QApplication::processEvents();
    CHECK(
        shell.cadInteractionSettings().
            dynamic_input_enabled);
    CHECK(
        shell.findChild<QLineEdit*>(
            QStringLiteral("cadCommandInput"))->
            text().isEmpty());

    // R10: empty Enter with DYN ON belongs to the active semantic
    // request even when the shared CAD text buffer is empty.
    QTest::keyClick(
        cad_surface,
        Qt::Key_Return);
    QApplication::processEvents();
    CHECK(first.dynamic_submit_count == 1U);

    QTest::keyClick(
        cad_surface,
        Qt::Key_Tab);
    QApplication::processEvents();

    QTest::keyClicks(
        cad_surface,
        QStringLiteral("100"));
    CHECK(input->text() == QStringLiteral("100"));
    QTest::keyClick(
        cad_surface,
        Qt::Key_Tab);
    QApplication::processEvents();
    CHECK(first.locked_field == 1U);
    CHECK(first.locked_token == "100");
    CHECK(input->text().isEmpty());

    QTest::keyClick(
        cad_surface,
        Qt::Key_Backtab);
    QApplication::processEvents();

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
    QTest::keyClick(
        input,
        Qt::Key_Tab);
    QApplication::processEvents();
    QTest::keyClick(
        input,
        Qt::Key_Backtab);
    QApplication::processEvents();
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
    const auto settings_before_editor_keys =
        shell.cadInteractionSettings();
    QTest::keyClick(
        ordinary_editor,
        Qt::Key_F10);
    QTest::keyClick(
        ordinary_editor,
        Qt::Key_F12);
    QApplication::processEvents();
    CHECK(
        shell.cadInteractionSettings() ==
        settings_before_editor_keys);
    QTest::keyClicks(
        ordinary_editor,
        QStringLiteral("PartName50"));
    QApplication::processEvents();
    CHECK(
        ordinary_editor->text() ==
        QStringLiteral("PartName50"));
    CHECK(input->text().isEmpty());

    const auto editor_text_before_undo =
        ordinary_editor->text();
    QTest::keyClick(
        ordinary_editor,
        Qt::Key_Z,
        Qt::ControlModifier);
    QApplication::processEvents();
    CHECK(
        ordinary_editor->text() !=
        editor_text_before_undo);
    CHECK(undo_shortcut_count == 1U);
    CHECK(redo_shortcut_count == 1U);
    QTest::keyClick(
        ordinary_editor,
        Qt::Key_Y,
        Qt::ControlModifier);
    QApplication::processEvents();
    CHECK(
        ordinary_editor->text() ==
        editor_text_before_undo);
    CHECK(undo_shortcut_count == 1U);
    CHECK(redo_shortcut_count == 1U);

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

    // Same endpoint object, new semantic generation: live input is
    // invalidated without attach/detach.
    ++first.generation;
    shell.refreshCadInputPresentation();
    CHECK(input->text().isEmpty());

    QTest::keyClicks(
        cad_surface,
        QStringLiteral("BOUND"));
    CHECK(input->text() == QStringLiteral("BOUND"));

    // A foreign non-modal top-level window must own its key events.
    QWidget foreign_window;
    auto* foreign_surface =
        new QWidget(&foreign_window);
    foreign_surface->setFocusPolicy(Qt::StrongFocus);
    foreign_window.show();
    foreign_window.activateWindow();
    foreign_surface->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();
    QTest::keyClicks(
        foreign_surface,
        QStringLiteral("FOREIGN"));
    QApplication::processEvents();
    CHECK(input->text() == QStringLiteral("BOUND"));

    foreign_window.hide();
    shell.activateWindow();
    cad_surface->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();

    // Popup/menu ownership also blocks background CAD capture.
    QMenu popup(&shell);
    popup.addAction(QStringLiteral("Menu Action"));
    popup.popup(shell.mapToGlobal(QPoint{10, 10}));
    QApplication::processEvents();
    CHECK(QApplication::activePopupWidget() != nullptr);
    QTest::keyClick(
        QApplication::activePopupWidget(),
        Qt::Key_X);
    QApplication::processEvents();
    CHECK(input->text() == QStringLiteral("BOUND"));
    popup.close();
    QApplication::processEvents();

    const auto settings_before_endpoint_switch =
        shell.cadInteractionSettings();
    shell.setCadInputEndpoint(&second);
    CHECK(input->text().isEmpty());
    CHECK(
        shell.cadInteractionSettings() ==
        settings_before_endpoint_switch);

    QTest::keyClicks(
        cad_surface,
        QStringLiteral("NEXT"));
    QTest::keyClick(
        cad_surface,
        Qt::Key_Return);
    CHECK(second.last == "NEXT");
    CHECK(first.last == "BAD");

    // Two visible shells are isolated by active-window/focus ownership.
    simplesolid2::ui::ProjectWorkspaceShell other_shell;
    QWidget other_workbench;
    other_workbench.setFocusPolicy(Qt::StrongFocus);
    auto* other_layout =
        new QVBoxLayout(&other_workbench);
    auto* other_surface =
        new QWidget(&other_workbench);
    other_surface->setFocusPolicy(Qt::StrongFocus);
    other_layout->addWidget(other_surface);
    other_workbench.setFocusProxy(other_surface);
    FakeEndpoint other_endpoint;
    other_shell.setDocumentWorkbench(&other_workbench);
    other_shell.setCadInputEndpoint(&other_endpoint);
    other_shell.showDocumentWorkbench();
    other_shell.show();
    other_shell.activateWindow();
    other_surface->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();

    auto* other_input =
        other_shell.findChild<QLineEdit*>(
            QStringLiteral("cadCommandInput"));
    CHECK(other_input != nullptr);
    QTest::keyClicks(
        other_surface,
        QStringLiteral("TWO"));
    QApplication::processEvents();
    CHECK(other_input->text() == QStringLiteral("TWO"));
    CHECK(input->text().isEmpty());

    other_shell.hide();
    shell.activateWindow();
    cad_surface->setFocus(Qt::OtherFocusReason);
    QApplication::processEvents();

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
