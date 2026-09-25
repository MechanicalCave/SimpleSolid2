#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>

namespace simplesolid2::sketch {

class SketchModel;

class Line final {
public:
    [[nodiscard]] constexpr EntityId id() const noexcept {
        return id_;
    }

    [[nodiscard]] constexpr const Point2& start() const noexcept {
        return start_;
    }

    [[nodiscard]] constexpr const Point2& end() const noexcept {
        return end_;
    }

    friend bool operator==(const Line&, const Line&) = default;

private:
    constexpr Line(
        EntityId id,
        Point2 start,
        Point2 end) noexcept
        : id_{id},
          start_{start},
          end_{end} {}

    EntityId id_;
    Point2 start_;
    Point2 end_;

    friend class SketchModel;
};

} // namespace simplesolid2::sketch
