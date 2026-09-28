#include <simplesolid2/part/part_document_store.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <limits>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

const char* entityRoleName(
    sketch::EntityRole role) noexcept {
    switch (role) {
    case sketch::EntityRole::regular:
        return "regular";
    case sketch::EntityRole::construction:
        return "construction";
    }
    return "";
}

std::optional<sketch::EntityRole>
parseEntityRole(std::string_view value) noexcept {
    if (value == "regular") {
        return sketch::EntityRole::regular;
    }
    if (value == "construction") {
        return sketch::EntityRole::construction;
    }
    return std::nullopt;
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

nlohmann::json pointJson(
    const sketch::Point2& value) {
    return nlohmann::json::array(
        {value.u, value.v});
}

const char* profileAnchorKindName(
    ProfileBoundaryAnchorKind kind) noexcept {
    switch (kind) {
    case ProfileBoundaryAnchorKind::endpoint_start:
        return "endpoint_start";
    case ProfileBoundaryAnchorKind::endpoint_end:
        return "endpoint_end";
    case ProfileBoundaryAnchorKind::intersection:
        return "intersection";
    }
    return "";
}

nlohmann::json profileAnchorJson(
    const ProfileBoundaryAnchor& anchor) {
    if (anchor.kind ==
        ProfileBoundaryAnchorKind::intersection) {
        return nlohmann::json{
            {"kind", profileAnchorKindName(anchor.kind)},
            {"other_entity",
             anchor.other_entity.serialized()},
            {"branch", anchor.canonical_branch},
        };
    }
    return nlohmann::json{
        {"kind", profileAnchorKindName(anchor.kind)},
    };
}

nlohmann::json profileUseJson(
    const ProfileBoundaryUseIntent& use) {
    nlohmann::json result{
        {"source_entity",
         use.source_entity.serialized()},
        {"follows_source_direction",
         use.follows_source_direction},
        {"whole_closed_curve",
         use.whole_closed_curve},
    };
    if (!use.whole_closed_curve) {
        result["start_anchor"] =
            profileAnchorJson(*use.start_anchor);
        result["end_anchor"] =
            profileAnchorJson(*use.end_anchor);
    }
    return result;
}

nlohmann::json profileLoopJson(
    const ProfileLoopIntent& loop) {
    nlohmann::json result =
        nlohmann::json::array();
    for (const auto& use : loop.boundary) {
        result.push_back(profileUseJson(use));
    }
    return result;
}

nlohmann::json profileIntentJson(
    const ProfileRegionIntent& intent) {
    nlohmann::json holes =
        nlohmann::json::array();
    for (const auto& hole : intent.holes) {
        holes.push_back(profileLoopJson(hole));
    }
    return nlohmann::json{
        {"outer", profileLoopJson(intent.outer)},
        {"holes", std::move(holes)},
    };
}

std::string serializeAuthored(
    const PartDocument& document) {
    const auto& properties =
        document.properties();

    nlohmann::json sketches =
        nlohmann::json::array();
    for (const auto& hosted : document.sketches()) {
        const auto model_state =
            hosted.model.state();

        nlohmann::json entities =
            nlohmann::json::array();
        for (const auto& line : model_state.lines) {
            entities.push_back(
                {
                    {"kind", "line"},
                    {"id", line.id.serialized()},
                    {"role", entityRoleName(line.role)},
                    {"start", pointJson(line.start)},
                    {"end", pointJson(line.end)},
                });
        }
        for (const auto& circle : model_state.circles) {
            entities.push_back(
                {
                    {"kind", "circle"},
                    {"id", circle.id.serialized()},
                    {"role", entityRoleName(circle.role)},
                    {"center", pointJson(circle.center)},
                    {"radius", circle.radius},
                });
        }
        for (const auto& arc : model_state.arcs) {
            entities.push_back(
                {
                    {"kind", "arc"},
                    {"id", arc.id.serialized()},
                    {"role", entityRoleName(arc.role)},
                    {"center", pointJson(arc.center)},
                    {"radius", arc.radius},
                    {"start_angle", arc.start_angle},
                    {"sweep_angle", arc.sweep_angle},
                });
        }

        sketches.push_back(
            {
                {"id", std::string{hosted.id.value()}},
                {"support",
                 {
                     {"kind", "builtin_origin_plane"},
                     {"builtin_plane",
                      supportRoleName(
                          hosted.support.builtin_plane)},
                 }},
                {"placement",
                 {
                     {"origin",
                      vectorJson(hosted.placement.origin)},
                     {"u_axis",
                      vectorJson(hosted.placement.u_axis)},
                     {"v_axis",
                      vectorJson(hosted.placement.v_axis)},
                 }},
                {"visible", hosted.visible},
                {"model",
                 {
                     {"next_entity_id",
                      model_state.next_entity_id.serialized()},
                     {"entities", std::move(entities)},
                 }},
            });
    }

    nlohmann::json profiles =
        nlohmann::json::array();
    for (const auto& profile :
         document.profiles()) {
        profiles.push_back(
            {
                {"id",
                 profile.id.serialized()},
                {"source_sketch_id",
                 std::string{
                     profile.source_sketch_id.value()}},
                {"name", profile.name},
                {"visible", profile.visible},
                {"region_intent",
                 profileIntentJson(
                     profile.region_intent)},
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
        {"next_profile_id",
         document.profileIdCursor().serialized()},
        {"profiles", std::move(profiles)},
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

std::optional<sketch::Point2>
parsePoint2(const nlohmann::json& value) {
    if (!value.is_array() ||
        value.size() != 2U) {
        return std::nullopt;
    }

    sketch::Point2 result;
    for (std::size_t index = 0U;
         index < 2U;
         ++index) {
        const auto& item = value[index];
        if (!item.is_number()) {
            return std::nullopt;
        }

        const auto parsed = item.get<double>();
        if (!std::isfinite(parsed)) {
            return std::nullopt;
        }

        if (index == 0U) {
            result.u = parsed;
        } else {
            result.v = parsed;
        }
    }

    return result;
}

std::optional<sketch::SketchModel>
parseSketchModelV3(
    const nlohmann::json& model_json,
    std::string& error) {
    if (!model_json.is_object() ||
        model_json.size() != 2U ||
        !model_json.contains("next_entity_id") ||
        !model_json.contains("lines") ||
        !model_json["next_entity_id"].is_string() ||
        !model_json["lines"].is_array()) {
        error =
            "Native Part contains malformed schema-v3 Sketch model";
        return std::nullopt;
    }

    const auto cursor =
        sketch::EntityIdCursor::parse(
            model_json[
                "next_entity_id"].get<std::string>());
    if (!cursor) {
        error =
            "Native Part contains invalid Sketch next_entity_id";
        return std::nullopt;
    }

    sketch::SketchModelState state;
    state.next_entity_id = *cursor;

    for (const auto& item : model_json["lines"]) {
        if (!item.is_object() ||
            item.size() != 3U ||
            !item.contains("id") ||
            !item.contains("start") ||
            !item.contains("end") ||
            !item["id"].is_string()) {
            error =
                "Native Part contains malformed schema-v3 Sketch Line record";
            return std::nullopt;
        }

        const auto id =
            sketch::EntityId::parse(
                item["id"].get<std::string>());
        const auto start =
            parsePoint2(item["start"]);
        const auto end =
            parsePoint2(item["end"]);
        if (!id || !start || !end) {
            error =
                "Native Part contains invalid schema-v3 Sketch Line values";
            return std::nullopt;
        }

        state.lines.push_back(
            sketch::SketchLineState{
                *id,
                *start,
                *end});
    }

    auto model =
        sketch::SketchModel::restore(
            std::move(state));
    if (!model) {
        error =
            "Native Part contains inconsistent schema-v3 Sketch model";
        return std::nullopt;
    }

    return model;
}

std::optional<sketch::SketchModel>
parseSketchModelV4OrV5(
    const nlohmann::json& model_json,
    bool has_entity_role,
    std::string& error) {
    if (!model_json.is_object() ||
        model_json.size() != 2U ||
        !model_json.contains("next_entity_id") ||
        !model_json.contains("entities") ||
        !model_json["next_entity_id"].is_string() ||
        !model_json["entities"].is_array()) {
        error =
            "Native Part contains malformed schema-v4 Sketch model";
        return std::nullopt;
    }

    const auto cursor =
        sketch::EntityIdCursor::parse(
            model_json[
                "next_entity_id"].get<std::string>());
    if (!cursor) {
        error =
            "Native Part contains invalid Sketch next_entity_id";
        return std::nullopt;
    }

    sketch::SketchModelState state;
    state.next_entity_id = *cursor;

    for (const auto& item : model_json["entities"]) {
        if (!item.is_object() ||
            !item.contains("kind") ||
            !item.contains("id") ||
            !item["kind"].is_string() ||
            !item["id"].is_string()) {
            error =
                "Native Part contains malformed schema-v4 Sketch entity record";
            return std::nullopt;
        }

        const auto id =
            sketch::EntityId::parse(
                item["id"].get<std::string>());
        if (!id) {
            error =
                "Native Part contains invalid schema-v4 Sketch EntityId";
            return std::nullopt;
        }

        const auto kind =
            item["kind"].get<std::string>();

        sketch::EntityRole entity_role =
            sketch::EntityRole::regular;
        if (has_entity_role) {
            if (!item.contains("role") ||
                !item["role"].is_string()) {
                error =
                    "Native Part contains malformed schema-v5 Sketch entity role";
                return std::nullopt;
            }
            const auto parsed_role =
                parseEntityRole(
                    item["role"].get<std::string>());
            if (!parsed_role) {
                error =
                    "Native Part contains invalid schema-v5 Sketch entity role";
                return std::nullopt;
            }
            entity_role = *parsed_role;
        } else if (item.contains("role")) {
            error =
                "Native Part schema-v4 Sketch entity unexpectedly contains role";
            return std::nullopt;
        }
        const std::size_t role_field =
            has_entity_role ? 1U : 0U;

        if (kind == "line") {
            if (item.size() != 4U + role_field ||
                !item.contains("start") ||
                !item.contains("end")) {
                error =
                    "Native Part contains malformed schema-v4 Sketch Line record";
                return std::nullopt;
            }
            const auto start = parsePoint2(item["start"]);
            const auto end = parsePoint2(item["end"]);
            if (!start || !end) {
                error =
                    "Native Part contains invalid schema-v4 Sketch Line values";
                return std::nullopt;
            }
            state.lines.push_back(
                sketch::SketchLineState{
                    *id,
                    *start,
                    *end,
                    entity_role});
            continue;
        }

        if (kind == "circle") {
            if (item.size() != 4U + role_field ||
                !item.contains("center") ||
                !item.contains("radius") ||
                !item["radius"].is_number()) {
                error =
                    "Native Part contains malformed schema-v4 Sketch Circle record";
                return std::nullopt;
            }
            const auto center = parsePoint2(item["center"]);
            const double radius =
                item["radius"].get<double>();
            if (!center || !std::isfinite(radius)) {
                error =
                    "Native Part contains invalid schema-v4 Sketch Circle values";
                return std::nullopt;
            }
            state.circles.push_back(
                sketch::SketchCircleState{
                    *id,
                    *center,
                    radius,
                    entity_role});
            continue;
        }

        if (kind == "arc") {
            if (item.size() != 6U + role_field ||
                !item.contains("center") ||
                !item.contains("radius") ||
                !item.contains("start_angle") ||
                !item.contains("sweep_angle") ||
                !item["radius"].is_number() ||
                !item["start_angle"].is_number() ||
                !item["sweep_angle"].is_number()) {
                error =
                    "Native Part contains malformed schema-v4 Sketch Arc record";
                return std::nullopt;
            }
            const auto center = parsePoint2(item["center"]);
            const double radius =
                item["radius"].get<double>();
            const double start_angle =
                item["start_angle"].get<double>();
            const double sweep_angle =
                item["sweep_angle"].get<double>();
            if (!center ||
                !std::isfinite(radius) ||
                !std::isfinite(start_angle) ||
                !std::isfinite(sweep_angle)) {
                error =
                    "Native Part contains invalid schema-v4 Sketch Arc values";
                return std::nullopt;
            }
            state.arcs.push_back(
                sketch::SketchArcState{
                    *id,
                    *center,
                    radius,
                    start_angle,
                    sweep_angle,
                    entity_role});
            continue;
        }

        error =
            "Native Part contains unknown schema-v4 Sketch entity kind";
        return std::nullopt;
    }

    auto model =
        sketch::SketchModel::restore(
            std::move(state));
    if (!model) {
        error =
            "Native Part contains inconsistent schema-v4 Sketch model";
        return std::nullopt;
    }

    return model;
}

bool parseSketches(
    const nlohmann::json& sketches_json,
    int schema_version,
    std::vector<PartSketch>& sketches,
    std::string& error) {
    if (!sketches_json.is_array()) {
        error = "Native Part sketches payload must be an array";
        return false;
    }

    const bool schema_has_model =
        schema_version >= 3;
    const std::size_t expected_fields =
        schema_has_model ? 5U : 4U;

    std::set<std::string> ids;

    for (const auto& item : sketches_json) {
        if (!item.is_object() ||
            item.size() != expected_fields ||
            !item.contains("id") ||
            !item.contains("support") ||
            !item.contains("placement") ||
            !item.contains("visible") ||
            (schema_has_model && !item.contains("model")) ||
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

        sketch::SketchModel model;
        if (schema_has_model) {
            auto parsed_model =
                schema_version == 3
                    ? parseSketchModelV3(
                          item["model"],
                          error)
                    : parseSketchModelV4OrV5(
                          item["model"],
                          schema_version >= 5,
                          error);
            if (!parsed_model) {
                return false;
            }
            model = std::move(*parsed_model);
        }

        sketches.push_back(
            PartSketch{
                std::move(*id),
                *support,
                placement,
                item["visible"].get<bool>(),
                std::move(model)});
    }

    return true;
}

std::optional<std::uint32_t> parseUint32(
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
    if (parsed >
        std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }
    return static_cast<std::uint32_t>(parsed);
}

std::optional<ProfileBoundaryAnchor>
parseProfileAnchor(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_object() ||
        !value.contains("kind") ||
        !value["kind"].is_string()) {
        error =
            "Native Part contains malformed Profile boundary anchor";
        return std::nullopt;
    }

    const auto kind =
        value["kind"].get<std::string>();
    if (kind == "endpoint_start" ||
        kind == "endpoint_end") {
        if (value.size() != 1U) {
            error =
                "Native Part endpoint Profile anchor has unexpected fields";
            return std::nullopt;
        }
        return ProfileBoundaryAnchor{
            kind == "endpoint_start"
                ? ProfileBoundaryAnchorKind::
                      endpoint_start
                : ProfileBoundaryAnchorKind::
                      endpoint_end,
            {},
            0U};
    }

    if (kind != "intersection" ||
        value.size() != 3U ||
        !value.contains("other_entity") ||
        !value.contains("branch") ||
        !value["other_entity"].is_string()) {
        error =
            "Native Part contains malformed Profile intersection anchor";
        return std::nullopt;
    }

    const auto other =
        sketch::EntityId::parse(
            value["other_entity"]
                .get<std::string>());
    const auto branch =
        parseUint32(value["branch"]);
    if (!other || !branch) {
        error =
            "Native Part contains invalid Profile intersection anchor";
        return std::nullopt;
    }

    return ProfileBoundaryAnchor{
        ProfileBoundaryAnchorKind::intersection,
        *other,
        *branch};
}

std::optional<ProfileBoundaryUseIntent>
parseProfileUse(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_object() ||
        !value.contains("source_entity") ||
        !value.contains(
            "follows_source_direction") ||
        !value.contains(
            "whole_closed_curve") ||
        !value["source_entity"].is_string() ||
        !value["follows_source_direction"]
             .is_boolean() ||
        !value["whole_closed_curve"]
             .is_boolean()) {
        error =
            "Native Part contains malformed Profile boundary use";
        return std::nullopt;
    }

    const auto source =
        sketch::EntityId::parse(
            value["source_entity"]
                .get<std::string>());
    if (!source) {
        error =
            "Native Part contains invalid Profile source EntityId";
        return std::nullopt;
    }

    ProfileBoundaryUseIntent result;
    result.source_entity = *source;
    result.follows_source_direction =
        value["follows_source_direction"]
            .get<bool>();
    result.whole_closed_curve =
        value["whole_closed_curve"]
            .get<bool>();

    if (result.whole_closed_curve) {
        if (value.size() != 3U) {
            error =
                "Native Part whole-curve Profile use has unexpected anchors";
            return std::nullopt;
        }
        return result;
    }

    if (value.size() != 5U ||
        !value.contains("start_anchor") ||
        !value.contains("end_anchor")) {
        error =
            "Native Part open Profile boundary use is missing anchors";
        return std::nullopt;
    }

    result.start_anchor =
        parseProfileAnchor(
            value["start_anchor"],
            error);
    if (!result.start_anchor) {
        return std::nullopt;
    }
    result.end_anchor =
        parseProfileAnchor(
            value["end_anchor"],
            error);
    if (!result.end_anchor) {
        return std::nullopt;
    }
    return result;
}

std::optional<ProfileLoopIntent>
parseProfileLoop(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_array() ||
        value.empty()) {
        error =
            "Native Part contains malformed Profile loop";
        return std::nullopt;
    }

    ProfileLoopIntent result;
    result.boundary.reserve(value.size());
    for (const auto& item : value) {
        auto use =
            parseProfileUse(item, error);
        if (!use) {
            return std::nullopt;
        }
        result.boundary.push_back(
            std::move(*use));
    }
    return result;
}

std::optional<ProfileRegionIntent>
parseProfileIntent(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 2U ||
        !value.contains("outer") ||
        !value.contains("holes") ||
        !value["holes"].is_array()) {
        error =
            "Native Part contains malformed Profile RegionIntent";
        return std::nullopt;
    }

    auto outer =
        parseProfileLoop(
            value["outer"],
            error);
    if (!outer) {
        return std::nullopt;
    }

    ProfileRegionIntent result;
    result.outer = std::move(*outer);
    result.holes.reserve(
        value["holes"].size());
    for (const auto& hole_json :
         value["holes"]) {
        auto hole =
            parseProfileLoop(
                hole_json,
                error);
        if (!hole) {
            return std::nullopt;
        }
        result.holes.push_back(
            std::move(*hole));
    }

    if (!profileRegionIntentStructurallyValid(
            result)) {
        error =
            "Native Part Profile RegionIntent violates structural invariants";
        return std::nullopt;
    }
    return result;
}

bool parseProfiles(
    const nlohmann::json& value,
    ProfileIdCursor cursor,
    std::vector<PartProfile>& profiles,
    std::string& error) {
    if (!value.is_array()) {
        error =
            "Native Part profiles payload is invalid";
        return false;
    }

    profiles.clear();
    profiles.reserve(value.size());
    for (const auto& item : value) {
        if (!item.is_object() ||
            item.size() != 5U ||
            !item.contains("id") ||
            !item.contains("source_sketch_id") ||
            !item.contains("name") ||
            !item.contains("visible") ||
            !item.contains("region_intent") ||
            !item["id"].is_string() ||
            !item["source_sketch_id"].is_string() ||
            !item["name"].is_string() ||
            !item["visible"].is_boolean()) {
            error =
                "Native Part contains malformed Profile record";
            return false;
        }

        const auto id =
            ProfileId::parse(
                item["id"].get<std::string>());
        const auto sketch_id =
            sketch::SketchId::parse(
                item["source_sketch_id"]
                    .get<std::string>());
        auto intent =
            parseProfileIntent(
                item["region_intent"],
                error);
        if (!id || !sketch_id || !intent ||
            !cursor.containsAllocated(*id)) {
            if (error.empty()) {
                error =
                    "Native Part contains invalid Profile identity/reference";
            }
            return false;
        }

        profiles.push_back(
            PartProfile{
                *id,
                std::move(*sketch_id),
                item["name"].get<std::string>(),
                item["visible"].get<bool>(),
                std::move(*intent)});
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
    const bool has_profiles =
        schema_version >= 6;
    const std::size_t expected_fields =
        legacy_v1
            ? 2U
            : (has_profiles ? 5U : 3U);

    if (authored.is_discarded() ||
        !authored.is_object() ||
        authored.size() != expected_fields ||
        !authored.contains("properties") ||
        !authored.contains("presentation") ||
        (!legacy_v1 &&
         !authored.contains("sketches")) ||
        (has_profiles &&
         (!authored.contains("next_profile_id") ||
          !authored.contains("profiles")))) {
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
            schema_version,
            state.sketches,
            error)) {
        return std::nullopt;
    }

    if (has_profiles) {
        if (!authored["next_profile_id"].is_string()) {
            error =
                "Native Part next_profile_id is invalid";
            return std::nullopt;
        }
        const auto cursor =
            ProfileIdCursor::parse(
                authored["next_profile_id"]
                    .get<std::string>());
        if (!cursor) {
            error =
                "Native Part next_profile_id is invalid";
            return std::nullopt;
        }
        state.next_profile_id = *cursor;
        if (!parseProfiles(
                authored["profiles"],
                *cursor,
                state.profiles,
                error)) {
            return std::nullopt;
        }
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

std::optional<std::string> buildPartBytes(
    const std::filesystem::path& path,
    const PartDocument& document,
    PartSaveResult& failure_result) {
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
        failure_result =
            saveFailure(
                PartStoreErrorCode::container_failure,
                built.diagnostic.message,
                path,
                built.diagnostic.code);
        return std::nullopt;
    }
    return std::move(*built.bytes);
}

PartFileCheckpoint makeCheckpoint(
    const core::DocumentId& id,
    std::string_view bytes,
    const persistence::FileIdentity& identity) {
    return PartFileCheckpoint{
        id,
        static_cast<std::uint64_t>(
            bytes.size()),
        persistence::sha256(bytes),
        identity,
    };
}

PartStoreErrorCode snapshotErrorToStore(
    persistence::FileSnapshotErrorCode code) noexcept {
    using persistence::FileSnapshotErrorCode;
    if (code == FileSnapshotErrorCode::not_found) {
        return PartStoreErrorCode::
            save_conflict_target_missing;
    }
    if (code == FileSnapshotErrorCode::not_regular) {
        return PartStoreErrorCode::io_failure;
    }
    if (code == FileSnapshotErrorCode::too_large) {
        return PartStoreErrorCode::io_failure;
    }
    return PartStoreErrorCode::io_failure;
}

bool sameContent(
    const persistence::FileSnapshot& snapshot,
    const PartFileCheckpoint& checkpoint) noexcept {
    return snapshot.bytes.size() ==
               checkpoint.byte_length &&
           snapshot.digest ==
               checkpoint.digest;
}


} // namespace

bool isSaveConflict(
    PartStoreErrorCode code) noexcept {
    switch (code) {
    case PartStoreErrorCode::save_conflict_busy:
    case PartStoreErrorCode::save_conflict_target_missing:
    case PartStoreErrorCode::save_conflict_document_identity_changed:
    case PartStoreErrorCode::save_conflict_file_replaced:
    case PartStoreErrorCode::save_conflict_content_changed:
        return true;
    default:
        return false;
    }
}

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

    const auto snapshot =
        persistence::readFileSnapshot(
            path,
            persistence::
                maximum_native_document_container_bytes);
    if (!snapshot.ok()) {
        return loadFailure(
            snapshot.diagnostic.code ==
                    persistence::
                        FileSnapshotErrorCode::not_found
                ? PartStoreErrorCode::not_found
                : PartStoreErrorCode::io_failure,
            snapshot.diagnostic.message,
            snapshot.diagnostic.path);
    }

    auto container =
        persistence::parseNativeDocumentContainer(
            snapshot.snapshot->bytes,
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
        descriptor.domain_schema_version != 2 &&
        descriptor.domain_schema_version != 3 &&
        descriptor.domain_schema_version != 4 &&
        descriptor.domain_schema_version != 5 &&
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

    auto restored =
        PartDocument::restore(
            std::move(*id),
            std::move(*state));
    if (!restored.ok()) {
        return loadFailure(
            PartStoreErrorCode::malformed_document,
            "Native Part violates Part authored-state invariants",
            path);
    }

    const auto checkpoint =
        makeCheckpoint(
            restored.document->documentId(),
            snapshot.snapshot->bytes,
            snapshot.snapshot->identity);

    return PartLoadResult{
        std::move(restored.document),
        checkpoint,
        PartStoreDiagnostic{},
    };
}

PartSaveResult PartDocumentStore::createNew(
    const std::filesystem::path& path,
    const PartDocument& document) const {
    if (!hasNativeExtension(path)) {
        return saveFailure(
            PartStoreErrorCode::wrong_extension,
            "Native Part path must use the .ss2part extension",
            path);
    }

    PartSaveResult build_failure;
    const auto bytes =
        buildPartBytes(
            path,
            document,
            build_failure);
    if (!bytes) return build_failure;

    const auto written =
        persistence::writeFileAtomically(
            path,
            *bytes,
            persistence::AtomicWriteMode::
                create_new);
    if (!written.ok()) {
        return saveFailure(
            atomicErrorToStore(
                written.diagnostic.code),
            written.diagnostic.message,
            written.diagnostic.path,
            persistence::NativeContainerErrorCode::none,
            written.diagnostic.code);
    }

    return PartSaveResult{
        makeCheckpoint(
            document.documentId(),
            *bytes,
            *written.published_identity),
        PartStoreDiagnostic{},
    };
}

PartSaveResult PartDocumentStore::save(
    const std::filesystem::path& path,
    const PartDocument& document,
    const PartFileCheckpoint& expected_checkpoint) const {
    if (!hasNativeExtension(path)) {
        return saveFailure(
            PartStoreErrorCode::wrong_extension,
            "Native Part path must use the .ss2part extension",
            path);
    }

    const auto guard =
        persistence::acquireCooperativeSaveGuard(
            path);
    if (!guard.ok()) {
        return saveFailure(
            guard.diagnostic.code ==
                    persistence::
                        SaveGuardErrorCode::busy
                ? PartStoreErrorCode::
                      save_conflict_busy
                : PartStoreErrorCode::
                      io_failure,
            guard.diagnostic.message,
            guard.diagnostic.path);
    }

    const auto current =
        persistence::readFileSnapshot(
            path,
            persistence::
                maximum_native_document_container_bytes);
    if (!current.ok()) {
        return saveFailure(
            snapshotErrorToStore(
                current.diagnostic.code),
            current.diagnostic.message,
            current.diagnostic.path);
    }

    const bool content_matches =
        sameContent(
            *current.snapshot,
            expected_checkpoint);
    if (!content_matches) {
        const auto parsed =
            persistence::
                parseNativeDocumentContainer(
                    current.snapshot->bytes,
                    path);
        if (parsed.ok()) {
            const auto current_id =
                core::DocumentId::parse(
                    parsed.package->
                        descriptor.document_id);
            if (current_id &&
                *current_id !=
                    expected_checkpoint.document_id) {
                return saveFailure(
                    PartStoreErrorCode::
                        save_conflict_document_identity_changed,
                    "Native Part DocumentId changed since load/save checkpoint",
                    path);
            }
        }
    }

    if (current.snapshot->identity !=
        expected_checkpoint.file_identity) {
        return saveFailure(
            PartStoreErrorCode::
                save_conflict_file_replaced,
            "Native Part file object was replaced since load/save checkpoint",
            path);
    }

    if (!content_matches) {
        return saveFailure(
            PartStoreErrorCode::
                save_conflict_content_changed,
            "Native Part file content changed since load/save checkpoint",
            path);
    }

    if (document.documentId() !=
        expected_checkpoint.document_id) {
        return saveFailure(
            PartStoreErrorCode::
                save_conflict_document_identity_changed,
            "In-memory Part identity does not match Save checkpoint",
            path);
    }

    PartSaveResult build_failure;
    const auto bytes =
        buildPartBytes(
            path,
            document,
            build_failure);
    if (!bytes) return build_failure;

    const auto written =
        persistence::writeFileAtomically(
            path,
            *bytes,
            persistence::AtomicWriteMode::replace);
    if (!written.ok()) {
        return saveFailure(
            atomicErrorToStore(
                written.diagnostic.code),
            written.diagnostic.message,
            written.diagnostic.path,
            persistence::NativeContainerErrorCode::none,
            written.diagnostic.code);
    }

    return PartSaveResult{
        makeCheckpoint(
            document.documentId(),
            *bytes,
            *written.published_identity),
        PartStoreDiagnostic{},
    };
}

} // namespace simplesolid2::part
