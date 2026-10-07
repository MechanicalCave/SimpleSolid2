#include <simplesolid2/kernel/solid_modeling.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
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

bool GeneratedFaceRole::valid() const noexcept {
    if (kind == GeneratedFaceRoleKind::cap) {
        return cap_role.has_value() &&
               capRole(*cap_role) &&
               !side_provenance.has_value();
    }
    if (kind == GeneratedFaceRoleKind::side ||
        kind == GeneratedFaceRoleKind::revolve_side) {
        return !cap_role.has_value() &&
               side_provenance.has_value() &&
               !side_provenance
                    ->source_entity.empty();
    }
    if (kind == GeneratedFaceRoleKind::revolve_start_cap ||
        kind == GeneratedFaceRoleKind::revolve_end_cap) {
        return !cap_role.has_value() &&
               !side_provenance.has_value();
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

bool Axis3::valid() const noexcept {
    const auto finite_point =
        [](const Point3& point) noexcept {
            return std::isfinite(point.x) &&
                   std::isfinite(point.y) &&
                   std::isfinite(point.z);
        };
    if (!finite_point(origin) ||
        !finite_point(direction)) {
        return false;
    }
    const double length_squared =
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z;
    return std::isfinite(length_squared) &&
           length_squared > 0.0;
}

bool AngularRevolveInput::valid() const noexcept {
    if (!profile.valid() ||
        !axis.valid() ||
        !std::isfinite(start_angle_radians) ||
        !std::isfinite(end_angle_radians)) {
        return false;
    }

    switch (operation) {
    case SolidBooleanOperation::add:
    case SolidBooleanOperation::cut:
        break;
    default:
        return false;
    }

    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    const double magnitude =
        std::abs(
            end_angle_radians -
            start_angle_radians);
    return std::isfinite(magnitude) &&
           magnitude > 0.0 &&
           magnitude <= full_turn;
}

bool EdgeFeatureInput::valid() const noexcept {
    if (edges.empty() ||
        !std::isfinite(parameter_mm) ||
        parameter_mm <= 0.0) {
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
            if (edges[index] == edges[other]) {
                return false;
            }
        }
    }
    return true;
}

bool EdgeFeatureInputMembership::valid() const noexcept {
    if (provider_contour_edges.empty()) {
        return false;
    }
    for (std::size_t index = 0U;
         index < provider_contour_edges.size();
         ++index) {
        if (!provider_contour_edges[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < provider_contour_edges.size();
             ++other) {
            if (provider_contour_edges[index] ==
                provider_contour_edges[other]) {
                return false;
            }
        }
    }
    return true;
}

bool EdgeFeatureInputMembership::exactFor(
    const std::vector<RuntimeEdgeToken>&
        requested_edges) const noexcept {
    if (!valid() ||
        requested_edges.size() !=
            provider_contour_edges.size()) {
        return false;
    }

    for (std::size_t index = 0U;
         index < requested_edges.size();
         ++index) {
        if (!requested_edges[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < requested_edges.size();
             ++other) {
            if (requested_edges[index] ==
                requested_edges[other]) {
                return false;
            }
        }
        if (std::find(
                provider_contour_edges.begin(),
                provider_contour_edges.end(),
                requested_edges[index]) ==
            provider_contour_edges.end()) {
            return false;
        }
    }
    return true;
}

bool AngularRevolveInput::fullTurn() const noexcept {
    if (!valid()) return false;
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    return std::abs(
               end_angle_radians -
               start_angle_radians) ==
           full_turn;
}

SolidModelingResult
ISolidModelingKernel::revolve(
    const AngularRevolveInput& input,
    RuntimeSolidHandle upstream) noexcept {
    SolidModelingResult result;
    if (!input.valid()) {
        result.status =
            SolidModelingStatus::invalid_input;
        return result;
    }
    if (input.operation ==
            SolidBooleanOperation::cut &&
        upstream == nullptr) {
        result.status =
            SolidModelingStatus::missing_upstream;
        return result;
    }
    result.status =
        SolidModelingStatus::provider_failure;
    return result;
}

SolidModelingResult
ISolidModelingKernel::edgeFeature(
    const EdgeFeatureInput& input,
    RuntimeSolidHandle upstream) noexcept {
    SolidModelingResult result;
    if (!input.valid()) {
        result.status =
            SolidModelingStatus::invalid_input;
        return result;
    }
    if (upstream == nullptr) {
        result.status =
            SolidModelingStatus::missing_upstream;
        return result;
    }
    result.status =
        SolidModelingStatus::provider_failure;
    return result;
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
ISolidModelingKernel::revolvePreviewMesh(
    const AngularRevolveInput& input,
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
