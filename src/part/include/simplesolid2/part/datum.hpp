#pragma once

#include <simplesolid2/core/document_reference.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/part/datum_id.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>

#include <optional>
#include <utility>
#include <variant>

namespace simplesolid2::part {

struct BuiltinOriginPlaneReference final {
    core::BuiltinReferenceRole role{
        core::BuiltinReferenceRole::xy_plane};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BuiltinOriginPlaneReference&,
        const BuiltinOriginPlaneReference&) = default;
};

struct BodyPlanarSurfacePlaneReference final {
    SurfaceReference reference;

    [[nodiscard]] bool valid() const noexcept {
        return reference.valid();
    }

    friend bool operator==(
        const BodyPlanarSurfacePlaneReference&,
        const BodyPlanarSurfacePlaneReference&) = default;
};

struct DatumPlaneReference final {
    DatumId datum_id;

    [[nodiscard]] bool valid() const noexcept {
        return datum_id.valid();
    }

    friend bool operator==(
        const DatumPlaneReference&,
        const DatumPlaneReference&) = default;
};

using PlaneReferenceValue = std::variant<
    BuiltinOriginPlaneReference,
    BodyPlanarSurfacePlaneReference,
    DatumPlaneReference>;

struct PlaneReference final {
    PlaneReferenceValue value{
        BuiltinOriginPlaneReference{}};

    PlaneReference() = default;
    explicit PlaneReference(
        BuiltinOriginPlaneReference reference)
        : value{std::move(reference)} {}
    explicit PlaneReference(
        BodyPlanarSurfacePlaneReference reference)
        : value{std::move(reference)} {}
    explicit PlaneReference(
        DatumPlaneReference reference)
        : value{std::move(reference)} {}

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PlaneReference&,
        const PlaneReference&) = default;
};

struct OffsetDatumPlane final {
    DatumId id;
    PlaneReference source;
    core::LengthValue offset;
    bool visible{true};

    friend bool operator==(
        const OffsetDatumPlane&,
        const OffsetDatumPlane&) = default;
};

[[nodiscard]] bool isDatumOriginPlane(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] bool offsetDatumPlaneStructurallyValid(
    const OffsetDatumPlane& datum) noexcept;

[[nodiscard]] std::optional<core::BuiltinReferenceRole>
builtinOriginPlaneForPlaneReference(
    const PlaneReference& reference) noexcept;

[[nodiscard]] const SurfaceReference*
bodyPlanarSurfaceForPlaneReference(
    const PlaneReference& reference) noexcept;

[[nodiscard]] std::optional<DatumId>
datumPlaneIdForPlaneReference(
    const PlaneReference& reference) noexcept;

} // namespace simplesolid2::part
