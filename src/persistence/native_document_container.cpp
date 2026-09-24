#include <simplesolid2/persistence/native_document_container.hpp>

#include <miniz.h>
#include <nlohmann/json.hpp>

#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace simplesolid2::persistence {
namespace {

constexpr std::uintmax_t maximum_container_bytes =
    64U * 1024U * 1024U;
constexpr std::uint64_t maximum_total_uncompressed_bytes =
    64U * 1024U * 1024U;
constexpr std::uint64_t maximum_manifest_bytes =
    64U * 1024U;
constexpr std::uint64_t maximum_authored_bytes =
    32U * 1024U * 1024U;
constexpr mz_uint maximum_entries = 128U;
constexpr mz_uint maximum_entry_name_bytes = 256U;

constexpr std::string_view manifest_entry = "manifest.json";
constexpr std::string_view authored_entry = "authored/document.json";
constexpr std::string_view derived_prefix = "derived/";

NativeContainerReadResult readFailure(
    NativeContainerErrorCode code,
    std::string message,
    std::filesystem::path path) {
    return NativeContainerReadResult{
        std::nullopt,
        NativeContainerDiagnostic{
            code,
            std::move(message),
            std::move(path),
        },
    };
}

NativeContainerBuildResult buildFailure(
    NativeContainerErrorCode code,
    std::string message) {
    return NativeContainerBuildResult{
        std::nullopt,
        NativeContainerDiagnostic{
            code,
            std::move(message),
            {},
        },
    };
}

bool safeEntryName(std::string_view name) {
    if (name.empty() ||
        name.size() > maximum_entry_name_bytes ||
        name.front() == '/' ||
        name.find('\\') != std::string_view::npos ||
        name.find(':') != std::string_view::npos) {
        return false;
    }

    std::size_t start = 0;
    while (start <= name.size()) {
        const auto slash = name.find('/', start);
        const auto end =
            slash == std::string_view::npos
                ? name.size()
                : slash;
        const auto segment = name.substr(start, end - start);

        if (segment.empty() ||
            segment == "." ||
            segment == "..") {
            return false;
        }

        if (slash == std::string_view::npos) {
            break;
        }
        start = slash + 1U;
    }

    return true;
}

bool allowedFileEntry(std::string_view name) {
    return name == manifest_entry ||
           name == authored_entry ||
           (name.starts_with(derived_prefix) &&
            name.size() > derived_prefix.size());
}

std::optional<std::string> readFileBounded(
    const std::filesystem::path& path,
    NativeContainerDiagnostic& diagnostic) {
    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        diagnostic = NativeContainerDiagnostic{
            ec
                ? NativeContainerErrorCode::io_failure
                : NativeContainerErrorCode::not_found,
            ec
                ? "Unable to inspect native Document file"
                : "Native Document file does not exist",
            path,
        };
        return std::nullopt;
    }

    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::malformed_container,
            "Native Document path is not a regular file",
            path,
        };
        return std::nullopt;
    }

    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::io_failure,
            "Unable to inspect native Document file size",
            path,
        };
        return std::nullopt;
    }
    if (size > maximum_container_bytes) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::too_large,
            "Native Document container exceeds the supported size",
            path,
        };
        return std::nullopt;
    }

    std::ifstream in{path, std::ios::binary};
    if (!in) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::io_failure,
            "Unable to open native Document file",
            path,
        };
        return std::nullopt;
    }

    std::string bytes(static_cast<std::size_t>(size), '\0');
    if (!bytes.empty()) {
        in.read(
            bytes.data(),
            static_cast<std::streamsize>(bytes.size()));
    }
    if (!in && !in.eof()) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::io_failure,
            "Unable to read native Document file",
            path,
        };
        return std::nullopt;
    }

    return bytes;
}

std::optional<std::string> entryName(
    mz_zip_archive& zip,
    mz_uint index) {
    const auto required =
        mz_zip_reader_get_filename(
            &zip,
            index,
            nullptr,
            0);
    if (required == 0U ||
        required > maximum_entry_name_bytes + 1U) {
        return std::nullopt;
    }

    std::vector<char> buffer(required, '\0');
    if (mz_zip_reader_get_filename(
            &zip,
            index,
            buffer.data(),
            required) == 0U) {
        return std::nullopt;
    }

    return std::string{buffer.data()};
}

std::optional<std::string> extractEntry(
    mz_zip_archive& zip,
    mz_uint index,
    std::uint64_t expected_size) {
    if (expected_size >
        static_cast<std::uint64_t>(
            std::numeric_limits<std::size_t>::max())) {
        return std::nullopt;
    }

    std::string result(
        static_cast<std::size_t>(expected_size),
        '\0');
    if (result.empty()) {
        return result;
    }

    if (!mz_zip_reader_extract_to_mem(
            &zip,
            index,
            result.data(),
            result.size(),
            0U)) {
        return std::nullopt;
    }

    return result;
}

std::optional<int> positiveInt(
    const nlohmann::json& value) {
    if (!value.is_number_integer() &&
        !value.is_number_unsigned()) {
        return std::nullopt;
    }

    const auto parsed =
        value.get<std::int64_t>();
    if (parsed <= 0 ||
        parsed >
            static_cast<std::int64_t>(
                std::numeric_limits<int>::max())) {
        return std::nullopt;
    }

    return static_cast<int>(parsed);
}

std::optional<NativeDocumentDescriptor> parseManifest(
    const std::string& text,
    NativeContainerDiagnostic& diagnostic,
    const std::filesystem::path& path) {
    const auto manifest =
        nlohmann::json::parse(
            text,
            nullptr,
            false);
    if (manifest.is_discarded() ||
        !manifest.is_object()) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document manifest is not valid JSON object",
            path,
        };
        return std::nullopt;
    }

    constexpr std::array<std::string_view, 5> fields{
        "format",
        "container_version",
        "document_kind",
        "document_id",
        "domain_schema_version",
    };

    if (manifest.size() != fields.size()) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document manifest contains unknown or missing fields",
            path,
        };
        return std::nullopt;
    }

    for (const auto field : fields) {
        if (!manifest.contains(std::string{field})) {
            diagnostic = NativeContainerDiagnostic{
                NativeContainerErrorCode::invalid_manifest,
                "Native Document manifest is missing a required field",
                path,
            };
            return std::nullopt;
        }
    }

    if (!manifest["format"].is_string() ||
        manifest["format"].get<std::string>() !=
            native_document_format) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document manifest has an invalid format identifier",
            path,
        };
        return std::nullopt;
    }

    const auto container_version =
        positiveInt(manifest["container_version"]);
    if (!container_version) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document container_version is invalid",
            path,
        };
        return std::nullopt;
    }
    if (*container_version !=
        native_document_container_version) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::unsupported_container_version,
            "Unsupported native Document container version",
            path,
        };
        return std::nullopt;
    }

    if (!manifest["document_kind"].is_string() ||
        !manifest["document_id"].is_string()) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document kind or identity has an invalid JSON type",
            path,
        };
        return std::nullopt;
    }

    auto document_kind =
        manifest["document_kind"].get<std::string>();
    auto document_id =
        manifest["document_id"].get<std::string>();
    if (document_kind.empty() ||
        document_id.empty()) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document kind and identity must not be empty",
            path,
        };
        return std::nullopt;
    }

    const auto domain_schema_version =
        positiveInt(manifest["domain_schema_version"]);
    if (!domain_schema_version) {
        diagnostic = NativeContainerDiagnostic{
            NativeContainerErrorCode::invalid_manifest,
            "Native Document domain_schema_version is invalid",
            path,
        };
        return std::nullopt;
    }

    return NativeDocumentDescriptor{
        std::move(document_kind),
        std::move(document_id),
        *domain_schema_version,
    };
}

} // namespace

NativeContainerReadResult readNativeDocumentContainer(
    const std::filesystem::path& path) {
    NativeContainerDiagnostic diagnostic;
    const auto bytes =
        readFileBounded(path, diagnostic);
    if (!bytes) {
        return NativeContainerReadResult{
            std::nullopt,
            std::move(diagnostic),
        };
    }

    mz_zip_archive zip{};
    mz_zip_zero_struct(&zip);
    if (!mz_zip_reader_init_mem(
            &zip,
            bytes->data(),
            bytes->size(),
            0U)) {
        return readFailure(
            NativeContainerErrorCode::malformed_container,
            "Native Document is not a valid ZIP-compatible container",
            path);
    }

    const auto fail = [&](
        NativeContainerErrorCode code,
        std::string message) {
        static_cast<void>(
            mz_zip_reader_end(&zip));
        return readFailure(
            code,
            std::move(message),
            path);
    };

    if (mz_zip_is_zip64(&zip)) {
        return fail(
            NativeContainerErrorCode::unsupported_zip_feature,
            "ZIP64 is not supported by native Document container v1");
    }

    const auto count =
        mz_zip_reader_get_num_files(&zip);
    if (count > maximum_entries) {
        return fail(
            NativeContainerErrorCode::too_large,
            "Native Document contains too many ZIP entries");
    }

    std::set<std::string> names;
    int manifest_index = -1;
    int authored_index = -1;
    std::uint64_t manifest_size = 0U;
    std::uint64_t authored_size = 0U;
    std::uint64_t total_uncompressed = 0U;

    for (mz_uint index = 0U;
         index < count;
         ++index) {
        mz_zip_archive_file_stat stat{};
        if (!mz_zip_reader_file_stat(
                &zip,
                index,
                &stat)) {
            return fail(
                NativeContainerErrorCode::malformed_container,
                "Unable to inspect native Document ZIP entry");
        }

        const auto name =
            entryName(zip, index);
        if (!name ||
            !safeEntryName(*name)) {
            return fail(
                NativeContainerErrorCode::unsafe_entry,
                "Native Document contains an unsafe ZIP entry name");
        }

        if (stat.m_is_directory) {
            return fail(
                NativeContainerErrorCode::unsafe_entry,
                "Native Document container v1 does not use directory entries");
        }

        if (!names.insert(*name).second) {
            return fail(
                NativeContainerErrorCode::duplicate_entry,
                "Native Document contains a duplicate ZIP entry");
        }

        if (stat.m_is_encrypted ||
            !stat.m_is_supported) {
            return fail(
                NativeContainerErrorCode::unsupported_zip_feature,
                "Native Document contains an encrypted or unsupported ZIP entry");
        }

        if (!allowedFileEntry(*name)) {
            return fail(
                NativeContainerErrorCode::unsafe_entry,
                "Native Document contains an entry outside accepted namespaces");
        }

        if (stat.m_uncomp_size >
            maximum_total_uncompressed_bytes -
                total_uncompressed) {
            return fail(
                NativeContainerErrorCode::too_large,
                "Native Document uncompressed content exceeds the supported size");
        }
        total_uncompressed +=
            stat.m_uncomp_size;

        if (*name == manifest_entry) {
            if (stat.m_uncomp_size >
                maximum_manifest_bytes) {
                return fail(
                    NativeContainerErrorCode::too_large,
                    "Native Document manifest exceeds the supported size");
            }
            manifest_index =
                static_cast<int>(index);
            manifest_size =
                stat.m_uncomp_size;
        } else if (*name == authored_entry) {
            if (stat.m_uncomp_size >
                maximum_authored_bytes) {
                return fail(
                    NativeContainerErrorCode::too_large,
                    "Native Document authored payload exceeds the supported size");
            }
            authored_index =
                static_cast<int>(index);
            authored_size =
                stat.m_uncomp_size;
        }
    }

    if (manifest_index < 0) {
        return fail(
            NativeContainerErrorCode::missing_manifest,
            "Native Document is missing manifest.json");
    }
    if (authored_index < 0) {
        return fail(
            NativeContainerErrorCode::missing_authored_payload,
            "Native Document is missing authored/document.json");
    }

    const auto manifest =
        extractEntry(
            zip,
            static_cast<mz_uint>(manifest_index),
            manifest_size);
    if (!manifest) {
        return fail(
            NativeContainerErrorCode::malformed_container,
            "Unable to extract native Document manifest");
    }

    const auto authored =
        extractEntry(
            zip,
            static_cast<mz_uint>(authored_index),
            authored_size);
    if (!authored) {
        return fail(
            NativeContainerErrorCode::malformed_container,
            "Unable to extract native Document authored payload");
    }

    static_cast<void>(
        mz_zip_reader_end(&zip));

    NativeContainerDiagnostic manifest_diagnostic;
    auto descriptor =
        parseManifest(
            *manifest,
            manifest_diagnostic,
            path);
    if (!descriptor) {
        return NativeContainerReadResult{
            std::nullopt,
            std::move(manifest_diagnostic),
        };
    }

    if (!nlohmann::json::accept(
            authored->begin(),
            authored->end())) {
        return readFailure(
            NativeContainerErrorCode::invalid_authored_json,
            "Native Document authored payload is not valid UTF-8 JSON",
            path);
    }

    return NativeContainerReadResult{
        NativeDocumentPackage{
            std::move(*descriptor),
            std::move(*authored),
        },
        NativeContainerDiagnostic{},
    };
}

NativeContainerBuildResult buildNativeDocumentContainer(
    const NativeDocumentDescriptor& descriptor,
    std::string_view authored_json) {
    if (descriptor.document_kind.empty() ||
        descriptor.document_id.empty() ||
        descriptor.domain_schema_version <= 0) {
        return buildFailure(
            NativeContainerErrorCode::invalid_manifest,
            "Native Document descriptor is invalid");
    }

    if (authored_json.size() >
        maximum_authored_bytes) {
        return buildFailure(
            NativeContainerErrorCode::too_large,
            "Native Document authored payload exceeds the supported size");
    }

    if (!nlohmann::json::accept(
            authored_json.begin(),
            authored_json.end())) {
        return buildFailure(
            NativeContainerErrorCode::invalid_authored_json,
            "Native Document authored payload must be valid UTF-8 JSON");
    }

    nlohmann::json manifest{
        {"format", native_document_format},
        {"container_version",
         native_document_container_version},
        {"document_kind",
         descriptor.document_kind},
        {"document_id",
         descriptor.document_id},
        {"domain_schema_version",
         descriptor.domain_schema_version},
    };

    std::string manifest_text =
        manifest.dump(2);
    manifest_text.push_back('\n');

    mz_zip_archive zip{};
    mz_zip_zero_struct(&zip);
    if (!mz_zip_writer_init_heap(
            &zip,
            0U,
            64U * 1024U)) {
        return buildFailure(
            NativeContainerErrorCode::io_failure,
            "Unable to initialize native Document ZIP writer");
    }

    const auto fail = [&](
        NativeContainerErrorCode code,
        std::string message) {
        static_cast<void>(
            mz_zip_writer_end(&zip));
        return buildFailure(
            code,
            std::move(message));
    };

    if (!mz_zip_writer_add_mem(
            &zip,
            manifest_entry.data(),
            manifest_text.data(),
            manifest_text.size(),
            MZ_DEFAULT_COMPRESSION)) {
        return fail(
            NativeContainerErrorCode::io_failure,
            "Unable to add native Document manifest to ZIP container");
    }

    if (!mz_zip_writer_add_mem(
            &zip,
            authored_entry.data(),
            authored_json.data(),
            authored_json.size(),
            MZ_DEFAULT_COMPRESSION)) {
        return fail(
            NativeContainerErrorCode::io_failure,
            "Unable to add authored payload to ZIP container");
    }

    void* buffer = nullptr;
    std::size_t size = 0U;
    if (!mz_zip_writer_finalize_heap_archive(
            &zip,
            &buffer,
            &size)) {
        return fail(
            NativeContainerErrorCode::io_failure,
            "Unable to finalize native Document ZIP container");
    }

    if (!mz_zip_writer_end(&zip)) {
        mz_free(buffer);
        return buildFailure(
            NativeContainerErrorCode::io_failure,
            "Unable to release native Document ZIP writer");
    }

    if (size > maximum_container_bytes) {
        mz_free(buffer);
        return buildFailure(
            NativeContainerErrorCode::too_large,
            "Native Document ZIP container exceeds the supported size");
    }

    std::string bytes;
    if (size != 0U) {
        bytes.assign(
            static_cast<const char*>(buffer),
            size);
    }
    mz_free(buffer);

    return NativeContainerBuildResult{
        std::move(bytes),
        NativeContainerDiagnostic{},
    };
}

} // namespace simplesolid2::persistence
