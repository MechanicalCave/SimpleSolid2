#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <numbers>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02B production Surface lineage CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

constexpr double epsilon = 1.0e-12;

bool near(double lhs, double rhs) {
    return std::abs(lhs - rhs) <= epsilon;
}

bool near(
    const kernel::Point3& lhs,
    const kernel::Point3& rhs) {
    return near(lhs.x, rhs.x) &&
           near(lhs.y, rhs.y) &&
           near(lhs.z, rhs.z);
}

bool near(
    const kernel::Frame3& lhs,
    const kernel::Frame3& rhs) {
    return near(lhs.origin, rhs.origin) &&
           near(lhs.u_axis, rhs.u_axis) &&
           near(lhs.v_axis, rhs.v_axis) &&
           near(lhs.normal, rhs.normal);
}

kernel::BoundaryUse2D lineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index,
    bool reversed = false) {
    return {
        kernel::Line2{start, end},
        reversed ? 1.0 : 0.0,
        reversed ? 0.0 : 1.0,
        !reversed,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

kernel::BoundaryUse2D circleUse(
    kernel::Point2 center,
    double radius,
    std::string source) {
    return {
        kernel::Circle2{center, radius},
        0.0,
        1.0,
        true,
        false,
        true,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            0U,
            false},
    };
}

kernel::PlanarProfileInput rectangle(
    double x0,
    double y0,
    double x1,
    double y1,
    double z,
    std::string_view prefix,
    std::optional<kernel::Frame3> frame =
        std::nullopt) {
    kernel::PlanarProfileInput profile;
    if (frame) {
        profile.frame = *frame;
    }
    profile.frame.origin.x += 0.0;
    profile.frame.origin.y += 0.0;
    profile.frame.origin.z += z;
    profile.outer.boundary = {
        lineUse(
            {x0, y0},
            {x1, y0},
            std::string{prefix} + "-bottom",
            0U),
        lineUse(
            {x1, y0},
            {x1, y1},
            std::string{prefix} + "-right",
            1U),
        lineUse(
            {x1, y1},
            {x0, y1},
            std::string{prefix} + "-top",
            2U),
        lineUse(
            {x0, y1},
            {x0, y0},
            std::string{prefix} + "-left",
            3U),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput reversedRectangle(
    double x0,
    double y0,
    double x1,
    double y1,
    double z,
    std::string_view prefix,
    std::optional<kernel::Frame3> frame =
        std::nullopt) {
    kernel::PlanarProfileInput profile;
    if (frame) {
        profile.frame = *frame;
    }
    profile.frame.origin.z += z;
    profile.outer.boundary = {
        lineUse(
            {x0, y1},
            {x0, y0},
            std::string{prefix} + "-left",
            3U,
            true),
        lineUse(
            {x1, y1},
            {x0, y1},
            std::string{prefix} + "-top",
            2U,
            true),
        lineUse(
            {x1, y0},
            {x1, y1},
            std::string{prefix} + "-right",
            1U,
            true),
        lineUse(
            {x0, y0},
            {x1, y0},
            std::string{prefix} + "-bottom",
            0U,
            true),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::PlanarProfileInput circleProfile(
    double z,
    std::string source) {
    kernel::PlanarProfileInput profile;
    profile.frame.origin = {0.0, 0.0, z};
    profile.outer.boundary = {
        circleUse(
            {0.0, 0.0},
            10.0,
            std::move(source)),
    };
    CHECK(profile.valid());
    return profile;
}

kernel::LinearExtrudeInput oneSide(
    kernel::PlanarProfileInput profile,
    double distance,
    kernel::SolidBooleanOperation operation =
        kernel::SolidBooleanOperation::add) {
    kernel::LinearExtrudeInput result;
    result.profile = std::move(profile);
    if (distance > 0.0) {
        result.start_offset_mm = 0.0;
        result.end_offset_mm = distance;
        result.start_cap_role =
            kernel::ExtrudeCapRole::profile_cap;
        result.end_cap_role =
            kernel::ExtrudeCapRole::extent_cap;
    } else {
        result.start_offset_mm = distance;
        result.end_offset_mm = 0.0;
        result.start_cap_role =
            kernel::ExtrudeCapRole::extent_cap;
        result.end_cap_role =
            kernel::ExtrudeCapRole::profile_cap;
    }
    result.operation = operation;
    CHECK(result.valid());
    return result;
}

kernel::LinearExtrudeInput midplane(
    kernel::PlanarProfileInput profile,
    double total_distance) {
    kernel::LinearExtrudeInput result;
    result.profile = std::move(profile);
    result.start_offset_mm =
        -total_distance * 0.5;
    result.end_offset_mm =
        total_distance * 0.5;
    result.start_cap_role =
        kernel::ExtrudeCapRole::negative_cap;
    result.end_cap_role =
        kernel::ExtrudeCapRole::positive_cap;
    result.operation =
        kernel::SolidBooleanOperation::add;
    CHECK(result.valid());
    return result;
}

const kernel::NewSurfaceLineage*
newCap(
    const kernel::SolidModelingResult& result,
    kernel::ExtrudeCapRole role) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [role](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::cap &&
                       surface.role.cap_role ==
                           std::optional<kernel::ExtrudeCapRole>{role};
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::NewSurfaceLineage*
newSide(
    const kernel::SolidModelingResult& result,
    std::string_view source) {
    const auto found =
        std::find_if(
            result.new_surfaces.begin(),
            result.new_surfaces.end(),
            [source](const auto& surface) {
                return surface.role.kind ==
                           kernel::ExtrudeGeneratedFaceRoleKind::side &&
                       surface.role.side_provenance &&
                       surface.role.side_provenance->source_entity ==
                           source;
            });
    return found == result.new_surfaces.end()
        ? nullptr
        : &*found;
}

const kernel::InheritedSurfaceLineage*
inheritedSurface(
    const kernel::SolidModelingResult& result,
    kernel::RuntimeSurfaceToken token) {
    const auto found =
        std::find_if(
            result.inherited_surfaces.begin(),
            result.inherited_surfaces.end(),
            [token](const auto& surface) {
                return surface.token == token;
            });
    return found == result.inherited_surfaces.end()
        ? nullptr
        : &*found;
}

void verifyResolvedSingle(
    const kernel::NewSurfaceLineage& surface) {
    CHECK(
        surface.surface_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        surface.strict_face_status ==
        kernel::ReferenceStatus::resolved);
    CHECK(surface.candidate_face_count == 1U);
    CHECK(surface.resolved_token.has_value());
    CHECK(surface.resolved_token->valid());
    CHECK(surface.current_faces.size() == 1U);
    CHECK(surface.current_faces.front().valid());
}

void verifyPristineAndFrames(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "base"),
                10.0));
    CHECK(base.ok());
    CHECK(base.new_surfaces.size() == 6U);

    for (const auto& surface : base.new_surfaces) {
        verifyResolvedSingle(surface);
    }

    const auto* profile_cap =
        newCap(
            base,
            kernel::ExtrudeCapRole::profile_cap);
    const auto* extent_cap =
        newCap(
            base,
            kernel::ExtrudeCapRole::extent_cap);
    const auto* bottom =
        newSide(base, "base-bottom");
    CHECK(profile_cap != nullptr);
    CHECK(extent_cap != nullptr);
    CHECK(bottom != nullptr);

    CHECK(
        profile_cap->surface_kind ==
        kernel::SurfaceKind::plane);
    CHECK(profile_cap->canonical_frame.has_value());
    CHECK(near(*profile_cap->canonical_frame, kernel::Frame3{}));

    kernel::Frame3 end_expected;
    end_expected.origin = {0.0, 0.0, 10.0};
    CHECK(extent_cap->canonical_frame.has_value());
    CHECK(near(*extent_cap->canonical_frame, end_expected));

    kernel::Frame3 bottom_expected;
    bottom_expected.origin = {0.0, 0.0, 0.0};
    bottom_expected.u_axis = {1.0, 0.0, 0.0};
    bottom_expected.v_axis = {0.0, 0.0, 1.0};
    bottom_expected.normal = {0.0, -1.0, 0.0};
    CHECK(bottom->canonical_frame.has_value());
    CHECK(near(*bottom->canonical_frame, bottom_expected));

    const auto reverse =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "reverse"),
                -10.0));
    CHECK(reverse.ok());
    const auto* reverse_bottom =
        newSide(reverse, "reverse-bottom");
    const auto* reverse_extent =
        newCap(
            reverse,
            kernel::ExtrudeCapRole::extent_cap);
    CHECK(reverse_bottom != nullptr);
    CHECK(reverse_extent != nullptr);
    CHECK(reverse_bottom->canonical_frame.has_value());
    CHECK(
        near(
            *reverse_bottom->canonical_frame,
            bottom_expected));

    kernel::Frame3 reverse_extent_expected;
    reverse_extent_expected.origin = {0.0, 0.0, -10.0};
    CHECK(reverse_extent->canonical_frame.has_value());
    CHECK(
        near(
            *reverse_extent->canonical_frame,
            reverse_extent_expected));

    const auto centered =
        provider.extrude(
            midplane(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "mid"),
                10.0));
    CHECK(centered.ok());
    const auto* mid_bottom =
        newSide(centered, "mid-bottom");
    CHECK(mid_bottom != nullptr);
    CHECK(mid_bottom->canonical_frame.has_value());
    CHECK(near(*mid_bottom->canonical_frame, bottom_expected));

    kernel::Frame3 negative_expected;
    negative_expected.origin = {0.0, 0.0, -5.0};
    kernel::Frame3 positive_expected;
    positive_expected.origin = {0.0, 0.0, 5.0};
    const auto* negative =
        newCap(
            centered,
            kernel::ExtrudeCapRole::negative_cap);
    const auto* positive =
        newCap(
            centered,
            kernel::ExtrudeCapRole::positive_cap);
    CHECK(negative != nullptr);
    CHECK(positive != nullptr);
    CHECK(negative->canonical_frame.has_value());
    CHECK(positive->canonical_frame.has_value());
    CHECK(near(*negative->canonical_frame, negative_expected));
    CHECK(near(*positive->canonical_frame, positive_expected));

    const auto cylinder =
        provider.extrude(
            oneSide(
                circleProfile(
                    0.0,
                    "circle"),
                10.0));
    CHECK(cylinder.ok());
    const auto* curved =
        newSide(cylinder, "circle");
    CHECK(curved != nullptr);
    verifyResolvedSingle(*curved);
    CHECK(
        curved->surface_kind ==
        kernel::SurfaceKind::cylinder);
    CHECK(!curved->canonical_frame.has_value());

    kernel::Frame3 xz;
    xz.u_axis = {1.0, 0.0, 0.0};
    xz.v_axis = {0.0, 0.0, 1.0};
    xz.normal = {0.0, -1.0, 0.0};
    CHECK(xz.valid());
    const auto xz_result =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "xz",
                    xz),
                12.0));
    CHECK(xz_result.ok());
    const auto* xz_bottom =
        newSide(xz_result, "xz-bottom");
    CHECK(xz_bottom != nullptr);
    CHECK(xz_bottom->canonical_frame.has_value());
    kernel::Frame3 xz_expected;
    xz_expected.u_axis = {1.0, 0.0, 0.0};
    xz_expected.v_axis = {0.0, -1.0, 0.0};
    xz_expected.normal = {0.0, 0.0, -1.0};
    CHECK(near(*xz_bottom->canonical_frame, xz_expected));

    kernel::Frame3 yz;
    yz.u_axis = {0.0, 1.0, 0.0};
    yz.v_axis = {0.0, 0.0, 1.0};
    yz.normal = {1.0, 0.0, 0.0};
    CHECK(yz.valid());
    const auto yz_result =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "yz",
                    yz),
                8.0));
    CHECK(yz_result.ok());
    const auto* yz_bottom =
        newSide(yz_result, "yz-bottom");
    CHECK(yz_bottom != nullptr);
    CHECK(yz_bottom->canonical_frame.has_value());
    kernel::Frame3 yz_expected;
    yz_expected.u_axis = {0.0, 1.0, 0.0};
    yz_expected.v_axis = {1.0, 0.0, 0.0};
    yz_expected.normal = {0.0, 0.0, -1.0};
    CHECK(near(*yz_bottom->canonical_frame, yz_expected));
}

void verifySemanticEditSurvival(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto baseline =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "stable"),
                10.0));
    CHECK(baseline.ok());

    const auto* baseline_profile =
        newCap(
            baseline,
            kernel::ExtrudeCapRole::profile_cap);
    const auto* baseline_extent =
        newCap(
            baseline,
            kernel::ExtrudeCapRole::extent_cap);
    const auto* baseline_bottom =
        newSide(
            baseline,
            "stable-bottom");
    CHECK(baseline_profile != nullptr);
    CHECK(baseline_extent != nullptr);
    CHECK(baseline_bottom != nullptr);
    CHECK(baseline_profile->canonical_frame.has_value());
    CHECK(baseline_extent->canonical_frame.has_value());
    CHECK(baseline_bottom->canonical_frame.has_value());

    // Extent-distance edit: semantic roles stay resolved; only the extent-cap
    // origin follows the semantic offset.
    const auto taller =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "stable"),
                18.0));
    CHECK(taller.ok());
    const auto* taller_profile =
        newCap(
            taller,
            kernel::ExtrudeCapRole::profile_cap);
    const auto* taller_extent =
        newCap(
            taller,
            kernel::ExtrudeCapRole::extent_cap);
    const auto* taller_bottom =
        newSide(
            taller,
            "stable-bottom");
    CHECK(taller_profile != nullptr);
    CHECK(taller_extent != nullptr);
    CHECK(taller_bottom != nullptr);
    verifyResolvedSingle(*taller_profile);
    verifyResolvedSingle(*taller_extent);
    verifyResolvedSingle(*taller_bottom);
    CHECK(
        near(
            *taller_profile->canonical_frame,
            *baseline_profile->canonical_frame));
    CHECK(
        near(
            *taller_bottom->canonical_frame,
            *baseline_bottom->canonical_frame));
    kernel::Frame3 taller_extent_expected;
    taller_extent_expected.origin =
        {0.0, 0.0, 18.0};
    CHECK(
        near(
            *taller_extent->canonical_frame,
            taller_extent_expected));

    // Authored line-length edit: same source provenance and authored direction
    // remain the same semantic planar side carrier.
    const auto longer =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    60.0, 20.0,
                    0.0,
                    "stable"),
                10.0));
    CHECK(longer.ok());
    const auto* longer_bottom =
        newSide(
            longer,
            "stable-bottom");
    CHECK(longer_bottom != nullptr);
    verifyResolvedSingle(*longer_bottom);
    CHECK(
        near(
            *longer_bottom->canonical_frame,
            *baseline_bottom->canonical_frame));

    // Profile/support translation moves the semantic frame origin but not its
    // canonical orientation.
    kernel::Frame3 translated_frame;
    translated_frame.origin =
        {7.0, -3.0, 4.0};
    const auto translated =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "stable",
                    translated_frame),
                10.0));
    CHECK(translated.ok());
    const auto* translated_bottom =
        newSide(
            translated,
            "stable-bottom");
    CHECK(translated_bottom != nullptr);
    verifyResolvedSingle(*translated_bottom);
    CHECK(translated_bottom->canonical_frame.has_value());
    kernel::Frame3 translated_expected =
        *baseline_bottom->canonical_frame;
    translated_expected.origin =
        {7.0, -3.0, 4.0};
    CHECK(
        near(
            *translated_bottom->canonical_frame,
            translated_expected));

    // Legal loop traversal reversal changes B-Rep traversal only. Authored
    // Line direction/provenance remains frame authority.
    const auto reversed =
        provider.extrude(
            oneSide(
                reversedRectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "stable"),
                10.0));
    CHECK(reversed.ok());
    for (const auto* source :
         {
             "stable-bottom",
             "stable-right",
             "stable-top",
             "stable-left",
         }) {
        const auto* before =
            newSide(baseline, source);
        const auto* after =
            newSide(reversed, source);
        CHECK(before != nullptr);
        CHECK(after != nullptr);
        verifyResolvedSingle(*after);
        CHECK(before->surface_kind == after->surface_kind);
        CHECK(before->canonical_frame.has_value());
        CHECK(after->canonical_frame.has_value());
        CHECK(
            near(
                *before->canonical_frame,
                *after->canonical_frame));
    }

    // Cold provider reconstruction: runtime token numbers are irrelevant;
    // semantic roles, classification and canonical frames repeat.
    kernel_occt::OcctSolidModelingKernel cold_provider;
    const auto cold =
        cold_provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "stable"),
                10.0));
    CHECK(cold.ok());
    for (const auto& before :
         baseline.new_surfaces) {
        const kernel::NewSurfaceLineage* after =
            nullptr;
        if (before.role.kind ==
            kernel::ExtrudeGeneratedFaceRoleKind::cap) {
            CHECK(before.role.cap_role.has_value());
            after =
                newCap(
                    cold,
                    *before.role.cap_role);
        } else {
            CHECK(before.role.side_provenance.has_value());
            after =
                newSide(
                    cold,
                    before.role.side_provenance
                        ->source_entity);
        }
        CHECK(after != nullptr);
        CHECK(
            after->surface_status ==
            before.surface_status);
        CHECK(
            after->strict_face_status ==
            before.strict_face_status);
        CHECK(
            after->candidate_face_count ==
            before.candidate_face_count);
        CHECK(
            after->surface_kind ==
            before.surface_kind);
        CHECK(
            after->canonical_frame ==
            before.canonical_frame);
    }
}

void verifyTrimSplitDeleteRecreate(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto make_base = [&provider]() {
        return provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 20.0,
                    0.0,
                    "base"),
                10.0));
    };

    {
        const auto base = make_base();
        CHECK(base.ok());
        const auto* cap =
            newCap(
                base,
                kernel::ExtrudeCapRole::extent_cap);
        CHECK(cap != nullptr);
        CHECK(cap->resolved_token.has_value());
        const auto token = *cap->resolved_token;
        const auto frame = cap->canonical_frame;

        const auto attached =
            provider.extrude(
                oneSide(
                    rectangle(
                        10.0, 5.0,
                        30.0, 15.0,
                        10.0,
                        "add-trim"),
                    10.0),
                base.solid);
        CHECK(attached.ok());

        const auto* inherited =
            inheritedSurface(attached, token);
        CHECK(inherited != nullptr);
        CHECK(
            inherited->surface_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            inherited->strict_face_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(inherited->candidate_face_count == 1U);
        CHECK(inherited->current_faces.size() == 1U);
        CHECK(inherited->canonical_frame == frame);
    }

    {
        const auto base = make_base();
        CHECK(base.ok());
        const auto* cap =
            newCap(
                base,
                kernel::ExtrudeCapRole::extent_cap);
        CHECK(cap != nullptr);
        CHECK(cap->resolved_token.has_value());
        const auto token = *cap->resolved_token;
        const auto frame = cap->canonical_frame;

        const auto trimmed =
            provider.extrude(
                oneSide(
                    rectangle(
                        30.0, 5.0,
                        45.0, 15.0,
                        5.0,
                        "cut-trim"),
                    10.0,
                    kernel::SolidBooleanOperation::cut),
                base.solid);
        CHECK(trimmed.ok());

        const auto* inherited =
            inheritedSurface(trimmed, token);
        CHECK(inherited != nullptr);
        CHECK(
            inherited->surface_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            inherited->strict_face_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(inherited->candidate_face_count == 1U);
        CHECK(inherited->current_faces.size() == 1U);
        CHECK(inherited->canonical_frame == frame);
    }

    {
        const auto base = make_base();
        CHECK(base.ok());
        const auto* cap =
            newCap(
                base,
                kernel::ExtrudeCapRole::extent_cap);
        CHECK(cap != nullptr);
        CHECK(cap->resolved_token.has_value());
        const auto token = *cap->resolved_token;
        const auto frame = cap->canonical_frame;

        const auto split =
            provider.extrude(
                oneSide(
                    rectangle(
                        15.0, -5.0,
                        25.0, 25.0,
                        8.0,
                        "cut-split"),
                    7.0,
                    kernel::SolidBooleanOperation::cut),
                base.solid);
        CHECK(split.ok());

        const auto* inherited =
            inheritedSurface(split, token);
        CHECK(inherited != nullptr);
        CHECK(
            inherited->surface_status ==
            kernel::ReferenceStatus::resolved);
        CHECK(
            inherited->strict_face_status ==
            kernel::ReferenceStatus::ambiguous);
        CHECK(inherited->candidate_face_count == 2U);
        CHECK(inherited->current_faces.size() == 2U);
        CHECK(inherited->canonical_frame == frame);
    }

    {
        const auto base = make_base();
        CHECK(base.ok());
        const auto* old_cap =
            newCap(
                base,
                kernel::ExtrudeCapRole::extent_cap);
        CHECK(old_cap != nullptr);
        CHECK(old_cap->resolved_token.has_value());
        CHECK(old_cap->canonical_frame.has_value());
        const auto old_token =
            *old_cap->resolved_token;
        const auto old_frame =
            *old_cap->canonical_frame;

        const auto deleted =
            provider.extrude(
                oneSide(
                    rectangle(
                        -5.0, -5.0,
                        45.0, 25.0,
                        8.0,
                        "delete"),
                    7.0,
                    kernel::SolidBooleanOperation::cut),
                base.solid);
        CHECK(deleted.ok());

        const auto* missing =
            inheritedSurface(
                deleted,
                old_token);
        CHECK(missing != nullptr);
        CHECK(
            missing->surface_status ==
            kernel::ReferenceStatus::missing);
        CHECK(
            missing->strict_face_status ==
            kernel::ReferenceStatus::missing);
        CHECK(missing->candidate_face_count == 0U);
        CHECK(missing->current_faces.empty());
        CHECK(!missing->canonical_frame.has_value());

        const auto recreated =
            provider.extrude(
                oneSide(
                    rectangle(
                        0.0, 0.0,
                        40.0, 20.0,
                        8.0,
                        "replacement"),
                    2.0),
                deleted.solid);
        CHECK(recreated.ok());

        CHECK(
            std::none_of(
                recreated.inherited_surfaces.begin(),
                recreated.inherited_surfaces.end(),
                [old_token](const auto& surface) {
                    return surface.token == old_token;
                }));

        const auto* replacement =
            newCap(
                recreated,
                kernel::ExtrudeCapRole::extent_cap);
        CHECK(replacement != nullptr);
        verifyResolvedSingle(*replacement);
        CHECK(replacement->resolved_token.has_value());
        CHECK(*replacement->resolved_token != old_token);
        CHECK(replacement->canonical_frame.has_value());
        CHECK(
            near(
                *replacement->canonical_frame,
                old_frame));
    }
}

void verifyCutExposed(
    kernel_occt::OcctSolidModelingKernel& provider) {
    const auto base =
        provider.extrude(
            oneSide(
                rectangle(
                    0.0, 0.0,
                    40.0, 30.0,
                    0.0,
                    "base"),
                10.0));
    CHECK(base.ok());

    const auto cut =
        provider.extrude(
            oneSide(
                rectangle(
                    10.0, 10.0,
                    30.0, 20.0,
                    5.0,
                    "cut-tool"),
                10.0,
                kernel::SolidBooleanOperation::cut),
            base.solid);
    CHECK(cut.ok());

    const auto* exposed =
        newSide(
            cut,
            "cut-tool-bottom");
    CHECK(exposed != nullptr);
    verifyResolvedSingle(*exposed);
    CHECK(
        exposed->surface_kind ==
        kernel::SurfaceKind::plane);
    CHECK(exposed->canonical_frame.has_value());

    kernel::Frame3 expected;
    expected.origin = {10.0, 10.0, 5.0};
    expected.u_axis = {1.0, 0.0, 0.0};
    expected.v_axis = {0.0, 0.0, 1.0};
    expected.normal = {0.0, -1.0, 0.0};
    CHECK(near(*exposed->canonical_frame, expected));
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel provider;

    verifyPristineAndFrames(provider);
    verifySemanticEditSurvival(provider);
    verifyTrimSplitDeleteRecreate(provider);
    verifyCutExposed(provider);

    std::cout
        << "PM02B_PRODUCTION_SURFACE_LINEAGE_PASS"
        << " false_resolved=0"
        << " split_face=ambiguous"
        << " split_surface=resolved"
        << " deleted_surface=missing_no_frame"
        << " cut_exposed_surface=resolved"
        << " edit_survival=pass"
        << " traversal_reversal=pass"
        << " cold_rebuild=pass"
        << '\n';
    return EXIT_SUCCESS;
}
