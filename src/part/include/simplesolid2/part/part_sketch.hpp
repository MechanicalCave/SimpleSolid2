#pragma once

#include <simplesolid2/core/document_reference.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <array>
#include <optional>

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

struct PartSketchSupport final {
    core::BuiltinReferenceRole builtin_plane{
        core::BuiltinReferenceRole::xy_plane};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PartSketchSupport&,
        const PartSketchSupport&) = default;
};

struct PartSketch final {
    sketch::SketchId id;
    PartSketchSupport support;
    SketchPlacement placement;
    bool visible{true};
    sketch::SketchModel model;

    friend bool operator==(
        const PartSketch&,
        const PartSketch&) = default;
};

[[nodiscard]] bool isSketchOriginPlane(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] std::optional<PartSketchSupport>
partSketchSupportForBuiltinPlane(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] std::optional<SketchPlacement>
sketchPlacementForSupport(
    const PartSketchSupport& support) noexcept;

[[nodiscard]] bool sketchPlacementMatchesSupport(
    const SketchPlacement& placement,
    const PartSketchSupport& support) noexcept;

} // namespace simplesolid2::part
