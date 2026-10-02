#include <simplesolid2/kernel/profile_input.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E06 registration CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    kernel::PlanarProfileInput input;
    input.frame = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}};

    kernel::BoundaryUse2D first;
    first.curve =
        kernel::Line2{{10.0, 0.0}, {20.0, 0.0}};
    first.start_parameter = 0.0;
    first.end_parameter = 1.0;
    first.provenance = {"bottom", 0U, 0U, false};

    kernel::BoundaryUse2D second;
    second.curve =
        kernel::Line2{{20.0, 0.0}, {20.0, 10.0}};
    second.start_parameter = 0.0;
    second.end_parameter = 1.0;
    second.provenance = {"outer", 0U, 1U, false};

    kernel::BoundaryUse2D third;
    third.curve =
        kernel::Line2{{20.0, 10.0}, {10.0, 10.0}};
    third.start_parameter = 0.0;
    third.end_parameter = 1.0;
    third.provenance = {"top", 0U, 2U, false};

    kernel::BoundaryUse2D fourth;
    fourth.curve =
        kernel::Line2{{10.0, 10.0}, {10.0, 0.0}};
    fourth.start_parameter = 0.0;
    fourth.end_parameter = 1.0;
    fourth.provenance = {"inner", 0U, 3U, false};

    input.outer.boundary = {
        first,
        second,
        third,
        fourth};

    CHECK(input.valid());

    std::cout
        << "PM00A_E06_REGISTRATION_PASS\n";
    return EXIT_SUCCESS;
}
