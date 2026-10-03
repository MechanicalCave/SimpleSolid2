#pragma once

namespace simplesolid2::kernel {

enum class ReferenceStatus {
    resolved,
    missing,
    ambiguous,
    unsupported,
};

} // namespace simplesolid2::kernel
