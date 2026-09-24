#pragma once

#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>
#include <simplesolid2/viewer/camera_state.hpp>

#include "viewport_surface.hpp"

#include <QWidget>

#include <filesystem>
#include <optional>
#include <string>
#include <unordered_map>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QTabBar;
class QStackedWidget;
class QTreeWidget;
class QWidget;

namespace simplesolid2::ui {

class CadWorkbenchShell;
class PartDocumentTreeController;
class PartViewportController;
class ViewCubeWidget;

enum class ProjectCloseDisposition {
    clean,
    discard,
    cancel,
};

class CadWorkbench final : public QWidget {
public:
    explicit CadWorkbench(QWidget* parent = nullptr);
    CadWorkbench(
        ViewportFactory viewport_factory,
        QWidget* parent = nullptr);

    void setProjectSession(application::ProjectSession* session);
    void clearProjectSession();

    [[nodiscard]] ProjectCloseDisposition prepareProjectClose();

    [[nodiscard]] std::optional<core::DocumentId> activeDocumentId() const {
        return active_document_id_;
    }

    [[nodiscard]] bool activateDocument(const core::DocumentId& document_id);

private:
    void buildUi();
    void refreshWorkspaceIndex();
    void syncOpenTabs();
    void ensureDocumentTab(const core::DocumentId& document_id);
    void activateTab(int index);
    void closeTab(int index);

    void newPart();
    void openDocument();
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
    void updateTabPresentation(const core::DocumentId& document_id);

    [[nodiscard]] application::DocumentSession* activeDocumentSession() noexcept;
    [[nodiscard]] const application::DocumentSession* activeDocumentSession() const noexcept;
    [[nodiscard]] std::filesystem::path defaultPartPath() const;
    [[nodiscard]] int tabIndexFor(const core::DocumentId& document_id) const;
    [[nodiscard]] std::optional<core::DocumentId> tabDocumentId(int index) const;

    void showFailure(const application::ProjectDocumentDiagnostic& diagnostic);
    void showFailure(const application::DocumentSessionDiagnostic& diagnostic);

    application::ProjectSession* session_{};
    std::optional<core::DocumentId> active_document_id_;
    ViewportFactory viewport_factory_;
    viewer::IDocumentViewport* viewport_{};
    std::unordered_map<std::string, viewer::CameraState>
        document_view_states_;
    bool sketch_support_pick_active_{};
    std::optional<core::DocumentId>
        sketch_edit_document_id_;
    std::optional<sketch::SketchId>
        active_sketch_id_;

    CadWorkbenchShell* shell_{};
    PartDocumentTreeController* tree_controller_{};
    PartViewportController* viewport_controller_{};
    ViewCubeWidget* view_cube_{};

    QPushButton* new_part_button_{};
    QPushButton* open_document_button_{};
    QPushButton* refresh_button_{};
    QPushButton* undo_button_{};
    QPushButton* redo_button_{};
    QPushButton* save_button_{};
    QPushButton* close_document_button_{};

    QTreeWidget* document_tree_{};
    QWidget* editor_surface_{};

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
    QPushButton* cancel_sketch_button_{};
    QPushButton* finish_sketch_button_{};

    QTabBar* document_tabs_{};
    QLabel* status_{};
};

} // namespace simplesolid2::ui
