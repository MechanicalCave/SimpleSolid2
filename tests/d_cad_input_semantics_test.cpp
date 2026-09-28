#include <simplesolid2/application/cad_input_semantics.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
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
    std::optional<sketch::PointRequest> request;
    std::optional<sketch::SketchTool> activated;
    std::optional<double> submitted_distance;

    bool cadInputSemanticActive() const noexcept override { return active; }
    std::optional<sketch::PointRequest>
    cadInputSemanticPointRequest() const noexcept override { return request; }
    bool activateCadInputSemanticTool(sketch::SketchTool tool) override {
        activated = tool;
        return activation_result;
    }
    bool submitCadInputSemanticDirectDistance(double distance) override {
        submitted_distance = distance;
        return direct_result;
    }
};
} // namespace

int main() {
    Target target;
    application::SketchCadInputSemanticEndpoint dot{
        target, application::CadInputNumberFormat{"."}};

    auto result = dot.submit("  line  ");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::line);

    target.activated.reset();
    result = dot.submit("MOVE");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::move);

    target.activated.reset();
    result = dot.submit("copy");
    CHECK(result.accepted);
    CHECK(target.activated == sketch::SketchTool::copy);

    target.activated.reset();
    result = dot.submit("nonesuch");
    CHECK(!result.accepted);
    CHECK(result.diagnostic == "Unknown Sketch command.");
    CHECK(!target.activated.has_value());

    target.request = sketch::PointRequest{
        sketch::Point2{0.0, 0.0},
        sketch::Point2{3.0, 4.0},
        true};
    target.submitted_distance.reset();
    result = dot.submit("12.5");
    CHECK(result.accepted);
    CHECK(target.submitted_distance.has_value());
    CHECK(near(*target.submitted_distance, 12.5));

    target.activated.reset();
    target.submitted_distance.reset();
    result = dot.submit("LINE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic ==
          "Active point input expects a bare finite distance.");
    CHECK(!target.activated.has_value());
    CHECK(!target.submitted_distance.has_value());

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

    target.request.reset();
    target.active = false;
    result = dot.submit("LINE");
    CHECK(!result.accepted);
    CHECK(result.diagnostic == "No active CAD command context.");

    CHECK(application::parseBareCadDistance(
              ".5", application::CadInputNumberFormat{"."}).has_value());
    CHECK(!application::parseBareCadDistance(
               "-1", application::CadInputNumberFormat{"."}).has_value());
    CHECK(!application::parseBareCadDistance(
               "1e3", application::CadInputNumberFormat{"."}).has_value());

    std::cout << "D CAD input semantics PASS\n";
    return EXIT_SUCCESS;
}
