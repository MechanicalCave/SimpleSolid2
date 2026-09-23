#include <simplesolid2/viewer/camera_state.hpp>
#include <cmath>
namespace simplesolid2::viewer {
CameraValidationResult validateCameraState(const CameraState& s) noexcept {
 if(!finite(s.eye)||!finite(s.target)||!finite(s.up)||!std::isfinite(s.scale)||s.scale<=0.0) return {false,"Camera requires finite eye/target/up values and a positive finite scale"};
 const auto sight=s.target-s.eye;
 if(sight.squaredLength()<=1.0e-24||s.up.squaredLength()<=1.0e-24) return {false,"Camera eye/target and up vector must be non-degenerate"};
 if(cross(sight,s.up).squaredLength()<=1.0e-24) return {false,"Camera up vector must not be collinear with sight direction"};
 return {true,{}};
}
}
