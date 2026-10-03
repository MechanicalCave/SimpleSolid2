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
    dynamic_input_field_index_ = 0U;
}

void CadInputSession::detachEndpoint() noexcept {
    endpoint_ = nullptr;
    ++endpoint_generation_;
    observed_context_generation_ = {};
    buffer_context_generation_.reset();
    buffer_.clear();
    diagnostic_.clear();
    dynamic_input_field_index_ = 0U;
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
    dynamic_input_field_index_ = 0U;
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

// R10 ownership invariant: CadInputSession owns the one live text buffer,
// application-session aid configuration and field focus only. Field meaning
// and request-local numeric locks remain owned by the attached semantic
// semantic endpoint/request owner; Dynamic Input never becomes a second CAD state path.
std::vector<CadDynamicInputField>
CadInputSession::dynamicInputFields() const {
    if (endpoint_ == nullptr ||
        !interaction_settings_.dynamic_input_enabled) {
        return {};
    }

    auto fields =
        endpoint_->cadDynamicInputFields();
    fields.erase(
        std::remove_if(
            fields.begin(),
            fields.end(),
            [](const CadDynamicInputField& field) {
                return !field.valid();
            }),
        fields.end());
    return fields;
}

std::optional<CadDynamicInputField>
CadInputSession::currentDynamicInputField() const {
    const auto fields = dynamicInputFields();
    if (fields.empty()) {
        return std::nullopt;
    }
    const auto index =
        dynamic_input_field_index_ %
        fields.size();
    return fields[index];
}

bool CadInputSession::cycleDynamicInputField(
    bool reverse) {
    static_cast<void>(synchronizeContext());

    if (endpoint_ == nullptr ||
        !interaction_settings_.dynamic_input_enabled) {
        return false;
    }

    const auto fields = dynamicInputFields();
    if (fields.empty()) {
        dynamic_input_field_index_ = 0U;
        return false;
    }

    dynamic_input_field_index_ %=
        fields.size();

    if (!buffer_.empty()) {
        if (!buffer_context_generation_ ||
            *buffer_context_generation_ !=
                observed_context_generation_) {
            clearBuffer();
            diagnostic_ =
                "CAD input context changed before Dynamic Input lock.";
            return true;
        }

        auto result =
            endpoint_->lockCadDynamicInputField(
                dynamic_input_field_index_,
                buffer_,
                observed_context_generation_);
        diagnostic_ = result.diagnostic;
        if (!result.accepted) {
            return true;
        }

        buffer_.clear();
        buffer_context_generation_.reset();
        diagnostic_.clear();

        // A successful request-local lock may legitimately advance the
        // semantic generation. Synchronize only after the endpoint has
        // consumed the token, preserving the one-buffer ownership rule.
        const auto current =
            endpoint_->cadInputContextGeneration();
        observed_context_generation_ = current;
    }

    const auto next_fields =
        dynamicInputFields();
    if (next_fields.empty()) {
        dynamic_input_field_index_ = 0U;
        return true;
    }

    if (reverse) {
        dynamic_input_field_index_ =
            dynamic_input_field_index_ == 0U
                ? next_fields.size() - 1U
                : dynamic_input_field_index_ - 1U;
    } else {
        dynamic_input_field_index_ =
            (dynamic_input_field_index_ + 1U) %
            next_fields.size();
    }
    return true;
}

bool CadInputSession::setInteractionSettings(
    CadInteractionSettings settings) noexcept {
    if (!settings.valid()) {
        return false;
    }
    const bool disabling_dynamic_input =
        interaction_settings_.dynamic_input_enabled &&
        !settings.dynamic_input_enabled;
    interaction_settings_ = std::move(settings);
    if (disabling_dynamic_input) {
        dynamic_input_field_index_ = 0U;
    }
    return true;
}

CadInputSubmitResult CadInputSession::submit() {
    if (buffer_.empty()) {
        static_cast<void>(synchronizeContext());

        auto* const dynamic_target = endpoint_;
        if (dynamic_target != nullptr &&
            interaction_settings_.dynamic_input_enabled &&
            !dynamicInputFields().empty()) {
            const auto endpoint_generation =
                endpoint_generation_;
            const auto current_context =
                dynamic_target->
                    cadInputContextGeneration();

            auto result =
                dynamic_target->
                    submitCadDynamicInputRequest(
                        current_context);

            if (endpoint_ != dynamic_target ||
                endpoint_generation_ !=
                    endpoint_generation) {
                CadInputSubmitResult stale{
                    false,
                    "CAD input endpoint changed during Dynamic Input submission."};
                diagnostic_ = stale.diagnostic;
                return stale;
            }

            diagnostic_ = result.diagnostic;
            if (result.accepted) {
                diagnostic_.clear();
                dynamic_input_field_index_ = 0U;
            }
            return result;
        }

        if (endpoint_ != nullptr &&
            endpoint_->acceptsEmptyCadInput()) {
            auto* const target = endpoint_;
            const auto endpoint_generation =
                endpoint_generation_;
            const auto current_context =
                target->cadInputContextGeneration();

            auto result =
                target->submitCadInput(
                    {},
                    current_context);

            if (endpoint_ != target ||
                endpoint_generation_ !=
                    endpoint_generation) {
                CadInputSubmitResult stale{
                    false,
                    "CAD input endpoint changed during empty submission."};
                diagnostic_ = stale.diagnostic;
                return stale;
            }

            observed_context_generation_ =
                endpoint_->cadInputContextGeneration();
            diagnostic_ = result.diagnostic;
            if (result.accepted) {
                diagnostic_.clear();
            }
            return result;
        }

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
