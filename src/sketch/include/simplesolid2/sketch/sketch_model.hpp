#pragma once

#include <simplesolid2/sketch/arc.hpp>
#include <simplesolid2/sketch/circle.hpp>
#include <simplesolid2/sketch/line.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::sketch {

struct SketchLineState final {
    EntityId id;
    Point2 start;
    Point2 end;

    friend bool operator==(
        const SketchLineState&,
        const SketchLineState&) = default;
};

struct SketchCircleState final {
    EntityId id;
    Point2 center;
    double radius{};

    friend bool operator==(
        const SketchCircleState&,
        const SketchCircleState&) = default;
};

struct SketchArcState final {
    EntityId id;
    Point2 center;
    double radius{};
    double start_angle{};
    double sweep_angle{};

    friend bool operator==(
        const SketchArcState&,
        const SketchArcState&) = default;
};

struct SketchModelState final {
    EntityIdCursor next_entity_id;
    std::vector<SketchLineState> lines;
    std::vector<SketchCircleState> circles;
    std::vector<SketchArcState> arcs;

    friend bool operator==(
        const SketchModelState&,
        const SketchModelState&) = default;
};

class SketchModel final {
public:
    [[nodiscard]] EntityId addLine(
        Point2 start,
        Point2 end);

    [[nodiscard]] EntityId addCircle(
        Point2 center,
        double radius);

    [[nodiscard]] EntityId addArc(
        Point2 center,
        double radius,
        double start_angle,
        double sweep_angle);

    [[nodiscard]] const Line* findLine(
        EntityId id) const noexcept;

    [[nodiscard]] const Circle* findCircle(
        EntityId id) const noexcept;

    [[nodiscard]] const Arc* findArc(
        EntityId id) const noexcept;

    [[nodiscard]] bool contains(
        EntityId id) const noexcept;

    [[nodiscard]] bool updateLine(
        EntityId id,
        Point2 start,
        Point2 end) noexcept;

    [[nodiscard]] bool updateCircle(
        EntityId id,
        Point2 center,
        double radius) noexcept;

    [[nodiscard]] bool updateArc(
        EntityId id,
        Point2 center,
        double radius,
        double start_angle,
        double sweep_angle) noexcept;

    [[nodiscard]] bool erase(
        EntityId id) noexcept;

    [[nodiscard]] std::size_t entityCount() const noexcept {
        return lines_.size() +
               circles_.size() +
               arcs_.size();
    }

    [[nodiscard]] EntityIdCursor entityIdCursor() const noexcept {
        return EntityIdCursor{next_entity_value_};
    }

    void preserveEntityIdCursor(
        EntityIdCursor cursor) noexcept;

    [[nodiscard]] SketchModelState state() const;

    [[nodiscard]] static std::optional<SketchModel> restore(
        SketchModelState state);

    friend bool operator==(
        const SketchModel& lhs,
        const SketchModel& rhs) noexcept {
        return lhs.lines_ == rhs.lines_ &&
               lhs.circles_ == rhs.circles_ &&
               lhs.arcs_ == rhs.arcs_;
    }

private:
    [[nodiscard]] EntityId allocateEntityId();

    std::vector<Line> lines_;
    std::vector<Circle> circles_;
    std::vector<Arc> arcs_;
    std::uint64_t next_entity_value_{1U};
};

} // namespace simplesolid2::sketch
