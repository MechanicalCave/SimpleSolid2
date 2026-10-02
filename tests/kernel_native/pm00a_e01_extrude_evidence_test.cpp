#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E01 CHECK failed at line "
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
            ("simplesolid2_pm00a_e01_" +
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
    std::array<std::optional<sketch::EntityId>, 4U>
        rectangle_ids;
    std::optional<sketch::EntityId> circle_id;
    std::optional<part::ProfileId> profile_id;
};

struct SideKey final {
    std::string source_entity;
    bool hole{false};

    friend bool operator==(
        const SideKey&,
        const SideKey&) = default;

    friend bool operator<(
        const SideKey& first,
        const SideKey& second) noexcept {
        if (first.source_entity !=
            second.source_entity) {
            return first.source_entity <
                   second.source_entity;
        }
        return first.hole < second.hole;
    }
};

using SideMap =
    std::map<SideKey, kernel::ExtrudeFaceEvidence>;

SideMap sideMap(
    const kernel::ExtrudeEvidence& evidence) {
    SideMap result;
    for (const auto& side : evidence.sides) {
        CHECK(side.role ==
              kernel::ExtrudeFaceRoleKind::side);
        CHECK(side.provenance.has_value());
        const SideKey key{
            side.provenance->source_entity,
            side.provenance->hole};
        CHECK(result.emplace(key, side).second);
    }
    return result;
}

kernel::ExtrudeEvidence extrude(
    const part::PartDocument& document,
    part::ProfileId profile_id,
    double distance) {
    const auto input =
        part::makeKernelProfileInput(
            document,
            profile_id);
    CHECK(input.has_value());
    CHECK(input->valid());

    const auto evidence =
        kernel_occt::buildProfileExtrudeEvidence(
            *input,
            distance);
    CHECK(evidence.ok());
    CHECK(evidence.shape.brep_valid);
    CHECK(evidence.shape.solid_count == 1U);
    CHECK(
        evidence.start_cap.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.start_cap.candidate_face_count ==
        1U);
    CHECK(
        evidence.end_cap.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        evidence.end_cap.candidate_face_count ==
        1U);
    CHECK(evidence.sides.size() == 5U);

    for (const auto& side : evidence.sides) {
        CHECK(
            side.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(side.candidate_face_count == 1U);
        CHECK(side.provenance.has_value());
        CHECK(
            !side.provenance
                 ->source_entity.empty());
    }

    return evidence;
}

void verifyExpectedSides(
    const kernel::ExtrudeEvidence& evidence,
    const FixtureIds& ids) {
    const auto sides = sideMap(evidence);
    CHECK(sides.size() == 5U);

    for (const auto& id : ids.rectangle_ids) {
        CHECK(id.has_value());
        const SideKey key{
            id->serialized(),
            false};
        const auto found = sides.find(key);
        CHECK(found != sides.end());
        CHECK(
            found->second.status ==
            kernel::ReferenceStatus::resolved);
    }

    CHECK(ids.circle_id.has_value());
    const SideKey hole_key{
        ids.circle_id->serialized(),
        true};
    const auto hole = sides.find(hole_key);
    CHECK(hole != sides.end());
    CHECK(
        hole->second.status ==
        kernel::ReferenceStatus::resolved);
}

FixtureIds createAuthoredProfile(
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

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *ids.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.changed);
    CHECK(rectangle.entity_ids.size() == 4U);
    for (std::size_t index = 0U;
         index < 4U;
         ++index) {
        ids.rectangle_ids[index] =
            rectangle.entity_ids[index];
    }

    const auto circle =
        session.execute(
            application::AddSketchCircleCommand{
                *ids.sketch_id,
                {20.0, 15.0},
                5.0});
    CHECK(
        circle.ok() &&
        circle.entity_id.has_value());
    ids.circle_id =
        *circle.entity_id;

    const auto* source =
        session.document().findSketch(
            *ids.sketch_id);
    CHECK(source != nullptr);

    const auto analysis =
        sketch::analyzeRegions(
            source->model);
    CHECK(analysis.complete());

    const auto pick =
        sketch::pickRegion(
            source->model,
            analysis,
            {2.0, 2.0});
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
    CHECK(region->holes.size() == 1U);

    const auto intent =
        part::makeProfileRegionIntent(
            *region);
    CHECK(intent.has_value());

    const auto created =
        session.execute(
            application::CreateProfileCommand{
                *ids.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        created.ok() &&
        created.profile_id.has_value());
    ids.profile_id =
        *created.profile_id;

    return ids;
}

void resizeRectangle(
    application::DocumentSession& session,
    const FixtureIds& ids) {
    CHECK(ids.sketch_id.has_value());
    for (const auto& id : ids.rectangle_ids) {
        CHECK(id.has_value());
    }

    const sketch::Point2 a{0.0, 0.0};
    const sketch::Point2 b{60.0, 0.0};
    const sketch::Point2 c{60.0, 45.0};
    const sketch::Point2 d{0.0, 45.0};

    const auto result =
        session.execute(
            application::UpdateSketchLinesCommand{
                *ids.sketch_id,
                session.document().revision(),
                {
                    {*ids.rectangle_ids[0], a, b},
                    {*ids.rectangle_ids[1], b, c},
                    {*ids.rectangle_ids[2], c, d},
                    {*ids.rectangle_ids[3], d, a},
                }});
    CHECK(result.ok());
    CHECK(result.changed);
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path / "PM00AE01.ss2part";

    part::PartDocumentStore store;
    FixtureIds ids;
    SideMap baseline_sides;

    // E01-01/02/03: authored Profile with one hole -> valid one-solid
    // Extrude evidence with unique caps and one side per boundary use.
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

        ids = createAuthoredProfile(session);
        CHECK(ids.profile_id.has_value());

        const auto initial =
            extrude(
                session.document(),
                *ids.profile_id,
                10.0);
        verifyExpectedSides(initial, ids);
        baseline_sides = sideMap(initial);

        // E01-04: distance edit keeps the semantic cap/side roles stable.
        const auto longer =
            extrude(
                session.document(),
                *ids.profile_id,
                25.0);
        verifyExpectedSides(longer, ids);
        CHECK(sideMap(longer) == baseline_sides);

        CHECK(session.save().ok());
    }

    // COLD replay of E01-01..04.
    auto loaded =
        store.load(path);
    CHECK(loaded.ok());
    CHECK(ids.profile_id.has_value());

    const auto cold_initial =
        extrude(
            *loaded.document,
            *ids.profile_id,
            10.0);
    verifyExpectedSides(cold_initial, ids);
    CHECK(sideMap(cold_initial) == baseline_sides);

    {
        application::DocumentSession session{
            path,
            std::move(*loaded.document),
            *loaded.checkpoint};

        // E01-05: change rectangle dimensions but preserve the four authored
        // Line EntityIds. Corresponding side provenance must remain Resolved.
        resizeRectangle(session, ids);

        const auto resized =
            extrude(
                session.document(),
                *ids.profile_id,
                10.0);
        verifyExpectedSides(resized, ids);

        const auto resized_sides =
            sideMap(resized);
        CHECK(
            resized_sides.size() ==
            baseline_sides.size());
        for (const auto& [key, before] :
             baseline_sides) {
            const auto after =
                resized_sides.find(key);
            CHECK(after != resized_sides.end());
            CHECK(
                after->second.status ==
                before.status);
            CHECK(
                after->second
                    .candidate_face_count ==
                before.candidate_face_count);
        }

        CHECK(session.save().ok());
    }

    // COLD replay of E01-05.
    loaded = store.load(path);
    CHECK(loaded.ok());
    const auto cold_resized =
        extrude(
            *loaded.document,
            *ids.profile_id,
            10.0);
    verifyExpectedSides(cold_resized, ids);

    {
        application::DocumentSession session{
            path,
            std::move(*loaded.document),
            *loaded.checkpoint};

        // E01-06: remove the semantic source Circle. The Profile becomes
        // unresolved/missing-source; no provider probe may choose another
        // circular face as a substitute.
        CHECK(ids.circle_id.has_value());
        const auto erased =
            session.execute(
                application::EraseSketchEntityCommand{
                    *ids.sketch_id,
                    *ids.circle_id});
        CHECK(erased.ok());
        CHECK(erased.changed);

        const auto resolved =
            session.document().evaluateProfile(
                *ids.profile_id);
        CHECK(resolved.has_value());
        CHECK(!resolved->valid());
        CHECK(
            resolved->status ==
            part::ProfileIntentResolutionStatus::
                missing_source_entity);

        const auto missing_input =
            part::makeKernelProfileInput(
                session.document(),
                *ids.profile_id);
        CHECK(!missing_input.has_value());

        CHECK(session.save().ok());
    }

    // COLD replay of E01-06: missing-source status survives reopen and no
    // neutral Kernel/OCCT input is fabricated.
    loaded = store.load(path);
    CHECK(loaded.ok());
    const auto cold_missing =
        loaded.document->evaluateProfile(
            *ids.profile_id);
    CHECK(cold_missing.has_value());
    CHECK(!cold_missing->valid());
    CHECK(
        cold_missing->status ==
        part::ProfileIntentResolutionStatus::
            missing_source_entity);
    CHECK(
        !part::makeKernelProfileInput(
             *loaded.document,
             *ids.profile_id)
             .has_value());

    std::cout
        << "PM00A_E01_PASS rows=E01-01..E01-06"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
