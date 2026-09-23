#include "navigation_mapping.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace simplesolid2::viewer_qt_occt::detail;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 mapping CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

bool near(double a, double b, double eps = 1.0e-12) {
    return std::abs(a - b) <= eps;
}

} // namespace

int main() {
    constexpr double sensitivity = 0.006;

    const auto right =
        orbitScreenAnglesFromMouseDelta(10, 0, sensitivity);
    CHECK(near(right.x, 0.06));
    CHECK(near(right.y, 0.0));
    CHECK(near(right.z, 0.0));

    const auto up =
        orbitScreenAnglesFromMouseDelta(0, -10, sensitivity);
    CHECK(near(up.x, 0.0));
    CHECK(near(up.y, 0.06));

    const auto diagonal =
        orbitScreenAnglesFromMouseDelta(4, -3, sensitivity);
    CHECK(near(diagonal.x, 0.024));
    CHECK(near(diagonal.y, 0.018));

    CHECK(near(wheelZoomDragFraction(120), 0.025));
    CHECK(near(wheelZoomDragFraction(-120), -0.025));
    CHECK(near(wheelZoomDragFraction(60), 0.0125));

    const auto invalid =
        orbitScreenAnglesFromMouseDelta(10, 10, 0.0);
    CHECK(near(invalid.x, 0.0));
    CHECK(near(invalid.y, 0.0));

    return EXIT_SUCCESS;
}
