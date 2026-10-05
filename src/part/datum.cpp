#include <simplesolid2/part/datum.hpp>

namespace simplesolid2::part {

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
