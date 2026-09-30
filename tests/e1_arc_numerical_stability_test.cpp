#include <simplesolid2/sketch/interaction_state.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <string_view>

using namespace simplesolid2;

namespace {

constexpr double full_turn =
    2.0 * 3.141592653589793238462643383279502884;

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
    const sketch::ArcIntent& arc) {
    const double coordinate_scale =
        std::max({
            std::abs(start.u),
            std::abs(start.v),
            std::abs(through.u),
            std::abs(through.v),
            std::abs(end.u),
            std::abs(end.v),
            std::abs(arc.center.u),
            std::abs(arc.center.v),
            std::abs(arc.radius),
            std::numeric_limits<double>::min()});
    return
        512.0 *
        std::numeric_limits<double>::epsilon() *
        coordinate_scale;
}

double positiveTurn(double angle) {
    double normalized = std::fmod(angle, full_turn);
    if (normalized < 0.0) {
        normalized += full_turn;
    }
    return normalized;
}

double pointDirection(
    sketch::Point2 center,
    sketch::Point2 point) {
    return std::atan2(
        point.v - center.v,
        point.u - center.u);
}

std::optional<CaseResult> buildArc(
    sketch::Point2 start,
    sketch::Point2 through,
    sketch::Point2 end) {
    sketch::SketchInteractionState state;
    state.activateArc();

    if (state.acceptArcPoint(start).outcome !=
            sketch::ArcPointOutcome::start_accepted ||
        state.acceptArcPoint(end).outcome !=
            sketch::ArcPointOutcome::end_accepted) {
        return std::nullopt;
    }

    const auto preview = state.previewArc(through);
    if (!preview) {
        return std::nullopt;
    }

    const auto result = state.acceptArcPoint(through);
    if (result.outcome !=
            sketch::ArcPointOutcome::arc_requested ||
        !result.request ||
        *preview != *result.request) {
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
            measured.arc);
    return measured;
}

std::optional<CaseResult> buildQuarterArc(
    double radius,
    sketch::Point2 translation,
    bool clockwise = false) {
    const double quadrant =
        radius / std::sqrt(2.0);
    const double sign = clockwise ? -1.0 : 1.0;

    return buildArc(
        {translation.u + radius,
         translation.v},
        {translation.u + quadrant,
         translation.v + sign * quadrant},
        {translation.u,
         translation.v + sign * radius});
}

void printCase(
    std::string_view name,
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

void verifyResiduals(const CaseResult& value) {
    for (const double residual : value.residuals) {
        CHECK(residual <= value.bound);
    }
}

void verifyThroughOnSignedSweep(
    const CaseResult& value) {
    const double start_angle =
        pointDirection(
            value.arc.center,
            value.start);
    CHECK(
        std::abs(
            std::remainder(
                start_angle -
                    value.arc.start_angle,
                full_turn)) <=
        1024.0 *
            std::numeric_limits<double>::epsilon());

    const double through_angle =
        pointDirection(
            value.arc.center,
            value.through);
    const double end_angle =
        pointDirection(
            value.arc.center,
            value.end);

    if (value.arc.sweep_angle > 0.0) {
        const double through_delta =
            positiveTurn(
                through_angle -
                value.arc.start_angle);
        const double end_delta =
            positiveTurn(
                end_angle -
                value.arc.start_angle);
        CHECK(through_delta > 0.0);
        CHECK(through_delta < value.arc.sweep_angle);
        CHECK(
            std::abs(
                end_delta -
                value.arc.sweep_angle) <=
            1024.0 *
                std::numeric_limits<double>::epsilon());
    } else {
        const double through_delta =
            positiveTurn(
                value.arc.start_angle -
                through_angle);
        const double end_delta =
            positiveTurn(
                value.arc.start_angle -
                end_angle);
        CHECK(through_delta > 0.0);
        CHECK(
            through_delta <
            std::abs(value.arc.sweep_angle));
        CHECK(
            std::abs(
                end_delta -
                std::abs(value.arc.sweep_angle)) <=
            1024.0 *
                std::numeric_limits<double>::epsilon());
    }
}

void verifyAccepted(
    std::string_view name,
    const std::optional<CaseResult>& value) {
    CHECK(value.has_value());
    printCase(name, *value);
    verifyResiduals(*value);
    verifyThroughOnSignedSweep(*value);
}

void verifyEquivalentQuarter(
    const CaseResult& origin,
    const CaseResult& translated,
    sketch::Point2 translation) {
    const double error =
        std::max({
            std::abs(
                translated.arc.center.u -
                translation.u -
                origin.arc.center.u),
            std::abs(
                translated.arc.center.v -
                translation.v -
                origin.arc.center.v),
            std::abs(
                translated.arc.radius -
                origin.arc.radius)});

    std::cout
        << std::setprecision(17)
        << "translation=("
        << translation.u << ", "
        << translation.v << ")"
        << " equivalence_error="
        << error
        << " bound="
        << translated.bound
        << '\n';

    CHECK(error <= translated.bound);
    CHECK(
        std::abs(
            translated.arc.sweep_angle -
            origin.arc.sweep_angle) <=
        1024.0 *
            std::numeric_limits<double>::epsilon());
}

} // namespace

int main() {
    // Mandatory E1 case: radius 0.1 at origin and +/-1e6 translation.
    const auto origin =
        buildQuarterArc(0.1, {0.0, 0.0});
    const auto translated_positive =
        buildQuarterArc(
            0.1,
            {1000000.0, 1000000.0});
    const auto translated_negative =
        buildQuarterArc(
            0.1,
            {-1000000.0, -1000000.0});

    verifyAccepted("origin-r0.1", origin);
    verifyAccepted(
        "translated+1e6-r0.1",
        translated_positive);
    verifyAccepted(
        "translated-1e6-r0.1",
        translated_negative);

    verifyEquivalentQuarter(
        *origin,
        *translated_positive,
        {1000000.0, 1000000.0});
    verifyEquivalentQuarter(
        *origin,
        *translated_negative,
        {-1000000.0, -1000000.0});

    // Existing branch semantics at ordinary scale.
    const auto unit_ccw =
        buildQuarterArc(1.0, {0.0, 0.0});
    const auto unit_cw =
        buildQuarterArc(
            1.0,
            {0.0, 0.0},
            true);
    const auto long_ccw =
        buildArc(
            {1.0, 0.0},
            {-1.0, 0.0},
            {0.0, -1.0});

    verifyAccepted("unit-short-ccw", unit_ccw);
    verifyAccepted("unit-short-cw", unit_cw);
    verifyAccepted("unit-long-ccw", long_ccw);

    CHECK(unit_ccw->arc.sweep_angle > 0.0);
    CHECK(unit_ccw->arc.sweep_angle < full_turn * 0.5);
    CHECK(unit_cw->arc.sweep_angle < 0.0);
    CHECK(
        std::abs(unit_cw->arc.sweep_angle) <
        full_turn * 0.5);
    CHECK(long_ccw->arc.sweep_angle > full_turn * 0.5);
    CHECK(long_ccw->arc.sweep_angle < full_turn);

    // Normalization must cover scales at which the old absolute-square
    // formulation underflowed or overflowed while the local construction
    // itself remains representable.
    const auto tiny =
        buildQuarterArc(
            1.0e-200,
            {0.0, 0.0});
    const auto huge =
        buildQuarterArc(
            1.0e200,
            {0.0, 0.0});

    verifyAccepted("tiny-r1e-200", tiny);
    verifyAccepted("huge-r1e200", huge);

    // Near-collinear but non-collinear remains a valid finite construction;
    // no new fixed Product epsilon is introduced by E1.
    const auto near_collinear =
        buildArc(
            {0.0, 0.0},
            {1.0, 1.0e-6},
            {2.0, 0.0});
    verifyAccepted(
        "near-collinear",
        near_collinear);

    // Exact degeneracy and invalid input retain fail-closed semantics.
    {
        sketch::SketchInteractionState state;
        state.activateArc();
        CHECK(
            state.acceptArcPoint({0.0, 0.0}).outcome ==
            sketch::ArcPointOutcome::start_accepted);
        CHECK(
            state.acceptArcPoint({1.0, 0.0}).outcome ==
            sketch::ArcPointOutcome::end_accepted);
        CHECK(
            state.acceptArcPoint({2.0, 0.0}).outcome ==
            sketch::ArcPointOutcome::degenerate_ignored);
    }

    {
        sketch::SketchInteractionState state;
        state.activateArc();
        CHECK(
            state.acceptArcPoint({0.0, 0.0}).outcome ==
            sketch::ArcPointOutcome::start_accepted);
        CHECK(
            state.acceptArcPoint({0.0, 0.0}).outcome ==
            sketch::ArcPointOutcome::degenerate_ignored);
    }

    {
        sketch::SketchInteractionState state;
        state.activateArc();
        CHECK(
            state.acceptArcPoint({
                std::numeric_limits<double>::infinity(),
                0.0}).outcome ==
            sketch::ArcPointOutcome::invalid_point);
    }

    // Finite endpoints whose local subtraction is not representable fail
    // closed instead of relying on an overflowing intermediate.
    {
        constexpr double high =
            std::numeric_limits<double>::max();
        sketch::SketchInteractionState state;
        state.activateArc();
        CHECK(
            state.acceptArcPoint({high, 0.0}).outcome ==
            sketch::ArcPointOutcome::start_accepted);
        CHECK(
            state.acceptArcPoint({-high, 1.0}).outcome ==
            sketch::ArcPointOutcome::end_accepted);
        CHECK(
            state.acceptArcPoint({0.0, 2.0}).outcome ==
            sketch::ArcPointOutcome::degenerate_ignored);
    }

    std::cout << "E1 Arc numerical stability PASS\n";
    return EXIT_SUCCESS;
}
