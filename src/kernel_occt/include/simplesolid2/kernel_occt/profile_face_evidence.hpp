#pragma once

#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/edge_projection.hpp>

#include <cstddef>
#include <simplesolid2/kernel/profile_input.hpp>

namespace simplesolid2::kernel_occt {

// PM-00A evidence-only OCCT adapter. Provider topology and handles remain
// private to the implementation; callers receive neutral evidence only.
[[nodiscard]] kernel::ShapeEvidence
buildProfileFaceEvidence(
    const kernel::PlanarProfileInput& input) noexcept;

[[nodiscard]] kernel::ExtrudeEvidence
buildProfileExtrudeEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept;

[[nodiscard]] kernel::BodyTopologyInventoryEvidence
buildExtrudeTopologyInventoryEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept;

[[nodiscard]] kernel::ExtrudeSurfaceCarrierEvidence
buildExtrudeSurfaceCarrierEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept;

[[nodiscard]] kernel::SurfaceBooleanLineageEvidence
buildSurfaceBooleanLineageEvidence(
    kernel::SurfaceBooleanProbeScenario scenario) noexcept;

[[nodiscard]] kernel::SurfaceDeleteRecreateEvidence
buildSurfaceDeleteRecreateEvidence() noexcept;

[[nodiscard]] kernel::CutExposedSurfaceEvidence
buildCutExposedSurfaceEvidence() noexcept;

[[nodiscard]] kernel::ExtrudeEdgeOntologyEvidence
buildExtrudeEdgeOntologyEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept;

[[nodiscard]] kernel::EdgeBooleanLineageEvidence
buildEdgeBooleanLineageEvidence(
    kernel::EdgeBooleanProbeScenario scenario) noexcept;

[[nodiscard]] kernel::BooleanIntersectionEdgeEvidence
buildBooleanIntersectionEdgeEvidence() noexcept;

[[nodiscard]] kernel::SurfacePairBranchEvidence
buildSurfacePairBranchEvidence() noexcept;

[[nodiscard]] kernel::ExtrudeVertexOntologyEvidence
buildExtrudeVertexOntologyEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept;

[[nodiscard]] kernel::VertexDimensionEditEvidence
buildVertexDimensionEditEvidence() noexcept;

[[nodiscard]] kernel::VertexDeletionGenerationEvidence
buildVertexDeletionGenerationEvidence() noexcept;

[[nodiscard]] kernel::VertexSamePointReplacementEvidence
buildVertexSamePointReplacementEvidence() noexcept;

[[nodiscard]] kernel::MultiStageTopologyAccountingEvidence
buildMultiStageTopologyAccountingEvidence() noexcept;

[[nodiscard]] kernel::GeometrySimilarityDecoyEvidence
buildGeometrySimilarityDecoyEvidence() noexcept;

[[nodiscard]] kernel::ProspectiveSketchSupportEvidence
buildProspectiveSketchSupportEvidence() noexcept;


[[nodiscard]] kernel::FullRevolveEvidence
buildProfileFullRevolveEvidence(
    const kernel::PlanarProfileInput& input,
    kernel::Point2 axis_origin,
    kernel::Point2 axis_direction) noexcept;

[[nodiscard]] kernel::EdgeSplitHistoryEvidence
buildEdgeSplitHistoryEvidence(
    kernel::EdgeSplitProbeScenario scenario) noexcept;

[[nodiscard]] kernel::FaceMergeHistoryEvidence
buildFaceMergeHistoryEvidence(
    kernel::FaceMergeProbeScenario scenario) noexcept;


[[nodiscard]] kernel::MultiStageLineageEvidence
buildMultiStageLineageEvidence(
    kernel::MultiStageProbeScenario scenario,
    double extrusion_height) noexcept;

// OCCT-specific negative scenario lives in the already existing evidence
// adapter (which owns the OCCT include path). The actual test target remains
// provider-neutral and does not gain an OCCT header/CMake dependency.
struct EdgeProjectionExceptionEvidence final {
    kernel::EdgeProjectionResult occt_failure;
    kernel::EdgeProjectionResult unexpected_failure;
    kernel::EdgeProjectionResult success;
    std::size_t occt_invocations{};
};

[[nodiscard]] EdgeProjectionExceptionEvidence
buildEdgeProjectionExceptionEvidence() noexcept;

} // namespace simplesolid2::kernel_occt
