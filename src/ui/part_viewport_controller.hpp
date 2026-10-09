#pragma once

#include "part_document_tree_controller.hpp"

#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>
#include <simplesolid2/sketch/measurement.hpp>
#include <simplesolid2/part/axis_evaluation.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/effective_sketch_projection.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/viewer/document_viewport.hpp>

#include <QObject>

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace simplesolid2::ui {

struct SketchEntityAddress final {
    sketch::SketchId sketch_id;
    sketch::EntityId entity_id;

    friend bool operator==(
        const SketchEntityAddress&,
        const SketchEntityAddress&) = default;
};

struct SketchPointerInput final {
    sketch::SketchId sketch_id;
    viewer::SpatialPointerPhase phase{
        viewer::SpatialPointerPhase::move};
    viewer::ViewportPoint2 viewport_position;
    sketch::Point2 position;
    bool control{};
};

struct SketchEntityPointQueryResult final {
    bool completed{};
    std::optional<SketchEntityAddress> hit;
};

struct SketchGripAddress final {
    sketch::SketchId sketch_id;
    sketch::LineGripRef grip;

    friend bool operator==(
        const SketchGripAddress&,
        const SketchGripAddress&) = default;
};

struct SketchGripPointQueryResult final {
    bool completed{};
    std::optional<SketchGripAddress> hit;
};

struct SketchEntityRectangleQueryResult final {
    bool completed{};
    std::vector<SketchEntityAddress> hits;
};

struct SketchMeasureMarkerAddress final {
    sketch::SketchId sketch_id;
    sketch::MeasurePointRef point;

    friend bool operator==(
        const SketchMeasureMarkerAddress&,
        const SketchMeasureMarkerAddress&) = default;
};

struct SketchMeasureMarkerPointQueryResult final {
    bool completed{};
    std::vector<SketchMeasureMarkerAddress> hits;
};

struct SketchPreviewLine2D final {
    sketch::Point2 start;
    sketch::Point2 end;
    bool construction{false};

    [[nodiscard]] bool valid() const noexcept {
        return start.finite() &&
               end.finite() &&
               start != end;
    }
};

struct BodyTopologySelectionAddress final {
    viewer::BodyTopologyPresentationKind kind{
        viewer::BodyTopologyPresentationKind::face};
    std::uint64_t runtime_token_value{};
    viewer::BodyPresentationGeneration generation;

    [[nodiscard]] bool valid() const noexcept {
        return runtime_token_value != 0U &&
               generation.valid();
    }

    friend bool operator==(
        const BodyTopologySelectionAddress&,
        const BodyTopologySelectionAddress&) = default;
};

enum class SketchSupportInspectionCapability {
    not_applicable,
    supported,
    unsupported_non_planar,
    missing,
    ambiguous,
    unsupported,
};

struct BodyTopologyInspection final {
    viewer::BodyTopologyPresentationKind kind{
        viewer::BodyTopologyPresentationKind::face};
    part::BodyStageRef stage;
    bool diagnostic_prefix{};
    std::size_t selection_count{};
    part::TopologyAccountingClass accounting_class{
        part::TopologyAccountingClass::
            semantically_unsupported};
    kernel::ReferenceStatus strict_referenceability{
        kernel::ReferenceStatus::unsupported};
    kernel::ReferenceStatus carrier_referenceability{
        kernel::ReferenceStatus::unsupported};
    std::size_t semantic_candidate_count{};
    std::optional<part::FeatureId>
        producer_feature_id;
    std::optional<part::FeatureSurfaceAddress>
        surface_address;
    std::optional<part::FeatureCurveAddress>
        curve_address;
    std::optional<part::FeaturePointAddress>
        point_address;
    std::optional<kernel::SurfaceKind>
        surface_kind;
    std::optional<kernel::CurveKind>
        curve_kind;
    bool periodic_seam{};
    std::vector<part::FeatureSurfaceAddress>
        adjacent_surfaces;
    std::optional<kernel::Point3>
        provider_point;
    SketchSupportInspectionCapability
        sketch_support{
            SketchSupportInspectionCapability::
                not_applicable};

    [[nodiscard]] bool valid() const noexcept {
        return stage.valid() &&
               selection_count > 0U;
    }
};

struct TopologyKindSummary final {
    std::size_t total{};
    std::size_t referenceable{};
    std::size_t representation_artifact{};
    std::size_t semantically_unsupported{};
    std::size_t integrity_failure{};

    friend bool operator==(
        const TopologyKindSummary&,
        const TopologyKindSummary&) = default;
};

struct BodyTopologySummary final {
    part::BodyStageRef stage;
    TopologyKindSummary faces;
    TopologyKindSummary edges;
    TopologyKindSummary vertices;

    [[nodiscard]] bool valid() const noexcept {
        return stage.valid();
    }

    friend bool operator==(
        const BodyTopologySummary&,
        const BodyTopologySummary&) = default;
};

struct FeatureContributionSummary final {
    part::BodyStageRef current_stage;
    std::size_t faces{};
    std::size_t direct_edges{};
    std::size_t direct_vertices{};
    std::size_t boundary_edges{};
    std::size_t boundary_vertices{};
    std::size_t missing_outputs{};
    std::size_t ambiguous_outputs{};

    [[nodiscard]] bool valid() const noexcept {
        return current_stage.valid();
    }

    friend bool operator==(
        const FeatureContributionSummary&,
        const FeatureContributionSummary&) = default;
};

class PartViewportController final : public QObject {
public:
    using SelectionChangedHandler = std::function<void(
        const std::vector<core::BuiltinReferenceRole>&,
        std::optional<core::BuiltinReferenceRole>)>;

    using ProfileSelectionChangedHandler =
        std::function<void(
            const std::vector<part::ProfileId>&,
            std::optional<part::ProfileId>)>;

    using AxisSelectionChangedHandler =
        std::function<void(
            const std::vector<part::AxisId>&,
            std::optional<part::AxisId>)>;

    using DatumSelectionChangedHandler =
        std::function<void(
            const std::vector<part::DatumId>&,
            std::optional<part::DatumId>)>;

    using BodyTopologySelectionChangedHandler =
        std::function<void(
            std::optional<BodyTopologyInspection>)>;

    using SketchPointerHandler =
        std::function<void(const SketchPointerInput&)>;

    using PresentationStateChangedHandler =
        std::function<void(bool degraded)>;

    PartViewportController(
        PartDocumentTreeController& tree,
        viewer::IDocumentViewport* viewport,
        QObject* parent = nullptr);

    void setDocumentSession(
        application::DocumentSession* session);

    void setSolidModelingKernel(
        kernel::ISolidModelingKernel* modeling_kernel);

    void clear();
    void resetRuntimeState();
    void refreshDocumentTree();
    void refreshPresentation();

    [[nodiscard]] bool presentationDegraded() const noexcept {
        return presentation_degraded_;
    }

    void setPresentationStateChangedHandler(
        PresentationStateChangedHandler handler) {
        presentation_state_changed_handler_ =
            std::move(handler);
    }

    void setSketchEditSketch(
        std::optional<sketch::SketchId> sketch_id);

    // Read-only current evaluated Sketch, identical to the one used to
    // build the visible scene. No new OCCT projection on mouse moves.
    // Absent on stale revision, switched Document or failed scene.
    [[nodiscard]] const sketch::SketchModel*
    currentSketchInteractionModel() const;

    // Current stage-aware Profile resolution for Workbench Properties and
    // Feature pick gates. Matches the effective Sketch used by the Viewer;
    // linked authored seeds are never accepted as current geometry.
    [[nodiscard]] std::optional<part::ResolvedProfileRegion>
    currentProfileResolution(part::ProfileId profile_id) const;

    // C2: transient exact-source Project Geometry draft. The actual
    // Part and current Sketch remain unchanged until Workbench Finish.
    // A failed source/provider evaluation clears its entire preview.
    [[nodiscard]] bool setProjectedEdgeDraftPreview(
        const std::vector<part::MaterialEdgeReference>& sources,
        sketch::EntityRole role);

    [[nodiscard]] bool setSketchPreview(
        const std::vector<SketchPreviewLine2D>& lines);
    [[nodiscard]] bool setSketchCirclePreview(
        const sketch::CircleIntent& circle);
    [[nodiscard]] bool setSketchArcPreview(
        const sketch::ArcIntent& arc);
    [[nodiscard]] bool setSketchGeometryPreview(
        const sketch::DirectManipulationGeometry& geometry);
    void clearSketchPreview();

    [[nodiscard]] bool setSolidPreview(
        kernel::RuntimeSolidHandle solid,
        viewer::SolidPreviewTone tone);
    [[nodiscard]] bool setSolidPreview(
        const kernel::SolidPresentationMesh& mesh,
        viewer::SolidPreviewTone tone);
    [[nodiscard]] bool setSolidMaterialDeltaPreview(
        const std::optional<kernel::SolidPresentationMesh>& removed,
        const std::optional<kernel::SolidPresentationMesh>& added);
    void clearSolidPreview();

    [[nodiscard]] bool setProfileDraftPreview(
        const std::optional<sketch::RegionCandidate2D>& region,
        bool show_boundary = false,
        viewer::ProfilePreviewTone tone =
            viewer::ProfilePreviewTone::additive,
        const std::optional<sketch::RegionCandidate2D>&
            emphasis_region = std::nullopt);
    void clearProfileDraftPreview();

    void setDatumPlaneDraftPreview(
        std::optional<kernel::Frame3> frame);

    [[nodiscard]] bool setSketchPrimaryPointerRouting(
        viewer::PrimaryPointerRouting routing);

    [[nodiscard]] bool setSketchCursorMode(
        viewer::ViewportCursorMode mode);

    [[nodiscard]] std::optional<
        viewer::ViewportPoint2>
    projectSketchPointToViewport(
        sketch::Point2 point) const;

    [[nodiscard]] SketchEntityPointQueryResult
    querySketchEntityAt(
        viewer::ViewportPoint2 point);

    [[nodiscard]] SketchGripPointQueryResult
    querySketchGripAt(
        viewer::ViewportPoint2 point);

    [[nodiscard]] SketchMeasureMarkerPointQueryResult
    querySketchMeasureMarkersAt(
        viewer::ViewportPoint2 point);

    [[nodiscard]] bool projectSketchMeasurePresentation(
        const std::vector<sketch::ResolvedMeasurePoint>& catalog,
        const std::vector<sketch::MeasurePointRef>& selected,
        const std::optional<sketch::RelationalMeasurementCue>& cue);

    void clearSketchMeasurePresentation();

    [[nodiscard]] bool
    projectSketchSnapInferencePresentation(
        const std::optional<sketch::SnapCandidate>&
            current,
        const sketch::TrackingAnchorState&
            anchors,
        bool show_anchors,
        const std::vector<sketch::InferenceGuide>&
            active_guides = {},
        std::optional<sketch::Point2>
            inference_point = std::nullopt,
        bool guide_intersection = false,
        std::optional<sketch::LineExtensionRay>
            extension_ray = std::nullopt,
        std::optional<sketch::Point2>
            extension_point = std::nullopt);

    void clearSketchSnapInferencePresentation();

    [[nodiscard]] SketchEntityRectangleQueryResult
    querySketchEntities(
        const viewer::ViewportRect2& rectangle,
        viewer::SketchRectangleSelectionRule rule);

    [[nodiscard]] bool setSketchSelectionBoxOverlay(
        const viewer::SketchSelectionBoxOverlay& overlay);

    void clearSketchSelectionBoxOverlay();

    [[nodiscard]] bool setSketchDynamicInputOverlay(
        const viewer::SketchDynamicInputOverlay& overlay);

    void clearSketchDynamicInputOverlay();

    void setSketchPointerHandler(
        SketchPointerHandler handler) {
        sketch_pointer_handler_ =
            std::move(handler);
    }

    [[nodiscard]] std::optional<SketchEntityAddress>
    sketchEntityFor(
        viewer::PresentationToken token) const;

    [[nodiscard]] std::optional<viewer::PresentationToken>
    sketchPresentationFor(
        sketch::EntityId entity_id) const;

    [[nodiscard]] std::optional<part::ProfileId>
    profileFor(viewer::PresentationToken token) const;
    [[nodiscard]] std::optional<viewer::PresentationToken>
    profilePresentationFor(part::ProfileId profile_id) const;

    [[nodiscard]] std::optional<part::AxisId>
    axisFor(viewer::PresentationToken token) const;
    [[nodiscard]] std::optional<viewer::PresentationToken>
    axisPresentationFor(part::AxisId axis_id) const;

    [[nodiscard]] std::optional<part::DatumId>
    datumFor(viewer::PresentationToken token) const;
    [[nodiscard]] std::optional<viewer::PresentationToken>
    datumPresentationFor(part::DatumId datum_id) const;

    void setProfileSelectionFromTree(
        const std::vector<part::ProfileId>& selected,
        std::optional<part::ProfileId> primary);

    void setAxisSelectionFromTree(
        const std::vector<part::AxisId>& selected,
        std::optional<part::AxisId> primary);

    void setTransientProfileReveal(
        std::optional<part::ProfileId> profile_id);
    void setTransientProfilePresentationOverride(
        std::optional<part::ProfileId> reveal_profile_id,
        std::optional<part::ProfileId> hide_profile_id);

    // PM-04D: presentation-only source-Axis cue for an active Revolve draft.
    // This never changes authored Origin/Axis visibility or semantic selection.
    void setTransientAxisEmphasis(
        std::optional<part::AxisReference> axis);

    [[nodiscard]] bool projectSketchEntitySelection(
        const std::vector<sketch::EntityId>& selected,
        std::optional<sketch::EntityId> primary);

    [[nodiscard]] bool projectSketchInteraction(
        const std::vector<sketch::EntityId>& selected,
        std::optional<sketch::EntityId> hovered_entity,
        std::optional<sketch::LineGripRef> hovered_grip,
        std::optional<sketch::LineGripRef> active_grip,
        bool grips_visible);

    void setSelectionChangedHandler(
        SelectionChangedHandler handler) {
        selection_changed_handler_ = std::move(handler);
    }

    void setProfileSelectionChangedHandler(
        ProfileSelectionChangedHandler handler) {
        profile_selection_changed_handler_ =
            std::move(handler);
    }

    void setAxisSelectionChangedHandler(
        AxisSelectionChangedHandler handler) {
        axis_selection_changed_handler_ =
            std::move(handler);
    }

    void setDatumSelectionChangedHandler(
        DatumSelectionChangedHandler handler) {
        datum_selection_changed_handler_ =
            std::move(handler);
    }

    void setBodyTopologySelectionChangedHandler(
        BodyTopologySelectionChangedHandler handler) {
        body_topology_selection_changed_handler_ =
            std::move(handler);
    }

    void setBodyTopologyFacePickOnly(bool enabled);
    void setBodyTopologyEdgeDraftMode(bool enabled);
    [[nodiscard]] bool setBodyTopologyToolStage(
        std::optional<part::BodyStageRef> stage);
    [[nodiscard]] std::optional<
        std::vector<part::MaterialEdgeReference>>
    restoreMaterialEdgeToolSelection(
        const std::vector<
            part::MaterialEdgeReference>& references);
    void clearBodyTopologyToolSelection();
    [[nodiscard]] bool
    removePrimaryBodyTopologyToolSelection();

    [[nodiscard]] std::optional<
        std::vector<part::MaterialEdgeReference>>
    selectedMaterialEdgeReferences() const;

    // PG-01D: read one exact generation-bound bounded Face from the
    // current scene. Wires are transient, never authored Face identity.
    [[nodiscard]] part::MaterialFaceBoundaryAdmission
    selectedMaterialFaceBoundaryAdmission() const;

    // Recheck a previously staged transient Face using the *current*
    // scene lease; the argument is never persisted or used as identity.
    [[nodiscard]] part::MaterialFaceBoundaryAdmission
    inspectCurrentMaterialFaceBoundary(
        const BodyTopologySelectionAddress& source) const;

    // Classify each strict Edge's current geometric projection separately
    // so only geometric Unsupported may be skipped from a Face batch.
    [[nodiscard]] part::ProjectedSketchSourceStatus
    currentMaterialEdgeProjectionStatus(
        const part::MaterialEdgeReference& source) const;

    [[nodiscard]] std::optional<core::BuiltinReferenceRole>
    primarySelection() const;

    [[nodiscard]] std::vector<BodyTopologySelectionAddress>
    bodyTopologySelection() const;

    [[nodiscard]] std::optional<BodyTopologySelectionAddress>
    primaryBodyTopologySelection() const;

    [[nodiscard]] std::optional<BodyTopologyInspection>
    primaryBodyTopologyInspection() const;

    [[nodiscard]] std::optional<BodyTopologySummary>
    bodyTopologySummary() const;

    [[nodiscard]] std::optional<FeatureContributionSummary>
    featureContributionSummary(
        part::FeatureId feature_id) const;

    [[nodiscard]] viewer::ViewStyle
    viewStyle() const noexcept;

    [[nodiscard]] bool setViewStyle(
        viewer::ViewStyle style);

    void setFeatureContributionSelection(
        std::optional<part::FeatureId> feature_id);
    void setFeatureContributionHover(
        std::optional<part::FeatureId> feature_id);

private:
    struct SemanticSelection final {
        std::vector<core::BuiltinReferenceRole> selected;
        std::optional<core::BuiltinReferenceRole> primary;
        std::vector<part::ProfileId> profiles;
        std::optional<part::ProfileId> primary_profile;
        std::vector<part::DatumId> datums;
        std::optional<part::DatumId> primary_datum;
        std::vector<part::AxisId> axes;
        std::optional<part::AxisId> primary_axis;
        std::vector<viewer::PresentationToken>
            body_topology;
        std::optional<viewer::PresentationToken>
            primary_body_topology;
        viewer::BodyPresentationGeneration
            body_topology_generation;
    };

    struct BodyTopologyBinding final {
        viewer::BodyTopologyPresentationKind kind{
            viewer::BodyTopologyPresentationKind::face};
        std::uint64_t runtime_token_value{};
        viewer::BodyPresentationGeneration generation;

        [[nodiscard]] bool valid() const noexcept {
            return runtime_token_value != 0U &&
                   generation.valid();
        }
    };

    struct BodyTopologyCandidateStack final {
        viewer::BodyPresentationGeneration generation;
        viewer::ViewportPoint2 anchor;
        std::vector<viewer::BodyTopologyPickCandidate>
            candidates;
        std::size_t active_index{};

        [[nodiscard]] bool valid() const noexcept {
            return generation.valid() &&
                   anchor.valid() &&
                   !candidates.empty() &&
                   active_index < candidates.size();
        }
    };

    [[nodiscard]] static viewer::PresentationToken tokenFor(
        core::BuiltinReferenceRole role) noexcept;

    [[nodiscard]] static std::optional<core::BuiltinReferenceRole>
    roleFor(viewer::PresentationToken token) noexcept;

    [[nodiscard]] const part::PartSketch*
    activeSketch() const noexcept;

    [[nodiscard]] std::optional<part::SketchPlacement>
    resolvedPlacementForSketch(
        const part::PartSketch& sketch) const noexcept;

    [[nodiscard]] std::optional<viewer::Point3>
    sketchPointToWorld(
        const part::PartSketch& sketch,
        sketch::Point2 point) const noexcept;

    [[nodiscard]] std::optional<viewer::ReferenceScene>
    buildReferenceScene();

    [[nodiscard]] std::optional<viewer::BodyScene>
    buildBodyScene();

    // PG-01C: disposable same-revision Sketch geometry for drawing and
    // sampling. Missing projection provider/evaluation strips linked seeds,
    // preserving unaffected unlinked entities without touching Part intent.
    [[nodiscard]] sketch::SketchModel
    effectiveSketchModelForPresentation(
        const part::PartSketch& hosted) const;

    [[nodiscard]] std::optional<viewer::SketchScene>
    buildSketchScene();

    [[nodiscard]] std::optional<viewer::ProfileScene>
    buildProfileScene();

    [[nodiscard]] std::optional<viewer::ProfileRegionPresentation>
    buildProfileRegionPresentation(
        const part::PartSketch& source,
        const sketch::SketchModel& evaluated_model,
        const sketch::RegionCandidate2D& region) const;

    [[nodiscard]] std::optional<viewer::PresentationToken>
    allocatePresentationToken() noexcept;

    [[nodiscard]] SemanticSelection& activeSelection();
    [[nodiscard]] const SemanticSelection* activeSelection() const;

    void onTreeSelection(
        const std::vector<core::BuiltinReferenceRole>& selected,
        std::optional<core::BuiltinReferenceRole> primary);

    void onViewportIntent(
        const viewer::SelectionIntent& intent);

    void onBodyTopologyIntent(
        const viewer::BodyTopologyPickQueryResult& query,
        viewer::SelectionIntentMode mode);

    void onBodyTopologyPreselectionIntent(
        const viewer::BodyTopologyPickQueryResult& query,
        viewer::ViewportPoint2 point);

    void onBodyTopologyCycleIntent(bool reverse);

    [[nodiscard]] std::vector<
        viewer::BodyTopologyPickCandidate>
    rankedBodyTopologyCandidates(
        const viewer::BodyTopologyPickQueryResult& query) const;

    [[nodiscard]] bool bodyTopologyQueryCurrent(
        const viewer::BodyTopologyPickQueryResult& query) const noexcept;

    void applyBodyTopologyPreselection();
    void clearBodyTopologyPreselection();

    [[nodiscard]] std::optional<BodyTopologySelectionAddress>
    bodyTopologyAddressFor(
        viewer::PresentationToken token) const;

    [[nodiscard]] bool bodyTopologyOrdinaryPickable(
        const BodyTopologyBinding& binding) const;

    void clearBodyTopologySelection();

    void onSpatialPointer(
        const viewer::SpatialPointerEvent& event);

    [[nodiscard]] std::optional<viewer::PresentationToken>
    bodyPresentationTokenFor(
        viewer::BodyTopologyPresentationKind kind,
        std::uint64_t runtime_token_value) const;

    [[nodiscard]] std::vector<viewer::PresentationToken>
    featureContributionTokens(
        part::FeatureId feature_id) const;

    [[nodiscard]] bool applyFeatureContributionOverlay();

    void applySelectionToSurfaces();
    void applySketchViewportMode();
    void notifySelectionChanged();
    void setPresentationDegraded(bool degraded);

    PartDocumentTreeController* tree_{};
    viewer::IDocumentViewport* viewport_{};
    application::DocumentSession* session_{};
    kernel::ISolidModelingKernel*
        solid_modeling_kernel_{};
    std::optional<core::DocumentRevision>
        body_scene_revision_;
    std::optional<viewer::BodyScene>
        body_scene_cache_;
    std::optional<part::PartEvaluation>
        part_evaluation_cache_;
    std::optional<part::DatumEvaluation>
        datum_evaluation_cache_;
    std::optional<kernel::Frame3>
        datum_plane_draft_preview_frame_;
    std::optional<part::BodyStageTopologyCatalog>
        body_topology_catalog_cache_;
    // Runtime-only exact-stage catalogs from the same current PartEvaluation.
    // Final-stage catalog above remains authoritative for Body picking.
    std::vector<part::BodyStageTopologyCatalog>
        body_stage_topology_catalogs_cache_;
    std::unordered_map<
        std::uint64_t,
        BodyTopologyBinding>
        body_topology_bindings_;
    std::optional<BodyTopologyCandidateStack>
        body_topology_candidate_stack_;
    bool body_topology_face_pick_only_{false};
    bool body_topology_edge_draft_mode_{false};
    std::optional<part::BodyStageRef>
        body_topology_tool_stage_;
    std::uint64_t next_body_scene_generation_{1U};
    viewer::ViewStyle view_style_{
        viewer::ViewStyle::shaded};
    std::optional<part::FeatureId>
        selected_feature_contribution_;
    std::optional<part::FeatureId>
        hovered_feature_contribution_;
    std::optional<sketch::SketchId>
        sketch_edit_id_;
    viewer::PrimaryPointerRouting
        sketch_primary_pointer_routing_{
            viewer::PrimaryPointerRouting::
                presentation_selection};
    viewer::ViewportCursorMode
        sketch_cursor_mode_{
            viewer::ViewportCursorMode::
                select_pick_box};

    std::unordered_map<std::string, SemanticSelection>
        selections_;
    std::unordered_map<
        std::uint64_t,
        SketchEntityAddress>
        sketch_entity_bindings_;
    struct CurrentSketchModelSnapshot final {
        core::DocumentId document_id;
        sketch::SketchId sketch_id;
        core::DocumentRevision revision;
        sketch::SketchModel model;
    };
    std::optional<CurrentSketchModelSnapshot>
        current_sketch_model_snapshot_;
    std::unordered_map<
        std::uint64_t,
        part::ProfileId>
        profile_bindings_;
    std::unordered_map<
        std::uint64_t,
        part::DatumId>
        datum_bindings_;
    std::unordered_map<
        std::uint64_t,
        part::AxisId>
        axis_bindings_;
    std::optional<part::ProfileId>
        transient_profile_reveal_;
    std::optional<part::ProfileId>
        transient_profile_hide_;
    std::optional<part::AxisReference>
        transient_axis_emphasis_;
    std::uint64_t next_presentation_token_{
        0x10000U};
    bool sketch_grip_projection_valid_{};
    bool projected_grips_visible_{};
    std::vector<sketch::EntityId>
        projected_grip_selection_;
    bool presentation_degraded_{};

    SelectionChangedHandler selection_changed_handler_;
    ProfileSelectionChangedHandler
        profile_selection_changed_handler_;
    AxisSelectionChangedHandler
        axis_selection_changed_handler_;
    DatumSelectionChangedHandler
        datum_selection_changed_handler_;
    BodyTopologySelectionChangedHandler
        body_topology_selection_changed_handler_;
    SketchPointerHandler sketch_pointer_handler_;
    PresentationStateChangedHandler
        presentation_state_changed_handler_;
};

} // namespace simplesolid2::ui
