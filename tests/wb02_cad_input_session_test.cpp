#include <simplesolid2/application/cad_input.hpp>

#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

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

    [[nodiscard]] std::string
    cadInputPrompt() const override {
        return prompt;
    }

    [[nodiscard]]
    simplesolid2::application::CadInputSubmitResult
    submitCadInput(std::string_view text) override {
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

    CHECK(!session.hasEndpoint());
    CHECK(session.prompt() == "Command:");

    session.attachEndpoint(&first);
    CHECK(session.hasEndpoint());
    CHECK(session.endpoint() == &first);
    CHECK(session.prompt() == "Command: FAKE");

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

    first.accept = false;
    session.setBuffer("BAD");
    auto rejected = session.submit();
    CHECK(!rejected.accepted);
    CHECK(first.last == "BAD");
    CHECK(session.buffer() == "BAD");
    CHECK(session.diagnostic() == "Rejected.");

    session.attachEndpoint(&second);
    CHECK(session.endpoint() == &second);
    CHECK(session.buffer().empty());
    CHECK(session.diagnostic().empty());

    session.setBuffer("50");
    session.detachEndpoint();
    CHECK(!session.hasEndpoint());
    CHECK(session.buffer().empty());
    CHECK(session.prompt() == "Command:");

    session.setBuffer("ORPHAN");
    auto no_context = session.submit();
    CHECK(!no_context.accepted);
    CHECK(session.buffer() == "ORPHAN");
    CHECK(!session.diagnostic().empty());

    return EXIT_SUCCESS;
}
