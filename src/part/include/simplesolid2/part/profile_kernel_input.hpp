#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/part/part_document.hpp>

#include <optional>

namespace simplesolid2::part {

struct DatumEvaluation;

enum class ProfileKernelInputStatus {
    resolved,
    missing_profile,
    missing_source_sketch,
    unresolved_profile,
    support_missing,
    support_ambiguous,
    support_unsupported,
    invalid_input,
};

struct ProfileKernelInputResult final {
    ProfileKernelInputStatus status{
        ProfileKernelInputStatus::invalid_input};
    SketchSupportResolutionDiagnostic
        support_diagnostic{
            SketchSupportResolutionDiagnostic::none};
    std::optional<kernel::PlanarProfileInput> input;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
                   ProfileKernelInputStatus::resolved &&
               input.has_value() &&
               input->valid();
    }
};

// Resolves one durable Part Profile against the exact current support stage.
// Origin-backed Profiles require no derived support context. Body-Surface
// support requires the complete catalog for its declared BodyStageRef.
// Datum-backed support requires a same-revision derived DatumEvaluation.
// No evaluated frame or world geometry is cached or persisted here.
[[nodiscard]] ProfileKernelInputResult
resolveKernelProfileInput(
    const PartDocument& document,
    ProfileId profile_id,
    const BodyStageTopologyCatalog*
        support_topology = nullptr,
    const DatumEvaluation*
        datum_evaluation = nullptr,
    // Authorized same-revision, same-Sketch projection snapshot. When
    // provided, BOTH semantic loop intent and exact Kernel curves read
    // this temporary model, never the persisted authored seed.
    const sketch::SketchModel*
        effective_sketch = nullptr);

// Compatibility convenience for Origin-backed callers that do not own a
// Body-stage evaluation context. Body-Surface-backed Profiles intentionally
// fail closed through this overload until a caller supplies the exact stage.
[[nodiscard]] std::optional<kernel::PlanarProfileInput>
makeKernelProfileInput(
    const PartDocument& document,
    ProfileId profile_id);

} // namespace simplesolid2::part
