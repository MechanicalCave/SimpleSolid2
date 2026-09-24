#include <simplesolid2/persistence/native_document_container.hpp>

#include <miniz.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PERSIST-01 container CHECK failed at line "
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
            ("simplesolid2_native_container_" +
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

std::string makeZip(
    const std::vector<
        std::pair<std::string, std::string>>& entries,
    bool force_zip64 = false) {
    mz_zip_archive zip{};
    mz_zip_zero_struct(&zip);

    CHECK(
        mz_zip_writer_init_heap_v2(
            &zip,
            0U,
            64U * 1024U,
            force_zip64
                ? MZ_ZIP_FLAG_WRITE_ZIP64
                : 0U));

    for (const auto& [name, bytes] : entries) {
        CHECK(
            mz_zip_writer_add_mem(
                &zip,
                name.c_str(),
                bytes.data(),
                bytes.size(),
                MZ_DEFAULT_COMPRESSION));
    }

    void* buffer = nullptr;
    std::size_t size = 0U;
    CHECK(
        mz_zip_writer_finalize_heap_archive(
            &zip,
            &buffer,
            &size));
    CHECK(mz_zip_writer_end(&zip));

    std::string result{
        static_cast<const char*>(buffer),
        size};
    mz_free(buffer);
    return result;
}

std::string manifest(
    int container_version = 1) {
    return
        "{\n"
        "  \"format\": \"simplesolid.native-document\",\n"
        "  \"container_version\": " +
        std::to_string(container_version) +
        ",\n"
        "  \"document_kind\": \"part\",\n"
        "  \"document_id\": "
        "\"00000000-0000-4000-8000-000000000001\",\n"
        "  \"domain_schema_version\": 1\n"
        "}\n";
}

} // namespace

int main() {
    TempDirectory temp;

    const auto descriptor =
        persistence::NativeDocumentDescriptor{
            "part",
            "00000000-0000-4000-8000-000000000001",
            1,
        };

    const auto built =
        persistence::buildNativeDocumentContainer(
            descriptor,
            "{\"hello\":\"world\"}\n");
    CHECK(built.ok());

    const auto valid_path =
        temp.path / "Valid.ss2part";
    writeBytes(valid_path, *built.bytes);

    const auto valid =
        persistence::readNativeDocumentContainer(
            valid_path);
    CHECK(valid.ok());
    CHECK(
        valid.package->descriptor.document_kind ==
        "part");
    CHECK(
        valid.package->descriptor.document_id ==
        descriptor.document_id);
    CHECK(
        valid.package->descriptor.domain_schema_version ==
        1);
    CHECK(
        valid.package->authored_json ==
        "{\"hello\":\"world\"}\n");

    const auto derived_path =
        temp.path / "Derived.ss2part";
    writeBytes(
        derived_path,
        makeZip({
            {"manifest.json", manifest()},
            {"authored/document.json", "{}\n"},
            {"derived/preview.future", "disposable"},
        }));
    CHECK(
        persistence::readNativeDocumentContainer(
            derived_path)
            .ok());

    const auto future_path =
        temp.path / "FutureContainer.ss2part";
    writeBytes(
        future_path,
        makeZip({
            {"manifest.json", manifest(2)},
            {"authored/document.json", "{}\n"},
        }));
    const auto future =
        persistence::readNativeDocumentContainer(
            future_path);
    CHECK(!future.ok());
    CHECK(
        future.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            unsupported_container_version);

    const auto missing_path =
        temp.path / "MissingAuthored.ss2part";
    writeBytes(
        missing_path,
        makeZip({
            {"manifest.json", manifest()},
        }));
    const auto missing =
        persistence::readNativeDocumentContainer(
            missing_path);
    CHECK(!missing.ok());
    CHECK(
        missing.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            missing_authored_payload);

    const auto unsafe_path =
        temp.path / "Unsafe.ss2part";
    writeBytes(
        unsafe_path,
        makeZip({
            {"manifest.json", manifest()},
            {"authored/document.json", "{}\n"},
            {"../outside.bin", "bad"},
        }));
    const auto unsafe =
        persistence::readNativeDocumentContainer(
            unsafe_path);
    CHECK(!unsafe.ok());
    CHECK(
        unsafe.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            unsafe_entry);

    const auto duplicate_path =
        temp.path / "Duplicate.ss2part";
    writeBytes(
        duplicate_path,
        makeZip({
            {"manifest.json", manifest()},
            {"manifest.json", manifest()},
            {"authored/document.json", "{}\n"},
        }));
    const auto duplicate =
        persistence::readNativeDocumentContainer(
            duplicate_path);
    CHECK(!duplicate.ok());
    CHECK(
        duplicate.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            duplicate_entry);

    const auto invalid_manifest_path =
        temp.path / "InvalidManifest.ss2part";
    writeBytes(
        invalid_manifest_path,
        makeZip({
            {"manifest.json", "{broken"},
            {"authored/document.json", "{}\n"},
        }));
    const auto invalid_manifest =
        persistence::readNativeDocumentContainer(
            invalid_manifest_path);
    CHECK(!invalid_manifest.ok());
    CHECK(
        invalid_manifest.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            invalid_manifest);

    const auto invalid_authored_path =
        temp.path / "InvalidAuthored.ss2part";
    writeBytes(
        invalid_authored_path,
        makeZip({
            {"manifest.json", manifest()},
            {"authored/document.json", "{broken"},
        }));
    const auto invalid_authored =
        persistence::readNativeDocumentContainer(
            invalid_authored_path);
    CHECK(!invalid_authored.ok());
    CHECK(
        invalid_authored.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            invalid_authored_json);

    const auto zip64_path =
        temp.path / "Zip64.ss2part";
    writeBytes(
        zip64_path,
        makeZip(
            {
                {"manifest.json", manifest()},
                {"authored/document.json", "{}\n"},
            },
            true));
    const auto zip64 =
        persistence::readNativeDocumentContainer(
            zip64_path);
    CHECK(!zip64.ok());
    CHECK(
        zip64.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            unsupported_zip_feature);

    const auto plain_path =
        temp.path / "Plain.ss2part";
    writeBytes(plain_path, "not a ZIP");
    const auto plain =
        persistence::readNativeDocumentContainer(
            plain_path);
    CHECK(!plain.ok());
    CHECK(
        plain.diagnostic.code ==
        persistence::NativeContainerErrorCode::
            malformed_container);

    return EXIT_SUCCESS;
}
