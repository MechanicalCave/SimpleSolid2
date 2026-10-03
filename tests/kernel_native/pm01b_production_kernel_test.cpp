#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <cmath>
#include <cstdlib>
#include <limits>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01B production kernel CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string_view source) {
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
            0U,
            false},
    };
}

kernel::BoundaryUse2D arcUse(
    kernel::Point2 center,
    double radius,
    double start_angle,
    double sweep_angle,
    double start_parameter,
    double end_parameter,
    bool follows_source_direction,
    std::string_view source) {
    return {
        kernel::Arc2{
            center,
            radius,
            start_angle,
            sweep_angle},
        start_parameter,
        end_parameter,
        follows_source_direction,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::string{source},
            0U,
            0U,
            false},
    };
}

kernel::PlanarProfileInput upperHalfDisk(
    bool reverse_traversal,
    kernel::Point3 origin = {}) {
    constexpr double pi =
        3.141592653589793238462643383279502884;
    kernel::PlanarProfileInput profile;
    profile.frame.origin = origin;

    if (!reverse_traversal) {
        profile.outer.boundary = {
            arcUse(
                {0.0, 0.0},
                10.0,
                0.0,
                pi,
                0.0,
                1.0,
                true,
                "upper-arc"),
            lineUse(
                {-10.0, 0.0},
                {10.0, 0.0},
                "diameter"),
        };
    } else {
        // The resolved Profile already carries traversal start/end in loop
        // order. follows_source_direction only records provenance relative to
        // the authored Arc; it must not cause a second parameter reversal.
        profile.outer.boundary = {
            lineUse(
                {10.0, 0.0},
                {-10.0, 0.0},
                "diameter"),
            arcUse(
                {0.0, 0.0},
                10.0,
                0.0,
                pi,
                1.0,
                0.0,
                false,
                "upper-arc"),
        };
    }

    CHECK(profile.valid());
    return profile;
}

struct MeshBounds final {
    double min_y{std::numeric_limits<double>::infinity()};
    double max_y{-std::numeric_limits<double>::infinity()};
};

MeshBounds meshBoundsY(
    const kernel::SolidPresentationMesh& mesh) {
    MeshBounds bounds;
    const auto include =
        [&bounds](const kernel::Point3& point) {
            bounds.min_y =
                std::min(bounds.min_y, point.y);
            bounds.max_y =
                std::max(bounds.max_y, point.y);
        };
    for (const auto& triangle : mesh.triangles) {
        include(triangle.first);
        include(triangle.second);
        include(triangle.third);
    }
    return bounds;
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
        lineUse({x0, y0}, {x1, y0}, "bottom"),
        lineUse({x1, y0}, {x1, y1}, "right"),
        lineUse({x1, y1}, {x0, y1}, "top"),
        lineUse({x0, y1}, {x0, y0}, "left"),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput rectangleWithHole() {
    auto profile =
        rectangle(
            0.0, 0.0,
            40.0, 30.0);
    kernel::ProfileLoopInput hole;
    hole.boundary.push_back(
        kernel::BoundaryUse2D{
            kernel::Circle2{
                {20.0, 15.0},
                5.0},
            0.0,
            1.0,
            true,
            false,
            true,
            kernel::BoundaryUseProvenance{
                "hole-circle",
                1U,
                0U,
                true},
        });
    profile.holes.push_back(
        std::move(hole));
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

kernel::LinearExtrudeInput reverse(
    kernel::PlanarProfileInput profile,
    double distance,
    kernel::SolidBooleanOperation operation) {
    return {
        std::move(profile),
        -distance,
        0.0,
        kernel::ExtrudeCapRole::extent_cap,
        kernel::ExtrudeCapRole::profile_cap,
        operation,
    };
}

kernel::LinearExtrudeInput midplane(
    kernel::PlanarProfileInput profile,
    double distance) {
    return {
        std::move(profile),
        -distance * 0.5,
        distance * 0.5,
        kernel::ExtrudeCapRole::negative_cap,
        kernel::ExtrudeCapRole::positive_cap,
        kernel::SolidBooleanOperation::add,
    };
}

const kernel::NewFaceLineage* cap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    for (const auto& face : result.new_faces) {
        if (face.role.kind ==
                kernel::ExtrudeGeneratedFaceRoleKind::cap &&
            face.role.cap_role == role) {
            return &face;
        }
    }
    return nullptr;
}

std::size_t resolvedSides(
    const kernel::SolidModelingResult& result) {
    std::size_t count = 0U;
    for (const auto& face : result.new_faces) {
        if (face.role.kind ==
                kernel::ExtrudeGeneratedFaceRoleKind::side &&
            face.status ==
                kernel::ReferenceStatus::resolved &&
            face.resolved_token.has_value()) {
            ++count;
        }
    }
    return count;
}

class ForeignSolid final
    : public kernel::RuntimeSolid {};

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    // First Add: one solid, explicit cap semantics, four provenance sides.
    const auto base =
        provider.extrude(
            forward(
                rectangle(
                    0.0, 0.0,
                    40.0, 30.0),
                10.0));
    CHECK(base.ok());
    CHECK(base.solid_count == 1U);
    CHECK(base.brep_valid);
    CHECK(base.inherited_faces.empty());
    CHECK(base.new_faces.size() == 6U);
    CHECK(
        cap(
            base,
            kernel::ExtrudeCapRole::
                profile_cap) != nullptr);
    CHECK(
        cap(
            base,
            kernel::ExtrudeCapRole::
                extent_cap) != nullptr);
    CHECK(
        cap(
            base,
            kernel::ExtrudeCapRole::
                profile_cap)
            ->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        cap(
            base,
            kernel::ExtrudeCapRole::
                extent_cap)
            ->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(resolvedSides(base) == 4U);

    const auto base_mesh =
        provider.presentationMesh(
            base.solid);
    CHECK(base_mesh.ok());
    CHECK(!base_mesh.mesh.triangles.empty());
    for (const auto& triangle :
         base_mesh.mesh.triangles) {
        CHECK(triangle.valid());
    }

    const auto missing_mesh =
        provider.presentationMesh({});
    CHECK(!missing_mesh.ok());
    CHECK(
        missing_mesh.status ==
        kernel::SolidPresentationStatus::
            invalid_input);

    const auto holed =
        provider.extrude(
            forward(
                rectangleWithHole(),
                10.0));
    CHECK(holed.ok());
    CHECK(holed.solid_count == 1U);
    CHECK(resolvedSides(holed) == 5U);

    // Mixed Line+Arc Profile: reversing traversal must preserve the same
    // geometric upper semicircle instead of reflecting it to the lower side.
    const auto arc_forward =
        provider.extrude(
            forward(
                upperHalfDisk(false),
                10.0));
    CHECK(arc_forward.ok());
    CHECK(arc_forward.solid_count == 1U);
    CHECK(resolvedSides(arc_forward) == 2U);

    const auto arc_reverse =
        provider.extrude(
            forward(
                upperHalfDisk(true),
                10.0));
    CHECK(arc_reverse.ok());
    CHECK(arc_reverse.solid_count == 1U);
    CHECK(resolvedSides(arc_reverse) == 2U);

    const auto arc_forward_mesh =
        provider.presentationMesh(
            arc_forward.solid);
    const auto arc_reverse_mesh =
        provider.presentationMesh(
            arc_reverse.solid);
    CHECK(arc_forward_mesh.ok());
    CHECK(arc_reverse_mesh.ok());

    const auto forward_bounds =
        meshBoundsY(arc_forward_mesh.mesh);
    const auto reverse_bounds =
        meshBoundsY(arc_reverse_mesh.mesh);
    constexpr double geometry_epsilon = 1.0e-7;
    CHECK(forward_bounds.min_y >= -geometry_epsilon);
    CHECK(reverse_bounds.min_y >= -geometry_epsilon);
    CHECK(forward_bounds.max_y > 9.0);
    CHECK(reverse_bounds.max_y > 9.0);

    // Attached chained Add: remains one Body; the coincident profile cap may
    // disappear into the Boolean, but no missing/merged role is promoted to
    // false Resolved.
    const auto attached =
        provider.extrude(
            forward(
                rectangle(
                    10.0, 5.0,
                    30.0, 20.0,
                    {0.0, 0.0, 10.0}),
                5.0),
            base.solid);
    CHECK(attached.ok());
    CHECK(attached.solid_count == 1U);
    for (const auto& inherited :
         attached.inherited_faces) {
        CHECK(
            inherited.status !=
                kernel::ReferenceStatus::resolved ||
            inherited.candidate_count == 1U);
    }
    for (const auto& created :
         attached.new_faces) {
        CHECK(
            created.status !=
                kernel::ReferenceStatus::resolved ||
            (created.candidate_count == 1U &&
             created.resolved_token.has_value()));
    }

    // Reverse OneSide Cut against the original Body.
    const auto cut =
        provider.extrude(
            reverse(
                rectangle(
                    10.0, 8.0,
                    30.0, 22.0,
                    {0.0, 0.0, 10.0}),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(cut.ok());
    CHECK(cut.solid_count == 1U);
    CHECK(resolvedSides(cut) == 4U);

    // Midplane uses total distance and stable negative/positive cap roles.
    const auto centered =
        provider.extrude(
            midplane(
                rectangle(
                    0.0, 0.0,
                    20.0, 10.0),
                10.0));
    CHECK(centered.ok());
    CHECK(
        cap(
            centered,
            kernel::ExtrudeCapRole::
                negative_cap) != nullptr);
    CHECK(
        cap(
            centered,
            kernel::ExtrudeCapRole::
                positive_cap) != nullptr);
    CHECK(
        cap(
            centered,
            kernel::ExtrudeCapRole::
                negative_cap)
            ->status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        cap(
            centered,
            kernel::ExtrudeCapRole::
                positive_cap)
            ->status ==
        kernel::ReferenceStatus::resolved);

    // Detached Add cannot manufacture a second Body.
    const auto detached =
        provider.extrude(
            forward(
                rectangle(
                    100.0, 0.0,
                    120.0, 20.0),
                10.0),
            base.solid);
    CHECK(!detached.ok());
    CHECK(
        detached.status ==
        kernel::SolidModelingStatus::
            detached_add);
    CHECK(detached.solid == nullptr);

    // Add entirely inside the Body is explicit no-effect.
    const auto add_no_effect =
        provider.extrude(
            forward(
                rectangle(
                    5.0, 5.0,
                    15.0, 15.0,
                    {0.0, 0.0, 2.0}),
                5.0),
            base.solid);
    CHECK(!add_no_effect.ok());
    CHECK(
        add_no_effect.status ==
        kernel::SolidModelingStatus::
            no_effect);

    // Cut far away is explicit no-effect.
    const auto cut_no_effect =
        provider.extrude(
            forward(
                rectangle(
                    100.0, 0.0,
                    120.0, 20.0),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(!cut_no_effect.ok());
    CHECK(
        cut_no_effect.status ==
        kernel::SolidModelingStatus::
            no_effect);

    // Removing the entire Body is an explicit zero-solid outcome.
    const auto empty =
        provider.extrude(
            forward(
                rectangle(
                    0.0, 0.0,
                    40.0, 30.0),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(!empty.ok());
    CHECK(
        empty.status ==
        kernel::SolidModelingStatus::
            empty_result);

    // A through-slot that separates left/right material yields >1 solid and
    // is rejected instead of silently becoming multi-body.
    const auto split =
        provider.extrude(
            forward(
                rectangle(
                    18.0, -5.0,
                    22.0, 35.0),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(!split.ok());
    CHECK(
        split.status ==
        kernel::SolidModelingStatus::
            multi_solid);
    CHECK(split.solid_count > 1U);

    // Provider runtime objects are opaque and provider-scoped.
    auto foreign =
        std::make_shared<ForeignSolid>();
    const auto mismatch =
        provider.extrude(
            forward(
                rectangle(
                    0.0, 0.0,
                    5.0, 5.0),
                5.0),
            foreign);
    CHECK(!mismatch.ok());
    CHECK(
        mismatch.status ==
        kernel::SolidModelingStatus::
            provider_mismatch);

    const auto foreign_mesh =
        provider.presentationMesh(
            foreign);
    CHECK(!foreign_mesh.ok());
    CHECK(
        foreign_mesh.status ==
        kernel::SolidPresentationStatus::
            provider_mismatch);

    // Cut can never start an Empty Body.
    const auto missing =
        provider.extrude(
            forward(
                rectangle(
                    0.0, 0.0,
                    5.0, 5.0),
                5.0,
                kernel::SolidBooleanOperation::cut));
    CHECK(!missing.ok());
    CHECK(
        missing.status ==
        kernel::SolidModelingStatus::
            missing_upstream);

    std::cout
        << "PM01B_PRODUCTION_KERNEL_PASS"
        << " false_resolved=0"
        << " fuzzy=0"
        << " refine=off\n";
    return EXIT_SUCCESS;
}
