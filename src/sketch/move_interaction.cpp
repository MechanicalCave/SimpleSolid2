#include <simplesolid2/sketch/interaction_state.hpp>

#include <utility>

namespace simplesolid2::sketch {

std::optional<MoveStage>
SketchInteractionState::moveStage() const noexcept {
    return tool_ == SketchTool::move &&
            move_session_
        ? std::optional<MoveStage>{
              move_session_->stage}
        : std::nullopt;
}

bool SketchInteractionState::activateMove(
    const SketchModel& model) {
    manipulation_.reset();
    resetMoveStage();
    clearHover();
    resetLineStage();
    resetCircleStage();
    resetArcStage();

    MoveSession session;
    if (selected_.empty()) {
        session.stage = MoveStage::select_objects;
    } else {
        const auto captured =
            captureSketchTransformGeometry(
                model,
                selected_);
        if (!captured) {
            tool_ = SketchTool::select;
            return false;
        }

        session.stage =
            MoveStage::await_base_point;
        session.selection_snapshot = selected_;
        session.initial_geometry = *captured;
    }

    move_session_ = std::move(session);
    tool_ = SketchTool::move;
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

    const auto captured =
        captureSketchTransformGeometry(
            model,
            selected_);
    if (!captured) {
        return false;
    }

    move_session_->selection_snapshot = selected_;
    move_session_->initial_geometry = *captured;
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
        move_session_->initial_geometry.empty() ||
        !input.valid()) {
        return false;
    }

    move_session_->base_point = input.position;
    move_session_->current_destination = input;
    move_session_->stage =
        MoveStage::await_destination;
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

    const auto base =
        *move_session_->base_point;
    const auto destination =
        move_session_->current_destination->
            position;

    return translateSketchGeometry(
        move_session_->initial_geometry,
        Point2{
            destination.u - base.u,
            destination.v - base.v});
}

void SketchInteractionState::finishMove() noexcept {
    resetMoveStage();
    clearHover();
    resetToSelect();
}

void SketchInteractionState::resetMoveStage()
    noexcept {
    move_session_.reset();
}

} // namespace simplesolid2::sketch
