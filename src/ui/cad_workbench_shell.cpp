#include "cad_workbench_shell.hpp"

#include <QAbstractItemView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace simplesolid2::ui {

CadWorkbenchShell::CadWorkbenchShell(QWidget* parent)
    : QWidget{parent} {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(5);

    auto* document_top_row = new QWidget(this);
    document_top_row->setObjectName(
        QStringLiteral("documentTopRow"));
    auto* document_top_layout =
        new QHBoxLayout(document_top_row);
    document_top_layout->setContentsMargins(0, 0, 0, 0);
    document_top_layout->setSpacing(6);

    auto* document_tools_scroll =
        new QScrollArea(document_top_row);
    document_tools_scroll->setObjectName(
        QStringLiteral("documentToolsScroll"));
    document_tools_scroll->setFrameShape(
        QFrame::NoFrame);
    document_tools_scroll->setWidgetResizable(true);
    document_tools_scroll->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
    document_tools_scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAsNeeded);
    document_tools_scroll->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Preferred);
    document_tools_scroll->setMinimumWidth(0);

    auto* document_tools_host =
        new QWidget(document_tools_scroll);
    document_tools_host->setObjectName(
        QStringLiteral("documentToolsHost"));
    editor_tools_layout_ =
        new QHBoxLayout(document_tools_host);
    editor_tools_layout_->setContentsMargins(0, 0, 0, 0);
    editor_tools_layout_->addStretch(1);
    document_tools_scroll->setWidget(
        document_tools_host);

    document_actions_ = new QHBoxLayout;
    document_actions_->setContentsMargins(0, 0, 0, 0);
    document_actions_->setSpacing(4);

    document_top_layout->addWidget(
        document_tools_scroll,
        1);
    document_top_layout->addLayout(
        document_actions_,
        0);
    root->addWidget(document_top_row, 0);

    splitter_ = new QSplitter(Qt::Horizontal, this);
    auto* splitter = splitter_;
    splitter->setObjectName(QStringLiteral("workbenchSplitter"));
    splitter->setChildrenCollapsible(false);

    auto* tree_group =
        new QGroupBox(QStringLiteral("Document Tree"), splitter);
    tree_group->setObjectName(
        QStringLiteral("documentTreePanel"));
    auto* tree_layout = new QVBoxLayout(tree_group);

    document_tree_ = new QTreeWidget(tree_group);
    document_tree_->setObjectName(QStringLiteral("documentTree"));
    document_tree_->setHeaderHidden(true);
    document_tree_->setSelectionMode(
        QAbstractItemView::SingleSelection);
    tree_layout->addWidget(document_tree_);

    splitter->addWidget(tree_group);

    auto* editor_host = new QWidget(splitter);
    editor_host->setObjectName(QStringLiteral("editorSurfaceHost"));
    editor_host_layout_ = new QVBoxLayout(editor_host);
    editor_host_layout_->setContentsMargins(0, 0, 0, 0);
    editor_host_layout_->setSpacing(4);

    splitter->addWidget(editor_host);

    auto* right_panel = new QWidget(splitter);
    right_panel->setObjectName(
        QStringLiteral("workbenchRightPanel"));
    auto* right_layout = new QVBoxLayout(right_panel);
    right_layout->setContentsMargins(0, 0, 0, 0);

    auto* properties_group =
        new QGroupBox(QStringLiteral("Properties"), right_panel);
    properties_host_layout_ = new QVBoxLayout(properties_group);
    right_layout->addWidget(properties_group, 0);

    auto* operations_group =
        new QGroupBox(QStringLiteral("Operations"), right_panel);
    operations_host_layout_ = new QVBoxLayout(operations_group);
    right_layout->addWidget(operations_group, 1);

    splitter->addWidget(right_panel);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 0);
    splitter->setSizes({150, 900, 270});

    root->addWidget(splitter, 1);

    status_ = new QLabel(this);
    status_->setObjectName(QStringLiteral("workbenchStatus"));
    status_->setWordWrap(true);
    root->addWidget(status_);
}

QHBoxLayout& CadWorkbenchShell::documentActionsLayout() noexcept {
    return *document_actions_;
}

QHBoxLayout& CadWorkbenchShell::editorToolsLayout() noexcept {
    return *editor_tools_layout_;
}

QTreeWidget& CadWorkbenchShell::documentTree() noexcept {
    return *document_tree_;
}

QLabel& CadWorkbenchShell::statusLabel() noexcept {
    return *status_;
}

void CadWorkbenchShell::replaceContent(
    QVBoxLayout& layout,
    QWidget*& current,
    QWidget* replacement) {
    if (current == replacement) return;

    if (current != nullptr) {
        layout.removeWidget(current);
        current->deleteLater();
    }

    current = replacement;
    if (current != nullptr) {
        layout.addWidget(current, 1);
    }
}

void CadWorkbenchShell::setEditorSurface(QWidget* widget) {
    replaceContent(
        *editor_host_layout_,
        editor_surface_,
        widget);
}

void CadWorkbenchShell::setCommandLineContent(QWidget* widget) {
    replaceContent(
        *editor_host_layout_,
        command_line_content_,
        widget);
}

void CadWorkbenchShell::setPropertiesContent(QWidget* widget) {
    replaceContent(
        *properties_host_layout_,
        properties_content_,
        widget);
}

void CadWorkbenchShell::setOperationsContent(QWidget* widget) {
    replaceContent(
        *operations_host_layout_,
        operations_content_,
        widget);

    // Apply the accepted normal-workbench geometry after the final
    // right-panel content has been installed. Applying this only in the
    // constructor lets later content size hints rebalance the splitter and
    // defeats the intended narrow side-panel defaults.
    if (splitter_ != nullptr) {
        splitter_->setSizes({150, 900, 270});
    }
}

} // namespace simplesolid2::ui
