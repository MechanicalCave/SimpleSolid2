#pragma once

#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>

#include <compare>
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
};

struct FeatureSurfaceAddress final {
    FeatureId producer_feature_id;
    FeatureSurfaceRoleKind role{
        FeatureSurfaceRoleKind::side};
    std::optional<sketch::EntityId>
        source_entity;
    std::uint32_t loop_index{};
    std::uint32_t use_index{};
    bool hole{false};

    [[nodiscard]] bool valid() const noexcept {
        if (!producer_feature_id.valid()) {
            return false;
        }
        if (role == FeatureSurfaceRoleKind::side ||
            role == FeatureSurfaceRoleKind::revolve_side) {
            return source_entity.has_value() &&
                   source_entity->valid();
        }
        return !source_entity.has_value();
    }

    friend bool operator==(
        const FeatureSurfaceAddress&,
        const FeatureSurfaceAddress&) = default;
    friend auto operator<=>(
        const FeatureSurfaceAddress&,
        const FeatureSurfaceAddress&) = default;
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

using EdgeBranchDiscriminator =
    std::variant<
        SingularAtAuthoredStage,
        BetweenSemanticPoints>;

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
        return endpoints != nullptr &&
               endpoints->valid();
    }

    friend bool operator==(
        const MaterialEdgeReference&,
        const MaterialEdgeReference&) = default;
    friend auto operator<=>(
        const MaterialEdgeReference&,
        const MaterialEdgeReference&) = default;
};

} // namespace simplesolid2::part
