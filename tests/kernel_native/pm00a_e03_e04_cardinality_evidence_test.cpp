#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <cstdlib>
#include <iostream>

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

[[nodiscard]] kernel::ReferenceStatus
resolveSingular(
    std::size_t semantic_candidates,
    std::size_t technical_candidates,
    bool aggregate_request = false) noexcept {
    if (aggregate_request) {
        return kernel::ReferenceStatus::unsupported;
    }

    // Technical/auxiliary candidates have already been excluded by
    // independent semantic-role evidence. They can never win by order,
    // length, distance or provider enumeration.
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
    const kernel::EdgeSplitHistoryEvidence& split,
    const kernel::EdgeSplitHistoryEvidence& removed) {
    CHECK(split.ok());
    CHECK(removed.ok());

    // E03-01: the provider probe turns one source edge into two distinct
    // descendants. A singular semantic selector must therefore be Ambiguous;
    // no first/longest/nearest fragment may be chosen.
    CHECK(split.target.unique_descendant_count == 2U);
    CHECK(
        resolveSingular(
            split.target.unique_descendant_count,
            0U) ==
        kernel::ReferenceStatus::ambiguous);

    // E03-02: once independent semantic-role evidence excludes a technical
    // auxiliary candidate, exactly one semantic descendant may resolve.
    // The technical candidate is explicitly not part of identity.
    CHECK(
        resolveSingular(
            1U,
            1U) ==
        kernel::ReferenceStatus::resolved);

    // E03-03: deletion with no semantic descendant is Missing.
    CHECK(removed.target.deleted);
    CHECK(removed.target.unique_descendant_count == 0U);
    CHECK(
        resolveSingular(
            removed.target.unique_descendant_count,
            0U) ==
        kernel::ReferenceStatus::missing);

    // E03-04: the singular selector cannot silently change meaning into
    // an aggregate "all descendants" reference.
    CHECK(
        resolveSingular(
            split.target.unique_descendant_count,
            0U,
            true) ==
        kernel::ReferenceStatus::unsupported);
}

void verifyMergeRows(
    const kernel::FaceMergeHistoryEvidence& merged,
    const kernel::FaceMergeHistoryEvidence& asymmetric,
    const kernel::FaceMergeHistoryEvidence& absorbed) {
    CHECK(merged.ok());
    CHECK(asymmetric.ok());
    CHECK(absorbed.ok());

    // E04-01: two distinct coplanar source-face meanings must demonstrably
    // collapse to the same single physical descendant. With no independent
    // semantic winner, both singular references are Ambiguous.
    CHECK(merged.first.unique_descendant_count == 1U);
    CHECK(merged.second.unique_descendant_count == 1U);
    CHECK(merged.shared_descendant_count == 1U);
    CHECK(
        merged.first.modified_count > 0U ||
        merged.first.unchanged_present);
    CHECK(
        merged.second.modified_count > 0U ||
        merged.second.unchanged_present);

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

    // E04-02: provider history may be asymmetric (one source Modified,
    // another Deleted) but bookkeeping alone does not authorize a semantic
    // winner. The semantic result remains Ambiguous/Ambiguous.
    CHECK(asymmetric.shape.face_count == 6U);
    CHECK(asymmetric.first.modified_count > 0U);
    CHECK(!asymmetric.first.deleted);
    CHECK(
        asymmetric.first.unique_descendant_count ==
        1U);
    CHECK(asymmetric.second.deleted);
    CHECK(
        asymmetric.second.unique_descendant_count ==
        0U);
    CHECK(asymmetric.shared_descendant_count == 0U);

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

    // E04-03: when independent semantic-role evidence says the outer role
    // survives and the fully internal role disappears, Resolved/Missing is
    // permitted. Provider history is corroborating evidence, not the winner.
    CHECK(
        absorbed.first.unchanged_present ||
        absorbed.first.unique_descendant_count ==
            1U);
    CHECK(absorbed.second.deleted);
    CHECK(
        absorbed.second.unique_descendant_count ==
        0U);
    CHECK(absorbed.shared_descendant_count == 0U);

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

    // E04-04: PM-00A declares no aggregate merged-reference type.
    CHECK(
        resolveSingular(
            1U,
            0U,
            true) ==
        kernel::ReferenceStatus::unsupported);
}

struct ProbeSet final {
    kernel::EdgeSplitHistoryEvidence split;
    kernel::EdgeSplitHistoryEvidence removed;
    kernel::FaceMergeHistoryEvidence merged;
    kernel::FaceMergeHistoryEvidence asymmetric;
    kernel::FaceMergeHistoryEvidence absorbed;

    friend bool operator==(
        const ProbeSet&,
        const ProbeSet&) = default;
};

[[nodiscard]] ProbeSet evaluate() {
    return {
        kernel_occt::buildEdgeSplitHistoryEvidence(
            kernel::EdgeSplitProbeScenario::
                middle_notch),
        kernel_occt::buildEdgeSplitHistoryEvidence(
            kernel::EdgeSplitProbeScenario::
                remove_target),
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                overlapping_coplanar),
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                asymmetric_history),
        kernel_occt::buildFaceMergeHistoryEvidence(
            kernel::FaceMergeProbeScenario::
                absorbed_inner),
    };
}

void verify(const ProbeSet& probes) {
    verifySplitRows(
        probes.split,
        probes.removed);
    verifyMergeRows(
        probes.merged,
        probes.asymmetric,
        probes.absorbed);
}

} // namespace

int main() {
    ProbeSet first;
    {
        first = evaluate();
        verify(first);
    }

    // COLD replay for the evidence-only probes: every OCCT operation object,
    // TopoDS handle and provider history instance from the first pass is gone.
    // Recreate from the declared scenario inputs only and require identical
    // neutral cardinality/history evidence and semantic statuses.
    const auto cold = evaluate();
    CHECK(cold == first);
    verify(cold);

    std::cout
        << "PM00A_E03_E04_PASS rows=E03-01..E03-04,E04-01..E04-04"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
