#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <compare>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::sketch {

enum class SnapKind : std::uint8_t {
    endpoint,
    midpoint,
    center,
    quadrant,
    intersection,
    origin,
    perpendicular,
    tangent,
    nearest,
};

enum class SnapSourceKind : std::uint8_t {
    intrinsic_origin,
    entity_point,
    entity_curve,
    intersection,
};

enum class SnapSemanticRole : std::uint8_t {
    intrinsic_origin,
    line_start,
    line_midpoint,
    line_end,
    circle_center,
    circle_quadrant_pos_u,
    circle_quadrant_pos_v,
    circle_quadrant_neg_u,
    circle_quadrant_neg_v,
    arc_center,
    arc_start,
    arc_end,
    arc_midpoint,
    arc_quadrant_pos_u,
    arc_quadrant_pos_v,
    arc_quadrant_neg_u,
    arc_quadrant_neg_v,
    curve_nearest,
    curve_perpendicular,
    curve_tangent,
    intersection,
};

struct SnapSourceRef final {
    SnapSourceKind kind{SnapSourceKind::intrinsic_origin};
    std::optional<EntityId> first_entity;
    std::optional<EntityId> second_entity;
    SnapSemanticRole role{SnapSemanticRole::intrinsic_origin};
    std::uint32_t canonical_branch{};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const SnapSourceRef&,
        const SnapSourceRef&) = default;
};

struct SnapCandidate final {
    Point2 point;
    SnapKind kind{SnapKind::endpoint};
    SnapSourceRef source;

    [[nodiscard]] bool valid() const noexcept {
        return point.finite() && source.valid();
    }

    friend bool operator==(
        const SnapCandidate&,
        const SnapCandidate&) = default;
};

struct SnapModeSet final {
    bool endpoint{true};
    bool midpoint{true};
    bool center{true};
    bool quadrant{true};
    bool intersection{true};
    bool origin{true};
    bool perpendicular{false};
    bool tangent{false};
    bool nearest{false};
    bool extension{false};

    friend bool operator==(
        const SnapModeSet&,
        const SnapModeSet&) = default;
};


enum class TemporarySnapOverrideKind : std::uint8_t {
    endpoint,
    midpoint,
    center,
    quadrant,
    intersection,
    perpendicular,
    tangent,
    nearest,
    origin,
    extension,
    none,
};

struct ObjectSnapPreferences final {
    bool master_enabled{true};
    SnapModeSet modes;
    bool object_tracking_enabled{false};

    friend bool operator==(
        const ObjectSnapPreferences&,
        const ObjectSnapPreferences&) = default;
};

struct SnapEligibility final {
    bool endpoint{};
    bool midpoint{};
    bool center{};
    bool quadrant{};
    bool intersection{};
    bool origin{};
    bool perpendicular{};
    bool tangent{};
    bool nearest{};
    bool extension{};
    bool object_tracking{};
    bool suppress_object_assistance{};

    [[nodiscard]] bool enabled(
        SnapKind kind) const noexcept;

    friend bool operator==(
        const SnapEligibility&,
        const SnapEligibility&) = default;
};

[[nodiscard]] SnapEligibility resolveSnapEligibility(
    const ObjectSnapPreferences& preferences,
    std::optional<TemporarySnapOverrideKind>
        temporary_override = std::nullopt) noexcept;

struct SnapStableKey final {
    std::uint8_t kind_rank{};
    SnapSourceKind source_kind{
        SnapSourceKind::intrinsic_origin};
    std::optional<EntityId> first_entity;
    std::optional<EntityId> second_entity;
    SnapSemanticRole role{
        SnapSemanticRole::intrinsic_origin};
    std::uint32_t canonical_branch{};

    friend auto operator<=>(
        const SnapStableKey&,
        const SnapStableKey&) = default;
};

[[nodiscard]] SnapStableKey snapStableKey(
    const SnapCandidate& candidate) noexcept;

// Exact semantic contact equivalence for same-point collapse. Coordinate
// equality is necessary but not sufficient: unrelated semantic points at the
// same numeric location remain distinct unless Shared 2D relation authority
// proves a discrete contact (or the intrinsic Origin is one side).
[[nodiscard]] bool sameSnapContact(
    const SketchModel& model,
    const SnapCandidate& first,
    const SnapCandidate& second);


struct SnapScreenCandidate final {
    SnapCandidate candidate;
    double screen_distance{};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const SnapScreenCandidate&,
        const SnapScreenCandidate&) = default;
};

struct SnapResolutionPolicy final {
    double capture_distance{9.0};
    double release_distance{15.0};

    [[nodiscard]] bool valid() const noexcept;
};

struct SnapCaptureState final {
    std::optional<SnapStableKey> captured;

    void clear() noexcept {
        captured.reset();
    }
};

struct SnapResolution final {
    SnapCandidate primary;
    std::vector<SnapCandidate> coincident_candidates;

    [[nodiscard]] bool valid() const noexcept {
        return primary.valid() &&
               !coincident_candidates.empty();
    }
};

[[nodiscard]] std::optional<SnapResolution>
resolveScreenSnap(
    const SketchModel& model,
    SnapCaptureState& state,
    const std::vector<SnapScreenCandidate>& candidates,
    SnapResolutionPolicy policy = {});

[[nodiscard]] std::vector<SnapCandidate>
staticSnapCandidates(
    const SketchModel& model,
    const SnapModeSet& modes = {});

[[nodiscard]] std::vector<SnapCandidate>
intersectionSnapCandidates(
    const SketchModel& model,
    EntityId first,
    EntityId second);

[[nodiscard]] std::optional<SnapCandidate>
nearestSnapCandidate(
    const SketchModel& model,
    EntityId entity,
    Point2 pointer) noexcept;

[[nodiscard]] std::vector<SnapCandidate>
perpendicularSnapCandidates(
    const SketchModel& model,
    EntityId entity,
    Point2 base) noexcept;

[[nodiscard]] std::vector<SnapCandidate>
tangentSnapCandidates(
    const SketchModel& model,
    EntityId entity,
    Point2 base) noexcept;

} // namespace simplesolid2::sketch
