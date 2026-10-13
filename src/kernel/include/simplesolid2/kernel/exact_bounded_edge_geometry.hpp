// Pure evaluation-local exact bounded analytic Edge geometry. Never persisted.
// Q0 provider-neutral interface; no authored END index, Frame3 sign or UI.
#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>
#include <array>
#include <optional>
#include <utility>

namespace simplesolid2::kernel {

enum class ExactEdge3dStatus {
    ok,
    invalid_input,
    provider_mismatch,
    edge_unavailable,
    unsupported_curve,
    closed_or_periodic,
    degenerate_edge,
    kernel_failure,
};
enum class ExactEdge3dKind {
    bounded_line,
    open_circular_arc,
};

// The geometry is one unoriented line {+t,-t}.
// Point3 merely stores three components, NOT a source point or signed frame.
struct UnorientedUnitTangent3 final {
    Point3 representative;
};
struct ExactBoundedEdge3d final {
    ExactEdge3dKind kind{ExactEdge3dKind::bounded_line};
    // Position[i] and tangent[i] belong to the SAME current physical terminal.
    // The index is NEVER a durable END key; order may change on rebuild.
    std::array<Point3, 2> current_terminal_positions_world{};
    std::array<UnorientedUnitTangent3, 2> terminal_tangent_lines{};
    double bounded_length_mm{};
    Point3 midpoint_at_half_bounded_length_world{};
    UnorientedUnitTangent3 midpoint_tangent_line{};
};
struct ExactEdge3dResult final {
    ExactEdge3dStatus status{ExactEdge3dStatus::invalid_input};
    std::optional<ExactBoundedEdge3d> geometry;

    [[nodiscard]] bool ok() const noexcept {
        return status == ExactEdge3dStatus::ok && geometry.has_value();
    }
};

class ScopedExactEdge final {
public:
    ScopedExactEdge() = default;

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
    friend class IExactBoundedEdgeGeometryQuery;
    ScopedExactEdge(RuntimeSolidHandle body, RuntimeEdgeToken token) noexcept
        : source_body_{std::move(body)}, edge_{token} {}

    RuntimeSolidHandle source_body_;
    RuntimeEdgeToken edge_;
};

class IExactBoundedEdgeGeometryQuery {
protected:
    [[nodiscard]] static ScopedExactEdge makeScopedExactEdge(
        RuntimeSolidHandle body, RuntimeEdgeToken token) noexcept {
        return {std::move(body), token};
    }
public:
    virtual ~IExactBoundedEdgeGeometryQuery() = default;

    [[nodiscard]] virtual std::optional<ScopedExactEdge>
    bindExactEdgeToBody(
        RuntimeSolidHandle source_body,
        RuntimeEdgeToken current_edge) noexcept = 0;

    [[nodiscard]] virtual ExactEdge3dResult queryExactBoundedEdge3d(
        RuntimeSolidHandle current_body,
        const ScopedExactEdge& edge) noexcept = 0;
};

} // namespace simplesolid2::kernel
