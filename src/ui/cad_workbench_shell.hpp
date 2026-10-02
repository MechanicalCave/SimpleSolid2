#pragma once

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QResizeEvent;
class QSplitter;
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

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class ResponsiveRegime {
        normal,
        compact,
        narrow,
    };

    void updateResponsiveLayout(bool force = false);
    void setNarrowPanel(QWidget* panel);
    [[nodiscard]] bool panelContainsFocus(
        const QWidget* panel) const;

    static void replaceContent(
        QVBoxLayout& layout,
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
