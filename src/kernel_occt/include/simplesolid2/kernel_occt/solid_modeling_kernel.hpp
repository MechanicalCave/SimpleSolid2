#pragma once

#include <simplesolid2/kernel/edge_projection.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>

namespace simplesolid2::kernel_occt {

// Production OCCT provider for the provider-neutral PM-01 solid-modeling
// contract. No OCCT handle/type crosses this public header.
class OcctSolidModelingKernel final
    : public kernel::ISolidModelingKernel,
      public kernel::IEdgeProjectionQuery {
public:
    [[nodiscard]] kernel::EdgeProjectionResult
    projectEdgeToPlane(
        kernel::RuntimeSolidHandle source_body,
        kernel::RuntimeEdgeToken current_edge,
        const kernel::Frame3& target_frame) noexcept override;

    OcctSolidModelingKernel() = default;
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
};

} // namespace simplesolid2::kernel_occt
