#include "cad_workbench.hpp"
#include "cad_workbench_shell.hpp"
#include "part_document_tree_controller.hpp"
#include "part_sketch_interaction_controller.hpp"
#include "part_viewport_controller.hpp"

#include <QEvent>
#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace simplesolid2::ui {
namespace {

std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return std::string{
        bytes.constData(),
        static_cast<std::size_t>(bytes.size())};
}

QString fromUtf8(std::string_view value) {
    return QString::fromUtf8(
        value.data(),
        static_cast<qsizetype>(value.size()));
}

QString fromFilesystemPath(const std::filesystem::path& value) {
#if defined(_WIN32)
    return QString::fromStdWString(value.wstring());
#else
    const auto utf8 = value.generic_u8string();
    return QString::fromUtf8(
        reinterpret_cast<const char*>(utf8.data()),
        static_cast<qsizetype>(utf8.size()));
#endif
}

std::optional<viewer::StandardView>
standardViewForSketchSupport(
    core::BuiltinReferenceRole role) noexcept {
    switch (role) {
    case core::BuiltinReferenceRole::xy_plane:
        return viewer::StandardView::top;
    case core::BuiltinReferenceRole::xz_plane:
        return viewer::StandardView::front;
    case core::BuiltinReferenceRole::yz_plane:
        return viewer::StandardView::right;
    default:
        return std::nullopt;
    }
}

} // namespace

CadWorkbench::CadWorkbench(QWidget* parent)
    : CadWorkbench{ViewportFactory{}, parent} {}

CadWorkbench::~CadWorkbench() = default;

CadWorkbench::CadWorkbench(
    ViewportFactory viewport_factory,
    QWidget* parent)
    : QWidget{parent},
      viewport_factory_{std::move(viewport_factory)} {
    buildUi();
    deactivateDocument();
    status_->setText(
        QStringLiteral("No Part is active."));
}

void CadWorkbench::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    shell_ = new CadWorkbenchShell(this);
    shell_->setObjectName(QStringLiteral("cadWorkbenchShell"));
    root->addWidget(shell_, 1);

    auto& lifecycle_actions = shell_->documentActionsLayout();

    undo_button_ =
        new QPushButton(QStringLiteral("Undo"), shell_);
    undo_button_->setObjectName(QStringLiteral("undoDocumentButton"));

    redo_button_ =
        new QPushButton(QStringLiteral("Redo"), shell_);
    redo_button_->setObjectName(QStringLiteral("redoDocumentButton"));

    save_button_ =
        new QPushButton(QStringLiteral("Save"), shell_);
    save_button_->setObjectName(QStringLiteral("saveDocumentButton"));

    close_document_button_ =
        new QPushButton(QStringLiteral("Close"), shell_);
    close_document_button_->setObjectName(
        QStringLiteral("closeDocumentButton"));

    lifecycle_actions.addStretch(1);
    lifecycle_actions.addWidget(undo_button_);
    lifecycle_actions.addWidget(redo_button_);
    lifecycle_actions.addWidget(save_button_);
    lifecycle_actions.addWidget(close_document_button_);

    document_tree_ = &shell_->documentTree();
    status_ = &shell_->statusLabel();

    tree_controller_ =
        new PartDocumentTreeController(*document_tree_, this);
    tree_controller_->setResultHandler(
        [this](
            const application::DocumentSessionResult& result,
            bool visible) {
            if (!result.ok()) {
                showFailure(result.diagnostic);
                refreshActiveContext();
                return;
            }

            refreshActiveContext();
            status_->setText(
                result.changed
                    ? (visible
                           ? QStringLiteral(
                                 "Selected Origin references shown.")
                           : QStringLiteral(
                                 "Selected Origin references hidden."))
                    : QStringLiteral(
                          "No Origin visibility change."));
        });

    tree_controller_->setSketchEditHandler(
        [this](const sketch::SketchId& sketch_id) {
            requestEditSketch(sketch_id);
        });

    ViewportSurface viewport_surface;
    if (viewport_factory_) {
        viewport_surface = viewport_factory_(shell_);
    }

    auto* editor_container = new QFrame(shell_);
    editor_container->setObjectName(
        QStringLiteral("editorSurface"));
    editor_container->setFrameShape(QFrame::StyledPanel);
    auto* editor_layout =
        new QGridLayout(editor_container);
    editor_layout->setContentsMargins(0, 0, 0, 0);

    if (viewport_surface.valid()) {
        viewport_ = viewport_surface.viewport;
        auto* viewport_widget =
            viewport_surface.widget;
        viewport_widget_ = viewport_widget;

        viewport_widget->setObjectName(
            QStringLiteral("documentViewport"));
        if (viewport_widget->parentWidget() != editor_container) {
            viewport_widget->setParent(editor_container);
        }

        editor_layout->addWidget(
            viewport_widget,
            0,
            0);

        viewport_->setNavigationCubeActionHandler(
            [this](
                const viewer::NavigationCubeAction& action) {
                if (viewport_ == nullptr) {
                    return;
                }

                const auto current =
                    viewport_->cameraState();
                if (!current) {
                    return;
                }

                const auto navigation =
                    viewer::navigationForCubeAction(
                        *current,
                        action);
                if (!navigation) {
                    return;
                }

                const double duration_seconds =
                    action.kind ==
                            viewer::NavigationCubeActionKind::
                                toggle_projection
                        ? 0.0
                        : 0.28;

                static_cast<void>(
                    viewport_->animateCameraState(
                        navigation->camera,
                        duration_seconds,
                        navigation->fit_all));
            });
    } else {
        if (viewport_surface.widget != nullptr) {
            viewport_surface.widget->deleteLater();
        }

        viewport_ = nullptr;

        auto* editor_label = new QLabel(
            QStringLiteral(
                "3D Document View\n"
                "Native Viewer surface is unavailable."),
            editor_container);
        editor_label->setObjectName(
            QStringLiteral("editorSurfacePlaceholder"));
        editor_label->setAlignment(Qt::AlignCenter);
        editor_layout->addWidget(
            editor_label,
            0,
            0);
    }

    editor_surface_ = editor_container;
    shell_->setEditorSurface(editor_surface_);

    sketch_button_ =
        new QPushButton(
            QStringLiteral("Sketch"),
            shell_);
    sketch_button_->setObjectName(
        QStringLiteral("sketchToolButton"));
    shell_->editorToolsLayout().insertWidget(
        0,
        sketch_button_);

    select_sketch_button_ =
        new QPushButton(
            QStringLiteral("Select"),
            shell_);
    select_sketch_button_->setObjectName(
        QStringLiteral("selectSketchToolButton"));
    select_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        1,
        select_sketch_button_);

    line_sketch_button_ =
        new QPushButton(
            QStringLiteral("Line"),
            shell_);
    line_sketch_button_->setObjectName(
        QStringLiteral("lineSketchToolButton"));
    line_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        2,
        line_sketch_button_);

    circle_sketch_button_ =
        new QPushButton(
            QStringLiteral("Circle"),
            shell_);
    circle_sketch_button_->setObjectName(
        QStringLiteral("circleSketchToolButton"));
    circle_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        3,
        circle_sketch_button_);

    arc_sketch_button_ =
        new QPushButton(
            QStringLiteral("Arc"),
            shell_);
    arc_sketch_button_->setObjectName(
        QStringLiteral("arcSketchToolButton"));
    arc_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        4,
        arc_sketch_button_);

    move_sketch_button_ =
        new QPushButton(
            QStringLiteral("Move"),
            shell_);
    move_sketch_button_->setObjectName(
        QStringLiteral("moveSketchToolButton"));
    move_sketch_button_->setCheckable(true);
    shell_->editorToolsLayout().insertWidget(
        5,
        move_sketch_button_);

    viewport_controller_ =
        new PartViewportController(
            *tree_controller_,
            viewport_,
            this);
    viewport_controller_->setSelectionChangedHandler(
        [this](
            const std::vector<core::BuiltinReferenceRole>&,
            std::optional<core::BuiltinReferenceRole> primary) {
            refreshPropertiesContext(primary);
            tryCreateSketchFromSupport(primary);
        });

    sketch_interaction_controller_ =
        std::make_unique<PartSketchInteractionController>(
            *viewport_controller_);
    sketch_interaction_controller_->setStateChangedHandler(
        [this] {
            syncSketchInteractionUi();
            syncActionState();
            notifyDocumentStateChanged();
        });
    sketch_interaction_controller_->setStatusHandler(
        [this](const std::string& message) {
            status_->setText(fromUtf8(message));
        });
    viewport_controller_->setSketchPointerHandler(
        [this](const SketchPointerInput& input) {
            if (sketch_interaction_controller_) {
                sketch_interaction_controller_->onPointer(
                    input);
            }
        });

    command_line_widget_ = new QWidget(shell_);
    command_line_widget_->setObjectName(
        QStringLiteral("sketchCommandLine"));
    command_line_widget_->setMaximumHeight(58);
    auto* command_line_layout =
        new QHBoxLayout(command_line_widget_);
    command_line_layout->setContentsMargins(4, 2, 4, 2);

    command_prompt_ =
        new QLabel(
            QStringLiteral("Command: SELECT"),
            command_line_widget_);
    command_prompt_->setObjectName(
        QStringLiteral("sketchCommandPrompt"));
    command_line_layout->addWidget(command_prompt_);

    command_input_ =
        new QLineEdit(command_line_widget_);
    command_input_->setObjectName(
        QStringLiteral("sketchCommandInput"));
    command_input_->setPlaceholderText(
        QStringLiteral("SELECT, LINE, CIRCLE, ARC or MOVE"));
    command_line_layout->addWidget(command_input_, 1);
    shell_->setCommandLineContent(
        command_line_widget_);

    if (viewport_widget_ != nullptr) {
        viewport_widget_->installEventFilter(this);
    }
    command_input_->installEventFilter(this);

    auto* properties_content = new QWidget(shell_);
    properties_content->setObjectName(
        QStringLiteral("partPropertiesContent"));
    auto* properties_root =
        new QVBoxLayout(properties_content);
    properties_root->setContentsMargins(0, 0, 0, 0);

    properties_stack_ =
        new QStackedWidget(properties_content);
    properties_stack_->setObjectName(
        QStringLiteral("propertiesContextStack"));
    properties_root->addWidget(properties_stack_, 1);

    document_properties_page_ =
        new QWidget(properties_stack_);
    document_properties_page_->setObjectName(
        QStringLiteral("documentPropertiesPage"));
    auto* document_properties_root =
        new QVBoxLayout(document_properties_page_);
    document_properties_root->setContentsMargins(0, 0, 0, 0);

    active_path_ =
        new QLabel(document_properties_page_);
    active_path_->setObjectName(
        QStringLiteral("activeDocumentPath"));
    active_path_->setWordWrap(true);

    active_id_ =
        new QLabel(document_properties_page_);
    active_id_->setObjectName(
        QStringLiteral("activeDocumentId"));
    active_id_->setWordWrap(true);

    document_properties_root->addWidget(active_path_);
    document_properties_root->addWidget(active_id_);

    auto* form = new QFormLayout;

    number_ =
        new QLineEdit(document_properties_page_);
    number_->setObjectName(
        QStringLiteral("documentNumberEdit"));

    title_ =
        new QLineEdit(document_properties_page_);
    title_->setObjectName(
        QStringLiteral("documentTitleEdit"));

    description_ =
        new QPlainTextEdit(document_properties_page_);
    description_->setObjectName(
        QStringLiteral("documentDescriptionEdit"));
    description_->setMaximumHeight(90);

    engineering_revision_ =
        new QLineEdit(document_properties_page_);
    engineering_revision_->setObjectName(
        QStringLiteral("documentEngineeringRevisionEdit"));

    form->addRow(QStringLiteral("Number"), number_);
    form->addRow(QStringLiteral("Title"), title_);
    form->addRow(QStringLiteral("Description"), description_);
    form->addRow(
        QStringLiteral("Engineering revision"),
        engineering_revision_);
    document_properties_root->addLayout(form);

    apply_button_ = new QPushButton(
        QStringLiteral("Apply Properties"),
        document_properties_page_);
    apply_button_->setObjectName(
        QStringLiteral("applyDocumentPropertiesButton"));
    document_properties_root->addWidget(apply_button_);
    document_properties_root->addStretch(1);

    properties_stack_->addWidget(
        document_properties_page_);

    reference_properties_page_ =
        new QWidget(properties_stack_);
    reference_properties_page_->setObjectName(
        QStringLiteral("referencePropertiesPage"));
    auto* reference_root =
        new QFormLayout(reference_properties_page_);

    reference_name_ =
        new QLabel(reference_properties_page_);
    reference_name_->setObjectName(
        QStringLiteral("referencePropertyName"));

    reference_kind_ =
        new QLabel(reference_properties_page_);
    reference_kind_->setObjectName(
        QStringLiteral("referencePropertyKind"));

    reference_identity_ =
        new QLabel(reference_properties_page_);
    reference_identity_->setObjectName(
        QStringLiteral("referencePropertyIdentity"));
    reference_identity_->setWordWrap(true);

    reference_visibility_ =
        new QLabel(reference_properties_page_);
    reference_visibility_->setObjectName(
        QStringLiteral("referencePropertyVisibility"));

    reference_root->addRow(
        QStringLiteral("Reference"),
        reference_name_);
    reference_root->addRow(
        QStringLiteral("Type"),
        reference_kind_);
    reference_root->addRow(
        QStringLiteral("Identity"),
        reference_identity_);
    reference_root->addRow(
        QStringLiteral("Visibility"),
        reference_visibility_);

    properties_stack_->addWidget(
        reference_properties_page_);
    properties_stack_->setCurrentWidget(
        document_properties_page_);

    shell_->setPropertiesContent(properties_content);

    auto* operations_content = new QWidget(shell_);
    operations_content->setObjectName(
        QStringLiteral("partOperationsContent"));
    auto* operations_layout = new QVBoxLayout(operations_content);
    operations_layout->setContentsMargins(0, 0, 0, 0);

    operations_placeholder_ = new QLabel(
        QStringLiteral("Part modeling context."),
        operations_content);
    operations_placeholder_->setObjectName(
        QStringLiteral("operationsPlaceholder"));
    operations_placeholder_->setWordWrap(true);
    operations_layout->addWidget(operations_placeholder_);

    cancel_sketch_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            operations_content);
    cancel_sketch_button_->setObjectName(
        QStringLiteral("cancelSketchButton"));
    operations_layout->addWidget(
        cancel_sketch_button_);

    delete_selection_button_ =
        new QPushButton(
            QStringLiteral("Delete Selection"),
            operations_content);
    delete_selection_button_->setObjectName(
        QStringLiteral("deleteSketchSelectionButton"));
    operations_layout->addWidget(
        delete_selection_button_);

    finish_line_button_ =
        new QPushButton(
            QStringLiteral("Finish Line"),
            operations_content);
    finish_line_button_->setObjectName(
        QStringLiteral("finishSketchLineButton"));
    operations_layout->addWidget(
        finish_line_button_);

    cancel_line_button_ =
        new QPushButton(
            QStringLiteral("Cancel Line"),
            operations_content);
    cancel_line_button_->setObjectName(
        QStringLiteral("cancelSketchLineButton"));
    operations_layout->addWidget(
        cancel_line_button_);

    operations_layout->addStretch(1);

    finish_sketch_button_ =
        new QPushButton(
            QStringLiteral("Finish Sketch"),
            operations_content);
    finish_sketch_button_->setObjectName(
        QStringLiteral("finishSketchButton"));
    operations_layout->addWidget(
        finish_sketch_button_);

    shell_->setOperationsContent(operations_content);

    QObject::connect(
        undo_button_,
        &QPushButton::clicked,
        this,
        [this] { undo(); });
    QObject::connect(
        redo_button_,
        &QPushButton::clicked,
        this,
        [this] { redo(); });
    QObject::connect(
        save_button_,
        &QPushButton::clicked,
        this,
        [this] { save(); });
    QObject::connect(
        close_document_button_,
        &QPushButton::clicked,
        this,
        [this] { closeActiveDocument(); });
    QObject::connect(
        apply_button_,
        &QPushButton::clicked,
        this,
        [this] { applyProperties(); });
    QObject::connect(
        sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { startSketchTool(); });
    QObject::connect(
        select_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchSelect(); });
    QObject::connect(
        line_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchLine(); });
    QObject::connect(
        circle_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchCircle(); });
    QObject::connect(
        arc_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchArc(); });
    QObject::connect(
        move_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { activateSketchMove(); });
    QObject::connect(
        cancel_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { cancelSketchTool(); });
    QObject::connect(
        finish_sketch_button_,
        &QPushButton::clicked,
        this,
        [this] { finishSketch(); });
    QObject::connect(
        finish_line_button_,
        &QPushButton::clicked,
        this,
        [this] { finishSketchLine(); });
    QObject::connect(
        cancel_line_button_,
        &QPushButton::clicked,
        this,
        [this] { cancelSketchLine(); });
    QObject::connect(
        delete_selection_button_,
        &QPushButton::clicked,
        this,
        [this] { deleteSketchSelection(); });
    QObject::connect(
        command_input_,
        &QLineEdit::returnPressed,
        this,
        [this] { submitSketchCommandLine(); });

    syncSketchInteractionUi();
}

bool CadWorkbench::activateDocument(
    application::DocumentSession* session,
    std::filesystem::path workspace_root) {
    if (session == nullptr) {
        deactivateDocument();
        return false;
    }

    if (document_session_ == session) {
        workspace_root_ = std::move(workspace_root);
        refreshActiveContext();
        status_->setText(
            QStringLiteral("Part Document activated."));
        return true;
    }

    captureActiveViewState();
    clearSketchRuntimeContext();
    if (viewport_controller_ != nullptr) {
        viewport_controller_->clear();
    }

    document_session_ = session;
    workspace_root_ = std::move(workspace_root);

    restoreActiveViewState();
    refreshActiveContext();
    status_->setText(
        QStringLiteral("Part Document activated."));
    return true;
}

void CadWorkbench::deactivateDocument() {
    if (document_session_ != nullptr) {
        captureActiveViewState();
    }

    clearSketchRuntimeContext();
    document_session_ = nullptr;
    workspace_root_.clear();

    clearActiveContext();
    status_->setText(
        QStringLiteral("No Part Document is active."));
}

void CadWorkbench::resetRuntimeState() {
    deactivateDocument();
    document_view_states_.clear();
    status_->setText(
        QStringLiteral(
            "No Part Document is active."));
}

void CadWorkbench::forgetDocumentRuntimeState(
    const core::DocumentId& document_id) {
    if (document_session_ != nullptr &&
        document_session_->documentId() ==
            document_id) {
        deactivateDocument();
    }

    document_view_states_.erase(
        std::string{document_id.value()});
}

void CadWorkbench::applyProperties() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    core::DocumentProperties properties;
    properties.number = toUtf8(number_->text());
    properties.title = toUtf8(title_->text());
    properties.description =
        toUtf8(description_->toPlainText());
    properties.engineering_revision =
        toUtf8(engineering_revision_->text());

    const auto result = document_session->execute(
        application::SetDocumentPropertiesCommand{
            std::move(properties)});
    if (!result.ok()) {
        showFailure(result.diagnostic);
        refreshActiveContext();
        return;
    }

    refreshActiveContext();
    status_->setText(
        result.changed
            ? QStringLiteral(
                  "Properties changed — save is required.")
            : QStringLiteral("No authored property change."));
}

void CadWorkbench::startSketchTool() {
    if (activeDocumentSession() == nullptr ||
        sketch_support_pick_active_) {
        return;
    }

    if (active_sketch_id_) {
        status_->setText(
            QStringLiteral(
                "Finish the active Sketch before creating another one."));
        return;
    }

    sketch_support_pick_active_ = true;
    operations_placeholder_->setText(
        QStringLiteral(
            "Sketch: select XY, XZ or YZ Origin plane in the Tree or 3D Viewport."));
    status_->setText(
        QStringLiteral(
            "Sketch tool active — select an Origin plane."));
    syncActionState();
}

void CadWorkbench::cancelSketchTool() {
    if (!sketch_support_pick_active_) {
        return;
    }

    clearSketchRuntimeContext();
    status_->setText(
        QStringLiteral(
            "Sketch creation cancelled."));
    syncActionState();
}

void CadWorkbench::requestEditSketch(
    const sketch::SketchId& sketch_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        return;
    }

    if (active_sketch_id_) {
        if (*active_sketch_id_ == sketch_id) {
            status_->setText(
                QStringLiteral(
                    "This Sketch is already being edited."));
        } else {
            status_->setText(
                QStringLiteral(
                    "Finish the active Sketch before editing another one."));
        }
        return;
    }

    if (sketch_support_pick_active_) {
        clearSketchRuntimeContext();
    }

    if (document_session->document()
            .findSketch(sketch_id) == nullptr) {
        status_->setText(
            QStringLiteral(
                "Sketch is no longer available in the active Part."));
        syncActionState();
        return;
    }

    enterSketchEdit(sketch_id);
    status_->setText(
        QStringLiteral(
            "Sketch edit context opened in the 3D Viewport."));
}

void CadWorkbench::tryCreateSketchFromSupport(
    std::optional<core::BuiltinReferenceRole> support) {
    if (!sketch_support_pick_active_) {
        return;
    }

    if (!support) {
        return;
    }

    if (!part::isSketchOriginPlane(*support)) {
        status_->setText(
            QStringLiteral(
                "Sketch support must be XY, XZ or YZ Origin plane."));
        return;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    const auto created =
        document_session->execute(
            application::CreatePartSketchCommand{
                *support});
    if (!created.ok()) {
        showFailure(created.diagnostic);
        return;
    }

    if (!created.changed ||
        !created.sketch_id) {
        status_->setText(
            QStringLiteral(
                "Sketch was not created."));
        return;
    }

    sketch_support_pick_active_ = false;

    const auto created_id =
        *created.sketch_id;

    refreshActiveContext();
    enterSketchEdit(created_id);

    status_->setText(
        QStringLiteral(
            "Sketch created — editing in the 3D Viewport. "
            "Pan/Zoom/Orbit remain available."));
}

void CadWorkbench::enterSketchEdit(
    const sketch::SketchId& sketch_id) {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    const auto* sketch =
        document_session->document()
            .findSketch(sketch_id);
    if (sketch == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    active_sketch_id_ = sketch_id;
    sketch_edit_document_id_ =
        document_session->documentId();

    viewport_controller_->setSketchEditSketch(
        sketch_id);

    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->begin(
            *document_session,
            sketch_id);
    }

    if (viewport_ != nullptr) {
        const auto standard_view =
            standardViewForSketchSupport(
                sketch->support.builtin_plane);
        if (standard_view) {
            static_cast<void>(
                viewport_->setStandardView(
                    *standard_view));
        }
        viewport_->fitAll();
    }

    syncSketchInteractionUi();
    syncActionState();
}

void CadWorkbench::activateSketchSelect() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateSelect();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchLine() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateLine();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchCircle() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateCircle();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchArc() {
    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->activateArc();
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::activateSketchMove() {
    if (sketch_interaction_controller_ &&
        !sketch_interaction_controller_->activateMove()) {
        status_->setText(
            QStringLiteral("MOVE could not be activated."));
        return;
    }
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::finishSketchLine() {
    if (!sketch_interaction_controller_) {
        return;
    }

    const auto tool =
        sketch_interaction_controller_->tool();
    sketch_interaction_controller_->activateSelect();

    switch (tool) {
    case sketch::SketchTool::line:
        status_->setText(
            QStringLiteral("Line finished — Select active."));
        break;
    case sketch::SketchTool::circle:
        status_->setText(
            QStringLiteral("Circle finished — Select active."));
        break;
    case sketch::SketchTool::arc:
        status_->setText(
            QStringLiteral("Arc finished — Select active."));
        break;
    case sketch::SketchTool::select:
        break;
    }
}

void CadWorkbench::cancelSketchLine() {
    if (!sketch_interaction_controller_) {
        return;
    }

    const auto tool =
        sketch_interaction_controller_->tool();
    sketch_interaction_controller_->activateSelect();

    switch (tool) {
    case sketch::SketchTool::line:
        status_->setText(
            QStringLiteral(
                "Line cancelled — committed segments preserved."));
        break;
    case sketch::SketchTool::circle:
        status_->setText(
            QStringLiteral(
                "Circle cancelled — committed circles preserved."));
        break;
    case sketch::SketchTool::arc:
        status_->setText(
            QStringLiteral(
                "Arc cancelled — committed arcs preserved."));
        break;
    case sketch::SketchTool::select:
        break;
    }
}

void CadWorkbench::deleteSketchSelection() {
    if (!sketch_interaction_controller_ ||
        !sketch_interaction_controller_->deleteSelection()) {
        return;
    }

    status_->setText(
        QStringLiteral("Sketch selection deleted."));
}

void CadWorkbench::submitSketchCommandLine() {
    if (command_input_ == nullptr ||
        !sketch_interaction_controller_ ||
        !sketch_interaction_controller_->active()) {
        return;
    }

    const auto command =
        command_input_->text().trimmed().toUpper();
    if (command.isEmpty()) {
        return;
    }

    if (command == QStringLiteral("SELECT")) {
        activateSketchSelect();
    } else if (command == QStringLiteral("LINE")) {
        activateSketchLine();
    } else if (command == QStringLiteral("CIRCLE")) {
        activateSketchCircle();
    } else if (command == QStringLiteral("ARC")) {
        activateSketchArc();
    } else if (command == QStringLiteral("MOVE")) {
        activateSketchMove();
    } else {
        status_->setText(
            QStringLiteral("Unknown Sketch command."));
        return;
    }

    command_input_->clear();
    if (viewport_widget_ != nullptr) {
        viewport_widget_->setFocus(Qt::OtherFocusReason);
    }
}

void CadWorkbench::finishSketch() {
    if (!active_sketch_id_) {
        return;
    }

    clearSketchRuntimeContext();
    status_->setText(
        QStringLiteral(
            "Sketch edit finished. The Sketch remains authored in the Part."));
    syncActionState();
}

void CadWorkbench::clearSketchRuntimeContext() {
    sketch_support_pick_active_ = false;

    if (sketch_interaction_controller_) {
        sketch_interaction_controller_->end();
    }

    active_sketch_id_.reset();
    sketch_edit_document_id_.reset();

    if (viewport_controller_ != nullptr) {
        viewport_controller_->setSketchEditSketch(
            std::nullopt);
    }

    syncSketchInteractionUi();
}

void CadWorkbench::reconcileSketchRuntimeContext() {
    if (!active_sketch_id_) {
        return;
    }

    if (document_session_ == nullptr ||
        !sketch_edit_document_id_ ||
        document_session_->documentId() !=
            *sketch_edit_document_id_) {
        clearSketchRuntimeContext();
        return;
    }

    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    const auto* sketch =
        document_session->document()
            .findSketch(*active_sketch_id_);
    if (sketch == nullptr) {
        clearSketchRuntimeContext();
        return;
    }

    viewport_controller_->setSketchEditSketch(
        *active_sketch_id_);

    if (sketch_interaction_controller_) {
        if (!sketch_interaction_controller_->active()) {
            sketch_interaction_controller_->begin(
                *document_session,
                *active_sketch_id_);
        } else {
            static_cast<void>(
                sketch_interaction_controller_->
                    reconcileAfterHistory());
        }
    }

    syncSketchInteractionUi();
}

void CadWorkbench::undo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->cancelForHistory();
    }

    const auto result = document_session->undo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    status_->setText(
        result.changed
            ? QStringLiteral("Undo applied.")
            : QStringLiteral("Nothing to undo."));
}

void CadWorkbench::redo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    if (sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        sketch_interaction_controller_->cancelForHistory();
    }

    const auto result = document_session->redo();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    status_->setText(
        result.changed
            ? QStringLiteral("Redo applied.")
            : QStringLiteral("Nothing to redo."));
}

void CadWorkbench::save() {
    auto* document_session =
        activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result =
        document_session->save();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshActiveContext();
    status_->setText(
        QStringLiteral("Part saved."));
}

void CadWorkbench::closeActiveDocument() {
    if (document_session_ == nullptr ||
        !close_document_handler_) {
        return;
    }

    close_document_handler_(
        document_session_->documentId());
}

void CadWorkbench::refreshActiveContext() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) {
        clearActiveContext();
        return;
    }

    const auto& properties =
        document_session->document().properties();

    number_->setText(fromUtf8(properties.number));
    title_->setText(fromUtf8(properties.title));
    description_->setPlainText(
        fromUtf8(properties.description));
    engineering_revision_->setText(
        fromUtf8(properties.engineering_revision));

    std::error_code ec;
    const auto relative = std::filesystem::relative(
        document_session->path(),
        workspace_root_,
        ec);

    active_path_->setText(
        QStringLiteral("Path: ") +
        fromFilesystemPath(
            ec ? document_session->path() : relative));

    active_id_->setText(
        QStringLiteral("DocumentId: ") +
        fromUtf8(document_session->documentId().value()));

    number_->setEnabled(true);
    title_->setEnabled(true);
    description_->setEnabled(true);
    engineering_revision_->setEnabled(true);

    viewport_controller_->setDocumentSession(document_session);
    reconcileSketchRuntimeContext();
    syncActionState();
    notifyDocumentStateChanged();
}

void CadWorkbench::clearActiveContext() {
    clearSketchRuntimeContext();
    active_path_->setText(QStringLiteral("No Part is open."));
    active_id_->clear();

    number_->clear();
    title_->clear();
    description_->clear();
    engineering_revision_->clear();

    number_->setEnabled(false);
    title_->setEnabled(false);
    description_->setEnabled(false);
    engineering_revision_->setEnabled(false);

    viewport_controller_->clear();
    syncActionState();
}

void CadWorkbench::captureActiveViewState() {
    if (viewport_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto state = viewport_->cameraState();
    if (!state) return;

    document_view_states_.insert_or_assign(
        std::string{
            document_session_->documentId().value()},
        *state);
}

void CadWorkbench::restoreActiveViewState() {
    if (viewport_ == nullptr ||
        document_session_ == nullptr) {
        return;
    }

    const auto key =
        std::string{
            document_session_->documentId().value()};
    const auto found =
        document_view_states_.find(key);

    if (found != document_view_states_.end()) {
        static_cast<void>(
            viewport_->setCameraState(found->second));
        return;
    }

    viewer::CameraState initial;
    static_cast<void>(
        viewport_->setCameraState(initial));
}

void CadWorkbench::refreshPropertiesContext(
    std::optional<core::BuiltinReferenceRole> primary) {
    if (properties_stack_ == nullptr) return;

    if (!primary) {
        properties_stack_->setCurrentWidget(
            document_properties_page_);
        return;
    }

    QString name;
    QString kind;
    QString identity;

    switch (*primary) {
    case core::BuiltinReferenceRole::origin_point:
        name = QStringLiteral("Origin Point");
        kind = QStringLiteral("Point");
        identity = QStringLiteral(
            "BuiltinReference::OriginPoint");
        break;
    case core::BuiltinReferenceRole::x_axis:
        name = QStringLiteral("X Axis");
        kind = QStringLiteral("Axis");
        identity = QStringLiteral(
            "BuiltinReference::XAxis");
        break;
    case core::BuiltinReferenceRole::y_axis:
        name = QStringLiteral("Y Axis");
        kind = QStringLiteral("Axis");
        identity = QStringLiteral(
            "BuiltinReference::YAxis");
        break;
    case core::BuiltinReferenceRole::z_axis:
        name = QStringLiteral("Z Axis");
        kind = QStringLiteral("Axis");
        identity = QStringLiteral(
            "BuiltinReference::ZAxis");
        break;
    case core::BuiltinReferenceRole::xy_plane:
        name = QStringLiteral("XY Plane");
        kind = QStringLiteral("Plane");
        identity = QStringLiteral(
            "BuiltinReference::XYPlane");
        break;
    case core::BuiltinReferenceRole::xz_plane:
        name = QStringLiteral("XZ Plane");
        kind = QStringLiteral("Plane");
        identity = QStringLiteral(
            "BuiltinReference::XZPlane");
        break;
    case core::BuiltinReferenceRole::yz_plane:
        name = QStringLiteral("YZ Plane");
        kind = QStringLiteral("Plane");
        identity = QStringLiteral(
            "BuiltinReference::YZPlane");
        break;
    }

    reference_name_->setText(name);
    reference_kind_->setText(kind);
    reference_identity_->setText(identity);

    const auto* document_session =
        activeDocumentSession();
    const bool visible =
        document_session != nullptr &&
        document_session->document()
            .builtinReferenceVisible(*primary);

    reference_visibility_->setText(
        visible
            ? QStringLiteral("Shown")
            : QStringLiteral("Hidden"));

    properties_stack_->setCurrentWidget(
        reference_properties_page_);
}

bool CadWorkbench::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (event != nullptr &&
        event->type() == QEvent::MouseButtonPress &&
        watched == viewport_widget_ &&
        sketch_interaction_controller_ &&
        sketch_interaction_controller_->active()) {
        auto* mouse_event =
            static_cast<QMouseEvent*>(event);
        if (mouse_event->button() == Qt::RightButton &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::move &&
            sketch_interaction_controller_->moveStage() ==
                sketch::MoveStage::select_objects) {
            if (sketch_interaction_controller_->
                    completeMoveSelection()) {
                status_->setText(
                    QStringLiteral(
                        "MOVE objects accepted — specify Base Point."));
            }
            return true;
        }
    }

    if (event != nullptr &&
        event->type() == QEvent::KeyPress) {
        auto* key_event =
            static_cast<QKeyEvent*>(event);

        if (watched == command_input_) {
            if (key_event->key() == Qt::Key_Escape) {
                command_input_->clear();
                if (viewport_widget_ != nullptr) {
                    viewport_widget_->setFocus(
                        Qt::OtherFocusReason);
                }
                return true;
            }
        } else if (watched == viewport_widget_ &&
                   sketch_interaction_controller_ &&
                   sketch_interaction_controller_->active()) {
            if (key_event->key() == Qt::Key_Delete) {
                deleteSketchSelection();
                return true;
            }

            if (key_event->key() == Qt::Key_Escape) {
                if (sketch_interaction_controller_->escape()) {
                    status_->setText(
                        QStringLiteral(
                            "Sketch interaction cancelled."));
                }
                return true;
            }

            if ((key_event->key() == Qt::Key_Return ||
                 key_event->key() == Qt::Key_Enter) &&
                sketch_interaction_controller_->
                    directManipulationActive()) {
                if (sketch_interaction_controller_->
                        commitDirectManipulation()) {
                    status_->setText(
                        QStringLiteral(
                            "Sketch edit committed."));
                }
                return true;
            }

            if (sketch_interaction_controller_->tool() ==
                    sketch::SketchTool::move) {
                const auto stage =
                    sketch_interaction_controller_->moveStage();

                if ((key_event->key() == Qt::Key_Return ||
                     key_event->key() == Qt::Key_Enter ||
                     key_event->key() == Qt::Key_Space) &&
                    stage ==
                        sketch::MoveStage::select_objects) {
                    if (sketch_interaction_controller_->
                            completeMoveSelection()) {
                        status_->setText(
                            QStringLiteral(
                                "MOVE objects accepted — specify Base Point."));
                    }
                    return true;
                }

                if ((key_event->key() == Qt::Key_Return ||
                     key_event->key() == Qt::Key_Enter) &&
                    stage ==
                        sketch::MoveStage::await_destination) {
                    static_cast<void>(
                        sketch_interaction_controller_->
                            commitMove());
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(watched, event);
}

void CadWorkbench::syncSketchInteractionUi() {
    const bool editing =
        sketch_interaction_controller_ &&
        sketch_interaction_controller_->active();

    if (sketch_button_ != nullptr) {
        sketch_button_->setVisible(!editing);
    }

    if (select_sketch_button_ != nullptr) {
        select_sketch_button_->setVisible(editing);
        select_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::select);
    }

    if (line_sketch_button_ != nullptr) {
        line_sketch_button_->setVisible(editing);
        line_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::line);
    }

    if (circle_sketch_button_ != nullptr) {
        circle_sketch_button_->setVisible(editing);
        circle_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::circle);
    }

    if (arc_sketch_button_ != nullptr) {
        arc_sketch_button_->setVisible(editing);
        arc_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::arc);
    }

    if (move_sketch_button_ != nullptr) {
        move_sketch_button_->setVisible(editing);
        move_sketch_button_->setChecked(
            editing &&
            sketch_interaction_controller_->tool() ==
                sketch::SketchTool::move);
    }

    if (command_line_widget_ != nullptr) {
        command_line_widget_->setVisible(editing);
    }

    if (!editing) {
        if (operations_placeholder_ != nullptr &&
            !sketch_support_pick_active_) {
            operations_placeholder_->setText(
                QStringLiteral("Part modeling context."));
        }
        if (delete_selection_button_ != nullptr) {
            delete_selection_button_->setVisible(false);
        }
        if (finish_line_button_ != nullptr) {
            finish_line_button_->setVisible(false);
        }
        if (cancel_line_button_ != nullptr) {
            cancel_line_button_->setVisible(false);
        }
        return;
    }

    const auto tool =
        sketch_interaction_controller_->tool();

    if (tool == sketch::SketchTool::select) {
        const auto selected =
            sketch_interaction_controller_->selectedCount();
        operations_placeholder_->setText(
            QStringLiteral("Select — %1 entit%2 selected")
                .arg(static_cast<qulonglong>(selected))
                .arg(selected == 1U
                         ? QStringLiteral("y")
                         : QStringLiteral("ies")));
        delete_selection_button_->setVisible(true);
        delete_selection_button_->setEnabled(
            selected > 0U);
        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);
        command_prompt_->setText(
            QStringLiteral("Command: SELECT"));
        return;
    }

    if (tool == sketch::SketchTool::move) {
        delete_selection_button_->setVisible(false);
        finish_line_button_->setVisible(false);
        cancel_line_button_->setVisible(false);

        const auto stage =
            sketch_interaction_controller_->moveStage();
        if (stage == sketch::MoveStage::select_objects) {
            operations_placeholder_->setText(
                QStringLiteral(
                    "Move — Select objects; Enter/Space/RMB to continue"));
            command_prompt_->setText(
                QStringLiteral(
                    "Command: MOVE — Select objects"));
        } else if (
            stage ==
            sketch::MoveStage::await_destination) {
            operations_placeholder_->setText(
                QStringLiteral(
                    "Move — Specify destination point"));
            command_prompt_->setText(
                QStringLiteral(
                    "Command: MOVE — Specify destination point"));
        } else {
            operations_placeholder_->setText(
                QStringLiteral(
                    "Move — Specify Base Point"));
            command_prompt_->setText(
                QStringLiteral(
                    "Command: MOVE — Specify Base Point"));
        }
        return;
    }

    delete_selection_button_->setVisible(false);
    finish_line_button_->setVisible(true);
    cancel_line_button_->setVisible(true);

    if (tool == sketch::SketchTool::line) {
        finish_line_button_->setText(
            QStringLiteral("Finish Line"));
        cancel_line_button_->setText(
            QStringLiteral("Cancel Line"));

        const auto stage =
            sketch_interaction_controller_->lineStage();
        const bool next =
            stage &&
            *stage ==
                sketch::LineStage::await_next_point;

        operations_placeholder_->setText(
            next
                ? QStringLiteral("Line — Specify next point")
                : QStringLiteral("Line — Specify first point"));
        command_prompt_->setText(
            next
                ? QStringLiteral(
                      "Command: LINE — Specify next point")
                : QStringLiteral(
                      "Command: LINE — Specify first point"));
        return;
    }

    if (tool == sketch::SketchTool::circle) {
        finish_line_button_->setText(
            QStringLiteral("Finish Circle"));
        cancel_line_button_->setText(
            QStringLiteral("Cancel Circle"));

        const auto stage =
            sketch_interaction_controller_->circleStage();
        const bool radius =
            stage &&
            *stage ==
                sketch::CircleStage::await_radius;

        operations_placeholder_->setText(
            radius
                ? QStringLiteral("Circle — Specify radius")
                : QStringLiteral("Circle — Specify center"));
        command_prompt_->setText(
            radius
                ? QStringLiteral(
                      "Command: CIRCLE — Specify radius")
                : QStringLiteral(
                      "Command: CIRCLE — Specify center"));
        return;
    }

    finish_line_button_->setText(
        QStringLiteral("Finish Arc"));
    cancel_line_button_->setText(
        QStringLiteral("Cancel Arc"));

    const auto stage =
        sketch_interaction_controller_->arcStage();
    if (stage &&
        *stage == sketch::ArcStage::await_through) {
        operations_placeholder_->setText(
            QStringLiteral("Arc — Specify through point"));
        command_prompt_->setText(
            QStringLiteral(
                "Command: ARC — Specify through point"));
    } else if (
        stage &&
        *stage == sketch::ArcStage::await_end) {
        operations_placeholder_->setText(
            QStringLiteral("Arc — Specify end point"));
        command_prompt_->setText(
            QStringLiteral(
                "Command: ARC — Specify end point"));
    } else {
        operations_placeholder_->setText(
            QStringLiteral("Arc — Specify start point"));
        command_prompt_->setText(
            QStringLiteral(
                "Command: ARC — Specify start point"));
    }
}

void CadWorkbench::syncActionState() {
    const auto* document_session = activeDocumentSession();
    const bool active = document_session != nullptr;

    apply_button_->setEnabled(active);
    undo_button_->setEnabled(
        active && document_session->canUndo());
    redo_button_->setEnabled(
        active && document_session->canRedo());
    save_button_->setEnabled(
        active && document_session->needsSave());
    close_document_button_->setEnabled(active);

    const bool editing_sketch =
        active &&
        active_sketch_id_.has_value() &&
        sketch_edit_document_id_.has_value() &&
        document_session_ != nullptr &&
        *sketch_edit_document_id_ ==
            document_session_->documentId();

    sketch_button_->setText(
        QStringLiteral("Sketch"));
    sketch_button_->setVisible(!editing_sketch);
    sketch_button_->setEnabled(
        active &&
        !editing_sketch &&
        !sketch_support_pick_active_);

    select_sketch_button_->setVisible(
        editing_sketch);
    line_sketch_button_->setVisible(
        editing_sketch);
    circle_sketch_button_->setVisible(
        editing_sketch);
    arc_sketch_button_->setVisible(
        editing_sketch);
    move_sketch_button_->setVisible(
        editing_sketch);

    cancel_sketch_button_->setVisible(
        active && sketch_support_pick_active_);
    cancel_sketch_button_->setEnabled(
        active && sketch_support_pick_active_);

    finish_sketch_button_->setVisible(
        editing_sketch);
    finish_sketch_button_->setEnabled(
        editing_sketch);

    if (command_line_widget_ != nullptr) {
        command_line_widget_->setVisible(
            editing_sketch);
    }

    syncSketchInteractionUi();
}

void CadWorkbench::notifyDocumentStateChanged() {
    if (document_session_ != nullptr &&
        document_state_changed_handler_) {
        document_state_changed_handler_(
            document_session_->documentId());
    }
}

void CadWorkbench::showFailure(
    const application::DocumentSessionDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral("\n\nPath: ") +
            fromFilesystemPath(diagnostic.path);
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
}

} // namespace simplesolid2::ui
