#pragma once

#include <simplesolid2/viewer/camera_state.hpp>
#include <simplesolid2/viewer/navigation.hpp>
#include <simplesolid2/viewer/reference_presentation.hpp>
#include <simplesolid2/viewer/selection.hpp>

#include <functional>
#include <optional>

namespace simplesolid2::viewer {

using CameraStateChangedHandler =
    std::function<void(const CameraState&)>;

class IDocumentViewport {
public:
    virtual ~IDocumentViewport() = default;

    [[nodiscard]] virtual std::optional<CameraState>
    cameraState() const = 0;

    virtual bool setCameraState(
        const CameraState& state) = 0;

    virtual bool setStandardView(
        StandardView view) = 0;

    virtual bool setProjection(
        CameraProjection projection) = 0;

    virtual void fitAll() = 0;

    virtual void setCameraStateChangedHandler(
        CameraStateChangedHandler handler) = 0;

    virtual bool setReferenceScene(
        const ReferenceScene& scene) = 0;

    virtual bool setPresentationSelection(
        const PresentationSelection& selection) = 0;

    virtual void setSelectionIntentHandler(
        SelectionIntentHandler handler) = 0;
};

} // namespace simplesolid2::viewer
