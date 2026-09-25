#include <simplesolid2/sketch/interaction_state.hpp>

#include <algorithm>
#include <set>
#include <utility>

namespace simplesolid2::sketch {

std::optional<LineStage>
SketchInteractionState::lineStage() const noexcept {
    if (tool_ != SketchTool::line) {
        return std::nullopt;
    }
    return line_stage_;
}

void SketchInteractionState::activateLine() noexcept {
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
    resetLineToSelect();
}

void SketchInteractionState::cancelTool() noexcept {
    resetLineToSelect();
}

bool SketchInteractionState::escape() noexcept {
    if (tool_ == SketchTool::select) {
        return false;
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
    resetLineToSelect();
}

bool SketchInteractionState::replaceSelection(
    EntityId id) {
    if (!id.valid()) {
        return false;
    }

    selected_ = {id};
    primary_ = id;
    return true;
}

bool SketchInteractionState::toggleSelection(
    EntityId id) {
    if (!id.valid()) {
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
        primary_ = selected_.back();
    }

    return true;
}

void SketchInteractionState::clearSelection() noexcept {
    selected_.clear();
    primary_.reset();
}

bool SketchInteractionState::replaceSelection(
    std::vector<EntityId> ids,
    std::optional<EntityId> primary) {
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
        primary_ = selected_.empty()
            ? std::nullopt
            : std::optional<EntityId>{
                  selected_.back()};
    }
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
