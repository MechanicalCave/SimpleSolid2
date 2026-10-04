#include <simplesolid2/part/part_sketch.hpp>

#include <simplesolid2/part/feature_evaluation.hpp>

#include <algorithm>
#include <cmath>

namespace simplesolid2::part {
namespace {

bool finiteVector(
    const std::array<double, 3>& value) noexcept {
    return std::isfinite(value[0]) &&
           std::isfinite(value[1]) &&
           std::isfinite(value[2]);
}

double squaredLength(
    const std::array<double, 3>& value) noexcept {
    return value[0] * value[0] +
           value[1] * value[1] +
           value[2] * value[2];
}

std::array<double, 3> cross(
    const std::array<double, 3>& a,
    const std::array<double, 3>& b) noexcept {
    return {
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    };
}

} // namespace

bool SketchPlacement::valid() const noexcept {
    return finiteVector(origin) &&
           finiteVector(u_axis) &&
           finiteVector(v_axis) &&
           squaredLength(u_axis) > 1.0e-24 &&
           squaredLength(v_axis) > 1.0e-24 &&
           squaredLength(cross(u_axis, v_axis)) > 1.0e-24;
}

bool BuiltinOriginPlaneSketchSupport::valid() const noexcept {
    return isSketchOriginPlane(role);
}

bool PartSketchSupport::valid() const noexcept {
    return std::visit(
        [](const auto& support) {
            return support.valid();
        },
        value);
}

bool isSketchOriginPlane(
    core::BuiltinReferenceRole role) noexcept {
    return role == core::BuiltinReferenceRole::xy_plane ||
           role == core::BuiltinReferenceRole::xz_plane ||
           role == core::BuiltinReferenceRole::yz_plane;
}

std::optional<PartSketchSupport>
partSketchSupportForBuiltinPlane(
    core::BuiltinReferenceRole role) noexcept {
    if (!isSketchOriginPlane(role)) {
        return std::nullopt;
    }

    return PartSketchSupport{
        BuiltinOriginPlaneSketchSupport{
            role}};
}

std::optional<PartSketchSupport>
partSketchSupportForBodyPlanarSurface(
    SurfaceReference reference) noexcept {
    BodyPlanarSurfaceSketchSupport support{
        std::move(reference)};
    if (!support.valid()) {
        return std::nullopt;
    }
    return PartSketchSupport{
        std::move(support)};
}

std::optional<core::BuiltinReferenceRole>
builtinOriginPlaneForSketchSupport(
    const PartSketchSupport& support) noexcept {
    if (!support.valid()) {
        return std::nullopt;
    }
    const auto* origin =
        std::get_if<
            BuiltinOriginPlaneSketchSupport>(
            &support.value);
    return origin != nullptr
        ? std::optional<
              core::BuiltinReferenceRole>{
              origin->role}
        : std::nullopt;
}

const SurfaceReference*
bodyPlanarSurfaceReference(
    const PartSketchSupport& support) noexcept {
    if (!support.valid()) {
        return nullptr;
    }
    const auto* body =
        std::get_if<
            BodyPlanarSurfaceSketchSupport>(
            &support.value);
    return body != nullptr
        ? &body->reference
        : nullptr;
}

std::optional<SketchPlacement>
sketchPlacementForSupport(
    const PartSketchSupport& support) noexcept {
    const auto role =
        builtinOriginPlaneForSketchSupport(
            support);
    if (!role) {
        return std::nullopt;
    }

    SketchPlacement placement;
    switch (*role) {
    case core::BuiltinReferenceRole::xy_plane:
        placement.u_axis = {1.0, 0.0, 0.0};
        placement.v_axis = {0.0, 1.0, 0.0};
        return placement;

    case core::BuiltinReferenceRole::xz_plane:
        placement.u_axis = {1.0, 0.0, 0.0};
        placement.v_axis = {0.0, 0.0, 1.0};
        return placement;

    case core::BuiltinReferenceRole::yz_plane:
        placement.u_axis = {0.0, 1.0, 0.0};
        placement.v_axis = {0.0, 0.0, 1.0};
        return placement;

    default:
        return std::nullopt;
    }
}

bool ResolvedSketchSupport::valid() const noexcept {
    switch (status) {
    case SketchSupportResolutionStatus::resolved:
        return diagnostic ==
                   SketchSupportResolutionDiagnostic::
                       none &&
               frame.has_value() &&
               frame->valid();
    case SketchSupportResolutionStatus::missing:
        return !frame.has_value() &&
               (diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        missing_stage ||
                diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        missing_surface);
    case SketchSupportResolutionStatus::ambiguous:
        return !frame.has_value() &&
               diagnostic ==
                   SketchSupportResolutionDiagnostic::
                       ambiguous_surface;
    case SketchSupportResolutionStatus::unsupported:
        return !frame.has_value() &&
               (diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        invalid_support ||
                diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        unsupported_non_planar ||
                diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        unsupported_surface);
    }
    return false;
}

ResolvedSketchSupport
resolveSketchSupport(
    const PartSketchSupport& support,
    const BodyStageTopologyCatalog* topology) noexcept {
    if (!support.valid()) {
        return {
            SketchSupportResolutionStatus::
                unsupported,
            SketchSupportResolutionDiagnostic::
                invalid_support,
            std::nullopt};
    }

    if (const auto origin =
            sketchPlacementForSupport(support)) {
        return {
            SketchSupportResolutionStatus::
                resolved,
            SketchSupportResolutionDiagnostic::
                none,
            *origin};
    }

    const auto* reference =
        bodyPlanarSurfaceReference(support);
    if (reference == nullptr) {
        return {
            SketchSupportResolutionStatus::
                unsupported,
            SketchSupportResolutionDiagnostic::
                invalid_support,
            std::nullopt};
    }

    if (topology == nullptr ||
        !topology->complete() ||
        topology->stage != reference->stage) {
        return {
            SketchSupportResolutionStatus::
                missing,
            SketchSupportResolutionDiagnostic::
                missing_stage,
            std::nullopt};
    }

    const FeatureSurfaceResolution* found =
        nullptr;
    for (const auto& surface :
         topology->surfaces) {
        if (surface.address !=
            reference->surface) {
            continue;
        }
        if (found != nullptr) {
            return {
                SketchSupportResolutionStatus::
                    ambiguous,
                SketchSupportResolutionDiagnostic::
                    ambiguous_surface,
                std::nullopt};
        }
        found = &surface;
    }

    if (found == nullptr) {
        return {
            SketchSupportResolutionStatus::
                missing,
            SketchSupportResolutionDiagnostic::
                missing_surface,
            std::nullopt};
    }

    switch (found->status) {
    case kernel::ReferenceStatus::missing:
        return {
            SketchSupportResolutionStatus::
                missing,
            SketchSupportResolutionDiagnostic::
                missing_surface,
            std::nullopt};
    case kernel::ReferenceStatus::ambiguous:
        return {
            SketchSupportResolutionStatus::
                ambiguous,
            SketchSupportResolutionDiagnostic::
                ambiguous_surface,
            std::nullopt};
    case kernel::ReferenceStatus::unsupported:
        return {
            SketchSupportResolutionStatus::
                unsupported,
            SketchSupportResolutionDiagnostic::
                unsupported_surface,
            std::nullopt};
    case kernel::ReferenceStatus::resolved:
        break;
    }

    if (found->surface_kind !=
        kernel::SurfaceKind::plane) {
        return {
            SketchSupportResolutionStatus::
                unsupported,
            SketchSupportResolutionDiagnostic::
                unsupported_non_planar,
            std::nullopt};
    }
    if (!found->canonical_frame ||
        !found->canonical_frame->valid()) {
        return {
            SketchSupportResolutionStatus::
                unsupported,
            SketchSupportResolutionDiagnostic::
                unsupported_surface,
            std::nullopt};
    }

    SketchPlacement frame;
    frame.origin = {
        found->canonical_frame->origin.x,
        found->canonical_frame->origin.y,
        found->canonical_frame->origin.z};
    frame.u_axis = {
        found->canonical_frame->u_axis.x,
        found->canonical_frame->u_axis.y,
        found->canonical_frame->u_axis.z};
    frame.v_axis = {
        found->canonical_frame->v_axis.x,
        found->canonical_frame->v_axis.y,
        found->canonical_frame->v_axis.z};
    if (!frame.valid()) {
        return {
            SketchSupportResolutionStatus::
                unsupported,
            SketchSupportResolutionDiagnostic::
                unsupported_surface,
            std::nullopt};
    }

    return {
        SketchSupportResolutionStatus::
            resolved,
        SketchSupportResolutionDiagnostic::
            none,
        frame};
}

} // namespace simplesolid2::part
