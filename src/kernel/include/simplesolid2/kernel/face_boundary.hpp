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

    [[nodiscard]] bool valid() const noexcept {
        return edge.valid();
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
        for (const auto& use : edges) {
            if (!use.valid()) return false;
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
        for (const auto& wire : wires) {
            if (!wire.valid()) return false;
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
};

} // namespace simplesolid2::kernel
