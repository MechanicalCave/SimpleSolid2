#pragma once

#include <QWidget>
#include <QHash>
#include <QPoint>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <vector>

class QEvent;
class QGroupBox;
class QHBoxLayout;
class QMenu;
class QPushButton;
class QLabel;
class QResizeEvent;
class QScrollArea;
class QSplitter;
class QStackedWidget;
class QTabWidget;
class QToolButton;
class QTreeWidget;
class QVBoxLayout;
class QWidget;

namespace simplesolid2::ui {

class CadWorkbenchShell final : public QWidget {
public:
    explicit CadWorkbenchShell(QWidget* parent = nullptr);

    [[nodiscard]] QHBoxLayout& documentActionsLayout() noexcept;
    [[nodiscard]] QHBoxLayout& editorToolsLayout() noexcept;
    [[nodiscard]] QTreeWidget& documentTree() noexcept;
    [[nodiscard]] QLabel& statusLabel() noexcept;

    void setEditorSurface(QWidget* widget);
    void setCommandLineContent(QWidget* widget);
    void setPropertiesContent(QWidget* widget);
    void setOperationsContent(QWidget* widget);
    [[nodiscard]] double rightPanelRatio() const;
    void setRightPanelRatio(double ratio);
    void setOperationsActive(bool active);
    void registerOperationsFooterRow(
        const QString& mode,
        QWidget* primary,
        QWidget* secondary = nullptr);
    void showOperationsFooterRow(const QString& mode);
    void setModeCue(const QString& mode);
    void setToolbarContext(bool sketch_context);

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum class ResponsiveRegime {
        normal,
        compact,
        narrow,
    };

    void updateResponsiveLayout(bool force = false);
    void updateRightPanelHeightMode();
    void refreshToolbarOverflow();
    void rebuildToolbarMoreMenu();
    void setToolbarCustomizeActive(bool active);
    void moveToolbarGroup(
        const QString& context,
        const QString& group,
        int direction);
    void resetToolbarGroups(const QString& context);
    void applyToolbarGroupOrder(const QString& context);
    [[nodiscard]] QString toolbarActionLabel(
        const QPushButton* button) const;
    void applyRightPanelRatio();
    void setNarrowPanel(QWidget* panel);
    [[nodiscard]] bool panelContainsFocus(
        const QWidget* panel) const;

    static void replaceContent(
        QVBoxLayout& layout,
        QWidget*& current,
        QWidget* replacement);
    static void replaceScrollContent(
        QScrollArea& scroll,
        QWidget*& current,
        QWidget* replacement);

    QHBoxLayout* document_actions_{};
    QTreeWidget* document_tree_{};
    QVBoxLayout* editor_host_layout_{};
    QHBoxLayout* editor_tools_layout_{};
    QVBoxLayout* properties_host_layout_{};
    QVBoxLayout* operations_host_layout_{};
    QLabel* status_{};
    QSplitter* splitter_{};
    QSplitter* right_vertical_splitter_{};
    QGroupBox* properties_group_{};
    QGroupBox* operations_group_{};
    QTabWidget* right_short_tabs_{};
    QStackedWidget* operations_footer_stack_{};
    QWidget* operations_footer_{};
    QLabel* viewport_mode_cue_{};
    QLabel* operations_mode_cue_{};
    QHash<QString, QWidget*> operations_footer_rows_;
    double right_panel_ratio_{0.4};
    int initial_font_line_height_{1};
    bool short_right_panel_mode_{};
    bool operations_active_{};
    QScrollArea* contextual_tools_scroll_{};
    QToolButton* toolbar_more_{};
    QToolButton* toolbar_customize_{};
    QHash<QString, QToolButton*> toolbar_pin_markers_;
    QHash<QString, QToolButton*> toolbar_group_handles_;
    QHash<QString, QStringList> toolbar_group_order_;
    QHash<QString, bool> toolbar_pin_overrides_;
    bool toolbar_customize_active_{};
    bool toolbar_order_initialized_{};
    QPointer<QToolButton> toolbar_drag_handle_;
    QPoint toolbar_drag_press_global_;
    QMenu* toolbar_more_menu_{};
    QHash<QString, QToolButton*> toolbar_group_dropdowns_;
    QHash<QPushButton*, QString> toolbar_icon_compacted_;
    // Overflow-hidden command widgets must survive switching away from
    // their Part/Sketch context until that SAME context is restored.
    QHash<QString, std::vector<QWidget*>> toolbar_overflow_hidden_by_context_;
    bool toolbar_sketch_context_{};
    bool refreshing_toolbar_{};
    QScrollArea* properties_scroll_{};
    QScrollArea* operations_scroll_{};
    QWidget* tree_panel_{};
    QWidget* editor_host_{};
    QWidget* right_panel_{};
    QToolButton* tree_recovery_button_{};
    QToolButton* right_recovery_button_{};
    ResponsiveRegime responsive_regime_{
        ResponsiveRegime::normal};
    bool responsive_initialized_{};

    QWidget* editor_surface_{};
    QWidget* command_line_content_{};
    QWidget* properties_content_{};
    QWidget* operations_content_{};
};

} // namespace simplesolid2::ui
