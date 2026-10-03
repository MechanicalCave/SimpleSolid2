#pragma once

#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel/reference_status.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace simplesolid2::kernel {

enum class EvidenceStatus {
    ok,
    invalid_input,
    provider_failure,
    invalid_brep,
};

struct BoundaryLineageEvidence final {
    BoundaryUseProvenance provenance;
    std::size_t generated_edge_count{};

    friend bool operator==(
        const BoundaryLineageEvidence&,
        const BoundaryLineageEvidence&) = default;
};

struct ShapeEvidence final {
    EvidenceStatus status{EvidenceStatus::provider_failure};
    bool brep_valid{false};
    std::size_t solid_count{};
    std::size_t face_count{};
    std::size_t wire_count{};
    std::size_t edge_count{};
    std::vector<BoundaryLineageEvidence> boundary_lineage;

    [[nodiscard]] bool ok() const noexcept {
        return status == EvidenceStatus::ok && brep_valid;
    }

    friend bool operator==(
        const ShapeEvidence&,
        const ShapeEvidence&) = default;
};

enum class EvidenceTopologyAccountingClass {
    referenceable,
    known_representation_artifact,
    semantically_unsupported,
    integrity_failure,
};

// PM-02P evidence-only topology accounting. These records deliberately carry
// no provider handle, topology ordinal or durable selector meaning. Later
// checkpoints may enrich semantic classification after the inventory harness
// has proved that no current B-Rep subshape disappears from accounting.
struct EvidenceTopologyRecord final {
    EvidenceTopologyAccountingClass accounting_class{
        EvidenceTopologyAccountingClass::semantically_unsupported};

    friend bool operator==(
        const EvidenceTopologyRecord&,
        const EvidenceTopologyRecord&) = default;
};

struct EvidenceTopologyKindInventory final {
    // Explorer occurrences are recorded independently from the unique
    // provider subshape set used to populate the evidence catalog.
    std::size_t provider_occurrence_count{};
    std::size_t provider_unique_count{};
    std::vector<EvidenceTopologyRecord> catalog;

    [[nodiscard]] bool complete() const noexcept {
        return provider_unique_count == catalog.size();
    }

    friend bool operator==(
        const EvidenceTopologyKindInventory&,
        const EvidenceTopologyKindInventory&) = default;
};

struct BodyTopologyInventoryEvidence final {
    EvidenceStatus status{EvidenceStatus::provider_failure};
    bool brep_valid{false};
    std::size_t solid_count{};
    EvidenceTopologyKindInventory faces;
    EvidenceTopologyKindInventory edges;
    EvidenceTopologyKindInventory vertices;

    [[nodiscard]] bool complete() const noexcept {
        return status == EvidenceStatus::ok &&
               brep_valid &&
               solid_count == 1U &&
               faces.complete() &&
               edges.complete() &&
               vertices.complete();
    }

    friend bool operator==(
        const BodyTopologyInventoryEvidence&,
        const BodyTopologyInventoryEvidence&) = default;
};

enum class ExtrudeFaceRoleKind {
    start_cap,
    end_cap,
    side,
};

enum class FaceSurfaceKind {
    plane,
    cylinder,
    cone,
    sphere,
    torus,
    other,
};

enum class EvidenceSurfaceCarrierRoleKind {
    start_cap,
    end_cap,
    side,
};

// PM-02P.B evidence-only semantic carrier record. The role/provenance is the
// semantic claim; provider geometry classification corroborates that claim
// but does not define identity.
struct EvidenceSurfaceCarrierRecord final {
    EvidenceSurfaceCarrierRoleKind role{
        EvidenceSurfaceCarrierRoleKind::side};
    std::optional<BoundaryUseProvenance> provenance;
    ReferenceStatus status{ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    FaceSurfaceKind semantic_surface_kind{
        FaceSurfaceKind::other};
    std::optional<FaceSurfaceKind>
        provider_surface_kind;
    std::optional<Frame3> canonical_frame;

    friend bool operator==(
        const EvidenceSurfaceCarrierRecord&,
        const EvidenceSurfaceCarrierRecord&) = default;
};

struct ExtrudeSurfaceCarrierEvidence final {
    ShapeEvidence shape;
    BodyTopologyInventoryEvidence topology;
    EvidenceSurfaceCarrierRecord start_cap;
    EvidenceSurfaceCarrierRecord end_cap;
    std::vector<EvidenceSurfaceCarrierRecord> sides;
    std::size_t unique_claimed_face_count{};
    std::size_t unclaimed_face_count{};
    std::size_t multiply_claimed_face_count{};
    std::size_t claim_outside_body_count{};

    [[nodiscard]] bool completeFaceClaims() const noexcept {
        if (!(shape.ok() &&
              shape.solid_count == 1U &&
              topology.complete() &&
              topology.faces.provider_unique_count ==
                  unique_claimed_face_count &&
              unclaimed_face_count == 0U &&
              multiply_claimed_face_count == 0U &&
              claim_outside_body_count == 0U &&
              start_cap.status == ReferenceStatus::resolved &&
              end_cap.status == ReferenceStatus::resolved)) {
            return false;
        }

        for (const auto& side : sides) {
            if (side.status != ReferenceStatus::resolved) {
                return false;
            }
        }
        return true;
    }

    friend bool operator==(
        const ExtrudeSurfaceCarrierEvidence&,
        const ExtrudeSurfaceCarrierEvidence&) = default;
};

struct FaceGeometryDiagnostics final {
    FaceSurfaceKind surface_kind{
        FaceSurfaceKind::other};
    double area{};
    Point3 centroid;
    Point3 surface_axis;

    friend bool operator==(
        const FaceGeometryDiagnostics&,
        const FaceGeometryDiagnostics&) = default;
};

struct ExtrudeFaceEvidence final {
    ExtrudeFaceRoleKind role{ExtrudeFaceRoleKind::side};
    ReferenceStatus status{ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    std::optional<BoundaryUseProvenance> provenance;

    // Evidence diagnostics only. These counts expose where provider lineage
    // failed without becoming semantic identity or persistence.
    std::size_t basis_edge_match_count{};
    std::size_t generated_shape_count{};
    std::optional<FaceGeometryDiagnostics>
        geometry_diagnostics;

    friend bool operator==(
        const ExtrudeFaceEvidence&,
        const ExtrudeFaceEvidence&) = default;
};

struct ExtrudeEvidence final {
    ShapeEvidence shape;
    ExtrudeFaceEvidence start_cap{
        ExtrudeFaceRoleKind::start_cap,
        ReferenceStatus::unsupported,
        0U,
        std::nullopt};
    ExtrudeFaceEvidence end_cap{
        ExtrudeFaceRoleKind::end_cap,
        ReferenceStatus::unsupported,
        0U,
        std::nullopt};
    std::vector<ExtrudeFaceEvidence> sides;

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U &&
               start_cap.status ==
                   ReferenceStatus::resolved &&
               end_cap.status ==
                   ReferenceStatus::resolved;
    }

    friend bool operator==(
        const ExtrudeEvidence&,
        const ExtrudeEvidence&) = default;
};


// PM-00A E06 full-Revolve evidence. Axis input remains evidence-only and
// does not define durable Axis/Datum schema.
struct RevolveBoundaryFaceEvidence final {
    ReferenceStatus status{ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    BoundaryUseProvenance provenance;
    bool periodic_surface{false};
    std::size_t seam_edge_count{};
    double seam_total_length{};
    std::optional<FaceGeometryDiagnostics>
        geometry_diagnostics;

    friend bool operator==(
        const RevolveBoundaryFaceEvidence&,
        const RevolveBoundaryFaceEvidence&) = default;
};

struct FullRevolveEvidence final {
    ShapeEvidence shape;
    std::vector<RevolveBoundaryFaceEvidence>
        boundary_faces;

    // A provider-created periodic seam has no semantic source/provenance
    // under PM-00A and therefore cannot be promoted to Resolved.
    ReferenceStatus periodic_seam_reference_status{
        ReferenceStatus::unsupported};

    std::size_t provider_seam_edge_count{};
    double provider_seam_total_length{};

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U;
    }

    friend bool operator==(
        const FullRevolveEvidence&,
        const FullRevolveEvidence&) = default;
};

enum class EdgeSplitProbeScenario {
    middle_notch,
    remove_target,
};

enum class FaceMergeProbeScenario {
    overlapping_coplanar,
    asymmetric_history,
    absorbed_inner,
};

// PM-00A E03/E04 raw provider-history evidence. These values are transient
// observations only; they are not a persistent selector or identity format.
struct BooleanSubshapeHistoryEvidence final {
    std::size_t modified_count{};
    std::size_t generated_count{};
    bool deleted{false};
    bool unchanged_present{false};
    std::size_t unique_descendant_count{};

    friend bool operator==(
        const BooleanSubshapeHistoryEvidence&,
        const BooleanSubshapeHistoryEvidence&) = default;
};

enum class SurfaceBooleanProbeScenario {
    add_trim,
    cut_trim_and_expose,
    cut_split,
    cut_delete,
};

// PM-02P.B evidence-only distinction between one bounded Face realization and
// the semantic Surface carrier that may survive trimming/splitting. Nothing
// here is a durable selector or production topology API.
struct SurfaceBooleanLineageEvidence final {
    ShapeEvidence shape;
    BodyTopologyInventoryEvidence topology;

    BooleanSubshapeHistoryEvidence source_face_history;
    ReferenceStatus strict_face_status{
        ReferenceStatus::unsupported};
    ReferenceStatus surface_status{
        ReferenceStatus::unsupported};
    std::size_t surface_realization_count{};
    FaceSurfaceKind surface_kind{
        FaceSurfaceKind::other};
    bool all_surface_realizations_match_kind{false};
    std::optional<Frame3> source_frame;
    std::optional<Frame3> resolved_surface_frame;

    // Used by the Cut pocket scenario to prove that a semantic tool Surface
    // can become a current material Body boundary with its own canonical
    // frame. Unsupported means the scenario does not request this evidence.
    BooleanSubshapeHistoryEvidence tool_surface_history;
    ReferenceStatus tool_surface_status{
        ReferenceStatus::unsupported};
    std::size_t tool_surface_realization_count{};
    FaceSurfaceKind tool_surface_kind{
        FaceSurfaceKind::other};
    bool all_tool_realizations_match_kind{false};
    std::optional<Frame3> tool_source_frame;
    std::optional<Frame3> resolved_tool_surface_frame;

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U &&
               topology.complete();
    }

    friend bool operator==(
        const SurfaceBooleanLineageEvidence&,
        const SurfaceBooleanLineageEvidence&) = default;
};

enum class SurfaceBooleanProbeScenario {
    attached_add_trim,
    cut_trim,
    cut_split,
};

struct SurfaceBooleanLineageEvidence final {
    ShapeEvidence before_shape;
    ShapeEvidence after_shape;
    BodyTopologyInventoryEvidence after_topology;
    BooleanSubshapeHistoryEvidence source_history;
    ReferenceStatus strict_face_status{
        ReferenceStatus::unsupported};
    ReferenceStatus surface_status{
        ReferenceStatus::unsupported};
    std::size_t current_face_realization_count{};
    std::size_t planar_realization_count{};
    std::optional<Frame3> canonical_frame;

    [[nodiscard]] bool ok() const noexcept {
        return before_shape.ok() &&
               before_shape.solid_count == 1U &&
               after_shape.ok() &&
               after_shape.solid_count == 1U &&
               after_topology.complete();
    }

    friend bool operator==(
        const SurfaceBooleanLineageEvidence&,
        const SurfaceBooleanLineageEvidence&) = default;
};

struct SurfaceDeleteRecreateEvidence final {
    ShapeEvidence before_shape;
    ShapeEvidence after_delete_shape;
    ShapeEvidence after_recreate_shape;
    BooleanSubshapeHistoryEvidence delete_history;
    ReferenceStatus old_surface_after_delete{
        ReferenceStatus::unsupported};
    ReferenceStatus old_surface_after_recreate{
        ReferenceStatus::unsupported};
    ReferenceStatus replacement_surface_status{
        ReferenceStatus::unsupported};
    std::size_t replacement_face_count{};
    bool replacement_geometry_matches_old{false};
    std::optional<Frame3> old_canonical_frame;
    std::optional<Frame3> replacement_canonical_frame;

    [[nodiscard]] bool ok() const noexcept {
        return before_shape.ok() &&
               before_shape.solid_count == 1U &&
               after_delete_shape.ok() &&
               after_delete_shape.solid_count == 1U &&
               after_recreate_shape.ok() &&
               after_recreate_shape.solid_count == 1U;
    }

    friend bool operator==(
        const SurfaceDeleteRecreateEvidence&,
        const SurfaceDeleteRecreateEvidence&) = default;
};

struct CutExposedSurfaceEvidence final {
    ShapeEvidence result_shape;
    BodyTopologyInventoryEvidence result_topology;
    BooleanSubshapeHistoryEvidence tool_surface_history;
    ReferenceStatus strict_face_status{
        ReferenceStatus::unsupported};
    ReferenceStatus surface_status{
        ReferenceStatus::unsupported};
    std::size_t current_face_realization_count{};
    FaceSurfaceKind semantic_surface_kind{
        FaceSurfaceKind::other};
    std::optional<FaceSurfaceKind>
        provider_surface_kind;
    std::optional<BoundaryUseProvenance>
        provenance;
    std::optional<Frame3> canonical_frame;

    [[nodiscard]] bool ok() const noexcept {
        return result_shape.ok() &&
               result_shape.solid_count == 1U &&
               result_topology.complete();
    }

    friend bool operator==(
        const CutExposedSurfaceEvidence&,
        const CutExposedSurfaceEvidence&) = default;
};

struct EdgeSplitHistoryEvidence final {
    ShapeEvidence shape;
    BooleanSubshapeHistoryEvidence target;

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U;
    }

    friend bool operator==(
        const EdgeSplitHistoryEvidence&,
        const EdgeSplitHistoryEvidence&) = default;
};

struct FaceMergeHistoryEvidence final {
    ShapeEvidence shape;
    BooleanSubshapeHistoryEvidence first;
    BooleanSubshapeHistoryEvidence second;
    std::size_t shared_descendant_count{};

    [[nodiscard]] bool ok() const noexcept {
        return shape.ok() &&
               shape.solid_count == 1U;
    }

    friend bool operator==(
        const FaceMergeHistoryEvidence&,
        const FaceMergeHistoryEvidence&) = default;
};

enum class MultiStageProbeScenario {
    stable_fillet,
    remove_selected_face,
    upstream_thin_fillet_failure,
};

enum class EvidenceProducerStage {
    extrude_output,
    cut_output,
    downstream_output,
};

enum class EvidenceOperationOutcome {
    not_run,
    valid,
    geometric_failure,
};

struct StageReferenceEvidence final {
    EvidenceProducerStage stage{
        EvidenceProducerStage::extrude_output};
    ReferenceStatus status{
        ReferenceStatus::unsupported};
    std::size_t candidate_count{};

    friend bool operator==(
        const StageReferenceEvidence&,
        const StageReferenceEvidence&) = default;
};

struct MultiStageLineageEvidence final {
    ShapeEvidence extrude_shape;
    ShapeEvidence cut_shape;
    std::optional<ShapeEvidence> downstream_shape;

    StageReferenceEvidence at_extrude{
        EvidenceProducerStage::extrude_output,
        ReferenceStatus::unsupported,
        0U};
    StageReferenceEvidence at_cut{
        EvidenceProducerStage::cut_output,
        ReferenceStatus::unsupported,
        0U};
    StageReferenceEvidence at_downstream{
        EvidenceProducerStage::downstream_output,
        ReferenceStatus::unsupported,
        0U};

    BooleanSubshapeHistoryEvidence extrude_to_cut;
    BooleanSubshapeHistoryEvidence cut_to_downstream;

    std::size_t downstream_input_edge_candidate_count{};
    EvidenceOperationOutcome downstream_outcome{
        EvidenceOperationOutcome::not_run};

    [[nodiscard]] bool base_ok() const noexcept {
        return extrude_shape.ok() &&
               extrude_shape.solid_count == 1U &&
               cut_shape.ok() &&
               cut_shape.solid_count == 1U;
    }

    friend bool operator==(
        const MultiStageLineageEvidence&,
        const MultiStageLineageEvidence&) = default;
};

} // namespace simplesolid2::kernel
