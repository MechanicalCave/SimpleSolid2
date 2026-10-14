#include <simplesolid2/part/datum.hpp>

#include <cmath>
#include <type_traits>

namespace simplesolid2::part {

bool edgePointDatumPlaneStructurallyValid(
    const EdgePointDatumPlane& datum) noexcept {
    if (!datum.id.valid() ||
        !std::visit([](const auto& source) {
            return source.valid();
        }, datum.source)) {
        return false;
    }

    const auto& n = datum.orientation.signed_normal_reference_world;
    const auto& u = datum.orientation.u_seed_world;
    const auto finite = [](kernel::Point3 v) noexcept {
        return std::isfinite(v.x) && std::isfinite(v.y) &&
               std::isfinite(v.z);
    };
    if (!finite(n) || !finite(u)) {
        return false;
    }
    const auto dot = [](kernel::Point3 a, kernel::Point3 b) noexcept {
        return a.x*b.x + a.y*b.y + a.z*b.z;
    };
    constexpr double tolerance = 1.0e-10;
    return std::abs(dot(n,n)-1.0) <= tolerance &&
           std::abs(dot(u,u)-1.0) <= tolerance &&
           std::abs(dot(n,u)) <= tolerance;
}

SelectedEdgeFrameResult resolveEdgePointDatumFrame(
    const PartDocument& owner,
    const EdgePointDatumPlane& datum,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider) {
    if (!datum.id.valid() ||
        !std::visit([](const auto& source) {
            return source.valid();
        }, datum.source)) {
        return {};
    }
    if (!edgePointDatumPlaneStructurallyValid(datum)) {
        SelectedEdgeFrameResult invalid;
        invalid.status =
            SelectedEdgeFrameStatus::invalid_orientation_intent;
        return invalid;
    }
    return std::visit(
        [&](const auto& source) -> SelectedEdgeFrameResult {
            using Source = std::decay_t<decltype(source)>;
            if constexpr (std::is_same_v<
                              Source, SelectedMaterialEdgeEnd>) {
                return resolveSelectedEdgeEndFrame(
                    owner, source, datum.orientation,
                    evaluation, provider);
            } else {
                return resolveSelectedEdgeMidFrame(
                    owner, source, datum.orientation,
                    evaluation, provider);
            }
        }, datum.source);
}

bool isDatumOriginPlane(
    core::BuiltinReferenceRole role) noexcept {
    switch (role) {
    case core::BuiltinReferenceRole::xy_plane:
    case core::BuiltinReferenceRole::xz_plane:
    case core::BuiltinReferenceRole::yz_plane:
        return true;
    default:
        return false;
    }
}

bool BuiltinOriginPlaneReference::valid() const noexcept {
    return isDatumOriginPlane(role);
}

bool PlaneReference::valid() const noexcept {
    return std::visit(
        [](const auto& reference) {
            return reference.valid();
        },
        value);
}

bool offsetDatumPlaneStructurallyValid(
    const OffsetDatumPlane& datum) noexcept {
    return datum.id.valid() &&
           datum.source.valid() &&
           datum.offset.finite();
}

std::optional<core::BuiltinReferenceRole>
builtinOriginPlaneForPlaneReference(
    const PlaneReference& reference) noexcept {
    const auto* origin =
        std::get_if<BuiltinOriginPlaneReference>(
            &reference.value);
    return origin != nullptr && origin->valid()
        ? std::optional<core::BuiltinReferenceRole>{
              origin->role}
        : std::nullopt;
}

const SurfaceReference*
bodyPlanarSurfaceForPlaneReference(
    const PlaneReference& reference) noexcept {
    const auto* surface =
        std::get_if<BodyPlanarSurfacePlaneReference>(
            &reference.value);
    return surface != nullptr && surface->valid()
        ? &surface->reference
        : nullptr;
}

std::optional<DatumId>
datumPlaneIdForPlaneReference(
    const PlaneReference& reference) noexcept {
    const auto* datum =
        std::get_if<DatumPlaneReference>(
            &reference.value);
    return datum != nullptr && datum->valid()
        ? std::optional<DatumId>{datum->datum_id}
        : std::nullopt;
}

} // namespace simplesolid2::part
