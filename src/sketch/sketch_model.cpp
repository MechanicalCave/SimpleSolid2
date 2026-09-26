#include <simplesolid2/sketch/sketch_model.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <set>
#include <stdexcept>
#include <utility>

namespace simplesolid2::sketch {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;

[[nodiscard]] bool validCircleGeometry(
    Point2 center,
    double radius) noexcept {
    return center.finite() &&
           std::isfinite(radius) &&
           radius > 0.0;
}

[[nodiscard]] bool validArcGeometry(
    Point2 center,
    double radius,
    double start_angle,
    double sweep_angle) noexcept {
    return validCircleGeometry(center, radius) &&
           std::isfinite(start_angle) &&
           std::isfinite(sweep_angle) &&
           sweep_angle != 0.0 &&
           std::abs(sweep_angle) < full_turn;
}

} // namespace

EntityId SketchModel::allocateEntityId() {
    if (next_entity_value_ ==
        std::numeric_limits<std::uint64_t>::max()) {
        throw std::overflow_error{
            "Sketch EntityId space exhausted"};
    }

    const EntityId id{next_entity_value_};
    ++next_entity_value_;
    return id;
}

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

    const auto id = allocateEntityId();
    lines_.push_back(Line{id, start, end});
    return id;
}

EntityId SketchModel::addCircle(
    Point2 center,
    double radius) {
    if (!validCircleGeometry(center, radius)) {
        throw std::invalid_argument{
            "Sketch Circle center/radius must be finite and radius positive"};
    }

    const auto id = allocateEntityId();
    circles_.push_back(Circle{id, center, radius});
    return id;
}

EntityId SketchModel::addArc(
    Point2 center,
    double radius,
    double start_angle,
    double sweep_angle) {
    if (!validArcGeometry(
            center,
            radius,
            start_angle,
            sweep_angle)) {
        throw std::invalid_argument{
            "Sketch Arc geometry is invalid"};
    }

    const auto id = allocateEntityId();
    arcs_.push_back(
        Arc{
            id,
            center,
            radius,
            start_angle,
            sweep_angle});
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

const Circle* SketchModel::findCircle(
    EntityId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }

    const auto found = std::find_if(
        circles_.cbegin(),
        circles_.cend(),
        [id](const Circle& circle) {
            return circle.id() == id;
        });

    return found == circles_.cend()
        ? nullptr
        : &*found;
}

const Arc* SketchModel::findArc(
    EntityId id) const noexcept {
    if (!id.valid()) {
        return nullptr;
    }

    const auto found = std::find_if(
        arcs_.cbegin(),
        arcs_.cend(),
        [id](const Arc& arc) {
            return arc.id() == id;
        });

    return found == arcs_.cend()
        ? nullptr
        : &*found;
}

bool SketchModel::contains(
    EntityId id) const noexcept {
    return findLine(id) != nullptr ||
           findCircle(id) != nullptr ||
           findArc(id) != nullptr;
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

bool SketchModel::updateCircle(
    EntityId id,
    Point2 center,
    double radius) noexcept {
    if (!id.valid() ||
        !validCircleGeometry(center, radius)) {
        return false;
    }

    const auto found = std::find_if(
        circles_.begin(),
        circles_.end(),
        [id](const Circle& circle) {
            return circle.id() == id;
        });

    if (found == circles_.end()) {
        return false;
    }

    *found = Circle{id, center, radius};
    return true;
}

bool SketchModel::updateArc(
    EntityId id,
    Point2 center,
    double radius,
    double start_angle,
    double sweep_angle) noexcept {
    if (!id.valid() ||
        !validArcGeometry(
            center,
            radius,
            start_angle,
            sweep_angle)) {
        return false;
    }

    const auto found = std::find_if(
        arcs_.begin(),
        arcs_.end(),
        [id](const Arc& arc) {
            return arc.id() == id;
        });

    if (found == arcs_.end()) {
        return false;
    }

    *found = Arc{
        id,
        center,
        radius,
        start_angle,
        sweep_angle};
    return true;
}

bool SketchModel::erase(
    EntityId id) noexcept {
    if (!id.valid()) {
        return false;
    }

    const auto line = std::find_if(
        lines_.begin(),
        lines_.end(),
        [id](const Line& value) {
            return value.id() == id;
        });
    if (line != lines_.end()) {
        lines_.erase(line);
        return true;
    }

    const auto circle = std::find_if(
        circles_.begin(),
        circles_.end(),
        [id](const Circle& value) {
            return value.id() == id;
        });
    if (circle != circles_.end()) {
        circles_.erase(circle);
        return true;
    }

    const auto arc = std::find_if(
        arcs_.begin(),
        arcs_.end(),
        [id](const Arc& value) {
            return value.id() == id;
        });
    if (arc != arcs_.end()) {
        arcs_.erase(arc);
        return true;
    }

    return false;
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
    result.circles.reserve(circles_.size());
    result.arcs.reserve(arcs_.size());

    for (const auto& line : lines_) {
        result.lines.push_back(
            SketchLineState{
                line.id(),
                line.start(),
                line.end()});
    }
    for (const auto& circle : circles_) {
        result.circles.push_back(
            SketchCircleState{
                circle.id(),
                circle.center(),
                circle.radius()});
    }
    for (const auto& arc : arcs_) {
        result.arcs.push_back(
            SketchArcState{
                arc.id(),
                arc.center(),
                arc.radius(),
                arc.startAngle(),
                arc.sweepAngle()});
    }

    return result;
}

std::optional<SketchModel> SketchModel::restore(
    SketchModelState state) {
    if (!state.next_entity_id.valid()) {
        return std::nullopt;
    }

    std::set<EntityId> ids;

    SketchModel model;
    model.lines_.reserve(state.lines.size());
    model.circles_.reserve(state.circles.size());
    model.arcs_.reserve(state.arcs.size());

    const auto valid_id =
        [&state, &ids](EntityId id) {
            return id.valid() &&
                   id.value_ <
                       state.next_entity_id.next_value_ &&
                   ids.insert(id).second;
        };

    for (const auto& item : state.lines) {
        if (!valid_id(item.id) ||
            !item.start.finite() ||
            !item.end.finite() ||
            item.start == item.end) {
            return std::nullopt;
        }

        model.lines_.push_back(
            Line{
                item.id,
                item.start,
                item.end});
    }

    for (const auto& item : state.circles) {
        if (!valid_id(item.id) ||
            !validCircleGeometry(
                item.center,
                item.radius)) {
            return std::nullopt;
        }

        model.circles_.push_back(
            Circle{
                item.id,
                item.center,
                item.radius});
    }

    for (const auto& item : state.arcs) {
        if (!valid_id(item.id) ||
            !validArcGeometry(
                item.center,
                item.radius,
                item.start_angle,
                item.sweep_angle)) {
            return std::nullopt;
        }

        model.arcs_.push_back(
            Arc{
                item.id,
                item.center,
                item.radius,
                item.start_angle,
                item.sweep_angle});
    }

    model.next_entity_value_ =
        state.next_entity_id.next_value_;
    return model;
}

} // namespace simplesolid2::sketch
