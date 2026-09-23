#include <simplesolid2/application/project_workspace_metadata.hpp>

#include <array>
#include <charconv>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <string_view>
#include <system_error>
#include <utility>

namespace simplesolid2::application {
namespace {

constexpr std::size_t maximum_metadata_bytes = 64U * 1024U;

ProjectMetadataResult failure(
    ProjectMetadataErrorCode code,
    std::string message,
    std::filesystem::path path = {}) {
    return ProjectMetadataResult{
        std::nullopt,
        ProjectMetadataDiagnostic{code, std::move(message), std::move(path)},
    };
}

ProjectMetadataResult success(ProjectWorkspaceMetadata metadata) {
    return ProjectMetadataResult{
        std::move(metadata),
        ProjectMetadataDiagnostic{},
    };
}

bool normalizedExistingDirectory(
    const std::filesystem::path& input,
    std::filesystem::path& output,
    ProjectMetadataDiagnostic& diagnostic) {
    if (input.empty()) {
        diagnostic = {ProjectMetadataErrorCode::invalid_workspace,
                      "Project workspace root must not be empty", input};
        return false;
    }

    std::error_code ec;
    auto absolute = std::filesystem::absolute(input, ec);
    if (ec) {
        diagnostic = {ProjectMetadataErrorCode::invalid_workspace,
                      "Unable to make Project workspace path absolute", input};
        return false;
    }

    auto canonical = std::filesystem::weakly_canonical(absolute, ec);
    if (ec) {
        diagnostic = {ProjectMetadataErrorCode::invalid_workspace,
                      "Unable to canonicalize Project workspace path", input};
        return false;
    }

    if (!std::filesystem::exists(canonical, ec) || ec) {
        diagnostic = {ProjectMetadataErrorCode::invalid_workspace,
                      "Project workspace does not exist", canonical};
        return false;
    }
    if (!std::filesystem::is_directory(canonical, ec) || ec) {
        diagnostic = {ProjectMetadataErrorCode::invalid_workspace,
                      "Project workspace is not a directory", canonical};
        return false;
    }

    output = canonical.lexically_normal();
    return true;
}

bool isHexDigit(char ch) noexcept {
    return (ch >= '0' && ch <= '9') ||
           (ch >= 'a' && ch <= 'f') ||
           (ch >= 'A' && ch <= 'F');
}

std::string makeUuidV4() {
    std::random_device rd;
    std::seed_seq seed{
        rd(), rd(), rd(), rd(),
        static_cast<unsigned>(std::chrono::high_resolution_clock::now().time_since_epoch().count()),
    };
    std::mt19937_64 engine{seed};
    std::uniform_int_distribution<unsigned> dist{0U, 255U};

    std::array<std::uint8_t, 16> bytes{};
    for (auto& byte : bytes) {
        byte = static_cast<std::uint8_t>(dist(engine));
    }
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0FU) | 0x40U);
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3FU) | 0x80U);

    std::ostringstream out;
    out << std::hex << std::nouppercase << std::setfill('0');
    for (std::size_t index = 0; index < bytes.size(); ++index) {
        out << std::setw(2) << static_cast<unsigned>(bytes[index]);
        if (index == 3 || index == 5 || index == 7 || index == 9) {
            out << '-';
        }
    }
    return out.str();
}

std::string utcTimestampNow() {
    const auto now = std::chrono::system_clock::now();
    const auto time = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &time);
#else
    gmtime_r(&time, &utc);
#endif

    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

bool isCanonicalUtcTimestamp(std::string_view value) noexcept {
    if (value.size() != 20U) return false;
    const auto digit = [&](std::size_t i) { return value[i] >= '0' && value[i] <= '9'; };
    for (const auto i : {0U, 1U, 2U, 3U, 5U, 6U, 8U, 9U, 11U, 12U, 14U, 15U, 17U, 18U}) {
        if (!digit(i)) return false;
    }
    if (value[4] != '-' || value[7] != '-' || value[10] != 'T' ||
        value[13] != ':' || value[16] != ':' || value[19] != 'Z') {
        return false;
    }

    const auto number = [&](std::size_t offset, std::size_t count) {
        unsigned result{};
        for (std::size_t i = 0; i < count; ++i) {
            result = result * 10U + static_cast<unsigned>(value[offset + i] - '0');
        }
        return result;
    };

    const auto year_value = static_cast<int>(number(0U, 4U));
    const auto month_value = number(5U, 2U);
    const auto day_value = number(8U, 2U);
    const auto hour_value = number(11U, 2U);
    const auto minute_value = number(14U, 2U);
    const auto second_value = number(17U, 2U);

    const std::chrono::year_month_day ymd{
        std::chrono::year{year_value},
        std::chrono::month{month_value},
        std::chrono::day{day_value},
    };
    return ymd.ok() && hour_value <= 23U && minute_value <= 59U && second_value <= 59U;
}

void appendUtf8(std::string& out, std::uint32_t codepoint) {
    if (codepoint <= 0x7FU) {
        out.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FFU) {
        out.push_back(static_cast<char>(0xC0U | (codepoint >> 6U)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    } else {
        out.push_back(static_cast<char>(0xE0U | (codepoint >> 12U)));
        out.push_back(static_cast<char>(0x80U | ((codepoint >> 6U) & 0x3FU)));
        out.push_back(static_cast<char>(0x80U | (codepoint & 0x3FU)));
    }
}

class MetadataJsonParser final {
public:
    explicit MetadataJsonParser(std::string_view text) : text_{text} {}

    bool parse(ProjectWorkspaceMetadata& metadata, std::string& error) {
        skipWhitespace();
        if (!consume('{')) return fail(error, "Expected JSON object");

        bool have_project_id = false;
        bool have_display_name = false;
        bool have_schema_version = false;
        bool have_created_at = false;

        skipWhitespace();
        if (consume('}')) return fail(error, "Project metadata object is empty");

        while (true) {
            std::string key;
            if (!parseString(key, error)) return false;
            skipWhitespace();
            if (!consume(':')) return fail(error, "Expected ':' after metadata key");
            skipWhitespace();

            if (key == "project_id") {
                if (have_project_id) return fail(error, "Duplicate metadata field: project_id");
                if (!parseString(metadata.project_id, error)) return false;
                have_project_id = true;
            } else if (key == "display_name") {
                if (have_display_name) return fail(error, "Duplicate metadata field: display_name");
                if (!parseString(metadata.display_name, error)) return false;
                have_display_name = true;
            } else if (key == "schema_version") {
                if (have_schema_version) return fail(error, "Duplicate metadata field: schema_version");
                if (!parseInteger(metadata.schema_version, error)) return false;
                have_schema_version = true;
            } else if (key == "created_at") {
                if (have_created_at) return fail(error, "Duplicate metadata field: created_at");
                if (!parseString(metadata.created_at, error)) return false;
                have_created_at = true;
            } else {
                return fail(error, "Unknown metadata field: " + key);
            }

            skipWhitespace();
            if (consume('}')) break;
            if (!consume(',')) return fail(error, "Expected ',' or '}' in metadata object");
            skipWhitespace();
        }

        skipWhitespace();
        if (position_ != text_.size()) return fail(error, "Trailing data after metadata object");

        if (!have_project_id) return fail(error, "Missing metadata field: project_id");
        if (!have_display_name) return fail(error, "Missing metadata field: display_name");
        if (!have_schema_version) return fail(error, "Missing metadata field: schema_version");
        if (!have_created_at) return fail(error, "Missing metadata field: created_at");
        return true;
    }

private:
    bool fail(std::string& error, std::string message) {
        error = std::move(message);
        return false;
    }

    void skipWhitespace() {
        while (position_ < text_.size()) {
            const char ch = text_[position_];
            if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') break;
            ++position_;
        }
    }

    bool consume(char expected) {
        if (position_ >= text_.size() || text_[position_] != expected) return false;
        ++position_;
        return true;
    }

    bool parseInteger(int& value, std::string& error) {
        const auto begin = position_;
        if (position_ < text_.size() && text_[position_] == '-') ++position_;
        const auto digits_begin = position_;
        while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
            ++position_;
        }
        if (digits_begin == position_) return fail(error, "Expected integer metadata value");

        const auto token = text_.substr(begin, position_ - begin);
        int parsed{};
        const auto* first = token.data();
        const auto* last = token.data() + token.size();
        const auto [ptr, ec] = std::from_chars(first, last, parsed);
        if (ec != std::errc{} || ptr != last) return fail(error, "Invalid integer metadata value");
        value = parsed;
        return true;
    }

    bool parseString(std::string& value, std::string& error) {
        if (!consume('"')) return fail(error, "Expected JSON string");
        value.clear();

        while (position_ < text_.size()) {
            const unsigned char ch = static_cast<unsigned char>(text_[position_++]);
            if (ch == '"') return true;
            if (ch < 0x20U) return fail(error, "Unescaped control character in JSON string");
            if (ch != '\\') {
                value.push_back(static_cast<char>(ch));
                continue;
            }

            if (position_ >= text_.size()) return fail(error, "Unterminated JSON escape");
            const char escaped = text_[position_++];
            switch (escaped) {
            case '"': value.push_back('"'); break;
            case '\\': value.push_back('\\'); break;
            case '/': value.push_back('/'); break;
            case 'b': value.push_back('\b'); break;
            case 'f': value.push_back('\f'); break;
            case 'n': value.push_back('\n'); break;
            case 'r': value.push_back('\r'); break;
            case 't': value.push_back('\t'); break;
            case 'u': {
                if (position_ + 4U > text_.size()) return fail(error, "Incomplete Unicode escape");
                std::uint32_t codepoint{};
                for (int i = 0; i < 4; ++i) {
                    const char hex = text_[position_++];
                    codepoint <<= 4U;
                    if (hex >= '0' && hex <= '9') codepoint |= static_cast<std::uint32_t>(hex - '0');
                    else if (hex >= 'a' && hex <= 'f') codepoint |= static_cast<std::uint32_t>(hex - 'a' + 10);
                    else if (hex >= 'A' && hex <= 'F') codepoint |= static_cast<std::uint32_t>(hex - 'A' + 10);
                    else return fail(error, "Invalid Unicode escape");
                }
                if (codepoint >= 0xD800U && codepoint <= 0xDFFFU) {
                    return fail(error, "Unicode surrogate escapes are not supported in project metadata");
                }
                appendUtf8(value, codepoint);
                break;
            }
            default:
                return fail(error, "Unsupported JSON escape");
            }
        }
        return fail(error, "Unterminated JSON string");
    }

    std::string_view text_;
    std::size_t position_{0};
};

std::string jsonEscape(std::string_view value) {
    std::ostringstream out;
    for (const unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20U) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned>(ch) << std::dec << std::setfill(' ');
            } else {
                out << static_cast<char>(ch);
            }
        }
    }
    return out.str();
}

std::string serialize(const ProjectWorkspaceMetadata& metadata) {
    std::ostringstream out;
    out << "{\n"
        << "  \"project_id\": \"" << jsonEscape(metadata.project_id) << "\",\n"
        << "  \"display_name\": \"" << jsonEscape(metadata.display_name) << "\",\n"
        << "  \"schema_version\": " << metadata.schema_version << ",\n"
        << "  \"created_at\": \"" << jsonEscape(metadata.created_at) << "\"\n"
        << "}\n";
    return out.str();
}

bool writeAtomically(
    const std::filesystem::path& path,
    std::string_view content,
    ProjectMetadataDiagnostic& diagnostic) {
    auto temporary = path;
    temporary += ".tmp-" + makeUuidV4();

    {
        std::ofstream out{temporary, std::ios::binary | std::ios::trunc};
        if (!out) {
            diagnostic = {ProjectMetadataErrorCode::io_failure,
                          "Unable to create temporary Project metadata file", temporary};
            return false;
        }
        out.write(content.data(), static_cast<std::streamsize>(content.size()));
        out.flush();
        if (!out) {
            std::error_code cleanup_ec;
            std::filesystem::remove(temporary, cleanup_ec);
            diagnostic = {ProjectMetadataErrorCode::io_failure,
                          "Unable to write Project metadata", temporary};
            return false;
        }
    }

    std::error_code ec;
    if (std::filesystem::exists(path, ec) && !ec) {
        std::filesystem::remove(temporary, ec);
        diagnostic = {ProjectMetadataErrorCode::already_initialized,
                      "Project workspace became initialized while metadata was being created", path};
        return false;
    }
    ec.clear();
    std::filesystem::rename(temporary, path, ec);
    if (ec) {
        std::error_code cleanup_ec;
        std::filesystem::remove(temporary, cleanup_ec);
        diagnostic = {ProjectMetadataErrorCode::io_failure,
                      "Unable to publish Project metadata", path};
        return false;
    }
    return true;
}

ProjectMetadataResult validateLoaded(
    ProjectWorkspaceMetadata metadata,
    const std::filesystem::path& path) {
    if (metadata.schema_version != ProjectWorkspaceMetadataService::current_schema_version) {
        return failure(ProjectMetadataErrorCode::unsupported_schema,
                       "Unsupported Project metadata schema version: " +
                           std::to_string(metadata.schema_version), path);
    }
    if (!ProjectWorkspaceMetadataService::isValidProjectId(metadata.project_id)) {
        return failure(ProjectMetadataErrorCode::invalid_project_id,
                       "Project metadata contains an invalid ProjectId", path);
    }
    if (metadata.display_name.empty()) {
        return failure(ProjectMetadataErrorCode::invalid_display_name,
                       "Project metadata contains an empty DisplayName", path);
    }
    if (!isCanonicalUtcTimestamp(metadata.created_at)) {
        return failure(ProjectMetadataErrorCode::invalid_created_at,
                       "Project metadata contains an invalid CreatedAt timestamp", path);
    }
    return success(std::move(metadata));
}

} // namespace

std::filesystem::path ProjectWorkspaceMetadataService::metadataDirectory(
    const std::filesystem::path& workspace_root) {
    return workspace_root / ".simplesolid";
}

std::filesystem::path ProjectWorkspaceMetadataService::metadataPath(
    const std::filesystem::path& workspace_root) {
    return metadataDirectory(workspace_root) / "project.json";
}

bool ProjectWorkspaceMetadataService::isValidProjectId(std::string_view project_id) noexcept {
    if (project_id.size() != 36U) return false;
    for (std::size_t index = 0; index < project_id.size(); ++index) {
        if (index == 8U || index == 13U || index == 18U || index == 23U) {
            if (project_id[index] != '-') return false;
        } else if (!isHexDigit(project_id[index])) {
            return false;
        }
    }

    if (project_id[14] != '4') return false;
    const char variant = project_id[19];
    return variant == '8' || variant == '9' || variant == 'a' || variant == 'A' ||
           variant == 'b' || variant == 'B';
}

bool ProjectWorkspaceMetadataService::isInitialized(
    const std::filesystem::path& workspace_root) const noexcept {
    if (workspace_root.empty()) return false;
    std::error_code ec;
    return std::filesystem::is_regular_file(metadataPath(workspace_root), ec) && !ec;
}

ProjectMetadataResult ProjectWorkspaceMetadataService::initialize(
    const std::filesystem::path& workspace_root,
    std::string display_name) const {
    std::filesystem::path root;
    ProjectMetadataDiagnostic diagnostic;
    if (!normalizedExistingDirectory(workspace_root, root, diagnostic)) {
        return ProjectMetadataResult{std::nullopt, std::move(diagnostic)};
    }

    if (display_name.empty()) display_name = root.filename().string();
    if (display_name.empty()) {
        return failure(ProjectMetadataErrorCode::invalid_display_name,
                       "Project DisplayName must not be empty", root);
    }

    const auto directory = metadataDirectory(root);
    const auto path = metadataPath(root);

    std::error_code ec;
    if (std::filesystem::exists(path, ec) && !ec) {
        return failure(ProjectMetadataErrorCode::already_initialized,
                       "Project workspace is already initialized", path);
    }
    if (ec) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to inspect Project metadata path", path);
    }

    const bool metadata_directory_preexisted = std::filesystem::exists(directory, ec) && !ec;
    if (ec) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to inspect Project metadata directory", directory);
    }
    if (metadata_directory_preexisted && !std::filesystem::is_directory(directory, ec)) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Project metadata path exists but is not a directory", directory);
    }
    if (ec) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to inspect Project metadata directory", directory);
    }

    if (!metadata_directory_preexisted) {
        if (!std::filesystem::create_directory(directory, ec)) {
            return failure(ProjectMetadataErrorCode::io_failure,
                           "Unable to create Project metadata directory", directory);
        }
        if (ec) {
            return failure(ProjectMetadataErrorCode::io_failure,
                           "Unable to create Project metadata directory", directory);
        }
    }

    ProjectWorkspaceMetadata metadata{
        makeUuidV4(),
        std::move(display_name),
        current_schema_version,
        utcTimestampNow(),
    };

    if (!writeAtomically(path, serialize(metadata), diagnostic)) {
        if (!metadata_directory_preexisted) {
            std::error_code rollback_ec;
            std::filesystem::remove(directory, rollback_ec);
        }
        return ProjectMetadataResult{std::nullopt, std::move(diagnostic)};
    }

    return success(std::move(metadata));
}

ProjectMetadataResult ProjectWorkspaceMetadataService::load(
    const std::filesystem::path& workspace_root) const {
    std::filesystem::path root;
    ProjectMetadataDiagnostic diagnostic;
    if (!normalizedExistingDirectory(workspace_root, root, diagnostic)) {
        return ProjectMetadataResult{std::nullopt, std::move(diagnostic)};
    }

    const auto path = metadataPath(root);
    std::error_code ec;
    const bool path_exists = std::filesystem::exists(path, ec);
    if (ec) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to inspect Project metadata file", path);
    }
    if (!path_exists) {
        return failure(ProjectMetadataErrorCode::not_initialized,
                       "Folder is not an initialized SimpleSolid Project workspace", path);
    }
    if (!std::filesystem::is_regular_file(path, ec) || ec) {
        return failure(ProjectMetadataErrorCode::malformed_metadata,
                       "Project metadata path is not a regular file", path);
    }

    const auto file_size = std::filesystem::file_size(path, ec);
    if (ec) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to inspect Project metadata file size", path);
    }
    if (file_size > maximum_metadata_bytes) {
        return failure(ProjectMetadataErrorCode::malformed_metadata,
                       "Project metadata file exceeds the supported size", path);
    }

    std::ifstream in{path, std::ios::binary};
    if (!in) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to open Project metadata file", path);
    }

    std::string text(static_cast<std::size_t>(file_size), '\0');
    if (!text.empty()) {
        in.read(text.data(), static_cast<std::streamsize>(text.size()));
    }
    if (!in && !in.eof()) {
        return failure(ProjectMetadataErrorCode::io_failure,
                       "Unable to read Project metadata file", path);
    }

    ProjectWorkspaceMetadata metadata;
    std::string parse_error;
    MetadataJsonParser parser{text};
    if (!parser.parse(metadata, parse_error)) {
        const auto code = parse_error.rfind("Missing metadata field:", 0) == 0
                              ? ProjectMetadataErrorCode::missing_field
                              : ProjectMetadataErrorCode::malformed_metadata;
        return failure(code, std::move(parse_error), path);
    }

    return validateLoaded(std::move(metadata), path);
}

} // namespace simplesolid2::application
