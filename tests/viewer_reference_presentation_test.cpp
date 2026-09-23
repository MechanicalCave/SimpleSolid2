#include <simplesolid2/viewer/reference_presentation.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2::viewer;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 reference CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    ReferencePresentation point;
    point.token = {1U};
    point.kind = ReferencePresentationKind::point;
    point.extent = 4.0;
    CHECK(point.valid());

    ReferencePresentation axis;
    axis.token = {2U};
    axis.kind = ReferencePresentationKind::x_axis;
    axis.u_axis = {1.0, 0.0, 0.0};
    axis.extent = 50.0;
    CHECK(axis.valid());

    ReferencePresentation plane;
    plane.token = {3U};
    plane.kind = ReferencePresentationKind::plane;
    plane.u_axis = {1.0, 0.0, 0.0};
    plane.v_axis = {0.0, 1.0, 0.0};
    plane.extent = 50.0;
    CHECK(plane.valid());

    plane.v_axis = {2.0, 0.0, 0.0};
    CHECK(!plane.valid());

    axis.token = {};
    CHECK(!axis.valid());

    ReferenceGridPresentation grid;
    CHECK(grid.valid());

    auto invalid_grid = grid;
    invalid_grid.spacing = 0.0;
    CHECK(!invalid_grid.valid());

    invalid_grid = grid;
    invalid_grid.v_axis = {2.0, 0.0, 0.0};
    CHECK(!invalid_grid.valid());

    invalid_grid = grid;
    invalid_grid.major_step = 0U;
    CHECK(!invalid_grid.valid());

    return EXIT_SUCCESS;
}
