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

} // namespace simplesolid2::part
