#include <simplesolid2/sketch/measurement.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>
#include <simplesolid2/sketch/sketch_model.hpp>

#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << '\n'; \
            return 1; \
        } \
    } while (false)

template <class Fn>
bool rejectsInvalidArgument(Fn&& fn) {
    try {
        fn();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

} // namespace

int main() {
    using namespace simplesolid2::sketch;

    SketchModel model;
    CHECK(model.entityCount() == 0U);
    CHECK(!EntityId{}.valid());

    const auto nan = std::numeric_limits<double>::quiet_NaN();
    const auto inf = std::numeric_limits<double>::infinity();

    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{nan, 0.0}, Point2{1.0, 0.0});
    }));
    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{0.0, 0.0}, Point2{inf, 1.0});
    }));
    CHECK(model.entityCount() == 0U);

    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{2.0, -4.0}, Point2{2.0, -4.0});
    }));
    CHECK(model.entityCount() == 0U);

    const auto first_id =
        model.addLine(Point2{0.0, 0.0}, Point2{10.0, 0.0});
    const auto second_id =
        model.addLine(
            Point2{10.0, 0.0},
            Point2{10.0, 5.0},
            EntityRole::construction);

    CHECK(first_id.valid());
    CHECK(second_id.valid());
    CHECK(first_id != second_id);
    CHECK(model.entityCount() == 2U);

    const auto* first = model.findLine(first_id);
    const auto* second = model.findLine(second_id);
    CHECK(first != nullptr);
    CHECK(second != nullptr);
    CHECK(first->id() == first_id);
    CHECK((first->start() == Point2{0.0, 0.0}));
    CHECK((first->end() == Point2{10.0, 0.0}));
    CHECK((second->start() == Point2{10.0, 0.0}));
    CHECK((second->end() == Point2{10.0, 5.0}));
    CHECK(first->role() == EntityRole::regular);
    CHECK(second->role() == EntityRole::construction);

    CHECK(model.setEntityRole(
        first_id,
        EntityRole::construction));
    CHECK(
        model.findLine(first_id)->role() ==
        EntityRole::construction);
    CHECK(model.updateLine(
        first_id,
        Point2{0.0, 0.0},
        Point2{10.0, 0.0}));
    CHECK(
        model.findLine(first_id)->role() ==
        EntityRole::construction);
    CHECK(model.setEntityRole(
        first_id,
        EntityRole::regular));

    // Equal endpoint coordinates are merely equal values. They do not create
    // another authored object, shared Point identity or persistent relation.
    CHECK(first->end() == second->start());

    auto copied = model;
    CHECK(copied.entityCount() == 2U);
    CHECK(copied.findLine(first_id) != nullptr);
    CHECK(copied.findLine(second_id) != nullptr);

    CHECK(copied.erase(first_id));
    CHECK(copied.entityCount() == 1U);
    CHECK(copied.findLine(first_id) == nullptr);

    // Value-copy state is independent: editing one state cannot alias another.
    CHECK(model.entityCount() == 2U);
    CHECK(model.findLine(first_id) != nullptr);

    CHECK(model.erase(first_id));
    CHECK(model.entityCount() == 1U);
    CHECK(model.findLine(first_id) == nullptr);

    // Storage compaction/reordering must not alter another entity's identity.
    second = model.findLine(second_id);
    CHECK(second != nullptr);
    CHECK(second->id() == second_id);
    CHECK((second->start() == Point2{10.0, 0.0}));
    CHECK((second->end() == Point2{10.0, 5.0}));

    CHECK(!model.erase(first_id));
    CHECK(!model.erase(EntityId{}));
    CHECK(model.entityCount() == 1U);

    const auto third_id =
        model.addLine(Point2{-2.0, 1.0}, Point2{-1.0, 2.0});
    CHECK(third_id.valid());
    CHECK(third_id != first_id);
    CHECK(third_id != second_id);

    // R1-A deliberately has no epsilon/near-zero rejection policy.
    const auto tiny_id =
        model.addLine(Point2{0.0, 0.0}, Point2{1.0e-300, 0.0});
    CHECK(tiny_id.valid());
    CHECK(model.findLine(tiny_id) != nullptr);

    const auto before_invalid = model.entityCount();
    CHECK(rejectsInvalidArgument([&] {
        (void)model.addLine(Point2{7.0, 7.0}, Point2{7.0, 7.0});
    }));
    CHECK(model.entityCount() == before_invalid);


    // F region-analysis seam: relation classification is provider-neutral,
    // canonical by EntityId and independent from Viewer tessellation.
    SketchModel relations;
    const auto horizontal =
        relations.addLine(
            Point2{-2.0, 0.0},
            Point2{2.0, 0.0});
    const auto vertical =
        relations.addLine(
            Point2{0.0, -2.0},
            Point2{0.0, 2.0});

    const auto crossing =
        analyzeCurveRelation(
            relations,
            vertical,
            horizontal);
    CHECK(crossing.valid());
    CHECK(
        crossing.status ==
        CurveRelationStatus::discrete);
    CHECK(crossing.first_entity == horizontal);
    CHECK(crossing.second_entity == vertical);
    CHECK(crossing.intersections.size() == 1U);
    CHECK(
        (crossing.intersections.front().point ==
         Point2{0.0, 0.0}));
    CHECK(
        crossing.intersections.front().contact ==
        CurveContactKind::proper_crossing);
    CHECK(
        crossing.intersections.front().canonical_branch ==
        0U);

    const auto endpoint_line =
        relations.addLine(
            Point2{2.0, 0.0},
            Point2{2.0, 3.0});
    const auto endpoint =
        analyzeCurveRelation(
            relations,
            horizontal,
            endpoint_line);
    CHECK(
        endpoint.status ==
        CurveRelationStatus::discrete);
    CHECK(endpoint.intersections.size() == 1U);
    CHECK(
        endpoint.intersections.front().contact ==
        CurveContactKind::endpoint_intersection);
    CHECK(endpoint.intersections.front().first_endpoint);
    CHECK(endpoint.intersections.front().second_endpoint);

    const auto overlapping =
        relations.addLine(
            Point2{-1.0, 0.0},
            Point2{1.0, 0.0});
    CHECK(
        analyzeCurveRelation(
            relations,
            horizontal,
            overlapping)
            .status ==
        CurveRelationStatus::overlap);

    const auto unit_circle =
        relations.addCircle(
            Point2{0.0, 0.0},
            1.0);
    const auto line_circle =
        analyzeCurveRelation(
            relations,
            horizontal,
            unit_circle);
    CHECK(
        line_circle.status ==
        CurveRelationStatus::discrete);
    CHECK(line_circle.intersections.size() == 2U);
    CHECK(
        line_circle.intersections[0].canonical_branch ==
        0U);
    CHECK(
        line_circle.intersections[1].canonical_branch ==
        1U);
    CHECK(
        line_circle.intersections[0].first_parameter <
        line_circle.intersections[1].first_parameter);

    const auto tangent_line =
        relations.addLine(
            Point2{-2.0, 1.0},
            Point2{2.0, 1.0});
    const auto tangent =
        analyzeCurveRelation(
            relations,
            tangent_line,
            unit_circle);
    CHECK(
        tangent.status ==
        CurveRelationStatus::discrete);
    CHECK(tangent.intersections.size() == 1U);
    CHECK(
        tangent.intersections.front().contact ==
        CurveContactKind::tangent);

    const auto second_circle =
        relations.addCircle(
            Point2{1.0, 0.0},
            1.0);
    const auto circle_circle =
        analyzeCurveRelation(
            relations,
            unit_circle,
            second_circle);
    CHECK(
        circle_circle.status ==
        CurveRelationStatus::discrete);
    CHECK(circle_circle.intersections.size() == 2U);
    CHECK(
        circle_circle.intersections[0].canonical_branch ==
        0U);
    CHECK(
        circle_circle.intersections[1].canonical_branch ==
        1U);

    const auto tangent_circle =
        relations.addCircle(
            Point2{2.0, 0.0},
            1.0);
    const auto circle_tangent =
        analyzeCurveRelation(
            relations,
            unit_circle,
            tangent_circle);
    CHECK(
        circle_tangent.status ==
        CurveRelationStatus::discrete);
    CHECK(circle_tangent.intersections.size() == 1U);
    CHECK(
        circle_tangent.intersections.front().contact ==
        CurveContactKind::tangent);

    const auto coincident_circle =
        relations.addCircle(
            Point2{0.0, 0.0},
            1.0);
    CHECK(
        analyzeCurveRelation(
            relations,
            unit_circle,
            coincident_circle)
            .status ==
        CurveRelationStatus::overlap);

    const double pi =
        std::numbers::pi_v<double>;
    const auto upper_arc =
        relations.addArc(
            Point2{0.0, 0.0},
            1.0,
            0.0,
            pi);
    const auto vertical_arc =
        analyzeCurveRelation(
            relations,
            vertical,
            upper_arc);
    CHECK(
        vertical_arc.status ==
        CurveRelationStatus::discrete);
    CHECK(vertical_arc.intersections.size() == 1U);
    CHECK(
        std::abs(
            vertical_arc.intersections.front().point.v -
            1.0) < 1.0e-12);

    CHECK(
        analyzeCurveRelation(
            relations,
            unit_circle,
            upper_arc)
            .status ==
        CurveRelationStatus::overlap);

    const auto right_arc =
        relations.addArc(
            Point2{0.0, 0.0},
            2.0,
            -pi * 0.5,
            pi);
    const auto left_arc =
        relations.addArc(
            Point2{2.0, 0.0},
            2.0,
            pi * 0.5,
            pi);
    const auto arc_arc =
        analyzeCurveRelation(
            relations,
            right_arc,
            left_arc);
    CHECK(
        arc_arc.status ==
        CurveRelationStatus::discrete);
    CHECK(arc_arc.intersections.size() == 2U);

    const auto first_quarter =
        relations.addArc(
            Point2{5.0, 0.0},
            1.0,
            0.0,
            pi * 0.5);
    const auto second_quarter =
        relations.addArc(
            Point2{5.0, 0.0},
            1.0,
            pi * 0.5,
            pi * 0.5);
    const auto shared_arc_endpoint =
        analyzeCurveRelation(
            relations,
            first_quarter,
            second_quarter);
    CHECK(
        shared_arc_endpoint.status ==
        CurveRelationStatus::discrete);
    CHECK(
        shared_arc_endpoint.intersections.size() ==
        1U);
    CHECK(
        shared_arc_endpoint.intersections.front().contact ==
        CurveContactKind::endpoint_intersection);

    const auto overlapping_arc =
        relations.addArc(
            Point2{5.0, 0.0},
            1.0,
            pi * 0.25,
            pi * 0.5);
    CHECK(
        analyzeCurveRelation(
            relations,
            first_quarter,
            overlapping_arc)
            .status ==
        CurveRelationStatus::overlap);

    CHECK(
        analyzeCurveRelation(
            relations,
            EntityId{},
            horizontal)
            .status ==
        CurveRelationStatus::invalid);
    CHECK(
        analyzeCurveRelation(
            relations,
            horizontal,
            horizontal)
            .status ==
        CurveRelationStatus::invalid);


    // F bounded-region analysis. Construction geometry does not participate.
    SketchModel rectangle;
    const auto bottom =
        rectangle.addLine(
            Point2{0.0, 0.0},
            Point2{4.0, 0.0});
    const auto right =
        rectangle.addLine(
            Point2{4.0, 0.0},
            Point2{4.0, 2.0});
    const auto top =
        rectangle.addLine(
            Point2{4.0, 2.0},
            Point2{0.0, 2.0});
    const auto left =
        rectangle.addLine(
            Point2{0.0, 2.0},
            Point2{0.0, 0.0});
    (void)bottom;
    (void)right;
    (void)top;
    (void)left;

    const auto construction_diagonal =
        rectangle.addLine(
            Point2{0.0, 0.0},
            Point2{4.0, 2.0},
            EntityRole::construction);
    (void)construction_diagonal;

    const auto rectangle_regions =
        analyzeRegions(rectangle);
    CHECK(rectangle_regions.complete());
    CHECK(rectangle_regions.regions.size() == 1U);
    CHECK(
        std::abs(
            rectangle_regions.regions.front().area -
            8.0) < 1.0e-12);
    CHECK(
        rectangle_regions.regions.front()
            .holes.empty());

    const auto rectangle_inside =
        pickRegion(
            rectangle,
            rectangle_regions,
            Point2{1.0, 1.0});
    CHECK(
        rectangle_inside.location ==
        RegionPointLocation::inside);
    CHECK(
        rectangle_inside.region_index.has_value());
    CHECK(
        *rectangle_inside.region_index == 0U);

    CHECK(
        pickRegion(
            rectangle,
            rectangle_regions,
            Point2{5.0, 1.0})
            .location ==
        RegionPointLocation::outside);
    CHECK(
        pickRegion(
            rectangle,
            rectangle_regions,
            Point2{0.0, 1.0})
            .location ==
        RegionPointLocation::boundary);

    // A blind T-branch does not divide the bounded material region.
    SketchModel tee = rectangle;
    (void)tee.addLine(
        Point2{2.0, 0.0},
        Point2{2.0, 1.0});
    const auto tee_regions =
        analyzeRegions(tee);
    CHECK(tee_regions.complete());
    CHECK(tee_regions.regions.size() == 1U);
    CHECK(
        std::abs(
            tee_regions.regions.front().area -
            8.0) < 1.0e-12);

    // Two through-lines create four independent bounded arrangement cells.
    SketchModel grid = rectangle;
    (void)grid.addLine(
        Point2{2.0, 0.0},
        Point2{2.0, 2.0});
    (void)grid.addLine(
        Point2{0.0, 1.0},
        Point2{4.0, 1.0});
    const auto grid_regions =
        analyzeRegions(grid);
    CHECK(grid_regions.complete());
    CHECK(grid_regions.regions.size() == 4U);
    bool saw_intersection_anchor = false;
    for (const auto& region :
         grid_regions.regions) {
        CHECK(
            std::abs(region.area - 2.0) <
            1.0e-12);
        for (const auto& use :
             region.outer.boundary) {
            CHECK(use.start_anchor.has_value());
            CHECK(use.end_anchor.has_value());
            if (use.start_anchor->kind ==
                    RegionBoundaryAnchorKind::intersection ||
                use.end_anchor->kind ==
                    RegionBoundaryAnchorKind::intersection) {
                saw_intersection_anchor = true;
            }
        }
    }
    CHECK(saw_intersection_anchor);

    // A standalone Circle is a bounded region without authored endpoints.
    SketchModel circle_region_model;
    (void)circle_region_model.addCircle(
        Point2{0.0, 0.0},
        2.0);
    const auto circle_regions =
        analyzeRegions(circle_region_model);
    CHECK(circle_regions.complete());
    CHECK(circle_regions.regions.size() == 1U);
    CHECK(
        circle_regions.regions.front()
            .outer.boundary.size() == 1U);
    CHECK(
        circle_regions.regions.front()
            .outer.boundary.front()
            .whole_closed_curve);
    CHECK(
        !circle_regions.regions.front()
             .outer.boundary.front()
             .start_anchor.has_value());
    CHECK(
        !circle_regions.regions.front()
             .outer.boundary.front()
             .end_anchor.has_value());
    CHECK(
        std::abs(
            circle_regions.regions.front().area -
            4.0 * pi) < 1.0e-12);
    CHECK(
        pickRegion(
            circle_region_model,
            circle_regions,
            Point2{0.0, 0.0})
            .location ==
        RegionPointLocation::inside);

    // Arc + Line may form a closed exact bounded region.
    SketchModel arc_region_model;
    (void)arc_region_model.addArc(
        Point2{0.0, 0.0},
        1.0,
        0.0,
        pi);
    (void)arc_region_model.addLine(
        Point2{-1.0, 0.0},
        Point2{1.0, 0.0});
    const auto arc_regions =
        analyzeRegions(arc_region_model);
    CHECK(arc_regions.complete());
    CHECK(arc_regions.regions.size() == 1U);
    for (const auto& use :
         arc_regions.regions.front().outer.boundary) {
        CHECK(use.start_anchor.has_value());
        CHECK(use.end_anchor.has_value());
        CHECK(
            use.start_anchor->kind !=
            RegionBoundaryAnchorKind::intersection);
        CHECK(
            use.end_anchor->kind !=
            RegionBoundaryAnchorKind::intersection);
    }
    CHECK(
        std::abs(
            arc_regions.regions.front().area -
            pi * 0.5) < 1.0e-12);
    CHECK(
        pickRegion(
            arc_region_model,
            arc_regions,
            Point2{0.0, 0.5})
            .location ==
        RegionPointLocation::inside);

    // Disconnected nested loops form an annular candidate plus the inner disk.
    SketchModel nested;
    (void)nested.addLine(
        Point2{-3.0, -3.0},
        Point2{3.0, -3.0});
    (void)nested.addLine(
        Point2{3.0, -3.0},
        Point2{3.0, 3.0});
    (void)nested.addLine(
        Point2{3.0, 3.0},
        Point2{-3.0, 3.0});
    (void)nested.addLine(
        Point2{-3.0, 3.0},
        Point2{-3.0, -3.0});
    (void)nested.addCircle(
        Point2{0.0, 0.0},
        1.0);
    const auto nested_regions =
        analyzeRegions(nested);
    CHECK(nested_regions.complete());
    CHECK(nested_regions.regions.size() == 2U);

    std::size_t regions_with_holes = 0U;
    for (const auto& region :
         nested_regions.regions) {
        if (!region.holes.empty()) {
            ++regions_with_holes;
            CHECK(region.holes.size() == 1U);
            CHECK(
                std::abs(
                    region.area -
                    (36.0 - pi)) <
                1.0e-12);
        }
    }
    CHECK(regions_with_holes == 1U);
    CHECK(
        pickRegion(
            nested,
            nested_regions,
            Point2{2.0, 0.0})
            .location ==
        RegionPointLocation::inside);

    // A non-boundary point remains pickable even when both axis-aligned
    // classification rays are tangent to the circular hole.
    const auto tangent_ray_pick =
        pickRegion(
            nested,
            nested_regions,
            Point2{-1.0, -1.0});
    CHECK(
        tangent_ray_pick.location ==
        RegionPointLocation::inside);
    CHECK(tangent_ray_pick.region_index.has_value());

    CHECK(
        pickRegion(
            nested,
            nested_regions,
            Point2{0.0, 0.0})
            .location ==
        RegionPointLocation::inside);

    // Point-only tangent contact does not combine two material regions.
    SketchModel tangent_regions_model;
    (void)tangent_regions_model.addCircle(
        Point2{-1.0, 0.0},
        1.0);
    (void)tangent_regions_model.addCircle(
        Point2{1.0, 0.0},
        1.0);
    const auto tangent_regions =
        analyzeRegions(
            tangent_regions_model);
    CHECK(tangent_regions.complete());
    CHECK(tangent_regions.regions.size() == 2U);

    // Overlap invalidates only its connected component; unrelated valid
    // regions remain available.
    SketchModel local_overlap = rectangle;
    (void)local_overlap.addLine(
        Point2{10.0, 0.0},
        Point2{14.0, 0.0});
    (void)local_overlap.addLine(
        Point2{11.0, 0.0},
        Point2{13.0, 0.0});
    const auto overlap_regions =
        analyzeRegions(local_overlap);
    CHECK(!overlap_regions.complete());
    CHECK(overlap_regions.diagnostics.size() == 1U);
    CHECK(
        overlap_regions.diagnostics.front().kind ==
        RegionAnalysisDiagnosticKind::
            ambiguous_overlap);
    CHECK(overlap_regions.regions.size() == 1U);
    CHECK(
        pickRegion(
            local_overlap,
            overlap_regions,
            Point2{1.0, 1.0})
            .location ==
        RegionPointLocation::inside);

    // Package F diagnostics keep open geometry local and explicit. An open
    // chain does not become a region and does not use a gap tolerance.
    SketchModel open_gap;
    (void)open_gap.addLine(
        Point2{0.0, 0.0},
        Point2{10.0, 0.0});
    (void)open_gap.addLine(
        Point2{10.0, 0.0},
        Point2{10.0, 10.0});
    (void)open_gap.addLine(
        Point2{10.0, 10.0},
        Point2{0.0, 10.0});
    const auto open_gap_regions =
        analyzeRegions(open_gap);
    CHECK(open_gap_regions.regions.empty());
    CHECK(open_gap_regions.diagnostics.size() == 1U);
    CHECK(
        open_gap_regions.diagnostics.front().kind ==
        RegionAnalysisDiagnosticKind::open_boundary);

    // Three nested circles model outer material, a hole-space cell and an
    // inner disconnected material island. Detect Islands reports only the
    // even-depth material island, not the immediate hole-space cell.
    SketchModel nested_islands;
    (void)nested_islands.addCircle(
        Point2{0.0, 0.0}, 10.0);
    (void)nested_islands.addCircle(
        Point2{0.0, 0.0}, 6.0);
    (void)nested_islands.addCircle(
        Point2{0.0, 0.0}, 2.0);
    const auto nested_analysis =
        analyzeRegions(nested_islands);
    CHECK(nested_analysis.complete());
    CHECK(nested_analysis.regions.size() == 3U);
    const auto outer_pick =
        pickRegion(
            nested_islands,
            nested_analysis,
            Point2{8.0, 0.0});
    CHECK(outer_pick.region_index.has_value());
    const auto outer_region =
        std::find_if(
            nested_analysis.regions.begin(),
            nested_analysis.regions.end(),
            [&outer_pick](
                const RegionCandidate2D& item) {
                return item.region_index ==
                       *outer_pick.region_index;
            });
    CHECK(outer_region != nested_analysis.regions.end());
    const auto islands =
        nestedIslandRegions(
            nested_islands,
            nested_analysis,
            *outer_region);
    CHECK(islands.size() == 1U);
    const auto island_pick =
        pickRegion(
            nested_islands,
            nested_analysis,
            Point2{0.0, 0.0});
    CHECK(island_pick.region_index.has_value());
    CHECK(islands.front() == *island_pick.region_index);


    // R8A: read-only single-entity measurement consumes semantic Sketch
    // geometry directly and remains independent from Viewer sampling.
    SketchModel measurement_model;
    const auto measured_line =
        measurement_model.addLine(
            Point2{1.0, 2.0},
            Point2{4.0, 6.0});
    const auto measured_construction =
        measurement_model.addLine(
            Point2{0.0, 0.0},
            Point2{0.0, 5.0},
            EntityRole::construction);
    const auto measured_circle =
        measurement_model.addCircle(
            Point2{10.0, 20.0},
            2.0);
    const auto measured_arc_ccw =
        measurement_model.addArc(
            Point2{0.0, 0.0},
            4.0,
            0.25,
            std::numbers::pi_v<double> * 0.5);
    const auto measured_arc_cw =
        measurement_model.addArc(
            Point2{0.0, 0.0},
            3.0,
            1.0,
            -std::numbers::pi_v<double> * 0.25);

    const auto line_measurement =
        measureEntity(measurement_model, measured_line);
    CHECK(line_measurement.has_value());
    CHECK(std::holds_alternative<LineMeasurement>(
        *line_measurement));
    const auto& line_result =
        std::get<LineMeasurement>(*line_measurement);
    CHECK(line_result.entity_id == measured_line);
    CHECK(line_result.role == EntityRole::regular);
    CHECK(std::abs(line_result.length - 5.0) < 1e-12);
    CHECK(std::abs(line_result.delta_u - 3.0) < 1e-12);
    CHECK(std::abs(line_result.delta_v - 4.0) < 1e-12);
    CHECK(
        std::abs(
            line_result.angle_from_positive_u -
            std::atan2(4.0, 3.0)) < 1e-12);

    const auto construction_measurement =
        measureEntity(
            measurement_model,
            measured_construction);
    CHECK(construction_measurement.has_value());
    CHECK(std::holds_alternative<LineMeasurement>(
        *construction_measurement));
    const auto& construction_result =
        std::get<LineMeasurement>(
            *construction_measurement);
    CHECK(
        construction_result.role ==
        EntityRole::construction);
    CHECK(
        std::abs(
            construction_result.length - 5.0) <
        1e-12);
    CHECK(
        std::abs(
            construction_result.angle_from_positive_u -
            std::numbers::pi_v<double> * 0.5) <
        1e-12);

    const auto circle_measurement =
        measureEntity(measurement_model, measured_circle);
    CHECK(circle_measurement.has_value());
    CHECK(std::holds_alternative<CircleMeasurement>(
        *circle_measurement));
    const auto& circle_result =
        std::get<CircleMeasurement>(*circle_measurement);
    CHECK(std::abs(circle_result.radius - 2.0) < 1e-12);
    CHECK(std::abs(circle_result.diameter - 4.0) < 1e-12);
    CHECK(
        std::abs(
            circle_result.circumference -
            4.0 * std::numbers::pi_v<double>) <
        1e-12);
    CHECK(
        std::abs(
            circle_result.area -
            4.0 * std::numbers::pi_v<double>) <
        1e-12);

    const auto arc_ccw_measurement =
        measureEntity(measurement_model, measured_arc_ccw);
    CHECK(arc_ccw_measurement.has_value());
    CHECK(std::holds_alternative<ArcMeasurement>(
        *arc_ccw_measurement));
    const auto& arc_ccw_result =
        std::get<ArcMeasurement>(*arc_ccw_measurement);
    CHECK(std::abs(arc_ccw_result.radius - 4.0) < 1e-12);
    CHECK(std::abs(arc_ccw_result.start_angle - 0.25) < 1e-12);
    CHECK(
        std::abs(
            arc_ccw_result.end_angle -
            (0.25 + std::numbers::pi_v<double> * 0.5)) <
        1e-12);
    CHECK(
        std::abs(
            arc_ccw_result.signed_sweep_angle -
            std::numbers::pi_v<double> * 0.5) <
        1e-12);
    CHECK(
        std::abs(
            arc_ccw_result.arc_length -
            2.0 * std::numbers::pi_v<double>) <
        1e-12);

    const auto arc_cw_measurement =
        measureEntity(measurement_model, measured_arc_cw);
    CHECK(arc_cw_measurement.has_value());
    const auto& arc_cw_result =
        std::get<ArcMeasurement>(*arc_cw_measurement);
    CHECK(arc_cw_result.signed_sweep_angle < 0.0);
    CHECK(arc_cw_result.arc_length > 0.0);

    CHECK(!measureEntity(
        measurement_model,
        EntityId{}).has_value());

    SketchModel overflow_measurement_model;
    const auto huge_circle =
        overflow_measurement_model.addCircle(
            Point2{0.0, 0.0},
            1e308);
    const auto huge_arc =
        overflow_measurement_model.addArc(
            Point2{0.0, 0.0},
            1e308,
            0.0,
            3.0);
    CHECK(
        !measureEntity(
            overflow_measurement_model,
            huge_circle).has_value());
    CHECK(
        !measureEntity(
            overflow_measurement_model,
            huge_arc).has_value());

    return 0;
}
