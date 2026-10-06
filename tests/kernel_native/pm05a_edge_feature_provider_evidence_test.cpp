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
            << "PM-05A Edge Feature provider evidence CHECK failed at line "
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

std::string_view scenarioName(
    kernel::EdgeFeatureProbeScenario scenario) {
    switch (scenario) {
    case kernel::EdgeFeatureProbeScenario::single_edge:
        return "single";
    case kernel::EdgeFeatureProbeScenario::disconnected_pair:
        return "disconnected";
    case kernel::EdgeFeatureProbeScenario::adjacent_pair:
        return "adjacent";
    case kernel::EdgeFeatureProbeScenario::trihedral_corner:
        return "trihedral";
    case kernel::EdgeFeatureProbeScenario::closed_loop:
        return "closed_loop";
    case kernel::EdgeFeatureProbeScenario::
            mixed_connected_disconnected:
        return "mixed";
    case kernel::EdgeFeatureProbeScenario::excessive_parameter:
        return "excessive";
    }
    return "unknown";
}

const kernel::EdgeFeatureProviderEvidence&
findProbe(
    const kernel::EdgeFeatureProviderMatrixEvidence& matrix,
    kernel::EdgeFeatureEvidenceOperation operation,
    kernel::EdgeFeatureProbeScenario scenario) {
    for (const auto& probe : matrix.probes) {
        if (probe.operation == operation &&
            probe.scenario == scenario) {
            return probe;
        }
    }
    std::cerr << "Missing PM-05A provider probe\n";
    std::exit(EXIT_FAILURE);
}

} // namespace

int main() {
    const auto matrix =
        kernel_occt::buildEdgeFeatureProviderMatrixEvidence();

    CHECK(matrix.complete());

    const auto cold =
        kernel_occt::buildEdgeFeatureProviderMatrixEvidence();
    CHECK(cold == matrix);

    std::size_t successful_normal = 0U;
    std::size_t exact_membership = 0U;
    std::size_t order_invariant = 0U;
    std::size_t unclaimed_corner_patch_cases = 0U;

    for (const auto& probe : matrix.probes) {
        CHECK(probe.sourceReady());
        CHECK(probe.build_attempted);
        CHECK(
            probe.generated_faces_per_selected_edge.empty() ||
            probe.generated_faces_per_selected_edge.size() ==
                probe.requested_edge_count);

        if (probe.exact_provider_input_membership) {
            ++exact_membership;
        }
        if (probe.reverse_same_topology_and_volume) {
            ++order_invariant;
        }
        if (probe.unclaimed_new_face_count > 0U) {
            ++unclaimed_corner_patch_cases;
        }

        if (probe.scenario !=
            kernel::EdgeFeatureProbeScenario::
                excessive_parameter &&
            probe.build_succeeded) {
            ++successful_normal;
            CHECK(probe.result_shape.has_value());
            CHECK(probe.result_shape->ok());
            CHECK(probe.result_shape->solid_count == 1U);
            CHECK(probe.result_signature.has_value());
        }

        std::cout
            << "PM05A_PROBE"
            << " op=" << operationName(probe.operation)
            << " scenario=" << scenarioName(probe.scenario)
            << " requested=" << probe.requested_edge_count
            << " contours=" << probe.provider_contour_count
            << " contour_edges=" << probe.provider_contour_edge_count
            << " exact_membership="
            << (probe.exact_provider_input_membership ? 1 : 0)
            << " success="
            << (probe.build_succeeded ? 1 : 0)
            << " reverse_same="
            << (probe.reverse_same_topology_and_volume ? 1 : 0)
            << " new_faces=" << probe.new_face_count
            << " edge_claimed_faces="
            << probe.generated_from_selected_edges_face_count
            << " vertex_claimed_faces="
            << probe.generated_from_shared_vertices_face_count
            << " unclaimed_new_faces="
            << probe.unclaimed_new_face_count
            << '\n';
    }

    // A single explicit Edge is only a provider sanity baseline, not the
    // production scope. Both operations must at least prove their native
    // provider path before the multi-edge observations are meaningful.
    CHECK(
        findProbe(
            matrix,
            kernel::EdgeFeatureEvidenceOperation::fillet,
            kernel::EdgeFeatureProbeScenario::single_edge)
            .build_succeeded);
    CHECK(
        findProbe(
            matrix,
            kernel::EdgeFeatureEvidenceOperation::chamfer,
            kernel::EdgeFeatureProbeScenario::single_edge)
            .build_succeeded);

    std::cout
        << "PM05A_EDGE_FEATURE_PROVIDER_MATRIX_PASS"
        << " probes=" << matrix.probes.size()
        << " normal_success=" << successful_normal
        << " exact_membership=" << exact_membership
        << " order_invariant=" << order_invariant
        << " unclaimed_corner_patch_cases="
        << unclaimed_corner_patch_cases
        << '\n';

    return EXIT_SUCCESS;
}
