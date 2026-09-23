#include "document_viewport_pane.hpp"

#include <QAction>
#include <QContextMenuEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QPalette>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>

namespace simplesolid2::ui {
namespace {

constexpr std::array<std::array<int, 2>, 12>
cube_edges{{
    {{0, 1}},
    {{0, 2}},
    {{0, 4}},
    {{1, 3}},
    {{1, 5}},
    {{2, 3}},
    {{2, 6}},
    {{3, 7}},
    {{4, 5}},
    {{4, 6}},
    {{5, 7}},
    {{6, 7}},
}};

viewer::Vec3 cubeVertex(int index) noexcept {
    return {
        (index & 1) != 0 ? 1.0 : -1.0,
        (index & 2) != 0 ? 1.0 : -1.0,
        (index & 4) != 0 ? 1.0 : -1.0,
    };
}

} // namespace

ViewCubeWidget::ViewCubeWidget(QWidget* parent)
    : QWidget{parent} {
    setObjectName(QStringLiteral("viewCube"));
    setFixedSize(104, 104);
    setToolTip(
        QStringLiteral(
            "Click the cube for common views. "
            "Right-click for all standard views."));

    const auto make_action =
        [this](
            const char* text,
            const char* object_name,
            viewer::StandardView view) {
            auto* action =
                new QAction(
                    QString::fromLatin1(text),
                    this);
            action->setObjectName(
                QString::fromLatin1(object_name));

            QObject::connect(
                action,
                &QAction::triggered,
                this,
                [this, view] {
                    triggerStandardView(view);
                });

            return action;
        };

    front_action_ =
        make_action(
            "Front",
            "viewFrontAction",
            viewer::StandardView::front);

    back_action_ =
        make_action(
            "Back",
            "viewBackAction",
            viewer::StandardView::back);

    left_action_ =
        make_action(
            "Left",
            "viewLeftAction",
            viewer::StandardView::left);

    right_action_ =
        make_action(
            "Right",
            "viewRightAction",
            viewer::StandardView::right);

    top_action_ =
        make_action(
            "Top",
            "viewTopAction",
            viewer::StandardView::top);

    bottom_action_ =
        make_action(
            "Bottom",
            "viewBottomAction",
            viewer::StandardView::bottom);

    isometric_action_ =
        make_action(
            "Isometric",
            "viewIsometricAction",
            viewer::StandardView::isometric);

    refresh_timer_ = new QTimer(this);
    refresh_timer_->setInterval(80);

    QObject::connect(
        refresh_timer_,
        &QTimer::timeout,
        this,
        [this] {
            update();
        });

    refresh_timer_->start();
}

void ViewCubeWidget::setViewport(
    viewer::IDocumentViewport* viewport) {
    viewport_ = viewport;
    update();
}

void ViewCubeWidget::paintEvent(
    QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter painter{this};
    painter.setRenderHint(
        QPainter::Antialiasing,
        true);

    painter.fillRect(
        rect(),
        palette().color(
            QPalette::Window));

    painter.setPen(
        palette().color(
            QPalette::Text));

    viewer::CameraState camera;
    bool have_camera = false;

    if (viewport_ != nullptr) {
        const auto state =
            viewport_->cameraState();
        if (state &&
            viewer::validateCameraState(
                *state)
                .valid) {
            camera = *state;
            have_camera = true;
        }
    }

    viewer::Vec3 right{
        1.0, 0.0, 0.0};
    viewer::Vec3 screen_up{
        0.0, 0.0, 1.0};

    if (have_camera) {
        const auto forward =
            viewer::normalized(
                camera.target -
                camera.eye);

        if (forward) {
            const auto maybe_right =
                viewer::normalized(
                    viewer::cross(
                        *forward,
                        camera.up));

            if (maybe_right) {
                right = *maybe_right;

                const auto maybe_up =
                    viewer::normalized(
                        viewer::cross(
                            right,
                            *forward));

                if (maybe_up) {
                    screen_up = *maybe_up;
                }
            }
        }
    }

    const QPointF center{
        width() * 0.5,
        height() * 0.5};

    const double scale =
        std::min(
            width(),
            height()) *
        0.27;

    std::array<QPointF, 8> projected;

    for (int index = 0;
         index < 8;
         ++index) {
        const auto vertex =
            cubeVertex(index);

        projected[
            static_cast<std::size_t>(
                index)] =
            {
                center.x() +
                    viewer::dot(
                        vertex,
                        right) *
                        scale,
                center.y() -
                    viewer::dot(
                        vertex,
                        screen_up) *
                        scale,
            };
    }

    for (const auto& edge :
         cube_edges) {
        painter.drawLine(
            projected[
                static_cast<std::size_t>(
                    edge[0])],
            projected[
                static_cast<std::size_t>(
                    edge[1])]);
    }

    painter.drawText(
        QRect{0, 0, width(), 20},
        Qt::AlignHCenter |
            Qt::AlignTop,
        QStringLiteral("TOP"));

    painter.drawText(
        QRect{
            0,
            height() - 20,
            width(),
            20},
        Qt::AlignHCenter |
            Qt::AlignBottom,
        QStringLiteral("FRONT"));

    painter.drawText(
        QRect{
            width() - 24,
            0,
            24,
            height()},
        Qt::AlignCenter,
        QStringLiteral("R"));
}

void ViewCubeWidget::mousePressEvent(
    QMouseEvent* event) {
    if (event->button() !=
        Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const auto point =
        event->position();

    if (point.y() <
        height() * 0.30) {
        triggerStandardView(
            viewer::StandardView::top);
    } else if (
        point.x() >
        width() * 0.70) {
        triggerStandardView(
            viewer::StandardView::right);
    } else if (
        point.x() <
        width() * 0.30) {
        triggerStandardView(
            viewer::StandardView::left);
    } else if (
        point.y() >
        height() * 0.70) {
        triggerStandardView(
            viewer::StandardView::front);
    } else {
        triggerStandardView(
            viewer::StandardView::isometric);
    }

    event->accept();
}

void ViewCubeWidget::contextMenuEvent(
    QContextMenuEvent* event) {
    QMenu menu{this};

    menu.addAction(front_action_);
    menu.addAction(back_action_);
    menu.addSeparator();
    menu.addAction(left_action_);
    menu.addAction(right_action_);
    menu.addSeparator();
    menu.addAction(top_action_);
    menu.addAction(bottom_action_);
    menu.addSeparator();
    menu.addAction(isometric_action_);

    menu.exec(event->globalPos());
    event->accept();
}

void ViewCubeWidget::triggerStandardView(
    viewer::StandardView view) {
    if (viewport_ == nullptr) return;

    static_cast<void>(
        viewport_->setStandardView(view));
    update();
}

DocumentViewportPane::DocumentViewportPane(
    QWidget* parent)
    : QWidget{parent} {
    setObjectName(
        QStringLiteral(
            "documentViewportPane"));

    auto* root =
        new QVBoxLayout(this);
    root->setContentsMargins(
        0, 0, 0, 0);
    root->setSpacing(4);

    auto* controls =
        new QHBoxLayout;

    fit_button_ =
        new QPushButton(
            QStringLiteral("Fit"),
            this);
    fit_button_->setObjectName(
        QStringLiteral("fitAllButton"));

    projection_button_ =
        new QPushButton(
            QStringLiteral(
                "Orthographic"),
            this);
    projection_button_->setObjectName(
        QStringLiteral(
            "projectionButton"));

    controls->addWidget(fit_button_);
    controls->addWidget(
        projection_button_);
    controls->addStretch(1);
    root->addLayout(controls);

    auto* content =
        new QHBoxLayout;
    content->setContentsMargins(
        0, 0, 0, 0);
    content->setSpacing(4);

    viewport_host_ =
        new QWidget(this);
    viewport_host_->setObjectName(
        QStringLiteral(
            "nativeViewportHost"));

    viewport_layout_ =
        new QVBoxLayout(viewport_host_);
    viewport_layout_->setContentsMargins(
        0, 0, 0, 0);

    content->addWidget(
        viewport_host_,
        1);

    auto* right_rail =
        new QVBoxLayout;
    view_cube_ =
        new ViewCubeWidget(this);

    right_rail->addWidget(
        view_cube_,
        0,
        Qt::AlignTop |
            Qt::AlignRight);
    right_rail->addStretch(1);

    content->addLayout(right_rail);
    root->addLayout(content, 1);

    QObject::connect(
        fit_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (viewport_ != nullptr) {
                viewport_->fitAll();
            }
        });

    QObject::connect(
        projection_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (viewport_ == nullptr) {
                return;
            }

            const auto state =
                viewport_->cameraState();
            if (!state) return;

            const auto next =
                state->projection ==
                        viewer::
                            CameraProjection::
                                orthographic
                    ? viewer::
                          CameraProjection::
                              perspective
                    : viewer::
                          CameraProjection::
                              orthographic;

            static_cast<void>(
                viewport_->
                    setProjection(next));

            refreshProjectionText();
        });

    refresh_timer_ =
        new QTimer(this);
    refresh_timer_->setInterval(120);

    QObject::connect(
        refresh_timer_,
        &QTimer::timeout,
        this,
        [this] {
            refreshProjectionText();
        });

    refresh_timer_->start();
    refreshProjectionText();
}

void DocumentViewportPane::
setViewportSurface(
    QWidget* widget,
    viewer::IDocumentViewport* viewport) {
    if (viewport_widget_ != nullptr &&
        viewport_widget_ != widget) {
        viewport_layout_->
            removeWidget(
                viewport_widget_);
        viewport_widget_->
            setParent(nullptr);
    }

    viewport_widget_ = widget;
    viewport_ = viewport;

    if (viewport_widget_ != nullptr) {
        if (viewport_widget_->
                parentWidget() !=
            viewport_host_) {
            viewport_widget_->
                setParent(viewport_host_);
        }

        viewport_layout_->addWidget(
            viewport_widget_,
            1);
    }

    view_cube_->setViewport(viewport_);
    refreshProjectionText();
}

void DocumentViewportPane::
refreshProjectionText() {
    if (viewport_ == nullptr) {
        projection_button_->setText(
            QStringLiteral("Projection"));
        projection_button_->setEnabled(false);
        fit_button_->setEnabled(false);
        return;
    }

    fit_button_->setEnabled(true);
    projection_button_->setEnabled(true);

    const auto state =
        viewport_->cameraState();

    if (!state) {
        projection_button_->setText(
            QStringLiteral("Projection"));
        return;
    }

    projection_button_->setText(
        state->projection ==
                viewer::CameraProjection::
                    orthographic
            ? QStringLiteral(
                  "Orthographic")
            : QStringLiteral(
                  "Perspective"));
}

} // namespace simplesolid2::ui
