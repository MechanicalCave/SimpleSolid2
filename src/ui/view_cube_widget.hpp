#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QFrame>

class QAction;
class QEvent;
class QMenu;
class QPushButton;
class QString;
class QToolButton;
class QWidget;

namespace simplesolid2::ui {

class ViewCubeWidget final : public QFrame {
public:
    explicit ViewCubeWidget(
        viewer::IDocumentViewport* viewport,
        QWidget* parent = nullptr,
        QWidget* repaint_target = nullptr);

    void setViewport(
        viewer::IDocumentViewport* viewport);

    [[nodiscard]] bool compactMode() const noexcept {
        return compact_mode_;
    }

protected:
    bool eventFilter(
        QObject* watched,
        QEvent* event) override;

private:
    QAction* addViewAction(
        QMenu& menu,
        const QString& text,
        const QString& object_name,
        viewer::StandardView view);

    void applyStandardView(
        viewer::StandardView view);
    void toggleProjection();
    void refreshProjectionPresentation();
    void syncEnabledState();
    void syncOverlayGeometry();
    void scheduleUnderlayRefresh();
    void setCompactMode(bool compact);

    viewer::IDocumentViewport* viewport_{};
    QWidget* host_{};
    QWidget* repaint_target_{};
    QWidget* regular_panel_{};
    QWidget* cube_canvas_{};
    QToolButton* views_button_{};
    QToolButton* compact_button_{};
    QPushButton* fit_button_{};
    QPushButton* projection_button_{};
    QMenu* navigation_menu_{};
    QAction* fit_action_{};
    QAction* projection_action_{};
    bool compact_mode_{};
    bool underlay_refresh_scheduled_{};
};

} // namespace simplesolid2::ui
