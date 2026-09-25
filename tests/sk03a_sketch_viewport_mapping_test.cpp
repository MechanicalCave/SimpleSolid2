#include "sketch_viewport_mapping.hpp"

#include <simplesolid2/part/part_sketch.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-03A Sketch mapping CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::SketchPlacement placementFor(
    core::BuiltinReferenceRole role) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(role);
    CHECK(support.has_value());
    const auto placement =
        part::sketchPlacementForSupport(*support);
    CHECK(placement.has_value());
    return *placement;
}

void checkPoint(
    const viewer::Point3& actual,
    double x,
    double y,
    double z) {
    CHECK(actual == viewer::Point3{x, y, z});
}

void checkUv(
    const sketch::Point2& actual,
    double u,
    double v) {
    CHECK(actual == sketch::Point2{u, v});
}

} // namespace

int main() {
    const auto xy =
        placementFor(
            core::BuiltinReferenceRole::xy_plane);
    const auto xz =
        placementFor(
            core::BuiltinReferenceRole::xz_plane);
    const auto yz =
        placementFor(
            core::BuiltinReferenceRole::yz_plane);

    const auto xy_world =
        ui::detail::sketchPointToWorld(
            xy,
            sketch::Point2{3.0, 4.0});
    CHECK(xy_world.has_value());
    checkPoint(*xy_world, 3.0, 4.0, 0.0);

    const auto xz_world =
        ui::detail::sketchPointToWorld(
            xz,
            sketch::Point2{3.0, 4.0});
    CHECK(xz_world.has_value());
    checkPoint(*xz_world, 3.0, 0.0, 4.0);

    const auto yz_world =
        ui::detail::sketchPointToWorld(
            yz,
            sketch::Point2{3.0, 4.0});
    CHECK(yz_world.has_value());
    checkPoint(*yz_world, 0.0, 3.0, 4.0);

    const auto xy_uv =
        ui::detail::sketchPointFromRay(
            xy,
            viewer::Ray3{
                viewer::Point3{3.0, 4.0, 10.0},
                viewer::Vec3{0.0, 0.0, -2.0}});
    CHECK(xy_uv.has_value());
    checkUv(*xy_uv, 3.0, 4.0);

    const auto xz_uv =
        ui::detail::sketchPointFromRay(
            xz,
            viewer::Ray3{
                viewer::Point3{3.0, 10.0, 4.0},
                viewer::Vec3{0.0, -1.0, 0.0}});
    CHECK(xz_uv.has_value());
    checkUv(*xz_uv, 3.0, 4.0);

    const auto yz_uv =
        ui::detail::sketchPointFromRay(
            yz,
            viewer::Ray3{
                viewer::Point3{10.0, 3.0, 4.0},
                viewer::Vec3{-1.0, 0.0, 0.0}});
    CHECK(yz_uv.has_value());
    checkUv(*yz_uv, 3.0, 4.0);

    const auto orbit_style_uv =
        ui::detail::sketchPointFromRay(
            xy,
            viewer::Ray3{
                viewer::Point3{3.0, 4.0, 10.0},
                viewer::Vec3{2.0, 1.0, -10.0}});
    CHECK(orbit_style_uv.has_value());
    checkUv(*orbit_style_uv, 5.0, 5.0);

    const auto parallel =
        ui::detail::sketchPointFromRay(
            xy,
            viewer::Ray3{
                viewer::Point3{0.0, 0.0, 1.0},
                viewer::Vec3{1.0, 0.0, 0.0}});
    CHECK(!parallel.has_value());

    const auto behind =
        ui::detail::sketchPointFromRay(
            xy,
            viewer::Ray3{
                viewer::Point3{0.0, 0.0, 1.0},
                viewer::Vec3{0.0, 0.0, 1.0}});
    CHECK(!behind.has_value());

    auto non_metric = xy;
    non_metric.u_axis = {2.0, 0.0, 0.0};
    CHECK(
        !ui::detail::sketchPointToWorld(
             non_metric,
             sketch::Point2{1.0, 1.0})
             .has_value());
    CHECK(
        !ui::detail::sketchPointFromRay(
             non_metric,
             viewer::Ray3{
                 viewer::Point3{0.0, 0.0, 1.0},
                 viewer::Vec3{0.0, 0.0, -1.0}})
             .has_value());

    return EXIT_SUCCESS;
}
