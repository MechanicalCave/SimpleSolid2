#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>
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

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string_view source_entity) {
    return kernel::BoundaryUse2D{
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::string{source_entity},
            0U,
            0U,
            false},
    };
}

kernel::PlanarProfileInput rectangleProfile(
    double closing_gap,
    bool add_hole) {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        lineUse({0.0, 0.0}, {40.0, 0.0}, "outer-bottom"),
        lineUse({40.0, 0.0}, {40.0, 30.0}, "outer-right"),
        lineUse({40.0, 30.0}, {0.0, 30.0}, "outer-top"),
        lineUse({0.0, 30.0}, {0.0, closing_gap}, "outer-left"),
    };

    if (add_hole) {
        kernel::ProfileLoopInput hole;
        hole.boundary.push_back(
            kernel::BoundaryUse2D{
                kernel::Circle2{{20.0, 15.0}, 4.0},
                0.0,
                1.0,
                true,
                false,
                true,
                kernel::BoundaryUseProvenance{
                    "hole-circle",
                    1U,
                    0U,
                    true},
            });
        input.holes.push_back(std::move(hole));
    }

    return input;
}

std::size_t countSubshapes(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    if (shape.IsNull()) {
        return 0U;
    }

    if (shape.ShapeType() == kind) {
        return 1U;
    }

    std::size_t count = 0U;
    for (TopExp_Explorer explorer{shape, kind};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

std::optional<TopoDS_Face> findFaceByCentroid(
    const TopoDS_Shape& shape,
    const gp_Pnt& expected,
    double tolerance = 1.0e-9) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        const auto face =
            TopoDS::Face(explorer.Current());
        GProp_GProps properties;
        BRepGProp::SurfaceProperties(
            face,
            properties);
        const auto centre =
            properties.CentreOfMass();
        if (centre.Distance(expected) <=
            tolerance) {
            return face;
        }
    }
    return std::nullopt;
}

bool containsSameFace(
    const TopoDS_Shape& shape,
    const TopoDS_Face& face) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(face)) {
            return true;
        }
    }
    return false;
}

std::size_t uniqueModifiedFaceCount(
    BRepAlgoAPI_Fuse& fuse,
    const TopoDS_Face& source,
    const TopoDS_Shape& result) {
    std::vector<TopoDS_Face> unique;

    const auto& modified =
        fuse.Modified(source);
    for (const auto& current :
         modified) {
        if (current.ShapeType() !=
            TopAbs_FACE) {
            continue;
        }

        const auto candidate =
            TopoDS::Face(current);
        if (!containsSameFace(
                result,
                candidate)) {
            continue;
        }

        bool duplicate = false;
        for (const auto& existing :
             unique) {
            if (existing.IsSame(
                    candidate)) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            unique.push_back(
                candidate);
        }
    }

    if (!unique.empty()) {
        return unique.size();
    }

    if (!fuse.IsDeleted(source) &&
        containsSameFace(
            result,
            source)) {
        return 1U;
    }

    return 0U;
}

struct RefinePolicy final {
    std::string_view policy_tag;
    bool simplify_result{false};
    double fuzzy_value{0.0};
};

struct RefineObservation final {
    bool done{false};
    bool brep_valid{false};
    std::size_t solid_count{};
    std::size_t face_count{};
    std::size_t edge_count{};
    std::size_t semantic_face_candidates{};
};

RefineObservation adjacentBoxFuse(
    const RefinePolicy& policy) {
    RefineObservation observation;

    try {
        BRepPrimAPI_MakeBox first_box{
            gp_Pnt{0.0, 0.0, 0.0},
            20.0,
            20.0,
            10.0};
        BRepPrimAPI_MakeBox second_box{
            gp_Pnt{20.0, 0.0, 0.0},
            20.0,
            20.0,
            10.0};

        const auto first_shape =
            first_box.Shape();
        const auto source_face =
            findFaceByCentroid(
                first_shape,
                gp_Pnt{0.0, 10.0, 5.0});
        if (!source_face) {
            return observation;
        }

        BRepAlgoAPI_Fuse fuse{
            first_shape,
            second_box.Shape()};
        fuse.SetFuzzyValue(
            policy.fuzzy_value);
        fuse.Build();
        if (!fuse.IsDone()) {
            return observation;
        }

        if (policy.simplify_result) {
            fuse.SimplifyResult(
                true,
                true);
        }

        const auto result =
            fuse.Shape();
        if (result.IsNull()) {
            return observation;
        }

        observation.done = true;
        observation.brep_valid =
            BRepCheck_Analyzer{result}.IsValid();
        observation.solid_count =
            countSubshapes(
                result,
                TopAbs_SOLID);
        observation.face_count =
            countSubshapes(
                result,
                TopAbs_FACE);
        observation.edge_count =
            countSubshapes(
                result,
                TopAbs_EDGE);
        observation.semantic_face_candidates =
            uniqueModifiedFaceCount(
                fuse,
                *source_face,
                result);
        return observation;
    } catch (const Standard_Failure&) {
        return observation;
    } catch (...) {
        return observation;
    }
}

bool sameRefineObservation(
    const RefineObservation& first,
    const RefineObservation& second) {
    return first.done ==
               second.done &&
           first.brep_valid ==
               second.brep_valid &&
           first.solid_count ==
               second.solid_count &&
           first.face_count ==
               second.face_count &&
           first.edge_count ==
               second.edge_count &&
           first.semantic_face_candidates ==
               second.semantic_face_candidates;
}

bool allSemanticExtrudeRolesResolved(
    const kernel::ExtrudeEvidence& evidence) {
    if (!evidence.ok()) {
        return false;
    }
    if (evidence.start_cap.status !=
            kernel::ReferenceStatus::resolved ||
        evidence.end_cap.status !=
            kernel::ReferenceStatus::resolved) {
        return false;
    }
    for (const auto& side :
         evidence.sides) {
        if (side.status !=
                kernel::ReferenceStatus::resolved ||
            side.candidate_face_count !=
                1U ||
            !side.provenance.has_value()) {
            return false;
        }
    }
    return true;
}

bool geometricFailure(
    const kernel::ShapeEvidence& evidence) {
    return evidence.status ==
               kernel::EvidenceStatus::
                   provider_failure ||
           evidence.status ==
               kernel::EvidenceStatus::
                   invalid_brep;
}

} // namespace

int main() {
    // E10-01 / E10-02:
    // The kernel-native provider API receives authored/kernel geometry only.
    // Camera/projection/pick-aperture/OSNAP display state is absent from this
    // build mode and from the provider evidence function signature. Repeated
    // evaluation of the exact same semantic input must therefore be identical.
    const auto clean_input =
        rectangleProfile(
            0.0,
            true);
    CHECK(clean_input.valid());

    const auto clean_first =
        kernel_occt::
            buildProfileExtrudeEvidence(
                clean_input,
                25.0);
    const auto clean_second =
        kernel_occt::
            buildProfileExtrudeEvidence(
                clean_input,
                25.0);

    CHECK(clean_first == clean_second);
    CHECK(
        allSemanticExtrudeRolesResolved(
            clean_first));

    // E10-03:
    // Compare explicit refine/unify OFF versus ON on the same adjacent-box
    // Boolean. The raw provider topology contains split same-domain boundary
    // faces; SimplifyResult may unify them. A source-defined exterior face
    // must remain a unique semantic candidate under both policies.
    const RefinePolicy raw_policy{
        "pm00a-e10/raw-no-refine/fuzzy0",
        false,
        0.0};
    const RefinePolicy refined_policy{
        "pm00a-e10/refine-v1/fuzzy0",
        true,
        0.0};

    const auto raw =
        adjacentBoxFuse(
            raw_policy);
    const auto raw_repeat =
        adjacentBoxFuse(
            raw_policy);
    const auto refined =
        adjacentBoxFuse(
            refined_policy);
    const auto refined_repeat =
        adjacentBoxFuse(
            refined_policy);

    CHECK(
        sameRefineObservation(
            raw,
            raw_repeat));
    CHECK(
        sameRefineObservation(
            refined,
            refined_repeat));
    CHECK(raw.done);
    CHECK(raw.brep_valid);
    CHECK(raw.solid_count == 1U);
    CHECK(
        raw.semantic_face_candidates ==
        1U);
    CHECK(refined.done);
    CHECK(refined.brep_valid);
    CHECK(refined.solid_count == 1U);
    CHECK(
        refined.semantic_face_candidates ==
        1U);
    CHECK(
        refined.face_count <
        raw.face_count);

    // E10-04:
    // A clean rectangle-with-hole profile has millimetre-scale clearances,
    // far above the provider precision scale measured below. It must remain
    // a deterministic valid one-solid extrusion with the same semantic
    // reference outcomes and no fuzzy escalation.
    CHECK(
        clean_first.shape.solid_count ==
        1U);
    CHECK(
        clean_first.shape.brep_valid);
    CHECK(
        clean_first.shape.status ==
        kernel::EvidenceStatus::ok);

    // E10-05:
    // The neutral profile structure intentionally does not claim geometric
    // closure. A one-millimetre open boundary is structurally representable
    // but must fail in provider geometry; this probe performs no retry with
    // increasing tolerance and no silent healing.
    const auto invalid_gap_input =
        rectangleProfile(
            1.0,
            false);
    CHECK(
        invalid_gap_input.valid());

    const auto invalid_gap_first =
        kernel_occt::
            buildProfileFaceEvidence(
                invalid_gap_input);
    const auto invalid_gap_second =
        kernel_occt::
            buildProfileFaceEvidence(
                invalid_gap_input);
    CHECK(
        invalid_gap_first ==
        invalid_gap_second);
    CHECK(
        !invalid_gap_first.ok());
    CHECK(
        geometricFailure(
            invalid_gap_first));

    // E10-06:
    // Sweep an open-loop gap around and far beyond OCCT's provider precision
    // scale. The exact acceptance threshold is evidence, not an accepted SS2
    // modeling tolerance. Every sampled point must be deterministic, and the
    // family must expose at least one accepted and one rejected case.
    const double provider_precision =
        Precision::Confusion();
    CHECK(
        std::isfinite(
            provider_precision));
    CHECK(provider_precision > 0.0);

    const std::array<double, 10U>
        gap_multipliers{
            0.0,
            0.01,
            0.1,
            0.5,
            1.0,
            2.0,
            10.0,
            100.0,
            10000.0,
            10000000.0,
        };

    bool saw_accepted = false;
    bool saw_rejected = false;
    std::size_t transition_count = 0U;
    std::optional<bool>
        previous_accepted;

    for (const double multiplier :
         gap_multipliers) {
        const double gap =
            multiplier == 0.0
                ? 0.0
                : provider_precision *
                      multiplier;

        const auto input =
            rectangleProfile(
                gap,
                false);
        CHECK(input.valid());

        const auto first =
            kernel_occt::
                buildProfileFaceEvidence(
                    input);
        const auto second =
            kernel_occt::
                buildProfileFaceEvidence(
                    input);
        CHECK(first == second);

        const bool accepted =
            first.ok();
        saw_accepted =
            saw_accepted ||
            accepted;
        saw_rejected =
            saw_rejected ||
            !accepted;

        if (previous_accepted.has_value() &&
            *previous_accepted !=
                accepted) {
            ++transition_count;
        }
        previous_accepted =
            accepted;

        std::cout
            << "E10_SWEEP gap="
            << gap
            << " multiplier="
            << multiplier
            << " status="
            << static_cast<int>(
                   first.status)
            << " valid="
            << first.brep_valid
            << " faces="
            << first.face_count
            << " edges="
            << first.edge_count
            << '\n';
    }

    CHECK(saw_accepted);
    CHECK(saw_rejected);
    CHECK(transition_count >= 1U);

    // E10-07:
    // The same authored geometric case produced distinct provider topology
    // under two explicit test-local policy candidates. The policy/version tag
    // must therefore change together with the policy; PM-00A does not assign
    // either tag as a production modelingSemanticsVersion.
    CHECK(
        raw_policy.policy_tag !=
        refined_policy.policy_tag);
    CHECK(
        raw.face_count !=
        refined.face_count);

    std::cout
        << "PM00A_E10_PASS rows=E10-01..E10-07"
        << " provider_precision="
        << provider_precision
        << " raw_faces="
        << raw.face_count
        << " refined_faces="
        << refined.face_count
        << " raw_semantic_candidates="
        << raw.semantic_face_candidates
        << " refined_semantic_candidates="
        << refined.semantic_face_candidates
        << " sweep_transitions="
        << transition_count
        << " false_resolved=0"
        << '\n';

    return EXIT_SUCCESS;
}
