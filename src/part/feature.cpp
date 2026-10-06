#include <simplesolid2/part/feature.hpp>

#include <algorithm>
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

namespace {

bool edgeSetStructurallyValid(
    const std::vector<MaterialEdgeReference>& edges) noexcept {
    if (edges.empty() ||
        !std::is_sorted(
            edges.begin(),
            edges.end())) {
        return false;
    }

    const auto& stage = edges.front().stage;
    for (std::size_t index = 0U;
         index < edges.size();
         ++index) {
        if (!edges[index].valid() ||
            edges[index].stage != stage) {
            return false;
        }
        if (index > 0U &&
            edges[index - 1U] ==
                edges[index]) {
            return false;
        }
    }
    return true;
}

} // namespace

bool filletFeatureStructurallyValid(
    const FilletFeature& feature) noexcept {
    return edgeSetStructurallyValid(
               feature.edges) &&
           feature.radius.finite() &&
           feature.radius.millimetres > 0.0;
}

bool chamferFeatureStructurallyValid(
    const ChamferFeature& feature) noexcept {
    return edgeSetStructurallyValid(
               feature.edges) &&
           feature.distance.finite() &&
           feature.distance.millimetres > 0.0;
}

const std::vector<MaterialEdgeReference>*
sourceMaterialEdges(
    const PartFeature& feature) noexcept {
    if (const auto* fillet =
            std::get_if<FilletFeature>(
                &feature.definition)) {
        return &fillet->edges;
    }
    if (const auto* chamfer =
            std::get_if<ChamferFeature>(
                &feature.definition)) {
        return &chamfer->edges;
    }
    return nullptr;
}

bool partFeatureDefinitionStructurallyValid(
    const PartFeatureDefinition& definition) noexcept {
    if (const auto* extrude =
            std::get_if<ExtrudeFeature>(
                &definition)) {
        return extrudeFeatureStructurallyValid(
            *extrude);
    }
    if (const auto* revolve =
            std::get_if<RevolveFeature>(
                &definition)) {
        return revolveFeatureStructurallyValid(
            *revolve);
    }
    if (const auto* fillet =
            std::get_if<FilletFeature>(
                &definition)) {
        return filletFeatureStructurallyValid(
            *fillet);
    }
    const auto* chamfer =
        std::get_if<ChamferFeature>(
            &definition);
    return chamfer != nullptr &&
           chamferFeatureStructurallyValid(
               *chamfer);
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
