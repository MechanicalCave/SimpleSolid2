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

enum class SplitCardinalityFixture {
    two_semantic_descendants,
    semantic_plus_technical,
    deleted_target,
};

enum class MergeCardinalityFixture {
    lost_distinction,
    modified_deleted_same_output,
    preserve_first_remove_second,
};

[[nodiscard]] kernel::CardinalityFixtureEvidence
buildSplitEdgeCardinalityFixture(
    const kernel::BoundaryUseProvenance& target,
    SplitCardinalityFixture fixture) noexcept;

[[nodiscard]] kernel::CardinalityFixtureEvidence
buildMergeEdgeCardinalityFixture(
    const kernel::BoundaryUseProvenance& first,
    const kernel::BoundaryUseProvenance& second,
    MergeCardinalityFixture fixture) noexcept;

} // namespace simplesolid2::kernel_occt
