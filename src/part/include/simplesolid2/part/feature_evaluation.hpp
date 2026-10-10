#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/reference_status.hpp>
#include <simplesolid2/kernel/face_boundary.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/feature_id.hpp>
#include <simplesolid2/part/semantic_topology_reference.hpp>
#include <simplesolid2/sketch/entity_id.hpp>

#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::part {

class PartDocument;
struct DatumEvaluation;

enum class FeatureEvaluationStatus {
    up_to_date,
    failed,
    blocked,
    suppressed,
};

enum class FeatureEvaluationDiagnosticCode {
    none,
    missing_profile,
    profile_unavailable,
    unresolved_profile,
    sketch_support_missing,
    sketch_support_ambiguous,
    sketch_support_unsupported,
    missing_axis,
    axis_unavailable,
    axis_not_in_profile_plane,
    profile_crosses_axis,
    missing_upstream_body,
    upstream_unavailable,
    kernel_invalid_input,
    kernel_provider_mismatch,
    kernel_provider_failure,
    invalid_brep,
    detached_add,
    no_effect,
    empty_result,
    multi_solid,
    topology_integrity_failure,
    edge_reference_missing,
    edge_reference_ambiguous,
    edge_reference_unsupported,
};

enum class BodyEvaluationStatus {
    empty,
    up_to_date,
    unavailable,
};

enum class TopologyAccountingClass {
    referenceable,
    known_representation_artifact,
    semantically_unsupported,
    integrity_failure,
};

enum class FeatureFaceRoleKind {
    profile_cap,
    extent_cap,
    negative_cap,
    positive_cap,
    side,
    revolve_start_cap,
    revolve_end_cap,
    revolve_side,
};

struct FeatureFaceAddress final {
    FeatureId producer_feature_id;
    FeatureFaceRoleKind role{
        FeatureFaceRoleKind::side};
    std::optional<sketch::EntityId>
        source_entity;
    std::uint32_t loop_index{};
    std::uint32_t use_index{};
    bool hole{false};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureFaceAddress&,
        const FeatureFaceAddress&) = default;
};

struct FeatureFaceResolution final {
    FeatureFaceAddress address;
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_count{};
    // Runtime-only bridge to the current provider result. Never serialized.
    std::optional<kernel::RuntimeFaceToken>
        runtime_token;

    friend bool operator==(
        const FeatureFaceResolution&,
        const FeatureFaceResolution&) = default;
};

struct FeatureSurfaceResolution final {
    FeatureSurfaceAddress address;
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    kernel::ReferenceStatus strict_face_status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_face_count{};
    kernel::SurfaceKind surface_kind{
        kernel::SurfaceKind::other};
    std::optional<kernel::Frame3>
        canonical_frame;
    // Runtime-only semantic-carrier bridge. Present only while Surface is
    // Resolved in the current provider generation; never serialized.
    std::optional<kernel::RuntimeSurfaceToken>
        runtime_token;
    // Current bounded Face realizations of this carrier. May contain >1 token
    // while Surface remains Resolved and strict Face becomes Ambiguous.
    std::vector<kernel::RuntimeFaceToken>
        current_faces;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureSurfaceResolution&,
        const FeatureSurfaceResolution&) = default;
};

struct FeatureCurveResolution final {
    FeatureCurveAddress address;
    // Semantic carrier status. An inherited Curve may remain Resolved while
    // its bounded Edge realization splits into several current Edges.
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    // Strict bounded Edge selector status for the current stage.
    kernel::ReferenceStatus strict_edge_status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_edge_count{};
    kernel::CurveKind curve_kind{
        kernel::CurveKind::other};
    std::vector<kernel::RuntimeEdgeToken>
        current_edges;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureCurveResolution&,
        const FeatureCurveResolution&) = default;
};

struct FeaturePointResolution final {
    FeaturePointAddress address;
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::size_t candidate_vertex_count{};
    std::vector<kernel::RuntimeVertexToken>
        current_vertices;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeaturePointResolution&,
        const FeaturePointResolution&) = default;
};

struct BodyFaceTopologyRecord final {
    kernel::RuntimeFaceToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    // Strict bounded Face meaning is present only when that semantic Face is
    // singularly Resolved at this stage.
    std::optional<FeatureFaceAddress>
        semantic_address;
    // Carrier candidates are distinct from strict Face identity. A single
    // Face normally maps to one semantic Surface; >1 is an explicit
    // Ambiguous carrier situation, never provider-order winner selection.
    std::vector<FeatureSurfaceAddress>
        surface_candidates;
    // Runtime-only Feature contribution membership. This is deliberately
    // separate from semantic Surface ownership so an Add may continue an
    // inherited carrier without losing truthful Current Feature Contribution.
    std::vector<FeatureId>
        contributing_features;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyFaceTopologyRecord&,
        const BodyFaceTopologyRecord&) = default;
};

struct BodyEdgeTopologyRecord final {
    kernel::RuntimeEdgeToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    kernel::ReferenceStatus referenceability{
        kernel::ReferenceStatus::unsupported};
    kernel::CurveKind curve_kind{
        kernel::CurveKind::other};
    bool periodic_seam{false};
    bool representation_partition{false};
    std::vector<FeatureCurveAddress>
        curve_candidates;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyEdgeTopologyRecord&,
        const BodyEdgeTopologyRecord&) = default;
};

struct BodyVertexTopologyRecord final {
    kernel::RuntimeVertexToken runtime_token;
    TopologyAccountingClass accounting_class{
        TopologyAccountingClass::semantically_unsupported};
    kernel::ReferenceStatus referenceability{
        kernel::ReferenceStatus::unsupported};
    std::vector<FeaturePointAddress>
        point_candidates;
    // Runtime-only material Edge incidence. This is derived from the current
    // provider stage solely to resolve semantic Point-pair branch
    // discriminators. It is never authored or serialized.
    std::vector<kernel::RuntimeEdgeToken>
        incident_material_edges;
    std::size_t incident_material_edge_count{};
    // Current provider XYZ is diagnostic only, never identity.
    std::optional<kernel::Point3>
        provider_point;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BodyVertexTopologyRecord&,
        const BodyVertexTopologyRecord&) = default;
};

struct BodyStageTopologyCatalog final {
    BodyStageRef stage;
    std::vector<BodyFaceTopologyRecord> faces;
    std::vector<BodyEdgeTopologyRecord> edges;
    std::vector<BodyVertexTopologyRecord> vertices;
    std::vector<FeatureSurfaceResolution>
        surfaces;
    std::vector<FeatureCurveResolution>
        curves;
    std::vector<FeaturePointResolution>
        points;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] bool complete() const noexcept;

    friend bool operator==(
        const BodyStageTopologyCatalog&,
        const BodyStageTopologyCatalog&) = default;
};

struct MaterialEdgeResolution final {
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::vector<kernel::RuntimeEdgeToken>
        current_edges;

    [[nodiscard]] bool resolved() const noexcept {
        return status ==
                   kernel::ReferenceStatus::resolved &&
               current_edges.size() == 1U &&
               current_edges.front().valid();
    }
};

struct MaterialEdgeAuthoringResult final {
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::optional<MaterialEdgeReference> reference;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
                   kernel::ReferenceStatus::resolved &&
               reference.has_value() &&
               reference->valid();
    }
};

// Resolve one durable strict material Edge only in its declared Body stage.
// Invalid/mismatched runtime context returns nullopt. Missing/Ambiguous/
// Unsupported are semantic resolution outcomes and never trigger geometry,
// provider-order, or nearest-match fallback.
[[nodiscard]] std::optional<MaterialEdgeResolution>
resolveMaterialEdgeReference(
    const MaterialEdgeReference& reference,
    const BodyStageTopologyCatalog& catalog);

// Convert one current runtime material Edge into durable semantic intent.
// Multi-branch Curve families require two defensible semantic endpoint Points;
// otherwise authoring fails closed as Unsupported.
[[nodiscard]] MaterialEdgeAuthoringResult
authorMaterialEdgeReference(
    const BodyStageTopologyCatalog& catalog,
    kernel::RuntimeEdgeToken edge);

struct FeatureContribution final {
    // Set-valued current contribution query. These runtime tokens identify
    // current Body realizations only; no member is persisted.
    std::vector<kernel::RuntimeFaceToken> faces;
    std::vector<kernel::RuntimeEdgeToken> direct_edges;
    std::vector<kernel::RuntimeVertexToken> direct_vertices;
    // Presentation envelope around carrier contribution. Boundary members are
    // not re-owned by the Feature merely because they bound its Faces.
    std::vector<kernel::RuntimeEdgeToken> boundary_edges;
    std::vector<kernel::RuntimeVertexToken> boundary_vertices;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const FeatureContribution&,
        const FeatureContribution&) = default;
};

struct FeatureEvaluation final {
    FeatureId feature_id;
    FeatureEvaluationStatus status{
        FeatureEvaluationStatus::blocked};
    FeatureEvaluationDiagnosticCode diagnostic{
        FeatureEvaluationDiagnosticCode::none};
    std::optional<kernel::SolidModelingStatus>
        kernel_status;
    std::vector<FeatureFaceResolution>
        produced_faces;
    std::vector<FeatureSurfaceResolution>
        produced_surfaces;
    std::vector<FeatureCurveResolution>
        produced_curves;
    std::vector<FeaturePointResolution>
        produced_points;
    // Same-revision runtime result for this exact successful Feature stage.
    // Retained only so stage-scoped topology tokens remain resolvable during
    // the current evaluation; never serialized or treated as final Body truth.
    kernel::RuntimeSolidHandle result_solid;
    // Complete runtime-only topology for this successful Feature stage.
    // Empty for Failed/Blocked/Suppressed Features. Never serialized.
    std::optional<BodyStageTopologyCatalog>
        result_topology;
    // PM-05 structured input diagnostics. Appended to preserve existing
    // positional aggregate initialization of derived FeatureEvaluation data.
    std::optional<std::size_t>
        failing_edge_input_index;
    std::optional<kernel::ReferenceStatus>
        edge_reference_status;
};

// PG-01D D1 read-only stage-bound Face boundary admission. The Face is
// an ephemeral selection, never native Part intent. All durable members are
// existing individually authored MaterialEdgeReferences; a single invalid
// source Edge blocks the entire Face, never silently drops a loop member.
enum class MaterialFaceBoundaryStatus {
    resolved,
    invalid_stage,
    face_unavailable,
    face_not_strict,
    native_boundary_unavailable,
    material_edge_unavailable,
};

struct MaterialFaceBoundaryMember final {
    // Transient only; the material reference is the sole authored identity.
    kernel::RuntimeEdgeToken current_edge;
    MaterialEdgeReference reference;
    bool reversed{false};

    [[nodiscard]] bool valid() const noexcept {
        return current_edge.valid() && reference.valid();
    }
};

struct MaterialFaceBoundaryWire final {
    bool outer{false};
    std::vector<MaterialFaceBoundaryMember> edges;
};

struct MaterialFaceBoundaryAdmission final {
    MaterialFaceBoundaryStatus status{
        MaterialFaceBoundaryStatus::invalid_stage};
    std::optional<FeatureFaceAddress> bounded_face;
    std::vector<MaterialFaceBoundaryWire> wires;

    [[nodiscard]] bool ok() const noexcept {
        if (status != MaterialFaceBoundaryStatus::resolved ||
            !bounded_face || !bounded_face->valid() ||
            wires.empty()) {
            return false;
        }
        std::size_t outer = 0U;
        for (const auto& wire : wires) {
            if (wire.edges.empty()) return false;
            if (wire.outer) ++outer;
            for (const auto& edge : wire.edges) {
                if (!edge.valid()) return false;
            }
        }
        return outer == 1U;
    }
};

// Runtime FeatureEvaluation contains a same-revision paired Body and
// complete topology catalog for its exact stage. A Surface carrier alone
// is NOT a unique bounded Face and cannot pass this admission.
[[nodiscard]] MaterialFaceBoundaryAdmission
inspectMaterialFaceBoundary(
    const FeatureEvaluation& current_stage,
    kernel::RuntimeFaceToken picked_bounded_face,
    kernel::IFaceBoundaryQuery& provider);

// PG-01D Owner manual multi-Face extension: a clicked native Face is
// scoped only to the current Body generation. Unlike Planar Face above,
// it does not require a singular authored Face address, and a known
// representation seam/partition may occur inside its native wires.
// Exact *material Edge* references remain the only authorable output.
// Full native wire uses (including excluded artifacts) are retained for
// lossless same-revision Finish revalidation, never persisted.
struct SelectedFaceBoundaryMember final {
    kernel::FaceBoundaryEdgeUse native_use;
    std::optional<MaterialEdgeReference> material;
    bool excluded_nonmaterial{false};

    [[nodiscard]] bool valid() const noexcept {
        return native_use.valid() &&
            (material.has_value() != excluded_nonmaterial) &&
            (!material || material->valid());
    }

    friend bool operator==(
        const SelectedFaceBoundaryMember&,
        const SelectedFaceBoundaryMember&) = default;
};

struct SelectedFaceBoundaryWire final {
    bool outer{false};
    std::vector<SelectedFaceBoundaryMember> edges;

    friend bool operator==(
        const SelectedFaceBoundaryWire&,
        const SelectedFaceBoundaryWire&) = default;
};

// Diagnostic evidence from one rejected native Face-wire occurrence.
// Runtime-only, never stored as a stable material or Face identity.
enum class SelectedFaceBoundaryRejectKind {
    missing_catalog_edge,
    uncertified_material_edge,
    repeated_material_edge,
    invalid_member,
};

struct SelectedFaceBoundaryRejectDetail final {
    SelectedFaceBoundaryRejectKind kind{
        SelectedFaceBoundaryRejectKind::uncertified_material_edge};
    kernel::RuntimeEdgeToken edge;
    std::size_t wire_index{};
    std::size_t edge_index{};
    std::optional<TopologyAccountingClass> accounting_class;
    std::optional<kernel::ReferenceStatus> referenceability;
    kernel::CurveKind curve_kind{kernel::CurveKind::other};
    std::size_t curve_candidate_count{};
    bool periodic_seam{false};
    bool representation_partition{false};

    friend bool operator==(
        const SelectedFaceBoundaryRejectDetail&,
        const SelectedFaceBoundaryRejectDetail&) = default;
};

struct SelectedFaceBoundaryAdmission final {
    MaterialFaceBoundaryStatus status{
        MaterialFaceBoundaryStatus::invalid_stage};
    kernel::RuntimeFaceToken bounded_face;
    std::vector<SelectedFaceBoundaryWire> wires;
    std::optional<SelectedFaceBoundaryRejectDetail> rejected_edge;

    [[nodiscard]] bool ok() const noexcept {
        if (status != MaterialFaceBoundaryStatus::resolved ||
            !bounded_face.valid() || wires.empty()) {
            return false;
        }
        std::size_t outer_count = 0U;
        for (const auto& wire : wires) {
            if (wire.outer) ++outer_count;
            if (wire.edges.empty()) return false;
            for (const auto& item : wire.edges) {
                if (!item.valid()) return false;
            }
        }
        return outer_count == 1U;
    }

    friend bool operator==(
        const SelectedFaceBoundaryAdmission&,
        const SelectedFaceBoundaryAdmission&) = default;
};

[[nodiscard]] SelectedFaceBoundaryAdmission
inspectSelectedFaceBoundary(
    const FeatureEvaluation& current_stage,
    kernel::RuntimeFaceToken picked_bounded_face,
    kernel::IFaceBoundaryQuery& provider);

struct PartEvaluation final {
    core::DocumentRevision source_revision;
    BodyEvaluationStatus body_status{
        BodyEvaluationStatus::empty};
    // Final current Body truth. Null whenever body_status is unavailable.
    kernel::RuntimeSolidHandle body_solid;
    // Runtime-only current-revision result immediately before the first
    // Failed/Blocked active Feature. Presentation may use this prefix while
    // final Body truth remains unavailable. Never persisted, published as
    // semantic identity, or consumed by downstream evaluation.
    kernel::RuntimeSolidHandle resolved_prefix_solid;
    std::vector<FeatureEvaluation> features;
    // Complete final Body-stage topology truth. Empty when Body is unavailable
    // or Empty. This is derived runtime state and is never serialized.
    std::optional<BodyStageTopologyCatalog>
        current_topology;
    // Same-revision topology immediately before the first Failed/Blocked
    // active Feature. Diagnostic/presentation only, never final Body truth.
    std::optional<BodyStageTopologyCatalog>
        resolved_prefix_topology;
    // Semantic references resolved at the current final Body stage only.
    // Empty when Body is unavailable.
    std::vector<FeatureFaceResolution>
        current_face_references;
    // Semantic Surface carriers resolved at the current final Body stage.
    // Empty when Body is unavailable.
    std::vector<FeatureSurfaceResolution>
        current_surface_references;
    std::vector<FeatureCurveResolution>
        current_curve_references;
    std::vector<FeaturePointResolution>
        current_point_references;

    [[nodiscard]] const FeatureEvaluation*
    findFeature(FeatureId id) const noexcept;
};

enum class EdgeFeatureKernelInputStatus {
    resolved,
    missing_upstream_body,
    missing_edge,
    ambiguous_edge,
    unsupported_edge,
    invalid_input,
};

struct EdgeFeatureKernelInputResult final {
    EdgeFeatureKernelInputStatus status{
        EdgeFeatureKernelInputStatus::invalid_input};
    std::optional<kernel::EdgeFeatureInput>
        input;
    std::optional<std::size_t>
        failing_edge_input_index;
    std::optional<kernel::ReferenceStatus>
        reference_status;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
                   EdgeFeatureKernelInputStatus::resolved &&
               input.has_value() &&
               input->valid();
    }
};

// Materializes the complete explicit authored Edge set against one exact
// upstream Body stage. Any unresolved member blocks the whole operation and
// no provider call is permitted.
[[nodiscard]] EdgeFeatureKernelInputResult
resolveKernelEdgeFeatureInput(
    const FilletFeature& feature,
    const BodyStageTopologyCatalog*
        upstream_topology);

[[nodiscard]] EdgeFeatureKernelInputResult
resolveKernelEdgeFeatureInput(
    const ChamferFeature& feature,
    const BodyStageTopologyCatalog*
        upstream_topology);

enum class RevolveKernelInputStatus {
    resolved,
    missing_profile,
    profile_unavailable,
    missing_axis,
    axis_unavailable,
    axis_not_in_profile_plane,
    profile_crosses_axis,
    invalid_input,
};

struct RevolveKernelInputResult final {
    RevolveKernelInputStatus status{
        RevolveKernelInputStatus::invalid_input};
    std::optional<kernel::AngularRevolveInput>
        input;
    std::optional<BodyStageRef>
        required_axis_stage;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
                   RevolveKernelInputStatus::resolved &&
               input.has_value() &&
               input->valid();
    }
};

// Resolves current Profile support + AxisReference and applies the PM-04
// coplanarity/closed-half-plane admission contract. The supplied Part/Datum
// evaluations must be same-revision prefix evidence; no last-good geometry is
// reused. This is transient derived state shared by evaluation and later
// preview/draft adapters.
[[nodiscard]] RevolveKernelInputResult
resolveKernelRevolveInput(
    const PartDocument& document,
    const RevolveFeature& feature,
    const PartEvaluation* prefix_evaluation,
    const DatumEvaluation* datum_evaluation,
    // Optional strictly evaluated same-Sketch source: Region, input
    // curves and Revolve axis checks use the same current profile.
    const sketch::SketchModel* effective_sketch = nullptr);

// Builds the exact provider-neutral modeling input for one authored
// Extrude definition. This is transient derived data shared by evaluation
// and runtime preview; it is never persisted.
[[nodiscard]] std::optional<
    kernel::LinearExtrudeInput>
makeKernelExtrudeInput(
    const PartDocument& document,
    const ExtrudeFeature& feature,
    const BodyStageTopologyCatalog*
        support_topology = nullptr,
    const DatumEvaluation*
        datum_evaluation = nullptr,
    // Current disposable projection geometry. Preview and Kernel Profile
    // must NEVER reuse linked authored Sketch seeds as current truth.
    const sketch::SketchModel*
        effective_sketch = nullptr);

[[nodiscard]] FeatureContribution currentFeatureContribution(
    const BodyStageTopologyCatalog& catalog,
    FeatureId feature_id);

[[nodiscard]] PartEvaluation evaluatePart(
    const PartDocument& document,
    kernel::ISolidModelingKernel& modeling_kernel);

} // namespace simplesolid2::part
