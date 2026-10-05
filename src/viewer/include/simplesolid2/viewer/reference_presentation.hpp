#pragma once

#include <simplesolid2/viewer/math3.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <vector>

namespace simplesolid2::viewer {

struct PresentationToken final {
    std::uint64_t value{};

    [[nodiscard]] constexpr bool valid() const noexcept {
        return value != 0U;
    }

    friend constexpr bool operator==(
        const PresentationToken&,
        const PresentationToken&) = default;
};

enum class ReferencePresentationKind : std::uint8_t {
    point,
    x_axis,
    y_axis,
    z_axis,
    plane,
    datum_plane,
};

enum class PresentationRole : std::uint8_t {
    base,
    hover,
    secondary_selection,
    primary_selection,
};

struct ReferencePresentation final {
    PresentationToken token;
    ReferencePresentationKind kind{ReferencePresentationKind::point};
    Point3 origin{};
    Vec3 u_axis{};
    Vec3 v_axis{};
    double extent{1.0};
    PresentationRole role{PresentationRole::base};
    bool visible{true};

    [[nodiscard]] bool valid() const noexcept {
        if (!token.valid() ||
            !finite(origin) ||
            !std::isfinite(extent) ||
            extent <= 0.0) {
            return false;
        }

        if (kind == ReferencePresentationKind::point) {
            return true;
        }

        if (!finite(u_axis) ||
            u_axis.squaredLength() <= 1.0e-24) {
            return false;
        }

        if (kind != ReferencePresentationKind::plane &&
            kind != ReferencePresentationKind::datum_plane) {
            return true;
        }

        if (!finite(v_axis) ||
            v_axis.squaredLength() <= 1.0e-24) {
            return false;
        }

        return cross(u_axis, v_axis).squaredLength() > 1.0e-24;
    }
};

struct ReferenceLineSegmentPresentation final {
    Point3 first;
    Point3 second;

    [[nodiscard]] bool valid() const noexcept {
        return finite(first) &&
               finite(second) &&
               first != second;
    }

    friend bool operator==(
        const ReferenceLineSegmentPresentation&,
        const ReferenceLineSegmentPresentation&) = default;
};

// Presentation-only geometry owned by one ReferencePresentation. It has no
// presentation token or semantic identity of its own: provider picking must
// resolve every rendered segment to owner. This is the PM-03D boundary that
// prevents a Datum/Body intersection cue from becoming Edge/Curve authority.
struct ReferenceOwnedLineOverlay final {
    PresentationToken owner;
    std::vector<ReferenceLineSegmentPresentation>
        segments;

    [[nodiscard]] bool valid() const noexcept {
        return owner.valid() &&
               !segments.empty() &&
               std::all_of(
                   segments.begin(),
                   segments.end(),
                   [](const auto& segment) {
                       return segment.valid();
                   });
    }

    friend bool operator==(
        const ReferenceOwnedLineOverlay&,
        const ReferenceOwnedLineOverlay&) = default;
};

// Transient, non-pickable Datum Plane draft presentation. Unlike a
// ReferencePresentation it deliberately has no PresentationToken and therefore
// cannot become semantic selection or durable CAD identity.
struct ReferencePlanePreviewPresentation final {
    Point3 origin{};
    Vec3 u_axis{};
    Vec3 v_axis{};
    double extent{1.0};
    std::vector<ReferenceLineSegmentPresentation>
        intersection_segments;

    [[nodiscard]] bool valid() const noexcept {
        if (!finite(origin) ||
            !finite(u_axis) ||
            !finite(v_axis) ||
            u_axis.squaredLength() <= 1.0e-24 ||
            v_axis.squaredLength() <= 1.0e-24 ||
            cross(u_axis, v_axis).squaredLength() <=
                1.0e-24 ||
            !std::isfinite(extent) ||
            extent <= 0.0) {
            return false;
        }
        return std::all_of(
            intersection_segments.begin(),
            intersection_segments.end(),
            [](const auto& segment) {
                return segment.valid();
            });
    }

    friend bool operator==(
        const ReferencePlanePreviewPresentation&,
        const ReferencePlanePreviewPresentation&) = default;
};

struct GridPresentation final {
    Point3 origin{};
    Vec3 u_axis{1.0, 0.0, 0.0};
    Vec3 v_axis{0.0, 1.0, 0.0};
    double extent{100.0};
    double spacing{10.0};
    std::uint32_t major_every{5U};
    bool visible{true};

    [[nodiscard]] bool valid() const noexcept {
        return finite(origin) &&
               finite(u_axis) &&
               finite(v_axis) &&
               u_axis.squaredLength() > 1.0e-24 &&
               v_axis.squaredLength() > 1.0e-24 &&
               cross(u_axis, v_axis).squaredLength() > 1.0e-24 &&
               std::isfinite(extent) &&
               extent > 0.0 &&
               std::isfinite(spacing) &&
               spacing > 0.0 &&
               spacing <= extent &&
               major_every > 0U;
    }
};

struct ReferenceScene final {
    std::vector<ReferencePresentation> references;
    std::vector<ReferenceOwnedLineOverlay> overlays;
    std::optional<ReferencePlanePreviewPresentation> preview;
    std::optional<GridPresentation> grid;

    [[nodiscard]] bool valid() const noexcept {
        if (grid && !grid->valid()) {
            return false;
        }
        if (preview && !preview->valid()) {
            return false;
        }

        for (const auto& reference : references) {
            if (!reference.valid()) {
                return false;
            }
        }

        for (std::size_t left = 0; left < references.size(); ++left) {
            for (std::size_t right = left + 1U;
                 right < references.size();
                 ++right) {
                if (references[left].token ==
                    references[right].token) {
                    return false;
                }
            }
        }

        for (std::size_t index = 0U;
             index < overlays.size();
             ++index) {
            const auto& overlay = overlays[index];
            if (!overlay.valid()) {
                return false;
            }

            const auto owner =
                std::find_if(
                    references.begin(),
                    references.end(),
                    [&overlay](const auto& reference) {
                        return reference.token ==
                               overlay.owner;
                    });
            if (owner == references.end() ||
                owner->kind !=
                    ReferencePresentationKind::datum_plane ||
                !owner->visible) {
                return false;
            }

            for (std::size_t other = index + 1U;
                 other < overlays.size();
                 ++other) {
                if (overlays[other].owner ==
                    overlay.owner) {
                    return false;
                }
            }
        }

        return true;
    }
};

} // namespace simplesolid2::viewer
