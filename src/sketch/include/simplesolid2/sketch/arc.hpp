#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>

namespace simplesolid2::sketch {

class SketchModel;

class Arc final {
public:
    [[nodiscard]] constexpr EntityId id() const noexcept {
        return id_;
    }

    [[nodiscard]] constexpr const Point2& center() const noexcept {
        return center_;
    }

    [[nodiscard]] constexpr double radius() const noexcept {
        return radius_;
    }

    [[nodiscard]] constexpr double startAngle() const noexcept {
        return start_angle_;
    }

    [[nodiscard]] constexpr double sweepAngle() const noexcept {
        return sweep_angle_;
    }

    friend bool operator==(const Arc&, const Arc&) = default;

private:
    constexpr Arc(
        EntityId id,
        Point2 center,
        double radius,
        double start_angle,
        double sweep_angle) noexcept
        : id_{id},
          center_{center},
          radius_{radius},
          start_angle_{start_angle},
          sweep_angle_{sweep_angle} {}

    EntityId id_;
    Point2 center_;
    double radius_{};
    double start_angle_{};
    double sweep_angle_{};

    friend class SketchModel;
};

} // namespace simplesolid2::sketch
