#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E05 registration CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    std::string source,
    std::uint32_t use_index,
    kernel::Point2 start,
    kernel::Point2 end) {
    kernel::BoundaryUse2D use;
    use.curve = kernel::Line2{start, end};
    use.start_parameter = 0.0;
    use.end_parameter = 1.0;
    use.follows_source_direction = true;
    use.crosses_closed_seam = false;
    use.whole_closed_curve = false;
    use.provenance = {
        std::move(source),
        0U,
        use_index,
        false};
    return use;
}

kernel::PlanarProfileInput splitBottomProfile() {
    kernel::PlanarProfileInput input;
    input.frame = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}};

    input.outer.boundary = {
        lineUse("bottom-left", 0U, {0.0, 0.0}, {20.0, 0.0}),
        lineUse("bottom-right", 1U, {20.0, 0.0}, {40.0, 0.0}),
        lineUse("right", 2U, {40.0, 0.0}, {40.0, 30.0}),
        lineUse("top", 3U, {40.0, 30.0}, {0.0, 30.0}),
        lineUse("left", 4U, {0.0, 30.0}, {0.0, 0.0}),
    };

    return input;
}

} // namespace

int main() {
    const auto input = splitBottomProfile();
    CHECK(input.valid());

    const auto evidence =
        kernel_occt::buildProfileExtrudeEvidence(
            input,
            10.0);

    CHECK(evidence.ok());
    CHECK(evidence.shape.brep_valid);
    CHECK(evidence.shape.solid_count == 1U);
    CHECK(evidence.sides.size() == 5U);

    for (const auto& side : evidence.sides) {
        CHECK(
            side.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(side.candidate_face_count == 1U);
        CHECK(side.provenance.has_value());
    }

    std::cout
        << "PM00A_E05_REGISTRATION_PASS sides="
        << evidence.sides.size()
        << '\n';
    return EXIT_SUCCESS;
}
