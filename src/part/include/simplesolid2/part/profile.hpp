#pragma once

#include <simplesolid2/part/profile_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/sketch/sketch_id.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstdint>
#include <optional>
#include <string>
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

struct PartProfile final {
    ProfileId id;
    sketch::SketchId source_sketch_id;
    std::string name;
    bool visible{true};
    ProfileRegionIntent region_intent;

    friend bool operator==(
        const PartProfile&,
        const PartProfile&) = default;
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

enum class ProfileAreaEditMode {
    add_area,
    subtract_area,
};

enum class ProfileAreaEditStatus {
    changed,
    no_change,
    invalid_draft,
    invalid_selection,
    disconnected_result,
    ambiguous_topology,
};

struct ProfileAreaEditResult final {
    ProfileAreaEditStatus status{
        ProfileAreaEditStatus::invalid_draft};
    std::optional<ProfileRegionIntent> region_intent;
    std::optional<sketch::RegionCandidate2D> region;
    std::optional<ProfileIntentResolutionStatus>
        draft_resolution_status;

    [[nodiscard]] bool changed() const noexcept {
        return status ==
               ProfileAreaEditStatus::changed;
    }
};

[[nodiscard]] bool
profileRegionIntentStructurallyValid(
    const ProfileRegionIntent& intent) noexcept;

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

// Pure transient-draft operation. No ProfileId allocation, DocumentRevision
// mutation or Undo entry occurs here; Finish persists the returned intent via
// the normal Replace/Edit Profile command.
[[nodiscard]] ProfileAreaEditResult
applyProfileAreaEdit(
    const sketch::SketchModel& model,
    const ProfileRegionIntent& draft,
    std::uint32_t region_index,
    ProfileAreaEditMode mode);

} // namespace simplesolid2::part
