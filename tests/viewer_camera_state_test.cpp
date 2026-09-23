#include <simplesolid2/viewer/camera_state.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2::viewer;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "WB-01 camera CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    CameraState state;
    CHECK(validateCameraState(state).valid);

    auto invalid = state;
    invalid.target = invalid.eye;
    CHECK(!validateCameraState(invalid).valid);

    invalid = state;
    invalid.up = invalid.target - invalid.eye;
    CHECK(!validateCameraState(invalid).valid);

    invalid = state;
    invalid.scale = 0.0;
    CHECK(!validateCameraState(invalid).valid);

    return EXIT_SUCCESS;
}
