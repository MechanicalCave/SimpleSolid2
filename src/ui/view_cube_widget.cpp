#include "view_cube_widget.hpp"

#include <QGridLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QVBoxLayout>

namespace simplesolid2::ui {

ViewCubeWidget::ViewCubeWidget(
    viewer::IDocumentViewport* viewport,
    QWidget* parent)
    : QFrame{parent},
      viewport_{viewport} {
    setObjectName(QStringLiteral("viewCubeWidget"));
    setFrameShape(QFrame::StyledPanel);
    setSizePolicy(
        QSizePolicy::Maximum,
        QSizePolicy::Maximum);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(3);

    grid_ = new QGridLayout;
    grid_->setContentsMargins(0, 0, 0, 0);
    grid_->setHorizontalSpacing(2);
    grid_->setVerticalSpacing(2);

    addViewButton(
        QStringLiteral("TFL"),
        QStringLiteral("Top Front Left"),
        viewer::StandardView::top_front_left,
        0,
        0);
    addViewButton(
        QStringLiteral("TOP"),
        QStringLiteral("Top"),
        viewer::StandardView::top,
        0,
        1);
    addViewButton(
        QStringLiteral("TFR"),
        QStringLiteral("Top Front Right"),
        viewer::StandardView::top_front_right,
        0,
        2);

    addViewButton(
        QStringLiteral("LEFT"),
        QStringLiteral("Left"),
        viewer::StandardView::left,
        1,
        0);
    addViewButton(
        QStringLiteral("FRONT"),
        QStringLiteral("Front"),
        viewer::StandardView::front,
        1,
        1);
    addViewButton(
        QStringLiteral("RIGHT"),
        QStringLiteral("Right"),
        viewer::StandardView::right,
        1,
        2);

    addViewButton(
        QStringLiteral("TBL"),
        QStringLiteral("Top Back Left"),
        viewer::StandardView::top_back_left,
        2,
        0);
    addViewButton(
        QStringLiteral("BACK"),
        QStringLiteral("Back"),
        viewer::StandardView::back,
        2,
        1);
    addViewButton(
        QStringLiteral("TBR"),
        QStringLiteral("Top Back Right"),
        viewer::StandardView::top_back_right,
        2,
        2);

    addViewButton(
        QStringLiteral("BFL"),
        QStringLiteral("Bottom Front Left"),
        viewer::StandardView::bottom_front_left,
        3,
        0);
    addViewButton(
        QStringLiteral("BOTTOM"),
        QStringLiteral("Bottom"),
        viewer::StandardView::bottom,
        3,
        1);
    addViewButton(
        QStringLiteral("BFR"),
        QStringLiteral("Bottom Front Right"),
        viewer::StandardView::bottom_front_right,
        3,
        2);

    addViewButton(
        QStringLiteral("BBL"),
        QStringLiteral("Bottom Back Left"),
        viewer::StandardView::bottom_back_left,
        4,
        0);
    addViewButton(
        QStringLiteral("ISO"),
        QStringLiteral("Isometric"),
        viewer::StandardView::isometric,
        4,
        1);
    addViewButton(
        QStringLiteral("BBR"),
        QStringLiteral("Bottom Back Right"),
        viewer::StandardView::bottom_back_right,
        4,
        2);

    root->addLayout(grid_);

    auto* bottom = new QGridLayout;
    bottom->setContentsMargins(0, 0, 0, 0);

    fit_button_ =
        new QPushButton(QStringLiteral("Fit"), this);
    fit_button_->setObjectName(
        QStringLiteral("viewCubeFitButton"));
    fit_button_->setToolTip(
        QStringLiteral("Fit All"));

    projection_button_ =
        new QPushButton(this);
    projection_button_->setObjectName(
        QStringLiteral("viewCubeProjectionButton"));

    bottom->addWidget(fit_button_, 0, 0);
    bottom->addWidget(projection_button_, 0, 1);
    root->addLayout(bottom);

    QObject::connect(
        fit_button_,
        &QPushButton::clicked,
        this,
        [this] {
            if (viewport_ == nullptr) return;
            viewport_->fitAll();
        });

    QObject::connect(
        projection_button_,
        &QPushButton::clicked,
        this,
        [this] { toggleProjection(); });

    refreshProjectionLabel();
    syncEnabledState();
}

QPushButton* ViewCubeWidget::addViewButton(
    const QString& text,
    const QString& tooltip,
    viewer::StandardView view,
    int row,
    int column) {
    auto* button =
        new QPushButton(text, this);
    button->setToolTip(tooltip);
    button->setMaximumWidth(64);
    button->setMinimumWidth(44);
    button->setObjectName(
        QStringLiteral("viewCubeViewButton"));

    grid_->addWidget(
        button,
        row,
        column);

    QObject::connect(
        button,
        &QPushButton::clicked,
        this,
        [this, view] {
            applyStandardView(view);
        });

    return button;
}

void ViewCubeWidget::setViewport(
    viewer::IDocumentViewport* viewport) {
    viewport_ = viewport;
    refreshProjectionLabel();
    syncEnabledState();
}

void ViewCubeWidget::applyStandardView(
    viewer::StandardView view) {
    if (viewport_ == nullptr) return;

    static_cast<void>(
        viewport_->setStandardView(view));
    refreshProjectionLabel();
}

void ViewCubeWidget::toggleProjection() {
    if (viewport_ == nullptr) return;

    const auto camera =
        viewport_->cameraState();
    if (!camera) return;

    const auto next =
        camera->projection ==
                viewer::CameraProjection::orthographic
            ? viewer::CameraProjection::perspective
            : viewer::CameraProjection::orthographic;

    static_cast<void>(
        viewport_->setProjection(next));
    refreshProjectionLabel();
}

void ViewCubeWidget::refreshProjectionLabel() {
    if (projection_button_ == nullptr) return;

    const auto camera =
        viewport_ == nullptr
            ? std::optional<viewer::CameraState>{}
            : viewport_->cameraState();

    if (!camera) {
        projection_button_->setText(
            QStringLiteral("Projection"));
        projection_button_->setToolTip(
            QStringLiteral(
                "Orthographic / Perspective"));
        return;
    }

    const bool orthographic =
        camera->projection ==
        viewer::CameraProjection::orthographic;

    projection_button_->setText(
        orthographic
            ? QStringLiteral("Ortho")
            : QStringLiteral("Persp"));
    projection_button_->setToolTip(
        orthographic
            ? QStringLiteral(
                  "Switch to Perspective")
            : QStringLiteral(
                  "Switch to Orthographic"));
}

void ViewCubeWidget::syncEnabledState() {
    setEnabled(viewport_ != nullptr);
}

} // namespace simplesolid2::ui
