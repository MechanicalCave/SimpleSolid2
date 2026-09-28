#pragma once

#include <simplesolid2/sketch/entity_id.hpp>
#include <simplesolid2/sketch/point2.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cstdint>
#include <optional>
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

enum class RegionAnalysisDiagnosticKind {
    ambiguous_overlap,
    invalid_topology,
};

struct RegionAnalysisDiagnostic2D final {
    RegionAnalysisDiagnosticKind kind{
        RegionAnalysisDiagnosticKind::invalid_topology};
    std::vector<EntityId> entities;
};

struct RegionBoundaryUse2D final {
    EntityId source_entity;
    double start_parameter{};
    double end_parameter{};
    bool follows_source_direction{true};
    bool crosses_closed_seam{false};
    bool whole_closed_curve{false};

    friend bool operator==(
        const RegionBoundaryUse2D&,
        const RegionBoundaryUse2D&) = default;
};

struct RegionLoop2D final {
    std::vector<RegionBoundaryUse2D> boundary;
    double signed_area{};
    double perimeter{};
};

struct RegionCandidate2D final {
    std::uint32_t region_index{};
    RegionLoop2D outer;
    std::vector<RegionLoop2D> holes;
    double area{};
    double perimeter{};
};

struct RegionAnalysis2D final {
    std::vector<RegionCandidate2D> regions;
    std::vector<RegionAnalysisDiagnostic2D> diagnostics;

    [[nodiscard]] bool complete() const noexcept {
        return diagnostics.empty();
    }
};

enum class RegionPointLocation {
    outside,
    inside,
    boundary,
    ambiguous,
};

struct RegionPick2D final {
    RegionPointLocation location{
        RegionPointLocation::outside};
    std::optional<std::uint32_t> region_index;
};

// Builds bounded faces only from current Regular geometry. Construction
// entities never split/close a region. Ambiguous overlap invalidates only the
// connected topology component that contains it; unrelated components remain
// available for region picking.
[[nodiscard]] RegionAnalysis2D analyzeRegions(
    const SketchModel& model);

// Runtime point-pick against a previously built analysis. A point on any
// profile boundary is reported as boundary, never assigned arbitrarily to an
// adjacent region.
[[nodiscard]] RegionPick2D pickRegion(
    const SketchModel& model,
    const RegionAnalysis2D& analysis,
    Point2 point);

} // namespace simplesolid2::sketch
