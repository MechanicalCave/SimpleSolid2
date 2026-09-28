#pragma once

#include <simplesolid2/part/profile_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::part {

enum class ProfileBoundaryAnchorKind {
    endpoint_start,
    endpoint_end,
    intersection,
};

struct ProfileBoundaryAnchor final {
    ProfileBoundaryAnchorKind kind{
        ProfileBoundaryAnchorKind::endpoint_start};
    sketch::EntityId other_entity;
    std::uint32_t canonical_branch{};

    friend bool operator==(
        const ProfileBoundaryAnchor&,
        const ProfileBoundaryAnchor&) = default;
};

struct ProfileBoundaryUseIntent final {
    sketch::EntityId source_entity;
    std::optional<ProfileBoundaryAnchor> start_anchor;
    std::optional<ProfileBoundaryAnchor> end_anchor;
    bool follows_source_direction{true};
    bool whole_closed_curve{false};

    friend bool operator==(
        const ProfileBoundaryUseIntent&,
        const ProfileBoundaryUseIntent&) = default;
};

struct ProfileLoopIntent final {
    std::vector<ProfileBoundaryUseIntent> boundary;

    friend bool operator==(
        const ProfileLoopIntent&,
        const ProfileLoopIntent&) = default;
};

struct ProfileRegionIntent final {
    ProfileLoopIntent outer;
    std::vector<ProfileLoopIntent> holes;

    friend bool operator==(
        const ProfileRegionIntent&,
        const ProfileRegionIntent&) = default;
};

enum class ProfileIntentResolutionStatus {
    valid,
    invalid_intent,
    missing_source_entity,
    ambiguous_topology,
    unresolved_intent,
};

struct ResolvedProfileRegion final {
    ProfileIntentResolutionStatus status{
        ProfileIntentResolutionStatus::invalid_intent};
    std::optional<sketch::RegionCandidate2D> region;

    [[nodiscard]] bool valid() const noexcept {
        return status ==
                   ProfileIntentResolutionStatus::valid &&
               region.has_value();
    }
};

// Converts derived Shared-2D topology to durable Part intent. Parameters,
// coordinates, Viewer tokens and tessellation are deliberately discarded.
[[nodiscard]] std::optional<ProfileRegionIntent>
makeProfileRegionIntent(
    const sketch::RegionCandidate2D& region);

// Resolves the exact same semantic source intent against current Sketch
// geometry. It never searches for a nearest/similar replacement region.
[[nodiscard]] ResolvedProfileRegion
resolveProfileRegionIntent(
    const sketch::SketchModel& model,
    const ProfileRegionIntent& intent);

} // namespace simplesolid2::part
