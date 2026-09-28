#include <simplesolid2/sketch/interaction_state.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "E1 Arc numerical stability CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

struct CaseResult final {
    sketch::Point2 start;
    sketch::Point2 through;
    sketch::Point2 end;
    sketch::ArcIntent arc;
    std::array<double, 3U> residuals{};
    double bound{};
};

double radialResidual(
    sketch::Point2 point,
    const sketch::ArcIntent& arc) {
    return std::abs(
        std::hypot(
            point.u - arc.center.u,
            point.v - arc.center.v) -
        arc.radius);
}

double numericalBound(
    sketch::Point2 start,
    sketch::Point2 through,
    sketch::Point2 end,
    double radius) {
    const double coordinate_scale =
        std::max({
            1.0,
            std::abs(start.u),
            std::abs(start.v),
            std::abs(through.u),
            std::abs(through.v),
            std::abs(end.u),
            std::abs(end.v),
            std::abs(radius)});
    return
        128.0 *
        std::numeric_limits<double>::epsilon() *
        coordinate_scale;
}

std::optional<CaseResult> buildQuarterArc(
    sketch::Point2 translation) {
    constexpr double radius = 0.1;
    const double quadrant =
        radius / std::sqrt(2.0);

    const sketch::Point2 start{
        translation.u + radius,
        translation.v};
    const sketch::Point2 through{
        translation.u + quadrant,
        translation.v + quadrant};
    const sketch::Point2 end{
        translation.u,
        translation.v + radius};

    sketch::SketchInteractionState state;
    state.activateArc();

    if (state.acceptArcPoint(start).outcome !=
            sketch::ArcPointOutcome::start_accepted ||
        state.acceptArcPoint(through).outcome !=
            sketch::ArcPointOutcome::through_accepted) {
        return std::nullopt;
    }

    const auto result = state.acceptArcPoint(end);
    if (result.outcome !=
            sketch::ArcPointOutcome::arc_requested ||
        !result.request) {
        return std::nullopt;
    }

    CaseResult measured{
        start,
        through,
        end,
        *result.request};
    measured.residuals = {
        radialResidual(start, measured.arc),
        radialResidual(through, measured.arc),
        radialResidual(end, measured.arc)};
    measured.bound =
        numericalBound(
            start,
            through,
            end,
            measured.arc.radius);
    return measured;
}

void printCase(
    const char* name,
    const CaseResult& value) {
    std::cout
        << std::setprecision(17)
        << name
        << " center=("
        << value.arc.center.u << ", "
        << value.arc.center.v << ")"
        << " radius=" << value.arc.radius
        << " sweep=" << value.arc.sweep_angle
        << " residuals=("
        << value.residuals[0] << ", "
        << value.residuals[1] << ", "
        << value.residuals[2] << ")"
        << " bound=" << value.bound
        << '\n';
}

} // namespace

int main() {
    const auto origin =
        buildQuarterArc({0.0, 0.0});
    const auto translated =
        buildQuarterArc({1000000.0, 1000000.0});

    CHECK(origin.has_value());
    CHECK(translated.has_value());

    printCase("origin", *origin);
    printCase("translated+1e6", *translated);

    const double translated_center_u =
        translated->arc.center.u - 1000000.0;
    const double translated_center_v =
        translated->arc.center.v - 1000000.0;

    const double translation_equivalence_error =
        std::max({
            std::abs(
                translated_center_u -
                origin->arc.center.u),
            std::abs(
                translated_center_v -
                origin->arc.center.v),
            std::abs(
                translated->arc.radius -
                origin->arc.radius)});

    std::cout
        << std::setprecision(17)
        << "translation_equivalence_error="
        << translation_equivalence_error
        << " translated_bound="
        << translated->bound
        << '\n';

    for (const double residual : origin->residuals) {
        CHECK(residual <= origin->bound);
    }
    for (const double residual : translated->residuals) {
        CHECK(residual <= translated->bound);
    }
    CHECK(
        translation_equivalence_error <=
        translated->bound);

    CHECK(origin->arc.sweep_angle > 0.0);
    CHECK(translated->arc.sweep_angle > 0.0);
    CHECK(
        std::abs(
            translated->arc.sweep_angle -
            origin->arc.sweep_angle) <=
        128.0 *
            std::numeric_limits<double>::epsilon());

    std::cout << "E1 Arc numerical stability PASS\n";
    return EXIT_SUCCESS;
}
