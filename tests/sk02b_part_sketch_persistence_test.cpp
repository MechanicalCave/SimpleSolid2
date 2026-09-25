#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/persistence/native_document_container.hpp>

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
        std::abort();
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
    CHECK(store.createNew(path, document).ok());

    application::DocumentSession session{
        path,
        std::move(document)};

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
            .domain_schema_version == 3);
    CHECK(
        package.package->authored_json.find(
            "\"next_entity_id\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"lines\"") !=
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

    application::DocumentSession reopened{
        path,
        std::move(*loaded.document)};

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
    CHECK(legacy_before == legacy_v2_bytes);

    CHECK(
        store.save(
            legacy_v2_path,
            *legacy_v2.document)
            .ok());

    const auto migrated =
        persistence::readNativeDocumentContainer(
            legacy_v2_path);
    CHECK(migrated.ok());
    CHECK(
        migrated.package->descriptor
            .domain_schema_version == 3);
    CHECK(
        migrated.package->authored_json.find(
            "\"model\"") !=
        std::string::npos);
    CHECK(
        migrated.package->authored_json.find(
            "\"next_entity_id\"") !=
        std::string::npos);

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

    CHECK(malformedV3ModelRejected(
        temp.path / "NonFinite.ss2part",
        store,
        "{"
        "\"next_entity_id\":\"2\","
        "\"lines\":["
            "{\"id\":\"1\",\"start\":[1e400,0],\"end\":[1,0]}"
        "]"
        "}"));

    return EXIT_SUCCESS;
}
