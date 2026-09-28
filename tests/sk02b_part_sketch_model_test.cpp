#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/part/profile.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-02B Part Sketch model CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::PartSketch makeSketch() {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());

    const auto placement =
        part::sketchPlacementForSupport(*support);
    CHECK(placement.has_value());

    return part::PartSketch{
        sketch::SketchId::generate(),
        *support,
        *placement,
        true};
}

} // namespace

int main() {
    auto hosted = makeSketch();

    CHECK(hosted.model.entityCount() == 0U);

    const auto line_id =
        hosted.model.addLine(
            sketch::Point2{0.0, 0.0},
            sketch::Point2{25.0, 0.0});

    CHECK(line_id.valid());
    CHECK(hosted.model.entityCount() == 1U);
    CHECK(hosted.model.findLine(line_id) != nullptr);

    auto copied = hosted;
    CHECK(copied == hosted);
    CHECK(copied.model.findLine(line_id) != nullptr);

    CHECK(copied.model.erase(line_id));
    CHECK(copied.model.entityCount() == 0U);
    CHECK(hosted.model.entityCount() == 1U);
    CHECK(hosted.model.findLine(line_id) != nullptr);
    CHECK(copied != hosted);

    part::PartAuthoredState state;
    state.sketches.push_back(hosted);

    auto copied_state = state;
    CHECK(copied_state == state);
    CHECK(copied_state.sketches.size() == 1U);

    CHECK(copied_state.sketches[0].model.erase(line_id));
    CHECK(copied_state != state);
    CHECK(state.sketches[0].model.findLine(line_id) != nullptr);


    // Package F: derived region geometry becomes durable Part intent without
    // persisting coordinates, curve parameters or Viewer presentation data.
    part::ProfileIdCursor profile_cursor;
    const auto profile_id_1 =
        profile_cursor.allocate();
    const auto profile_id_2 =
        profile_cursor.allocate();
    CHECK(profile_id_1.has_value());
    CHECK(profile_id_2.has_value());
    CHECK(profile_id_1->valid());
    CHECK(profile_id_2->valid());
    CHECK(*profile_id_1 != *profile_id_2);
    CHECK(profile_id_1->serialized() == "1");
    CHECK(profile_id_2->serialized() == "2");
    CHECK(
        part::ProfileId::parse(
            profile_id_1->serialized()) ==
        profile_id_1);
    const auto observed_cursor =
        part::ProfileIdCursor::parse("10");
    CHECK(observed_cursor.has_value());
    profile_cursor.preserve(*observed_cursor);
    const auto profile_id_10 =
        profile_cursor.allocate();
    CHECK(profile_id_10.has_value());
    CHECK(profile_id_10->serialized() == "10");

    sketch::SketchModel profile_model;
    const auto profile_bottom =
        profile_model.addLine(
            {0.0, 0.0},
            {4.0, 0.0});
    const auto profile_right =
        profile_model.addLine(
            {4.0, 0.0},
            {4.0, 2.0});
    const auto profile_top =
        profile_model.addLine(
            {4.0, 2.0},
            {0.0, 2.0});
    const auto profile_left =
        profile_model.addLine(
            {0.0, 2.0},
            {0.0, 0.0});

    const auto profile_regions =
        sketch::analyzeRegions(
            profile_model);
    CHECK(profile_regions.complete());
    CHECK(profile_regions.regions.size() == 1U);

    const auto profile_intent =
        part::makeProfileRegionIntent(
            profile_regions.regions.front());
    CHECK(profile_intent.has_value());
    CHECK(!profile_intent->outer.boundary.empty());

    const auto resolved_initial =
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent);
    CHECK(resolved_initial.valid());
    CHECK(
        std::abs(
            resolved_initial.region->area -
            8.0) < 1.0e-12);

    // Geometry may move while the same EntityId/endpoint topology remains.
    CHECK(
        profile_model.updateLine(
            profile_right,
            {4.0, 0.0},
            {4.0, 3.0}));
    CHECK(
        profile_model.updateLine(
            profile_top,
            {4.0, 3.0},
            {0.0, 3.0}));
    CHECK(
        profile_model.updateLine(
            profile_left,
            {0.0, 3.0},
            {0.0, 0.0}));

    const auto resolved_stretched =
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent);
    CHECK(resolved_stretched.valid());
    CHECK(
        std::abs(
            resolved_stretched.region->area -
            12.0) < 1.0e-12);

    // New unrelated Regular geometry may subdivide the arrangement inside
    // a Profile without changing its authored semantic outer boundary.
    const auto internal_circle =
        profile_model.addCircle(
            {2.0, 1.5},
            0.5);
    CHECK(internal_circle.valid());
    const auto subdivided =
        sketch::analyzeRegions(
            profile_model);
    CHECK(subdivided.complete());
    CHECK(subdivided.regions.size() == 2U);
    const auto resolved_subdivided =
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent);
    CHECK(resolved_subdivided.valid());
    CHECK(
        std::abs(
            resolved_subdivided.region->area -
            12.0) < 1.0e-12);
    CHECK(
        resolved_subdivided.region
            ->holes.empty());

    // Construction removes the source from material-region topology without
    // rewriting the durable intent. Restoring Regular recovers it.
    CHECK(
        profile_model.setEntityRole(
            profile_top,
            sketch::EntityRole::construction));
    CHECK(
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent)
            .status ==
        part::ProfileIntentResolutionStatus::
            unresolved_intent);
    CHECK(
        profile_model.setEntityRole(
            profile_top,
            sketch::EntityRole::regular));
    CHECK(
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent)
            .valid());

    // Deleting a referenced EntityId fails closed. Geometrically identical
    // replacement geometry receives a fresh id and does not steal the intent.
    CHECK(profile_model.erase(profile_top));
    CHECK(
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent)
            .status ==
        part::ProfileIntentResolutionStatus::
            missing_source_entity);
    const auto replacement_top =
        profile_model.addLine(
            {4.0, 3.0},
            {0.0, 3.0});
    CHECK(replacement_top != profile_top);
    CHECK(
        part::resolveProfileRegionIntent(
            profile_model,
            *profile_intent)
            .status ==
        part::ProfileIntentResolutionStatus::
            missing_source_entity);

    // Intersection-based source spans carry semantic counterpart EntityId +
    // canonical branch, not raw curve parameters.
    sketch::SketchModel grid_model;
    (void)grid_model.addLine(
        {0.0, 0.0},
        {4.0, 0.0});
    (void)grid_model.addLine(
        {4.0, 0.0},
        {4.0, 2.0});
    (void)grid_model.addLine(
        {4.0, 2.0},
        {0.0, 2.0});
    (void)grid_model.addLine(
        {0.0, 2.0},
        {0.0, 0.0});
    const auto grid_vertical =
        grid_model.addLine(
            {2.0, 0.0},
            {2.0, 2.0});
    const auto grid_horizontal =
        grid_model.addLine(
            {0.0, 1.0},
            {4.0, 1.0});

    const auto grid_analysis =
        sketch::analyzeRegions(grid_model);
    CHECK(grid_analysis.complete());
    const auto picked =
        sketch::pickRegion(
            grid_model,
            grid_analysis,
            {1.0, 0.5});
    CHECK(
        picked.location ==
        sketch::RegionPointLocation::inside);
    CHECK(picked.region_index.has_value());

    const auto selected =
        std::find_if(
            grid_analysis.regions.begin(),
            grid_analysis.regions.end(),
            [&picked](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *picked.region_index;
            });
    CHECK(selected != grid_analysis.regions.end());

    const auto grid_intent =
        part::makeProfileRegionIntent(*selected);
    CHECK(grid_intent.has_value());

    bool has_intersection_intent = false;
    for (const auto& use :
         grid_intent->outer.boundary) {
        if ((use.start_anchor &&
             use.start_anchor->kind ==
                 part::ProfileBoundaryAnchorKind::
                     intersection) ||
            (use.end_anchor &&
             use.end_anchor->kind ==
                 part::ProfileBoundaryAnchorKind::
                     intersection)) {
            has_intersection_intent = true;
        }
    }
    CHECK(has_intersection_intent);

    CHECK(
        grid_model.updateLine(
            grid_vertical,
            {3.0, 0.0},
            {3.0, 2.0}));
    CHECK(
        grid_model.updateLine(
            grid_horizontal,
            {0.0, 1.25},
            {4.0, 1.25}));
    CHECK(
        part::resolveProfileRegionIntent(
            grid_model,
            *grid_intent)
            .valid());


    // Direct intent validation accepts the legitimate two-use Arc+Line loop,
    // where the same curve pair meets at both closure vertices.
    sketch::SketchModel two_use_model;
    const auto two_use_arc =
        two_use_model.addArc(
            {0.0, 0.0},
            1.0,
            0.0,
            std::numbers::pi_v<double>);
    const auto two_use_line =
        two_use_model.addLine(
            {-1.0, 0.0},
            {1.0, 0.0});
    CHECK(two_use_arc.valid());
    CHECK(two_use_line.valid());
    const auto two_use_analysis =
        sketch::analyzeRegions(
            two_use_model);
    CHECK(two_use_analysis.complete());
    CHECK(
        two_use_analysis.regions.size() ==
        1U);
    const auto two_use_intent =
        part::makeProfileRegionIntent(
            two_use_analysis.regions.front());
    CHECK(two_use_intent.has_value());
    const auto two_use_resolved =
        part::resolveProfileRegionIntent(
            two_use_model,
            *two_use_intent);
    CHECK(two_use_resolved.valid());
    CHECK(
        std::abs(
            two_use_resolved.region->area -
            std::numbers::pi_v<double> *
                0.5) < 1.0e-12);

    // A blind T branch may fragment runtime arrangement edges, but bridge
    // cancellation plus same-source coalescing keeps it out of durable intent.
    sketch::SketchModel tee_intent_model;
    const auto tee_bottom =
        tee_intent_model.addLine(
            {0.0, 0.0},
            {4.0, 0.0});
    (void)tee_intent_model.addLine(
        {4.0, 0.0},
        {4.0, 2.0});
    (void)tee_intent_model.addLine(
        {4.0, 2.0},
        {0.0, 2.0});
    (void)tee_intent_model.addLine(
        {0.0, 2.0},
        {0.0, 0.0});
    const auto tee_branch =
        tee_intent_model.addLine(
            {2.0, 0.0},
            {2.0, 1.0});

    const auto tee_intent_analysis =
        sketch::analyzeRegions(
            tee_intent_model);
    CHECK(tee_intent_analysis.complete());
    CHECK(
        tee_intent_analysis.regions.size() ==
        1U);
    const auto tee_intent =
        part::makeProfileRegionIntent(
            tee_intent_analysis.regions.front());
    CHECK(tee_intent.has_value());

    bool tee_branch_referenced = false;
    for (const auto& use :
         tee_intent->outer.boundary) {
        CHECK(use.source_entity != tee_branch);
        if ((use.start_anchor &&
             use.start_anchor->kind ==
                 part::ProfileBoundaryAnchorKind::
                     intersection &&
             use.start_anchor->other_entity ==
                 tee_branch) ||
            (use.end_anchor &&
             use.end_anchor->kind ==
                 part::ProfileBoundaryAnchorKind::
                     intersection &&
             use.end_anchor->other_entity ==
                 tee_branch)) {
            tee_branch_referenced = true;
        }
    }
    CHECK(!tee_branch_referenced);
    CHECK(
        tee_intent_model.erase(
            tee_branch));
    const auto tee_after_delete =
        part::resolveProfileRegionIntent(
            tee_intent_model,
            *tee_intent);
    CHECK(tee_after_delete.valid());
    CHECK(
        std::abs(
            tee_after_delete.region->area -
            8.0) < 1.0e-12);
    CHECK(
        tee_intent_model.findLine(
            tee_bottom) != nullptr);


    // Package F transient Add/Subtract algebra. These pure draft operations
    // produce RegionIntent only; authored mutation remains a later Finish.
    sketch::SketchModel add_model;
    (void)add_model.addLine(
        {0.0, 0.0},
        {4.0, 0.0});
    (void)add_model.addLine(
        {4.0, 0.0},
        {4.0, 2.0});
    (void)add_model.addLine(
        {4.0, 2.0},
        {0.0, 2.0});
    (void)add_model.addLine(
        {0.0, 2.0},
        {0.0, 0.0});
    (void)add_model.addLine(
        {2.0, 0.0},
        {2.0, 2.0});

    const auto add_analysis =
        sketch::analyzeRegions(add_model);
    CHECK(add_analysis.complete());
    CHECK(add_analysis.regions.size() == 2U);
    const auto add_left_pick =
        sketch::pickRegion(
            add_model,
            add_analysis,
            {1.0, 1.0});
    const auto add_right_pick =
        sketch::pickRegion(
            add_model,
            add_analysis,
            {3.0, 1.0});
    CHECK(add_left_pick.region_index.has_value());
    CHECK(add_right_pick.region_index.has_value());

    const auto add_left =
        std::find_if(
            add_analysis.regions.begin(),
            add_analysis.regions.end(),
            [&add_left_pick](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *add_left_pick.region_index;
            });
    CHECK(add_left != add_analysis.regions.end());
    const auto add_draft =
        part::makeProfileRegionIntent(*add_left);
    CHECK(add_draft.has_value());

    const auto added =
        part::applyProfileAreaEdit(
            add_model,
            *add_draft,
            *add_right_pick.region_index,
            part::ProfileAreaEditMode::add_area);
    CHECK(added.changed());
    CHECK(added.region_intent.has_value());
    CHECK(added.region.has_value());
    CHECK(
        std::abs(added.region->area - 8.0) <
        1.0e-12);
    CHECK(added.region->holes.empty());
    CHECK(
        part::resolveProfileRegionIntent(
            add_model,
            *added.region_intent)
            .valid());

    const auto add_again =
        part::applyProfileAreaEdit(
            add_model,
            *added.region_intent,
            *add_right_pick.region_index,
            part::ProfileAreaEditMode::add_area);
    CHECK(
        add_again.status ==
        part::ProfileAreaEditStatus::no_change);

    const auto subtract_right =
        part::applyProfileAreaEdit(
            add_model,
            *added.region_intent,
            *add_right_pick.region_index,
            part::ProfileAreaEditMode::
                subtract_area);
    CHECK(subtract_right.changed());
    CHECK(subtract_right.region.has_value());
    CHECK(
        std::abs(
            subtract_right.region->area -
            4.0) < 1.0e-12);

    // Subtracting an internal bounded cell creates a hole; adding it back
    // removes the shared hole boundary.
    sketch::SketchModel hole_model;
    (void)hole_model.addLine(
        {0.0, 0.0},
        {4.0, 0.0});
    (void)hole_model.addLine(
        {4.0, 0.0},
        {4.0, 4.0});
    (void)hole_model.addLine(
        {4.0, 4.0},
        {0.0, 4.0});
    (void)hole_model.addLine(
        {0.0, 4.0},
        {0.0, 0.0});
    const auto hole_base_analysis =
        sketch::analyzeRegions(
            hole_model);
    CHECK(
        hole_base_analysis.regions.size() ==
        1U);
    const auto hole_full_intent =
        part::makeProfileRegionIntent(
            hole_base_analysis.regions.front());
    CHECK(hole_full_intent.has_value());

    (void)hole_model.addCircle(
        {2.0, 2.0},
        1.0);
    const auto hole_analysis =
        sketch::analyzeRegions(
            hole_model);
    CHECK(hole_analysis.complete());
    CHECK(hole_analysis.regions.size() == 2U);
    const auto hole_disk_pick =
        sketch::pickRegion(
            hole_model,
            hole_analysis,
            {2.0, 2.0});
    CHECK(hole_disk_pick.region_index.has_value());

    const auto hole_subtracted =
        part::applyProfileAreaEdit(
            hole_model,
            *hole_full_intent,
            *hole_disk_pick.region_index,
            part::ProfileAreaEditMode::
                subtract_area);
    CHECK(hole_subtracted.changed());
    CHECK(hole_subtracted.region.has_value());
    CHECK(
        hole_subtracted.region->holes.size() ==
        1U);
    CHECK(
        std::abs(
            hole_subtracted.region->area -
            (16.0 -
             std::numbers::pi_v<double>)) <
        1.0e-12);

    const auto hole_added_back =
        part::applyProfileAreaEdit(
            hole_model,
            *hole_subtracted.region_intent,
            *hole_disk_pick.region_index,
            part::ProfileAreaEditMode::add_area);
    CHECK(hole_added_back.changed());
    CHECK(hole_added_back.region.has_value());
    CHECK(hole_added_back.region->holes.empty());
    CHECK(
        std::abs(
            hole_added_back.region->area -
            16.0) < 1.0e-12);

    // A corner-cell subtraction creates a notch while preserving one
    // connected material component.
    sketch::SketchModel notch_model;
    (void)notch_model.addLine(
        {0.0, 0.0},
        {4.0, 0.0});
    (void)notch_model.addLine(
        {4.0, 0.0},
        {4.0, 4.0});
    (void)notch_model.addLine(
        {4.0, 4.0},
        {0.0, 4.0});
    (void)notch_model.addLine(
        {0.0, 4.0},
        {0.0, 0.0});
    const auto notch_base =
        sketch::analyzeRegions(
            notch_model);
    const auto notch_full_intent =
        part::makeProfileRegionIntent(
            notch_base.regions.front());
    CHECK(notch_full_intent.has_value());
    (void)notch_model.addLine(
        {2.0, 0.0},
        {2.0, 4.0});
    (void)notch_model.addLine(
        {0.0, 2.0},
        {4.0, 2.0});
    const auto notch_analysis =
        sketch::analyzeRegions(
            notch_model);
    CHECK(notch_analysis.regions.size() == 4U);
    const auto notch_pick =
        sketch::pickRegion(
            notch_model,
            notch_analysis,
            {3.0, 3.0});
    CHECK(notch_pick.region_index.has_value());
    const auto notch_result =
        part::applyProfileAreaEdit(
            notch_model,
            *notch_full_intent,
            *notch_pick.region_index,
            part::ProfileAreaEditMode::
                subtract_area);
    CHECK(notch_result.changed());
    CHECK(notch_result.region.has_value());
    CHECK(notch_result.region->holes.empty());
    CHECK(
        std::abs(
            notch_result.region->area -
            12.0) < 1.0e-12);

    // Removing a middle strip would split the material into two islands.
    sketch::SketchModel split_model;
    (void)split_model.addLine(
        {0.0, 0.0},
        {6.0, 0.0});
    (void)split_model.addLine(
        {6.0, 0.0},
        {6.0, 2.0});
    (void)split_model.addLine(
        {6.0, 2.0},
        {0.0, 2.0});
    (void)split_model.addLine(
        {0.0, 2.0},
        {0.0, 0.0});
    const auto split_base =
        sketch::analyzeRegions(
            split_model);
    const auto split_full_intent =
        part::makeProfileRegionIntent(
            split_base.regions.front());
    CHECK(split_full_intent.has_value());
    (void)split_model.addLine(
        {2.0, 0.0},
        {2.0, 2.0});
    (void)split_model.addLine(
        {4.0, 0.0},
        {4.0, 2.0});
    const auto split_analysis =
        sketch::analyzeRegions(
            split_model);
    CHECK(split_analysis.regions.size() == 3U);
    const auto split_middle_pick =
        sketch::pickRegion(
            split_model,
            split_analysis,
            {3.0, 1.0});
    CHECK(
        split_middle_pick.region_index.has_value());
    const auto split_result =
        part::applyProfileAreaEdit(
            split_model,
            *split_full_intent,
            *split_middle_pick.region_index,
            part::ProfileAreaEditMode::
                subtract_area);
    CHECK(
        split_result.status ==
        part::ProfileAreaEditStatus::
            disconnected_result);
    CHECK(!split_result.region_intent.has_value());

    // Disconnected and point-only Add are rejected.
    sketch::SketchModel disconnected_model;
    (void)disconnected_model.addCircle(
        {0.0, 0.0},
        1.0);
    (void)disconnected_model.addCircle(
        {4.0, 0.0},
        1.0);
    const auto disconnected_analysis =
        sketch::analyzeRegions(
            disconnected_model);
    CHECK(
        disconnected_analysis.regions.size() ==
        2U);
    const auto disconnected_left =
        sketch::pickRegion(
            disconnected_model,
            disconnected_analysis,
            {0.0, 0.0});
    const auto disconnected_right =
        sketch::pickRegion(
            disconnected_model,
            disconnected_analysis,
            {4.0, 0.0});
    CHECK(disconnected_left.region_index.has_value());
    CHECK(disconnected_right.region_index.has_value());
    const auto disconnected_left_region =
        std::find_if(
            disconnected_analysis.regions.begin(),
            disconnected_analysis.regions.end(),
            [&disconnected_left](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *disconnected_left.region_index;
            });
    CHECK(
        disconnected_left_region !=
        disconnected_analysis.regions.end());
    const auto disconnected_intent =
        part::makeProfileRegionIntent(
            *disconnected_left_region);
    CHECK(disconnected_intent.has_value());
    CHECK(
        part::applyProfileAreaEdit(
            disconnected_model,
            *disconnected_intent,
            *disconnected_right.region_index,
            part::ProfileAreaEditMode::add_area)
            .status ==
        part::ProfileAreaEditStatus::
            disconnected_result);

    sketch::SketchModel tangent_add_model;
    (void)tangent_add_model.addCircle(
        {-1.0, 0.0},
        1.0);
    (void)tangent_add_model.addCircle(
        {1.0, 0.0},
        1.0);
    const auto tangent_add_analysis =
        sketch::analyzeRegions(
            tangent_add_model);
    CHECK(
        tangent_add_analysis.regions.size() ==
        2U);
    const auto tangent_left_pick =
        sketch::pickRegion(
            tangent_add_model,
            tangent_add_analysis,
            {-1.0, 0.0});
    const auto tangent_right_pick =
        sketch::pickRegion(
            tangent_add_model,
            tangent_add_analysis,
            {1.0, 0.0});
    CHECK(tangent_left_pick.region_index.has_value());
    CHECK(tangent_right_pick.region_index.has_value());
    const auto tangent_left_region =
        std::find_if(
            tangent_add_analysis.regions.begin(),
            tangent_add_analysis.regions.end(),
            [&tangent_left_pick](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *tangent_left_pick.region_index;
            });
    CHECK(
        tangent_left_region !=
        tangent_add_analysis.regions.end());
    const auto tangent_add_intent =
        part::makeProfileRegionIntent(
            *tangent_left_region);
    CHECK(tangent_add_intent.has_value());
    CHECK(
        part::applyProfileAreaEdit(
            tangent_add_model,
            *tangent_add_intent,
            *tangent_right_pick.region_index,
            part::ProfileAreaEditMode::add_area)
            .status ==
        part::ProfileAreaEditStatus::
            disconnected_result);

    // Subtracting a disjoint candidate is a draft no-op.
    CHECK(
        part::applyProfileAreaEdit(
            disconnected_model,
            *disconnected_intent,
            *disconnected_right.region_index,
            part::ProfileAreaEditMode::
                subtract_area)
            .status ==
        part::ProfileAreaEditStatus::no_change);

    return EXIT_SUCCESS;
}
