#include "part_viewport_controller.hpp"
#include "sketch_viewport_mapping.hpp"

#include <simplesolid2/part/feature_evaluation.hpp>

#include <QPointer>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <utility>

namespace simplesolid2::ui {
namespace {

constexpr viewer::Vec3 xAxis{1.0, 0.0, 0.0};
constexpr viewer::Vec3 yAxis{0.0, 1.0, 0.0};
constexpr viewer::Vec3 zAxis{0.0, 0.0, 1.0};

constexpr double axisExtent = 45.0;
constexpr double planeExtent = 35.0;
constexpr double pointExtent = 3.0;


[[nodiscard]] std::optional<viewer::SolidScene>
viewerSolidScene(
    const kernel::SolidPresentationMesh& mesh) {
    if (!mesh.valid()) {
        return std::nullopt;
    }

    viewer::SolidScene scene;
    scene.triangles.reserve(
        mesh.triangles.size());
    for (const auto& triangle :
         mesh.triangles) {
        scene.triangles.push_back(
            viewer::SolidTrianglePresentation{
                {triangle.first.x,
                 triangle.first.y,
                 triangle.first.z},
                {triangle.second.x,
                 triangle.second.y,
                 triangle.second.z},
                {triangle.third.x,
                 triangle.third.y,
                 triangle.third.z},
                {triangle.first_normal.x,
                 triangle.first_normal.y,
                 triangle.first_normal.z},
                {triangle.second_normal.x,
                 triangle.second_normal.y,
                 triangle.second_normal.z},
                {triangle.third_normal.x,
                 triangle.third_normal.y,
                 triangle.third_normal.z}});
    }
    return scene.valid()
        ? std::optional<viewer::SolidScene>{
              std::move(scene)}
        : std::nullopt;
}

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

[[nodiscard]] viewer::SketchGripRole
viewerGripRole(
    sketch::SketchGripRole role) noexcept {
    switch (role) {
    case sketch::SketchGripRole::line_start: return viewer::SketchGripRole::line_start;
    case sketch::SketchGripRole::line_center: return viewer::SketchGripRole::line_center;
    case sketch::SketchGripRole::line_end: return viewer::SketchGripRole::line_end;
    case sketch::SketchGripRole::circle_center: return viewer::SketchGripRole::circle_center;
    case sketch::SketchGripRole::circle_quadrant_pos_u: return viewer::SketchGripRole::circle_quadrant_pos_u;
    case sketch::SketchGripRole::circle_quadrant_pos_v: return viewer::SketchGripRole::circle_quadrant_pos_v;
    case sketch::SketchGripRole::circle_quadrant_neg_u: return viewer::SketchGripRole::circle_quadrant_neg_u;
    case sketch::SketchGripRole::circle_quadrant_neg_v: return viewer::SketchGripRole::circle_quadrant_neg_v;
    case sketch::SketchGripRole::arc_center: return viewer::SketchGripRole::arc_center;
    case sketch::SketchGripRole::arc_start: return viewer::SketchGripRole::arc_start;
    case sketch::SketchGripRole::arc_end: return viewer::SketchGripRole::arc_end;
    case sketch::SketchGripRole::arc_mid: return viewer::SketchGripRole::arc_mid;
    }
    return viewer::SketchGripRole::line_center;
}

[[nodiscard]] sketch::SketchGripRole
semanticGripRole(
    viewer::SketchGripRole role) noexcept {
    switch (role) {
    case viewer::SketchGripRole::line_start: return sketch::SketchGripRole::line_start;
    case viewer::SketchGripRole::line_center: return sketch::SketchGripRole::line_center;
    case viewer::SketchGripRole::line_end: return sketch::SketchGripRole::line_end;
    case viewer::SketchGripRole::circle_center: return sketch::SketchGripRole::circle_center;
    case viewer::SketchGripRole::circle_quadrant_pos_u: return sketch::SketchGripRole::circle_quadrant_pos_u;
    case viewer::SketchGripRole::circle_quadrant_pos_v: return sketch::SketchGripRole::circle_quadrant_pos_v;
    case viewer::SketchGripRole::circle_quadrant_neg_u: return sketch::SketchGripRole::circle_quadrant_neg_u;
    case viewer::SketchGripRole::circle_quadrant_neg_v: return sketch::SketchGripRole::circle_quadrant_neg_v;
    case viewer::SketchGripRole::arc_center: return sketch::SketchGripRole::arc_center;
    case viewer::SketchGripRole::arc_start: return sketch::SketchGripRole::arc_start;
    case viewer::SketchGripRole::arc_end: return sketch::SketchGripRole::arc_end;
    case viewer::SketchGripRole::arc_mid: return sketch::SketchGripRole::arc_mid;
    }
    return sketch::SketchGripRole::line_center;
}

[[nodiscard]] viewer::SketchInferenceGuideKind
viewerInferenceGuideKind(
    sketch::InferenceGuideKind kind) noexcept {
    switch (kind) {
    case sketch::InferenceGuideKind::sketch_u:
        return viewer::SketchInferenceGuideKind::sketch_u;
    case sketch::InferenceGuideKind::sketch_v:
        return viewer::SketchInferenceGuideKind::sketch_v;
    case sketch::InferenceGuideKind::additional_direction:
        return viewer::SketchInferenceGuideKind::
            additional_direction;
    }
    return viewer::SketchInferenceGuideKind::sketch_u;
}

[[nodiscard]] viewer::SketchSnapMarkerKind
viewerSnapMarkerKind(
    sketch::SnapKind kind) noexcept {
    switch (kind) {
    case sketch::SnapKind::endpoint:
        return viewer::SketchSnapMarkerKind::endpoint;
    case sketch::SnapKind::midpoint:
        return viewer::SketchSnapMarkerKind::midpoint;
    case sketch::SnapKind::center:
        return viewer::SketchSnapMarkerKind::center;
    case sketch::SnapKind::quadrant:
        return viewer::SketchSnapMarkerKind::quadrant;
    case sketch::SnapKind::intersection:
        return viewer::SketchSnapMarkerKind::intersection;
    case sketch::SnapKind::origin:
        return viewer::SketchSnapMarkerKind::origin;
    case sketch::SnapKind::perpendicular:
        return viewer::SketchSnapMarkerKind::perpendicular;
    case sketch::SnapKind::tangent:
        return viewer::SketchSnapMarkerKind::tangent;
    case sketch::SnapKind::nearest:
        return viewer::SketchSnapMarkerKind::nearest;
    }
    return viewer::SketchSnapMarkerKind::endpoint;
}

[[nodiscard]] std::string snapMarkerLabel(
    sketch::SnapKind kind) {
    switch (kind) {
    case sketch::SnapKind::endpoint: return "END";
    case sketch::SnapKind::midpoint: return "MID";
    case sketch::SnapKind::center: return "CEN";
    case sketch::SnapKind::quadrant: return "QUAD";
    case sketch::SnapKind::intersection: return "INT";
    case sketch::SnapKind::origin: return "ORG";
    case sketch::SnapKind::perpendicular: return "PER";
    case sketch::SnapKind::tangent: return "TAN";
    case sketch::SnapKind::nearest: return "NEA";
    }
    return {};
}

[[nodiscard]] viewer::SketchMeasureMarkerRole
viewerMeasureMarkerRole(
    sketch::MeasurePointRole role) noexcept {
    switch (role) {
    case sketch::MeasurePointRole::line_start: return viewer::SketchMeasureMarkerRole::line_start;
    case sketch::MeasurePointRole::line_midpoint: return viewer::SketchMeasureMarkerRole::line_midpoint;
    case sketch::MeasurePointRole::line_end: return viewer::SketchMeasureMarkerRole::line_end;
    case sketch::MeasurePointRole::circle_center: return viewer::SketchMeasureMarkerRole::circle_center;
    case sketch::MeasurePointRole::circle_quadrant_pos_u: return viewer::SketchMeasureMarkerRole::circle_quadrant_pos_u;
    case sketch::MeasurePointRole::circle_quadrant_pos_v: return viewer::SketchMeasureMarkerRole::circle_quadrant_pos_v;
    case sketch::MeasurePointRole::circle_quadrant_neg_u: return viewer::SketchMeasureMarkerRole::circle_quadrant_neg_u;
    case sketch::MeasurePointRole::circle_quadrant_neg_v: return viewer::SketchMeasureMarkerRole::circle_quadrant_neg_v;
    case sketch::MeasurePointRole::arc_center: return viewer::SketchMeasureMarkerRole::arc_center;
    case sketch::MeasurePointRole::arc_start: return viewer::SketchMeasureMarkerRole::arc_start;
    case sketch::MeasurePointRole::arc_end: return viewer::SketchMeasureMarkerRole::arc_end;
    case sketch::MeasurePointRole::arc_midpoint: return viewer::SketchMeasureMarkerRole::arc_midpoint;
    }
    return viewer::SketchMeasureMarkerRole::line_midpoint;
}

[[nodiscard]] sketch::MeasurePointRole
semanticMeasurePointRole(
    viewer::SketchMeasureMarkerRole role) noexcept {
    switch (role) {
    case viewer::SketchMeasureMarkerRole::line_start: return sketch::MeasurePointRole::line_start;
    case viewer::SketchMeasureMarkerRole::line_midpoint: return sketch::MeasurePointRole::line_midpoint;
    case viewer::SketchMeasureMarkerRole::line_end: return sketch::MeasurePointRole::line_end;
    case viewer::SketchMeasureMarkerRole::circle_center: return sketch::MeasurePointRole::circle_center;
    case viewer::SketchMeasureMarkerRole::circle_quadrant_pos_u: return sketch::MeasurePointRole::circle_quadrant_pos_u;
    case viewer::SketchMeasureMarkerRole::circle_quadrant_pos_v: return sketch::MeasurePointRole::circle_quadrant_pos_v;
    case viewer::SketchMeasureMarkerRole::circle_quadrant_neg_u: return sketch::MeasurePointRole::circle_quadrant_neg_u;
    case viewer::SketchMeasureMarkerRole::circle_quadrant_neg_v: return sketch::MeasurePointRole::circle_quadrant_neg_v;
    case viewer::SketchMeasureMarkerRole::arc_center: return sketch::MeasurePointRole::arc_center;
    case viewer::SketchMeasureMarkerRole::arc_start: return sketch::MeasurePointRole::arc_start;
    case viewer::SketchMeasureMarkerRole::arc_end: return sketch::MeasurePointRole::arc_end;
    case viewer::SketchMeasureMarkerRole::arc_midpoint: return sketch::MeasurePointRole::arc_midpoint;
    }
    return sketch::MeasurePointRole::line_midpoint;
}

struct CurveSegment2D final {
    sketch::Point2 start;
    sketch::Point2 end;
};

[[nodiscard]] std::vector<CurveSegment2D>
curveSegments(
    sketch::Point2 center,
    double radius,
    double start_angle,
    double sweep_angle) {
    constexpr std::size_t full_circle_segments = 96U;
    constexpr std::size_t minimum_arc_segments = 8U;
    constexpr double full_turn = 2.0 * std::numbers::pi_v<double>;

    if (!center.finite() || !std::isfinite(radius) || radius <= 0.0 ||
        !std::isfinite(start_angle) || !std::isfinite(sweep_angle) ||
        sweep_angle == 0.0 || std::abs(sweep_angle) > full_turn) {
        return {};
    }

    const auto proportional = static_cast<std::size_t>(
        std::ceil(std::abs(sweep_angle) / full_turn *
                  static_cast<double>(full_circle_segments)));
    const auto count = std::max(minimum_arc_segments, proportional);

    const auto point_at = [center, radius](double angle) {
        return sketch::Point2{
            center.u + radius * std::cos(angle),
            center.v + radius * std::sin(angle)};
    };

    std::vector<CurveSegment2D> result;
    result.reserve(count);
    auto previous = point_at(start_angle);
    for (std::size_t index = 1U; index <= count; ++index) {
        const double fraction =
            static_cast<double>(index) / static_cast<double>(count);
        const auto current =
            point_at(start_angle + sweep_angle * fraction);
        result.push_back({previous, current});
        previous = current;
    }
    return result;
}

} // namespace

[[nodiscard]] std::optional<sketch::Point2>
profilePointAt(
    const sketch::SketchModel& model,
    sketch::EntityId id,
    double parameter) {
    if (!std::isfinite(parameter)) return std::nullopt;

    if (const auto* line = model.findLine(id)) {
        const sketch::Point2 point{
            std::fma(parameter, line->end().u - line->start().u, line->start().u),
            std::fma(parameter, line->end().v - line->start().v, line->start().v)};
        return point.finite()
            ? std::optional<sketch::Point2>{point}
            : std::nullopt;
    }

    sketch::Point2 center;
    double radius{};
    double angle{};
    if (const auto* circle = model.findCircle(id)) {
        center = circle->center();
        radius = circle->radius();
        angle = 2.0 * std::numbers::pi_v<double> * parameter;
    } else if (const auto* arc = model.findArc(id)) {
        center = arc->center();
        radius = arc->radius();
        angle = arc->startAngle() + arc->sweepAngle() * parameter;
    } else {
        return std::nullopt;
    }

    const sketch::Point2 point{
        std::fma(radius, std::cos(angle), center.u),
        std::fma(radius, std::sin(angle), center.v)};
    return point.finite()
        ? std::optional<sketch::Point2>{point}
        : std::nullopt;
}

[[nodiscard]] std::optional<double>
profileUseDeltaAngle(
    const sketch::SketchModel& model,
    const sketch::RegionBoundaryUse2D& use) {
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;

    if (model.findCircle(use.source_entity)) {
        if (use.whole_closed_curve) {
            return use.follows_source_direction
                ? full_turn
                : -full_turn;
        }
        const double from =
            use.follows_source_direction
                ? use.start_parameter
                : use.end_parameter;
        const double to =
            use.follows_source_direction
                ? use.end_parameter
                : use.start_parameter;
        double delta =
            use.crosses_closed_seam
                ? (1.0 - from) + to
                : to - from;
        delta *= full_turn;
        if (!use.follows_source_direction) delta = -delta;
        return std::isfinite(delta)
            ? std::optional<double>{delta}
            : std::nullopt;
    }

    if (const auto* arc = model.findArc(use.source_entity)) {
        const double from =
            use.follows_source_direction
                ? use.start_parameter
                : use.end_parameter;
        const double to =
            use.follows_source_direction
                ? use.end_parameter
                : use.start_parameter;
        double delta = arc->sweepAngle() * (to - from);
        if (!use.follows_source_direction) delta = -delta;
        return std::isfinite(delta)
            ? std::optional<double>{delta}
            : std::nullopt;
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<std::vector<sketch::Point2>>
sampleProfileUse(
    const sketch::SketchModel& model,
    const sketch::RegionBoundaryUse2D& use) {
    if (model.findLine(use.source_entity)) {
        const auto first = profilePointAt(
            model, use.source_entity, use.start_parameter);
        const auto second = profilePointAt(
            model, use.source_entity, use.end_parameter);
        if (!first || !second) return std::nullopt;
        return std::vector<sketch::Point2>{*first, *second};
    }

    const auto delta = profileUseDeltaAngle(model, use);
    if (!delta) return std::nullopt;

    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    constexpr std::size_t full_segments = 96U;
    const auto proportional =
        static_cast<std::size_t>(
            std::ceil(
                std::abs(*delta) / full_turn *
                static_cast<double>(full_segments)));
    const auto count =
        std::max<std::size_t>(4U, proportional);

    sketch::Point2 center;
    double radius{};
    double start_angle{};
    if (const auto* circle = model.findCircle(use.source_entity)) {
        center = circle->center();
        radius = circle->radius();
        start_angle =
            full_turn * use.start_parameter;
    } else if (const auto* arc = model.findArc(use.source_entity)) {
        center = arc->center();
        radius = arc->radius();
        start_angle =
            arc->startAngle() +
            arc->sweepAngle() * use.start_parameter;
    } else {
        return std::nullopt;
    }

    std::vector<sketch::Point2> points;
    points.reserve(count + 1U);
    for (std::size_t index = 0U; index <= count; ++index) {
        const double fraction =
            static_cast<double>(index) /
            static_cast<double>(count);
        const double angle =
            start_angle + *delta * fraction;
        const sketch::Point2 point{
            std::fma(radius, std::cos(angle), center.u),
            std::fma(radius, std::sin(angle), center.v)};
        if (!point.finite()) return std::nullopt;
        points.push_back(point);
    }

    if (use.whole_closed_curve && points.size() > 1U) {
        points.pop_back();
    }
    return points;
}

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

void PartViewportController::setSolidModelingKernel(
    kernel::ISolidModelingKernel* modeling_kernel) {
    if (solid_modeling_kernel_ ==
        modeling_kernel) {
        return;
    }
    solid_modeling_kernel_ =
        modeling_kernel;
    body_scene_revision_.reset();
    body_scene_cache_.reset();
    body_topology_catalog_cache_.reset();
    body_topology_bindings_.clear();
    clearSolidPreview();
    refreshPresentation();
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
        profile_bindings_.clear();
        transient_profile_reveal_.reset();
        transient_profile_hide_.reset();
        body_scene_revision_.reset();
        body_scene_cache_.reset();
        body_topology_catalog_cache_.reset();
        body_topology_bindings_.clear();
        clearSolidPreview();
        clearSketchPreview();
        clearProfileDraftPreview();
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
    profile_bindings_.clear();
    transient_profile_reveal_.reset();
    transient_profile_hide_.reset();
    body_scene_revision_.reset();
    body_scene_cache_.reset();
    body_topology_catalog_cache_.reset();
    body_topology_bindings_.clear();
    clearSketchSelectionBoxOverlay();
    tree_->clear();

    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setSolidPreviewScene(
                viewer::SolidPreviewScene{}));
        static_cast<void>(
            viewport_->setBodyScene(
                viewer::BodyScene{}));
        static_cast<void>(
            viewport_->setReferenceScene(
                viewer::ReferenceScene{}));
        static_cast<void>(
            viewport_->setSketchScene(
                viewer::SketchScene{}));
        static_cast<void>(
            viewport_->setProfileScene(
                viewer::ProfileScene{}));
        static_cast<void>(
            viewport_->setProfilePreviewScene(
                viewer::ProfilePreviewScene{}));
        static_cast<void>(
            viewport_->setSketchPreviewScene(
                viewer::SketchPreviewScene{}));
        static_cast<void>(
            viewport_->setPresentationSelection(
                viewer::PresentationSelection{}));
    }

    setPresentationDegraded(false);
    applySketchViewportMode();
    notifySelectionChanged();
}

void PartViewportController::resetRuntimeState() {
    selections_.clear();
    clear();
}

void PartViewportController::refreshDocumentTree() {
    if (tree_ != nullptr) {
        tree_->setDocumentSession(session_);
    }
}

void PartViewportController::refreshPresentation() {
    if (viewport_ == nullptr) return;

    sketch_grip_projection_valid_ = false;
    projected_grip_selection_.clear();

    if (session_ == nullptr) {
        sketch_entity_bindings_.clear();
        profile_bindings_.clear();
        body_scene_revision_.reset();
        body_scene_cache_.reset();
        body_topology_catalog_cache_.reset();
        body_topology_bindings_.clear();
        clearSolidPreview();
        clearSketchPreview();
        clearProfileDraftPreview();
        clearSketchSelectionBoxOverlay();
        applySketchViewportMode();
        static_cast<void>(
            viewport_->setBodyScene(
                viewer::BodyScene{}));
        static_cast<void>(
            viewport_->setReferenceScene(
                viewer::ReferenceScene{}));
        static_cast<void>(
            viewport_->setSketchScene(
                viewer::SketchScene{}));
        static_cast<void>(
            viewport_->setProfileScene(
                viewer::ProfileScene{}));
        static_cast<void>(
            viewport_->setProfilePreviewScene(
                viewer::ProfilePreviewScene{}));
        setPresentationDegraded(false);
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

    const auto solid_scene =
        buildBodyScene();
    bool solid_ok = false;
    if (solid_scene) {
        solid_ok =
            viewport_->setBodyScene(
                *solid_scene);
    } else {
        // Presentation generation failed. Clear any previously published
        // Body before reporting degradation; stale solid geometry is never
        // allowed to remain current truth.
        static_cast<void>(
            viewport_->setBodyScene(
                viewer::BodyScene{}));
    }

    const bool reference_ok =
        viewport_->setReferenceScene(
            buildReferenceScene());

    const auto profile_scene =
        buildProfileScene();
    const bool profile_ok =
        profile_scene.has_value() &&
        viewport_->setProfileScene(*profile_scene);

    const auto sketch_scene =
        buildSketchScene();
    const bool sketch_ok =
        sketch_scene.has_value() &&
        viewport_->setSketchScene(*sketch_scene);

    setPresentationDegraded(
        !solid_ok ||
        !reference_ok ||
        !profile_ok ||
        !sketch_ok);

    applySelectionToSurfaces();
}

void PartViewportController::setPresentationDegraded(
    bool degraded) {
    if (presentation_degraded_ == degraded) {
        return;
    }

    presentation_degraded_ = degraded;
    if (presentation_state_changed_handler_) {
        presentation_state_changed_handler_(degraded);
    }
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
    clearProfileDraftPreview();
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
                *end,
                line.construction});
    }

    if (!scene.valid()) {
        return false;
    }

    return viewport_->setSketchPreviewScene(
        scene);
}

bool PartViewportController::setSketchCirclePreview(
    const sketch::CircleIntent& circle) {
    if (!circle.valid()) return false;
    const auto segments = curveSegments(
        circle.center, circle.radius, 0.0,
        2.0 * std::numbers::pi_v<double>);
    std::vector<SketchPreviewLine2D> lines;
    lines.reserve(segments.size());
    for (const auto& segment : segments) {
        lines.push_back({segment.start, segment.end});
    }
    return !lines.empty() && setSketchPreview(lines);
}

bool PartViewportController::setSketchArcPreview(
    const sketch::ArcIntent& arc) {
    if (!arc.valid()) return false;
    const auto segments = curveSegments(
        arc.center, arc.radius,
        arc.start_angle, arc.sweep_angle);
    std::vector<SketchPreviewLine2D> lines;
    lines.reserve(segments.size());
    for (const auto& segment : segments) {
        lines.push_back({segment.start, segment.end});
    }
    return !lines.empty() && setSketchPreview(lines);
}

bool PartViewportController::setSketchGeometryPreview(
    const sketch::DirectManipulationGeometry& geometry) {
    if (geometry.empty()) return false;

    std::vector<SketchPreviewLine2D> lines;
    lines.reserve(
        geometry.lines.size() +
        geometry.circles.size() * 96U +
        geometry.arcs.size() * 48U);

    for (const auto& line : geometry.lines) {
        lines.push_back(
            {
                line.start,
                line.end,
                line.role ==
                    sketch::EntityRole::construction});
    }

    for (const auto& circle : geometry.circles) {
        const auto segments = curveSegments(
            circle.center,
            circle.radius,
            0.0,
            2.0 * std::numbers::pi_v<double>);
        if (segments.empty()) return false;
        for (const auto& segment : segments) {
            lines.push_back(
                {
                    segment.start,
                    segment.end,
                    circle.role ==
                        sketch::EntityRole::construction});
        }
    }

    for (const auto& arc : geometry.arcs) {
        const auto segments = curveSegments(
            arc.center,
            arc.radius,
            arc.start_angle,
            arc.sweep_angle);
        if (segments.empty()) return false;
        for (const auto& segment : segments) {
            lines.push_back(
                {
                    segment.start,
                    segment.end,
                    arc.role ==
                        sketch::EntityRole::construction});
        }
    }

    return setSketchPreview(lines);
}


bool PartViewportController::setSolidPreview(
    kernel::RuntimeSolidHandle solid,
    viewer::SolidPreviewTone tone) {
    if (solid_modeling_kernel_ == nullptr ||
        solid == nullptr) {
        return false;
    }

    const auto mesh =
        solid_modeling_kernel_->
            presentationMesh(
                std::move(solid));
    if (!mesh.ok()) {
        if (viewport_ != nullptr) {
            static_cast<void>(
                viewport_->setSolidPreviewScene(
                    viewer::SolidPreviewScene{}));
        }
        return false;
    }
    return setSolidPreview(
        mesh.mesh,
        tone);
}

bool PartViewportController::setSolidPreview(
    const kernel::SolidPresentationMesh& mesh,
    viewer::SolidPreviewTone tone) {
    if (viewport_ == nullptr ||
        !mesh.valid()) {
        return false;
    }

    const auto scene =
        viewerSolidScene(mesh);
    if (!scene) {
        static_cast<void>(
            viewport_->setSolidPreviewScene(
                viewer::SolidPreviewScene{}));
        return false;
    }

    viewer::SolidPreviewScene preview;
    preview.triangles =
        scene->triangles;
    preview.tone = tone;
    return viewport_->setSolidPreviewScene(
        preview);
}

void PartViewportController::clearSolidPreview() {
    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setSolidPreviewScene(
                viewer::SolidPreviewScene{}));
    }
}

void PartViewportController::clearSketchPreview() {
    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setSketchPreviewScene(
                viewer::SketchPreviewScene{}));
    }
}

bool PartViewportController::setProfileDraftPreview(
    const std::optional<sketch::RegionCandidate2D>& region,
    bool show_boundary,
    viewer::ProfilePreviewTone tone,
    const std::optional<sketch::RegionCandidate2D>&
        emphasis_region) {
    if (viewport_ == nullptr) return false;
    if (!region && !emphasis_region) {
        return viewport_->setProfilePreviewScene(
            viewer::ProfilePreviewScene{});
    }

    const auto* hosted = activeSketch();
    if (hosted == nullptr) return false;

    viewer::ProfilePreviewScene scene;
    scene.show_boundary = show_boundary;
    scene.tone = tone;

    if (region) {
        scene.region =
            buildProfileRegionPresentation(
                *hosted,
                *region);
        if (!scene.region) return false;
    }

    if (emphasis_region) {
        scene.emphasis_region =
            buildProfileRegionPresentation(
                *hosted,
                *emphasis_region);
        if (!scene.emphasis_region) return false;
    }

    return viewport_->setProfilePreviewScene(scene);
}

void PartViewportController::clearProfileDraftPreview() {
    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setProfilePreviewScene(
                viewer::ProfilePreviewScene{}));
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

std::optional<viewer::ViewportPoint2>
PartViewportController::projectSketchPointToViewport(
    sketch::Point2 point) const {
    if (viewport_ == nullptr ||
        !point.finite()) {
        return std::nullopt;
    }

    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return std::nullopt;
    }

    const auto world =
        detail::sketchPointToWorld(
            hosted->placement,
            point);
    if (!world) {
        return std::nullopt;
    }

    const auto projected =
        viewport_->projectWorldPoint(*world);
    return projected &&
                   projected->valid()
        ? projected
        : std::nullopt;
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

SketchGripPointQueryResult
PartViewportController::querySketchGripAt(
    viewer::ViewportPoint2 point) {
    if (viewport_ == nullptr ||
        activeSketch() == nullptr ||
        !point.valid()) {
        return {};
    }

    const auto queried =
        viewport_->querySketchGrip(point);
    if (!queried.valid() ||
        !queried.completed) {
        return {};
    }

    if (!queried.grip) {
        return {true, std::nullopt};
    }

    const auto owner =
        sketchEntityFor(
            queried.grip->owner);
    if (!owner ||
        !sketch_edit_id_ ||
        owner->sketch_id != *sketch_edit_id_) {
        return {};
    }

    return {
        true,
        SketchGripAddress{
            owner->sketch_id,
            sketch::LineGripRef{
                owner->entity_id,
                semanticGripRole(
                    queried.grip->role)}}};
}

SketchMeasureMarkerPointQueryResult
PartViewportController::querySketchMeasureMarkersAt(
    viewer::ViewportPoint2 point) {
    if (viewport_ == nullptr ||
        activeSketch() == nullptr ||
        !point.valid()) {
        return {};
    }

    const auto queried =
        viewport_->querySketchMeasureMarkers(point);
    if (!queried.valid() ||
        !queried.completed) {
        return {};
    }

    SketchMeasureMarkerPointQueryResult result;
    result.completed = true;
    result.hits.reserve(queried.markers.size());

    const auto* hosted = activeSketch();
    if (hosted == nullptr || !sketch_edit_id_) {
        return {};
    }

    for (const auto& marker : queried.markers) {
        const auto owner =
            sketchEntityFor(marker.owner);
        if (!owner ||
            owner->sketch_id != *sketch_edit_id_) {
            return {};
        }

        const sketch::MeasurePointRef ref{
            owner->entity_id,
            semanticMeasurePointRole(marker.role)};
        if (!sketch::resolveMeasurePoint(
                hosted->model,
                ref)) {
            return {};
        }

        result.hits.push_back(
            SketchMeasureMarkerAddress{
                owner->sketch_id,
                ref});
    }

    return result;
}

bool PartViewportController::projectSketchMeasurePresentation(
    const std::vector<sketch::ResolvedMeasurePoint>& catalog,
    const std::vector<sketch::MeasurePointRef>& selected,
    const std::optional<sketch::RelationalMeasurementCue>& cue) {
    if (viewport_ == nullptr) {
        return false;
    }
    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return false;
    }

    viewer::SketchMeasureMarkerScene marker_scene;
    marker_scene.markers.reserve(catalog.size());
    for (const auto& point : catalog) {
        const auto token =
            sketchPresentationFor(
                point.ref.entity_id);
        const auto world =
            detail::sketchPointToWorld(
                hosted->placement,
                point.point);
        if (!token || !world) {
            return false;
        }
        marker_scene.markers.push_back(
            viewer::SketchMeasureMarkerPresentation{
                {
                    *token,
                    viewerMeasureMarkerRole(
                        point.ref.role)},
                *world});
    }

    marker_scene.selected.reserve(selected.size());
    for (const auto& ref : selected) {
        const auto token =
            sketchPresentationFor(ref.entity_id);
        if (!token) {
            return false;
        }
        marker_scene.selected.push_back(
            viewer::SketchMeasureMarkerKey{
                *token,
                viewerMeasureMarkerRole(ref.role)});
    }

    if (!marker_scene.valid()) {
        return false;
    }

    viewer::SketchMeasureCueScene cue_scene;
    if (cue) {
        if (!cue->valid()) {
            return false;
        }

        cue_scene.highlighted_entities.reserve(
            cue->highlighted_lines.size());
        for (const auto id :
             cue->highlighted_lines) {
            const auto token =
                sketchPresentationFor(id);
            if (!token) {
                return false;
            }
            cue_scene.highlighted_entities.push_back(
                *token);
        }

        cue_scene.segments.reserve(
            cue->segments.size());
        for (const auto& segment :
             cue->segments) {
            const auto start =
                detail::sketchPointToWorld(
                    hosted->placement,
                    segment.start);
            const auto end =
                detail::sketchPointToWorld(
                    hosted->placement,
                    segment.end);
            if (!start || !end) {
                return false;
            }
            cue_scene.segments.push_back(
                viewer::SketchMeasureCueSegment{
                    *start,
                    *end,
                    segment.kind ==
                            sketch::MeasureCueSegmentKind::
                                supporting_line_continuation
                        ? viewer::SketchMeasureCueSegmentKind::
                              supporting_line_continuation
                        : viewer::SketchMeasureCueSegmentKind::
                              relation});
        }

        if (cue->cue_point) {
            const auto world =
                detail::sketchPointToWorld(
                    hosted->placement,
                    *cue->cue_point);
            if (!world) {
                return false;
            }
            cue_scene.cue_point = *world;
        }
    }

    if (!cue_scene.valid()) {
        return false;
    }

    if (!viewport_->setSketchMeasureMarkerScene(
            marker_scene)) {
        return false;
    }
    if (!viewport_->setSketchMeasureCueScene(
            cue_scene)) {
        static_cast<void>(
            viewport_->setSketchMeasureMarkerScene(
                viewer::SketchMeasureMarkerScene{}));
        return false;
    }
    return true;
}

void PartViewportController::clearSketchMeasurePresentation() {
    if (viewport_ == nullptr) {
        return;
    }
    static_cast<void>(
        viewport_->setSketchMeasureMarkerScene(
            viewer::SketchMeasureMarkerScene{}));
    static_cast<void>(
        viewport_->setSketchMeasureCueScene(
            viewer::SketchMeasureCueScene{}));
}

bool PartViewportController::
projectSketchSnapInferencePresentation(
    const std::optional<sketch::SnapCandidate>&
        current,
    const sketch::TrackingAnchorState& anchors,
    bool show_anchors,
    const std::vector<sketch::InferenceGuide>&
        active_guides,
    std::optional<sketch::Point2>
        inference_point,
    bool guide_intersection,
    std::optional<sketch::LineExtensionRay>
        extension_ray,
    std::optional<sketch::Point2>
        extension_point) {
    if (viewport_ == nullptr) {
        return false;
    }
    const auto* hosted = activeSketch();
    if (hosted == nullptr ||
        !anchors.valid() ||
        active_guides.size() > 2U ||
        (inference_point &&
         !inference_point->finite()) ||
        (extension_ray &&
         !extension_ray->valid()) ||
        (extension_point &&
         (!extension_ray ||
          !extension_point->finite()))) {
        return false;
    }

    viewer::SketchSnapInferenceScene scene;
    const auto make_marker =
        [hosted](
            const sketch::SnapCandidate& candidate)
            -> std::optional<
                viewer::SketchSnapMarkerPresentation> {
            if (!candidate.valid()) {
                return std::nullopt;
            }
            const auto world =
                detail::sketchPointToWorld(
                    hosted->placement,
                    candidate.point);
            const auto label =
                snapMarkerLabel(candidate.kind);
            if (!world || label.empty()) {
                return std::nullopt;
            }
            viewer::SketchSnapMarkerPresentation
                marker{
                    *world,
                    viewerSnapMarkerKind(
                        candidate.kind),
                    label};
            return marker.valid()
                ? std::optional<
                      viewer::
                          SketchSnapMarkerPresentation>{
                      std::move(marker)}
                : std::nullopt;
        };

    if (current) {
        scene.current = make_marker(*current);
        if (!scene.current) {
            return false;
        }
    } else if (inference_point) {
        const auto world =
            detail::sketchPointToWorld(
                hosted->placement,
                *inference_point);
        if (!world) {
            return false;
        }
        scene.current =
            viewer::SketchSnapMarkerPresentation{
                *world,
                guide_intersection
                    ? viewer::SketchSnapMarkerKind::
                          guide_intersection
                    : viewer::SketchSnapMarkerKind::
                          guide_projection,
                guide_intersection
                    ? "TRACK INT"
                    : "TRACK"};
    }

    if (extension_ray) {
        const auto origin_world =
            detail::sketchPointToWorld(
                hosted->placement,
                extension_ray->origin);
        const sketch::Point2 direction_point{
            extension_ray->origin.u +
                extension_ray->direction.u,
            extension_ray->origin.v +
                extension_ray->direction.v};
        const auto direction_world_point =
            detail::sketchPointToWorld(
                hosted->placement,
                direction_point);
        if (!origin_world ||
            !direction_world_point) {
            return false;
        }

        std::optional<viewer::Point3>
            resolved_world;
        if (extension_point) {
            resolved_world =
                detail::sketchPointToWorld(
                    hosted->placement,
                    *extension_point);
            if (!resolved_world) {
                return false;
            }
        }

        scene.extension_guide =
            viewer::SketchExtensionGuidePresentation{
                *origin_world,
                {
                    direction_world_point->x -
                        origin_world->x,
                    direction_world_point->y -
                        origin_world->y,
                    direction_world_point->z -
                        origin_world->z,
                },
                resolved_world};
        if (!scene.extension_guide->valid()) {
            return false;
        }

        if (!scene.current) {
            scene.current =
                viewer::SketchSnapMarkerPresentation{
                    resolved_world
                        ? *resolved_world
                        : *origin_world,
                    viewer::SketchSnapMarkerKind::
                        extension,
                    "EXT"};
        }
    }

    if (show_anchors) {
        scene.acquired.reserve(
            anchors.anchors.size());
        for (const auto& anchor :
             anchors.anchors) {
            const auto marker =
                make_marker(anchor.snap);
            if (!marker) {
                return false;
            }
            scene.acquired.push_back(*marker);
        }
    }

    scene.guides.reserve(active_guides.size());
    for (const auto& guide :
         active_guides) {
        if (!guide.valid()) {
            return false;
        }

        const auto anchor_world =
            detail::sketchPointToWorld(
                hosted->placement,
                guide.anchor);
        const sketch::Point2 direction_point{
            guide.anchor.u + guide.direction.u,
            guide.anchor.v + guide.direction.v};
        const auto direction_world_point =
            detail::sketchPointToWorld(
                hosted->placement,
                direction_point);
        if (!anchor_world ||
            !direction_world_point) {
            return false;
        }

        viewer::SketchInferenceGuidePresentation
            presentation{
                *anchor_world,
                {
                    direction_world_point->x -
                        anchor_world->x,
                    direction_world_point->y -
                        anchor_world->y,
                    direction_world_point->z -
                        anchor_world->z,
                },
                viewerInferenceGuideKind(
                    guide.kind)};
        if (!presentation.valid()) {
            return false;
        }
        scene.guides.push_back(
            std::move(presentation));
    }

    return scene.valid() &&
           viewport_->
               setSketchSnapInferenceScene(scene);
}

void PartViewportController::
clearSketchSnapInferencePresentation() {
    if (viewport_ != nullptr) {
        static_cast<void>(
            viewport_->setSketchSnapInferenceScene(
                viewer::SketchSnapInferenceScene{}));
    }
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

bool PartViewportController::setSketchDynamicInputOverlay(
    const viewer::SketchDynamicInputOverlay& overlay) {
    return viewport_ != nullptr &&
           overlay.valid() &&
           viewport_->setSketchDynamicInputOverlay(overlay);
}

void PartViewportController::clearSketchDynamicInputOverlay() {
    if (viewport_ != nullptr) {
        viewport_->clearSketchDynamicInputOverlay();
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

std::optional<part::ProfileId>
PartViewportController::profileFor(
    viewer::PresentationToken token) const {
    if (!token.valid()) return std::nullopt;
    const auto found =
        profile_bindings_.find(token.value);
    return found == profile_bindings_.end()
        ? std::nullopt
        : std::optional<part::ProfileId>{
              found->second};
}

std::optional<viewer::PresentationToken>
PartViewportController::profilePresentationFor(
    part::ProfileId profile_id) const {
    if (!profile_id.valid()) return std::nullopt;
    for (const auto& [token, id] :
         profile_bindings_) {
        if (id == profile_id) {
            return viewer::PresentationToken{
                token};
        }
    }
    return std::nullopt;
}

void PartViewportController::setProfileSelectionFromTree(
    const std::vector<part::ProfileId>& selected,
    std::optional<part::ProfileId> primary) {
    if (session_ == nullptr) return;

    auto& selection = activeSelection();
    selection.profiles = selected;
    if (primary &&
        std::find(
            selected.begin(),
            selected.end(),
            *primary) != selected.end()) {
        selection.primary_profile = primary;
        selection.primary.reset();
    } else if (!selected.empty()) {
        selection.primary_profile =
            selected.front();
        selection.primary.reset();
    } else {
        selection.primary_profile.reset();
    }

    applySelectionToSurfaces();
    notifySelectionChanged();
}


void PartViewportController::setTransientProfileReveal(
    std::optional<part::ProfileId> profile_id) {
    setTransientProfilePresentationOverride(
        profile_id,
        transient_profile_hide_);
}

void PartViewportController::
setTransientProfilePresentationOverride(
    std::optional<part::ProfileId> reveal_profile_id,
    std::optional<part::ProfileId> hide_profile_id) {
    const auto sanitize =
        [this](
            std::optional<part::ProfileId> id) {
            if (id &&
                (session_ == nullptr ||
                 session_->document().findProfile(
                     *id) == nullptr)) {
                id.reset();
            }
            return id;
        };

    reveal_profile_id =
        sanitize(reveal_profile_id);
    hide_profile_id =
        sanitize(hide_profile_id);

    if (transient_profile_reveal_ ==
            reveal_profile_id &&
        transient_profile_hide_ ==
            hide_profile_id) {
        return;
    }

    transient_profile_reveal_ =
        reveal_profile_id;
    transient_profile_hide_ =
        hide_profile_id;
    refreshPresentation();
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
            reference_selection->profiles.size() +
            selected.size());

        for (const auto role :
             reference_selection->selected) {
            presentation.selected.push_back(
                tokenFor(role));
        }

        for (const auto profile_id :
             reference_selection->profiles) {
            const auto token =
                profilePresentationFor(profile_id);
            if (token) {
                presentation.selected.push_back(*token);
            }
        }

        if (reference_selection->primary_profile) {
            presentation.primary =
                profilePresentationFor(
                    *reference_selection->primary_profile);
        } else if (reference_selection->primary) {
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

bool PartViewportController::projectSketchInteraction(
    const std::vector<sketch::EntityId>& selected,
    std::optional<sketch::EntityId> hovered_entity,
    std::optional<sketch::LineGripRef> hovered_grip,
    std::optional<sketch::LineGripRef> active_grip,
    bool grips_visible) {
    if (viewport_ == nullptr) {
        return false;
    }

    const auto* hosted = activeSketch();
    if (hosted == nullptr) {
        return false;
    }

    if (!sketch_grip_projection_valid_ ||
        projected_grips_visible_ != grips_visible ||
        projected_grip_selection_ != selected) {
        viewer::SketchGripScene grip_scene;
        if (grips_visible) {
            grip_scene.grips.reserve(selected.size() * 5U);
            const auto push_grip =
                [&grip_scene, hosted](
                    viewer::PresentationToken token,
                    viewer::SketchGripRole role,
                    sketch::Point2 point) {
                    const auto world = detail::sketchPointToWorld(
                        hosted->placement, point);
                    if (!world) return false;
                    grip_scene.grips.push_back(
                        viewer::SketchGripPresentation{
                            {token, role}, *world});
                    return true;
                };

            for (const auto id : selected) {
                const auto token = sketchPresentationFor(id);
                if (!token) return false;

                if (const auto* line = hosted->model.findLine(id)) {
                    const sketch::Point2 center{
                        (line->start().u + line->end().u) * 0.5,
                        (line->start().v + line->end().v) * 0.5};
                    if (!push_grip(*token, viewer::SketchGripRole::line_start, line->start()) ||
                        !push_grip(*token, viewer::SketchGripRole::line_center, center) ||
                        !push_grip(*token, viewer::SketchGripRole::line_end, line->end())) {
                        return false;
                    }
                    continue;
                }

                if (const auto* circle = hosted->model.findCircle(id)) {
                    const auto c = circle->center();
                    const auto r = circle->radius();
                    if (!push_grip(*token, viewer::SketchGripRole::circle_center, c) ||
                        !push_grip(*token, viewer::SketchGripRole::circle_quadrant_pos_u, {c.u + r, c.v}) ||
                        !push_grip(*token, viewer::SketchGripRole::circle_quadrant_pos_v, {c.u, c.v + r}) ||
                        !push_grip(*token, viewer::SketchGripRole::circle_quadrant_neg_u, {c.u - r, c.v}) ||
                        !push_grip(*token, viewer::SketchGripRole::circle_quadrant_neg_v, {c.u, c.v - r})) {
                        return false;
                    }
                    continue;
                }

                if (const auto* arc = hosted->model.findArc(id)) {
                    const auto c = arc->center();
                    const auto at = [arc, c](double angle) {
                        return sketch::Point2{
                            c.u + arc->radius() * std::cos(angle),
                            c.v + arc->radius() * std::sin(angle)};
                    };
                    const auto start = arc->startAngle();
                    const auto end = start + arc->sweepAngle();
                    const auto mid = start + arc->sweepAngle() * 0.5;
                    if (!push_grip(*token, viewer::SketchGripRole::arc_center, c) ||
                        !push_grip(*token, viewer::SketchGripRole::arc_start, at(start)) ||
                        !push_grip(*token, viewer::SketchGripRole::arc_end, at(end)) ||
                        !push_grip(*token, viewer::SketchGripRole::arc_mid, at(mid))) {
                        return false;
                    }
                    continue;
                }
                return false;
            }
        }

        if (!viewport_->setSketchGripScene(grip_scene)) {
            sketch_grip_projection_valid_ = false;
            return false;
        }
        projected_grips_visible_ = grips_visible;
        projected_grip_selection_ = selected;
        sketch_grip_projection_valid_ = true;
    }

    viewer::SketchInteractionPresentation
        presentation;

    if (hovered_entity) {
        const auto token =
            sketchPresentationFor(
                *hovered_entity);
        if (!token) {
            return false;
        }
        presentation.hovered_entity =
            *token;
    }

    const auto map_grip =
        [this](
            const sketch::SketchGripRef& grip)
            -> std::optional<
                viewer::SketchGripKey> {
            const auto token =
                sketchPresentationFor(
                    grip.entity_id);
            if (!token) {
                return std::nullopt;
            }
            return viewer::SketchGripKey{
                *token,
                viewerGripRole(grip.role)};
        };

    if (hovered_grip) {
        const auto mapped =
            map_grip(*hovered_grip);
        if (!mapped) {
            return false;
        }
        presentation.hovered_grip =
            *mapped;
    }

    if (active_grip) {
        const auto mapped =
            map_grip(*active_grip);
        if (!mapped) {
            return false;
        }
        presentation.active_grip =
            *mapped;
    }

    if (!presentation.valid()) {
        return false;
    }

    return viewport_->
        setSketchInteractionPresentation(
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

std::optional<viewer::SolidScene>
PartViewportController::buildBodyScene() {
    viewer::SolidScene empty_scene;
    if (session_ == nullptr ||
        solid_modeling_kernel_ == nullptr) {
        body_scene_revision_.reset();
        body_scene_cache_.reset();
        body_topology_catalog_cache_.reset();
        body_topology_bindings_.clear();
        return empty_scene;
    }

    const auto revision =
        session_->document().revision();
    if (body_scene_revision_ &&
        *body_scene_revision_ == revision &&
        body_scene_cache_) {
        return *body_scene_cache_;
    }

    const auto evaluation =
        part::evaluatePart(
            session_->document(),
            *solid_modeling_kernel_);
    kernel::RuntimeSolidHandle presentation_solid;
    if (evaluation.body_status ==
            part::BodyEvaluationStatus::
                up_to_date &&
        evaluation.body_solid != nullptr) {
        presentation_solid =
            evaluation.body_solid;
    } else if (
        evaluation.body_status ==
            part::BodyEvaluationStatus::
                unavailable &&
        evaluation.resolved_prefix_solid != nullptr) {
        // H7: present only the current-revision prefix immediately before the
        // first failing active Feature. Final Body truth remains unavailable;
        // no downstream evaluation or semantic reference consumes this solid.
        presentation_solid =
            evaluation.resolved_prefix_solid;
    }

    if (presentation_solid == nullptr) {
        // Empty history or failure at the first active Feature has no valid
        // current-revision prefix to present.
        body_scene_revision_ = revision;
        body_scene_cache_ = empty_scene;
        return *body_scene_cache_;
    }

    const auto mesh =
        solid_modeling_kernel_->
            presentationMesh(
                presentation_solid);
    if (!mesh.ok()) {
        // Provider/presentation failure remains retryable at the same
        // authored revision; never cache it as valid current truth.
        body_scene_revision_.reset();
        body_scene_cache_.reset();
        body_topology_catalog_cache_.reset();
        body_topology_bindings_.clear();
        return std::nullopt;
    }

    const auto scene =
        viewerSolidScene(mesh.mesh);
    if (!scene) {
        body_scene_revision_.reset();
        body_scene_cache_.reset();
        body_topology_catalog_cache_.reset();
        body_topology_bindings_.clear();
        return std::nullopt;
    }

    body_scene_revision_ = revision;
    body_scene_cache_ = *scene;
    return *body_scene_cache_;
}

std::optional<viewer::ProfileRegionPresentation>
PartViewportController::buildProfileRegionPresentation(
    const part::PartSketch& source,
    const sketch::RegionCandidate2D& region) const {
    const auto sample_loop =
        [&source](
            const sketch::RegionLoop2D& loop)
            -> std::optional<
                std::vector<viewer::Point3>> {
            std::vector<viewer::Point3> result;
            for (const auto& use : loop.boundary) {
                const auto points =
                    sampleProfileUse(
                        source.model,
                        use);
                if (!points || points->empty()) {
                    return std::nullopt;
                }

                const std::size_t first =
                    result.empty() ? 0U : 1U;
                for (std::size_t i = first;
                     i < points->size();
                     ++i) {
                    const auto world =
                        detail::sketchPointToWorld(
                            source.placement,
                            (*points)[i]);
                    if (!world) return std::nullopt;
                    result.push_back(*world);
                }
            }
            return result.size() >= 3U
                ? std::optional<
                      std::vector<viewer::Point3>>{
                      std::move(result)}
                : std::nullopt;
        };

    auto outer = sample_loop(region.outer);
    if (!outer) return std::nullopt;

    viewer::ProfileRegionPresentation result;
    result.outer = std::move(*outer);
    result.holes.reserve(region.holes.size());
    for (const auto& hole : region.holes) {
        auto sampled = sample_loop(hole);
        if (!sampled) return std::nullopt;
        result.holes.push_back(
            std::move(*sampled));
    }
    return result.valid()
        ? std::optional<
              viewer::ProfileRegionPresentation>{
              std::move(result)}
        : std::nullopt;
}

std::optional<viewer::ProfileScene>
PartViewportController::buildProfileScene() {
    profile_bindings_.clear();
    viewer::ProfileScene scene;
    if (session_ == nullptr) return scene;

    for (const auto& profile :
         session_->document().profiles()) {
        const bool transient_hide =
            transient_profile_hide_ &&
            *transient_profile_hide_ ==
                profile.id;
        if (transient_hide) {
            continue;
        }

        const bool transient_reveal =
            transient_profile_reveal_ &&
            *transient_profile_reveal_ ==
                profile.id;
        if (!transient_reveal &&
            !session_->document()
                 .profilePresentationVisible(
                     profile.id)) {
            continue;
        }

        const auto evaluation =
            session_->document()
                .evaluateProfile(profile.id);
        if (!evaluation || !evaluation->valid()) {
            continue;
        }

        const auto* source =
            session_->document().findSketch(
                profile.source_sketch_id);
        if (source == nullptr) {
            profile_bindings_.clear();
            return std::nullopt;
        }

        const auto region =
            buildProfileRegionPresentation(
                *source,
                *evaluation->region);
        const auto token =
            allocatePresentationToken();
        if (!region || !token ||
            !profile_bindings_
                 .emplace(token->value, profile.id)
                 .second) {
            profile_bindings_.clear();
            return std::nullopt;
        }
        scene.profiles.push_back(
            viewer::ProfilePresentation{
                *token,
                *region});
    }

    if (!scene.valid()) {
        profile_bindings_.clear();
        return std::nullopt;
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

    const auto bind = [this, hosted](
        viewer::PresentationToken token,
        sketch::EntityId id) {
        return sketch_entity_bindings_.emplace(
            token.value,
            SketchEntityAddress{hosted->id, id}).second;
    };

    for (const auto& line : model_state.lines) {
        const auto start = detail::sketchPointToWorld(hosted->placement, line.start);
        const auto end = detail::sketchPointToWorld(hosted->placement, line.end);
        const auto token = allocatePresentationToken();
        if (!start || !end || !token || !bind(*token, line.id)) {
            sketch_entity_bindings_.clear();
            return std::nullopt;
        }
        scene.lines.push_back(
            {*token,
             *start,
             *end,
             line.role ==
                 sketch::EntityRole::construction});
    }

    const auto add_curve = [&](sketch::EntityId id,
                               sketch::Point2 center,
                               double radius,
                               double start_angle,
                               double sweep_angle,
                               sketch::EntityRole role) {
        const auto token = allocatePresentationToken();
        const auto segments =
            curveSegments(center, radius, start_angle, sweep_angle);
        if (!token || segments.empty() || !bind(*token, id)) {
            return false;
        }

        viewer::SketchCurvePresentation curve;
        curve.token = *token;
        curve.construction =
            role == sketch::EntityRole::construction;
        curve.points.reserve(segments.size() + 1U);

        const auto first = detail::sketchPointToWorld(
            hosted->placement,
            segments.front().start);
        if (!first) return false;
        curve.points.push_back(*first);

        for (const auto& segment : segments) {
            const auto end = detail::sketchPointToWorld(
                hosted->placement, segment.end);
            if (!end) return false;
            curve.points.push_back(*end);
        }

        if (!curve.valid()) {
            return false;
        }
        scene.curves.push_back(std::move(curve));
        return true;
    };

    for (const auto& circle : model_state.circles) {
        if (!add_curve(
                circle.id, circle.center, circle.radius, 0.0,
                2.0 * std::numbers::pi_v<double>,
                circle.role)) {
            sketch_entity_bindings_.clear();
            return std::nullopt;
        }
    }
    for (const auto& arc : model_state.arcs) {
        if (!add_curve(
                arc.id, arc.center, arc.radius,
                arc.start_angle, arc.sweep_angle,
                arc.role)) {
            sketch_entity_bindings_.clear();
            return std::nullopt;
        }
    }

    if (!scene.valid()) {
        sketch_entity_bindings_.clear();
        return std::nullopt;
    }

    return scene;
}

std::optional<viewer::PresentationToken>
PartViewportController::allocatePresentationToken()
    noexcept {
    if (next_presentation_token_ ==
        std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }

    return viewer::PresentationToken{
        next_presentation_token_++};
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
    selection.profiles =
        tree_->selectedProfileIds();

    const auto profile_primary =
        tree_->primaryProfileId();

    if (primary &&
        std::find(
            selected.begin(),
            selected.end(),
            *primary) != selected.end()) {
        selection.primary = primary;
        selection.primary_profile.reset();
    } else if (profile_primary &&
               std::find(
                   selection.profiles.begin(),
                   selection.profiles.end(),
                   *profile_primary) !=
                   selection.profiles.end()) {
        selection.primary.reset();
        selection.primary_profile =
            profile_primary;
    } else if (!selected.empty()) {
        selection.primary = selected.front();
        selection.primary_profile.reset();
    } else if (!selection.profiles.empty()) {
        selection.primary.reset();
        selection.primary_profile =
            selection.profiles.front();
    } else {
        selection.primary.reset();
        selection.primary_profile.reset();
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
        selection.profiles.clear();
        selection.primary_profile.reset();
        applySelectionToSurfaces();
        notifySelectionChanged();
        return;
    }

    if (const auto profile =
            profileFor(intent.token)) {
        if (intent.mode ==
            viewer::SelectionIntentMode::replace) {
            selection.selected.clear();
            selection.primary.reset();
            selection.profiles = {*profile};
            selection.primary_profile = *profile;
        } else {
            const auto found =
                std::find(
                    selection.profiles.begin(),
                    selection.profiles.end(),
                    *profile);
            if (found == selection.profiles.end()) {
                selection.profiles.push_back(*profile);
                selection.primary_profile = *profile;
                selection.primary.reset();
            } else {
                selection.profiles.erase(found);
                if (selection.primary_profile &&
                    *selection.primary_profile == *profile) {
                    selection.primary_profile =
                        selection.profiles.empty()
                            ? std::nullopt
                            : std::optional<part::ProfileId>{
                                  selection.profiles.back()};
                }
            }
        }
        applySelectionToSurfaces();
        notifySelectionChanged();
        return;
    }

    const auto role = roleFor(intent.token);
    if (!role) return;

    if (intent.mode ==
        viewer::SelectionIntentMode::replace) {
        selection.profiles.clear();
        selection.primary_profile.reset();
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
            selection.primary_profile.reset();
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
        tree_->setProfileSelection(
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
    tree_->setProfileSelection(
        selection.profiles,
        selection.primary_profile);

    if (viewport_ == nullptr) return;

    viewer::PresentationSelection presentation;
    presentation.selected.reserve(
        selection.selected.size() +
        selection.profiles.size());

    for (const auto role : selection.selected) {
        presentation.selected.push_back(
            tokenFor(role));
    }

    for (const auto profile_id :
         selection.profiles) {
        const auto token =
            profilePresentationFor(profile_id);
        if (token) {
            presentation.selected.push_back(*token);
        }
    }

    if (selection.primary_profile) {
        presentation.primary =
            profilePresentationFor(
                *selection.primary_profile);
    } else if (selection.primary) {
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
    const auto* selection =
        static_cast<const PartViewportController&>(*this)
            .activeSelection();

    if (selection_changed_handler_) {
        if (selection == nullptr) {
            selection_changed_handler_(
                {},
                std::nullopt);
        } else {
            selection_changed_handler_(
                selection->selected,
                selection->primary);
        }
    }

    if (profile_selection_changed_handler_) {
        if (selection == nullptr) {
            profile_selection_changed_handler_(
                {},
                std::nullopt);
        } else {
            profile_selection_changed_handler_(
                selection->profiles,
                selection->primary_profile);
        }
    }
}

} // namespace simplesolid2::ui
