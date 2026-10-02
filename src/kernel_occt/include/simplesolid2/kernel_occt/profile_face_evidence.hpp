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

} // namespace simplesolid2::kernel_occt
