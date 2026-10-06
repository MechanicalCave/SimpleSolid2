#include <simplesolid2/part/feature.hpp>

#include <numbers>

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

bool revolveFeatureStructurallyValid(
    const RevolveFeature& feature) noexcept {
    if (!feature.profile_id.valid() ||
        !feature.axis.valid()) {
        return false;
    }

    switch (feature.operation) {
    case RevolveOperation::add:
    case RevolveOperation::cut:
        break;
    default:
        return false;
    }

    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;

    if (const auto* one_sided =
            std::get_if<OneSidedRevolveExtent>(
                &feature.extent)) {
        return one_sided->angle.finite() &&
               one_sided->angle.radians > 0.0 &&
               one_sided->angle.radians <=
                   full_turn;
    }

    const auto* midplane =
        std::get_if<MidplaneRevolveExtent>(
            &feature.extent);
    return midplane != nullptr &&
           midplane->total_angle.finite() &&
           midplane->total_angle.radians > 0.0 &&
           midplane->total_angle.radians <=
               full_turn;
}

bool partFeatureDefinitionStructurallyValid(
    const PartFeatureDefinition& definition) noexcept {
    if (const auto* extrude =
            std::get_if<ExtrudeFeature>(
                &definition)) {
        return extrudeFeatureStructurallyValid(
            *extrude);
    }
    const auto* revolve =
        std::get_if<RevolveFeature>(
            &definition);
    return revolve != nullptr &&
           revolveFeatureStructurallyValid(
               *revolve);
}

std::optional<ProfileId> sourceProfileId(
    const PartFeature& feature) noexcept {
    if (const auto* extrude =
            std::get_if<ExtrudeFeature>(
                &feature.definition)) {
        return extrude->profile_id.valid()
            ? std::optional<ProfileId>{
                  extrude->profile_id}
            : std::nullopt;
    }
    const auto* revolve =
        std::get_if<RevolveFeature>(
            &feature.definition);
    return revolve != nullptr &&
                   revolve->profile_id.valid()
        ? std::optional<ProfileId>{
              revolve->profile_id}
        : std::nullopt;
}

const AxisReference* sourceAxisReference(
    const PartFeature& feature) noexcept {
    const auto* revolve =
        std::get_if<RevolveFeature>(
            &feature.definition);
    return revolve != nullptr &&
                   revolve->axis.valid()
        ? &revolve->axis
        : nullptr;
}

} // namespace simplesolid2::part
