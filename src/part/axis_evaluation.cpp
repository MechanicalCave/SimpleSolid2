#include <simplesolid2/part/axis_evaluation.hpp>

#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>

#include <cmath>

namespace simplesolid2::part {
namespace {

[[nodiscard]] bool finite(
    const kernel::Point3& point) noexcept {
    return std::isfinite(point.x) &&
           std::isfinite(point.y) &&
           std::isfinite(point.z);
}

[[nodiscard]] double squaredLength(
    const kernel::Point3& point) noexcept {
    return point.x * point.x +
           point.y * point.y +
           point.z * point.z;
}

[[nodiscard]] kernel::Point3 worldPoint(
    const SketchPlacement& frame,
    const sketch::Point2& point) noexcept {
    return {
        frame.origin[0] +
            frame.u_axis[0] * point.u +
            frame.v_axis[0] * point.v,
        frame.origin[1] +
            frame.u_axis[1] * point.u +
            frame.v_axis[1] * point.v,
        frame.origin[2] +
            frame.u_axis[2] * point.u +
            frame.v_axis[2] * point.v};
}

[[nodiscard]] AxisEvaluation unresolved(
    AxisEvaluationStatus status,
    AxisEvaluationDiagnostic diagnostic,
    std::optional<BodyStageRef> required_stage =
        std::nullopt) noexcept {
    return {
        status,
        diagnostic,
        std::nullopt,
        std::move(required_stage)};
}

[[nodiscard]] AxisEvaluation resolved(
    ResolvedAxisLine line,
    std::optional<BodyStageRef> required_stage =
        std::nullopt) noexcept {
    return {
        AxisEvaluationStatus::resolved,
        AxisEvaluationDiagnostic::none,
        std::move(line),
        std::move(required_stage)};
}

[[nodiscard]] AxisEvaluation originAxis(
    core::BuiltinReferenceRole role) noexcept {
    ResolvedAxisLine line;
    switch (role) {
    case core::BuiltinReferenceRole::x_axis:
        line.direction = {1.0, 0.0, 0.0};
        break;
    case core::BuiltinReferenceRole::y_axis:
        line.direction = {0.0, 1.0, 0.0};
        break;
    case core::BuiltinReferenceRole::z_axis:
        line.direction = {0.0, 0.0, 1.0};
        break;
    default:
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }
    return resolved(line);
}

[[nodiscard]] const BodyStageTopologyCatalog*
supportTopology(
    const PartSketch& sketch,
    const PartEvaluation* part_evaluation,
    const PartDocument& document,
    AxisEvaluation& failure) noexcept {
    const auto* surface =
        bodyPlanarSurfaceReference(
            sketch.support);
    if (surface == nullptr) {
        return nullptr;
    }

    if (part_evaluation == nullptr ||
        part_evaluation->source_revision !=
            document.revision()) {
        failure = unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::
                stale_part_evaluation,
            surface->stage);
        return nullptr;
    }

    if (surface->stage.kind !=
            BodyStageKind::after_feature ||
        !surface->stage.feature_id) {
        failure = unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::
                support_stage_unavailable,
            surface->stage);
        return nullptr;
    }

    const auto* feature =
        part_evaluation->findFeature(
            *surface->stage.feature_id);
    if (feature == nullptr ||
        !feature->result_topology ||
        !feature->result_topology->complete() ||
        feature->result_topology->stage !=
            surface->stage) {
        failure = unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::
                support_stage_unavailable,
            surface->stage);
        return nullptr;
    }

    return &*feature->result_topology;
}

[[nodiscard]] std::optional<BodyStageRef>
requiredStage(
    const PartSketch& sketch,
    const DatumEvaluation* datum_evaluation) noexcept {
    if (const auto* surface =
            bodyPlanarSurfaceReference(
                sketch.support)) {
        return surface->stage;
    }

    const auto datum_id =
        datumPlaneIdForSketchSupport(
            sketch.support);
    if (!datum_id ||
        datum_evaluation == nullptr) {
        return std::nullopt;
    }

    const auto* datum =
        datum_evaluation->find(*datum_id);
    return datum != nullptr
        ? datum->required_body_stage
        : std::nullopt;
}

} // namespace

bool ResolvedAxisLine::valid() const noexcept {
    return finite(origin) &&
           finite(direction) &&
           squaredLength(direction) > 0.0;
}

bool AxisEvaluation::valid() const noexcept {
    if (status == AxisEvaluationStatus::resolved) {
        return diagnostic ==
                   AxisEvaluationDiagnostic::none &&
               line.has_value() &&
               line->valid();
    }

    return diagnostic !=
               AxisEvaluationDiagnostic::none &&
           !line.has_value();
}

AxisEvaluation resolveAxisReference(
    const PartDocument& document,
    const AxisReference& reference,
    const PartEvaluation* part_evaluation,
    const DatumEvaluation* datum_evaluation) noexcept {
    if (!reference.valid()) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }

    if (const auto role =
            builtinOriginAxisForAxisReference(
                reference)) {
        return originAxis(*role);
    }

    const auto axis_id =
        authoredAxisIdForAxisReference(reference);
    if (!axis_id) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }

    const auto* axis =
        document.findAxis(*axis_id);
    if (axis == nullptr) {
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::missing_axis);
    }

    const auto* source =
        document.findSketch(axis->source.sketch_id);
    if (source == nullptr) {
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::missing_sketch);
    }

    const auto* line =
        source->model.findLine(
            axis->source.entity_id);
    if (line == nullptr) {
        return unresolved(
            source->model.contains(
                axis->source.entity_id)
                ? AxisEvaluationStatus::unsupported
                : AxisEvaluationStatus::missing,
            source->model.contains(
                axis->source.entity_id)
                ? AxisEvaluationDiagnostic::source_not_line
                : AxisEvaluationDiagnostic::missing_line);
    }

    if (datumPlaneIdForSketchSupport(source->support)) {
        if (datum_evaluation == nullptr ||
            datum_evaluation->source_revision !=
                document.revision()) {
            return unresolved(
                AxisEvaluationStatus::blocked,
                AxisEvaluationDiagnostic::
                    stale_datum_evaluation);
        }
    }

    AxisEvaluation topology_failure =
        unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    const auto* topology =
        supportTopology(
            *source,
            part_evaluation,
            document,
            topology_failure);
    if (bodyPlanarSurfaceReference(source->support) != nullptr &&
        topology == nullptr) {
        return topology_failure;
    }

    const auto support =
        resolveSketchSupport(
            source->support,
            topology,
            datum_evaluation);
    const auto stage =
        requiredStage(
            *source,
            datum_evaluation);

    switch (support.status) {
    case SketchSupportResolutionStatus::missing:
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::
                sketch_support_missing,
            stage);
    case SketchSupportResolutionStatus::ambiguous:
        return unresolved(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::
                sketch_support_ambiguous,
            stage);
    case SketchSupportResolutionStatus::unsupported:
        return unresolved(
            support.diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        blocked_datum
                ? AxisEvaluationStatus::blocked
                : AxisEvaluationStatus::unsupported,
            support.diagnostic ==
                    SketchSupportResolutionDiagnostic::
                        blocked_datum
                ? AxisEvaluationDiagnostic::
                      sketch_support_blocked
                : AxisEvaluationDiagnostic::
                      sketch_support_unsupported,
            stage);
    case SketchSupportResolutionStatus::resolved:
        break;
    }

    if (!support.frame ||
        !support.frame->valid()) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_frame,
            stage);
    }

    const auto start =
        worldPoint(
            *support.frame,
            line->start());
    const auto end =
        worldPoint(
            *support.frame,
            line->end());
    kernel::Point3 direction{
        end.x - start.x,
        end.y - start.y,
        end.z - start.z};
    const auto length_squared =
        squaredLength(direction);
    if (!finite(start) ||
        !finite(direction) ||
        !(length_squared > 0.0) ||
        !std::isfinite(length_squared)) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_frame,
            stage);
    }

    const auto length =
        std::sqrt(length_squared);
    direction.x /= length;
    direction.y /= length;
    direction.z /= length;

    return resolved(
        ResolvedAxisLine{
            start,
            direction},
        stage);
}

} // namespace simplesolid2::part
