#include <simplesolid2/application/cad_input_semantics.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr << "D CAD input semantics CHECK failed at line "
                  << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)
bool near(double a, double b) { return std::abs(a - b) < 1.0e-12; }

class Target final : public application::ISketchCadInputSemanticTarget {
public:
    bool active{true};
    bool activation_result{true};
    bool direct_result{true};
    bool grip_copy_available{};
    bool grip_copy_result{true};
    unsigned grip_copy_submit_count{};
    bool measure_between_available{};
    bool measure_between_result{true};
    unsigned measure_between_submit_count{};
    std::optional<sketch::PointRequest> request;
    std::optional<sketch::SketchTool> activated;
    std::optional<double> submitted_distance;
    std::optional<sketch::ExplicitPointInput>
        submitted_point;
    bool explicit_point_result{true};
    std::optional<application::ProfileCadInputCommand>
        profile_command;
    application::CadInputSubmitResult
        profile_result{true, {}};

    bool cadInputSemanticActive() const noexcept override { return active; }
    std::optional<sketch::PointRequest>
    cadInputSemanticPointRequest() const noexcept override { return request; }
    bool activateCadInputSemanticTool(sketch::SketchTool tool) override {
        activated = tool;
        return activation_result;
    }
    bool submitCadInputSemanticExplicitPoint(
        sketch::ExplicitPointInput input) override {
        submitted_point = input;
        return explicit_point_result;
    }
    bool submitCadInputSemanticDirectDistance(double distance) override {
        submitted_distance = distance;
        return direct_result;
    }
    bool cadInputSemanticGripCopyAvailable()
        const noexcept override {
        return grip_copy_available;
    }
    bool submitCadInputSemanticGripCopy() override {
        ++grip_copy_submit_count;
        return grip_copy_result;
    }
    bool cadInputSemanticMeasureBetweenAvailable()
        const noexcept override {
        return measure_between_available;
    }
    bool submitCadInputSemanticMeasureBetween() override {
        ++measure_between_submit_count;
        return measure_between_result;
    }

    application::CadInputSubmitResult
    submitCadInputSemanticProfileCommand(
        const application::ProfileCadInputCommand& command) override {
        profile_command = command;
        return profile_result;
    }
};
} // namespace

int main() {
    Target target;
    application::SketchCadInputSemanticEndpoint dot{
        target, application::CadInputNumberFormat{"."}};

    auto result = dot.submit("  line  ");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::line);

    target.activated.reset();
    result = dot.submit(" rectangle ");
    CHECK(result.accepted);
    CHECK(
        target.activated ==
        sketch::SketchTool::rectangle);

    target.activated.reset();
    result = dot.submit("measure");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::measure);

    // BETWEEN is tool-local; clear the previous top-level activation
    // observation before asserting that BETWEEN does not activate a new tool.
    target.activated.reset();
    target.measure_between_available = true;
    result = dot.submit(" BeTwEeN ");
    CHECK(result.accepted);
    CHECK(target.measure_between_submit_count == 1U);
    CHECK(!target.activated.has_value());

    target.measure_between_result = false;
    result = dot.submit("BETWEEN");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Measure Between could not be activated.");
    CHECK(target.measure_between_submit_count == 2U);
    target.measure_between_result = true;
    target.measure_between_available = false;
    result = dot.submit("BETWEEN");
    CHECK(!result.accepted);
    CHECK(result.diagnostic == "Unknown Sketch command.");

    target.activated.reset();
    result = dot.submit("MOVE");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::move);

    target.activated.reset();
    result = dot.submit("copy");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::copy);

    target.profile_command.reset();
    result = dot.submit("PROFILE");
    CHECK(result.accepted);
    CHECK(target.profile_command.has_value());
    CHECK(
        target.profile_command->kind ==
        application::ProfileCadInputCommandKind::
            start_create);

    target.profile_command.reset();
    result = dot.submit(" subtract ");
    CHECK(result.accepted);
    CHECK(target.profile_command.has_value());
    CHECK(
        target.profile_command->kind ==
        application::ProfileCadInputCommandKind::
            subtract_area);

    target.profile_command.reset();
    result = dot.submit("ISLANDS OFF");
    CHECK(result.accepted);
    CHECK(target.profile_command.has_value());
    CHECK(
        target.profile_command->kind ==
        application::ProfileCadInputCommandKind::
            set_detect_islands);
    CHECK(target.profile_command->enabled == false);

    target.profile_command.reset();
    result = dot.submit("BOUNDARIES ON");
    CHECK(result.accepted);
    CHECK(target.profile_command.has_value());
    CHECK(
        target.profile_command->kind ==
        application::ProfileCadInputCommandKind::
            set_show_boundaries);
    CHECK(target.profile_command->enabled == true);

    target.profile_command.reset();
    result = dot.submit("EDITPROFILE");
    CHECK(result.accepted);
    CHECK(target.profile_command.has_value());
    CHECK(
        target.profile_command->kind ==
        application::ProfileCadInputCommandKind::
            start_edit);

    target.activated.reset();
    result = dot.submit("nonesuch");
    CHECK(!result.accepted);
    CHECK(result.diagnostic == "Unknown Sketch command.");
    CHECK(!target.activated.has_value());

    target.request = sketch::PointRequest{
        sketch::Point2{0.0, 0.0},
        sketch::Point2{3.0, 4.0},
        true,
        true,
        true,
        true};
    target.submitted_distance.reset();
    result = dot.submit("C");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Active point input expects supported point coordinates or a valid non-negative Length expression.");
    CHECK(target.grip_copy_submit_count == 0U);

    target.grip_copy_available = true;
    result = dot.submit(" c ");
    CHECK(result.accepted);
    CHECK(target.grip_copy_submit_count == 1U);
    CHECK(!target.submitted_distance.has_value());

    target.grip_copy_result = false;
    result = dot.submit("C");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Grip Copy could not be enabled.");
    CHECK(target.grip_copy_submit_count == 2U);
    target.grip_copy_result = true;

    target.submitted_point.reset();
    result = dot.submit("25;10");
    CHECK(result.accepted);
    CHECK(target.submitted_point.has_value());
    CHECK(
        target.submitted_point->kind ==
        sketch::ExplicitPointInputKind::
            absolute_cartesian);
    CHECK(near(target.submitted_point->first, 25.0));
    CHECK(near(target.submitted_point->second, 10.0));

    target.submitted_point.reset();
    result = dot.submit("@2in;10mm");
    CHECK(result.accepted);
    CHECK(target.submitted_point.has_value());
    CHECK(
        target.submitted_point->kind ==
        sketch::ExplicitPointInputKind::
            relative_cartesian);
    CHECK(near(target.submitted_point->first, 50.8));
    CHECK(near(target.submitted_point->second, 10.0));

    target.submitted_point.reset();
    result = dot.submit("@50<90");
    CHECK(result.accepted);
    CHECK(target.submitted_point.has_value());
    CHECK(
        target.submitted_point->kind ==
        sketch::ExplicitPointInputKind::
            relative_polar);
    CHECK(near(target.submitted_point->first, 50.0));
    CHECK(near(
        target.submitted_point->second,
        std::numbers::pi_v<double> / 2.0));

    target.explicit_point_result = false;
    result = dot.submit("1;2");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Explicit point input is not available for the active point stage.");
    target.explicit_point_result = true;

    result = dot.submit("12.5");
    CHECK(result.accepted);
    CHECK(target.submitted_distance.has_value());
    CHECK(near(*target.submitted_distance, 12.5));

    target.submitted_distance.reset();
    result = dot.submit("12,5");
    CHECK(result.accepted);
    CHECK(target.submitted_distance.has_value());
    CHECK(near(*target.submitted_distance, 12.5));

    target.submitted_distance.reset();
    result = dot.submit("1e3");
    CHECK(result.accepted);
    CHECK(target.submitted_distance.has_value());
    CHECK(near(*target.submitted_distance, 1000.0));

    target.submitted_distance.reset();
    result = dot.submit("25mm + 1in");
    CHECK(result.accepted);
    CHECK(target.submitted_distance.has_value());
    CHECK(near(*target.submitted_distance, 50.4));

    target.activated.reset();
    target.submitted_distance.reset();
    target.profile_command.reset();
    result = dot.submit("PROFILE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Active point input expects supported point coordinates or a valid non-negative Length expression.");
    CHECK(!target.profile_command.has_value());

    result = dot.submit("LINE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Active point input expects supported point coordinates or a valid non-negative Length expression.");
    CHECK(!target.activated.has_value());
    CHECK(!target.submitted_distance.has_value());

    result = dot.submit("RECTANGLE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Active point input expects supported point coordinates or a valid non-negative Length expression.");
    CHECK(!target.activated.has_value());

    application::SketchCadInputSemanticEndpoint comma{
        target, application::CadInputNumberFormat{","}};
    result = comma.submit("12,5");
    CHECK(result.accepted);
    CHECK(target.submitted_distance.has_value());
    CHECK(near(*target.submitted_distance, 12.5));

    target.submitted_distance.reset();
    result = comma.submit("12.5,1");
    CHECK(!result.accepted);
    CHECK(!target.submitted_distance.has_value());

    target.request->direct_distance_enabled = false;
    result = dot.submit("10");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Direct Distance is not available at this point stage.");

    target.request->direct_distance_enabled = true;
    target.direct_result = false;
    result = dot.submit("10");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Direct Distance could not be resolved.");

    target.request.reset();
    target.grip_copy_available = false;
    result = dot.submit("C");
    CHECK(!result.accepted);
    CHECK(result.diagnostic == "Unknown Sketch command.");

    target.active = false;
    result = dot.submit("LINE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic == "No active CAD command context.");

    CHECK(application::parseBareCadDistance(
              ".5", application::CadInputNumberFormat{"."}).has_value());
    CHECK(!application::parseBareCadDistance(
               "-1", application::CadInputNumberFormat{"."}).has_value());
    const auto scientific = application::parseBareCadDistance(
        "1e3", application::CadInputNumberFormat{"."});
    CHECK(scientific.has_value());
    CHECK(near(*scientific, 1000.0));

    const auto explicit_inch = application::parseBareCadDistance(
        "1in", application::CadInputNumberFormat{"."});
    CHECK(explicit_inch.has_value());
    CHECK(near(*explicit_inch, 25.4));

    const auto unitless_inch = application::parseBareCadDistance(
        "2",
        application::CadInputNumberFormat{
            ".",
            core::LengthUnit::inch});
    CHECK(unitless_inch.has_value());
    CHECK(near(*unitless_inch, 50.8));

    CHECK(!application::parseBareCadDistance(
               "1mm * 2mm",
               application::CadInputNumberFormat{"."}).has_value());

    std::cout << "D CAD input semantics PASS\n";
    return EXIT_SUCCESS;
}
