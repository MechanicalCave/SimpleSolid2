#pragma once

#include <cstddef>
#include <cstdint>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace simplesolid2::application {

using CadInputContextGeneration = std::uint64_t;

struct CadInputSubmitResult final {
    bool accepted{};
    std::string diagnostic;
};

enum class PolarReferenceMode : std::uint8_t {
    absolute,
    relative,
};

struct PolarInputSettings final {
    bool enabled{true};
    double primary_spacing{
        std::numbers::pi_v<double> / 4.0};
    PolarReferenceMode reference_mode{
        PolarReferenceMode::absolute};
    std::vector<double> additional_angles;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PolarInputSettings&,
        const PolarInputSettings&) = default;
};

struct CadInteractionSettings final {
    PolarInputSettings polar;
    bool dynamic_input_enabled{};

    [[nodiscard]] bool valid() const noexcept {
        return polar.valid();
    }

    friend bool operator==(
        const CadInteractionSettings&,
        const CadInteractionSettings&) = default;
};

struct PolarTrackScreenDistance final {
    double angle{};
    double distance{};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PolarTrackScreenDistance&,
        const PolarTrackScreenDistance&) = default;
};

struct PolarCaptureState final {
    std::optional<double> captured_angle;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PolarCaptureState&,
        const PolarCaptureState&) = default;
};

inline constexpr double polar_capture_distance = 9.0;
inline constexpr double polar_release_distance = 15.0;
inline constexpr double polar_base_dead_zone = 12.0;

[[nodiscard]] std::vector<double>
generatePolarTrackAngles(
    const PolarInputSettings& settings,
    std::optional<double> relative_reference =
        std::nullopt);

[[nodiscard]] std::optional<double>
resolvePolarCapture(
    PolarCaptureState& state,
    bool enabled,
    double base_screen_distance,
    const std::vector<PolarTrackScreenDistance>&
        track_distances) noexcept;

class ICadInputEndpoint {
public:
    virtual ~ICadInputEndpoint() = default;

    [[nodiscard]] virtual std::string
    cadInputPrompt() const = 0;

    [[nodiscard]] virtual CadInputContextGeneration
    cadInputContextGeneration() const noexcept = 0;

    [[nodiscard]] virtual CadInputSubmitResult
    submitCadInput(
        std::string_view text,
        CadInputContextGeneration expected_context_generation) = 0;
};

class CadInputSession final {
public:
    void attachEndpoint(ICadInputEndpoint* endpoint);
    void detachEndpoint() noexcept;

    [[nodiscard]] bool hasEndpoint() const noexcept;
    [[nodiscard]] ICadInputEndpoint* endpoint() const noexcept;

    [[nodiscard]] bool synchronizeContext();

    void setBuffer(std::string text);
    void appendText(std::string_view text);
    [[nodiscard]] bool backspace();
    void clearBuffer() noexcept;

    [[nodiscard]] const std::string& buffer() const noexcept;
    [[nodiscard]] std::string prompt() const;
    [[nodiscard]] const std::string& diagnostic() const noexcept;

    [[nodiscard]] const CadInteractionSettings&
    interactionSettings() const noexcept {
        return interaction_settings_;
    }

    [[nodiscard]] bool setInteractionSettings(
        CadInteractionSettings settings) noexcept;

    [[nodiscard]] CadInputSubmitResult submit();

private:
    ICadInputEndpoint* endpoint_{};
    std::string buffer_;
    std::string diagnostic_;
    std::size_t endpoint_generation_{};
    CadInputContextGeneration observed_context_generation_{};
    std::optional<CadInputContextGeneration>
        buffer_context_generation_;
    CadInteractionSettings interaction_settings_;
};

} // namespace simplesolid2::application
