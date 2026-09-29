#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/application/precision_input.hpp>

#include <cmath>
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
    if (token == "FIND") {
        return ProfileCadInputCommand{
            ProfileCadInputCommandKind::find_all_regions,
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
            ProfileCadInputCommandKind::set_detect_islands,
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
    return std::nullopt;
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

    if (const auto request =
            target_->cadInputSemanticPointRequest()) {
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

        const auto distance =
            parseBareCadDistance(submitted, number_format_);
        if (!distance) {
            return {
                false,
                "Active point input expects supported point coordinates or a valid non-negative Length expression."};
        }
        if (!request->direct_distance_enabled) {
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
