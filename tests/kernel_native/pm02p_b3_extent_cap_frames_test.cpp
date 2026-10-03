#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.B3 extent cap-frame CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double epsilon = 1.0e-12;

bool near(double first, double second) {
    return std::abs(first - second) <= epsilon;
}

bool near(
    const kernel::Point3& first,
    const kernel::Point3& second) {
    return near(first.x, second.x) &&
           near(first.y, second.y) &&
           near(first.z, second.z);
}

bool near(
    const kernel::Frame3& first,
    const kernel::Frame3& second) {
    return near(first.origin, second.origin) &&
           near(first.u_axis, second.u_axis) &&
           near(first.v_axis, second.v_axis) &&
           near(first.normal, second.normal);
}

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string_view source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::string{source},
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    kernel::Frame3 frame) {
    kernel::PlanarProfileInput profile;
    profile.frame = frame;
    profile.outer.boundary = {
        lineUse({0.0, 0.0}, {40.0, 0.0}, "bottom", 0U),
        lineUse({40.0, 0.0}, {40.0, 30.0}, "right", 1U),
        lineUse({40.0, 30.0}, {0.0, 30.0}, "top", 2U),
        lineUse({0.0, 30.0}, {0.0, 0.0}, "left", 3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::Frame3 xyFrame() {
    kernel::Frame3 result;
    result.origin = {3.0, 5.0, 7.0};
    CHECK(result.valid());
    return result;
}

kernel::Frame3 xzFrame() {
    kernel::Frame3 result;
    result.origin = {11.0, 13.0, 17.0};
    result.u_axis = {1.0, 0.0, 0.0};
    result.v_axis = {0.0, 0.0, 1.0};
    result.normal = {0.0, -1.0, 0.0};
    CHECK(result.valid());
    return result;
}

kernel::Frame3 yzFrame() {
    kernel::Frame3 result;
    result.origin = {19.0, 23.0, 29.0};
    result.u_axis = {0.0, 1.0, 0.0};
    result.v_axis = {0.0, 0.0, 1.0};
    result.normal = {1.0, 0.0, 0.0};
    CHECK(result.valid());
    return result;
}

kernel::LinearExtrudeInput forward(
    kernel::PlanarProfileInput profile,
    double distance) {
    return {
        std::move(profile),
        0.0,
        distance,
        kernel::ExtrudeCapRole::profile_cap,
        kernel::ExtrudeCapRole::extent_cap,
        kernel::SolidBooleanOperation::add,
    };
}

kernel::LinearExtrudeInput reverse(
    kernel::PlanarProfileInput profile,
    double distance) {
    return {
        std::move(profile),
        -distance,
        0.0,
        kernel::ExtrudeCapRole::extent_cap,
        kernel::ExtrudeCapRole::profile_cap,
        kernel::SolidBooleanOperation::add,
    };
}

kernel::LinearExtrudeInput midplane(
    kernel::PlanarProfileInput profile,
    double total_distance) {
    const double half = total_distance * 0.5;
    return {
        std::move(profile),
        -half,
        half,
        kernel::ExtrudeCapRole::negative_cap,
        kernel::ExtrudeCapRole::positive_cap,
        kernel::SolidBooleanOperation::add,
    };
}

kernel::Frame3 shifted(
    kernel::Frame3 frame,
    double offset) {
    frame.origin.x += frame.normal.x * offset;
    frame.origin.y += frame.normal.y * offset;
    frame.origin.z += frame.normal.z * offset;
    CHECK(frame.valid());
    return frame;
}

std::size_t capCount(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    std::size_t count = 0U;
    for (const auto& face : result.new_faces) {
        if (face.role.kind ==
                kernel::ExtrudeGeneratedFaceRoleKind::cap &&
            face.role.cap_role == role &&
            face.status ==
                kernel::ReferenceStatus::resolved &&
            face.candidate_count == 1U &&
            face.resolved_token.has_value()) {
            ++count;
        }
    }
    return count;
}

struct NeutralExtentOutcome final {
    kernel::ExtrudeCapRole start_role{
        kernel::ExtrudeCapRole::profile_cap};
    kernel::ExtrudeCapRole end_role{
        kernel::ExtrudeCapRole::extent_cap};
    kernel::Frame3 start_frame;
    kernel::Frame3 end_frame;

    friend bool operator==(
        const NeutralExtentOutcome&,
        const NeutralExtentOutcome&) = default;
};

NeutralExtentOutcome evaluate(
    const kernel::LinearExtrudeInput& input,
    kernel::ExtrudeCapRole expected_start_role,
    kernel::ExtrudeCapRole expected_end_role) {
    CHECK(input.valid());
    CHECK(input.start_cap_role == expected_start_role);
    CHECK(input.end_cap_role == expected_end_role);

    kernel_occt::OcctSolidModelingKernel provider;
    const auto production =
        provider.extrude(input);
    CHECK(production.ok());
    CHECK(capCount(production, expected_start_role) == 1U);
    CHECK(capCount(production, expected_end_role) == 1U);

    // No other semantic cap role may be falsely published as Resolved.
    for (const auto role : {
             kernel::ExtrudeCapRole::profile_cap,
             kernel::ExtrudeCapRole::extent_cap,
             kernel::ExtrudeCapRole::negative_cap,
             kernel::ExtrudeCapRole::positive_cap}) {
        const auto count = capCount(production, role);
        if (role == expected_start_role ||
            role == expected_end_role) {
            CHECK(count == 1U);
        } else {
            CHECK(count == 0U);
        }
    }

    auto shifted_profile = input.profile;
    shifted_profile.frame =
        shifted(
            input.profile.frame,
            input.start_offset_mm);

    const double span =
        input.end_offset_mm -
        input.start_offset_mm;
    CHECK(span > 0.0);

    const auto carrier =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            shifted_profile,
            span);
    CHECK(carrier.completeFaceClaims());
    CHECK(
        carrier.start_cap.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        carrier.end_cap.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        carrier.start_cap.semantic_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(
        carrier.end_cap.semantic_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(
        carrier.start_cap.provider_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(
        carrier.end_cap.provider_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(carrier.start_cap.canonical_frame.has_value());
    CHECK(carrier.end_cap.canonical_frame.has_value());

    const auto expected_start =
        shifted(
            input.profile.frame,
            input.start_offset_mm);
    const auto expected_end =
        shifted(
            input.profile.frame,
            input.end_offset_mm);

    CHECK(
        near(
            *carrier.start_cap.canonical_frame,
            expected_start));
    CHECK(
        near(
            *carrier.end_cap.canonical_frame,
            expected_end));

    // Canonical cap frame orientation remains the authored support
    // orientation. Material/outward side sense is a separate semantic field;
    // negative/reverse extent does not silently mirror U/V/N.
    CHECK(
        near(
            carrier.start_cap.canonical_frame->u_axis,
            input.profile.frame.u_axis));
    CHECK(
        near(
            carrier.start_cap.canonical_frame->v_axis,
            input.profile.frame.v_axis));
    CHECK(
        near(
            carrier.start_cap.canonical_frame->normal,
            input.profile.frame.normal));
    CHECK(
        near(
            carrier.end_cap.canonical_frame->u_axis,
            input.profile.frame.u_axis));
    CHECK(
        near(
            carrier.end_cap.canonical_frame->v_axis,
            input.profile.frame.v_axis));
    CHECK(
        near(
            carrier.end_cap.canonical_frame->normal,
            input.profile.frame.normal));

    return {
        expected_start_role,
        expected_end_role,
        *carrier.start_cap.canonical_frame,
        *carrier.end_cap.canonical_frame,
    };
}

std::vector<NeutralExtentOutcome> evaluateFamily() {
    std::vector<NeutralExtentOutcome> outcomes;
    for (const auto& frame : {
             xyFrame(),
             xzFrame(),
             yzFrame()}) {
        outcomes.push_back(
            evaluate(
                forward(
                    rectangle(frame),
                    20.0),
                kernel::ExtrudeCapRole::profile_cap,
                kernel::ExtrudeCapRole::extent_cap));

        outcomes.push_back(
            evaluate(
                reverse(
                    rectangle(frame),
                    20.0),
                kernel::ExtrudeCapRole::extent_cap,
                kernel::ExtrudeCapRole::profile_cap));

        outcomes.push_back(
            evaluate(
                midplane(
                    rectangle(frame),
                    20.0),
                kernel::ExtrudeCapRole::negative_cap,
                kernel::ExtrudeCapRole::positive_cap));
    }
    return outcomes;
}

} // namespace

int main() {
    const auto first = evaluateFamily();
    const auto cold = evaluateFamily();

    CHECK(first == cold);
    CHECK(first.size() == 9U);

    std::cout
        << "PM02P_B3_EXTENT_CAP_FRAMES_PASS"
        << " cases=9"
        << " forward=ProfileCap/ExtentCap"
        << " reverse=ExtentCap/ProfileCap"
        << " midplane=NegativeCap/PositiveCap"
        << " frame_instability=0"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
