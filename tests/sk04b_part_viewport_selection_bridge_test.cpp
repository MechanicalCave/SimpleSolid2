#include "part_document_tree_controller.hpp"
#include "part_viewport_controller.hpp"

#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/profile.hpp>

#include <QApplication>
#include <QTreeWidget>
#include <QWidget>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <utility>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04B controller bridge CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

class TestViewport final
    : public QWidget,
      public viewer::IDocumentViewport {
public:
    using QWidget::QWidget;

    std::optional<viewer::CameraState>
    cameraState() const override {
        return camera_;
    }

    bool setCameraState(
        const viewer::CameraState& state) override {
        camera_ = state;
        return true;
    }

    bool setStandardView(
        viewer::StandardView) override {
        return true;
    }

    bool setProjection(
        viewer::CameraProjection) override {
        return true;
    }

    void fitAll() override {}

    bool setReferenceScene(
        const viewer::ReferenceScene& scene) override {
        return scene.valid();
    }

    bool setSketchScene(
        const viewer::SketchScene& scene) override {
        if (!scene.valid()) return false;
        sketch_scene_ = scene;
        return true;
    }

    bool setSketchPreviewScene(
        const viewer::SketchPreviewScene& scene) override {
        return scene.valid();
    }

    bool setProfileScene(
        const viewer::ProfileScene& scene) override {
        if (!scene.valid()) return false;
        profile_scene_ = scene;
        return true;
    }

    bool setProfilePreviewScene(
        const viewer::ProfilePreviewScene& scene) override {
        if (!scene.valid()) return false;
        profile_preview_scene_ = scene;
        return true;
    }

    bool setPresentationSelection(
        const viewer::PresentationSelection& selection) override {
        if (!selection.valid()) return false;
        selection_ = selection;
        return true;
    }

    bool setSketchMeasureMarkerScene(
        const viewer::SketchMeasureMarkerScene& scene) override {
        if (!scene.valid()) return false;
        measure_marker_scene_ = scene;
        return true;
    }

    viewer::SketchMeasureMarkerQueryResult
    querySketchMeasureMarkers(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return measure_marker_query_;
    }

    bool setSketchMeasureCueScene(
        const viewer::SketchMeasureCueScene& scene) override {
        if (!scene.valid()) return false;
        measure_cue_scene_ = scene;
        return true;
    }

    viewer::SketchPointQueryResult
    querySketchPresentation(
        viewer::ViewportPoint2 point) override {
        if (!point.valid()) return {};
        return point_query_;
    }

    viewer::SketchRectangleQueryResult
    querySketchPresentations(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule rule) override {
        if (!rectangle.valid()) return {};
        last_rule_ = rule;
        return rectangle_query_;
    }

    bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay) override {
        if (!overlay.valid()) return false;
        overlay_ = overlay;
        return true;
    }

    void clearSketchSelectionBoxOverlay() override {
        overlay_.reset();
    }

    void setSelectionIntentHandler(
        viewer::SelectionIntentHandler handler) override {
        selection_handler_ = std::move(handler);
    }

    void setSpatialPointerHandler(
        viewer::SpatialPointerHandler handler) override {
        spatial_handler_ = std::move(handler);
    }

    void setPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing) override {
        routing_ = routing;
    }

    void setCursorMode(
        viewer::ViewportCursorMode mode) override {
        cursor_mode_ = mode;
    }

    void emitReferenceIntent(
        const viewer::SelectionIntent& intent) {
        CHECK(static_cast<bool>(selection_handler_));
        selection_handler_(intent);
    }

    viewer::CameraState camera_;
    viewer::SketchScene sketch_scene_;
    viewer::ProfileScene profile_scene_;
    viewer::ProfilePreviewScene profile_preview_scene_;
    viewer::PresentationSelection selection_;
    viewer::SketchPointQueryResult point_query_{
        true,
        std::nullopt};
    viewer::SketchRectangleQueryResult rectangle_query_{
        true,
        {}};
    viewer::SketchMeasureMarkerScene measure_marker_scene_;
    viewer::SketchMeasureMarkerQueryResult measure_marker_query_{
        true,
        {}};
    viewer::SketchMeasureCueScene measure_cue_scene_;
    std::optional<viewer::SketchRectangleSelectionRule>
        last_rule_;
    std::optional<viewer::SketchSelectionBoxOverlay>
        overlay_;
    viewer::SelectionIntentHandler selection_handler_;
    viewer::SpatialPointerHandler spatial_handler_;
    viewer::PrimaryPointerRouting routing_{
        viewer::PrimaryPointerRouting::
            presentation_selection};
    viewer::ViewportCursorMode cursor_mode_{
        viewer::ViewportCursorMode::
            system_default};
};

// PG-01C/C0: a linked Circle's authored seed may be a valid local
// Circle/closed Profile, but it is never current geometric truth without
// the corresponding current upstream Body stage and exact provider.
void verifyMissingProjectionProviderNeverShowsSeed() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession authored{
        std::filesystem::path{
            "pg01c-broken-presentation.ss2part"},
        std::move(document)};

    const auto upstream = authored.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(upstream.ok() && upstream.sketch_id);
    const auto rectangle = authored.execute(
        application::AddSketchRectangleCommand{
            *upstream.sketch_id,
            authored.document().revision(),
            {-15.0, -10.0},
            {15.0, 10.0},
            sketch::EntityRole::regular,
            false});
    CHECK(rectangle.ok() && rectangle.entity_ids.size() == 4U);
    const auto* upstream_sketch =
        authored.document().findSketch(*upstream.sketch_id);
    CHECK(upstream_sketch);
    const auto upstream_regions =
        sketch::analyzeRegions(upstream_sketch->model);
    CHECK(upstream_regions.complete());
    CHECK(upstream_regions.regions.size() == 1U);
    const auto upstream_intent =
        part::makeProfileRegionIntent(
            upstream_regions.regions.front());
    CHECK(upstream_intent);
    const auto upstream_profile = authored.execute(
        application::CreateProfileCommand{
            *upstream.sketch_id,
            authored.document().revision(),
            *upstream_intent});
    CHECK(upstream_profile.ok() && upstream_profile.profile_id);

    const auto target = authored.execute(
        application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
    CHECK(target.ok() && target.sketch_id);
    const auto old_seed = authored.execute(
        application::AddSketchCircleCommand{
            *target.sketch_id,
            {55.0, 60.0},
            8.0});
    const auto local = authored.execute(
        application::AddSketchLineCommand{
            *target.sketch_id,
            {0.0, 0.0},
            {6.0, 0.0}});
    CHECK(old_seed.ok() && old_seed.entity_id);
    CHECK(local.ok() && local.entity_id);
    const auto* target_sketch =
        authored.document().findSketch(*target.sketch_id);
    CHECK(target_sketch);
    const auto region =
        sketch::analyzeRegions(target_sketch->model);
    CHECK(region.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(region.regions.front());
    CHECK(intent);
    const auto linked_profile = authored.execute(
        application::CreateProfileCommand{
            *target.sketch_id,
            authored.document().revision(),
            *intent});
    CHECK(linked_profile.ok() && linked_profile.profile_id);

    auto state = authored.document().state();
    const auto producer = state.body.next_feature_id.allocate();
    CHECK(producer);
    state.body.features.push_back(
        part::PartFeature{
            *producer,
            "Upstream",
            false,
            part::ExtrudeFeature{
                *upstream_profile.profile_id,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false}}});
    const part::FeatureSurfaceAddress cap{
        *producer,
        part::FeatureSurfaceRoleKind::extent_cap,
        std::nullopt,
        0U,
        0U,
        false};
    const part::FeatureSurfaceAddress side{
        *producer,
        part::FeatureSurfaceRoleKind::side,
        rectangle.entity_ids.front(),
        0U,
        0U,
        false};
    std::vector<part::FeatureSurfaceAddress> surfaces{cap, side};
    std::sort(surfaces.begin(), surfaces.end());
    part::MaterialEdgeReference source{
        {part::BodyStageKind::after_feature, *producer},
        {*producer,
         part::FeatureCurveRoleKind::cap_side,
         std::move(surfaces)},
        part::SingularAtAuthoredStage{}};
    CHECK(source.valid());
    auto staged = std::find_if(
        state.sketches.begin(), state.sketches.end(),
        [&target](const part::PartSketch& item) {
            return item.id == *target.sketch_id;
        });
    CHECK(staged != state.sketches.end());
    staged->projection_bindings.push_back(
        {*old_seed.entity_id, source});

    auto restored = part::PartDocument::restore(
        authored.document().documentId(),
        std::move(state),
        authored.document().revision());
    CHECK(restored.ok() && restored.document);
    application::DocumentSession session{
        std::filesystem::path{"pg01c-restored.ss2part"},
        std::move(*restored.document)};

    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    TestViewport viewport;
    ui::PartViewportController controller{
        tree_controller, &viewport};
    // Deliberately no ModelingKernel/EdgeProjectionQuery: the linked
    // source is structurally valid intent but cannot currently resolve.
    controller.setDocumentSession(&session);
    CHECK(!controller.profilePresentationFor(
        *linked_profile.profile_id));
    controller.setSketchEditSketch(*target.sketch_id);
    CHECK(viewport.sketch_scene_.lines.size() == 1U);
    CHECK(viewport.sketch_scene_.curves.empty());
    CHECK(!controller.sketchPresentationFor(
        *old_seed.entity_id));
    CHECK(controller.sketchPresentationFor(
        *local.entity_id));
    CHECK(!controller.profilePresentationFor(
        *linked_profile.profile_id));
    controller.refreshPresentation();
    CHECK(viewport.sketch_scene_.lines.size() == 1U);
    CHECK(viewport.sketch_scene_.curves.empty());
    CHECK(!controller.sketchPresentationFor(
        *old_seed.entity_id));
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app{argc, argv};

    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        std::filesystem::path{
            "sk04b-controller.ss2part"},
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok());
    CHECK(created.sketch_id.has_value());
    const auto sketch_id = *created.sketch_id;

    const auto first =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{-10.0, 0.0},
                sketch::Point2{10.0, 0.0}});
    const auto second =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                sketch::Point2{0.0, -10.0},
                sketch::Point2{0.0, 10.0}});
    CHECK(first.ok() && first.entity_id.has_value());
    CHECK(second.ok() && second.entity_id.has_value());

    const auto profile_circle =
        session.execute(
            application::AddSketchCircleCommand{
                sketch_id,
                sketch::Point2{30.0, 30.0},
                4.0});
    CHECK(
        profile_circle.ok() &&
        profile_circle.entity_id.has_value());

    const auto* profile_source =
        session.document().findSketch(sketch_id);
    CHECK(profile_source != nullptr);
    const auto profile_analysis =
        sketch::analyzeRegions(
            profile_source->model);
    const auto profile_pick =
        sketch::pickRegion(
            profile_source->model,
            profile_analysis,
            sketch::Point2{30.0, 30.0});
    CHECK(profile_pick.region_index.has_value());
    const auto profile_region =
        std::find_if(
            profile_analysis.regions.begin(),
            profile_analysis.regions.end(),
            [&profile_pick](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *profile_pick.region_index;
            });
    CHECK(
        profile_region !=
        profile_analysis.regions.end());
    const auto profile_intent =
        part::makeProfileRegionIntent(
            *profile_region);
    CHECK(profile_intent.has_value());
    const auto profile_created =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *profile_intent});
    CHECK(profile_created.ok());
    CHECK(profile_created.profile_id.has_value());
    const auto profile_id =
        *profile_created.profile_id;

    QTreeWidget tree;
    ui::PartDocumentTreeController tree_controller{tree};
    TestViewport viewport;
    ui::PartViewportController controller{
        tree_controller,
        &viewport};

    controller.setDocumentSession(&session);
    CHECK(viewport.profile_scene_.profiles.size() == 1U);
    const auto profile_token =
        viewport.profile_scene_.profiles.front().token;
    CHECK(
        controller.profileFor(profile_token) ==
        profile_id);
    CHECK(
        controller.profilePresentationFor(profile_id) ==
        profile_token);
    CHECK(
        viewport.profile_scene_.profiles.front()
            .region.outer.size() >= 16U);

    controller.setSketchEditSketch(sketch_id);
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    // Package F visibility is semantic per-object state: hiding the Profile
    // must not hide or replace the active source Sketch presentation.
    CHECK(
        session.execute(
            application::SetProfilePropertiesCommand{
                profile_id,
                session.document().revision(),
                "Profile001",
                part::ProfileVisibilityPolicy::
                    force_hidden})
            .ok());
    controller.refreshPresentation();
    CHECK(viewport.profile_scene_.profiles.empty());
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    CHECK(
        session.execute(
            application::SetProfilePropertiesCommand{
                profile_id,
                session.document().revision(),
                "Profile001",
                part::ProfileVisibilityPolicy::
                    force_shown})
            .ok());
    controller.refreshPresentation();
    CHECK(viewport.profile_scene_.profiles.size() == 1U);
    CHECK(viewport.sketch_scene_.lines.size() == 2U);

    const auto profile_edit_token =
        controller.profilePresentationFor(
            profile_id);
    CHECK(profile_edit_token.has_value());
    CHECK(*profile_edit_token != profile_token);

    const auto first_token =
        viewport.sketch_scene_.lines[0].token;
    const auto second_token =
        viewport.sketch_scene_.lines[1].token;

    CHECK(
        controller.sketchPresentationFor(
            *first.entity_id) == first_token);
    CHECK(
        controller.sketchPresentationFor(
            *second.entity_id) == second_token);

    viewport.point_query_ = {
        true,
        first_token};
    const auto point_hit =
        controller.querySketchEntityAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(point_hit.completed);
    CHECK(point_hit.hit.has_value());
    CHECK(point_hit.hit->sketch_id == sketch_id);
    CHECK(point_hit.hit->entity_id == *first.entity_id);

    viewport.point_query_ = {
        true,
        std::nullopt};
    const auto point_empty =
        controller.querySketchEntityAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(point_empty.completed);
    CHECK(!point_empty.hit.has_value());

    viewport.point_query_ = {};
    const auto point_failed =
        controller.querySketchEntityAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(!point_failed.completed);

    // R8B marker/cue bridge maps semantic runtime references to
    // provider tokens and back without exposing EntityId to the Viewer.
    const auto* measure_source =
        session.document().findSketch(sketch_id);
    CHECK(measure_source != nullptr);
    const auto measure_catalog =
        sketch::measurePointCatalog(
            measure_source->model);
    const sketch::MeasurePointRef measure_selected{
        *first.entity_id,
        sketch::MeasurePointRole::line_start};
    const sketch::MeasurePointRef measure_circle_center{
        *profile_circle.entity_id,
        sketch::MeasurePointRole::circle_center};
    const auto measure_relation =
        sketch::measureRelation(
            measure_source->model,
            sketch::MeasureRelationTarget{
                measure_circle_center},
            sketch::MeasureRelationTarget{
                sketch::MeasureLineRef{
                    *first.entity_id}});
    CHECK(measure_relation.has_value());
    const auto measure_cue =
        sketch::makeRelationalMeasurementCue(
            *measure_relation);
    CHECK(measure_cue.has_value());
    CHECK(controller.projectSketchMeasurePresentation(
        measure_catalog,
        {measure_selected},
        measure_cue));
    CHECK(viewport.measure_marker_scene_.valid());
    CHECK(viewport.measure_marker_scene_.markers.size() == 11U);
    CHECK(viewport.measure_marker_scene_.selected.size() == 1U);
    CHECK(viewport.measure_cue_scene_.valid());
    CHECK(viewport.measure_cue_scene_.highlighted_entities.size() == 1U);
    CHECK(viewport.measure_cue_scene_.segments.size() == 2U);
    CHECK(viewport.measure_cue_scene_.cue_point.has_value());

    const auto selected_marker_key =
        viewport.measure_marker_scene_.selected.front();
    const auto circle_center_key =
        std::find_if(
            viewport.measure_marker_scene_.markers.begin(),
            viewport.measure_marker_scene_.markers.end(),
            [profile_circle](
                const viewer::SketchMeasureMarkerPresentation& marker) {
                return marker.key.role ==
                    viewer::SketchMeasureMarkerRole::circle_center;
            });
    CHECK(
        circle_center_key !=
        viewport.measure_marker_scene_.markers.end());

    viewport.measure_marker_query_ = {
        true,
        {
            selected_marker_key,
            circle_center_key->key}};
    const auto measure_hits =
        controller.querySketchMeasureMarkersAt(
            viewer::ViewportPoint2{100.0, 100.0});
    CHECK(measure_hits.completed);
    CHECK(measure_hits.hits.size() == 2U);
    CHECK(
        measure_hits.hits[0].sketch_id ==
        sketch_id);
    CHECK(
        measure_hits.hits[0].point ==
        measure_selected);
    CHECK(
        measure_hits.hits[1].point ==
        measure_circle_center);

    viewport.measure_marker_query_ = {};
    CHECK(
        !controller.querySketchMeasureMarkersAt(
            viewer::ViewportPoint2{100.0, 100.0})
             .completed);

    controller.clearSketchMeasurePresentation();
    CHECK(viewport.measure_marker_scene_.empty());
    CHECK(viewport.measure_cue_scene_.empty());

    const auto rectangle =
        viewer::normalizedViewportRect(
            viewer::ViewportPoint2{10.0, 10.0},
            viewer::ViewportPoint2{200.0, 150.0});
    CHECK(rectangle.has_value());

    viewport.rectangle_query_ = {
        true,
        {first_token, second_token}};
    const auto rectangle_hits =
        controller.querySketchEntities(
            *rectangle,
            viewer::SketchRectangleSelectionRule::
                crossing);
    CHECK(rectangle_hits.completed);
    CHECK(rectangle_hits.hits.size() == 2U);
    CHECK(
        viewport.last_rule_ ==
        viewer::SketchRectangleSelectionRule::
            crossing);

    const auto revision_before_overlay =
        session.document().revision();
    const auto undo_before_overlay =
        session.undoDepth();
    const bool dirty_before_overlay =
        session.needsSave();

    const viewer::SketchSelectionBoxOverlay overlay{
        {20.0, 30.0},
        {140.0, 90.0},
        viewer::SketchRectangleSelectionRule::window};
    CHECK(
        controller.setSketchSelectionBoxOverlay(
            overlay));
    CHECK(viewport.overlay_ == overlay);
    controller.clearSketchSelectionBoxOverlay();
    CHECK(!viewport.overlay_.has_value());
    CHECK(
        session.document().revision() ==
        revision_before_overlay);
    CHECK(session.undoDepth() == undo_before_overlay);
    CHECK(session.needsSave() == dirty_before_overlay);

    // Existing built-in reference selection and Sketch highlight projection
    // can coexist in one runtime PresentationSelection.
    viewport.emitReferenceIntent(
        viewer::SelectionIntent{
            viewer::PresentationToken{0x101U},
            viewer::SelectionIntentMode::replace});
    CHECK(
        controller.projectSketchEntitySelection(
            {*first.entity_id, *second.entity_id},
            *second.entity_id));
    CHECK(viewport.selection_.selected.size() == 3U);
    CHECK(
        viewport.selection_.primary ==
        second_token);

    CHECK(!controller.projectSketchEntitySelection(
        {*first.entity_id},
        *second.entity_id));

    std::vector<part::ProfileId> reported_profiles;
    std::optional<part::ProfileId>
        reported_primary_profile;
    controller.setProfileSelectionChangedHandler(
        [&reported_profiles,
         &reported_primary_profile](
            const std::vector<part::ProfileId>& selected,
            std::optional<part::ProfileId> primary) {
            reported_profiles = selected;
            reported_primary_profile = primary;
        });

    controller.setProfileSelectionFromTree(
        {profile_id},
        profile_id);
    CHECK(
        viewport.selection_.primary ==
        *profile_edit_token);
    CHECK(
        tree_controller.selectedProfileIds() ==
        std::vector<part::ProfileId>{profile_id});

    viewport.emitReferenceIntent(
        viewer::SelectionIntent{
            *profile_edit_token,
            viewer::SelectionIntentMode::replace});
    CHECK(
        reported_profiles ==
        std::vector<part::ProfileId>{profile_id});
    CHECK(
        reported_primary_profile ==
        profile_id);
    CHECK(
        tree_controller.primaryProfileId() ==
        profile_id);

    viewport.emitReferenceIntent(
        viewer::SelectionIntent{
            *profile_edit_token,
            viewer::SelectionIntentMode::toggle});
    CHECK(reported_profiles.empty());
    CHECK(!reported_primary_profile.has_value());
    CHECK(
        tree_controller.selectedProfileIds().empty());

    CHECK(
        controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            select_pick_box);

    CHECK(
        controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                create_edit_crosshair));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);
    CHECK(
        viewport.cursor_mode_ ==
        viewer::ViewportCursorMode::
            create_edit_crosshair);

    CHECK(
        controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                select_pick_box));
    CHECK(
        viewport.routing_ ==
        viewer::PrimaryPointerRouting::
            spatial_tool_input);

    const auto stale_token = first_token;
    const auto stale_profile_token =
        *profile_edit_token;
    controller.refreshPresentation();
    CHECK(
        !controller.sketchEntityFor(
             stale_token)
             .has_value());
    CHECK(
        !controller.profileFor(
             stale_profile_token)
             .has_value());
    const auto new_first_token =
        controller.sketchPresentationFor(
            *first.entity_id);
    CHECK(new_first_token.has_value());
    CHECK(*new_first_token != stale_token);
    const auto new_profile_token =
        controller.profilePresentationFor(profile_id);
    CHECK(new_profile_token.has_value());
    CHECK(
        *new_profile_token !=
        stale_profile_token);

    controller.setSketchEditSketch(std::nullopt);
    CHECK(
        !controller.setSketchPrimaryPointerRouting(
            viewer::PrimaryPointerRouting::
                spatial_tool_input));
    CHECK(
        !controller.setSketchCursorMode(
            viewer::ViewportCursorMode::
                create_edit_crosshair));
    CHECK(
        !controller.sketchPresentationFor(
             *first.entity_id)
             .has_value());

    verifyMissingProjectionProviderNeverShowsSeed();
    return EXIT_SUCCESS;
}
