#include <simplesolid2/part/profile_kernel_input.hpp>

#include <simplesolid2/sketch/arc.hpp>
#include <simplesolid2/sketch/circle.hpp>
#include <simplesolid2/sketch/line.hpp>

#include <cmath>
#include <cstdint>
#include <optional>
#include <utility>

namespace simplesolid2::part {
namespace {

[[nodiscard]] kernel::Point2 point(
    const sketch::Point2& source) noexcept {
    return {source.u, source.v};
}

[[nodiscard]] kernel::Point3 point(
    const std::array<double, 3>& source) noexcept {
    return {source[0], source[1], source[2]};
}

[[nodiscard]] std::optional<kernel::Point3> normal(
    const SketchPlacement& placement) noexcept {
    const kernel::Point3 u = point(placement.u_axis);
    const kernel::Point3 v = point(placement.v_axis);
    kernel::Point3 n{
        u.y * v.z - u.z * v.y,
        u.z * v.x - u.x * v.z,
        u.x * v.y - u.y * v.x,
    };
    const double length =
        std::sqrt(
            n.x * n.x +
            n.y * n.y +
            n.z * n.z);
    if (!std::isfinite(length) || !(length > 0.0)) {
        return std::nullopt;
    }
    n.x /= length;
    n.y /= length;
    n.z /= length;
    return n;
}

[[nodiscard]] std::optional<kernel::Curve2> curve(
    const sketch::SketchModel& model,
    sketch::EntityId id) {
    if (const auto* line = model.findLine(id)) {
        return kernel::Line2{
            point(line->start()),
            point(line->end())};
    }
    if (const auto* circle = model.findCircle(id)) {
        return kernel::Circle2{
            point(circle->center()),
            circle->radius()};
    }
    if (const auto* arc = model.findArc(id)) {
        return kernel::Arc2{
            point(arc->center()),
            arc->radius(),
            arc->startAngle(),
            arc->sweepAngle()};
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<kernel::ProfileLoopInput>
convertLoop(
    const sketch::SketchModel& model,
    const sketch::RegionLoop2D& source,
    std::uint32_t loop_index,
    bool hole) {
    kernel::ProfileLoopInput result;
    result.boundary.reserve(source.boundary.size());

    for (std::size_t index = 0U;
         index < source.boundary.size();
         ++index) {
        const auto& use = source.boundary[index];
        const auto exact_curve =
            curve(model, use.source_entity);
        if (!exact_curve) {
            return std::nullopt;
        }

        kernel::BoundaryUse2D converted;
        converted.curve = *exact_curve;
        converted.start_parameter =
            use.start_parameter;
        converted.end_parameter =
            use.end_parameter;
        converted.follows_source_direction =
            use.follows_source_direction;
        converted.crosses_closed_seam =
            use.crosses_closed_seam;
        converted.whole_closed_curve =
            use.whole_closed_curve;
        converted.provenance = {
            use.source_entity.serialized(),
            loop_index,
            static_cast<std::uint32_t>(index),
            hole,
        };
        if (!converted.valid()) {
            return std::nullopt;
        }
        result.boundary.push_back(
            std::move(converted));
    }

    return result.valid()
        ? std::optional<kernel::ProfileLoopInput>{
              std::move(result)}
        : std::nullopt;
}

} // namespace

std::optional<kernel::PlanarProfileInput>
makeKernelProfileInput(
    const PartDocument& document,
    ProfileId profile_id) {
    const auto* profile =
        document.findProfile(profile_id);
    if (!profile) {
        return std::nullopt;
    }

    const auto* source =
        document.findSketch(
            profile->source_sketch_id);
    if (!source ||
        !source->placement.valid() ||
        !sketchPlacementMatchesSupport(
            source->placement,
            source->support)) {
        return std::nullopt;
    }

    const auto resolved =
        document.evaluateProfile(profile_id);
    if (!resolved || !resolved->valid()) {
        return std::nullopt;
    }

    const auto n = normal(source->placement);
    if (!n) {
        return std::nullopt;
    }

    kernel::PlanarProfileInput result;
    result.frame = {
        point(source->placement.origin),
        point(source->placement.u_axis),
        point(source->placement.v_axis),
        *n,
    };

    auto outer =
        convertLoop(
            source->model,
            resolved->region->outer,
            0U,
            false);
    if (!outer) {
        return std::nullopt;
    }
    result.outer = std::move(*outer);

    result.holes.reserve(
        resolved->region->holes.size());
    for (std::size_t index = 0U;
         index < resolved->region->holes.size();
         ++index) {
        auto hole =
            convertLoop(
                source->model,
                resolved->region->holes[index],
                static_cast<std::uint32_t>(
                    index + 1U),
                true);
        if (!hole) {
            return std::nullopt;
        }
        result.holes.push_back(
            std::move(*hole));
    }

    return result.valid()
        ? std::optional<kernel::PlanarProfileInput>{
              std::move(result)}
        : std::nullopt;
}

} // namespace simplesolid2::part
