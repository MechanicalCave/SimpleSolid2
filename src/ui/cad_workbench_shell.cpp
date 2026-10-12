#include "cad_workbench_shell.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QGroupBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QMenu>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScopedValueRollback>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <vector>

namespace simplesolid2::ui {

namespace {

struct ToolbarGroupSpec final {
    const char* name;
    const char* label_object_name;
    std::vector<const char*> tool_object_names;
    bool pinned{};
};

// Existing real command buttons only. This is transient presentation
// metadata; no QAction replacement, new CAD command or persisted Part data.
std::vector<ToolbarGroupSpec> toolbarGroups(bool sketch) {
    if (!sketch) {
        return {
            {"Sketch", "", {"sketchToolButton"}, true},
            {"Work Features", "", {"datumPlaneToolButton"}, false},
            {"Create", "partCreateToolsLabel",
                {"extrudeToolButton", "revolveToolButton"}, true},
            {"Modify", "partModifyToolsLabel",
                {"filletToolButton", "chamferToolButton"}, false},
        };
    }
    return {
        {"Select", "", {"selectSketchToolButton"}, true},
        {"Create", "sketchCreateToolsLabel",
            {"lineSketchToolButton", "circleSketchToolButton",
             "arcSketchToolButton", "rectangleSketchToolButton"}, true},
        {"Profile", "sketchProfileToolsLabel",
            {"profileSketchToolButton"}, false},
        {"References", "", {"projectEdgeToolButton"}, true},
        {"Modify", "sketchModifyToolsLabel",
            {"trimSketchToolButton", "extendSketchToolButton",
             "extendBothSketchToolButton"}, false},
        {"Transform", "",
            {"moveSketchToolButton", "copySketchToolButton",
             "rotateSketchToolButton", "scaleSketchToolButton",
             "mirrorSketchToolButton"}, false},
        {"Inspect", "sketchInspectToolsLabel",
            {"measureSketchToolButton"}, false},
    };
}

} // namespace

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

    contextual_tools_scroll_ =
        new QScrollArea(document_top_row);
    auto* document_tools_scroll = contextual_tools_scroll_;
    document_tools_scroll->setObjectName(
        QStringLiteral("documentToolsScroll"));
    document_tools_scroll->setFrameShape(
        QFrame::NoFrame);
    document_tools_scroll->setWidgetResizable(true);
    document_tools_scroll->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
    document_tools_scroll->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAlwaysOff);
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
    // The scroll host may be smaller than the fixed Document zone.
    // Center its one-row tools vertically, and recompute overflow only
    // once the QScrollArea viewport has a measured final size.
    document_tools_scroll->setAlignment(
        Qt::AlignLeft | Qt::AlignVCenter);
    document_tools_scroll->viewport()->installEventFilter(this);

    document_actions_ = new QHBoxLayout;
    document_actions_->setContentsMargins(0, 0, 0, 0);
    document_actions_->setSpacing(4);

    document_top_layout->addWidget(
        document_tools_scroll,
        1);

    toolbar_more_ = new QToolButton(document_top_row);
    toolbar_more_->setObjectName(
        QStringLiteral("workbenchToolbarMoreButton"));
    toolbar_more_->setText(QStringLiteral("Więcej"));
    toolbar_more_->setAccessibleName(
        QStringLiteral("More contextual CAD tools"));
    toolbar_more_->setToolTip(
        QStringLiteral("More CAD tools, grouped by function"));
    toolbar_more_->setPopupMode(QToolButton::InstantPopup);
    toolbar_more_menu_ = new QMenu(toolbar_more_);
    toolbar_more_->setMenu(toolbar_more_menu_);
    toolbar_more_->hide();
    connect(toolbar_more_menu_, &QMenu::aboutToShow, this,
            [this] { rebuildToolbarMoreMenu(); });
    document_top_layout->addWidget(toolbar_more_, 0);

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
    viewport_mode_cue_ = new QLabel(editor_host);
    viewport_mode_cue_->setObjectName(
        QStringLiteral("workbenchViewportModeCue"));
    viewport_mode_cue_->setWordWrap(true);
    viewport_mode_cue_->setVisible(false);
    editor_host_layout_->addWidget(viewport_mode_cue_, 0);

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
    right_layout->setSpacing(0);

    // G2: two independently scrolling panels. The splitter changes only
    // transient UI presentation, never the active Part/Sketch edit owner.
    right_vertical_splitter_ =
        new QSplitter(Qt::Vertical, right_panel);
    right_vertical_splitter_->setObjectName(
        QStringLiteral("workbenchRightVerticalSplitter"));
    right_vertical_splitter_->setChildrenCollapsible(false);

    properties_group_ =
        new QGroupBox(
            QStringLiteral("Properties"), right_vertical_splitter_);
    properties_group_->setMinimumHeight(0);
    properties_host_layout_ = new QVBoxLayout(properties_group_);
    properties_scroll_ = new QScrollArea(properties_group_);
    properties_scroll_->setObjectName(
        QStringLiteral("workbenchPropertiesScroll"));
    properties_scroll_->setWidgetResizable(true);
    properties_scroll_->setFrameShape(QFrame::NoFrame);
    properties_scroll_->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOn);
    properties_scroll_->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAsNeeded);
    properties_scroll_->setMinimumHeight(0);
    properties_scroll_->setSizePolicy(
        QSizePolicy::Ignored, QSizePolicy::Expanding);
    properties_host_layout_->addWidget(properties_scroll_, 1);
    right_vertical_splitter_->addWidget(properties_group_);

    operations_group_ =
        new QGroupBox(
            QStringLiteral("Operations"), right_vertical_splitter_);
    operations_group_->setMinimumHeight(0);
    operations_host_layout_ = new QVBoxLayout(operations_group_);
    operations_mode_cue_ = new QLabel(operations_group_);
    operations_mode_cue_->setObjectName(
        QStringLiteral("workbenchOperationsModeCue"));
    operations_mode_cue_->setWordWrap(true);
    operations_mode_cue_->hide();
    operations_host_layout_->addWidget(operations_mode_cue_, 0);
    operations_scroll_ = new QScrollArea(operations_group_);
    operations_scroll_->setObjectName(
        QStringLiteral("workbenchOperationsScroll"));
    operations_scroll_->setWidgetResizable(true);
    operations_scroll_->setFrameShape(QFrame::NoFrame);
    operations_scroll_->setVerticalScrollBarPolicy(
        Qt::ScrollBarAlwaysOn);
    operations_scroll_->setHorizontalScrollBarPolicy(
        Qt::ScrollBarAsNeeded);
    operations_scroll_->setMinimumHeight(0);
    operations_scroll_->setSizePolicy(
        QSizePolicy::Ignored, QSizePolicy::Expanding);
    operations_host_layout_->addWidget(operations_scroll_, 1);

    // Fixed-height action area stays outside the scrolling form.
    // Existing Finish/Cancel QWidget identity is preserved.
    operations_footer_ = new QWidget(operations_group_);
    operations_footer_->setObjectName(
        QStringLiteral("workbenchOperationsFooter"));
    auto* footer_layout = new QVBoxLayout(operations_footer_);
    footer_layout->setContentsMargins(0, 0, 0, 0);
    footer_layout->setSpacing(0);
    operations_footer_stack_ =
        new QStackedWidget(operations_footer_);
    operations_footer_stack_->setObjectName(
        QStringLiteral("workbenchOperationsFooterStack"));
    footer_layout->addWidget(operations_footer_stack_);
    operations_host_layout_->addWidget(operations_footer_, 0);
    operations_footer_->hide();

    right_vertical_splitter_->addWidget(operations_group_);
    right_vertical_splitter_->setStretchFactor(0, 2);
    right_vertical_splitter_->setStretchFactor(1, 3);
    right_vertical_splitter_->setSizes({400, 600});
    right_layout->addWidget(right_vertical_splitter_, 1);

    // Tabs occupy the same right-column slot only when measured effective
    // height is insufficient for two independently usable scroll panes.
    right_short_tabs_ = new QTabWidget(right_panel);
    right_short_tabs_->setObjectName(
        QStringLiteral("workbenchRightShortTabs"));
    right_short_tabs_->setTabPosition(QTabWidget::North);
    right_short_tabs_->setVisible(false);
    right_layout->addWidget(right_short_tabs_, 1);
    connect(
        right_vertical_splitter_,
        &QSplitter::splitterMoved,
        this,
        [this](int, int) {
            const auto sizes = right_vertical_splitter_->sizes();
            if (!short_right_panel_mode_ && sizes.size() == 2 &&
                sizes[0] > 0 && sizes[1] > 0) {
                right_panel_ratio_ =
                    static_cast<double>(sizes[0]) /
                    static_cast<double>(sizes[0] + sizes[1]);
            }
        });

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

    initial_font_line_height_ =
        std::max(1, fontMetrics().lineSpacing());
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

void CadWorkbenchShell::replaceScrollContent(
    QScrollArea& scroll,
    QWidget*& current,
    QWidget* replacement) {
    if (current == replacement) return;
    if (auto* previous = scroll.takeWidget()) {
        previous->deleteLater();
    }
    current = replacement;
    if (current != nullptr) {
        scroll.setWidget(current);
    }
}

void CadWorkbenchShell::setPropertiesContent(QWidget* widget) {
    replaceScrollContent(
        *properties_scroll_, properties_content_, widget);
}

void CadWorkbenchShell::setOperationsContent(QWidget* widget) {
    replaceScrollContent(
        *operations_scroll_, operations_content_, widget);

    // Re-apply the active regime after final right-panel content has been
    // installed. Content size hints must not silently promote the side
    // panels or undo a narrow/compact presentation.
    updateResponsiveLayout(true);
}

double CadWorkbenchShell::rightPanelRatio() const {
    if (!short_right_panel_mode_ &&
        right_vertical_splitter_ != nullptr &&
        !right_vertical_splitter_->isHidden()) {
        const auto sizes = right_vertical_splitter_->sizes();
        if (sizes.size() == 2 && sizes[0] > 0 && sizes[1] > 0) {
            return static_cast<double>(sizes[0]) /
                static_cast<double>(sizes[0] + sizes[1]);
        }
    }
    return right_panel_ratio_;
}

void CadWorkbenchShell::setRightPanelRatio(double ratio) {
    if (!(ratio > 0.0 && ratio < 1.0)) return;
    right_panel_ratio_ = ratio;
    applyRightPanelRatio();
}

void CadWorkbenchShell::setOperationsActive(bool active) {
    const bool entering = active && !operations_active_;
    operations_active_ = active;
    if (entering && short_right_panel_mode_) {
        right_short_tabs_->setCurrentIndex(1);
    }
}

void CadWorkbenchShell::registerOperationsFooterRow(
    const QString& mode,
    QWidget* primary,
    QWidget* secondary) {
    if (mode.isEmpty() || primary == nullptr ||
        operations_footer_stack_ == nullptr ||
        operations_footer_rows_.contains(mode)) {
        return;
    }
    auto* row = new QWidget(operations_footer_stack_);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(5);
    for (auto* control : {primary, secondary}) {
        if (control == nullptr) continue;
        const bool explicitly_hidden = control->isHidden();
        if (auto* old_parent = control->parentWidget()) {
            if (auto* old_layout = old_parent->layout()) {
                old_layout->removeWidget(control);
            }
        }
        control->setParent(row);
        layout->addWidget(control);
        if (!explicitly_hidden) control->show();
    }
    operations_footer_stack_->addWidget(row);
    operations_footer_rows_.insert(mode, row);
}

void CadWorkbenchShell::showOperationsFooterRow(
    const QString& mode) {
    const auto row = operations_footer_rows_.value(mode, nullptr);
    if (row == nullptr) {
        operations_footer_->hide();
        return;
    }
    operations_footer_stack_->setCurrentWidget(row);
    operations_footer_->show();
}

void CadWorkbenchShell::setModeCue(const QString& mode) {
    viewport_mode_cue_->setText(mode);
    operations_mode_cue_->setText(mode);
    const bool present = !mode.isEmpty();
    viewport_mode_cue_->setVisible(present);
    operations_mode_cue_->setVisible(present);
}

void CadWorkbenchShell::applyRightPanelRatio() {
    if (right_vertical_splitter_ == nullptr ||
        short_right_panel_mode_) {
        return;
    }
    const int total = std::max(
        200, right_vertical_splitter_->height());
    const int properties_height = static_cast<int>(
        right_panel_ratio_ * total);
    right_vertical_splitter_->setSizes(
        {properties_height, total - properties_height});
}

void CadWorkbenchShell::updateRightPanelHeightMode() {
    if (right_panel_ == nullptr ||
        right_vertical_splitter_ == nullptr ||
        right_short_tabs_ == nullptr ||
        !isVisible()) {
        return;
    }
    // Responsive threshold in the same measured logical coordinates as
    // the available panel height. Preserve the established 470/550
    // reference hysteresis, but scale it with live Qt font metrics so
    // larger accessibility fonts do not clip two unusable short panes.
    // Reserve space for the real, non-scrolling Operations footer.
    // Window WIDTH never triggers short-height tabs.
    const int current_line_height = std::max({
        fontMetrics().lineSpacing(),
        properties_group_->fontMetrics().lineSpacing(),
        operations_group_->fontMetrics().lineSpacing()});
    const int reference_line_height =
        std::max(1, initial_font_line_height_);
    const int responsive_line_height =
        std::max(reference_line_height, current_line_height);
    const int footer_extra = operations_footer_ != nullptr
        ? std::max(0, operations_footer_->sizeHint().height() - 48)
        : 0;
    const int enter_short_height =
        (470 * responsive_line_height +
            reference_line_height - 1) /
            reference_line_height +
        footer_extra;
    const int leave_short_height =
        (550 * responsive_line_height +
            reference_line_height - 1) /
            reference_line_height +
        footer_extra;
    const int available_height = right_panel_->height();
    const bool short_required = short_right_panel_mode_
        ? available_height < leave_short_height
        : available_height < enter_short_height;
    if (short_required == short_right_panel_mode_) return;

    if (short_required) {
        right_panel_ratio_ = rightPanelRatio();
        right_vertical_splitter_->hide();
        right_short_tabs_->addTab(
            properties_group_, QStringLiteral("Properties"));
        right_short_tabs_->addTab(
            operations_group_, QStringLiteral("Operations"));
        short_right_panel_mode_ = true;
        right_short_tabs_->setCurrentIndex(
            operations_active_ ? 1 : 0);
        right_short_tabs_->show();
    } else {
        right_short_tabs_->hide();
        right_short_tabs_->removeTab(1);
        right_short_tabs_->removeTab(0);
        right_vertical_splitter_->addWidget(properties_group_);
        right_vertical_splitter_->addWidget(operations_group_);
        short_right_panel_mode_ = false;
        right_vertical_splitter_->show();
        properties_group_->show();
        operations_group_->show();
        applyRightPanelRatio();
    }
}

void CadWorkbenchShell::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateResponsiveLayout();
    refreshToolbarOverflow();
}

bool CadWorkbenchShell::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (contextual_tools_scroll_ != nullptr &&
        watched == contextual_tools_scroll_->viewport() &&
        event != nullptr &&
        event->type() == QEvent::Resize) {
        refreshToolbarOverflow();
    }
    return QWidget::eventFilter(watched, event);
}


void CadWorkbenchShell::setToolbarContext(bool sketch_context) {
    toolbar_sketch_context_ = sketch_context;
    refreshToolbarOverflow();
}

void CadWorkbenchShell::rebuildToolbarMoreMenu() {
    if (toolbar_more_menu_ == nullptr) return;
    toolbar_more_menu_->clear();
    const auto groups = toolbarGroups(toolbar_sketch_context_);
    for (const auto& group : groups) {
        QMenu* subgroup = nullptr;
        for (const char* name : group.tool_object_names) {
            auto* original = findChild<QPushButton*>(
                QString::fromLatin1(name));
            if (original == nullptr) continue;
            if (subgroup == nullptr) {
                subgroup = toolbar_more_menu_->addMenu(
                    QString::fromLatin1(group.name));
            }
            auto* action =
                subgroup->addAction(original->text());
            action->setEnabled(original->isEnabled());
            action->setCheckable(original->isCheckable());
            action->setChecked(original->isChecked());
            action->setToolTip(original->toolTip());
            QObject::connect(
                action, &QAction::triggered, original,
                [original] {
                    if (original->isEnabled()) {
                        original->click();
                    }
                });
        }
    }
}

void CadWorkbenchShell::refreshToolbarOverflow() {
    if (refreshing_toolbar_ ||
        contextual_tools_scroll_ == nullptr ||
        toolbar_more_ == nullptr ||
        editor_tools_layout_ == nullptr) {
        return;
    }
    QScopedValueRollback<bool> reentry_guard(
        refreshing_toolbar_, true);
    const auto groups = toolbarGroups(toolbar_sketch_context_);
    const auto belongs_to_context = [&](QWidget* widget) {
        if (widget == nullptr) return false;
        for (const auto& group : groups) {
            if (*group.label_object_name != '\0' &&
                widget->objectName() ==
                    QString::fromLatin1(group.label_object_name)) {
                return true;
            }
            for (const char* id : group.tool_object_names) {
                if (widget->objectName() ==
                    QString::fromLatin1(id)) {
                    return true;
                }
            }
        }
        return false;
    };

    // Restore only controls hidden by the previous responsive pass and
    // belonging to the CURRENT context. A Sketch/Part context switch
    // must not resurrect production-hidden controls from another mode.
    for (QWidget* widget : toolbar_overflow_hidden_) {
        if (belongs_to_context(widget)) widget->show();
    }
    toolbar_overflow_hidden_.clear();

    const int gap = std::max(0, editor_tools_layout_->spacing());
    const auto group_width = [&](const ToolbarGroupSpec& group) {
        int width = 0;
        const auto count = [&](QWidget* widget) {
            return widget != nullptr && !widget->isHidden()
                ? widget->sizeHint().width() + gap
                : 0;
        };
        if (*group.label_object_name != '\0') {
            width += count(findChild<QWidget*>(
                QString::fromLatin1(group.label_object_name)));
        }
        for (const char* id : group.tool_object_names) {
            width += count(findChild<QPushButton*>(
                QString::fromLatin1(id)));
        }
        return width;
    };
    int required = 0;
    for (const auto& group : groups) {
        required += group_width(group);
    }
    // Preserve the fixed Document-control zone. Reserve room for the
    // named two-level More menu before collapsing whole tool groups.
    const int available = std::max(
        0, contextual_tools_scroll_->viewport()->width() -
           toolbar_more_->sizeHint().width() - 16);
    if (required <= available) {
        toolbar_more_->hide();
        return;
    }
    toolbar_more_->show();
    const auto collapse_group = [&](const ToolbarGroupSpec& group) {
        const auto hide_one = [&](QWidget* widget) {
            if (widget == nullptr || widget->isHidden()) return;
            required -= widget->sizeHint().width() + gap;
            widget->hide();
            toolbar_overflow_hidden_.push_back(widget);
        };
        if (*group.label_object_name != '\0') {
            hide_one(findChild<QWidget*>(
                QString::fromLatin1(group.label_object_name)));
        }
        for (const char* id : group.tool_object_names) {
            hide_one(findChild<QPushButton*>(
                QString::fromLatin1(id)));
        }
    };
    // Unpinned complete groups collapse before pinned groups. The
    // named More -> group -> real action is always available; direct
    // named dropdowns, icon-only pinned states and personalization
    // remain the next bounded G3 work, not an asserted acceptance.
    if (!toolbar_sketch_context_) {
        for (int i : {3, 1, 2, 0}) {
            if (required <= available) break;
            collapse_group(groups[static_cast<std::size_t>(i)]);
        }
    } else {
        for (int i : {6, 5, 4, 2, 3, 1, 0}) {
            if (required <= available) break;
            collapse_group(groups[static_cast<std::size_t>(i)]);
        }
    }
    rebuildToolbarMoreMenu();
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

    updateRightPanelHeightMode();

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
