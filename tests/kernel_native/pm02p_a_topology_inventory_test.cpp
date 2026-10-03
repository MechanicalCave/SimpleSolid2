#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string_view>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.A topology inventory CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

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

kernel::PlanarProfileInput rectangle() {
    kernel::PlanarProfileInput profile;
    profile.outer.boundary = {
        lineUse({0.0, 0.0}, {40.0, 0.0}, "bottom", 0U),
        lineUse({40.0, 0.0}, {40.0, 30.0}, "right", 1U),
        lineUse({40.0, 30.0}, {0.0, 30.0}, "top", 2U),
        lineUse({0.0, 30.0}, {0.0, 0.0}, "left", 3U),
    };
    CHECK(profile.valid());
    return profile;
}

std::size_t countClass(
    const kernel::EvidenceTopologyKindInventory& inventory,
    kernel::EvidenceTopologyAccountingClass expected) {
    return static_cast<std::size_t>(
        std::count_if(
            inventory.catalog.begin(),
            inventory.catalog.end(),
            [expected](const kernel::EvidenceTopologyRecord& record) {
                return record.accounting_class == expected;
            }));
}

void verify(
    const kernel::BodyTopologyInventoryEvidence& evidence) {
    CHECK(evidence.complete());
    CHECK(evidence.status == kernel::EvidenceStatus::ok);
    CHECK(evidence.brep_valid);
    CHECK(evidence.solid_count == 1U);

    // PM-02P E01 baseline: canonical unique topology of a non-degenerate
    // rectangular prism. The evidence catalog must account for every unique
    // provider subshape; semantic role promotion is deliberately deferred to
    // later checkpoints.
    CHECK(evidence.faces.provider_unique_count == 6U);
    CHECK(evidence.edges.provider_unique_count == 12U);
    CHECK(evidence.vertices.provider_unique_count == 8U);

    CHECK(evidence.faces.catalog.size() == 6U);
    CHECK(evidence.edges.catalog.size() == 12U);
    CHECK(evidence.vertices.catalog.size() == 8U);

    CHECK(
        evidence.faces.provider_occurrence_count >=
        evidence.faces.provider_unique_count);
    CHECK(
        evidence.edges.provider_occurrence_count >=
        evidence.edges.provider_unique_count);
    CHECK(
        evidence.vertices.provider_occurrence_count >=
        evidence.vertices.provider_unique_count);

    CHECK(
        countClass(
            evidence.faces,
            kernel::EvidenceTopologyAccountingClass::integrity_failure) ==
        0U);
    CHECK(
        countClass(
            evidence.edges,
            kernel::EvidenceTopologyAccountingClass::integrity_failure) ==
        0U);
    CHECK(
        countClass(
            evidence.vertices,
            kernel::EvidenceTopologyAccountingClass::integrity_failure) ==
        0U);
}

} // namespace

int main() {
    const auto input = rectangle();

    const auto first =
        kernel_occt::buildExtrudeTopologyInventoryEvidence(
            input,
            10.0);
    verify(first);

    // A cold repeat at this checkpoint is deliberately modest: it only proves
    // that the inventory itself depends on the declared input, not retained
    // TopoDS identity. Full semantic cold-rebuild coverage belongs to E21.
    const auto cold =
        kernel_occt::buildExtrudeTopologyInventoryEvidence(
            input,
            10.0);
    CHECK(cold == first);
    verify(cold);

    std::cout
        << "PM02P_A_TOPOLOGY_INVENTORY_PASS"
        << " faces=" << first.faces.provider_unique_count
        << " edges=" << first.edges.provider_unique_count
        << " vertices=" << first.vertices.provider_unique_count
        << " unaccounted=0\n";

    return EXIT_SUCCESS;
}
