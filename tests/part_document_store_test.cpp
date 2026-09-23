#include <simplesolid2/part/part_document_store.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "PART-01 Part store CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::abort();
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

struct TempDirectory final {
    std::filesystem::path path;
    TempDirectory() {
        path = std::filesystem::temp_directory_path() /
               ("simplesolid2_part_store_" +
                std::to_string(
                    std::filesystem::file_time_type::clock::now()
                        .time_since_epoch().count()));
        std::filesystem::create_directories(path);
    }
    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

std::string readText(const std::filesystem::path& path) {
    std::ifstream in{path, std::ios::binary};
    CHECK(static_cast<bool>(in));
    return std::string{
        std::istreambuf_iterator<char>{in},
        std::istreambuf_iterator<char>{}};
}

void writeLegacyV1(
    const std::filesystem::path& path,
    const core::DocumentId& id) {
    std::ofstream out{path, std::ios::binary};
    CHECK(static_cast<bool>(out));
    out << "SS2PART\n"
        << "schema_version=1\n"
        << "document_kind=part\n"
        << "document_id=" << id.value() << "\n"
        << "number_hex=\n"
        << "title_hex=\n"
        << "description_hex=\n"
        << "engineering_revision_hex=\n";
}
}

int main() {
    TempDirectory temp;
    const auto path = temp.path / "Part001.ss2part";

    const auto id = core::DocumentId::generate();
    auto document = part::PartDocument::create(id);
    {
        part::PartDocumentTransaction tx{document};
        core::DocumentProperties properties;
        properties.number = "12-04-117";
        properties.title = "Wał napędowy";
        properties.description = "Main\nshaft";
        properties.engineering_revision = "B";
        tx.setProperties(properties);
        CHECK(tx.setBuiltinReferenceVisible(
            core::BuiltinReferenceRole::xy_plane,
            true));
        CHECK(tx.setBuiltinReferenceVisible(
            core::BuiltinReferenceRole::x_axis,
            false));
        CHECK(tx.commit().changed);
    }

    part::PartDocumentStore store;
    CHECK(store.createNew(path, document).ok());
    CHECK(!store.createNew(path, document).ok());

    const auto bytes = readText(path);
    CHECK(bytes.find("SS2PART") != std::string::npos);
    CHECK(bytes.find("schema_version=2") != std::string::npos);
    CHECK(bytes.find("builtin_reference_visibility_mask=") != std::string::npos);
    CHECK(bytes.find("project_id") == std::string::npos);
    CHECK(bytes.find("ProjectId") == std::string::npos);
    CHECK(bytes.find("TopoDS") == std::string::npos);
    CHECK(bytes.find("evaluation") == std::string::npos);

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(loaded.document->documentId() == id);
    CHECK(loaded.document->properties() == document.properties());
    CHECK(loaded.document->revision().value() == 0U);
    CHECK(loaded.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(!loaded.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::x_axis));

    {
        part::PartDocumentTransaction tx{document};
        auto properties = document.properties();
        properties.title = "Drive Shaft";
        tx.setProperties(properties);
        CHECK(tx.commit().changed);
    }
    CHECK(store.save(path, document).ok());
    auto reloaded = store.load(path);
    CHECK(reloaded.ok());
    CHECK(reloaded.document->documentId() == id);
    CHECK(reloaded.document->properties().title == "Drive Shaft");
    CHECK(reloaded.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));

    const auto legacy_path = temp.path / "Legacy.ss2part";
    const auto legacy_id = core::DocumentId::generate();
    writeLegacyV1(legacy_path, legacy_id);
    const auto legacy_bytes_before = readText(legacy_path);

    auto legacy = store.load(legacy_path);
    CHECK(legacy.ok());
    CHECK(legacy.document->documentId() == legacy_id);
    CHECK(legacy.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(legacy.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::x_axis));
    CHECK(!legacy.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));
    CHECK(readText(legacy_path) == legacy_bytes_before);

    CHECK(store.save(legacy_path, *legacy.document).ok());
    const auto legacy_bytes_after = readText(legacy_path);
    CHECK(legacy_bytes_after != legacy_bytes_before);
    CHECK(legacy_bytes_after.find("schema_version=2") != std::string::npos);
    CHECK(legacy_bytes_after.find(
              "builtin_reference_visibility_mask=15") != std::string::npos);

    auto migrated = store.load(legacy_path);
    CHECK(migrated.ok());
    CHECK(migrated.document->documentId() == legacy_id);
    CHECK(migrated.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::origin_point));
    CHECK(!migrated.document->builtinReferenceVisible(
        core::BuiltinReferenceRole::xy_plane));

    const auto invalid = temp.path / "Broken.ss2part";
    {
        std::ofstream out{invalid, std::ios::binary};
        out << "not a native Part\n";
    }
    const auto broken = store.load(invalid);
    CHECK(!broken.ok());
    CHECK(broken.diagnostic.code == part::PartStoreErrorCode::malformed_document);

    const auto invalid_visibility = temp.path / "InvalidVisibility.ss2part";
    {
        std::ofstream out{invalid_visibility, std::ios::binary};
        CHECK(static_cast<bool>(out));
        out << "SS2PART\n"
            << "schema_version=2\n"
            << "document_kind=part\n"
            << "document_id=" << core::DocumentId::generate().value() << "\n"
            << "number_hex=\n"
            << "title_hex=\n"
            << "description_hex=\n"
            << "engineering_revision_hex=\n"
            << "builtin_reference_visibility_mask=128\n";
    }
    const auto bad_visibility = store.load(invalid_visibility);
    CHECK(!bad_visibility.ok());
    CHECK(bad_visibility.diagnostic.code ==
          part::PartStoreErrorCode::malformed_document);

    const auto unsupported = temp.path / "Future.ss2part";
    {
        std::ofstream out{unsupported, std::ios::binary};
        CHECK(static_cast<bool>(out));
        out << "SS2PART\n"
            << "schema_version=999\n"
            << "document_kind=part\n"
            << "document_id=" << id.value() << "\n"
            << "number_hex=\n"
            << "title_hex=\n"
            << "description_hex=\n"
            << "engineering_revision_hex=\n";
    }
    const auto future = store.load(unsupported);
    CHECK(!future.ok());
    CHECK(future.diagnostic.code == part::PartStoreErrorCode::unsupported_schema);

    return EXIT_SUCCESS;
}
