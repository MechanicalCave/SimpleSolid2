#pragma once

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>
#include <simplesolid2/viewer/camera_state.hpp>

#include "viewport_surface.hpp"

#include <QWidget>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>

class QEvent;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QStackedWidget;
class QTreeWidget;
class QWidget;

namespace simplesolid2::ui {

class CadWorkbenchShell;
class PartDocumentTreeController;
class PartSketchInteractionController;
class PartViewportController;
class ViewCubeWidget;

class CadWorkbench final : public QWidget {
public:
    explicit CadWorkbench(QWidget* parent = nullptr);
    ~CadWorkbench() override;
    CadWorkbench(
        ViewportFactory viewport_factory,
        QWidget* parent = nullptr);

    [[nodiscard]] bool activateDocument(
        application::DocumentSession* session,
        std::filesystem::path workspace_root);
    void deactivateDocument();
    void resetRuntimeState();
    void forgetDocumentRuntimeState(
        const core::DocumentId& document_id);

    [[nodiscard]] std::optional<core::DocumentId>
    activeDocumentId() const {
        if (document_session_ == nullptr) {
            return std::nullopt;
        }
        return document_session_->documentId();
    }

    using CloseDocumentHandler =
        std::function<void(
            const core::DocumentId&)>;
    void setCloseDocumentHandler(
        CloseDocumentHandler handler) {
        close_document_handler_ =
            std::move(handler);
    }

    using DocumentStateChangedHandler =
        std::function<void(
            const core::DocumentId&)>;
    void setDocumentStateChangedHandler(
        DocumentStateChangedHandler handler) {
        document_state_changed_handler_ =
            std::move(handler);
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void buildUi();

    void applyProperties();
    void startSketchTool();
    void cancelSketchTool();
    void requestEditSketch(
        const sketch::SketchId& sketch_id);
    void tryCreateSketchFromSupport(
        std::optional<core::BuiltinReferenceRole> support);
    void enterSketchEdit(
        const sketch::SketchId& sketch_id);
    void finishSketch();
    void activateSketchSelect();
    void activateSketchLine();
    void finishSketchLine();
    void cancelSketchLine();
    void deleteSketchSelection();
    void submitSketchCommandLine();
    void syncSketchInteractionUi();
    void clearSketchRuntimeContext();
    void reconcileSketchRuntimeContext();
    void undo();
    void redo();
    void save();
    void closeActiveDocument();

    void refreshActiveContext();
    void clearActiveContext();
    void captureActiveViewState();
    void restoreActiveViewState();
    void refreshPropertiesContext(
        std::optional<core::BuiltinReferenceRole> primary);
    void syncActionState();
    void notifyDocumentStateChanged();

    [[nodiscard]] application::DocumentSession*
    activeDocumentSession() noexcept {
        return document_session_;
    }
    [[nodiscard]] const application::DocumentSession*
    activeDocumentSession() const noexcept {
        return document_session_;
    }

    void showFailure(
        const application::DocumentSessionDiagnostic& diagnostic);

    application::DocumentSession* document_session_{};
    std::filesystem::path workspace_root_;
    ViewportFactory viewport_factory_;
    viewer::IDocumentViewport* viewport_{};
    std::unordered_map<
        std::string,
        viewer::CameraState>
        document_view_states_;

    bool sketch_support_pick_active_{};
    std::optional<core::DocumentId>
        sketch_edit_document_id_;
    std::optional<sketch::SketchId>
        active_sketch_id_;

    CloseDocumentHandler
        close_document_handler_;
    DocumentStateChangedHandler
        document_state_changed_handler_;

    CadWorkbenchShell* shell_{};
    PartDocumentTreeController* tree_controller_{};
    PartViewportController* viewport_controller_{};
    std::unique_ptr<PartSketchInteractionController>
        sketch_interaction_controller_;

    QPushButton* undo_button_{};
    QPushButton* redo_button_{};
    QPushButton* save_button_{};
    QPushButton* close_document_button_{};

    QTreeWidget* document_tree_{};
    QWidget* editor_surface_{};
    QWidget* viewport_widget_{};

    QStackedWidget* properties_stack_{};
    QWidget* document_properties_page_{};
    QWidget* reference_properties_page_{};
    QLabel* active_path_{};
    QLabel* active_id_{};
    QLabel* reference_name_{};
    QLabel* reference_kind_{};
    QLabel* reference_identity_{};
    QLabel* reference_visibility_{};
    QLineEdit* number_{};
    QLineEdit* title_{};
    QPlainTextEdit* description_{};
    QLineEdit* engineering_revision_{};
    QPushButton* apply_button_{};

    QLabel* operations_placeholder_{};
    QPushButton* sketch_button_{};
    QPushButton* select_sketch_button_{};
    QPushButton* line_sketch_button_{};
    QPushButton* cancel_sketch_button_{};
    QPushButton* finish_sketch_button_{};
    QPushButton* finish_line_button_{};
    QPushButton* cancel_line_button_{};
    QPushButton* delete_selection_button_{};

    QWidget* command_line_widget_{};
    QLabel* command_prompt_{};
    QLineEdit* command_input_{};

    QLabel* status_{};
};

} // namespace simplesolid2::ui
