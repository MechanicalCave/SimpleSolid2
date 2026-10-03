#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel/reference_status.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace simplesolid2::kernel {

// Runtime-only provider object. It is intentionally opaque to Part/UI and is
// never authored or serialized.
class RuntimeSolid {
public:
    RuntimeSolid() = default;
    RuntimeSolid(const RuntimeSolid&) = delete;
    RuntimeSolid& operator=(const RuntimeSolid&) = delete;
    virtual ~RuntimeSolid() = default;
};

using RuntimeSolidHandle = std::shared_ptr<const RuntimeSolid>;

struct RuntimeFaceToken final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend bool operator==(
        const RuntimeFaceToken&,
        const RuntimeFaceToken&) = default;
};

enum class SolidBooleanOperation {
    add,
    cut,
};

enum class ExtrudeCapRole {
    profile_cap,
    extent_cap,
    negative_cap,
    positive_cap,
};

enum class ExtrudeGeneratedFaceRoleKind {
    cap,
    side,
};

struct ExtrudeFaceRole final {
    ExtrudeGeneratedFaceRoleKind kind{
        ExtrudeGeneratedFaceRoleKind::side};
    std::optional<ExtrudeCapRole> cap_role;
    std::optional<BoundaryUseProvenance>
        side_provenance;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const ExtrudeFaceRole&,
        const ExtrudeFaceRole&) = default;
};

struct LinearExtrudeInput final {
    PlanarProfileInput profile;
    double start_offset_mm{};
    double end_offset_mm{};
    ExtrudeCapRole start_cap_role{
        ExtrudeCapRole::profile_cap};
    ExtrudeCapRole end_cap_role{
        ExtrudeCapRole::extent_cap};
    SolidBooleanOperation operation{
        SolidBooleanOperation::add};

    // PM-01 supports exactly OneSide or Midplane semantics.
    [[nodiscard]] bool valid() const noexcept;
};

enum class SolidModelingStatus {
    ok,
    invalid_input,
    missing_upstream,
    provider_mismatch,
    provider_failure,
    invalid_brep,
    detached_add,
    no_effect,
    empty_result,
    multi_solid,
};

struct NewFaceLineage final {
    ExtrudeFaceRole role;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    std::optional<RuntimeFaceToken>
        resolved_token;

    friend bool operator==(
        const NewFaceLineage&,
        const NewFaceLineage&) = default;
};

struct InheritedFaceLineage final {
    RuntimeFaceToken token;
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};

    friend bool operator==(
        const InheritedFaceLineage&,
        const InheritedFaceLineage&) = default;
};

struct SolidModelingResult final {
    SolidModelingStatus status{
        SolidModelingStatus::provider_failure};
    RuntimeSolidHandle solid;
    bool brep_valid{false};
    std::size_t solid_count{};
    std::size_t face_count{};
    std::size_t edge_count{};
    std::vector<InheritedFaceLineage>
        inherited_faces;
    std::vector<NewFaceLineage> new_faces;

    [[nodiscard]] bool ok() const noexcept {
        return status == SolidModelingStatus::ok &&
               solid != nullptr &&
               brep_valid &&
               solid_count == 1U;
    }
};

class ISolidModelingKernel {
public:
    ISolidModelingKernel() = default;
    ISolidModelingKernel(
        const ISolidModelingKernel&) = delete;
    ISolidModelingKernel& operator=(
        const ISolidModelingKernel&) = delete;
    virtual ~ISolidModelingKernel() = default;

    [[nodiscard]] virtual SolidModelingResult
    extrude(
        const LinearExtrudeInput& input,
        RuntimeSolidHandle upstream = {}) noexcept = 0;
};

} // namespace simplesolid2::kernel
