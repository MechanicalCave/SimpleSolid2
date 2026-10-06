#include <simplesolid2/kernel/edge_feature_evidence.hpp>
#include <simplesolid2/kernel_occt/edge_feature_evidence.hpp>

#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05A lifecycle evidence CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

std::string_view operationName(
    kernel::EdgeFeatureEvidenceOperation operation) {
    return operation ==
                   kernel::EdgeFeatureEvidenceOperation::fillet
        ? "fillet"
        : "chamfer";
}

std::string_view upstreamName(
    kernel::EdgeFeatureUpstreamScenario scenario) {
    switch (scenario) {
    case kernel::EdgeFeatureUpstreamScenario::dimension_change:
        return "dimension_change";
    case kernel::EdgeFeatureUpstreamScenario::unchanged:
        return "unchanged";
    case kernel::EdgeFeatureUpstreamScenario::trim:
        return "trim";
    case kernel::EdgeFeatureUpstreamScenario::split:
        return "split";
    case kernel::EdgeFeatureUpstreamScenario::remove:
        return "remove";
    }
    return "unknown";
}

std::string_view statusName(
    kernel::ReferenceStatus status) {
    switch (status) {
    case kernel::ReferenceStatus::resolved:
        return "resolved";
    case kernel::ReferenceStatus::missing:
        return "missing";
    case kernel::ReferenceStatus::ambiguous:
        return "ambiguous";
    case kernel::ReferenceStatus::unsupported:
        return "unsupported";
    }
    return "unknown";
}

const kernel::EdgeFeatureUpstreamEvidence&
findUpstream(
    const kernel::EdgeFeatureLifecycleEvidence& evidence,
    kernel::EdgeFeatureEvidenceOperation operation,
    kernel::EdgeFeatureUpstreamScenario scenario) {
    for (const auto& item : evidence.upstream) {
        if (item.operation == operation &&
            item.scenario == scenario) {
            return item;
        }
    }
    std::cerr << "Missing PM-05A upstream probe\n";
    std::exit(EXIT_FAILURE);
}

} // namespace

int main() {
    const auto evidence =
        kernel_occt::buildEdgeFeatureLifecycleEvidence();

    CHECK(evidence.complete());

    const auto cold =
        kernel_occt::buildEdgeFeatureLifecycleEvidence();
    CHECK(cold == evidence);

    for (const auto& tangent : evidence.tangent_chain) {
        CHECK(tangent.source_shape.ok());
        CHECK(tangent.source_shape.solid_count == 1U);
        CHECK(tangent.requested_edge_count == 1U);
        CHECK(tangent.provider_contour_count == 1U);
        CHECK(tangent.provider_contour_edge_count == 2U);
        CHECK(!tangent.exact_provider_input_membership);
        CHECK(tangent.build_succeeded);
        CHECK(tangent.full_chain_requested_edge_count == 2U);
        CHECK(tangent.full_chain_provider_contour_edge_count == 2U);
        CHECK(tangent.full_chain_exact_provider_input_membership);
        CHECK(tangent.full_chain_build_succeeded);
        CHECK(tangent.single_and_full_same_topology_and_volume);

        std::cout
            << "PM05A_TANGENT"
            << " op=" << operationName(tangent.operation)
            << " requested=" << tangent.requested_edge_count
            << " contours=" << tangent.provider_contour_count
            << " contour_edges="
            << tangent.provider_contour_edge_count
            << " exact_membership="
            << (tangent.exact_provider_input_membership ? 1 : 0)
            << " success="
            << (tangent.build_succeeded ? 1 : 0)
            << " full_requested="
            << tangent.full_chain_requested_edge_count
            << " full_contour_edges="
            << tangent.full_chain_provider_contour_edge_count
            << " full_exact="
            << (tangent.full_chain_exact_provider_input_membership ? 1 : 0)
            << " full_success="
            << (tangent.full_chain_build_succeeded ? 1 : 0)
            << " same_result="
            << (tangent.single_and_full_same_topology_and_volume ? 1 : 0)
            << '\n';
    }

    for (const auto operation : {
             kernel::EdgeFeatureEvidenceOperation::fillet,
             kernel::EdgeFeatureEvidenceOperation::chamfer}) {
        const auto& dimension =
            findUpstream(
                evidence,
                operation,
                kernel::EdgeFeatureUpstreamScenario::
                    dimension_change);
        CHECK(dimension.source_shape.ok());
        CHECK(dimension.edited_shape.ok());
        CHECK(
            dimension.reference_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(dimension.current_edge_candidate_count == 1U);
        CHECK(dimension.downstream_attempted);
        CHECK(dimension.downstream_succeeded);
        CHECK(dimension.exact_provider_input_membership);

        const auto& unchanged =
            findUpstream(
                evidence,
                operation,
                kernel::EdgeFeatureUpstreamScenario::unchanged);
        CHECK(unchanged.source_shape.ok());
        CHECK(unchanged.edited_shape.ok());
        CHECK(
            unchanged.reference_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(unchanged.current_edge_candidate_count == 1U);
        CHECK(unchanged.downstream_attempted);
        CHECK(unchanged.downstream_succeeded);
        CHECK(unchanged.exact_provider_input_membership);

        const auto& trim =
            findUpstream(
                evidence,
                operation,
                kernel::EdgeFeatureUpstreamScenario::trim);
        CHECK(trim.source_shape.ok());
        CHECK(trim.edited_shape.ok());
        CHECK(
            trim.reference_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(trim.current_edge_candidate_count == 1U);
        CHECK(trim.downstream_attempted);
        CHECK(trim.downstream_succeeded);
        CHECK(trim.exact_provider_input_membership);

        const auto& split =
            findUpstream(
                evidence,
                operation,
                kernel::EdgeFeatureUpstreamScenario::split);
        CHECK(split.source_shape.ok());
        CHECK(split.edited_shape.ok());
        CHECK(
            split.reference_status ==
            kernel::ReferenceStatus::ambiguous);
        CHECK(split.current_edge_candidate_count > 1U);
        CHECK(!split.downstream_attempted);
        CHECK(!split.downstream_succeeded);

        const auto& remove =
            findUpstream(
                evidence,
                operation,
                kernel::EdgeFeatureUpstreamScenario::remove);
        CHECK(remove.source_shape.ok());
        CHECK(remove.edited_shape.ok());
        CHECK(
            remove.reference_status ==
            kernel::ReferenceStatus::missing);
        CHECK(remove.current_edge_candidate_count == 0U);
        CHECK(!remove.downstream_attempted);
        CHECK(!remove.downstream_succeeded);
    }

    for (const auto& item : evidence.upstream) {
        std::cout
            << "PM05A_UPSTREAM"
            << " op=" << operationName(item.operation)
            << " scenario=" << upstreamName(item.scenario)
            << " status=" << statusName(item.reference_status)
            << " candidates="
            << item.current_edge_candidate_count
            << " attempted="
            << (item.downstream_attempted ? 1 : 0)
            << " success="
            << (item.downstream_succeeded ? 1 : 0)
            << " exact_membership="
            << (item.exact_provider_input_membership ? 1 : 0)
            << '\n';
    }

    for (const auto& chain : evidence.chaining) {
        CHECK(chain.source_shape.ok());
        CHECK(chain.source_shape.solid_count == 1U);
        CHECK(chain.first_result_shape.ok());
        CHECK(chain.first_result_shape.solid_count == 1U);
        CHECK(chain.first_generated_face_count > 0U);
        CHECK(chain.generated_boundary_edge_count > 0U);
        CHECK(
            chain.second_operation_attempt_count ==
            chain.generated_boundary_edge_count);
        CHECK(chain.second_operation_success_count > 0U);
        CHECK(chain.chainable());

        std::cout
            << "PM05A_CHAIN"
            << " first=" << operationName(chain.first_operation)
            << " second=" << operationName(chain.second_operation)
            << " first_generated_faces="
            << chain.first_generated_face_count
            << " boundary_edges="
            << chain.generated_boundary_edge_count
            << " attempts="
            << chain.second_operation_attempt_count
            << " successes="
            << chain.second_operation_success_count
            << '\n';
    }

    std::cout
        << "PM05A_EDGE_FEATURE_LIFECYCLE_PASS"
        << " tangent=" << evidence.tangent_chain.size()
        << " upstream=" << evidence.upstream.size()
        << " chaining=" << evidence.chaining.size()
        << '\n';

    return EXIT_SUCCESS;
}
