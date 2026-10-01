#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/measurement.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>
#include <simplesolid2/sketch/snap.hpp>
#include <simplesolid2/sketch/transform.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::sketch {

enum class SketchTool : std::uint8_t {
    select,
    line,
    circle,
    arc,
    rectangle,
    measure,
    move,
    copy,
    rotate,
    scale,
    mirror,
};

enum class MeasureRelationAcceptOutcome : std::uint8_t {
    inactive,
    invalid_target,
    first_target_accepted,
    relation_accepted,
    relation_rejected,
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
    await_end,
    await_arc_point,
};

enum class RectangleStage : std::uint8_t {
    await_first_corner,
    await_opposite_corner,
};

enum class MoveStage : std::uint8_t {
    select_objects,
    await_base_point,
    await_destination,
};

enum class CommonTransformStage : std::uint8_t {
    select_objects,
    await_base_point,
    await_reference_point,
    await_destination,
    await_axis_start,
    await_axis_end,
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
    invalid_radius,
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
    invalid_radius,
    start_accepted,
    end_accepted,
    radius_locked,
    degenerate_ignored,
    arc_requested,
    request_pending,
};

struct ArcPointResult final {
    ArcPointOutcome outcome{
        ArcPointOutcome::inactive_tool};
    std::optional<ArcIntent> request;
};

struct RectangleIntent final {
    Point2 first_corner;
    Point2 opposite_corner;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] std::array<LineSegmentIntent, 4>
    perimeter() const noexcept;

    [[nodiscard]] std::array<LineSegmentIntent, 2>
    diagonals() const noexcept;

    friend bool operator==(
        const RectangleIntent&,
        const RectangleIntent&) = default;
};

enum class RectanglePointOutcome : std::uint8_t {
    inactive_tool,
    invalid_point,
    invalid_size,
    first_corner_accepted,
    size_locked,
    degenerate_ignored,
    rectangle_requested,
    request_pending,
};

struct RectanglePointResult final {
    RectanglePointOutcome outcome{
        RectanglePointOutcome::inactive_tool};
    std::optional<RectangleIntent> request;
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
    rotate,
    scale,
    mirror,
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

enum class PointResolutionSource : std::uint8_t {
    raw_pointer,
    polar,
    object_snap,
    tracking_inference,
    explicit_numeric,
    numeric_lock,
};

struct PointResolution final {
    Point2 position;
    PointResolutionSource source{
        PointResolutionSource::raw_pointer};
    std::optional<SnapCandidate> object_snap;

    [[nodiscard]] bool valid() const noexcept {
        if (!position.finite()) {
            return false;
        }
        if (object_snap) {
            return object_snap->valid() &&
                   object_snap->point == position &&
                   (source ==
                        PointResolutionSource::
                            object_snap ||
                    source ==
                        PointResolutionSource::
                            numeric_lock);
        }
        return source !=
               PointResolutionSource::object_snap;
    }

    friend bool operator==(
        const PointResolution&,
        const PointResolution&) = default;
};

// Source compatibility: pre-R11 code consumes the same final semantic point
// object under its historical name.
using ResolvedSketchInput = PointResolution;

enum class ExplicitPointInputKind : std::uint8_t {
    absolute_cartesian,
    relative_cartesian,
    relative_polar,
};

struct ExplicitPointInput final {
    ExplicitPointInputKind kind{
        ExplicitPointInputKind::absolute_cartesian};
    double first{};
    double second{};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(first) &&
               std::isfinite(second) &&
               (kind != ExplicitPointInputKind::relative_polar ||
                first >= 0.0);
    }

    friend bool operator==(
        const ExplicitPointInput&,
        const ExplicitPointInput&) = default;
};

enum class PointFieldLockSemantic : std::uint8_t {
    u,
    v,
    distance,
    angle,
    delta_u,
    delta_v,
};

struct PointFieldLocks final {
    std::optional<double> u;
    std::optional<double> v;
    std::optional<double> distance;
    std::optional<double> angle;
    std::optional<double> delta_u;
    std::optional<double> delta_v;

    [[nodiscard]] bool empty() const noexcept {
        return !u && !v &&
               !distance && !angle &&
               !delta_u && !delta_v;
    }

    friend bool operator==(
        const PointFieldLocks&,
        const PointFieldLocks&) = default;
};

struct PointRequest final {
    std::optional<Point2> base;
    std::optional<Point2> pointer_candidate;
    bool direct_distance_enabled{};
    bool absolute_cartesian_enabled{};
    bool relative_cartesian_enabled{};
    bool relative_polar_enabled{};
    std::optional<double> polar_relative_reference;
    std::optional<TemporarySnapOverrideKind>
        temporary_snap_override;
    std::optional<DeferredSnapReference>
        deferred_snap_reference;
    TrackingAnchorState tracking_anchors;
    std::optional<PointResolution> resolution;

    [[nodiscard]] bool valid() const noexcept {
        const bool requires_base =
            direct_distance_enabled ||
            relative_cartesian_enabled ||
            relative_polar_enabled;
        return (!base || base->finite()) &&
               (!pointer_candidate ||
                pointer_candidate->finite()) &&
               (!polar_relative_reference ||
                std::isfinite(
                    *polar_relative_reference)) &&
               (!requires_base || base.has_value()) &&
               (!deferred_snap_reference ||
                deferred_snap_reference->valid()) &&
               tracking_anchors.valid() &&
               (!resolution ||
                (pointer_candidate &&
                 resolution->valid()));
    }

    friend bool operator==(
        const PointRequest&,
        const PointRequest&) = default;
};

// R5/R6 source compatibility for clients that only need
// identity pointer resolution. SK-07F stateful clients use the
// active PointRequest methods on SketchInteractionState.
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

    [[nodiscard]] std::optional<RectangleStage>
    rectangleStage() const noexcept;

    [[nodiscard]] std::optional<MoveStage>
    moveStage() const noexcept;

    [[nodiscard]] std::optional<CommonTransformStage>
    commonTransformStage() const noexcept;

    [[nodiscard]] std::optional<PointRequest>
    activePointRequest() const noexcept;

    [[nodiscard]] std::optional<ResolvedSketchInput>
    resolvePointerInput(Point2 raw) noexcept;

    [[nodiscard]] std::optional<ResolvedSketchInput>
    resolvePointerInput(
        Point2 raw,
        PointResolutionSource source,
        std::optional<SnapCandidate> object_snap =
            std::nullopt) noexcept;

    [[nodiscard]] std::optional<ResolvedSketchInput>
    resolvedPointRequestCandidate() const noexcept;

    [[nodiscard]] bool pointCandidateCompatible(
        Point2 raw,
        PointResolutionSource source,
        std::optional<SnapCandidate> object_snap =
            std::nullopt) const noexcept;

    void clearPointerResolution() noexcept {
        point_pointer_candidate_.reset();
        point_pointer_source_ =
            PointResolutionSource::raw_pointer;
        point_pointer_snap_.reset();
    }

    [[nodiscard]] std::optional<ResolvedSketchInput>
    resolveExplicitPoint(
        ExplicitPointInput input) const noexcept;

    [[nodiscard]] std::optional<ResolvedSketchInput>
    resolveDirectDistance(double distance) const noexcept;

    [[nodiscard]] bool lockPointField(
        PointFieldLockSemantic semantic,
        double value) noexcept;
    void clearPointFieldLocks() noexcept {
        point_field_locks_ = {};
    }
    [[nodiscard]] const PointFieldLocks&
    pointFieldLocks() const noexcept {
        return point_field_locks_;
    }


    [[nodiscard]] bool setTemporarySnapOverride(
        TemporarySnapOverrideKind value) noexcept;
    [[nodiscard]] bool clearTemporarySnapOverride() noexcept;
    [[nodiscard]] std::optional<TemporarySnapOverrideKind>
    temporarySnapOverride() const noexcept {
        return temporary_snap_override_;
    }

    [[nodiscard]] bool setDeferredSnapReference(
        DeferredSnapReference reference) noexcept;
    [[nodiscard]] bool clearDeferredSnapReference() noexcept;
    [[nodiscard]] std::optional<DeferredSnapReference>
    deferredSnapReference() const noexcept {
        return deferred_snap_reference_;
    }

    [[nodiscard]] TrackingAcquireResult
    acquireCurrentTrackingAnchor() noexcept;
    [[nodiscard]] bool removeTrackingAnchor(
        const SnapStableKey& key);
    void clearTrackingAnchors() noexcept {
        tracking_anchors_.clear();
    }
    [[nodiscard]] const TrackingAnchorState&
    trackingAnchors() const noexcept {
        return tracking_anchors_;
    }

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
    void activateRectangle() noexcept;
    void activateMeasure(const SketchModel& model) noexcept;
    [[nodiscard]] std::optional<EntityId>
    measureTarget() const noexcept {
        return measure_target_;
    }
    [[nodiscard]] bool setMeasureTarget(
        const SketchModel& model,
        std::optional<EntityId> target) noexcept;

    [[nodiscard]] bool measureBetweenActive()
        const noexcept {
        return measure_between_active_;
    }
    [[nodiscard]] bool enterMeasureBetween() noexcept;
    void leaveMeasureBetween() noexcept;
    [[nodiscard]] std::optional<MeasureRelationTarget>
    measureFirstRelationTarget() const noexcept {
        return measure_first_target_;
    }
    [[nodiscard]] std::optional<MeasureRelationTarget>
    measureSecondRelationTarget() const noexcept {
        return measure_second_target_;
    }
    [[nodiscard]] MeasureRelationAcceptOutcome
    acceptMeasureRelationTarget(
        const SketchModel& model,
        MeasureRelationTarget target) noexcept;
    [[nodiscard]] bool clearMeasureRelation() noexcept;
    [[nodiscard]] std::optional<RelationalMeasurement>
    measureRelationalResult(
        const SketchModel& model) const noexcept;

    [[nodiscard]] bool activateMove(
        const SketchModel& model);
    [[nodiscard]] bool activateCopy(
        const SketchModel& model);
    [[nodiscard]] bool activateRotate(
        const SketchModel& model);
    [[nodiscard]] bool activateScale(
        const SketchModel& model);
    [[nodiscard]] bool activateMirror(
        const SketchModel& model);

    [[nodiscard]] bool completeTransformSelection(
        const SketchModel& model);
    [[nodiscard]] bool acceptTransformPoint(
        ResolvedSketchInput input) noexcept;
    [[nodiscard]] bool updateTransformPreview(
        ResolvedSketchInput input) noexcept;
    [[nodiscard]] bool acceptTransformValue(
        double value) noexcept;
    [[nodiscard]] std::optional<SketchTransformGeometry>
    transformGeometryState() const;
    [[nodiscard]] bool continueCopyPlacement() noexcept;
    void finishTransform() noexcept;

    // SK-07A source compatibility.
    [[nodiscard]] bool completeMoveSelection(
        const SketchModel& model);
    [[nodiscard]] bool acceptMoveBasePoint(
        ResolvedSketchInput input) noexcept;
    [[nodiscard]] bool updateMoveDestination(
        ResolvedSketchInput input) noexcept;
    [[nodiscard]] std::optional<SketchTransformGeometry>
    moveGeometryState() const;
    void finishMove() noexcept;

    [[nodiscard]] LinePointResult acceptLinePoint(
        Point2 point) noexcept;

    [[nodiscard]] CirclePointResult acceptCirclePoint(
        Point2 point) noexcept;
    [[nodiscard]] bool lockCircleRadius(
        double radius) noexcept;
    void clearCircleRadiusLock() noexcept {
        circle_radius_lock_.reset();
    }
    [[nodiscard]] bool circleRadiusLocked() const noexcept {
        return circle_radius_lock_.has_value();
    }
    [[nodiscard]] CirclePointResult acceptCircleRadius(
        double radius) noexcept;

    [[nodiscard]] ArcPointResult acceptArcPoint(
        Point2 point) noexcept;
    [[nodiscard]] ArcPointResult acceptArcPointer(
        Point2 point) noexcept;
    [[nodiscard]] bool lockArcRadius(
        double radius) noexcept;
    [[nodiscard]] ArcPointResult acceptArcRadius(
        double radius) noexcept;
    [[nodiscard]] bool arcRadiusLocked() const noexcept {
        return arc_radius_lock_.has_value();
    }

    [[nodiscard]] RectanglePointResult acceptRectanglePoint(
        Point2 point) noexcept;
    [[nodiscard]] bool lockRectangleWidth(
        double width) noexcept;
    [[nodiscard]] bool lockRectangleHeight(
        double height) noexcept;
    [[nodiscard]] RectanglePointResult acceptRectangleSize(
        double width,
        double height) noexcept;

    [[nodiscard]] bool resolveLineRequest(
        bool committed) noexcept;

    [[nodiscard]] bool resolveCircleRequest(
        bool committed) noexcept;

    [[nodiscard]] bool resolveArcRequest(
        bool committed) noexcept;

    [[nodiscard]] bool resolveRectangleRequest(
        bool committed) noexcept;

    [[nodiscard]] std::optional<LineSegmentIntent>
    previewLine(Point2 current) const noexcept;

    [[nodiscard]] std::optional<CircleIntent>
    previewCircle(Point2 current) const noexcept;

    [[nodiscard]] std::optional<ArcIntent>
    previewArc(Point2 current) const noexcept;

    [[nodiscard]] std::optional<RectangleIntent>
    previewRectangle(Point2 current) const noexcept;

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

    [[nodiscard]] bool directManipulationCopyEnabled()
        const noexcept;

    [[nodiscard]] bool enableDirectManipulationCopy()
        noexcept;

    [[nodiscard]] bool cycleDirectEditMode() noexcept;

    [[nodiscard]] bool beginDirectManipulation(
        const SketchModel& model,
        SketchGripRef grip);

    [[nodiscard]] bool updateDirectManipulation(
        ResolvedSketchInput input) noexcept;
    [[nodiscard]] bool acceptDirectManipulationValue(
        double value) noexcept;

    [[nodiscard]] std::optional<
        DirectManipulationGeometry>
    directManipulationGeometryState() const;

    [[nodiscard]] bool
    continueDirectManipulationCopyPlacement() noexcept;

    // R5 source compatibility for Line-only callers.
    [[nodiscard]] std::optional<
        std::vector<SketchLineState>>
    directManipulationGeometry() const;

    void finishDirectManipulation() noexcept;
    void cancelDirectManipulation() noexcept;

private:
    struct CommonTransformSession final {
        CommonTransformStage stage{
            CommonTransformStage::select_objects};
        std::vector<EntityId> selection_snapshot;
        SketchTransformGeometry initial_geometry;
        std::optional<Point2> base_point;
        std::optional<Point2> reference_point;
        std::optional<ResolvedSketchInput> current_preview;
        std::optional<double> explicit_value;
    };

    struct DirectManipulationSession final {
        SketchGripRef active_grip;
        DirectEditMode mode{DirectEditMode::reshape};
        std::vector<EntityId> selection_snapshot;
        DirectManipulationGeometry owner_geometry;
        DirectManipulationGeometry selection_geometry;
        Point2 pivot;
        ResolvedSketchInput current_input;
        std::optional<Point2> rotate_reference_point;
        std::optional<double> scale_reference_radius;
        std::optional<double> explicit_value;
        bool copy_enabled{};
    };

    [[nodiscard]] std::optional<EntityId>
    deterministicPrimary() const noexcept;

    [[nodiscard]] bool selectionMutable() const noexcept {
        return !manipulation_.has_value() &&
               (!transform_session_ ||
                transform_session_->stage ==
                    CommonTransformStage::select_objects);
    }

    [[nodiscard]] bool activateCommonTransform(
        SketchTool tool,
        const SketchModel& model);

    [[nodiscard]] bool commonTransformTool() const noexcept;
    [[nodiscard]] bool
    clearRequestLocalNumericLocks() noexcept;
    void completePointAcquisition() noexcept {
        temporary_snap_override_.reset();
        deferred_snap_reference_.reset();
        tracking_anchors_.clear();
        point_pointer_source_ =
            PointResolutionSource::raw_pointer;
        point_pointer_snap_.reset();
    }

    [[nodiscard]] bool clearR11RequestState() noexcept {
        const bool changed =
            temporary_snap_override_.has_value() ||
            deferred_snap_reference_.has_value() ||
            !tracking_anchors_.anchors.empty();
        if (!changed) {
            return false;
        }
        temporary_snap_override_.reset();
        deferred_snap_reference_.reset();
        tracking_anchors_.clear();
        clearPointerResolution();
        return true;
    }

    void resetToSelect() noexcept;
    void resetLineStage() noexcept;
    void resetCircleStage() noexcept;
    void resetArcStage() noexcept;
    void resetRectangleStage() noexcept;
    void resetCommonTransform() noexcept;
    void resetMeasure() noexcept;

    SketchTool tool_{SketchTool::select};

    LineStage line_stage_{
        LineStage::await_first_point};
    std::optional<Point2> line_anchor_;
    std::optional<double>
        line_relative_reference_;
    std::optional<LineSegmentIntent>
        pending_line_request_;

    CircleStage circle_stage_{
        CircleStage::await_center};
    std::optional<Point2> circle_center_;
    std::optional<double> circle_radius_lock_;
    std::optional<CircleIntent>
        pending_circle_request_;

    ArcStage arc_stage_{
        ArcStage::await_start};
    std::optional<Point2> arc_start_;
    std::optional<Point2> arc_end_;
    std::optional<double> arc_radius_lock_;
    std::optional<ArcIntent>
        pending_arc_request_;

    RectangleStage rectangle_stage_{
        RectangleStage::await_first_corner};
    std::optional<Point2> rectangle_first_corner_;
    std::optional<double> rectangle_width_lock_;
    std::optional<double> rectangle_height_lock_;
    std::optional<RectangleIntent>
        pending_rectangle_request_;

    std::optional<CommonTransformSession>
        transform_session_;

    std::vector<EntityId> selected_;
    std::optional<EntityId> primary_;
    std::optional<EntityId> hovered_entity_;
    std::optional<SketchGripRef> hovered_grip_;
    std::optional<EntityId> measure_target_;
    bool measure_between_active_{};
    std::optional<MeasureRelationTarget>
        measure_first_target_;
    std::optional<MeasureRelationTarget>
        measure_second_target_;
    std::optional<DirectManipulationSession> manipulation_;

    // One shared runtime pointer candidate feeds the active semantic
    // PointRequest. It is never authored or persisted.
    std::optional<Point2> point_pointer_candidate_;
    PointResolutionSource point_pointer_source_{
        PointResolutionSource::raw_pointer};
    std::optional<SnapCandidate> point_pointer_snap_;
    PointFieldLocks point_field_locks_;
    std::optional<TemporarySnapOverrideKind>
        temporary_snap_override_;
    std::optional<DeferredSnapReference>
        deferred_snap_reference_;
    TrackingAnchorState tracking_anchors_;
};

} // namespace simplesolid2::sketch
