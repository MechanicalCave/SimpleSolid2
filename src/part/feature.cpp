#include <simplesolid2/part/feature.hpp>

namespace simplesolid2::part {

bool extrudeFeatureStructurallyValid(
    const ExtrudeFeature& feature) noexcept {
    if (!feature.profile_id.valid()) return false;

    switch (feature.operation) {
    case ExtrudeOperation::add:
    case ExtrudeOperation::cut:
        break;
    default:
        return false;
    }

    if (const auto* one_sided =
            std::get_if<OneSidedExtrudeExtent>(&feature.extent)) {
        return one_sided->distance.finite() &&
               one_sided->distance.millimetres > 0.0;
    }

    const auto* midplane =
        std::get_if<MidplaneExtrudeExtent>(&feature.extent);
    return midplane != nullptr &&
           midplane->total_distance.finite() &&
           midplane->total_distance.millimetres > 0.0;
}

bool partFeatureDefinitionStructurallyValid(
    const PartFeatureDefinition& definition) noexcept {
    const auto* extrude = std::get_if<ExtrudeFeature>(&definition);
    return extrude != nullptr &&
           extrudeFeatureStructurallyValid(*extrude);
}

std::optional<ProfileId> sourceProfileId(
    const PartFeature& feature) noexcept {
    const auto* extrude =
        std::get_if<ExtrudeFeature>(&feature.definition);
    return extrude != nullptr && extrude->profile_id.valid()
        ? std::optional<ProfileId>{extrude->profile_id}
        : std::nullopt;
}

} // namespace simplesolid2::part
