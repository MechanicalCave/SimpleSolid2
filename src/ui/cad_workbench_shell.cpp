#include "cad_workbench_shell.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSplitter>
#include <QToolButton>
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

    tree_recovery_button_ =
        new QToolButton(document_top_row);
    tree_recovery_button_->setObjectName(
        QStringLiteral("workbenchTreeRecoveryButton"));
    tree_recovery_button_->setText(
        QStringLiteral("Tree"));
    tree_recovery_button_->setToolTip(
        QStringLiteral("Show Document Tree"));
    tree_recovery_button_->setCheckable(true);
    tree_recovery_button_->setAutoRaise(true);
    tree_recovery_button_->hide();

    right_recovery_button_ =
        new QToolButton(document_top_row);
    right_recovery_button_->setObjectName(
        QStringLiteral("workbenchRightPanelRecoveryButton"));
    right_recovery_button_->setText(
        QStringLiteral("Panel"));
    right_recovery_button_->setToolTip(
        QStringLiteral("Show Properties and Operations"));
    right_recovery_button_->setCheckable(true);
    right_recovery_button_->setAutoRaise(true);
    right_recovery_button_->hide();

    connect(
        tree_recovery_button_,
        &QToolButton::clicked,
        this,
        [this](bool checked) {
            if (responsive_regime_ !=
                ResponsiveRegime::narrow) {
                return;
            }
            setNarrowPanel(
                checked ? tree_panel_ : nullptr);
        });
    connect(
        right_recovery_button_,
        &QToolButton::clicked,
        this,
        [this](bool checked) {
            if (responsive_regime_ !=
                ResponsiveRegime::narrow) {
                return;
            }
            setNarrowPanel(
                checked ? right_panel_ : nullptr);
        });

    document_top_layout->addWidget(
        tree_recovery_button_,
        0);
    document_top_layout->addWidget(
        right_recovery_button_,
        0);
    document_top_layout->addLayout(
        document_actions_,
        0);
    root->addWidget(document_top_row, 0);

    splitter_ = new QSplitter(Qt::Horizontal, this);
    auto* splitter = splitter_;
    splitter->setObjectName(QStringLiteral("workbenchSplitter"));
    splitter->setChildrenCollapsible(false);

    tree_panel_ =
        new QGroupBox(QStringLiteral("Document Tree"), splitter);
    auto* tree_group = tree_panel_;
    tree_group->setObjectName(
        QStringLiteral("documentTreePanel"));
    tree_group->setMinimumWidth(0);
    tree_group->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Expanding);
    auto* tree_layout = new QVBoxLayout(tree_group);

    document_tree_ = new QTreeWidget(tree_group);
    document_tree_->setObjectName(QStringLiteral("documentTree"));
    document_tree_->setHeaderHidden(true);
    document_tree_->setSelectionMode(
        QAbstractItemView::SingleSelection);
    tree_layout->addWidget(document_tree_);

    splitter->addWidget(tree_group);

    editor_host_ = new QWidget(splitter);
    auto* editor_host = editor_host_;
    editor_host->setObjectName(QStringLiteral("editorSurfaceHost"));
    editor_host->setMinimumWidth(0);
    editor_host->setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Expanding);
    editor_host_layout_ = new QVBoxLayout(editor_host);
    editor_host_layout_->setContentsMargins(0, 0, 0, 0);
    editor_host_layout_->setSpacing(4);

    splitter->addWidget(editor_host);

    right_panel_ = new QWidget(splitter);
    auto* right_panel = right_panel_;
    right_panel->setObjectName(
        QStringLiteral("workbenchRightPanel"));
    // The panel contains forms whose natural size hint is intentionally
    // larger than the accepted normal CAD-shell width. Let the splitter
    // honor the explicit ~270 px default instead of promoting that content
    // hint to a shell-wide minimum in either axis; contextual Operations may
    // grow substantially (for example Profile tooling) without resizing the
    // top-level application window. Individual controls remain responsible
    // for their own compact/narrow presentation.
    right_panel->setMinimumWidth(0);
    right_panel->setSizePolicy(
        QSizePolicy::Ignored,
        QSizePolicy::Ignored);
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

    updateResponsiveLayout(true);
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

    // Re-apply the active regime after final right-panel content has been
    // installed. Content size hints must not silently promote the side
    // panels or undo a narrow/compact presentation.
    updateResponsiveLayout(true);
}

void CadWorkbenchShell::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
}

bool CadWorkbenchShell::panelContainsFocus(
    const QWidget* panel) const {
    if (panel == nullptr) return false;
    auto* focus = QApplication::focusWidget();
    return focus != nullptr &&
           (focus == panel || panel->isAncestorOf(focus));
}

void CadWorkbenchShell::setNarrowPanel(QWidget* panel) {
    if (responsive_regime_ != ResponsiveRegime::narrow ||
        tree_panel_ == nullptr ||
        right_panel_ == nullptr) {
        return;
    }

    const bool show_tree = panel == tree_panel_;
    const bool show_right = panel == right_panel_;

    tree_panel_->setVisible(show_tree);
    right_panel_->setVisible(show_right);
    tree_recovery_button_->setChecked(show_tree);
    right_recovery_button_->setChecked(show_right);
    tree_recovery_button_->setToolTip(
        show_tree
            ? QStringLiteral("Hide Document Tree")
            : QStringLiteral("Show Document Tree"));
    right_recovery_button_->setToolTip(
        show_right
            ? QStringLiteral("Hide Properties and Operations")
            : QStringLiteral("Show Properties and Operations"));

    if (show_tree) {
        splitter_->setSizes({200, 1000, 0});
    } else if (show_right) {
        splitter_->setSizes({0, 1000, 270});
    } else {
        splitter_->setSizes({0, 1000, 0});
    }
}

void CadWorkbenchShell::updateResponsiveLayout(bool force) {
    if (splitter_ == nullptr ||
        tree_panel_ == nullptr ||
        editor_host_ == nullptr ||
        right_panel_ == nullptr ||
        tree_recovery_button_ == nullptr ||
        right_recovery_button_ == nullptr) {
        return;
    }

    // Deliberately separated enter/leave thresholds prevent visible
    // oscillation when native resize metrics hover around a breakpoint.
    constexpr int enter_normal_width = 1120;
    constexpr int leave_normal_width = 1040;
    constexpr int enter_narrow_width = 760;
    constexpr int leave_narrow_width = 820;

    auto next = responsive_regime_;
    if (!responsive_initialized_) {
        if (width() <= enter_narrow_width) {
            next = ResponsiveRegime::narrow;
        } else if (width() < enter_normal_width) {
            next = ResponsiveRegime::compact;
        } else {
            next = ResponsiveRegime::normal;
        }
    } else {
        switch (responsive_regime_) {
        case ResponsiveRegime::normal:
            if (width() < leave_normal_width) {
                next =
                    width() <= enter_narrow_width
                        ? ResponsiveRegime::narrow
                        : ResponsiveRegime::compact;
            }
            break;
        case ResponsiveRegime::compact:
            if (width() >= enter_normal_width) {
                next = ResponsiveRegime::normal;
            } else if (width() <= enter_narrow_width) {
                next = ResponsiveRegime::narrow;
            }
            break;
        case ResponsiveRegime::narrow:
            if (width() >= leave_narrow_width) {
                next =
                    width() >= enter_normal_width
                        ? ResponsiveRegime::normal
                        : ResponsiveRegime::compact;
            }
            break;
        }
    }

    if (!force &&
        responsive_initialized_ &&
        next == responsive_regime_) {
        return;
    }

    const bool entering_narrow =
        next == ResponsiveRegime::narrow &&
        (!responsive_initialized_ ||
         responsive_regime_ != ResponsiveRegime::narrow);

    responsive_regime_ = next;
    responsive_initialized_ = true;

    if (responsive_regime_ == ResponsiveRegime::normal) {
        tree_panel_->show();
        right_panel_->show();
        tree_recovery_button_->hide();
        right_recovery_button_->hide();
        tree_recovery_button_->setChecked(false);
        right_recovery_button_->setChecked(false);
        splitter_->setSizes({150, 1000, 270});
        return;
    }

    if (responsive_regime_ == ResponsiveRegime::compact) {
        tree_panel_->show();
        right_panel_->show();
        tree_recovery_button_->hide();
        right_recovery_button_->hide();
        tree_recovery_button_->setChecked(false);
        right_recovery_button_->setChecked(false);
        splitter_->setSizes({120, 1000, 230});
        return;
    }

    tree_recovery_button_->show();
    right_recovery_button_->show();

    if (entering_narrow) {
        // Do not make a focused text/property editor disappear during
        // reflow. Otherwise narrow mode starts with the center surface only.
        if (panelContainsFocus(right_panel_)) {
            setNarrowPanel(right_panel_);
        } else if (panelContainsFocus(tree_panel_)) {
            setNarrowPanel(tree_panel_);
        } else {
            setNarrowPanel(nullptr);
        }
        return;
    }

    // Forced re-application (for example after installing Operations
    // content) preserves the user's currently recovered narrow panel.
    if (!right_panel_->isHidden()) {
        setNarrowPanel(right_panel_);
    } else if (!tree_panel_->isHidden()) {
        setNarrowPanel(tree_panel_);
    } else {
        setNarrowPanel(nullptr);
    }
}

} // namespace simplesolid2::ui
