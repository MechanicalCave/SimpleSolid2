#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace simplesolid2::kernel {

struct Point2 final {
    double u{};
    double v{};

    friend bool operator==(const Point2&, const Point2&) = default;
};

struct Point3 final {
    double x{};
    double y{};
    double z{};

    friend bool operator==(const Point3&, const Point3&) = default;
};

struct Frame3 final {
    Point3 origin;
    Point3 u_axis{1.0, 0.0, 0.0};
    Point3 v_axis{0.0, 1.0, 0.0};
    Point3 normal{0.0, 0.0, 1.0};

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(const Frame3&, const Frame3&) = default;
};

struct Line2 final {
    Point2 start;
    Point2 end;

    friend bool operator==(const Line2&, const Line2&) = default;
};

struct Circle2 final {
    Point2 center;
    double radius{};

    friend bool operator==(const Circle2&, const Circle2&) = default;
};

struct Arc2 final {
    Point2 center;
    double radius{};
    double start_angle{};
    double sweep_angle{};

    friend bool operator==(const Arc2&, const Arc2&) = default;
};

using Curve2 = std::variant<Line2, Circle2, Arc2>;

struct BoundaryUseProvenance final {
    std::string source_entity;
    std::uint32_t loop_index{};
    std::uint32_t use_index{};
    bool hole{false};

    friend bool operator==(
        const BoundaryUseProvenance&,
        const BoundaryUseProvenance&) = default;
};

struct BoundaryUse2D final {
    Curve2 curve;
    double start_parameter{};
    double end_parameter{};
    bool follows_source_direction{true};
    bool crosses_closed_seam{false};
    bool whole_closed_curve{false};
    BoundaryUseProvenance provenance;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const BoundaryUse2D&,
        const BoundaryUse2D&) = default;
};

struct ProfileLoopInput final {
    std::vector<BoundaryUse2D> boundary;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const ProfileLoopInput&,
        const ProfileLoopInput&) = default;
};

struct PlanarProfileInput final {
    Frame3 frame;
    ProfileLoopInput outer;
    std::vector<ProfileLoopInput> holes;

    [[nodiscard]] bool valid() const noexcept;

    friend bool operator==(
        const PlanarProfileInput&,
        const PlanarProfileInput&) = default;
};

} // namespace simplesolid2::kernel
