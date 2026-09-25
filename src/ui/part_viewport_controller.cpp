#include "part_viewport_controller.hpp"
#include "sketch_viewport_mapping.hpp"

#include <QPointer>

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr viewer::Vec3 xAxis{1.0, 0.0, 0.0};
constexpr viewer::Vec3 yAxis{0.0, 1.0, 0.0};
constexpr viewer::Vec3 zAxis{0.0, 0.0, 1.0};

constexpr double axisExtent = 45.0;
constexpr double planeExtent = 35.0;
constexpr double pointExtent = 3.0;

viewer::PresentationToken presentationTokenFor(
    core::BuiltinReferenceRole role) noexcept {
    return viewer::PresentationToken{
        0x100U +
        static_cast<std::uint64_t>(role) +
        1U};
}

viewer::ReferencePresentation makeReference(
    core::BuiltinReferenceRole role,
    bool visible) {
    viewer::ReferencePresentation reference;
    reference.token =
        presentationTokenFor(role);
    reference.visible = visible;

    switch (role) {
    case core::BuiltinReferenceRole::origin_point:
        reference.kind =
            viewer::ReferencePresentationKind::point;
        reference.extent = pointExtent;
        break;

    case core::BuiltinReferenceRole::x_axis:
        reference.kind =
            viewer::ReferencePresentationKind::x_axis;
        reference.u_axis = xAxis;
        reference.extent = axisExtent;
        break;

    case core::BuiltinReferenceRole::y_axis:
        reference.kind =
            viewer::ReferencePresentationKind::y_axis;
        reference.u_axis = yAxis;
        reference.extent = axisExtent;
        break;

    case core::BuiltinReferenceRole::z_axis:
        reference.kind =
            viewer::ReferencePresentationKind::z_axis;
        reference.u_axis = zAxis;
        reference.extent = axisExtent;
        break;

    case core::BuiltinReferenceRole::xy_plane:
        reference.kind =
            viewer::ReferencePresentationKind::plane;
        reference.u_axis = xAxis;
        reference.v_axis = yAxis;
        reference.extent = planeExtent;
        break;

    case core::BuiltinReferenceRole::xz_plane:
        reference.kind =
            viewer::ReferencePresentationKind::plane;
        reference.u_axis = xAxis;
        reference.v_axis = zAxis;
        reference.extent = planeExtent;
        break;

    case core::BuiltinReferenceRole::yz_plane:
        reference.kind =
            viewer::ReferencePresentationKind::plane;
        reference.u_axis = yAxis;
        reference.v_axis = zAxis;
        reference.extent = planeExtent;
        break;
    }

    return reference;
}

} // namespace

PartViewportController::PartViewportController(
    PartDocumentTreeController& tree,
    viewer::IDocumentViewport* viewport,
    QObject* parent)
    : QObject{parent},
      tree_{&tree},
      viewport_{viewport} {
    tree_->setSelectionHandler(
        [this](
            const std::vector<core::BuiltinReferenceRole>& selected,
            std::optional<core::BuiltinReferenceRole> primary) {
            onTreeSelection(selected, primary);
        });

    if (viewport_ != nullptr) {
        const QPointer<PartViewportController> self{this};

        viewport_->setSelectionIntentHandler(
            [self](const viewer::SelectionIntent& intent) {
                if (self) {
                    self->onViewportIntent(intent);
                }
            });

        viewport_->setSpatialPointerHandler(
            [self](const viewer::SpatialPointerEvent& event) {
                if (self) {
                    self->onSpatialPointer(event);
                }
            });

        applySketchViewportMode();
    }
}

void PartViewportController::setDocumentSession(
    application::DocumentSession* session) {
    if (session_ != session) {
        sketch_edit_id_.reset();
        sketch_primary_pointer_routing_ =
            viewer::PrimaryPointerRouting::
                presentation_selection;
        sketch_cursor_mode_ =
            viewer::ViewportCursorMode::
                select_pick_box;
        sketch_entity_bindings_.clear();
        clearSketchPreview();
        clearSketchSelectionBoxOverlay();
    }

    session_ = session;
    tree_->setDocumentSession(session_);

    applySketchViewportMode();
    refreshPresentation();
    applySelectionToSurfaces();
    notifySelectionChanged();
}

void PartViewportController::clear() {
    session_ = nullptr;
    sketch_edit_id_.reset();
    sketch_primary_pointer_routing_ =
        viewer::PrimaryPointerRouting::
            presentation_selection;
    sketch_cursor_mode_ =
        viewer::ViewportCursorMode::
            select_pick_box;
    sketch_entity_bindings_.clear();
    clearSketchSelectionBoxOverlay();
    tree_->clear();

    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setReferenceScene(
                viewer::ReferenceScene{}));
        static_cast<void>(
            viewport_->setSketchScene(
                viewer::SketchScene{}));
        static_cast<void>(
            viewport_->setSketchPreviewScene(
                viewer::SketchPreviewScene{}));
        static_cast<void>(
            viewport_->setPresentationSelection(
                viewer::PresentationSelection{}));
    }

    applySketchViewportMode();
    notifySelectionChanged();
}

void PartViewportController::resetRuntimeState() {
    selections_.clear();
    clear();
}

void PartViewportController::refreshPresentation() {
    if (viewport_ == nullptr) return;

    if (session_ == nullptr) {
        sketch_entity_bindings_.clear();
        clearSketchPreview();
        clearSketchSelectionBoxOverlay();
        applySketchViewportMode();
        static_cast<void>(
            viewport_->setReferenceScene(
                viewer::ReferenceScene{}));
        static_cast<void>(
            viewport_->setSketchScene(
                viewer::SketchScene{}));
        return;
    }

    if (sketch_edit_id_ &&
        activeSketch() == nullptr) {
        sketch_edit_id_.reset();
        sketch_primary_pointer_routing_ =
            viewer::PrimaryPointerRouting::
                presentation_selection;
        sketch_cursor_mode_ =
            viewer::ViewportCursorMode::
                select_pick_box;
        sketch_entity_bindings_.clear();
        clearSketchPreview();
        clearSketchSelectionBoxOverlay();
        applySketchViewportMode();
    }

    static_cast<void>(
        viewport_->setReferenceScene(
            buildReferenceScene()));

    const auto sketch_scene =
        buildSketchScene();
    static_cast<void>(
        viewport_->setSketchScene(
            sketch_scene
                ? *sketch_scene
                : viewer::SketchScene{}));

    applySelectionToSurfaces();
}

void PartViewportController::setSketchEditSketch(
    std::optional<sketch::SketchId> sketch_id) {
    if (sketch_edit_id_ == sketch_id) {
        if (sketch_edit_id_ &&
            activeSketch() == nullptr) {
            sketch_edit_id_.reset();
            sketch_primary_pointer_routing_ =
                viewer::PrimaryPointerRouting::
                    presentation_selection;
            sketch_cursor_mode_ =
                viewer::ViewportCursorMode::
                    select_pick_box;
            sketch_entity_bindings_.clear();
            clearSketchPreview();
            clearSketchSelectionBoxOverlay();
        }

        applySketchViewportMode();
        refreshPresentation();
        return;
    }

    sketch_edit_id_ = std::move(sketch_id);
    sketch_primary_pointer_routing_ =
        viewer::PrimaryPointerRouting::
            presentation_selection;
    sketch_cursor_mode_ =
        viewer::ViewportCursorMode::
            select_pick_box;
    sketch_entity_bindings_.clear();
    clearSketchPreview();
    clearSketchSelectionBoxOverlay();

    if (sketch_edit_id_ &&
        activeSketch() == nullptr) {
        sketch_edit_id_.reset();
    }

    applySketchViewportMode();
    refreshPresentation();
}

bool PartViewportController::setSketchPreview(
    const std::vector<SketchPreviewLine2D>& lines) {
    if (viewport_ == nullptr) {
        return false;
    }

    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return false;
    }

    viewer::SketchPreviewScene scene;
    scene.lines.reserve(lines.size());

    for (const auto& line : lines) {
        if (!line.valid()) {
            return false;
        }

        const auto start =
            detail::sketchPointToWorld(
                hosted->placement,
                line.start);
        const auto end =
            detail::sketchPointToWorld(
                hosted->placement,
                line.end);
        if (!start || !end) {
            return false;
        }

        scene.lines.push_back(
            viewer::SketchPreviewLine{
                *start,
                *end});
    }

    if (!scene.valid()) {
        return false;
    }

    return viewport_->setSketchPreviewScene(
        scene);
}

void PartViewportController::clearSketchPreview() {
    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setSketchPreviewScene(
                viewer::SketchPreviewScene{}));
    }
}

bool PartViewportController::setSketchPrimaryPointerRouting(
    viewer::PrimaryPointerRouting routing) {
    if (activeSketch() == nullptr) {
        return false;
    }

    sketch_primary_pointer_routing_ = routing;
    applySketchViewportMode();
    return true;
}

bool PartViewportController::setSketchCursorMode(
    viewer::ViewportCursorMode mode) {
    if (activeSketch() == nullptr) {
        return false;
    }

    sketch_cursor_mode_ = mode;
    applySketchViewportMode();
    return true;
}

SketchEntityPointQueryResult
PartViewportController::querySketchEntityAt(
    viewer::ViewportPoint2 point) {
    if (viewport_ == nullptr ||
        activeSketch() == nullptr ||
        !point.valid()) {
        return {};
    }

    const auto queried =
        viewport_->querySketchPresentation(point);
    if (!queried.valid() ||
        !queried.completed) {
        return {};
    }

    if (!queried.token) {
        return {true, std::nullopt};
    }

    const auto address =
        sketchEntityFor(*queried.token);
    if (!address ||
        !sketch_edit_id_ ||
        address->sketch_id != *sketch_edit_id_) {
        return {};
    }

    return {true, address};
}

SketchEntityRectangleQueryResult
PartViewportController::querySketchEntities(
    const viewer::ViewportRect2& rectangle,
    viewer::SketchRectangleSelectionRule rule) {
    if (viewport_ == nullptr ||
        activeSketch() == nullptr ||
        !rectangle.valid()) {
        return {};
    }

    const auto queried =
        viewport_->querySketchPresentations(
            rectangle,
            rule);
    if (!queried.valid() ||
        !queried.completed) {
        return {};
    }

    SketchEntityRectangleQueryResult result;
    result.completed = true;
    result.hits.reserve(queried.tokens.size());

    for (const auto token : queried.tokens) {
        const auto address =
            sketchEntityFor(token);
        if (!address ||
            !sketch_edit_id_ ||
            address->sketch_id != *sketch_edit_id_) {
            return {};
        }

        const auto duplicate =
            std::find(
                result.hits.begin(),
                result.hits.end(),
                *address);
        if (duplicate != result.hits.end()) {
            return {};
        }

        result.hits.push_back(*address);
    }

    return result;
}

bool PartViewportController::setSketchSelectionBoxOverlay(
    const viewer::SketchSelectionBoxOverlay& overlay) {
    if (viewport_ == nullptr ||
        activeSketch() == nullptr ||
        !overlay.valid()) {
        return false;
    }

    return viewport_->setSketchSelectionBoxOverlay(
        overlay);
}

void PartViewportController::clearSketchSelectionBoxOverlay() {
    if (viewport_ != nullptr) {
        viewport_->clearSketchSelectionBoxOverlay();
    }
}

std::optional<SketchEntityAddress>
PartViewportController::sketchEntityFor(
    viewer::PresentationToken token) const {
    if (!token.valid()) {
        return std::nullopt;
    }

    const auto found =
        sketch_entity_bindings_.find(
            token.value);
    return found ==
            sketch_entity_bindings_.end()
        ? std::nullopt
        : std::optional<SketchEntityAddress>{
              found->second};
}

std::optional<viewer::PresentationToken>
PartViewportController::sketchPresentationFor(
    sketch::EntityId entity_id) const {
    if (!entity_id.valid() ||
        !sketch_edit_id_ ||
        activeSketch() == nullptr) {
        return std::nullopt;
    }

    for (const auto& [token_value, address] :
         sketch_entity_bindings_) {
        if (address.sketch_id == *sketch_edit_id_ &&
            address.entity_id == entity_id) {
            return viewer::PresentationToken{
                token_value};
        }
    }

    return std::nullopt;
}

bool PartViewportController::projectSketchEntitySelection(
    const std::vector<sketch::EntityId>& selected,
    std::optional<sketch::EntityId> primary) {
    if (viewport_ == nullptr ||
        activeSketch() == nullptr) {
        return false;
    }

    if (primary &&
        std::find(
            selected.begin(),
            selected.end(),
            *primary) == selected.end()) {
        return false;
    }

    viewer::PresentationSelection presentation;

    const auto* reference_selection =
        static_cast<const PartViewportController&>(
            *this)
            .activeSelection();

    if (reference_selection != nullptr) {
        presentation.selected.reserve(
            reference_selection->selected.size() +
            selected.size());

        for (const auto role :
             reference_selection->selected) {
            presentation.selected.push_back(
                tokenFor(role));
        }

        if (reference_selection->primary) {
            presentation.primary =
                tokenFor(
                    *reference_selection->primary);
        }
    } else {
        presentation.selected.reserve(
            selected.size());
    }

    std::vector<sketch::EntityId> unique;
    unique.reserve(selected.size());

    for (const auto id : selected) {
        if (!id.valid() ||
            std::find(
                unique.begin(),
                unique.end(),
                id) != unique.end()) {
            return false;
        }

        const auto token =
            sketchPresentationFor(id);
        if (!token) {
            return false;
        }

        unique.push_back(id);
        presentation.selected.push_back(*token);

        if (primary && id == *primary) {
            presentation.primary = *token;
        }
    }

    if (!presentation.valid()) {
        return false;
    }

    return viewport_->setPresentationSelection(
        presentation);
}

std::optional<core::BuiltinReferenceRole>
PartViewportController::primarySelection() const {
    const auto* selection = activeSelection();
    return selection == nullptr
        ? std::nullopt
        : selection->primary;
}

viewer::PresentationToken PartViewportController::tokenFor(
    core::BuiltinReferenceRole role) noexcept {
    return presentationTokenFor(role);
}

std::optional<core::BuiltinReferenceRole>
PartViewportController::roleFor(
    viewer::PresentationToken token) noexcept {
    if (!token.valid() ||
        token.value <= 0x100U ||
        token.value > 0x107U) {
        return std::nullopt;
    }

    const auto role =
        static_cast<core::BuiltinReferenceRole>(
            token.value - 0x101U);

    return core::isBuiltinReferenceRole(role)
        ? std::optional<core::BuiltinReferenceRole>{role}
        : std::nullopt;
}

const part::PartSketch*
PartViewportController::activeSketch() const noexcept {
    if (session_ == nullptr ||
        !sketch_edit_id_) {
        return nullptr;
    }

    return session_->document().findSketch(
        *sketch_edit_id_);
}

viewer::ReferenceScene
PartViewportController::buildReferenceScene() const {
    viewer::ReferenceScene scene;
    if (session_ == nullptr) return scene;

    if (const auto* hosted = activeSketch()) {
        const auto& placement =
            hosted->placement;
        scene.grid = viewer::GridPresentation{
            {
                placement.origin[0],
                placement.origin[1],
                placement.origin[2],
            },
            {
                placement.u_axis[0],
                placement.u_axis[1],
                placement.u_axis[2],
            },
            {
                placement.v_axis[0],
                placement.v_axis[1],
                placement.v_axis[2],
            },
            100.0,
            10.0,
            5U,
            true};
    } else {
        scene.grid = viewer::GridPresentation{
            {},
            xAxis,
            yAxis,
            100.0,
            10.0,
            5U,
            true};
    }

    for (const auto role : core::builtin_reference_roles) {
        scene.references.push_back(
            makeReference(
                role,
                session_->document()
                    .builtinReferenceVisible(role)));
    }

    return scene;
}

std::optional<viewer::SketchScene>
PartViewportController::buildSketchScene() {
    sketch_entity_bindings_.clear();

    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return viewer::SketchScene{};
    }

    const auto origin =
        detail::sketchPointToWorld(
            hosted->placement,
            sketch::Point2{0.0, 0.0});
    if (!origin) {
        return std::nullopt;
    }

    viewer::SketchScene scene;
    scene.origin =
        viewer::SketchOriginPresentation{
            *origin};

    const auto model_state =
        hosted->model.state();
    scene.lines.reserve(
        model_state.lines.size());

    for (const auto& line : model_state.lines) {
        const auto start =
            detail::sketchPointToWorld(
                hosted->placement,
                line.start);
        const auto end =
            detail::sketchPointToWorld(
                hosted->placement,
                line.end);
        const auto token =
            allocateSketchPresentationToken();

        if (!start || !end || !token) {
            sketch_entity_bindings_.clear();
            return std::nullopt;
        }

        scene.lines.push_back(
            viewer::SketchLinePresentation{
                *token,
                *start,
                *end});

        sketch_entity_bindings_.emplace(
            token->value,
            SketchEntityAddress{
                hosted->id,
                line.id});
    }

    if (!scene.valid()) {
        sketch_entity_bindings_.clear();
        return std::nullopt;
    }

    return scene;
}

std::optional<viewer::PresentationToken>
PartViewportController::allocateSketchPresentationToken()
    noexcept {
    if (next_sketch_presentation_token_ ==
        std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }

    return viewer::PresentationToken{
        next_sketch_presentation_token_++};
}

PartViewportController::SemanticSelection&
PartViewportController::activeSelection() {
    return selections_[
        std::string{session_->documentId().value()}];
}

const PartViewportController::SemanticSelection*
PartViewportController::activeSelection() const {
    if (session_ == nullptr) return nullptr;

    const auto found = selections_.find(
        std::string{session_->documentId().value()});
    return found == selections_.end()
        ? nullptr
        : &found->second;
}

void PartViewportController::onTreeSelection(
    const std::vector<core::BuiltinReferenceRole>& selected,
    std::optional<core::BuiltinReferenceRole> primary) {
    if (session_ == nullptr) return;

    auto& selection = activeSelection();
    selection.selected = selected;

    if (primary &&
        std::find(
            selected.begin(),
            selected.end(),
            *primary) != selected.end()) {
        selection.primary = primary;
    } else if (!selected.empty()) {
        selection.primary = selected.front();
    } else {
        selection.primary.reset();
    }

    applySelectionToSurfaces();
    notifySelectionChanged();
}

void PartViewportController::onViewportIntent(
    const viewer::SelectionIntent& intent) {
    if (session_ == nullptr ||
        !intent.valid()) {
        return;
    }

    auto& selection = activeSelection();

    if (intent.mode ==
        viewer::SelectionIntentMode::clear) {
        selection.selected.clear();
        selection.primary.reset();
        applySelectionToSurfaces();
        notifySelectionChanged();
        return;
    }

    const auto role = roleFor(intent.token);
    if (!role) {
        return;
    }

    if (intent.mode ==
        viewer::SelectionIntentMode::replace) {
        selection.selected = {*role};
        selection.primary = *role;
    } else {
        const auto found = std::find(
            selection.selected.begin(),
            selection.selected.end(),
            *role);

        if (found == selection.selected.end()) {
            selection.selected.push_back(*role);
            selection.primary = *role;
        } else {
            selection.selected.erase(found);
            if (selection.primary &&
                *selection.primary == *role) {
                if (selection.selected.empty()) {
                    selection.primary.reset();
                } else {
                    selection.primary =
                        selection.selected.back();
                }
            }
        }
    }

    applySelectionToSurfaces();
    notifySelectionChanged();
}

void PartViewportController::onSpatialPointer(
    const viewer::SpatialPointerEvent& event) {
    if (!event.valid() ||
        !sketch_pointer_handler_) {
        return;
    }

    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return;
    }

    const auto local =
        detail::sketchPointFromRay(
            hosted->placement,
            event.ray);
    if (!local) {
        return;
    }

    sketch_pointer_handler_(
        SketchPointerInput{
            hosted->id,
            event.phase,
            event.position,
            *local,
            event.modifiers.control});
}

void PartViewportController::applySelectionToSurfaces() {
    if (session_ == nullptr) {
        tree_->setBuiltinReferenceSelection(
            {},
            std::nullopt);
        if (viewport_ != nullptr) {
            static_cast<void>(
                viewport_->setPresentationSelection(
                    viewer::PresentationSelection{}));
        }
        return;
    }

    auto& selection = activeSelection();
    tree_->setBuiltinReferenceSelection(
        selection.selected,
        selection.primary);

    if (viewport_ == nullptr) return;

    viewer::PresentationSelection presentation;
    presentation.selected.reserve(
        selection.selected.size());

    for (const auto role : selection.selected) {
        presentation.selected.push_back(
            tokenFor(role));
    }

    if (selection.primary) {
        presentation.primary =
            tokenFor(*selection.primary);
    }

    static_cast<void>(
        viewport_->setPresentationSelection(
            presentation));
}

void PartViewportController::applySketchViewportMode() {
    if (viewport_ == nullptr) {
        return;
    }

    if (activeSketch() == nullptr) {
        viewport_->setPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                presentation_selection);
        viewport_->setCursorMode(
            viewer::ViewportCursorMode::
                system_default);
        return;
    }

    viewport_->setPrimaryPointerRouting(
        sketch_primary_pointer_routing_);

    viewport_->setCursorMode(
        sketch_cursor_mode_);
}

void PartViewportController::notifySelectionChanged() {
    if (!selection_changed_handler_) return;

    const auto* selection =
        static_cast<const PartViewportController&>(*this)
            .activeSelection();
    if (selection == nullptr) {
        selection_changed_handler_(
            {},
            std::nullopt);
        return;
    }

    selection_changed_handler_(
        selection->selected,
        selection->primary);
}

} // namespace simplesolid2::ui
