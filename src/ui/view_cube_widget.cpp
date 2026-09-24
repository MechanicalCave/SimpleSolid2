#include "view_cube_widget.hpp"

#include <QAction>
#include <QEvent>
#include <QHBoxLayout>
#include <QLineF>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QPolygonF>
#include <QPushButton>
#include <QSizePolicy>
#include <QString>
#include <QToolButton>
#include <QTimer>
#include <QVBoxLayout>

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <optional>

namespace simplesolid2::ui {
namespace {

constexpr QSize regularOverlaySize{132, 118};
constexpr QSize compactOverlaySize{54, 34};
constexpr int overlayMargin = 8;
constexpr int compactWidthThreshold = 180;
constexpr int compactHeightThreshold = 145;

class ViewCubeCanvas final : public QWidget {
public:
    using ViewHandler =
        std::function<void(viewer::StandardView)>;

    explicit ViewCubeCanvas(
        ViewHandler handler,
        QWidget* parent = nullptr)
        : QWidget{parent},
          handler_{std::move(handler)} {
        setObjectName(QStringLiteral("viewCubeCanvas"));
        setFixedSize(96, 76);
        setMouseTracking(true);
        setCursor(Qt::PointingHandCursor);
        setToolTip(QStringLiteral(
            "Click a face or corner to orient the view."));
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter{this};
        painter.setRenderHint(QPainter::Antialiasing, true);

        const auto palette_window =
            palette().color(QPalette::Window);
        const auto palette_mid =
            palette().color(QPalette::Mid);
        const auto palette_light =
            palette().color(QPalette::Light);
        const auto palette_button =
            palette().color(QPalette::Button);
        const auto palette_text =
            palette().color(QPalette::ButtonText);
        const auto palette_highlight =
            palette().color(QPalette::Highlight);

        painter.fillRect(rect(), palette_window);

        auto drawFace = [&](
            const QPolygonF& polygon,
            viewer::StandardView view,
            const QColor& fill) {
            painter.setPen(
                QPen{
                    hovered_ && *hovered_ == view
                        ? palette_highlight
                        : palette_mid,
                    hovered_ && *hovered_ == view
                        ? 2.0
                        : 1.0});
            painter.setBrush(fill);
            painter.drawPolygon(polygon);
        };

        drawFace(
            topPolygon(),
            viewer::StandardView::top,
            palette_light);
        drawFace(
            frontPolygon(),
            viewer::StandardView::front,
            palette_button);
        drawFace(
            rightPolygon(),
            viewer::StandardView::right,
            palette_mid.lighter(125));

        painter.setPen(
            QPen{palette_mid, 1.0, Qt::DashLine});
        painter.drawLine(vertex(H), vertex(D));
        painter.drawLine(vertex(H), vertex(G));
        painter.drawLine(vertex(H), vertex(E));

        for (const auto& hotspot : cornerHotspots()) {
            const bool hovered =
                hovered_ && *hovered_ == hotspot.view;
            painter.setPen(
                QPen{
                    hovered
                        ? palette_highlight
                        : palette_mid,
                    hovered ? 2.0 : 1.0});
            painter.setBrush(
                hovered
                    ? palette_highlight
                    : palette_window);
            painter.drawEllipse(
                hotspot.point,
                hovered ? 4.5 : 3.2,
                hovered ? 4.5 : 3.2);
        }

        const QRectF isoRect{2.0, 3.0, 20.0, 18.0};
        const bool isoHovered =
            hovered_ &&
            *hovered_ == viewer::StandardView::isometric;
        painter.setPen(
            QPen{
                isoHovered
                    ? palette_highlight
                    : palette_mid,
                isoHovered ? 2.0 : 1.0});
        painter.setBrush(
            isoHovered
                ? palette_highlight
                : palette_window);
        painter.drawRoundedRect(isoRect, 3.0, 3.0);
        painter.setPen(
            isoHovered
                ? palette().color(
                      QPalette::HighlightedText)
                : palette_text);
        painter.drawText(
            isoRect,
            Qt::AlignCenter,
            QStringLiteral("ISO"));
    }

    void mouseMoveEvent(QMouseEvent* event) override {
        const auto next = viewAt(event->position());
        if (next != hovered_) {
            hovered_ = next;
            update();
        }
        QWidget::mouseMoveEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        hovered_.reset();
        update();
        QWidget::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() != Qt::LeftButton) {
            QWidget::mousePressEvent(event);
            return;
        }

        const auto view = viewAt(event->position());
        if (view && handler_) {
            handler_(*view);
            event->accept();
            return;
        }

        QWidget::mousePressEvent(event);
    }

private:
    enum VertexIndex : std::size_t {
        A,
        B,
        C,
        D,
        E,
        F,
        G,
        H,
    };

    struct CornerHotspot final {
        QPointF point;
        viewer::StandardView view;
    };

    [[nodiscard]] static QPointF vertex(
        VertexIndex index) {
        constexpr std::array<QPointF, 8> vertices{
            QPointF{20.0, 28.0},
            QPointF{55.0, 28.0},
            QPointF{78.0, 12.0},
            QPointF{43.0, 12.0},
            QPointF{20.0, 60.0},
            QPointF{55.0, 60.0},
            QPointF{78.0, 44.0},
            QPointF{43.0, 44.0},
        };
        return vertices[index];
    }

    [[nodiscard]] static QPolygonF topPolygon() {
        return QPolygonF{
            vertex(D),
            vertex(C),
            vertex(B),
            vertex(A)};
    }

    [[nodiscard]] static QPolygonF frontPolygon() {
        return QPolygonF{
            vertex(A),
            vertex(B),
            vertex(F),
            vertex(E)};
    }

    [[nodiscard]] static QPolygonF rightPolygon() {
        return QPolygonF{
            vertex(B),
            vertex(C),
            vertex(G),
            vertex(F)};
    }

    [[nodiscard]] static std::array<
        CornerHotspot,
        8> cornerHotspots() {
        return {{
            {vertex(A),
             viewer::StandardView::top_front_left},
            {vertex(B),
             viewer::StandardView::top_front_right},
            {vertex(C),
             viewer::StandardView::top_back_right},
            {vertex(D),
             viewer::StandardView::top_back_left},
            {vertex(E),
             viewer::StandardView::bottom_front_left},
            {vertex(F),
             viewer::StandardView::bottom_front_right},
            {vertex(G),
             viewer::StandardView::bottom_back_right},
            {vertex(H),
             viewer::StandardView::bottom_back_left},
        }};
    }

    [[nodiscard]] static bool near(
        const QPointF& a,
        const QPointF& b,
        double radius) {
        return QLineF{a, b}.length() <= radius;
    }

    [[nodiscard]] std::optional<viewer::StandardView>
    viewAt(const QPointF& point) const {
        if (QRectF{2.0, 3.0, 20.0, 18.0}
                .contains(point)) {
            return viewer::StandardView::isometric;
        }

        for (const auto& hotspot : cornerHotspots()) {
            if (near(point, hotspot.point, 6.5)) {
                return hotspot.view;
            }
        }

        if (topPolygon().containsPoint(
                point,
                Qt::OddEvenFill)) {
            return viewer::StandardView::top;
        }
        if (rightPolygon().containsPoint(
                point,
                Qt::OddEvenFill)) {
            return viewer::StandardView::right;
        }
        if (frontPolygon().containsPoint(
                point,
                Qt::OddEvenFill)) {
            return viewer::StandardView::front;
        }

        return std::nullopt;
    }

    ViewHandler handler_;
    std::optional<viewer::StandardView> hovered_;
};

} // namespace

ViewCubeWidget::ViewCubeWidget(
    viewer::IDocumentViewport* viewport,
    QWidget* parent,
    QWidget* repaint_target)
    : QFrame{parent},
      viewport_{viewport},
      host_{parent},
      repaint_target_{repaint_target} {
    setObjectName(QStringLiteral("viewCubeWidget"));
    setFrameShape(QFrame::StyledPanel);
    setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(4, 4, 4, 4);
    root->setSpacing(2);

    navigation_menu_ = new QMenu(this);
    navigation_menu_->setObjectName(
        QStringLiteral("viewCubeNavigationMenu"));

    addViewAction(
        *navigation_menu_,
        QStringLiteral("Front"),
        QStringLiteral("viewCubeFrontAction"),
        viewer::StandardView::front);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Back"),
        QStringLiteral("viewCubeBackAction"),
        viewer::StandardView::back);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Left"),
        QStringLiteral("viewCubeLeftAction"),
        viewer::StandardView::left);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Right"),
        QStringLiteral("viewCubeRightAction"),
        viewer::StandardView::right);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Top"),
        QStringLiteral("viewCubeTopAction"),
        viewer::StandardView::top);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Bottom"),
        QStringLiteral("viewCubeBottomAction"),
        viewer::StandardView::bottom);

    navigation_menu_->addSeparator();

    addViewAction(
        *navigation_menu_,
        QStringLiteral("Isometric"),
        QStringLiteral("viewCubeIsometricAction"),
        viewer::StandardView::isometric);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Top Front Left"),
        QStringLiteral("viewCubeTopFrontLeftAction"),
        viewer::StandardView::top_front_left);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Top Front Right"),
        QStringLiteral("viewCubeTopFrontRightAction"),
        viewer::StandardView::top_front_right);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Top Back Left"),
        QStringLiteral("viewCubeTopBackLeftAction"),
        viewer::StandardView::top_back_left);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Top Back Right"),
        QStringLiteral("viewCubeTopBackRightAction"),
        viewer::StandardView::top_back_right);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Bottom Front Left"),
        QStringLiteral("viewCubeBottomFrontLeftAction"),
        viewer::StandardView::bottom_front_left);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Bottom Front Right"),
        QStringLiteral("viewCubeBottomFrontRightAction"),
        viewer::StandardView::bottom_front_right);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Bottom Back Left"),
        QStringLiteral("viewCubeBottomBackLeftAction"),
        viewer::StandardView::bottom_back_left);
    addViewAction(
        *navigation_menu_,
        QStringLiteral("Bottom Back Right"),
        QStringLiteral("viewCubeBottomBackRightAction"),
        viewer::StandardView::bottom_back_right);

    navigation_menu_->addSeparator();

    fit_action_ =
        navigation_menu_->addAction(
            QStringLiteral("Fit"));
    fit_action_->setObjectName(
        QStringLiteral("viewCubeFitAction"));
    QObject::connect(
        fit_action_,
        &QAction::triggered,
        this,
        [this] {
            if (viewport_ != nullptr) {
                viewport_->fitAll();
            }
        });

    projection_action_ =
        navigation_menu_->addAction(QString{});
    projection_action_->setObjectName(
        QStringLiteral("viewCubeProjectionAction"));
    QObject::connect(
        projection_action_,
        &QAction::triggered,
        this,
        [this] { toggleProjection(); });

    regular_panel_ = new QWidget(this);
    regular_panel_->setObjectName(
        QStringLiteral("viewCubeRegularPanel"));
    auto* regular_layout =
        new QVBoxLayout(regular_panel_);
    regular_layout->setContentsMargins(0, 0, 0, 0);
    regular_layout->setSpacing(1);

    cube_canvas_ =
        new ViewCubeCanvas(
            [this](viewer::StandardView view) {
                applyStandardView(view);
            },
            regular_panel_);
    regular_layout->addWidget(
        cube_canvas_,
        0,
        Qt::AlignHCenter);

    auto* controls = new QHBoxLayout;
    controls->setContentsMargins(0, 0, 0, 0);
    controls->setSpacing(2);

    views_button_ =
        new QToolButton(regular_panel_);
    views_button_->setObjectName(
        QStringLiteral("viewCubeViewsButton"));
    views_button_->setText(QStringLiteral("Views"));
    views_button_->setMenu(navigation_menu_);
    views_button_->setPopupMode(
        QToolButton::InstantPopup);
    views_button_->setToolTip(
        QStringLiteral("All standard views"));

    fit_button_ =
        new QPushButton(
            QStringLiteral("Fit"),
            regular_panel_);
    fit_button_->setObjectName(
        QStringLiteral("viewCubeFitButton"));
    fit_button_->setToolTip(
        QStringLiteral("Fit All"));

    projection_button_ =
        new QPushButton(regular_panel_);
    projection_button_->setObjectName(
        QStringLiteral("viewCubeProjectionButton"));

    controls->addWidget(views_button_);
    controls->addWidget(fit_button_);
    controls->addWidget(projection_button_);
    regular_layout->addLayout(controls);
    root->addWidget(regular_panel_);

    compact_button_ =
        new QToolButton(this);
    compact_button_->setObjectName(
        QStringLiteral("viewCubeCompactButton"));
    compact_button_->setText(QStringLiteral("View"));
    compact_button_->setToolTip(
        QStringLiteral("View navigation"));
    compact_button_->setMenu(navigation_menu_);
    compact_button_->setPopupMode(
        QToolButton::InstantPopup);
    compact_button_->setVisible(false);
    root->addWidget(compact_button_);

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
        [this] { toggleProjection(); });

    if (host_ != nullptr) {
        host_->installEventFilter(this);
    }

    refreshProjectionPresentation();
    syncEnabledState();
    syncOverlayGeometry();
}

void ViewCubeWidget::setViewport(
    viewer::IDocumentViewport* viewport) {
    viewport_ = viewport;
    refreshProjectionPresentation();
    syncEnabledState();
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

void ViewCubeWidget::applyStandardView(
    viewer::StandardView view) {
    if (viewport_ == nullptr) return;

    static_cast<void>(
        viewport_->setStandardView(view));
    refreshProjectionPresentation();
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
    refreshProjectionPresentation();
}

void ViewCubeWidget::refreshProjectionPresentation() {
    const auto camera =
        viewport_ == nullptr
            ? std::optional<viewer::CameraState>{}
            : viewport_->cameraState();

    QString button_text =
        QStringLiteral("Proj");
    QString action_text =
        QStringLiteral("Projection");
    QString tooltip =
        QStringLiteral(
            "Orthographic / Perspective");

    if (camera) {
        const bool orthographic =
            camera->projection ==
            viewer::CameraProjection::orthographic;

        button_text =
            orthographic
                ? QStringLiteral("Ortho")
                : QStringLiteral("Persp");
        action_text =
            orthographic
                ? QStringLiteral(
                      "Switch to Perspective")
                : QStringLiteral(
                      "Switch to Orthographic");
        tooltip = action_text;
    }

    if (projection_button_ != nullptr) {
        projection_button_->setText(button_text);
        projection_button_->setToolTip(tooltip);
    }
    if (projection_action_ != nullptr) {
        projection_action_->setText(action_text);
    }
}

void ViewCubeWidget::syncEnabledState() {
    const bool enabled = viewport_ != nullptr;
    setEnabled(enabled);
    if (navigation_menu_ != nullptr) {
        navigation_menu_->setEnabled(enabled);
    }
}

void ViewCubeWidget::setCompactMode(bool compact) {
    if (compact_mode_ == compact &&
        regular_panel_ != nullptr &&
        compact_button_ != nullptr) {
        return;
    }

    compact_mode_ = compact;

    if (regular_panel_ != nullptr) {
        regular_panel_->setVisible(!compact_mode_);
    }
    if (compact_button_ != nullptr) {
        compact_button_->setVisible(compact_mode_);
    }
}

void ViewCubeWidget::syncOverlayGeometry() {
    if (host_ == nullptr) return;

    const bool compact =
        host_->width() < compactWidthThreshold ||
        host_->height() < compactHeightThreshold;
    setCompactMode(compact);

    const auto desired =
        compact_mode_
            ? compactOverlaySize
            : regularOverlaySize;

    const int available_width =
        std::max(
            0,
            host_->width() - 2 * overlayMargin);
    const int available_height =
        std::max(
            0,
            host_->height() - 2 * overlayMargin);

    if (available_width < 28 ||
        available_height < 28) {
        if (isVisible()) {
            hide();
        }
        scheduleUnderlayRefresh();
        return;
    }

    const int width =
        std::min(
            desired.width(),
            available_width);
    const int height =
        std::min(
            desired.height(),
            available_height);

    const int x =
        std::max(
            0,
            host_->width() - width - overlayMargin);
    const int y =
        std::min(
            overlayMargin,
            std::max(
                0,
                host_->height() - height));

    setGeometry(x, y, width, height);

    if (host_->isVisible() && !isVisible()) {
        show();
    }
    raise();

    // The OCCT viewport is a native child window (WA_PaintOnScreen).
    // Moving or shrinking a Qt overlay does not reliably invalidate the
    // pixels that were previously covered by the overlay on Windows.
    // Defer and coalesce an explicit repaint request for the underlying
    // native surface after the overlay geometry has settled.
    scheduleUnderlayRefresh();
}

void ViewCubeWidget::scheduleUnderlayRefresh() {
    if (underlay_refresh_scheduled_) {
        return;
    }

    underlay_refresh_scheduled_ = true;
    QTimer::singleShot(
        0,
        this,
        [this] {
            underlay_refresh_scheduled_ = false;

            if (host_ != nullptr) {
                host_->update();
            }

            if (repaint_target_ != nullptr &&
                repaint_target_->isVisible()) {
                repaint_target_->update();
            }
        });
}

bool ViewCubeWidget::eventFilter(
    QObject* watched,
    QEvent* event) {
    if (watched == host_ && event != nullptr) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::LayoutRequest:
            syncOverlayGeometry();
            break;
        default:
            break;
        }
    }

    return QFrame::eventFilter(
        watched,
        event);
}

} // namespace simplesolid2::ui
