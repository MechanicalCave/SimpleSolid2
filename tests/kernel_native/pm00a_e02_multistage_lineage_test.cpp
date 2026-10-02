#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <array>
#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E02 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] kernel::ReferenceStatus
resolveAtStage(
    kernel::EvidenceProducerStage requested,
    const kernel::MultiStageLineageEvidence& evidence) noexcept {
    const std::array<
        kernel::StageReferenceEvidence,
        3U>
        stages{
            evidence.at_extrude,
            evidence.at_cut,
            evidence.at_downstream,
        };

    for (const auto& candidate : stages) {
        if (candidate.stage == requested) {
            return candidate.status;
        }
    }
    return kernel::ReferenceStatus::missing;
}

[[nodiscard]] kernel::ReferenceStatus
resolveWithoutStage(
    const kernel::MultiStageLineageEvidence& evidence) noexcept {
    const std::array<
        kernel::StageReferenceEvidence,
        3U>
        stages{
            evidence.at_extrude,
            evidence.at_cut,
            evidence.at_downstream,
        };

    std::size_t live_stages = 0U;
    for (const auto& candidate : stages) {
        if (candidate.status ==
                kernel::ReferenceStatus::resolved &&
            candidate.candidate_count == 1U) {
            ++live_stages;
        }
    }

    // Producer stage is part of the semantic address. Omitting it can expose
    // ambiguity, but it is never enough to manufacture Resolved.
    return live_stages > 1U
        ? kernel::ReferenceStatus::ambiguous
        : kernel::ReferenceStatus::unsupported;
}

void verifyStable(
    const kernel::MultiStageLineageEvidence& evidence) {
    CHECK(evidence.base_ok());

    // E02-01: the selected Extrude-side meaning survives an unrelated Cut
    // on the opposite side of the solid.
    CHECK(
        evidence.at_extrude.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.at_extrude.candidate_count == 1U);
    CHECK(
        evidence.at_cut.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.at_cut.candidate_count == 1U);
    CHECK(!evidence.extrude_to_cut.deleted);
    CHECK(
        evidence.extrude_to_cut
            .unique_descendant_count == 1U);

    // E02-04: Fillet input is addressed on the immediate Cut output. The
    // fixture edge is unique and the adjacent selected face has one semantic
    // descendant after the real OCCT fillet.
    CHECK(
        evidence.downstream_input_edge_candidate_count ==
        1U);
    CHECK(
        evidence.downstream_outcome ==
        kernel::EvidenceOperationOutcome::valid);
    CHECK(evidence.downstream_shape.has_value());
    CHECK(evidence.downstream_shape->ok());
    CHECK(
        evidence.downstream_shape->solid_count ==
        1U);
    CHECK(
        evidence.cut_to_downstream.modified_count >
        0U);
    CHECK(!evidence.cut_to_downstream.deleted);
    CHECK(
        evidence.cut_to_downstream
            .unique_descendant_count == 1U);
    CHECK(
        evidence.at_downstream.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.at_downstream.candidate_count ==
        1U);

    // E02-06: the same semantic role is observable at several producer
    // stages. The consumed stage must therefore participate in resolution.
    CHECK(
        resolveAtStage(
            kernel::EvidenceProducerStage::
                extrude_output,
            evidence) ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        resolveAtStage(
            kernel::EvidenceProducerStage::
                cut_output,
            evidence) ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        resolveAtStage(
            kernel::EvidenceProducerStage::
                downstream_output,
            evidence) ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        resolveWithoutStage(evidence) ==
        kernel::ReferenceStatus::ambiguous);
}

void verifyDistanceEdit(
    const kernel::MultiStageLineageEvidence& before,
    const kernel::MultiStageLineageEvidence& after) {
    verifyStable(before);
    verifyStable(after);

    // E02-02: rebuild from a different upstream Extrude distance. No provider
    // handle continuity is shared between calls, but the declared semantic
    // stage/role remains uniquely Resolved.
    CHECK(
        before.at_extrude.status ==
        after.at_extrude.status);
    CHECK(
        before.at_cut.status ==
        after.at_cut.status);
    CHECK(
        before.at_downstream.status ==
        after.at_downstream.status);
    CHECK(
        before.at_cut.candidate_count ==
        after.at_cut.candidate_count);
    CHECK(
        before.at_downstream.candidate_count ==
        after.at_downstream.candidate_count);
}

void verifyRemoved(
    const kernel::MultiStageLineageEvidence& evidence) {
    CHECK(evidence.base_ok());

    // E02-03: Cut removes the previously referenced semantic face. A newly
    // created cut face is not a replacement for that meaning.
    CHECK(
        evidence.at_extrude.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.extrude_to_cut.deleted);
    CHECK(
        evidence.extrude_to_cut
            .unique_descendant_count == 0U);
    CHECK(
        evidence.at_cut.status ==
        kernel::ReferenceStatus::missing);
    CHECK(evidence.at_cut.candidate_count == 0U);
    CHECK(
        evidence.downstream_outcome ==
        kernel::EvidenceOperationOutcome::not_run);
}

void verifyGeometricFailure(
    const kernel::MultiStageLineageEvidence& evidence) {
    CHECK(evidence.base_ok());

    // E02-05: an upstream thin-dimension edit makes the unchanged Fillet
    // radius infeasible. The Fillet input reference still resolves on the
    // Cut output; geometric failure is a separate outcome.
    CHECK(
        evidence.at_cut.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(evidence.at_cut.candidate_count == 1U);
    CHECK(
        resolveAtStage(
            kernel::EvidenceProducerStage::
                cut_output,
            evidence) ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.downstream_input_edge_candidate_count ==
        1U);
    CHECK(
        evidence.downstream_outcome ==
        kernel::EvidenceOperationOutcome::
            geometric_failure);
    CHECK(!evidence.downstream_shape.has_value());
    CHECK(
        evidence.at_downstream.status ==
        kernel::ReferenceStatus::unsupported);
}

struct EvidenceSet final {
    kernel::MultiStageLineageEvidence stable_10;
    kernel::MultiStageLineageEvidence stable_20;
    kernel::MultiStageLineageEvidence removed;
    kernel::MultiStageLineageEvidence failure;

    friend bool operator==(
        const EvidenceSet&,
        const EvidenceSet&) = default;
};

[[nodiscard]] EvidenceSet evaluate() {
    return {
        kernel_occt::buildMultiStageLineageEvidence(
            kernel::MultiStageProbeScenario::
                stable_fillet,
            10.0),
        kernel_occt::buildMultiStageLineageEvidence(
            kernel::MultiStageProbeScenario::
                stable_fillet,
            20.0),
        kernel_occt::buildMultiStageLineageEvidence(
            kernel::MultiStageProbeScenario::
                remove_selected_face,
            10.0),
        kernel_occt::buildMultiStageLineageEvidence(
            kernel::MultiStageProbeScenario::
                upstream_thin_fillet_failure,
            10.0),
    };
}

void verify(const EvidenceSet& evidence) {
    verifyDistanceEdit(
        evidence.stable_10,
        evidence.stable_20);
    verifyRemoved(evidence.removed);
    verifyGeometricFailure(
        evidence.failure);
}

} // namespace

int main() {
    EvidenceSet first;
    {
        first = evaluate();
        verify(first);
    }

    // COLD replay for E02 evidence-only operations: all B-Rep, Boolean,
    // Fillet, provider history and TopoDS objects from the first pass are
    // destroyed. Re-evaluate from declared scenario inputs only.
    const auto cold = evaluate();
    CHECK(cold == first);
    verify(cold);

    std::cout
        << "PM00A_E02_PASS rows=E02-01..E02-06"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
