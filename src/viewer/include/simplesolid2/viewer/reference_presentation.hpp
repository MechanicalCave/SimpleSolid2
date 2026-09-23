#pragma once

#include <simplesolid2/viewer/math3.hpp>

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

        if (kind != ReferencePresentationKind::plane) {
            return true;
        }

        if (!finite(v_axis) ||
            v_axis.squaredLength() <= 1.0e-24) {
            return false;
        }

        return cross(u_axis, v_axis).squaredLength() > 1.0e-24;
    }
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
    std::optional<GridPresentation> grid;

    [[nodiscard]] bool valid() const noexcept {
        if (grid && !grid->valid()) {
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

        return true;
    }
};

} // namespace simplesolid2::viewer
