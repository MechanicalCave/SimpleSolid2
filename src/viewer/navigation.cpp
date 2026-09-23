#include <simplesolid2/viewer/navigation.hpp>
#include <cmath>
namespace simplesolid2::viewer {
namespace { double distance(const Point3&a,const Point3&b) noexcept { const auto d=a-b; return std::sqrt(d.squaredLength()); } }
ViewOrientation standardViewOrientation(StandardView v) noexcept {
 switch(v){
 case StandardView::front:return {{0,1,0},{0,0,1}};
 case StandardView::back:return {{0,-1,0},{0,0,1}};
 case StandardView::left:return {{1,0,0},{0,0,1}};
 case StandardView::right:return {{-1,0,0},{0,0,1}};
 case StandardView::top:return {{0,0,-1},{0,1,0}};
 case StandardView::bottom:return {{0,0,1},{0,1,0}};
 case StandardView::isometric:return {{-1,1,-1},{0,0,1}};
 }
 return {{0,1,0},{0,0,1}};
}
std::optional<CameraState> cameraForStandardView(const CameraState& c,StandardView v) noexcept {
 if(!validateCameraState(c).valid) return std::nullopt;
 const auto o=standardViewOrientation(v); const auto dir=normalized(o.view_direction); if(!dir) return std::nullopt;
 const auto dist=distance(c.eye,c.target); if(!std::isfinite(dist)||dist<=1.0e-12) return std::nullopt;
 auto r=c; r.eye=c.target-(*dir*dist); r.up=o.up; if(!validateCameraState(r).valid) return std::nullopt; return r;
}
std::optional<CameraState> cameraWithProjection(const CameraState& c,CameraProjection p) noexcept { if(!validateCameraState(c).valid) return std::nullopt; auto r=c; r.projection=p; return r; }
}
