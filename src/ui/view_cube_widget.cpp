#include "view_cube_widget.hpp"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPolygonF>
#include <QShowEvent>
#include <QSizePolicy>
#include <QString>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr int overlayMargin = 8;
constexpr int compactThreshold = 250;
constexpr int fullWidth = 148;
constexpr int compactWidth = 88;

double squaredDistance(
    const QPointF& left,
    const QPointF& right) noexcept {
    const auto dx = left.x() - right.x();
    const auto dy = left.y() - right.y();
    return dx * dx + dy * dy;
}

} // namespace

class NavigationCubeCanvas final : public QWidget {
public:
    using ViewHandler =
        std::function<void(viewer::StandardView)>;

    NavigationCubeCanvas(
        ViewHandler handler,
        QWidget* parent)
        : QWidget{parent},
          handler_{std::move(handler)} {
        setObjectName(
            QStringLiteral("viewCubeCanvas"));
        setCursor(Qt::PointingHandCursor);
        setToolTip(
            QStringLiteral(
                "Click a visible face or corner. "
                "Use Views for every standard orientation."));
        setSizePolicy(
            QSizePolicy::Fixed,
            QSizePolicy::Fixed);
    }

    void setCompact(bool compact) {
        if (compact_ == compact) return;
        compact_ = compact;
        updateGeometry();
        update();
    }

    [[nodiscard]] QSize sizeHint() const override {
        return compact_
            ? QSize{72, 64}
            : QSize{116, 98};
    }

    [[nodiscard]] QSize minimumSizeHint() const override {
        return QSize{0, 0};
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter{this};
        painter.setRenderHint(
            QPainter::Antialiasing,
            true);

        const auto geometry =
            cubeGeometry(rect());

        const auto base = palette().button().color();
        const auto text = palette().buttonText().color();
        const auto edge = palette().mid().color();

        painter.setPen(QPen{edge, 1.2});

        painter.setBrush(base.lighter(118));
        painter.drawPolygon(geometry.top);

        painter.setBrush(base);
        painter.drawPolygon(geometry.front);

        painter.setBrush(base.darker(108));
        painter.drawPolygon(geometry.right);

        painter.setPen(QPen{text, 1.0});
        painter.setBrush(text);

        const auto corner_radius =
            compact_ ? 2.7 : 3.5;
        for (const auto& corner : geometry.corners) {
            painter.drawEllipse(
                corner.point,
                corner_radius,
                corner_radius);
        }

        if (!compact_) {
            painter.drawText(
                geometry.top.boundingRect(),
                Qt::AlignCenter,
                QStringLiteral("TOP"));
            painter.drawText(
                geometry.front.boundingRect(),
                Qt::AlignCenter,
                QStringLiteral("FRONT"));
            painter.drawText(
                geometry.right.boundingRect(),
                Qt::AlignCenter,
                QStringLiteral("RIGHT"));
        }
    }

    void mousePressEvent(
        QMouseEvent* event) override {
        if (event->button() != Qt::LeftButton ||
            !handler_) {
            QWidget::mousePressEvent(event);
            return;
        }

        const auto geometry =
            cubeGeometry(rect());
        const auto point = event->position();

        const auto corner_radius =
            compact_ ? 8.0 : 10.0;
        const auto corner_radius_sq =
            corner_radius * corner_radius;

        for (const auto& corner : geometry.corners) {
            if (squaredDistance(
                    point,
                    corner.point) <=
                corner_radius_sq) {
                handler_(corner.view);
                event->accept();
                return;
            }
        }

        if (geometry.top.containsPoint(
                point,
                Qt::OddEvenFill)) {
            handler_(viewer::StandardView::top);
            event->accept();
            return;
        }

        if (geometry.front.containsPoint(
                point,
                Qt::OddEvenFill)) {
            handler_(viewer::StandardView::front);
            event->accept();
            return;
        }

        if (geometry.right.containsPoint(
                point,
                Qt::OddEvenFill)) {
            handler_(viewer::StandardView::right);
            event->accept();
            return;
        }

        handler_(viewer::StandardView::isometric);
        event->accept();
    }

private:
    struct Corner final {
        QPointF point;
        viewer::StandardView view;
    };

    struct Geometry final {
        QPolygonF top;
        QPolygonF front;
        QPolygonF right;
        std::array<Corner, 7> corners;
    };

    [[nodiscard]] static Geometry cubeGeometry(
        const QRect& rect) {
        const double width =
            std::max(1, rect.width());
        const double height =
            std::max(1, rect.height());

        const QPointF a{
            width * 0.12,
            height * 0.30};
        const QPointF b{
            width * 0.50,
            height * 0.08};
        const QPointF c{
            width * 0.88,
            height * 0.30};
        const QPointF d{
            width * 0.50,
            height * 0.48};
        const QPointF e{
            width * 0.12,
            height * 0.68};
        const QPointF f{
            width * 0.50,
            height * 0.90};
        const QPointF g{
            width * 0.88,
            height * 0.68};

        Geometry geometry;
        geometry.top = QPolygonF{a, b, c, d};
        geometry.front = QPolygonF{a, d, f, e};
        geometry.right = QPolygonF{d, c, g, f};
        geometry.corners = {{
            {a, viewer::StandardView::top_front_left},
            {b, viewer::StandardView::top_back_left},
            {c, viewer::StandardView::top_back_right},
            {d, viewer::StandardView::top_front_right},
            {e, viewer::StandardView::bottom_front_left},
            {f, viewer::StandardView::bottom_front_right},
            {g, viewer::StandardView::bottom_back_right},
        }};
        return geometry;
    }

    ViewHandler handler_;
    bool compact_{};
};

ViewCubeWidget::ViewCubeWidget(
    viewer::IDocumentViewport* viewport,
    QWidget* parent)
    : QFrame{parent},
      viewport_{viewport} {
    setObjectName(
        QStringLiteral("viewCubeWidget"));
    setFrameShape(QFrame::StyledPanel);
    setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed);
    setMinimumSize(0, 0);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(2);

    cube_canvas_ =
        new NavigationCubeCanvas(
            [this](viewer::StandardView view) {
                applyStandardView(view);
            },
            this);
    root->addWidget(
        cube_canvas_,
        0,
        Qt::AlignHCenter);

    controls_layout_ = new QHBoxLayout;
    controls_layout_->setContentsMargins(
        0, 0, 0, 0);
    controls_layout_->setSpacing(2);

    views_button_ = new QToolButton(this);
    views_button_->setObjectName(
        QStringLiteral("viewCubeViewsButton"));
    views_button_->setPopupMode(
        QToolButton::InstantPopup);

    auto* views_menu =
        new QMenu(views_button_);

    addViewAction(
        *views_menu,
        QStringLiteral("Front"),
        QStringLiteral("viewCubeViewActionFront"),
        viewer::StandardView::front);
    addViewAction(
        *views_menu,
        QStringLiteral("Back"),
        QStringLiteral("viewCubeViewActionBack"),
        viewer::StandardView::back);
    addViewAction(
        *views_menu,
        QStringLiteral("Left"),
        QStringLiteral("viewCubeViewActionLeft"),
        viewer::StandardView::left);
    addViewAction(
        *views_menu,
        QStringLiteral("Right"),
        QStringLiteral("viewCubeViewActionRight"),
        viewer::StandardView::right);
    addViewAction(
        *views_menu,
        QStringLiteral("Top"),
        QStringLiteral("viewCubeViewActionTop"),
        viewer::StandardView::top);
    addViewAction(
        *views_menu,
        QStringLiteral("Bottom"),
        QStringLiteral("viewCubeViewActionBottom"),
        viewer::StandardView::bottom);

    views_menu->addSeparator();

    addViewAction(
        *views_menu,
        QStringLiteral("Isometric"),
        QStringLiteral("viewCubeViewActionIsometric"),
        viewer::StandardView::isometric);
    addViewAction(
        *views_menu,
        QStringLiteral("Top Front Left"),
        QStringLiteral("viewCubeViewActionTopFrontLeft"),
        viewer::StandardView::top_front_left);
    addViewAction(
        *views_menu,
        QStringLiteral("Top Front Right"),
        QStringLiteral("viewCubeViewActionTopFrontRight"),
        viewer::StandardView::top_front_right);
    addViewAction(
        *views_menu,
        QStringLiteral("Top Back Left"),
        QStringLiteral("viewCubeViewActionTopBackLeft"),
        viewer::StandardView::top_back_left);
    addViewAction(
        *views_menu,
        QStringLiteral("Top Back Right"),
        QStringLiteral("viewCubeViewActionTopBackRight"),
        viewer::StandardView::top_back_right);
    addViewAction(
        *views_menu,
        QStringLiteral("Bottom Front Left"),
        QStringLiteral("viewCubeViewActionBottomFrontLeft"),
        viewer::StandardView::bottom_front_left);
    addViewAction(
        *views_menu,
        QStringLiteral("Bottom Front Right"),
        QStringLiteral("viewCubeViewActionBottomFrontRight"),
        viewer::StandardView::bottom_front_right);
    addViewAction(
        *views_menu,
        QStringLiteral("Bottom Back Left"),
        QStringLiteral("viewCubeViewActionBottomBackLeft"),
        viewer::StandardView::bottom_back_left);
    addViewAction(
        *views_menu,
        QStringLiteral("Bottom Back Right"),
        QStringLiteral("viewCubeViewActionBottomBackRight"),
        viewer::StandardView::bottom_back_right);

    views_button_->setMenu(views_menu);

    fit_button_ = new QToolButton(this);
    fit_button_->setObjectName(
        QStringLiteral("viewCubeFitButton"));
    fit_button_->setToolTip(
        QStringLiteral("Fit All"));

    projection_button_ =
        new QToolButton(this);
    projection_button_->setObjectName(
        QStringLiteral(
            "viewCubeProjectionButton"));

    controls_layout_->addWidget(
        views_button_);
    controls_layout_->addWidget(
        fit_button_);
    controls_layout_->addWidget(
        projection_button_);
    root->addLayout(controls_layout_);

    QObject::connect(
        fit_button_,
        &QToolButton::clicked,
        this,
        [this] {
            if (viewport_ == nullptr) return;
            viewport_->fitAll();
        });

    QObject::connect(
        projection_button_,
        &QToolButton::clicked,
        this,
        [this] { toggleProjection(); });

    if (parentWidget() != nullptr) {
        parentWidget()->installEventFilter(this);
    }

    refreshProjectionLabel();
    syncEnabledState();
    updateResponsiveLayout();

    QTimer::singleShot(
        0,
        this,
        [this] {
            updateResponsiveLayout();
            repositionInsideParent();
            raise();
        });
}

ViewCubeWidget::~ViewCubeWidget() {
    if (parentWidget() != nullptr) {
        parentWidget()->removeEventFilter(this);
    }
}

QAction* ViewCubeWidget::addViewAction(
    QMenu& menu,
    const QString& text,
    const QString& object_name,
    viewer::StandardView view) {
    auto* action = menu.addAction(text);
    action->setObjectName(object_name);

    QObject::connect(
        action,
        &QAction::triggered,
        this,
        [this, view] {
            applyStandardView(view);
        });

    return action;
}

void ViewCubeWidget::setViewport(
    viewer::IDocumentViewport* viewport) {
    viewport_ = viewport;
    refreshProjectionLabel();
    syncEnabledState();
}

bool ViewCubeWidget::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (watched == parentWidget() &&
        (event->type() == QEvent::Resize ||
         event->type() == QEvent::Show ||
         event->type() == QEvent::LayoutRequest)) {
        updateResponsiveLayout();
        repositionInsideParent();
    }

    return QFrame::eventFilter(
        watched,
        event);
}

void ViewCubeWidget::showEvent(
    QShowEvent* event) {
    QFrame::showEvent(event);
    updateResponsiveLayout();
    repositionInsideParent();
    raise();
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
            compact_mode_
                ? QStringLiteral("P")
                : QStringLiteral("Projection"));
        projection_button_->setToolTip(
            QStringLiteral(
                "Orthographic / Perspective"));
        return;
    }

    const bool orthographic =
        camera->projection ==
        viewer::CameraProjection::orthographic;

    projection_button_->setText(
        compact_mode_
            ? (orthographic
                   ? QStringLiteral("O")
                   : QStringLiteral("P"))
            : (orthographic
                   ? QStringLiteral("Ortho")
                   : QStringLiteral("Persp")));
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

void ViewCubeWidget::updateResponsiveLayout() {
    const auto* parent = parentWidget();
    const auto available_width =
        parent != nullptr
            ? parent->contentsRect().width()
            : fullWidth;

    const bool compact =
        available_width < compactThreshold;

    if (compact_mode_ != compact) {
        compact_mode_ = compact;
        setProperty(
            "compactMode",
            compact_mode_);
        cube_canvas_->setCompact(
            compact_mode_);
    }

    views_button_->setText(
        compact_mode_
            ? QStringLiteral("V")
            : QStringLiteral("Views"));
    views_button_->setToolTip(
        QStringLiteral("Standard views"));

    fit_button_->setText(
        compact_mode_
            ? QStringLiteral("F")
            : QStringLiteral("Fit"));

    refreshProjectionLabel();

    const auto target_width =
        compact_mode_
            ? compactWidth
            : fullWidth;

    setFixedWidth(
        std::max(
            0,
            std::min(
                target_width,
                available_width -
                    2 * overlayMargin)));

    adjustSize();
}

void ViewCubeWidget::repositionInsideParent() {
    auto* parent = parentWidget();
    if (parent == nullptr) return;

    const auto bounds =
        parent->contentsRect();
    if (bounds.isEmpty()) return;

    auto desired = sizeHint();
    desired.setWidth(
        std::min(
            desired.width(),
            std::max(
                0,
                bounds.width() -
                    2 * overlayMargin)));
    desired.setHeight(
        std::min(
            desired.height(),
            std::max(
                0,
                bounds.height() -
                    2 * overlayMargin)));

    resize(desired);

    const int x =
        std::max(
            bounds.left(),
            bounds.right() -
                overlayMargin -
                width() +
                1);
    const int y =
        bounds.top() + overlayMargin;

    move(x, y);
    raise();
}

} // namespace simplesolid2::ui
