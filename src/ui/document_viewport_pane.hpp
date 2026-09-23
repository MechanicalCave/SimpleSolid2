#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QWidget>

class QAction;
class QContextMenuEvent;
class QHBoxLayout;
class QMouseEvent;
class QPaintEvent;
class QPushButton;
class QTimer;
class QVBoxLayout;

namespace simplesolid2::ui {

class ViewCubeWidget final : public QWidget {
public:
    explicit ViewCubeWidget(QWidget* parent = nullptr);

    void setViewport(
        viewer::IDocumentViewport* viewport);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void contextMenuEvent(
        QContextMenuEvent* event) override;

private:
    void triggerStandardView(
        viewer::StandardView view);

    viewer::IDocumentViewport* viewport_{};

    QAction* front_action_{};
    QAction* back_action_{};
    QAction* left_action_{};
    QAction* right_action_{};
    QAction* top_action_{};
    QAction* bottom_action_{};
    QAction* isometric_action_{};
    QTimer* refresh_timer_{};
};

class DocumentViewportPane final : public QWidget {
public:
    explicit DocumentViewportPane(
        QWidget* parent = nullptr);

    [[nodiscard]] QWidget& viewportHost() noexcept {
        return *viewport_host_;
    }

    void setViewportSurface(
        QWidget* widget,
        viewer::IDocumentViewport* viewport);

    [[nodiscard]] viewer::IDocumentViewport*
    viewport() const noexcept {
        return viewport_;
    }

private:
    void refreshProjectionText();

    QWidget* viewport_host_{};
    QVBoxLayout* viewport_layout_{};
    ViewCubeWidget* view_cube_{};
    QPushButton* fit_button_{};
    QPushButton* projection_button_{};
    QTimer* refresh_timer_{};
    QWidget* viewport_widget_{};
    viewer::IDocumentViewport* viewport_{};
};

} // namespace simplesolid2::ui
