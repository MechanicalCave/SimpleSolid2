#include "navigation_mapping.hpp"
#include <cmath>
namespace simplesolid2::viewer_qt_occt::detail {
OrbitScreenAngles orbitScreenAnglesFromMouseDelta(int dx,int dy,double k) noexcept {
 if(!std::isfinite(k)||k<=0.0) return {};
 return {static_cast<double>(dx)*k,-static_cast<double>(dy)*k,0.0};
}
double wheelZoomDragFraction(int angle_delta_y) noexcept { constexpr double per_notch=0.025; return per_notch*(static_cast<double>(angle_delta_y)/120.0); }
}
