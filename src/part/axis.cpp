#include <simplesolid2/part/axis.hpp>

namespace simplesolid2::part {

bool SketchLineAxisSource::valid() const noexcept {
    return !sketch_id.value().empty() &&
           entity_id.valid();
}

bool BuiltinOriginAxisReference::valid() const noexcept {
    return isOriginAxis(role);
}

bool AxisReference::valid() const noexcept {
    return std::visit(
        [](const auto& reference) {
            return reference.valid();
        },
        value);
}

bool isOriginAxis(
    core::BuiltinReferenceRole role) noexcept {
    return role == core::BuiltinReferenceRole::x_axis ||
           role == core::BuiltinReferenceRole::y_axis ||
           role == core::BuiltinReferenceRole::z_axis;
}

bool partAxisStructurallyValid(
    const PartAxis& axis) noexcept {
    return axis.id.valid() &&
           axis.source.valid();
}

std::optional<core::BuiltinReferenceRole>
builtinOriginAxisForAxisReference(
    const AxisReference& reference) noexcept {
    if (!reference.valid()) {
        return std::nullopt;
    }
    const auto* origin =
        std::get_if<BuiltinOriginAxisReference>(
            &reference.value);
    return origin != nullptr
        ? std::optional<core::BuiltinReferenceRole>{
              origin->role}
        : std::nullopt;
}

std::optional<AxisId>
authoredAxisIdForAxisReference(
    const AxisReference& reference) noexcept {
    if (!reference.valid()) {
        return std::nullopt;
    }
    const auto* authored =
        std::get_if<AuthoredAxisReference>(
            &reference.value);
    return authored != nullptr
        ? std::optional<AxisId>{
              authored->axis_id}
        : std::nullopt;
}

} // namespace simplesolid2::part
