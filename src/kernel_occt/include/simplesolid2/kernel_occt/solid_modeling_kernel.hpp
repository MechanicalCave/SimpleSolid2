#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>

namespace simplesolid2::kernel_occt {

// Production OCCT provider for the provider-neutral PM-01 solid-modeling
// contract. No OCCT handle/type crosses this public header.
class OcctSolidModelingKernel final
    : public kernel::ISolidModelingKernel {
public:
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
    revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidPresentationResult
    extrudePreviewMesh(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override;

    [[nodiscard]] kernel::SolidPresentationResult
    presentationMesh(
        kernel::RuntimeSolidHandle solid) noexcept override;

    [[nodiscard]] kernel::BodyPresentationResult
    bodyPresentation(
        kernel::RuntimeSolidHandle solid) noexcept override;
};

} // namespace simplesolid2::kernel_occt
