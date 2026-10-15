#pragma once

#include <simplesolid2/core/document_reference.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/part/datum_id.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>

#include <array>
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

// P4-S2 in-development authored orientation/source value.
// Unlike S1's caller-carried transient intent, this model owns a DatumId.
// END/MID are genuine semantic Edge-local references; never native tokens,
// world XYZ topology keys, a cached frame or a provider-instance handle.
// The independent off-Edge Point source is a later supported source kind.
using EdgePointDatumSource = std::variant<
    SelectedMaterialEdgeEnd,
    SelectedMaterialEdgeMid>;

struct EdgePointDatumPlane final {
    DatumId id;
    EdgePointDatumSource source;
    SelectedEdgeFrameIntent orientation;
    bool visible{true};

    friend bool operator==(
        const EdgePointDatumPlane&,
        const EdgePointDatumPlane&) = default;
};

[[nodiscard]] bool edgePointDatumPlaneStructurallyValid(
    const EdgePointDatumPlane& datum) noexcept;

// Derive the current plane anew from the certified Body stage, preserving
// the authored pre-Reverse N/U seed. No native handle/frame is retained.
[[nodiscard]] SelectedEdgeFrameResult resolveEdgePointDatumFrame(
    const PartDocument& owner,
    const EdgePointDatumPlane& datum,
    const PartEvaluation& evaluation,
    kernel::IExactBoundedEdgeGeometryQuery& provider);

// S4A: one source-stage semantic Point, never an incident source Edge,
// current OCCT Vertex, provider position, UI token or cursor coordinate.
struct StageSemanticPointReference final {
    BodyStageRef stage;
    FeaturePointAddress point;

    [[nodiscard]] bool valid() const noexcept {
        return stage.valid() &&
               stage.kind == BodyStageKind::after_feature &&
               stage.feature_id.has_value() &&
               point.valid() &&
               point.producer_feature_id <= *stage.feature_id;
    }

    friend bool operator==(
        const StageSemanticPointReference&,
        const StageSemanticPointReference&) = default;
};

struct ThreePointDatumPlane final {
    DatumId id;
    std::array<StageSemanticPointReference, 3> points;
    bool reverse{false};
    bool visible{true};

    friend bool operator==(
        const ThreePointDatumPlane&,
        const ThreePointDatumPlane&) = default;
};

[[nodiscard]] bool threePointDatumPlaneStructurallyValid(
    const ThreePointDatumPlane& datum) noexcept;

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
