#include <simplesolid2/sketch/interaction_state.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <utility>

namespace simplesolid2::sketch {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;
constexpr double half_turn =
    std::numbers::pi_v<double>;

[[nodiscard]] Point2 lineCenter(
    const Line& line) noexcept {
    return {
        (line.start().u + line.end().u) * 0.5,
        (line.start().v + line.end().v) * 0.5};
}

[[nodiscard]] double distance(
    Point2 first,
    Point2 second) noexcept {
    return std::hypot(
        second.u - first.u,
        second.v - first.v);
}

[[nodiscard]] double direction(
    Point2 center,
    Point2 point) noexcept {
    return std::atan2(
        point.v - center.v,
        point.u - center.u);
}

[[nodiscard]] double positiveTurn(
    double angle) noexcept {
    double normalized =
        std::fmod(angle, full_turn);
    if (normalized < 0.0) {
        normalized += full_turn;
    }
    return normalized;
}

[[nodiscard]] double ccwDelta(
    double from,
    double to) noexcept {
    return positiveTurn(to - from);
}

[[nodiscard]] Point2 pointOnCircle(
    Point2 center,
    double radius,
    double angle) noexcept {
    return {
        center.u + radius * std::cos(angle),
        center.v + radius * std::sin(angle)};
}

[[nodiscard]] bool validReplacement(
    const SketchLineState& line) noexcept {
    return line.id.valid() &&
           line.start.finite() &&
           line.end.finite() &&
           line.start != line.end;
}

[[nodiscard]] bool validReplacement(
    const SketchCircleState& circle) noexcept {
    return circle.id.valid() &&
           circle.center.finite() &&
           std::isfinite(circle.radius) &&
           circle.radius > 0.0;
}

[[nodiscard]] bool validReplacement(
    const SketchArcState& arc) noexcept {
    return arc.id.valid() &&
           arc.center.finite() &&
           std::isfinite(arc.radius) &&
           arc.radius > 0.0 &&
           std::isfinite(arc.start_angle) &&
           std::isfinite(arc.sweep_angle) &&
           arc.sweep_angle != 0.0 &&
           std::abs(arc.sweep_angle) < full_turn;
}

[[nodiscard]] std::optional<ArcIntent>
arcThroughThreePoints(
    Point2 start,
    Point2 through,
    Point2 end) noexcept {
    if (!start.finite() ||
        !through.finite() ||
        !end.finite() ||
        start == through ||
        start == end ||
        through == end) {
        return std::nullopt;
    }

    const double determinant =
        2.0 *
        (start.u * (through.v - end.v) +
         through.u * (end.v - start.v) +
         end.u * (start.v - through.v));
    if (!std::isfinite(determinant) ||
        determinant == 0.0) {
        return std::nullopt;
    }

    const double start_squared =
        start.u * start.u +
        start.v * start.v;
    const double through_squared =
        through.u * through.u +
        through.v * through.v;
    const double end_squared =
        end.u * end.u +
        end.v * end.v;

    Point2 center{
        (start_squared *
             (through.v - end.v) +
         through_squared *
             (end.v - start.v) +
         end_squared *
             (start.v - through.v)) /
            determinant,
        (start_squared *
             (end.u - through.u) +
         through_squared *
             (start.u - end.u) +
         end_squared *
             (through.u - start.u)) /
            determinant};

    const double radius =
        distance(center, start);
    if (!center.finite() ||
        !std::isfinite(radius) ||
        radius <= 0.0) {
        return std::nullopt;
    }

    const double start_angle =
        direction(center, start);
    const double through_angle =
        direction(center, through);
    const double end_angle =
        direction(center, end);

    const double to_through =
        ccwDelta(start_angle, through_angle);
    const double to_end =
        ccwDelta(start_angle, end_angle);
    if (to_through == 0.0 ||
        to_end == 0.0) {
        return std::nullopt;
    }

    const double sweep =
        to_through < to_end
            ? to_end
            : to_end - full_turn;

    ArcIntent result{
        center,
        radius,
        start_angle,
        sweep};
    return result.valid()
        ? std::optional<ArcIntent>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<double>
sameDirectionSweep(
    double start_angle,
    double end_angle,
    double previous_sweep) noexcept {
    const double positive =
        ccwDelta(start_angle, end_angle);
    if (positive == 0.0) {
        return std::nullopt;
    }

    const double candidate =
        previous_sweep > 0.0
            ? positive
            : positive - full_turn;

    if (candidate == 0.0 ||
        std::abs(candidate) >= full_turn ||
        std::abs(candidate) == half_turn) {
        return std::nullopt;
    }

    return candidate;
}

} // namespace

bool CircleIntent::valid() const noexcept {
    return center.finite() &&
           std::isfinite(radius) &&
           radius > 0.0;
}

bool ArcIntent::valid() const noexcept {
    return center.finite() &&
           std::isfinite(radius) &&
           radius > 0.0 &&
           std::isfinite(start_angle) &&
           std::isfinite(sweep_angle) &&
           sweep_angle != 0.0 &&
           std::abs(sweep_angle) < full_turn;
}

std::optional<LineStage>
SketchInteractionState::lineStage() const noexcept {
    return tool_ == SketchTool::line
        ? std::optional<LineStage>{line_stage_}
        : std::nullopt;
}

std::optional<CircleStage>
SketchInteractionState::circleStage() const noexcept {
    return tool_ == SketchTool::circle
        ? std::optional<CircleStage>{circle_stage_}
        : std::nullopt;
}

std::optional<ArcStage>
SketchInteractionState::arcStage() const noexcept {
    return tool_ == SketchTool::arc
        ? std::optional<ArcStage>{arc_stage_}
        : std::nullopt;
}

std::optional<MoveStage>
SketchInteractionState::moveStage() const noexcept {
    return tool_ == SketchTool::move &&
                   move_session_
        ? std::optional<MoveStage>{
              move_session_->stage}
        : std::nullopt;
}

void SketchInteractionState::activateLine() noexcept {
    manipulation_.reset();
    resetMoveStage();
    clearHover();
    tool_ = SketchTool::line;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
}

void SketchInteractionState::activateCircle() noexcept {
    manipulation_.reset();
    resetMoveStage();
    clearHover();
    tool_ = SketchTool::circle;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
}

void SketchInteractionState::activateArc() noexcept {
    manipulation_.reset();
    resetMoveStage();
    clearHover();
    tool_ = SketchTool::arc;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
}

bool SketchInteractionState::activateMove(
    const SketchModel& model) {
    manipulation_.reset();
    clearHover();
    resetLineStage();
    resetCircleStage();
    resetArcStage();

    MoveSession session;
    if (selected_.empty()) {
        session.stage = MoveStage::select_objects;
        tool_ = SketchTool::move;
        move_session_ = std::move(session);
        return true;
    }

    const auto geometry =
        captureSketchTransformGeometry(
            model,
            selected_);
    if (!geometry) {
        resetToSelect();
        return false;
    }

    session.stage = MoveStage::await_base_point;
    session.selection_snapshot = selected_;
    session.initial_geometry = *geometry;
    tool_ = SketchTool::move;
    move_session_ = std::move(session);
    return true;
}

bool SketchInteractionState::completeMoveSelection(
    const SketchModel& model) {
    if (tool_ != SketchTool::move ||
        !move_session_ ||
        move_session_->stage !=
            MoveStage::select_objects ||
        selected_.empty()) {
        return false;
    }

    const auto geometry =
        captureSketchTransformGeometry(
            model,
            selected_);
    if (!geometry) {
        return false;
    }

    move_session_->selection_snapshot = selected_;
    move_session_->initial_geometry = *geometry;
    move_session_->base_point.reset();
    move_session_->current_destination.reset();
    move_session_->stage =
        MoveStage::await_base_point;
    clearHover();
    return true;
}

bool SketchInteractionState::acceptMoveBasePoint(
    ResolvedSketchInput input) noexcept {
    if (tool_ != SketchTool::move ||
        !move_session_ ||
        move_session_->stage !=
            MoveStage::await_base_point ||
        !input.valid() ||
        move_session_->selection_snapshot.empty() ||
        move_session_->initial_geometry.empty()) {
        return false;
    }

    move_session_->base_point = input.position;
    move_session_->current_destination = input;
    move_session_->stage =
        MoveStage::await_destination;
    clearHover();
    return true;
}

bool SketchInteractionState::updateMoveDestination(
    ResolvedSketchInput input) noexcept {
    if (tool_ != SketchTool::move ||
        !move_session_ ||
        move_session_->stage !=
            MoveStage::await_destination ||
        !move_session_->base_point ||
        !input.valid()) {
        return false;
    }

    move_session_->current_destination = input;
    return true;
}

std::optional<SketchTransformGeometry>
SketchInteractionState::moveGeometryState() const {
    if (tool_ != SketchTool::move ||
        !move_session_ ||
        move_session_->stage !=
            MoveStage::await_destination ||
        !move_session_->base_point ||
        !move_session_->current_destination) {
        return std::nullopt;
    }

    const Point2 delta{
        move_session_->current_destination->position.u -
            move_session_->base_point->u,
        move_session_->current_destination->position.v -
            move_session_->base_point->v};

    return translateSketchGeometry(
        move_session_->initial_geometry,
        delta);
}

void SketchInteractionState::finishMove() noexcept {
    if (tool_ != SketchTool::move) {
        return;
    }
    resetToSelect();
    clearHover();
}

LinePointResult
SketchInteractionState::acceptLinePoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::line) {
        return {
            LinePointOutcome::inactive_tool,
            std::nullopt};
    }

    if (!point.finite()) {
        return {
            LinePointOutcome::invalid_point,
            std::nullopt};
    }

    if (line_stage_ ==
        LineStage::await_first_point) {
        line_anchor_ = point;
        line_stage_ =
            LineStage::await_next_point;
        return {
            LinePointOutcome::first_point_accepted,
            std::nullopt};
    }

    if (pending_line_request_) {
        return {
            LinePointOutcome::request_pending,
            std::nullopt};
    }

    if (!line_anchor_) {
        resetLineStage();
        return {
            LinePointOutcome::invalid_point,
            std::nullopt};
    }

    if (point == *line_anchor_) {
        return {
            LinePointOutcome::zero_length_ignored,
            std::nullopt};
    }

    LineSegmentIntent request{
        *line_anchor_,
        point};
    if (!request.valid()) {
        return {
            LinePointOutcome::invalid_point,
            std::nullopt};
    }

    pending_line_request_ = request;
    return {
        LinePointOutcome::segment_requested,
        request};
}

CirclePointResult
SketchInteractionState::acceptCirclePoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::circle) {
        return {
            CirclePointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!point.finite()) {
        return {
            CirclePointOutcome::invalid_point,
            std::nullopt};
    }

    if (circle_stage_ ==
        CircleStage::await_center) {
        circle_center_ = point;
        circle_stage_ =
            CircleStage::await_radius;
        return {
            CirclePointOutcome::center_accepted,
            std::nullopt};
    }

    if (pending_circle_request_) {
        return {
            CirclePointOutcome::request_pending,
            std::nullopt};
    }
    if (!circle_center_) {
        resetCircleStage();
        return {
            CirclePointOutcome::invalid_point,
            std::nullopt};
    }

    CircleIntent request{
        *circle_center_,
        distance(*circle_center_, point)};
    if (!request.valid()) {
        return {
            CirclePointOutcome::zero_radius_ignored,
            std::nullopt};
    }

    pending_circle_request_ = request;
    return {
        CirclePointOutcome::circle_requested,
        request};
}

ArcPointResult
SketchInteractionState::acceptArcPoint(
    Point2 point) noexcept {
    if (tool_ != SketchTool::arc) {
        return {
            ArcPointOutcome::inactive_tool,
            std::nullopt};
    }
    if (!point.finite()) {
        return {
            ArcPointOutcome::invalid_point,
            std::nullopt};
    }

    if (arc_stage_ == ArcStage::await_start) {
        arc_start_ = point;
        arc_stage_ = ArcStage::await_through;
        return {
            ArcPointOutcome::start_accepted,
            std::nullopt};
    }

    if (arc_stage_ == ArcStage::await_through) {
        if (!arc_start_ || point == *arc_start_) {
            return {
                ArcPointOutcome::degenerate_ignored,
                std::nullopt};
        }
        arc_through_ = point;
        arc_stage_ = ArcStage::await_end;
        return {
            ArcPointOutcome::through_accepted,
            std::nullopt};
    }

    if (pending_arc_request_) {
        return {
            ArcPointOutcome::request_pending,
            std::nullopt};
    }
    if (!arc_start_ || !arc_through_) {
        resetArcStage();
        return {
            ArcPointOutcome::invalid_point,
            std::nullopt};
    }

    const auto request =
        arcThroughThreePoints(
            *arc_start_,
            *arc_through_,
            point);
    if (!request) {
        return {
            ArcPointOutcome::degenerate_ignored,
            std::nullopt};
    }

    pending_arc_request_ = *request;
    return {
        ArcPointOutcome::arc_requested,
        request};
}

bool SketchInteractionState::resolveLineRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::line ||
        line_stage_ !=
            LineStage::await_next_point ||
        !pending_line_request_) {
        return false;
    }

    const auto resolved =
        *pending_line_request_;
    pending_line_request_.reset();

    if (committed) {
        line_anchor_ = resolved.end;
    }

    return true;
}

bool SketchInteractionState::resolveCircleRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::circle ||
        circle_stage_ !=
            CircleStage::await_radius ||
        !pending_circle_request_) {
        return false;
    }

    pending_circle_request_.reset();
    if (committed) {
        resetCircleStage();
    }
    return true;
}

bool SketchInteractionState::resolveArcRequest(
    bool committed) noexcept {
    if (tool_ != SketchTool::arc ||
        arc_stage_ != ArcStage::await_end ||
        !pending_arc_request_) {
        return false;
    }

    pending_arc_request_.reset();
    if (committed) {
        resetArcStage();
    }
    return true;
}

std::optional<LineSegmentIntent>
SketchInteractionState::previewLine(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::line ||
        line_stage_ !=
            LineStage::await_next_point ||
        !line_anchor_ ||
        pending_line_request_ ||
        !current.finite() ||
        current == *line_anchor_) {
        return std::nullopt;
    }

    LineSegmentIntent preview{
        *line_anchor_,
        current};

    return preview.valid()
        ? std::optional<LineSegmentIntent>{
              preview}
        : std::nullopt;
}

std::optional<CircleIntent>
SketchInteractionState::previewCircle(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::circle ||
        circle_stage_ !=
            CircleStage::await_radius ||
        !circle_center_ ||
        pending_circle_request_ ||
        !current.finite()) {
        return std::nullopt;
    }

    CircleIntent preview{
        *circle_center_,
        distance(*circle_center_, current)};
    return preview.valid()
        ? std::optional<CircleIntent>{preview}
        : std::nullopt;
}

std::optional<ArcIntent>
SketchInteractionState::previewArc(
    Point2 current) const noexcept {
    if (tool_ != SketchTool::arc ||
        arc_stage_ != ArcStage::await_end ||
        !arc_start_ ||
        !arc_through_ ||
        pending_arc_request_ ||
        !current.finite()) {
        return std::nullopt;
    }

    return arcThroughThreePoints(
        *arc_start_,
        *arc_through_,
        current);
}

void SketchInteractionState::finishTool() noexcept {
    manipulation_.reset();
    clearHover();
    resetToSelect();
}

void SketchInteractionState::cancelTool() noexcept {
    manipulation_.reset();
    clearHover();
    resetToSelect();
}

bool SketchInteractionState::escape() noexcept {
    if (manipulation_) {
        cancelDirectManipulation();
        return true;
    }

    if (tool_ == SketchTool::select) {
        if (selected_.empty()) {
            return false;
        }
        clearSelection();
        clearHover();
        return true;
    }

    if (tool_ == SketchTool::line) {
        if (line_stage_ ==
            LineStage::await_next_point) {
            resetLineStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (tool_ == SketchTool::circle) {
        if (circle_stage_ ==
            CircleStage::await_radius) {
            resetCircleStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (tool_ == SketchTool::arc) {
        if (arc_stage_ != ArcStage::await_start) {
            resetArcStage();
            return true;
        }
        resetToSelect();
        return true;
    }

    if (tool_ == SketchTool::move) {
        move_session_.reset();
        resetToSelect();
        clearHover();
        return true;
    }

    return false;
}

void SketchInteractionState::cancelForHistory() noexcept {
    manipulation_.reset();
    resetMoveStage();
    clearHover();
    resetToSelect();
}

bool SketchInteractionState::addSelection(
    EntityId id) {
    if (!selectionMutable() || !id.valid()) {
        return false;
    }

    if (std::find(
            selected_.begin(),
            selected_.end(),
            id) == selected_.end()) {
        selected_.push_back(id);
    }
    primary_ = id;
    return true;
}

bool SketchInteractionState::addSelection(
    std::vector<EntityId> ids) {
    if (!selectionMutable()) {
        return false;
    }

    std::set<EntityId> incoming;
    for (const auto id : ids) {
        if (!id.valid() ||
            !incoming.insert(id).second) {
            return false;
        }
    }

    for (const auto id : incoming) {
        if (std::find(
                selected_.begin(),
                selected_.end(),
                id) == selected_.end()) {
            selected_.push_back(id);
        }
    }
    return true;
}

bool SketchInteractionState::replaceSelection(
    EntityId id) {
    if (!selectionMutable() || !id.valid()) {
        return false;
    }

    selected_ = {id};
    primary_ = id;
    return true;
}

bool SketchInteractionState::toggleSelection(
    EntityId id) {
    if (!selectionMutable() || !id.valid()) {
        return false;
    }

    const auto found =
        std::find(
            selected_.begin(),
            selected_.end(),
            id);

    if (found == selected_.end()) {
        selected_.push_back(id);
        primary_ = id;
        return true;
    }

    const bool removed_primary =
        primary_ && *primary_ == id;
    selected_.erase(found);

    if (selected_.empty()) {
        primary_.reset();
    } else if (removed_primary) {
        primary_ = deterministicPrimary();
    }

    return true;
}

bool SketchInteractionState::toggleSelection(
    std::vector<EntityId> ids) {
    if (!selectionMutable()) {
        return false;
    }

    std::set<EntityId> incoming;
    for (const auto id : ids) {
        if (!id.valid() ||
            !incoming.insert(id).second) {
            return false;
        }
    }

    bool removed_primary = false;
    for (const auto id : incoming) {
        const auto found =
            std::find(
                selected_.begin(),
                selected_.end(),
                id);
        if (found == selected_.end()) {
            selected_.push_back(id);
        } else {
            removed_primary =
                removed_primary ||
                (primary_ && *primary_ == id);
            selected_.erase(found);
        }
    }

    if (selected_.empty()) {
        primary_.reset();
    } else if (removed_primary) {
        primary_ = deterministicPrimary();
    }

    return true;
}

void SketchInteractionState::clearSelection() noexcept {
    if (!selectionMutable()) {
        return;
    }
    selected_.clear();
    primary_.reset();
}

bool SketchInteractionState::replaceSelection(
    std::vector<EntityId> ids,
    std::optional<EntityId> primary) {
    if (!selectionMutable()) {
        return false;
    }

    std::set<EntityId> unique;

    for (const auto id : ids) {
        if (!id.valid() ||
            !unique.insert(id).second) {
            return false;
        }
    }

    if (primary) {
        if (!primary->valid() ||
            unique.find(*primary) ==
                unique.end()) {
            return false;
        }
    }

    selected_ = std::move(ids);
    primary_ = primary;
    return true;
}

void SketchInteractionState::reconcileSelection(
    const SketchModel& model) {
    if (!selectionMutable()) {
        return;
    }

    selected_.erase(
        std::remove_if(
            selected_.begin(),
            selected_.end(),
            [&model](EntityId id) {
                return !model.contains(id);
            }),
        selected_.end());

    if (primary_ &&
        std::find(
            selected_.begin(),
            selected_.end(),
            *primary_) ==
            selected_.end()) {
        primary_ = deterministicPrimary();
    }
}

bool SketchInteractionState::setHoveredEntity(
    std::optional<EntityId> entity) noexcept {
    if (tool_ != SketchTool::select ||
        manipulation_) {
        return false;
    }
    if (entity && !entity->valid()) {
        return false;
    }

    const bool changed =
        hovered_entity_ != entity ||
        hovered_grip_.has_value();
    hovered_entity_ = entity;
    hovered_grip_.reset();
    return changed;
}

bool SketchInteractionState::setHoveredGrip(
    std::optional<SketchGripRef> grip) noexcept {
    if (tool_ != SketchTool::select ||
        manipulation_) {
        return false;
    }
    if (grip && !grip->valid()) {
        return false;
    }

    const bool changed =
        hovered_grip_ != grip ||
        hovered_entity_.has_value();
    hovered_grip_ = grip;
    hovered_entity_.reset();
    return changed;
}

void SketchInteractionState::clearHover() noexcept {
    hovered_entity_.reset();
    hovered_grip_.reset();
}

std::optional<SketchGripRef>
SketchInteractionState::activeGrip() const noexcept {
    if (!manipulation_) {
        return std::nullopt;
    }
    return manipulation_->active_grip;
}

std::optional<DirectEditMode>
SketchInteractionState::directEditMode() const noexcept {
    if (!manipulation_) {
        return std::nullopt;
    }
    return manipulation_->mode;
}

bool SketchInteractionState::beginDirectManipulation(
    const SketchModel& model,
    SketchGripRef grip) {
    if (tool_ != SketchTool::select ||
        manipulation_ ||
        !grip.valid() ||
        std::find(
            selected_.begin(),
            selected_.end(),
            grip.entity_id) == selected_.end()) {
        return false;
    }

    DirectManipulationSession session;
    session.active_grip = grip;
    session.selection_snapshot = selected_;

    const auto capture_move =
        [&]() {
            const auto captured =
                captureSketchTransformGeometry(
                    model,
                    session.selection_snapshot);
            if (!captured) {
                return false;
            }

            session.mode = DirectEditMode::move;
            session.initial_geometry = *captured;
            return true;
        };

    switch (grip.role) {
    case SketchGripRole::line_start: {
        const auto* owner =
            model.findLine(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.pivot = owner->start();
        session.initial_geometry.lines.push_back(
            {owner->id(), owner->start(), owner->end()});
        break;
    }
    case SketchGripRole::line_end: {
        const auto* owner =
            model.findLine(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.pivot = owner->end();
        session.initial_geometry.lines.push_back(
            {owner->id(), owner->start(), owner->end()});
        break;
    }
    case SketchGripRole::line_center: {
        const auto* owner =
            model.findLine(grip.entity_id);
        if (owner == nullptr) return false;
        session.pivot = lineCenter(*owner);
        if (!capture_move()) return false;
        break;
    }
    case SketchGripRole::circle_center: {
        const auto* owner =
            model.findCircle(grip.entity_id);
        if (owner == nullptr) return false;
        session.pivot = owner->center();
        if (!capture_move()) return false;
        break;
    }
    case SketchGripRole::circle_quadrant_pos_u:
    case SketchGripRole::circle_quadrant_pos_v:
    case SketchGripRole::circle_quadrant_neg_u:
    case SketchGripRole::circle_quadrant_neg_v: {
        const auto* owner =
            model.findCircle(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.initial_geometry.circles.push_back(
            {owner->id(), owner->center(), owner->radius()});

        Point2 direction_vector{};
        if (grip.role ==
            SketchGripRole::circle_quadrant_pos_u) {
            direction_vector = {owner->radius(), 0.0};
        } else if (grip.role ==
                   SketchGripRole::circle_quadrant_pos_v) {
            direction_vector = {0.0, owner->radius()};
        } else if (grip.role ==
                   SketchGripRole::circle_quadrant_neg_u) {
            direction_vector = {-owner->radius(), 0.0};
        } else {
            direction_vector = {0.0, -owner->radius()};
        }
        session.pivot = {
            owner->center().u + direction_vector.u,
            owner->center().v + direction_vector.v};
        break;
    }
    case SketchGripRole::arc_center: {
        const auto* owner =
            model.findArc(grip.entity_id);
        if (owner == nullptr) return false;
        session.pivot = owner->center();
        if (!capture_move()) return false;
        break;
    }
    case SketchGripRole::arc_start:
    case SketchGripRole::arc_end:
    case SketchGripRole::arc_mid: {
        const auto* owner =
            model.findArc(grip.entity_id);
        if (owner == nullptr) return false;
        session.mode = DirectEditMode::reshape;
        session.initial_geometry.arcs.push_back(
            {
                owner->id(),
                owner->center(),
                owner->radius(),
                owner->startAngle(),
                owner->sweepAngle()});

        double angle = owner->startAngle();
        if (grip.role == SketchGripRole::arc_end) {
            angle += owner->sweepAngle();
        } else if (grip.role ==
                   SketchGripRole::arc_mid) {
            angle += owner->sweepAngle() * 0.5;
        }
        session.pivot =
            pointOnCircle(
                owner->center(),
                owner->radius(),
                angle);
        break;
    }
    }

    session.current_input =
        ResolvedSketchInput{session.pivot};
    manipulation_ = std::move(session);
    clearHover();
    return true;
}

bool SketchInteractionState::updateDirectManipulation(
    ResolvedSketchInput input) noexcept {
    if (!manipulation_ || !input.valid()) {
        return false;
    }

    manipulation_->current_input = input;
    return true;
}

std::optional<DirectManipulationGeometry>
SketchInteractionState::directManipulationGeometryState()
    const {
    if (!manipulation_ ||
        !manipulation_->current_input.valid()) {
        return std::nullopt;
    }

    auto result =
        manipulation_->initial_geometry;
    const auto current =
        manipulation_->current_input.position;

    if (manipulation_->mode ==
        DirectEditMode::move) {
        return translateSketchGeometry(
            manipulation_->initial_geometry,
            Point2{
                current.u - manipulation_->pivot.u,
                current.v - manipulation_->pivot.v});
    }

    switch (manipulation_->active_grip.role) {
    case SketchGripRole::line_start:
    case SketchGripRole::line_end:
        if (result.lines.size() != 1U ||
            !result.circles.empty() ||
            !result.arcs.empty()) {
            return std::nullopt;
        }
        if (manipulation_->active_grip.role ==
            SketchGripRole::line_start) {
            result.lines.front().start = current;
        } else {
            result.lines.front().end = current;
        }
        return validReplacement(result.lines.front())
            ? std::optional<DirectManipulationGeometry>{
                  std::move(result)}
            : std::nullopt;

    case SketchGripRole::circle_quadrant_pos_u:
    case SketchGripRole::circle_quadrant_pos_v:
    case SketchGripRole::circle_quadrant_neg_u:
    case SketchGripRole::circle_quadrant_neg_v:
        if (result.circles.size() != 1U ||
            !result.lines.empty() ||
            !result.arcs.empty()) {
            return std::nullopt;
        }
        result.circles.front().radius =
            distance(
                result.circles.front().center,
                current);
        return validReplacement(
                   result.circles.front())
            ? std::optional<DirectManipulationGeometry>{
                  std::move(result)}
            : std::nullopt;

    case SketchGripRole::arc_mid:
        if (result.arcs.size() != 1U ||
            !result.lines.empty() ||
            !result.circles.empty()) {
            return std::nullopt;
        }
        result.arcs.front().radius =
            distance(
                result.arcs.front().center,
                current);
        return validReplacement(result.arcs.front())
            ? std::optional<DirectManipulationGeometry>{
                  std::move(result)}
            : std::nullopt;

    case SketchGripRole::arc_start:
        if (result.arcs.size() != 1U ||
            !result.lines.empty() ||
            !result.circles.empty()) {
            return std::nullopt;
        } else {
            auto& arc = result.arcs.front();
            if (current == arc.center) {
                return std::nullopt;
            }
            const double end_angle =
                arc.start_angle +
                arc.sweep_angle;
            const double new_start =
                direction(arc.center, current);
            const auto sweep =
                sameDirectionSweep(
                    new_start,
                    end_angle,
                    arc.sweep_angle);
            if (!sweep) return std::nullopt;
            arc.start_angle = new_start;
            arc.sweep_angle = *sweep;
            return validReplacement(arc)
                ? std::optional<
                      DirectManipulationGeometry>{
                      std::move(result)}
                : std::nullopt;
        }

    case SketchGripRole::arc_end:
        if (result.arcs.size() != 1U ||
            !result.lines.empty() ||
            !result.circles.empty()) {
            return std::nullopt;
        } else {
            auto& arc = result.arcs.front();
            if (current == arc.center) {
                return std::nullopt;
            }
            const double end_angle =
                direction(arc.center, current);
            const auto sweep =
                sameDirectionSweep(
                    arc.start_angle,
                    end_angle,
                    arc.sweep_angle);
            if (!sweep) return std::nullopt;
            arc.sweep_angle = *sweep;
            return validReplacement(arc)
                ? std::optional<
                      DirectManipulationGeometry>{
                      std::move(result)}
                : std::nullopt;
        }

    case SketchGripRole::line_center:
    case SketchGripRole::circle_center:
    case SketchGripRole::arc_center:
        return std::nullopt;
    }

    return std::nullopt;
}

std::optional<std::vector<SketchLineState>>
SketchInteractionState::directManipulationGeometry()
    const {
    const auto geometry =
        directManipulationGeometryState();
    if (!geometry ||
        !geometry->circles.empty() ||
        !geometry->arcs.empty()) {
        return std::nullopt;
    }
    return geometry->lines;
}

void SketchInteractionState::finishDirectManipulation()
    noexcept {
    manipulation_.reset();
    clearHover();
}

void SketchInteractionState::cancelDirectManipulation()
    noexcept {
    manipulation_.reset();
    clearHover();
}

std::optional<EntityId>
SketchInteractionState::deterministicPrimary()
    const noexcept {
    if (selected_.empty()) {
        return std::nullopt;
    }

    return *std::min_element(
        selected_.begin(),
        selected_.end());
}

void SketchInteractionState::resetToSelect() noexcept {
    tool_ = SketchTool::select;
    resetLineStage();
    resetCircleStage();
    resetArcStage();
    resetMoveStage();
}

void SketchInteractionState::resetLineStage()
    noexcept {
    line_stage_ =
        LineStage::await_first_point;
    line_anchor_.reset();
    pending_line_request_.reset();
}

void SketchInteractionState::resetCircleStage()
    noexcept {
    circle_stage_ =
        CircleStage::await_center;
    circle_center_.reset();
    pending_circle_request_.reset();
}

void SketchInteractionState::resetArcStage()
    noexcept {
    arc_stage_ =
        ArcStage::await_start;
    arc_start_.reset();
    arc_through_.reset();
    pending_arc_request_.reset();
}

void SketchInteractionState::resetMoveStage()
    noexcept {
    move_session_.reset();
}

} // namespace simplesolid2::sketch
