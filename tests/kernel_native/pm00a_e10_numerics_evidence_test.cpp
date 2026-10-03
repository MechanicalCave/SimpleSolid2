#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E10 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct ShapeSummary final {
    bool provider_done{};
    bool brep_valid{};
    std::size_t solid_count{};
    std::size_t face_count{};
    std::size_t edge_count{};

    friend bool operator==(
        const ShapeSummary&,
        const ShapeSummary&) = default;
};

struct EvidencePolicy final {
    std::string_view version;
    double fuzzy_tolerance{};
    bool simplify_result{};
};

struct PresentationSettings final {
    double zoom{};
    double pick_aperture_pixels{};
    bool osnap_glyphs{};
    int projection_mode{};
};

using ExtrudeEvidenceFunction =
    kernel::ExtrudeEvidence (*)(
        const kernel::PlanarProfileInput&,
        double) noexcept;

static_assert(
    std::is_same_v<
        decltype(
            &kernel_occt::
                buildProfileExtrudeEvidence),
        ExtrudeEvidenceFunction>);

[[nodiscard]] std::size_t countSubshapes(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    std::size_t count = 0U;
    for (TopExp_Explorer explorer{shape, kind};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

[[nodiscard]] ShapeSummary fuseSeparatedBoxes(
    double gap,
    const EvidencePolicy& policy) noexcept {
    ShapeSummary result;
    if (!std::isfinite(gap) || gap < 0.0 ||
        !std::isfinite(policy.fuzzy_tolerance) ||
        policy.fuzzy_tolerance < 0.0) {
        return result;
    }

    try {
        BRepPrimAPI_MakeBox left{
            gp_Pnt{0.0, 0.0, 0.0},
            10.0,
            10.0,
            10.0};
        BRepPrimAPI_MakeBox right{
            gp_Pnt{10.0 + gap, 0.0, 0.0},
            10.0,
            10.0,
            10.0};

        BRepAlgoAPI_Fuse fuse{
            left.Shape(),
            right.Shape()};
        fuse.SetFuzzyValue(
            policy.fuzzy_tolerance);
        fuse.Build();
        if (!fuse.IsDone()) {
            return result;
        }

        if (policy.simplify_result) {
            fuse.SimplifyResult(
                true,
                true);
        }

        const auto shape = fuse.Shape();
        if (shape.IsNull()) {
            return result;
        }

        result.provider_done = true;
        const BRepCheck_Analyzer analyzer{shape};
        result.brep_valid =
            analyzer.IsValid();
        result.solid_count =
            countSubshapes(
                shape,
                TopAbs_SOLID);
        result.face_count =
            countSubshapes(
                shape,
                TopAbs_FACE);
        result.edge_count =
            countSubshapes(
                shape,
                TopAbs_EDGE);
        return result;
    } catch (const Standard_Failure&) {
        return result;
    } catch (...) {
        return result;
    }
}

[[nodiscard]] kernel::BoundaryUse2D lineUse(
    std::string_view source,
    std::uint32_t index,
    kernel::Point2 start,
    kernel::Point2 end) {
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
            index,
            false},
    };
}

[[nodiscard]] kernel::PlanarProfileInput
rectangleWithClosingGap(double gap) {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        lineUse(
            "bottom",
            0U,
            {0.0, 0.0},
            {10.0, 0.0}),
        lineUse(
            "right",
            1U,
            {10.0, 0.0},
            {10.0, 10.0}),
        lineUse(
            "top",
            2U,
            {10.0, 10.0},
            {0.0, 10.0}),
        lineUse(
            "left",
            3U,
            {0.0, 10.0},
            {0.0, gap}),
    };
    return input;
}

[[nodiscard]] kernel::ExtrudeEvidence
evaluateUnderPresentation(
    const kernel::PlanarProfileInput& input,
    const PresentationSettings& presentation) {
    // Deliberately not forwarded to the Kernel provider. E10-01/02 test
    // that camera, projection, pick aperture and OSNAP display state are
    // outside modeling input.
    (void)presentation;
    return kernel_occt::buildProfileExtrudeEvidence(
        input,
        10.0);
}

void printSummary(
    std::string_view prefix,
    double gap,
    const EvidencePolicy& policy,
    const ShapeSummary& value) {
    std::cerr
        << std::setprecision(17)
        << prefix
        << " gap=" << gap
        << " policy=" << policy.version
        << " fuzzy=" << policy.fuzzy_tolerance
        << " simplify=" << policy.simplify_result
        << " done=" << value.provider_done
        << " valid=" << value.brep_valid
        << " solids=" << value.solid_count
        << " faces=" << value.face_count
        << " edges=" << value.edge_count
        << '\n';
}

void printProfile(
    double gap,
    const kernel::ShapeEvidence& evidence) {
    std::cerr
        << std::setprecision(17)
        << "E10_PROFILE_GAP"
        << " gap=" << gap
        << " input_valid=1"
        << " status="
        << static_cast<int>(evidence.status)
        << " brep_valid=" << evidence.brep_valid
        << " faces=" << evidence.face_count
        << " wires=" << evidence.wire_count
        << " edges=" << evidence.edge_count
        << '\n';
}

} // namespace

int main() {
    constexpr EvidencePolicy exact_no_refine{
        "e10-measure-exact-v0",
        0.0,
        false};
    constexpr EvidencePolicy exact_refine{
        "e10-measure-refine-v1",
        0.0,
        true};

    // E10-01 / E10-02: presentation state is not a modeling input.
    const auto exact_profile =
        rectangleWithClosingGap(0.0);
    CHECK(exact_profile.valid());

    const std::array<PresentationSettings, 4U>
        presentation_variants{
            PresentationSettings{
                0.01,
                1.0,
                false,
                0},
            PresentationSettings{
                1000000.0,
                64.0,
                true,
                1},
            PresentationSettings{
                3.25,
                7.0,
                true,
                2},
            PresentationSettings{
                0.5,
                0.25,
                false,
                3},
        };

    const auto presentation_baseline =
        evaluateUnderPresentation(
            exact_profile,
            presentation_variants.front());
    CHECK(presentation_baseline.ok());
    for (const auto& settings :
         presentation_variants) {
        CHECK(
            evaluateUnderPresentation(
                exact_profile,
                settings) ==
            presentation_baseline);
    }

    // E10-03 / E10-07 measurement: make provider refinement and fuzzy policy
    // explicit. Adjacent boxes are the same authored/evidence geometry under
    // two named policies; topology counts may change, but valid one-solid
    // semantics must remain observable without a hidden default.
    const auto adjacent_raw =
        fuseSeparatedBoxes(
            0.0,
            exact_no_refine);
    const auto adjacent_refined =
        fuseSeparatedBoxes(
            0.0,
            exact_refine);
    printSummary(
        "E10_REFINE",
        0.0,
        exact_no_refine,
        adjacent_raw);
    printSummary(
        "E10_REFINE",
        0.0,
        exact_refine,
        adjacent_refined);
    CHECK(adjacent_raw.provider_done);
    CHECK(adjacent_raw.brep_valid);
    CHECK(adjacent_raw.solid_count == 1U);
    CHECK(adjacent_refined.provider_done);
    CHECK(adjacent_refined.brep_valid);
    CHECK(adjacent_refined.solid_count == 1U);
    CHECK(
        adjacent_raw.face_count !=
            adjacent_refined.face_count ||
        adjacent_raw.edge_count !=
            adjacent_refined.edge_count);
    CHECK(
        exact_no_refine.version !=
        exact_refine.version);

    // E10-04: an ordinary valid Profile/Extrude is accepted with the exact
    // provider path; no fuzzy escalation is needed.
    CHECK(
        presentation_baseline.shape.ok());
    CHECK(
        presentation_baseline.shape.solid_count ==
        1U);

    // E10-05/E10-06 measurement family. PlanarProfileInput remains
    // structurally valid for every non-zero closing gap, so any transition
    // below is geometric/provider executability rather than input parsing.
    constexpr std::array<double, 11U>
        profile_gaps{
            0.0,
            1.0e-12,
            1.0e-11,
            1.0e-10,
            1.0e-9,
            1.0e-8,
            1.0e-7,
            1.0e-6,
            1.0e-5,
            1.0e-4,
            1.0e-3,
        };

    constexpr double
        measured_provider_profile_limit =
            1.0e-7;

    for (const auto gap : profile_gaps) {
        const auto input =
            rectangleWithClosingGap(gap);
        CHECK(input.valid());
        const auto first =
            kernel_occt::buildProfileFaceEvidence(
                input);
        const auto second =
            kernel_occt::buildProfileFaceEvidence(
                input);
        CHECK(first == second);
        printProfile(gap, first);

        if (gap <=
            measured_provider_profile_limit) {
            CHECK(first.ok());
            CHECK(first.face_count == 1U);
            CHECK(first.wire_count == 1U);
            CHECK(first.edge_count == 4U);
        } else {
            CHECK(
                first.status ==
                kernel::EvidenceStatus::
                    invalid_brep);
            CHECK(!first.brep_valid);
        }
    }

    // E10-05: a clearly non-closed semantic boundary is not rescued by
    // escalating tolerance. The provider returns invalid B-Rep evidence.
    const auto clearly_invalid =
        kernel_occt::buildProfileFaceEvidence(
            rectangleWithClosingGap(
                1.0e-3));
    CHECK(
        clearly_invalid.status ==
        kernel::EvidenceStatus::
            invalid_brep);
    CHECK(!clearly_invalid.brep_valid);

    // Explicit Boolean fuzzy-policy sweep. This is measurement only: the
    // sampled values are candidates, not accepted Product tolerances.
    constexpr std::array<double, 9U>
        boolean_gaps{
            0.0,
            1.0e-10,
            1.0e-9,
            1.0e-8,
            1.0e-7,
            1.0e-6,
            1.0e-5,
            1.0e-4,
            1.0e-3,
        };
    constexpr std::array<double, 6U>
        fuzzy_candidates{
            0.0,
            1.0e-9,
            1.0e-8,
            1.0e-7,
            1.0e-6,
            1.0e-5,
        };

    constexpr double
        measured_provider_boolean_limit =
            1.0e-7;

    for (const auto fuzzy :
         fuzzy_candidates) {
        const EvidencePolicy policy{
            fuzzy == 0.0
                ? "e10-policy-no-added-fuzzy"
                : "e10-policy-explicit-fuzzy-candidate",
            fuzzy,
            false};
        const double observed_join_limit =
            std::max(
                measured_provider_boolean_limit,
                fuzzy);

        for (const auto gap :
             boolean_gaps) {
            const auto first =
                fuseSeparatedBoxes(
                    gap,
                    policy);
            const auto second =
                fuseSeparatedBoxes(
                    gap,
                    policy);
            CHECK(first == second);
            printSummary(
                "E10_FUZZY",
                gap,
                policy,
                first);

            CHECK(first.provider_done);
            CHECK(first.brep_valid);
            const std::size_t
                expected_solid_count =
                    gap <= observed_join_limit
                        ? 1U
                        : 2U;
            CHECK(
                first.solid_count ==
                expected_solid_count);
        }
    }

    // E10-07: policy identity is explicit in the evidence. The same authored
    // geometry under a changed refine/fuzzy candidate is never described as
    // the same unnamed modeling policy.
    CHECK(
        exact_no_refine.version ==
        std::string_view{
            "e10-measure-exact-v0"});
    CHECK(
        exact_refine.version ==
        std::string_view{
            "e10-measure-refine-v1"});

    std::cout
        << "PM00A_E10_PASS rows=E10-01..E10-07"
        << " provider_profile_transition=1e-7_to_1e-6"
        << " provider_boolean_transition=1e-7_to_1e-6"
        << " ss2_added_fuzzy=0"
        << " healing=none"
        << '\n';
    return EXIT_SUCCESS;
}
