#pragma once

#include <simplesolid2/kernel/edge_projection.hpp>

#include <Standard_Failure.hxx>

#include <optional>
#include <utility>

namespace simplesolid2::kernel_occt::detail {

// The actual production OCCT projection body and its native tests use this
// identical exception boundary. It has no mutable fault-injection flag and
// does not change the public IEdgeProjectionQuery ABI or Part persistence.
template <typename Calculate>
[[nodiscard]] kernel::EdgeProjectionResult guardedExactEdgeProjection(
    Calculate&& calculate) noexcept {
    try {
        return std::forward<Calculate>(calculate)();
    } catch (const Standard_Failure&) {
        return {kernel::EdgeProjectionStatus::kernel_failure,
                std::nullopt};
    } catch (...) {
        return {kernel::EdgeProjectionStatus::kernel_failure,
                std::nullopt};
    }
}

} // namespace simplesolid2::kernel_occt::detail
