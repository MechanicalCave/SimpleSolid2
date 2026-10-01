#include <simplesolid2/sketch/snap.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>

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
    return std::abs(first - second) <= 1.0e-12;
}

bool nearPoint(
    simplesolid2::sketch::Point2 first,
    simplesolid2::sketch::Point2 second) {
    return near(first.u, second.u) &&
           near(first.v, second.v);
}

const simplesolid2::sketch::SnapCandidate*
findCandidate(
    const std::vector<
        simplesolid2::sketch::SnapCandidate>& candidates,
    simplesolid2::sketch::SnapKind kind,
    simplesolid2::sketch::SnapSemanticRole role,
    simplesolid2::sketch::EntityId entity = {}) {
    for (const auto& candidate : candidates) {
        if (candidate.kind != kind ||
            candidate.source.role != role) {
            continue;
        }
        if (entity.valid()) {
            if (!candidate.source.first_entity ||
                *candidate.source.first_entity != entity) {
                continue;
            }
        }
        return &candidate;
    }
    return nullptr;
}

} // namespace

int main() {
    using namespace simplesolid2::sketch;

    const SnapModeSet defaults;
    CHECK(defaults.endpoint);
    CHECK(defaults.midpoint);
    CHECK(defaults.center);
    CHECK(defaults.quadrant);
    CHECK(defaults.intersection);
    CHECK(defaults.origin);
    CHECK(!defaults.perpendicular);
    CHECK(!defaults.tangent);
    CHECK(!defaults.nearest);
    CHECK(!defaults.extension);

    const ObjectSnapPreferences preference_defaults;
    CHECK(preference_defaults.master_enabled);
    CHECK(!preference_defaults.object_tracking_enabled);

    const auto default_eligibility =
        resolveSnapEligibility(
            preference_defaults);
    CHECK(default_eligibility.endpoint);
    CHECK(default_eligibility.midpoint);
    CHECK(default_eligibility.center);
    CHECK(default_eligibility.quadrant);
    CHECK(default_eligibility.intersection);
    CHECK(default_eligibility.origin);
    CHECK(!default_eligibility.perpendicular);
    CHECK(!default_eligibility.tangent);
    CHECK(!default_eligibility.nearest);
    CHECK(!default_eligibility.extension);
    CHECK(!default_eligibility.object_tracking);
    CHECK(!default_eligibility.suppress_object_assistance);

    ObjectSnapPreferences disabled_preferences =
        preference_defaults;
    disabled_preferences.master_enabled = false;
    disabled_preferences.object_tracking_enabled = true;
    const auto disabled_eligibility =
        resolveSnapEligibility(
            disabled_preferences);
    CHECK(!disabled_eligibility.endpoint);
    CHECK(!disabled_eligibility.origin);
    CHECK(disabled_eligibility.object_tracking);

    const auto endpoint_override =
        resolveSnapEligibility(
            disabled_preferences,
            TemporarySnapOverrideKind::endpoint);
    CHECK(endpoint_override.endpoint);
    CHECK(!endpoint_override.midpoint);
    CHECK(!endpoint_override.object_tracking);
    CHECK(
        endpoint_override.enabled(
            SnapKind::endpoint));
    CHECK(
        !endpoint_override.enabled(
            SnapKind::nearest));

    const auto extension_override =
        resolveSnapEligibility(
            preference_defaults,
            TemporarySnapOverrideKind::extension);
    CHECK(extension_override.extension);
    CHECK(!extension_override.endpoint);
    CHECK(!extension_override.object_tracking);

    const auto none_override =
        resolveSnapEligibility(
            ObjectSnapPreferences{
                true,
                SnapModeSet{},
                true},
            TemporarySnapOverrideKind::none);
    CHECK(none_override.suppress_object_assistance);
    CHECK(!none_override.endpoint);
    CHECK(!none_override.midpoint);
    CHECK(!none_override.center);
    CHECK(!none_override.quadrant);
    CHECK(!none_override.intersection);
    CHECK(!none_override.origin);
    CHECK(!none_override.perpendicular);
    CHECK(!none_override.tangent);
    CHECK(!none_override.nearest);
    CHECK(!none_override.extension);
    CHECK(!none_override.object_tracking);

    SketchModel model;
    const auto horizontal =
        model.addLine(
            {0.0, 0.0},
            {10.0, 0.0});
    const auto vertical =
        model.addLine(
            {10.0, -5.0},
            {10.0, 5.0},
            EntityRole::construction);
    const auto circle =
        model.addCircle(
            {20.0, 0.0},
            5.0);
    const auto arc =
        model.addArc(
            {0.0, 20.0},
            5.0,
            0.0,
            std::numbers::pi_v<double> / 2.0,
            EntityRole::construction);
    const auto unrelated_center_endpoint =
        model.addLine(
            {20.0, 0.0},
            {20.0, -10.0});

    const auto static_candidates =
        staticSnapCandidates(
            model,
            defaults);

    const auto* line_start =
        findCandidate(
            static_candidates,
            SnapKind::endpoint,
            SnapSemanticRole::line_start,
            horizontal);
    CHECK(line_start != nullptr);
    CHECK((line_start->point == Point2{0.0, 0.0}));

    const auto* line_mid =
        findCandidate(
            static_candidates,
            SnapKind::midpoint,
            SnapSemanticRole::line_midpoint,
            horizontal);
    CHECK(line_mid != nullptr);
    CHECK((line_mid->point == Point2{5.0, 0.0}));

    const auto* circle_center =
        findCandidate(
            static_candidates,
            SnapKind::center,
            SnapSemanticRole::circle_center,
            circle);
    CHECK(circle_center != nullptr);
    CHECK((circle_center->point == Point2{20.0, 0.0}));

    const auto* circle_quad =
        findCandidate(
            static_candidates,
            SnapKind::quadrant,
            SnapSemanticRole::circle_quadrant_pos_v,
            circle);
    CHECK(circle_quad != nullptr);
    CHECK((circle_quad->point == Point2{20.0, 5.0}));

    const auto* arc_quad_u =
        findCandidate(
            static_candidates,
            SnapKind::quadrant,
            SnapSemanticRole::arc_quadrant_pos_u,
            arc);
    const auto* arc_quad_v =
        findCandidate(
            static_candidates,
            SnapKind::quadrant,
            SnapSemanticRole::arc_quadrant_pos_v,
            arc);
    CHECK(arc_quad_u != nullptr);
    CHECK(arc_quad_v != nullptr);
    CHECK(nearPoint(
        arc_quad_u->point,
        {5.0, 20.0}));
    CHECK(nearPoint(
        arc_quad_v->point,
        {0.0, 25.0}));
    CHECK(
        findCandidate(
            static_candidates,
            SnapKind::quadrant,
            SnapSemanticRole::arc_quadrant_neg_u,
            arc) == nullptr);

    const auto* origin =
        findCandidate(
            static_candidates,
            SnapKind::origin,
            SnapSemanticRole::intrinsic_origin);
    CHECK(origin != nullptr);
    CHECK((origin->point == Point2{0.0, 0.0}));
    CHECK(
        origin->source.kind ==
        SnapSourceKind::intrinsic_origin);
    CHECK(!origin->source.first_entity);
    CHECK(!origin->source.second_entity);

    SnapModeSet endpoint_only{};
    endpoint_only.midpoint = false;
    endpoint_only.center = false;
    endpoint_only.quadrant = false;
    endpoint_only.intersection = false;
    endpoint_only.origin = false;
    const auto endpoints =
        staticSnapCandidates(
            model,
            endpoint_only);
    for (const auto& candidate : endpoints) {
        CHECK(candidate.kind == SnapKind::endpoint);
    }

    const auto crossings =
        intersectionSnapCandidates(
            model,
            vertical,
            horizontal);
    CHECK(crossings.size() == 1U);
    CHECK(
        crossings.front().kind ==
        SnapKind::intersection);
    CHECK((
        crossings.front().point ==
        Point2{10.0, 0.0}));
    CHECK(
        crossings.front().source.kind ==
        SnapSourceKind::intersection);
    CHECK(
        crossings.front().source.first_entity ==
        horizontal);
    CHECK(
        crossings.front().source.second_entity ==
        vertical);

    const auto crossings_reversed =
        intersectionSnapCandidates(
            model,
            horizontal,
            vertical);
    CHECK(
        crossings_reversed ==
        crossings);
    CHECK(
        snapStableKey(crossings.front()) ==
        snapStableKey(
            crossings_reversed.front()));

    const auto overlap =
        model.addLine(
            {2.0, 0.0},
            {8.0, 0.0});
    CHECK(
        intersectionSnapCandidates(
            model,
            horizontal,
            overlap)
            .empty());

    const auto nearest_line =
        nearestSnapCandidate(
            model,
            horizontal,
            {4.0, 3.0});
    CHECK(nearest_line.has_value());
    CHECK(nearest_line->kind == SnapKind::nearest);
    CHECK(
        nearPoint(
            nearest_line->point,
            {4.0, 0.0}));

    const auto nearest_line_end =
        nearestSnapCandidate(
            model,
            horizontal,
            {20.0, 3.0});
    CHECK(nearest_line_end.has_value());
    CHECK((
        nearest_line_end->point ==
        Point2{10.0, 0.0}));

    const auto nearest_circle =
        nearestSnapCandidate(
            model,
            circle,
            {22.0, 0.0});
    CHECK(nearest_circle.has_value());
    CHECK((
        nearest_circle->point ==
        Point2{25.0, 0.0}));
    CHECK(
        !nearestSnapCandidate(
            model,
            circle,
            {20.0, 0.0})
             .has_value());

    const auto nearest_arc =
        nearestSnapCandidate(
            model,
            arc,
            {-10.0, 20.0});
    CHECK(nearest_arc.has_value());
    CHECK(
        nearPoint(
            nearest_arc->point,
            {0.0, 25.0}));

    const auto perpendicular_line =
        perpendicularSnapCandidates(
            model,
            horizontal,
            {4.0, 5.0});
    CHECK(perpendicular_line.size() == 1U);
    CHECK(
        nearPoint(
            perpendicular_line.front().point,
            {4.0, 0.0}));
    CHECK(
        perpendicularSnapCandidates(
            model,
            horizontal,
            {-1.0, 5.0})
            .empty());

    const auto perpendicular_circle =
        perpendicularSnapCandidates(
            model,
            circle,
            {30.0, 0.0});
    CHECK(perpendicular_circle.size() == 2U);
    CHECK((
        perpendicular_circle[0].point ==
        Point2{25.0, 0.0}));
    CHECK((
        perpendicular_circle[1].point ==
        Point2{15.0, 0.0}));
    CHECK(
        perpendicularSnapCandidates(
            model,
            circle,
            {20.0, 0.0})
            .empty());

    const auto perpendicular_arc =
        perpendicularSnapCandidates(
            model,
            arc,
            {10.0, 20.0});
    CHECK(perpendicular_arc.size() == 1U);
    CHECK(
        nearPoint(
            perpendicular_arc.front().point,
            {5.0, 20.0}));

    const auto tangents =
        tangentSnapCandidates(
            model,
            circle,
            {30.0, 0.0});
    CHECK(tangents.size() == 2U);
    CHECK(near(tangents[0].point.u, 22.5));
    CHECK(near(tangents[1].point.u, 22.5));
    CHECK(near(
        std::abs(tangents[0].point.v),
        5.0 * std::sqrt(3.0) / 2.0));
    CHECK(near(
        tangents[0].point.v,
        -tangents[1].point.v));
    CHECK(
        tangentSnapCandidates(
            model,
            circle,
            {22.0, 0.0})
            .empty());

    const auto on_circle_tangent =
        tangentSnapCandidates(
            model,
            circle,
            {25.0, 0.0});
    CHECK(on_circle_tangent.size() == 1U);
    CHECK((
        on_circle_tangent.front().point ==
        Point2{25.0, 0.0}));

    const auto arc_tangents =
        tangentSnapCandidates(
            model,
            arc,
            {10.0, 20.0});
    CHECK(arc_tangents.size() == 1U);
    CHECK(
        arc_tangents.front().point.u >= 0.0);
    CHECK(
        arc_tangents.front().point.v >= 20.0);

    CHECK(
        tangentSnapCandidates(
            model,
            horizontal,
            {0.0, 10.0})
            .empty());

    // Screen-space resolver: specific snaps hard-tier above Nearest,
    // exact same-coordinate provenance collapses without epsilon, and
    // capture/release hysteresis only retains the same stable candidate.
    {
        SnapCaptureState capture;
        const SnapCandidate endpoint_candidate{
            {10.0, 0.0},
            SnapKind::endpoint,
            {
                SnapSourceKind::entity_point,
                horizontal,
                std::nullopt,
                SnapSemanticRole::line_end,
                0U}};
        const auto exact_intersections =
            intersectionSnapCandidates(
                model,
                horizontal,
                vertical);
        CHECK(exact_intersections.size() == 1U);
        const SnapCandidate intersection_candidate =
            exact_intersections.front();
        CHECK(sameSnapContact(
            model,
            endpoint_candidate,
            intersection_candidate));

        const SnapCandidate unrelated_center_candidate{
            {20.0, 0.0},
            SnapKind::center,
            {
                SnapSourceKind::entity_point,
                circle,
                std::nullopt,
                SnapSemanticRole::circle_center,
                0U}};
        const SnapCandidate unrelated_endpoint_candidate{
            {20.0, 0.0},
            SnapKind::endpoint,
            {
                SnapSourceKind::entity_point,
                unrelated_center_endpoint,
                std::nullopt,
                SnapSemanticRole::line_start,
                0U}};
        CHECK(
            unrelated_center_candidate.point ==
            unrelated_endpoint_candidate.point);
        CHECK(!sameSnapContact(
            model,
            unrelated_center_candidate,
            unrelated_endpoint_candidate));
        const SnapCandidate nearest_candidate{
            {3.0, 4.0},
            SnapKind::nearest,
            {
                SnapSourceKind::entity_curve,
                horizontal,
                std::nullopt,
                SnapSemanticRole::curve_nearest,
                0U}};

        auto resolved = resolveScreenSnap(
            model,
            capture,
            {
                {nearest_candidate, 1.0},
                {endpoint_candidate, 8.0},
                {intersection_candidate, 8.0},
            });
        CHECK(resolved.has_value());
        CHECK(
            resolved->primary.kind ==
            SnapKind::endpoint);
        CHECK(
            resolved->primary.point ==
            endpoint_candidate.point);
        CHECK(
            resolved->coincident_candidates.size() ==
            2U);
        CHECK(capture.captured.has_value());

        // Hysteresis retains the captured specific candidate inside
        // release distance even if another specific snap becomes closer.
        const SnapCandidate center_candidate{
            {9.0, 9.0},
            SnapKind::center,
            {
                SnapSourceKind::entity_point,
                circle,
                std::nullopt,
                SnapSemanticRole::circle_center,
                0U}};
        resolved = resolveScreenSnap(
            model,
            capture,
            {
                {endpoint_candidate, 12.0},
                {center_candidate, 1.0},
            });
        CHECK(resolved.has_value());
        CHECK(
            resolved->primary.kind ==
            SnapKind::endpoint);

        // Once beyond release, the closer eligible specific candidate wins.
        resolved = resolveScreenSnap(
            model,
            capture,
            {
                {endpoint_candidate, 16.0},
                {center_candidate, 1.0},
            });
        CHECK(resolved.has_value());
        CHECK(
            resolved->primary.kind ==
            SnapKind::center);

        // A captured Nearest cannot block a newly eligible specific snap.
        capture.captured =
            snapStableKey(nearest_candidate);
        resolved = resolveScreenSnap(
            model,
            capture,
            {
                {nearest_candidate, 2.0},
                {center_candidate, 8.0},
            });
        CHECK(resolved.has_value());
        CHECK(
            resolved->primary.kind ==
            SnapKind::center);

        // Exact collapse is exact: a merely nearby coordinate remains separate.
        const SnapCandidate nearby_candidate{
            {10.0 + 1.0e-12, 0.0},
            SnapKind::midpoint,
            {
                SnapSourceKind::entity_point,
                vertical,
                std::nullopt,
                SnapSemanticRole::line_midpoint,
                0U}};
        capture.clear();
        resolved = resolveScreenSnap(
            model,
            capture,
            {
                {endpoint_candidate, 5.0},
                {intersection_candidate, 5.0},
                {nearby_candidate, 5.0},
            });
        CHECK(resolved.has_value());
        CHECK(
            resolved->coincident_candidates.size() ==
            2U);

        capture.clear();
        resolved = resolveScreenSnap(
            model,
            capture,
            {
                {unrelated_center_candidate, 2.0},
                {unrelated_endpoint_candidate, 2.0},
            });
        CHECK(resolved.has_value());
        CHECK(
            resolved->coincident_candidates.size() ==
            1U);

        // Release and invalid-policy behavior are fail-closed.
        CHECK(
            !resolveScreenSnap(
                 model,
                 capture,
                 {{endpoint_candidate, 16.0}})
                 .has_value());
        CHECK(!capture.captured.has_value());
        CHECK(
            !resolveScreenSnap(
                 model,
                 capture,
                 {{endpoint_candidate, 1.0}},
                 {15.0, 9.0})
                 .has_value());
    }

    // Temporary override belongs to the active PointRequest. Pointer
    // preview does not consume it; accepted point, Esc and request/tool
    // replacement do.
    {
        SketchInteractionState interaction;
        CHECK(
            !interaction.setTemporarySnapOverride(
                TemporarySnapOverrideKind::endpoint));

        interaction.activateLine();
        CHECK(
            interaction.setTemporarySnapOverride(
                TemporarySnapOverrideKind::endpoint));
        auto point_request =
            interaction.activePointRequest();
        CHECK(point_request.has_value());
        CHECK(
            point_request->temporary_snap_override ==
            TemporarySnapOverrideKind::endpoint);

        CHECK(
            interaction.resolvePointerInput(
                {3.0, 4.0})
                .has_value());
        CHECK(
            interaction.temporarySnapOverride() ==
            TemporarySnapOverrideKind::endpoint);

        const auto first =
            interaction.acceptLinePoint(
                {3.0, 4.0});
        CHECK(
            first.outcome ==
            LinePointOutcome::first_point_accepted);
        CHECK(
            !interaction.temporarySnapOverride()
                 .has_value());

        CHECK(
            interaction.setTemporarySnapOverride(
                TemporarySnapOverrideKind::none));
        const auto degenerate =
            interaction.acceptLinePoint(
                {3.0, 4.0});
        CHECK(
            degenerate.outcome ==
            LinePointOutcome::zero_length_ignored);
        CHECK(
            interaction.temporarySnapOverride() ==
            TemporarySnapOverrideKind::none);

        CHECK(interaction.escape());
        CHECK(
            !interaction.temporarySnapOverride()
                 .has_value());

        CHECK(
            interaction.setTemporarySnapOverride(
                TemporarySnapOverrideKind::midpoint));
        CHECK(
            interaction.setTemporarySnapOverride(
                TemporarySnapOverrideKind::none));
        CHECK(
            interaction.temporarySnapOverride() ==
            TemporarySnapOverrideKind::none);

        interaction.activateCircle();
        CHECK(
            !interaction.temporarySnapOverride()
                 .has_value());
        CHECK(
            interaction.setTemporarySnapOverride(
                TemporarySnapOverrideKind::center));
        const auto center_result =
            interaction.acceptCirclePoint(
                {20.0, 20.0});
        CHECK(
            center_result.outcome ==
            CirclePointOutcome::center_accepted);
        CHECK(
            !interaction.temporarySnapOverride()
                 .has_value());
    }

    std::cout << "r11_snap_core_test passed\n";
    return 0;
}
