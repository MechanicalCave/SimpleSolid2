#pragma once

#include <simplesolid2/application/cad_input.hpp>

#include <QWidget>

#include <functional>
#include <memory>
#include <string>
#include <utility>

class QEvent;
class QLabel;
class QLineEdit;
class QString;
class QPushButton;
class QSettings;
class QStackedWidget;
class QTabBar;
class QWidget;

namespace simplesolid2::ui {

class ProjectWorkspaceShell final : public QWidget {
public:
    explicit ProjectWorkspaceShell(QWidget* parent = nullptr);
    ProjectWorkspaceShell(
        std::unique_ptr<QSettings> user_settings,
        QWidget* parent = nullptr);
    ~ProjectWorkspaceShell() override;

    void setProjectInfo(
        const QString& display_name,
        const QString& project_id,
        const QString& workspace_path);

    [[nodiscard]] QPushButton& workspaceButton() noexcept;
    [[nodiscard]] QPushButton& newPartButton() noexcept;
    [[nodiscard]] QPushButton& openDocumentButton() noexcept;
    [[nodiscard]] QPushButton& refreshButton() noexcept;
    [[nodiscard]] QPushButton& closeProjectButton() noexcept;
    [[nodiscard]] QTabBar& documentTabs() noexcept;

    [[nodiscard]] QWidget& dashboard() noexcept;
    void setDocumentWorkbench(QWidget* workbench);
    void showWorkspace();
    void showDocumentWorkbench();

    void setCadInputEndpoint(
        application::ICadInputEndpoint* endpoint);
    void refreshCadInputPresentation();

    using CadInteractionSettingsChangedHandler =
        std::function<void()>;
    using CadInputPresentationChangedHandler =
        std::function<void()>;
    void setCadInteractionSettingsChangedHandler(
        CadInteractionSettingsChangedHandler handler) {
        cad_interaction_settings_changed_handler_ =
            std::move(handler);
    }
    void setCadInputPresentationChangedHandler(
        CadInputPresentationChangedHandler handler) {
        cad_input_presentation_changed_handler_ =
            std::move(handler);
    }

    using DocumentHistoryActionHandler =
        std::function<void()>;
    void setDocumentHistoryHandlers(
        DocumentHistoryActionHandler undo_handler,
        DocumentHistoryActionHandler redo_handler) {
        document_undo_handler_ =
            std::move(undo_handler);
        document_redo_handler_ =
            std::move(redo_handler);
    }

    [[nodiscard]] const std::string&
    cadInputBuffer() const noexcept;
    [[nodiscard]] std::size_t
    cadDynamicInputFieldIndex() const noexcept {
        return cad_input_.dynamicInputFieldIndex();
    }

    [[nodiscard]] const application::CadInteractionSettings&
    cadInteractionSettings() const noexcept {
        return cad_input_.interactionSettings();
    }

    [[nodiscard]] bool setCadInteractionSettings(
        application::CadInteractionSettings settings) {
        const bool accepted =
            cad_input_.setInteractionSettings(
                std::move(settings));
        if (accepted) {
            persistObjectSnapSettings();
            refreshCadInputPresentation();
            if (cad_interaction_settings_changed_handler_) {
                cad_interaction_settings_changed_handler_();
            }
        }
        return accepted;
    }

    [[nodiscard]] bool showingWorkspace() const noexcept;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    [[nodiscard]] bool focusOwnsTextInput(
        QWidget* focus) const noexcept;
    [[nodiscard]] bool focusBelongsToActiveCadSurface(
        QWidget* focus) const noexcept;
    void syncCadInputLineEdit();
    void submitCadInputFromLineEdit();
    void loadObjectSnapSettings();
    void persistObjectSnapSettings();

    QLabel* project_name_{};
    QLabel* project_id_{};
    QLabel* workspace_path_{};

    QPushButton* workspace_button_{};
    QPushButton* new_part_button_{};
    QPushButton* open_document_button_{};
    QPushButton* refresh_button_{};
    QPushButton* close_project_button_{};

    QStackedWidget* content_{};
    QWidget* dashboard_{};
    QWidget* document_workbench_{};
    QTabBar* document_tabs_{};

    std::unique_ptr<QSettings> user_settings_;
    application::CadInputSession cad_input_;
    CadInteractionSettingsChangedHandler
        cad_interaction_settings_changed_handler_;
    CadInputPresentationChangedHandler
        cad_input_presentation_changed_handler_;
    DocumentHistoryActionHandler
        document_undo_handler_;
    DocumentHistoryActionHandler
        document_redo_handler_;
    QWidget* command_line_widget_{};
    QLabel* command_prompt_{};
    QLineEdit* command_input_{};
    QLabel* command_diagnostic_{};
    bool syncing_command_input_{};
};

} // namespace simplesolid2::ui
