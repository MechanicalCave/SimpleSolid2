#include "part_viewer_projection.hpp"

#include <algorithm>

namespace simplesolid2::ui {
namespace {

viewer::PresentationRole presentationRole(
    core::BuiltinReferenceRole role,
    const DocumentSelectionState& selection) {
    const auto target =
        DocumentSelectionTarget::builtinReference(role);

    if (selection.primary &&
        *selection.primary == target) {
        return viewer::PresentationRole::primary_selection;
    }

    if (selection.contains(target)) {
        return viewer::PresentationRole::secondary_selection;
    }

    return viewer::PresentationRole::base;
}

viewer::ReferencePresentation makeReference(
    const part::PartDocument& document,
    core::BuiltinReferenceRole role) {
    viewer::ReferencePresentation item;
    item.token = presentationTokenFor(role);
    item.origin = {0.0, 0.0, 0.0};
    item.visible =
        document.builtinReferenceVisible(role);

    switch (role) {
    case core::BuiltinReferenceRole::origin_point:
        item.kind =
            viewer::ReferencePresentationKind::point;
        item.extent = 4.0;
        break;

    case core::BuiltinReferenceRole::x_axis:
        item.kind =
            viewer::ReferencePresentationKind::x_axis;
        item.u_axis = {1.0, 0.0, 0.0};
        item.extent = 80.0;
        break;

    case core::BuiltinReferenceRole::y_axis:
        item.kind =
            viewer::ReferencePresentationKind::y_axis;
        item.u_axis = {0.0, 1.0, 0.0};
        item.extent = 80.0;
        break;

    case core::BuiltinReferenceRole::z_axis:
        item.kind =
            viewer::ReferencePresentationKind::z_axis;
        item.u_axis = {0.0, 0.0, 1.0};
        item.extent = 80.0;
        break;

    case core::BuiltinReferenceRole::xy_plane:
        item.kind =
            viewer::ReferencePresentationKind::plane;
        item.u_axis = {1.0, 0.0, 0.0};
        item.v_axis = {0.0, 1.0, 0.0};
        item.extent = 55.0;
        break;

    case core::BuiltinReferenceRole::xz_plane:
        item.kind =
            viewer::ReferencePresentationKind::plane;
        item.u_axis = {1.0, 0.0, 0.0};
        item.v_axis = {0.0, 0.0, 1.0};
        item.extent = 55.0;
        break;

    case core::BuiltinReferenceRole::yz_plane:
        item.kind =
            viewer::ReferencePresentationKind::plane;
        item.u_axis = {0.0, 1.0, 0.0};
        item.v_axis = {0.0, 0.0, 1.0};
        item.extent = 55.0;
        break;
    }

    return item;
}

} // namespace

viewer::PresentationToken presentationTokenFor(
    core::BuiltinReferenceRole role) noexcept {
    return {
        static_cast<std::uint64_t>(
            core::builtinReferenceBit(role))};
}

std::optional<core::BuiltinReferenceRole>
builtinReferenceFor(
    viewer::PresentationToken token) noexcept {
    if (!token.valid() ||
        token.value > 0xFFU) {
        return std::nullopt;
    }

    const auto bit =
        static_cast<std::uint8_t>(token.value);

    for (const auto role :
         core::builtin_reference_roles) {
        if (core::builtinReferenceBit(role) == bit) {
            return role;
        }
    }

    return std::nullopt;
}

viewer::ReferenceScene partReferenceScene(
    const part::PartDocument& document,
    const DocumentSelectionState& selection) {
    viewer::ReferenceScene scene;
    scene.references.reserve(
        core::builtin_reference_roles.size());

    for (const auto role :
         core::builtin_reference_roles) {
        auto item = makeReference(document, role);
        item.role =
            presentationRole(role, selection);
        scene.references.push_back(
            std::move(item));
    }

    return scene;
}

viewer::ReferenceGridPresentation
defaultPartReferenceGrid() noexcept {
    viewer::ReferenceGridPresentation grid;
    grid.origin = {0.0, 0.0, 0.0};
    grid.u_axis = {1.0, 0.0, 0.0};
    grid.v_axis = {0.0, 1.0, 0.0};
    grid.spacing = 10.0;
    grid.major_step = 5U;
    grid.extent = 500.0;
    grid.visible = true;
    return grid;
}

} // namespace simplesolid2::ui
