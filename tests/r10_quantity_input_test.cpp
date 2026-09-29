#include <simplesolid2/application/precision_input.hpp>
#include <simplesolid2/core/units.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>
#include <string_view>

using namespace simplesolid2;

namespace {
void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "R10 quantity input CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(...) check(static_cast<bool>((__VA_ARGS__)), #__VA_ARGS__, __LINE__)

bool near(double a, double b) {
    return std::abs(a - b) < 1.0e-11;
}

std::optional<application::CadQuantity>
length(
    std::string_view text,
    core::LengthUnit unit = core::LengthUnit::millimetre) {
    return application::parseCadQuantity(
        text,
        {application::CadQuantityDimension::length, unit});
}

std::optional<application::CadQuantity>
angle(std::string_view text) {
    return application::parseCadQuantity(
        text,
        {application::CadQuantityDimension::angle,
         core::LengthUnit::millimetre});
}

std::optional<application::CadQuantity>
scalar(std::string_view text) {
    return application::parseCadQuantity(
        text,
        {application::CadQuantityDimension::scalar,
         core::LengthUnit::millimetre});
}
} // namespace

int main() {
    using application::CadPointTokenKind;
    using application::CadQuantityDimension;
    using core::LengthUnit;

    CHECK(core::millimetresPerUnit(LengthUnit::millimetre) == 1.0);
    CHECK(core::millimetresPerUnit(LengthUnit::centimetre) == 10.0);
    CHECK(core::millimetresPerUnit(LengthUnit::metre) == 1000.0);
    CHECK(core::millimetresPerUnit(LengthUnit::inch) == 25.4);
    CHECK(core::millimetresPerUnit(LengthUnit::foot) == 304.8);
    CHECK(core::lengthUnitSuffix(LengthUnit::inch) == "in");

    auto value = length("25");
    CHECK(value.has_value());
    CHECK(value->dimension == CadQuantityDimension::length);
    CHECK(near(value->canonical_value, 25.0));

    value = length("25mm");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 25.0));

    value = length("2.5cm");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 25.0));

    value = length("0,25m");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 250.0));

    value = length("1in");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 25.4));

    value = length("2ft");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 609.6));

    value = length("2", LengthUnit::inch);
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 50.8));

    value = length("25mm", LengthUnit::inch);
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 25.0));

    value = length("1e-3m");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 1.0));

    value = length(".5in");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 12.7));

    value = length("25mm + 1in");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 50.4));

    value = length("2 * 12.5mm");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 25.0));

    value = length("1in / 2");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 12.7));

    value = length("(50 + 25)mm");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 75.0));

    value = length("25mm + 1");
    CHECK(value.has_value());
    CHECK(near(value->canonical_value, 26.0));

    auto angle_value = angle("45");
    CHECK(angle_value.has_value());
    CHECK(near(
        angle_value->canonical_value,
        std::numbers::pi_v<double> / 4.0));

    angle_value = angle("45deg");
    CHECK(angle_value.has_value());
    CHECK(near(
        angle_value->canonical_value,
        std::numbers::pi_v<double> / 4.0));

    angle_value = angle("0.7853981633974483rad");
    CHECK(angle_value.has_value());
    CHECK(near(
        angle_value->canonical_value,
        std::numbers::pi_v<double> / 4.0));

    angle_value = angle("90deg / 2");
    CHECK(angle_value.has_value());
    CHECK(near(
        angle_value->canonical_value,
        std::numbers::pi_v<double> / 4.0));

    const auto bare_angle_in_inch_document =
        application::parseCadQuantity(
            "30",
            {CadQuantityDimension::angle, LengthUnit::inch});
    CHECK(bare_angle_in_inch_document.has_value());
    CHECK(near(
        bare_angle_in_inch_document->canonical_value,
        std::numbers::pi_v<double> / 6.0));

    const auto scalar_value = scalar("1 + 0.25");
    CHECK(scalar_value.has_value());
    CHECK(near(scalar_value->canonical_value, 1.25));

    CHECK(!length("1mm * 2mm").has_value());
    CHECK(!angle("1deg * 2deg").has_value());
    CHECK(!length("1mm / 2mm").has_value());
    CHECK(!length("1mm + 1deg").has_value());
    CHECK(!length("pi").has_value());
    CHECK(!length("sqrt(4)").has_value());
    CHECK(!length("1'-2\"").has_value());
    CHECK(!length("1.2,3").has_value());
    CHECK(!length("1 / 0").has_value());
    CHECK(!length("1e9999").has_value());

    auto point = application::parseCadPointToken(
        "25;10", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(point->kind == CadPointTokenKind::absolute_cartesian);
    CHECK(near(point->first, 25.0));
    CHECK(near(point->second, 10.0));

    point = application::parseCadPointToken(
        "25mm;1in", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(near(point->first, 25.0));
    CHECK(near(point->second, 25.4));

    point = application::parseCadPointToken(
        "@2in;10mm", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(point->kind == CadPointTokenKind::relative_cartesian);
    CHECK(near(point->first, 50.8));
    CHECK(near(point->second, 10.0));

    point = application::parseCadPointToken(
        "25,5;10,25", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(near(point->first, 25.5));
    CHECK(near(point->second, 10.25));

    point = application::parseCadPointToken(
        "@50<45", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(point->kind == CadPointTokenKind::relative_polar);
    CHECK(near(point->first, 50.0));
    CHECK(near(
        point->second,
        std::numbers::pi_v<double> / 4.0));

    point = application::parseCadPointToken(
        "@2in<30deg", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(near(point->first, 50.8));
    CHECK(near(
        point->second,
        std::numbers::pi_v<double> / 6.0));

    point = application::parseCadPointToken(
        "@25mm<0.5rad", LengthUnit::millimetre);
    CHECK(point.has_value());
    CHECK(near(point->first, 25.0));
    CHECK(near(point->second, 0.5));

    CHECK(!application::parseCadPointToken(
        "@-1<0", LengthUnit::millimetre).has_value());
    CHECK(!application::parseCadPointToken(
        "1,2,3;4", LengthUnit::millimetre).has_value());
    CHECK(!application::parseCadPointToken(
        "1;2;3", LengthUnit::millimetre).has_value());
    CHECK(!application::parseCadPointToken(
        "@1<2<3", LengthUnit::millimetre).has_value());

    std::cout << "R10 quantity input PASS\n";
    return EXIT_SUCCESS;
}
