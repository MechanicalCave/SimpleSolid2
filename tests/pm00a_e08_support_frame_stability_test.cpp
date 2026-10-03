#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
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
#include <string>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E08 CHECK failed at line "
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
            ("simplesolid2_pm00a_e08_" +
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

struct ProfileFixture final {
    core::BuiltinReferenceRole support_role{
        core::BuiltinReferenceRole::xy_plane};
    std::optional<sketch::SketchId> sketch_id;
    std::optional<part::ProfileId> profile_id;
    std::array<std::optional<sketch::EntityId>, 4U>
        rectangle_ids;
};

using FrameMap =
    std::map<
        core::BuiltinReferenceRole,
        kernel::Frame3>;

[[nodiscard]] kernel::Frame3 expectedFrame(
    core::BuiltinReferenceRole role) {
    switch (role) {
    case core::BuiltinReferenceRole::xy_plane:
        return {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
        };
    case core::BuiltinReferenceRole::xz_plane:
        return {
            {0.0, 0.0, 0.0},
            {1.0, 0.0, 0.0},
            {0.0, 0.0, 1.0},
            {0.0, -1.0, 0.0},
        };
    case core::BuiltinReferenceRole::yz_plane:
        return {
            {0.0, 0.0, 0.0},
            {0.0, 1.0, 0.0},
            {0.0, 0.0, 1.0},
            {1.0, 0.0, 0.0},
        };
    default:
        CHECK(false);
        return {};
    }
}

[[nodiscard]] kernel::ReferenceStatus
supportEvidenceStatus(
    core::BuiltinReferenceRole role) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(role);
    if (!support) {
        return kernel::ReferenceStatus::unsupported;
    }

    const auto placement =
        part::sketchPlacementForSupport(*support);
    return placement.has_value()
        ? kernel::ReferenceStatus::resolved
        : kernel::ReferenceStatus::unsupported;
}

[[nodiscard]] ProfileFixture createProfile(
    application::DocumentSession& session,
    core::BuiltinReferenceRole role) {
    ProfileFixture fixture;
    fixture.support_role = role;

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                role});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id.has_value());
    fixture.sketch_id =
        *sketch_created.sketch_id;

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *fixture.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.changed);
    CHECK(rectangle.entity_ids.size() == 4U);

    for (std::size_t index = 0U;
         index < fixture.rectangle_ids.size();
         ++index) {
        fixture.rectangle_ids[index] =
            rectangle.entity_ids[index];
    }

    const auto* source =
        session.document().findSketch(
            *fixture.sketch_id);
    CHECK(source != nullptr);

    const auto analysis =
        sketch::analyzeRegions(
            source->model);
    CHECK(analysis.complete());

    const auto picked =
        sketch::pickRegion(
            source->model,
            analysis,
            {10.0, 10.0});
    CHECK(picked.region_index.has_value());

    const auto region =
        std::find_if(
            analysis.regions.begin(),
            analysis.regions.end(),
            [&picked](
                const sketch::RegionCandidate2D&
                    candidate) {
                return candidate.region_index ==
                       *picked.region_index;
            });
    CHECK(region != analysis.regions.end());

    const auto intent =
        part::makeProfileRegionIntent(
            *region);
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *fixture.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id.has_value());
    fixture.profile_id =
        *profile.profile_id;

    return fixture;
}

[[nodiscard]] kernel::Frame3 profileFrame(
    const part::PartDocument& document,
    const ProfileFixture& fixture) {
    CHECK(fixture.profile_id.has_value());

    const auto input =
        part::makeKernelProfileInput(
            document,
            *fixture.profile_id);
    CHECK(input.has_value());
    CHECK(input->valid());
    CHECK(input->frame.valid());

    return input->frame;
}

void verifyFixtureFrame(
    const part::PartDocument& document,
    const ProfileFixture& fixture,
    const FrameMap& baseline) {
    const auto frame =
        profileFrame(
            document,
            fixture);

    const auto expected =
        expectedFrame(
            fixture.support_role);
    CHECK(frame == expected);

    const auto found =
        baseline.find(
            fixture.support_role);
    CHECK(found != baseline.end());
    CHECK(frame == found->second);

    CHECK(fixture.sketch_id.has_value());
    const auto* hosted =
        document.findSketch(
            *fixture.sketch_id);
    CHECK(hosted != nullptr);
    CHECK(hosted->support.valid());
    CHECK(hosted->placement.valid());
    CHECK(
        part::sketchPlacementMatchesSupport(
            hosted->placement,
            hosted->support));
}

[[nodiscard]] FrameMap captureFrames(
    const part::PartDocument& document,
    const std::vector<ProfileFixture>& fixtures,
    const std::vector<std::size_t>& order) {
    FrameMap result;
    for (const auto index : order) {
        CHECK(index < fixtures.size());
        const auto& fixture =
            fixtures[index];
        const auto frame =
            profileFrame(
                document,
                fixture);
        CHECK(frame ==
              expectedFrame(
                  fixture.support_role));
        CHECK(
            result.emplace(
                fixture.support_role,
                frame)
                .second);
    }
    return result;
}

void resizeRectangle(
    application::DocumentSession& session,
    const ProfileFixture& fixture,
    double width,
    double height) {
    CHECK(fixture.sketch_id.has_value());
    for (const auto& id : fixture.rectangle_ids) {
        CHECK(id.has_value());
    }

    const sketch::Point2 a{0.0, 0.0};
    const sketch::Point2 b{width, 0.0};
    const sketch::Point2 c{width, height};
    const sketch::Point2 d{0.0, height};

    const auto changed =
        session.execute(
            application::UpdateSketchLinesCommand{
                *fixture.sketch_id,
                session.document().revision(),
                {
                    {*fixture.rectangle_ids[0], a, b},
                    {*fixture.rectangle_ids[1], b, c},
                    {*fixture.rectangle_ids[2], c, d},
                    {*fixture.rectangle_ids[3], d, a},
                }});
    CHECK(changed.ok());
    CHECK(changed.changed);
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path / "PM00AE08.ss2part";

    part::PartDocumentStore store;
    std::vector<ProfileFixture> fixtures;
    FrameMap baseline;

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

        fixtures.push_back(
            createProfile(
                session,
                core::BuiltinReferenceRole::xy_plane));
        fixtures.push_back(
            createProfile(
                session,
                core::BuiltinReferenceRole::xz_plane));
        fixtures.push_back(
            createProfile(
                session,
                core::BuiltinReferenceRole::yz_plane));

        // E08-01: unchanged semantic Origin-plane support produces the exact
        // deterministic right-handed O/U/V/N frame.
        baseline =
            captureFrames(
                session.document(),
                fixtures,
                {0U, 1U, 2U});
        CHECK(baseline.size() == 3U);

        // E08-03: support-frame evaluation has no provider-topology input.
        // Reordering the evidence traversal therefore cannot alter O/U/V/N.
        const auto reverse =
            captureFrames(
                session.document(),
                fixtures,
                {2U, 1U, 0U});
        CHECK(reverse == baseline);

        // E08-02: edit source geometry while support is unchanged. The Profile
        // remains valid and its neutral Kernel frame must not flip or rotate.
        resizeRectangle(
            session,
            fixtures[0],
            55.0,
            35.0);
        resizeRectangle(
            session,
            fixtures[1],
            60.0,
            40.0);
        resizeRectangle(
            session,
            fixtures[2],
            65.0,
            45.0);

        for (const auto& fixture : fixtures) {
            verifyFixtureFrame(
                session.document(),
                fixture,
                baseline);
        }

        const auto after_edit_reordered =
            captureFrames(
                session.document(),
                fixtures,
                {1U, 2U, 0U});
        CHECK(after_edit_reordered == baseline);

        CHECK(session.save().ok());
        CHECK(!session.needsSave());
    }

    // COLD replay for E08-01/02/03: DocumentSession and all transient Kernel
    // Profile inputs have left scope. Reopen durable Part state and evaluate
    // in yet another order.
    const auto loaded =
        store.load(path);
    CHECK(loaded.ok());

    for (const auto& fixture : fixtures) {
        verifyFixtureFrame(
            *loaded.document,
            fixture,
            baseline);
    }

    const auto cold_reordered =
        captureFrames(
            *loaded.document,
            fixtures,
            {2U, 0U, 1U});
    CHECK(cold_reordered == baseline);

    // E08-04: the accepted support contract represents only Origin planes.
    // Origin point/axes (and future planar-face support, for which no durable
    // support type exists yet) cannot be invented as support-frame semantics.
    for (const auto unsupported :
         {
             core::BuiltinReferenceRole::origin_point,
             core::BuiltinReferenceRole::x_axis,
             core::BuiltinReferenceRole::y_axis,
             core::BuiltinReferenceRole::z_axis,
         }) {
        CHECK(
            supportEvidenceStatus(
                unsupported) ==
            kernel::ReferenceStatus::unsupported);
        CHECK(
            !part::partSketchSupportForBuiltinPlane(
                 unsupported)
                 .has_value());

        const part::PartSketchSupport raw{
            unsupported};
        CHECK(!raw.valid());
        CHECK(
            !part::sketchPlacementForSupport(
                 raw)
                 .has_value());
    }

    std::cout
        << "PM00A_E08_PASS rows=E08-01..E08-04"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
