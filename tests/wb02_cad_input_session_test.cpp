#include <simplesolid2/application/cad_input.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <string>
#include <string_view>
#include <vector>

namespace {

#define CHECK(expr) \
    do { \
        if (!(expr)) { \
            std::cerr << "CHECK failed: " #expr \
                      << " at line " << __LINE__ << "\n"; \
            return EXIT_FAILURE; \
        } \
    } while (false)

class FakeEndpoint final
    : public simplesolid2::application::ICadInputEndpoint {
public:
    std::string prompt{"Command: FAKE"};
    std::string last;
    bool accept{true};
    bool expose_dynamic_fields{};
    bool accept_dynamic_lock{true};
    std::size_t locked_field{};
    std::string locked_token;
    simplesolid2::application::CadInputContextGeneration
        generation{1U};

    [[nodiscard]]
    simplesolid2::application::CadInputContextGeneration
    cadInputContextGeneration() const noexcept override {
        return generation;
    }

    [[nodiscard]] std::string
    cadInputPrompt() const override {
        return prompt;
    }

    [[nodiscard]]
    std::vector<
        simplesolid2::application::CadDynamicInputField>
    cadDynamicInputFields() const override {
        if (!expose_dynamic_fields) {
            return {};
        }
        using Semantic =
            simplesolid2::application::
                CadDynamicInputFieldSemantic;
        return {
            {Semantic::distance, "Distance"},
            {Semantic::angle, "Angle"},
        };
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    lockCadDynamicInputField(
        std::size_t index,
        std::string_view text,
        simplesolid2::application::CadInputContextGeneration
            expected_context_generation) override {
        if (expected_context_generation != generation) {
            return {false, "Stale fake DYN context."};
        }
        locked_field = index;
        locked_token.assign(text);
        return {
            accept_dynamic_lock,
            accept_dynamic_lock
                ? std::string{}
                : std::string{"Rejected fake DYN lock."}};
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    submitCadInput(
        std::string_view text,
        simplesolid2::application::CadInputContextGeneration
            expected_context_generation) override {
        if (expected_context_generation != generation) {
            return {false, "Stale fake context."};
        }
        last.assign(text);
        return {
            accept,
            accept ? std::string{} :
                     std::string{"Rejected."}};
    }
};

} // namespace

int main() {
    using simplesolid2::application::CadInputSession;

    CadInputSession session;
    FakeEndpoint first;
    FakeEndpoint second;

    const auto defaults =
        session.interactionSettings();
    CHECK(defaults.polar.enabled);
    CHECK(
        defaults.polar.reference_mode ==
        simplesolid2::application::
            PolarReferenceMode::absolute);
    CHECK(
        std::abs(
            defaults.polar.primary_spacing -
            std::numbers::pi_v<double> / 4.0) <
        1.0e-12);
    CHECK(defaults.polar.additional_angles.empty());
    CHECK(!defaults.dynamic_input_enabled);

    auto runtime_settings = defaults;
    runtime_settings.polar.primary_spacing =
        std::numbers::pi_v<double> / 6.0;
    runtime_settings.polar.reference_mode =
        simplesolid2::application::
            PolarReferenceMode::relative;
    runtime_settings.polar.additional_angles = {
        17.0 * std::numbers::pi_v<double> / 180.0,
        std::numbers::pi_v<double> / 6.0};
    CHECK(
        session.setInteractionSettings(
            runtime_settings));

    auto invalid_settings = runtime_settings;
    invalid_settings.polar.primary_spacing = 0.0;
    CHECK(
        !session.setInteractionSettings(
            invalid_settings));
    CHECK(
        session.interactionSettings() ==
        runtime_settings);

    const auto relative_tracks =
        simplesolid2::application::
            generatePolarTrackAngles(
                runtime_settings.polar,
                12.0 *
                    std::numbers::pi_v<double> /
                    180.0);
    CHECK(!relative_tracks.empty());

    const auto no_reference_tracks =
        simplesolid2::application::
            generatePolarTrackAngles(
                runtime_settings.polar);
    CHECK(no_reference_tracks.empty());

    simplesolid2::application::PolarCaptureState
        capture;
    std::vector<
        simplesolid2::application::
            PolarTrackScreenDistance>
        candidates{
            {
                std::numbers::pi_v<double> / 4.0,
                8.0},
            {
                std::numbers::pi_v<double> / 2.0,
                6.0}};

    auto captured =
        simplesolid2::application::
            resolvePolarCapture(
                capture,
                true,
                20.0,
                candidates);
    CHECK(captured.has_value());
    CHECK(
        std::abs(
            *captured -
            std::numbers::pi_v<double> / 2.0) <
        1.0e-12);

    candidates = {
        {
            std::numbers::pi_v<double> / 4.0,
            2.0},
        {
            std::numbers::pi_v<double> / 2.0,
            14.0}};
    captured =
        simplesolid2::application::
            resolvePolarCapture(
                capture,
                true,
                20.0,
                candidates);
    CHECK(captured.has_value());
    CHECK(
        std::abs(
            *captured -
            std::numbers::pi_v<double> / 2.0) <
        1.0e-12);

    candidates[1].distance = 16.0;
    captured =
        simplesolid2::application::
            resolvePolarCapture(
                capture,
                true,
                20.0,
                candidates);
    CHECK(!captured.has_value());
    CHECK(!capture.captured_angle.has_value());

    captured =
        simplesolid2::application::
            resolvePolarCapture(
                capture,
                true,
                20.0,
                candidates);
    CHECK(captured.has_value());
    CHECK(
        std::abs(
            *captured -
            std::numbers::pi_v<double> / 4.0) <
        1.0e-12);

    captured =
        simplesolid2::application::
            resolvePolarCapture(
                capture,
                true,
                10.0,
                candidates);
    CHECK(!captured.has_value());
    CHECK(!capture.captured_angle.has_value());

    CHECK(!session.hasEndpoint());
    CHECK(session.prompt() == "Command:");

    session.attachEndpoint(&first);
    CHECK(session.hasEndpoint());
    CHECK(session.endpoint() == &first);
    CHECK(session.prompt() == "Command: FAKE");
    CHECK(session.dynamicInputFields().empty());

    auto dyn_settings =
        session.interactionSettings();
    dyn_settings.dynamic_input_enabled = true;
    CHECK(session.setInteractionSettings(
        dyn_settings));
    first.expose_dynamic_fields = true;
    CHECK(session.dynamicInputFields().size() == 2U);
    CHECK(session.dynamicInputFieldIndex() == 0U);
    CHECK(
        session.currentDynamicInputField()->
            label == "Distance");

    CHECK(session.cycleDynamicInputField());
    CHECK(session.dynamicInputFieldIndex() == 1U);
    CHECK(
        session.currentDynamicInputField()->
            label == "Angle");
    CHECK(session.cycleDynamicInputField(true));
    CHECK(session.dynamicInputFieldIndex() == 0U);

    session.setBuffer("100");
    CHECK(session.cycleDynamicInputField());
    CHECK(first.locked_field == 0U);
    CHECK(first.locked_token == "100");
    CHECK(session.buffer().empty());
    CHECK(session.dynamicInputFieldIndex() == 1U);

    first.accept_dynamic_lock = false;
    session.setBuffer("BADLOCK");
    CHECK(session.cycleDynamicInputField());
    CHECK(first.locked_field == 1U);
    CHECK(first.locked_token == "BADLOCK");
    CHECK(session.buffer() == "BADLOCK");
    CHECK(session.dynamicInputFieldIndex() == 1U);
    CHECK(
        session.diagnostic() ==
        "Rejected fake DYN lock.");
    session.clearBuffer();
    first.accept_dynamic_lock = true;

    ++first.generation;
    CHECK(session.synchronizeContext());
    CHECK(session.dynamicInputFieldIndex() == 0U);

    session.appendText("MO");
    session.appendText("VE");
    CHECK(session.buffer() == "MOVE");
    CHECK(session.backspace());
    CHECK(session.buffer() == "MOV");
    session.appendText("E");

    auto accepted = session.submit();
    CHECK(accepted.accepted);
    CHECK(first.last == "MOVE");
    CHECK(session.buffer().empty());
    CHECK(session.diagnostic().empty());

    session.setBuffer("STALE");
    ++first.generation;
    auto stale = session.submit();
    CHECK(!stale.accepted);
    CHECK(first.last == "MOVE");
    CHECK(session.buffer().empty());
    CHECK(
        session.diagnostic() ==
        "CAD input context changed before submission.");

    first.accept = false;
    session.setBuffer("BAD");
    auto rejected = session.submit();
    CHECK(!rejected.accepted);
    CHECK(first.last == "BAD");
    CHECK(session.buffer().empty());
    CHECK(session.diagnostic() == "Rejected.");

    session.attachEndpoint(&second);
    CHECK(session.endpoint() == &second);
    CHECK(session.buffer().empty());
    CHECK(session.diagnostic().empty());
    CHECK(
        session.interactionSettings() ==
        runtime_settings);

    session.setBuffer("50");
    session.detachEndpoint();
    CHECK(!session.hasEndpoint());
    CHECK(session.buffer().empty());
    CHECK(session.prompt() == "Command:");
    CHECK(
        session.interactionSettings() ==
        runtime_settings);

    session.setBuffer("ORPHAN");
    auto no_context = session.submit();
    CHECK(!no_context.accepted);
    CHECK(session.buffer().empty());
    CHECK(!session.diagnostic().empty());

    return EXIT_SUCCESS;
}
