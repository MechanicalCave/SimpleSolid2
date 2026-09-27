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
    transform_revision_.reset();
    last_repeatable_command_.reset();

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
    transform_revision_.reset();
    last_repeatable_command_.reset();

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

std::optional<sketch::MoveStage>
PartSketchInteractionController::moveStage() const noexcept {
    return interaction_.moveStage();
}

std::optional<sketch::CommonTransformStage>
PartSketchInteractionController::commonTransformStage()
    const noexcept {
    return interaction_.commonTransformStage();
}

std::size_t
PartSketchInteractionController::selectedCount() const noexcept {
    return interaction_.selectedEntities().size();
}

bool PartSketchInteractionController::
directManipulationActive() const noexcept {
    return interaction_.directManipulationActive();
}

std::optional<sketch::DirectEditMode>
PartSketchInteractionController::directEditMode() const noexcept {
    return interaction_.directEditMode();
}

bool PartSketchInteractionController::cycleDirectEditMode() {
    if (!active() ||
        !interaction_.directManipulationActive()) {
        return false;
    }

    const bool changed =
        interaction_.cycleDirectEditMode();
    if (!changed) {
        reportStatus(
            "Only Move is available for this grip.");
        notifyStateChanged();
        return false;
    }

    const auto geometry =
        interaction_.directManipulationGeometryState();
    if (!geometry ||
        !viewport_controller_->setSketchGeometryPreview(
            *geometry)) {
        viewport_controller_->clearSketchPreview();
    }

    projectInteraction();
    notifyStateChanged();

    reportStatus(
        interaction_.directEditMode() ==
                sketch::DirectEditMode::move
            ? "Grip edit mode: Move."
            : "Grip edit mode: Reshape.");
    return true;
}

bool PartSketchInteractionController::repeatLastCommand() {
    if (!active() ||
        interaction_.tool() != sketch::SketchTool::select ||
        interaction_.directManipulationActive() ||
        interaction_.commonTransformStage().has_value() ||
        !last_repeatable_command_) {
        return false;
    }

    switch (*last_repeatable_command_) {
    case sketch::SketchTool::line:
        activateLine();
        return true;
    case sketch::SketchTool::circle:
        activateCircle();
        return true;
    case sketch::SketchTool::arc:
        activateArc();
        return true;
    case sketch::SketchTool::move:
        return activateMove();
    case sketch::SketchTool::copy:
        return activateCopy();
    case sketch::SketchTool::rotate:
        return activateRotate();
    case sketch::SketchTool::scale:
        return activateScale();
    case sketch::SketchTool::mirror:
        return activateMirror();
    case sketch::SketchTool::select:
        return false;
    }

    return false;
}

void PartSketchInteractionController::activateSelect() {
    if (!active()) return;

    interaction_.finishTool();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_.reset();
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
    transform_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::line;
    notifyStateChanged();
}

void PartSketchInteractionController::activateCircle() {
    if (!active()) return;

    interaction_.activateCircle();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::circle;
    notifyStateChanged();
}

void PartSketchInteractionController::activateArc() {
    if (!active()) return;

    interaction_.activateArc();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_.reset();
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::arc;
    notifyStateChanged();
}

bool PartSketchInteractionController::activateMove() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr || session_ == nullptr ||
        !interaction_.activateMove(hosted->model)) {
        return false;
    }

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_ =
        interaction_.commonTransformStage() ==
                sketch::CommonTransformStage::select_objects
            ? std::nullopt
            : std::optional<core::DocumentRevision>{
                  session_->document().revision()};

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::move;
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::activateCopy() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr || session_ == nullptr ||
        !interaction_.activateCopy(hosted->model)) {
        return false;
    }

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_ =
        interaction_.commonTransformStage() ==
                sketch::CommonTransformStage::select_objects
            ? std::nullopt
            : std::optional<core::DocumentRevision>{
                  session_->document().revision()};

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::copy;
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::activateRotate() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr || session_ == nullptr ||
        !interaction_.activateRotate(hosted->model)) {
        return false;
    }

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_ =
        interaction_.commonTransformStage() ==
                sketch::CommonTransformStage::select_objects
            ? std::nullopt
            : std::optional<core::DocumentRevision>{
                  session_->document().revision()};

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::rotate;
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::activateScale() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr || session_ == nullptr ||
        !interaction_.activateScale(hosted->model)) {
        return false;
    }

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_ =
        interaction_.commonTransformStage() ==
                sketch::CommonTransformStage::select_objects
            ? std::nullopt
            : std::optional<core::DocumentRevision>{
                  session_->document().revision()};

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::scale;
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::activateMirror() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr || session_ == nullptr ||
        !interaction_.activateMirror(hosted->model)) {
        return false;
    }

    press_anchor_.reset();
    rectangle_drag_active_ = false;
    manipulation_revision_.reset();
    transform_revision_ =
        interaction_.commonTransformStage() ==
                sketch::CommonTransformStage::select_objects
            ? std::nullopt
            : std::optional<core::DocumentRevision>{
                  session_->document().revision()};

    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    last_repeatable_command_ =
        sketch::SketchTool::mirror;
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::
completeTransformSelection() {
    const auto* hosted = activeSketch();
    if (hosted == nullptr || session_ == nullptr ||
        !interaction_.completeTransformSelection(
            hosted->model)) {
        return false;
    }

    transform_revision_ =
        session_->document().revision();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchSelectionBoxOverlay();
    viewport_controller_->clearSketchPreview();
    projectSelection();
    projectInteraction();
    configureForCurrentTool();
    notifyStateChanged();
    return true;
}

bool PartSketchInteractionController::
completeMoveSelection() {
    return interaction_.tool() ==
               sketch::SketchTool::move &&
           completeTransformSelection();
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
    const bool was_transform =
        interaction_.commonTransformStage().has_value();
    const bool changed = interaction_.escape();
    if (!changed) return false;

    if (was_manipulating) {
        manipulation_revision_.reset();
    }
    if (was_transform) {
        transform_revision_.reset();
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

application::DocumentSessionResult
PartSketchInteractionController::executeGeometryUpdate(
    const sketch::SketchTransformGeometry& geometry,
    core::DocumentRevision expected_revision) {
    application::UpdateSketchGeometryCommand command{
        *sketch_id_,
        expected_revision,
        {},
        {},
        {}};
    command.lines.reserve(geometry.lines.size());
    command.circles.reserve(geometry.circles.size());
    command.arcs.reserve(geometry.arcs.size());

    for (const auto& line : geometry.lines) {
        command.lines.push_back(
            {line.id, line.start, line.end});
    }
    for (const auto& circle : geometry.circles) {
        command.circles.push_back(
            {circle.id, circle.center, circle.radius});
    }
    for (const auto& arc : geometry.arcs) {
        command.arcs.push_back(
            {
                arc.id,
                arc.center,
                arc.radius,
                arc.start_angle,
                arc.sweep_angle});
    }

    return session_->execute(command);
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

    const auto result =
        executeGeometryUpdate(
            *geometry,
            *manipulation_revision_);

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

bool PartSketchInteractionController::commitTransform() {
    if (!active() || !transform_revision_) {
        return false;
    }

    const auto stage =
        interaction_.commonTransformStage();
    const auto tool =
        interaction_.tool();
    const bool final_stage =
        stage &&
        ((*stage ==
              sketch::CommonTransformStage::await_destination &&
          (tool == sketch::SketchTool::move ||
           tool == sketch::SketchTool::copy ||
           tool == sketch::SketchTool::rotate ||
           tool == sketch::SketchTool::scale)) ||
         (*stage ==
              sketch::CommonTransformStage::await_axis_end &&
          tool == sketch::SketchTool::mirror));
    if (!final_stage) {
        return false;
    }

    const auto geometry =
        interaction_.transformGeometryState();
    if (!geometry) {
        reportStatus(
            "Transform has no valid commit geometry.");
        return false;
    }

    const char* command_name =
        tool == sketch::SketchTool::move
            ? "MOVE"
            : tool == sketch::SketchTool::copy
                ? "COPY"
                : tool == sketch::SketchTool::rotate
                    ? "ROTATE"
                    : tool == sketch::SketchTool::scale
                        ? "SCALE"
                        : "MIRROR";

    if (tool == sketch::SketchTool::copy) {
        if (session_ == nullptr || !sketch_id_ ||
            session_->document().revision() !=
                *transform_revision_) {
            interaction_.finishTransform();
            transform_revision_.reset();
            viewport_controller_->clearSketchPreview();
            configureForCurrentTool();
            projectSelection();
            projectInteraction();
            notifyStateChanged();
            reportStatus(
                "COPY was started from a stale DocumentRevision.");
            return false;
        }

        const auto* hosted = activeSketch();
        const auto source =
            hosted != nullptr
                ? sketch::captureSketchTransformGeometry(
                      hosted->model,
                      interaction_.selectedEntities())
                : std::nullopt;
        if (!source) {
            interaction_.finishTransform();
            transform_revision_.reset();
            viewport_controller_->clearSketchPreview();
            configureForCurrentTool();
            projectSelection();
            projectInteraction();
            notifyStateChanged();
            reportStatus(
                "COPY source selection is no longer valid.");
            return false;
        }

        if (*geometry == *source) {
            viewport_controller_->clearSketchPreview();
            reportStatus(
                "COPY requires a non-zero placement.");
            notifyStateChanged();
            return false;
        }

        const auto result =
            session_->execute(
                application::DuplicateSketchGeometryCommand{
                    *sketch_id_,
                    *transform_revision_,
                    *geometry});

        press_anchor_.reset();
        rectangle_drag_active_ = false;
        viewport_controller_->clearSketchPreview();
        viewport_controller_->clearSketchSelectionBoxOverlay();

        if (!result.ok() || !result.changed) {
            interaction_.finishTransform();
            transform_revision_.reset();
            configureForCurrentTool();
            projectSelection();
            projectInteraction();
            notifyStateChanged();
            reportStatus(
                result.diagnostic.message.empty()
                    ? std::string{"COPY commit failed."}
                    : result.diagnostic.message);
            return false;
        }

        viewport_controller_->refreshPresentation();

        if (!interaction_.continueCopyPlacement()) {
            interaction_.finishTransform();
            transform_revision_.reset();
            configureForCurrentTool();
            projectSelection();
            projectInteraction();
            notifyStateChanged();
            reportStatus(
                "COPY committed; repeated placement session ended unexpectedly.");
            return true;
        }

        transform_revision_ =
            session_->document().revision();
        configureForCurrentTool();
        projectSelection();
        projectInteraction();
        notifyStateChanged();
        reportStatus("COPY placement committed.");
        return true;
    }

    const auto result =
        executeGeometryUpdate(
            *geometry,
            *transform_revision_);

    interaction_.finishTransform();
    transform_revision_.reset();
    press_anchor_.reset();
    rectangle_drag_active_ = false;
    viewport_controller_->clearSketchPreview();
    viewport_controller_->clearSketchSelectionBoxOverlay();
    viewport_controller_->refreshPresentation();

    const auto* hosted = activeSketch();
    if (hosted != nullptr) {
        interaction_.reconcileSelection(hosted->model);
    }

    configureForCurrentTool();
    projectSelection();
    projectInteraction();
    notifyStateChanged();

    if (!result.ok()) {
        reportStatus(
            result.diagnostic.message.empty()
                ? std::string{command_name} +
                      " commit failed."
                : result.diagnostic.message);
        return false;
    }

    reportStatus(
        result.changed
            ? std::string{command_name} + " committed."
            : std::string{command_name} +
                  " completed with no authored change.");
    return true;
}

bool PartSketchInteractionController::commitMove() {
    return interaction_.tool() ==
               sketch::SketchTool::move &&
           commitTransform();
}

void PartSketchInteractionController::cancelForHistory() {
    if (!active()) return;

    interaction_.cancelForHistory();
    manipulation_revision_.reset();
    transform_revision_.reset();
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
    transform_revision_.reset();
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
    case sketch::SketchTool::move:
    case sketch::SketchTool::copy:
    case sketch::SketchTool::rotate:
    case sketch::SketchTool::scale:
    case sketch::SketchTool::mirror:
        handleCommonTransformPointer(input);
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

void PartSketchInteractionController::
handleCommonTransformPointer(
    const SketchPointerInput& input) {
    const auto stage =
        interaction_.commonTransformStage();
    if (!stage) return;

    const auto tool = interaction_.tool();
    const char* command_name =
        tool == sketch::SketchTool::move
            ? "MOVE"
            : tool == sketch::SketchTool::copy
                ? "COPY"
                : tool == sketch::SketchTool::rotate
                    ? "ROTATE"
                    : tool == sketch::SketchTool::scale
                        ? "SCALE"
                        : "MIRROR";

    if (*stage ==
        sketch::CommonTransformStage::select_objects) {
        switch (input.phase) {
        case viewer::SpatialPointerPhase::primary_press:
            interaction_.clearHover();
            projectInteraction();
            press_anchor_ = input.viewport_position;
            rectangle_drag_active_ = false;
            viewport_controller_->
                clearSketchSelectionBoxOverlay();
            return;

        case viewer::SpatialPointerPhase::move: {
            if (!press_anchor_) {
                updateHover(input.viewport_position);
                return;
            }

            const auto dx =
                input.viewport_position.x -
                press_anchor_->x;
            const auto dy =
                input.viewport_position.y -
                press_anchor_->y;

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

            updateRectangleOverlay(
                input.viewport_position);
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
            viewport_controller_->
                clearSketchSelectionBoxOverlay();

            const auto rectangle =
                viewer::normalizedViewportRect(
                    anchor,
                    input.viewport_position);
            if (!rectangle) return;

            const auto rule =
                input.viewport_position.x >= anchor.x
                    ? viewer::SketchRectangleSelectionRule::
                          window
                    : viewer::SketchRectangleSelectionRule::
                          crossing;

            const auto queried =
                viewport_controller_->
                    querySketchEntities(
                        *rectangle,
                        rule);
            if (!queried.completed) {
                reportStatus(
                    std::string{command_name} +
                    " rectangle query failed.");
                return;
            }

            std::vector<sketch::EntityId> ids;
            ids.reserve(queried.hits.size());
            for (const auto& hit : queried.hits) {
                if (hit.sketch_id != *sketch_id_) {
                    reportStatus(
                        std::string{command_name} +
                        " rectangle query returned stale context.");
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
                    std::string{command_name} +
                    " object selection was rejected.");
                return;
            }

            projectSelection();
            projectInteraction();
            notifyStateChanged();
            return;
        }

        const auto queried =
            viewport_controller_->querySketchEntityAt(
                input.viewport_position);
        if (!queried.completed) {
            reportStatus(
                std::string{command_name} +
                " point query failed.");
            return;
        }

        if (queried.hit) {
            if (queried.hit->sketch_id !=
                *sketch_id_) {
                reportStatus(
                    std::string{command_name} +
                    " point query returned stale context.");
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

            projectSelection();
            projectInteraction();
            notifyStateChanged();
        }

        // Blank LMB is intentionally a no-op during
        // command-first common-transform object collection.
        return;
    }

    const auto resolved =
        sketch::resolveSketchInput(input.position);

    const bool reference_stage =
        *stage ==
            sketch::CommonTransformStage::await_base_point ||
        *stage ==
            sketch::CommonTransformStage::await_reference_point ||
        *stage ==
            sketch::CommonTransformStage::await_axis_start;

    if (reference_stage) {
        if (input.phase ==
                viewer::SpatialPointerPhase::primary_press &&
            resolved &&
            interaction_.acceptTransformPoint(
                *resolved)) {
            viewport_controller_->clearSketchPreview();
            configureForCurrentTool();
            projectInteraction();
            notifyStateChanged();
        }
        return;
    }

    const bool preview_stage =
        *stage ==
            sketch::CommonTransformStage::await_destination ||
        *stage ==
            sketch::CommonTransformStage::await_axis_end;
    if (!preview_stage) {
        return;
    }

    if (input.phase ==
        viewer::SpatialPointerPhase::move) {
        updateCommonTransformPreview(
            input.position);
        return;
    }

    if (input.phase ==
        viewer::SpatialPointerPhase::primary_press) {
        updateCommonTransformPreview(
            input.position);
        static_cast<void>(commitTransform());
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
    const bool select_mode =
        interaction_.tool() ==
        sketch::SketchTool::select;
    const bool transform_collect_mode =
        interaction_.commonTransformStage() ==
            sketch::CommonTransformStage::select_objects;

    if ((!select_mode && !transform_collect_mode) ||
        interaction_.directManipulationActive()) {
        return;
    }

    if (select_mode &&
        !interaction_.selectedEntities().empty()) {
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

void PartSketchInteractionController::
updateCommonTransformPreview(
    sketch::Point2 raw_input) {
    const auto resolved =
        sketch::resolveSketchInput(raw_input);
    if (!resolved ||
        !interaction_.updateTransformPreview(
            *resolved)) {
        viewport_controller_->clearSketchPreview();
        return;
    }

    const auto geometry =
        interaction_.transformGeometryState();
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

    const bool pick_box =
        interaction_.tool() ==
            sketch::SketchTool::select ||
        interaction_.commonTransformStage() ==
            sketch::CommonTransformStage::select_objects;

    static_cast<void>(
        viewport_controller_->setSketchCursorMode(
            pick_box
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
