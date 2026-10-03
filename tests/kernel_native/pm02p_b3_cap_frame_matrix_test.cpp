#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.B3 cap frame matrix CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double epsilon = 1.0e-10;

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

kernel::Frame3 sourceFrame() {
    // Non-XY origin frame: U=+Y, V=+Z, N=+X.
    return {
        {7.0, 11.0, 13.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0},
        {1.0, 0.0, 0.0},
    };
}

kernel::PlanarProfileInput rectangle() {
    kernel::PlanarProfileInput profile;
    profile.frame = sourceFrame();
    profile.outer.boundary = {
        lineUse({0.0, 0.0}, {20.0, 0.0}, "bottom", 0U),
        lineUse({20.0, 0.0}, {20.0, 10.0}, "right", 1U),
        lineUse({20.0, 10.0}, {0.0, 10.0}, "top", 2U),
        lineUse({0.0, 10.0}, {0.0, 0.0}, "left", 3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::Frame3 shiftedFrame(
    const kernel::Frame3& source,
    double offset) {
    auto result = source;
    result.origin.x += source.normal.x * offset;
    result.origin.y += source.normal.y * offset;
    result.origin.z += source.normal.z * offset;
    CHECK(result.valid());
    return result;
}

const kernel::NewFaceLineage* cap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    for (const auto& face : result.new_faces) {
        if (face.role.kind ==
                kernel::ExtrudeGeneratedFaceRoleKind::cap &&
            face.role.cap_role == role) {
            return &face;
        }
    }
    return nullptr;
}

struct Case final {
    const char* name;
    double start_offset;
    double end_offset;
    kernel::ExtrudeCapRole start_role;
    kernel::ExtrudeCapRole end_role;
};

void verifyCase(
    kernel_occt::OcctSolidModelingKernel& provider,
    const Case& item) {
    kernel::LinearExtrudeInput input;
    input.profile = rectangle();
    input.start_offset_mm = item.start_offset;
    input.end_offset_mm = item.end_offset;
    input.start_cap_role = item.start_role;
    input.end_cap_role = item.end_role;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());

    const auto modeled =
        provider.extrude(input);
    CHECK(modeled.ok());

    const auto* start_cap =
        cap(modeled, item.start_role);
    const auto* end_cap =
        cap(modeled, item.end_role);
    CHECK(start_cap != nullptr);
    CHECK(end_cap != nullptr);
    CHECK(
        start_cap->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        end_cap->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(start_cap->candidate_count == 1U);
    CHECK(end_cap->candidate_count == 1U);

    // B1's carrier builder begins at its supplied Profile frame. Shift that
    // frame to the production start offset, then use the exact signed span.
    // This independently proves the cap-frame algebra for the same semantic
    // start/end roles emitted by the production provider.
    auto carrier_profile = input.profile;
    carrier_profile.frame =
        shiftedFrame(
            input.profile.frame,
            input.start_offset_mm);
    const double span =
        input.end_offset_mm -
        input.start_offset_mm;
    const auto carriers =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            carrier_profile,
            span);
    CHECK(carriers.completeFaceClaims());
    CHECK(carriers.start_cap.canonical_frame.has_value());
    CHECK(carriers.end_cap.canonical_frame.has_value());

    const auto expected_start =
        shiftedFrame(
            input.profile.frame,
            input.start_offset_mm);
    const auto expected_end =
        shiftedFrame(
            input.profile.frame,
            input.end_offset_mm);

    CHECK(
        near(
            *carriers.start_cap.canonical_frame,
            expected_start));
    CHECK(
        near(
            *carriers.end_cap.canonical_frame,
            expected_end));

    // Orientation is inherited from the semantic Profile/support frame.
    CHECK(
        near(
            carriers.start_cap.canonical_frame->u_axis,
            input.profile.frame.u_axis));
    CHECK(
        near(
            carriers.start_cap.canonical_frame->v_axis,
            input.profile.frame.v_axis));
    CHECK(
        near(
            carriers.start_cap.canonical_frame->normal,
            input.profile.frame.normal));
    CHECK(
        near(
            carriers.end_cap.canonical_frame->u_axis,
            input.profile.frame.u_axis));
    CHECK(
        near(
            carriers.end_cap.canonical_frame->v_axis,
            input.profile.frame.v_axis));
    CHECK(
        near(
            carriers.end_cap.canonical_frame->normal,
            input.profile.frame.normal));

    std::cout
        << "B3_CASE " << item.name
        << " start=" << item.start_offset
        << " end=" << item.end_offset
        << '\n';
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    const Case cases[] = {
        {
            "OneSideForward",
            0.0,
            10.0,
            kernel::ExtrudeCapRole::profile_cap,
            kernel::ExtrudeCapRole::extent_cap,
        },
        {
            "OneSideReverse",
            -10.0,
            0.0,
            kernel::ExtrudeCapRole::extent_cap,
            kernel::ExtrudeCapRole::profile_cap,
        },
        {
            "Midplane",
            -5.0,
            5.0,
            kernel::ExtrudeCapRole::negative_cap,
            kernel::ExtrudeCapRole::positive_cap,
        },
    };

    for (const auto& item : cases) {
        verifyCase(provider, item);
    }

    // Cold reconstruction of the same role/frame matrix.
    kernel_occt::OcctSolidModelingKernel cold_provider;
    for (const auto& item : cases) {
        verifyCase(cold_provider, item);
    }

    std::cout
        << "PM02P_B3_CAP_FRAME_MATRIX_PASS"
        << " cases=Forward,Reverse,Midplane"
        << " roles=ProfileCap,ExtentCap,NegativeCap,PositiveCap"
        << " frame_instability=0\n";
    return EXIT_SUCCESS;
}
