#include <simplesolid2/part/part_document_store.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace simplesolid2::part {
namespace {

PartLoadResult loadFailure(
    PartStoreErrorCode code,
    std::string message,
    std::filesystem::path path,
    persistence::NativeContainerErrorCode container_code =
        persistence::NativeContainerErrorCode::none,
    persistence::AtomicWriteErrorCode atomic_code =
        persistence::AtomicWriteErrorCode::none) {
    return PartLoadResult{
        std::nullopt,
        PartStoreDiagnostic{
            code,
            atomic_code,
            container_code,
            std::move(message),
            std::move(path),
        },
    };
}

PartSaveResult saveFailure(
    PartStoreErrorCode code,
    std::string message,
    std::filesystem::path path,
    persistence::NativeContainerErrorCode container_code =
        persistence::NativeContainerErrorCode::none,
    persistence::AtomicWriteErrorCode atomic_code =
        persistence::AtomicWriteErrorCode::none) {
    return PartSaveResult{
        PartStoreDiagnostic{
            code,
            atomic_code,
            container_code,
            std::move(message),
            std::move(path),
        },
    };
}

std::string serializeAuthored(
    const PartDocument& document) {
    const auto& properties =
        document.properties();

    nlohmann::json authored{
        {"properties",
         {
             {"number", properties.number},
             {"title", properties.title},
             {"description", properties.description},
             {"engineering_revision",
              properties.engineering_revision},
         }},
        {"presentation",
         {
             {"builtin_reference_visibility_mask",
              static_cast<unsigned>(
                  document.presentation()
                      .builtin_references.mask())},
         }},
    };

    std::string text = authored.dump(2);
    text.push_back('\n');
    return text;
}

std::optional<std::uint8_t> parseVisibilityMask(
    const nlohmann::json& value) {
    std::uint64_t parsed{};

    if (value.is_number_unsigned()) {
        parsed = value.get<std::uint64_t>();
    } else if (value.is_number_integer()) {
        const auto signed_value =
            value.get<std::int64_t>();
        if (signed_value < 0) {
            return std::nullopt;
        }
        parsed =
            static_cast<std::uint64_t>(
                signed_value);
    } else {
        return std::nullopt;
    }

    if (parsed > 0xFFU) {
        return std::nullopt;
    }

    return static_cast<std::uint8_t>(parsed);
}

std::optional<PartAuthoredState> parseAuthored(
    const std::string& text,
    std::string& error) {
    const auto authored =
        nlohmann::json::parse(
            text,
            nullptr,
            false);

    if (authored.is_discarded() ||
        !authored.is_object() ||
        authored.size() != 2U ||
        !authored.contains("properties") ||
        !authored.contains("presentation")) {
        error =
            "Native Part authored payload has an invalid top-level schema";
        return std::nullopt;
    }

    const auto& properties_json =
        authored["properties"];
    if (!properties_json.is_object() ||
        properties_json.size() != 4U ||
        !properties_json.contains("number") ||
        !properties_json.contains("title") ||
        !properties_json.contains("description") ||
        !properties_json.contains("engineering_revision") ||
        !properties_json["number"].is_string() ||
        !properties_json["title"].is_string() ||
        !properties_json["description"].is_string() ||
        !properties_json["engineering_revision"].is_string()) {
        error =
            "Native Part properties payload is invalid";
        return std::nullopt;
    }

    const auto& presentation_json =
        authored["presentation"];
    if (!presentation_json.is_object() ||
        presentation_json.size() != 1U ||
        !presentation_json.contains(
            "builtin_reference_visibility_mask")) {
        error =
            "Native Part presentation payload is invalid";
        return std::nullopt;
    }

    const auto mask =
        parseVisibilityMask(
            presentation_json[
                "builtin_reference_visibility_mask"]);
    if (!mask) {
        error =
            "Native Part contains malformed built-in reference visibility";
        return std::nullopt;
    }

    const auto visibility =
        core::BuiltinReferenceVisibility::fromMask(
            *mask);
    if (!visibility) {
        error =
            "Native Part contains unsupported built-in reference visibility bits";
        return std::nullopt;
    }

    PartAuthoredState state;
    state.properties.number =
        properties_json["number"].get<std::string>();
    state.properties.title =
        properties_json["title"].get<std::string>();
    state.properties.description =
        properties_json["description"].get<std::string>();
    state.properties.engineering_revision =
        properties_json[
            "engineering_revision"].get<std::string>();
    state.presentation.builtin_references =
        *visibility;

    return state;
}

PartStoreErrorCode containerReadErrorToStore(
    persistence::NativeContainerErrorCode code) noexcept {
    using persistence::NativeContainerErrorCode;

    switch (code) {
    case NativeContainerErrorCode::none:
        return PartStoreErrorCode::none;
    case NativeContainerErrorCode::not_found:
        return PartStoreErrorCode::not_found;
    case NativeContainerErrorCode::io_failure:
        return PartStoreErrorCode::io_failure;
    case NativeContainerErrorCode::unsupported_container_version:
        return PartStoreErrorCode::unsupported_schema;
    case NativeContainerErrorCode::too_large:
    case NativeContainerErrorCode::malformed_container:
    case NativeContainerErrorCode::unsupported_zip_feature:
    case NativeContainerErrorCode::unsafe_entry:
    case NativeContainerErrorCode::duplicate_entry:
    case NativeContainerErrorCode::missing_manifest:
    case NativeContainerErrorCode::invalid_manifest:
    case NativeContainerErrorCode::missing_authored_payload:
    case NativeContainerErrorCode::invalid_authored_json:
        return PartStoreErrorCode::malformed_document;
    }

    return PartStoreErrorCode::container_failure;
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

    const auto authored =
        serializeAuthored(document);
    const auto built =
        persistence::buildNativeDocumentContainer(
            persistence::NativeDocumentDescriptor{
                "part",
                std::string{
                    document.documentId().value()},
                PartDocumentStore::
                    current_schema_version,
            },
            authored);

    if (!built.ok()) {
        return saveFailure(
            PartStoreErrorCode::container_failure,
            built.diagnostic.message,
            path,
            built.diagnostic.code);
    }

    const auto written =
        persistence::writeFileAtomically(
            path,
            *built.bytes,
            mode);
    if (!written.ok()) {
        return saveFailure(
            atomicErrorToStore(
                written.diagnostic.code),
            written.diagnostic.message,
            written.diagnostic.path,
            persistence::NativeContainerErrorCode::none,
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
        [](unsigned char ch) {
            return static_cast<char>(
                std::tolower(ch));
        });
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

    auto container =
        persistence::readNativeDocumentContainer(
            path);
    if (!container.ok()) {
        return loadFailure(
            containerReadErrorToStore(
                container.diagnostic.code),
            container.diagnostic.message,
            container.diagnostic.path,
            container.diagnostic.code);
    }

    const auto& descriptor =
        container.package->descriptor;

    if (descriptor.document_kind != "part") {
        return loadFailure(
            PartStoreErrorCode::wrong_document_kind,
            "Native .ss2part file does not declare DocumentKind part",
            path);
    }

    if (descriptor.domain_schema_version !=
        current_schema_version) {
        return loadFailure(
            PartStoreErrorCode::unsupported_schema,
            "Unsupported native Part domain schema version",
            path);
    }

    auto id =
        core::DocumentId::parse(
            descriptor.document_id);
    if (!id) {
        return loadFailure(
            PartStoreErrorCode::invalid_document_id,
            "Native Part contains an invalid DocumentId",
            path);
    }

    std::string parse_error;
    auto state =
        parseAuthored(
            container.package->authored_json,
            parse_error);
    if (!state) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            std::move(parse_error),
            path);
    }

    return PartLoadResult{
        std::optional<PartDocument>{
            PartDocument::restore(
                std::move(*id),
                std::move(*state))},
        PartStoreDiagnostic{},
    };
}

PartSaveResult PartDocumentStore::createNew(
    const std::filesystem::path& path,
    const PartDocument& document) const {
    return write(
        path,
        document,
        persistence::AtomicWriteMode::create_new);
}

PartSaveResult PartDocumentStore::save(
    const std::filesystem::path& path,
    const PartDocument& document) const {
    return write(
        path,
        document,
        persistence::AtomicWriteMode::replace);
}

} // namespace simplesolid2::part
