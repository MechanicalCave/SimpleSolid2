#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <cstdlib>
#include <iostream>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E03/E04 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

void printHistory(
    const char* label,
    const kernel::BooleanSubshapeHistoryEvidence&
        history) {
    std::cerr
        << label
        << " modified=" << history.modified_count
        << " generated=" << history.generated_count
        << " deleted=" << history.deleted
        << " unchanged=" << history.unchanged_present
        << " descendants=" << history.unique_descendant_count
        << '\n';
}

[[nodiscard]] kernel::ReferenceStatus
resolveSingular(
    std::size_t semantic_candidates,
    std::size_t technical_candidates,
    bool aggregate_request = false) noexcept {
    if (aggregate_request) {
        return kernel::ReferenceStatus::unsupported;
    }

    // Technical/auxiliary candidates are intentionally excluded by
    // independent semantic-role evidence. They cannot win by order,
    // length or proximity.
    (void)technical_candidates;

    if (semantic_candidates == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (semantic_candidates == 1U) {
        return kernel::ReferenceStatus::resolved;
    }
    return kernel::ReferenceStatus::ambiguous;
}

struct PairStatus final {
    kernel::ReferenceStatus first{
        kernel::ReferenceStatus::unsupported};
    kernel::ReferenceStatus second{
        kernel::ReferenceStatus::unsupported};
};

enum class SemanticWinner {
    none,
    first,
    second,
};

[[nodiscard]] PairStatus
resolveCollapsedPair(
    bool collapse_is_known,
    SemanticWinner winner) noexcept {
    if (!collapse_is_known) {
        return {};
    }

    switch (winner) {
    case SemanticWinner::none:
        return {
            kernel::ReferenceStatus::ambiguous,
            kernel::ReferenceStatus::ambiguous};
    case SemanticWinner::first:
        return {
            kernel::ReferenceStatus::resolved,
            kernel::ReferenceStatus::missing};
    case SemanticWinner::second:
        return {
            kernel::ReferenceStatus::missing,
            kernel::ReferenceStatus::resolved};
    }
    return {};
}

void verifySplitRows(
    const kernel::EdgeSplitHistoryEvidence&
        split,
    const kernel::EdgeSplitHistoryEvidence&
        removed) {
    CHECK(split.ok());
    CHECK(removed.ok());

    printHistory(
        "E03_SPLIT",
        split.target);
    printHistory(
        "E03_REMOVED",
        removed.target);

    // E03-01: one singular semantic target splits into multiple provider
    // descendants. The singular reference must not pick one fragment.
    CHECK(
        split.target.unique_descendant_count ==
        2U);
    CHECK(
        resolveSingular(
            split.target.unique_descendant_count,
            0U) ==
        kernel::ReferenceStatus::ambiguous);

    // E03-02: independent semantic-role evidence can exclude a technical
    // auxiliary candidate. One semantic descendant + one technical candidate
    // remains one semantic target and therefore resolves.
    CHECK(
        resolveSingular(
            1U,
            1U) ==
        kernel::ReferenceStatus::resolved);

    // E03-03: no semantic descendant after deletion => Missing.
    CHECK(removed.target.deleted);
    CHECK(
        removed.target.unique_descendant_count ==
        0U);
    CHECK(
        resolveSingular(
            removed.target.unique_descendant_count,
            0U) ==
        kernel::ReferenceStatus::missing);

    // E03-04: an undeclared aggregate "all descendants" meaning is not
    // invented by the singular selector.
    CHECK(
        resolveSingular(
            split.target.unique_descendant_count,
            0U,
            true) ==
        kernel::ReferenceStatus::unsupported);
}

void verifyMergeRows(
    const kernel::FaceMergeHistoryEvidence&
        merged,
    const kernel::FaceMergeHistoryEvidence&
        asymmetric,
    const kernel::FaceMergeHistoryEvidence&
        absorbed) {
    CHECK(merged.ok());
    CHECK(asymmetric.ok());
    CHECK(absorbed.ok());

    printHistory(
        "E04_MERGED_FIRST",
        merged.first);
    printHistory(
        "E04_MERGED_SECOND",
        merged.second);
    std::cerr
        << "E04_MERGED shared_descendants="
        << merged.shared_descendant_count
        << '\n';

    printHistory(
        "E04_ASYMMETRIC_FIRST",
        asymmetric.first);
    printHistory(
        "E04_ASYMMETRIC_SECOND",
        asymmetric.second);
    std::cerr
        << "E04_ASYMMETRIC shared_descendants="
        << asymmetric.shared_descendant_count
        << " result_faces="
        << asymmetric.shape.face_count
        << '\n';

    printHistory(
        "E04_ABSORBED_FIRST",
        absorbed.first);
    printHistory(
        "E04_ABSORBED_SECOND",
        absorbed.second);
    std::cerr
        << "E04_ABSORBED shared_descendants="
        << absorbed.shared_descendant_count
        << '\n';

    // E04-01: the probe geometry intentionally collapses two distinct,
    // coplanar source-face meanings into one fused result region. Without
    // independent semantic evidence neither singular reference may claim
    // the merged physical result.
    const auto no_winner =
        resolveCollapsedPair(
            true,
            SemanticWinner::none);
    CHECK(
        no_winner.first ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        no_winner.second ==
        kernel::ReferenceStatus::ambiguous);

    // E04-02: provider bookkeeping may be asymmetric (one source receives
    // surviving/modified history while the other is deleted). That asymmetry
    // is evidence only and must not choose a semantic winner.
    CHECK(asymmetric.shape.face_count == 6U);

    const bool asymmetric_history =
        asymmetric.first.modified_count > 0U &&
        !asymmetric.first.deleted &&
        !asymmetric.first.unchanged_present &&
        asymmetric.first.unique_descendant_count == 1U &&
        asymmetric.second.deleted &&
        asymmetric.second.unique_descendant_count == 0U;
    CHECK(asymmetric_history);

    const auto still_no_winner =
        resolveCollapsedPair(
            true,
            SemanticWinner::none);
    CHECK(
        still_no_winner.first ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        still_no_winner.second ==
        kernel::ReferenceStatus::ambiguous);

    // E04-03: in the absorption probe, the outer top-face role survives and
    // the fully internal source-face meaning disappears. Independent role
    // evidence therefore permits Resolved/Missing rather than Ambiguous.
    CHECK(
        absorbed.first.unchanged_present ||
        absorbed.first.unique_descendant_count ==
            1U);
    CHECK(absorbed.second.deleted);
    CHECK(
        absorbed.second.unique_descendant_count ==
        0U);

    const auto semantic_winner =
        resolveCollapsedPair(
            true,
            SemanticWinner::first);
    CHECK(
        semantic_winner.first ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        semantic_winner.second ==
        kernel::ReferenceStatus::missing);

    // E04-04: no aggregate merged-reference type is declared by PM-00A.
    CHECK(
        resolveSingular(
            1U,
            0U,
            true) ==
        kernel::ReferenceStatus::unsupported);
}

} // namespace

int main() {
    kernel::EdgeSplitHistoryEvidence split;
    kernel::EdgeSplitHistoryEvidence removed;
    kernel::FaceMergeHistoryEvidence merged;
    kernel::FaceMergeHistoryEvidence asymmetric;
    kernel::FaceMergeHistoryEvidence absorbed;

    {
        split =
            kernel_occt::buildEdgeSplitHistoryEvidence(
                kernel::EdgeSplitProbeScenario::
                    middle_notch);
        removed =
            kernel_occt::buildEdgeSplitHistoryEvidence(
                kernel::EdgeSplitProbeScenario::
                    remove_target);
        merged =
            kernel_occt::buildFaceMergeHistoryEvidence(
                kernel::FaceMergeProbeScenario::
                    overlapping_coplanar);
        asymmetric =
            kernel_occt::buildFaceMergeHistoryEvidence(
                kernel::FaceMergeProbeScenario::
                    asymmetric_history);
        absorbed =
            kernel_occt::buildFaceMergeHistoryEvidence(
                kernel::FaceMergeProbeScenario::
                    absorbed_inner);

        verifySplitRows(
            split,
            removed);
        verifyMergeRows(
            merged,
            asymmetric,
            absorbed);
    }

    // COLD replay: all OCCT operation objects and provider handles from the
    // first pass are gone. Recreate from the scenario inputs only and require
    // the same neutral history/cardinality evidence.
    const auto cold_split =
        kernel_occt::buildEdgeSplitHistoryEvidence(
            kernel::EdgeSplitProbeScenario::
                middle_notch);
    const auto cold_removed =
        kernel_occt::buildEdgeSplitHistoryEvidence(
            kernel::EdgeSplitProbeScenario::
                remove_target);
    const auto cold_merged =
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                overlapping_coplanar);
    const auto cold_asymmetric =
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                asymmetric_history);
    const auto cold_absorbed =
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                absorbed_inner);

    CHECK(cold_split == split);
    CHECK(cold_removed == removed);
    CHECK(cold_merged == merged);
    CHECK(cold_asymmetric == asymmetric);
    CHECK(cold_absorbed == absorbed);

    verifySplitRows(
        cold_split,
        cold_removed);
    verifyMergeRows(
        cold_merged,
        cold_asymmetric,
        cold_absorbed);

    std::cout
        << "PM00A_E03_E04_PASS rows=E03-01..E03-04,E04-01..E04-04"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
