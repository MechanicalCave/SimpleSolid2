#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/part/profile.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>

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

    return EXIT_SUCCESS;
}
