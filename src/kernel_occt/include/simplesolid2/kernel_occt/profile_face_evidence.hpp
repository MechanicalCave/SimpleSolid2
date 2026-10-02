#pragma once

#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>

namespace simplesolid2::kernel_occt {

// PM-00A evidence-only OCCT adapter. Provider topology and handles remain
// private to the implementation; callers receive neutral evidence only.
[[nodiscard]] kernel::ShapeEvidence
buildProfileFaceEvidence(
    const kernel::PlanarProfileInput& input) noexcept;

[[nodiscard]] kernel::ExtrudeEvidence
buildProfileExtrudeEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept;

[[nodiscard]] kernel::FullRevolveEvidence
buildProfileFullRevolveEvidence(
    const kernel::PlanarProfileInput& input,
    kernel::Point2 axis_origin,
    kernel::Point2 axis_direction) noexcept;

[[nodiscard]] kernel::EdgeSplitHistoryEvidence
buildEdgeSplitHistoryEvidence(
    kernel::EdgeSplitProbeScenario scenario) noexcept;

[[nodiscard]] kernel::FaceMergeHistoryEvidence
buildFaceMergeHistoryEvidence(
    kernel::FaceMergeProbeScenario scenario) noexcept;

[[nodiscard]] kernel::MultiStageLineageEvidence
buildMultiStageLineageEvidence(
    kernel::MultiStageProbeScenario scenario,
    double extrusion_height) noexcept;

} // namespace simplesolid2::kernel_occt
