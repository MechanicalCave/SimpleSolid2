#include <simplesolid2/sketch/transform.hpp>

#include <set>
#include <utility>

namespace simplesolid2::sketch {

std::optional<SketchTransformGeometry>
captureSketchTransformGeometry(
    const SketchModel& model,
    const std::vector<EntityId>& entity_ids) {
    if (entity_ids.empty()) {
        return std::nullopt;
    }

    SketchTransformGeometry result;
    result.lines.reserve(entity_ids.size());
    result.circles.reserve(entity_ids.size());
    result.arcs.reserve(entity_ids.size());

    std::set<EntityId> seen;

    for (const auto id : entity_ids) {
        if (!id.valid() ||
            !seen.insert(id).second) {
            return std::nullopt;
        }

        if (const auto* line = model.findLine(id)) {
            result.lines.push_back(
                SketchLineState{
                    line->id(),
                    line->start(),
                    line->end()});
            continue;
        }

        if (const auto* circle = model.findCircle(id)) {
            result.circles.push_back(
                SketchCircleState{
                    circle->id(),
                    circle->center(),
                    circle->radius()});
            continue;
        }

        if (const auto* arc = model.findArc(id)) {
            result.arcs.push_back(
                SketchArcState{
                    arc->id(),
                    arc->center(),
                    arc->radius(),
                    arc->startAngle(),
                    arc->sweepAngle()});
            continue;
        }

        return std::nullopt;
    }

    return result.empty()
        ? std::nullopt
        : std::optional<SketchTransformGeometry>{
              std::move(result)};
}

std::optional<SketchTransformGeometry>
translateSketchGeometry(
    const SketchTransformGeometry& geometry,
    Point2 delta) {
    if (geometry.empty() ||
        !delta.finite()) {
        return std::nullopt;
    }

    auto result = geometry;

    for (auto& line : result.lines) {
        line.start.u += delta.u;
        line.start.v += delta.v;
        line.end.u += delta.u;
        line.end.v += delta.v;

        if (!line.id.valid() ||
            !line.start.finite() ||
            !line.end.finite() ||
            line.start == line.end) {
            return std::nullopt;
        }
    }

    for (auto& circle : result.circles) {
        circle.center.u += delta.u;
        circle.center.v += delta.v;

        if (!circle.id.valid() ||
            !circle.center.finite()) {
            return std::nullopt;
        }
    }

    for (auto& arc : result.arcs) {
        arc.center.u += delta.u;
        arc.center.v += delta.v;

        if (!arc.id.valid() ||
            !arc.center.finite()) {
            return std::nullopt;
        }
    }

    return result;
}

} // namespace simplesolid2::sketch
