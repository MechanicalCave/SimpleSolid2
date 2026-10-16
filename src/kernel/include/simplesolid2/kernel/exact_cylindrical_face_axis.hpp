// RG-02C/C1-P1-A: provider-neutral, read-only exact native cylinder axis.
// The Face token and scoped Body are evaluation-local; neither is Part intent.
#pragma once

#include <simplesolid2/kernel/solid_modeling.hpp>

#include <cmath>
#include <optional>
#include <utility>

namespace simplesolid2::kernel {

enum class ExactCylindricalFaceAxisStatus {
    ok,
    invalid_input,
    provider_mismatch,
    face_unavailable,
    non_cylindrical_surface,
    invalid_cylinder,
    kernel_failure,
};

// Infinite UNORIENTED world-mm line. Any point on the same line is valid;
// {+direction, -direction} have precisely the same geometric meaning.
struct ExactCylindricalFaceAxisGeometry final {
    Point3 point_on_axis_world_mm;
    Point3 unoriented_unit_direction;
    double radius_mm{};

    [[nodiscard]] bool valid() const noexcept {
        const auto finite = [](const Point3& point) noexcept {
            return std::isfinite(point.x) &&
                   std::isfinite(point.y) &&
                   std::isfinite(point.z);
        };
        if (!finite(point_on_axis_world_mm) ||
            !finite(unoriented_unit_direction) ||
            !std::isfinite(radius_mm) || !(radius_mm > 0.0)) {
            return false;
        }
        const auto& d = unoriented_unit_direction;
        const double length_squared =
            d.x * d.x + d.y * d.y + d.z * d.z;
        return std::isfinite(length_squared) &&
               std::abs(length_squared - 1.0) <= 1e-8;
    }
};

struct ExactCylindricalFaceAxisResult final {
    ExactCylindricalFaceAxisStatus status{
        ExactCylindricalFaceAxisStatus::invalid_input};
    std::optional<ExactCylindricalFaceAxisGeometry> geometry;

    [[nodiscard]] bool ok() const noexcept {
        return status == ExactCylindricalFaceAxisStatus::ok &&
               geometry.has_value() && geometry->valid();
    }
};

// Only the provider may mint a scoped Face after checking membership in the
// ACTUAL current Body inventory; a reused bare integer is never authority.
class ScopedExactCylindricalFace final {
public:
    ScopedExactCylindricalFace() = default;

    [[nodiscard]] bool valid() const noexcept {
        return source_body_ != nullptr && face_.valid();
    }
    [[nodiscard]] const RuntimeSolidHandle&
    sourceBody() const noexcept {
        return source_body_;
    }
    [[nodiscard]] RuntimeFaceToken face() const noexcept {
        return face_;
    }

private:
    friend class IExactCylindricalFaceAxisQuery;
    ScopedExactCylindricalFace(
        RuntimeSolidHandle body, RuntimeFaceToken face) noexcept
        : source_body_{std::move(body)}, face_{face} {}

    RuntimeSolidHandle source_body_;
    RuntimeFaceToken face_;
};

class IExactCylindricalFaceAxisQuery {
protected:
    [[nodiscard]] static ScopedExactCylindricalFace
    makeScopedExactCylindricalFace(
        RuntimeSolidHandle body, RuntimeFaceToken token) noexcept {
        return {std::move(body), token};
    }

public:
    virtual ~IExactCylindricalFaceAxisQuery() = default;

    [[nodiscard]] virtual std::optional<ScopedExactCylindricalFace>
    bindExactCylindricalFaceToBody(
        RuntimeSolidHandle source_body,
        RuntimeFaceToken current_face) noexcept = 0;

    [[nodiscard]] virtual ExactCylindricalFaceAxisResult
    queryExactCylindricalFaceAxis(
        RuntimeSolidHandle current_body,
        const ScopedExactCylindricalFace& bound_face) noexcept = 0;
};

} // namespace simplesolid2::kernel
