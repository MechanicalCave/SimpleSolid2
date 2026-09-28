#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstdint>
#include <vector>

namespace simplesolid2::sketch {

enum class CurveRelationStatus {
    invalid,
    disjoint,
    discrete,
    overlap,
};

enum class CurveContactKind {
    proper_crossing,
    endpoint_intersection,
    tangent,
};

struct CurveIntersection2D final {
    Point2 point;

    // Parameters are reported on the canonical EntityId-ordered pair.
    // Line/Arc use [0,1] in authored direction. Circle uses [0,1)
    // counter-clockwise from the +U axis.
    double first_parameter{};
    double second_parameter{};

    // Stable for a fixed valid evaluated pair: intersections are sorted by
    // canonical first-curve parameter, then second-curve parameter.
    std::uint32_t canonical_branch{};

    bool first_endpoint{false};
    bool second_endpoint{false};
    CurveContactKind contact{
        CurveContactKind::proper_crossing};

    friend bool operator==(
        const CurveIntersection2D&,
        const CurveIntersection2D&) = default;
};

struct CurveRelation2D final {
    EntityId first_entity;
    EntityId second_entity;
    CurveRelationStatus status{CurveRelationStatus::invalid};
    std::vector<CurveIntersection2D> intersections;

    [[nodiscard]] bool valid() const noexcept {
        return status != CurveRelationStatus::invalid;
    }
};

// Provider-neutral relation analysis over current authored=evaluated Sketch
// geometry. The returned entity order is canonical (lowest EntityId first),
// independent of call order. Construction role is intentionally not filtered
// here; region analysis applies participation policy above this geometry seam.
[[nodiscard]] CurveRelation2D analyzeCurveRelation(
    const SketchModel& model,
    EntityId first,
    EntityId second);

} // namespace simplesolid2::sketch
