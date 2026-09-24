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
            << "PERSIST-01 Part store CHECK failed at line "
            << line << ": " << expression << '\n';
        std::abort();
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_part_store_" +
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
    std::string document_id,
    int schema_version,
    std::string authored_json,
    std::string kind = "part") {
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                std::move(kind),
                std::move(document_id),
                schema_version,
            },
            authored_json);
    CHECK(built.ok());
    return *built.bytes;
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path / "Part001.ss2part";

    const auto id =
        core::DocumentId::generate();
    auto document =
        part::PartDocument::create(id);

    {
        part::PartDocumentTransaction tx{
            document};
        core::DocumentProperties properties;
        properties.number = "12-04-117";
        properties.title = "Wał napędowy";
        properties.description = "Main\nshaft";
        properties.engineering_revision = "B";
        tx.setProperties(properties);
        CHECK(
            tx.setBuiltinReferenceVisible(
                core::BuiltinReferenceRole::
                    xy_plane,
                true));
        CHECK(
            tx.setBuiltinReferenceVisible(
                core::BuiltinReferenceRole::
                    x_axis,
                false));
        CHECK(tx.commit().changed);
    }

    part::PartDocumentStore store;
    CHECK(
        store.createNew(
            path,
            document)
            .ok());
    CHECK(
        !store.createNew(
            path,
            document)
             .ok());

    const auto package =
        persistence::readNativeDocumentContainer(
            path);
    CHECK(package.ok());
    CHECK(
        package.package->descriptor.document_kind ==
        "part");
    CHECK(
        package.package->descriptor.document_id ==
        id.value());
    CHECK(
        package.package->descriptor.domain_schema_version ==
        part::PartDocumentStore::
            current_schema_version);
    CHECK(
        package.package->authored_json.find(
            "\"properties\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"presentation\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "\"sketches\"") !=
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "ProjectId") ==
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "TopoDS") ==
        std::string::npos);
    CHECK(
        package.package->authored_json.find(
            "evaluation") ==
        std::string::npos);

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->documentId() == id);
    CHECK(
        loaded.document->properties() ==
        document.properties());
    CHECK(
        loaded.document->revision().value() ==
        0U);
    CHECK(
        loaded.document->builtinReferenceVisible(
            core::BuiltinReferenceRole::
                xy_plane));
    CHECK(
        !loaded.document->builtinReferenceVisible(
            core::BuiltinReferenceRole::
                x_axis));

    {
        part::PartDocumentTransaction tx{
            document};
        auto properties =
            document.properties();
        properties.title =
            "Drive Shaft";
        tx.setProperties(
            std::move(properties));
        CHECK(tx.commit().changed);
    }

    CHECK(store.save(path, document).ok());
    auto reloaded = store.load(path);
    CHECK(reloaded.ok());
    CHECK(
        reloaded.document->documentId() ==
        id);
    CHECK(
        reloaded.document
            ->properties()
            .title ==
        "Drive Shaft");
    CHECK(
        reloaded.document
            ->builtinReferenceVisible(
                core::BuiltinReferenceRole::
                    xy_plane));

    const auto old_bootstrap =
        temp.path / "OldBootstrap.ss2part";
    writeBytes(
        old_bootstrap,
        "SS2PART\n"
        "schema_version=2\n");
    const auto old =
        store.load(old_bootstrap);
    CHECK(!old.ok());
    CHECK(
        old.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    const auto bad_visibility_path =
        temp.path / "BadVisibility.ss2part";
    writeBytes(
        bad_visibility_path,
        buildPartPackage(
            std::string{
                core::DocumentId::generate()
                    .value()},
            part::PartDocumentStore::
                current_schema_version,
            "{"
            "\"properties\":{"
                "\"number\":\"\","
                "\"title\":\"\","
                "\"description\":\"\","
                "\"engineering_revision\":\"\""
            "},"
            "\"presentation\":{"
                "\"builtin_reference_visibility_mask\":128"
            "},"
            "\"sketches\":[]"
            "}"));
    const auto bad_visibility =
        store.load(
            bad_visibility_path);
    CHECK(!bad_visibility.ok());
    CHECK(
        bad_visibility.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    const auto legacy_v1_path =
        temp.path / "LegacyV1.ss2part";
    const auto legacy_v1_id =
        core::DocumentId::generate();
    const auto legacy_v1_bytes =
        buildPartPackage(
            std::string{
                legacy_v1_id.value()},
            1,
            "{"
            "\"properties\":{"
                "\"number\":\"L-1\","
                "\"title\":\"Legacy native v1\","
                "\"description\":\"\","
                "\"engineering_revision\":\"A\""
            "},"
            "\"presentation\":{"
                "\"builtin_reference_visibility_mask\":15"
            "}"
            "}");
    writeBytes(
        legacy_v1_path,
        legacy_v1_bytes);

    const auto legacy_v1 =
        store.load(legacy_v1_path);
    CHECK(legacy_v1.ok());
    CHECK(
        legacy_v1.document->documentId() ==
        legacy_v1_id);
    CHECK(
        legacy_v1.document->properties().title ==
        "Legacy native v1");
    CHECK(legacy_v1.document->sketches().empty());

    std::ifstream legacy_before_stream{
        legacy_v1_path,
        std::ios::binary};
    const std::string legacy_before{
        std::istreambuf_iterator<char>{
            legacy_before_stream},
        std::istreambuf_iterator<char>{}};
    legacy_before_stream.close();

    CHECK(
        legacy_before ==
        legacy_v1_bytes);

    CHECK(
        store.save(
            legacy_v1_path,
            *legacy_v1.document)
            .ok());

    const auto legacy_after =
        persistence::readNativeDocumentContainer(
            legacy_v1_path);
    CHECK(legacy_after.ok());
    CHECK(
        legacy_after.package->descriptor
            .domain_schema_version ==
        part::PartDocumentStore::
            current_schema_version);
    CHECK(
        legacy_after.package->authored_json.find(
            "\"sketches\"") !=
        std::string::npos);

    const auto duplicate_sketch_path =
        temp.path / "DuplicateSketch.ss2part";
    const auto duplicate_sketch_id =
        sketch::SketchId::generate();
    const auto duplicate_record =
        std::string{
            "{"
            "\"id\":\""} +
        std::string{duplicate_sketch_id.value()} +
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
        "}";

    writeBytes(
        duplicate_sketch_path,
        buildPartPackage(
            std::string{
                core::DocumentId::generate()
                    .value()},
            part::PartDocumentStore::
                current_schema_version,
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
                "\"sketches\":["} +
                duplicate_record + "," +
                duplicate_record +
                "]}"));

    const auto duplicate_sketch =
        store.load(
            duplicate_sketch_path);
    CHECK(!duplicate_sketch.ok());
    CHECK(
        duplicate_sketch.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    const auto mismatched_sketch_path =
        temp.path / "MismatchedSketchPlacement.ss2part";
    const auto mismatched_sketch_id =
        sketch::SketchId::generate();

    writeBytes(
        mismatched_sketch_path,
        buildPartPackage(
            std::string{
                core::DocumentId::generate()
                    .value()},
            part::PartDocumentStore::
                current_schema_version,
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
                std::string{
                    mismatched_sketch_id.value()} +
                "\","
                "\"support\":{"
                    "\"kind\":\"builtin_origin_plane\","
                    "\"builtin_plane\":\"xz_plane\""
                "},"
                "\"placement\":{"
                    "\"origin\":[0,0,0],"
                    "\"u_axis\":[1,0,0],"
                    "\"v_axis\":[0,1,0]"
                "},"
                "\"visible\":true"
                "}]"
                "}"));

    const auto mismatched_sketch =
        store.load(
            mismatched_sketch_path);
    CHECK(!mismatched_sketch.ok());
    CHECK(
        mismatched_sketch.diagnostic.code ==
        part::PartStoreErrorCode::
            malformed_document);

    const auto future_schema_path =
        temp.path / "FutureSchema.ss2part";
    writeBytes(
        future_schema_path,
        buildPartPackage(
            std::string{id.value()},
            999,
            "{}"));
    const auto future_schema =
        store.load(
            future_schema_path);
    CHECK(!future_schema.ok());
    CHECK(
        future_schema.diagnostic.code ==
        part::PartStoreErrorCode::
            unsupported_schema);

    const auto wrong_kind_path =
        temp.path / "WrongKind.ss2part";
    writeBytes(
        wrong_kind_path,
        buildPartPackage(
            std::string{id.value()},
            part::PartDocumentStore::
                current_schema_version,
            "{}",
            "drawing"));
    const auto wrong_kind =
        store.load(
            wrong_kind_path);
    CHECK(!wrong_kind.ok());
    CHECK(
        wrong_kind.diagnostic.code ==
        part::PartStoreErrorCode::
            wrong_document_kind);

    const auto invalid_id_path =
        temp.path / "InvalidId.ss2part";
    writeBytes(
        invalid_id_path,
        buildPartPackage(
            "not-a-document-id",
            part::PartDocumentStore::
                current_schema_version,
            "{}"));
    const auto invalid_id =
        store.load(
            invalid_id_path);
    CHECK(!invalid_id.ok());
    CHECK(
        invalid_id.diagnostic.code ==
        part::PartStoreErrorCode::
            invalid_document_id);

    return EXIT_SUCCESS;
}
