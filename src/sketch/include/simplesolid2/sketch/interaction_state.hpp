#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::sketch {

enum class SketchTool : std::uint8_t {
    select,
    line,
};

enum class LineStage : std::uint8_t {
    await_first_point,
    await_next_point,
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

enum class LineHandleRole : std::uint8_t {
    start,
    center,
    end,
};

enum class DirectEditMode : std::uint8_t {
    reshape,
    move,
};

struct LineGripRef final {
    EntityId entity_id;
    LineHandleRole role{LineHandleRole::center};

    [[nodiscard]] bool valid() const noexcept {
        return entity_id.valid();
    }

    friend bool operator==(
        const LineGripRef&,
        const LineGripRef&) = default;
};

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

    [[nodiscard]] LinePointResult acceptLinePoint(
        Point2 point) noexcept;

    [[nodiscard]] bool resolveLineRequest(
        bool committed) noexcept;

    [[nodiscard]] std::optional<LineSegmentIntent>
    previewLine(Point2 current) const noexcept;

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

    [[nodiscard]] std::optional<LineGripRef>
    hoveredGrip() const noexcept {
        return hovered_grip_;
    }

    [[nodiscard]] bool setHoveredEntity(
        std::optional<EntityId> entity) noexcept;

    [[nodiscard]] bool setHoveredGrip(
        std::optional<LineGripRef> grip) noexcept;

    void clearHover() noexcept;

    [[nodiscard]] bool directManipulationActive()
        const noexcept {
        return manipulation_.has_value();
    }

    [[nodiscard]] std::optional<LineGripRef>
    activeGrip() const noexcept;

    [[nodiscard]] std::optional<DirectEditMode>
    directEditMode() const noexcept;

    [[nodiscard]] bool beginDirectManipulation(
        const SketchModel& model,
        LineGripRef grip);

    [[nodiscard]] bool updateDirectManipulation(
        ResolvedSketchInput input) noexcept;

    [[nodiscard]] std::optional<
        std::vector<SketchLineState>>
    directManipulationGeometry() const;

    void finishDirectManipulation() noexcept;
    void cancelDirectManipulation() noexcept;

private:
    struct DirectManipulationSession final {
        LineGripRef active_grip;
        DirectEditMode mode{DirectEditMode::reshape};
        std::vector<EntityId> selection_snapshot;
        std::vector<SketchLineState> initial_geometry;
        Point2 pivot;
        ResolvedSketchInput current_input;
    };

    [[nodiscard]] std::optional<EntityId>
    deterministicPrimary() const noexcept;

    [[nodiscard]] bool selectionMutable() const noexcept {
        return !manipulation_.has_value();
    }

    void resetLineToSelect() noexcept;
    void resetLineStage() noexcept;

    SketchTool tool_{SketchTool::select};
    LineStage line_stage_{
        LineStage::await_first_point};
    std::optional<Point2> line_anchor_;
    std::optional<LineSegmentIntent>
        pending_line_request_;

    std::vector<EntityId> selected_;
    std::optional<EntityId> primary_;
    std::optional<EntityId> hovered_entity_;
    std::optional<LineGripRef> hovered_grip_;
    std::optional<DirectManipulationSession> manipulation_;
};

} // namespace simplesolid2::sketch
