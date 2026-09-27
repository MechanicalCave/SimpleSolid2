#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::application {

using CadInputContextGeneration = std::uint64_t;

struct CadInputSubmitResult final {
    bool accepted{};
    std::string diagnostic;
};

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

    [[nodiscard]] CadInputSubmitResult submit();

private:
    ICadInputEndpoint* endpoint_{};
    std::string buffer_;
    std::string diagnostic_;
    std::size_t endpoint_generation_{};
    CadInputContextGeneration observed_context_generation_{};
    std::optional<CadInputContextGeneration>
        buffer_context_generation_;
};

} // namespace simplesolid2::application
