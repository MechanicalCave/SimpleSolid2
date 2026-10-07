#include <simplesolid2/part/feature_evaluation.hpp>

#include <simplesolid2/part/axis_evaluation.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>

#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <numbers>
#include <type_traits>
#include <utility>

namespace simplesolid2::part {
namespace {

[[nodiscard]] FeatureEvaluationDiagnosticCode
diagnosticForKernel(
    kernel::SolidModelingStatus status) noexcept {
    switch (status) {
    case kernel::SolidModelingStatus::ok:
        return FeatureEvaluationDiagnosticCode::none;
    case kernel::SolidModelingStatus::invalid_input:
        return FeatureEvaluationDiagnosticCode::
            kernel_invalid_input;
    case kernel::SolidModelingStatus::missing_upstream:
        return FeatureEvaluationDiagnosticCode::
            missing_upstream_body;
    case kernel::SolidModelingStatus::provider_mismatch:
        return FeatureEvaluationDiagnosticCode::
            kernel_provider_mismatch;
    case kernel::SolidModelingStatus::provider_failure:
        return FeatureEvaluationDiagnosticCode::
            kernel_provider_failure;
    case kernel::SolidModelingStatus::invalid_brep:
        return FeatureEvaluationDiagnosticCode::
            invalid_brep;
    case kernel::SolidModelingStatus::detached_add:
        return FeatureEvaluationDiagnosticCode::
            detached_add;
    case kernel::SolidModelingStatus::no_effect:
        return FeatureEvaluationDiagnosticCode::
            no_effect;
    case kernel::SolidModelingStatus::empty_result:
        return FeatureEvaluationDiagnosticCode::
            empty_result;
    case kernel::SolidModelingStatus::multi_solid:
        return FeatureEvaluationDiagnosticCode::
            multi_solid;
    }
    return FeatureEvaluationDiagnosticCode::
        kernel_provider_failure;
}

[[nodiscard]] kernel::SolidBooleanOperation
kernelOperation(
    ExtrudeOperation operation) noexcept {
    return operation == ExtrudeOperation::cut
        ? kernel::SolidBooleanOperation::cut
        : kernel::SolidBooleanOperation::add;
}

[[nodiscard]] kernel::SolidBooleanOperation
kernelOperation(
    RevolveOperation operation) noexcept {
    return operation == RevolveOperation::cut
        ? kernel::SolidBooleanOperation::cut
        : kernel::SolidBooleanOperation::add;
}

[[nodiscard]] FeatureEvaluationDiagnosticCode
diagnosticForProfileMaterialization(
    ProfileKernelInputStatus status) noexcept {
    switch (status) {
    case ProfileKernelInputStatus::resolved:
        return FeatureEvaluationDiagnosticCode::none;
    case ProfileKernelInputStatus::missing_profile:
        return FeatureEvaluationDiagnosticCode::
            missing_profile;
    case ProfileKernelInputStatus::missing_source_sketch:
    case ProfileKernelInputStatus::unresolved_profile:
        return FeatureEvaluationDiagnosticCode::
            unresolved_profile;
    case ProfileKernelInputStatus::support_missing:
        return FeatureEvaluationDiagnosticCode::
            sketch_support_missing;
    case ProfileKernelInputStatus::support_ambiguous:
        return FeatureEvaluationDiagnosticCode::
            sketch_support_ambiguous;
    case ProfileKernelInputStatus::support_unsupported:
        return FeatureEvaluationDiagnosticCode::
            sketch_support_unsupported;
    case ProfileKernelInputStatus::invalid_input:
        return FeatureEvaluationDiagnosticCode::
            kernel_invalid_input;
    }
    return FeatureEvaluationDiagnosticCode::
        kernel_invalid_input;
}

[[nodiscard]] FeatureEvaluationDiagnosticCode
diagnosticForRevolveInput(
    RevolveKernelInputStatus status) noexcept {
    switch (status) {
    case RevolveKernelInputStatus::resolved:
        return FeatureEvaluationDiagnosticCode::none;
    case RevolveKernelInputStatus::missing_profile:
        return FeatureEvaluationDiagnosticCode::
            missing_profile;
    case RevolveKernelInputStatus::profile_unavailable:
        return FeatureEvaluationDiagnosticCode::
            profile_unavailable;
    case RevolveKernelInputStatus::missing_axis:
        return FeatureEvaluationDiagnosticCode::
            missing_axis;
    case RevolveKernelInputStatus::axis_unavailable:
        return FeatureEvaluationDiagnosticCode::
            axis_unavailable;
    case RevolveKernelInputStatus::axis_not_in_profile_plane:
        return FeatureEvaluationDiagnosticCode::
            axis_not_in_profile_plane;
    case RevolveKernelInputStatus::profile_crosses_axis:
        return FeatureEvaluationDiagnosticCode::
            profile_crosses_axis;
    case RevolveKernelInputStatus::invalid_input:
        return FeatureEvaluationDiagnosticCode::
            kernel_invalid_input;
    }
    return FeatureEvaluationDiagnosticCode::
        kernel_invalid_input;
}


[[nodiscard]] std::optional<kernel::LinearExtrudeInput>
makeKernelExtrudeInputFromProfile(
    kernel::PlanarProfileInput profile,
    const ExtrudeFeature& feature) {
    kernel::LinearExtrudeInput result;
    result.profile = std::move(profile);
    result.operation =
        kernelOperation(feature.operation);

    if (const auto* one_sided =
            std::get_if<OneSidedExtrudeExtent>(
                &feature.extent)) {
        const double distance =
            one_sided->distance.millimetres;
        if (one_sided->reversed) {
            result.start_offset_mm =
                -distance;
            result.end_offset_mm = 0.0;
            result.start_cap_role =
                kernel::ExtrudeCapRole::
                    extent_cap;
            result.end_cap_role =
                kernel::ExtrudeCapRole::
                    profile_cap;
        } else {
            result.start_offset_mm = 0.0;
            result.end_offset_mm =
                distance;
            result.start_cap_role =
                kernel::ExtrudeCapRole::
                    profile_cap;
            result.end_cap_role =
                kernel::ExtrudeCapRole::
                    extent_cap;
        }
        return result.valid()
            ? std::optional<
                  kernel::LinearExtrudeInput>{
                  std::move(result)}
            : std::nullopt;
    }

    const auto* midplane =
        std::get_if<MidplaneExtrudeExtent>(
            &feature.extent);
    if (midplane == nullptr) {
        return std::nullopt;
    }

    const double half =
        midplane->total_distance
            .millimetres *
        0.5;
    result.start_offset_mm = -half;
    result.end_offset_mm = half;
    result.start_cap_role =
        kernel::ExtrudeCapRole::
            negative_cap;
    result.end_cap_role =
        kernel::ExtrudeCapRole::
            positive_cap;
    return result.valid()
        ? std::optional<
              kernel::LinearExtrudeInput>{
              std::move(result)}
        : std::nullopt;
}

[[nodiscard]] kernel::Point3
cross3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return {
        first.y * second.z -
            first.z * second.y,
        first.z * second.x -
            first.x * second.z,
        first.x * second.y -
            first.y * second.x};
}

[[nodiscard]] double
dot3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return first.x * second.x +
           first.y * second.y +
           first.z * second.z;
}

[[nodiscard]] kernel::Point3
subtract3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return {
        first.x - second.x,
        first.y - second.y,
        first.z - second.z};
}

[[nodiscard]] double
signedHalfPlaneValue(
    double a,
    double b,
    double c,
    const kernel::Point2& point) noexcept {
    return a * point.u +
           b * point.v +
           c;
}

[[nodiscard]] kernel::Point2
linePoint(
    const kernel::Line2& line,
    double parameter) noexcept {
    return {
        line.start.u +
            (line.end.u - line.start.u) *
                parameter,
        line.start.v +
            (line.end.v - line.start.v) *
                parameter};
}

[[nodiscard]] double
positiveAngleDelta(
    double value) noexcept {
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    auto result =
        std::fmod(value, full_turn);
    if (result < 0.0) {
        result += full_turn;
    }
    return result;
}

[[nodiscard]] bool
angleOnSweep(
    double start,
    double delta,
    double candidate) noexcept {
    if (delta > 0.0) {
        return positiveAngleDelta(
                   candidate - start) <=
               delta;
    }
    return positiveAngleDelta(
               start - candidate) <=
           -delta;
}

void includeCircularExtrema(
    double center_u,
    double center_v,
    double radius,
    double start,
    double delta,
    bool whole,
    double a,
    double b,
    double c,
    double& minimum,
    double& maximum) noexcept {
    const auto value =
        [=](double angle) noexcept {
            return a *
                       (center_u +
                        radius *
                            std::cos(angle)) +
                   b *
                       (center_v +
                        radius *
                            std::sin(angle)) +
                   c;
        };

    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    const double amplitude =
        radius *
        std::sqrt(a * a + b * b);
    const double center =
        a * center_u +
        b * center_v +
        c;

    if (whole ||
        std::abs(delta) == full_turn) {
        minimum =
            std::min(
                minimum,
                center - amplitude);
        maximum =
            std::max(
                maximum,
                center + amplitude);
        return;
    }

    const double end =
        start + delta;
    minimum =
        std::min(
            minimum,
            std::min(
                value(start),
                value(end)));
    maximum =
        std::max(
            maximum,
            std::max(
                value(start),
                value(end)));

    const double maximum_angle =
        std::atan2(b, a);
    const double minimum_angle =
        maximum_angle +
        std::numbers::pi_v<double>;
    for (const double candidate :
         {maximum_angle, minimum_angle}) {
        if (!angleOnSweep(
                start,
                delta,
                candidate)) {
            continue;
        }
        const double evaluated =
            value(candidate);
        minimum =
            std::min(
                minimum,
                evaluated);
        maximum =
            std::max(
                maximum,
                evaluated);
    }
}

[[nodiscard]] bool
profileCrossesAxis(
    const kernel::PlanarProfileInput& profile,
    const ResolvedAxisLine& axis,
    bool coplanarity_proven,
    bool& coplanar) noexcept {
    const auto plane_normal =
        cross3(
            profile.frame.u_axis,
            profile.frame.v_axis);
    const auto origin_delta =
        subtract3(
            axis.origin,
            profile.frame.origin);
    coplanar =
        coplanarity_proven ||
        (dot3(
             plane_normal,
             origin_delta) == 0.0 &&
         dot3(
             plane_normal,
             axis.direction) == 0.0);
    if (!coplanar) {
        return false;
    }

    // The signed half-plane functional is evaluated directly on Profile-local
    // U/V geometry but derived from the current world Axis and support frame.
    // No tolerance/proximity policy or geometry search is introduced.
    const auto axis_cross_u =
        cross3(
            axis.direction,
            profile.frame.u_axis);
    const auto axis_cross_v =
        cross3(
            axis.direction,
            profile.frame.v_axis);
    const auto axis_cross_origin =
        cross3(
            axis.direction,
            subtract3(
                profile.frame.origin,
                axis.origin));
    const double a =
        dot3(
            axis_cross_u,
            plane_normal);
    const double b =
        dot3(
            axis_cross_v,
            plane_normal);
    const double c =
        dot3(
            axis_cross_origin,
            plane_normal);

    double minimum =
        std::numeric_limits<double>::infinity();
    double maximum =
        -std::numeric_limits<double>::infinity();

    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;

    for (const auto& use :
         profile.outer.boundary) {
        std::visit(
            [&](const auto& curve) {
                using T =
                    std::decay_t<decltype(curve)>;
                if constexpr (
                    std::is_same_v<
                        T,
                        kernel::Line2>) {
                    const auto first =
                        linePoint(
                            curve,
                            use.start_parameter);
                    const auto second =
                        linePoint(
                            curve,
                            use.end_parameter);
                    const double first_value =
                        signedHalfPlaneValue(
                            a, b, c, first);
                    const double second_value =
                        signedHalfPlaneValue(
                            a, b, c, second);
                    minimum =
                        std::min(
                            minimum,
                            std::min(
                                first_value,
                                second_value));
                    maximum =
                        std::max(
                            maximum,
                            std::max(
                                first_value,
                                second_value));
                } else if constexpr (
                    std::is_same_v<
                        T,
                        kernel::Circle2>) {
                    double start =
                        full_turn *
                        use.start_parameter;
                    double delta{};
                    if (use.whole_closed_curve) {
                        delta =
                            use.follows_source_direction
                                ? full_turn
                                : -full_turn;
                    } else {
                        const double from =
                            use.follows_source_direction
                                ? use.start_parameter
                                : use.end_parameter;
                        const double to =
                            use.follows_source_direction
                                ? use.end_parameter
                                : use.start_parameter;
                        start =
                            full_turn * from;
                        delta =
                            use.crosses_closed_seam
                                ? (1.0 - from) + to
                                : to - from;
                        delta *= full_turn;
                        if (!use.follows_source_direction) {
                            delta = -delta;
                        }
                    }
                    includeCircularExtrema(
                        curve.center.u,
                        curve.center.v,
                        curve.radius,
                        start,
                        delta,
                        use.whole_closed_curve,
                        a,
                        b,
                        c,
                        minimum,
                        maximum);
                } else {
                    const double start =
                        curve.start_angle +
                        curve.sweep_angle *
                            use.start_parameter;
                    const double delta =
                        curve.sweep_angle *
                        (use.end_parameter -
                         use.start_parameter);
                    includeCircularExtrema(
                        curve.center.u,
                        curve.center.v,
                        curve.radius,
                        start,
                        delta,
                        false,
                        a,
                        b,
                        c,
                        minimum,
                        maximum);
                }
            },
            use.curve);
    }

    return minimum < 0.0 &&
           maximum > 0.0;
}

[[nodiscard]] bool
originAxisLiesInOriginPlane(
    core::BuiltinReferenceRole axis,
    core::BuiltinReferenceRole plane) noexcept {
    switch (plane) {
    case core::BuiltinReferenceRole::xy_plane:
        return axis ==
                   core::BuiltinReferenceRole::x_axis ||
               axis ==
                   core::BuiltinReferenceRole::y_axis;
    case core::BuiltinReferenceRole::xz_plane:
        return axis ==
                   core::BuiltinReferenceRole::x_axis ||
               axis ==
                   core::BuiltinReferenceRole::z_axis;
    case core::BuiltinReferenceRole::yz_plane:
        return axis ==
                   core::BuiltinReferenceRole::y_axis ||
               axis ==
                   core::BuiltinReferenceRole::z_axis;
    default:
        return false;
    }
}

[[nodiscard]] bool
coplanarityProvenBySupportIdentity(
    const PartDocument& document,
    const PartProfile& profile,
    const AxisReference& reference) noexcept {
    const auto* profile_sketch =
        document.findSketch(
            profile.source_sketch_id);
    if (profile_sketch == nullptr) {
        return false;
    }

    if (const auto origin_axis =
            builtinOriginAxisForAxisReference(
                reference)) {
        const auto origin_plane =
            builtinOriginPlaneForSketchSupport(
                profile_sketch->support);
        return origin_plane &&
               originAxisLiesInOriginPlane(
                   *origin_axis,
                   *origin_plane);
    }

    const auto axis_id =
        authoredAxisIdForAxisReference(
            reference);
    if (!axis_id) {
        return false;
    }
    const auto* axis =
        document.findAxis(*axis_id);
    if (axis == nullptr) {
        return false;
    }
    const auto* axis_sketch =
        document.findSketch(
            axis->source.sketch_id);
    return axis_sketch != nullptr &&
           axis_sketch->support ==
               profile_sketch->support;
}

[[nodiscard]] const BodyStageTopologyCatalog*
profileSupportTopology(
    const PartDocument& document,
    ProfileId profile_id,
    const PartEvaluation* prefix_evaluation) noexcept {
    const auto* profile =
        document.findProfile(profile_id);
    if (profile == nullptr) {
        return nullptr;
    }
    const auto* sketch =
        document.findSketch(
            profile->source_sketch_id);
    if (sketch == nullptr) {
        return nullptr;
    }
    const auto* surface =
        bodyPlanarSurfaceReference(
            sketch->support);
    if (surface == nullptr) {
        return nullptr;
    }
    if (prefix_evaluation == nullptr ||
        prefix_evaluation->source_revision !=
            document.revision() ||
        surface->stage.kind !=
            BodyStageKind::after_feature ||
        !surface->stage.feature_id) {
        return nullptr;
    }
    const auto* feature =
        prefix_evaluation->findFeature(
            *surface->stage.feature_id);
    return feature != nullptr &&
                   feature->result_topology &&
                   feature->result_topology->complete() &&
                   feature->result_topology->stage ==
                       surface->stage
        ? &*feature->result_topology
        : nullptr;
}

[[nodiscard]] kernel::AngularRevolveInput
makeKernelRevolveInputFromResolved(
    kernel::PlanarProfileInput profile,
    const ResolvedAxisLine& axis,
    const RevolveFeature& feature) {
    kernel::AngularRevolveInput result;
    result.profile = std::move(profile);
    result.axis = {
        axis.origin,
        axis.direction};
    result.operation =
        kernelOperation(feature.operation);

    if (const auto* one_sided =
            std::get_if<
                OneSidedRevolveExtent>(
                &feature.extent)) {
        result.start_angle_radians = 0.0;
        result.end_angle_radians =
            one_sided->reversed
                ? -one_sided->angle.radians
                : one_sided->angle.radians;
        return result;
    }

    const auto& midplane =
        std::get<MidplaneRevolveExtent>(
            feature.extent);
    const double half =
        midplane.total_angle.radians * 0.5;
    result.start_angle_radians = -half;
    result.end_angle_radians = half;
    return result;
}

} // namespace

std::optional<kernel::LinearExtrudeInput>
makeKernelExtrudeInput(
    const PartDocument& document,
    const ExtrudeFeature& feature,
    const BodyStageTopologyCatalog*
        support_topology,
    const DatumEvaluation*
        datum_evaluation) {
    auto profile =
        resolveKernelProfileInput(
            document,
            feature.profile_id,
            support_topology,
            datum_evaluation);
    if (!profile.ok()) {
        return std::nullopt;
    }

    return makeKernelExtrudeInputFromProfile(
        std::move(*profile.input),
        feature);
}

RevolveKernelInputResult
resolveKernelRevolveInput(
    const PartDocument& document,
    const RevolveFeature& feature,
    const PartEvaluation* prefix_evaluation,
    const DatumEvaluation* datum_evaluation) {
    RevolveKernelInputResult result;

    if (!revolveFeatureStructurallyValid(
            feature)) {
        result.status =
            RevolveKernelInputStatus::
                invalid_input;
        return result;
    }

    const auto* profile =
        document.findProfile(
            feature.profile_id);
    if (profile == nullptr) {
        result.status =
            RevolveKernelInputStatus::
                missing_profile;
        return result;
    }

    const auto* support_topology =
        profileSupportTopology(
            document,
            profile->id,
            prefix_evaluation);
    auto materialized =
        resolveKernelProfileInput(
            document,
            profile->id,
            support_topology,
            datum_evaluation);
    if (!materialized.ok()) {
        result.status =
            materialized.status ==
                    ProfileKernelInputStatus::
                        missing_profile
                ? RevolveKernelInputStatus::
                      missing_profile
                : RevolveKernelInputStatus::
                      profile_unavailable;
        return result;
    }

    const auto axis =
        resolveAxisReference(
            document,
            feature.axis,
            prefix_evaluation,
            datum_evaluation);
    result.required_axis_stage =
        axis.required_body_stage;
    if (axis.status !=
            AxisEvaluationStatus::resolved ||
        !axis.line) {
        result.status =
            axis.diagnostic ==
                    AxisEvaluationDiagnostic::
                        missing_axis
                ? RevolveKernelInputStatus::
                      missing_axis
                : RevolveKernelInputStatus::
                      axis_unavailable;
        return result;
    }

    bool coplanar = false;
    const bool crosses =
        profileCrossesAxis(
            *materialized.input,
            *axis.line,
            coplanarityProvenBySupportIdentity(
                document,
                *profile,
                feature.axis),
            coplanar);
    if (!coplanar) {
        result.status =
            RevolveKernelInputStatus::
                axis_not_in_profile_plane;
        return result;
    }
    if (crosses) {
        result.status =
            RevolveKernelInputStatus::
                profile_crosses_axis;
        return result;
    }

    auto input =
        makeKernelRevolveInputFromResolved(
            std::move(*materialized.input),
            *axis.line,
            feature);
    if (!input.valid()) {
        result.status =
            RevolveKernelInputStatus::
                invalid_input;
        return result;
    }

    result.status =
        RevolveKernelInputStatus::resolved;
    result.input =
        std::move(input);
    return result;
}

namespace {

[[nodiscard]] FeatureFaceRoleKind
partCapRole(
    kernel::ExtrudeCapRole role) noexcept {
    switch (role) {
    case kernel::ExtrudeCapRole::profile_cap:
        return FeatureFaceRoleKind::
            profile_cap;
    case kernel::ExtrudeCapRole::extent_cap:
        return FeatureFaceRoleKind::
            extent_cap;
    case kernel::ExtrudeCapRole::negative_cap:
        return FeatureFaceRoleKind::
            negative_cap;
    case kernel::ExtrudeCapRole::positive_cap:
        return FeatureFaceRoleKind::
            positive_cap;
    }
    return FeatureFaceRoleKind::side;
}

[[nodiscard]] FeatureSurfaceRoleKind
partSurfaceCapRole(
    kernel::ExtrudeCapRole role) noexcept {
    switch (role) {
    case kernel::ExtrudeCapRole::profile_cap:
        return FeatureSurfaceRoleKind::
            profile_cap;
    case kernel::ExtrudeCapRole::extent_cap:
        return FeatureSurfaceRoleKind::
            extent_cap;
    case kernel::ExtrudeCapRole::negative_cap:
        return FeatureSurfaceRoleKind::
            negative_cap;
    case kernel::ExtrudeCapRole::positive_cap:
        return FeatureSurfaceRoleKind::
            positive_cap;
    }
    return FeatureSurfaceRoleKind::side;
}

[[nodiscard]] FeatureFaceResolution
convertNewFace(
    FeatureId producer,
    const kernel::NewFaceLineage& source) {
    FeatureFaceResolution result;
    result.address.producer_feature_id =
        producer;
    result.status = source.status;
    result.candidate_count =
        source.candidate_count;
    result.runtime_token =
        source.resolved_token;

    switch (source.role.kind) {
    case kernel::GeneratedFaceRoleKind::cap:
        if (!source.role.cap_role) {
            result.status =
                kernel::ReferenceStatus::
                    unsupported;
            result.runtime_token.reset();
            return result;
        }
        result.address.role =
            partCapRole(
                *source.role.cap_role);
        return result;
    case kernel::GeneratedFaceRoleKind::
        revolve_start_cap:
        result.address.role =
            FeatureFaceRoleKind::
                revolve_start_cap;
        return result;
    case kernel::GeneratedFaceRoleKind::
        revolve_end_cap:
        result.address.role =
            FeatureFaceRoleKind::
                revolve_end_cap;
        return result;
    case kernel::GeneratedFaceRoleKind::side:
        result.address.role =
            FeatureFaceRoleKind::side;
        break;
    case kernel::GeneratedFaceRoleKind::
        revolve_side:
        result.address.role =
            FeatureFaceRoleKind::
                revolve_side;
        break;
    }

    if (!source.role.side_provenance) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        return result;
    }

    const auto& provenance =
        *source.role.side_provenance;
    const auto entity =
        sketch::EntityId::parse(
            provenance.source_entity);
    if (!entity) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        return result;
    }

    result.address.source_entity =
        *entity;
    result.address.loop_index =
        provenance.loop_index;
    result.address.use_index =
        provenance.use_index;
    result.address.hole =
        provenance.hole;
    return result;
}

[[nodiscard]] FeatureSurfaceResolution
convertNewSurface(
    FeatureId producer,
    const kernel::NewSurfaceLineage& source) {
    FeatureSurfaceResolution result;
    result.address.producer_feature_id =
        producer;
    result.status =
        source.surface_status;
    result.strict_face_status =
        source.strict_face_status;
    result.candidate_face_count =
        source.candidate_face_count;
    result.surface_kind =
        source.surface_kind;
    result.canonical_frame =
        source.canonical_frame;
    result.runtime_token =
        source.resolved_token;
    result.current_faces =
        source.current_faces;

    switch (source.role.kind) {
    case kernel::GeneratedFaceRoleKind::cap:
        if (!source.role.cap_role) {
            result.status =
                kernel::ReferenceStatus::
                    unsupported;
            result.strict_face_status =
                kernel::ReferenceStatus::
                    unsupported;
            result.runtime_token.reset();
            result.current_faces.clear();
            result.candidate_face_count = 0U;
            return result;
        }
        result.address.role =
            partSurfaceCapRole(
                *source.role.cap_role);
        return result;
    case kernel::GeneratedFaceRoleKind::
        revolve_start_cap:
        result.address.role =
            FeatureSurfaceRoleKind::
                revolve_start_cap;
        return result;
    case kernel::GeneratedFaceRoleKind::
        revolve_end_cap:
        result.address.role =
            FeatureSurfaceRoleKind::
                revolve_end_cap;
        return result;
    case kernel::GeneratedFaceRoleKind::side:
        result.address.role =
            FeatureSurfaceRoleKind::side;
        break;
    case kernel::GeneratedFaceRoleKind::
        revolve_side:
        result.address.role =
            FeatureSurfaceRoleKind::
                revolve_side;
        break;
    }

    if (!source.role.side_provenance) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.strict_face_status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        result.current_faces.clear();
        result.candidate_face_count = 0U;
        return result;
    }

    const auto& provenance =
        *source.role.side_provenance;
    const auto entity =
        sketch::EntityId::parse(
            provenance.source_entity);
    if (!entity) {
        result.status =
            kernel::ReferenceStatus::
                unsupported;
        result.strict_face_status =
            kernel::ReferenceStatus::
                unsupported;
        result.runtime_token.reset();
        result.current_faces.clear();
        result.candidate_face_count = 0U;
        return result;
    }

    result.address.source_entity =
        *entity;
    result.address.loop_index =
        provenance.loop_index;
    result.address.use_index =
        provenance.use_index;
    result.address.hole =
        provenance.hole;
    return result;
}

void propagateCurrentReferences(
    std::vector<FeatureFaceResolution>& references,
    const std::vector<
        kernel::InheritedFaceLineage>& inherited) {
    std::map<std::uint64_t, std::size_t>
        by_token;
    for (std::size_t index = 0U;
         index < references.size();
         ++index) {
        if (references[index].status ==
                kernel::ReferenceStatus::
                    resolved &&
            references[index].runtime_token) {
            by_token.emplace(
                references[index]
                    .runtime_token->value,
                index);
        }
    }

    std::vector<bool> seen(
        references.size(),
        false);
    for (const auto& item : inherited) {
        const auto found =
            by_token.find(
                item.token.value);
        if (found == by_token.end()) {
            continue;
        }

        auto& reference =
            references[found->second];
        reference.status =
            item.status;
        reference.candidate_count =
            item.candidate_count;
        seen[found->second] = true;
        if (item.status !=
            kernel::ReferenceStatus::
                resolved) {
            reference.runtime_token.reset();
        }
    }

    // Every previously Resolved reference must be reported by the provider.
    // Missing propagation evidence is fail-closed, never assumed Resolved.
    for (const auto& [token, index] :
         by_token) {
        static_cast<void>(token);
        if (!seen[index]) {
            references[index].status =
                kernel::ReferenceStatus::
                    unsupported;
            references[index]
                .candidate_count = 0U;
            references[index]
                .runtime_token.reset();
        }
    }
}

[[nodiscard]] bool propagateCurrentSurfaces(
    std::vector<FeatureSurfaceResolution>& references,
    const std::vector<
        kernel::InheritedSurfaceLineage>& inherited) {
    std::map<std::uint64_t, std::size_t>
        by_token;
    for (std::size_t index = 0U;
         index < references.size();
         ++index) {
        if (references[index].status !=
                kernel::ReferenceStatus::resolved ||
            !references[index].runtime_token) {
            continue;
        }
        const auto [it, inserted] =
            by_token.emplace(
                references[index]
                    .runtime_token->value,
                index);
        static_cast<void>(it);
        if (!inserted) {
            return false;
        }
    }

    std::vector<bool> seen(
        references.size(),
        false);
    for (const auto& item : inherited) {
        if (!item.token.valid()) {
            return false;
        }
        const auto found =
            by_token.find(item.token.value);
        if (found == by_token.end()) {
            return false;
        }

        auto& reference =
            references[found->second];
        if (reference.surface_kind !=
            item.surface_kind) {
            return false;
        }
        if (item.surface_status ==
                kernel::ReferenceStatus::resolved &&
            reference.canonical_frame !=
                item.canonical_frame) {
            return false;
        }

        reference.status =
            item.surface_status;
        reference.strict_face_status =
            item.strict_face_status;
        reference.candidate_face_count =
            item.candidate_face_count;
        reference.canonical_frame =
            item.canonical_frame;
        reference.current_faces =
            item.current_faces;
        seen[found->second] = true;

        if (item.surface_status !=
            kernel::ReferenceStatus::resolved) {
            reference.runtime_token.reset();
        }
    }

    for (const auto& [token, index] :
         by_token) {
        static_cast<void>(token);
        if (!seen[index]) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] bool isSideSurface(
    const FeatureSurfaceAddress& address) noexcept {
    return address.role ==
           FeatureSurfaceRoleKind::side;
}

[[nodiscard]] bool isCapSurface(
    const FeatureSurfaceAddress& address) noexcept {
    return !isSideSurface(address);
}

struct SemanticSurfaceObservation final {
    FeatureSurfaceAddress address;
    kernel::SurfaceKind kind{
        kernel::SurfaceKind::other};

    friend bool operator==(
        const SemanticSurfaceObservation&,
        const SemanticSurfaceObservation&) = default;
};

struct CurveRelation final {
    FeatureCurveRoleKind role{
        FeatureCurveRoleKind::
            boolean_intersection};
    kernel::CurveKind curve_kind{
        kernel::CurveKind::other};
    std::vector<FeatureSurfaceAddress>
        adjacent_surfaces;

    friend bool operator==(
        const CurveRelation&,
        const CurveRelation&) = default;
};

struct PointRelation final {
    std::vector<FeatureSurfaceAddress>
        adjacent_surfaces;

    friend bool operator==(
        const PointRelation&,
        const PointRelation&) = default;
};

[[nodiscard]] std::optional<
    SemanticSurfaceObservation>
semanticSurfaceForRuntimeToken(
    kernel::RuntimeSurfaceToken token,
    const std::vector<FeatureSurfaceResolution>&
        surfaces) {
    if (!token.valid()) return std::nullopt;

    std::optional<SemanticSurfaceObservation>
        result;
    for (const auto& surface : surfaces) {
        if (surface.status !=
                kernel::ReferenceStatus::resolved ||
            !surface.runtime_token ||
            *surface.runtime_token != token) {
            continue;
        }
        if (result) {
            return std::nullopt;
        }
        result = SemanticSurfaceObservation{
            surface.address,
            surface.surface_kind};
    }
    return result;
}

[[nodiscard]] std::optional<CurveRelation>
curveRelationForObservation(
    const kernel::CurrentEdgeSemanticObservation&
        observation,
    const std::vector<FeatureSurfaceResolution>&
        surfaces,
    bool& provider_mismatch) {
    provider_mismatch = false;
    if (!observation.runtime_token.valid() ||
        observation.periodic_seam ||
        observation.same_surface_partition) {
        return std::nullopt;
    }

    std::vector<SemanticSurfaceObservation>
        semantic_surfaces;
    semantic_surfaces.reserve(
        observation.adjacent_surfaces.size());

    for (const auto token :
         observation.adjacent_surfaces) {
        const auto surface =
            semanticSurfaceForRuntimeToken(
                token,
                surfaces);
        if (!surface) {
            return std::nullopt;
        }
        const auto duplicate =
            std::find_if(
                semantic_surfaces.begin(),
                semantic_surfaces.end(),
                [&surface](const auto& existing) {
                    return existing.address ==
                           surface->address;
                });
        if (duplicate !=
            semantic_surfaces.end()) {
            continue;
        }
        semantic_surfaces.push_back(
            *surface);
    }

    if (semantic_surfaces.size() != 2U) {
        return std::nullopt;
    }

    std::sort(
        semantic_surfaces.begin(),
        semantic_surfaces.end(),
        [](const auto& first, const auto& second) {
            return first.address <
                   second.address;
        });

    CurveRelation result;
    result.adjacent_surfaces = {
        semantic_surfaces[0].address,
        semantic_surfaces[1].address,
    };

    const bool same_producer =
        semantic_surfaces[0]
            .address.producer_feature_id ==
        semantic_surfaces[1]
            .address.producer_feature_id;
    const bool first_cap =
        isCapSurface(
            semantic_surfaces[0].address);
    const bool second_cap =
        isCapSurface(
            semantic_surfaces[1].address);
    const bool first_side =
        isSideSurface(
            semantic_surfaces[0].address);
    const bool second_side =
        isSideSurface(
            semantic_surfaces[1].address);

    if (same_producer &&
        ((first_cap && second_side) ||
         (first_side && second_cap))) {
        result.role =
            FeatureCurveRoleKind::cap_side;
        const auto& side =
            first_side
                ? semantic_surfaces[0]
                : semantic_surfaces[1];
        switch (side.kind) {
        case kernel::SurfaceKind::plane:
            result.curve_kind =
                kernel::CurveKind::line;
            break;
        case kernel::SurfaceKind::cylinder:
            result.curve_kind =
                kernel::CurveKind::circle;
            break;
        default:
            return std::nullopt;
        }
    } else if (same_producer &&
               first_side &&
               second_side) {
        result.role =
            FeatureCurveRoleKind::side_side;
        result.curve_kind =
            kernel::CurveKind::line;
    } else if (!same_producer) {
        result.role =
            FeatureCurveRoleKind::
                boolean_intersection;
        if (semantic_surfaces[0].kind ==
                kernel::SurfaceKind::plane &&
            semantic_surfaces[1].kind ==
                kernel::SurfaceKind::plane) {
            result.curve_kind =
                kernel::CurveKind::line;
        } else {
            return std::nullopt;
        }
    } else {
        return std::nullopt;
    }

    if (observation.provider_curve_kind !=
        result.curve_kind) {
        provider_mismatch = true;
        return std::nullopt;
    }

    return result;
}

[[nodiscard]] std::optional<PointRelation>
pointRelationForObservation(
    const kernel::CurrentVertexSemanticObservation&
        observation,
    const std::vector<FeatureSurfaceResolution>&
        surfaces) {
    if (!observation.runtime_token.valid()) {
        return std::nullopt;
    }

    std::vector<FeatureSurfaceAddress>
        addresses;
    addresses.reserve(
        observation.adjacent_surfaces.size());
    for (const auto token :
         observation.adjacent_surfaces) {
        const auto surface =
            semanticSurfaceForRuntimeToken(
                token,
                surfaces);
        if (!surface) {
            return std::nullopt;
        }
        if (std::find(
                addresses.begin(),
                addresses.end(),
                surface->address) ==
            addresses.end()) {
            addresses.push_back(
                surface->address);
        }
    }

    if (addresses.size() != 3U) {
        return std::nullopt;
    }

    std::sort(
        addresses.begin(),
        addresses.end());
    return PointRelation{
        std::move(addresses)};
}

[[nodiscard]] bool sameCurveRelation(
    const FeatureCurveResolution& reference,
    const CurveRelation& relation) noexcept {
    return reference.address.role ==
               relation.role &&
           reference.curve_kind ==
               relation.curve_kind &&
           reference.address.adjacent_surfaces ==
               relation.adjacent_surfaces;
}

[[nodiscard]] bool samePointRelation(
    const FeaturePointResolution& reference,
    const PointRelation& relation) noexcept {
    return reference.address.adjacent_surfaces ==
           relation.adjacent_surfaces;
}

[[nodiscard]] kernel::ReferenceStatus
strictReferenceStatus(
    std::size_t count) noexcept {
    if (count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    return count == 1U
        ? kernel::ReferenceStatus::resolved
        : kernel::ReferenceStatus::ambiguous;
}

template <typename Token>
void appendUniqueRuntimeToken(
    std::vector<Token>& tokens,
    Token token) {
    if (std::find(
            tokens.begin(),
            tokens.end(),
            token) ==
        tokens.end()) {
        tokens.push_back(token);
    }
}

[[nodiscard]] bool isActiveReferenceStatus(
    kernel::ReferenceStatus status) noexcept {
    return status ==
               kernel::ReferenceStatus::resolved ||
           status ==
               kernel::ReferenceStatus::ambiguous;
}

struct CurrentEdgeMeaning final {
    kernel::RuntimeEdgeToken token;
    kernel::CurveKind provider_curve_kind{
        kernel::CurveKind::other};
    bool periodic_seam{false};
    bool representation_partition{false};
    std::optional<CurveRelation> relation;
};

struct CurrentVertexMeaning final {
    kernel::RuntimeVertexToken token;
    std::optional<PointRelation> relation;
    std::vector<kernel::RuntimeEdgeToken>
        incident_material_edges;
    std::optional<kernel::Point3>
        provider_point;
};

struct CurveStageBuild final {
    std::vector<FeatureCurveResolution>
        references;
    std::vector<FeatureCurveResolution>
        produced;
    std::vector<BodyEdgeTopologyRecord>
        records;
};

struct PointStageBuild final {
    std::vector<FeaturePointResolution>
        references;
    std::vector<FeaturePointResolution>
        produced;
    std::vector<BodyVertexTopologyRecord>
        records;
};

[[nodiscard]] const kernel::
    InheritedEdgeRealizationLineage*
findInheritedEdgeLineage(
    kernel::RuntimeEdgeToken source,
    const kernel::SolidModelingResult& result) {
    const kernel::InheritedEdgeRealizationLineage*
        found = nullptr;
    for (const auto& item :
         result.inherited_edge_realizations) {
        if (item.source_token != source) {
            continue;
        }
        if (found != nullptr) {
            return nullptr;
        }
        found = &item;
    }
    return found;
}

[[nodiscard]] const kernel::
    InheritedVertexRealizationLineage*
findInheritedVertexLineage(
    kernel::RuntimeVertexToken source,
    const kernel::SolidModelingResult& result) {
    const kernel::InheritedVertexRealizationLineage*
        found = nullptr;
    for (const auto& item :
         result.inherited_vertex_realizations) {
        if (item.source_token != source) {
            continue;
        }
        if (found != nullptr) {
            return nullptr;
        }
        found = &item;
    }
    return found;
}

[[nodiscard]] const CurrentEdgeMeaning*
findCurrentEdgeMeaning(
    kernel::RuntimeEdgeToken token,
    const std::vector<CurrentEdgeMeaning>&
        meanings) {
    const CurrentEdgeMeaning* found = nullptr;
    for (const auto& item : meanings) {
        if (item.token != token) continue;
        if (found != nullptr) {
            return nullptr;
        }
        found = &item;
    }
    return found;
}

[[nodiscard]] const CurrentVertexMeaning*
findCurrentVertexMeaning(
    kernel::RuntimeVertexToken token,
    const std::vector<CurrentVertexMeaning>&
        meanings) {
    const CurrentVertexMeaning* found = nullptr;
    for (const auto& item : meanings) {
        if (item.token != token) continue;
        if (found != nullptr) {
            return nullptr;
        }
        found = &item;
    }
    return found;
}

[[nodiscard]] std::optional<CurveStageBuild>
buildCurveStage(
    FeatureId current_feature,
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureSurfaceResolution>&
        surfaces,
    const std::vector<FeatureCurveResolution>&
        previous) {
    if (!current_feature.valid() ||
        kernel_result.current_edge_semantics.size() !=
            kernel_result.current_edges.size()) {
        return std::nullopt;
    }

    std::vector<CurrentEdgeMeaning> meanings;
    meanings.reserve(
        kernel_result.current_edge_semantics.size());

    for (const auto& observation :
         kernel_result.current_edge_semantics) {
        if (!observation.runtime_token.valid() ||
            std::count(
                kernel_result.current_edges.begin(),
                kernel_result.current_edges.end(),
                observation.runtime_token) != 1) {
            return std::nullopt;
        }

        bool provider_mismatch = false;
        auto relation =
            curveRelationForObservation(
                observation,
                surfaces,
                provider_mismatch);
        if (provider_mismatch) {
            return std::nullopt;
        }

        meanings.push_back(
            {
                observation.runtime_token,
                observation.provider_curve_kind,
                observation.periodic_seam,
                observation.same_surface_partition,
                std::move(relation),
            });
    }

    for (const auto token :
         kernel_result.current_edges) {
        if (std::count_if(
                meanings.begin(),
                meanings.end(),
                [token](const auto& item) {
                    return item.token == token;
                }) != 1) {
            return std::nullopt;
        }
    }

    CurveStageBuild result;
    result.references = previous;

    // Existing semantic Curve meanings survive only through explicit
    // provider-history descendants that still satisfy the same semantic
    // Surface relation. Missing meanings are terminal for this evaluation:
    // later equal geometry creates new provenance instead of reviving them.
    for (auto& reference : result.references) {
        if (!reference.valid()) {
            return std::nullopt;
        }
        if (!isActiveReferenceStatus(
                reference.status)) {
            reference.current_edges.clear();
            reference.candidate_edge_count = 0U;
            reference.strict_edge_status =
                reference.status;
            continue;
        }
        if (reference.current_edges.empty()) {
            return std::nullopt;
        }

        std::vector<kernel::RuntimeEdgeToken>
            descendants;
        for (const auto source :
             reference.current_edges) {
            const auto* lineage =
                findInheritedEdgeLineage(
                    source,
                    kernel_result);
            if (lineage == nullptr ||
                !lineage->source_token.valid() ||
                lineage->candidate_count !=
                    lineage->current_edges.size()) {
                return std::nullopt;
            }
            for (const auto token :
                 lineage->current_edges) {
                appendUniqueRuntimeToken(
                    descendants,
                    token);
            }
        }

        bool relation_preserved =
            !descendants.empty();
        for (const auto token : descendants) {
            const auto* meaning =
                findCurrentEdgeMeaning(
                    token,
                    meanings);
            if (meaning == nullptr ||
                meaning->periodic_seam ||
                !meaning->relation ||
                !sameCurveRelation(
                    reference,
                    *meaning->relation)) {
                relation_preserved = false;
                break;
            }
        }

        if (!relation_preserved) {
            reference.status =
                kernel::ReferenceStatus::missing;
            reference.strict_edge_status =
                kernel::ReferenceStatus::missing;
            reference.candidate_edge_count = 0U;
            reference.current_edges.clear();
            continue;
        }

        const auto previous_curve_status =
            reference.status;
        reference.current_edges =
            std::move(descendants);
        reference.candidate_edge_count =
            reference.current_edges.size();
        reference.strict_edge_status =
            strictReferenceStatus(
                reference.candidate_edge_count);

        // Explicit provider lineage preserves one semantic Curve family even
        // when its bounded Edge realization splits. A previously ambiguous
        // pair-only Curve may become singular again if only one branch
        // survives in the current stage.
        reference.status =
            previous_curve_status ==
                    kernel::ReferenceStatus::resolved
                ? kernel::ReferenceStatus::resolved
                : strictReferenceStatus(
                      reference.candidate_edge_count);
    }

    // If independent previous meanings collapse onto one current provider
    // Edge, neither may win merely because provider history listed it first.
    for (std::size_t first = 0U;
         first < result.references.size();
         ++first) {
        if (!isActiveReferenceStatus(
                result.references[first].status)) {
            continue;
        }
        for (std::size_t second = first + 1U;
             second < result.references.size();
             ++second) {
            if (!isActiveReferenceStatus(
                    result.references[second].status)) {
                continue;
            }
            bool shared = false;
            for (const auto token :
                 result.references[first].current_edges) {
                if (std::find(
                        result.references[second]
                            .current_edges.begin(),
                        result.references[second]
                            .current_edges.end(),
                        token) !=
                    result.references[second]
                        .current_edges.end()) {
                    shared = true;
                    break;
                }
            }
            if (shared) {
                result.references[first].status =
                    kernel::ReferenceStatus::
                        ambiguous;
                result.references[first]
                    .strict_edge_status =
                    kernel::ReferenceStatus::
                        ambiguous;
                result.references[second].status =
                    kernel::ReferenceStatus::
                        ambiguous;
                result.references[second]
                    .strict_edge_status =
                    kernel::ReferenceStatus::
                        ambiguous;
            }
        }
    }

    std::vector<kernel::RuntimeEdgeToken>
        inherited_claimed;
    for (const auto& reference :
         result.references) {
        if (!isActiveReferenceStatus(
                reference.status)) {
            continue;
        }
        for (const auto token :
             reference.current_edges) {
            appendUniqueRuntimeToken(
                inherited_claimed,
                token);
        }
    }

    struct NewCurveGroup final {
        CurveRelation relation;
        std::vector<kernel::RuntimeEdgeToken>
            edges;
    };
    std::vector<NewCurveGroup> new_groups;

    for (const auto& meaning : meanings) {
        if (meaning.periodic_seam ||
            meaning.representation_partition ||
            !meaning.relation ||
            std::find(
                inherited_claimed.begin(),
                inherited_claimed.end(),
                meaning.token) !=
                inherited_claimed.end()) {
            continue;
        }

        auto group =
            std::find_if(
                new_groups.begin(),
                new_groups.end(),
                [&meaning](const auto& item) {
                    return item.relation ==
                           *meaning.relation;
                });
        if (group == new_groups.end()) {
            new_groups.push_back(
                {
                    *meaning.relation,
                    {meaning.token},
                });
        } else {
            appendUniqueRuntimeToken(
                group->edges,
                meaning.token);
        }
    }

    for (auto& group : new_groups) {
        FeatureCurveResolution reference;
        reference.address.producer_feature_id =
            current_feature;
        reference.address.role =
            group.relation.role;
        reference.address.adjacent_surfaces =
            group.relation.adjacent_surfaces;
        reference.curve_kind =
            group.relation.curve_kind;
        reference.current_edges =
            std::move(group.edges);
        reference.candidate_edge_count =
            reference.current_edges.size();
        reference.strict_edge_status =
            strictReferenceStatus(
                reference.candidate_edge_count);
        reference.status =
            reference.candidate_edge_count == 1U
                ? kernel::ReferenceStatus::resolved
                : kernel::ReferenceStatus::ambiguous;
        if (!reference.valid()) {
            return std::nullopt;
        }
        result.produced.push_back(reference);
        result.references.push_back(
            std::move(reference));
    }

    result.records.reserve(
        meanings.size());
    for (const auto& meaning : meanings) {
        BodyEdgeTopologyRecord record;
        record.runtime_token = meaning.token;
        record.curve_kind =
            meaning.provider_curve_kind;
        record.periodic_seam =
            meaning.periodic_seam;
        record.representation_partition =
            meaning.representation_partition;

        if (meaning.periodic_seam ||
            meaning.representation_partition) {
            record.accounting_class =
                TopologyAccountingClass::
                    known_representation_artifact;
            record.referenceability =
                kernel::ReferenceStatus::
                    unsupported;
            result.records.push_back(
                std::move(record));
            continue;
        }

        if (!meaning.relation) {
            record.accounting_class =
                TopologyAccountingClass::
                    semantically_unsupported;
            record.referenceability =
                kernel::ReferenceStatus::
                    unsupported;
            result.records.push_back(
                std::move(record));
            continue;
        }

        std::vector<kernel::ReferenceStatus>
            statuses;
        for (const auto& reference :
             result.references) {
            if (!isActiveReferenceStatus(
                    reference.status) ||
                std::find(
                    reference.current_edges.begin(),
                    reference.current_edges.end(),
                    meaning.token) ==
                    reference.current_edges.end()) {
                continue;
            }
            if (std::find(
                    record.curve_candidates.begin(),
                    record.curve_candidates.end(),
                    reference.address) ==
                record.curve_candidates.end()) {
                record.curve_candidates.push_back(
                    reference.address);
                statuses.push_back(
                    reference.strict_edge_status);
            }
        }

        if (record.curve_candidates.empty()) {
            // A supported material relation must not disappear from the
            // semantic catalog simply because lifecycle bookkeeping failed.
            return std::nullopt;
        }

        record.accounting_class =
            TopologyAccountingClass::
                referenceable;
        if (record.curve_candidates.size() > 1U ||
            std::any_of(
                statuses.begin(),
                statuses.end(),
                [](kernel::ReferenceStatus status) {
                    return status ==
                           kernel::ReferenceStatus::
                               ambiguous;
                })) {
            record.referenceability =
                kernel::ReferenceStatus::
                    ambiguous;
        } else {
            record.referenceability =
                kernel::ReferenceStatus::
                    resolved;
        }
        result.records.push_back(
            std::move(record));
    }

    return result;
}

[[nodiscard]] std::optional<PointStageBuild>
buildPointStage(
    FeatureId current_feature,
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureSurfaceResolution>&
        surfaces,
    const std::vector<FeaturePointResolution>&
        previous) {
    if (!current_feature.valid() ||
        kernel_result.current_vertex_semantics.size() !=
            kernel_result.current_vertices.size()) {
        return std::nullopt;
    }

    std::vector<CurrentVertexMeaning> meanings;
    meanings.reserve(
        kernel_result.current_vertex_semantics.size());
    for (const auto& observation :
         kernel_result.current_vertex_semantics) {
        if (!observation.runtime_token.valid() ||
            std::count(
                kernel_result.current_vertices.begin(),
                kernel_result.current_vertices.end(),
                observation.runtime_token) != 1) {
            return std::nullopt;
        }
        meanings.push_back(
            {
                observation.runtime_token,
                pointRelationForObservation(
                    observation,
                    surfaces),
                observation.incident_material_edges,
                observation.provider_point,
            });
    }

    for (const auto token :
         kernel_result.current_vertices) {
        if (std::count_if(
                meanings.begin(),
                meanings.end(),
                [token](const auto& item) {
                    return item.token == token;
                }) != 1) {
            return std::nullopt;
        }
    }

    PointStageBuild result;
    result.references = previous;

    for (auto& reference : result.references) {
        if (!reference.valid()) {
            return std::nullopt;
        }
        if (!isActiveReferenceStatus(
                reference.status)) {
            reference.current_vertices.clear();
            reference.candidate_vertex_count = 0U;
            continue;
        }
        if (reference.current_vertices.empty()) {
            return std::nullopt;
        }

        std::vector<kernel::RuntimeVertexToken>
            descendants;
        for (const auto source :
             reference.current_vertices) {
            const auto* lineage =
                findInheritedVertexLineage(
                    source,
                    kernel_result);
            if (lineage == nullptr ||
                !lineage->source_token.valid() ||
                lineage->candidate_count !=
                    lineage->current_vertices.size()) {
                return std::nullopt;
            }
            for (const auto token :
                 lineage->current_vertices) {
                appendUniqueRuntimeToken(
                    descendants,
                    token);
            }
        }

        bool relation_preserved =
            !descendants.empty();
        for (const auto token : descendants) {
            const auto* meaning =
                findCurrentVertexMeaning(
                    token,
                    meanings);
            if (meaning == nullptr ||
                !meaning->relation ||
                !samePointRelation(
                    reference,
                    *meaning->relation)) {
                relation_preserved = false;
                break;
            }
        }

        if (!relation_preserved) {
            reference.status =
                kernel::ReferenceStatus::missing;
            reference.candidate_vertex_count = 0U;
            reference.current_vertices.clear();
            continue;
        }

        reference.current_vertices =
            std::move(descendants);
        reference.candidate_vertex_count =
            reference.current_vertices.size();
        reference.status =
            strictReferenceStatus(
                reference.candidate_vertex_count);
    }

    for (std::size_t first = 0U;
         first < result.references.size();
         ++first) {
        if (!isActiveReferenceStatus(
                result.references[first].status)) {
            continue;
        }
        for (std::size_t second = first + 1U;
             second < result.references.size();
             ++second) {
            if (!isActiveReferenceStatus(
                    result.references[second].status)) {
                continue;
            }
            bool shared = false;
            for (const auto token :
                 result.references[first].current_vertices) {
                if (std::find(
                        result.references[second]
                            .current_vertices.begin(),
                        result.references[second]
                            .current_vertices.end(),
                        token) !=
                    result.references[second]
                        .current_vertices.end()) {
                    shared = true;
                    break;
                }
            }
            if (shared) {
                result.references[first].status =
                    kernel::ReferenceStatus::
                        ambiguous;
                result.references[second].status =
                    kernel::ReferenceStatus::
                        ambiguous;
            }
        }
    }

    std::vector<kernel::RuntimeVertexToken>
        inherited_claimed;
    for (const auto& reference :
         result.references) {
        if (!isActiveReferenceStatus(
                reference.status)) {
            continue;
        }
        for (const auto token :
             reference.current_vertices) {
            appendUniqueRuntimeToken(
                inherited_claimed,
                token);
        }
    }

    struct NewPointGroup final {
        PointRelation relation;
        std::vector<kernel::RuntimeVertexToken>
            vertices;
    };
    std::vector<NewPointGroup> new_groups;

    for (const auto& meaning : meanings) {
        if (!meaning.relation ||
            std::find(
                inherited_claimed.begin(),
                inherited_claimed.end(),
                meaning.token) !=
                inherited_claimed.end()) {
            continue;
        }

        auto group =
            std::find_if(
                new_groups.begin(),
                new_groups.end(),
                [&meaning](const auto& item) {
                    return item.relation ==
                           *meaning.relation;
                });
        if (group == new_groups.end()) {
            new_groups.push_back(
                {
                    *meaning.relation,
                    {meaning.token},
                });
        } else {
            appendUniqueRuntimeToken(
                group->vertices,
                meaning.token);
        }
    }

    for (auto& group : new_groups) {
        FeaturePointResolution reference;
        reference.address.producer_feature_id =
            current_feature;
        reference.address.adjacent_surfaces =
            group.relation.adjacent_surfaces;
        reference.current_vertices =
            std::move(group.vertices);
        reference.candidate_vertex_count =
            reference.current_vertices.size();
        reference.status =
            strictReferenceStatus(
                reference.candidate_vertex_count);
        if (!reference.valid()) {
            return std::nullopt;
        }
        result.produced.push_back(reference);
        result.references.push_back(
            std::move(reference));
    }

    result.records.reserve(
        meanings.size());
    for (const auto& meaning : meanings) {
        BodyVertexTopologyRecord record;
        record.runtime_token = meaning.token;
        record.incident_material_edges =
            meaning.incident_material_edges;
        record.incident_material_edge_count =
            record.incident_material_edges.size();
        record.provider_point =
            meaning.provider_point;

        if (!meaning.relation) {
            record.accounting_class =
                TopologyAccountingClass::
                    semantically_unsupported;
            record.referenceability =
                kernel::ReferenceStatus::
                    unsupported;
            result.records.push_back(
                std::move(record));
            continue;
        }

        std::vector<kernel::ReferenceStatus>
            statuses;
        for (const auto& reference :
             result.references) {
            if (!isActiveReferenceStatus(
                    reference.status) ||
                std::find(
                    reference.current_vertices.begin(),
                    reference.current_vertices.end(),
                    meaning.token) ==
                    reference.current_vertices.end()) {
                continue;
            }
            if (std::find(
                    record.point_candidates.begin(),
                    record.point_candidates.end(),
                    reference.address) ==
                record.point_candidates.end()) {
                record.point_candidates.push_back(
                    reference.address);
                statuses.push_back(
                    reference.status);
            }
        }

        if (record.point_candidates.empty()) {
            return std::nullopt;
        }

        record.accounting_class =
            TopologyAccountingClass::
                referenceable;
        if (record.point_candidates.size() > 1U ||
            std::any_of(
                statuses.begin(),
                statuses.end(),
                [](kernel::ReferenceStatus status) {
                    return status ==
                           kernel::ReferenceStatus::
                               ambiguous;
                })) {
            record.referenceability =
                kernel::ReferenceStatus::
                    ambiguous;
        } else {
            record.referenceability =
                kernel::ReferenceStatus::
                    resolved;
        }
        result.records.push_back(
            std::move(record));
    }

    return result;
}

template <typename Token>
[[nodiscard]] bool uniqueValidTokens(
    const std::vector<Token>& tokens) noexcept {
    for (std::size_t index = 0U;
         index < tokens.size();
         ++index) {
        if (!tokens[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < tokens.size();
             ++other) {
            if (tokens[index] == tokens[other]) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] std::optional<BodyStageTopologyCatalog>
makeBodyStageTopologyCatalog(
    FeatureId feature_id,
    const kernel::SolidModelingResult& kernel_result,
    const std::vector<FeatureFaceResolution>&
        semantic_faces,
    const std::vector<FeatureSurfaceResolution>&
        semantic_surfaces,
    const CurveStageBuild& curve_stage,
    const PointStageBuild& point_stage) {
    if (!feature_id.valid() ||
        !kernel_result.ok() ||
        kernel_result.face_count !=
            kernel_result.current_faces.size() ||
        kernel_result.edge_count !=
            kernel_result.current_edges.size() ||
        kernel_result.vertex_count !=
            kernel_result.current_vertices.size() ||
        !uniqueValidTokens(
            kernel_result.current_faces) ||
        !uniqueValidTokens(
            kernel_result.current_edges) ||
        !uniqueValidTokens(
            kernel_result.current_vertices)) {
        return std::nullopt;
    }

    BodyStageTopologyCatalog result;
    result.stage.kind =
        BodyStageKind::after_feature;
    result.stage.feature_id = feature_id;
    result.surfaces = semantic_surfaces;

    for (const auto& surface :
         result.surfaces) {
        if (!surface.valid()) {
            return std::nullopt;
        }
        for (const auto token :
             surface.current_faces) {
            if (std::count(
                    kernel_result.current_faces.begin(),
                    kernel_result.current_faces.end(),
                    token) != 1) {
                return std::nullopt;
            }
        }
    }

    result.faces.reserve(
        kernel_result.current_faces.size());
    for (const auto token :
         kernel_result.current_faces) {
        std::size_t strict_claim_count = 0U;
        std::optional<FeatureFaceAddress>
            strict_address;

        for (const auto& reference :
             semantic_faces) {
            if (reference.status !=
                    kernel::ReferenceStatus::
                        resolved ||
                !reference.runtime_token ||
                *reference.runtime_token != token) {
                continue;
            }
            ++strict_claim_count;
            strict_address =
                reference.address;
        }

        if (strict_claim_count > 1U) {
            return std::nullopt;
        }

        BodyFaceTopologyRecord record;
        record.runtime_token = token;
        record.semantic_address =
            std::move(strict_address);

        for (const auto& surface :
             semantic_surfaces) {
            if (surface.status ==
                    kernel::ReferenceStatus::
                        missing ||
                surface.status ==
                    kernel::ReferenceStatus::
                        unsupported) {
                continue;
            }
            if (std::find(
                    surface.current_faces.begin(),
                    surface.current_faces.end(),
                    token) ==
                surface.current_faces.end()) {
                continue;
            }
            if (std::find(
                    record.surface_candidates.begin(),
                    record.surface_candidates.end(),
                    surface.address) ==
                record.surface_candidates.end()) {
                record.surface_candidates.push_back(
                    surface.address);
            }
        }

        for (const auto& produced :
             kernel_result.new_surfaces) {
            if (!produced.continued_into ||
                std::find(
                    produced.contribution_faces.begin(),
                    produced.contribution_faces.end(),
                    token) ==
                    produced.contribution_faces.end()) {
                continue;
            }

            const auto owner =
                semanticSurfaceForRuntimeToken(
                    *produced.continued_into,
                    semantic_surfaces);
            if (!owner) {
                return std::nullopt;
            }

            if (std::find(
                    record.contributing_features.begin(),
                    record.contributing_features.end(),
                    feature_id) ==
                record.contributing_features.end()) {
                record.contributing_features.push_back(
                    feature_id);
            }
        }

        if (!record.surface_candidates.empty()) {
            record.accounting_class =
                TopologyAccountingClass::
                    referenceable;
        } else if (record.semantic_address) {
            // A strict semantic Face without a carrier is an incomplete
            // PM-02B semantic claim, not a valid fallback.
            return std::nullopt;
        } else {
            record.accounting_class =
                TopologyAccountingClass::
                    semantically_unsupported;
        }

        result.faces.push_back(
            std::move(record));
    }

    // Every current supported Extrude Add/Cut Face must map to at least one
    // semantic Surface carrier. Multiple candidates are explicit ambiguity.
    if (!result.faces.empty() &&
        std::any_of(
            result.faces.begin(),
            result.faces.end(),
            [](const BodyFaceTopologyRecord& face) {
                return face.surface_candidates.empty();
            })) {
        return std::nullopt;
    }

    // Every semantic Face still reported Resolved must belong to exactly one
    // current provider Face in this same stage inventory.
    for (const auto& reference :
         semantic_faces) {
        if (reference.status !=
            kernel::ReferenceStatus::resolved) {
            continue;
        }
        if (!reference.runtime_token ||
            std::count(
                kernel_result.current_faces.begin(),
                kernel_result.current_faces.end(),
                *reference.runtime_token) != 1) {
            return std::nullopt;
        }
    }

    result.edges = curve_stage.records;
    result.vertices = point_stage.records;
    result.curves = curve_stage.references;
    result.points = point_stage.references;

    if (result.edges.size() !=
            kernel_result.current_edges.size() ||
        result.vertices.size() !=
            kernel_result.current_vertices.size()) {
        return std::nullopt;
    }

    for (const auto token :
         kernel_result.current_edges) {
        if (std::count_if(
                result.edges.begin(),
                result.edges.end(),
                [token](const auto& record) {
                    return record.runtime_token ==
                           token;
                }) != 1) {
            return std::nullopt;
        }
    }
    for (const auto token :
         kernel_result.current_vertices) {
        if (std::count_if(
                result.vertices.begin(),
                result.vertices.end(),
                [token](const auto& record) {
                    return record.runtime_token ==
                           token;
                }) != 1) {
            return std::nullopt;
        }
    }

    return result.complete()
        ? std::optional<BodyStageTopologyCatalog>{
              std::move(result)}
        : std::nullopt;
}

} // namespace

bool FeatureFaceAddress::valid() const noexcept {
    if (!producer_feature_id.valid()) {
        return false;
    }
    if (role == FeatureFaceRoleKind::side ||
        role == FeatureFaceRoleKind::revolve_side) {
        return source_entity.has_value() &&
               source_entity->valid();
    }
    return !source_entity.has_value();
}

bool FeatureSurfaceResolution::valid() const noexcept {
    if (!address.valid() ||
        candidate_face_count !=
            current_faces.size()) {
        return false;
    }

    for (std::size_t index = 0U;
         index < current_faces.size();
         ++index) {
        if (!current_faces[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < current_faces.size();
             ++other) {
            if (current_faces[index] ==
                current_faces[other]) {
                return false;
            }
        }
    }

    if (status ==
        kernel::ReferenceStatus::resolved) {
        if (surface_kind ==
            kernel::SurfaceKind::plane) {
            if (!canonical_frame ||
                !canonical_frame->valid()) {
                return false;
            }
        } else if (canonical_frame) {
            return false;
        }
    } else if (canonical_frame) {
        return false;
    }

    switch (status) {
    case kernel::ReferenceStatus::resolved:
        if (!runtime_token ||
            !runtime_token->valid() ||
            candidate_face_count == 0U) {
            return false;
        }
        return candidate_face_count == 1U
            ? strict_face_status ==
                  kernel::ReferenceStatus::
                      resolved
            : strict_face_status ==
                  kernel::ReferenceStatus::
                      ambiguous;
    case kernel::ReferenceStatus::missing:
        return !runtime_token &&
               candidate_face_count == 0U &&
               strict_face_status ==
                   kernel::ReferenceStatus::
                       missing;
    case kernel::ReferenceStatus::ambiguous:
        return !runtime_token &&
               candidate_face_count > 0U &&
               strict_face_status ==
                   kernel::ReferenceStatus::
                       ambiguous;
    case kernel::ReferenceStatus::unsupported:
        return !runtime_token &&
               candidate_face_count == 0U &&
               current_faces.empty() &&
               strict_face_status ==
                   kernel::ReferenceStatus::
                       unsupported;
    }
    return false;
}

bool FeatureCurveAddress::valid() const noexcept {
    if (!producer_feature_id.valid() ||
        adjacent_surfaces.size() != 2U ||
        adjacent_surfaces[0] ==
            adjacent_surfaces[1] ||
        !std::is_sorted(
            adjacent_surfaces.begin(),
            adjacent_surfaces.end())) {
        return false;
    }
    return adjacent_surfaces[0].valid() &&
           adjacent_surfaces[1].valid();
}

bool FeatureCurveResolution::valid() const noexcept {
    if (!address.valid() ||
        curve_kind == kernel::CurveKind::other ||
        candidate_edge_count !=
            current_edges.size()) {
        return false;
    }
    if (!uniqueValidTokens(current_edges)) {
        return false;
    }

    switch (status) {
    case kernel::ReferenceStatus::resolved:
        if (candidate_edge_count == 0U) {
            return false;
        }
        return candidate_edge_count == 1U
            ? strict_edge_status ==
                  kernel::ReferenceStatus::resolved
            : strict_edge_status ==
                  kernel::ReferenceStatus::ambiguous;
    case kernel::ReferenceStatus::ambiguous:
        return candidate_edge_count >= 1U &&
               strict_edge_status ==
                   kernel::ReferenceStatus::ambiguous;
    case kernel::ReferenceStatus::missing:
        return candidate_edge_count == 0U &&
               current_edges.empty() &&
               strict_edge_status ==
                   kernel::ReferenceStatus::missing;
    case kernel::ReferenceStatus::unsupported:
        return candidate_edge_count == 0U &&
               current_edges.empty() &&
               strict_edge_status ==
                   kernel::ReferenceStatus::unsupported;
    }
    return false;
}

bool FeaturePointAddress::valid() const noexcept {
    if (!producer_feature_id.valid() ||
        adjacent_surfaces.size() != 3U ||
        !std::is_sorted(
            adjacent_surfaces.begin(),
            adjacent_surfaces.end())) {
        return false;
    }
    for (std::size_t index = 0U;
         index < adjacent_surfaces.size();
         ++index) {
        if (!adjacent_surfaces[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < adjacent_surfaces.size();
             ++other) {
            if (adjacent_surfaces[index] ==
                adjacent_surfaces[other]) {
                return false;
            }
        }
    }
    return true;
}

bool FeaturePointResolution::valid() const noexcept {
    if (!address.valid() ||
        candidate_vertex_count !=
            current_vertices.size() ||
        !uniqueValidTokens(current_vertices)) {
        return false;
    }
    switch (status) {
    case kernel::ReferenceStatus::resolved:
        return candidate_vertex_count == 1U;
    case kernel::ReferenceStatus::ambiguous:
        return candidate_vertex_count >= 1U;
    case kernel::ReferenceStatus::missing:
    case kernel::ReferenceStatus::unsupported:
        return candidate_vertex_count == 0U &&
               current_vertices.empty();
    }
    return false;
}

bool BodyFaceTopologyRecord::valid() const noexcept {
    if (!runtime_token.valid()) {
        return false;
    }

    if (semantic_address &&
        !semantic_address->valid()) {
        return false;
    }
    for (std::size_t index = 0U;
         index < surface_candidates.size();
         ++index) {
        if (!surface_candidates[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < surface_candidates.size();
             ++other) {
            if (surface_candidates[index] ==
                surface_candidates[other]) {
                return false;
            }
        }
    }
    for (std::size_t index = 0U;
         index < contributing_features.size();
         ++index) {
        if (!contributing_features[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < contributing_features.size();
             ++other) {
            if (contributing_features[index] ==
                contributing_features[other]) {
                return false;
            }
        }
    }

    switch (accounting_class) {
    case TopologyAccountingClass::referenceable:
        return !surface_candidates.empty();
    case TopologyAccountingClass::
        known_representation_artifact:
    case TopologyAccountingClass::
        semantically_unsupported:
        return !semantic_address.has_value() &&
               surface_candidates.empty();
    case TopologyAccountingClass::
        integrity_failure:
        return false;
    }
    return false;
}

bool BodyEdgeTopologyRecord::valid() const noexcept {
    if (!runtime_token.valid()) {
        return false;
    }
    for (std::size_t index = 0U;
         index < curve_candidates.size();
         ++index) {
        if (!curve_candidates[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < curve_candidates.size();
             ++other) {
            if (curve_candidates[index] ==
                curve_candidates[other]) {
                return false;
            }
        }
    }

    switch (accounting_class) {
    case TopologyAccountingClass::referenceable:
        return !periodic_seam &&
               !curve_candidates.empty() &&
               curve_kind !=
                   kernel::CurveKind::other &&
               (referenceability ==
                    kernel::ReferenceStatus::resolved ||
                referenceability ==
                    kernel::ReferenceStatus::ambiguous);
    case TopologyAccountingClass::
        known_representation_artifact:
        return (periodic_seam ||
                representation_partition) &&
               curve_candidates.empty() &&
               referenceability ==
                   kernel::ReferenceStatus::
                       unsupported;
    case TopologyAccountingClass::
        semantically_unsupported:
        return !periodic_seam &&
               !representation_partition &&
               curve_candidates.empty() &&
               referenceability ==
                   kernel::ReferenceStatus::
                       unsupported;
    case TopologyAccountingClass::
        integrity_failure:
        return false;
    }
    return false;
}

bool BodyVertexTopologyRecord::valid() const noexcept {
    if (!runtime_token.valid()) {
        return false;
    }
    for (std::size_t index = 0U;
         index < point_candidates.size();
         ++index) {
        if (!point_candidates[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < point_candidates.size();
             ++other) {
            if (point_candidates[index] ==
                point_candidates[other]) {
                return false;
            }
        }
    }
    if (incident_material_edge_count !=
            incident_material_edges.size() ||
        !uniqueValidTokens(
            incident_material_edges)) {
        return false;
    }
    if (provider_point &&
        (!std::isfinite(provider_point->x) ||
         !std::isfinite(provider_point->y) ||
         !std::isfinite(provider_point->z))) {
        return false;
    }

    switch (accounting_class) {
    case TopologyAccountingClass::referenceable:
        return !point_candidates.empty() &&
               (referenceability ==
                    kernel::ReferenceStatus::resolved ||
                referenceability ==
                    kernel::ReferenceStatus::ambiguous);
    case TopologyAccountingClass::
        semantically_unsupported:
        return point_candidates.empty() &&
               referenceability ==
                   kernel::ReferenceStatus::
                       unsupported;
    case TopologyAccountingClass::
        known_representation_artifact:
        return point_candidates.empty() &&
               referenceability ==
                   kernel::ReferenceStatus::
                       unsupported;
    case TopologyAccountingClass::
        integrity_failure:
        return false;
    }
    return false;
}

bool BodyStageTopologyCatalog::valid() const noexcept {
    if (!stage.valid()) {
        return false;
    }

    if (stage.kind ==
        BodyStageKind::empty_body) {
        return faces.empty() &&
               edges.empty() &&
               vertices.empty() &&
               surfaces.empty() &&
               curves.empty() &&
               points.empty();
    }

    for (std::size_t index = 0U;
         index < faces.size();
         ++index) {
        if (!faces[index].valid()) {
            return false;
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

    for (std::size_t index = 0U;
         index < surfaces.size();
         ++index) {
        if (!surfaces[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < surfaces.size();
             ++other) {
            if (surfaces[index].address ==
                surfaces[other].address) {
                return false;
            }
            if (surfaces[index].runtime_token &&
                surfaces[other].runtime_token &&
                surfaces[index].runtime_token ==
                    surfaces[other].runtime_token) {
                return false;
            }
        }
    }

    for (std::size_t index = 0U;
         index < curves.size();
         ++index) {
        if (!curves[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < curves.size();
             ++other) {
            if (curves[index].address ==
                curves[other].address) {
                return false;
            }
        }
    }

    for (std::size_t index = 0U;
         index < points.size();
         ++index) {
        if (!points[index].valid()) {
            return false;
        }
        for (std::size_t other = index + 1U;
             other < points.size();
             ++other) {
            if (points[index].address ==
                points[other].address) {
                return false;
            }
        }
    }

    return true;
}

bool BodyStageTopologyCatalog::complete() const noexcept {
    return valid();
}

namespace {

[[nodiscard]] const FeatureCurveResolution*
findCurveResolution(
    const BodyStageTopologyCatalog& catalog,
    const FeatureCurveAddress& address) noexcept {
    const auto found =
        std::find_if(
            catalog.curves.begin(),
            catalog.curves.end(),
            [&address](
                const FeatureCurveResolution& item) {
                return item.address == address;
            });
    return found == catalog.curves.end()
        ? nullptr
        : &*found;
}

[[nodiscard]] const FeaturePointResolution*
findPointResolution(
    const BodyStageTopologyCatalog& catalog,
    const FeaturePointAddress& address) noexcept {
    const auto found =
        std::find_if(
            catalog.points.begin(),
            catalog.points.end(),
            [&address](
                const FeaturePointResolution& item) {
                return item.address == address;
            });
    return found == catalog.points.end()
        ? nullptr
        : &*found;
}

[[nodiscard]] const BodyEdgeTopologyRecord*
findEdgeRecord(
    const BodyStageTopologyCatalog& catalog,
    kernel::RuntimeEdgeToken token) noexcept {
    const auto found =
        std::find_if(
            catalog.edges.begin(),
            catalog.edges.end(),
            [token](
                const BodyEdgeTopologyRecord& item) {
                return item.runtime_token == token;
            });
    return found == catalog.edges.end()
        ? nullptr
        : &*found;
}

[[nodiscard]] const BodyVertexTopologyRecord*
findVertexRecord(
    const BodyStageTopologyCatalog& catalog,
    kernel::RuntimeVertexToken token) noexcept {
    const auto found =
        std::find_if(
            catalog.vertices.begin(),
            catalog.vertices.end(),
            [token](
                const BodyVertexTopologyRecord& item) {
                return item.runtime_token == token;
            });
    return found == catalog.vertices.end()
        ? nullptr
        : &*found;
}

[[nodiscard]] bool vertexCarriesEdge(
    const BodyStageTopologyCatalog& catalog,
    const std::vector<kernel::RuntimeVertexToken>&
        vertices,
    kernel::RuntimeEdgeToken edge) noexcept {
    for (const auto vertex : vertices) {
        const auto* record =
            findVertexRecord(
                catalog,
                vertex);
        if (record == nullptr) {
            continue;
        }
        if (std::find(
                record->incident_material_edges.begin(),
                record->incident_material_edges.end(),
                edge) !=
            record->incident_material_edges.end()) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] kernel::ReferenceStatus
statusForCandidateCount(
    std::size_t count) noexcept {
    if (count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (count == 1U) {
        return kernel::ReferenceStatus::resolved;
    }
    return kernel::ReferenceStatus::ambiguous;
}

} // namespace

std::optional<MaterialEdgeResolution>
resolveMaterialEdgeReference(
    const MaterialEdgeReference& reference,
    const BodyStageTopologyCatalog& catalog) {
    if (!reference.valid() ||
        !catalog.complete() ||
        catalog.stage != reference.stage) {
        return std::nullopt;
    }

    const auto* curve =
        findCurveResolution(
            catalog,
            reference.curve);
    if (curve == nullptr) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::missing,
            {}};
    }
    if (curve->status ==
        kernel::ReferenceStatus::unsupported) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::unsupported,
            {}};
    }
    if (curve->status ==
        kernel::ReferenceStatus::missing) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::missing,
            {}};
    }

    if (std::holds_alternative<
            SingularAtAuthoredStage>(
                reference.branch)) {
        if (curve->current_edges.empty()) {
            return MaterialEdgeResolution{
                kernel::ReferenceStatus::missing,
                {}};
        }
        if (curve->current_edges.size() != 1U ||
            curve->status ==
                kernel::ReferenceStatus::ambiguous) {
            return MaterialEdgeResolution{
                kernel::ReferenceStatus::ambiguous,
                curve->current_edges};
        }

        const auto* record =
            findEdgeRecord(
                catalog,
                curve->current_edges.front());
        if (record == nullptr) {
            return std::nullopt;
        }
        if (record->accounting_class !=
                TopologyAccountingClass::referenceable ||
            record->periodic_seam ||
            record->representation_partition) {
            return MaterialEdgeResolution{
                kernel::ReferenceStatus::unsupported,
                {}};
        }
        if (record->curve_candidates.size() != 1U ||
            record->curve_candidates.front() !=
                reference.curve) {
            return MaterialEdgeResolution{
                kernel::ReferenceStatus::ambiguous,
                curve->current_edges};
        }
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::resolved,
            curve->current_edges};
    }

    const auto* endpoints =
        std::get_if<BetweenSemanticPoints>(
            &reference.branch);
    if (endpoints == nullptr) {
        return std::nullopt;
    }

    const auto* first =
        findPointResolution(
            catalog,
            endpoints->first);
    const auto* second =
        findPointResolution(
            catalog,
            endpoints->second);
    if (first == nullptr ||
        second == nullptr) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::missing,
            {}};
    }
    if (first->status ==
            kernel::ReferenceStatus::unsupported ||
        second->status ==
            kernel::ReferenceStatus::unsupported) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::unsupported,
            {}};
    }
    if (first->status ==
            kernel::ReferenceStatus::missing ||
        second->status ==
            kernel::ReferenceStatus::missing) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::missing,
            {}};
    }
    if (first->status ==
            kernel::ReferenceStatus::ambiguous ||
        second->status ==
            kernel::ReferenceStatus::ambiguous) {
        return MaterialEdgeResolution{
            kernel::ReferenceStatus::ambiguous,
            {}};
    }

    std::vector<kernel::RuntimeEdgeToken>
        candidates;
    bool ambiguous_curve_provenance = false;
    for (const auto edge :
         curve->current_edges) {
        if (!vertexCarriesEdge(
                catalog,
                first->current_vertices,
                edge) ||
            !vertexCarriesEdge(
                catalog,
                second->current_vertices,
                edge)) {
            continue;
        }

        const auto* record =
            findEdgeRecord(
                catalog,
                edge);
        if (record == nullptr) {
            return std::nullopt;
        }
        if (record->accounting_class !=
                TopologyAccountingClass::referenceable ||
            record->periodic_seam ||
            record->representation_partition) {
            return MaterialEdgeResolution{
                kernel::ReferenceStatus::unsupported,
                {}};
        }
        if (std::find(
                record->curve_candidates.begin(),
                record->curve_candidates.end(),
                reference.curve) ==
            record->curve_candidates.end()) {
            return std::nullopt;
        }
        ambiguous_curve_provenance =
            ambiguous_curve_provenance ||
            record->curve_candidates.size() != 1U;
        appendUniqueRuntimeToken(
            candidates,
            edge);
    }

    auto status =
        statusForCandidateCount(
            candidates.size());
    if (status ==
            kernel::ReferenceStatus::resolved &&
        ambiguous_curve_provenance) {
        status =
            kernel::ReferenceStatus::ambiguous;
    }
    return MaterialEdgeResolution{
        status,
        std::move(candidates)};
}

MaterialEdgeAuthoringResult
authorMaterialEdgeReference(
    const BodyStageTopologyCatalog& catalog,
    kernel::RuntimeEdgeToken edge) {
    MaterialEdgeAuthoringResult result;
    if (!edge.valid() ||
        !catalog.complete() ||
        catalog.stage.kind !=
            BodyStageKind::after_feature) {
        return result;
    }

    const auto* record =
        findEdgeRecord(
            catalog,
            edge);
    if (record == nullptr ||
        record->accounting_class !=
            TopologyAccountingClass::referenceable ||
        record->periodic_seam ||
        record->representation_partition ||
        record->curve_candidates.size() != 1U) {
        return result;
    }

    const auto& curve_address =
        record->curve_candidates.front();
    const auto* curve =
        findCurveResolution(
            catalog,
            curve_address);
    if (curve == nullptr ||
        std::find(
            curve->current_edges.begin(),
            curve->current_edges.end(),
            edge) ==
            curve->current_edges.end()) {
        return result;
    }

    MaterialEdgeReference authored{
        catalog.stage,
        curve_address,
        SingularAtAuthoredStage{}};

    if (curve->current_edges.size() != 1U) {
        std::vector<FeaturePointAddress>
            endpoint_points;
        std::size_t incident_vertices = 0U;

        for (const auto& vertex :
             catalog.vertices) {
            if (std::find(
                    vertex.incident_material_edges.begin(),
                    vertex.incident_material_edges.end(),
                    edge) ==
                vertex.incident_material_edges.end()) {
                continue;
            }
            ++incident_vertices;

            if (vertex.accounting_class !=
                    TopologyAccountingClass::referenceable ||
                vertex.referenceability !=
                    kernel::ReferenceStatus::resolved ||
                vertex.point_candidates.size() != 1U) {
                continue;
            }

            const auto& point_address =
                vertex.point_candidates.front();
            const auto* point =
                findPointResolution(
                    catalog,
                    point_address);
            if (point == nullptr ||
                point->status !=
                    kernel::ReferenceStatus::resolved ||
                point->current_vertices.size() != 1U ||
                point->current_vertices.front() !=
                    vertex.runtime_token) {
                continue;
            }

            if (std::find(
                    endpoint_points.begin(),
                    endpoint_points.end(),
                    point_address) ==
                endpoint_points.end()) {
                endpoint_points.push_back(
                    point_address);
            }
        }

        if (incident_vertices != 2U ||
            endpoint_points.size() != 2U) {
            return result;
        }
        std::sort(
            endpoint_points.begin(),
            endpoint_points.end());
        if (endpoint_points[0] ==
            endpoint_points[1]) {
            return result;
        }
        authored.branch =
            BetweenSemanticPoints{
                endpoint_points[0],
                endpoint_points[1]};
    }

    const auto resolution =
        resolveMaterialEdgeReference(
            authored,
            catalog);
    if (!resolution ||
        !resolution->resolved() ||
        resolution->current_edges.front() !=
            edge) {
        return result;
    }

    result.status =
        kernel::ReferenceStatus::resolved;
    result.reference =
        std::move(authored);
    return result;
}

bool FeatureContribution::valid() const noexcept {
    if (!uniqueValidTokens(faces) ||
        !uniqueValidTokens(direct_edges) ||
        !uniqueValidTokens(direct_vertices) ||
        !uniqueValidTokens(boundary_edges) ||
        !uniqueValidTokens(boundary_vertices)) {
        return false;
    }

    for (const auto token : direct_edges) {
        if (std::find(
                boundary_edges.begin(),
                boundary_edges.end(),
                token) != boundary_edges.end()) {
            return false;
        }
    }
    for (const auto token : direct_vertices) {
        if (std::find(
                boundary_vertices.begin(),
                boundary_vertices.end(),
                token) != boundary_vertices.end()) {
            return false;
        }
    }
    return true;
}

FeatureContribution currentFeatureContribution(
    const BodyStageTopologyCatalog& catalog,
    FeatureId feature_id) {
    FeatureContribution result;
    if (!catalog.complete() ||
        !feature_id.valid()) {
        return result;
    }

    for (const auto& face : catalog.faces) {
        const bool explicit_contribution =
            std::find(
                face.contributing_features.begin(),
                face.contributing_features.end(),
                feature_id) !=
            face.contributing_features.end();
        const bool owned_surface =
            std::any_of(
                face.surface_candidates.begin(),
                face.surface_candidates.end(),
                [feature_id](const auto& surface) {
                    return surface.producer_feature_id ==
                           feature_id;
                });
        if (explicit_contribution ||
            owned_surface) {
            result.faces.push_back(
                face.runtime_token);
        }
    }

    for (const auto& edge : catalog.edges) {
        const bool direct =
            std::any_of(
                edge.curve_candidates.begin(),
                edge.curve_candidates.end(),
                [feature_id](const auto& curve) {
                    return curve.producer_feature_id ==
                           feature_id;
                });
        if (direct) {
            result.direct_edges.push_back(
                edge.runtime_token);
            continue;
        }

        const bool boundary =
            std::any_of(
                edge.curve_candidates.begin(),
                edge.curve_candidates.end(),
                [feature_id](const auto& curve) {
                    return std::any_of(
                        curve.adjacent_surfaces.begin(),
                        curve.adjacent_surfaces.end(),
                        [feature_id](const auto& surface) {
                            return surface.producer_feature_id ==
                                   feature_id;
                        });
                });
        if (boundary) {
            result.boundary_edges.push_back(
                edge.runtime_token);
        }
    }

    for (const auto& vertex : catalog.vertices) {
        const bool direct =
            std::any_of(
                vertex.point_candidates.begin(),
                vertex.point_candidates.end(),
                [feature_id](const auto& point) {
                    return point.producer_feature_id ==
                           feature_id;
                });
        if (direct) {
            result.direct_vertices.push_back(
                vertex.runtime_token);
            continue;
        }

        const bool boundary =
            std::any_of(
                vertex.point_candidates.begin(),
                vertex.point_candidates.end(),
                [feature_id](const auto& point) {
                    return std::any_of(
                        point.adjacent_surfaces.begin(),
                        point.adjacent_surfaces.end(),
                        [feature_id](const auto& surface) {
                            return surface.producer_feature_id ==
                                   feature_id;
                        });
                });
        if (boundary) {
            result.boundary_vertices.push_back(
                vertex.runtime_token);
        }
    }

    return result.valid()
        ? result
        : FeatureContribution{};
}

const FeatureEvaluation*
PartEvaluation::findFeature(
    FeatureId id) const noexcept {
    const auto found =
        std::find_if(
            features.begin(),
            features.end(),
            [id](const FeatureEvaluation& item) {
                return item.feature_id == id;
            });
    return found == features.end()
        ? nullptr
        : &*found;
}

PartEvaluation evaluatePart(
    const PartDocument& document,
    kernel::ISolidModelingKernel& modeling_kernel) {
    PartEvaluation result;
    result.source_revision =
        document.revision();
    result.features.reserve(
        document.body().features.size());

    kernel::RuntimeSolidHandle current_solid;
    std::optional<BodyStageTopologyCatalog>
        current_topology;
    std::vector<FeatureFaceResolution>
        current_references;
    std::vector<FeatureSurfaceResolution>
        current_surfaces;
    std::vector<FeatureCurveResolution>
        current_curves;
    std::vector<FeaturePointResolution>
        current_points;
    bool chain_broken = false;

    for (const auto& authored :
         document.body().features) {
        FeatureEvaluation evaluated;
        evaluated.feature_id = authored.id;

        if (authored.suppressed) {
            evaluated.status =
                FeatureEvaluationStatus::
                    suppressed;
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        if (chain_broken) {
            evaluated.status =
                FeatureEvaluationStatus::
                    blocked;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    upstream_unavailable;
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const auto* extrude =
            std::get_if<ExtrudeFeature>(
                &authored.definition);
        const auto* revolve =
            std::get_if<RevolveFeature>(
                &authored.definition);
        if (extrude == nullptr &&
            revolve == nullptr) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    kernel_invalid_input;
            chain_broken = true;
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const ProfileId profile_id =
            extrude != nullptr
                ? extrude->profile_id
                : revolve->profile_id;
        const auto* profile =
            document.findProfile(
                profile_id);
        if (profile == nullptr) {
            evaluated.status =
                FeatureEvaluationStatus::
                    blocked;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    missing_profile;
            chain_broken = true;
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const BodyStageTopologyCatalog*
            support_topology = nullptr;
        std::optional<DatumEvaluation>
            support_datums;
        if (const auto* source =
                document.findSketch(
                    profile->source_sketch_id)) {
            if (const auto* surface =
                    bodyPlanarSurfaceReference(
                        source->support)) {
                const auto stage =
                    std::find_if(
                        result.features.begin(),
                        result.features.end(),
                        [surface](
                            const FeatureEvaluation& prior) {
                            return prior.result_topology &&
                                   prior.result_topology
                                       ->stage ==
                                       surface->stage;
                        });
                if (stage != result.features.end()) {
                    support_topology =
                        &*stage->result_topology;
                }
            } else if (
                datumPlaneIdForSketchSupport(
                    source->support)) {
                support_datums =
                    evaluateDatums(
                        document,
                        result);
            }
        }

        // Authored Axis may itself live on a Datum-backed Sketch even when
        // the consuming Profile does not. Revolve therefore evaluates the
        // same-revision Datum prefix unconditionally; resolution remains
        // demand-driven and fail-closed.
        if (revolve != nullptr &&
            !support_datums) {
            support_datums =
                evaluateDatums(
                    document,
                    result);
        }

        kernel::SolidModelingResult
            kernel_result;

        if (extrude != nullptr) {
            auto materialized_profile =
                resolveKernelProfileInput(
                    document,
                    profile->id,
                    support_topology,
                    support_datums
                        ? &*support_datums
                        : nullptr);
            if (!materialized_profile.ok()) {
                evaluated.status =
                    materialized_profile.status ==
                            ProfileKernelInputStatus::
                                invalid_input
                        ? FeatureEvaluationStatus::
                              failed
                        : FeatureEvaluationStatus::
                              blocked;
                evaluated.diagnostic =
                    diagnosticForProfileMaterialization(
                        materialized_profile.status);
                chain_broken = true;
                current_references.clear();
                result.features.push_back(
                    std::move(evaluated));
                continue;
            }

            if (extrude->operation ==
                    ExtrudeOperation::cut &&
                current_solid == nullptr) {
                evaluated.status =
                    FeatureEvaluationStatus::
                        blocked;
                evaluated.diagnostic =
                    FeatureEvaluationDiagnosticCode::
                        missing_upstream_body;
                chain_broken = true;
                current_references.clear();
                result.features.push_back(
                    std::move(evaluated));
                continue;
            }

            auto input =
                makeKernelExtrudeInputFromProfile(
                    std::move(
                        *materialized_profile.input),
                    *extrude);
            if (!input) {
                evaluated.status =
                    FeatureEvaluationStatus::
                        failed;
                evaluated.diagnostic =
                    FeatureEvaluationDiagnosticCode::
                        kernel_invalid_input;
                chain_broken = true;
                current_references.clear();
                result.features.push_back(
                    std::move(evaluated));
                continue;
            }

            kernel_result =
                modeling_kernel.extrude(
                    *input,
                    current_solid);
        } else {
            if (revolve->operation ==
                    RevolveOperation::cut &&
                current_solid == nullptr) {
                evaluated.status =
                    FeatureEvaluationStatus::
                        blocked;
                evaluated.diagnostic =
                    FeatureEvaluationDiagnosticCode::
                        missing_upstream_body;
                chain_broken = true;
                current_references.clear();
                result.features.push_back(
                    std::move(evaluated));
                continue;
            }

            const auto resolved =
                resolveKernelRevolveInput(
                    document,
                    *revolve,
                    &result,
                    support_datums
                        ? &*support_datums
                        : nullptr);
            if (!resolved.ok()) {
                evaluated.status =
                    resolved.status ==
                            RevolveKernelInputStatus::
                                invalid_input
                        ? FeatureEvaluationStatus::
                              failed
                        : FeatureEvaluationStatus::
                              blocked;
                evaluated.diagnostic =
                    diagnosticForRevolveInput(
                        resolved.status);
                chain_broken = true;
                current_references.clear();
                result.features.push_back(
                    std::move(evaluated));
                continue;
            }

            kernel_result =
                modeling_kernel.revolve(
                    *resolved.input,
                    current_solid);
        }

        evaluated.kernel_status =
            kernel_result.status;
        if (!kernel_result.ok()) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                diagnosticForKernel(
                    kernel_result.status);
            chain_broken = true;
            current_references.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        auto candidate_references =
            current_references;
        propagateCurrentReferences(
            candidate_references,
            kernel_result.inherited_faces);

        evaluated.produced_faces.reserve(
            kernel_result.new_faces.size());
        for (const auto& face :
             kernel_result.new_faces) {
            auto converted =
                convertNewFace(
                    authored.id,
                    face);
            evaluated.produced_faces.push_back(
                converted);
            candidate_references.push_back(
                std::move(converted));
        }

        auto candidate_surfaces =
            current_surfaces;
        if (!propagateCurrentSurfaces(
                candidate_surfaces,
                kernel_result.inherited_surfaces)) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    topology_integrity_failure;
            chain_broken = true;
            current_references.clear();
            current_surfaces.clear();
            current_curves.clear();
            current_points.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        evaluated.produced_surfaces.reserve(
            kernel_result.new_surfaces.size());
        for (const auto& surface :
             kernel_result.new_surfaces) {
            if (surface.continued_into) {
                if (!surface.continued_into->valid() ||
                    surface.contribution_faces.empty()) {
                    evaluated.status =
                        FeatureEvaluationStatus::failed;
                    evaluated.diagnostic =
                        FeatureEvaluationDiagnosticCode::
                            topology_integrity_failure;
                    chain_broken = true;
                    break;
                }
                continue;
            }
            auto converted =
                convertNewSurface(
                    authored.id,
                    surface);
            evaluated.produced_surfaces.push_back(
                converted);
            candidate_surfaces.push_back(
                std::move(converted));
        }
        if (chain_broken) {
            current_references.clear();
            current_surfaces.clear();
            current_curves.clear();
            current_points.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const auto curve_stage =
            buildCurveStage(
                authored.id,
                kernel_result,
                candidate_surfaces,
                current_curves);
        if (!curve_stage) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    topology_integrity_failure;
            chain_broken = true;
            current_references.clear();
            current_surfaces.clear();
            current_curves.clear();
            current_points.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        const auto point_stage =
            buildPointStage(
                authored.id,
                kernel_result,
                candidate_surfaces,
                current_points);
        if (!point_stage) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    topology_integrity_failure;
            chain_broken = true;
            current_references.clear();
            current_surfaces.clear();
            current_curves.clear();
            current_points.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        evaluated.produced_curves =
            curve_stage->produced;
        evaluated.produced_points =
            point_stage->produced;

        auto candidate_topology =
            makeBodyStageTopologyCatalog(
                authored.id,
                kernel_result,
                candidate_references,
                candidate_surfaces,
                *curve_stage,
                *point_stage);
        if (!candidate_topology) {
            evaluated.status =
                FeatureEvaluationStatus::
                    failed;
            evaluated.diagnostic =
                FeatureEvaluationDiagnosticCode::
                    topology_integrity_failure;
            chain_broken = true;
            current_references.clear();
            current_surfaces.clear();
            result.features.push_back(
                std::move(evaluated));
            continue;
        }

        evaluated.status =
            FeatureEvaluationStatus::
                up_to_date;
        evaluated.diagnostic =
            FeatureEvaluationDiagnosticCode::
                none;
        evaluated.result_solid =
            kernel_result.solid;
        evaluated.result_topology =
            *candidate_topology;
        current_references =
            std::move(candidate_references);
        current_surfaces =
            std::move(candidate_surfaces);
        current_curves =
            curve_stage->references;
        current_points =
            point_stage->references;
        current_topology =
            std::move(candidate_topology);
        current_solid =
            std::move(kernel_result.solid);
        result.features.push_back(
            std::move(evaluated));
    }

    if (chain_broken) {
        result.body_status =
            BodyEvaluationStatus::
                unavailable;
        result.body_solid.reset();
        result.resolved_prefix_solid =
            std::move(current_solid);
        result.resolved_prefix_topology =
            std::move(current_topology);
        result.current_topology.reset();
        result.current_face_references.clear();
        return result;
    }

    if (current_solid != nullptr) {
        result.body_status =
            BodyEvaluationStatus::
                up_to_date;
        result.body_solid =
            std::move(current_solid);
        result.current_topology =
            std::move(current_topology);
        result.current_face_references =
            std::move(current_references);
        result.current_surface_references =
            std::move(current_surfaces);
        result.current_curve_references =
            std::move(current_curves);
        result.current_point_references =
            std::move(current_points);
        return result;
    }

    result.body_status =
        BodyEvaluationStatus::empty;
    return result;
}

} // namespace simplesolid2::part
