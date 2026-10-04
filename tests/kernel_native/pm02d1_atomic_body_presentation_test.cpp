#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02D1 Body presentation CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle() {
    kernel::PlanarProfileInput profile;
    profile.outer.boundary = {
        lineUse({0.0, 0.0}, {40.0, 0.0}, "bottom", 0U),
        lineUse({40.0, 0.0}, {40.0, 30.0}, "right", 1U),
        lineUse({40.0, 30.0}, {0.0, 30.0}, "top", 2U),
        lineUse({0.0, 30.0}, {0.0, 0.0}, "left", 3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput extrudeInput() {
    kernel::LinearExtrudeInput input;
    input.profile = rectangle();
    input.start_offset_mm = 0.0;
    input.end_offset_mm = 10.0;
    input.start_cap_role =
        kernel::ExtrudeCapRole::profile_cap;
    input.end_cap_role =
        kernel::ExtrudeCapRole::extent_cap;
    input.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(input.valid());
    return input;
}

template <typename Token, typename Presentation>
void checkTokenSet(
    const std::vector<Token>& expected,
    const std::vector<Presentation>& actual) {
    CHECK(expected.size() == actual.size());
    for (const auto token : expected) {
        CHECK(
            std::count_if(
                actual.begin(),
                actual.end(),
                [token](const auto& item) {
                    return item.runtime_token == token;
                }) == 1);
    }
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    const auto modeled =
        provider.extrude(extrudeInput());
    CHECK(modeled.ok());
    CHECK(modeled.face_count == 6U);
    CHECK(modeled.edge_count == 12U);
    CHECK(modeled.vertex_count == 8U);

    const auto presented =
        provider.bodyPresentation(
            modeled.solid);
    CHECK(presented.ok());
    CHECK(presented.body.valid());

    CHECK(presented.body.faces.size() == 6U);
    CHECK(presented.body.edges.size() == 12U);
    CHECK(presented.body.vertices.size() == 8U);
    CHECK(!presented.body.mesh.triangles.empty());

    checkTokenSet(
        modeled.current_faces,
        presented.body.faces);
    checkTokenSet(
        modeled.current_edges,
        presented.body.edges);
    checkTokenSet(
        modeled.current_vertices,
        presented.body.vertices);

    std::vector<bool> covered(
        presented.body.mesh.triangles.size(),
        false);
    for (const auto& face :
         presented.body.faces) {
        CHECK(
            face.valid(
                presented.body.mesh.triangles.size()));
        for (std::size_t triangle =
                 face.first_triangle;
             triangle <
                 face.first_triangle +
                     face.triangle_count;
             ++triangle) {
            CHECK(!covered[triangle]);
            covered[triangle] = true;
        }
    }
    CHECK(
        std::all_of(
            covered.begin(),
            covered.end(),
            [](bool value) {
                return value;
            }));

    for (const auto& edge :
         presented.body.edges) {
        CHECK(edge.valid());
        CHECK(edge.points.size() >= 2U);
    }
    for (const auto& vertex :
         presented.body.vertices) {
        CHECK(vertex.valid());
    }

    // Re-extracting presentation from the same RuntimeSolid must preserve
    // provider runtime-token membership. PresentationToken/generation are
    // intentionally owned later by PartViewportController, not the kernel.
    const auto repeated =
        provider.bodyPresentation(
            modeled.solid);
    CHECK(repeated.ok());
    checkTokenSet(
        modeled.current_faces,
        repeated.body.faces);
    checkTokenSet(
        modeled.current_edges,
        repeated.body.edges);
    checkTokenSet(
        modeled.current_vertices,
        repeated.body.vertices);

    std::cout
        << "PM02D1_ATOMIC_BODY_PRESENTATION_PASS"
        << " faces=" << presented.body.faces.size()
        << " edges=" << presented.body.edges.size()
        << " vertices=" << presented.body.vertices.size()
        << " triangles="
        << presented.body.mesh.triangles.size()
        << '\n';
    return EXIT_SUCCESS;
}
