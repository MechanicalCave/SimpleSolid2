#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::sketch {

enum class SketchTool : std::uint8_t {
    select,
    line,
    circle,
    arc,
};

enum class LineStage : std::uint8_t {
    await_first_point,
    await_next_point,
};

enum class CircleStage : std::uint8_t {
    await_center,
    await_radius,
};

enum class ArcStage : std::uint8_t {
    await_start,
    await_through,
    await_end,
};

struct LineSegmentIntent final {
    Point2 start;
    Point2 end;

    [[nodiscard]] bool valid() const noexcept {
        return start.finite() &&
               end.finite() &&
               start != end;
    }

    friend bool operator==(
        const LineSegmentIntent&,
        const LineSegmentIntent&) = default;
};

struct CircleIntent final {
    Point2 center;
    double radius{};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const CircleIntent&,
        const CircleIntent&) = default;
};

struct ArcIntent final {
    Point2 center;
    double radius{};
    double start_angle{};
    double sweep_angle{};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const ArcIntent&,
        const ArcIntent&) = default;
};

enum class LinePointOutcome : std::uint8_t {
    inactive_tool,
    invalid_point,
    first_point_accepted,
    zero_length_ignored,
    segment_requested,
    request_pending,
};

struct LinePointResult final {
    LinePointOutcome outcome{
        LinePointOutcome::inactive_tool};
    std::optional<LineSegmentIntent> request;
};

enum class CirclePointOutcome : std::uint8_t {
    inactive_tool,
    invalid_point,
    center_accepted,
    zero_radius_ignored,
    circle_requested,
    request_pending,
};

struct CirclePointResult final {
    CirclePointOutcome outcome{
        CirclePointOutcome::inactive_tool};
    std::optional<CircleIntent> request;
};

enum class ArcPointOutcome : std::uint8_t {
    inactive_tool,
    invalid_point,
    start_accepted,
    through_accepted,
    degenerate_ignored,
    arc_requested,
    request_pending,
};

struct ArcPointResult final {
    ArcPointOutcome outcome{
        ArcPointOutcome::inactive_tool};
    std::optional<ArcIntent> request;
};

enum class SketchGripRole : std::uint8_t {
    line_start,
    line_center,
    line_end,
    circle_center,
    circle_quadrant_pos_u,
    circle_quadrant_pos_v,
    circle_quadrant_neg_u,
    circle_quadrant_neg_v,
    arc_center,
    arc_start,
    arc_end,
    arc_mid,

    // R5 source compatibility.
    start = line_start,
    center = line_center,
    end = line_end,
};

using LineHandleRole = SketchGripRole;

enum class DirectEditMode : std::uint8_t {
    reshape,
    move,
};

struct SketchGripRef final {
    EntityId entity_id;
    SketchGripRole role{SketchGripRole::line_center};

    [[nodiscard]] bool valid() const noexcept {
        return entity_id.valid();
    }

    friend bool operator==(
        const SketchGripRef&,
        const SketchGripRef&) = default;
};

using LineGripRef = SketchGripRef;

using DirectManipulationGeometry =
    SketchTransformGeometry;

struct ResolvedSketchInput final {
    Point2 position;

    [[nodiscard]] bool valid() const noexcept {
        return position.finite();
    }

    friend bool operator==(
        const ResolvedSketchInput&,
        const ResolvedSketchInput&) = default;
};

[[nodiscard]] inline std::optional<ResolvedSketchInput>
resolveSketchInput(Point2 raw) noexcept {
    if (!raw.finite()) {
        return std::nullopt;
    }
    return ResolvedSketchInput{raw};
}

class SketchInteractionState final {
public:
    [[nodiscard]] SketchTool tool() const noexcept {
        return tool_;
    }

    [[nodiscard]] std::optional<LineStage>
    lineStage() const noexcept;

    [[nodiscard]] std::optional<CircleStage>
    circleStage() const noexcept;

    [[nodiscard]] std::optional<ArcStage>
    arcStage() const noexcept;

    [[nodiscard]] std::optional<Point2>
    lineAnchor() const noexcept {
        return line_anchor_;
    }

    [[nodiscard]] bool lineRequestPending() const noexcept {
        return pending_line_request_.has_value();
    }

    [[nodiscard]] std::optional<LineSegmentIntent>
    pendingLineRequest() const noexcept {
        return pending_line_request_;
    }

    void activateLine() noexcept;
    void activateCircle() noexcept;
    void activateArc() noexcept;

    [[nodiscard]] LinePointResult acceptLinePoint(
        Point2 point) noexcept;

    [[nodiscard]] CirclePointResult acceptCirclePoint(
        Point2 point) noexcept;

    [[nodiscard]] ArcPointResult acceptArcPoint(
        Point2 point) noexcept;

    [[nodiscard]] bool resolveLineRequest(
        bool committed) noexcept;

    [[nodiscard]] bool resolveCircleRequest(
        bool committed) noexcept;

    [[nodiscard]] bool resolveArcRequest(
        bool committed) noexcept;

    [[nodiscard]] std::optional<LineSegmentIntent>
    previewLine(Point2 current) const noexcept;

    [[nodiscard]] std::optional<CircleIntent>
    previewCircle(Point2 current) const noexcept;

    [[nodiscard]] std::optional<ArcIntent>
    previewArc(Point2 current) const noexcept;

    void finishTool() noexcept;
    void cancelTool() noexcept;

    [[nodiscard]] bool escape() noexcept;

    void cancelForHistory() noexcept;

    [[nodiscard]] const std::vector<EntityId>&
    selectedEntities() const noexcept {
        return selected_;
    }

    [[nodiscard]] std::optional<EntityId>
    primarySelection() const noexcept {
        return primary_;
    }

    [[nodiscard]] bool addSelection(
        EntityId id);

    [[nodiscard]] bool addSelection(
        std::vector<EntityId> ids);

    [[nodiscard]] bool replaceSelection(
        EntityId id);

    [[nodiscard]] bool toggleSelection(
        EntityId id);

    [[nodiscard]] bool toggleSelection(
        std::vector<EntityId> ids);

    void clearSelection() noexcept;

    [[nodiscard]] bool replaceSelection(
        std::vector<EntityId> ids,
        std::optional<EntityId> primary);

    void reconcileSelection(
        const SketchModel& model);

    [[nodiscard]] std::optional<EntityId>
    hoveredEntity() const noexcept {
        return hovered_entity_;
    }

    [[nodiscard]] std::optional<SketchGripRef>
    hoveredGrip() const noexcept {
        return hovered_grip_;
    }

    [[nodiscard]] bool setHoveredEntity(
        std::optional<EntityId> entity) noexcept;

    [[nodiscard]] bool setHoveredGrip(
        std::optional<SketchGripRef> grip) noexcept;

    void clearHover() noexcept;

    [[nodiscard]] bool directManipulationActive()
        const noexcept {
        return manipulation_.has_value();
    }

    [[nodiscard]] std::optional<SketchGripRef>
    activeGrip() const noexcept;

    [[nodiscard]] std::optional<DirectEditMode>
    directEditMode() const noexcept;

    [[nodiscard]] bool beginDirectManipulation(
        const SketchModel& model,
        SketchGripRef grip);

    [[nodiscard]] bool updateDirectManipulation(
        ResolvedSketchInput input) noexcept;

    [[nodiscard]] std::optional<
        DirectManipulationGeometry>
    directManipulationGeometryState() const;

    // R5 source compatibility for Line-only callers.
    [[nodiscard]] std::optional<
        std::vector<SketchLineState>>
    directManipulationGeometry() const;

    void finishDirectManipulation() noexcept;
    void cancelDirectManipulation() noexcept;

private:
    struct DirectManipulationSession final {
        SketchGripRef active_grip;
        DirectEditMode mode{DirectEditMode::reshape};
        std::vector<EntityId> selection_snapshot;
        DirectManipulationGeometry initial_geometry;
        Point2 pivot;
        ResolvedSketchInput current_input;
    };

    [[nodiscard]] std::optional<EntityId>
    deterministicPrimary() const noexcept;

    [[nodiscard]] bool selectionMutable() const noexcept {
        return !manipulation_.has_value();
    }

    void resetToSelect() noexcept;
    void resetLineStage() noexcept;
    void resetCircleStage() noexcept;
    void resetArcStage() noexcept;

    SketchTool tool_{SketchTool::select};

    LineStage line_stage_{
        LineStage::await_first_point};
    std::optional<Point2> line_anchor_;
    std::optional<LineSegmentIntent>
        pending_line_request_;

    CircleStage circle_stage_{
        CircleStage::await_center};
    std::optional<Point2> circle_center_;
    std::optional<CircleIntent>
        pending_circle_request_;

    ArcStage arc_stage_{
        ArcStage::await_start};
    std::optional<Point2> arc_start_;
    std::optional<Point2> arc_through_;
    std::optional<ArcIntent>
        pending_arc_request_;

    std::vector<EntityId> selected_;
    std::optional<EntityId> primary_;
    std::optional<EntityId> hovered_entity_;
    std::optional<SketchGripRef> hovered_grip_;
    std::optional<DirectManipulationSession> manipulation_;
};

} // namespace simplesolid2::sketch
