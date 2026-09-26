#pragma once

#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <utility>

namespace simplesolid2::ui {

class PartSketchInteractionController final {
public:
    using StateChangedHandler = std::function<void()>;
    using StatusHandler = std::function<void(const std::string&)>;

    explicit PartSketchInteractionController(
        PartViewportController& viewport_controller);

    void begin(
        application::DocumentSession& session,
        sketch::SketchId sketch_id);
    void end();

    [[nodiscard]] bool active() const noexcept;
    [[nodiscard]] sketch::SketchTool tool() const noexcept;
    [[nodiscard]] std::optional<sketch::LineStage>
    lineStage() const noexcept;
    [[nodiscard]] std::optional<sketch::CircleStage>
    circleStage() const noexcept;
    [[nodiscard]] std::optional<sketch::ArcStage>
    arcStage() const noexcept;
    [[nodiscard]] std::size_t selectedCount() const noexcept;
    [[nodiscard]] bool directManipulationActive()
        const noexcept;

    // Line/Circle/Arc are adapters to the same semantic
    // SketchInteractionState and resolved-input path.
    void activateSelect();
    void activateLine();
    void activateCircle();
    void activateArc();
    void finishLine();
    void cancelLine();
    [[nodiscard]] bool escape();

    [[nodiscard]] bool deleteSelection();
    [[nodiscard]] bool commitDirectManipulation();

    void cancelForHistory();
    [[nodiscard]] bool reconcileAfterHistory();

    void onPointer(const SketchPointerInput& input);

    void setStateChangedHandler(StateChangedHandler handler) {
        state_changed_handler_ = std::move(handler);
    }

    void setStatusHandler(StatusHandler handler) {
        status_handler_ = std::move(handler);
    }

private:
    static constexpr double drag_threshold_pixels = 4.0;

    [[nodiscard]] const part::PartSketch*
    activeSketch() const noexcept;

    void handleSelectPointer(const SketchPointerInput& input);
    void handleLinePointer(const SketchPointerInput& input);
    void handleCirclePointer(const SketchPointerInput& input);
    void handleArcPointer(const SketchPointerInput& input);
    void updateRectangleOverlay(viewer::ViewportPoint2 current);
    void updateHover(viewer::ViewportPoint2 point);
    [[nodiscard]] bool beginDirectManipulation(
        sketch::SketchGripRef grip);
    void updateDirectManipulationPreview(
        sketch::Point2 raw_input);

    void projectSelection();
    void projectInteraction();
    void configureForCurrentTool();
    void notifyStateChanged();
    void reportStatus(std::string message);

    PartViewportController* viewport_controller_{};
    application::DocumentSession* session_{};
    std::optional<sketch::SketchId> sketch_id_;
    sketch::SketchInteractionState interaction_;

    std::optional<viewer::ViewportPoint2> press_anchor_;
    bool rectangle_drag_active_{};
    std::optional<core::DocumentRevision>
        manipulation_revision_;

    StateChangedHandler state_changed_handler_;
    StatusHandler status_handler_;
};

} // namespace simplesolid2::ui
