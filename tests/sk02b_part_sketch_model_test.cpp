#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/part/part_sketch.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "SK-02B Part Sketch model CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::PartSketch makeSketch() {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());

    const auto placement =
        part::sketchPlacementForSupport(*support);
    CHECK(placement.has_value());

    return part::PartSketch{
        sketch::SketchId::generate(),
        *support,
        *placement,
        true};
}

} // namespace

int main() {
    auto hosted = makeSketch();

    CHECK(hosted.model.entityCount() == 0U);

    const auto line_id =
        hosted.model.addLine(
            sketch::Point2{0.0, 0.0},
            sketch::Point2{25.0, 0.0});

    CHECK(line_id.valid());
    CHECK(hosted.model.entityCount() == 1U);
    CHECK(hosted.model.findLine(line_id) != nullptr);

    auto copied = hosted;
    CHECK(copied == hosted);
    CHECK(copied.model.findLine(line_id) != nullptr);

    CHECK(copied.model.erase(line_id));
    CHECK(copied.model.entityCount() == 0U);
    CHECK(hosted.model.entityCount() == 1U);
    CHECK(hosted.model.findLine(line_id) != nullptr);
    CHECK(copied != hosted);

    part::PartAuthoredState state;
    state.sketches.push_back(hosted);

    auto copied_state = state;
    CHECK(copied_state == state);
    CHECK(copied_state.sketches.size() == 1U);

    CHECK(copied_state.sketches[0].model.erase(line_id));
    CHECK(copied_state != state);
    CHECK(state.sketches[0].model.findLine(line_id) != nullptr);

    return EXIT_SUCCESS;
}
