#include <simplesolid2/part/axis_evaluation.hpp>

#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

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
        !ownsCurrentPartEvaluation(document, *part_evaluation)) {
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
    const DatumEvaluation* datum_evaluation,
    kernel::IExactBoundedEdgeGeometryQuery* provider,
    kernel::IExactCylindricalFaceAxisQuery* face_provider) noexcept {
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

    // Only native Body source kinds need exact provider geometry. Legacy
    // Sketch-Line evaluation is preserved as an independent source path.
    // Both query interfaces are provider-neutral; do not import OCCT types.
    if (const auto* face =
            std::get_if<StrictCylindricalFaceAxisSource>(
                &axis->source)) {
        auto* exact = face_provider;
        if (exact == nullptr && provider != nullptr) {
            // Existing Part/Revolve callers pass the exact Edge capability;
            // the SAME kernel instance may additionally support P1-A.
            exact = dynamic_cast<
                kernel::IExactCylindricalFaceAxisQuery*>(provider);
        }
        if (part_evaluation == nullptr || exact == nullptr) {
            return unresolved(
                AxisEvaluationStatus::blocked,
                AxisEvaluationDiagnostic::
                    cylindrical_face_native_geometry_unavailable,
                face->surface.stage);
        }
        return resolveStrictCylindricalFaceAxisLine(
            document, face->surface,
            *part_evaluation, *exact);
    }
    const auto* sketch_source =
        std::get_if<SketchLineAxisSource>(&axis->source);
    if (sketch_source == nullptr) {
        const auto* edge_source =
            std::get_if<StraightMaterialEdgeAxisSource>(
                &axis->source);
        if (edge_source == nullptr) {
            return unresolved(
                AxisEvaluationStatus::unsupported,
                AxisEvaluationDiagnostic::invalid_reference);
        }
        if (part_evaluation == nullptr || provider == nullptr) {
            return unresolved(
                AxisEvaluationStatus::blocked,
                AxisEvaluationDiagnostic::
                    edge_native_geometry_unavailable,
                edge_source->edge.stage);
        }
        return resolveStraightMaterialEdgeAxisLine(
            document,
            edge_source->edge,
            *part_evaluation,
            *provider);
    }
    const auto* source =
        document.findSketch(sketch_source->sketch_id);
    if (source == nullptr) {
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::missing_sketch);
    }

    const auto* line =
        source->model.findLine(
            sketch_source->entity_id);
    if (line == nullptr) {
        return unresolved(
            source->model.contains(
                sketch_source->entity_id)
                ? AxisEvaluationStatus::unsupported
                : AxisEvaluationStatus::missing,
            source->model.contains(
                sketch_source->entity_id)
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

// H1 strict option A: durable Surface identity plus a strict singleton
// bounded Face at the ORIGINAL declared stage. The Surface alone never
// authorizes a multi-Face analytic cylinder, even when all patches are
// coaxial. Transient Face tokens and world geometry remain disposable.
AxisEvaluation resolveStrictCylindricalFaceAxisLine(
    const PartDocument& document,
    const SurfaceReference& source,
    const PartEvaluation& evaluation,
    kernel::IExactCylindricalFaceAxisQuery& provider) noexcept {
    using ReferenceStatus = kernel::ReferenceStatus;
    if (!source.valid()) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }
    if (!ownsCurrentPartEvaluation(document, evaluation) ||
        // During actual sequential Revolve feature evaluation, prefix
        // Body status is still 'empty', even though earlier Feature stages
        // have real current solids and catalogues. The exact source stage
        // (checked below) is authority; final unavailable is never used.
        evaluation.body_status == BodyEvaluationStatus::unavailable) {
        return unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::stale_part_evaluation,
            source.stage);
    }
    const auto* feature = evaluation.findFeature(
        *source.stage.feature_id);
    if (feature == nullptr ||
        feature->status != FeatureEvaluationStatus::up_to_date ||
        !feature->result_solid ||
        !feature->result_topology ||
        feature->result_topology->stage != source.stage ||
        !feature->result_topology->complete()) {
        return unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::support_stage_unavailable,
            source.stage);
    }

    const auto& catalog = *feature->result_topology;
    const auto matches = [&source](
        const FeatureSurfaceResolution& carrier) {
        return carrier.address == source.surface;
    };
    const auto count = std::count_if(
        catalog.surfaces.begin(), catalog.surfaces.end(), matches);
    if (count == 0) {
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::cylindrical_face_missing,
            source.stage);
    }
    if (count != 1) {
        return unresolved(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::cylindrical_face_ambiguous,
            source.stage);
    }
    const auto carrier = std::find_if(
        catalog.surfaces.begin(), catalog.surfaces.end(), matches);
    if (carrier->status == ReferenceStatus::missing) {
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::cylindrical_face_missing,
            source.stage);
    }
    if (carrier->status != ReferenceStatus::resolved) {
        return unresolved(
            carrier->status == ReferenceStatus::ambiguous
                ? AxisEvaluationStatus::ambiguous
                : AxisEvaluationStatus::unsupported,
            carrier->status == ReferenceStatus::ambiguous
                ? AxisEvaluationDiagnostic::cylindrical_face_ambiguous
                : AxisEvaluationDiagnostic::cylindrical_face_not_cylinder,
            source.stage);
    }
    if (carrier->surface_kind != kernel::SurfaceKind::cylinder) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::cylindrical_face_not_cylinder,
            source.stage);
    }
    if (carrier->strict_face_status !=
            ReferenceStatus::resolved ||
        carrier->candidate_face_count != 1U ||
        carrier->current_faces.size() != 1U ||
        !carrier->current_faces.front().valid()) {
        // This intentionally refuses a uniquely resolved Surface whose
        // strict bounded Face has partitioned into multiple patches.
        return unresolved(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::cylindrical_face_ambiguous,
            source.stage);
    }
    const auto token = carrier->current_faces.front();
    const auto same_face = [token](
        const BodyFaceTopologyRecord& face) {
        return face.runtime_token == token;
    };
    if (std::count_if(
            catalog.faces.begin(), catalog.faces.end(),
            same_face) != 1) {
        return unresolved(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::cylindrical_face_ambiguous,
            source.stage);
    }
    const auto face = std::find_if(
        catalog.faces.begin(), catalog.faces.end(), same_face);
    // H2 Owner-approved 19th-path D2 addendum (Fillet R30): a unique
    // generated Face may have a uniquely resolved semantic Surface
    // without a separate durable FeatureFaceAddress. Axis persists
    // SurfaceReference, NOT FeatureFaceAddress. Keep every strict
    // carrier/one-current-Face constraint checked above, and require
    // this Face to certify that *one* identical Surface candidate.
    if (face->surface_candidates.size() != 1U ||
        face->surface_candidates.front() != source.surface) {
        return unresolved(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::cylindrical_face_ambiguous,
            source.stage);
    }

    // P1-A alone may mint a scoped native Face bound to this exact Body
    // generation; never trust the catalog's token as durable identity.
    const auto native = provider.bindExactCylindricalFaceToBody(
        feature->result_solid, token);
    if (!ownsCurrentPartEvaluation(document, evaluation)) {
        return unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::stale_part_evaluation, source.stage);
    }
    if (!native || !native->valid()) {
        return unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::
                cylindrical_face_native_geometry_unavailable,
            source.stage);
    }
    const auto geometry = provider.queryExactCylindricalFaceAxis(
        feature->result_solid, *native);
    if (!ownsCurrentPartEvaluation(document, evaluation)) {
        return unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::stale_part_evaluation, source.stage);
    }
    if (!geometry.ok()) {
        return unresolved(
            geometry.status ==
                kernel::ExactCylindricalFaceAxisStatus::
                    non_cylindrical_surface
                ? AxisEvaluationStatus::unsupported
                : AxisEvaluationStatus::blocked,
            geometry.status ==
                kernel::ExactCylindricalFaceAxisStatus::
                    non_cylindrical_surface
                ? AxisEvaluationDiagnostic::
                    cylindrical_face_not_cylinder
                : AxisEvaluationDiagnostic::
                    cylindrical_face_native_geometry_unavailable,
            source.stage);
    }
    const ResolvedAxisLine line{
        geometry.geometry->point_on_axis_world_mm,
        geometry.geometry->unoriented_unit_direction};
    if (!line.valid()) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::
                cylindrical_face_native_geometry_unavailable,
            source.stage);
    }
    return resolved(line, source.stage);
}

// A user pick is a transient bounded Face. Author ONLY the one durable
// SurfaceReference which was actually certified for this exact Face/Body;
// geometric equality never selects or repairs semantic provenance.
StrictCylindricalFaceAxisAuthoringResult
authorStrictCylindricalFaceAxisSource(
    const PartDocument& document,
    BodyStageRef stage,
    kernel::RuntimeFaceToken picked_face,
    const PartEvaluation& evaluation,
    kernel::IExactCylindricalFaceAxisQuery& provider) noexcept {
    auto fail = [](
        AxisEvaluationStatus status,
        AxisEvaluationDiagnostic diagnostic)
        -> StrictCylindricalFaceAxisAuthoringResult {
        return {status, diagnostic, std::nullopt, std::nullopt};
    };
    if (!stage.valid() ||
        stage.kind != BodyStageKind::after_feature ||
        !picked_face.valid()) {
        return fail(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }
    if (!ownsCurrentPartEvaluation(document, evaluation) ||
        // During actual sequential Revolve feature evaluation, prefix
        // Body status is still 'empty', even though earlier Feature stages
        // have real current solids and catalogues. The exact source stage
        // (checked below) is authority; final unavailable is never used.
        evaluation.body_status == BodyEvaluationStatus::unavailable) {
        return fail(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::stale_part_evaluation);
    }
    const auto* feature =
        evaluation.findFeature(*stage.feature_id);
    if (feature == nullptr ||
        feature->status != FeatureEvaluationStatus::up_to_date ||
        !feature->result_topology ||
        !feature->result_solid ||
        !feature->result_topology->complete() ||
        feature->result_topology->stage != stage) {
        return fail(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::support_stage_unavailable);
    }
    const auto& catalog = *feature->result_topology;
    const auto same = [picked_face](
        const BodyFaceTopologyRecord& entry) {
        return entry.runtime_token == picked_face;
    };
    if (std::count_if(
            catalog.faces.begin(), catalog.faces.end(),
            same) != 1) {
        return fail(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::cylindrical_face_ambiguous);
    }
    const auto picked = std::find_if(
        catalog.faces.begin(), catalog.faces.end(), same);
    // Fillet's generated bounded Face can have no FeatureFaceAddress
    // while its single semantic Surface candidate is fully resolved.
    // The resolver below re-checks unique stage, carrier and one
    // current native Face; no provider Face token becomes durable.
    if (picked->surface_candidates.size() != 1U) {
        return fail(
            AxisEvaluationStatus::ambiguous,
            AxisEvaluationDiagnostic::cylindrical_face_ambiguous);
    }
    const SurfaceReference source{
        stage, picked->surface_candidates.front()};
    if (!source.valid()) {
        return fail(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }
    const auto evaluated = resolveStrictCylindricalFaceAxisLine(
        document, source, evaluation, provider);
    if (evaluated.status != AxisEvaluationStatus::resolved ||
        !evaluated.line) {
        return fail(evaluated.status, evaluated.diagnostic);
    }
    return {
        AxisEvaluationStatus::resolved,
        AxisEvaluationDiagnostic::none,
        StrictCylindricalFaceAxisSource{source},
        evaluated.line};
}

// RG-02C/B1: no independent Edge resolver, no native token authoring and
// no copy of the A0 test-only line algorithm. Q2 owns strict stage/revision
// admission and the scoped native MID/tangent observation; the Axis consumer
// owns only line-kind admission and the Axis-specific typed result.
AxisEvaluation resolveStraightMaterialEdgeAxisLine(
    const PartDocument& document,
    const MaterialEdgeReference& source,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider) noexcept {
    if (!source.valid()) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::invalid_reference);
    }

    const auto observation = resolveSelectedMaterialEdgeMid(
        document,
        SelectedMaterialEdgeMid{source},
        evaluation,
        provider);

    if (!observation.resolved()) {
        switch (observation.status) {
        case SelectedEdgeMidStatus::edge_missing:
            return unresolved(
                AxisEvaluationStatus::missing,
                AxisEvaluationDiagnostic::edge_source_missing,
                source.stage);
        case SelectedEdgeMidStatus::edge_ambiguous:
            return unresolved(
                AxisEvaluationStatus::ambiguous,
                AxisEvaluationDiagnostic::edge_source_ambiguous,
                source.stage);
        case SelectedEdgeMidStatus::edge_unsupported:
            return unresolved(
                AxisEvaluationStatus::unsupported,
                AxisEvaluationDiagnostic::edge_source_unsupported,
                source.stage);
        case SelectedEdgeMidStatus::native_geometry_unavailable:
            return unresolved(
                AxisEvaluationStatus::blocked,
                AxisEvaluationDiagnostic::edge_native_geometry_unavailable,
                source.stage);
        case SelectedEdgeMidStatus::invalid_context:
        case SelectedEdgeMidStatus::resolved:
            return unresolved(
                AxisEvaluationStatus::blocked,
                AxisEvaluationDiagnostic::stale_part_evaluation,
                source.stage);
        }
    }

    // Even though Q2 can legitimately observe an open circular Arc MID,
    // the first Body-Edge Axis constructor is LINE ONLY.
    const auto* stage = evaluation.findFeature(
        *source.stage.feature_id);
    if (stage == nullptr ||
        !stage->result_topology ||
        stage->result_topology->stage != source.stage) {
        return unresolved(
            AxisEvaluationStatus::blocked,
            AxisEvaluationDiagnostic::support_stage_unavailable,
            source.stage);
    }
    const auto& edges = stage->result_topology->edges;
    const auto current = std::find_if(
        edges.begin(), edges.end(),
        [&observation](const auto& edge) {
            return edge.runtime_token == *observation.current_edge;
        });
    if (current == edges.end()) {
        return unresolved(
            AxisEvaluationStatus::missing,
            AxisEvaluationDiagnostic::edge_source_missing,
            source.stage);
    }
    if (current->curve_kind != kernel::CurveKind::line) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::edge_not_straight,
            source.stage);
    }

    ResolvedAxisLine line{
        *observation.current_position_world,
        observation.current_tangent_line->representative};
    if (!line.valid()) {
        return unresolved(
            AxisEvaluationStatus::unsupported,
            AxisEvaluationDiagnostic::edge_native_geometry_unavailable,
            source.stage);
    }
    return resolved(std::move(line), source.stage);
}

} // namespace simplesolid2::part
