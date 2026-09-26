#include "part_sketch_interaction_controller.hpp"

#include <cmath>
#include <utility>
#include <vector>

namespace simplesolid2::ui {

PartSketchInteractionController::PartSketchInteractionController(
    PartViewportController& viewport_controller)
    : viewport_controller_{&viewport_controller} {}

void PartSketchInteractionController::begin(
    application::DocumentSession& session,
    sketch::SketchId sketch_id) {
    session_ = &session;
    sketch_id_ = sketch_id;
    interaction_ = sketch::SketchInteractionState{};
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    projectSelection();
    projectInteraction();
    notifyStateChanged();
}

void PartSketchInteractionController::end() {
    interaction_ = sketch::SketchInteractionState{};
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();

    if (viewport_controller_ != nullptr) {
        viewport_controller_->clearSketchPreview();
        viewport_controller_->clearSketchSelectionBoxOverlay();
        if (sketch_id_) {
            static_cast<void>(
                viewport_controller_->projectSketchEntitySelection(
                    {},
                    std::nullopt));
            static_cast<void>(
                viewport_controller_->projectSketchInteraction(
                    {},
                    std::nullopt,
                    std::nullopt,
                    std::nullopt,
                    false));
        }
    }

    session_ = nullptr;
    sketch_id_.reset();
    notifyStateChanged();
}

bool PartSketchInteractionController::active() const noexcept {
    return activeSketch() != nullptr;
}

sketch::SketchTool
PartSketchInteractionController::tool() const noexcept {
    return interaction_.tool();
}

std::optional<sketch::LineStage>
PartSketchInteractionController::lineStage() const noexcept {
    return interaction_.lineStage();
}

std::optional<sketch::CircleStage>
PartSketchInteractionController::circleStage() const noexcept {
    return interaction_.circleStage();
}

std::optional<sketch::ArcStage>
PartSketchInteractionController::arcStage() const noexcept {
    return interaction_.arcStage();
}

std::size_t
PartSketchInteractionController::selectedCount() const noexcept {
    return interaction_.selectedEntities().size();
}

bool PartSketchInteractionController::
directManipulationActive() const noexcept {
    return interaction_.directManipulationActive();
}

void PartSketchInteractionController::activateSelect() {
    if (!active()) return;

    interaction_.finishTool();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    projectSelection();
    projectInteraction();
    notifyStateChanged();
}

void PartSketchInteractionController::activateLine() {
    if (!active()) return;

    interaction_.activateLine();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    notifyStateChanged();
}

void PartSketchInteractionController::activateCircle() {
    if (!active()) return;

    interaction_.activateCircle();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    notifyStateChanged();
}

void PartSketchInteractionController::activateArc() {
    if (!active()) return;

    interaction_.activateArc();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    notifyStateChanged();
}

void PartSketchInteractionController::finishLine() {
    activateSelect();
}

void PartSketchInteractionController::cancelLine() {
    activateSelect();
}

bool PartSketchInteractionController::escape() {
    if (!active()) return false;

    const bool was_manipulating =
        interaction_.directManipulationActive();
    const bool changed = interaction_.escape();
    if (!changed) return false;

    if (was_manipulating) {
        manipulation_revision_.reset();
    }

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    projectSelection();
    projectInteraction();
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::deleteSelection() {
    if (!active() ||
        interaction_.tool() != sketch::SketchTool::select ||
        interaction_.directManipulationActive() ||
        interaction_.selectedEntities().empty()) {
        return false;
    }

    const auto selected = interaction_.selectedEntities();
    const auto result = session_->execute(
        application::EraseSketchEntitiesCommand{
            *sketch_id_,
            selected});

    if (!result.ok() || !result.changed) {
        reportStatus(
            result.diagnostic.message.empty()
                ? std::string{"Delete Selection failed."}
                : result.diagnostic.message);
        return false;
    }

    interaction_.clearSelection();
    interaction_.clearHover();
    viewport_controller_->refreshPresentation();
    projectSelection();
    projectInteraction();
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::
commitDirectManipulation() {
    if (!active() ||
        !interaction_.directManipulationActive() ||
        !manipulation_revision_) {
        return false;
    }

    const auto geometry =
        interaction_.directManipulationGeometryState();
    if (!geometry) {
        reportStatus(
            "Direct manipulation has no valid commit geometry.");
        return false;
    }

    application::UpdateSketchGeometryCommand command{
        *sketch_id_,
        *manipulation_revision_,
        {},
        {},
        {}};
    command.lines.reserve(geometry->lines.size());
    command.circles.reserve(geometry->circles.size());
    command.arcs.reserve(geometry->arcs.size());

    for (const auto& line : geometry->lines) {
        command.lines.push_back(
            {line.id, line.start, line.end});
    }
    for (const auto& circle : geometry->circles) {
        command.circles.push_back(
            {circle.id, circle.center, circle.radius});
    }
    for (const auto& arc : geometry->arcs) {
        command.arcs.push_back(
            {
                arc.id,
                arc.center,
                arc.radius,
                arc.start_angle,
                arc.sweep_angle});
    }

    const auto result =
        session_->execute(command);

    interaction_.finishDirectManipulation();
    manipulation_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->refreshPresentation();

    if (!result.ok()) {
        const auto* hosted = activeSketch();
        if (hosted != nullptr) {
            interaction_.reconcileSelection(hosted->model);
        }
        projectSelection();
        projectInteraction();
        notifyStateChanged();
        reportStatus(
            result.diagnostic.message.empty()
                ? std::string{
                      "Direct manipulation commit failed."}
                : result.diagnostic.message);
        return false;
    }

    const auto* hosted = activeSketch();
    if (hosted != nullptr) {
        interaction_.reconcileSelection(hosted->model);
    }
    projectSelection();
    projectInteraction();
    notifyStateChanged();
    return true;
}

void PartSketchInteractionController::cancelForHistory() {
    if (!active()) return;

    interaction_.cancelForHistory();
    manipulation_revision_.reset();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    projectSelection();
    projectInteraction();
    notifyStateChanged();
}

bool PartSketchInteractionController::reconcileAfterHistory() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return false;
    }

    interaction_.reconcileSelection(hosted->model);
    interaction_.clearHover();
    manipulation_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->refreshPresentation();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    notifyStateChanged();
    return true;
}

void PartSketchInteractionController::onPointer(
    const SketchPointerInput& input) {
    if (!active() ||
        !sketch_id_ ||
        input.sketch_id != *sketch_id_) {
        return;
    }

    switch (interaction_.tool()) {
    case sketch::SketchTool::select:
        handleSelectPointer(input);
        return;
    case sketch::SketchTool::line:
        handleLinePointer(input);
        return;
    case sketch::SketchTool::circle:
        handleCirclePointer(input);
        return;
    case sketch::SketchTool::arc:
        handleArcPointer(input);
        return;
    }
}

const part::PartSketch*
PartSketchInteractionController::activeSketch() const noexcept {
    if (session_ == nullptr || !sketch_id_) {
        return nullptr;
    }

    return session_->document().findSketch(*sketch_id_);
}

void PartSketchInteractionController::handleSelectPointer(
    const SketchPointerInput& input) {
    if (interaction_.directManipulationActive()) {
        if (input.phase ==
            viewer::SpatialPointerPhase::move) {
            updateDirectManipulationPreview(
                input.position);
            return;
        }

        if (input.phase ==
            viewer::SpatialPointerPhase::
                primary_press) {
            updateDirectManipulationPreview(
                input.position);
            static_cast<void>(
                commitDirectManipulation());
        }
        return;
    }

    switch (input.phase) {
    case viewer::SpatialPointerPhase::primary_press:
        interaction_.clearHover();
        projectInteraction();
        press_anchor_ = input.viewport_position;
        rectangle_drag_active_ = false;
        viewport_controller_->clearSketchSelectionBoxOverlay();
        return;

    case viewer::SpatialPointerPhase::move: {
        if (!press_anchor_) {
            updateHover(input.viewport_position);
            return;
        }

        const auto dx =
            input.viewport_position.x - press_anchor_->x;
        const auto dy =
            input.viewport_position.y - press_anchor_->y;

        if (!rectangle_drag_active_) {
            if (dx == 0.0 || dy == 0.0 ||
                std::hypot(dx, dy) <
                    drag_threshold_pixels) {
                return;
            }
            rectangle_drag_active_ = true;
            interaction_.clearHover();
            projectInteraction();
        }

        updateRectangleOverlay(input.viewport_position);
        return;
    }

    case viewer::SpatialPointerPhase::primary_release:
        break;
    }

    if (!press_anchor_) return;

    const auto anchor = *press_anchor_;
    press_anchor_.reset();

    if (rectangle_drag_active_) {
        rectangle_drag_active_ = false;
        viewport_controller_->clearSketchSelectionBoxOverlay();

        const auto rectangle =
            viewer::normalizedViewportRect(
                anchor,
                input.viewport_position);
        if (!rectangle) {
            return;
        }

        const auto rule =
            input.viewport_position.x >= anchor.x
                ? viewer::SketchRectangleSelectionRule::window
                : viewer::SketchRectangleSelectionRule::crossing;

        const auto queried =
            viewport_controller_->querySketchEntities(
                *rectangle,
                rule);
        if (!queried.completed) {
            reportStatus("Sketch rectangle query failed.");
            return;
        }

        std::vector<sketch::EntityId> ids;
        ids.reserve(queried.hits.size());
        for (const auto& hit : queried.hits) {
            if (hit.sketch_id != *sketch_id_) {
                reportStatus(
                    "Sketch rectangle query returned stale context.");
                return;
            }
            ids.push_back(hit.entity_id);
        }

        const bool accepted =
            input.control
                ? interaction_.toggleSelection(
                      std::move(ids))
                : interaction_.addSelection(
                      std::move(ids));
        if (!accepted) {
            reportStatus(
                "Sketch rectangle selection was rejected.");
            return;
        }

        projectSelection();
        projectInteraction();
        notifyStateChanged();
        return;
    }

    if (!interaction_.selectedEntities().empty()) {
        const auto grip =
            viewport_controller_->querySketchGripAt(
                input.viewport_position);
        if (!grip.completed) {
            reportStatus("Sketch grip query failed.");
            return;
        }

        if (grip.hit) {
            if (grip.hit->sketch_id !=
                *sketch_id_) {
                reportStatus(
                    "Sketch grip query returned stale context.");
                return;
            }

            if (!beginDirectManipulation(
                    grip.hit->grip)) {
                reportStatus(
                    "Sketch grip activation was rejected.");
            }
            return;
        }
    }

    const auto queried =
        viewport_controller_->querySketchEntityAt(
            input.viewport_position);
    if (!queried.completed) {
        reportStatus("Sketch point query failed.");
        return;
    }

    if (queried.hit) {
        if (queried.hit->sketch_id != *sketch_id_) {
            reportStatus("Sketch point query returned stale context.");
            return;
        }

        if (input.control) {
            static_cast<void>(
                interaction_.toggleSelection(
                    queried.hit->entity_id));
        } else {
            static_cast<void>(
                interaction_.addSelection(
                    queried.hit->entity_id));
        }
    } else if (!input.control) {
        interaction_.clearSelection();
    }

    projectSelection();
    projectInteraction();
    notifyStateChanged();
}

void PartSketchInteractionController::handleLinePointer(
    const SketchPointerInput& input) {
    const auto resolved =
        sketch::resolveSketchInput(
            input.position);

    if (input.phase ==
        viewer::SpatialPointerPhase::move) {
        const auto preview =
            resolved
                ? interaction_.previewLine(
                      resolved->position)
                : std::nullopt;
        if (preview) {
            static_cast<void>(
                viewport_controller_->setSketchPreview(
                    {SketchPreviewLine2D{
                        preview->start,
                        preview->end}}));
        } else {
            viewport_controller_->clearSketchPreview();
        }
        return;
    }

    if (input.phase !=
            viewer::SpatialPointerPhase::primary_press ||
        !resolved) {
        return;
    }

    const auto accepted =
        interaction_.acceptLinePoint(
            resolved->position);

    if (accepted.outcome ==
        sketch::LinePointOutcome::segment_requested &&
        accepted.request) {
        const auto result =
            session_->execute(
                application::AddSketchLineCommand{
                    *sketch_id_,
                    accepted.request->start,
                    accepted.request->end});

        const bool committed =
            result.ok() && result.changed;
        static_cast<void>(
            interaction_.resolveLineRequest(committed));

        viewport_controller_->clearSketchPreview();

        if (!committed) {
            reportStatus(
                result.diagnostic.message.empty()
                    ? std::string{"Line segment commit failed."}
                    : result.diagnostic.message);
            notifyStateChanged();
            return;
        }

        viewport_controller_->refreshPresentation();
        projectSelection();
        projectInteraction();
        notifyStateChanged();
        return;
    }

    if (accepted.outcome ==
            sketch::LinePointOutcome::first_point_accepted ||
        accepted.outcome ==
            sketch::LinePointOutcome::zero_length_ignored) {
        viewport_controller_->clearSketchPreview();
        notifyStateChanged();
    }
}

void PartSketchInteractionController::handleCirclePointer(
    const SketchPointerInput& input) {
    const auto resolved =
        sketch::resolveSketchInput(input.position);

    if (input.phase == viewer::SpatialPointerPhase::move) {
        const auto preview =
            resolved
                ? interaction_.previewCircle(
                      resolved->position)
                : std::nullopt;
        if (preview) {
            static_cast<void>(
                viewport_controller_->setSketchCirclePreview(
                    *preview));
        } else {
            viewport_controller_->clearSketchPreview();
        }
        return;
    }

    if (input.phase !=
            viewer::SpatialPointerPhase::primary_press ||
        !resolved) {
        return;
    }

    const auto accepted =
        interaction_.acceptCirclePoint(
            resolved->position);

    if (accepted.outcome ==
            sketch::CirclePointOutcome::circle_requested &&
        accepted.request) {
        const auto result =
            session_->execute(
                application::AddSketchCircleCommand{
                    *sketch_id_,
                    accepted.request->center,
                    accepted.request->radius});
        const bool committed =
            result.ok() && result.changed;
        static_cast<void>(
            interaction_.resolveCircleRequest(committed));

        viewport_controller_->clearSketchPreview();
        if (!committed) {
            reportStatus(
                result.diagnostic.message.empty()
                    ? std::string{"Circle commit failed."}
                    : result.diagnostic.message);
            notifyStateChanged();
            return;
        }

        viewport_controller_->refreshPresentation();
        projectSelection();
        projectInteraction();
        notifyStateChanged();
        return;
    }

    if (accepted.outcome ==
            sketch::CirclePointOutcome::center_accepted ||
        accepted.outcome ==
            sketch::CirclePointOutcome::zero_radius_ignored) {
        viewport_controller_->clearSketchPreview();
        notifyStateChanged();
    }
}

void PartSketchInteractionController::handleArcPointer(
    const SketchPointerInput& input) {
    const auto resolved =
        sketch::resolveSketchInput(input.position);

    if (input.phase == viewer::SpatialPointerPhase::move) {
        const auto preview =
            resolved
                ? interaction_.previewArc(
                      resolved->position)
                : std::nullopt;
        if (preview) {
            static_cast<void>(
                viewport_controller_->setSketchArcPreview(
                    *preview));
        } else {
            viewport_controller_->clearSketchPreview();
        }
        return;
    }

    if (input.phase !=
            viewer::SpatialPointerPhase::primary_press ||
        !resolved) {
        return;
    }

    const auto accepted =
        interaction_.acceptArcPoint(
            resolved->position);

    if (accepted.outcome ==
            sketch::ArcPointOutcome::arc_requested &&
        accepted.request) {
        const auto result =
            session_->execute(
                application::AddSketchArcCommand{
                    *sketch_id_,
                    accepted.request->center,
                    accepted.request->radius,
                    accepted.request->start_angle,
                    accepted.request->sweep_angle});
        const bool committed =
            result.ok() && result.changed;
        static_cast<void>(
            interaction_.resolveArcRequest(committed));

        viewport_controller_->clearSketchPreview();
        if (!committed) {
            reportStatus(
                result.diagnostic.message.empty()
                    ? std::string{"Arc commit failed."}
                    : result.diagnostic.message);
            notifyStateChanged();
            return;
        }

        viewport_controller_->refreshPresentation();
        projectSelection();
        projectInteraction();
        notifyStateChanged();
        return;
    }

    if (accepted.outcome ==
            sketch::ArcPointOutcome::start_accepted ||
        accepted.outcome ==
            sketch::ArcPointOutcome::through_accepted ||
        accepted.outcome ==
            sketch::ArcPointOutcome::degenerate_ignored) {
        viewport_controller_->clearSketchPreview();
        notifyStateChanged();
    }
}

void PartSketchInteractionController::updateRectangleOverlay(
    viewer::ViewportPoint2 current) {
    if (!press_anchor_) return;

    const auto rule =
        current.x >= press_anchor_->x
            ? viewer::SketchRectangleSelectionRule::window
            : viewer::SketchRectangleSelectionRule::crossing;

    static_cast<void>(
        viewport_controller_->setSketchSelectionBoxOverlay(
            viewer::SketchSelectionBoxOverlay{
                *press_anchor_,
                current,
                rule}));
}

void PartSketchInteractionController::updateHover(
    viewer::ViewportPoint2 point) {
    if (interaction_.tool() !=
            sketch::SketchTool::select ||
        interaction_.directManipulationActive()) {
        return;
    }

    if (!interaction_.selectedEntities().empty()) {
        const auto grip =
            viewport_controller_->querySketchGripAt(
                point);
        if (!grip.completed) {
            interaction_.clearHover();
            projectInteraction();
            return;
        }

        if (grip.hit) {
            if (grip.hit->sketch_id !=
                *sketch_id_) {
                interaction_.clearHover();
                projectInteraction();
                return;
            }

            static_cast<void>(
                interaction_.setHoveredGrip(
                    grip.hit->grip));
            projectInteraction();
            return;
        }
    }

    const auto entity =
        viewport_controller_->querySketchEntityAt(
            point);
    if (!entity.completed) {
        interaction_.clearHover();
        projectInteraction();
        return;
    }

    if (entity.hit) {
        if (entity.hit->sketch_id !=
            *sketch_id_) {
            interaction_.clearHover();
        } else {
            static_cast<void>(
                interaction_.setHoveredEntity(
                    entity.hit->entity_id));
        }
    } else {
        interaction_.clearHover();
    }

    projectInteraction();
}

bool PartSketchInteractionController::
beginDirectManipulation(
    sketch::SketchGripRef grip) {
    const auto* hosted = activeSketch();
    if (hosted == nullptr ||
        session_ == nullptr) {
        return false;
    }

    if (!interaction_.beginDirectManipulation(
            hosted->model,
            grip)) {
        return false;
    }

    manipulation_revision_ =
        session_->document().revision();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchSelectionBoxOverlay();
    viewport_controller_->clearSketchPreview();
    projectSelection();
    projectInteraction();
    notifyStateChanged();
    return true;
}

void PartSketchInteractionController::
updateDirectManipulationPreview(
    sketch::Point2 raw_input) {
    const auto resolved =
        sketch::resolveSketchInput(raw_input);
    if (!resolved ||
        !interaction_.updateDirectManipulation(*resolved)) {
        viewport_controller_->clearSketchPreview();
        return;
    }

    const auto geometry =
        interaction_.directManipulationGeometryState();
    if (!geometry ||
        !viewport_controller_->setSketchGeometryPreview(
            *geometry)) {
        viewport_controller_->clearSketchPreview();
    }
}

void PartSketchInteractionController::projectSelection() {
    if (!active()) return;

    static_cast<void>(
        viewport_controller_->projectSketchEntitySelection(
            interaction_.selectedEntities(),
            interaction_.primarySelection()));
}

void PartSketchInteractionController::projectInteraction() {
    if (!active()) return;

    static_cast<void>(
        viewport_controller_->projectSketchInteraction(
            interaction_.selectedEntities(),
            interaction_.hoveredEntity(),
            interaction_.hoveredGrip(),
            interaction_.activeGrip(),
            interaction_.tool() ==
                sketch::SketchTool::select));
}

void PartSketchInteractionController::configureForCurrentTool() {
    if (!active()) return;

    static_cast<void>(
        viewport_controller_->setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));

    static_cast<void>(
        viewport_controller_->setSketchCursorMode(
            interaction_.tool() ==
                    sketch::SketchTool::select
                ? viewer::ViewportCursorMode::
                      select_pick_box
                : viewer::ViewportCursorMode::
                      create_edit_crosshair));
}

void PartSketchInteractionController::notifyStateChanged() {
    if (state_changed_handler_) {
        state_changed_handler_();
    }
}

void PartSketchInteractionController::reportStatus(
    std::string message) {
    if (status_handler_) {
        status_handler_(message);
    }
}

} // namespace simplesolid2::ui
