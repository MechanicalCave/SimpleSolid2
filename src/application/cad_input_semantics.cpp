#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/precision_input.hpp>

#include <array>
#include <charconv>
#include <cmath>
#include <numbers>
#include <cctype>
#include <string>
#include <utility>

namespace simplesolid2::application {
namespace {

std::string_view trimAscii(std::string_view text) noexcept {
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1U);
    }
    while (!text.empty() &&
           std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1U);
    }
    return text;
}

std::string upperAscii(std::string_view text) {
    std::string result;
    result.reserve(text.size());
    for (const unsigned char ch : text) {
        result.push_back(static_cast<char>(std::toupper(ch)));
    }
    return result;
}

std::string formatDynamicNumber(
    double value) {
    if (value == 0.0) {
        value = 0.0;
    }

    std::array<char, 64> buffer{};
    const auto converted =
        std::to_chars(
            buffer.data(),
            buffer.data() + buffer.size(),
            value,
            std::chars_format::general,
            12);
    if (converted.ec != std::errc{}) {
        return {};
    }
    return std::string{
        buffer.data(),
        converted.ptr};
}

std::string formatDynamicValue(
    CadDynamicInputFieldSemantic semantic,
    double canonical_value,
    core::LengthUnit length_unit) {
    if (!std::isfinite(canonical_value)) {
        return {};
    }

    switch (semantic) {
    case CadDynamicInputFieldSemantic::u:
    case CadDynamicInputFieldSemantic::v:
    case CadDynamicInputFieldSemantic::distance:
    case CadDynamicInputFieldSemantic::delta_u:
    case CadDynamicInputFieldSemantic::delta_v:
    case CadDynamicInputFieldSemantic::width:
    case CadDynamicInputFieldSemantic::height:
    case CadDynamicInputFieldSemantic::diameter:
    case CadDynamicInputFieldSemantic::radius: {
        const double display =
            core::fromCanonicalLength(
                core::LengthValue{
                    canonical_value},
                length_unit);
        auto text =
            formatDynamicNumber(display);
        if (text.empty()) {
            return {};
        }
        text += " ";
        text += core::lengthUnitSuffix(length_unit);
        return text;
    }

    case CadDynamicInputFieldSemantic::angle:
    case CadDynamicInputFieldSemantic::axis_angle: {
        const double degrees =
            canonical_value * 180.0 /
            std::numbers::pi_v<double>;
        auto text =
            formatDynamicNumber(degrees);
        if (text.empty()) {
            return {};
        }
        text += "\xC2\xB0";
        return text;
    }

    case CadDynamicInputFieldSemantic::factor:
        return formatDynamicNumber(
            canonical_value);
    }

    return {};
}

std::optional<ProfileCadInputCommand>
profileCommand(std::string_view token) noexcept {
    if (token == "PROFILE") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::start_create,
            std::nullopt};
    }
    if (token == "EDITPROFILE") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::start_edit,
            std::nullopt};
    }
    if (token == "ADD") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::add_area,
            std::nullopt};
    }
    if (token == "SUBTRACT") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::subtract_area,
            std::nullopt};
    }
    if (token == "FINISH") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::finish,
            std::nullopt};
    }
    if (token == "CANCEL") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::cancel,
            std::nullopt};
    }
    if (token == "ISLANDS ON" ||
        token == "ISLANDS OFF") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::set_show_islands,
            token == "ISLANDS ON"};
    }
    if (token == "BOUNDARIES ON" ||
        token == "BOUNDARIES OFF") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::set_show_boundaries,
            token == "BOUNDARIES ON"};
    }
    if (token == "PROBLEMS ON" ||
        token == "PROBLEMS OFF") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::set_show_problems,
            token == "PROBLEMS ON"};
    }
    return std::nullopt;
}

std::optional<sketch::SketchTool>
commandTool(std::string_view token) noexcept {
    if (token == "SELECT") return sketch::SketchTool::select;
    if (token == "LINE") return sketch::SketchTool::line;
    if (token == "CIRCLE") return sketch::SketchTool::circle;
    if (token == "ARC") return sketch::SketchTool::arc;
    if (token == "RECTANGLE") return sketch::SketchTool::rectangle;
    if (token == "MEASURE") return sketch::SketchTool::measure;
    if (token == "MOVE") return sketch::SketchTool::move;
    if (token == "COPY") return sketch::SketchTool::copy;
    if (token == "ROTATE") return sketch::SketchTool::rotate;
    if (token == "SCALE") return sketch::SketchTool::scale;
    if (token == "MIRROR") return sketch::SketchTool::mirror;
    if (token == "TRIM") return sketch::SketchTool::trim;
    if (token == "EXTEND") return sketch::SketchTool::extend;
    if (token == "EXTENDBOTH" ||
        token == "EXTEND BOTH") {
        return sketch::SketchTool::extend_both;
    }
    return std::nullopt;
}

std::optional<std::pair<double, double>>
parseSemanticPair(
    std::string_view text,
    const CadInputPairRequest& request,
    core::LengthUnit length_unit) {
    const auto delimiter = text.find(';');
    if (delimiter == std::string_view::npos ||
        text.find(';', delimiter + 1U) !=
            std::string_view::npos) {
        return std::nullopt;
    }

    const auto first_text =
        trimAscii(text.substr(0U, delimiter));
    const auto second_text =
        trimAscii(text.substr(delimiter + 1U));
    if (first_text.empty() || second_text.empty()) {
        return std::nullopt;
    }

    const auto first = parseCadQuantity(
        first_text,
        {request.first_dimension, length_unit});
    const auto second = parseCadQuantity(
        second_text,
        {request.second_dimension, length_unit});
    if (!first || !second) {
        return std::nullopt;
    }
    if (request.strictly_positive &&
        (first->canonical_value <= 0.0 ||
         second->canonical_value <= 0.0)) {
        return std::nullopt;
    }

    return std::pair{
        first->canonical_value,
        second->canonical_value};
}

std::optional<sketch::ExplicitPointInput>
explicitPointInput(const CadPointToken& token) noexcept {
    sketch::ExplicitPointInputKind kind{};
    switch (token.kind) {
    case CadPointTokenKind::absolute_cartesian:
        kind =
            sketch::ExplicitPointInputKind::
                absolute_cartesian;
        break;
    case CadPointTokenKind::relative_cartesian:
        kind =
            sketch::ExplicitPointInputKind::
                relative_cartesian;
        break;
    case CadPointTokenKind::relative_polar:
        kind =
            sketch::ExplicitPointInputKind::
                relative_polar;
        break;
    }

    sketch::ExplicitPointInput input{
        kind,
        token.first,
        token.second};
    return input.valid()
        ? std::optional<sketch::ExplicitPointInput>{input}
        : std::nullopt;
}

} // namespace

SketchCadInputSemanticEndpoint::SketchCadInputSemanticEndpoint(
    ISketchCadInputSemanticTarget& target,
    CadInputNumberFormat number_format)
    : target_{&target},
      number_format_{std::move(number_format)} {}

std::vector<CadDynamicInputField>
SketchCadInputSemanticEndpoint::dynamicInputFields() const {
    if (target_ == nullptr ||
        !target_->cadInputSemanticActive()) {
        return {};
    }

    using Field = CadDynamicInputField;
    using Semantic = CadDynamicInputFieldSemantic;

    if (const auto pair =
            target_->cadInputSemanticPairRequest()) {
        switch (pair->semantic) {
        case CadInputPairRequestSemantic::rectangle_size:
            return {
                Field{Semantic::width, "Width"},
                Field{Semantic::height, "Height"},
            };
        }
    }

    if (const auto value =
            target_->cadInputSemanticValueRequest()) {
        switch (value->semantic) {
        case CadInputValueRequestSemantic::circle_size:
            return {
                target_->cadInputSemanticCircleSizeMode() ==
                        CircleSizeInputMode::radius
                    ? Field{Semantic::radius, "Radius"}
                    : Field{Semantic::diameter, "Diameter"},
            };
        case CadInputValueRequestSemantic::arc_radius:
            return {
                Field{Semantic::radius, "Radius"}};
        case CadInputValueRequestSemantic::rotate_angle:
            return {
                Field{Semantic::angle, "Angle"}};
        case CadInputValueRequestSemantic::scale_factor:
            return {
                Field{Semantic::factor, "Factor"}};
        case CadInputValueRequestSemantic::mirror_axis_angle:
            return {
                Field{Semantic::axis_angle, "Axis Angle"}};
        }
    }

    const auto point =
        target_->cadInputSemanticPointRequest();
    if (!point) {
        return {};
    }

    if (!point->base) {
        return {
            Field{Semantic::u, "U"},
            Field{Semantic::v, "V"},
        };
    }

    return {
        Field{Semantic::distance, "Distance"},
        Field{Semantic::angle, "Angle"},
        Field{Semantic::delta_u, "dU"},
        Field{Semantic::delta_v, "dV"},
    };
}

std::vector<CadDynamicInputFieldSnapshot>
SketchCadInputSemanticEndpoint::
dynamicInputFieldSnapshots() const {
    std::vector<CadDynamicInputFieldSnapshot>
        snapshots;
    const auto fields = dynamicInputFields();
    snapshots.reserve(fields.size());

    for (const auto& field : fields) {
        CadDynamicInputFieldSnapshot snapshot;
        snapshot.field = field;

        if (target_ != nullptr) {
            const auto value =
                target_->
                    cadInputSemanticDynamicFieldValue(
                        field.semantic);
            if (value &&
                value->valid() &&
                value->semantic ==
                    field.semantic) {
                snapshot.value = value;
                snapshot.display_value =
                    formatDynamicValue(
                        field.semantic,
                        value->canonical_value,
                        number_format_.length_unit);
            }
        }

        snapshots.push_back(
            std::move(snapshot));
    }

    return snapshots;
}

CadInputSubmitResult
SketchCadInputSemanticEndpoint::lockDynamicInputField(
    std::size_t index,
    std::string_view text) {
    if (target_ == nullptr ||
        !target_->cadInputSemanticActive()) {
        return {false, "No active CAD command context."};
    }

    const auto fields = dynamicInputFields();
    if (index >= fields.size()) {
        return {
            false,
            "Dynamic Input field index is stale."};
    }

    const auto submitted = trimAscii(text);
    if (submitted.empty()) {
        return {
            false,
            "Dynamic Input field token is empty."};
    }

    const auto pair_request =
        target_->cadInputSemanticPairRequest();
    if (pair_request && fields.size() == 2U) {
        const auto semantic =
            fields[index].semantic;
        if (semantic !=
                CadDynamicInputFieldSemantic::width &&
            semantic !=
                CadDynamicInputFieldSemantic::height) {
            return {
                false,
                "Active pair Dynamic Input field is invalid."};
        }

        const auto dimension =
            semantic ==
                    CadDynamicInputFieldSemantic::width
                ? pair_request->first_dimension
                : pair_request->second_dimension;
        const auto quantity =
            parseCadQuantity(
                submitted,
                {
                    dimension,
                    number_format_.length_unit});
        if (!quantity ||
            (pair_request->strictly_positive &&
             quantity->canonical_value <= 0.0)) {
            return {
                false,
                semantic ==
                        CadDynamicInputFieldSemantic::width
                    ? "Rectangle Width expects a positive Length expression."
                    : "Rectangle Height expects a positive Length expression."};
        }

        if (!target_->
                lockCadInputSemanticPairField(
                    semantic,
                    quantity->canonical_value)) {
            return {
                false,
                "Active Dynamic Input pair field could not be locked."};
        }
        return {true, {}};
    }

    const auto value_request =
        target_->cadInputSemanticValueRequest();
    if (value_request && fields.size() == 1U) {
        const auto quantity =
            parseCadQuantity(
                submitted,
                {
                    value_request->dimension,
                    number_format_.length_unit});
        if (!quantity ||
            (value_request->strictly_positive &&
             quantity->canonical_value <= 0.0)) {
            switch (value_request->semantic) {
            case CadInputValueRequestSemantic::circle_size:
                return {
                    false,
                    "Circle Size expects a positive Length expression."};
            case CadInputValueRequestSemantic::arc_radius:
                return {
                    false,
                    "Arc Radius expects a positive Length expression."};
            case CadInputValueRequestSemantic::rotate_angle:
                return {
                    false,
                    "Rotate Angle expects a valid Angle expression."};
            case CadInputValueRequestSemantic::scale_factor:
                return {
                    false,
                    "Scale Factor expects a positive Scalar expression."};
            case CadInputValueRequestSemantic::mirror_axis_angle:
                return {
                    false,
                    "Mirror Axis Angle expects a valid Angle expression."};
            }
        }

        if (!target_->lockCadInputSemanticValue(
                quantity->canonical_value)) {
            return {
                false,
                "Active Dynamic Input value could not be locked."};
        }
        return {true, {}};
    }

    if (!target_->cadInputSemanticPointRequest()) {
        return {
            false,
            "Active Dynamic Input field does not support scalar locking yet."};
    }

    CadQuantityDimension dimension{
        CadQuantityDimension::length};
    bool non_negative = false;
    switch (fields[index].semantic) {
    case CadDynamicInputFieldSemantic::u:
    case CadDynamicInputFieldSemantic::v:
    case CadDynamicInputFieldSemantic::delta_u:
    case CadDynamicInputFieldSemantic::delta_v:
        break;

    case CadDynamicInputFieldSemantic::distance:
        non_negative = true;
        break;

    case CadDynamicInputFieldSemantic::angle:
        dimension = CadQuantityDimension::angle;
        break;

    case CadDynamicInputFieldSemantic::width:
    case CadDynamicInputFieldSemantic::height:
    case CadDynamicInputFieldSemantic::diameter:
    case CadDynamicInputFieldSemantic::radius:
    case CadDynamicInputFieldSemantic::factor:
    case CadDynamicInputFieldSemantic::axis_angle:
        return {
            false,
            "Active Dynamic Input field does not support point locking."};
    }

    const auto quantity =
        parseCadQuantity(
            submitted,
            {
                dimension,
                number_format_.length_unit});
    if (!quantity ||
        (non_negative &&
         quantity->canonical_value < 0.0)) {
        return {
            false,
            fields[index].semantic ==
                    CadDynamicInputFieldSemantic::distance
                ? "Distance expects a non-negative Length expression."
                : fields[index].semantic ==
                          CadDynamicInputFieldSemantic::angle
                    ? "Angle expects a valid Angle expression."
                    : "Point component expects a valid Length expression."};
    }

    if (!target_->lockCadInputSemanticPointField(
            fields[index].semantic,
            quantity->canonical_value)) {
        return {
            false,
            "Active Dynamic Input point field could not be locked."};
    }
    return {true, {}};
}

CadInputSubmitResult
SketchCadInputSemanticEndpoint::submit(
    std::string_view text) {
    const auto submitted = trimAscii(text);
    if (submitted.empty()) {
        return {false, "CAD input is empty."};
    }
    if (target_ == nullptr ||
        !target_->cadInputSemanticActive()) {
        return {false, "No active CAD command context."};
    }

    const auto upper = upperAscii(submitted);
    const auto pair_request =
        target_->cadInputSemanticPairRequest();
    const auto point_request =
        target_->cadInputSemanticPointRequest();
    const auto value_request =
        target_->cadInputSemanticValueRequest();

    // A pair request owns its grammar completely. Rectangle Width;Height
    // must not fall through to the generic U;V point grammar.
    if (pair_request) {
        const auto values =
            parseSemanticPair(
                submitted,
                *pair_request,
                number_format_.length_unit);
        if (!values) {
            return {
                false,
                pair_request->semantic ==
                        CadInputPairRequestSemantic::
                            rectangle_size
                    ? "Rectangle size expects positive Width;Height Length expressions."
                    : "Active pair input is invalid."};
        }
        if (!target_->submitCadInputSemanticPair(
                values->first,
                values->second)) {
            return {
                false,
                "Active pair input could not be resolved."};
        }
        return {true, {}};
    }

    if (point_request) {
        if (upper == "C" &&
            target_->
                cadInputSemanticGripCopyAvailable()) {
            if (!target_->
                    submitCadInputSemanticGripCopy()) {
                return {
                    false,
                    "Grip Copy could not be enabled."};
            }
            return {true, {}};
        }

        // Complete point input outranks a numeric value request. This is
        // required by Arc's third-stage Arc Point / Radius grammar.
        if (const auto point =
                parseCadPointToken(
                    submitted,
                    number_format_.length_unit)) {
            const auto input =
                explicitPointInput(*point);
            if (!input ||
                !target_->
                    submitCadInputSemanticExplicitPoint(
                        *input)) {
                return {
                    false,
                    "Explicit point input is not available for the active point stage."};
            }
            return {true, {}};
        }
    }

    if (value_request) {
        if (value_request->semantic ==
            CadInputValueRequestSemantic::circle_size) {
            if (upper == "D" || upper == "R") {
                const auto mode =
                    upper == "D"
                        ? CircleSizeInputMode::diameter
                        : CircleSizeInputMode::radius;
                if (!target_->
                        submitCadInputSemanticCircleSizeMode(
                            mode)) {
                    return {
                        false,
                        "Circle Size mode could not be changed."};
                }
                return {true, {}};
            }
        }

        const auto quantity =
            parseCadQuantity(
                submitted,
                {
                    value_request->dimension,
                    number_format_.length_unit});
        if (!quantity ||
            (value_request->strictly_positive &&
             quantity->canonical_value <= 0.0)) {
            switch (value_request->semantic) {
            case CadInputValueRequestSemantic::circle_size:
                return {
                    false,
                    "Circle Size expects a positive Length expression."};
            case CadInputValueRequestSemantic::arc_radius:
                return {
                    false,
                    "Arc Radius expects a positive Length expression."};
            case CadInputValueRequestSemantic::rotate_angle:
                return {
                    false,
                    "Rotate Angle expects a valid Angle expression."};
            case CadInputValueRequestSemantic::scale_factor:
                return {
                    false,
                    "Scale Factor expects a positive Scalar expression."};
            case CadInputValueRequestSemantic::mirror_axis_angle:
                return {
                    false,
                    "Mirror Axis Angle expects a valid Angle expression."};
            }
        }

        if (!target_->submitCadInputSemanticValue(
                quantity->canonical_value)) {
            return {
                false,
                "Active numeric input could not be resolved."};
        }
        return {true, {}};
    }

    if (point_request) {
        const auto distance =
            parseBareCadDistance(submitted, number_format_);
        if (!distance) {
            return {
                false,
                "Active point input expects supported point coordinates or a valid non-negative Length expression."};
        }
        if (!point_request->direct_distance_enabled) {
            return {
                false,
                "Direct Distance is not available at this point stage."};
        }
        if (!target_->submitCadInputSemanticDirectDistance(*distance)) {
            return {
                false,
                "Direct Distance could not be resolved."};
        }
        return {true, {}};
    }

    if (upper == "BETWEEN" &&
        target_->
            cadInputSemanticMeasureBetweenAvailable()) {
        if (!target_->
                submitCadInputSemanticMeasureBetween()) {
            return {
                false,
                "Measure Between could not be activated."};
        }
        return {true, {}};
    }

    if (const auto profile =
            profileCommand(upper)) {
        return target_->
            submitCadInputSemanticProfileCommand(
                *profile);
    }

    const auto command = commandTool(upper);
    if (!command) {
        return {false, "Unknown Sketch command."};
    }
    if (!target_->activateCadInputSemanticTool(*command)) {
        return {false, "CAD command could not be activated."};
    }
    return {true, {}};
}

std::optional<double>
parseBareCadDistance(
    std::string_view text,
    const CadInputNumberFormat& number_format) {
    const auto quantity = parseCadQuantity(
        text,
        {CadQuantityDimension::length,
         number_format.length_unit});
    if (!quantity ||
        quantity->canonical_value < 0.0) {
        return std::nullopt;
    }
    return quantity->canonical_value;
}

} // namespace simplesolid2::application
