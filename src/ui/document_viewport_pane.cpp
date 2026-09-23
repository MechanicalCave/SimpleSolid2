#include "document_viewport_pane.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QPointer>
#include <QResizeEvent>
#include <QToolButton>

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace simplesolid2::ui {
namespace {

QToolButton* makeButton(
    QWidget& parent,
    const QString& text,
    const QString& object_name) {
    auto* button = new QToolButton(&parent);
    button->setText(text);
    button->setObjectName(object_name);
    button->setAutoRaise(false);
    button->setCheckable(false);
    button->setMinimumSize(48, 28);
    return button;
}

} // namespace

DocumentViewportPane::DocumentViewportPane(
    QWidget* viewport_widget,
    viewer::IDocumentViewport& viewport,
    QWidget* parent)
    : QWidget{parent},
      viewport_widget_{viewport_widget},
      viewport_{&viewport} {
    setObjectName(QStringLiteral("editorSurface"));

    viewport_widget_->setParent(this);
    viewport_widget_->setObjectName(
        QStringLiteral("nativeViewportSurface"));
    viewport_widget_->show();

    navigation_panel_ = new QFrame(this);
    navigation_panel_->setObjectName(
        QStringLiteral("viewCube"));
    navigation_panel_->setFrameShape(QFrame::StyledPanel);
    navigation_panel_->setAutoFillBackground(true);

    auto* grid = new QGridLayout(navigation_panel_);
    grid->setContentsMargins(5, 5, 5, 5);
    grid->setHorizontalSpacing(3);
    grid->setVerticalSpacing(3);

    const auto add_view = [
        this,
        grid
    ](
        int row,
        int column,
        const QString& text,
        const QString& object_name,
        viewer::StandardView view) {
        auto* button =
            makeButton(
                *navigation_panel_,
                text,
                object_name);
        button->setCheckable(true);
        grid->addWidget(button, row, column);

        standard_view_buttons_.push_back(
            StandardViewButton{button, view});

        QObject::connect(
            button,
            &QToolButton::clicked,
            this,
            [this, view] {
                if (viewport_ != nullptr) {
                    static_cast<void>(
                        viewport_->setStandardView(view));
                }
            });
    };

    add_view(
        0,
        0,
        QStringLiteral("Back"),
        QStringLiteral("viewBackButton"),
        viewer::StandardView::back);
    add_view(
        0,
        1,
        QStringLiteral("Top"),
        QStringLiteral("viewTopButton"),
        viewer::StandardView::top);
    add_view(
        0,
        2,
        QStringLiteral("ISO"),
        QStringLiteral("viewIsoButton"),
        viewer::StandardView::isometric);

    add_view(
        1,
        0,
        QStringLiteral("Left"),
        QStringLiteral("viewLeftButton"),
        viewer::StandardView::left);
    add_view(
        1,
        1,
        QStringLiteral("Front"),
        QStringLiteral("viewFrontButton"),
        viewer::StandardView::front);
    add_view(
        1,
        2,
        QStringLiteral("Right"),
        QStringLiteral("viewRightButton"),
        viewer::StandardView::right);

    auto* fit_button =
        makeButton(
            *navigation_panel_,
            QStringLiteral("Fit"),
            QStringLiteral("fitAllButton"));
    grid->addWidget(fit_button, 2, 0);

    add_view(
        2,
        1,
        QStringLiteral("Bottom"),
        QStringLiteral("viewBottomButton"),
        viewer::StandardView::bottom);

    projection_button_ =
        makeButton(
            *navigation_panel_,
            QStringLiteral("Ortho"),
            QStringLiteral("projectionToggleButton"));
    grid->addWidget(projection_button_, 2, 2);

    QObject::connect(
        fit_button,
        &QToolButton::clicked,
        this,
        [this] {
            if (viewport_ != nullptr) {
                viewport_->fitAll();
            }
        });

    QObject::connect(
        projection_button_,
        &QToolButton::clicked,
        this,
        [this] { toggleProjection(); });

    const QPointer<DocumentViewportPane> self{this};
    viewport_->setCameraStateChangedHandler(
        [self](const viewer::CameraState& state) {
            if (self) {
                self->syncFromCamera(state);
            }
        });

    if (const auto state = viewport_->cameraState()) {
        syncFromCamera(*state);
    }

    navigation_panel_->adjustSize();
    navigation_panel_->raise();
}

DocumentViewportPane::~DocumentViewportPane() {
    if (viewport_ != nullptr) {
        viewport_->setCameraStateChangedHandler({});
    }
}

void DocumentViewportPane::resizeEvent(
    QResizeEvent* event) {
    QWidget::resizeEvent(event);

    if (viewport_widget_ != nullptr) {
        viewport_widget_->setGeometry(rect());
    }

    if (navigation_panel_ != nullptr) {
        navigation_panel_->adjustSize();
        constexpr int margin = 10;
        navigation_panel_->move(
            std::max(
                margin,
                width() -
                    navigation_panel_->width() -
                    margin),
            margin);
        navigation_panel_->raise();
    }
}

void DocumentViewportPane::syncFromCamera(
    const viewer::CameraState& state) {
    projection_button_->setText(
        state.projection ==
                viewer::CameraProjection::orthographic
            ? QStringLiteral("Ortho")
            : QStringLiteral("Persp"));

    const auto direction =
        viewer::normalized(state.target - state.eye);
    if (!direction) {
        for (const auto& entry : standard_view_buttons_) {
            entry.button->setChecked(false);
        }
        return;
    }

    double best_score =
        -std::numeric_limits<double>::infinity();
    QToolButton* best_button = nullptr;

    for (const auto& entry : standard_view_buttons_) {
        const auto orientation =
            viewer::standardViewOrientation(entry.view);
        const auto standard_direction =
            viewer::normalized(
                orientation.view_direction);
        if (!standard_direction) continue;

        const auto score =
            viewer::dot(
                *direction,
                *standard_direction);
        if (score > best_score) {
            best_score = score;
            best_button = entry.button;
        }
    }

    for (const auto& entry : standard_view_buttons_) {
        entry.button->setChecked(
            entry.button == best_button);
    }
}

void DocumentViewportPane::toggleProjection() {
    if (viewport_ == nullptr) return;

    const auto state = viewport_->cameraState();
    if (!state) return;

    const auto next =
        state->projection ==
                viewer::CameraProjection::orthographic
            ? viewer::CameraProjection::perspective
            : viewer::CameraProjection::orthographic;

    static_cast<void>(
        viewport_->setProjection(next));
}

} // namespace simplesolid2::ui
