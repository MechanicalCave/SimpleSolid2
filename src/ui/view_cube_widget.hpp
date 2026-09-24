#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QFrame>

class QAction;
class QEvent;
class QHBoxLayout;
class QMenu;
class QShowEvent;
class QString;
class QToolButton;

namespace simplesolid2::ui {

class NavigationCubeCanvas;

class ViewCubeWidget final : public QFrame {
public:
    explicit ViewCubeWidget(
        viewer::IDocumentViewport* viewport,
        QWidget* parent = nullptr);
    ~ViewCubeWidget() override;

    void setViewport(
        viewer::IDocumentViewport* viewport);

protected:
    bool eventFilter(
        QObject* watched,
        QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    QAction* addViewAction(
        QMenu& menu,
        const QString& text,
        const QString& object_name,
        viewer::StandardView view);

    void applyStandardView(
        viewer::StandardView view);
    void toggleProjection();
    void refreshProjectionLabel();
    void syncEnabledState();
    void updateResponsiveLayout();
    void repositionInsideParent();

    viewer::IDocumentViewport* viewport_{};
    NavigationCubeCanvas* cube_canvas_{};
    QToolButton* views_button_{};
    QToolButton* fit_button_{};
    QToolButton* projection_button_{};
    QHBoxLayout* controls_layout_{};
    bool compact_mode_{};
};

} // namespace simplesolid2::ui
