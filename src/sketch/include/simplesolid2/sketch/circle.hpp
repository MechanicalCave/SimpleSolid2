#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>

namespace simplesolid2::sketch {

class SketchModel;

class Circle final {
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

    friend bool operator==(const Circle&, const Circle&) = default;

private:
    constexpr Circle(
        EntityId id,
        Point2 center,
        double radius) noexcept
        : id_{id},
          center_{center},
          radius_{radius} {}

    EntityId id_;
    Point2 center_;
    double radius_{};

    friend class SketchModel;
};

} // namespace simplesolid2::sketch
