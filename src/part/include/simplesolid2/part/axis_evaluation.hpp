#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel/exact_bounded_edge_geometry.hpp>
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
    const DatumEvaluation* datum_evaluation = nullptr) noexcept;

// RG-02C/B1 first product seam: a read-only, provider-aware candidate
// infinite UNORIENTED line from one strict stage-bound material Body Edge.
// Reuses Q2's certified MaterialEdgeReference + scoped native geometry
// resolver. This does not author/persist a PartAxis or expose a UI command.
[[nodiscard]] AxisEvaluation resolveStraightMaterialEdgeAxisLine(
    const PartDocument& document,
    const MaterialEdgeReference& source,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider) noexcept;

} // namespace simplesolid2::part
