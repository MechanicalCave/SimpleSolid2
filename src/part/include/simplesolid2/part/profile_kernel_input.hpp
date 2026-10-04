#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/part/part_document.hpp>

#include <optional>

namespace simplesolid2::part {

// Evaluates one durable Part Profile into exact provider-neutral Kernel input.
// The result is transient: it carries semantic provenance but is not persisted
// and contains no Viewer/Qt/OCCT identity.
[[nodiscard]] std::optional<kernel::PlanarProfileInput>
makeKernelProfileInput(
    const PartDocument& document,
    ProfileId profile_id);

// Materializes the same authored local Profile through an already-resolved
// current Sketch support frame. The caller owns stage/reference resolution;
// this function never searches topology, uses stale frame state, or persists
// the derived world placement.
[[nodiscard]] std::optional<kernel::PlanarProfileInput>
makeKernelProfileInputAtResolvedFrame(
    const PartDocument& document,
    ProfileId profile_id,
    const SketchPlacement& resolved_frame);

} // namespace simplesolid2::part
