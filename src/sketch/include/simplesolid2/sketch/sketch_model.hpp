#pragma once

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

struct SketchModelState final {
    EntityIdCursor next_entity_id;
    std::vector<SketchLineState> lines;

    friend bool operator==(
        const SketchModelState&,
        const SketchModelState&) = default;
};

class SketchModel final {
public:
    [[nodiscard]] EntityId addLine(
        Point2 start,
        Point2 end);

    [[nodiscard]] const Line* findLine(
        EntityId id) const noexcept;

    [[nodiscard]] bool erase(
        EntityId id) noexcept;

    [[nodiscard]] std::size_t entityCount() const noexcept {
        return lines_.size();
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
        return lhs.lines_ == rhs.lines_;
    }

private:
    std::vector<Line> lines_;
    std::uint64_t next_entity_value_{1U};
};

} // namespace simplesolid2::sketch
