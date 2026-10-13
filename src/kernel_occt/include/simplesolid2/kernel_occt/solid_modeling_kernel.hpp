#pragma once

#include <simplesolid2/kernel/edge_projection.hpp>
#include <simplesolid2/kernel/exact_bounded_edge_geometry.hpp>
#include <simplesolid2/kernel/face_boundary.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>

namespace simplesolid2::kernel_occt {

// The SAME exception-to-typed-failure boundary is used by the production
// provider query and the Q0 native kernel lifecycle witness.
namespace detail {
template <typename Calculate>
[[nodiscard]] kernel::ExactEdge3dResult
guardExactBoundedEdge3dResult(Calculate&& calculate) noexcept {
    try {
        return std::forward<Calculate>(calculate)();
    } catch (...) {
        return {kernel::ExactEdge3dStatus::kernel_failure, std::nullopt};
    }
}
} // namespace detail

// Production OCCT provider for the provider-neutral PM-01 solid-modeling
// contract. No OCCT handle/type crosses this public header.
class OcctSolidModelingKernel final
    : public kernel::ISolidModelingKernel,
      public kernel::IEdgeProjectionQuery,
      public kernel::IExactBoundedEdgeGeometryQuery,
      public kernel::IFaceBoundaryQuery {
public:
    [[nodiscard]] std::optional<kernel::ScopedBoundaryFace>
    bindFaceToBody(
        kernel::RuntimeSolidHandle source_body,
        kernel::RuntimeFaceToken current_face) noexcept override;

    [[nodiscard]] kernel::FaceBoundaryResult
    queryFaceBoundary(
        kernel::RuntimeSolidHandle current_body,
        const kernel::ScopedBoundaryFace& bound_face) noexcept override;

    [[nodiscard]] kernel::FaceBoundaryResult
    queryFaceBoundaryAnySurface(
        kernel::RuntimeSolidHandle current_body,
        const kernel::ScopedBoundaryFace& bound_face) noexcept override;

    [[nodiscard]] std::optional<kernel::ScopedProjectionEdge>
    bindEdgeToBody(
        kernel::RuntimeSolidHandle source_body,
        kernel::RuntimeEdgeToken current_edge) noexcept override;

    [[nodiscard]] kernel::EdgeProjectionResult
    projectEdgeToPlane(
        kernel::RuntimeSolidHandle current_body,
        const kernel::ScopedProjectionEdge& bound_edge,
        const kernel::Frame3& target_frame) noexcept override;

    // Q0: strictly evaluation-local analytic bounded 3D geometry.
    [[nodiscard]] std::optional<kernel::ScopedExactEdge>
    bindExactEdgeToBody(
        kernel::RuntimeSolidHandle source_body,
        kernel::RuntimeEdgeToken current_edge) noexcept override;

    [[nodiscard]] kernel::ExactEdge3dResult
    queryExactBoundedEdge3d(
        kernel::RuntimeSolidHandle current_body,
        const kernel::ScopedExactEdge& bound_edge) noexcept override;

    // D2-R1 production default: history-aware Boolean same-domain Refine
    // for Extrude/Revolve Add. Explicit disabled is for native legacy
    // comparison/negative tests only, never an implicit user setting.
    enum class AddRefinementExperiment {
        disabled,
        simplify_same_domain,
    };

    OcctSolidModelingKernel() = default;
    explicit OcctSolidModelingKernel(
        AddRefinementExperiment mode) noexcept
        : refinement_experiment_(mode) {}
    ~OcctSolidModelingKernel() override = default;

    [[nodiscard]] kernel::SolidModelingResult
    extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidModelingResult
    revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidModelingResult
    edgeFeature(
        const kernel::EdgeFeatureInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidPresentationResult
    extrudePreviewMesh(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidPresentationResult
    revolvePreviewMesh(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidMaterialDeltaPresentationResult
    materialDifferencePreview(
        kernel::RuntimeSolidHandle before,
        kernel::RuntimeSolidHandle after) noexcept override;

    [[nodiscard]] kernel::SolidPresentationResult
    presentationMesh(
        kernel::RuntimeSolidHandle solid) noexcept override;

    [[nodiscard]] kernel::BodyPresentationResult
    bodyPresentation(
        kernel::RuntimeSolidHandle solid) noexcept override;

private:
    AddRefinementExperiment refinement_experiment_{
        AddRefinementExperiment::simplify_same_domain};

    // One provider-owned strict native wire extraction implementation.
    // The original planar-only API retains its exact preexisting policy.
    [[nodiscard]] kernel::FaceBoundaryResult
    queryFaceBoundaryImpl(
        kernel::RuntimeSolidHandle current_body,
        const kernel::ScopedBoundaryFace& bound_face,
        bool require_planar) noexcept;
};

} // namespace simplesolid2::kernel_occt
