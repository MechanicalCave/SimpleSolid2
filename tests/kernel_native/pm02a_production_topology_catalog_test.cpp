#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02A topology catalog CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string_view source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::string{source},
            0U,
            use_index,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    double x0,
    double y0,
    double x1,
    double y1,
    kernel::Point3 origin = {}) {
    kernel::PlanarProfileInput profile;
    profile.frame.origin = origin;
    profile.outer.boundary = {
        lineUse({x0, y0}, {x1, y0}, "bottom", 0U),
        lineUse({x1, y0}, {x1, y1}, "right", 1U),
        lineUse({x1, y1}, {x0, y1}, "top", 2U),
        lineUse({x0, y1}, {x0, y0}, "left", 3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput forward(
    kernel::PlanarProfileInput profile,
    double distance,
    kernel::SolidBooleanOperation operation =
        kernel::SolidBooleanOperation::add) {
    return {
        std::move(profile),
        0.0,
        distance,
        kernel::ExtrudeCapRole::profile_cap,
        kernel::ExtrudeCapRole::extent_cap,
        operation,
    };
}

template <typename Token>
void verifyTokens(
    const std::vector<Token>& tokens) {
    for (std::size_t index = 0U;
         index < tokens.size();
         ++index) {
        CHECK(tokens[index].valid());
        for (std::size_t other = index + 1U;
             other < tokens.size();
             ++other) {
            CHECK(tokens[index] != tokens[other]);
        }
    }
}

void verifyCompleteInventory(
    const kernel::SolidModelingResult& result) {
    CHECK(result.ok());
    CHECK(result.face_count == result.current_faces.size());
    CHECK(result.edge_count == result.current_edges.size());
    CHECK(result.vertex_count == result.current_vertices.size());
    verifyTokens(result.current_faces);
    verifyTokens(result.current_edges);
    verifyTokens(result.current_vertices);

    for (const auto& face : result.new_faces) {
        if (face.status !=
            kernel::ReferenceStatus::resolved) {
            continue;
        }
        CHECK(face.resolved_token.has_value());
        CHECK(
            std::count(
                result.current_faces.begin(),
                result.current_faces.end(),
                *face.resolved_token) == 1);
    }
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    const auto base =
        provider.extrude(
            forward(
                rectangle(
                    0.0, 0.0,
                    40.0, 30.0),
                10.0));
    verifyCompleteInventory(base);

    // Canonical PM-02P E01 baseline promoted into the production runtime
    // contract: every unique provider subshape has one typed runtime token.
    CHECK(base.face_count == 6U);
    CHECK(base.edge_count == 12U);
    CHECK(base.vertex_count == 8U);

    const auto attached =
        provider.extrude(
            forward(
                rectangle(
                    10.0, 5.0,
                    30.0, 20.0,
                    {0.0, 0.0, 10.0}),
                5.0),
            base.solid);
    verifyCompleteInventory(attached);

    for (const auto& inherited :
         attached.inherited_faces) {
        if (inherited.status !=
            kernel::ReferenceStatus::resolved) {
            continue;
        }
        CHECK(
            std::count(
                attached.current_faces.begin(),
                attached.current_faces.end(),
                inherited.token) == 1);
    }

    const auto cut =
        provider.extrude(
            forward(
                rectangle(
                    15.0, 8.0,
                    25.0, 22.0),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    verifyCompleteInventory(cut);

    for (const auto& inherited :
         cut.inherited_faces) {
        if (inherited.status !=
            kernel::ReferenceStatus::resolved) {
            continue;
        }
        CHECK(
            std::count(
                cut.current_faces.begin(),
                cut.current_faces.end(),
                inherited.token) == 1);
    }

    // Edge/Vertex tokens are complete runtime inventory in PM-02A, not
    // semantic lineage. A later stage therefore receives fresh tokens rather
    // than implying unsupported durable identity.
    if (!base.current_edges.empty() &&
        !attached.current_edges.empty()) {
        CHECK(
            attached.current_edges.front().value >
            base.current_edges.front().value);
    }
    if (!base.current_vertices.empty() &&
        !attached.current_vertices.empty()) {
        CHECK(
            attached.current_vertices.front().value >
            base.current_vertices.front().value);
    }

    std::cout
        << "PM02A_PRODUCTION_TOPOLOGY_CATALOG_PASS"
        << " base_faces=" << base.face_count
        << " base_edges=" << base.edge_count
        << " base_vertices=" << base.vertex_count
        << " attached_faces=" << attached.face_count
        << " cut_faces=" << cut.face_count
        << '\n';

    return EXIT_SUCCESS;
}
