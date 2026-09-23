#pragma once
#include <simplesolid2/viewer/math3.hpp>
#include <cmath>
#include <cstdint>
#include <vector>
namespace simplesolid2::viewer {
struct PresentationToken final { std::uint64_t value{}; [[nodiscard]] constexpr bool valid() const noexcept { return value!=0U; } friend constexpr bool operator==(const PresentationToken&,const PresentationToken&)=default; };
enum class ReferencePresentationKind:std::uint8_t { point,x_axis,y_axis,z_axis,plane };
enum class PresentationRole:std::uint8_t { base,hover,secondary_selection,primary_selection };
struct ReferencePresentation final {
 PresentationToken token; ReferencePresentationKind kind{ReferencePresentationKind::point}; Point3 origin{}; Vec3 u_axis{}; Vec3 v_axis{}; double extent{1.0}; PresentationRole role{PresentationRole::base}; bool visible{true};
 [[nodiscard]] bool valid() const noexcept {
  if(!token.valid()||!finite(origin)||!std::isfinite(extent)||extent<=0.0) return false;
  if(kind==ReferencePresentationKind::point) return true;
  if(!finite(u_axis)||u_axis.squaredLength()<=1.0e-24) return false;
  if(kind!=ReferencePresentationKind::plane) return true;
  if(!finite(v_axis)||v_axis.squaredLength()<=1.0e-24) return false;
  return cross(u_axis,v_axis).squaredLength()>1.0e-24;
 }
};
struct ReferenceScene final { std::vector<ReferencePresentation> references; };
}
