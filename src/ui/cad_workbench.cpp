#include "cad_workbench.hpp"
#include "cad_workbench_shell.hpp"
#include "open_document_dialog.hpp"
#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"
#include "view_cube_widget.hpp"
#include "workspace_location_dialog.hpp"

#include <QFormLayout>
#include <QGridLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTabBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include <algorithm>
#include <iomanip>
#include <sstream>
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

QString documentIndexStateLabel(application::DocumentIndexState state) {
    switch (state) {
    case application::DocumentIndexState::resolved:
        return QStringLiteral("Ready");
    case application::DocumentIndexState::identity_conflict:
        return QStringLiteral("Identity conflict");
    case application::DocumentIndexState::invalid:
        return QStringLiteral("Invalid Part");
    }
    return QStringLiteral("Unavailable");
}

QString documentCandidateName(
    const application::DocumentIndexEntry& entry) {
    if (!entry.title.empty()) {
        return fromUtf8(entry.title);
    }

    if (!entry.relative_paths.empty()) {
        auto name = fromFilesystemPath(
            entry.relative_paths.front().stem());
        if (!name.isEmpty()) return name;
    }

    return QStringLiteral("<untitled>");
}

QString documentCandidateLocations(
    const application::DocumentIndexEntry& entry) {
    QString result;
    for (const auto& path : entry.relative_paths) {
        if (!result.isEmpty()) {
            result += QStringLiteral("\n");
        }
        result += fromFilesystemPath(path);
    }
    return result;
}

std::vector<OpenDocumentCandidate> openDocumentCandidates(
    const application::DocumentWorkspaceIndex& index) {
    std::vector<OpenDocumentCandidate> result;
    result.reserve(index.entries().size());

    for (const auto& entry : index.entries()) {
        const bool openable =
            entry.state ==
                application::DocumentIndexState::resolved &&
            entry.document_id.has_value();

        QString tooltip;
        if (entry.document_id) {
            tooltip =
                QStringLiteral("DocumentId: ") +
                fromUtf8(entry.document_id->value());
        }
        if (!entry.diagnostic.empty()) {
            if (!tooltip.isEmpty()) {
                tooltip += QStringLiteral("\n");
            }
            tooltip += fromUtf8(entry.diagnostic);
        }

        result.push_back(
            OpenDocumentCandidate{
                QStringLiteral("Part"),
                documentCandidateName(entry),
                documentCandidateLocations(entry),
                documentIndexStateLabel(entry.state),
                std::move(tooltip),
                entry.document_id,
                openable});
    }

    return result;
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

QString partDisplayName(const application::DocumentSession& session) {
    const auto& title = session.document().properties().title;
    if (!title.empty()) {
        return fromUtf8(title);
    }

    const auto stem = session.path().stem();
    auto fallback = fromFilesystemPath(stem);
    if (fallback.isEmpty()) {
        fallback = QStringLiteral("<untitled Part>");
    }
    return fallback;
}

} // namespace

CadWorkbench::CadWorkbench(QWidget* parent)
    : CadWorkbench{ViewportFactory{}, parent} {}

CadWorkbench::CadWorkbench(
    ViewportFactory viewport_factory,
    QWidget* parent)
    : QWidget{parent},
      viewport_factory_{std::move(viewport_factory)} {
    buildUi();
    clearProjectSession();
}

void CadWorkbench::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    shell_ = new CadWorkbenchShell(this);
    shell_->setObjectName(QStringLiteral("cadWorkbenchShell"));
    root->addWidget(shell_, 1);

    auto& lifecycle_actions = shell_->documentActionsLayout();

    new_part_button_ =
        new QPushButton(QStringLiteral("New Part…"), shell_);
    new_part_button_->setObjectName(QStringLiteral("newPartButton"));

    open_document_button_ =
        new QPushButton(QStringLiteral("Open…"), shell_);
    open_document_button_->setObjectName(
        QStringLiteral("openDocumentButton"));

    refresh_button_ =
        new QPushButton(QStringLiteral("Refresh"), shell_);
    refresh_button_->setObjectName(
        QStringLiteral("refreshDocumentsButton"));

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

    lifecycle_actions.addWidget(new_part_button_);
    lifecycle_actions.addWidget(open_document_button_);
    lifecycle_actions.addWidget(refresh_button_);
    lifecycle_actions.addStretch(1);
    lifecycle_actions.addWidget(undo_button_);
    lifecycle_actions.addWidget(redo_button_);
    lifecycle_actions.addWidget(save_button_);
    lifecycle_actions.addWidget(close_document_button_);

    document_tree_ = &shell_->documentTree();
    document_tabs_ = &shell_->documentTabs();
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

        viewport_widget->setObjectName(
            QStringLiteral("documentViewport"));
        if (viewport_widget->parentWidget() != editor_container) {
            viewport_widget->setParent(editor_container);
        }

        editor_layout->addWidget(
            viewport_widget,
            0,
            0);

        view_cube_ =
            new ViewCubeWidget(
                viewport_,
                editor_container,
                viewport_widget);
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

    cancel_sketch_button_ =
        new QPushButton(
            QStringLiteral("Cancel"),
            operations_content);
    cancel_sketch_button_->setObjectName(
        QStringLiteral("cancelSketchButton"));
    operations_layout->addWidget(
        cancel_sketch_button_);

    finish_sketch_button_ =
        new QPushButton(
            QStringLiteral("Finish Sketch"),
            operations_content);
    finish_sketch_button_->setObjectName(
        QStringLiteral("finishSketchButton"));
    operations_layout->addWidget(
        finish_sketch_button_);

    operations_placeholder_ = new QLabel(
        QStringLiteral("No active tool."),
        operations_content);
    operations_placeholder_->setObjectName(
        QStringLiteral("operationsPlaceholder"));
    operations_placeholder_->setWordWrap(true);
    operations_layout->addWidget(operations_placeholder_);
    operations_layout->addStretch(1);

    shell_->setOperationsContent(operations_content);

    QObject::connect(
        new_part_button_,
        &QPushButton::clicked,
        this,
        [this] { newPart(); });
    QObject::connect(
        open_document_button_,
        &QPushButton::clicked,
        this,
        [this] { openDocument(); });
    QObject::connect(
        refresh_button_,
        &QPushButton::clicked,
        this,
        [this] { refreshWorkspaceIndex(); });
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
        document_tabs_,
        &QTabBar::currentChanged,
        this,
        [this](int index) { activateTab(index); });
    QObject::connect(
        document_tabs_,
        &QTabBar::tabCloseRequested,
        this,
        [this](int index) { closeTab(index); });
}

void CadWorkbench::setProjectSession(
    application::ProjectSession* session) {
    clearSketchRuntimeContext();
    session_ = session;
    active_document_id_.reset();
    document_view_states_.clear();
    viewport_controller_->resetRuntimeState();
    syncOpenTabs();
    refreshWorkspaceIndex();

    if (document_tabs_->count() > 0) {
        activateTab(document_tabs_->currentIndex());
    } else {
        clearActiveContext();
        status_->setText(
            QStringLiteral(
                "Project open. Create or open a Part Document."));
    }
}

void CadWorkbench::clearProjectSession() {
    clearSketchRuntimeContext();
    session_ = nullptr;
    active_document_id_.reset();
    document_view_states_.clear();
    viewport_controller_->resetRuntimeState();

    {
        const QSignalBlocker blocked{document_tabs_};
        while (document_tabs_->count() > 0) {
            document_tabs_->removeTab(0);
        }
        document_tabs_->setCurrentIndex(-1);
    }

    new_part_button_->setEnabled(false);
    open_document_button_->setEnabled(false);
    refresh_button_->setEnabled(false);
    clearActiveContext();
    status_->setText(QStringLiteral("No Project is open."));
}

void CadWorkbench::refreshWorkspaceIndex() {
    if (session_ == nullptr) {
        status_->setText(QStringLiteral("No Project is open."));
        return;
    }

    new_part_button_->setEnabled(true);
    open_document_button_->setEnabled(true);
    refresh_button_->setEnabled(true);

    const auto refreshed = session_->refreshDocuments();
    if (!refreshed.ok()) {
        status_->setText(
            QStringLiteral("Document discovery warning: ") +
            fromUtf8(refreshed.diagnostic.message));
        return;
    }

    status_->setText(
        QStringLiteral("%1 native Part entr%2 available in Workspace.")
            .arg(
                static_cast<qulonglong>(
                    session_->documentIndex().entries().size()))
            .arg(
                session_->documentIndex().entries().size() == 1U
                    ? QStringLiteral("y")
                    : QStringLiteral("ies")));

    for (const auto& id : session_->openDocumentIds()) {
        updateTabPresentation(id);
    }
}

void CadWorkbench::syncOpenTabs() {
    const QSignalBlocker blocked{document_tabs_};

    while (document_tabs_->count() > 0) {
        document_tabs_->removeTab(0);
    }

    if (session_ == nullptr) {
        document_tabs_->setCurrentIndex(-1);
        return;
    }

    for (const auto& id : session_->openDocumentIds()) {
        ensureDocumentTab(id);
    }

    document_tabs_->setCurrentIndex(
        document_tabs_->count() > 0 ? 0 : -1);
}

void CadWorkbench::ensureDocumentTab(
    const core::DocumentId& document_id) {
    if (session_ == nullptr || tabIndexFor(document_id) >= 0) return;

    auto* document_session = session_->documentSession(document_id);
    if (document_session == nullptr) return;

    const int index = document_tabs_->addTab(
        partDisplayName(*document_session));
    document_tabs_->setTabData(
        index,
        fromUtf8(document_id.value()));
    updateTabPresentation(document_id);
}

bool CadWorkbench::activateDocument(
    const core::DocumentId& document_id) {
    if (session_ == nullptr) return false;

    auto opened = session_->openDocument(document_id);
    if (!opened.ok()) {
        showFailure(opened.diagnostic);
        return false;
    }

    ensureDocumentTab(document_id);
    const int index = tabIndexFor(document_id);
    if (index < 0) return false;

    document_tabs_->setCurrentIndex(index);
    if (document_tabs_->currentIndex() == index) {
        activateTab(index);
    }

    status_->setText(
        opened.reused_session
            ? QStringLiteral(
                  "Part was already open; activated existing DocumentSession.")
            : QStringLiteral("Part opened."));
    return true;
}

void CadWorkbench::activateTab(int index) {
    const auto requested_id =
        tabDocumentId(index);

    if (active_document_id_ &&
        (!requested_id ||
         *requested_id != *active_document_id_)) {
        clearSketchRuntimeContext();
    }

    captureActiveViewState();

    const auto id = requested_id;
    if (!id || session_ == nullptr ||
        session_->documentSession(*id) == nullptr) {
        active_document_id_.reset();
        clearActiveContext();
        return;
    }

    active_document_id_ = *id;
    restoreActiveViewState();
    refreshActiveContext();
}

int CadWorkbench::tabIndexFor(
    const core::DocumentId& document_id) const {
    const auto id = fromUtf8(document_id.value());
    for (int index = 0; index < document_tabs_->count(); ++index) {
        if (document_tabs_->tabData(index).toString() == id) {
            return index;
        }
    }
    return -1;
}

std::optional<core::DocumentId>
CadWorkbench::tabDocumentId(int index) const {
    if (index < 0 || index >= document_tabs_->count()) {
        return std::nullopt;
    }

    const auto encoded =
        toUtf8(document_tabs_->tabData(index).toString());
    return core::DocumentId::parse(encoded);
}

std::filesystem::path CadWorkbench::defaultPartPath() const {
    if (session_ == nullptr) return "Part001.ss2part";

    for (unsigned index = 1U; index <= 9999U; ++index) {
        std::ostringstream name;
        name << "Part" << std::setw(3) << std::setfill('0')
             << index << ".ss2part";

        const auto relative = std::filesystem::path{name.str()};
        std::error_code ec;
        if (!std::filesystem::exists(
                session_->workspaceRoot() / relative,
                ec) &&
            !ec) {
            return relative;
        }
    }

    return "Part.ss2part";
}

void CadWorkbench::newPart() {
    if (session_ == nullptr) return;

    WorkspaceLocationDialog dialog{
        session_->workspaceRoot(),
        QStringLiteral("Part"),
        QStringLiteral(".ss2part"),
        fromFilesystemPath(
            defaultPartPath().filename()),
        this};

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const auto relative =
        dialog.selectedRelativeFilePath();
    if (!relative) return;

    auto created =
        session_->createPart(*relative);
    if (!created.ok()) {
        showFailure(created.diagnostic);
        refreshWorkspaceIndex();
        return;
    }

    ensureDocumentTab(
        created.session->documentId());
    const int index =
        tabIndexFor(
            created.session->documentId());
    document_tabs_->setCurrentIndex(index);
    activateTab(index);
    refreshWorkspaceIndex();
    status_->setText(
        QStringLiteral(
            "New Part created and saved."));
}

void CadWorkbench::openDocument() {
    if (session_ == nullptr) return;

    const auto refreshed =
        session_->refreshDocuments();
    if (!refreshed.ok()) {
        status_->setText(
            QStringLiteral(
                "Document discovery warning: ") +
            fromUtf8(
                refreshed.diagnostic.message));
    }

    OpenDocumentDialog dialog{
        openDocumentCandidates(
            session_->documentIndex()),
        this};

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    const auto document_id =
        dialog.selectedDocumentId();
    if (!document_id) return;

    static_cast<void>(
        activateDocument(*document_id));
}

application::DocumentSession*
CadWorkbench::activeDocumentSession() noexcept {
    if (session_ == nullptr || !active_document_id_) return nullptr;
    return session_->documentSession(*active_document_id_);
}

const application::DocumentSession*
CadWorkbench::activeDocumentSession() const noexcept {
    if (session_ == nullptr || !active_document_id_) return nullptr;
    return session_->documentSession(*active_document_id_);
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
    if (activeDocumentSession() == nullptr) {
        return;
    }

    if (sketch_support_pick_active_) {
        clearSketchRuntimeContext();
        status_->setText(
            QStringLiteral(
                "Sketch creation cancelled."));
        syncActionState();
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
    if (document_session == nullptr ||
        !active_document_id_) {
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
        *active_document_id_;

    viewport_controller_->setSketchEditPlacement(
        sketch->placement);

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

    operations_placeholder_->setText(
        QStringLiteral(
            "Sketch edit context is active in the same 3D Viewport. "
            "This SK-01 Sketch is intentionally empty; 2D entities come later."));
    syncActionState();
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
    active_sketch_id_.reset();
    sketch_edit_document_id_.reset();

    if (viewport_controller_ != nullptr) {
        viewport_controller_->setSketchEditPlacement(
            std::nullopt);
    }

    if (operations_placeholder_ != nullptr) {
        operations_placeholder_->setText(
            QStringLiteral(
                "Sketch creates an empty Part-hosted Sketch on an Origin plane."));
    }
}

void CadWorkbench::reconcileSketchRuntimeContext() {
    if (!active_sketch_id_) {
        return;
    }

    if (!active_document_id_ ||
        !sketch_edit_document_id_ ||
        *active_document_id_ !=
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

    viewport_controller_->setSketchEditPlacement(
        sketch->placement);

    operations_placeholder_->setText(
        QStringLiteral(
            "Sketch edit context is active in the same 3D Viewport. "
            "This SK-01 Sketch is intentionally empty; 2D entities come later."));
}

void CadWorkbench::undo() {
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

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
    auto* document_session = activeDocumentSession();
    if (document_session == nullptr) return;

    const auto result = document_session->save();
    if (!result.ok()) {
        showFailure(result.diagnostic);
        return;
    }

    refreshWorkspaceIndex();
    refreshActiveContext();
    status_->setText(QStringLiteral("Part saved."));
}

void CadWorkbench::closeActiveDocument() {
    if (!active_document_id_) return;
    const int index = tabIndexFor(*active_document_id_);
    if (index >= 0) {
        closeTab(index);
    }
}

void CadWorkbench::closeTab(int index) {
    if (session_ == nullptr) return;

    const auto id = tabDocumentId(index);
    if (!id) return;

    auto* document_session = session_->documentSession(*id);
    if (document_session == nullptr) {
        document_tabs_->removeTab(index);
        return;
    }

    bool discard = false;
    if (document_session->needsSave()) {
        QMessageBox box{
            QMessageBox::Warning,
            QStringLiteral("Unsaved Part"),
            QStringLiteral(
                "This Part contains unsaved authored changes.\n"
                "Save them before closing?"),
            QMessageBox::NoButton,
            this};

        auto* save_button =
            box.addButton(
                QStringLiteral("Save"),
                QMessageBox::AcceptRole);
        auto* discard_button =
            box.addButton(
                QStringLiteral("Discard"),
                QMessageBox::DestructiveRole);
        auto* cancel_button =
            box.addButton(
                QStringLiteral("Cancel"),
                QMessageBox::RejectRole);

        box.exec();

        if (box.clickedButton() == cancel_button ||
            box.clickedButton() == nullptr) {
            return;
        }

        if (box.clickedButton() == save_button) {
            const auto saved = document_session->save();
            if (!saved.ok()) {
                showFailure(saved.diagnostic);
                return;
            }
        } else if (box.clickedButton() == discard_button) {
            discard = true;
        }
    }

    const bool closing_active =
        active_document_id_.has_value() &&
        *active_document_id_ == *id;

    // Runtime controllers keep non-owning DocumentSession pointers.
    // Detach them while the owning ProjectSession still owns the
    // session; closeDocument() may erase it immediately.
    if (closing_active) {
        clearSketchRuntimeContext();
        viewport_controller_->clear();
    }

    if (!session_->closeDocument(*id, discard)) {
        if (closing_active) {
            refreshActiveContext();
        }
        status_->setText(
            QStringLiteral(
                "Part remains open because it still has unsaved changes."));
        return;
    }

    document_view_states_.erase(
        std::string{id->value()});

    int next_index = -1;
    {
        const QSignalBlocker blocked{document_tabs_};
        document_tabs_->removeTab(index);
        if (document_tabs_->count() > 0) {
            next_index = std::min(
                index,
                document_tabs_->count() - 1);
            document_tabs_->setCurrentIndex(next_index);
        } else {
            document_tabs_->setCurrentIndex(-1);
        }
    }

    if (closing_active) {
        active_document_id_.reset();
        if (next_index >= 0) {
            activateTab(next_index);
        } else {
            clearActiveContext();
        }
    }

    status_->setText(QStringLiteral("Part closed."));
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
        session_->workspaceRoot(),
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
    updateTabPresentation(document_session->documentId());
    syncActionState();
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
        !active_document_id_.has_value()) {
        return;
    }

    const auto state = viewport_->cameraState();
    if (!state) return;

    document_view_states_.insert_or_assign(
        std::string{active_document_id_->value()},
        *state);
}

void CadWorkbench::restoreActiveViewState() {
    if (viewport_ == nullptr ||
        !active_document_id_.has_value()) {
        return;
    }

    const auto key =
        std::string{active_document_id_->value()};
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
        active_document_id_.has_value() &&
        *sketch_edit_document_id_ ==
            *active_document_id_;

    sketch_button_->setText(
        sketch_support_pick_active_
            ? QStringLiteral("Cancel Sketch")
            : QStringLiteral("Sketch"));
    sketch_button_->setEnabled(
        active && !editing_sketch);
    finish_sketch_button_->setEnabled(
        editing_sketch);

    new_part_button_->setEnabled(session_ != nullptr);
    open_document_button_->setEnabled(session_ != nullptr);
    refresh_button_->setEnabled(session_ != nullptr);
}

void CadWorkbench::updateTabPresentation(
    const core::DocumentId& document_id) {
    if (session_ == nullptr) return;

    const int index = tabIndexFor(document_id);
    if (index < 0) return;

    const auto* document_session =
        session_->documentSession(document_id);
    if (document_session == nullptr) return;

    auto label = partDisplayName(*document_session);
    if (document_session->needsSave()) {
        label += QStringLiteral(" *");
    }

    document_tabs_->setTabText(index, label);
    document_tabs_->setTabToolTip(
        index,
        QStringLiteral("DocumentId: ") +
            fromUtf8(document_id.value()) +
            QStringLiteral("\nPath: ") +
            fromFilesystemPath(document_session->path()));
}

ProjectCloseDisposition CadWorkbench::prepareProjectClose() {
    if (session_ == nullptr ||
        !session_->hasDirtyDocuments()) {
        return ProjectCloseDisposition::clean;
    }

    QMessageBox box{
        QMessageBox::Warning,
        QStringLiteral("Unsaved Parts"),
        QStringLiteral(
            "One or more open Parts contain unsaved authored changes.\n"
            "Save all changes before closing the Project?"),
        QMessageBox::NoButton,
        this};

    auto* save_button =
        box.addButton(
            QStringLiteral("Save All"),
            QMessageBox::AcceptRole);
    auto* discard_button =
        box.addButton(
            QStringLiteral("Discard"),
            QMessageBox::DestructiveRole);
    auto* cancel_button =
        box.addButton(
            QStringLiteral("Cancel"),
            QMessageBox::RejectRole);

    box.exec();

    if (box.clickedButton() == cancel_button ||
        box.clickedButton() == nullptr) {
        return ProjectCloseDisposition::cancel;
    }

    if (box.clickedButton() == discard_button) {
        return ProjectCloseDisposition::discard;
    }

    if (box.clickedButton() == save_button) {
        const auto saved =
            session_->saveAllDirtyDocuments();
        if (!saved.ok()) {
            showFailure(saved.diagnostic);
            return ProjectCloseDisposition::cancel;
        }

        for (const auto& id : session_->openDocumentIds()) {
            updateTabPresentation(id);
        }
        syncActionState();
        return ProjectCloseDisposition::clean;
    }

    return ProjectCloseDisposition::cancel;
}

void CadWorkbench::showFailure(
    const application::ProjectDocumentDiagnostic& diagnostic) {
    auto message = fromUtf8(diagnostic.message);

    if (!diagnostic.path.empty()) {
        message +=
            QStringLiteral("\n\nPath: ") +
            fromFilesystemPath(diagnostic.path);
    }

    if (!diagnostic.candidates.empty()) {
        message += QStringLiteral(
            "\n\nConflicting locations:");
        for (const auto& path : diagnostic.candidates) {
            message +=
                QStringLiteral("\n• ") +
                fromFilesystemPath(path);
        }
    }

    QMessageBox::warning(
        this,
        QStringLiteral("Part Document"),
        message);
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
