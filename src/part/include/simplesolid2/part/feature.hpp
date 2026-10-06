#pragma once

#include <simplesolid2/core/units.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/body_id.hpp>
#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/part/profile_id.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace simplesolid2::part {

struct ModelingSemanticsVersion final {
    std::uint32_t value{1U};
    [[nodiscard]] constexpr bool valid() const noexcept { return value != 0U; }
    friend bool operator==(const ModelingSemanticsVersion&,
                           const ModelingSemanticsVersion&) = default;
};

inline constexpr ModelingSemanticsVersion current_modeling_semantics_version{1U};

enum class ExtrudeOperation : std::uint8_t {
    add,
    cut,
};

struct OneSidedExtrudeExtent final {
    core::LengthValue distance;
    bool reversed{false};
    friend bool operator==(const OneSidedExtrudeExtent&,
                           const OneSidedExtrudeExtent&) = default;
};

struct MidplaneExtrudeExtent final {
    core::LengthValue total_distance;
    friend bool operator==(const MidplaneExtrudeExtent&,
                           const MidplaneExtrudeExtent&) = default;
};

using ExtrudeExtent =
    std::variant<OneSidedExtrudeExtent, MidplaneExtrudeExtent>;

struct ExtrudeFeature final {
    ProfileId profile_id;
    ExtrudeOperation operation{ExtrudeOperation::add};
    ExtrudeExtent extent;
    friend bool operator==(const ExtrudeFeature&,
                           const ExtrudeFeature&) = default;
};

enum class RevolveOperation : std::uint8_t {
    add,
    cut,
};

struct OneSidedRevolveExtent final {
    core::AngleValue angle;
    bool reversed{false};

    friend bool operator==(
        const OneSidedRevolveExtent&,
        const OneSidedRevolveExtent&) = default;
};

struct MidplaneRevolveExtent final {
    core::AngleValue total_angle;

    friend bool operator==(
        const MidplaneRevolveExtent&,
        const MidplaneRevolveExtent&) = default;
};

using RevolveExtent =
    std::variant<
        OneSidedRevolveExtent,
        MidplaneRevolveExtent>;

struct RevolveFeature final {
    ProfileId profile_id;
    AxisReference axis;
    RevolveOperation operation{
        RevolveOperation::add};
    RevolveExtent extent;

    friend bool operator==(
        const RevolveFeature&,
        const RevolveFeature&) = default;
};

using PartFeatureDefinition =
    std::variant<
        ExtrudeFeature,
        RevolveFeature>;

struct PartFeature final {
    FeatureId id;
    std::string name;
    bool suppressed{false};
    PartFeatureDefinition definition;
    friend bool operator==(const PartFeature&, const PartFeature&) = default;
};

struct PartBody final {
    BodyId id;
    FeatureIdCursor next_feature_id;
    std::vector<PartFeature> features;
    friend bool operator==(const PartBody&, const PartBody&) = default;
};

[[nodiscard]] bool extrudeFeatureStructurallyValid(
    const ExtrudeFeature& feature) noexcept;
[[nodiscard]] bool revolveFeatureStructurallyValid(
    const RevolveFeature& feature) noexcept;
[[nodiscard]] bool partFeatureDefinitionStructurallyValid(
    const PartFeatureDefinition& definition) noexcept;
[[nodiscard]] std::optional<ProfileId> sourceProfileId(
    const PartFeature& feature) noexcept;
[[nodiscard]] const AxisReference* sourceAxisReference(
    const PartFeature& feature) noexcept;

} // namespace simplesolid2::part
