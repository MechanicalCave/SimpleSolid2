#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E06 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] kernel::BoundaryUse2D lineUse(
    std::string source,
    std::uint32_t use_index,
    kernel::Point2 start,
    kernel::Point2 end) {
    kernel::BoundaryUse2D use;
    use.curve = kernel::Line2{start, end};
    use.start_parameter = 0.0;
    use.end_parameter = 1.0;
    use.follows_source_direction = true;
    use.crosses_closed_seam = false;
    use.whole_closed_curve = false;
    use.provenance = {
        std::move(source),
        0U,
        use_index,
        false};
    return use;
}

[[nodiscard]] kernel::PlanarProfileInput makeProfile(
    double height,
    double outer_radius,
    double inner_radius) {
    kernel::PlanarProfileInput input;
    input.frame = {
        {0.0, 0.0, 0.0},
        {1.0, 0.0, 0.0},
        {0.0, 1.0, 0.0},
        {0.0, 0.0, 1.0}};

    input.outer.boundary = {
        lineUse(
            "bottom",
            0U,
            {inner_radius, 0.0},
            {outer_radius, 0.0}),
        lineUse(
            "outer",
            1U,
            {outer_radius, 0.0},
            {outer_radius, height}),
        lineUse(
            "top",
            2U,
            {outer_radius, height},
            {inner_radius, height}),
        lineUse(
            "inner",
            3U,
            {inner_radius, height},
            {inner_radius, 0.0}),
    };

    return input;
}

[[nodiscard]] const kernel::RevolveBoundaryFaceEvidence*
faceBySource(
    const kernel::FullRevolveEvidence& evidence,
    const std::string& source_entity) {
    const auto found =
        std::find_if(
            evidence.boundary_faces.begin(),
            evidence.boundary_faces.end(),
            [&source_entity](
                const kernel::RevolveBoundaryFaceEvidence&
                    face) {
                return face.provenance.source_entity ==
                       source_entity;
            });

    return found == evidence.boundary_faces.end()
        ? nullptr
        : &*found;
}

[[nodiscard]] bool near(
    double first,
    double second) noexcept {
    const double scale =
        std::max({
            1.0,
            std::abs(first),
            std::abs(second)});
    return std::abs(first - second) <=
           1.0e-9 * scale;
}

struct SemanticSnapshot final {
    kernel::ReferenceStatus outer_status{
        kernel::ReferenceStatus::unsupported};
    std::size_t outer_candidate_count{};
    kernel::BoundaryUseProvenance outer_provenance;
    kernel::FaceSurfaceKind outer_surface_kind{
        kernel::FaceSurfaceKind::other};
    bool outer_periodic_surface{false};
    std::size_t outer_seam_edge_count{};
    kernel::ReferenceStatus seam_reference_status{
        kernel::ReferenceStatus::resolved};
    std::size_t provider_seam_edge_count{};

    friend bool operator==(
        const SemanticSnapshot&,
        const SemanticSnapshot&) = default;
};

[[nodiscard]] SemanticSnapshot snapshot(
    const kernel::FullRevolveEvidence& evidence) {
    const auto* outer =
        faceBySource(evidence, "outer");
    CHECK(outer != nullptr);
    CHECK(outer->geometry_diagnostics.has_value());

    return {
        outer->status,
        outer->candidate_face_count,
        outer->provenance,
        outer->geometry_diagnostics
            ->surface_kind,
        outer->periodic_surface,
        outer->seam_edge_count,
        evidence.periodic_seam_reference_status,
        evidence.provider_seam_edge_count,
    };
}

void verifyCommon(
    const kernel::FullRevolveEvidence& evidence) {
    // E06-01: full 360-degree Revolve produces one valid B-Rep solid.
    CHECK(evidence.ok());
    CHECK(evidence.shape.brep_valid);
    CHECK(evidence.shape.solid_count == 1U);
    CHECK(evidence.boundary_faces.size() == 4U);

    for (const auto& face : evidence.boundary_faces) {
        CHECK(
            face.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(face.candidate_face_count == 1U);
        CHECK(
            !face.provenance
                 .source_entity.empty());
        CHECK(face.geometry_diagnostics.has_value());
    }

    // E06-02: the outer semantic revolved side is uniquely reconstructed
    // from its exact Profile boundary-use provenance.
    const auto* outer =
        faceBySource(evidence, "outer");
    CHECK(outer != nullptr);
    CHECK(
        outer->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(outer->candidate_face_count == 1U);
    CHECK(
        outer->geometry_diagnostics
            ->surface_kind ==
        kernel::FaceSurfaceKind::cylinder);

    // The provider does create real periodic/seam topology for the full
    // revolution. That topology is observable diagnostics only.
    CHECK(outer->periodic_surface);
    CHECK(outer->seam_edge_count > 0U);
    CHECK(evidence.provider_seam_edge_count > 0U);
    CHECK(evidence.provider_seam_total_length > 0.0);

    // E06-03: a provider-created periodic seam has no semantic source record
    // and is deliberately non-addressable.
    CHECK(
        evidence.periodic_seam_reference_status ==
        kernel::ReferenceStatus::unsupported);
}

[[nodiscard]] kernel::FullRevolveEvidence evaluate(
    const kernel::PlanarProfileInput& input) {
    CHECK(input.valid());

    const auto evidence =
        kernel_occt::buildProfileFullRevolveEvidence(
            input,
            {0.0, 0.0},
            {0.0, 1.0});

    verifyCommon(evidence);
    return evidence;
}

} // namespace

int main() {
    SemanticSnapshot baseline_snapshot;
    double baseline_outer_seam_length = 0.0;

    {
        const auto input =
            makeProfile(
                10.0,
                20.0,
                10.0);
        const auto evidence =
            evaluate(input);

        baseline_snapshot =
            snapshot(evidence);

        const auto* outer =
            faceBySource(
                evidence,
                "outer");
        CHECK(outer != nullptr);
        baseline_outer_seam_length =
            outer->seam_total_length;
        CHECK(
            baseline_outer_seam_length >
            0.0);
    }

    // E07-06 cold replay: all provider sweep/B-Rep/TopoDS state from the
    // first evaluation has left scope. Reconstruct from the same legal
    // evidence input and require the same semantic side/seam outcomes.
    const auto cold =
        evaluate(
            makeProfile(
                10.0,
                20.0,
                10.0));
    CHECK(snapshot(cold) == baseline_snapshot);

    // E06-04: an upstream dimension edit changes the revolved geometry and
    // actual provider seam representation, while the supported semantic side
    // remains Resolved from the same boundary provenance.
    const auto changed =
        evaluate(
            makeProfile(
                15.0,
                25.0,
                10.0));

    const auto* changed_outer =
        faceBySource(
            changed,
            "outer");
    CHECK(changed_outer != nullptr);
    CHECK(
        changed_outer->provenance ==
        baseline_snapshot.outer_provenance);
    CHECK(
        changed_outer->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(changed_outer->candidate_face_count == 1U);
    CHECK(changed_outer->periodic_surface);
    CHECK(changed_outer->seam_edge_count > 0U);

    CHECK(
        !near(
            changed_outer->seam_total_length,
            baseline_outer_seam_length));

    CHECK(
        changed.periodic_seam_reference_status ==
        kernel::ReferenceStatus::unsupported);

    // Cold replay the edited case too; provider seam diagnostics may be
    // rebuilt, but semantic status/provenance must reproduce.
    const auto changed_snapshot =
        snapshot(changed);
    {
        const auto cold_changed =
            evaluate(
                makeProfile(
                    15.0,
                    25.0,
                    10.0));
        CHECK(
            snapshot(cold_changed) ==
            changed_snapshot);
    }

    std::cout
        << "PM00A_E06_PASS rows=E06-01..E06-04"
        << " E07-06=PASS"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
