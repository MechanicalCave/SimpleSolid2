#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.B2 Boolean Surface lineage CHECK failed at line "
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

kernel::Frame3 topFrame(double z) {
    kernel::Frame3 frame;
    frame.origin = {0.0, 0.0, z};
    return frame;
}

void verifyAttachedAddTrim() {
    const auto evidence =
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::
                attached_add_trim);

    CHECK(evidence.ok());
    CHECK(evidence.source_history.modified_count > 0U);
    CHECK(!evidence.source_history.deleted);
    CHECK(
        evidence.source_history.unique_descendant_count ==
        1U);
    CHECK(
        evidence.strict_face_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.current_face_realization_count == 1U);
    CHECK(evidence.planar_realization_count == 1U);
    CHECK(evidence.canonical_frame.has_value());
    CHECK(near(*evidence.canonical_frame, topFrame(10.0)));
}

void verifyCutTrim() {
    const auto evidence =
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::
                cut_trim);

    CHECK(evidence.ok());
    CHECK(evidence.source_history.modified_count > 0U);
    CHECK(!evidence.source_history.deleted);
    CHECK(
        evidence.source_history.unique_descendant_count ==
        1U);
    CHECK(
        evidence.strict_face_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.current_face_realization_count == 1U);
    CHECK(evidence.planar_realization_count == 1U);
    CHECK(evidence.canonical_frame.has_value());
    CHECK(near(*evidence.canonical_frame, topFrame(10.0)));
}

void verifySplitFaceCarrierSurvives() {
    const auto evidence =
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::
                cut_split);

    CHECK(evidence.ok());
    CHECK(evidence.source_history.modified_count > 0U);
    CHECK(!evidence.source_history.deleted);
    CHECK(
        evidence.source_history.unique_descendant_count ==
        2U);
    CHECK(
        evidence.strict_face_status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        evidence.surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.current_face_realization_count == 2U);
    CHECK(evidence.planar_realization_count == 2U);
    CHECK(evidence.canonical_frame.has_value());
    CHECK(near(*evidence.canonical_frame, topFrame(10.0)));
}

void verifyDeleteAndIdenticalRecreate() {
    const auto evidence =
        kernel_occt::buildSurfaceDeleteRecreateEvidence();

    CHECK(evidence.ok());
    CHECK(evidence.delete_history.deleted);
    CHECK(
        evidence.delete_history.unique_descendant_count ==
        0U);
    CHECK(
        evidence.old_surface_after_delete ==
        kernel::ReferenceStatus::missing);
    CHECK(
        evidence.old_surface_after_recreate ==
        kernel::ReferenceStatus::missing);

    CHECK(
        evidence.replacement_surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.replacement_face_count == 1U);
    CHECK(evidence.replacement_geometry_matches_old);

    CHECK(evidence.old_canonical_frame.has_value());
    CHECK(evidence.replacement_canonical_frame.has_value());
    CHECK(
        near(
            *evidence.old_canonical_frame,
            *evidence.replacement_canonical_frame));

    // Same provider geometry and even the same canonical plane frame are
    // insufficient to steal the deleted semantic meaning.
    CHECK(
        evidence.old_surface_after_recreate !=
        evidence.replacement_surface_status);
}

struct AliasPair final {
    kernel::ReferenceStatus first{
        kernel::ReferenceStatus::unsupported};
    kernel::ReferenceStatus second{
        kernel::ReferenceStatus::unsupported};
};

AliasPair resolveAliasedSurfaceClaims(
    const kernel::FaceMergeHistoryEvidence& evidence,
    bool independent_first_winner) {
    if (independent_first_winner) {
        return {
            kernel::ReferenceStatus::resolved,
            kernel::ReferenceStatus::missing};
    }

    if (evidence.shared_descendant_count > 0U ||
        (evidence.first.unique_descendant_count == 1U &&
         evidence.second.unique_descendant_count == 0U &&
         evidence.second.deleted)) {
        // Provider history asymmetry cannot choose a semantic winner when
        // the fixture declares a collapse/alias relation.
        return {
            kernel::ReferenceStatus::ambiguous,
            kernel::ReferenceStatus::ambiguous};
    }

    return {};
}

void verifyAliasMerge() {
    const auto merged =
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                overlapping_coplanar);
    CHECK(merged.ok());
    CHECK(merged.shared_descendant_count > 0U);

    const auto no_winner =
        resolveAliasedSurfaceClaims(
            merged,
            false);
    CHECK(
        no_winner.first ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        no_winner.second ==
        kernel::ReferenceStatus::ambiguous);

    const auto asymmetric =
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                asymmetric_history);
    CHECK(asymmetric.ok());
    CHECK(asymmetric.first.modified_count > 0U);
    CHECK(asymmetric.second.deleted);

    const auto still_no_winner =
        resolveAliasedSurfaceClaims(
            asymmetric,
            false);
    CHECK(
        still_no_winner.first ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        still_no_winner.second ==
        kernel::ReferenceStatus::ambiguous);

    const auto absorbed =
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                absorbed_inner);
    CHECK(absorbed.ok());
    CHECK(absorbed.second.deleted);

    const auto independent_winner =
        resolveAliasedSurfaceClaims(
            absorbed,
            true);
    CHECK(
        independent_winner.first ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        independent_winner.second ==
        kernel::ReferenceStatus::missing);
}

void verifyCutExposedSurface() {
    const auto evidence =
        kernel_occt::buildCutExposedSurfaceEvidence();

    CHECK(evidence.ok());
    CHECK(evidence.result_topology.complete());
    CHECK(
        evidence.tool_surface_history
            .unique_descendant_count ==
        1U);
    CHECK(
        evidence.strict_face_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.current_face_realization_count == 1U);
    CHECK(
        evidence.semantic_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(
        evidence.provider_surface_kind ==
        kernel::FaceSurfaceKind::plane);
    CHECK(evidence.provenance.has_value());
    CHECK(
        evidence.provenance->source_entity ==
        "cut-tool-bottom");
    CHECK(evidence.canonical_frame.has_value());

    kernel::Frame3 expected;
    expected.origin = {10.0, 10.0, 5.0};
    expected.u_axis = {1.0, 0.0, 0.0};
    expected.v_axis = {0.0, 0.0, 1.0};
    expected.normal = {0.0, -1.0, 0.0};
    CHECK(near(*evidence.canonical_frame, expected));
}

struct EvidenceSet final {
    kernel::SurfaceBooleanLineageEvidence add_trim;
    kernel::SurfaceBooleanLineageEvidence cut_trim;
    kernel::SurfaceBooleanLineageEvidence split;
    kernel::SurfaceDeleteRecreateEvidence recreate;
    kernel::FaceMergeHistoryEvidence merged;
    kernel::FaceMergeHistoryEvidence asymmetric;
    kernel::FaceMergeHistoryEvidence absorbed;
    kernel::CutExposedSurfaceEvidence cut_exposed;

    friend bool operator==(
        const EvidenceSet&,
        const EvidenceSet&) = default;
};

EvidenceSet evaluate() {
    return {
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::
                attached_add_trim),
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::
                cut_trim),
        kernel_occt::buildSurfaceBooleanLineageEvidence(
            kernel::SurfaceBooleanProbeScenario::
                cut_split),
        kernel_occt::buildSurfaceDeleteRecreateEvidence(),
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                overlapping_coplanar),
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                asymmetric_history),
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                absorbed_inner),
        kernel_occt::buildCutExposedSurfaceEvidence(),
    };
}

} // namespace

int main() {
    verifyAttachedAddTrim();
    verifyCutTrim();
    verifySplitFaceCarrierSurvives();
    verifyDeleteAndIdenticalRecreate();
    verifyAliasMerge();
    verifyCutExposedSurface();

    // Cold provider reconstruction: no TopoDS object from the first pass is
    // retained. Semantic outcomes and provenance-derived frames must repeat.
    const auto first = evaluate();
    const auto cold = evaluate();
    CHECK(cold == first);

    std::cout
        << "PM02P_B2_BOOLEAN_SURFACE_PASS"
        << " false_resolved=0"
        << " split_face=ambiguous"
        << " split_surface=resolved"
        << " deleted_surface=missing"
        << " cut_exposed_surface=resolved\n";
    return EXIT_SUCCESS;
}
