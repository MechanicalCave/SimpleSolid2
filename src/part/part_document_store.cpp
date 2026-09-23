#include <simplesolid2/part/part_document_store.hpp>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <fstream>
#include <map>
#include <string_view>
#include <system_error>

namespace simplesolid2::part {
namespace {

constexpr std::uintmax_t maximum_part_bytes = 4U * 1024U * 1024U;

PartLoadResult loadFailure(
    PartStoreErrorCode code,
    std::string message,
    std::filesystem::path path,
    persistence::AtomicWriteErrorCode atomic_code =
        persistence::AtomicWriteErrorCode::none) {
    return PartLoadResult{
        std::nullopt,
        PartStoreDiagnostic{code, atomic_code, std::move(message), std::move(path)},
    };
}

PartSaveResult saveFailure(
    PartStoreErrorCode code,
    std::string message,
    std::filesystem::path path,
    persistence::AtomicWriteErrorCode atomic_code =
        persistence::AtomicWriteErrorCode::none) {
    return PartSaveResult{
        PartStoreDiagnostic{code, atomic_code, std::move(message), std::move(path)},
    };
}

std::string hexEncode(std::string_view input) {
    constexpr char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2U);
    for (const unsigned char ch : input) {
        out.push_back(digits[(ch >> 4U) & 0x0FU]);
        out.push_back(digits[ch & 0x0FU]);
    }
    return out;
}

int hexValue(char ch) noexcept {
    if (ch >= '0' && ch <= '9') return ch - '0';
    if (ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
    if (ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
    return -1;
}

bool hexDecode(std::string_view input, std::string& output) {
    if ((input.size() % 2U) != 0U) return false;
    output.clear();
    output.reserve(input.size() / 2U);
    for (std::size_t i = 0; i < input.size(); i += 2U) {
        const int hi = hexValue(input[i]);
        const int lo = hexValue(input[i + 1U]);
        if (hi < 0 || lo < 0) return false;
        output.push_back(static_cast<char>((hi << 4) | lo));
    }
    return true;
}

std::string serialize(const PartDocument& document) {
    const auto& properties = document.properties();
    std::string out;
    out += "SS2PART\n";
    out += "schema_version=1\n";
    out += "document_kind=part\n";
    out += "document_id=" + std::string{document.documentId().value()} + "\n";
    out += "number_hex=" + hexEncode(properties.number) + "\n";
    out += "title_hex=" + hexEncode(properties.title) + "\n";
    out += "description_hex=" + hexEncode(properties.description) + "\n";
    out += "engineering_revision_hex=" +
           hexEncode(properties.engineering_revision) + "\n";
    return out;
}

bool parseLines(
    const std::string& text,
    std::map<std::string, std::string>& fields,
    std::string& error) {
    std::size_t start = 0;
    std::size_t line_number = 0;
    while (start <= text.size()) {
        const auto end = text.find('\n', start);
        const auto length =
            end == std::string::npos ? text.size() - start : end - start;
        std::string line = text.substr(start, length);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        ++line_number;

        if (line_number == 1U) {
            if (line != "SS2PART") {
                error = "Missing SS2PART file signature";
                return false;
            }
        } else if (!line.empty()) {
            const auto equals = line.find('=');
            if (equals == std::string::npos || equals == 0U) {
                error = "Malformed native Part field";
                return false;
            }
            auto key = line.substr(0U, equals);
            auto value = line.substr(equals + 1U);
            if (!fields.emplace(std::move(key), std::move(value)).second) {
                error = "Duplicate native Part field";
                return false;
            }
        }

        if (end == std::string::npos) break;
        start = end + 1U;
    }
    return true;
}

bool getRequired(
    const std::map<std::string, std::string>& fields,
    std::string_view key,
    std::string& output) {
    const auto it = fields.find(std::string{key});
    if (it == fields.end()) return false;
    output = it->second;
    return true;
}

PartStoreErrorCode atomicErrorToStore(
    persistence::AtomicWriteErrorCode code) noexcept {
    using persistence::AtomicWriteErrorCode;
    if (code == AtomicWriteErrorCode::target_exists) {
        return PartStoreErrorCode::target_exists;
    }
    if (code == AtomicWriteErrorCode::invalid_parent) {
        return PartStoreErrorCode::invalid_path;
    }
    return PartStoreErrorCode::io_failure;
}

PartSaveResult write(
    const std::filesystem::path& path,
    const PartDocument& document,
    persistence::AtomicWriteMode mode) {
    if (!PartDocumentStore::hasNativeExtension(path)) {
        return saveFailure(
            PartStoreErrorCode::wrong_extension,
            "Native Part path must use the .ss2part extension",
            path);
    }

    const auto written =
        persistence::writeFileAtomically(path, serialize(document), mode);
    if (!written.ok()) {
        return saveFailure(
            atomicErrorToStore(written.diagnostic.code),
            written.diagnostic.message,
            written.diagnostic.path,
            written.diagnostic.code);
    }
    return PartSaveResult{};
}

} // namespace

bool PartDocumentStore::hasNativeExtension(
    const std::filesystem::path& path) noexcept {
    auto extension = path.extension().string();
    std::transform(
        extension.begin(),
        extension.end(),
        extension.begin(),
        [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension == ".ss2part";
}

PartLoadResult PartDocumentStore::load(
    const std::filesystem::path& path) const {
    if (!hasNativeExtension(path)) {
        return loadFailure(
            PartStoreErrorCode::wrong_extension,
            "Native Part path must use the .ss2part extension",
            path);
    }

    std::error_code ec;
    if (!std::filesystem::exists(path, ec) || ec) {
        return loadFailure(
            ec ? PartStoreErrorCode::io_failure : PartStoreErrorCode::not_found,
            ec ? "Unable to inspect native Part file" : "Native Part file does not exist",
            path);
    }
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            "Native Part path is not a regular file",
            path);
    }

    const auto size = std::filesystem::file_size(path, ec);
    if (ec) {
        return loadFailure(
            PartStoreErrorCode::io_failure,
            "Unable to inspect native Part file size",
            path);
    }
    if (size > maximum_part_bytes) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            "Native Part file exceeds the supported size",
            path);
    }

    std::ifstream in{path, std::ios::binary};
    if (!in) {
        return loadFailure(
            PartStoreErrorCode::io_failure,
            "Unable to open native Part file",
            path);
    }

    std::string text(static_cast<std::size_t>(size), '\0');
    if (!text.empty()) {
        in.read(text.data(), static_cast<std::streamsize>(text.size()));
    }
    if (!in && !in.eof()) {
        return loadFailure(
            PartStoreErrorCode::io_failure,
            "Unable to read native Part file",
            path);
    }

    std::map<std::string, std::string> fields;
    std::string parse_error;
    if (!parseLines(text, fields, parse_error)) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            std::move(parse_error),
            path);
    }

    constexpr std::size_t expected_fields = 7U;
    if (fields.size() != expected_fields) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            "Native Part contains unknown or missing fields",
            path);
    }

    std::string schema;
    std::string kind;
    std::string id_text;
    std::string number_hex;
    std::string title_hex;
    std::string description_hex;
    std::string engineering_revision_hex;
    if (!getRequired(fields, "schema_version", schema) ||
        !getRequired(fields, "document_kind", kind) ||
        !getRequired(fields, "document_id", id_text) ||
        !getRequired(fields, "number_hex", number_hex) ||
        !getRequired(fields, "title_hex", title_hex) ||
        !getRequired(fields, "description_hex", description_hex) ||
        !getRequired(fields, "engineering_revision_hex", engineering_revision_hex)) {
        return loadFailure(
            PartStoreErrorCode::missing_field,
            "Native Part is missing a required field",
            path);
    }

    int schema_version{};
    const auto parsed_schema =
        std::from_chars(schema.data(), schema.data() + schema.size(), schema_version);
    if (parsed_schema.ec != std::errc{} ||
        parsed_schema.ptr != schema.data() + schema.size()) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            "Native Part schema_version is not an integer",
            path);
    }
    if (schema_version != current_schema_version) {
        return loadFailure(
            PartStoreErrorCode::unsupported_schema,
            "Unsupported native Part schema version",
            path);
    }
    if (kind != "part") {
        return loadFailure(
            PartStoreErrorCode::wrong_document_kind,
            "Native file does not declare a Part document",
            path);
    }

    auto id = core::DocumentId::parse(id_text);
    if (!id) {
        return loadFailure(
            PartStoreErrorCode::invalid_document_id,
            "Native Part contains an invalid DocumentId",
            path);
    }

    core::DocumentProperties properties;
    if (!hexDecode(number_hex, properties.number) ||
        !hexDecode(title_hex, properties.title) ||
        !hexDecode(description_hex, properties.description) ||
        !hexDecode(engineering_revision_hex, properties.engineering_revision)) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            "Native Part contains malformed property encoding",
            path);
    }

    PartAuthoredState state{std::move(properties)};
    return PartLoadResult{
        std::optional<PartDocument>{
            PartDocument::restore(std::move(*id), std::move(state))},
        PartStoreDiagnostic{},
    };
}

PartSaveResult PartDocumentStore::createNew(
    const std::filesystem::path& path,
    const PartDocument& document) const {
    return write(path, document, persistence::AtomicWriteMode::create_new);
}

PartSaveResult PartDocumentStore::save(
    const std::filesystem::path& path,
    const PartDocument& document) const {
    return write(path, document, persistence::AtomicWriteMode::replace);
}

} // namespace simplesolid2::part
