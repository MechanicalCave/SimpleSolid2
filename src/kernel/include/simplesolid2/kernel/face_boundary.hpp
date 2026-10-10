#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>

#include <optional>
#include <utility>
#include <vector>

namespace simplesolid2::kernel {

// PG-01D transient Face boundary observation; never authored CAD identity.
// All tokens are scoped to one exact current RuntimeSolidHandle generation.
enum class FaceBoundaryStatus {
    ok,
    invalid_input,
    provider_mismatch,
    face_unavailable,
    unsupported_surface,
    malformed_boundary,
    provider_failure,
};

struct FaceBoundaryEdgeUse final {
    RuntimeEdgeToken edge;
    bool reversed{false};
    // PG-01D E0 OPTIONAL, provider-scoped OCCT evidence for THIS
    // oriented Face-wire occurrence. These are current runtime Vertex
    // tokens, never persistent Point/Edge identity. A full-circle
    // Edge may legitimately have the same start and end Vertex.
    // Older/other providers may leave both fields absent; consumers
    // requiring directed continuity must fail closed in that case.
    std::optional<RuntimeVertexToken> start_vertex;
    std::optional<RuntimeVertexToken> end_vertex;

    [[nodiscard]] bool valid() const noexcept {
        if (!edge.valid() ||
            start_vertex.has_value() != end_vertex.has_value()) {
            return false;
        }
        return !start_vertex ||
            (start_vertex->valid() && end_vertex->valid());
    }

    friend bool operator==(
        const FaceBoundaryEdgeUse&,
        const FaceBoundaryEdgeUse&) = default;
};

struct FaceBoundaryWire final {
    // Exactly one wire is the native bounded Face's outer boundary.
    // All other actual native wires are inner/hole uses.
    bool outer{false};
    std::vector<FaceBoundaryEdgeUse> edges;

    [[nodiscard]] bool valid() const noexcept {
        if (edges.empty()) return false;
        // The optional E0 signed-Vertex observation is wire-ATOMIC:
        // older providers may omit it on every use, but mixed coverage
        // cannot be mistaken for a directed certified native boundary.
        // When supplied, use[i].end must be exactly use[i+1].start,
        // including the closure from the last use back to the first.
        // No coordinate comparison, tolerance or curve joining.
        const bool has_directed_endpoints =
            edges.front().start_vertex.has_value();
        for (std::size_t i = 0U; i < edges.size(); ++i) {
            const auto& use = edges[i];
            const auto& next =
                edges[(i + 1U) % edges.size()];
            if (!use.valid() ||
                use.start_vertex.has_value() !=
                    has_directed_endpoints) {
                return false;
            }
            if (has_directed_endpoints &&
                (!next.start_vertex ||
                 *use.end_vertex != *next.start_vertex)) {
                return false;
            }
        }
        return true;
    }

    friend bool operator==(
        const FaceBoundaryWire&,
        const FaceBoundaryWire&) = default;
};

struct FaceBoundaryResult final {
    FaceBoundaryStatus status{FaceBoundaryStatus::invalid_input};
    std::vector<FaceBoundaryWire> wires;

    [[nodiscard]] bool ok() const noexcept {
        if (status != FaceBoundaryStatus::ok ||
            wires.empty()) {
            return false;
        }
        std::size_t outer_count = 0U;
        // Directed OCCT endpoint observation must be RESULT-ATOMIC,
        // not just wire-atomic. One legacy (Edge-only) inner wire
        // alongside a signed outer wire would otherwise report ok()
        // despite supplying only a PARTIAL native contour proof.
        // A fully legacy provider remains backward-compatible but
        // its output is never a signed-continuity certificate.
        std::optional<bool> has_directed_endpoints;
        for (const auto& wire : wires) {
            if (!wire.valid()) return false;
            const bool directed =
                wire.edges.front().start_vertex.has_value();
            if (has_directed_endpoints &&
                *has_directed_endpoints != directed) {
                return false;
            }
            has_directed_endpoints = directed;
            if (wire.outer) ++outer_count;
        }
        return outer_count == 1U;
    }
};

// Cannot be forged by UI/Part; only a live provider can bind a Face to its
// current Body. A numerically reused runtime Face token is not authority.
class ScopedBoundaryFace final {
public:
    ScopedBoundaryFace() = default;

    [[nodiscard]] bool valid() const noexcept {
        return source_body_ != nullptr && face_.valid();
    }

    [[nodiscard]] const RuntimeSolidHandle&
    sourceBody() const noexcept {
        return source_body_;
    }

    [[nodiscard]] RuntimeFaceToken face() const noexcept {
        return face_;
    }

private:
    friend class IFaceBoundaryQuery;

    ScopedBoundaryFace(
        RuntimeSolidHandle source_body,
        RuntimeFaceToken face)
        : source_body_{std::move(source_body)},
          face_{face} {}

    RuntimeSolidHandle source_body_;
    RuntimeFaceToken face_;
};

// Read-only native boundary interrogation. It does not certify a Part
// semantic Face nor a durable material Edge. Part must separately validate
// strict bounded Face meaning and authorMaterialEdgeReference for every
// reported occurrence at the same BodyStageRef before staging anything.
class IFaceBoundaryQuery {
protected:
    [[nodiscard]] static ScopedBoundaryFace makeScopedFace(
        RuntimeSolidHandle source_body,
        RuntimeFaceToken face) {
        return {std::move(source_body), face};
    }

public:
    virtual ~IFaceBoundaryQuery() = default;

    [[nodiscard]] virtual std::optional<ScopedBoundaryFace>
    bindFaceToBody(
        RuntimeSolidHandle source_body,
        RuntimeFaceToken current_face) noexcept = 0;

    [[nodiscard]] virtual FaceBoundaryResult queryFaceBoundary(
        RuntimeSolidHandle current_body,
        const ScopedBoundaryFace& bound_face) noexcept = 0;

    // PG-01D Face Boundary extension E0: optional *read-only* native
    // oriented wires from an arbitrary bounded surface (plane, cylinder,
    // etc.). This does NOT prove a continuous material surface region or
    // authorize a link. Existing strict Planar Face calls remain unchanged.
    // Providers without this optional capability fail typed/closed.
    [[nodiscard]] virtual FaceBoundaryResult
    queryFaceBoundaryAnySurface(
        RuntimeSolidHandle,
        const ScopedBoundaryFace&) noexcept {
        return {FaceBoundaryStatus::unsupported_surface, {}};
    }
};

} // namespace simplesolid2::kernel
