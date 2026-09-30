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
    std::optional<application::CadInputValueRequest>
        value_request;
    std::optional<double> submitted_value;
    std::optional<double> locked_value;
    bool value_result{true};
    bool lock_value_result{true};
    std::optional<application::CadInputPairRequest>
        pair_request;
    std::optional<std::pair<double, double>>
        submitted_pair;
    bool pair_result{true};
    std::optional<application::CircleSizeInputMode>
        circle_size_mode;
    application::CircleSizeInputMode
        current_circle_size_mode{
            application::CircleSizeInputMode::diameter};
    bool circle_size_mode_result{true};
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
    std::optional<application::CadInputValueRequest>
    cadInputSemanticValueRequest() const noexcept override {
        return value_request;
    }
    bool submitCadInputSemanticValue(double value) override {
        submitted_value = value;
        return value_result;
    }
    bool lockCadInputSemanticValue(double value) override {
        locked_value = value;
        return lock_value_result;
    }
    std::optional<application::CadInputPairRequest>
    cadInputSemanticPairRequest() const noexcept override {
        return pair_request;
    }
    bool submitCadInputSemanticPair(
        double first,
        double second) override {
        submitted_pair = std::pair{first, second};
        return pair_result;
    }
    bool submitCadInputSemanticCircleSizeMode(
        application::CircleSizeInputMode mode) override {
        circle_size_mode = mode;
        current_circle_size_mode = mode;
        return circle_size_mode_result;
    }
    application::CircleSizeInputMode
    cadInputSemanticCircleSizeMode() const noexcept override {
        return current_circle_size_mode;
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

    CHECK(dot.dynamicInputFields().empty());

    target.request = sketch::PointRequest{
        std::nullopt,
        sketch::Point2{1.0, 2.0},
        false,
        true,
        false,
        false};
    auto fields = dot.dynamicInputFields();
    CHECK(fields.size() == 2U);
    CHECK(
        fields[0].semantic ==
        application::CadDynamicInputFieldSemantic::u);
    CHECK(fields[0].label == "U");
    CHECK(
        fields[1].semantic ==
        application::CadDynamicInputFieldSemantic::v);
    CHECK(fields[1].label == "V");

    target.request = sketch::PointRequest{
        sketch::Point2{0.0, 0.0},
        sketch::Point2{3.0, 4.0},
        true,
        true,
        true,
        true};
    fields = dot.dynamicInputFields();
    CHECK(fields.size() == 4U);
    CHECK(fields[0].label == "Distance");
    CHECK(fields[1].label == "Angle");
    CHECK(fields[2].label == "dU");
    CHECK(fields[3].label == "dV");

    target.pair_request =
        application::CadInputPairRequest{
            application::CadInputPairRequestSemantic::
                rectangle_size,
            application::CadQuantityDimension::length,
            application::CadQuantityDimension::length,
            true};
    fields = dot.dynamicInputFields();
    CHECK(fields.size() == 2U);
    CHECK(fields[0].label == "Width");
    CHECK(fields[1].label == "Height");
    target.pair_request.reset();

    target.value_request =
        application::CadInputValueRequest{
            application::CadInputValueRequestSemantic::
                arc_radius,
            application::CadQuantityDimension::length,
            true};
    fields = dot.dynamicInputFields();
    CHECK(fields.size() == 1U);
    CHECK(fields[0].label == "Radius");

    target.value_request->semantic =
        application::CadInputValueRequestSemantic::
            circle_size;
    target.current_circle_size_mode =
        application::CircleSizeInputMode::diameter;
    fields = dot.dynamicInputFields();
    CHECK(fields.size() == 1U);
    CHECK(fields[0].label == "Diameter");
    target.current_circle_size_mode =
        application::CircleSizeInputMode::radius;
    fields = dot.dynamicInputFields();
    CHECK(fields.size() == 1U);
    CHECK(fields[0].label == "Radius");

    target.value_request->semantic =
        application::CadInputValueRequestSemantic::
            rotate_angle;
    fields = dot.dynamicInputFields();
    CHECK(fields[0].label == "Angle");
    target.value_request->semantic =
        application::CadInputValueRequestSemantic::
            scale_factor;
    fields = dot.dynamicInputFields();
    CHECK(fields[0].label == "Factor");
    target.value_request->semantic =
        application::CadInputValueRequestSemantic::
            mirror_axis_angle;
    fields = dot.dynamicInputFields();
    CHECK(fields[0].label == "Axis Angle");

    target.value_request->semantic =
        application::CadInputValueRequestSemantic::
            rotate_angle;
    target.value_request->dimension =
        application::CadQuantityDimension::angle;
    target.value_request->strictly_positive = false;
    target.locked_value.reset();
    auto lock_result =
        dot.lockDynamicInputField(
            0U,
            "30");
    CHECK(lock_result.accepted);
    CHECK(target.locked_value.has_value());
    CHECK(near(
        *target.locked_value,
        std::numbers::pi_v<double> / 6.0));

    target.value_request->semantic =
        application::CadInputValueRequestSemantic::
            scale_factor;
    target.value_request->dimension =
        application::CadQuantityDimension::scalar;
    target.value_request->strictly_positive = true;
    target.locked_value.reset();
    lock_result =
        dot.lockDynamicInputField(
            0U,
            "0");
    CHECK(!lock_result.accepted);
    CHECK(
        lock_result.diagnostic ==
        "Scale Factor expects a positive Scalar expression.");
    CHECK(!target.locked_value.has_value());

    target.lock_value_result = false;
    lock_result =
        dot.lockDynamicInputField(
            0U,
            "2");
    CHECK(!lock_result.accepted);
    CHECK(
        lock_result.diagnostic ==
        "Active Dynamic Input value could not be locked.");
    target.lock_value_result = true;

    target.value_request.reset();
    target.request.reset();

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

    // Context-specific pair grammar owns Width;Height and never falls
    // through to U;V point parsing or top-level command activation.
    target.request.reset();
    target.pair_request =
        application::CadInputPairRequest{
            application::CadInputPairRequestSemantic::
                rectangle_size,
            application::CadQuantityDimension::length,
            application::CadQuantityDimension::length,
            true};
    target.submitted_pair.reset();
    result = dot.submit("2;1in");
    CHECK(result.accepted);
    CHECK(target.submitted_pair.has_value());
    CHECK(near(target.submitted_pair->first, 2.0));
    CHECK(near(target.submitted_pair->second, 25.4));

    target.submitted_pair.reset();
    result = dot.submit("0;1");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Rectangle size expects positive Width;Height Length expressions.");
    CHECK(!target.submitted_pair.has_value());

    target.activated.reset();
    result = dot.submit("LINE");
    CHECK(!result.accepted);
    CHECK(!target.activated.has_value());
    target.pair_request.reset();

    // Circle Size owns D/R setters and positive Length input.
    target.value_request =
        application::CadInputValueRequest{
            application::CadInputValueRequestSemantic::
                circle_size,
            application::CadQuantityDimension::length,
            true};
    target.circle_size_mode.reset();
    result = dot.submit(" d ");
    CHECK(result.accepted);
    CHECK(
        target.circle_size_mode ==
        application::CircleSizeInputMode::diameter);
    result = dot.submit("R");
    CHECK(result.accepted);
    CHECK(
        target.circle_size_mode ==
        application::CircleSizeInputMode::radius);

    target.submitted_value.reset();
    result = dot.submit("2in");
    CHECK(result.accepted);
    CHECK(target.submitted_value.has_value());
    CHECK(near(*target.submitted_value, 50.8));

    target.submitted_value.reset();
    result = dot.submit("0");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Circle Size expects a positive Length expression.");
    CHECK(!target.submitted_value.has_value());
    target.value_request.reset();

    // Arc third stage may expose point and Radius simultaneously. A complete
    // point token wins; a scalar Length is routed to Radius, not Direct
    // Distance.
    target.request = sketch::PointRequest{
        std::nullopt,
        std::nullopt,
        false,
        true,
        false,
        false};
    target.value_request =
        application::CadInputValueRequest{
            application::CadInputValueRequestSemantic::
                arc_radius,
            application::CadQuantityDimension::length,
            true};
    target.submitted_point.reset();
    target.submitted_value.reset();
    target.submitted_distance.reset();

    result = dot.submit("25;10");
    CHECK(result.accepted);
    CHECK(target.submitted_point.has_value());
    CHECK(!target.submitted_value.has_value());
    CHECK(!target.submitted_distance.has_value());

    target.submitted_point.reset();
    result = dot.submit("50");
    CHECK(result.accepted);
    CHECK(target.submitted_value.has_value());
    CHECK(near(*target.submitted_value, 50.0));
    CHECK(!target.submitted_point.has_value());
    CHECK(!target.submitted_distance.has_value());

    target.activated.reset();
    result = dot.submit("LINE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Arc Radius expects a positive Length expression.");
    CHECK(!target.activated.has_value());

    target.value_request.reset();
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
