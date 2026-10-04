#include <simplesolid2/kernel/solid_modeling.hpp>

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

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

bool BodyFacePresentationRange::valid(
    std::size_t total_triangles) const noexcept {
    return runtime_token.valid() &&
           triangle_count > 0U &&
           first_triangle < total_triangles &&
           triangle_count <=
               total_triangles - first_triangle;
}

bool BodyEdgePresentationPath::valid() const noexcept {
    if (!runtime_token.valid() ||
        points.size() < 2U) {
        return false;
    }
    return std::all_of(
        points.begin(),
        points.end(),
        [](const Point3& point) {
            return std::isfinite(point.x) &&
                   std::isfinite(point.y) &&
                   std::isfinite(point.z);
        });
}

bool BodyVertexPresentationPoint::valid() const noexcept {
    return runtime_token.valid() &&
           std::isfinite(point.x) &&
           std::isfinite(point.y) &&
           std::isfinite(point.z);
}

bool BodyPresentation::valid() const noexcept {
    if (!mesh.valid()) {
        return false;
    }

    std::vector<bool> triangle_coverage(
        mesh.triangles.size(),
        false);
    for (std::size_t index = 0U;
         index < faces.size();
         ++index) {
        if (!faces[index].valid(
                mesh.triangles.size())) {
            return false;
        }
        for (std::size_t triangle =
                 faces[index].first_triangle;
             triangle <
                 faces[index].first_triangle +
                     faces[index].triangle_count;
             ++triangle) {
            if (triangle_coverage[triangle]) {
                return false;
            }
            triangle_coverage[triangle] = true;
        }
        for (std::size_t other = index + 1U;
             other < faces.size();
             ++other) {
            if (faces[index].runtime_token ==
                faces[other].runtime_token) {
                return false;
            }
        }
    }
    if (!faces.empty() &&
        !std::all_of(
            triangle_coverage.begin(),
            triangle_coverage.end(),
            [](bool covered) {
                return covered;
            })) {
        return false;
    }

    for (std::size_t index = 0U;
         index < edges.size();
         ++index) {
        if (!edges[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < edges.size();
             ++other) {
            if (edges[index].runtime_token ==
                edges[other].runtime_token) {
                return false;
            }
        }
    }

    for (std::size_t index = 0U;
         index < vertices.size();
         ++index) {
        if (!vertices[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < vertices.size();
             ++other) {
            if (vertices[index].runtime_token ==
                vertices[other].runtime_token) {
                return false;
            }
        }
    }

    return true;
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
ISolidModelingKernel::extrudePreviewMesh(
    const LinearExtrudeInput& input,
    RuntimeSolidHandle upstream) noexcept {
    if (!input.valid()) {
        return {
            SolidPresentationStatus::invalid_input,
            {}};
    }
    if (input.operation ==
            SolidBooleanOperation::cut &&
        upstream == nullptr) {
        return {
            SolidPresentationStatus::invalid_input,
            {}};
    }
    return {
        SolidPresentationStatus::unsupported,
        {}};
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

BodyPresentationResult
ISolidModelingKernel::bodyPresentation(
    RuntimeSolidHandle solid) noexcept {
    if (!solid) {
        return {
            SolidPresentationStatus::invalid_input,
            {}};
    }

    const auto mesh =
        presentationMesh(solid);
    if (!mesh.ok()) {
        return {
            mesh.status,
            {}};
    }

    BodyPresentation body;
    body.mesh = mesh.mesh;
    return {
        SolidPresentationStatus::ok,
        std::move(body)};
}

} // namespace simplesolid2::kernel
