#include <simplesolid2/kernel/solid_modeling.hpp>

#include <cmath>

namespace simplesolid2::kernel {
namespace {

[[nodiscard]] bool capRole(
    ExtrudeCapRole role) noexcept {
    switch (role) {
    case ExtrudeCapRole::profile_cap:
    case ExtrudeCapRole::extent_cap:
    case ExtrudeCapRole::negative_cap:
    case ExtrudeCapRole::positive_cap:
        return true;
    }
    return false;
}

} // namespace

bool ExtrudeFaceRole::valid() const noexcept {
    if (kind == ExtrudeFaceRoleKind::cap) {
        return cap_role.has_value() &&
               capRole(*cap_role) &&
               !side_provenance.has_value();
    }
    if (kind == ExtrudeFaceRoleKind::side) {
        return !cap_role.has_value() &&
               side_provenance.has_value() &&
               !side_provenance
                    ->source_entity.empty();
    }
    return false;
}

bool LinearExtrudeInput::valid() const noexcept {
    if (!profile.valid() ||
        !std::isfinite(start_offset_mm) ||
        !std::isfinite(end_offset_mm) ||
        !(start_offset_mm < end_offset_mm) ||
        !capRole(start_cap_role) ||
        !capRole(end_cap_role) ||
        start_cap_role == end_cap_role) {
        return false;
    }

    switch (operation) {
    case SolidBooleanOperation::add:
    case SolidBooleanOperation::cut:
        break;
    default:
        return false;
    }

    const bool forward_one_side =
        start_offset_mm == 0.0 &&
        end_offset_mm > 0.0 &&
        start_cap_role ==
            ExtrudeCapRole::profile_cap &&
        end_cap_role ==
            ExtrudeCapRole::extent_cap;
    const bool reverse_one_side =
        start_offset_mm < 0.0 &&
        end_offset_mm == 0.0 &&
        start_cap_role ==
            ExtrudeCapRole::extent_cap &&
        end_cap_role ==
            ExtrudeCapRole::profile_cap;
    const bool midplane =
        start_offset_mm < 0.0 &&
        end_offset_mm > 0.0 &&
        start_offset_mm == -end_offset_mm &&
        start_cap_role ==
            ExtrudeCapRole::negative_cap &&
        end_cap_role ==
            ExtrudeCapRole::positive_cap;

    return forward_one_side ||
           reverse_one_side ||
           midplane;
}

} // namespace simplesolid2::kernel
