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
            .domain_schema_version ==
        part::PartDocumentStore::
            current_schema_version);
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
            .domain_schema_version ==
        part::PartDocumentStore::
            current_schema_version);
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

    // P21-01/P21-02 RED-first: actual native Part file persistence,
    // not SketchModel-only restoration or an in-memory copied state.
    // v20 intentionally fails to save this intent; accepted v21 must
    // survive a completely new PartDocumentStore::load and session.
    {
        const auto constraint_path =
            temp.path / "R13A1V21ColdRelationPersistence.ss2part";
        application::DocumentSession constraint_session{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto created = constraint_session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(created.ok() && created.sketch_id);
        const auto first = constraint_session.execute(
            application::AddSketchLineCommand{
                *created.sketch_id, {0.0, 0.0}, {10.0, 2.0}});
        CHECK(first.ok() && first.entity_id);
        const auto relation = constraint_session.execute(
            application::AddHorizontalSketchConstraintCommand{
                *created.sketch_id,
                constraint_session.document().revision(), *first.entity_id});
        CHECK(relation.ok() && relation.constraint_id);
        CHECK(relation.constraint_id->serialized() == "1");
        const auto* before =
            constraint_session.document().findSketch(*created.sketch_id);
        CHECK(before != nullptr);
        const auto authored_state = before->model.state();
        CHECK(authored_state.next_constraint_id.serialized() == "2");
        const auto persisted = store.createNew(
            constraint_path, constraint_session.document());
        CHECK(persisted.ok());
        const auto raw =
            persistence::readNativeDocumentContainer(constraint_path);
        CHECK(raw.ok());
        CHECK(raw.package->descriptor.domain_schema_version == 21);
        CHECK(raw.package->authored_json.find(
            "\"next_constraint_id\"") != std::string::npos);
        CHECK(raw.package->authored_json.find(
            "\"constraints\"") != std::string::npos);
        auto cold = store.load(constraint_path);
        CHECK(cold.ok());
        CHECK(cold.document->documentId() ==
              constraint_session.document().documentId());
        const auto* after = cold.document->findSketch(*created.sketch_id);
        CHECK(after != nullptr);
        CHECK(after->model.state() == authored_state);
        CHECK(after->model.findConstraint(*relation.constraint_id) != nullptr);
        CHECK(after->model.constraintIdCursor().serialized() == "2");

        // Deleted relation IDs never recycle, including after cold reopen.
        application::DocumentSession reopened{
            constraint_path, std::move(*cold.document), *cold.checkpoint};
        const auto erased = reopened.execute(
            application::EraseSketchConstraintCommand{
                *created.sketch_id, reopened.document().revision(),
                *relation.constraint_id});
        CHECK(erased.ok() && erased.changed);
        CHECK(reopened.save().ok());
        const auto cold_again = store.load(constraint_path);
        CHECK(cold_again.ok());
        const auto* erased_state =
            cold_again.document->findSketch(*created.sketch_id);
        CHECK(erased_state != nullptr);
        CHECK(erased_state->model.constraintCount() == 0U);
        CHECK(erased_state->model.constraintIdCursor().serialized() == "2");
        CHECK(erased_state->model.state().lines == authored_state.lines);

        // P21-04: checksum-valid native packages with malformed authored
        // relation records must fail atomically at strict Part Load.
        const auto original = raw.package->authored_json;
        const auto arrayKey = original.find("\"constraints\":");
        CHECK(arrayKey != std::string::npos);
        const auto arrayStart = original.find('[', arrayKey);
        const auto arrayEnd = original.find(']', arrayStart);
        CHECK(arrayStart != std::string::npos &&
              arrayEnd != std::string::npos);
        const auto invalidArray = [&](std::string name,
                                      const std::string& replacement) {
            auto authored = original;
            authored.replace(arrayStart, arrayEnd - arrayStart + 1U,
                             replacement);
            const auto badPath = temp.path / (name + ".ss2part");
            writeBytes(badPath, buildPartPackage(
                constraint_session.document().documentId(), 21, authored));
            const auto failed = store.load(badPath);
            CHECK(!failed.ok());
            CHECK(failed.diagnostic.code ==
                  part::PartStoreErrorCode::malformed_document);
            CHECK(failed.document == std::nullopt);
        };
        const std::string lineRef =
            R"({"entity_id":"1","element":"line"})";
        const std::string endpointRef =
            R"({"entity_id":"1","element":"line_end"})";
        const auto makeRelation = [](std::string id, std::string kind,
                                     std::string first, std::string second) {
            return "{\"id\":\"" + id + "\",\"kind\":\"" + kind +
                "\",\"first\":" + first + ",\"second\":" + second + "}";
        };
        invalidArray("P21UnknownKind", "[" +
            makeRelation("1", "future", lineRef, "null") + "]");
        invalidArray("P21NonCanonicalId", "[" +
            makeRelation("01", "horizontal", lineRef, "null") + "]");
        invalidArray("P21MissingLine", "[" +
            makeRelation("1", "horizontal",
                R"({"entity_id":"999","element":"line"})", "null") + "]");
        invalidArray("P21WrongHorizontalReference", "[" +
            makeRelation("1", "horizontal", endpointRef, "null") + "]");
        invalidArray("P21UnaryHasSecond", "[" +
            makeRelation("1", "horizontal", lineRef, lineRef) + "]");
        invalidArray("P21BinaryMissingSecond", "[" +
            makeRelation("1", "parallel", lineRef, "null") + "]");
        invalidArray("P21DuplicateId", "[" +
            makeRelation("1", "horizontal", lineRef, "null") + "," +
            makeRelation("1", "horizontal", lineRef, "null") + "]");
        invalidArray("P21MissingRequiredField", std::string{"["} +
            R"({"id":"1","kind":"horizontal","first":{"entity_id":"1","element":"line"}})" + "]");
        invalidArray("P21ExtraField", std::string{"["} +
            R"({"id":"1","kind":"horizontal","first":{"entity_id":"1","element":"line"},"second":null,"future":7})" + "]");
        invalidArray("P21WrongRefShape", "[" +
            makeRelation("1", "horizontal",
                R"({"entity_id":"1","element":"line","extra":true})", "null") +
            "]");
        invalidArray("P21InvalidFirstType", "[" +
            makeRelation("1", "horizontal", "null", "null") + "]");
        invalidArray("P21WrongArrayType", "{}");
        invalidArray("P21UnknownElement", "[" +
            makeRelation("1", "horizontal",
                R"({"entity_id":"1","element":"future"})", "null") + "]");
        invalidArray("P21ForbiddenNoneElement", "[" +
            makeRelation("1", "horizontal",
                R"({"entity_id":"1","element":"none"})", "null") + "]");
        invalidArray("P21MissingRefField", "[" +
            makeRelation("1", "horizontal",
                R"({"entity_id":"1"})", "null") + "]");
        invalidArray("P21WrongRefEntityType", "[" +
            makeRelation("1", "horizontal",
                R"({"entity_id":1,"element":"line"})", "null") + "]");
        invalidArray("P21SameLineBinaryPair", "[" +
            makeRelation("1", "parallel", lineRef, lineRef) + "]");
        invalidArray("P21CoincidentWholeLine", "[" +
            makeRelation("1", "coincident", lineRef, endpointRef) + "]");
        invalidArray("P21NegativeId", "[" +
            makeRelation("-1", "horizontal", lineRef, "null") + "]");
        invalidArray("P21OverflowId", "[" +
            makeRelation("18446744073709551616", "horizontal",
                lineRef, "null") + "]");

        // Both IDs and cursor are valid here. Reject semantic duplication
        // and real authored contradictions, not merely a bad cursor.
        const auto invalidPair = [&](std::string name,
                                     const std::string& second) {
            auto authored = original;
            const auto first = makeRelation(
                "1", "horizontal", lineRef, "null");
            authored.replace(arrayStart, arrayEnd - arrayStart + 1U,
                "[" + first + "," + second + "]");
            const std::string cursorOld =
                "\"next_constraint_id\": \"2\"";
            const auto at = authored.find(cursorOld);
            CHECK(at != std::string::npos);
            CHECK(authored.find(cursorOld, at + cursorOld.size()) ==
                  std::string::npos);
            authored.replace(at, cursorOld.size(),
                "\"next_constraint_id\": \"3\"");
            const auto path = temp.path / (name + ".ss2part");
            writeBytes(path, buildPartPackage(
                constraint_session.document().documentId(), 21, authored));
            const auto result = store.load(path);
            CHECK(!result.ok());
            CHECK(result.diagnostic.code ==
                  part::PartStoreErrorCode::malformed_document);
            CHECK(!result.document);
        };
        invalidPair("P21DuplicateMeaningDifferentId",
            makeRelation("2", "horizontal", lineRef, "null"));
        invalidPair("P21HorizontalVerticalConflict",
            makeRelation("2", "vertical", lineRef, "null"));
        invalidPair("P21FixedNonHorizontalConflict",
            makeRelation("2", "fix", lineRef, "null"));

        const auto replaceOnce = [](std::string& value,
                                    const std::string& from,
                                    const std::string& to) {
            const auto at = value.find(from);
            CHECK(at != std::string::npos);
            CHECK(value.find(from, at + from.size()) == std::string::npos);
            value.replace(at, from.size(), to);
        };
        const auto invalidCursor = [&](std::string name,
                                       const std::string& cursor) {
            auto authored = original;
            replaceOnce(authored, "\"next_constraint_id\": \"2\"",
                "\"next_constraint_id\": \"" + cursor + "\"");
            const auto badPath = temp.path / (name + ".ss2part");
            writeBytes(badPath, buildPartPackage(
                constraint_session.document().documentId(), 21, authored));
            const auto failed = store.load(badPath);
            CHECK(!failed.ok());
            CHECK(failed.diagnostic.code ==
                  part::PartStoreErrorCode::malformed_document);
        };
        invalidCursor("P21IdAtCursor", "1");
        invalidCursor("P21NonCanonicalCursor", "01");
        invalidCursor("P21ZeroCursor", "0");
        invalidCursor("P21OverflowCursor", "18446744073709551616");
        invalidCursor("P21NegativeCursor", "-1");

        const auto invalidModel = [&](std::string name,
                                      const std::string& before,
                                      const std::string& after) {
            auto authored = original;
            replaceOnce(authored, before, after);
            const auto badPath = temp.path / (name + ".ss2part");
            writeBytes(badPath, buildPartPackage(
                constraint_session.document().documentId(), 21, authored));
            const auto failed = store.load(badPath);
            CHECK(!failed.ok());
            CHECK(failed.diagnostic.code ==
                  part::PartStoreErrorCode::malformed_document);
            CHECK(!failed.document);
        };
        invalidModel("P21MissingModelCursor",
            "\"next_constraint_id\": \"2\",\n", "");
        invalidModel("P21ExtraModelField",
            "\"next_constraint_id\": \"2\"",
            "\"next_constraint_id\": \"2\", \"future_field\": true");
        invalidModel("P21CursorAsNumber",
            "\"next_constraint_id\": \"2\"",
            "\"next_constraint_id\": 2");

        // P21-03: a REAL native v20 container must retain its exact
        // two-key Sketch model on Load and upgrade only by explicit Save.
        // Reconstruct the old v20 model bytes from this fixture's identical
        // authored seed after the constraint is erased. No solver runs.
        const auto savedWithoutRelation =
            persistence::readNativeDocumentContainer(constraint_path);
        CHECK(savedWithoutRelation.ok());
        auto v20Authored = savedWithoutRelation.package->authored_json;
        replaceOnce(v20Authored, "\"constraints\": [],\n", "");
        replaceOnce(v20Authored, "\"next_constraint_id\": \"2\",\n", "");
        const auto v20Path = temp.path / "P21V20Migration.ss2part";
        writeBytes(v20Path, buildPartPackage(
            constraint_session.document().documentId(), 20, v20Authored));
        auto oldLoaded = store.load(v20Path);
        CHECK(oldLoaded.ok());
        const auto oldSketch =
            oldLoaded.document->findSketch(*created.sketch_id);
        CHECK(oldSketch != nullptr);
        CHECK(oldSketch->model.constraintCount() == 0U);
        CHECK(oldSketch->model.constraintIdCursor().serialized() == "1");
        CHECK(oldSketch->model.state().lines == authored_state.lines);
        const auto preSave =
            persistence::readNativeDocumentContainer(v20Path);
        CHECK(preSave.ok());
        CHECK(preSave.package->descriptor.domain_schema_version == 20);
        CHECK(preSave.package->authored_json == v20Authored);
        CHECK(store.save(
            v20Path, *oldLoaded.document, *oldLoaded.checkpoint).ok());
        const auto upgraded =
            persistence::readNativeDocumentContainer(v20Path);
        CHECK(upgraded.ok());
        CHECK(upgraded.package->descriptor.domain_schema_version == 21);
        CHECK(upgraded.package->authored_json.find(
            "\"next_constraint_id\": \"1\"") != std::string::npos);
        const auto reopenedV21 = store.load(v20Path);
        CHECK(reopenedV21.ok());
        CHECK(reopenedV21.document->findSketch(*created.sketch_id)
            ->model.state().lines == authored_state.lines);

        // A v20 label must NEVER allow invisible v21 relations to disappear.
        const auto mislabeled = temp.path / "P21V20RejectSmuggledFields.ss2part";
        writeBytes(mislabeled, buildPartPackage(
            constraint_session.document().documentId(), 20,
            savedWithoutRelation.package->authored_json));
        const auto smuggled = store.load(mislabeled);
        CHECK(!smuggled.ok());
        CHECK(smuggled.diagnostic.code ==
              part::PartStoreErrorCode::malformed_document);
    }

    // P21-09: a real v21 relation-bearing native file uses the SAME
    // optimistic Save checkpoint protocol; external in-place modification
    // cannot be silently overwritten and cannot clear unsaved Undo history.
    {
        const auto conflictPath =
            temp.path / "P21RelationCheckpointConflict.ss2part";
        CHECK(store.createNew(
            conflictPath, part::PartDocument::create(
                core::DocumentId::generate())).ok());
        auto firstLoad = store.load(conflictPath);
        CHECK(firstLoad.ok());
        application::DocumentSession conflictSession{
            conflictPath, std::move(*firstLoad.document),
            *firstLoad.checkpoint};
        const auto made = conflictSession.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
        CHECK(made.ok() && made.sketch_id);
        const auto line = conflictSession.execute(
            application::AddSketchLineCommand{
                *made.sketch_id, {1.0, 2.0}, {5.0, 3.0}});
        CHECK(line.ok() && line.entity_id);
        const auto relation = conflictSession.execute(
            application::AddHorizontalSketchConstraintCommand{
                *made.sketch_id, conflictSession.document().revision(),
                *line.entity_id});
        CHECK(relation.ok() && relation.constraint_id);
        CHECK(conflictSession.save().ok());
        CHECK(!conflictSession.needsSave());

        // Add another line so a conflict is checked against genuinely
        // dirty authored intent while retaining the existing relation.
        const auto more = conflictSession.execute(
            application::AddSketchLineCommand{
                *made.sketch_id, {6.0, 4.0}, {9.0, 5.0}});
        CHECK(more.ok() && more.entity_id);
        CHECK(conflictSession.needsSave());
        const auto beforeConflict = conflictSession.document().state();
        const auto revisionBefore = conflictSession.document().revision();
        const auto undoBefore = conflictSession.undoDepth();

        // Append without replacing the NTFS file object. Same identity but
        // different bytes must map to save_conflict_content_changed.
        {
            std::ofstream tamper{conflictPath,
                std::ios::binary | std::ios::app};
            CHECK(static_cast<bool>(tamper));
            tamper.put('X');
            CHECK(static_cast<bool>(tamper));
        }
        std::ifstream tamperedStream{conflictPath, std::ios::binary};
        CHECK(static_cast<bool>(tamperedStream));
        const std::string tamperedBytes{
            std::istreambuf_iterator<char>{tamperedStream},
            std::istreambuf_iterator<char>{}};
        CHECK(!tamperedBytes.empty() && tamperedBytes.back() == 'X');

        const auto rejected = conflictSession.save();
        CHECK(!rejected.ok());
        CHECK(rejected.diagnostic.code ==
            application::DocumentSessionErrorCode::save_conflict);
        CHECK(rejected.diagnostic.store_code ==
            part::PartStoreErrorCode::save_conflict_content_changed);
        CHECK(conflictSession.needsSave());
        CHECK(conflictSession.document().state() == beforeConflict);
        CHECK(conflictSession.document().revision() == revisionBefore);
        CHECK(conflictSession.undoDepth() == undoBefore);
        CHECK(conflictSession.document().findSketch(*made.sketch_id)
            ->model.findConstraint(*relation.constraint_id) != nullptr);
        std::ifstream postReject{conflictPath, std::ios::binary};
        CHECK(static_cast<bool>(postReject));
        const std::string afterRejectBytes{
            std::istreambuf_iterator<char>{postReject},
            std::istreambuf_iterator<char>{}};
        CHECK(afterRejectBytes == tamperedBytes);
    }

    // P21-01: every A1 kind (including immutable Fix and endpoint
    // Coincident) survives a real cold native Part round trip with exact
    // authored order and semantic IDs; independent Sketches may BOTH own
    // their own ConstraintId "1" without any global ID collision.
    {
        application::DocumentSession multi{
            {}, part::PartDocument::create(core::DocumentId::generate())};
        const auto xy = multi.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xy_plane});
        const auto xz = multi.execute(application::CreatePartSketchCommand{
            core::BuiltinReferenceRole::xz_plane});
        CHECK(xy.ok() && xy.sketch_id && xz.ok() && xz.sketch_id);
        const auto a = multi.execute(application::AddSketchLineCommand{
            *xy.sketch_id, {0.0, 0.0}, {10.0, 0.0}});
        const auto b = multi.execute(application::AddSketchLineCommand{
            *xy.sketch_id, {10.0, 0.0}, {10.0, 8.0}});
        const auto c = multi.execute(application::AddSketchLineCommand{
            *xy.sketch_id, {5.0, 5.0}, {15.0, 5.0}});
        const auto outside = multi.execute(application::AddSketchLineCommand{
            *xz.sketch_id, {0.0, 0.0}, {8.0, 2.0}});
        CHECK(a.ok() && a.entity_id && b.ok() && b.entity_id &&
              c.ok() && c.entity_id &&
              outside.ok() && outside.entity_id);
        auto multiState = multi.document().state();
        CHECK(multiState.sketches.size() == 2U);
        CHECK(multiState.sketches[0].id == *xy.sketch_id);
        CHECK(multiState.sketches[1].id == *xz.sketch_id);
        auto& authored = multiState.sketches[0].model;
        const auto ref = [](sketch::EntityId id,
                            sketch::ConstraintElement element) {
            return sketch::ConstraintReference{id, element};
        };
        const auto whole = sketch::ConstraintElement::line;
        const auto start = sketch::ConstraintElement::line_start;
        const auto end = sketch::ConstraintElement::line_end;
        CHECK(authored.addConstraint(
            sketch::ConstraintKind::horizontal, ref(*a.entity_id, whole)));
        CHECK(authored.addConstraint(
            sketch::ConstraintKind::vertical, ref(*b.entity_id, whole)));
        CHECK(authored.addConstraint(
            sketch::ConstraintKind::coincident, ref(*a.entity_id, end),
            ref(*b.entity_id, start)));
        CHECK(authored.addConstraint(
            sketch::ConstraintKind::parallel, ref(*a.entity_id, whole),
            ref(*c.entity_id, whole)));
        CHECK(authored.addConstraint(
            sketch::ConstraintKind::perpendicular, ref(*a.entity_id, whole),
            ref(*b.entity_id, whole)));
        CHECK(authored.addConstraint(
            sketch::ConstraintKind::fix, ref(*a.entity_id, whole)));
        CHECK(authored.constraintCount() == 6U);
        CHECK(authored.isFixedLine(*a.entity_id));
        CHECK(authored.constraintIdCursor().serialized() == "7");
        auto& secondModel = multiState.sketches[1].model;
        const auto separate = secondModel.addHorizontal(*outside.entity_id);
        CHECK(separate && separate->serialized() == "1");
        const auto firstState = authored.state();
        const auto secondState = secondModel.state();
        CHECK(firstState.constraints.front().id.serialized() == "1");
        const auto restored = part::PartDocument::restore(
            multi.document().documentId(), std::move(multiState),
            multi.document().revision());
        CHECK(restored.ok() && restored.document);
        const auto pathSix =
            temp.path / "P21AllSixRelationsTwoSketches.ss2part";
        CHECK(store.createNew(pathSix, *restored.document).ok());
        const auto packageSix =
            persistence::readNativeDocumentContainer(pathSix);
        CHECK(packageSix.ok());
        CHECK(packageSix.package->descriptor.domain_schema_version == 21);
        for (const std::string kind : {
            "horizontal", "vertical", "coincident",
            "parallel", "perpendicular", "fix"}) {
            CHECK(packageSix.package->authored_json.find(
                "\"kind\": \"" + kind + "\"") != std::string::npos);
        }
        const auto coldSix = store.load(pathSix);
        CHECK(coldSix.ok());
        const auto* firstCold =
            coldSix.document->findSketch(*xy.sketch_id);
        const auto* secondCold =
            coldSix.document->findSketch(*xz.sketch_id);
        CHECK(firstCold && secondCold);
        CHECK(firstCold->model.state() == firstState);
        CHECK(secondCold->model.state() == secondState);
        CHECK(firstCold->model.isFixedLine(*a.entity_id));
        CHECK(firstCold->model.constraintIdCursor().serialized() == "7");
        CHECK(secondCold->model.constraintIdCursor().serialized() == "2");
        CHECK(coldSix.document->documentId() == multi.document().documentId());

        // An EntityId belonging ONLY to a different Sketch cannot satisfy
        // a reference in this Sketch, even when the numeric token exists
        // elsewhere in the same Part. Restore must reject the WHOLE file.
        CHECK(b.entity_id->serialized() == "2");
        CHECK(secondState.lines.size() == 1U);
        CHECK(secondState.lines.front().id.serialized() == "1");
        auto nonlocal = packageSix.package->authored_json;
        const auto secondConstraints = nonlocal.rfind("\"constraints\":");
        CHECK(secondConstraints != std::string::npos);
        const auto arrayOpen = nonlocal.find('[', secondConstraints);
        const auto arrayClose = nonlocal.find(']', arrayOpen);
        CHECK(arrayOpen != std::string::npos &&
              arrayClose != std::string::npos);
        nonlocal.replace(arrayOpen, arrayClose - arrayOpen + 1U,
            R"([{"id":"1","kind":"horizontal","first":{"entity_id":"2","element":"line"},"second":null}])");
        const auto crossPath =
            temp.path / "P21NonlocalSketchEntityReference.ss2part";
        writeBytes(crossPath, buildPartPackage(
            multi.document().documentId(), 21, nonlocal));
        const auto cross = store.load(crossPath);
        CHECK(!cross.ok());
        CHECK(cross.diagnostic.code ==
              part::PartStoreErrorCode::malformed_document);
        CHECK(!cross.document);
    }

    // P21-08: the Owner's original linked PG-01 native v16 Part fixture
    // provides REAL cross-Feature projected Edge bindings. The old
    // modeling-semantics v1 is intentionally unsupported by current v2;
    // convert a TEST-ONLY COPY (same method as pm02jr2), never rewrite the
    // checked-in fixture or relax the production policy. A v21 relation
    // authored in that bound Sketch must not unlink or rewrite its sources.
    {
        const auto fixture = std::filesystem::path{__FILE__}
            .parent_path() / "fixtures" /
            "pg01d_same_domain_extrude_add_v16.ss2part";
        CHECK(std::filesystem::exists(fixture));
        const auto originalFixture =
            persistence::readNativeDocumentContainer(fixture);
        CHECK(originalFixture.ok());
        CHECK(originalFixture.package->descriptor.domain_schema_version == 16);
        auto converted = originalFixture.package->authored_json;
        const std::string semanticsKey = "\"modeling_semantics_version\"";
        const auto keyAt = converted.find(semanticsKey);
        CHECK(keyAt != std::string::npos);
        CHECK(converted.find(semanticsKey, keyAt + semanticsKey.size()) ==
              std::string::npos);
        const auto colon = converted.find(':', keyAt + semanticsKey.size());
        CHECK(colon != std::string::npos);
        const auto valueAt = converted.find_first_not_of(
            " \t\r\n", colon + 1U);
        CHECK(valueAt != std::string::npos && converted[valueAt] == '1');
        CHECK(valueAt + 1U < converted.size());
        CHECK(converted[valueAt + 1U] == ',' ||
              converted[valueAt + 1U] == '\n' ||
              converted[valueAt + 1U] == '\r');
        converted.replace(valueAt, 1U,
            std::to_string(part::current_modeling_semantics_version.value));
        const auto legacyCopy = temp.path / "P21Pg01OwnerV16V2Copy.ss2part";
        writeBytes(legacyCopy, buildPartPackage(
            *core::DocumentId::parse(
                originalFixture.package->descriptor.document_id),
            16, converted));
        auto oldProjection = store.load(legacyCopy);
        CHECK(oldProjection.ok());
        CHECK(oldProjection.document->state().sketches.size() >= 2U);
        const auto& boundSketch =
            oldProjection.document->state().sketches[1];
        CHECK(boundSketch.projection_bindings.size() == 4U);
        const auto linkedId = boundSketch.id;
        const auto beforeBindings = boundSketch.projection_bindings;
        const auto beforeSupport = boundSketch.support;
        const auto beforeSources = boundSketch.model.state();
        const auto beforeBody = oldProjection.document->state().body;
        const auto oldDocumentId = oldProjection.document->documentId();

        // Add a NEW regular user Line and local relation to this Sketch,
        // NOT a constraint referencing/rewriting a projected source.
        application::DocumentSession projectedSession{
            legacyCopy, std::move(*oldProjection.document),
            *oldProjection.checkpoint};
        const auto userLine = projectedSession.execute(
            application::AddSketchLineCommand{
                linkedId, {21.0, 22.0}, {27.0, 23.0}});
        CHECK(userLine.ok() && userLine.entity_id);
        const auto horizontal = projectedSession.execute(
            application::AddHorizontalSketchConstraintCommand{
                linkedId, projectedSession.document().revision(),
                *userLine.entity_id});
        CHECK(horizontal.ok() && horizontal.constraint_id);
        CHECK(horizontal.constraint_id->serialized() == "1");
        CHECK(projectedSession.document().findSketch(linkedId)
            ->projection_bindings == beforeBindings);
        CHECK(projectedSession.document().findSketch(linkedId)
            ->support == beforeSupport);
        CHECK(projectedSession.save().ok());

        // Explicit Save migrates the v16 TEST COPY to v21, while the
        // checked-in original retains its historical v16 bytes.
        const auto upgraded = persistence::readNativeDocumentContainer(
            legacyCopy);
        CHECK(upgraded.ok());
        CHECK(upgraded.package->descriptor.domain_schema_version == 21);
        const auto originalAfter =
            persistence::readNativeDocumentContainer(fixture);
        CHECK(originalAfter.ok());
        CHECK(originalAfter.package->descriptor.domain_schema_version == 16);
        CHECK(originalAfter.package->authored_json ==
              originalFixture.package->authored_json);
        const auto cold = store.load(legacyCopy);
        CHECK(cold.ok());
        CHECK(cold.document->documentId() == oldDocumentId);
        CHECK(cold.document->state().body == beforeBody);
        const auto* coldBound = cold.document->findSketch(linkedId);
        CHECK(coldBound != nullptr);
        CHECK(coldBound->support == beforeSupport);
        CHECK(coldBound->projection_bindings == beforeBindings);
        CHECK(coldBound->model.constraintCount() == 1U);
        CHECK(coldBound->model.findConstraint(*horizontal.constraint_id));
        CHECK(coldBound->model.constraintIdCursor().serialized() == "2");
        CHECK(coldBound->model.state().lines.size() ==
              beforeSources.lines.size() + 1U);
        for (const auto& prior : beforeSources.lines) {
            const auto* preserved = coldBound->model.findLine(prior.id);
            CHECK(preserved != nullptr);
            CHECK(preserved->id() == prior.id);
            CHECK(preserved->start() == prior.start);
            CHECK(preserved->end() == prior.end);
            CHECK(preserved->role() == prior.role);
        }
        // A stored projected source is ONLY a durable binding/seed.
        // This test does not invent a successful current-stage resolver
        // and does not claim an OCCT projection has been recalculated.
    }

    return EXIT_SUCCESS;
}
