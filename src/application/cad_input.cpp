#include <simplesolid2/application/cad_input.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace simplesolid2::application {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;
constexpr double polar_angle_epsilon = 1.0e-12;
constexpr std::size_t max_polar_tracks = 4096U;

double normalizeAngle(double angle) noexcept {
    double result = std::fmod(angle, full_turn);
    if (result < 0.0) {
        result += full_turn;
    }
    return result == full_turn ? 0.0 : result;
}

bool equivalentAngle(
    double left,
    double right) noexcept {
    const double delta =
        std::abs(normalizeAngle(left - right));
    const double wrapped =
        std::min(delta, full_turn - delta);
    return wrapped <= polar_angle_epsilon;
}

} // namespace

bool PolarInputSettings::valid() const noexcept {
    if (!std::isfinite(primary_spacing) ||
        primary_spacing <= 0.0 ||
        primary_spacing >
            std::numbers::pi_v<double>) {
        return false;
    }
    return std::all_of(
        additional_angles.begin(),
        additional_angles.end(),
        [](double angle) {
            return std::isfinite(angle);
        });
}

bool PolarTrackScreenDistance::valid() const noexcept {
    return std::isfinite(angle) &&
           std::isfinite(distance) &&
           distance >= 0.0;
}

bool PolarCaptureState::valid() const noexcept {
    return !captured_angle ||
           std::isfinite(*captured_angle);
}

std::vector<double> generatePolarTrackAngles(
    const PolarInputSettings& settings,
    std::optional<double> relative_reference) {
    if (!settings.valid() ||
        !settings.enabled) {
        return {};
    }

    double reference = 0.0;
    if (settings.reference_mode ==
        PolarReferenceMode::relative) {
        if (!relative_reference ||
            !std::isfinite(*relative_reference)) {
            return {};
        }
        reference = *relative_reference;
    }

    std::vector<double> tracks;
    tracks.reserve(std::min<std::size_t>(
        max_polar_tracks,
        static_cast<std::size_t>(
            std::ceil(
                full_turn /
                settings.primary_spacing)) +
            settings.additional_angles.size()));

    for (std::size_t index = 0U;
         index < max_polar_tracks;
         ++index) {
        const double offset =
            static_cast<double>(index) *
            settings.primary_spacing;
        if (!std::isfinite(offset) ||
            offset >= full_turn) {
            break;
        }
        tracks.push_back(
            normalizeAngle(reference + offset));
    }

    for (const double additional :
         settings.additional_angles) {
        if (tracks.size() >= max_polar_tracks) {
            break;
        }
        tracks.push_back(
            normalizeAngle(
                reference + additional));
    }

    std::sort(tracks.begin(), tracks.end());
    tracks.erase(
        std::unique(
            tracks.begin(),
            tracks.end(),
            [](double left, double right) {
                return equivalentAngle(left, right);
            }),
        tracks.end());

    if (tracks.size() > 1U &&
        equivalentAngle(
            tracks.front(),
            tracks.back())) {
        tracks.pop_back();
    }

    return tracks;
}

std::optional<double> resolvePolarCapture(
    PolarCaptureState& state,
    bool enabled,
    double base_screen_distance,
    const std::vector<PolarTrackScreenDistance>&
        track_distances) noexcept {
    if (!state.valid() ||
        !std::isfinite(base_screen_distance) ||
        base_screen_distance < 0.0) {
        state.captured_angle.reset();
        return std::nullopt;
    }

    if (!enabled ||
        base_screen_distance <=
            polar_base_dead_zone) {
        state.captured_angle.reset();
        return std::nullopt;
    }

    if (state.captured_angle) {
        const auto captured =
            std::find_if(
                track_distances.begin(),
                track_distances.end(),
                [&state](
                    const PolarTrackScreenDistance&
                        candidate) {
                    return candidate.valid() &&
                           equivalentAngle(
                               candidate.angle,
                               *state.captured_angle);
                });

        if (captured != track_distances.end() &&
            captured->distance <=
                polar_release_distance) {
            state.captured_angle =
                normalizeAngle(
                    *state.captured_angle);
            return state.captured_angle;
        }

        // Hysteresis rule: release first. A neighboring track may only
        // capture on a later pointer sample.
        state.captured_angle.reset();
        return std::nullopt;
    }

    const PolarTrackScreenDistance* best = nullptr;
    for (const auto& candidate :
         track_distances) {
        if (!candidate.valid() ||
            candidate.distance >
                polar_capture_distance) {
            continue;
        }
        if (best == nullptr ||
            candidate.distance < best->distance ||
            (candidate.distance == best->distance &&
             normalizeAngle(candidate.angle) <
                 normalizeAngle(best->angle))) {
            best = &candidate;
        }
    }

    if (best == nullptr) {
        return std::nullopt;
    }

    state.captured_angle =
        normalizeAngle(best->angle);
    return state.captured_angle;
}

void CadInputSession::attachEndpoint(
    ICadInputEndpoint* endpoint) {
    endpoint_ = endpoint;
    ++endpoint_generation_;
    observed_context_generation_ =
        endpoint_ != nullptr
            ? endpoint_->cadInputContextGeneration()
            : CadInputContextGeneration{};
    buffer_context_generation_.reset();
    buffer_.clear();
    diagnostic_.clear();
}

void CadInputSession::detachEndpoint() noexcept {
    endpoint_ = nullptr;
    ++endpoint_generation_;
    observed_context_generation_ = {};
    buffer_context_generation_.reset();
    buffer_.clear();
    diagnostic_.clear();
}

bool CadInputSession::hasEndpoint() const noexcept {
    return endpoint_ != nullptr;
}

ICadInputEndpoint*
CadInputSession::endpoint() const noexcept {
    return endpoint_;
}

// AUDIT-01 A1 authority boundary: semantic-context generation, not
// transport-adapter/endpoint object identity, owns the lifetime of a live token.
bool CadInputSession::synchronizeContext() {
    if (endpoint_ == nullptr) {
        return false;
    }

    const auto current =
        endpoint_->cadInputContextGeneration();
    if (current == observed_context_generation_) {
        return false;
    }

    observed_context_generation_ = current;
    buffer_context_generation_.reset();
    buffer_.clear();
    diagnostic_.clear();
    return true;
}

void CadInputSession::setBuffer(std::string text) {
    static_cast<void>(synchronizeContext());

    buffer_ = std::move(text);
    diagnostic_.clear();
    if (buffer_.empty()) {
        buffer_context_generation_.reset();
    } else {
        buffer_context_generation_ =
            observed_context_generation_;
    }
}

void CadInputSession::appendText(
    std::string_view text) {
    static_cast<void>(synchronizeContext());

    if (buffer_.empty() && !text.empty()) {
        buffer_context_generation_ =
            observed_context_generation_;
    }
    buffer_.append(text);
    diagnostic_.clear();
}

bool CadInputSession::backspace() {
    if (buffer_.empty()) {
        return false;
    }

    std::size_t start = buffer_.size() - 1U;
    while (start > 0U &&
           (static_cast<unsigned char>(buffer_[start]) & 0xC0U) ==
               0x80U) {
        --start;
    }
    buffer_.erase(start);

    if (buffer_.empty()) {
        buffer_context_generation_.reset();
    }

    diagnostic_.clear();
    return true;
}

void CadInputSession::clearBuffer() noexcept {
    buffer_context_generation_.reset();
    buffer_.clear();
    diagnostic_.clear();
}

const std::string&
CadInputSession::buffer() const noexcept {
    return buffer_;
}

std::string CadInputSession::prompt() const {
    if (endpoint_ == nullptr) {
        return "Command:";
    }

    auto value = endpoint_->cadInputPrompt();
    return value.empty()
               ? std::string{"Command:"}
               : value;
}

const std::string&
CadInputSession::diagnostic() const noexcept {
    return diagnostic_;
}

bool CadInputSession::setInteractionSettings(
    CadInteractionSettings settings) noexcept {
    if (!settings.valid()) {
        return false;
    }
    interaction_settings_ = std::move(settings);
    return true;
}

CadInputSubmitResult CadInputSession::submit() {
    if (buffer_.empty()) {
        CadInputSubmitResult result{
            false,
            "CAD input buffer is empty."};
        diagnostic_ = result.diagnostic;
        return result;
    }

    auto* const target = endpoint_;
    const auto endpoint_generation =
        endpoint_generation_;
    if (target == nullptr) {
        const std::string submitted = buffer_;
        static_cast<void>(submitted);
        clearBuffer();
        CadInputSubmitResult result{
            false,
            "No active CAD input context."};
        diagnostic_ = result.diagnostic;
        return result;
    }

    const auto current_context =
        target->cadInputContextGeneration();
    if (current_context != observed_context_generation_ ||
        !buffer_context_generation_ ||
        *buffer_context_generation_ != current_context) {
        observed_context_generation_ = current_context;
        clearBuffer();
        CadInputSubmitResult stale{
            false,
            "CAD input context changed before submission."};
        diagnostic_ = stale.diagnostic;
        return stale;
    }

    // Enter/Return consumes one submitted token regardless of whether
    // the active semantic context accepts or rejects it.
    const std::string submitted = buffer_;
    buffer_.clear();
    buffer_context_generation_.reset();

    // The endpoint receives the generation that owned the live token
    // and must validate it before interpreting the token or causing a
    // domain effect.
    auto result =
        target->submitCadInput(
            submitted,
            current_context);

    if (endpoint_ != target ||
        endpoint_generation_ != endpoint_generation) {
        CadInputSubmitResult stale{
            false,
            "CAD input endpoint changed during submission."};
        diagnostic_ = stale.diagnostic;
        return stale;
    }

    diagnostic_ = result.diagnostic;
    if (result.accepted) {
        diagnostic_.clear();
    }
    return result;
}

} // namespace simplesolid2::application
