#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace simplesolid2::application {

struct CadInputSubmitResult final {
    bool accepted{};
    std::string diagnostic;
};

class ICadInputEndpoint {
public:
    virtual ~ICadInputEndpoint() = default;

    [[nodiscard]] virtual std::string
    cadInputPrompt() const = 0;

    [[nodiscard]] virtual CadInputSubmitResult
    submitCadInput(std::string_view text) = 0;
};

class CadInputSession final {
public:
    void attachEndpoint(ICadInputEndpoint* endpoint);
    void detachEndpoint() noexcept;

    [[nodiscard]] bool hasEndpoint() const noexcept;
    [[nodiscard]] ICadInputEndpoint* endpoint() const noexcept;

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
    std::size_t generation_{};
};

} // namespace simplesolid2::application
