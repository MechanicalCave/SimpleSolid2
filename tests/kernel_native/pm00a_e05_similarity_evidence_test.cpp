#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E05 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm00a_e05_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

struct FixtureIds final {
    std::optional<sketch::SketchId> sketch_id;
    std::vector<sketch::EntityId> edges;
    std::optional<part::ProfileId> profile_id;
};

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

[[nodiscard]] bool nearPoint(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return near(first.x, second.x) &&
           near(first.y, second.y) &&
           near(first.z, second.z);
}

[[nodiscard]] bool sameSimilarityClass(
    const kernel::FaceGeometryDiagnostics& first,
    const kernel::FaceGeometryDiagnostics& second) noexcept {
    return first.surface_kind ==
               second.surface_kind &&
           near(first.area, second.area) &&
           nearPoint(
               first.surface_axis,
               second.surface_axis);
}

[[nodiscard]] bool sameGeometryDiagnostics(
    const kernel::FaceGeometryDiagnostics& first,
    const kernel::FaceGeometryDiagnostics& second) noexcept {
    return sameSimilarityClass(first, second) &&
           nearPoint(
               first.centroid,
               second.centroid);
}

[[nodiscard]] double distance(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    const double dx =
        first.x - second.x;
    const double dy =
        first.y - second.y;
    const double dz =
        first.z - second.z;
    return std::sqrt(
        dx * dx +
        dy * dy +
        dz * dz);
}

[[nodiscard]] const kernel::ExtrudeFaceEvidence*
sideBySource(
    const kernel::ExtrudeEvidence& evidence,
    const std::string& source_entity) {
    const auto found =
        std::find_if(
            evidence.sides.begin(),
            evidence.sides.end(),
            [&source_entity](
                const kernel::ExtrudeFaceEvidence&
                    side) {
                return side.provenance &&
                       side.provenance
                               ->source_entity ==
                           source_entity;
            });
    return found == evidence.sides.end()
        ? nullptr
        : &*found;
}

[[nodiscard]] kernel::ReferenceStatus
resolveExactProvenance(
    const kernel::ExtrudeEvidence& evidence,
    const kernel::BoundaryUseProvenance&
        target) {
    std::vector<
        const kernel::ExtrudeFaceEvidence*>
        matches;
    for (const auto& side : evidence.sides) {
        if (side.provenance &&
            *side.provenance == target) {
            matches.push_back(&side);
        }
    }

    if (matches.empty()) {
        return kernel::ReferenceStatus::missing;
    }
    if (matches.size() > 1U) {
        return kernel::ReferenceStatus::ambiguous;
    }
    return matches.front()->status;
}

[[nodiscard]] kernel::ReferenceStatus
resolveSimilarityOnly(
    const kernel::ExtrudeEvidence& evidence,
    const kernel::FaceGeometryDiagnostics&
        target) {
    std::size_t matches = 0U;
    for (const auto& side : evidence.sides) {
        if (side.geometry_diagnostics &&
            sameSimilarityClass(
                *side.geometry_diagnostics,
                target)) {
            ++matches;
        }
    }

    if (matches == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (matches == 1U) {
        return kernel::ReferenceStatus::resolved;
    }
    return kernel::ReferenceStatus::ambiguous;
}

[[nodiscard]] part::ProfileRegionIntent
currentRegionIntent(
    const part::PartDocument& document,
    sketch::SketchId sketch_id) {
    const auto* source_sketch =
        document.findSketch(sketch_id);
    CHECK(source_sketch != nullptr);

    const auto analysis =
        sketch::analyzeRegions(
            source_sketch->model);
    CHECK(analysis.complete());

    const auto pick =
        sketch::pickRegion(
            source_sketch->model,
            analysis,
            {10.0, 10.0});
    CHECK(pick.region_index.has_value());

    const auto region =
        std::find_if(
            analysis.regions.begin(),
            analysis.regions.end(),
            [&pick](
                const sketch::RegionCandidate2D&
                    candidate) {
                return candidate.region_index ==
                       *pick.region_index;
            });
    CHECK(region != analysis.regions.end());

    const auto intent =
        part::makeProfileRegionIntent(
            *region);
    CHECK(intent.has_value());
    return *intent;
}

FixtureIds createSplitBottomProfile(
    application::DocumentSession& session) {
    FixtureIds ids;

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id.has_value());
    ids.sketch_id =
        *sketch_created.sketch_id;

    const std::vector<
        std::pair<sketch::Point2, sketch::Point2>>
        edges{
            {{0.0, 0.0}, {20.0, 0.0}},
            {{20.0, 0.0}, {40.0, 0.0}},
            {{40.0, 0.0}, {40.0, 30.0}},
            {{40.0, 30.0}, {0.0, 30.0}},
            {{0.0, 30.0}, {0.0, 0.0}},
        };

    for (const auto& [start, end] : edges) {
        const auto added =
            session.execute(
                application::AddSketchLineCommand{
                    *ids.sketch_id,
                    start,
                    end,
                    sketch::EntityRole::regular});
        CHECK(
            added.ok() &&
            added.entity_id.has_value());
        ids.edges.push_back(
            *added.entity_id);
    }
    CHECK(ids.edges.size() == 5U);

    const auto intent =
        currentRegionIntent(
            session.document(),
            *ids.sketch_id);
    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *ids.sketch_id,
                session.document().revision(),
                intent});
    CHECK(
        profile.ok() &&
        profile.profile_id.has_value());
    ids.profile_id =
        *profile.profile_id;

    return ids;
}

[[nodiscard]] kernel::ExtrudeEvidence
extrude(
    const part::PartDocument& document,
    part::ProfileId profile_id) {
    const auto input =
        part::makeKernelProfileInput(
            document,
            profile_id);
    CHECK(input.has_value());
    CHECK(input->valid());

    const auto evidence =
        kernel_occt::buildProfileExtrudeEvidence(
            *input,
            10.0);
    CHECK(evidence.ok());
    CHECK(evidence.shape.brep_valid);
    CHECK(evidence.shape.solid_count == 1U);
    CHECK(evidence.sides.size() == 5U);

    for (const auto& side : evidence.sides) {
        CHECK(
            side.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(side.candidate_face_count == 1U);
        CHECK(side.provenance.has_value());
        CHECK(
            side.geometry_diagnostics
                .has_value());
    }

    return evidence;
}

void moveTopFarther(
    application::DocumentSession& session,
    const FixtureIds& ids) {
    CHECK(ids.sketch_id.has_value());
    CHECK(ids.edges.size() == 5U);

    const auto changed =
        session.execute(
            application::UpdateSketchLinesCommand{
                *ids.sketch_id,
                session.document().revision(),
                {
                    {
                        ids.edges[2],
                        {40.0, 0.0},
                        {40.0, 80.0},
                    },
                    {
                        ids.edges[3],
                        {40.0, 80.0},
                        {0.0, 80.0},
                    },
                    {
                        ids.edges[4],
                        {0.0, 80.0},
                        {0.0, 0.0},
                    },
                }});
    CHECK(changed.ok());
    CHECK(changed.changed);
}

void verifyBaselineSimilarity(
    const kernel::ExtrudeEvidence& evidence,
    const FixtureIds& ids) {
    CHECK(ids.edges.size() == 5U);

    const auto* first =
        sideBySource(
            evidence,
            ids.edges[0].serialized());
    const auto* second =
        sideBySource(
            evidence,
            ids.edges[1].serialized());
    CHECK(first != nullptr);
    CHECK(second != nullptr);
    CHECK(first->provenance.has_value());
    CHECK(second->provenance.has_value());
    CHECK(
        first->geometry_diagnostics
            .has_value());
    CHECK(
        second->geometry_diagnostics
            .has_value());

    // E05-01: two coplanar equal-area side faces have the same diagnostic
    // geometry class but remain individually Resolved by provenance.
    CHECK(
        sameSimilarityClass(
            *first->geometry_diagnostics,
            *second->geometry_diagnostics));
    CHECK(
        !nearPoint(
            first->geometry_diagnostics
                ->centroid,
            second->geometry_diagnostics
                ->centroid));
    CHECK(
        resolveExactProvenance(
            evidence,
            *first->provenance) ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        resolveExactProvenance(
            evidence,
            *second->provenance) ==
        kernel::ReferenceStatus::resolved);

    // E05-04: when provenance is deliberately omitted, geometry-only
    // evidence has two equally valid candidates and must not Resolved.
    CHECK(
        resolveSimilarityOnly(
            evidence,
            *first->geometry_diagnostics) ==
        kernel::ReferenceStatus::ambiguous);
}

void scenarioStableAndCloserDecoy(
    const std::filesystem::path& path,
    part::PartDocumentStore& store) {
    FixtureIds ids;
    kernel::BoundaryUseProvenance
        top_provenance;
    kernel::FaceGeometryDiagnostics
        original_top_geometry;

    {
        auto document =
            part::PartDocument::create(
                core::DocumentId::generate());
        const auto published =
            store.createNew(path, document);
        CHECK(published.ok());

        application::DocumentSession session{
            path,
            std::move(document),
            *published.checkpoint};

        ids =
            createSplitBottomProfile(
                session);
        CHECK(ids.profile_id.has_value());

        const auto baseline =
            extrude(
                session.document(),
                *ids.profile_id);
        verifyBaselineSimilarity(
            baseline,
            ids);

        const auto* top =
            sideBySource(
                baseline,
                ids.edges[3].serialized());
        CHECK(top != nullptr);
        CHECK(top->provenance.has_value());
        CHECK(
            top->geometry_diagnostics
                .has_value());
        top_provenance =
            *top->provenance;
        original_top_geometry =
            *top->geometry_diagnostics;

        CHECK(session.save().ok());
    }

    // COLD E05-01 replay.
    auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(ids.profile_id.has_value());
    const auto cold_baseline =
        extrude(
            *loaded.document,
            *ids.profile_id);
    verifyBaselineSimilarity(
        cold_baseline,
        ids);

    {
        application::DocumentSession session{
            path,
            std::move(*loaded.document),
            *loaded.checkpoint};

        // E05-03: the semantic top face moves far from its previous
        // centroid while bottom faces remain geometrically closer to the
        // old position. Exact provenance must still select the top.
        moveTopFarther(session, ids);

        const auto moved =
            extrude(
                session.document(),
                *ids.profile_id);

        const auto* current_top =
            sideBySource(
                moved,
                ids.edges[3].serialized());
        const auto* bottom_decoy =
            sideBySource(
                moved,
                ids.edges[0].serialized());
        CHECK(current_top != nullptr);
        CHECK(bottom_decoy != nullptr);
        CHECK(
            current_top->geometry_diagnostics
                .has_value());
        CHECK(
            bottom_decoy->geometry_diagnostics
                .has_value());

        CHECK(
            distance(
                original_top_geometry.centroid,
                bottom_decoy
                    ->geometry_diagnostics
                    ->centroid) <
            distance(
                original_top_geometry.centroid,
                current_top
                    ->geometry_diagnostics
                    ->centroid));

        CHECK(
            resolveExactProvenance(
                moved,
                top_provenance) ==
            kernel::ReferenceStatus::resolved);

        CHECK(session.save().ok());
    }

    // COLD E05-03 replay.
    loaded = store.load(path);
    CHECK(loaded.ok());
    const auto cold_moved =
        extrude(
            *loaded.document,
            *ids.profile_id);
    CHECK(
        resolveExactProvenance(
            cold_moved,
            top_provenance) ==
        kernel::ReferenceStatus::resolved);
}

void scenarioRemovedTargetWithIdenticalDecoy(
    const std::filesystem::path& path,
    part::PartDocumentStore& store) {
    FixtureIds ids;
    kernel::BoundaryUseProvenance
        removed_provenance;
    kernel::FaceGeometryDiagnostics
        removed_geometry;
    std::optional<sketch::EntityId>
        replacement_id;

    {
        auto document =
            part::PartDocument::create(
                core::DocumentId::generate());
        const auto published =
            store.createNew(path, document);
        CHECK(published.ok());

        application::DocumentSession session{
            path,
            std::move(document),
            *published.checkpoint};

        ids =
            createSplitBottomProfile(
                session);
        CHECK(ids.profile_id.has_value());
        CHECK(ids.sketch_id.has_value());

        const auto baseline =
            extrude(
                session.document(),
                *ids.profile_id);
        const auto* target =
            sideBySource(
                baseline,
                ids.edges[0].serialized());
        CHECK(target != nullptr);
        CHECK(target->provenance.has_value());
        CHECK(
            target->geometry_diagnostics
                .has_value());

        removed_provenance =
            *target->provenance;
        removed_geometry =
            *target->geometry_diagnostics;

        const auto erased =
            session.execute(
                application::EraseSketchEntityCommand{
                    *ids.sketch_id,
                    ids.edges[0]});
        CHECK(erased.ok());
        CHECK(erased.changed);

        const auto replacement =
            session.execute(
                application::AddSketchLineCommand{
                    *ids.sketch_id,
                    {0.0, 0.0},
                    {20.0, 0.0},
                    sketch::EntityRole::regular});
        CHECK(
            replacement.ok() &&
            replacement.entity_id.has_value());
        replacement_id =
            *replacement.entity_id;
        CHECK(
            replacement_id->serialized() !=
            ids.edges[0].serialized());

        const auto intent =
            currentRegionIntent(
                session.document(),
                *ids.sketch_id);
        const auto retargeted =
            session.execute(
                application::
                    ReplaceProfileRegionIntentCommand{
                        *ids.profile_id,
                        session.document().revision(),
                        intent});
        CHECK(retargeted.ok());
        CHECK(retargeted.changed);

        const auto after =
            extrude(
                session.document(),
                *ids.profile_id);

        // E05-02: the old semantic source is gone. The exact-geometry
        // replacement is a decoy and must not inherit the old reference.
        CHECK(
            resolveExactProvenance(
                after,
                removed_provenance) ==
            kernel::ReferenceStatus::missing);

        const auto* decoy =
            sideBySource(
                after,
                replacement_id->serialized());
        CHECK(decoy != nullptr);
        CHECK(
            decoy->geometry_diagnostics
                .has_value());
        CHECK(
            sameGeometryDiagnostics(
                removed_geometry,
                *decoy->geometry_diagnostics));

        CHECK(session.save().ok());
    }

    // COLD E05-02 replay.
    auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(ids.profile_id.has_value());
    CHECK(replacement_id.has_value());

    const auto cold =
        extrude(
            *loaded.document,
            *ids.profile_id);
    CHECK(
        resolveExactProvenance(
            cold,
            removed_provenance) ==
        kernel::ReferenceStatus::missing);

    const auto* cold_decoy =
        sideBySource(
            cold,
            replacement_id->serialized());
    CHECK(cold_decoy != nullptr);
    CHECK(
        cold_decoy->geometry_diagnostics
            .has_value());
    CHECK(
        sameGeometryDiagnostics(
            removed_geometry,
            *cold_decoy->geometry_diagnostics));
}

} // namespace

int main() {
    TempDirectory temp;
    part::PartDocumentStore store;

    scenarioStableAndCloserDecoy(
        temp.path / "E05Stable.ss2part",
        store);
    scenarioRemovedTargetWithIdenticalDecoy(
        temp.path / "E05Missing.ss2part",
        store);

    std::cout
        << "PM00A_E05_PASS rows=E05-01..E05-04"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
