#pragma once

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::application {

struct CadInputNumberFormat final {
    // Retained for source compatibility with pre-R10 adapters.
    // R10 precision grammar accepts both '.' and ',' regardless of locale.
    std::string decimal_separator{"."};
    core::LengthUnit length_unit{
        core::LengthUnit::millimetre};
};

enum class ProfileCadInputCommandKind {
    start_create,
    start_edit,
    add_area,
    subtract_area,
    find_all_regions,
    finish,
    cancel,
    set_detect_islands,
    set_show_boundaries,
    set_show_problems,
};

struct ProfileCadInputCommand final {
    ProfileCadInputCommandKind kind{
        ProfileCadInputCommandKind::start_create};
    std::optional<bool> enabled;
};

class ISketchCadInputSemanticTarget {
public:
    virtual ~ISketchCadInputSemanticTarget() = default;

    [[nodiscard]] virtual bool
    cadInputSemanticActive() const noexcept = 0;

    [[nodiscard]] virtual std::optional<sketch::PointRequest>
    cadInputSemanticPointRequest() const noexcept = 0;

    [[nodiscard]] virtual bool activateCadInputSemanticTool(
        sketch::SketchTool tool) = 0;

    [[nodiscard]] virtual bool
    submitCadInputSemanticExplicitPoint(
        sketch::ExplicitPointInput input) = 0;

    [[nodiscard]] virtual bool submitCadInputSemanticDirectDistance(
        double distance) = 0;

    [[nodiscard]] virtual bool
    cadInputSemanticGripCopyAvailable() const noexcept = 0;

    [[nodiscard]] virtual bool
    submitCadInputSemanticGripCopy() = 0;

    [[nodiscard]] virtual bool
    cadInputSemanticMeasureBetweenAvailable()
        const noexcept {
        return false;
    }

    [[nodiscard]] virtual bool
    submitCadInputSemanticMeasureBetween() {
        return false;
    }

    [[nodiscard]] virtual CadInputSubmitResult
    submitCadInputSemanticProfileCommand(
        const ProfileCadInputCommand& command) = 0;
};

class SketchCadInputSemanticEndpoint final {
public:
    SketchCadInputSemanticEndpoint(
        ISketchCadInputSemanticTarget& target,
        CadInputNumberFormat number_format);

    [[nodiscard]] CadInputSubmitResult submit(
        std::string_view text);

private:
    ISketchCadInputSemanticTarget* target_{};
    CadInputNumberFormat number_format_;
};

[[nodiscard]] std::optional<double>
parseBareCadDistance(
    std::string_view text,
    const CadInputNumberFormat& number_format);

} // namespace simplesolid2::application
