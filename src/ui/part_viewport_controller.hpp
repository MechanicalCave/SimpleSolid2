#pragma once

#include "part_document_tree_controller.hpp"

#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/viewer/document_viewport.hpp>

#include <QObject>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace simplesolid2::ui {

struct SketchEntityAddress final {
    sketch::SketchId sketch_id;
    sketch::EntityId entity_id;

    friend bool operator==(
        const SketchEntityAddress&,
        const SketchEntityAddress&) = default;
};

struct SketchPointerInput final {
    sketch::SketchId sketch_id;
    viewer::SpatialPointerPhase phase{
        viewer::SpatialPointerPhase::move};
    viewer::ViewportPoint2 viewport_position;
    sketch::Point2 position;
    bool control{};
};

struct SketchEntityPointQueryResult final {
    bool completed{};
    std::optional<SketchEntityAddress> hit;
};

struct SketchGripAddress final {
    sketch::SketchId sketch_id;
    sketch::LineGripRef grip;

    friend bool operator==(
        const SketchGripAddress&,
        const SketchGripAddress&) = default;
};

struct SketchGripPointQueryResult final {
    bool completed{};
    std::optional<SketchGripAddress> hit;
};

struct SketchEntityRectangleQueryResult final {
    bool completed{};
    std::vector<SketchEntityAddress> hits;
};

struct SketchPreviewLine2D final {
    sketch::Point2 start;
    sketch::Point2 end;

    [[nodiscard]] bool valid() const noexcept {
        return start.finite() &&
               end.finite() &&
               start != end;
    }
};

class PartViewportController final : public QObject {
public:
    using SelectionChangedHandler = std::function<void(
        const std::vector<core::BuiltinReferenceRole>&,
        std::optional<core::BuiltinReferenceRole>)>;

    using SketchPointerHandler =
        std::function<void(const SketchPointerInput&)>;

    PartViewportController(
        PartDocumentTreeController& tree,
        viewer::IDocumentViewport* viewport,
        QObject* parent = nullptr);

    void setDocumentSession(
        application::DocumentSession* session);

    void clear();
    void resetRuntimeState();
    void refreshPresentation();

    void setSketchEditSketch(
        std::optional<sketch::SketchId> sketch_id);

    [[nodiscard]] bool setSketchPreview(
        const std::vector<SketchPreviewLine2D>& lines);
    void clearSketchPreview();

    [[nodiscard]] bool setSketchPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing);

    [[nodiscard]] bool setSketchCursorMode(
        viewer::ViewportCursorMode mode);

    [[nodiscard]] SketchEntityPointQueryResult
    querySketchEntityAt(
        viewer::ViewportPoint2 point);

    [[nodiscard]] SketchGripPointQueryResult
    querySketchGripAt(
        viewer::ViewportPoint2 point);

    [[nodiscard]] SketchEntityRectangleQueryResult
    querySketchEntities(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule rule);

    [[nodiscard]] bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay);

    void clearSketchSelectionBoxOverlay();

    void setSketchPointerHandler(
        SketchPointerHandler handler) {
        sketch_pointer_handler_ =
            std::move(handler);
    }

    [[nodiscard]] std::optional<SketchEntityAddress>
    sketchEntityFor(
        viewer::PresentationToken token) const;

    [[nodiscard]] std::optional<viewer::PresentationToken>
    sketchPresentationFor(
        sketch::EntityId entity_id) const;

    [[nodiscard]] bool projectSketchEntitySelection(
        const std::vector<sketch::EntityId>& selected,
        std::optional<sketch::EntityId> primary);

    [[nodiscard]] bool projectSketchInteraction(
        const std::vector<sketch::EntityId>& selected,
        std::optional<sketch::EntityId> hovered_entity,
        std::optional<sketch::LineGripRef> hovered_grip,
        std::optional<sketch::LineGripRef> active_grip,
        bool grips_visible);

    void setSelectionChangedHandler(
        SelectionChangedHandler handler) {
        selection_changed_handler_ = std::move(handler);
    }

    [[nodiscard]] std::optional<core::BuiltinReferenceRole>
    primarySelection() const;

private:
    struct SemanticSelection final {
        std::vector<core::BuiltinReferenceRole> selected;
        std::optional<core::BuiltinReferenceRole> primary;
    };

    [[nodiscard]] static viewer::PresentationToken tokenFor(
        core::BuiltinReferenceRole role) noexcept;

    [[nodiscard]] static std::optional<core::BuiltinReferenceRole>
    roleFor(viewer::PresentationToken token) noexcept;

    [[nodiscard]] const part::PartSketch*
    activeSketch() const noexcept;

    [[nodiscard]] viewer::ReferenceScene
    buildReferenceScene() const;

    [[nodiscard]] std::optional<viewer::SketchScene>
    buildSketchScene();

    [[nodiscard]] std::optional<viewer::PresentationToken>
    allocateSketchPresentationToken() noexcept;

    [[nodiscard]] SemanticSelection& activeSelection();
    [[nodiscard]] const SemanticSelection* activeSelection() const;

    void onTreeSelection(
        const std::vector<core::BuiltinReferenceRole>& selected,
        std::optional<core::BuiltinReferenceRole> primary);

    void onViewportIntent(
        const viewer::SelectionIntent& intent);

    void onSpatialPointer(
        const viewer::SpatialPointerEvent& event);

    void applySelectionToSurfaces();
    void applySketchViewportMode();
    void notifySelectionChanged();

    PartDocumentTreeController* tree_{};
    viewer::IDocumentViewport* viewport_{};
    application::DocumentSession* session_{};
    std::optional<sketch::SketchId>
        sketch_edit_id_;
    viewer::PrimaryPointerRouting
        sketch_primary_pointer_routing_{
            viewer::PrimaryPointerRouting::
                presentation_selection};
    viewer::ViewportCursorMode
        sketch_cursor_mode_{
            viewer::ViewportCursorMode::
                select_pick_box};

    std::unordered_map<std::string, SemanticSelection>
        selections_;
    std::unordered_map<
        std::uint64_t,
        SketchEntityAddress>
        sketch_entity_bindings_;
    std::uint64_t next_sketch_presentation_token_{
        0x10000U};
    bool sketch_grip_projection_valid_{};
    bool projected_grips_visible_{};
    std::vector<sketch::EntityId>
        projected_grip_selection_;

    SelectionChangedHandler selection_changed_handler_;
    SketchPointerHandler sketch_pointer_handler_;
};

} // namespace simplesolid2::ui
