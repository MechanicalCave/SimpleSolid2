#pragma once

#include <simplesolid2/core/document_reference.hpp>
#include <simplesolid2/part/datum_id.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <array>
#include <optional>
#include <variant>
#include <vector>

namespace simplesolid2::part {

struct SketchPlacement final {
    std::array<double, 3> origin{0.0, 0.0, 0.0};
    std::array<double, 3> u_axis{1.0, 0.0, 0.0};
    std::array<double, 3> v_axis{0.0, 1.0, 0.0};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const SketchPlacement&,
        const SketchPlacement&) = default;
};

struct BuiltinOriginPlaneSketchSupport final {
    core::BuiltinReferenceRole role{
        core::BuiltinReferenceRole::xy_plane};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BuiltinOriginPlaneSketchSupport&,
        const BuiltinOriginPlaneSketchSupport&) = default;
};

struct BodyPlanarSurfaceSketchSupport final {
    SurfaceReference reference;

    [[nodiscard]] bool valid() const noexcept {
        return reference.valid();
    }

    friend bool operator==(
        const BodyPlanarSurfaceSketchSupport&,
        const BodyPlanarSurfaceSketchSupport&) = default;
};

struct DatumPlaneSketchSupport final {
    DatumId datum_id;

    [[nodiscard]] bool valid() const noexcept {
        return datum_id.valid();
    }

    friend bool operator==(
        const DatumPlaneSketchSupport&,
        const DatumPlaneSketchSupport&) = default;
};

using PartSketchSupportValue = std::variant<
    BuiltinOriginPlaneSketchSupport,
    BodyPlanarSurfaceSketchSupport,
    DatumPlaneSketchSupport>;

struct PartSketchSupport final {
    PartSketchSupportValue value{
        BuiltinOriginPlaneSketchSupport{}};

    PartSketchSupport() = default;
    explicit PartSketchSupport(
        BuiltinOriginPlaneSketchSupport support)
        : value{std::move(support)} {}
    explicit PartSketchSupport(
        BodyPlanarSurfaceSketchSupport support)
        : value{std::move(support)} {}
    explicit PartSketchSupport(
        DatumPlaneSketchSupport support)
        : value{std::move(support)} {}
    explicit PartSketchSupport(
        core::BuiltinReferenceRole role)
        : value{BuiltinOriginPlaneSketchSupport{
              role}} {}

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PartSketchSupport&,
        const PartSketchSupport&) = default;
};

// Part-owned source intent for a linked, existing Sketch entity.
// Runtime projection output and provider tokens are never stored here.
struct ProjectedEdgeBinding final {
    sketch::EntityId target_entity;
    MaterialEdgeReference source;

    [[nodiscard]] bool valid() const noexcept {
        return target_entity.valid() && source.valid();
    }

    friend bool operator==(
        const ProjectedEdgeBinding&,
        const ProjectedEdgeBinding&) = default;
};

struct PartSketch final {
    sketch::SketchId id;
    PartSketchSupport support;
    bool visible{true};
    sketch::SketchModel model;
    // Canonically ordered by target EntityId, no duplicate bindings.
    // A linked entity's authored Sketch coordinates are seed only.
    std::vector<ProjectedEdgeBinding> projection_bindings;

    friend bool operator==(
        const PartSketch&,
        const PartSketch&) = default;
};

[[nodiscard]] bool isSketchOriginPlane(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] std::optional<PartSketchSupport>
partSketchSupportForBuiltinPlane(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] std::optional<PartSketchSupport>
partSketchSupportForBodyPlanarSurface(
    SurfaceReference reference) noexcept;

[[nodiscard]] std::optional<PartSketchSupport>
partSketchSupportForDatumPlane(
    DatumId datum_id) noexcept;

[[nodiscard]] std::optional<core::BuiltinReferenceRole>
builtinOriginPlaneForSketchSupport(
    const PartSketchSupport& support) noexcept;

[[nodiscard]] const SurfaceReference*
bodyPlanarSurfaceReference(
    const PartSketchSupport& support) noexcept;

[[nodiscard]] std::optional<DatumId>
datumPlaneIdForSketchSupport(
    const PartSketchSupport& support) noexcept;

// Derived frame only. This value is never authored or persisted by schema v9.
[[nodiscard]] std::optional<SketchPlacement>
sketchPlacementForSupport(
    const PartSketchSupport& support) noexcept;

enum class SketchSupportResolutionStatus {
    resolved,
    missing,
    ambiguous,
    unsupported,
};

enum class SketchSupportResolutionDiagnostic {
    none,
    invalid_support,
    missing_stage,
    missing_surface,
    ambiguous_surface,
    unsupported_non_planar,
    unsupported_surface,
    missing_datum,
    ambiguous_datum,
    unsupported_datum,
    blocked_datum,
};

struct ResolvedSketchSupport final {
    SketchSupportResolutionStatus status{
        SketchSupportResolutionStatus::unsupported};
    SketchSupportResolutionDiagnostic diagnostic{
        SketchSupportResolutionDiagnostic::invalid_support};
    std::optional<SketchPlacement> frame;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const ResolvedSketchSupport&,
        const ResolvedSketchSupport&) = default;
};

struct BodyStageTopologyCatalog;
struct DatumEvaluation;

[[nodiscard]] ResolvedSketchSupport
resolveSketchSupport(
    const PartSketchSupport& support,
    const BodyStageTopologyCatalog* topology = nullptr,
    const DatumEvaluation* datum_evaluation = nullptr) noexcept;

} // namespace simplesolid2::part
