#pragma once

#include <simplesolid2/application/cad_input.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::application {

struct CadInputNumberFormat final {
    std::string decimal_separator{"."};
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

    [[nodiscard]] virtual bool submitCadInputSemanticDirectDistance(
        double distance) = 0;
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
