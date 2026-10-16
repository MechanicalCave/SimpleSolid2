#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel/exact_bounded_edge_geometry.hpp>
#include <simplesolid2/kernel/exact_cylindrical_face_axis.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>

#include <optional>

namespace simplesolid2::part {

class PartDocument;
struct PartEvaluation;
struct DatumEvaluation;

enum class AxisEvaluationStatus {
    resolved,
    missing,
    ambiguous,
    unsupported,
    blocked,
};

enum class AxisEvaluationDiagnostic {
    none,
    invalid_reference,
    missing_axis,
    missing_sketch,
    missing_line,
    source_not_line,
    sketch_support_missing,
    sketch_support_ambiguous,
    sketch_support_unsupported,
    sketch_support_blocked,
    stale_part_evaluation,
    stale_datum_evaluation,
    support_stage_unavailable,
    invalid_frame,
    // RG-02C/B1 — typed refusal for a candidate straight Body Edge
    // Axis. No native runtime token or last-good line escapes.
    edge_source_missing,
    edge_source_ambiguous,
    edge_source_unsupported,
    edge_native_geometry_unavailable,
    edge_not_straight,
    // H1 strict Face A: a resolved semantic carrier must also have exactly
    // one current bounded Face and an actual native analytic cylinder.
    cylindrical_face_missing,
    cylindrical_face_ambiguous,
    cylindrical_face_not_cylinder,
    cylindrical_face_native_geometry_unavailable,
};

struct ResolvedAxisLine final {
    kernel::Point3 origin;
    kernel::Point3 direction{1.0, 0.0, 0.0};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const ResolvedAxisLine&,
        const ResolvedAxisLine&) = default;
};

struct AxisEvaluation final {
    AxisEvaluationStatus status{
        AxisEvaluationStatus::unsupported};
    AxisEvaluationDiagnostic diagnostic{
        AxisEvaluationDiagnostic::invalid_reference};
    std::optional<ResolvedAxisLine> line;
    std::optional<BodyStageRef> required_body_stage;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const AxisEvaluation&,
        const AxisEvaluation&) = default;
};

[[nodiscard]] AxisEvaluation resolveAxisReference(
    const PartDocument& document,
    const AxisReference& reference,
    const PartEvaluation* part_evaluation = nullptr,
    const DatumEvaluation* datum_evaluation = nullptr,
    kernel::IExactBoundedEdgeGeometryQuery* provider = nullptr,
    kernel::IExactCylindricalFaceAxisQuery* face_provider = nullptr) noexcept;

// RG-02C/B1 first product seam: a read-only, provider-aware candidate
// infinite UNORIENTED line from one strict stage-bound material Body Edge.
// Reuses Q2's certified MaterialEdgeReference + scoped native geometry
// resolver. This does not author/persist a PartAxis or expose a UI command.
[[nodiscard]] AxisEvaluation resolveStraightMaterialEdgeAxisLine(
    const PartDocument& document,
    const MaterialEdgeReference& source,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider) noexcept;

// H1 current-stage strict Face resolution. `source` is durable provenance,
// not a claim that all bounded patches of an analytic Surface can identify
// the same authored Axis. Every query binds one actual native Face to its
// current Feature-stage Body generation through the existing P1-A provider.
[[nodiscard]] AxisEvaluation resolveStrictCylindricalFaceAxisLine(
    const PartDocument& document,
    const SurfaceReference& source,
    const PartEvaluation& evaluation,
    kernel::IExactCylindricalFaceAxisQuery& provider) noexcept;

// H1 read-only authoring admission from one transient actual picked Face
// to exactly one certified stage-qualified strict semantic Surface source.
// Never writes native Face tokens, XYZ, native radius or provider direction.
struct StrictCylindricalFaceAxisAuthoringResult final {
    AxisEvaluationStatus status{AxisEvaluationStatus::unsupported};
    AxisEvaluationDiagnostic diagnostic{
        AxisEvaluationDiagnostic::invalid_reference};
    std::optional<StrictCylindricalFaceAxisSource> source;
    std::optional<ResolvedAxisLine> line;

    [[nodiscard]] bool ok() const noexcept {
        return status == AxisEvaluationStatus::resolved &&
               diagnostic == AxisEvaluationDiagnostic::none &&
               source.has_value() && source->valid() &&
               line.has_value() && line->valid();
    }
};

[[nodiscard]] StrictCylindricalFaceAxisAuthoringResult
authorStrictCylindricalFaceAxisSource(
    const PartDocument& document,
    BodyStageRef stage,
    kernel::RuntimeFaceToken picked_face,
    const PartEvaluation& evaluation,
    kernel::IExactCylindricalFaceAxisQuery& provider) noexcept;

} // namespace simplesolid2::part
