#pragma once

#include <simplesolid2/kernel/edge_feature_evidence.hpp>

namespace simplesolid2::kernel_occt {

// PM-05A evidence-only OCCT probes. No production Fillet/Chamfer API,
// persistent Feature state, or provider handle crosses this boundary.
[[nodiscard]] kernel::EdgeFeatureProviderEvidence
buildEdgeFeatureProviderEvidence(
    kernel::EdgeFeatureEvidenceOperation operation,
    kernel::EdgeFeatureProbeScenario scenario) noexcept;

[[nodiscard]] kernel::EdgeFeatureProviderMatrixEvidence
buildEdgeFeatureProviderMatrixEvidence() noexcept;

[[nodiscard]] kernel::EdgeFeatureLifecycleEvidence
buildEdgeFeatureLifecycleEvidence() noexcept;

} // namespace simplesolid2::kernel_occt
