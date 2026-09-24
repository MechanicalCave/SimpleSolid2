#pragma once

#include <QWidget>

class QHBoxLayout;
class QLabel;
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
    void setPropertiesContent(QWidget* widget);
    void setOperationsContent(QWidget* widget);

private:
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

    QWidget* editor_surface_{};
    QWidget* properties_content_{};
    QWidget* operations_content_{};
};

} // namespace simplesolid2::ui
