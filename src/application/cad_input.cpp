#include <simplesolid2/application/cad_input.hpp>

#include <utility>

namespace simplesolid2::application {

void CadInputSession::attachEndpoint(
    ICadInputEndpoint* endpoint) {
    endpoint_ = endpoint;
    ++generation_;
    buffer_.clear();
    diagnostic_.clear();
}

void CadInputSession::detachEndpoint() noexcept {
    endpoint_ = nullptr;
    ++generation_;
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

void CadInputSession::setBuffer(std::string text) {
    buffer_ = std::move(text);
    diagnostic_.clear();
}

void CadInputSession::appendText(
    std::string_view text) {
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

    diagnostic_.clear();
    return true;
}

void CadInputSession::clearBuffer() noexcept {
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

    // Enter/Return consumes one submitted token regardless of whether
    // the active semantic context accepts or rejects it. Correction is
    // performed before submission; a rejected token reports a
    // diagnostic while the next input starts from an empty live buffer.
    const std::string submitted = buffer_;
    buffer_.clear();

    auto* const target = endpoint_;
    const auto generation = generation_;
    if (target == nullptr) {
        CadInputSubmitResult result{
            false,
            "No active CAD input context."};
        diagnostic_ = result.diagnostic;
        return result;
    }

    auto result = target->submitCadInput(submitted);

    if (endpoint_ != target ||
        generation_ != generation) {
        CadInputSubmitResult stale{
            false,
            "CAD input context changed during submission."};
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
