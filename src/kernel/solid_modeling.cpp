#include <simplesolid2/kernel/solid_modeling.hpp>

#include <algorithm>
#include <cmath>

namespace simplesolid2::kernel {
namespace {

[[nodiscard]] bool capRole(
    ExtrudeCapRole role) noexcept {
    switch (role) {
    case ExtrudeCapRole::profile_cap:
    case ExtrudeCapRole::extent_cap:
    case ExtrudeCapRole::negative_cap:
    case ExtrudeCapRole::positive_cap:
        return true;
    }
    return false;
}

} // namespace

bool SolidMeshTriangle::valid() const noexcept {
    const auto finite_point =
        [](const Point3& point) noexcept {
            return std::isfinite(point.x) &&
                   std::isfinite(point.y) &&
                   std::isfinite(point.z);
        };
    if (!finite_point(first) ||
        !finite_point(second) ||
        !finite_point(third) ||
        !finite_point(first_normal) ||
        !finite_point(second_normal) ||
        !finite_point(third_normal)) {
        return false;
    }

    const double abx = second.x - first.x;
    const double aby = second.y - first.y;
    const double abz = second.z - first.z;
    const double acx = third.x - first.x;
    const double acy = third.y - first.y;
    const double acz = third.z - first.z;
    const double cx = aby * acz - abz * acy;
    const double cy = abz * acx - abx * acz;
    const double cz = abx * acy - aby * acx;
    const double area2 =
        cx * cx + cy * cy + cz * cz;
    const auto normal2 =
        [](const Point3& value) noexcept {
            return value.x * value.x +
                   value.y * value.y +
                   value.z * value.z;
        };
    const double first_normal2 =
        normal2(first_normal);
    const double second_normal2 =
        normal2(second_normal);
    const double third_normal2 =
        normal2(third_normal);
    return std::isfinite(area2) &&
           std::isfinite(first_normal2) &&
           std::isfinite(second_normal2) &&
           std::isfinite(third_normal2) &&
           area2 > 0.0 &&
           first_normal2 > 0.0 &&
           second_normal2 > 0.0 &&
           third_normal2 > 0.0;
}

bool SolidPresentationMesh::valid() const noexcept {
    return !triangles.empty() &&
           std::all_of(
               triangles.begin(),
               triangles.end(),
               [](const SolidMeshTriangle& item) {
                   return item.valid();
               });
}

bool ExtrudeFaceRole::valid() const noexcept {
    if (kind == ExtrudeGeneratedFaceRoleKind::cap) {
        return cap_role.has_value() &&
               capRole(*cap_role) &&
               !side_provenance.has_value();
    }
    if (kind == ExtrudeGeneratedFaceRoleKind::side) {
        return !cap_role.has_value() &&
               side_provenance.has_value() &&
               !side_provenance
                    ->source_entity.empty();
    }
    return false;
}

bool LinearExtrudeInput::valid() const noexcept {
    if (!profile.valid() ||
        !std::isfinite(start_offset_mm) ||
        !std::isfinite(end_offset_mm) ||
        !(start_offset_mm < end_offset_mm) ||
        !capRole(start_cap_role) ||
        !capRole(end_cap_role) ||
        start_cap_role == end_cap_role) {
        return false;
    }

    switch (operation) {
    case SolidBooleanOperation::add:
    case SolidBooleanOperation::cut:
        break;
    default:
        return false;
    }

    const bool forward_one_side =
        start_offset_mm == 0.0 &&
        end_offset_mm > 0.0 &&
        start_cap_role ==
            ExtrudeCapRole::profile_cap &&
        end_cap_role ==
            ExtrudeCapRole::extent_cap;
    const bool reverse_one_side =
        start_offset_mm < 0.0 &&
        end_offset_mm == 0.0 &&
        start_cap_role ==
            ExtrudeCapRole::extent_cap &&
        end_cap_role ==
            ExtrudeCapRole::profile_cap;
    const bool midplane =
        start_offset_mm < 0.0 &&
        end_offset_mm > 0.0 &&
        start_offset_mm == -end_offset_mm &&
        start_cap_role ==
            ExtrudeCapRole::negative_cap &&
        end_cap_role ==
            ExtrudeCapRole::positive_cap;

    return forward_one_side ||
           reverse_one_side ||
           midplane;
}

SolidPresentationResult
ISolidModelingKernel::presentationMesh(
    RuntimeSolidHandle solid) noexcept {
    return {
        solid
            ? SolidPresentationStatus::unsupported
            : SolidPresentationStatus::invalid_input,
        {}};
}

} // namespace simplesolid2::kernel
