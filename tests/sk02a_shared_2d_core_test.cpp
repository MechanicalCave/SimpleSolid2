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

    return 0;
}
