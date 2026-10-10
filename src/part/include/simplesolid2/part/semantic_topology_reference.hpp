#pragma once

#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>

#include <algorithm>
#include <compare>
#include <tuple>
#include <cstdint>
#include <optional>
#include <variant>
#include <vector>

namespace simplesolid2::part {

enum class BodyStageKind {
    empty_body,
    after_feature,
};

struct BodyStageRef final {
    BodyStageKind kind{BodyStageKind::empty_body};
    std::optional<FeatureId> feature_id;

    [[nodiscard]] bool valid() const noexcept {
        switch (kind) {
        case BodyStageKind::empty_body:
            return !feature_id.has_value();
        case BodyStageKind::after_feature:
            return feature_id.has_value() &&
                   feature_id->valid();
        }
        return false;
    }

    friend bool operator==(
        const BodyStageRef&,
        const BodyStageRef&) = default;
    friend auto operator<=>(
        const BodyStageRef&,
        const BodyStageRef&) = default;
};

enum class FeatureSurfaceRoleKind {
    profile_cap,
    extent_cap,
    negative_cap,
    positive_cap,
    side,
    revolve_start_cap,
    revolve_end_cap,
    revolve_side,
    fillet_surface,
    chamfer_surface,
    corner_transition,
};

struct FeaturePointAddress;
struct MaterialEdgeReference;

struct FeatureSurfaceAddress final {
    FeatureId producer_feature_id;
    FeatureSurfaceRoleKind role{
        FeatureSurfaceRoleKind::side};
    std::optional<sketch::EntityId>
        source_entity;
    std::uint32_t loop_index{};
    std::uint32_t use_index{};
    bool hole{false};

    // PM-05A P2/P3 durable generated-Surface provenance. These are semantic
    // references only. Runtime/provider topology handles never enter this
    // value graph.
    //
    // fillet_surface / chamfer_surface:
    //   exactly one source MaterialEdgeReference.
    //
    // corner_transition:
    //   exactly one source FeaturePointAddress plus a canonical set of at
    //   least two participating authored MaterialEdgeReferences.
    std::vector<MaterialEdgeReference>
        source_edges;
    std::vector<FeaturePointAddress>
        source_points;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureSurfaceAddress&,
        const FeatureSurfaceAddress&);
    friend std::strong_ordering operator<=>(
        const FeatureSurfaceAddress&,
        const FeatureSurfaceAddress&);
};

// Durable provider-neutral semantic reference to one Surface carrier in one
// declared Body history stage. Stage is part of reference meaning; resolution
// must never fall back to a global final-Body search.
struct SurfaceReference final {
    BodyStageRef stage;
    FeatureSurfaceAddress surface;

    [[nodiscard]] bool valid() const noexcept {
        return stage.valid() &&
               stage.kind ==
                   BodyStageKind::after_feature &&
               surface.valid();
    }

    friend bool operator==(
        const SurfaceReference&,
        const SurfaceReference&) = default;
    friend auto operator<=>(
        const SurfaceReference&,
        const SurfaceReference&) = default;
};

enum class FeatureCurveRoleKind {
    cap_side,
    side_side,
    boolean_intersection,
    edge_feature_boundary,
};

struct FeatureCurveAddress final {
    FeatureId producer_feature_id;
    FeatureCurveRoleKind role{
        FeatureCurveRoleKind::boolean_intersection};
    // Exactly two distinct Surface addresses in canonical semantic order.
    std::vector<FeatureSurfaceAddress>
        adjacent_surfaces;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureCurveAddress&,
        const FeatureCurveAddress&) = default;
    friend auto operator<=>(
        const FeatureCurveAddress&,
        const FeatureCurveAddress&) = default;
};

struct FeaturePointAddress final {
    FeatureId producer_feature_id;
    // Current Part-v1 Point meaning is an intersection of exactly three
    // distinct semantic Surfaces in canonical semantic order.
    std::vector<FeatureSurfaceAddress>
        adjacent_surfaces;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeaturePointAddress&,
        const FeaturePointAddress&) = default;
    friend auto operator<=>(
        const FeaturePointAddress&,
        const FeaturePointAddress&) = default;
};

struct SingularAtAuthoredStage final {
    friend bool operator==(
        const SingularAtAuthoredStage&,
        const SingularAtAuthoredStage&) = default;
    friend auto operator<=>(
        const SingularAtAuthoredStage&,
        const SingularAtAuthoredStage&) = default;
};

struct BetweenSemanticPoints final {
    FeaturePointAddress first;
    FeaturePointAddress second;

    [[nodiscard]] bool valid() const noexcept {
        return first.valid() &&
               second.valid() &&
               first < second;
    }

    friend bool operator==(
        const BetweenSemanticPoints&,
        const BetweenSemanticPoints&) = default;
    friend auto operator<=>(
        const BetweenSemanticPoints&,
        const BetweenSemanticPoints&) = default;
};

// v16: strict single certified semantic endpoint of one Curve segment.
// Never carries provider tokens, coordinates, or a segment ordinal.
struct AtSingleSemanticPoint final {
    FeaturePointAddress point;

    [[nodiscard]] bool valid() const noexcept {
        return point.valid();
    }

    friend bool operator==(
        const AtSingleSemanticPoint&,
        const AtSingleSemanticPoint&) = default;
    friend auto operator<=> (
        const AtSingleSemanticPoint&,
        const AtSingleSemanticPoint&) = default;
};

using EdgeBranchDiscriminator =
    std::variant<
        SingularAtAuthoredStage,
        BetweenSemanticPoints,
        AtSingleSemanticPoint>;

struct MaterialEdgeReference final {
    BodyStageRef stage;
    FeatureCurveAddress curve;
    EdgeBranchDiscriminator branch;

    [[nodiscard]] bool valid() const noexcept {
        if (!stage.valid() ||
            stage.kind !=
                BodyStageKind::after_feature ||
            !curve.valid()) {
            return false;
        }
        if (std::holds_alternative<
                SingularAtAuthoredStage>(
                branch)) {
            return true;
        }
        const auto* endpoints =
            std::get_if<
                BetweenSemanticPoints>(
                &branch);
        if (endpoints != nullptr) {
            return endpoints->valid();
        }
        const auto* single =
            std::get_if<AtSingleSemanticPoint>(
                &branch);
        return single != nullptr && single->valid();
    }

    friend bool operator==(
        const MaterialEdgeReference&,
        const MaterialEdgeReference&) = default;
    friend auto operator<=>(
        const MaterialEdgeReference&,
        const MaterialEdgeReference&) = default;
};

inline bool FeatureSurfaceAddress::valid() const noexcept {
    if (!producer_feature_id.valid()) {
        return false;
    }

    const bool clean_generated_payload =
        !source_entity.has_value() &&
        loop_index == 0U &&
        use_index == 0U &&
        !hole;

    if (role == FeatureSurfaceRoleKind::side ||
        role == FeatureSurfaceRoleKind::revolve_side) {
        return source_entity.has_value() &&
               source_entity->valid() &&
               source_edges.empty() &&
               source_points.empty();
    }

    if (role == FeatureSurfaceRoleKind::fillet_surface ||
        role == FeatureSurfaceRoleKind::chamfer_surface) {
        if (!clean_generated_payload ||
            source_edges.size() != 1U ||
            !source_points.empty()) {
            return false;
        }
        const auto& source =
            source_edges.front();
        return source.valid() &&
               source.stage.feature_id.has_value() &&
               *source.stage.feature_id <
                   producer_feature_id;
    }

    if (role == FeatureSurfaceRoleKind::corner_transition) {
        if (!clean_generated_payload ||
            source_points.size() != 1U ||
            source_edges.size() < 2U ||
            !source_points.front().valid() ||
            !std::is_sorted(
                source_edges.begin(),
                source_edges.end()) ||
            std::adjacent_find(
                source_edges.begin(),
                source_edges.end()) !=
                source_edges.end()) {
            return false;
        }

        const auto stage =
            source_edges.front().stage;
        if (!stage.feature_id ||
            !(*stage.feature_id <
              producer_feature_id) ||
            source_points.front()
                    .producer_feature_id >
                *stage.feature_id) {
            return false;
        }

        return std::all_of(
            source_edges.begin(),
            source_edges.end(),
            [&stage](
                const MaterialEdgeReference& edge) {
                return edge.valid() &&
                       edge.stage == stage;
            });
    }

    return !source_entity.has_value() &&
           source_edges.empty() &&
           source_points.empty();
}

inline bool operator==(
    const FeatureSurfaceAddress& first,
    const FeatureSurfaceAddress& second) {
    return std::tie(
               first.producer_feature_id,
               first.role,
               first.source_entity,
               first.loop_index,
               first.use_index,
               first.hole,
               first.source_edges,
               first.source_points) ==
           std::tie(
               second.producer_feature_id,
               second.role,
               second.source_entity,
               second.loop_index,
               second.use_index,
               second.hole,
               second.source_edges,
               second.source_points);
}

inline std::strong_ordering operator<=>(
    const FeatureSurfaceAddress& first,
    const FeatureSurfaceAddress& second) {
    return std::tie(
               first.producer_feature_id,
               first.role,
               first.source_entity,
               first.loop_index,
               first.use_index,
               first.hole,
               first.source_edges,
               first.source_points) <=>
           std::tie(
               second.producer_feature_id,
               second.role,
               second.source_entity,
               second.loop_index,
               second.use_index,
               second.hole,
               second.source_edges,
               second.source_points);
}

} // namespace simplesolid2::part
