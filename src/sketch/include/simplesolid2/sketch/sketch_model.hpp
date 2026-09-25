#pragma once

#include <simplesolid2/sketch/line.hpp>

#include <cstddef>
#include <cstdint>
#include <vector>

namespace simplesolid2::sketch {

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
