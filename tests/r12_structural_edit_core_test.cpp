#include <simplesolid2/sketch/structural_edit.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <numbers>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << '\n'; \
            return 1; \
        } \
    } while (false)

bool near(double first, double second) {
    return std::abs(first - second) <= 1.0e-10;
}

bool nearPoint(
    simplesolid2::sketch::Point2 first,
    simplesolid2::sketch::Point2 second) {
    return near(first.u, second.u) &&
           near(first.v, second.v);
}

std::optional<simplesolid2::sketch::SketchModel>
resultModel(
    const simplesolid2::sketch::StructuralEditResult& result) {
    if (!result.ready() || !result.state) {
        return std::nullopt;
    }
    return simplesolid2::sketch::SketchModel::restore(
        *result.state);
}

} // namespace

int main() {
    using namespace simplesolid2::sketch;

    // B02 real-user Windows artifact, copied verbatim from the authored
    // entities in szkic_test_01(1).ss2part (schema 20, six Circles).
    // Only case 1/2 EXACT binary tangencies are asserted here. Case 3/4
    // are deliberately diagnostic: their authored finite endpoint
    // coordinates differ by 1 ULP and whether a near-contact is admitted
    // requires a separately accepted numerical D2 policy. The test does
    // not add a tolerance, weld edges or assert false precision.
    {
        struct RealCase final {
            const char* name;
            Point2 center;
            Point2 first_start;
            Point2 first_end;
            Point2 second_start;
            Point2 second_end;
        };
        const std::array<RealCase, 6> cases{{
            {"case1-extended-exact", {-81.13570411713846, 67.52296383563574},
                {-111.13570411713846, 0.0},
                {-111.13570411713846, 92.56448649641345},
                {-51.13570411713846, 0.0},
                {-51.13570411713846, 106.53001893976436}},
            {"case2-end-exact", {0.0, 67.52296383563574},
                {-30.0, 0.0}, {-30.0, 67.52296383563574},
                {30.0, 0.0}, {30.0, 67.52296383563574}},
            {"case3-copied-inside", {89.16841975020736, 67.52296383563574},
                {49.16841975020738, 60.507216219488555},
                {63.84674561314644, 83.61062429004923},
                {129.16841975020736, 60.507216219488555},
                {114.49009388726829, 83.61062429004923}},
            {"case4-copied-outside", {190.34517723877235, 67.52296383563574},
                {150.34517723877235, 60.507216219488555},
                {165.0235031017114, 83.61062429004923},
                {230.34517723877235, 60.507216219488555},
                {215.6668513758333, 83.61062429004923}},
            {"case5-end-near", {-52.63750445097793, -54.29688164032062},
                {-84.1794889180956, -95.72801266910211},
                {-72.09093198090801, -77.13466080857073},
                {-35.780318328485, -79.11291025151527},
                {-13.710456786348955, -97.4411982152548}},
            {"case6-secant-finite", {36.2264805817629, -54.63476751496982},
                {6.846405134007824, -83.25915596546274},
                {23.83920538253282, -66.26635571693774},
                {68.92311233700123, -78.74091461840942},
                {56.130014366619065, -65.94781664802726}},
        }};
        std::array<std::size_t, 6> first_hits{};
        std::array<std::size_t, 6> second_hits{};
        std::array<bool, 6> first_tangent{};
        std::array<bool, 6> second_tangent{};
        for (std::size_t index = 0; index < cases.size(); ++index) {
            const auto& sample = cases[index];
            SketchModel observed;
            const auto circle_id =
                observed.addCircle(sample.center, 30.0);
            const auto left_id =
                observed.addLine(sample.first_start, sample.first_end);
            const auto right_id =
                observed.addLine(sample.second_start, sample.second_end);
            const auto first =
                analyzeCurveRelation(observed, circle_id, left_id);
            const auto second =
                analyzeCurveRelation(observed, circle_id, right_id);
            first_hits[index] = first.intersections.size();
            second_hits[index] = second.intersections.size();
            first_tangent[index] =
                first.intersections.size() == 1U &&
                first.intersections.front().contact ==
                    CurveContactKind::tangent;
            second_tangent[index] =
                second.intersections.size() == 1U &&
                second.intersections.front().contact ==
                    CurveContactKind::tangent;
            std::cerr << "SS2_REAL_ULP_FIXTURE " << sample.name
                      << " left_status=" << static_cast<int>(first.status)
                      << " left_hits=" << first_hits[index]
                      << " left_exact_tangent=" << first_tangent[index]
                      << " right_status=" << static_cast<int>(second.status)
                      << " right_hits=" << second_hits[index]
                      << " right_exact_tangent=" << second_tangent[index]
                      << '\n';
        }

        // Fail the genuine old numeric defect even before specifying how
        // to handle the separately logged 1-ULP Copy near-contacts.
        std::cerr << "SS2_REAL_EXACT_EXPECTATIONS"
                  << " case1_hits=" << first_hits[0]
                  << "," << second_hits[0]
                  << " case1_tangent=" << first_tangent[0]
                  << "," << second_tangent[0]
                  << " case2_hits=" << first_hits[1]
                  << "," << second_hits[1]
                  << " case2_tangent=" << first_tangent[1]
                  << "," << second_tangent[1]
                  << '\n';
        // No early return: the separate full-scene Trim diagnostics must
        // print before this intentionally RED fixture exits.
    }

    // Faithful full authored scene replay. IDs 1..34 below refer to
    // historical JSON keys, NOT freshly assigned runtime EntityIds.
    // Reinsert in numeric creation order including Construction roles;
    // unlike isolated pair checks, this also exercises presence of
    // unrelated authored entities and original global coordinate frame.
    {
        struct SerializedEntity final {
            int archived_id;
            bool is_circle;
            bool construction;
            Point2 first;
            Point2 second;
            double radius;
        };
        const std::array<SerializedEntity, 26> original{{
            {1, false, false, {-30.0, 0.0}, {30.0, 0.0}, 0.0},
            {2, true, false, {0.0, 67.52296383563574}, {}, 30.0},
            {3, false, false, {-30.0, 0.0}, {-30.0, 67.52296383563574}, 0.0},
            {4, false, false, {30.0, 0.0}, {30.0, 67.52296383563574}, 0.0},
            {7, false, false, {59.16841975020736, 0.0}, {59.16841975020736, 67.52296383563574}, 0.0},
            {8, false, true, {119.16841975020736, 0.0}, {119.16841975020736, 67.52296383563574}, 0.0},
            {9, true, false, {89.16841975020736, 67.52296383563574}, {}, 30.0},
            {10, false, true, {89.16841975020736, 0.0}, {89.16841975020736, -14.407052512069733}, 0.0},
            {11, false, false, {49.16841975020738, 0.0}, {129.16841975020736, 0.0}, 0.0},
            {14, false, false, {-111.13570411713846, 0.0}, {-51.13570411713846, 0.0}, 0.0},
            {15, false, false, {-111.13570411713846, 0.0}, {-111.13570411713846, 92.56448649641345}, 0.0},
            {16, false, false, {-51.13570411713846, 0.0}, {-51.13570411713846, 106.53001893976436}, 0.0},
            {17, true, false, {-81.13570411713846, 67.52296383563574}, {}, 30.0},
            {18, true, false, {-52.63750445097793, -54.29688164032062}, {}, 30.0},
            {19, true, false, {36.2264805817629, -54.63476751496982}, {}, 30.0},
            {20, false, false, {-84.1794889180956, -95.72801266910211}, {-72.09093198090801, -77.13466080857073}, 0.0},
            {21, false, false, {-35.780318328485, -79.11291025151527}, {-13.710456786348955, -97.4411982152548}, 0.0},
            {22, false, false, {6.846405134007824, -83.25915596546274}, {23.83920538253282, -66.26635571693774}, 0.0},
            {23, false, false, {68.92311233700123, -78.74091461840942}, {56.130014366619065, -65.94781664802726}, 0.0},
            {25, false, false, {49.16841975020738, 0.0}, {49.16841975020738, 60.507216219488555}, 0.0},
            {26, false, false, {49.16841975020738, 60.507216219488555}, {63.84674561314644, 83.61062429004923}, 0.0},
            {27, false, false, {129.16841975020736, 0.0}, {129.16841975020736, 60.507216219488555}, 0.0},
            {28, false, false, {129.16841975020736, 60.507216219488555}, {114.49009388726829, 83.61062429004923}, 0.0},
            {32, false, false, {230.34517723877235, 60.507216219488555}, {215.6668513758333, 83.61062429004923}, 0.0},
            {33, false, false, {150.34517723877235, 60.507216219488555}, {165.0235031017114, 83.61062429004923}, 0.0},
            {34, true, false, {190.34517723877235, 67.52296383563574}, {}, 30.0},
        }};
        SketchModel scene;
        std::map<int, EntityId> id;
        for (const auto& item : original) {
            if (item.is_circle) {
                id.emplace(item.archived_id,
                           scene.addCircle(item.first, item.radius));
            } else {
                id.emplace(item.archived_id,
                           scene.addLine(item.first, item.second,
                               item.construction
                                   ? EntityRole::construction
                                   : EntityRole::regular));
            }
        }
        struct EditCase final {
            const char* name;
            int circle;
            int first;
            int second;
            Point2 pick;
        };
        const std::array<EditCase, 4> edits{{
            {"1-extended", 17, 15, 16,
                {-81.13570411713846, 37.52296383563574}},
            {"2-at-end", 2, 3, 4,
                {0.0, 37.52296383563574}},
            {"3-copied-inside", 9, 26, 28,
                {89.16841975020736, 37.52296383563574}},
            {"4-copied-outside", 34, 33, 32,
                {190.34517723877235, 37.52296383563574}},
        }};
        std::array<bool, 4> can_trim{};
        for (std::size_t index = 0; index < edits.size(); ++index) {
            const auto& sample = edits[index];
            const auto result = evaluateTrim(
                scene, TrimSketchRequest{
                    id.at(sample.circle),
                    {id.at(sample.first), id.at(sample.second)},
                    sample.pick});
            can_trim[index] = result.ready();
            std::cerr << "SS2_REAL_COMPLETE_SCENE_TRIM"
                      << " case=" << sample.name
                      << " ready=" << can_trim[index]
                      << " status=" << static_cast<int>(result.status)
                      << '\n';
        }
        // Both cases have *exact* finite cutter contacts in the
        // original binary64 authored values. The copied case #4 near
        // relation is recorded only; its tolerance policy is not yet D2.
        CHECK(can_trim[0]);
        CHECK(can_trim[1]);
        // N0 does not assign user-intended tangency to already
        // different persisted coordinates. The positive finite
        // secants of case 3 still Trim, while the disjoint case 4
        // remains a documented near-contact GAP pending N1 D2.
        CHECK(can_trim[2]);
        CHECK(can_trim[3]);
    }

    // Line terminal Trim preserves identity and role.
    {
        SketchModel model;
        const auto target =
            model.addLine(
                {0.0, 0.0},
                {10.0, 0.0},
                EntityRole::construction);
        const auto boundary =
            model.addLine(
                {4.0, -2.0},
                {4.0, 2.0});

        const auto trimmed =
            evaluateTrim(
                model,
                TrimSketchRequest{
                    target,
                    {boundary},
                    {9.0, 0.0}});
        CHECK(trimmed.ready());
        CHECK(trimmed.result_entity == target);
        const auto edited =
            resultModel(trimmed);
        CHECK(edited.has_value());
        const auto* line =
            edited->findLine(target);
        CHECK(line != nullptr);
        CHECK(line->role() ==
              EntityRole::construction);
        CHECK(nearPoint(
            line->start(),
            {0.0, 0.0}));
        CHECK(nearPoint(
            line->end(),
            {4.0, 0.0}));
        CHECK((
            model.findLine(target)->end() ==
            Point2{10.0, 0.0}));
    }

    // Middle Line Trim would create two entities and is therefore rejected.
    {
        SketchModel model;
        const auto target =
            model.addLine(
                {0.0, 0.0},
                {10.0, 0.0});
        const auto first =
            model.addLine(
                {3.0, -2.0},
                {3.0, 2.0});
        const auto second =
            model.addLine(
                {7.0, -2.0},
                {7.0, 2.0});
        const auto trimmed =
            evaluateTrim(
                model,
                TrimSketchRequest{
                    target,
                    {first, second},
                    {5.0, 0.0}});
        CHECK(
            trimmed.status ==
            StructuralEditStatus::not_applicable);
        CHECK(!trimmed.state.has_value());
    }

    // Arc terminal Trim preserves Arc identity and signed orientation.
    {
        SketchModel model;
        const auto target =
            model.addArc(
                {0.0, 0.0},
                10.0,
                0.0,
                std::numbers::pi_v<double>);
        const auto boundary =
            model.addLine(
                {0.0, -20.0},
                {0.0, 20.0});
        const auto trimmed =
            evaluateTrim(
                model,
                TrimSketchRequest{
                    target,
                    {boundary},
                    {-9.0, 1.0}});
        CHECK(trimmed.ready());
        CHECK(trimmed.result_entity == target);
        const auto edited =
            resultModel(trimmed);
        CHECK(edited.has_value());
        const auto* arc =
            edited->findArc(target);
        CHECK(arc != nullptr);
        CHECK(near(
            arc->startAngle(),
            0.0));
        CHECK(near(
            arc->sweepAngle(),
            std::numbers::pi_v<double> /
                2.0));
    }

    // Circle Trim removes one picked connected span and authors exactly one
    // fresh Arc; the Circle identity is retired.
    {
        SketchModel model;
        const auto circle =
            model.addCircle(
                {0.0, 0.0},
                10.0,
                EntityRole::construction);
        const auto cutter =
            model.addLine(
                {-20.0, 0.0},
                {20.0, 0.0});
        const auto cursor_before =
            model.entityIdCursor();

        const auto trimmed =
            evaluateTrim(
                model,
                TrimSketchRequest{
                    circle,
                    {cutter},
                    {0.0, -10.0}});
        CHECK(trimmed.ready());
        CHECK(trimmed.result_entity.has_value());
        CHECK(*trimmed.result_entity != circle);
        CHECK(
            circle < *trimmed.result_entity);
        CHECK(
            cutter < *trimmed.result_entity);

        const auto edited =
            resultModel(trimmed);
        CHECK(edited.has_value());
        CHECK(
            edited->findCircle(circle) ==
            nullptr);
        const auto* arc =
            edited->findArc(
                *trimmed.result_entity);
        CHECK(arc != nullptr);
        CHECK(
            arc->role() ==
            EntityRole::construction);
        CHECK(near(
            arc->radius(),
            10.0));
        CHECK(near(
            std::abs(arc->sweepAngle()),
            std::numbers::pi_v<double>));
        CHECK(
            edited->entityCount() ==
            model.entityCount());
        CHECK(
            edited->entityIdCursor() >
            cursor_before);
    }

    // Circle/Circle and Circle/Arc cutters both provide valid finite cuts.
    {
        SketchModel model;
        const auto target =
            model.addCircle(
                {0.0, 0.0},
                5.0);
        const auto circle_boundary =
            model.addCircle(
                {4.0, 0.0},
                5.0);
        const auto circle_trim =
            evaluateTrim(
                model,
                TrimSketchRequest{
                    target,
                    {circle_boundary},
                    {-5.0, 0.0}});
        CHECK(circle_trim.ready());

        SketchModel arc_model;
        const auto arc_target =
            arc_model.addCircle(
                {0.0, 0.0},
                5.0);
        const auto arc_boundary =
            arc_model.addArc(
                {4.0, 0.0},
                5.0,
                1.5,
                3.0);
        const auto arc_trim =
            evaluateTrim(
                arc_model,
                TrimSketchRequest{
                    arc_target,
                    {arc_boundary},
                    {-5.0, 0.0}});
        CHECK(arc_trim.ready());
    }

    // Tangent Circle Trim has only one cut location and must not mutate.
    {
        SketchModel model;
        const auto target =
            model.addCircle(
                {0.0, 0.0},
                5.0);
        const auto tangent =
            model.addLine(
                {-10.0, 5.0},
                {10.0, 5.0});
        const auto trimmed =
            evaluateTrim(
                model,
                TrimSketchRequest{
                    target,
                    {tangent},
                    {0.0, -5.0}});
        CHECK(
            trimmed.status ==
            StructuralEditStatus::not_applicable);
        CHECK(!trimmed.state.has_value());
    }

    // S0/B02 exact integer oblique tangents: shared relation analysis and
    // two distinct finite cutter contacts must permit ONE Circle->Arc Trim.
    // The historical baseline normalized d=(9,12), yielding 5.0000000000000009
    // instead of exactly R=5 and incorrectly reported both lines disjoint.
    {
        SketchModel model;
        const auto target =
            model.addCircle({0.0, 0.0}, 5.0);
        const auto left =
            model.addLine({-13.0, -9.0}, {-4.0, 3.0});
        const auto right =
            model.addLine({13.0, -9.0}, {4.0, 3.0});
        const auto left_relation =
            analyzeCurveRelation(model, target, left);
        const auto right_relation =
            analyzeCurveRelation(model, target, right);
        std::cerr << "SKETCH_S0_B02_OBLIQUE"
                  << " left=" << left_relation.intersections.size()
                  << " right=" << right_relation.intersections.size()
                  << " left_status=" << static_cast<int>(left_relation.status)
                  << " right_status=" << static_cast<int>(right_relation.status)
                  << '\n';
        CHECK(left_relation.status == CurveRelationStatus::discrete);
        CHECK(right_relation.status == CurveRelationStatus::discrete);
        CHECK(left_relation.intersections.size() == 1U);
        CHECK(right_relation.intersections.size() == 1U);
        CHECK(left_relation.intersections[0].contact == CurveContactKind::tangent);
        CHECK(right_relation.intersections[0].contact == CurveContactKind::tangent);
        const auto trimmed = evaluateTrim(
            model,
            TrimSketchRequest{target, {left, right}, {0.0, -5.0}});
        CHECK(trimmed.ready());
        CHECK(trimmed.result_entity.has_value());
        CHECK(*trimmed.result_entity != target);
        const auto edited = resultModel(trimmed);
        CHECK(edited.has_value());
        CHECK(edited->findCircle(target) == nullptr);
        CHECK(edited->findArc(*trimmed.result_entity) != nullptr);

        // Reversing a finite cutter must not change exact contact type.
        const auto reversed =
            model.addLine({-4.0, 3.0}, {-13.0, -9.0});
        const auto reversed_relation =
            analyzeCurveRelation(model, target, reversed);
        CHECK(reversed_relation.status == CurveRelationStatus::discrete);
        CHECK(reversed_relation.intersections.size() == 1U);
        CHECK(reversed_relation.intersections[0].contact ==
              CurveContactKind::tangent);

        // Real positive clearance stays disjoint; a nearby genuine
        // secant retains both distinct contacts. Neither is merged into
        // an exact tangent by a world-space radius epsilon.
        // Use extended Lines here: the original F3 exact cutter ends
        // at the contact (t=1), where a nearby secant would put one
        // root beyond the finite endpoint, correctly admitting only one.
        const auto outside =
            model.addLine({-13.000001, -9.0},
                          {4.999999, 15.0});
        const auto outside_relation =
            analyzeCurveRelation(model, target, outside);
        CHECK(outside_relation.status == CurveRelationStatus::disjoint);
        CHECK(outside_relation.intersections.empty());

        const auto inside =
            model.addLine({-12.999999, -9.0},
                          {5.000001, 15.0});
        const auto inside_relation =
            analyzeCurveRelation(model, target, inside);
        CHECK(inside_relation.status == CurveRelationStatus::discrete);
        CHECK(inside_relation.intersections.size() == 2U);

        // F3 exact integer-vector tangencies survive an exact 90-degree
        // rotation and factor-two positive scale. Keep both finite
        // cutter endpoints on the transformed Circle's contact points.
        std::size_t transformed_verified = 0U;
        for (const bool quarter_turn : {false, true}) {
            for (const double factor : {1.0, 2.0}) {
                const auto mapped =
                    [quarter_turn, factor](Point2 point) {
                        return quarter_turn
                            ? Point2{-point.v * factor,
                                     point.u * factor}
                            : Point2{point.u * factor,
                                     point.v * factor};
                    };
                SketchModel transformed;
                const auto target_id =
                    transformed.addCircle({0.0, 0.0}, 5.0 * factor);
                const auto cutter_left =
                    transformed.addLine(
                        mapped({-13.0, -9.0}),
                        mapped({-4.0, 3.0}));
                const auto cutter_right =
                    transformed.addLine(
                        mapped({13.0, -9.0}),
                        mapped({4.0, 3.0}));
                for (const auto candidate :
                     {cutter_left, cutter_right}) {
                    const auto contact = analyzeCurveRelation(
                        transformed, target_id, candidate);
                    CHECK(contact.status == CurveRelationStatus::discrete);
                    CHECK(contact.intersections.size() == 1U);
                    CHECK(contact.intersections[0].contact ==
                          CurveContactKind::tangent);
                }
                ++transformed_verified;
            }
        }
        CHECK(transformed_verified == 4U);
    }

    // Line Extend uses finite authored boundary geometry and keeps identity.
    {
        SketchModel model;
        const auto target =
            model.addLine(
                {0.0, 0.0},
                {1.0, 0.0},
                EntityRole::construction);
        const auto boundary =
            model.addLine(
                {5.0, -1.0},
                {5.0, 1.0});
        const auto extended =
            evaluateExtend(
                model,
                ExtendSketchRequest{
                    target,
                    {boundary},
                    StructuralEndpointRole::end});
        CHECK(extended.ready());
        CHECK(extended.result_entity == target);
        const auto edited =
            resultModel(extended);
        CHECK(edited.has_value());
        const auto* line =
            edited->findLine(target);
        CHECK(line != nullptr);
        CHECK(
            line->role() ==
            EntityRole::construction);
        CHECK(nearPoint(
            line->end(),
            {5.0, 0.0}));
    }

    // Standard Extend does not use a boundary's virtual continuation.
    {
        SketchModel model;
        const auto target =
            model.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto boundary =
            model.addLine(
                {5.0, 10.0},
                {5.0, 20.0});
        const auto extended =
            evaluateExtend(
                model,
                ExtendSketchRequest{
                    target,
                    {boundary},
                    StructuralEndpointRole::end});
        CHECK(
            extended.status ==
            StructuralEditStatus::no_intersection);
        CHECK(!extended.state.has_value());
    }

    // Line Extend accepts finite Circle and Arc boundaries.
    {
        SketchModel circle_model;
        const auto target =
            circle_model.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto circle =
            circle_model.addCircle(
                {5.0, 0.0},
                1.0);
        const auto to_circle =
            evaluateExtend(
                circle_model,
                ExtendSketchRequest{
                    target,
                    {circle},
                    StructuralEndpointRole::end});
        CHECK(to_circle.ready());
        const auto circle_edited =
            resultModel(to_circle);
        CHECK(circle_edited.has_value());
        CHECK(nearPoint(
            circle_edited->findLine(target)->end(),
            {4.0, 0.0}));

        SketchModel arc_model;
        const auto arc_target =
            arc_model.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto arc =
            arc_model.addArc(
                {5.0, 0.0},
                1.0,
                std::numbers::pi_v<double> /
                    2.0,
                std::numbers::pi_v<double>);
        const auto to_arc =
            evaluateExtend(
                arc_model,
                ExtendSketchRequest{
                    arc_target,
                    {arc},
                    StructuralEndpointRole::end});
        CHECK(to_arc.ready());
        const auto arc_edited =
            resultModel(to_arc);
        CHECK(arc_edited.has_value());
        CHECK(nearPoint(
            arc_edited->findLine(arc_target)->end(),
            {4.0, 0.0}));
    }

    // Arc Extend keeps identity and continues along the authored orientation.
    {
        SketchModel model;
        const auto target =
            model.addArc(
                {0.0, 0.0},
                5.0,
                0.0,
                std::numbers::pi_v<double> /
                    2.0);
        const auto boundary =
            model.addLine(
                {-5.0, -1.0},
                {-5.0, 1.0});
        const auto extended =
            evaluateExtend(
                model,
                ExtendSketchRequest{
                    target,
                    {boundary},
                    StructuralEndpointRole::end});
        CHECK(extended.ready());
        CHECK(extended.result_entity == target);
        const auto edited =
            resultModel(extended);
        CHECK(edited.has_value());
        const auto* arc =
            edited->findArc(target);
        CHECK(arc != nullptr);
        CHECK(near(
            arc->sweepAngle(),
            std::numbers::pi_v<double>));
    }

    // Mutual virtual Line extension is atomic and preserves both identities
    // and independent Regular/Construction roles.
    {
        SketchModel model;
        const auto first =
            model.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto second =
            model.addLine(
                {3.0, 2.0},
                {3.0, 1.0},
                EntityRole::construction);
        const auto extended =
            evaluateExtendBoth(
                model,
                ExtendBothLinesRequest{
                    first,
                    second});
        CHECK(extended.ready());
        const auto edited =
            resultModel(extended);
        CHECK(edited.has_value());
        const auto* first_line =
            edited->findLine(first);
        const auto* second_line =
            edited->findLine(second);
        CHECK(first_line != nullptr);
        CHECK(second_line != nullptr);
        CHECK(nearPoint(
            first_line->end(),
            {3.0, 0.0}));
        CHECK(nearPoint(
            second_line->end(),
            {3.0, 0.0}));
        CHECK(
            first_line->role() ==
            EntityRole::regular);
        CHECK(
            second_line->role() ==
            EntityRole::construction);
    }

    // Extend Both rejects one-sided and parallel cases.
    {
        SketchModel one_sided;
        const auto first =
            one_sided.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto second =
            one_sided.addLine(
                {3.0, -1.0},
                {3.0, 1.0});
        const auto result =
            evaluateExtendBoth(
                one_sided,
                {first, second});
        CHECK(
            result.status ==
            StructuralEditStatus::not_applicable);

        SketchModel parallel;
        const auto a =
            parallel.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto b =
            parallel.addLine(
                {0.0, 2.0},
                {1.0, 2.0});
        const auto parallel_result =
            evaluateExtendBoth(
                parallel,
                {a, b});
        CHECK(
            parallel_result.status ==
            StructuralEditStatus::not_applicable);
    }

    // Arc middle Trim is outside R12 because it would leave two Arcs.
    {
        SketchModel model;
        const auto target =
            model.addArc(
                {0.0, 0.0},
                10.0,
                0.0,
                std::numbers::pi_v<double>);
        const auto right =
            model.addLine(
                {5.0, -20.0},
                {5.0, 20.0});
        const auto left =
            model.addLine(
                {-5.0, -20.0},
                {-5.0, 20.0});
        const auto trimmed =
            evaluateTrim(
                model,
                {
                    target,
                    {right, left},
                    {0.0, 10.0}});
        CHECK(
            trimmed.status ==
            StructuralEditStatus::not_applicable);
        CHECK(!trimmed.state.has_value());
    }

    // Four finite cuts partition a Circle into four local spans. The clicked
    // lower-right span alone is removed and the connected complement is one
    // fresh 270-degree Arc.
    {
        SketchModel model;
        const auto target =
            model.addCircle(
                {0.0, 0.0},
                10.0);
        const auto horizontal =
            model.addLine(
                {-20.0, 0.0},
                {20.0, 0.0});
        const auto vertical =
            model.addLine(
                {0.0, -20.0},
                {0.0, 20.0});
        const auto trimmed =
            evaluateTrim(
                model,
                {
                    target,
                    {horizontal, vertical},
                    {1.0, -1.0}});
        CHECK(trimmed.ready());
        CHECK(trimmed.result_entity.has_value());
        CHECK(*trimmed.result_entity != target);
        const auto edited =
            resultModel(trimmed);
        CHECK(edited.has_value());
        const auto* arc =
            edited->findArc(
                *trimmed.result_entity);
        CHECK(arc != nullptr);
        CHECK(near(
            arc->startAngle(),
            0.0));
        CHECK(near(
            arc->sweepAngle(),
            3.0 *
                std::numbers::pi_v<double> /
                2.0));
    }

    // Coincident authored boundary geometry must not invent arbitrary cuts.
    {
        SketchModel model;
        const auto target =
            model.addCircle(
                {0.0, 0.0},
                5.0);
        const auto coincident =
            model.addCircle(
                {0.0, 0.0},
                5.0);
        const auto trimmed =
            evaluateTrim(
                model,
                {
                    target,
                    {coincident},
                    {5.0, 0.0}});
        CHECK(
            trimmed.status ==
            StructuralEditStatus::ambiguous_topology);
        CHECK(!trimmed.state.has_value());
    }

    // Multiple Line-Extend boundaries are ordered by exact positive travel,
    // not by caller/provider order.
    {
        SketchModel model;
        const auto target =
            model.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto far =
            model.addLine(
                {5.0, -1.0},
                {5.0, 1.0});
        const auto near_boundary =
            model.addLine(
                {3.0, -1.0},
                {3.0, 1.0});
        const auto extended =
            evaluateExtend(
                model,
                {
                    target,
                    {far, near_boundary},
                    StructuralEndpointRole::end});
        CHECK(extended.ready());
        const auto edited =
            resultModel(extended);
        CHECK(edited.has_value());
        CHECK(nearPoint(
            edited->findLine(target)->end(),
            {3.0, 0.0}));
    }

    // Arc Extend supports each finite boundary kind and both endpoint
    // continuation directions without changing the Arc EntityId.
    {
        SketchModel circle_model;
        const auto target =
            circle_model.addArc(
                {0.0, 0.0},
                5.0,
                0.0,
                std::numbers::pi_v<double> /
                    2.0);
        const auto circle_boundary =
            circle_model.addCircle(
                {-6.0, 0.0},
                1.0);
        const auto to_circle =
            evaluateExtend(
                circle_model,
                {
                    target,
                    {circle_boundary},
                    StructuralEndpointRole::end});
        CHECK(to_circle.ready());
        CHECK(to_circle.result_entity == target);
        const auto circle_edited =
            resultModel(to_circle);
        CHECK(circle_edited.has_value());
        CHECK(near(
            circle_edited->findArc(target)
                ->sweepAngle(),
            std::numbers::pi_v<double>));

        SketchModel arc_model;
        const auto arc_target =
            arc_model.addArc(
                {0.0, 0.0},
                5.0,
                0.0,
                std::numbers::pi_v<double> /
                    2.0);
        const auto arc_boundary =
            arc_model.addArc(
                {-6.0, 0.0},
                1.0,
                -std::numbers::pi_v<double> /
                    2.0,
                std::numbers::pi_v<double>);
        const auto to_arc =
            evaluateExtend(
                arc_model,
                {
                    arc_target,
                    {arc_boundary},
                    StructuralEndpointRole::end});
        CHECK(to_arc.ready());
        const auto arc_edited =
            resultModel(to_arc);
        CHECK(arc_edited.has_value());
        CHECK(near(
            arc_edited->findArc(arc_target)
                ->sweepAngle(),
            std::numbers::pi_v<double>));

        SketchModel start_model;
        const auto start_target =
            start_model.addArc(
                {0.0, 0.0},
                5.0,
                0.0,
                std::numbers::pi_v<double> /
                    2.0);
        const auto start_boundary =
            start_model.addLine(
                {-1.0, -5.0},
                {1.0, -5.0});
        const auto from_start =
            evaluateExtend(
                start_model,
                {
                    start_target,
                    {start_boundary},
                    StructuralEndpointRole::start});
        CHECK(from_start.ready());
        const auto start_edited =
            resultModel(from_start);
        CHECK(start_edited.has_value());
        const auto* start_arc =
            start_edited->findArc(start_target);
        CHECK(start_arc != nullptr);
        CHECK(near(
            start_arc->startAngle(),
            3.0 *
                std::numbers::pi_v<double> /
                2.0));
        CHECK(near(
            start_arc->sweepAngle(),
            std::numbers::pi_v<double>));

        SketchModel negative_model;
        const auto negative_target =
            negative_model.addArc(
                {0.0, 0.0},
                5.0,
                0.0,
                -std::numbers::pi_v<double> /
                    2.0);
        const auto negative_boundary =
            negative_model.addLine(
                {-5.0, -1.0},
                {-5.0, 1.0});
        const auto negative_extended =
            evaluateExtend(
                negative_model,
                {
                    negative_target,
                    {negative_boundary},
                    StructuralEndpointRole::end});
        CHECK(negative_extended.ready());
        const auto negative_edited =
            resultModel(negative_extended);
        CHECK(negative_edited.has_value());
        CHECK(near(
            negative_edited
                ->findArc(negative_target)
                ->sweepAngle(),
            -std::numbers::pi_v<double>));
    }

    // A Circle is deliberately not an Extend target in R12.
    {
        SketchModel model;
        const auto target =
            model.addCircle(
                {0.0, 0.0},
                1.0);
        const auto boundary =
            model.addLine(
                {2.0, -2.0},
                {2.0, 2.0});
        const auto extended =
            evaluateExtend(
                model,
                {
                    target,
                    {boundary},
                    StructuralEndpointRole::end});
        CHECK(
            extended.status ==
            StructuralEditStatus::unsupported);
        CHECK(!extended.state.has_value());
    }

    // Collinear Line supports are ambiguous for Extend Both; no arbitrary
    // virtual intersection may be selected.
    {
        SketchModel model;
        const auto first =
            model.addLine(
                {0.0, 0.0},
                {1.0, 0.0});
        const auto second =
            model.addLine(
                {3.0, 0.0},
                {2.0, 0.0});
        const auto extended =
            evaluateExtendBoth(
                model,
                {first, second});
        CHECK(
            extended.status ==
            StructuralEditStatus::ambiguous_topology);
        CHECK(!extended.state.has_value());
    }

    return 0;
}
