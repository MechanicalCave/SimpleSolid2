#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>

#include <optional>

namespace simplesolid2::kernel {

// Pure evaluation-local geometric projection result. An Edge token is not a
// persistent identity: Part must obtain it from the same evaluated Body stage.
// Only analytically exact Line/Circle/Arc Sketch geometry can be returned.
enum class EdgeProjectionStatus {
    ok,
    invalid_input,
    provider_mismatch,
    edge_unavailable,
    unsupported_curve,
    degenerate_projection,
    kernel_failure,
};

struct EdgeProjectionResult final {
    EdgeProjectionStatus status{EdgeProjectionStatus::invalid_input};
    std::optional<Curve2> curve;

    [[nodiscard]] bool ok() const noexcept {
        return status == EdgeProjectionStatus::ok &&
               curve.has_value();
    }
};

// Provider-neutral geometry-only seam. No Sketch mutation, authored bindings,
// EntityId, semantic topology ownership or Qt/OCCT type crosses this contract.
class IEdgeProjectionQuery {
public:
    virtual ~IEdgeProjectionQuery() = default;

    [[nodiscard]] virtual EdgeProjectionResult projectEdgeToPlane(
        RuntimeSolidHandle source_body,
        RuntimeEdgeToken current_edge,
        const Frame3& target_frame) noexcept = 0;
};

} // namespace simplesolid2::kernel
