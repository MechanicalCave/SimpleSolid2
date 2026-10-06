#pragma once

#include <simplesolid2/core/document_reference.hpp>
#include <simplesolid2/part/axis_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>

#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace simplesolid2::part {

struct SketchLineAxisSource final {
    sketch::SketchId sketch_id;
    sketch::EntityId entity_id;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const SketchLineAxisSource&,
        const SketchLineAxisSource&) = default;
};

struct PartAxis final {
    AxisId id;
    std::string name;
    SketchLineAxisSource source;
    bool visible{true};

    friend bool operator==(
        const PartAxis&,
        const PartAxis&) = default;
};

struct BuiltinOriginAxisReference final {
    core::BuiltinReferenceRole role{
        core::BuiltinReferenceRole::x_axis};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BuiltinOriginAxisReference&,
        const BuiltinOriginAxisReference&) = default;
};

struct AuthoredAxisReference final {
    AxisId axis_id;

    [[nodiscard]] bool valid() const noexcept {
        return axis_id.valid();
    }

    friend bool operator==(
        const AuthoredAxisReference&,
        const AuthoredAxisReference&) = default;
};

using AxisReferenceValue = std::variant<
    BuiltinOriginAxisReference,
    AuthoredAxisReference>;

struct AxisReference final {
    AxisReferenceValue value{
        BuiltinOriginAxisReference{}};

    AxisReference() = default;
    explicit AxisReference(
        BuiltinOriginAxisReference reference)
        : value{std::move(reference)} {}
    explicit AxisReference(
        AuthoredAxisReference reference)
        : value{std::move(reference)} {}

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const AxisReference&,
        const AxisReference&) = default;
};

[[nodiscard]] bool isOriginAxis(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] bool partAxisStructurallyValid(
    const PartAxis& axis) noexcept;

[[nodiscard]] std::optional<core::BuiltinReferenceRole>
builtinOriginAxisForAxisReference(
    const AxisReference& reference) noexcept;

[[nodiscard]] std::optional<AxisId>
authoredAxisIdForAxisReference(
    const AxisReference& reference) noexcept;

} // namespace simplesolid2::part
