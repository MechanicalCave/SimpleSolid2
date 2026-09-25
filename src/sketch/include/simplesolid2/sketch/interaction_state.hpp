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

    [[nodiscard]] bool replaceSelection(
        EntityId id);

    [[nodiscard]] bool toggleSelection(
        EntityId id);

    void clearSelection() noexcept;

    [[nodiscard]] bool replaceSelection(
        std::vector<EntityId> ids,
        std::optional<EntityId> primary);

    void reconcileSelection(
        const SketchModel& model);

private:
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
};

} // namespace simplesolid2::sketch
