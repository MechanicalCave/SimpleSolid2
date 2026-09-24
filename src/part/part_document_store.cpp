#include <simplesolid2/part/part_document_store.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <set>
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

const char* supportRoleName(
    core::BuiltinReferenceRole role) noexcept {
    switch (role) {
    case core::BuiltinReferenceRole::xy_plane:
        return "xy_plane";
    case core::BuiltinReferenceRole::xz_plane:
        return "xz_plane";
    case core::BuiltinReferenceRole::yz_plane:
        return "yz_plane";
    default:
        return "";
    }
}

std::optional<core::BuiltinReferenceRole>
parseSupportRole(std::string_view value) noexcept {
    if (value == "xy_plane") {
        return core::BuiltinReferenceRole::xy_plane;
    }
    if (value == "xz_plane") {
        return core::BuiltinReferenceRole::xz_plane;
    }
    if (value == "yz_plane") {
        return core::BuiltinReferenceRole::yz_plane;
    }
    return std::nullopt;
}

nlohmann::json vectorJson(
    const std::array<double, 3>& value) {
    return nlohmann::json::array(
        {value[0], value[1], value[2]});
}

std::string serializeAuthored(
    const PartDocument& document) {
    const auto& properties =
        document.properties();

    nlohmann::json sketches =
        nlohmann::json::array();
    for (const auto& sketch : document.sketches()) {
        sketches.push_back(
            {
                {"id", std::string{sketch.id.value()}},
                {"support",
                 {
                     {"kind", "builtin_origin_plane"},
                     {"builtin_plane",
                      supportRoleName(
                          sketch.support.builtin_plane)},
                 }},
                {"placement",
                 {
                     {"origin",
                      vectorJson(sketch.placement.origin)},
                     {"u_axis",
                      vectorJson(sketch.placement.u_axis)},
                     {"v_axis",
                      vectorJson(sketch.placement.v_axis)},
                 }},
                {"visible", sketch.visible},
            });
    }

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
        {"sketches", std::move(sketches)},
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

std::optional<std::array<double, 3>>
parseVector3(const nlohmann::json& value) {
    if (!value.is_array() ||
        value.size() != 3U) {
        return std::nullopt;
    }

    std::array<double, 3> result{};
    for (std::size_t index = 0U;
         index < result.size();
         ++index) {
        const auto& item = value[index];
        if (!item.is_number()) {
            return std::nullopt;
        }

        const auto parsed = item.get<double>();
        if (!std::isfinite(parsed)) {
            return std::nullopt;
        }
        result[index] = parsed;
    }

    return result;
}

bool parseSketches(
    const nlohmann::json& sketches_json,
    std::vector<PartSketch>& sketches,
    std::string& error) {
    if (!sketches_json.is_array()) {
        error = "Native Part sketches payload must be an array";
        return false;
    }

    std::set<std::string> ids;

    for (const auto& item : sketches_json) {
        if (!item.is_object() ||
            item.size() != 4U ||
            !item.contains("id") ||
            !item.contains("support") ||
            !item.contains("placement") ||
            !item.contains("visible") ||
            !item["id"].is_string() ||
            !item["visible"].is_boolean()) {
            error = "Native Part contains malformed Sketch record";
            return false;
        }

        const auto serialized_id =
            item["id"].get<std::string>();
        auto id =
            sketch::SketchId::parse(
                serialized_id);
        if (!id) {
            error = "Native Part contains invalid SketchId";
            return false;
        }
        if (!ids.insert(serialized_id).second) {
            error = "Native Part contains duplicate SketchId";
            return false;
        }

        const auto& support_json =
            item["support"];
        if (!support_json.is_object() ||
            support_json.size() != 2U ||
            !support_json.contains("kind") ||
            !support_json.contains("builtin_plane") ||
            !support_json["kind"].is_string() ||
            !support_json["builtin_plane"].is_string() ||
            support_json["kind"].get<std::string>() !=
                "builtin_origin_plane") {
            error = "Native Part contains malformed Sketch support";
            return false;
        }

        const auto role =
            parseSupportRole(
                support_json[
                    "builtin_plane"].get<std::string>());
        if (!role) {
            error = "Native Part contains unsupported Sketch support";
            return false;
        }

        const auto support =
            partSketchSupportForBuiltinPlane(*role);
        if (!support) {
            error = "Native Part contains invalid Sketch support";
            return false;
        }

        const auto& placement_json =
            item["placement"];
        if (!placement_json.is_object() ||
            placement_json.size() != 3U ||
            !placement_json.contains("origin") ||
            !placement_json.contains("u_axis") ||
            !placement_json.contains("v_axis")) {
            error = "Native Part contains malformed Sketch placement";
            return false;
        }

        const auto origin =
            parseVector3(placement_json["origin"]);
        const auto u_axis =
            parseVector3(placement_json["u_axis"]);
        const auto v_axis =
            parseVector3(placement_json["v_axis"]);
        if (!origin || !u_axis || !v_axis) {
            error = "Native Part contains invalid Sketch placement vectors";
            return false;
        }

        SketchPlacement placement{
            *origin,
            *u_axis,
            *v_axis};
        if (!sketchPlacementMatchesSupport(
                placement,
                *support)) {
            error =
                "Native Part Sketch placement does not match its Origin-plane support";
            return false;
        }

        sketches.push_back(
            PartSketch{
                std::move(*id),
                *support,
                placement,
                item["visible"].get<bool>()});
    }

    return true;
}

std::optional<PartAuthoredState> parseAuthored(
    const std::string& text,
    int schema_version,
    std::string& error) {
    const auto authored =
        nlohmann::json::parse(
            text,
            nullptr,
            false);

    const bool legacy_v1 =
        schema_version == 1;
    const std::size_t expected_fields =
        legacy_v1 ? 2U : 3U;

    if (authored.is_discarded() ||
        !authored.is_object() ||
        authored.size() != expected_fields ||
        !authored.contains("properties") ||
        !authored.contains("presentation") ||
        (!legacy_v1 &&
         !authored.contains("sketches"))) {
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

    if (!legacy_v1 &&
        !parseSketches(
            authored["sketches"],
            state.sketches,
            error)) {
        return std::nullopt;
    }

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

    if (descriptor.domain_schema_version != 1 &&
        descriptor.domain_schema_version !=
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
            descriptor.domain_schema_version,
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
