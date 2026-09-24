#include "view_cube_widget.hpp"

#include <QAction>
#include <QApplication>
#include <QToolButton>
#include <QTest>
#include <QWidget>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>

using namespace simplesolid2;

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr \
                << "WB-01A ViewCube CHECK failed at line " \
                << __LINE__ \
                << ": " #expr "\\n"; \
            return EXIT_FAILURE; \
        } \
    } while (false)

class TestViewport final
    : public viewer::IDocumentViewport {
public:
    [[nodiscard]] std::optional<viewer::CameraState>
    cameraState() const override {
        return state_;
    }

    bool setCameraState(
        const viewer::CameraState& state) override {
        if (!viewer::validateCameraState(state).valid) {
            return false;
        }
        state_ = state;
        return true;
    }

    bool setStandardView(
        viewer::StandardView view) override {
        const auto next =
            viewer::cameraForStandardView(
                state_,
                view);
        if (!next) return false;
        state_ = *next;
        return true;
    }

    bool setProjection(
        viewer::CameraProjection projection) override {
        const auto next =
            viewer::cameraWithProjection(
                state_,
                projection);
        if (!next) return false;
        state_ = *next;
        return true;
    }

    void fitAll() override {
        ++fit_count_;
    }

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        return scene.valid();
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        return selection.valid();
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        handler_ = std::move(handler);
    }

    [[nodiscard]] int fitCount() const noexcept {
        return fit_count_;
    }

private:
    viewer::CameraState state_;
    viewer::SelectionIntentHandler handler_;
    int fit_count_{};
};

bool containedBy(
    const QWidget& host,
    const QWidget& overlay) {
    return host.rect().contains(
        overlay.geometry());
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    TestViewport viewport;
    QWidget host;
    host.resize(420, 260);

    ui::ViewCubeWidget cube{
        &viewport,
        &host};

    host.show();
    QApplication::processEvents();

    CHECK(cube.isVisible());
    CHECK(!cube.compactMode());
    CHECK(containedBy(host, cube));

    auto* top_action =
        cube.findChild<QAction*>(
            QStringLiteral("viewCubeTopAction"));
    auto* fit_action =
        cube.findChild<QAction*>(
            QStringLiteral("viewCubeFitAction"));
    auto* projection_action =
        cube.findChild<QAction*>(
            QStringLiteral("viewCubeProjectionAction"));
    auto* canvas =
        cube.findChild<QWidget*>(
            QStringLiteral("viewCubeCanvas"));
    auto* compact_button =
        cube.findChild<QToolButton*>(
            QStringLiteral("viewCubeCompactButton"));

    CHECK(top_action != nullptr);
    CHECK(fit_action != nullptr);
    CHECK(projection_action != nullptr);
    CHECK(canvas != nullptr);
    CHECK(compact_button != nullptr);

    const auto initial_distance =
        (viewport.cameraState()->eye -
         viewport.cameraState()->target)
            .squaredLength();

    top_action->trigger();
    CHECK(viewport.cameraState().has_value());
    CHECK(viewport.cameraState()->eye.z >
          viewport.cameraState()->target.z);
    CHECK(std::abs(
        (viewport.cameraState()->eye -
         viewport.cameraState()->target)
                .squaredLength() -
        initial_distance) < 1.0e-9);

    const auto fit_before =
        viewport.fitCount();
    fit_action->trigger();
    CHECK(viewport.fitCount() ==
          fit_before + 1);

    CHECK(
        viewport.cameraState()->projection ==
        viewer::CameraProjection::orthographic);
    projection_action->trigger();
    CHECK(
        viewport.cameraState()->projection ==
        viewer::CameraProjection::perspective);

    QTest::mouseClick(
        canvas,
        Qt::LeftButton,
        Qt::NoModifier,
        QPoint{30, 50});
    CHECK(viewport.cameraState()->eye.y <
          viewport.cameraState()->target.y);

    host.resize(150, 220);
    QApplication::processEvents();

    CHECK(cube.compactMode());
    CHECK(cube.isVisible());
    CHECK(compact_button->isVisible());
    CHECK(containedBy(host, cube));

    projection_action->trigger();
    CHECK(
        viewport.cameraState()->projection ==
        viewer::CameraProjection::orthographic);

    const auto compact_fit_before =
        viewport.fitCount();
    fit_action->trigger();
    CHECK(viewport.fitCount() ==
          compact_fit_before + 1);

    top_action->trigger();
    CHECK(viewport.cameraState()->eye.z >
          viewport.cameraState()->target.z);

    for (int width = 70;
         width <= 420;
         width += 7) {
        host.resize(width, 200);
        QApplication::processEvents();

        CHECK(cube.isVisible());
        CHECK(containedBy(host, cube));
    }

    host.resize(420, 260);
    QApplication::processEvents();
    CHECK(!cube.compactMode());
    CHECK(containedBy(host, cube));

    return EXIT_SUCCESS;
}
