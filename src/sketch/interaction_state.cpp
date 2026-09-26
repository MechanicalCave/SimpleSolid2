#include <simplesolid2/sketch/interaction_state.hpp>

#include <algorithm>
#include <set>
#include <utility>

namespace simplesolid2::sketch {

namespace {

[[nodiscard]] Point2 lineCenter(
    const Line& line) noexcept {
    return {
        (line.start().u + line.end().u) * 0.5,
        (line.start().v + line.end().v) * 0.5};
}

[[nodiscard]] bool validReplacement(
    const SketchLineState& line) noexcept {
    return line.id.valid() &&
           line.start.finite() &&
           line.end.finite() &&
           line.start != line.end;
}

} // namespace

std::optional<LineStage>
SketchInteractionState::lineStage() const noexcept {
    if (tool_ != SketchTool::line) {
        return std::nullopt;
    }
    return line_stage_;
}

void SketchInteractionState::activateLine() noexcept {
    manipulation_.reset();
    clearHover();
    tool_ = SketchTool::line;
    resetLineStage();
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

void SketchInteractionState::finishTool() noexcept {
    manipulation_.reset();
    clearHover();
    resetLineToSelect();
}

void SketchInteractionState::cancelTool() noexcept {
    manipulation_.reset();
    clearHover();
    resetLineToSelect();
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

    if (line_stage_ ==
        LineStage::await_next_point) {
        resetLineStage();
        return true;
    }

    resetLineToSelect();
    return true;
}

void SketchInteractionState::cancelForHistory() noexcept {
    manipulation_.reset();
    clearHover();
    resetLineToSelect();
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
                return model.findLine(id) ==
                    nullptr;
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
    std::optional<LineGripRef> grip) noexcept {
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

std::optional<LineGripRef>
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
    LineGripRef grip) {
    if (tool_ != SketchTool::select ||
        manipulation_ ||
        !grip.valid() ||
        std::find(
            selected_.begin(),
            selected_.end(),
            grip.entity_id) == selected_.end()) {
        return false;
    }

    const auto* owner =
        model.findLine(grip.entity_id);
    if (owner == nullptr) {
        return false;
    }

    DirectManipulationSession session;
    session.active_grip = grip;
    session.selection_snapshot = selected_;

    switch (grip.role) {
    case LineHandleRole::start:
        session.mode = DirectEditMode::reshape;
        session.pivot = owner->start();
        session.initial_geometry.push_back(
            SketchLineState{
                owner->id(),
                owner->start(),
                owner->end()});
        break;

    case LineHandleRole::end:
        session.mode = DirectEditMode::reshape;
        session.pivot = owner->end();
        session.initial_geometry.push_back(
            SketchLineState{
                owner->id(),
                owner->start(),
                owner->end()});
        break;

    case LineHandleRole::center:
        session.mode = DirectEditMode::move;
        session.pivot = lineCenter(*owner);
        session.initial_geometry.reserve(
            session.selection_snapshot.size());
        for (const auto id :
             session.selection_snapshot) {
            const auto* line = model.findLine(id);
            if (line == nullptr) {
                return false;
            }
            session.initial_geometry.push_back(
                SketchLineState{
                    line->id(),
                    line->start(),
                    line->end()});
        }
        break;
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

std::optional<std::vector<SketchLineState>>
SketchInteractionState::directManipulationGeometry()
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
        DirectEditMode::reshape) {
        if (result.size() != 1U) {
            return std::nullopt;
        }

        if (manipulation_->active_grip.role ==
            LineHandleRole::start) {
            result.front().start = current;
        } else if (
            manipulation_->active_grip.role ==
            LineHandleRole::end) {
            result.front().end = current;
        } else {
            return std::nullopt;
        }

        return validReplacement(result.front())
            ? std::optional<std::vector<SketchLineState>>{
                  std::move(result)}
            : std::nullopt;
    }

    if (manipulation_->active_grip.role !=
        LineHandleRole::center) {
        return std::nullopt;
    }

    const double du =
        current.u - manipulation_->pivot.u;
    const double dv =
        current.v - manipulation_->pivot.v;

    for (auto& line : result) {
        line.start.u += du;
        line.start.v += dv;
        line.end.u += du;
        line.end.v += dv;
        if (!validReplacement(line)) {
            return std::nullopt;
        }
    }

    return result;
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

void SketchInteractionState::resetLineToSelect()
    noexcept {
    tool_ = SketchTool::select;
    resetLineStage();
}

void SketchInteractionState::resetLineStage()
    noexcept {
    line_stage_ =
        LineStage::await_first_point;
    line_anchor_.reset();
    pending_line_request_.reset();
}

} // namespace simplesolid2::sketch
