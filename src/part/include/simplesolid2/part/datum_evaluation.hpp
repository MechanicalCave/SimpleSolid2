#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/part/datum.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>

#include <optional>
#include <vector>

namespace simplesolid2::part {

class PartDocument;
struct PartEvaluation;

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
    body_stage_unavailable,
    missing_surface,
    ambiguous_surface,
    unsupported_surface,
    unsupported_non_planar,
    missing_datum,
    cyclic_dependency,
    upstream_datum_unavailable,
    invalid_frame,
};

struct DatumPlaneEvaluation final {
    DatumId datum_id;
    DatumPlaneEvaluationStatus status{
        DatumPlaneEvaluationStatus::blocked};
    DatumPlaneEvaluationDiagnostic diagnostic{
        DatumPlaneEvaluationDiagnostic::invalid_datum};
    // Derived current world frame only. Never authored or serialized.
    std::optional<kernel::Frame3> frame;
    // Transitive Body-stage dependency floor for this Datum chain. Empty for
    // Origin-only chains. This is derived semantic dependency data, not a
    // persisted scheduler edge.
    std::optional<BodyStageRef> required_body_stage;

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

// Pure derived evaluation over authored Datum intent plus the same-revision
// Part evaluation needed by Body-Surface sources. No authored mutation,
// provider identity persistence or global dependency graph is introduced.
[[nodiscard]] DatumEvaluation evaluateDatums(
    const PartDocument& document,
    const PartEvaluation& part_evaluation) noexcept;

} // namespace simplesolid2::part
