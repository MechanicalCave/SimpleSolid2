#include "cad_workbench_shell.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QFrame>
#include <QGroupBox>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QPixmap>
#include <QMenu>
#include <QMouseEvent>
#include <QPalette>
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>
#include <QScrollArea>
#include <QScopedValueRollback>
#include <QSettings>
#include <QSet>
#include <QSizePolicy>
#include <QSplitter>
#include <QStackedWidget>
#include <QTabWidget>
#include <QTimer>
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
            {"Sketch", "partSketchToolsLabel", {"sketchToolButton"}, true},
            {"Work Features", "partWorkFeaturesToolsLabel", {"datumPlaneToolButton"}, false},
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
        {"References", "sketchReferencesToolsLabel", {"projectEdgeToolButton"}, true},
        {"Modify", "sketchModifyToolsLabel",
            {"trimSketchToolButton", "extendSketchToolButton",
             "extendBothSketchToolButton"}, false},
        {"Transform", "sketchTransformToolsLabel",
            {"moveSketchToolButton", "copySketchToolButton",
             "rotateSketchToolButton", "scaleSketchToolButton",
             "mirrorSketchToolButton"}, false},
        {"Inspect", "sketchInspectToolsLabel",
            {"measureSketchToolButton"}, false},
    };
}

// Small antialiased geometric tool glyphs; generated in Qt rather than
// relying on theme icons (which may not exist on native Windows).
// All glyphs remain neutral and inherit the active UI text palette.
QIcon makeCadToolbarIcon(
    const QString& object_name,
    const QColor& foreground) {
    QPixmap image{20, 20};
    image.fill(Qt::transparent);
    QPainter painter{&image};
    painter.setRenderHint(QPainter::Antialiasing);
    QPen ink{foreground};
    ink.setWidthF(1.6);
    ink.setCapStyle(Qt::RoundCap);
    ink.setJoinStyle(Qt::RoundJoin);
    painter.setPen(ink);
    painter.setBrush(Qt::NoBrush);
    if (object_name.contains(QStringLiteral("extrude"),
                             Qt::CaseInsensitive)) {
        painter.drawRect(QRectF{3.5, 7, 10, 10});
        painter.drawLine(QPointF{3.5, 7}, QPointF{8, 3});
        painter.drawLine(QPointF{8, 3}, QPointF{17, 3});
        painter.drawLine(QPointF{17, 3}, QPointF{13.5, 7});
        painter.drawLine(QPointF{17, 3}, QPointF{17, 13});
        painter.drawLine(QPointF{13.5, 17}, QPointF{17, 13});
    } else if (object_name.contains(QStringLiteral("revolve"),
                                    Qt::CaseInsensitive)) {
        painter.drawArc(QRectF{3, 3, 14, 14}, 35 * 16, 295 * 16);
        painter.drawLine(QPointF{15.1, 3.2}, QPointF{18, 4});
        painter.drawLine(QPointF{15.1, 3.2}, QPointF{14, 6.1});
        painter.drawLine(QPointF{9, 5}, QPointF{9, 15});
    } else if (object_name.contains(QStringLiteral("circle"),
                                    Qt::CaseInsensitive)) {
        painter.drawEllipse(QRectF{3, 3, 14, 14});
    } else if (object_name.contains(QStringLiteral("rectangle"),
                                    Qt::CaseInsensitive)) {
        painter.drawRect(QRectF{3, 5, 14, 10});
    } else if (object_name.contains(QStringLiteral("arc"),
                                    Qt::CaseInsensitive)) {
        painter.drawArc(QRectF{3, 3, 14, 14}, 0, 180 * 16);
        painter.drawEllipse(QPointF{3, 10}, 1, 1);
        painter.drawEllipse(QPointF{17, 10}, 1, 1);
    } else if (object_name.contains(QStringLiteral("line"),
                                    Qt::CaseInsensitive)) {
        painter.drawLine(QPointF{4, 16}, QPointF{16, 4});
        painter.drawEllipse(QPointF{4, 16}, 1.4, 1.4);
        painter.drawEllipse(QPointF{16, 4}, 1.4, 1.4);
    } else if (object_name.contains(QStringLiteral("project"),
                                    Qt::CaseInsensitive)) {
        painter.drawEllipse(QRectF{3, 3, 10, 10});
        ink.setStyle(Qt::DashLine);
        painter.setPen(ink);
        painter.drawLine(QPointF{12, 12}, QPointF{17, 17});
    } else {
        painter.drawLine(QPointF{3, 15}, QPointF{16, 4});
        painter.drawLine(QPointF{3, 15}, QPointF{6, 9});
        painter.drawLine(QPointF{3, 15}, QPointF{9, 15});
    }
    painter.end();
    return QIcon{image};
}

QStringList defaultToolbarGroupOrder(bool sketch) {
    QStringList order;
    for (const auto& group : toolbarGroups(sketch)) {
        if (sketch &&
            QString::fromLatin1(group.name) ==
                QStringLiteral("Select")) {
            continue; // fixed base action, never a movable group
        }
        order.push_back(QString::fromLatin1(group.name));
    }
    return order;
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
    document_tools_host->installEventFilter(this);
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

    toolbar_customize_ = new QToolButton(document_top_row);
    toolbar_customize_->setObjectName(
        QStringLiteral("workbenchToolbarCustomizeButton"));
    // The entry remains explicit by tooltip/accessible name, but must
    // never enlarge the fixed Document zone or prevent native 750 px
    // narrow Workbench mode. It is not a second toolbar row.
    toolbar_customize_->setText(QStringLiteral("⚙"));
    toolbar_customize_->setFixedWidth(32);
    toolbar_customize_->setToolTip(
        QStringLiteral("Dostosuj pasek — customize toolbar groups"));
    toolbar_customize_->setAccessibleName(
        QStringLiteral("Dostosuj pasek"));
    toolbar_customize_->setCheckable(true);
    toolbar_customize_->setChecked(false);
    connect(toolbar_customize_, &QToolButton::toggled, this,
            [this](bool active) {
                setToolbarCustomizeActive(active);
            });
    document_top_layout->addWidget(toolbar_customize_, 0);

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
    // Deliberately non-CAD, OS user-scope UI preferences. Part and
    // Sketch have distinct keys; no Part Document/Undo/dirty mutation.
    QSettings toolbar_settings{
        QStringLiteral("MechanicalCave"),
        QStringLiteral("SimpleSolid2")};
    for (bool sketch : {false, true}) {
        const QString context = sketch
            ? QStringLiteral("Sketch") : QStringLiteral("Part");
        const QStringList defaults =
            defaultToolbarGroupOrder(sketch);
        const QStringList saved_order = toolbar_settings.value(
            QStringLiteral("ui/workbench/toolbar/v1/") +
            context + QStringLiteral("/order")).toStringList();
        QStringList validated_order = defaults;
        if (saved_order.size() == defaults.size()) {
            QStringList sorted_saved = saved_order;
            QStringList sorted_default = defaults;
            sorted_saved.sort();
            sorted_default.sort();
            if (sorted_saved == sorted_default) {
                validated_order = saved_order;
            }
        }
        toolbar_group_order_.insert(context, validated_order);
        for (const auto& group : toolbarGroups(sketch)) {
            const QString name = QString::fromLatin1(group.name);
            const QString storage_key =
                QStringLiteral("ui/workbench/toolbar/v1/") +
                context + QStringLiteral("/pins/") + name;
            if (toolbar_settings.contains(storage_key)) {
                toolbar_pin_overrides_.insert(
                    context + QStringLiteral("_") + name,
                    toolbar_settings.value(storage_key).toBool());
            }
        }
    }
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
    // The original command widgets stay in the single QHBoxLayout.
    // Redraw their shared lightweight group outline only AFTER Qt has
    // laid out the reflowed widgets; visual frames are not layout items.
    if (event != nullptr &&
        contextual_tools_scroll_ != nullptr &&
        watched == contextual_tools_scroll_->widget() &&
        (event->type() == QEvent::Resize ||
         event->type() == QEvent::LayoutRequest)) {
        QTimer::singleShot(0, this, [this] {
            updateToolbarGroupFrames();
        });
    }
    if (contextual_tools_scroll_ != nullptr &&
        watched == contextual_tools_scroll_->viewport() &&
        event != nullptr &&
        event->type() == QEvent::Resize) {
        refreshToolbarOverflow();
    }
    // A customization handle is a normal QToolButton with a keyboard
    // command menu on its arrow. Dragging its main area sends an adjacent
    // whole-group move through the same validated preference operation as
    // Move Left/Right. Other editing, focus, and viewport mouse routing
    // remain untouched.
    if (event != nullptr && toolbar_customize_active_) {
        for (auto it = toolbar_group_handles_.cbegin();
             it != toolbar_group_handles_.cend(); ++it) {
            QToolButton* handle = it.value();
            if (handle == nullptr || watched != handle ||
                handle->isHidden()) continue;
            const QString context = toolbar_sketch_context_
                ? QStringLiteral("Sketch") : QStringLiteral("Part");
            if (!it.key().startsWith(
                    context + QStringLiteral("_"))) break;
            if (event->type() == QEvent::MouseButtonPress) {
                auto* mouse = static_cast<QMouseEvent*>(event);
                // The right arrow retains normal QToolButton menu access.
                if (mouse->button() == Qt::LeftButton &&
                    mouse->position().x() <
                        handle->width() - 14) {
                    toolbar_drag_handle_ = handle;
                    toolbar_drag_press_global_ =
                        mouse->globalPosition().toPoint();
                }
            } else if (event->type() == QEvent::MouseMove &&
                       toolbar_drag_handle_ == handle) {
                auto* mouse = static_cast<QMouseEvent*>(event);
                const QPoint delta =
                    mouse->globalPosition().toPoint() -
                    toolbar_drag_press_global_;
                if ((mouse->buttons() & Qt::LeftButton) &&
                    std::abs(delta.x()) >=
                        QApplication::startDragDistance()) {
                    handle->setCursor(Qt::ClosedHandCursor);
                }
            } else if (event->type() == QEvent::MouseButtonRelease &&
                       toolbar_drag_handle_ == handle) {
                auto* mouse = static_cast<QMouseEvent*>(event);
                toolbar_drag_handle_.clear();
                handle->setCursor(Qt::OpenHandCursor);
                if (mouse->button() != Qt::LeftButton) break;
                const QPoint delta =
                    mouse->globalPosition().toPoint() -
                    toolbar_drag_press_global_;
                if (std::abs(delta.x()) >=
                        QApplication::startDragDistance() &&
                    std::abs(delta.x()) > std::abs(delta.y())) {
                    const QString name =
                        it.key().mid(context.size() + 1);
                    handle->setDown(false);
                    moveToolbarGroup(
                        context, name, delta.x() < 0 ? -1 : 1);
                    return true; // no QAction/CAD command click on drag
                }
            }
            break;
        }
    }
    return QWidget::eventFilter(watched, event);
}


void CadWorkbenchShell::setToolbarContext(bool sketch_context) {
    const bool context_changed = !toolbar_order_initialized_ ||
        toolbar_sketch_context_ != sketch_context;
    toolbar_sketch_context_ = sketch_context;
    toolbar_order_initialized_ = true;
    if (context_changed) {
        const QString context = sketch_context
            ? QStringLiteral("Sketch") : QStringLiteral("Part");
        // The previous context can have reordered its real tool widgets
        // within this shared QHBoxLayout. Even DEFAULT order must be
        // re-applied when switching back; otherwise groups from the
        // previous context occupy the available inline width and hide
        // reachable commands before the named dropdown/More tiers.
        // This moves only presentation widgets, never CAD actions/state.
        applyToolbarGroupOrder(context);
    }
    refreshToolbarOverflow();
}

void CadWorkbenchShell::setToolbarCustomizeActive(bool active) {
    if (toolbar_customize_active_ == active) return;
    toolbar_customize_active_ = active;
    refreshToolbarOverflow();
}

void CadWorkbenchShell::applyToolbarGroupOrder(
    const QString& context) {
    const bool sketch = context == QStringLiteral("Sketch");
    if ((sketch ? QStringLiteral("Sketch")
                : QStringLiteral("Part")) != context ||
        toolbar_sketch_context_ != sketch ||
        editor_tools_layout_ == nullptr) {
        return;
    }
    auto groups = toolbarGroups(sketch);
    const auto order = toolbar_group_order_.value(
        context, defaultToolbarGroupOrder(sketch));
    std::stable_sort(
        groups.begin(), groups.end(),
        [&](const ToolbarGroupSpec& a,
            const ToolbarGroupSpec& b) {
            if (sketch) {
                if (QString::fromLatin1(a.name) ==
                    QStringLiteral("Select")) return true;
                if (QString::fromLatin1(b.name) ==
                    QStringLiteral("Select")) return false;
            }
            return order.indexOf(QString::fromLatin1(a.name)) <
                order.indexOf(QString::fromLatin1(b.name));
        });

    std::vector<QWidget*> active_widgets;
    for (const auto& group : groups) {
        const QString group_name = QString::fromLatin1(group.name);
        if (sketch && group_name == QStringLiteral("Select")) {
            continue;
        }
        const QString key = context + QStringLiteral("_") +
            group_name;
        const auto push_if_present = [&](QWidget* widget) {
            if (widget != nullptr &&
                editor_tools_layout_->indexOf(widget) >= 0) {
                active_widgets.push_back(widget);
            }
        };
        push_if_present(toolbar_pin_markers_.value(key, nullptr));
        push_if_present(toolbar_group_handles_.value(key, nullptr));
        if (*group.label_object_name != '\0') {
            push_if_present(findChild<QWidget*>(
                QString::fromLatin1(group.label_object_name)));
        }
        for (const char* id : group.tool_object_names) {
            push_if_present(findChild<QPushButton*>(
                QString::fromLatin1(id)));
        }
        push_if_present(toolbar_group_dropdowns_.value(
            key, nullptr));
    }
    // Qt QWidget identity, enablement, check state, connections and
    // focus stay unchanged. Move only existing WHOLE groups as units.
    for (auto* widget : active_widgets) {
        editor_tools_layout_->removeWidget(widget);
    }
    int insert_at = 0;
    if (sketch) {
        auto* select = findChild<QPushButton*>(
            QStringLiteral("selectSketchToolButton"));
        if (select != nullptr &&
            editor_tools_layout_->indexOf(select) >= 0) {
            editor_tools_layout_->removeWidget(select);
            editor_tools_layout_->insertWidget(0, select);
            insert_at = 1;
        }
    }
    for (auto* widget : active_widgets) {
        editor_tools_layout_->insertWidget(insert_at++, widget);
    }
}

void CadWorkbenchShell::moveToolbarGroup(
    const QString& context,
    const QString& group,
    int direction) {
    if (!toolbar_customize_active_ ||
        (direction != -1 && direction != 1) ||
        context != (toolbar_sketch_context_
            ? QStringLiteral("Sketch") : QStringLiteral("Part"))) {
        return;
    }
    auto order = toolbar_group_order_.value(context);
    const int old_index = order.indexOf(group);
    const int new_index = old_index + direction;
    if (old_index < 0 || new_index < 0 ||
        new_index >= order.size()) return;
    order.swapItemsAt(old_index, new_index);
    toolbar_group_order_.insert(context, order);
    QSettings settings{
        QStringLiteral("MechanicalCave"),
        QStringLiteral("SimpleSolid2")};
    settings.setValue(
        QStringLiteral("ui/workbench/toolbar/v1/") +
            context + QStringLiteral("/order"),
        order);
    settings.sync();
    applyToolbarGroupOrder(context);
    refreshToolbarOverflow();
}

void CadWorkbenchShell::resetToolbarGroups(
    const QString& context) {
    if (!toolbar_customize_active_ ||
        context != (toolbar_sketch_context_
            ? QStringLiteral("Sketch") : QStringLiteral("Part"))) {
        return;
    }
    const bool sketch = context == QStringLiteral("Sketch");
    toolbar_group_order_.insert(
        context, defaultToolbarGroupOrder(sketch));
    for (const auto& group : toolbarGroups(sketch)) {
        toolbar_pin_overrides_.remove(
            context + QStringLiteral("_") +
            QString::fromLatin1(group.name));
    }
    QSettings settings{
        QStringLiteral("MechanicalCave"),
        QStringLiteral("SimpleSolid2")};
    const auto prefix =
        QStringLiteral("ui/workbench/toolbar/v1/") + context;
    settings.remove(prefix + QStringLiteral("/order"));
    settings.remove(prefix + QStringLiteral("/pins"));
    settings.sync();
    applyToolbarGroupOrder(context);
    refreshToolbarOverflow();
}

QString CadWorkbenchShell::toolbarActionLabel(
    const QPushButton* button) const {
    if (button == nullptr) return {};
    const auto it = toolbar_icon_compacted_.constFind(
        const_cast<QPushButton*>(button));
    return it != toolbar_icon_compacted_.cend()
        ? it.value() : button->text();
}

void CadWorkbenchShell::rebuildToolbarMoreMenu() {
    if (toolbar_more_menu_ == nullptr) return;
    toolbar_more_menu_->clear();
    auto groups = toolbarGroups(toolbar_sketch_context_);
    const QString context = toolbar_sketch_context_
        ? QStringLiteral("Sketch") : QStringLiteral("Part");
    const auto order = toolbar_group_order_.value(context);
    std::stable_sort(
        groups.begin(), groups.end(),
        [&](const ToolbarGroupSpec& a, const ToolbarGroupSpec& b) {
            if (toolbar_sketch_context_) {
                if (QString::fromLatin1(a.name) ==
                    QStringLiteral("Select")) return true;
                if (QString::fromLatin1(b.name) ==
                    QStringLiteral("Select")) return false;
            }
            return order.indexOf(QString::fromLatin1(a.name)) <
                order.indexOf(QString::fromLatin1(b.name));
        });
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
                subgroup->addAction(toolbarActionLabel(original));
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
    // Refresh may return at any compaction tier; every tier must update
    // the outline after the final inline/dropdown geometry has settled.
    QTimer::singleShot(0, this, [this] {
        updateToolbarGroupFrames();
    });
    auto groups = toolbarGroups(toolbar_sketch_context_);
    // Every native button follows the same baseline height, while
    // larger font/DPI metrics can grow the minimum instead of clipping.
    for (const auto& group : groups) {
        for (const char* object_name : group.tool_object_names) {
            auto* button = findChild<QPushButton*>(
                QString::fromLatin1(object_name));
            if (button == nullptr) continue;
            const int height = std::max(
                32, button->fontMetrics().height() + 12);
            if (button->minimumHeight() != height) {
                button->setMinimumHeight(height);
            }
        }
    }
    // A taller contextual row must not make fixed Document actions
    // appear lower than Sketch/Extrude. Their identity, zone and
    // functional roles stay fixed; only the baseline height matches.
    if (document_actions_ != nullptr) {
        for (int i = 0; i < document_actions_->count(); ++i) {
            auto* item = document_actions_->itemAt(i);
            auto* button = item == nullptr ? nullptr :
                qobject_cast<QPushButton*>(item->widget());
            if (button == nullptr) continue;
            const int height = std::max(
                32, button->fontMetrics().height() + 12);
            if (button->minimumHeight() != height) {
                button->setMinimumHeight(height);
            }
        }
    }
    const QString current_context =
        toolbar_sketch_context_
            ? QStringLiteral("Sketch") : QStringLiteral("Part");
    for (auto& group : groups) {
        const QString key = current_context + QStringLiteral("_") +
            QString::fromLatin1(group.name);
        const auto pin = toolbar_pin_overrides_.constFind(key);
        if (pin != toolbar_pin_overrides_.cend()) {
            group.pinned = pin.value();
        }
    }
    const auto preferred_order =
        toolbar_group_order_.value(current_context);
    std::stable_sort(
        groups.begin(), groups.end(),
        [&](const ToolbarGroupSpec& a, const ToolbarGroupSpec& b) {
            if (toolbar_sketch_context_) {
                if (QString::fromLatin1(a.name) ==
                    QStringLiteral("Select")) return true;
                if (QString::fromLatin1(b.name) ==
                    QStringLiteral("Select")) return false;
            }
            return preferred_order.indexOf(
                QString::fromLatin1(a.name)) <
                preferred_order.indexOf(QString::fromLatin1(b.name));
        });
    // Undo only UI presentation compaction, including when the Part /
    // Sketch mode changes. A draft that updates a label while compacted
    // is never inferred from the Tree inspection selection.
    for (auto it = toolbar_icon_compacted_.begin();
         it != toolbar_icon_compacted_.end(); ++it) {
        if (it.key() != nullptr && it.key()->text().isEmpty()) {
            it.key()->setText(it.value());
        }
    }
    toolbar_icon_compacted_.clear();
    const QString context_name = toolbar_sketch_context_
        ? QStringLiteral("Sketch") : QStringLiteral("Part");

    // Explicit Customize mode: one marker per WHOLE contextual group.
    // The marker is a passive hidden affordance outside this mode and
    // is never part of the CAD command routing/Undo/Part authored data.
    for (auto it = toolbar_pin_markers_.begin();
         it != toolbar_pin_markers_.end(); ++it) {
        if (it.value()) it.value()->hide();
    }
    for (const auto& group : groups) {
        // Select is the permanently fixed Sketch base action, not a
        // customisable/pinnable group. Its marker was stranded after
        // Inspect when restoring the default group order.
        if (toolbar_sketch_context_ &&
            QString::fromLatin1(group.name) ==
                QStringLiteral("Select")) continue;
        const QString key = current_context + QStringLiteral("_") +
            QString::fromLatin1(group.name);
        auto* pin = toolbar_pin_markers_.value(key, nullptr);
        if (pin == nullptr) {
            pin = new QToolButton(contextual_tools_scroll_->widget());
            pin->setObjectName(
                QStringLiteral("workbenchToolbarPin_") + key);
            pin->setAccessibleName(
                QStringLiteral("Pin toolbar group ") +
                QString::fromLatin1(group.name));
            pin->setToolTip(
                QStringLiteral("Pin or unpin the complete ") +
                QString::fromLatin1(group.name) +
                QStringLiteral(" group"));
            pin->setFixedWidth(16);
            pin->setAutoRaise(true);
            pin->setStyleSheet(QStringLiteral(
                "QToolButton { border: none; background: transparent;"
                " padding: 0px; }"));
            const QString preference_key =
                QStringLiteral("ui/workbench/toolbar/v1/") +
                current_context + QStringLiteral("/pins/") +
                QString::fromLatin1(group.name);
            const auto set_group_pin =
                [this, key, preference_key] {
                    if (!toolbar_customize_active_) return;
                    const bool currently_pinned =
                        toolbar_pin_markers_.value(key)
                            ->property("pinned").toBool();
                    const bool next_pin = !currently_pinned;
                    toolbar_pin_overrides_.insert(key, next_pin);
                    QSettings settings{
                        QStringLiteral("MechanicalCave"),
                        QStringLiteral("SimpleSolid2")};
                    settings.setValue(preference_key, next_pin);
                    settings.sync();
                    refreshToolbarOverflow();
                };
            connect(pin, &QToolButton::clicked,
                    this, set_group_pin);
            QWidget* group_first = nullptr;
            if (*group.label_object_name != '\0') {
                group_first = findChild<QWidget*>(
                    QString::fromLatin1(group.label_object_name));
            }
            if (group_first == nullptr &&
                !group.tool_object_names.empty()) {
                group_first = findChild<QWidget*>(
                    QString::fromLatin1(
                        group.tool_object_names.front()));
            }
            const int index = group_first == nullptr
                ? -1 : editor_tools_layout_->indexOf(group_first);
            editor_tools_layout_->insertWidget(
                index >= 0 ? index :
                    std::max(0, editor_tools_layout_->count() - 1),
                pin);
            toolbar_pin_markers_.insert(key, pin);
        }
        pin->setProperty("pinned", group.pinned);
        pin->setText(group.pinned
            ? QStringLiteral("●") : QStringLiteral("○"));
        // Contract: a small pinned marker is visible but NOT editable in
        // normal CAD use; Customize reveals and enables all group pins.
        pin->setEnabled(toolbar_customize_active_);
        pin->setFocusPolicy(
            toolbar_customize_active_
                ? Qt::StrongFocus : Qt::NoFocus);
        pin->setVisible(toolbar_customize_active_ || group.pinned);
    }

    // Intentional whole-group keyboard customization. No individual
    // tool drag/reorder is introduced; these handles are hidden outside
    // explicit Customize mode and move the actual QWidget group as a unit.
    for (auto it = toolbar_group_handles_.begin();
         it != toolbar_group_handles_.end(); ++it) {
        if (it.value()) it.value()->hide();
    }
    for (const auto& group : groups) {
        const QString name = QString::fromLatin1(group.name);
        if (toolbar_sketch_context_ &&
            name == QStringLiteral("Select")) {
            continue; // fixed direct base action, not a group handle
        }
        const QString key = current_context +
            QStringLiteral("_") + name;
        auto* handle = toolbar_group_handles_.value(key, nullptr);
        if (handle == nullptr) {
            handle = new QToolButton(
                contextual_tools_scroll_->widget());
            handle->setObjectName(
                QStringLiteral("workbenchToolbarHandle_") + key);
            handle->setText(QStringLiteral("⋮"));
            handle->setFixedWidth(36);
            handle->setCursor(Qt::OpenHandCursor);
            handle->setToolTip(
                QStringLiteral("Przeciągnij grupę w lewo/prawo "
                               "lub użyj menu obok: ") + name);
            handle->setAccessibleName(
                QStringLiteral("Reorder toolbar group ") + name);
            handle->setPopupMode(
                QToolButton::MenuButtonPopup);
            handle->installEventFilter(this);
            auto* menu = new QMenu(handle);
            auto* move_left = menu->addAction(
                QStringLiteral("Przesuń w lewo"));
            auto* move_right = menu->addAction(
                QStringLiteral("Przesuń w prawo"));
            menu->addSeparator();
            auto* reset = menu->addAction(
                QStringLiteral("Przywróć domyślne"));
            connect(move_left, &QAction::triggered, this,
                [this, context = current_context, name] {
                    moveToolbarGroup(context, name, -1);
                });
            connect(move_right, &QAction::triggered, this,
                [this, context = current_context, name] {
                    moveToolbarGroup(context, name, +1);
                });
            connect(reset, &QAction::triggered, this,
                [this, context = current_context] {
                    resetToolbarGroups(context);
                });
            handle->setMenu(menu);
            // Normal click opens the same keyboard-accessible group
            // options; the drag-release event filter consumes only
            // gestures over QApplication::startDragDistance().
            connect(handle, &QToolButton::clicked,
                    handle, &QToolButton::showMenu);
            QWidget* first = nullptr;
            if (*group.label_object_name != '\0') {
                first = findChild<QWidget*>(
                    QString::fromLatin1(group.label_object_name));
            }
            if (first == nullptr &&
                !group.tool_object_names.empty()) {
                first = findChild<QWidget*>(
                    QString::fromLatin1(
                        group.tool_object_names.front()));
            }
            const int idx = first == nullptr ? -1 :
                editor_tools_layout_->indexOf(first);
            editor_tools_layout_->insertWidget(
                idx >= 0 ? idx :
                    std::max(0, editor_tools_layout_->count() - 1),
                handle);
            handle->hide();
            toolbar_group_handles_.insert(key, handle);
        }
        handle->setVisible(toolbar_customize_active_);
    }

    // Install real-command dropdowns next to each existing unpinned
    // group, not as an unconditional clone row. Their menus delegate to
    // the original connected QPushButton and never author Part state.
    for (auto it = toolbar_group_dropdowns_.begin();
         it != toolbar_group_dropdowns_.end(); ++it) {
        if (it.value()) it.value()->hide();
    }
    for (const auto& group : groups) {
        const QString name = QString::fromLatin1(group.name);
        // Sketch Select is a direct, fixed base tool, never a
        // customizable/reorderable toolbar group.
        if (toolbar_sketch_context_ &&
            name == QStringLiteral("Select")) continue;
        const QString key = context_name + QStringLiteral("_") +
            name;
        if (toolbar_group_dropdowns_.contains(key)) continue;
        auto* dropdown = new QToolButton(
            contextual_tools_scroll_->widget());
        dropdown->setObjectName(
            QStringLiteral("workbenchToolbarGroupDropdown_") + key);
        dropdown->setText(name);
        dropdown->setAccessibleName(name + QStringLiteral(" tools"));
        dropdown->setToolTip(name + QStringLiteral(" tools"));
        dropdown->setPopupMode(QToolButton::InstantPopup);
        auto* menu = new QMenu(dropdown);
        dropdown->setMenu(menu);
        const auto populate = [this, menu, group] {
            menu->clear();
            for (const char* id : group.tool_object_names) {
                auto* original = findChild<QPushButton*>(
                    QString::fromLatin1(id));
                if (original == nullptr) continue;
                auto* action = menu->addAction(
                    toolbarActionLabel(original));
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
        };
        connect(menu, &QMenu::aboutToShow, this, populate);
        populate();
        QWidget* group_first = nullptr;
        if (*group.label_object_name != '\0') {
            group_first = findChild<QWidget*>(
                QString::fromLatin1(group.label_object_name));
        }
        if (group_first == nullptr &&
            !group.tool_object_names.empty()) {
            group_first = findChild<QWidget*>(
                QString::fromLatin1(
                    group.tool_object_names.front()));
        }
        const int insert_at = group_first != nullptr
            ? editor_tools_layout_->indexOf(group_first) : -1;
        editor_tools_layout_->insertWidget(
            insert_at >= 0 ? insert_at :
                std::max(0, editor_tools_layout_->count() - 1),
            dropdown);
        dropdown->hide();
        toolbar_group_dropdowns_.insert(key, dropdown);
    }
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

    // Restore only overflow-hidden original tool widgets of the CURRENT
    // Part/Sketch context. Keep the other context's hidden widgets in
    // its own bucket: changing modes must neither resurrect inactive
    // tool commands nor permanently lose the previous mode's buttons.
    auto& hidden_for_context =
        toolbar_overflow_hidden_by_context_[context_name];
    for (QWidget* widget : hidden_for_context) {
        if (belongs_to_context(widget)) widget->show();
    }
    hidden_for_context.clear();

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
        const QString group_key = context_name +
            QStringLiteral("_") + QString::fromLatin1(group.name);
        width += count(toolbar_pin_markers_.value(
            group_key, nullptr));
        width += count(toolbar_group_handles_.value(
            group_key, nullptr));
        return width;
    };
    int required = 0;
    for (const auto& group : groups) {
        required += group_width(group);
    }
    // Measure the WHOLE toolbar row. The QScrollArea viewport can still
    // reflect its previous, collapsed size when a responsive layout pass
    // restores hidden controls; using it as the total budget kept More
    // incorrectly visible even on a 1400 px ordinary Part window.
    // Subtract fixed Document actions/recovery without ever hiding them.
    const auto* top_row = contextual_tools_scroll_->parentWidget();
    int non_contextual_width =
        document_actions_->sizeHint().width() +
        toolbar_more_->sizeHint().width();
    if (!tree_recovery_button_->isHidden()) {
        non_contextual_width +=
            tree_recovery_button_->sizeHint().width();
    }
    if (!right_recovery_button_->isHidden()) {
        non_contextual_width +=
            right_recovery_button_->sizeHint().width();
    }
    constexpr int fixed_row_gutters = 64;
    const int available = std::max(
        0, (top_row == nullptr ? 0 : top_row->width()) -
           non_contextual_width - fixed_row_gutters);
    // A direct group dropdown does not need the More button's width.
    // Reserve that width only for the final fallback, otherwise direct
    // grouped tools and pinned Create collapse too early on Windows.
    const int direct_available =
        available + toolbar_more_->sizeHint().width();
    if (required <= direct_available) {
        toolbar_more_->hide();
        return;
    }
    const auto collapse_group = [&](const ToolbarGroupSpec& group) {
        const auto hide_one = [&](QWidget* widget) {
            if (widget == nullptr || widget->isHidden()) return;
            required -= widget->sizeHint().width() + gap;
            widget->hide();
            hidden_for_context.push_back(widget);
        };
        if (*group.label_object_name != '\0') {
            hide_one(findChild<QWidget*>(
                QString::fromLatin1(group.label_object_name)));
        }
        for (const char* id : group.tool_object_names) {
            hide_one(findChild<QPushButton*>(
                QString::fromLatin1(id)));
        }
        const QString group_key = context_name +
            QStringLiteral("_") + QString::fromLatin1(group.name);
        hide_one(toolbar_pin_markers_.value(
            group_key, nullptr));
        hide_one(toolbar_group_handles_.value(
            group_key, nullptr));
    };
    // Tier 1: collapse unpinned whole groups into named, direct menus.
    // Place each dropdown where its group used to be. They can be reached
    // without the last-resort two-level More menu at moderate widths.
    std::vector<QToolButton*> direct_dropdowns;
    const auto try_named_group = [&](const ToolbarGroupSpec& group) {
        if (group.pinned || required <= direct_available) return;
        const auto key = context_name + QStringLiteral("_") +
            QString::fromLatin1(group.name);
        auto* dropdown = toolbar_group_dropdowns_.value(
            key, nullptr);
        if (dropdown == nullptr) return;
        const int before = group_width(group);
        const int named_width = dropdown->sizeHint().width() + gap;
        if (before <= named_width) return;
        collapse_group(group);
        dropdown->show();
        required += named_width;
        direct_dropdowns.push_back(dropdown);
    };
    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
        try_named_group(*it);
    }
    if (required <= direct_available) {
        toolbar_more_->hide();
        return;
    }

    // Tier 2: pinned groups retain their ORIGINAL buttons and keep
    // checked/disabled/focus/shortcut semantics; only the presentation
    // changes to a neutral CAD icon and an accessible tooltip/name.
    // Labels are restored before each recalculation; no command proxy.
    const auto compact_pinned = [&](const ToolbarGroupSpec& group) {
        if (!group.pinned || required <= direct_available) return;
        for (const char* name : group.tool_object_names) {
            if (required <= direct_available) break;
            auto* button = findChild<QPushButton*>(
                QString::fromLatin1(name));
            if (button == nullptr || button->isHidden() ||
                button->text().isEmpty()) continue;
            const QString label = button->text();
            const int previous_width =
                button->sizeHint().width() + gap;
            if (button->icon().isNull()) {
                button->setIcon(makeCadToolbarIcon(
                    button->objectName(),
                    button->palette().buttonText().color()));
                button->setIconSize(QSize{20, 20});
            }
            button->setAccessibleName(label);
            if (button->toolTip().isEmpty()) {
                button->setToolTip(label);
            }
            toolbar_icon_compacted_.insert(button, label);
            button->setText({});
            required -= previous_width -
                (button->sizeHint().width() + gap);
        }
    };
    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
        compact_pinned(*it);
    }
    if (required <= direct_available) {
        toolbar_more_->hide();
        return;
    }

    // Tier 3: a still-too-wide pinned icon group can become a DIRECT
    // named dropdown. It delegates to the same connected CAD widgets
    // and retains their accessible labels, checks and enabled states.
    std::vector<QToolButton*> pinned_dropdowns;
    const auto try_named_pinned = [&](const ToolbarGroupSpec& group) {
        if (!group.pinned || required <= direct_available) return;
        const auto key = context_name + QStringLiteral("_") +
            QString::fromLatin1(group.name);
        auto* dropdown = toolbar_group_dropdowns_.value(
            key, nullptr);
        if (dropdown == nullptr) return;
        const int before = group_width(group);
        const int named_width = dropdown->sizeHint().width() + gap;
        if (before <= named_width) return;
        collapse_group(group);
        dropdown->show();
        required += named_width;
        pinned_dropdowns.push_back(dropdown);
    };
    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
        try_named_pinned(*it);
    }
    if (required <= direct_available) {
        toolbar_more_->hide();
        return;
    }

    // Conservative final fallback when even direct named groups cannot
    // fit: More -> group -> original action. No action is discarded.
    // open: no command is discarded, every button remains accessible in
    // More -> named group -> original tool. Never hide Document controls.
    toolbar_more_->show();
    for (auto it = groups.rbegin(); it != groups.rend(); ++it) {
        if (required <= available) break;
        if (it->pinned) collapse_group(*it);
    }
    if (required > available) {
        for (auto* dropdown : direct_dropdowns) {
            if (required <= available) break;
            required -= dropdown->sizeHint().width() + gap;
            dropdown->hide();
        }
        for (auto* dropdown : pinned_dropdowns) {
            if (required <= available) break;
            required -= dropdown->sizeHint().width() + gap;
            dropdown->hide();
        }
    }
    rebuildToolbarMoreMenu();
}

void CadWorkbenchShell::updateToolbarGroupFrames() {
    if (contextual_tools_scroll_ == nullptr) return;
    auto* host = contextual_tools_scroll_->widget();
    if (host == nullptr) return;
    QSet<QString> visible_frames;
    const QString context = toolbar_sketch_context_
        ? QStringLiteral("Sketch") : QStringLiteral("Part");
    for (const auto& group : toolbarGroups(toolbar_sketch_context_)) {
        const QString name = QString::fromLatin1(group.name);
        if (toolbar_sketch_context_ &&
            name == QStringLiteral("Select")) {
            continue; // fixed base tool, intentionally has no group box
        }
        const QString key = context + QStringLiteral("_") + name;
        auto* frame = toolbar_group_frames_.value(key, nullptr);
        if (frame == nullptr) {
            frame = new QFrame(host);
            frame->setObjectName(
                QStringLiteral("workbenchToolbarGroupFrame_") + key);
            frame->setAttribute(Qt::WA_TransparentForMouseEvents);
            frame->setFocusPolicy(Qt::NoFocus);
            frame->setStyleSheet(QStringLiteral(
                "QFrame { border: 1px solid palette(midlight);"
                " background-color: palette(alternate-base);"
                " border-radius: 4px; }"));
            frame->hide();
            toolbar_group_frames_.insert(key, frame);
        }

        QRect bounds;
        bool any = false;
        const auto collect = [&](QWidget* widget) {
            if (widget == nullptr || widget->isHidden() ||
                widget->width() <= 0 || widget->height() <= 0 ||
                widget->parentWidget() != host) return;
            const QRect rect{
                widget->mapTo(host, QPoint{0, 0}), widget->size()};
            bounds = any ? bounds.united(rect) : rect;
            any = true;
        };
        if (*group.label_object_name != '\0') {
            collect(findChild<QWidget*>(
                QString::fromLatin1(group.label_object_name)));
        }
        collect(toolbar_pin_markers_.value(key, nullptr));
        collect(toolbar_group_handles_.value(key, nullptr));
        for (const char* object_name : group.tool_object_names) {
            collect(findChild<QPushButton*>(
                QString::fromLatin1(object_name)));
        }
        collect(toolbar_group_dropdowns_.value(key, nullptr));
        if (!any) continue;
        // Paint behind the existing widgets: neither the single-row
        // layout's measured width nor native command identity changes.
        const QRect border_rect =
            bounds.adjusted(-3, -3, 3, 3).intersected(host->rect());
        if (frame->geometry() != border_rect) {
            frame->setGeometry(border_rect);
        }
        if (frame->isHidden()) frame->show();
        frame->lower();
        visible_frames.insert(key);
    }
    // Avoid hide/show on every resize callback: that can trigger
    // needless LayoutRequest feedback in the QScrollArea host.
    for (auto it = toolbar_group_frames_.begin();
         it != toolbar_group_frames_.end(); ++it) {
        if (it.value() && !visible_frames.contains(it.key()) &&
            !it.value()->isHidden()) {
            it.value()->hide();
        }
    }
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
