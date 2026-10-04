#include <simplesolid2/viewer/sketch_selection_query.hpp>
#include <simplesolid2/viewer/solid_presentation.hpp>

#include <cstdlib>
#include <iostream>
#include <limits>

using namespace simplesolid2::viewer;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04B query contract CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    const auto normalized =
        normalizedViewportRect(
            ViewportPoint2{30.0, 40.0},
            ViewportPoint2{10.0, 15.0});
    CHECK(normalized.has_value());
    CHECK(normalized->valid());
    CHECK(normalized->minimum.x == 10.0);
    CHECK(normalized->minimum.y == 15.0);
    CHECK(normalized->maximum.x == 30.0);
    CHECK(normalized->maximum.y == 40.0);

    CHECK(!normalizedViewportRect(
        ViewportPoint2{10.0, 10.0},
        ViewportPoint2{10.0, 30.0})
        .has_value());
    CHECK(!normalizedViewportRect(
        ViewportPoint2{10.0, 10.0},
        ViewportPoint2{30.0, 10.0})
        .has_value());

    const auto nan =
        std::numeric_limits<double>::quiet_NaN();
    CHECK(!normalizedViewportRect(
        ViewportPoint2{nan, 0.0},
        ViewportPoint2{10.0, 10.0})
        .has_value());

    SketchPointQueryResult failed_point;
    CHECK(failed_point.valid());
    CHECK(!failed_point.completed);
    CHECK(!failed_point.token.has_value());

    SketchPointQueryResult no_hit{
        true,
        std::nullopt};
    CHECK(no_hit.valid());
    CHECK(no_hit.completed);
    CHECK(!no_hit.token.has_value());

    SketchPointQueryResult hit{
        true,
        PresentationToken{7U}};
    CHECK(hit.valid());
    CHECK(hit.token.has_value());

    SketchPointQueryResult impossible{
        false,
        PresentationToken{7U}};
    CHECK(!impossible.valid());

    SketchPointQueryResult invalid_completed{
        true,
        PresentationToken{}};
    CHECK(!invalid_completed.valid());

    SketchRectangleQueryResult failed_rect;
    CHECK(failed_rect.valid());
    CHECK(!failed_rect.completed);

    SketchRectangleQueryResult empty_rect{
        true,
        {}};
    CHECK(empty_rect.valid());

    SketchRectangleQueryResult hits{
        true,
        {
            PresentationToken{11U},
            PresentationToken{12U},
        }};
    CHECK(hits.valid());

    hits.tokens.push_back(
        PresentationToken{11U});
    CHECK(!hits.valid());

    SketchRectangleQueryResult invalid_hit{
        true,
        {PresentationToken{}}};
    CHECK(!invalid_hit.valid());

    SketchRectangleQueryResult failed_with_hits{
        false,
        {PresentationToken{11U}}};
    CHECK(!failed_with_hits.valid());

    SketchSelectionBoxOverlay overlay{
        ViewportPoint2{100.0, 100.0},
        ViewportPoint2{100.0, 100.0},
        SketchRectangleSelectionRule::window};
    CHECK(overlay.valid());

    overlay.rule =
        SketchRectangleSelectionRule::crossing;
    CHECK(overlay.valid());

    overlay.current.x = nan;
    CHECK(!overlay.valid());

    BodyTopologyPickFilter all;
    CHECK(all.any());
    CHECK(all.allows(
        BodyTopologyPresentationKind::face));
    CHECK(all.allows(
        BodyTopologyPresentationKind::edge));
    CHECK(all.allows(
        BodyTopologyPresentationKind::vertex));

    BodyTopologyPickFilter face_only{
        true,
        false,
        false};
    CHECK(face_only.any());
    CHECK(face_only.allows(
        BodyTopologyPresentationKind::face));
    CHECK(!face_only.allows(
        BodyTopologyPresentationKind::edge));

    BodyTopologyPickQueryResult failed_body;
    CHECK(failed_body.valid());
    CHECK(!failed_body.completed);

    BodyTopologyPickQueryResult empty_body{
        true,
        {},
        {}};
    CHECK(empty_body.valid());

    BodyTopologyPickQueryResult body_hits{
        true,
        BodyPresentationGeneration{17U},
        {
            {
                PresentationToken{100U},
                BodyTopologyPresentationKind::vertex,
                1.5,
                5.0},
            {
                PresentationToken{101U},
                BodyTopologyPresentationKind::edge,
                0.5,
                5.5},
        }};
    CHECK(body_hits.valid());

    body_hits.candidates.front()
        .screen_distance = -1.0;
    CHECK(!body_hits.valid());

    BodyTopologyPickQueryResult
        impossible_body{
            false,
            BodyPresentationGeneration{17U},
            {}};
    CHECK(!impossible_body.valid());

    CHECK(
        ViewStyle::shaded !=
        ViewStyle::shaded_with_edges);
    CHECK(
        ViewStyle::shaded_with_edges !=
        ViewStyle::shaded_with_hidden_edges);

    return EXIT_SUCCESS;
}
