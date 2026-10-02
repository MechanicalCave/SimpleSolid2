#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <cmath>
#include <numbers>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

namespace simplesolid2::kernel_occt {
namespace {

constexpr double full_turn =
    2.0 * std::numbers::pi_v<double>;

[[nodiscard]] gp_Pnt point3(
    const kernel::Frame3& frame,
    const kernel::Point2& point) {
    return {
        frame.origin.x +
            frame.u_axis.x * point.u +
            frame.v_axis.x * point.v,
        frame.origin.y +
            frame.u_axis.y * point.u +
            frame.v_axis.y * point.v,
        frame.origin.z +
            frame.u_axis.z * point.u +
            frame.v_axis.z * point.v,
    };
}

[[nodiscard]] kernel::Point2 linePoint(
    const kernel::Line2& line,
    double parameter) noexcept {
    return {
        line.start.u +
            (line.end.u - line.start.u) *
                parameter,
        line.start.v +
            (line.end.v - line.start.v) *
                parameter,
    };
}

[[nodiscard]] gp_Ax2 circleAxis(
    const kernel::PlanarProfileInput& input,
    const kernel::Point2& center) {
    return {
        point3(input.frame, center),
        gp_Dir{
            input.frame.normal.x,
            input.frame.normal.y,
            input.frame.normal.z},
        gp_Dir{
            input.frame.u_axis.x,
            input.frame.u_axis.y,
            input.frame.u_axis.z},
    };
}

[[nodiscard]] double circleDelta(
    const kernel::BoundaryUse2D& use) noexcept {
    if (use.whole_closed_curve) {
        return use.follows_source_direction
            ? full_turn
            : -full_turn;
    }

    const double from =
        use.follows_source_direction
            ? use.start_parameter
            : use.end_parameter;
    const double to =
        use.follows_source_direction
            ? use.end_parameter
            : use.start_parameter;

    double delta =
        use.crosses_closed_seam
            ? (1.0 - from) + to
            : to - from;
    delta *= full_turn;
    if (!use.follows_source_direction) {
        delta = -delta;
    }
    return delta;
}

[[nodiscard]] std::optional<TopoDS_Edge>
circularEdge(
    const gp_Circ& circle,
    double start_angle,
    double delta,
    bool whole_closed_curve) {
    if (whole_closed_curve) {
        BRepBuilderAPI_MakeEdge make_edge{circle};
        if (!make_edge.IsDone()) {
            return std::nullopt;
        }
        auto edge = make_edge.Edge();
        if (delta < 0.0) {
            edge.Reverse();
        }
        return edge;
    }

    if (!std::isfinite(start_angle) ||
        !std::isfinite(delta) ||
        delta == 0.0) {
        return std::nullopt;
    }

    const double end_angle =
        start_angle + delta;
    if (delta > 0.0) {
        BRepBuilderAPI_MakeEdge make_edge{
            circle,
            start_angle,
            end_angle};
        return make_edge.IsDone()
            ? std::optional<TopoDS_Edge>{
                  make_edge.Edge()}
            : std::nullopt;
    }

    BRepBuilderAPI_MakeEdge make_edge{
        circle,
        end_angle,
        start_angle};
    if (!make_edge.IsDone()) {
        return std::nullopt;
    }
    auto edge = make_edge.Edge();
    edge.Reverse();
    return edge;
}

[[nodiscard]] std::optional<TopoDS_Edge>
buildEdge(
    const kernel::PlanarProfileInput& input,
    const kernel::BoundaryUse2D& use) {
    return std::visit(
        [&input, &use](const auto& curve)
            -> std::optional<TopoDS_Edge> {
            using T =
                std::decay_t<decltype(curve)>;

            if constexpr (
                std::is_same_v<T, kernel::Line2>) {
                if (use.whole_closed_curve) {
                    return std::nullopt;
                }
                const auto start =
                    linePoint(
                        curve,
                        use.start_parameter);
                const auto end =
                    linePoint(
                        curve,
                        use.end_parameter);
                BRepBuilderAPI_MakeEdge make_edge{
                    point3(input.frame, start),
                    point3(input.frame, end)};
                return make_edge.IsDone()
                    ? std::optional<TopoDS_Edge>{
                          make_edge.Edge()}
                    : std::nullopt;
            } else if constexpr (
                std::is_same_v<T, kernel::Circle2>) {
                const gp_Circ circle{
                    circleAxis(
                        input,
                        curve.center),
                    curve.radius};
                const double start_angle =
                    full_turn *
                    use.start_parameter;
                return circularEdge(
                    circle,
                    start_angle,
                    circleDelta(use),
                    use.whole_closed_curve);
            } else {
                if (use.whole_closed_curve) {
                    return std::nullopt;
                }
                const gp_Circ circle{
                    circleAxis(
                        input,
                        curve.center),
                    curve.radius};
                const double start_angle =
                    curve.start_angle +
                    curve.sweep_angle *
                        use.start_parameter;
                const double delta =
                    curve.sweep_angle *
                    (use.end_parameter -
                     use.start_parameter);
                return circularEdge(
                    circle,
                    start_angle,
                    delta,
                    false);
            }
        },
        use.curve);
}

struct WireEvidence final {
    TopoDS_Wire wire;
    std::vector<kernel::BoundaryLineageEvidence>
        lineage;
};

[[nodiscard]] std::optional<WireEvidence>
buildWire(
    const kernel::PlanarProfileInput& input,
    const kernel::ProfileLoopInput& loop) {
    BRepBuilderAPI_MakeWire make_wire;
    WireEvidence result;
    result.lineage.reserve(loop.boundary.size());

    for (const auto& use : loop.boundary) {
        const auto edge =
            buildEdge(input, use);
        if (!edge) {
            return std::nullopt;
        }
        make_wire.Add(*edge);
        if (!make_wire.IsDone()) {
            return std::nullopt;
        }
        result.lineage.push_back({
            use.provenance,
            1U,
        });
    }

    result.wire = make_wire.Wire();
    return result;
}

[[nodiscard]] std::size_t countSubshapes(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    std::size_t count = 0U;
    for (TopExp_Explorer explorer{shape, kind};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

} // namespace

kernel::ShapeEvidence buildProfileFaceEvidence(
    const kernel::PlanarProfileInput& input) noexcept {
    kernel::ShapeEvidence evidence;
    if (!input.valid()) {
        evidence.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const auto outer =
            buildWire(
                input,
                input.outer);
        if (!outer) {
            evidence.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepBuilderAPI_MakeFace make_face{
            outer->wire,
            true};
        if (!make_face.IsDone()) {
            evidence.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        evidence.boundary_lineage =
            outer->lineage;

        for (const auto& hole :
             input.holes) {
            auto built =
                buildWire(input, hole);
            if (!built) {
                evidence.status =
                    kernel::EvidenceStatus::provider_failure;
                return evidence;
            }

            auto hole_wire =
                built->wire;
            hole_wire.Reverse();
            make_face.Add(hole_wire);

            evidence.boundary_lineage.insert(
                evidence.boundary_lineage.end(),
                built->lineage.begin(),
                built->lineage.end());
        }

        if (!make_face.IsDone()) {
            evidence.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const TopoDS_Face face =
            make_face.Face();
        const BRepCheck_Analyzer analyzer{face};
        evidence.brep_valid =
            analyzer.IsValid();
        evidence.face_count =
            countSubshapes(
                face,
                TopAbs_FACE);
        evidence.wire_count =
            countSubshapes(
                face,
                TopAbs_WIRE);
        evidence.edge_count =
            countSubshapes(
                face,
                TopAbs_EDGE);
        evidence.solid_count =
            countSubshapes(
                face,
                TopAbs_SOLID);

        if (!evidence.brep_valid) {
            evidence.status =
                kernel::EvidenceStatus::invalid_brep;
            return evidence;
        }

        evidence.status =
            kernel::EvidenceStatus::ok;
        return evidence;
    } catch (const Standard_Failure&) {
        evidence.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

} // namespace simplesolid2::kernel_occt
