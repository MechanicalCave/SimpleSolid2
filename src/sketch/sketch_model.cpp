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

[[nodiscard]] bool validEntityRole(
    EntityRole role) noexcept {
    return role == EntityRole::regular ||
           role == EntityRole::construction;
}

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

// Exact semantic shape and duplicate identity, without any numeric solve.
[[nodiscard]] bool validConstraintShape(
    ConstraintKind kind,
    ConstraintReference first,
    ConstraintReference second) noexcept {
    if (!first.entity.valid()) return false;
    switch (kind) {
    case ConstraintKind::horizontal:
    case ConstraintKind::vertical:
    case ConstraintKind::fix:
        return first.element == ConstraintElement::line &&
               second == ConstraintReference{};
    case ConstraintKind::coincident:
        return (first.element == ConstraintElement::line_start ||
                first.element == ConstraintElement::line_end) &&
               (second.element == ConstraintElement::line_start ||
                second.element == ConstraintElement::line_end) &&
               second.entity.valid() && first.entity != second.entity;
    case ConstraintKind::parallel:
    case ConstraintKind::perpendicular:
        return first.element == ConstraintElement::line &&
               second.element == ConstraintElement::line &&
               second.entity.valid() && first.entity != second.entity;
    }
    return false; // unknown future enum values fail closed
}

[[nodiscard]] bool sameConstraintMeaning(
    const SketchConstraintState& lhs,
    const SketchConstraintState& rhs) noexcept {
    if (lhs.kind != rhs.kind) return false;
    if (lhs.first == rhs.first && lhs.second == rhs.second) return true;
    return lhs.kind != ConstraintKind::horizontal &&
           lhs.kind != ConstraintKind::vertical &&
           lhs.kind != ConstraintKind::fix &&
           lhs.first == rhs.second && lhs.second == rhs.first;
}

// Only mathematically unconditional contradictions belong at this layer.
// Geometric residuals, redundant systems and fixed-reference feasibility
// are determined later by the proven donor numerical adapter.
[[nodiscard]] bool directConflict(
    const SketchConstraintState& lhs,
    const SketchConstraintState& rhs) noexcept {
    const bool hv =
        (lhs.kind == ConstraintKind::horizontal &&
         rhs.kind == ConstraintKind::vertical) ||
        (lhs.kind == ConstraintKind::vertical &&
         rhs.kind == ConstraintKind::horizontal);
    if (hv && lhs.first.entity == rhs.first.entity) return true;

    const bool pair_conflict =
        (lhs.kind == ConstraintKind::parallel &&
         rhs.kind == ConstraintKind::perpendicular) ||
        (lhs.kind == ConstraintKind::perpendicular &&
         rhs.kind == ConstraintKind::parallel);
    return pair_conflict &&
           ((lhs.first == rhs.first && lhs.second == rhs.second) ||
            (lhs.first == rhs.second && lhs.second == rhs.first));
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
    Point2 end,
    EntityRole role) {
    if (!validEntityRole(role)) {
        throw std::invalid_argument{
            "Sketch EntityRole is invalid"};
    }

    if (!start.finite() || !end.finite()) {
        throw std::invalid_argument{
            "Sketch Line coordinates must be finite"};
    }

    if (start == end) {
        throw std::invalid_argument{
            "Sketch Line start and end must be distinct"};
    }

    const auto id = allocateEntityId();
    lines_.push_back(Line{id, start, end, role});
    return id;
}

EntityId SketchModel::addCircle(
    Point2 center,
    double radius,
    EntityRole role) {
    if (!validEntityRole(role)) {
        throw std::invalid_argument{
            "Sketch EntityRole is invalid"};
    }
    if (!validCircleGeometry(center, radius)) {
        throw std::invalid_argument{
            "Sketch Circle center/radius must be finite and radius positive"};
    }

    const auto id = allocateEntityId();
    circles_.push_back(Circle{id, center, radius, role});
    return id;
}

EntityId SketchModel::addArc(
    Point2 center,
    double radius,
    double start_angle,
    double sweep_angle,
    EntityRole role) {
    if (!validEntityRole(role)) {
        throw std::invalid_argument{
            "Sketch EntityRole is invalid"};
    }
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
            sweep_angle,
            role});
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

    if (found == lines_.end() || isFixedLine(id)) {
        return false;
    }

    *found = Line{id, start, end, found->role()};
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

    *found = Circle{id, center, radius, found->role()};
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
        sweep_angle,
        found->role()};
    return true;
}

bool SketchModel::setEntityRole(
    EntityId id,
    EntityRole role) noexcept {
    if (!id.valid() || !validEntityRole(role)) return false;
    if (auto it=std::find_if(lines_.begin(),lines_.end(),[id](const Line& v){return v.id()==id;}); it!=lines_.end()) { *it=Line{it->id(),it->start(),it->end(),role}; return true; }
    if (auto it=std::find_if(circles_.begin(),circles_.end(),[id](const Circle& v){return v.id()==id;}); it!=circles_.end()) { *it=Circle{it->id(),it->center(),it->radius(),role}; return true; }
    if (auto it=std::find_if(arcs_.begin(),arcs_.end(),[id](const Arc& v){return v.id()==id;}); it!=arcs_.end()) { *it=Arc{it->id(),it->center(),it->radius(),it->startAngle(),it->sweepAngle(),role}; return true; }
    return false;
}

void SketchModel::preserveConstraintIdCursor(
    ConstraintIdCursor cursor) noexcept {
    if (cursor.next_value_ > next_constraint_value_) {
        next_constraint_value_ = cursor.next_value_;
    }
}

std::optional<ConstraintId> SketchModel::addConstraint(
    ConstraintKind kind,
    ConstraintReference first,
    ConstraintReference second) {
    if (!validConstraintShape(kind, first, second) ||
        findLine(first.entity) == nullptr ||
        (second.entity.valid() && findLine(second.entity) == nullptr) ||
        next_constraint_value_ == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
    }

    const SketchConstraintState candidate{
        ConstraintId{next_constraint_value_}, kind, first, second};
    const auto duplicate = std::find_if(
        constraints_.cbegin(), constraints_.cend(),
        [&candidate](const SketchConstraintState& existing) {
            return sameConstraintMeaning(existing, candidate) ||
                   directConflict(existing, candidate);
        });
    if (duplicate != constraints_.cend()) {
        return std::nullopt;
    }

    constraints_.push_back(candidate);
    ++next_constraint_value_;
    return candidate.id;
}

bool SketchModel::isFixedLine(EntityId line) const noexcept {
    if (!line.valid()) return false;
    return std::any_of(
        constraints_.cbegin(), constraints_.cend(),
        [line](const SketchConstraintState& relation) {
            return relation.kind == ConstraintKind::fix &&
                relation.first.entity == line;
        });
}

std::optional<ConstraintId> SketchModel::addHorizontal(EntityId line) {
    return addConstraint(
        ConstraintKind::horizontal,
        ConstraintReference{line, ConstraintElement::line});
}

const SketchConstraintState* SketchModel::findConstraint(ConstraintId id) const noexcept {
    if (!id.valid()) return nullptr;
    const auto found = std::find_if(
        constraints_.cbegin(), constraints_.cend(),
        [id](const SketchConstraintState& value) { return value.id == id; });
    return found == constraints_.cend() ? nullptr : &*found;
}

bool SketchModel::eraseConstraint(ConstraintId id) noexcept {
    if (!id.valid()) return false;
    const auto found = std::find_if(
        constraints_.begin(), constraints_.end(),
        [id](const SketchConstraintState& value) { return value.id == id; });
    if (found == constraints_.end()) return false;
    constraints_.erase(found);
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
        std::erase_if(constraints_, [id](const SketchConstraintState& relation) {
            return relation.first.entity == id || relation.second.entity == id;
        });
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
        std::erase_if(constraints_, [id](const SketchConstraintState& relation) {
            return relation.first.entity == id || relation.second.entity == id;
        });
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
        std::erase_if(constraints_, [id](const SketchConstraintState& relation) {
            return relation.first.entity == id || relation.second.entity == id;
        });
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
    result.next_constraint_id = ConstraintIdCursor{next_constraint_value_};
    result.constraints = constraints_;
    result.lines.reserve(lines_.size());
    result.circles.reserve(circles_.size());
    result.arcs.reserve(arcs_.size());

    for (const auto& line : lines_) {
        result.lines.push_back(
            SketchLineState{
                line.id(),
                line.start(),
                line.end(),
                line.role()});
    }
    for (const auto& circle : circles_) {
        result.circles.push_back(
            SketchCircleState{
                circle.id(),
                circle.center(),
                circle.radius(),
                circle.role()});
    }
    for (const auto& arc : arcs_) {
        result.arcs.push_back(
            SketchArcState{
                arc.id(),
                arc.center(),
                arc.radius(),
                arc.startAngle(),
                arc.sweepAngle(),
                arc.role()});
    }

    return result;
}

std::optional<SketchModel> SketchModel::restore(
    SketchModelState state) {
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
            !validEntityRole(item.role) ||
            !item.start.finite() ||
            !item.end.finite() ||
            item.start == item.end) {
            return std::nullopt;
        }

        model.lines_.push_back(
            Line{
                item.id,
                item.start,
                item.end,
                item.role});
    }

    for (const auto& item : state.circles) {
        if (!valid_id(item.id) ||
            !validEntityRole(item.role) ||
            !validCircleGeometry(
                item.center,
                item.radius)) {
            return std::nullopt;
        }

        model.circles_.push_back(
            Circle{
                item.id,
                item.center,
                item.radius,
                item.role});
    }

    for (const auto& item : state.arcs) {
        if (!valid_id(item.id) ||
            !validEntityRole(item.role) ||
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
                item.sweep_angle,
                item.role});
    }

    // Restore all six explicit kinds; no malformed, stale, duplicate,
    // same-entity pair or unrecognised future kind may enter owned state.
    std::set<ConstraintId> seen_constraints;
    std::vector<SketchConstraintState> seen_meanings;
    seen_meanings.reserve(state.constraints.size());
    for (const auto& item : state.constraints) {
        if (!item.id.valid() ||
            item.id.value_ >= state.next_constraint_id.next_value_ ||
            !seen_constraints.insert(item.id).second ||
            !validConstraintShape(item.kind, item.first, item.second) ||
            model.findLine(item.first.entity) == nullptr ||
            (item.second.entity.valid() &&
             model.findLine(item.second.entity) == nullptr) ||
            std::any_of(
                seen_meanings.cbegin(), seen_meanings.cend(),
                [&item](const SketchConstraintState& existing) {
                    return sameConstraintMeaning(item, existing) ||
                           directConflict(item, existing);
                })) {
            return std::nullopt;
        }
        seen_meanings.push_back(item);
    }
    model.constraints_ = std::move(state.constraints);
    model.next_constraint_value_ = state.next_constraint_id.next_value_;
    model.next_entity_value_ = state.next_entity_id.next_value_;
    return model;
}

} // namespace simplesolid2::sketch
