#include <simplesolid2/sketch/structural_edit.hpp>

#include <cmath>
#include <iostream>
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
