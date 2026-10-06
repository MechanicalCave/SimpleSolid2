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



constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;

[[nodiscard]] bool finitePoint(
    const Point3& point) noexcept {
    return std::isfinite(point.x) &&
           std::isfinite(point.y) &&
           std::isfinite(point.z);
}

[[nodiscard]] double dot(
    const Point3& first,
    const Point3& second) noexcept {
    return first.x * second.x +
           first.y * second.y +
           first.z * second.z;
}

[[nodiscard]] Point3 subtract(
    const Point3& first,
    const Point3& second) noexcept {
    return {
        first.x - second.x,
        first.y - second.y,
        first.z - second.z};
}

struct LocalAxis2 final {
    double origin_u{};
    double origin_v{};
    double direction_u{};
    double direction_v{};
};

[[nodiscard]] std::optional<LocalAxis2>
axisInProfilePlane(
    const PlanarProfileInput& profile,
    const Axis3& axis) noexcept {
    if (!profile.valid() || !axis.valid()) {
        return std::nullopt;
    }

    const auto offset =
        subtract(
            axis.origin,
            profile.frame.origin);
    // PM-04 deliberately introduces no new modeling tolerance. Semantic
    // frames/axes must agree exactly at this neutral boundary; Viewer/provider
    // tolerances never relax admission.
    if (dot(offset, profile.frame.normal) != 0.0 ||
        dot(axis.direction, profile.frame.normal) != 0.0) {
        return std::nullopt;
    }

    LocalAxis2 result;
    result.origin_u =
        dot(offset, profile.frame.u_axis);
    result.origin_v =
        dot(offset, profile.frame.v_axis);
    result.direction_u =
        dot(axis.direction, profile.frame.u_axis);
    result.direction_v =
        dot(axis.direction, profile.frame.v_axis);
    const double length2 =
        result.direction_u * result.direction_u +
        result.direction_v * result.direction_v;
    return std::isfinite(result.origin_u) &&
                   std::isfinite(result.origin_v) &&
                   std::isfinite(result.direction_u) &&
                   std::isfinite(result.direction_v) &&
                   length2 > 0.0
        ? std::optional<LocalAxis2>{result}
        : std::nullopt;
}

[[nodiscard]] double signedSide(
    const LocalAxis2& axis,
    Point2 point) noexcept {
    return
        axis.direction_u *
            (point.v - axis.origin_v) -
        axis.direction_v *
            (point.u - axis.origin_u);
}

struct SideRange final {
    double minimum{};
    double maximum{};

    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(minimum) &&
               std::isfinite(maximum) &&
               minimum <= maximum;
    }
};

void includeValue(
    SideRange& range,
    bool& initialized,
    double value) noexcept {
    if (!initialized) {
        range.minimum = value;
        range.maximum = value;
        initialized = true;
        return;
    }
    range.minimum =
        std::min(range.minimum, value);
    range.maximum =
        std::max(range.maximum, value);
}

[[nodiscard]] bool angleOnSweep(
    double angle,
    double start,
    double delta) noexcept {
    if (!std::isfinite(angle) ||
        !std::isfinite(start) ||
        !std::isfinite(delta) ||
        delta == 0.0 ||
        std::abs(delta) > full_turn) {
        return false;
    }
    if (std::abs(delta) == full_turn) {
        return true;
    }

    if (delta > 0.0) {
        double distance =
            std::fmod(
                angle - start,
                full_turn);
        if (distance < 0.0) {
            distance += full_turn;
        }
        return distance <= delta;
    }

    double distance =
        std::fmod(
            start - angle,
            full_turn);
    if (distance < 0.0) {
        distance += full_turn;
    }
    return distance <= -delta;
}

[[nodiscard]] SideRange circularSideRange(
    const LocalAxis2& axis,
    Point2 center,
    double radius,
    double start_angle,
    double delta) noexcept {
    SideRange range;
    bool initialized = false;

    const auto value_at =
        [&](double angle) {
            return signedSide(
                axis,
                {
                    center.u +
                        radius * std::cos(angle),
                    center.v +
                        radius * std::sin(angle)});
        };

    includeValue(
        range,
        initialized,
        value_at(start_angle));
    includeValue(
        range,
        initialized,
        value_at(start_angle + delta));

    const double coefficient_cos =
        -axis.direction_v * radius;
    const double coefficient_sin =
        axis.direction_u * radius;
    const double maximum_angle =
        std::atan2(
            coefficient_sin,
            coefficient_cos);
    const double minimum_angle =
        maximum_angle +
        std::numbers::pi_v<double>;

    if (angleOnSweep(
            maximum_angle,
            start_angle,
            delta)) {
        includeValue(
            range,
            initialized,
            value_at(maximum_angle));
    }
    if (angleOnSweep(
            minimum_angle,
            start_angle,
            delta)) {
        includeValue(
            range,
            initialized,
            value_at(minimum_angle));
    }

    return range;
}

[[nodiscard]] std::optional<SideRange>
boundaryUseSideRange(
    const LocalAxis2& axis,
    const BoundaryUse2D& use) noexcept {
    if (!use.valid()) {
        return std::nullopt;
    }

    if (const auto* line =
            std::get_if<Line2>(&use.curve)) {
        const auto point_at =
            [line](double parameter) {
                return Point2{
                    line->start.u +
                        (line->end.u -
                         line->start.u) *
                            parameter,
                    line->start.v +
                        (line->end.v -
                         line->start.v) *
                            parameter};
            };
        const double first =
            signedSide(
                axis,
                point_at(use.start_parameter));
        const double second =
            signedSide(
                axis,
                point_at(use.end_parameter));
        SideRange range{
            std::min(first, second),
            std::max(first, second)};
        return range.valid()
            ? std::optional<SideRange>{range}
            : std::nullopt;
    }

    if (const auto* circle =
            std::get_if<Circle2>(&use.curve)) {
        if (!(circle->radius > 0.0) ||
            !std::isfinite(circle->radius)) {
            return std::nullopt;
        }

        if (use.whole_closed_curve) {
            const double center_side =
                signedSide(
                    axis,
                    circle->center);
            const double amplitude =
                circle->radius *
                std::sqrt(
                    axis.direction_u *
                        axis.direction_u +
                    axis.direction_v *
                        axis.direction_v);
            SideRange range{
                center_side - amplitude,
                center_side + amplitude};
            return range.valid()
                ? std::optional<SideRange>{range}
                : std::nullopt;
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
        if (!use.follows_source_direction) {
            delta = -delta;
        }
        const auto range =
            circularSideRange(
                axis,
                circle->center,
                circle->radius,
                full_turn * from,
                delta);
        return range.valid()
            ? std::optional<SideRange>{range}
            : std::nullopt;
    }

    const auto* arc =
        std::get_if<Arc2>(&use.curve);
    if (arc == nullptr ||
        !(arc->radius > 0.0) ||
        !std::isfinite(arc->radius)) {
        return std::nullopt;
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
        arc->sweep_angle * (to - from);
    if (!use.follows_source_direction) {
        delta = -delta;
    }
    const auto range =
        circularSideRange(
            axis,
            arc->center,
            arc->radius,
            arc->start_angle +
                arc->sweep_angle * from,
            delta);
    return range.valid()
        ? std::optional<SideRange>{range}
        : std::nullopt;
}
} // namespace

bool Axis3::valid() const noexcept {
    if (!finitePoint(origin) ||
        !finitePoint(direction)) {
        return false;
    }
    const double length2 =
        direction.x * direction.x +
        direction.y * direction.y +
        direction.z * direction.z;
    return std::isfinite(length2) &&
           length2 > 0.0;
}

RevolveProfileAdmission
classifyRevolveProfileAdmission(
    const PlanarProfileInput& profile,
    const Axis3& axis) noexcept {
    if (!profile.valid() || !axis.valid()) {
        return RevolveProfileAdmission::
            invalid_input;
    }

    const auto local =
        axisInProfilePlane(profile, axis);
    if (!local) {
        return RevolveProfileAdmission::
            axis_not_in_profile_plane;
    }

    bool initialized = false;
    SideRange outer_range;
    for (const auto& use :
         profile.outer.boundary) {
        const auto range =
            boundaryUseSideRange(
                *local,
                use);
        if (!range) {
            return RevolveProfileAdmission::
                invalid_input;
        }
        includeValue(
            outer_range,
            initialized,
            range->minimum);
        includeValue(
            outer_range,
            initialized,
            range->maximum);
    }

    if (!initialized ||
        !outer_range.valid()) {
        return RevolveProfileAdmission::
            invalid_input;
    }

    return outer_range.minimum < 0.0 &&
                   outer_range.maximum > 0.0
        ? RevolveProfileAdmission::
              profile_crosses_axis
        : RevolveProfileAdmission::ok;
}

bool RevolveInput::fullRotation() const noexcept {
    const double sweep =
        end_angle_radians -
        start_angle_radians;
    return std::isfinite(sweep) &&
           std::abs(sweep) == full_turn;
}

bool RevolveInput::valid() const noexcept {
    if (!profile.valid() ||
        !axis.valid() ||
        !std::isfinite(start_angle_radians) ||
        !std::isfinite(end_angle_radians)) {
        return false;
    }
    const double sweep =
        end_angle_radians -
        start_angle_radians;
    if (!std::isfinite(sweep) ||
        sweep == 0.0 ||
        std::abs(sweep) > full_turn ||
        classifyRevolveProfileAdmission(
            profile,
            axis) !=
            RevolveProfileAdmission::ok) {
        return false;
    }
    switch (operation) {
    case SolidBooleanOperation::add:
    case SolidBooleanOperation::cut:
        break;
    default:
        return false;
    }
    return capRole(start_cap_role) &&
           capRole(end_cap_role);
}

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

SolidModelingResult
ISolidModelingKernel::revolve(
    const RevolveInput& input,
    RuntimeSolidHandle) noexcept {
    SolidModelingResult result;
    result.status =
        input.valid()
            ? SolidModelingStatus::unsupported
            : SolidModelingStatus::invalid_input;
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
