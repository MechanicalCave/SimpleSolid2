#pragma once

#include <simplesolid2/viewer/camera_state.hpp>
#include <simplesolid2/viewer/navigation.hpp>
#include <simplesolid2/viewer/reference_presentation.hpp>
#include <simplesolid2/viewer/selection.hpp>
#include <simplesolid2/viewer/sketch_presentation.hpp>
#include <simplesolid2/viewer/spatial_pointer.hpp>

#include <optional>

namespace simplesolid2::viewer {

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

    virtual bool setReferenceScene(
        const ReferenceScene& scene) = 0;

    virtual bool setSketchScene(
        const SketchScene& scene) = 0;

    virtual bool setSketchPreviewScene(
        const SketchPreviewScene& scene) = 0;

    virtual bool setPresentationSelection(
        const PresentationSelection& selection) = 0;

    virtual void setSelectionIntentHandler(
        SelectionIntentHandler handler) = 0;

    virtual void setSpatialPointerHandler(
        SpatialPointerHandler handler) = 0;

    virtual void setPrimaryPointerRouting(
        PrimaryPointerRouting routing) = 0;

    virtual void setCursorMode(
        ViewportCursorMode mode) = 0;
};

} // namespace simplesolid2::viewer
