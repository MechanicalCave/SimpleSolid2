#pragma once

#include <simplesolid2/viewer/camera_state.hpp>
#include <simplesolid2/viewer/navigation.hpp>

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
};

} // namespace simplesolid2::viewer
