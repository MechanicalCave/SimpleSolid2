#pragma once

#include <simplesolid2/sketch/sketch_model.hpp>

#include <cmath>
#include <cstdint>
#include <numbers>
#include <optional>
#include <variant>

namespace simplesolid2::sketch {

enum class MeasurementEntityKind : std::uint8_t {
    line,
    circle,
    arc,
};

struct LineMeasurement final {
    EntityId entity_id;
    EntityRole role{EntityRole::regular};
    double length{};
    double delta_u{};
    double delta_v{};
    double angle_from_positive_u{};

    friend bool operator==(
        const LineMeasurement&,
        const LineMeasurement&) = default;
};

struct CircleMeasurement final {
    EntityId entity_id;
    EntityRole role{EntityRole::regular};
    double radius{};
    double diameter{};
    double circumference{};
    double area{};

    friend bool operator==(
        const CircleMeasurement&,
        const CircleMeasurement&) = default;
};

struct ArcMeasurement final {
    EntityId entity_id;
    EntityRole role{EntityRole::regular};
    double radius{};
    double start_angle{};
    double end_angle{};
    double signed_sweep_angle{};
    double arc_length{};

    friend bool operator==(
        const ArcMeasurement&,
        const ArcMeasurement&) = default;
};

using EntityMeasurement =
    std::variant<
        LineMeasurement,
        CircleMeasurement,
        ArcMeasurement>;

[[nodiscard]] inline std::optional<EntityMeasurement>
measureEntity(
    const SketchModel& model,
    EntityId entity_id) noexcept {
    if (!entity_id.valid()) {
        return std::nullopt;
    }

    if (const auto* line = model.findLine(entity_id)) {
        const double delta_u =
            line->end().u - line->start().u;
        const double delta_v =
            line->end().v - line->start().v;
        const double length =
            std::hypot(delta_u, delta_v);
        const double angle =
            std::atan2(delta_v, delta_u);

        if (!std::isfinite(delta_u) ||
            !std::isfinite(delta_v) ||
            !std::isfinite(length) ||
            !std::isfinite(angle)) {
            return std::nullopt;
        }

        return EntityMeasurement{
            LineMeasurement{
                entity_id,
                line->role(),
                length,
                delta_u,
                delta_v,
                angle}};
    }

    if (const auto* circle = model.findCircle(entity_id)) {
        const double radius = circle->radius();
        const double diameter = 2.0 * radius;
        const double circumference =
            2.0 * std::numbers::pi_v<double> * radius;
        const double area =
            std::numbers::pi_v<double> * radius * radius;

        if (!std::isfinite(radius) ||
            !std::isfinite(diameter) ||
            !std::isfinite(circumference) ||
            !std::isfinite(area)) {
            return std::nullopt;
        }

        return EntityMeasurement{
            CircleMeasurement{
                entity_id,
                circle->role(),
                radius,
                diameter,
                circumference,
                area}};
    }

    if (const auto* arc = model.findArc(entity_id)) {
        const double radius = arc->radius();
        const double start = arc->startAngle();
        const double sweep = arc->sweepAngle();
        const double end = start + sweep;
        const double length =
            radius * std::abs(sweep);

        if (!std::isfinite(radius) ||
            !std::isfinite(start) ||
            !std::isfinite(sweep) ||
            !std::isfinite(end) ||
            !std::isfinite(length)) {
            return std::nullopt;
        }

        return EntityMeasurement{
            ArcMeasurement{
                entity_id,
                arc->role(),
                radius,
                start,
                end,
                sweep,
                length}};
    }

    return std::nullopt;
}

} // namespace simplesolid2::sketch
