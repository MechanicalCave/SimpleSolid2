#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/part/datum.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::part {

enum class DatumPlaneEvaluationStatus {
    resolved,
    missing,
    ambiguous,
    unsupported,
    blocked,
};

enum class DatumPlaneEvaluationDiagnostic {
    none,
    invalid_datum,
    stale_part_evaluation,
    missing_stage,
    upstream_body_unavailable,
    missing_surface,
    ambiguous_surface,
    unsupported_non_planar,
    unsupported_surface,
    missing_source_datum,
    upstream_datum_unavailable,
    dependency_cycle,
    invalid_frame,
};

struct DatumPlaneEvaluation final {
    DatumId datum_id;
    DatumPlaneEvaluationStatus status{
        DatumPlaneEvaluationStatus::unsupported};
    DatumPlaneEvaluationDiagnostic diagnostic{
        DatumPlaneEvaluationDiagnostic::invalid_datum};
    std::optional<kernel::Frame3> frame;
    // Latest required Body stage for this Datum chain. Origin-only chains
    // have no Body-stage dependency. This is derived runtime truth only.
    std::optional<BodyStageRef> body_stage_dependency;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const DatumPlaneEvaluation&,
        const DatumPlaneEvaluation&) = default;
};

struct DatumEvaluation final {
    core::DocumentRevision source_revision;
    std::vector<DatumPlaneEvaluation> planes;

    [[nodiscard]] const DatumPlaneEvaluation* find(
        DatumId id) const noexcept;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const DatumEvaluation&,
        const DatumEvaluation&) = default;
};

// Evaluates the complete authored Datum Plane collection against an explicit
// same-revision set of available Body Feature stages. The feature vector may
// be a complete PartEvaluation or a bounded prefix owned by a later consumer.
// No runtime/provider identity is persisted or promoted to Datum identity.
[[nodiscard]] DatumEvaluation evaluateDatumPlanes(
    const PartDocument& document,
    core::DocumentRevision source_revision,
    const std::vector<FeatureEvaluation>& feature_evaluations);

// Convenience overload for a complete Part evaluation.
[[nodiscard]] DatumEvaluation evaluateDatumPlanes(
    const PartDocument& document,
    const PartEvaluation& part_evaluation);

} // namespace simplesolid2::part
