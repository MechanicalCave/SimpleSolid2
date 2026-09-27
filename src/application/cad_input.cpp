#include <simplesolid2/application/cad_input.hpp>

#include <utility>

namespace simplesolid2::application {

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
// QWidget/endpoint object identity, owns the lifetime of a live token.
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
