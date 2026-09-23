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
        CHECK(tx.commit().changed);
    }

    part::PartDocumentStore store;
    CHECK(store.createNew(path, document).ok());
    CHECK(!store.createNew(path, document).ok());

    const auto bytes = readText(path);
    CHECK(bytes.find("SS2PART") != std::string::npos);
    CHECK(bytes.find("project_id") == std::string::npos);
    CHECK(bytes.find("ProjectId") == std::string::npos);
    CHECK(bytes.find("TopoDS") == std::string::npos);
    CHECK(bytes.find("evaluation") == std::string::npos);

    auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(loaded.document->documentId() == id);
    CHECK(loaded.document->properties() == document.properties());
    CHECK(loaded.document->revision().value() == 0U);

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

    const auto invalid = temp.path / "Broken.ss2part";
    {
        std::ofstream out{invalid, std::ios::binary};
        out << "not a native Part\n";
    }
    const auto broken = store.load(invalid);
    CHECK(!broken.ok());
    CHECK(broken.diagnostic.code == part::PartStoreErrorCode::malformed_document);

    return EXIT_SUCCESS;
}
