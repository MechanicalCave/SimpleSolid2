#include "cad_workbench_shell.hpp"

#include <QAbstractItemView>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace simplesolid2::ui {

CadWorkbenchShell::CadWorkbenchShell(QWidget* parent)
    : QWidget{parent} {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(6, 6, 6, 6);
    root->setSpacing(5);

    document_actions_ = new QHBoxLayout;
    root->addLayout(document_actions_);

    auto* splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setObjectName(QStringLiteral("workbenchSplitter"));
    splitter->setChildrenCollapsible(false);

    auto* tree_group =
        new QGroupBox(QStringLiteral("Document Tree"), splitter);
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

    editor_tools_layout_ = new QHBoxLayout;
    editor_tools_layout_->setContentsMargins(0, 0, 0, 0);
    editor_tools_layout_->addStretch(1);
    editor_host_layout_->addLayout(editor_tools_layout_);

    splitter->addWidget(editor_host);

    auto* right_panel = new QWidget(splitter);
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

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);
    splitter->setStretchFactor(2, 2);
    splitter->setSizes({220, 620, 300});

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
}

} // namespace simplesolid2::ui
