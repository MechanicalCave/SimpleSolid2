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

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    projectSelection();
    notifyStateChanged();
}

void PartSketchInteractionController::end() {
    interaction_ = sketch::SketchInteractionState{};
    press_anchor_.reset();
    rectangle_drag_active_ = false;

    if (viewport_controller_ != nullptr) {
        viewport_controller_->clearSketchPreview();
        viewport_controller_->clearSketchSelectionBoxOverlay();
        if (sketch_id_) {
            static_cast<void>(
                viewport_controller_->projectSketchEntitySelection(
                    {},
                    std::nullopt));
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

std::size_t
PartSketchInteractionController::selectedCount() const noexcept {
    return interaction_.selectedEntities().size();
}

void PartSketchInteractionController::activateSelect() {
    if (!active()) return;

    interaction_.finishTool();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    projectSelection();
    notifyStateChanged();
}

void PartSketchInteractionController::activateLine() {
    if (!active()) return;

    interaction_.activateLine();
    interaction_.clearSelection();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
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

    const bool changed = interaction_.escape();
    if (!changed) return false;

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::deleteSelection() {
    if (!active() ||
        interaction_.tool() != sketch::SketchTool::select ||
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
    viewport_controller_->refreshPresentation();
    projectSelection();
    notifyStateChanged();
    return true;
}

void PartSketchInteractionController::cancelForHistory() {
    if (!active()) return;

    interaction_.cancelForHistory();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    configureForCurrentTool();
    notifyStateChanged();
}

bool PartSketchInteractionController::reconcileAfterHistory() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return false;
    }

    interaction_.reconcileSelection(hosted->model);
    viewport_controller_->refreshPresentation();
    projectSelection();
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

    if (interaction_.tool() ==
        sketch::SketchTool::select) {
        handleSelectPointer(input);
        return;
    }

    handleLinePointer(input);
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
    switch (input.phase) {
    case viewer::SpatialPointerPhase::primary_press:
        press_anchor_ = input.viewport_position;
        rectangle_drag_active_ = false;
        viewport_controller_->clearSketchSelectionBoxOverlay();
        return;

    case viewer::SpatialPointerPhase::move: {
        if (!press_anchor_) return;

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
                reportStatus("Sketch rectangle query returned stale context.");
                return;
            }
            ids.push_back(hit.entity_id);
        }

        if (!interaction_.replaceSelection(
                std::move(ids),
                std::nullopt)) {
            reportStatus("Sketch rectangle selection was rejected.");
            return;
        }

        projectSelection();
        notifyStateChanged();
        return;
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
                interaction_.replaceSelection(
                    queried.hit->entity_id));
        }
    } else if (!input.control) {
        interaction_.clearSelection();
    }

    projectSelection();
    notifyStateChanged();
}

void PartSketchInteractionController::handleLinePointer(
    const SketchPointerInput& input) {
    if (input.phase ==
        viewer::SpatialPointerPhase::move) {
        const auto preview =
            interaction_.previewLine(input.position);
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
        viewer::SpatialPointerPhase::primary_press) {
        return;
    }

    const auto accepted =
        interaction_.acceptLinePoint(input.position);

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

void PartSketchInteractionController::projectSelection() {
    if (!active()) return;

    static_cast<void>(
        viewport_controller_->projectSketchEntitySelection(
            interaction_.selectedEntities(),
            interaction_.primarySelection()));
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
