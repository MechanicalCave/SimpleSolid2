#include <simplesolid2/sketch/sketch_model.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace simplesolid2::sketch {

EntityId SketchModel::addLine(
    Point2 start,
    Point2 end) {
    if (!start.finite() || !end.finite()) {
        throw std::invalid_argument{
            "Sketch Line coordinates must be finite"};
    }

    if (start == end) {
        throw std::invalid_argument{
            "Sketch Line start and end must be distinct"};
    }

    if (next_entity_value_ ==
        std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error{
            "Sketch EntityId space exhausted"};
    }

    const EntityId id{next_entity_value_};
    const Line line{id, start, end};

    lines_.push_back(line);
    ++next_entity_value_;

    return id;
}

const Line* SketchModel::findLine(
    EntityId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }

    const auto found = std::find_if(
        lines_.cbegin(),
        lines_.cend(),
        [id](const Line& line) {
            return line.id() == id;
        });

    return found == lines_.cend()
        ? nullptr
        : &*found;
}

bool SketchModel::erase(
    EntityId id) noexcept {
    if (!id.valid()) {
        return false;
    }

    const auto found = std::find_if(
        lines_.begin(),
        lines_.end(),
        [id](const Line& line) {
            return line.id() == id;
        });

    if (found == lines_.end()) {
        return false;
    }

    lines_.erase(found);
    return true;
}

} // namespace simplesolid2::sketch
