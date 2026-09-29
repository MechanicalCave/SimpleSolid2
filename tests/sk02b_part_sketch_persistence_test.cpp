#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-02B persistence CHECK failed at line "
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
            ("simplesolid2_sk02b_persistence_" +
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

std::string buildPartPackage(
    const core::DocumentId& document_id,
    int schema_version,
    const std::string& authored_json) {
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                std::string{document_id.value()},
                schema_version,
            },
            authored_json);
    CHECK(built.ok());
    return *built.bytes;
}

std::string authoredWithSketches(
    const std::string& sketches_json) {
    return std::string{
        "{"
        "\"properties\":{"
            "\"number\":\"\","
            "\"title\":\"\","
            "\"description\":\"\","
            "\"engineering_revision\":\"\""
        "},"
        "\"presentation\":{"
            "\"builtin_reference_visibility_mask\":15"
        "},"
        "\"sketches\":"} +
        sketches_json +
        "}";
}

std::string v3SketchRecord(
    const sketch::SketchId& id,
    const std::string& model_json) {
    return std::string{
        "{"
        "\"id\":\""} +
        std::string{id.value()} +
        "\","
        "\"support\":{"
            "\"kind\":\"builtin_origin_plane\","
            "\"builtin_plane\":\"xy_plane\""
        "},"
        "\"placement\":{"
            "\"origin\":[0,0,0],"
            "\"u_axis\":[1,0,0],"
            "\"v_axis\":[0,1,0]"
        "},"
        "\"visible\":true,"
        "\"model\":" +
        model_json +
        "}";
}

bool malformedV3ModelRejected(
    const std::filesystem::path& path,
    part::PartDocumentStore& store,
    const std::string& model_json) {
    const auto sketch_id =
        sketch::SketchId::generate();
    const auto doc_id =
        core::DocumentId::generate();

    writeBytes(
        path,
        buildPartPackage(
            doc_id,
            3,
            authoredWithSketches(
                "[" +
                v3SketchRecord(
                    sketch_id,
                    model_json) +
                "]")));

    const auto loaded = store.load(path);
    return !loaded.ok() &&
           loaded.diagnostic.code ==
               part::PartStoreErrorCode::
                   malformed_document;
}

} // namespace

int main() {
    TempDirectory temp;
    part::PartDocumentStore store;

    const auto path =
        temp.path / "SketchLifecycle.ss2part";
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

    const auto first_sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    const auto second_sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xz_plane});
    CHECK(first_sketch_created.ok());
    CHECK(second_sketch_created.ok());
    CHECK(first_sketch_created.sketch_id.has_value());
    CHECK(second_sketch_created.sketch_id.has_value());

    const auto first_sketch =
        *first_sketch_created.sketch_id;
    const auto second_sketch =
        *second_sketch_created.sketch_id;

    const auto line_a =
        session.execute(
            application::AddSketchLineCommand{
                first_sketch,
                sketch::Point2{0.0, 0.0},
                sketch::Point2{10.0, 0.0}});
    const auto line_b =
        session.execute(
            application::AddSketchLineCommand{
                first_sketch,
                sketch::Point2{10.0, 0.0},
                sketch::Point2{10.0, 5.0}});
    const auto other_local_line =
        session.execute(
            application::AddSketchLineCommand{
                second_sketch,
                sketch::Point2{0.0, 0.0},
                sketch::Point2{0.0, 3.0}});

    CHECK(line_a.ok() && line_a.entity_id.has_value());
    CHECK(line_b.ok() && line_b.entity_id.has_value());
    CHECK(
        other_local_line.ok() &&
        other_local_line.entity_id.has_value());

    const auto profile_circle =
        session.execute(
            application::AddSketchCircleCommand{
                second_sketch,
                sketch::Point2{10.0, 10.0},
                2.0});
    CHECK(profile_circle.ok());
    CHECK(profile_circle.entity_id.has_value());

    const auto* profile_source =
        session.document().findSketch(
            second_sketch);
    CHECK(profile_source != nullptr);
    const auto profile_analysis =
        sketch::analyzeRegions(
            profile_source->model);
    const auto profile_pick =
        sketch::pickRegion(
            profile_source->model,
            profile_analysis,
            sketch::Point2{10.0, 10.0});
    CHECK(profile_pick.region_index.has_value());
    const auto profile_region =
        std::find_if(
            profile_analysis.regions.begin(),
            profile_analysis.regions.end(),
            [&profile_pick](
                const sketch::RegionCandidate2D& region) {
                return region.region_index ==
                       *profile_pick.region_index;
            });
    CHECK(
        profile_region !=
        profile_analysis.regions.end());
    const auto profile_intent =
        part::makeProfileRegionIntent(
            *profile_region);
    CHECK(profile_intent.has_value());

    const auto profile_created =
        session.execute(
            application::CreateProfileCommand{
                second_sketch,
                session.document().revision(),
                *profile_intent});
    CHECK(profile_created.ok());
    CHECK(profile_created.profile_id.has_value());
    CHECK(
        profile_created.profile_id->serialized() ==
        "1");

    CHECK(line_a.entity_id->serialized() == "1");
    CHECK(line_b.entity_id->serialized() == "2");
    CHECK(
        other_local_line.entity_id->serialized() ==
        "1");

    CHECK(
        session.execute(
            application::EraseSketchEntityCommand{
                first_sketch,
                *line_b.entity_id})
            .ok());

    CHECK(session.save().ok());
    CHECK(!session.needsSave());

    const auto package =
        persistence::readNativeDocumentContainer(
            path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor
            .domain_schema_version == 6);
    CHECK(
        package.package->authored_json.find(
            "\"next_entity_id\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"entities\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"next_profile_id\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"profiles\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"whole_closed_curve\"") !=
        std::string::npos);

    auto loaded = store.load(path);
    CHECK(loaded.ok());

    const auto* loaded_first =
        loaded.document->findSketch(first_sketch);
    const auto* loaded_second =
        loaded.document->findSketch(second_sketch);
    CHECK(loaded_first != nullptr);
    CHECK(loaded_second != nullptr);

    CHECK(
        loaded_first->model.entityIdCursor()
            .serialized() == "3");
    CHECK(
        loaded_first->model.findLine(
            *line_a.entity_id) != nullptr);
    CHECK(
        loaded_first->model.findLine(
            *line_b.entity_id) == nullptr);

    const auto* loaded_a =
        loaded_first->model.findLine(
            *line_a.entity_id);
    CHECK(loaded_a != nullptr);
    CHECK(
        (loaded_a->start() ==
         sketch::Point2{0.0, 0.0}));
    CHECK(
        (loaded_a->end() ==
         sketch::Point2{10.0, 0.0}));

    CHECK(
        loaded_second->model.findLine(
            *other_local_line.entity_id) != nullptr);
    CHECK(
        *other_local_line.entity_id ==
        *line_a.entity_id);
    CHECK(
        loaded.document->profileIdCursor()
            .serialized() == "2");
    const auto* loaded_profile =
        loaded.document->findProfile(
            *profile_created.profile_id);
    CHECK(loaded_profile != nullptr);
    CHECK(
        loaded_profile->source_sketch_id ==
        second_sketch);
    CHECK(
        loaded_profile->name ==
        "Profile001");
    CHECK(
        loaded.document
            ->evaluateProfile(
                *profile_created.profile_id)
            ->valid());

    application::DocumentSession reopened{
        path,
        std::move(*loaded.document),
        *loaded.checkpoint};

    const auto line_c =
        reopened.execute(
            application::AddSketchLineCommand{
                first_sketch,
                sketch::Point2{2.0, 2.0},
                sketch::Point2{4.0, 2.0}});
    CHECK(line_c.ok());
    CHECK(line_c.entity_id.has_value());
    CHECK(line_c.entity_id->serialized() == "3");
    CHECK(*line_c.entity_id != *line_b.entity_id);

    const auto duplicate_profile =
        reopened.execute(
            application::CreateProfileCommand{
                second_sketch,
                reopened.document().revision(),
                *profile_intent});
    CHECK(duplicate_profile.ok());
    CHECK(duplicate_profile.profile_id.has_value());
    CHECK(
        duplicate_profile.profile_id->serialized() ==
        "2");

    // R9 persistence is ordinary Line/role persistence: a Rectangle with
    // diagonals must reopen with the same six EntityIds and roles, with no
    // Rectangle-specific schema or durable membership.
    const auto rectangle_path =
        temp.path / "R9RectanglePersistence.ss2part";
    auto rectangle_document =
        part::PartDocument::create(
            core::DocumentId::generate());
    const auto rectangle_published =
        store.createNew(
            rectangle_path,
            rectangle_document);
    CHECK(rectangle_published.ok());

    application::DocumentSession rectangle_session{
        rectangle_path,
        std::move(rectangle_document),
        *rectangle_published.checkpoint};
    const auto rectangle_sketch_created =
        rectangle_session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        rectangle_sketch_created.ok() &&
        rectangle_sketch_created.sketch_id.has_value());
    const auto rectangle_sketch =
        *rectangle_sketch_created.sketch_id;

    const auto rectangle_created =
        rectangle_session.execute(
            application::AddSketchRectangleCommand{
                rectangle_sketch,
                rectangle_session.document().revision(),
                {1.0, 2.0},
                {6.0, 8.0},
                sketch::EntityRole::regular,
                true});
    CHECK(rectangle_created.ok());
    CHECK(rectangle_created.changed);
    CHECK(rectangle_created.entity_ids.size() == 6U);
    const auto rectangle_ids =
        rectangle_created.entity_ids;

    CHECK(rectangle_session.save().ok());
    auto rectangle_loaded =
        store.load(rectangle_path);
    CHECK(rectangle_loaded.ok());
    const auto* reopened_rectangle_sketch =
        rectangle_loaded.document->findSketch(
            rectangle_sketch);
    CHECK(reopened_rectangle_sketch != nullptr);
    CHECK(
        reopened_rectangle_sketch->model.entityCount() ==
        6U);
    CHECK(
        reopened_rectangle_sketch->model
            .entityIdCursor().serialized() ==
        "7");

    for (std::size_t index = 0U;
         index < rectangle_ids.size();
         ++index) {
        const auto* line =
            reopened_rectangle_sketch->model.findLine(
                rectangle_ids[index]);
        CHECK(line != nullptr);
        CHECK(
            line->role() ==
            (index < 4U
                 ? sketch::EntityRole::regular
                 : sketch::EntityRole::construction));
    }

    application::DocumentSession
        reopened_rectangle_session{
            rectangle_path,
            std::move(*rectangle_loaded.document),
            *rectangle_loaded.checkpoint};
    const auto after_rectangle =
        reopened_rectangle_session.execute(
            application::AddSketchLineCommand{
                rectangle_sketch,
                {10.0, 0.0},
                {11.0, 0.0}});
    CHECK(
        after_rectangle.ok() &&
        after_rectangle.entity_id.has_value());
    CHECK(
        after_rectangle.entity_id->serialized() ==
        "7");

    const auto legacy_v2_path =
        temp.path / "LegacyV2.ss2part";
    const auto legacy_v2_id =
        core::DocumentId::generate();
    const auto legacy_v2_sketch =
        sketch::SketchId::generate();

    const auto legacy_v2_json =
        authoredWithSketches(
            std::string{
                "[{"
                "\"id\":\""} +
            std::string{legacy_v2_sketch.value()} +
            "\","
            "\"support\":{"
                "\"kind\":\"builtin_origin_plane\","
                "\"builtin_plane\":\"xy_plane\""
            "},"
            "\"placement\":{"
                "\"origin\":[0,0,0],"
                "\"u_axis\":[1,0,0],"
                "\"v_axis\":[0,1,0]"
            "},"
            "\"visible\":true"
            "}]");

    const auto legacy_v2_bytes =
        buildPartPackage(
            legacy_v2_id,
            2,
            legacy_v2_json);
    writeBytes(
        legacy_v2_path,
        legacy_v2_bytes);

    const auto legacy_v2 =
        store.load(legacy_v2_path);
    CHECK(legacy_v2.ok());
    CHECK(
        legacy_v2.document->sketches().size() ==
        1U);
    CHECK(
        legacy_v2.document->sketches()[0]
            .model.entityCount() == 0U);
    CHECK(
        legacy_v2.document->sketches()[0]
            .model.entityIdCursor()
            .serialized() == "1");

    std::ifstream legacy_before_stream{
        legacy_v2_path,
        std::ios::binary};
    const std::string legacy_before{
        std::istreambuf_iterator<char>{
            legacy_before_stream},
        std::istreambuf_iterator<char>{}};
    legacy_before_stream.close();
    CHECK(legacy_before == legacy_v2_bytes);

    CHECK(
        store.save(
            legacy_v2_path,
            *legacy_v2.document,
            *legacy_v2.checkpoint)
            .ok());

    const auto migrated =
        persistence::readNativeDocumentContainer(
            legacy_v2_path);
    CHECK(migrated.ok());
    CHECK(
        migrated.package->descriptor
            .domain_schema_version == 6);
    CHECK(
        migrated.package->authored_json.find(
            "\"model\"") !=
        std::string::npos);
    CHECK(
        migrated.package->authored_json.find(
            "\"next_entity_id\"") !=
        std::string::npos);
    CHECK(
        migrated.package->authored_json.find(
            "\"next_profile_id\"") !=
        std::string::npos);
    CHECK(
        migrated.package->authored_json.find(
            "\"profiles\"") !=
        std::string::npos);

    // Package F: malformed durable Profile intent must fail closed at load.
    // The source Sketch and ProfileId are otherwise structurally valid, so
    // rejection proves the RegionIntent boundary rather than a missing owner.
    const auto malformed_profile_path =
        temp.path / "MalformedProfileIntent.ss2part";
    const auto malformed_profile_document_id =
        core::DocumentId::generate();
    const auto malformed_profile_sketch_id =
        sketch::SketchId::generate();
    const std::string malformed_profile_authored =
        std::string{
            "{"
            "\"properties\":{"
                "\"number\":\"\","
                "\"title\":\"\","
                "\"description\":\"\","
                "\"engineering_revision\":\"\""
            "},"
            "\"presentation\":{"
                "\"builtin_reference_visibility_mask\":15"
            "},"
            "\"sketches\":[{"
                "\"id\":\""} +
        std::string{malformed_profile_sketch_id.value()} +
        "\","
        "\"support\":{"
            "\"kind\":\"builtin_origin_plane\","
            "\"builtin_plane\":\"xy_plane\""
        "},"
        "\"placement\":{"
            "\"origin\":[0,0,0],"
            "\"u_axis\":[1,0,0],"
            "\"v_axis\":[0,1,0]"
        "},"
        "\"visible\":true,"
        "\"model\":{"
            "\"next_entity_id\":\"2\","
            "\"entities\":[{"
                "\"kind\":\"circle\","
                "\"id\":\"1\","
                "\"role\":\"regular\","
                "\"center\":[0,0],"
                "\"radius\":5"
            "}]"
        "}"
        "}],"
        "\"next_profile_id\":\"2\","
        "\"profiles\":[{"
            "\"id\":\"1\","
            "\"source_sketch_id\":\"" +
        std::string{malformed_profile_sketch_id.value()} +
        "\","
        "\"name\":\"Broken\","
        "\"visible\":true,"
        "\"region_intent\":{"
            "\"outer\":[],"
            "\"holes\":[]"
        "}"
        "}]"
        "}";
    writeBytes(
        malformed_profile_path,
        buildPartPackage(
            malformed_profile_document_id,
            6,
            malformed_profile_authored));
    const auto malformed_profile_loaded =
        store.load(malformed_profile_path);
    CHECK(!malformed_profile_loaded.ok());
    CHECK(
        malformed_profile_loaded.diagnostic.code ==
        part::PartStoreErrorCode::malformed_document);

    CHECK(malformedV3ModelRejected(
        temp.path / "DuplicateEntity.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"3\","
        "\"lines\":["
            "{\"id\":\"1\",\"start\":[0,0],\"end\":[1,0]},"
            "{\"id\":\"1\",\"start\":[0,1],\"end\":[1,1]}"
        "]"
        "}"));

    CHECK(malformedV3ModelRejected(
        temp.path / "IdAtCursor.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"2\","
        "\"lines\":["
            "{\"id\":\"2\",\"start\":[0,0],\"end\":[1,0]}"
        "]"
        "}"));

    CHECK(malformedV3ModelRejected(
        temp.path / "NonCanonicalId.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"3\","
        "\"lines\":["
            "{\"id\":\"01\",\"start\":[0,0],\"end\":[1,0]}"
        "]"
        "}"));

    CHECK(malformedV3ModelRejected(
        temp.path / "ZeroId.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"3\","
        "\"lines\":["
            "{\"id\":\"0\",\"start\":[0,0],\"end\":[1,0]}"
        "]"
        "}"));

    CHECK(malformedV3ModelRejected(
        temp.path / "OverflowId.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"3\","
        "\"lines\":["
            "{\"id\":\"18446744073709551616\",\"start\":[0,0],\"end\":[1,0]}"
        "]"
        "}"));

    CHECK(malformedV3ModelRejected(
        temp.path / "ZeroLength.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"2\","
        "\"lines\":["
            "{\"id\":\"1\",\"start\":[5,5],\"end\":[5,5]}"
        "]"
        "}"));

    const auto non_finite_authored =
        authoredWithSketches(
            "[" +
            v3SketchRecord(
                sketch::SketchId::generate(),
                "{"
                "\"next_entity_id\":\"2\","
                "\"lines\":["
                    "{\"id\":\"1\",\"start\":[1e400,0],\"end\":[1,0]}"
                "]"
                "}") +
            "]");
    const auto non_finite_container =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                std::string{
                    core::DocumentId::generate().value()},
                3,
            },
            non_finite_authored);
    CHECK(!non_finite_container.ok());
    CHECK(
        non_finite_container.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            invalid_authored_json);

    return EXIT_SUCCESS;
}
