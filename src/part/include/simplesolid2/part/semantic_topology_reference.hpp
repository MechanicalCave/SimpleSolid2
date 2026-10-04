#pragma once

#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/sketch/entity_id.hpp>

#include <compare>
#include <cstdint>
#include <optional>

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
        if (role == FeatureSurfaceRoleKind::side) {
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

} // namespace simplesolid2::part
