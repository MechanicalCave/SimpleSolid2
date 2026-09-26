#include <simplesolid2/sketch/sketch_model.hpp>

#include <algorithm>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

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

bool SketchModel::updateLine(
    EntityId id,
    Point2 start,
    Point2 end) noexcept {
    if (!id.valid() ||
        !start.finite() ||
        !end.finite() ||
        start == end) {
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

    *found = Line{id, start, end};
    return true;
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

void SketchModel::preserveEntityIdCursor(
    EntityIdCursor cursor) noexcept {
    if (cursor.next_value_ > next_entity_value_) {
        next_entity_value_ = cursor.next_value_;
    }
}

SketchModelState SketchModel::state() const {
    SketchModelState result;
    result.next_entity_id =
        EntityIdCursor{next_entity_value_};
    result.lines.reserve(lines_.size());

    for (const auto& line : lines_) {
        result.lines.push_back(
            SketchLineState{
                line.id(),
                line.start(),
                line.end()});
    }

    return result;
}

std::optional<SketchModel> SketchModel::restore(
    SketchModelState state) {
    std::set<EntityId> ids;

    SketchModel model;
    model.lines_.reserve(state.lines.size());

    for (const auto& item : state.lines) {
        if (!item.id.valid() ||
            item.id.value_ >=
                state.next_entity_id.next_value_ ||
            !item.start.finite() ||
            !item.end.finite() ||
            item.start == item.end ||
            !ids.insert(item.id).second) {
            return std::nullopt;
        }

        model.lines_.push_back(
            Line{
                item.id,
                item.start,
                item.end});
    }

    model.next_entity_value_ =
        state.next_entity_id.next_value_;
    return model;
}

} // namespace simplesolid2::sketch
