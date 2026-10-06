#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04E1 Revolve persistence CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double pi =
    std::numbers::pi_v<double>;

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm04e1_" +
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

void writeBytes(
    const std::filesystem::path& path,
    const std::string& bytes) {
    std::ofstream out{
        path,
        std::ios::binary | std::ios::trunc};
    CHECK(static_cast<bool>(out));
    out.write(
        bytes.data(),
        static_cast<std::streamsize>(
            bytes.size()));
    CHECK(static_cast<bool>(out));
}

part::AxisReference originAxis(
    core::BuiltinReferenceRole role) {
    return part::AxisReference{
        part::BuiltinOriginAxisReference{role}};
}

part::AxisReference authoredAxis(
    part::AxisId id) {
    return part::AxisReference{
        part::AuthoredAxisReference{id}};
}

struct Fixture final {
    part::PartDocument document;
    part::ProfileId profile_id;
    part::AxisId live_axis_id;
    part::AxisId missing_axis_id;
};

Fixture makeFixture() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                {10.0, 10.0},
                {20.0, 20.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto axis_line =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                {0.0, 0.0},
                {30.0, 0.0},
                sketch::EntityRole::
                    construction});
    CHECK(
        axis_line.ok() &&
        axis_line.entity_id);

    const auto* sketch =
        session.document().findSketch(
            sketch_id);
    CHECK(sketch != nullptr);
    const auto regions =
        sketch::analyzeRegions(
            sketch->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);

    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id);

    auto state =
        session.document().state();
    const auto live_axis =
        state.next_axis_id.allocate();
    const auto missing_axis =
        state.next_axis_id.allocate();
    CHECK(live_axis.has_value());
    CHECK(missing_axis.has_value());

    state.axes.push_back(
        part::PartAxis{
            *live_axis,
            "Axis001",
            {
                sketch_id,
                *axis_line.entity_id,
            },
            false});

    const auto first =
        state.body.next_feature_id.allocate();
    const auto second =
        state.body.next_feature_id.allocate();
    const auto third =
        state.body.next_feature_id.allocate();
    CHECK(first.has_value());
    CHECK(second.has_value());
    CHECK(third.has_value());

    state.body.features.push_back(
        part::PartFeature{
            *first,
            "Revolve Origin",
            false,
            part::RevolveFeature{
                *profile.profile_id,
                originAxis(
                    core::BuiltinReferenceRole::
                        x_axis),
                part::RevolveOperation::add,
                part::OneSidedRevolveExtent{
                    core::AngleValue{pi / 2.0},
                    true}}});

    state.body.features.push_back(
        part::PartFeature{
            *second,
            "Revolve Axis",
            false,
            part::RevolveFeature{
                *profile.profile_id,
                authoredAxis(*live_axis),
                part::RevolveOperation::cut,
                part::MidplaneRevolveExtent{
                    core::AngleValue{pi}}}});

    // The AxisId was allocated and then its Axis object is absent. This is
    // durable repairable Missing intent, not malformed identity.
    state.body.features.push_back(
        part::PartFeature{
            *third,
            "Revolve Missing Axis",
            true,
            part::RevolveFeature{
                *profile.profile_id,
                authoredAxis(*missing_axis),
                part::RevolveOperation::add,
                part::OneSidedRevolveExtent{
                    core::AngleValue{pi / 4.0},
                    false}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());

    return {
        std::move(*restored.document),
        *profile.profile_id,
        *live_axis,
        *missing_axis};
}

std::string repackage(
    const persistence::NativeDocumentPackage& package,
    int schema_version,
    std::string authored_json) {
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                package.descriptor.document_id,
                schema_version},
            std::move(authored_json));
    CHECK(built.ok());
    return std::move(*built.bytes);
}

} // namespace

int main() {
    CHECK(
        part::PartDocumentStore::current_schema_version ==
        13);

    TempDirectory temp;
    part::PartDocumentStore store;
    auto fixture = makeFixture();

    const auto current_path =
        temp.path / "RevolveCurrent.ss2part";
    CHECK(
        store.createNew(
            current_path,
            fixture.document)
            .ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            current_path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 13);

    const auto authored =
        nlohmann::json::parse(
            package.package->authored_json);
    const auto& features =
        authored.at("body").at("features");
    CHECK(features.size() == 3U);

    CHECK(
        features.at(0).at("kind") ==
        "revolve");
    CHECK(
        features.at(0).at("axis").at("kind") ==
        "origin_axis");
    CHECK(
        features.at(0).at("axis").at("axis") ==
        "x_axis");
    CHECK(
        features.at(0).at("operation") ==
        "add");
    CHECK(
        features.at(0).at("extent").at("mode") ==
        "one_side");
    CHECK(
        features.at(0).at("extent").at("reverse")
            .get<bool>());
    CHECK(
        std::abs(
            features.at(0)
                    .at("extent")
                    .at("angle_rad")
                    .get<double>() -
            pi / 2.0) <
        1.0e-12);

    CHECK(
        features.at(1).at("axis").at("kind") ==
        "authored_axis");
    CHECK(
        features.at(1)
                .at("axis")
                .at("axis_id")
                .get<std::string>() ==
        fixture.live_axis_id.serialized());
    CHECK(
        features.at(1).at("operation") ==
        "cut");
    CHECK(
        features.at(1).at("extent").at("mode") ==
        "midplane");

    CHECK(
        features.at(2)
                .at("axis")
                .at("axis_id")
                .get<std::string>() ==
        fixture.missing_axis_id.serialized());

    // Save / close / reopen reconstructs exactly the durable authored state.
    const auto loaded =
        store.load(current_path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->documentId() ==
        fixture.document.documentId());
    CHECK(
        loaded.document->state() ==
        fixture.document.state());
    CHECK(
        loaded.document->findAxis(
            fixture.missing_axis_id) ==
        nullptr);
    const auto* missing_feature =
        loaded.document->findFeature(
            fixture.document.body()
                .features.at(2).id);
    CHECK(missing_feature != nullptr);
    const auto* missing_revolve =
        std::get_if<part::RevolveFeature>(
            &missing_feature->definition);
    CHECK(missing_revolve != nullptr);
    CHECK(
        part::authoredAxisIdForAxisReference(
            missing_revolve->axis) ==
        fixture.missing_axis_id);

    // A valid schema-v12 document has no Revolve records. Loading it preserves
    // all old authored identities; the next save writes current schema v13.
    auto legacy_state =
        fixture.document.state();
    legacy_state.body.features.clear();
    auto legacy_document =
        part::PartDocument::restore(
            fixture.document.documentId(),
            std::move(legacy_state));
    CHECK(legacy_document.ok());

    const auto legacy_current_path =
        temp.path / "LegacySourceCurrent.ss2part";
    CHECK(
        store.createNew(
            legacy_current_path,
            *legacy_document.document)
            .ok());
    const auto legacy_current_package =
        persistence::readNativeDocumentContainer(
            legacy_current_path);
    CHECK(legacy_current_package.ok());

    const auto legacy_v12_path =
        temp.path / "LegacyV12.ss2part";
    writeBytes(
        legacy_v12_path,
        repackage(
            *legacy_current_package.package,
            12,
            legacy_current_package.package
                ->authored_json));

    const auto legacy_loaded =
        store.load(legacy_v12_path);
    CHECK(legacy_loaded.ok());
    CHECK(
        legacy_loaded.document->state() ==
        legacy_document.document->state());

    const auto migrated_path =
        temp.path / "MigratedV13.ss2part";
    CHECK(
        store.createNew(
            migrated_path,
            *legacy_loaded.document)
            .ok());
    const auto migrated =
        persistence::readNativeDocumentContainer(
            migrated_path);
    CHECK(migrated.ok());
    CHECK(
        migrated.package->descriptor
            .domain_schema_version == 13);

    // Revolve is a schema-v13 feature kind. Relabeling a v13 Revolve payload
    // as v12 must fail closed rather than silently interpreting future data.
    const auto mislabeled_path =
        temp.path / "MislabeledV12.ss2part";
    writeBytes(
        mislabeled_path,
        repackage(
            *package.package,
            12,
            package.package->authored_json));
    const auto mislabeled =
        store.load(mislabeled_path);
    CHECK(!mislabeled.ok());
    CHECK(
        mislabeled.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    // A never-allocated authored AxisId cannot be revived by persistence.
    auto bad_axis_json = authored;
    bad_axis_json["body"]["features"][1]["axis"]["axis_id"] =
        "999";
    auto bad_axis_text =
        bad_axis_json.dump(2);
    bad_axis_text.push_back('\n');
    const auto bad_axis_path =
        temp.path / "BadAxis.ss2part";
    writeBytes(
        bad_axis_path,
        repackage(
            *package.package,
            13,
            std::move(bad_axis_text)));
    const auto bad_axis =
        store.load(bad_axis_path);
    CHECK(!bad_axis.ok());
    CHECK(
        bad_axis.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    // Persisted angle remains bounded by the authored Revolve contract.
    auto bad_angle_json = authored;
    bad_angle_json["body"]["features"][0]["extent"]["angle_rad"] =
        2.0 * pi + 0.01;
    auto bad_angle_text =
        bad_angle_json.dump(2);
    bad_angle_text.push_back('\n');
    const auto bad_angle_path =
        temp.path / "BadAngle.ss2part";
    writeBytes(
        bad_angle_path,
        repackage(
            *package.package,
            13,
            std::move(bad_angle_text)));
    const auto bad_angle =
        store.load(bad_angle_path);
    CHECK(!bad_angle.ok());
    CHECK(
        bad_angle.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    std::cout
        << "PM-04E1 Revolve schema-v13 persistence tests passed\n";
    return EXIT_SUCCESS;
}
