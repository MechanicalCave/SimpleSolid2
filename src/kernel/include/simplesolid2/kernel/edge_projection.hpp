#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>

#include <optional>
#include <utility>

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

// Opaque provider-validated evaluation-local source, bound to the *exact*
// runtime Body generation that owned its Edge at capture. Public code cannot
// construct or retarget a populated binding from a bare numeric token.
class ScopedProjectionEdge final {
public:
    ScopedProjectionEdge() = default;

    [[nodiscard]] bool valid() const noexcept {
        return source_body_ != nullptr && edge_.valid();
    }
    [[nodiscard]] const RuntimeSolidHandle& sourceBody() const noexcept {
        return source_body_;
    }
    [[nodiscard]] RuntimeEdgeToken edge() const noexcept {
        return edge_;
    }

private:
    friend class IEdgeProjectionQuery;
    ScopedProjectionEdge(
        RuntimeSolidHandle source_body,
        RuntimeEdgeToken current_edge)
        : source_body_{std::move(source_body)},
          edge_{current_edge} {}

    RuntimeSolidHandle source_body_;
    RuntimeEdgeToken edge_;
};

// Provider-neutral geometry-only seam. Capture must be made using the Edge
// resolved in the same current Body-stage inventory (Part owns this rule).
// Recompute with a *different* RuntimeSolidHandle rejects the old binding,
// even if the provider reused an identical raw RuntimeEdgeToken value.
// This is not a durable topology reference; it is never serialized.
class IEdgeProjectionQuery {
protected:
    [[nodiscard]] static ScopedProjectionEdge makeScopedEdge(
        RuntimeSolidHandle body,
        RuntimeEdgeToken edge) {
        return {std::move(body), edge};
    }

public:
    virtual ~IEdgeProjectionQuery() = default;

    [[nodiscard]] virtual std::optional<ScopedProjectionEdge> bindEdgeToBody(
        RuntimeSolidHandle source_body,
        RuntimeEdgeToken current_edge) noexcept = 0;

    [[nodiscard]] virtual EdgeProjectionResult projectEdgeToPlane(
        RuntimeSolidHandle current_body,
        const ScopedProjectionEdge& bound_edge,
        const Frame3& target_frame) noexcept = 0;
};

} // namespace simplesolid2::kernel
