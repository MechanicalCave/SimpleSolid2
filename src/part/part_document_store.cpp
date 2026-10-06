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

const char* surfaceRoleName(
    FeatureSurfaceRoleKind role) noexcept {
    switch (role) {
    case FeatureSurfaceRoleKind::profile_cap:
        return "profile_cap";
    case FeatureSurfaceRoleKind::extent_cap:
        return "extent_cap";
    case FeatureSurfaceRoleKind::negative_cap:
        return "negative_cap";
    case FeatureSurfaceRoleKind::positive_cap:
        return "positive_cap";
    case FeatureSurfaceRoleKind::side:
        return "side";
    case FeatureSurfaceRoleKind::revolve_start_cap:
        return "revolve_start_cap";
    case FeatureSurfaceRoleKind::revolve_end_cap:
        return "revolve_end_cap";
    case FeatureSurfaceRoleKind::revolve_side:
        return "revolve_side";
    }
    return "";
}

std::optional<FeatureSurfaceRoleKind>
parseSurfaceRole(std::string_view value) noexcept {
    if (value == "profile_cap") {
        return FeatureSurfaceRoleKind::profile_cap;
    }
    if (value == "extent_cap") {
        return FeatureSurfaceRoleKind::extent_cap;
    }
    if (value == "negative_cap") {
        return FeatureSurfaceRoleKind::negative_cap;
    }
    if (value == "positive_cap") {
        return FeatureSurfaceRoleKind::positive_cap;
    }
    if (value == "side") {
        return FeatureSurfaceRoleKind::side;
    }
    if (value == "revolve_start_cap") {
        return FeatureSurfaceRoleKind::revolve_start_cap;
    }
    if (value == "revolve_end_cap") {
        return FeatureSurfaceRoleKind::revolve_end_cap;
    }
    if (value == "revolve_side") {
        return FeatureSurfaceRoleKind::revolve_side;
    }
    return std::nullopt;
}


const char* curveRoleName(
    FeatureCurveRoleKind role) noexcept {
    switch (role) {
    case FeatureCurveRoleKind::cap_side:
        return "cap_side";
    case FeatureCurveRoleKind::side_side:
        return "side_side";
    case FeatureCurveRoleKind::boolean_intersection:
        return "boolean_intersection";
    case FeatureCurveRoleKind::edge_feature_boundary:
        return "edge_feature_boundary";
    }
    return "";
}

std::optional<FeatureCurveRoleKind>
parseCurveRole(std::string_view value) noexcept {
    if (value == "cap_side") {
        return FeatureCurveRoleKind::cap_side;
    }
    if (value == "side_side") {
        return FeatureCurveRoleKind::side_side;
    }
    if (value == "boolean_intersection") {
        return FeatureCurveRoleKind::boolean_intersection;
    }
    if (value == "edge_feature_boundary") {
        return FeatureCurveRoleKind::edge_feature_boundary;
    }
    return std::nullopt;
}

nlohmann::json featureSurfaceAddressJson(
    const FeatureSurfaceAddress& surface) {
    if (!surface.valid()) {
        return nlohmann::json{};
    }
    nlohmann::json result{
        {"producer_feature_id",
         surface.producer_feature_id.serialized()},
        {"role", surfaceRoleName(surface.role)},
    };
    if (surface.role ==
            FeatureSurfaceRoleKind::side ||
        surface.role ==
            FeatureSurfaceRoleKind::revolve_side) {
        result["source_entity"] =
            surface.source_entity->serialized();
        result["loop_index"] =
            surface.loop_index;
        result["use_index"] =
            surface.use_index;
        result["hole"] =
            surface.hole;
    }
    return result;
}

std::optional<FeatureSurfaceAddress>
parseFeatureSurfaceAddressV14(
    const nlohmann::json& value,
    FeatureIdCursor feature_cursor,
    std::string& error) {
    if (!value.is_object() ||
        !value.contains("producer_feature_id") ||
        !value.contains("role") ||
        !value["producer_feature_id"].is_string() ||
        !value["role"].is_string()) {
        error =
            "Native Part contains malformed schema-v14 FeatureSurfaceAddress";
        return std::nullopt;
    }

    const auto producer =
        FeatureId::parse(
            value["producer_feature_id"]
                .get<std::string>());
    const auto role =
        parseSurfaceRole(
            value["role"].get<std::string>());
    if (!producer || !role ||
        !feature_cursor.containsAllocated(
            *producer)) {
        error =
            "Native Part contains invalid schema-v14 FeatureSurfaceAddress";
        return std::nullopt;
    }

    FeatureSurfaceAddress result;
    result.producer_feature_id = *producer;
    result.role = *role;

    const bool sided =
        *role == FeatureSurfaceRoleKind::side ||
        *role == FeatureSurfaceRoleKind::revolve_side;
    if (sided) {
        if (value.size() != 6U ||
            !value.contains("source_entity") ||
            !value.contains("loop_index") ||
            !value.contains("use_index") ||
            !value.contains("hole") ||
            !value["source_entity"].is_string() ||
            !value["loop_index"].is_number_unsigned() ||
            !value["use_index"].is_number_unsigned() ||
            !value["hole"].is_boolean()) {
            error =
                "Native Part contains malformed schema-v14 sided FeatureSurfaceAddress";
            return std::nullopt;
        }
        const auto source =
            sketch::EntityId::parse(
                value["source_entity"]
                    .get<std::string>());
        if (!source) {
            error =
                "Native Part contains invalid schema-v14 FeatureSurfaceAddress source EntityId";
            return std::nullopt;
        }
        result.source_entity = *source;
        result.loop_index =
            value["loop_index"].get<std::uint32_t>();
        result.use_index =
            value["use_index"].get<std::uint32_t>();
        result.hole =
            value["hole"].get<bool>();
    } else if (value.size() != 2U) {
        error =
            "Native Part contains unexpected schema-v14 FeatureSurfaceAddress fields";
        return std::nullopt;
    }

    if (!result.valid()) {
        error =
            "Native Part contains structurally invalid schema-v14 FeatureSurfaceAddress";
        return std::nullopt;
    }
    return result;
}

nlohmann::json featureCurveAddressJson(
    const FeatureCurveAddress& curve) {
    if (!curve.valid()) {
        return nlohmann::json{};
    }
    nlohmann::json surfaces =
        nlohmann::json::array();
    for (const auto& surface :
         curve.adjacent_surfaces) {
        auto item =
            featureSurfaceAddressJson(surface);
        if (item.empty()) {
            return nlohmann::json{};
        }
        surfaces.push_back(
            std::move(item));
    }
    return nlohmann::json{
        {"producer_feature_id",
         curve.producer_feature_id.serialized()},
        {"role", curveRoleName(curve.role)},
        {"adjacent_surfaces",
         std::move(surfaces)},
    };
}

std::optional<FeatureCurveAddress>
parseFeatureCurveAddressV14(
    const nlohmann::json& value,
    FeatureIdCursor feature_cursor,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 3U ||
        !value.contains("producer_feature_id") ||
        !value.contains("role") ||
        !value.contains("adjacent_surfaces") ||
        !value["producer_feature_id"].is_string() ||
        !value["role"].is_string() ||
        !value["adjacent_surfaces"].is_array()) {
        error =
            "Native Part contains malformed schema-v14 FeatureCurveAddress";
        return std::nullopt;
    }

    const auto producer =
        FeatureId::parse(
            value["producer_feature_id"]
                .get<std::string>());
    const auto role =
        parseCurveRole(
            value["role"].get<std::string>());
    if (!producer || !role ||
        !feature_cursor.containsAllocated(
            *producer)) {
        error =
            "Native Part contains invalid schema-v14 FeatureCurveAddress";
        return std::nullopt;
    }

    FeatureCurveAddress result;
    result.producer_feature_id = *producer;
    result.role = *role;
    for (const auto& item :
         value["adjacent_surfaces"]) {
        auto surface =
            parseFeatureSurfaceAddressV14(
                item,
                feature_cursor,
                error);
        if (!surface) {
            return std::nullopt;
        }
        result.adjacent_surfaces.push_back(
            std::move(*surface));
    }
    if (!result.valid()) {
        error =
            "Native Part contains structurally invalid schema-v14 FeatureCurveAddress";
        return std::nullopt;
    }
    return result;
}

nlohmann::json featurePointAddressJson(
    const FeaturePointAddress& point) {
    if (!point.valid()) {
        return nlohmann::json{};
    }
    nlohmann::json surfaces =
        nlohmann::json::array();
    for (const auto& surface :
         point.adjacent_surfaces) {
        auto item =
            featureSurfaceAddressJson(surface);
        if (item.empty()) {
            return nlohmann::json{};
        }
        surfaces.push_back(
            std::move(item));
    }
    return nlohmann::json{
        {"producer_feature_id",
         point.producer_feature_id.serialized()},
        {"adjacent_surfaces",
         std::move(surfaces)},
    };
}

std::optional<FeaturePointAddress>
parseFeaturePointAddressV14(
    const nlohmann::json& value,
    FeatureIdCursor feature_cursor,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 2U ||
        !value.contains("producer_feature_id") ||
        !value.contains("adjacent_surfaces") ||
        !value["producer_feature_id"].is_string() ||
        !value["adjacent_surfaces"].is_array()) {
        error =
            "Native Part contains malformed schema-v14 FeaturePointAddress";
        return std::nullopt;
    }

    const auto producer =
        FeatureId::parse(
            value["producer_feature_id"]
                .get<std::string>());
    if (!producer ||
        !feature_cursor.containsAllocated(
            *producer)) {
        error =
            "Native Part contains invalid schema-v14 FeaturePointAddress producer";
        return std::nullopt;
    }

    FeaturePointAddress result;
    result.producer_feature_id = *producer;
    for (const auto& item :
         value["adjacent_surfaces"]) {
        auto surface =
            parseFeatureSurfaceAddressV14(
                item,
                feature_cursor,
                error);
        if (!surface) {
            return std::nullopt;
        }
        result.adjacent_surfaces.push_back(
            std::move(*surface));
    }
    if (!result.valid()) {
        error =
            "Native Part contains structurally invalid schema-v14 FeaturePointAddress";
        return std::nullopt;
    }
    return result;
}

nlohmann::json materialEdgeReferenceJson(
    const MaterialEdgeReference& edge) {
    if (!edge.valid() ||
        !edge.stage.feature_id) {
        return nlohmann::json{};
    }

    auto curve =
        featureCurveAddressJson(
            edge.curve);
    if (curve.empty()) {
        return nlohmann::json{};
    }

    nlohmann::json branch;
    if (std::holds_alternative<
            SingularAtAuthoredStage>(
                edge.branch)) {
        branch = {
            {"kind",
             "singular_at_authored_stage"},
        };
    } else {
        const auto* endpoints =
            std::get_if<
                BetweenSemanticPoints>(
                &edge.branch);
        if (endpoints == nullptr ||
            !endpoints->valid()) {
            return nlohmann::json{};
        }
        auto first =
            featurePointAddressJson(
                endpoints->first);
        auto second =
            featurePointAddressJson(
                endpoints->second);
        if (first.empty() ||
            second.empty()) {
            return nlohmann::json{};
        }
        branch = {
            {"kind",
             "between_semantic_points"},
            {"first", std::move(first)},
            {"second", std::move(second)},
        };
    }

    return nlohmann::json{
        {"stage",
         {
             {"kind", "after_feature"},
             {"feature_id",
              edge.stage.feature_id
                  ->serialized()},
         }},
        {"curve", std::move(curve)},
        {"branch", std::move(branch)},
    };
}

std::optional<MaterialEdgeReference>
parseMaterialEdgeReferenceV14(
    const nlohmann::json& value,
    FeatureIdCursor feature_cursor,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 3U ||
        !value.contains("stage") ||
        !value.contains("curve") ||
        !value.contains("branch")) {
        error =
            "Native Part contains malformed schema-v14 MaterialEdgeReference";
        return std::nullopt;
    }

    const auto& stage_json = value["stage"];
    if (!stage_json.is_object() ||
        stage_json.size() != 2U ||
        !stage_json.contains("kind") ||
        !stage_json.contains("feature_id") ||
        !stage_json["kind"].is_string() ||
        !stage_json["feature_id"].is_string() ||
        stage_json["kind"].get<std::string>() !=
            "after_feature") {
        error =
            "Native Part contains malformed schema-v14 MaterialEdgeReference stage";
        return std::nullopt;
    }

    const auto stage_id =
        FeatureId::parse(
            stage_json["feature_id"]
                .get<std::string>());
    if (!stage_id ||
        !feature_cursor.containsAllocated(
            *stage_id)) {
        error =
            "Native Part contains invalid schema-v14 MaterialEdgeReference stage FeatureId";
        return std::nullopt;
    }

    auto curve =
        parseFeatureCurveAddressV14(
            value["curve"],
            feature_cursor,
            error);
    if (!curve) {
        return std::nullopt;
    }

    const auto& branch_json =
        value["branch"];
    if (!branch_json.is_object() ||
        !branch_json.contains("kind") ||
        !branch_json["kind"].is_string()) {
        error =
            "Native Part contains malformed schema-v14 EdgeBranchDiscriminator";
        return std::nullopt;
    }

    EdgeBranchDiscriminator branch;
    const auto kind =
        branch_json["kind"].get<std::string>();
    if (kind ==
        "singular_at_authored_stage") {
        if (branch_json.size() != 1U) {
            error =
                "Native Part contains unexpected schema-v14 singular Edge branch fields";
            return std::nullopt;
        }
        branch = SingularAtAuthoredStage{};
    } else if (
        kind == "between_semantic_points") {
        if (branch_json.size() != 3U ||
            !branch_json.contains("first") ||
            !branch_json.contains("second")) {
            error =
                "Native Part contains malformed schema-v14 semantic-point Edge branch";
            return std::nullopt;
        }
        auto first =
            parseFeaturePointAddressV14(
                branch_json["first"],
                feature_cursor,
                error);
        auto second =
            parseFeaturePointAddressV14(
                branch_json["second"],
                feature_cursor,
                error);
        if (!first || !second) {
            return std::nullopt;
        }
        BetweenSemanticPoints endpoints{
            std::move(*first),
            std::move(*second)};
        if (!endpoints.valid()) {
            error =
                "Native Part contains non-canonical schema-v14 semantic-point Edge branch";
            return std::nullopt;
        }
        branch = std::move(endpoints);
    } else {
        error =
            "Native Part contains unsupported schema-v14 EdgeBranchDiscriminator";
        return std::nullopt;
    }

    MaterialEdgeReference result{
        BodyStageRef{
            BodyStageKind::after_feature,
            *stage_id},
        std::move(*curve),
        std::move(branch)};
    if (!result.valid()) {
        error =
            "Native Part contains structurally invalid schema-v14 MaterialEdgeReference";
        return std::nullopt;
    }
    return result;
}

nlohmann::json sketchSupportJson(
    const PartSketchSupport& support) {
    if (const auto role =
            builtinOriginPlaneForSketchSupport(
                support)) {
        return nlohmann::json{
            {"kind", "builtin_origin_plane"},
            {"builtin_plane",
             supportRoleName(*role)},
        };
    }

    if (const auto datum_id =
            datumPlaneIdForSketchSupport(
                support)) {
        return nlohmann::json{
            {"kind", "datum_plane"},
            {"datum_id", datum_id->serialized()},
        };
    }

    const auto* reference =
        bodyPlanarSurfaceReference(support);
    if (reference == nullptr ||
        !reference->valid() ||
        !reference->stage.feature_id) {
        return nlohmann::json{};
    }

    nlohmann::json surface{
        {"producer_feature_id",
         reference->surface
             .producer_feature_id.serialized()},
        {"role",
         surfaceRoleName(
             reference->surface.role)},
    };
    if (reference->surface.role ==
        FeatureSurfaceRoleKind::side) {
        surface["source_entity"] =
            reference->surface
                .source_entity->serialized();
        surface["loop_index"] =
            reference->surface.loop_index;
        surface["use_index"] =
            reference->surface.use_index;
        surface["hole"] =
            reference->surface.hole;
    }

    return nlohmann::json{
        {"kind", "body_planar_surface"},
        {"stage",
         {
             {"kind", "after_feature"},
             {"feature_id",
              reference->stage
                  .feature_id->serialized()},
         }},
        {"surface", std::move(surface)},
    };
}

nlohmann::json planeReferenceJson(
    const PlaneReference& reference) {
    if (const auto role =
            builtinOriginPlaneForPlaneReference(
                reference)) {
        const auto support =
            partSketchSupportForBuiltinPlane(*role);
        return support
            ? sketchSupportJson(*support)
            : nlohmann::json{};
    }

    if (const auto* surface =
            bodyPlanarSurfaceForPlaneReference(
                reference)) {
        const auto support =
            partSketchSupportForBodyPlanarSurface(
                *surface);
        return support
            ? sketchSupportJson(*support)
            : nlohmann::json{};
    }

    if (const auto datum_id =
            datumPlaneIdForPlaneReference(
                reference)) {
        return nlohmann::json{
            {"kind", "datum_plane"},
            {"datum_id", datum_id->serialized()},
        };
    }

    return nlohmann::json{};
}

const char* profileVisibilityPolicyName(
    ProfileVisibilityPolicy policy) noexcept {
    switch (policy) {
    case ProfileVisibilityPolicy::automatic:
        return "automatic";
    case ProfileVisibilityPolicy::force_shown:
        return "force_shown";
    case ProfileVisibilityPolicy::force_hidden:
        return "force_hidden";
    }
    return "";
}

std::optional<ProfileVisibilityPolicy>
parseProfileVisibilityPolicy(
    std::string_view value) noexcept {
    if (value == "automatic") return ProfileVisibilityPolicy::automatic;
    if (value == "force_shown") return ProfileVisibilityPolicy::force_shown;
    if (value == "force_hidden") return ProfileVisibilityPolicy::force_hidden;
    return std::nullopt;
}

const char* extrudeOperationName(
    ExtrudeOperation operation) noexcept {
    switch (operation) {
    case ExtrudeOperation::add: return "add";
    case ExtrudeOperation::cut: return "cut";
    }
    return "";
}

std::optional<ExtrudeOperation>
parseExtrudeOperation(
    std::string_view value) noexcept {
    if (value == "add") return ExtrudeOperation::add;
    if (value == "cut") return ExtrudeOperation::cut;
    return std::nullopt;
}

const char* revolveOperationName(
    RevolveOperation operation) noexcept {
    switch (operation) {
    case RevolveOperation::add: return "add";
    case RevolveOperation::cut: return "cut";
    }
    return "";
}

std::optional<RevolveOperation>
parseRevolveOperation(
    std::string_view value) noexcept {
    if (value == "add") return RevolveOperation::add;
    if (value == "cut") return RevolveOperation::cut;
    return std::nullopt;
}

const char* originAxisRoleName(
    core::BuiltinReferenceRole role) noexcept {
    switch (role) {
    case core::BuiltinReferenceRole::x_axis:
        return "x_axis";
    case core::BuiltinReferenceRole::y_axis:
        return "y_axis";
    case core::BuiltinReferenceRole::z_axis:
        return "z_axis";
    default:
        return "";
    }
}

std::optional<core::BuiltinReferenceRole>
parseOriginAxisRole(
    std::string_view value) noexcept {
    if (value == "x_axis") {
        return core::BuiltinReferenceRole::x_axis;
    }
    if (value == "y_axis") {
        return core::BuiltinReferenceRole::y_axis;
    }
    if (value == "z_axis") {
        return core::BuiltinReferenceRole::z_axis;
    }
    return std::nullopt;
}

nlohmann::json axisReferenceJson(
    const AxisReference& reference) {
    if (const auto role =
            builtinOriginAxisForAxisReference(
                reference)) {
        const auto* name =
            originAxisRoleName(*role);
        if (name[0] == '\0') {
            return nlohmann::json{};
        }
        return nlohmann::json{
            {"kind", "origin_axis"},
            {"axis", name},
        };
    }

    if (const auto id =
            authoredAxisIdForAxisReference(
                reference)) {
        return nlohmann::json{
            {"kind", "authored_axis"},
            {"axis_id", id->serialized()},
        };
    }

    return nlohmann::json{};
}

std::optional<AxisReference>
parseAxisReferenceV13(
    const nlohmann::json& value,
    AxisIdCursor cursor,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 2U ||
        !value.contains("kind") ||
        !value["kind"].is_string()) {
        error =
            "Native Part contains malformed schema-v13 AxisReference";
        return std::nullopt;
    }

    const auto kind =
        value["kind"].get<std::string>();
    if (kind == "origin_axis") {
        if (!value.contains("axis") ||
            !value["axis"].is_string()) {
            error =
                "Native Part contains malformed schema-v13 Origin AxisReference";
            return std::nullopt;
        }
        const auto role =
            parseOriginAxisRole(
                value["axis"].get<std::string>());
        if (!role) {
            error =
                "Native Part contains unsupported schema-v13 Origin AxisReference";
            return std::nullopt;
        }
        AxisReference reference{
            BuiltinOriginAxisReference{*role}};
        if (!reference.valid()) {
            error =
                "Native Part contains invalid schema-v13 Origin AxisReference";
            return std::nullopt;
        }
        return reference;
    }

    if (kind == "authored_axis") {
        if (!value.contains("axis_id") ||
            !value["axis_id"].is_string()) {
            error =
                "Native Part contains malformed schema-v13 authored AxisReference";
            return std::nullopt;
        }
        const auto id =
            AxisId::parse(
                value["axis_id"].get<std::string>());
        if (!id ||
            !cursor.containsAllocated(*id)) {
            error =
                "Native Part contains invalid schema-v13 authored AxisReference";
            return std::nullopt;
        }
        AxisReference reference{
            AuthoredAxisReference{*id}};
        if (!reference.valid()) {
            error =
                "Native Part contains invalid schema-v13 authored AxisReference";
            return std::nullopt;
        }
        return reference;
    }

    error =
        "Native Part contains unsupported schema-v13 AxisReference";
    return std::nullopt;
}

std::optional<core::LengthUnit>
parseLengthUnitName(std::string_view value) noexcept {
    if (value == "mm") {
        return core::LengthUnit::millimetre;
    }
    if (value == "cm") {
        return core::LengthUnit::centimetre;
    }
    if (value == "m") {
        return core::LengthUnit::metre;
    }
    if (value == "in") {
        return core::LengthUnit::inch;
    }
    if (value == "ft") {
        return core::LengthUnit::foot;
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

    nlohmann::json datum_planes =
        nlohmann::json::array();
    for (const auto& datum :
         document.datumPlanes()) {
        const auto source =
            planeReferenceJson(datum.source);
        if (source.empty() ||
            !offsetDatumPlaneStructurallyValid(
                datum)) {
            return {};
        }

        datum_planes.push_back(
            {
                {"id", datum.id.serialized()},
                {"kind", "offset_plane"},
                {"source", source},
                {"offset_mm",
                 datum.offset.millimetres},
                {"visible", datum.visible},
            });
    }

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

        const auto support_json =
            sketchSupportJson(hosted.support);
        if (support_json.empty()) {
            return {};
        }

        sketches.push_back(
            {
                {"id", std::string{hosted.id.value()}},
                {"support", support_json},
                {"visible", hosted.visible},
                {"model",
                 {
                     {"next_entity_id",
                      model_state.next_entity_id.serialized()},
                     {"entities", std::move(entities)},
                 }},
            });
    }

    nlohmann::json axes =
        nlohmann::json::array();
    for (const auto& axis :
         document.axes()) {
        if (!partAxisStructurallyValid(axis)) {
            return {};
        }

        axes.push_back(
            {
                {"id", axis.id.serialized()},
                {"kind", "sketch_line"},
                {"name", axis.name},
                {"source_sketch_id",
                 std::string{
                     axis.source.sketch_id.value()}},
                {"source_entity_id",
                 axis.source.entity_id.serialized()},
                {"visible", axis.visible},
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
                {"visibility",
                 profileVisibilityPolicyName(
                     profile.visibility)},
                {"region_intent",
                 profileIntentJson(
                     profile.region_intent)},
            });
    }

    nlohmann::json features =
        nlohmann::json::array();
    for (const auto& feature :
         document.body().features) {
        if (const auto* extrude =
                std::get_if<ExtrudeFeature>(
                    &feature.definition)) {
            nlohmann::json extent;
            if (const auto* one_sided =
                    std::get_if<OneSidedExtrudeExtent>(
                        &extrude->extent)) {
                extent = {
                    {"mode", "one_side"},
                    {"distance_mm",
                     one_sided->distance.millimetres},
                    {"direction",
                     one_sided->reversed
                         ? "reverse"
                         : "forward"},
                };
            } else {
                const auto* midplane =
                    std::get_if<MidplaneExtrudeExtent>(
                        &extrude->extent);
                if (midplane == nullptr) {
                    return {};
                }
                extent = {
                    {"mode", "midplane"},
                    {"distance_mm",
                     midplane->total_distance.millimetres},
                };
            }
            features.push_back(
                {
                    {"id", feature.id.serialized()},
                    {"kind", "extrude"},
                    {"name", feature.name},
                    {"suppressed", feature.suppressed},
                    {"profile_id",
                     extrude->profile_id.serialized()},
                    {"operation",
                     extrudeOperationName(
                         extrude->operation)},
                    {"extent", std::move(extent)},
                });
            continue;
        }

        if (const auto* revolve =
                std::get_if<RevolveFeature>(
                    &feature.definition)) {
            auto axis =
                axisReferenceJson(
                    revolve->axis);
            if (axis.empty()) {
                return {};
            }

            nlohmann::json extent;
            if (const auto* one_sided =
                    std::get_if<OneSidedRevolveExtent>(
                        &revolve->extent)) {
                extent = {
                    {"mode", "one_side"},
                    {"angle_rad",
                     one_sided->angle.radians},
                    {"reverse",
                     one_sided->reversed},
                };
            } else {
                const auto* midplane =
                    std::get_if<MidplaneRevolveExtent>(
                        &revolve->extent);
                if (midplane == nullptr) {
                    return {};
                }
                extent = {
                    {"mode", "midplane"},
                    {"angle_rad",
                     midplane->total_angle.radians},
                };
            }

            features.push_back(
                {
                    {"id", feature.id.serialized()},
                    {"kind", "revolve"},
                    {"name", feature.name},
                    {"suppressed", feature.suppressed},
                    {"profile_id",
                     revolve->profile_id.serialized()},
                    {"axis", std::move(axis)},
                    {"operation",
                     revolveOperationName(
                         revolve->operation)},
                    {"extent", std::move(extent)},
                });
            continue;
        }

        const auto serialize_edges =
            [](const std::vector<
                   MaterialEdgeReference>& authored)
                -> nlohmann::json {
                nlohmann::json edges =
                    nlohmann::json::array();
                for (const auto& edge : authored) {
                    auto item =
                        materialEdgeReferenceJson(
                            edge);
                    if (item.empty()) {
                        return {};
                    }
                    edges.push_back(
                        std::move(item));
                }
                return edges;
            };

        if (const auto* fillet =
                std::get_if<FilletFeature>(
                    &feature.definition)) {
            auto edges =
                serialize_edges(
                    fillet->edges);
            if (edges.empty()) {
                return {};
            }
            features.push_back(
                {
                    {"id", feature.id.serialized()},
                    {"kind", "fillet"},
                    {"name", feature.name},
                    {"suppressed", feature.suppressed},
                    {"edges", std::move(edges)},
                    {"radius_mm",
                     fillet->radius.millimetres},
                });
            continue;
        }

        const auto* chamfer =
            std::get_if<ChamferFeature>(
                &feature.definition);
        if (chamfer == nullptr) {
            return {};
        }
        auto edges =
            serialize_edges(
                chamfer->edges);
        if (edges.empty()) {
            return {};
        }
        features.push_back(
            {
                {"id", feature.id.serialized()},
                {"kind", "chamfer"},
                {"name", feature.name},
                {"suppressed", feature.suppressed},
                {"edges", std::move(edges)},
                {"distance_mm",
                 chamfer->distance.millimetres},
            });
    }

    nlohmann::json body{
        {"id", document.body().id.serialized()},
        {"next_feature_id",
         document.body()
             .next_feature_id.serialized()},
        {"features", std::move(features)},
    };

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
        {"length_unit",
         std::string{
             core::lengthUnitSuffix(
                 document.lengthUnit())}},
        {"next_datum_id",
         document.datumIdCursor().serialized()},
        {"datum_planes",
         std::move(datum_planes)},
        {"sketches", std::move(sketches)},
        {"next_axis_id",
         document.axisIdCursor().serialized()},
        {"axes", std::move(axes)},
        {"next_profile_id",
         document.profileIdCursor().serialized()},
        {"profiles", std::move(profiles)},
        {"modeling_semantics_version",
         document.modelingSemanticsVersion().value},
        {"next_body_id",
         document.bodyIdCursor().serialized()},
        {"body", std::move(body)},
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

std::optional<std::uint32_t> parseUint32(
    const nlohmann::json& value);

std::optional<PartSketchSupport>
parseSketchSupportV9(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_object() ||
        !value.contains("kind") ||
        !value["kind"].is_string()) {
        error =
            "Native Part contains malformed schema-v9 Sketch support";
        return std::nullopt;
    }

    const auto kind =
        value["kind"].get<std::string>();

    if (kind == "builtin_origin_plane") {
        if (value.size() != 2U ||
            !value.contains("builtin_plane") ||
            !value["builtin_plane"].is_string()) {
            error =
                "Native Part contains malformed schema-v9 Origin Sketch support";
            return std::nullopt;
        }

        const auto role =
            parseSupportRole(
                value["builtin_plane"]
                    .get<std::string>());
        if (!role) {
            error =
                "Native Part contains unsupported schema-v9 Origin Sketch support";
            return std::nullopt;
        }

        auto support =
            partSketchSupportForBuiltinPlane(
                *role);
        if (!support) {
            error =
                "Native Part contains invalid schema-v9 Origin Sketch support";
        }
        return support;
    }

    if (kind != "body_planar_surface" ||
        value.size() != 3U ||
        !value.contains("stage") ||
        !value.contains("surface")) {
        error =
            "Native Part contains unsupported schema-v9 Sketch support";
        return std::nullopt;
    }

    const auto& stage_json = value["stage"];
    if (!stage_json.is_object() ||
        stage_json.size() != 2U ||
        !stage_json.contains("kind") ||
        !stage_json.contains("feature_id") ||
        !stage_json["kind"].is_string() ||
        !stage_json["feature_id"].is_string() ||
        stage_json["kind"].get<std::string>() !=
            "after_feature") {
        error =
            "Native Part contains malformed schema-v9 Sketch support stage";
        return std::nullopt;
    }

    const auto stage_feature =
        FeatureId::parse(
            stage_json["feature_id"]
                .get<std::string>());
    if (!stage_feature) {
        error =
            "Native Part contains invalid schema-v9 Sketch support stage FeatureId";
        return std::nullopt;
    }

    const auto& surface_json = value["surface"];
    if (!surface_json.is_object() ||
        !surface_json.contains(
            "producer_feature_id") ||
        !surface_json.contains("role") ||
        !surface_json["producer_feature_id"]
             .is_string() ||
        !surface_json["role"].is_string()) {
        error =
            "Native Part contains malformed schema-v9 Surface reference";
        return std::nullopt;
    }

    const auto producer =
        FeatureId::parse(
            surface_json["producer_feature_id"]
                .get<std::string>());
    const auto role =
        parseSurfaceRole(
            surface_json["role"]
                .get<std::string>());
    if (!producer || !role) {
        error =
            "Native Part contains invalid schema-v9 Surface reference identity";
        return std::nullopt;
    }

    FeatureSurfaceAddress surface;
    surface.producer_feature_id = *producer;
    surface.role = *role;

    if (*role == FeatureSurfaceRoleKind::side) {
        if (surface_json.size() != 6U ||
            !surface_json.contains("source_entity") ||
            !surface_json.contains("loop_index") ||
            !surface_json.contains("use_index") ||
            !surface_json.contains("hole") ||
            !surface_json["source_entity"]
                 .is_string() ||
            !surface_json["hole"].is_boolean()) {
            error =
                "Native Part contains malformed schema-v9 side Surface reference";
            return std::nullopt;
        }

        const auto entity =
            sketch::EntityId::parse(
                surface_json["source_entity"]
                    .get<std::string>());
        const auto loop =
            parseUint32(
                surface_json["loop_index"]);
        const auto use =
            parseUint32(
                surface_json["use_index"]);
        if (!entity || !loop || !use) {
            error =
                "Native Part contains invalid schema-v9 side Surface provenance";
            return std::nullopt;
        }

        surface.source_entity = *entity;
        surface.loop_index = *loop;
        surface.use_index = *use;
        surface.hole =
            surface_json["hole"].get<bool>();
    } else if (surface_json.size() != 2U) {
        error =
            "Native Part cap Surface reference contains unexpected provenance fields";
        return std::nullopt;
    }

    SurfaceReference reference{
        BodyStageRef{
            BodyStageKind::after_feature,
            *stage_feature},
        std::move(surface)};
    auto support =
        partSketchSupportForBodyPlanarSurface(
            std::move(reference));
    if (!support) {
        error =
            "Native Part contains invalid schema-v9 Body Surface Sketch support";
    }
    return support;
}

std::optional<PartSketchSupport>
parseSketchSupportV11(
    const nlohmann::json& value,
    std::string& error) {
    if (value.is_object() &&
        value.contains("kind") &&
        value["kind"].is_string() &&
        value["kind"].get<std::string>() ==
            "datum_plane") {
        if (value.size() != 2U ||
            !value.contains("datum_id") ||
            !value["datum_id"].is_string()) {
            error =
                "Native Part contains malformed schema-v11 Datum Sketch support";
            return std::nullopt;
        }

        const auto datum_id =
            DatumId::parse(
                value["datum_id"]
                    .get<std::string>());
        if (!datum_id) {
            error =
                "Native Part contains invalid schema-v11 DatumId Sketch support";
            return std::nullopt;
        }

        auto support =
            partSketchSupportForDatumPlane(
                *datum_id);
        if (!support) {
            error =
                "Native Part contains invalid schema-v11 Datum Sketch support";
        }
        return support;
    }

    return parseSketchSupportV9(
        value,
        error);
}

std::optional<PlaneReference>
parsePlaneReferenceV10(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_object() ||
        !value.contains("kind") ||
        !value["kind"].is_string()) {
        error =
            "Native Part contains malformed schema-v10 PlaneReference";
        return std::nullopt;
    }

    const auto kind =
        value["kind"].get<std::string>();
    if (kind == "datum_plane") {
        if (value.size() != 2U ||
            !value.contains("datum_id") ||
            !value["datum_id"].is_string()) {
            error =
                "Native Part contains malformed schema-v10 Datum Plane reference";
            return std::nullopt;
        }

        const auto datum_id =
            DatumId::parse(
                value["datum_id"]
                    .get<std::string>());
        if (!datum_id) {
            error =
                "Native Part contains invalid schema-v10 DatumId reference";
            return std::nullopt;
        }

        return PlaneReference{
            DatumPlaneReference{*datum_id}};
    }

    auto support =
        parseSketchSupportV9(value, error);
    if (!support) {
        return std::nullopt;
    }

    if (const auto role =
            builtinOriginPlaneForSketchSupport(
                *support)) {
        return PlaneReference{
            BuiltinOriginPlaneReference{*role}};
    }

    if (const auto* surface =
            bodyPlanarSurfaceReference(
                *support)) {
        return PlaneReference{
            BodyPlanarSurfacePlaneReference{
                *surface}};
    }

    error =
        "Native Part contains unsupported schema-v10 PlaneReference";
    return std::nullopt;
}

bool parseDatumPlanesV10(
    const nlohmann::json& value,
    DatumIdCursor cursor,
    std::vector<OffsetDatumPlane>& datum_planes,
    std::string& error) {
    if (!value.is_array()) {
        error =
            "Native Part Datum Plane payload must be an array";
        return false;
    }

    datum_planes.clear();
    datum_planes.reserve(value.size());
    std::set<std::string> ids;

    for (const auto& item : value) {
        if (!item.is_object() ||
            item.size() != 5U ||
            !item.contains("id") ||
            !item.contains("kind") ||
            !item.contains("source") ||
            !item.contains("offset_mm") ||
            !item.contains("visible") ||
            !item["id"].is_string() ||
            !item["kind"].is_string() ||
            item["kind"].get<std::string>() !=
                "offset_plane" ||
            !item["offset_mm"].is_number() ||
            !item["visible"].is_boolean()) {
            error =
                "Native Part contains malformed schema-v10 Datum Plane";
            return false;
        }

        const auto serialized_id =
            item["id"].get<std::string>();
        const auto id =
            DatumId::parse(serialized_id);
        auto source =
            parsePlaneReferenceV10(
                item["source"],
                error);
        const auto offset =
            item["offset_mm"].get<double>();

        if (!id ||
            !cursor.containsAllocated(*id) ||
            !ids.insert(serialized_id).second ||
            !source ||
            !std::isfinite(offset)) {
            if (error.empty()) {
                error =
                    "Native Part contains invalid schema-v10 Datum Plane state";
            }
            return false;
        }

        OffsetDatumPlane datum{
            *id,
            std::move(*source),
            core::LengthValue{offset},
            item["visible"].get<bool>()};
        if (!offsetDatumPlaneStructurallyValid(
                datum)) {
            error =
                "Native Part contains structurally invalid schema-v10 Datum Plane";
            return false;
        }

        datum_planes.push_back(
            std::move(datum));
    }

    return true;
}

std::optional<PartSketchSupport>
parseLegacyOriginSketchSupport(
    const nlohmann::json& value,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 2U ||
        !value.contains("kind") ||
        !value.contains("builtin_plane") ||
        !value["kind"].is_string() ||
        !value["builtin_plane"].is_string() ||
        value["kind"].get<std::string>() !=
            "builtin_origin_plane") {
        error =
            "Native Part contains malformed legacy Sketch support";
        return std::nullopt;
    }

    const auto role =
        parseSupportRole(
            value["builtin_plane"]
                .get<std::string>());
    if (!role) {
        error =
            "Native Part contains unsupported legacy Sketch support";
        return std::nullopt;
    }

    auto support =
        partSketchSupportForBuiltinPlane(*role);
    if (!support) {
        error =
            "Native Part contains invalid legacy Sketch support";
    }
    return support;
}

bool parseSketches(
    const nlohmann::json& sketches_json,
    int schema_version,
    std::vector<PartSketch>& sketches,
    std::string& error) {
    if (!sketches_json.is_array()) {
        error =
            "Native Part sketches payload must be an array";
        return false;
    }

    const bool schema_has_model =
        schema_version >= 3;
    const bool schema_v9 =
        schema_version >= 9;
    const bool schema_v11 =
        schema_version >= 11;
    const std::size_t expected_fields =
        schema_v9
            ? 4U
            : (schema_has_model ? 5U : 4U);

    std::set<std::string> ids;

    for (const auto& item : sketches_json) {
        if (!item.is_object() ||
            item.size() != expected_fields ||
            !item.contains("id") ||
            !item.contains("support") ||
            !item.contains("visible") ||
            (schema_v9
                 ? item.contains("placement")
                 : !item.contains("placement")) ||
            (schema_has_model &&
             !item.contains("model")) ||
            !item["id"].is_string() ||
            !item["visible"].is_boolean()) {
            error =
                "Native Part contains malformed Sketch record";
            return false;
        }

        const auto serialized_id =
            item["id"].get<std::string>();
        auto id =
            sketch::SketchId::parse(
                serialized_id);
        if (!id) {
            error =
                "Native Part contains invalid SketchId";
            return false;
        }
        if (!ids.insert(serialized_id).second) {
            error =
                "Native Part contains duplicate SketchId";
            return false;
        }

        auto support =
            schema_v11
                ? parseSketchSupportV11(
                      item["support"],
                      error)
                : (schema_v9
                       ? parseSketchSupportV9(
                             item["support"],
                             error)
                       : parseLegacyOriginSketchSupport(
                             item["support"],
                             error));
        if (!support) {
            return false;
        }

        if (!schema_v9) {
            const auto& placement_json =
                item["placement"];
            if (!placement_json.is_object() ||
                placement_json.size() != 3U ||
                !placement_json.contains("origin") ||
                !placement_json.contains("u_axis") ||
                !placement_json.contains("v_axis")) {
                error =
                    "Native Part contains malformed legacy Sketch placement";
                return false;
            }

            const auto origin =
                parseVector3(
                    placement_json["origin"]);
            const auto u_axis =
                parseVector3(
                    placement_json["u_axis"]);
            const auto v_axis =
                parseVector3(
                    placement_json["v_axis"]);
            if (!origin || !u_axis || !v_axis) {
                error =
                    "Native Part contains invalid legacy Sketch placement vectors";
                return false;
            }

            const SketchPlacement placement{
                *origin,
                *u_axis,
                *v_axis};
            const auto expected =
                sketchPlacementForSupport(
                    *support);
            if (!placement.valid() ||
                !expected ||
                placement != *expected) {
                error =
                    "Native Part legacy Sketch placement does not match its Origin-plane support";
                return false;
            }
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
                std::move(*support),
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

bool parseAxesV12(
    const nlohmann::json& value,
    AxisIdCursor cursor,
    std::vector<PartAxis>& axes,
    std::string& error) {
    if (!value.is_array()) {
        error =
            "Native Part Axis payload must be an array";
        return false;
    }

    axes.clear();
    axes.reserve(value.size());
    std::set<std::string> ids;

    for (const auto& item : value) {
        if (!item.is_object() ||
            item.size() != 6U ||
            !item.contains("id") ||
            !item.contains("kind") ||
            !item.contains("name") ||
            !item.contains("source_sketch_id") ||
            !item.contains("source_entity_id") ||
            !item.contains("visible") ||
            !item["id"].is_string() ||
            !item["kind"].is_string() ||
            !item["name"].is_string() ||
            !item["source_sketch_id"].is_string() ||
            !item["source_entity_id"].is_string() ||
            !item["visible"].is_boolean() ||
            item["kind"].get<std::string>() !=
                "sketch_line") {
            error =
                "Native Part Axis payload is invalid";
            return false;
        }

        const auto id_text =
            item["id"].get<std::string>();
        const auto id =
            AxisId::parse(id_text);
        if (!id ||
            !cursor.containsAllocated(*id) ||
            !ids.insert(id_text).second) {
            error =
                "Native Part Axis identity is invalid";
            return false;
        }

        const auto sketch_id =
            sketch::SketchId::parse(
                item["source_sketch_id"]
                    .get<std::string>());
        const auto entity_id =
            sketch::EntityId::parse(
                item["source_entity_id"]
                    .get<std::string>());
        if (!sketch_id || !entity_id) {
            error =
                "Native Part Axis source identity is invalid";
            return false;
        }

        PartAxis axis{
            *id,
            item["name"].get<std::string>(),
            SketchLineAxisSource{
                *sketch_id,
                *entity_id},
            item["visible"].get<bool>()};
        if (!partAxisStructurallyValid(axis)) {
            error =
                "Native Part contains structurally invalid schema-v12 Axis";
            return false;
        }

        axes.push_back(std::move(axis));
    }

    return true;
}

bool parseProfiles(
    const nlohmann::json& value,
    int schema_version,
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
        const bool has_visibility_policy =
            schema_version >= 8;
        if (!item.is_object() ||
            item.size() != 5U ||
            !item.contains("id") ||
            !item.contains("source_sketch_id") ||
            !item.contains("name") ||
            !(has_visibility_policy
                  ? item.contains("visibility")
                  : item.contains("visible")) ||
            !item.contains("region_intent") ||
            !item["id"].is_string() ||
            !item["source_sketch_id"].is_string() ||
            !item["name"].is_string() ||
            (has_visibility_policy
                 ? !item["visibility"].is_string()
                 : !item["visible"].is_boolean())) {
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

        ProfileVisibilityPolicy visibility =
            ProfileVisibilityPolicy::automatic;
        if (has_visibility_policy) {
            const auto parsed_visibility =
                parseProfileVisibilityPolicy(
                    item["visibility"]
                        .get<std::string>());
            if (!parsed_visibility) {
                error =
                    "Native Part contains invalid Profile visibility policy";
                return false;
            }
            visibility = *parsed_visibility;
        } else if (!item["visible"].get<bool>()) {
            visibility =
                ProfileVisibilityPolicy::force_hidden;
        }

        profiles.push_back(
            PartProfile{
                *id,
                std::move(*sketch_id),
                item["name"].get<std::string>(),
                visibility,
                std::move(*intent)});
    }
    return true;
}

bool parseBodyV8OrV13(
    const nlohmann::json& value,
    int schema_version,
    BodyIdCursor body_cursor,
    ProfileIdCursor profile_cursor,
    AxisIdCursor axis_cursor,
    PartBody& body,
    std::string& error) {
    if (!value.is_object() ||
        value.size() != 3U ||
        !value.contains("id") ||
        !value.contains("next_feature_id") ||
        !value.contains("features") ||
        !value["id"].is_string() ||
        !value["next_feature_id"].is_string() ||
        !value["features"].is_array()) {
        error = "Native Part contains malformed Body record";
        return false;
    }

    const auto body_id =
        BodyId::parse(value["id"].get<std::string>());
    const auto feature_cursor =
        FeatureIdCursor::parse(
            value["next_feature_id"].get<std::string>());
    if (!body_id || !feature_cursor ||
        !body_cursor.containsAllocated(*body_id)) {
        error = "Native Part contains invalid Body identity";
        return false;
    }

    PartBody parsed;
    parsed.id = *body_id;
    parsed.next_feature_id = *feature_cursor;
    parsed.features.reserve(value["features"].size());

    for (const auto& item : value["features"]) {
        if (!item.is_object() ||
            !item.contains("id") ||
            !item.contains("kind") ||
            !item.contains("name") ||
            !item.contains("suppressed") ||
            !item.contains("profile_id") ||
            !item.contains("operation") ||
            !item.contains("extent") ||
            !item["id"].is_string() ||
            !item["kind"].is_string() ||
            !item["name"].is_string() ||
            !item["suppressed"].is_boolean() ||
            !item["profile_id"].is_string() ||
            !item["operation"].is_string()) {
            error = "Native Part contains malformed Feature record";
            return false;
        }

        const auto feature_id =
            FeatureId::parse(item["id"].get<std::string>());
        const auto profile_id =
            ProfileId::parse(
                item["profile_id"].get<std::string>());
        if (!feature_id || !profile_id ||
            !feature_cursor->containsAllocated(*feature_id) ||
            !profile_cursor.containsAllocated(*profile_id)) {
            error =
                "Native Part contains invalid Feature identity/reference";
            return false;
        }

        const auto kind =
            item["kind"].get<std::string>();
        if (kind == "extrude") {
            if (item.size() != 7U) {
                error =
                    "Native Part contains malformed Extrude Feature record";
                return false;
            }
            const auto operation =
                parseExtrudeOperation(
                    item["operation"].get<std::string>());
            if (!operation) {
                error =
                    "Native Part contains invalid Extrude operation";
                return false;
            }

            const auto& extent_json = item["extent"];
            if (!extent_json.is_object() ||
                !extent_json.contains("mode") ||
                !extent_json.contains("distance_mm") ||
                !extent_json["mode"].is_string() ||
                !extent_json["distance_mm"].is_number()) {
                error =
                    "Native Part contains malformed Extrude extent";
                return false;
            }
            const double distance =
                extent_json["distance_mm"].get<double>();
            if (!std::isfinite(distance) ||
                distance <= 0.0) {
                error =
                    "Native Part contains invalid Extrude distance";
                return false;
            }

            ExtrudeExtent extent;
            const auto mode =
                extent_json["mode"].get<std::string>();
            if (mode == "one_side") {
                if (extent_json.size() != 3U ||
                    !extent_json.contains("direction") ||
                    !extent_json["direction"].is_string()) {
                    error =
                        "Native Part contains malformed OneSide Extrude extent";
                    return false;
                }
                const auto direction =
                    extent_json["direction"].get<std::string>();
                if (direction != "forward" &&
                    direction != "reverse") {
                    error =
                        "Native Part contains invalid OneSide Extrude direction";
                    return false;
                }
                extent = OneSidedExtrudeExtent{
                    core::LengthValue{distance},
                    direction == "reverse"};
            } else if (mode == "midplane") {
                if (extent_json.size() != 2U) {
                    error =
                        "Native Part Midplane extent contains unexpected fields";
                    return false;
                }
                extent = MidplaneExtrudeExtent{
                    core::LengthValue{distance}};
            } else {
                error =
                    "Native Part contains unsupported Extrude extent mode";
                return false;
            }

            PartFeature feature{
                *feature_id,
                item["name"].get<std::string>(),
                item["suppressed"].get<bool>(),
                ExtrudeFeature{
                    *profile_id,
                    *operation,
                    std::move(extent)}};
            if (!partFeatureDefinitionStructurallyValid(
                    feature.definition)) {
                error =
                    "Native Part contains structurally invalid Feature";
                return false;
            }
            parsed.features.push_back(
                std::move(feature));
            continue;
        }

        if (kind != "revolve" ||
            schema_version < 13) {
            error =
                "Native Part contains unsupported Feature kind";
            return false;
        }
        if (item.size() != 8U ||
            !item.contains("axis")) {
            error =
                "Native Part contains malformed Revolve Feature record";
            return false;
        }

        const auto operation =
            parseRevolveOperation(
                item["operation"].get<std::string>());
        auto axis =
            parseAxisReferenceV13(
                item["axis"],
                axis_cursor,
                error);
        if (!operation || !axis) {
            if (error.empty()) {
                error =
                    "Native Part contains invalid Revolve operation/reference";
            }
            return false;
        }

        const auto& extent_json = item["extent"];
        if (!extent_json.is_object() ||
            !extent_json.contains("mode") ||
            !extent_json.contains("angle_rad") ||
            !extent_json["mode"].is_string() ||
            !extent_json["angle_rad"].is_number()) {
            error =
                "Native Part contains malformed Revolve extent";
            return false;
        }
        const double angle =
            extent_json["angle_rad"].get<double>();
        if (!std::isfinite(angle) ||
            angle <= 0.0) {
            error =
                "Native Part contains invalid Revolve angle";
            return false;
        }

        RevolveExtent extent;
        const auto mode =
            extent_json["mode"].get<std::string>();
        if (mode == "one_side") {
            if (extent_json.size() != 3U ||
                !extent_json.contains("reverse") ||
                !extent_json["reverse"].is_boolean()) {
                error =
                    "Native Part contains malformed OneSide Revolve extent";
                return false;
            }
            extent = OneSidedRevolveExtent{
                core::AngleValue{angle},
                extent_json["reverse"].get<bool>()};
        } else if (mode == "midplane") {
            if (extent_json.size() != 2U) {
                error =
                    "Native Part Midplane Revolve extent contains unexpected fields";
                return false;
            }
            extent = MidplaneRevolveExtent{
                core::AngleValue{angle}};
        } else {
            error =
                "Native Part contains unsupported Revolve extent mode";
            return false;
        }

        PartFeature feature{
            *feature_id,
            item["name"].get<std::string>(),
            item["suppressed"].get<bool>(),
            RevolveFeature{
                *profile_id,
                std::move(*axis),
                *operation,
                std::move(extent)}};
        if (!partFeatureDefinitionStructurallyValid(
                feature.definition)) {
            error =
                "Native Part contains structurally invalid Revolve Feature";
            return false;
        }
        parsed.features.push_back(
            std::move(feature));
    }

    body = std::move(parsed);
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
    const bool has_length_unit =
        schema_version >= 7;
    const bool has_body_features =
        schema_version >= 8;
    const bool has_datums =
        schema_version >= 10;
    const bool has_axes =
        schema_version >= 12;
    const std::size_t expected_fields =
        legacy_v1
            ? 2U
            : (has_axes
                   ? 13U
                   : (has_datums
                   ? 11U
                   : (has_body_features
                          ? 9U
                          : (has_profiles
                                 ? (has_length_unit ? 6U : 5U)
                                 : 3U))));

    if (authored.is_discarded() ||
        !authored.is_object() ||
        authored.size() != expected_fields ||
        !authored.contains("properties") ||
        !authored.contains("presentation") ||
        (!legacy_v1 &&
         !authored.contains("sketches")) ||
        (has_length_unit &&
         (!authored.contains("length_unit") ||
          !authored["length_unit"].is_string())) ||
        (has_profiles &&
         (!authored.contains("next_profile_id") ||
          !authored.contains("profiles"))) ||
        (has_body_features &&
         (!authored.contains("modeling_semantics_version") ||
          !authored.contains("next_body_id") ||
          !authored.contains("body") ||
          !authored["next_body_id"].is_string())) ||
        (has_datums &&
         (!authored.contains("next_datum_id") ||
          !authored["next_datum_id"].is_string() ||
          !authored.contains("datum_planes") ||
          !authored["datum_planes"].is_array())) ||
        (has_axes &&
         (!authored.contains("next_axis_id") ||
          !authored["next_axis_id"].is_string() ||
          !authored.contains("axes") ||
          !authored["axes"].is_array()))) {
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

    if (has_length_unit) {
        const auto length_unit =
            parseLengthUnitName(
                authored["length_unit"]
                    .get<std::string>());
        if (!length_unit) {
            error =
                "Native Part contains an invalid length unit";
            return std::nullopt;
        }
        state.length_unit = *length_unit;
    }

    if (!legacy_v1 &&
        !parseSketches(
            authored["sketches"],
            schema_version,
            state.sketches,
            error)) {
        return std::nullopt;
    }

    if (has_axes) {
        const auto cursor =
            AxisIdCursor::parse(
                authored["next_axis_id"]
                    .get<std::string>());
        if (!cursor) {
            error =
                "Native Part next_axis_id is invalid";
            return std::nullopt;
        }
        state.next_axis_id = *cursor;
        if (!parseAxesV12(
                authored["axes"],
                *cursor,
                state.axes,
                error)) {
            return std::nullopt;
        }
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
                schema_version,
                *cursor,
                state.profiles,
                error)) {
            return std::nullopt;
        }
    }

    if (has_body_features) {
        const auto semantics =
            parseUint32(
                authored["modeling_semantics_version"]);
        if (!semantics ||
            *semantics !=
                current_modeling_semantics_version.value) {
            error =
                "Native Part contains unsupported modeling semantics version";
            return std::nullopt;
        }
        state.modeling_semantics_version =
            ModelingSemanticsVersion{*semantics};

        const auto body_cursor =
            BodyIdCursor::parse(
                authored["next_body_id"].get<std::string>());
        if (!body_cursor) {
            error = "Native Part next_body_id is invalid";
            return std::nullopt;
        }
        state.next_body_id = *body_cursor;
        if (!parseBodyV8OrV13(
                authored["body"],
                schema_version,
                *body_cursor,
                state.next_profile_id,
                state.next_axis_id,
                state.body,
                error)) {
            return std::nullopt;
        }
    }

    if (has_datums) {
        const auto cursor =
            DatumIdCursor::parse(
                authored["next_datum_id"]
                    .get<std::string>());
        if (!cursor) {
            error =
                "Native Part next_datum_id is invalid";
            return std::nullopt;
        }
        state.next_datum_id = *cursor;
        if (!parseDatumPlanesV10(
                authored["datum_planes"],
                *cursor,
                state.datum_planes,
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

    if (descriptor.domain_schema_version < 1 ||
        descriptor.domain_schema_version >
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
