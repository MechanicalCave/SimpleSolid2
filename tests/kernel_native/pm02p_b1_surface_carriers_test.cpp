#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <string>
#include <string_view>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.B1 Surface carrier CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double epsilon = 1.0e-12;

bool near(double lhs, double rhs) {
    return std::abs(lhs - rhs) <= epsilon;
}

bool near(
    const kernel::Point3& lhs,
    const kernel::Point3& rhs) {
    return near(lhs.x, rhs.x) &&
           near(lhs.y, rhs.y) &&
           near(lhs.z, rhs.z);
}

bool near(
    const kernel::Frame3& lhs,
    const kernel::Frame3& rhs) {
    return near(lhs.origin, rhs.origin) &&
           near(lhs.u_axis, rhs.u_axis) &&
           near(lhs.v_axis, rhs.v_axis) &&
           near(lhs.normal, rhs.normal);
}

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string_view source,
    std::uint32_t loop_index,
    std::uint32_t use_index,
    bool hole = false) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::string{source},
            loop_index,
            use_index,
            hole},
    };
}

kernel::BoundaryUse2D circleUse(
    kernel::Point2 center,
    double radius,
    std::string_view source,
    std::uint32_t loop_index,
    bool hole) {
    return {
        kernel::Circle2{center, radius},
        0.0,
        1.0,
        true,
        false,
        true,
        kernel::BoundaryUseProvenance{
            std::string{source},
            loop_index,
            0U,
            hole},
    };
}

kernel::BoundaryUse2D arcUse(
    kernel::Point2 center,
    double radius,
    double start_angle,
    double sweep_angle,
    std::string_view source,
    std::uint32_t use_index) {
    return {
        kernel::Arc2{
            center,
            radius,
            start_angle,
            sweep_angle},
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
    kernel::Frame3 frame = {}) {
    kernel::PlanarProfileInput profile;
    profile.frame = frame;
    profile.outer.boundary = {
        lineUse({0.0, 0.0}, {40.0, 0.0}, "bottom", 0U, 0U),
        lineUse({40.0, 0.0}, {40.0, 30.0}, "right", 0U, 1U),
        lineUse({40.0, 30.0}, {0.0, 30.0}, "top", 0U, 2U),
        lineUse({0.0, 30.0}, {0.0, 0.0}, "left", 0U, 3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput circleProfile() {
    kernel::PlanarProfileInput profile;
    profile.outer.boundary.push_back(
        circleUse(
            {0.0, 0.0},
            10.0,
            "outer-circle",
            0U,
            false));
    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput rectangleWithHole() {
    auto profile = rectangle();

    kernel::ProfileLoopInput hole;
    hole.boundary.push_back(
        circleUse(
            {20.0, 15.0},
            5.0,
            "hole-circle",
            1U,
            true));
    profile.holes.push_back(
        std::move(hole));

    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput mixedLineArcProfile() {
    kernel::PlanarProfileInput profile;
    profile.outer.boundary = {
        arcUse(
            {0.0, 0.0},
            10.0,
            0.0,
            std::numbers::pi_v<double>,
            "upper-arc",
            0U),
        lineUse(
            {-10.0, 0.0},
            {10.0, 0.0},
            "diameter",
            0U,
            1U),
    };
    CHECK(profile.valid());
    return profile;
}

const kernel::EvidenceSurfaceCarrierRecord*
findSide(
    const kernel::ExtrudeSurfaceCarrierEvidence& evidence,
    std::string_view source) {
    const auto found =
        std::find_if(
            evidence.sides.begin(),
            evidence.sides.end(),
            [source](const auto& side) {
                return side.provenance.has_value() &&
                       side.provenance->source_entity ==
                           source;
            });
    return found == evidence.sides.end()
        ? nullptr
        : &*found;
}

std::size_t countFaceClass(
    const kernel::ExtrudeSurfaceCarrierEvidence& evidence,
    kernel::EvidenceTopologyAccountingClass expected) {
    return static_cast<std::size_t>(
        std::count_if(
            evidence.topology.faces.catalog.begin(),
            evidence.topology.faces.catalog.end(),
            [expected](const auto& record) {
                return record.accounting_class ==
                       expected;
            }));
}

void verifyCommon(
    const kernel::ExtrudeSurfaceCarrierEvidence& evidence,
    std::size_t expected_faces,
    std::size_t expected_sides) {
    CHECK(evidence.completeFaceClaims());
    CHECK(evidence.topology.complete());
    CHECK(evidence.topology.faces.provider_unique_count ==
          expected_faces);
    CHECK(evidence.unique_claimed_face_count ==
          expected_faces);
    CHECK(evidence.unclaimed_face_count == 0U);
    CHECK(evidence.multiply_claimed_face_count == 0U);
    CHECK(evidence.claim_outside_body_count == 0U);
    CHECK(evidence.sides.size() == expected_sides);

    CHECK(
        countFaceClass(
            evidence,
            kernel::EvidenceTopologyAccountingClass::
                referenceable) ==
        expected_faces);
    CHECK(
        countFaceClass(
            evidence,
            kernel::EvidenceTopologyAccountingClass::
                integrity_failure) ==
        0U);

    const auto verify_carrier =
        [](const kernel::EvidenceSurfaceCarrierRecord& carrier) {
            CHECK(
                carrier.status ==
                kernel::ReferenceStatus::resolved);
            CHECK(carrier.candidate_face_count == 1U);
            CHECK(carrier.provider_surface_kind.has_value());
            CHECK(
                *carrier.provider_surface_kind ==
                carrier.semantic_surface_kind);
            if (carrier.semantic_surface_kind ==
                kernel::FaceSurfaceKind::plane) {
                CHECK(carrier.canonical_frame.has_value());
                CHECK(carrier.canonical_frame->valid());
            }
        };

    verify_carrier(evidence.start_cap);
    verify_carrier(evidence.end_cap);
    for (const auto& side : evidence.sides) {
        verify_carrier(side);
    }
}

void verifyRectangleXY() {
    const auto evidence =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangle(),
            10.0);
    verifyCommon(evidence, 6U, 4U);

    CHECK(evidence.start_cap.canonical_frame.has_value());
    CHECK(evidence.end_cap.canonical_frame.has_value());
    CHECK(
        near(
            *evidence.start_cap.canonical_frame,
            kernel::Frame3{}));

    kernel::Frame3 end_frame;
    end_frame.origin = {0.0, 0.0, 10.0};
    CHECK(
        near(
            *evidence.end_cap.canonical_frame,
            end_frame));

    const auto* bottom =
        findSide(evidence, "bottom");
    const auto* right =
        findSide(evidence, "right");
    const auto* top =
        findSide(evidence, "top");
    const auto* left =
        findSide(evidence, "left");
    CHECK(bottom != nullptr);
    CHECK(right != nullptr);
    CHECK(top != nullptr);
    CHECK(left != nullptr);

    kernel::Frame3 bottom_frame;
    bottom_frame.origin = {0.0, 0.0, 0.0};
    bottom_frame.u_axis = {1.0, 0.0, 0.0};
    bottom_frame.v_axis = {0.0, 0.0, 1.0};
    bottom_frame.normal = {0.0, -1.0, 0.0};

    kernel::Frame3 right_frame;
    right_frame.origin = {40.0, 0.0, 0.0};
    right_frame.u_axis = {0.0, 1.0, 0.0};
    right_frame.v_axis = {0.0, 0.0, 1.0};
    right_frame.normal = {1.0, 0.0, 0.0};

    kernel::Frame3 top_frame;
    top_frame.origin = {40.0, 30.0, 0.0};
    top_frame.u_axis = {-1.0, 0.0, 0.0};
    top_frame.v_axis = {0.0, 0.0, 1.0};
    top_frame.normal = {0.0, 1.0, 0.0};

    kernel::Frame3 left_frame;
    left_frame.origin = {0.0, 30.0, 0.0};
    left_frame.u_axis = {0.0, -1.0, 0.0};
    left_frame.v_axis = {0.0, 0.0, 1.0};
    left_frame.normal = {-1.0, 0.0, 0.0};

    CHECK(bottom->canonical_frame.has_value());
    CHECK(right->canonical_frame.has_value());
    CHECK(top->canonical_frame.has_value());
    CHECK(left->canonical_frame.has_value());
    CHECK(near(*bottom->canonical_frame, bottom_frame));
    CHECK(near(*right->canonical_frame, right_frame));
    CHECK(near(*top->canonical_frame, top_frame));
    CHECK(near(*left->canonical_frame, left_frame));

    const auto cold =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangle(),
            10.0);
    CHECK(cold == evidence);

    // Signed extent direction moves only the end-cap origin. Canonical carrier
    // U/V/N remains tied to the source support frame, not to signed sweep
    // direction.
    const auto reverse =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangle(),
            -10.0);
    verifyCommon(reverse, 6U, 4U);
    kernel::Frame3 reverse_end;
    reverse_end.origin = {0.0, 0.0, -10.0};
    CHECK(reverse.end_cap.canonical_frame.has_value());
    CHECK(
        near(
            *reverse.end_cap.canonical_frame,
            reverse_end));
    const auto* reverse_bottom =
        findSide(reverse, "bottom");
    CHECK(reverse_bottom != nullptr);
    CHECK(reverse_bottom->canonical_frame.has_value());
    CHECK(
        near(
            *reverse_bottom->canonical_frame,
            bottom_frame));
}

void verifyOtherOriginFrames() {
    kernel::Frame3 xz;
    xz.u_axis = {1.0, 0.0, 0.0};
    xz.v_axis = {0.0, 0.0, 1.0};
    xz.normal = {0.0, -1.0, 0.0};
    CHECK(xz.valid());

    const auto xz_evidence =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangle(xz),
            12.0);
    verifyCommon(xz_evidence, 6U, 4U);
    const auto* xz_bottom =
        findSide(xz_evidence, "bottom");
    CHECK(xz_bottom != nullptr);
    CHECK(xz_bottom->canonical_frame.has_value());

    kernel::Frame3 xz_bottom_expected;
    xz_bottom_expected.u_axis = {1.0, 0.0, 0.0};
    xz_bottom_expected.v_axis = {0.0, -1.0, 0.0};
    xz_bottom_expected.normal = {0.0, 0.0, -1.0};
    CHECK(
        near(
            *xz_bottom->canonical_frame,
            xz_bottom_expected));

    kernel::Frame3 yz;
    yz.u_axis = {0.0, 1.0, 0.0};
    yz.v_axis = {0.0, 0.0, 1.0};
    yz.normal = {1.0, 0.0, 0.0};
    CHECK(yz.valid());

    const auto yz_evidence =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangle(yz),
            8.0);
    verifyCommon(yz_evidence, 6U, 4U);
    const auto* yz_bottom =
        findSide(yz_evidence, "bottom");
    CHECK(yz_bottom != nullptr);
    CHECK(yz_bottom->canonical_frame.has_value());

    kernel::Frame3 yz_bottom_expected;
    yz_bottom_expected.u_axis = {0.0, 1.0, 0.0};
    yz_bottom_expected.v_axis = {1.0, 0.0, 0.0};
    yz_bottom_expected.normal = {0.0, 0.0, -1.0};
    CHECK(
        near(
            *yz_bottom->canonical_frame,
            yz_bottom_expected));
}

void verifyCurvedAndHoleCarriers() {
    const auto circle =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            circleProfile(),
            10.0);
    verifyCommon(circle, 3U, 1U);
    const auto* cylinder =
        findSide(circle, "outer-circle");
    CHECK(cylinder != nullptr);
    CHECK(
        cylinder->semantic_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(
        cylinder->provider_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(!cylinder->canonical_frame.has_value());

    const auto mixed =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            mixedLineArcProfile(),
            10.0);
    verifyCommon(mixed, 4U, 2U);
    const auto* arc =
        findSide(mixed, "upper-arc");
    const auto* diameter =
        findSide(mixed, "diameter");
    CHECK(arc != nullptr);
    CHECK(diameter != nullptr);
    CHECK(
        arc->semantic_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(!arc->canonical_frame.has_value());
    CHECK(
        diameter->semantic_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(diameter->canonical_frame.has_value());

    kernel::Frame3 diameter_expected;
    diameter_expected.origin = {-10.0, 0.0, 0.0};
    diameter_expected.u_axis = {1.0, 0.0, 0.0};
    diameter_expected.v_axis = {0.0, 0.0, 1.0};
    diameter_expected.normal = {0.0, -1.0, 0.0};
    CHECK(
        near(
            *diameter->canonical_frame,
            diameter_expected));

    const auto holed =
        kernel_occt::buildExtrudeSurfaceCarrierEvidence(
            rectangleWithHole(),
            10.0);
    verifyCommon(holed, 7U, 5U);
    const auto* hole =
        findSide(holed, "hole-circle");
    CHECK(hole != nullptr);
    CHECK(hole->provenance.has_value());
    CHECK(hole->provenance->hole);
    CHECK(
        hole->semantic_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(
        hole->provider_surface_kind ==
        kernel::FaceSurfaceKind::cylinder);
    CHECK(!hole->canonical_frame.has_value());
}

} // namespace

int main() {
    verifyRectangleXY();
    verifyOtherOriginFrames();
    verifyCurvedAndHoleCarriers();

    std::cout
        << "PM02P_B1_SURFACE_CARRIERS_PASS"
        << " pristine_face_claims=complete"
        << " planar_frames=stable"
        << " curved_classification=explicit\n";
    return EXIT_SUCCESS;
}
