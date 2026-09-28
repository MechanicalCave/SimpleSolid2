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
    open_boundary,
    invalid_topology,
};

struct RegionAnalysisDiagnostic2D final {
    RegionAnalysisDiagnosticKind kind{
        RegionAnalysisDiagnosticKind::invalid_topology};
    std::vector<EntityId> entities;
};

enum class RegionBoundaryAnchorKind {
    endpoint_start,
    endpoint_end,
    intersection,
};

struct RegionBoundaryAnchor2D final {
    RegionBoundaryAnchorKind kind{
        RegionBoundaryAnchorKind::endpoint_start};

    // Used only for intersection anchors.
    EntityId other_entity;
    std::uint32_t canonical_branch{};

    friend bool operator==(
        const RegionBoundaryAnchor2D&,
        const RegionBoundaryAnchor2D&) = default;
};

struct RegionBoundaryUse2D final {
    EntityId source_entity;
    double start_parameter{};
    double end_parameter{};

    // Semantic runtime provenance for durable RegionIntent construction.
    // Whole closed curves intentionally have no anchors.
    std::optional<RegionBoundaryAnchor2D> start_anchor;
    std::optional<RegionBoundaryAnchor2D> end_anchor;

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

// Validates one explicit provider-neutral connected region boundary against
// current evaluated Sketch geometry. Boundary parameters are runtime values;
// durable identity remains the semantic anchors carried by each use.
[[nodiscard]] std::optional<RegionCandidate2D>
validateRegionBoundary(
    const SketchModel& model,
    RegionLoop2D outer,
    std::vector<RegionLoop2D> holes);

enum class RegionCompositionStatus {
    valid,
    empty,
    disconnected,
    invalid_topology,
};

struct RegionComposition2D final {
    RegionCompositionStatus status{
        RegionCompositionStatus::invalid_topology};
    std::optional<RegionCandidate2D> region;

    [[nodiscard]] bool valid() const noexcept {
        return status == RegionCompositionStatus::valid &&
               region.has_value();
    }
};

// Returns one guaranteed interior material point for a valid candidate. The
// sample is runtime-only and never becomes durable Profile identity.
[[nodiscard]] std::optional<Point2>
regionInteriorPoint(
    const SketchModel& model,
    const RegionCandidate2D& region);

// Exact cell-union composition used by transient Profile Add/Subtract drafts.
// Shared boundaries are cancelled by semantic runtime fragments; point-only
// contact does not merge separate material components.
[[nodiscard]] RegionComposition2D
composeRegionCells(
    const SketchModel& model,
    const std::vector<RegionCandidate2D>& cells);

// Runtime point-pick against a previously built analysis. A point on any
// profile boundary is reported as boundary, never assigned arbitrarily to an
// adjacent region.
[[nodiscard]] RegionPick2D pickRegion(
    const SketchModel& model,
    const RegionAnalysis2D& analysis,
    Point2 point);

// Returns bounded arrangement regions that are material islands nested at an
// even positive containment depth below a hole of the supplied current
// result. This is runtime analysis only; it creates no authored Profile.
[[nodiscard]] std::vector<std::uint32_t>
nestedIslandRegions(
    const SketchModel& model,
    const RegionAnalysis2D& analysis,
    const RegionCandidate2D& current);

} // namespace simplesolid2::sketch
