#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepGProp.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepLib_ToolTriangulatedShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <Precision.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <GProp_GProps.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>
#include <GeomAbs_CurveType.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepSweep_Prism.hxx>
#include <BRepSweep_Revol.hxx>
#include <gp_Ax1.hxx>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <map>
#include <memory>
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

[[nodiscard]] kernel::Point3
add3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return {
        first.x + second.x,
        first.y + second.y,
        first.z + second.z};
}

[[nodiscard]] kernel::Point3
subtract3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return {
        first.x - second.x,
        first.y - second.y,
        first.z - second.z};
}

[[nodiscard]] kernel::Point3
scale3(
    const kernel::Point3& value,
    double scale) noexcept {
    return {
        value.x * scale,
        value.y * scale,
        value.z * scale};
}

[[nodiscard]] double
dot3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return first.x * second.x +
           first.y * second.y +
           first.z * second.z;
}

[[nodiscard]] kernel::Point3
cross3(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return {
        first.y * second.z -
            first.z * second.y,
        first.z * second.x -
            first.x * second.z,
        first.x * second.y -
            first.y * second.x};
}

[[nodiscard]] std::optional<kernel::Point3>
unit3(const kernel::Point3& value) noexcept {
    const double squared =
        dot3(value, value);
    if (!std::isfinite(squared) ||
        !(squared > 0.0)) {
        return std::nullopt;
    }
    const double length =
        std::sqrt(squared);
    if (!std::isfinite(length) ||
        !(length > 0.0)) {
        return std::nullopt;
    }
    return scale3(value, 1.0 / length);
}

[[nodiscard]] kernel::Point3
rotateVectorAroundAxis(
    const kernel::Point3& value,
    const kernel::Point3& unit_axis,
    double angle) noexcept {
    const double cosine = std::cos(angle);
    const double sine = std::sin(angle);
    return add3(
        add3(
            scale3(value, cosine),
            scale3(
                cross3(unit_axis, value),
                sine)),
        scale3(
            unit_axis,
            dot3(unit_axis, value) *
                (1.0 - cosine)));
}

[[nodiscard]] std::optional<kernel::Frame3>
rotatedFrame(
    const kernel::Frame3& source,
    const kernel::Axis3& axis,
    double angle) noexcept {
    const auto unit_axis =
        unit3(axis.direction);
    if (!source.valid() ||
        !axis.valid() ||
        !unit_axis ||
        !std::isfinite(angle)) {
        return std::nullopt;
    }

    kernel::Frame3 result;
    result.origin =
        add3(
            axis.origin,
            rotateVectorAroundAxis(
                subtract3(
                    source.origin,
                    axis.origin),
                *unit_axis,
                angle));
    result.u_axis =
        rotateVectorAroundAxis(
            source.u_axis,
            *unit_axis,
            angle);
    result.v_axis =
        rotateVectorAroundAxis(
            source.v_axis,
            *unit_axis,
            angle);
    result.normal =
        rotateVectorAroundAxis(
            source.normal,
            *unit_axis,
            angle);
    return result.valid()
        ? std::optional<kernel::Frame3>{
              result}
        : std::nullopt;
}

[[nodiscard]] std::optional<kernel::PlanarProfileInput>
rotatedProfile(
    const kernel::PlanarProfileInput& source,
    const kernel::Axis3& axis,
    double angle) noexcept {
    const auto frame =
        rotatedFrame(
            source.frame,
            axis,
            angle);
    if (!frame) {
        return std::nullopt;
    }
    auto result = source;
    result.frame = *frame;
    return result.valid()
        ? std::optional<kernel::PlanarProfileInput>{
              std::move(result)}
        : std::nullopt;
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

[[nodiscard]] double arcDelta(
    const kernel::Arc2& curve,
    const kernel::BoundaryUse2D& use) noexcept {
    const double from =
        use.follows_source_direction
            ? use.start_parameter
            : use.end_parameter;
    const double to =
        use.follows_source_direction
            ? use.end_parameter
            : use.start_parameter;
    double delta =
        curve.sweep_angle * (to - from);
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
        if (delta < 0.0) edge.Reverse();
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
                return circularEdge(
                    circle,
                    full_turn *
                        use.start_parameter,
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
                return circularEdge(
                    circle,
                    start_angle,
                    arcDelta(curve, use),
                    false);
            }
        },
        use.curve);
}

[[nodiscard]] kernel::SurfaceKind
semanticSurfaceKind(
    const kernel::BoundaryUse2D& use) noexcept {
    return std::holds_alternative<kernel::Line2>(
               use.curve)
        ? kernel::SurfaceKind::plane
        : kernel::SurfaceKind::cylinder;
}

[[nodiscard]] kernel::SurfaceKind
providerSurfaceKind(
    const TopoDS_Face& face) {
    BRepAdaptor_Surface surface{face, true};
    switch (surface.GetType()) {
    case GeomAbs_Plane:
        return kernel::SurfaceKind::plane;
    case GeomAbs_Cylinder:
        return kernel::SurfaceKind::cylinder;
    case GeomAbs_Cone:
        return kernel::SurfaceKind::cone;
    case GeomAbs_Sphere:
        return kernel::SurfaceKind::sphere;
    case GeomAbs_Torus:
        return kernel::SurfaceKind::torus;
    default:
        return kernel::SurfaceKind::other;
    }
}

[[nodiscard]] std::optional<kernel::Frame3>
shiftedCarrierFrame(
    const kernel::Frame3& source,
    double normal_offset) noexcept {
    kernel::Frame3 result = source;
    result.origin.x +=
        result.normal.x * normal_offset;
    result.origin.y +=
        result.normal.y * normal_offset;
    result.origin.z +=
        result.normal.z * normal_offset;
    return result.valid()
        ? std::optional<kernel::Frame3>{result}
        : std::nullopt;
}

[[nodiscard]] std::optional<kernel::Frame3>
lineSideCarrierFrame(
    const kernel::PlanarProfileInput& input,
    const kernel::BoundaryUse2D& use) {
    const auto* line =
        std::get_if<kernel::Line2>(
            &use.curve);
    if (line == nullptr) {
        return std::nullopt;
    }

    const double du =
        line->end.u - line->start.u;
    const double dv =
        line->end.v - line->start.v;
    const gp_Vec u_vector{
        input.frame.u_axis.x * du +
            input.frame.v_axis.x * dv,
        input.frame.u_axis.y * du +
            input.frame.v_axis.y * dv,
        input.frame.u_axis.z * du +
            input.frame.v_axis.z * dv};
    if (!(u_vector.SquareMagnitude() > 0.0)) {
        return std::nullopt;
    }

    const gp_Dir u{u_vector};
    const gp_Dir v{
        input.frame.normal.x,
        input.frame.normal.y,
        input.frame.normal.z};
    const gp_Vec n_vector =
        gp_Vec{u}.Crossed(gp_Vec{v});
    if (!(n_vector.SquareMagnitude() > 0.0)) {
        return std::nullopt;
    }
    const gp_Dir n{n_vector};
    const auto origin =
        point3(input.frame, line->start);

    kernel::Frame3 result;
    result.origin = {
        origin.X(),
        origin.Y(),
        origin.Z()};
    result.u_axis = {
        u.X(),
        u.Y(),
        u.Z()};
    // Canonical side V is source support N, independent of signed extent.
    result.v_axis = {
        v.X(),
        v.Y(),
        v.Z()};
    result.normal = {
        n.X(),
        n.Y(),
        n.Z()};

    return result.valid()
        ? std::optional<kernel::Frame3>{result}
        : std::nullopt;
}

[[nodiscard]] const kernel::BoundaryUse2D*
boundaryUseForProvenance(
    const kernel::PlanarProfileInput& input,
    const kernel::BoundaryUseProvenance& provenance) noexcept {
    const auto find_in_loop =
        [&provenance](
            const kernel::ProfileLoopInput& loop)
            -> const kernel::BoundaryUse2D* {
            const auto found =
                std::find_if(
                    loop.boundary.begin(),
                    loop.boundary.end(),
                    [&provenance](
                        const kernel::BoundaryUse2D& use) {
                        return use.provenance ==
                               provenance;
                    });
            return found == loop.boundary.end()
                ? nullptr
                : &*found;
        };

    if (const auto* found =
            find_in_loop(input.outer)) {
        return found;
    }
    for (const auto& hole : input.holes) {
        if (const auto* found =
                find_in_loop(hole)) {
            return found;
        }
    }
    return nullptr;
}

[[nodiscard]] std::optional<kernel::Frame3>
revolvePlanarSideCarrierFrame(
    const kernel::PlanarProfileInput& input,
    const kernel::Axis3& axis,
    const kernel::BoundaryUse2D& use) noexcept {
    const auto* line =
        std::get_if<kernel::Line2>(
            &use.curve);
    const auto unit_axis =
        unit3(axis.direction);
    if (line == nullptr ||
        !unit_axis) {
        return std::nullopt;
    }

    const auto world_point =
        [&input](const kernel::Point2& point) {
            const auto converted =
                point3(input.frame, point);
            return kernel::Point3{
                converted.X(),
                converted.Y(),
                converted.Z()};
        };
    const auto project =
        [&axis, &unit_axis](
            const kernel::Point3& point) {
            return add3(
                axis.origin,
                scale3(
                    *unit_axis,
                    dot3(
                        subtract3(
                            point,
                            axis.origin),
                        *unit_axis)));
        };

    const auto first =
        world_point(
            linePoint(
                *line,
                use.start_parameter));
    const auto second =
        world_point(
            linePoint(
                *line,
                use.end_parameter));
    const auto first_axis =
        project(first);
    const auto second_axis =
        project(second);
    const auto first_radial =
        subtract3(first, first_axis);
    const auto second_radial =
        subtract3(second, second_axis);
    const double first_squared =
        dot3(first_radial, first_radial);
    const double second_squared =
        dot3(second_radial, second_radial);

    kernel::Point3 radial;
    kernel::Point3 origin;
    if (first_squared >= second_squared &&
        first_squared > 0.0) {
        radial = first_radial;
        origin = first_axis;
    } else if (second_squared > 0.0) {
        radial = second_radial;
        origin = second_axis;
    } else {
        return std::nullopt;
    }

    const auto u =
        unit3(radial);
    if (!u) {
        return std::nullopt;
    }
    const auto v =
        unit3(
            cross3(
                *unit_axis,
                *u));
    if (!v) {
        return std::nullopt;
    }

    kernel::Frame3 result;
    result.origin = origin;
    result.u_axis = *u;
    result.v_axis = *v;
    result.normal = *unit_axis;
    return result.valid()
        ? std::optional<kernel::Frame3>{
              result}
        : std::nullopt;
}

struct SourceEdge final {
    TopoDS_Edge edge;
    kernel::BoundaryUseProvenance provenance;
    kernel::SurfaceKind surface_kind{
        kernel::SurfaceKind::other};
    std::optional<kernel::Frame3>
        canonical_frame;
};

struct WireBuild final {
    TopoDS_Wire wire;
    std::vector<SourceEdge> source_edges;
};

[[nodiscard]] std::optional<WireBuild>
buildWire(
    const kernel::PlanarProfileInput& input,
    const kernel::ProfileLoopInput& loop) {
    BRepBuilderAPI_MakeWire make_wire;
    WireBuild result;
    result.source_edges.reserve(
        loop.boundary.size());

    for (const auto& use : loop.boundary) {
        const auto edge = buildEdge(input, use);
        if (!edge) return std::nullopt;
        make_wire.Add(*edge);
        if (!make_wire.IsDone()) {
            return std::nullopt;
        }
        const auto surface_kind =
            semanticSurfaceKind(use);
        auto canonical_frame =
            surface_kind ==
                    kernel::SurfaceKind::plane
                ? lineSideCarrierFrame(
                      input,
                      use)
                : std::optional<
                      kernel::Frame3>{};
        if (surface_kind ==
                kernel::SurfaceKind::plane &&
            !canonical_frame) {
            return std::nullopt;
        }
        result.source_edges.push_back(
            {make_wire.Edge(),
             use.provenance,
             surface_kind,
             std::move(canonical_frame)});
    }
    result.wire = make_wire.Wire();
    return result;
}

struct ProfileFaceBuild final {
    TopoDS_Face face;
    std::vector<SourceEdge> source_edges;
};

[[nodiscard]] std::optional<ProfileFaceBuild>
buildProfileFace(
    const kernel::PlanarProfileInput& input) {
    const auto outer =
        buildWire(input, input.outer);
    if (!outer) return std::nullopt;

    BRepBuilderAPI_MakeFace make_face{
        outer->wire,
        true};
    if (!make_face.IsDone()) {
        return std::nullopt;
    }

    ProfileFaceBuild result;
    result.source_edges =
        outer->source_edges;

    for (const auto& hole : input.holes) {
        auto built = buildWire(input, hole);
        if (!built) return std::nullopt;

        auto hole_wire = built->wire;
        hole_wire.Reverse();
        make_face.Add(hole_wire);
        result.source_edges.insert(
            result.source_edges.end(),
            built->source_edges.begin(),
            built->source_edges.end());
    }

    if (!make_face.IsDone()) {
        return std::nullopt;
    }
    result.face = make_face.Face();
    return result;
}

[[nodiscard]] std::size_t countSubshapes(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    if (shape.IsNull()) return 0U;
    if (shape.ShapeType() == kind) {
        return 1U;
    }

    std::size_t count = 0U;
    for (TopExp_Explorer explorer{shape, kind};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

[[nodiscard]] std::size_t countUniqueSubshapes(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    if (shape.IsNull()) return 0U;
    if (shape.ShapeType() == kind) {
        return 1U;
    }

    TopTools_IndexedMapOfShape unique;
    TopExp::MapShapes(
        shape,
        kind,
        unique);
    return static_cast<std::size_t>(
        unique.Extent());
}

[[nodiscard]] std::optional<TopoDS_Solid>
singleSolid(
    const TopoDS_Shape& shape) {
    if (shape.IsNull()) return std::nullopt;
    if (shape.ShapeType() == TopAbs_SOLID) {
        return TopoDS::Solid(shape);
    }

    std::optional<TopoDS_Solid> result;
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_SOLID};
         explorer.More();
         explorer.Next()) {
        if (result) return std::nullopt;
        result =
            TopoDS::Solid(
                explorer.Current());
    }
    return result;
}

[[nodiscard]] bool containsSameFace(
    const TopoDS_Shape& shape,
    const TopoDS_Face& face) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(face)) {
            return true;
        }
    }
    return false;
}

void appendUniqueFace(
    std::vector<TopoDS_Face>& faces,
    const TopoDS_Shape& candidate,
    const TopoDS_Shape& result) {
    if (candidate.IsNull()) return;

    auto add = [&faces, &result](
                   const TopoDS_Face& face) {
        if (!containsSameFace(result, face)) {
            return;
        }
        const bool duplicate =
            std::any_of(
                faces.begin(),
                faces.end(),
                [&face](
                    const TopoDS_Face& existing) {
                    return existing.IsSame(face);
                });
        if (!duplicate) {
            faces.push_back(face);
        }
    };

    if (candidate.ShapeType() ==
        TopAbs_FACE) {
        add(TopoDS::Face(candidate));
        return;
    }

    for (TopExp_Explorer explorer{
             candidate,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        add(TopoDS::Face(
            explorer.Current()));
    }
}

template <typename Operation>
[[nodiscard]] std::vector<TopoDS_Face>
descendantFaces(
    Operation& operation,
    const TopoDS_Face& source,
    const TopoDS_Shape& result,
    bool include_generated = true) {
    std::vector<TopoDS_Face> descendants;

    const auto& modified =
        operation.Modified(source);
    for (const auto& item : modified) {
        appendUniqueFace(
            descendants,
            item,
            result);
    }

    if (include_generated) {
        const auto& generated =
            operation.Generated(source);
        for (const auto& item : generated) {
            appendUniqueFace(
                descendants,
                item,
                result);
        }
    }

    if (descendants.empty() &&
        !operation.IsDeleted(source) &&
        containsSameFace(result, source)) {
        descendants.push_back(source);
    }
    return descendants;
}

[[nodiscard]] bool containsSameEdge(
    const TopoDS_Shape& shape,
    const TopoDS_Edge& edge) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(edge)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool containsSameVertex(
    const TopoDS_Shape& shape,
    const TopoDS_Vertex& vertex) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_VERTEX};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(vertex)) {
            return true;
        }
    }
    return false;
}

void appendUniqueEdge(
    std::vector<TopoDS_Edge>& edges,
    const TopoDS_Shape& candidate,
    const TopoDS_Shape& result) {
    if (candidate.IsNull()) return;

    auto add = [&edges, &result](
                   const TopoDS_Edge& edge) {
        if (!containsSameEdge(result, edge)) {
            return;
        }
        const bool duplicate =
            std::any_of(
                edges.begin(),
                edges.end(),
                [&edge](
                    const TopoDS_Edge& existing) {
                    return existing.IsSame(edge);
                });
        if (!duplicate) {
            edges.push_back(edge);
        }
    };

    if (candidate.ShapeType() ==
        TopAbs_EDGE) {
        add(TopoDS::Edge(candidate));
        return;
    }

    for (TopExp_Explorer explorer{
             candidate,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        add(TopoDS::Edge(
            explorer.Current()));
    }
}

void appendUniqueVertex(
    std::vector<TopoDS_Vertex>& vertices,
    const TopoDS_Shape& candidate,
    const TopoDS_Shape& result) {
    if (candidate.IsNull()) return;

    auto add = [&vertices, &result](
                   const TopoDS_Vertex& vertex) {
        if (!containsSameVertex(
                result,
                vertex)) {
            return;
        }
        const bool duplicate =
            std::any_of(
                vertices.begin(),
                vertices.end(),
                [&vertex](
                    const TopoDS_Vertex& existing) {
                    return existing.IsSame(vertex);
                });
        if (!duplicate) {
            vertices.push_back(vertex);
        }
    };

    if (candidate.ShapeType() ==
        TopAbs_VERTEX) {
        add(TopoDS::Vertex(candidate));
        return;
    }

    for (TopExp_Explorer explorer{
             candidate,
             TopAbs_VERTEX};
         explorer.More();
         explorer.Next()) {
        add(TopoDS::Vertex(
            explorer.Current()));
    }
}

template <typename Operation>
[[nodiscard]] std::vector<TopoDS_Edge>
descendantEdges(
    Operation& operation,
    const TopoDS_Edge& source,
    const TopoDS_Shape& result,
    bool include_generated = true) {
    std::vector<TopoDS_Edge> descendants;

    const auto& modified =
        operation.Modified(source);
    for (const auto& item : modified) {
        appendUniqueEdge(
            descendants,
            item,
            result);
    }

    if (include_generated) {
        const auto& generated =
            operation.Generated(source);
        for (const auto& item : generated) {
            appendUniqueEdge(
                descendants,
                item,
                result);
        }
    }

    if (descendants.empty() &&
        !operation.IsDeleted(source) &&
        containsSameEdge(result, source)) {
        descendants.push_back(source);
    }
    return descendants;
}

template <typename Operation>
[[nodiscard]] std::vector<TopoDS_Vertex>
descendantVertices(
    Operation& operation,
    const TopoDS_Vertex& source,
    const TopoDS_Shape& result,
    bool include_generated = true) {
    std::vector<TopoDS_Vertex> descendants;

    const auto& modified =
        operation.Modified(source);
    for (const auto& item : modified) {
        appendUniqueVertex(
            descendants,
            item,
            result);
    }

    if (include_generated) {
        const auto& generated =
            operation.Generated(source);
        for (const auto& item : generated) {
            appendUniqueVertex(
                descendants,
                item,
                result);
        }
    }

    if (descendants.empty() &&
        !operation.IsDeleted(source) &&
        containsSameVertex(result, source)) {
        descendants.push_back(source);
    }
    return descendants;
}

[[nodiscard]] std::vector<TopoDS_Face>
facesFromShape(
    const TopoDS_Shape& shape) {
    std::vector<TopoDS_Face> result;
    if (shape.IsNull()) return result;
    if (shape.ShapeType() == TopAbs_FACE) {
        result.push_back(TopoDS::Face(shape));
        return result;
    }
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        const auto face =
            TopoDS::Face(
                explorer.Current());
        const bool duplicate =
            std::any_of(
                result.begin(),
                result.end(),
                [&face](
                    const TopoDS_Face& existing) {
                    return existing.IsSame(face);
                });
        if (!duplicate) {
            result.push_back(face);
        }
    }
    return result;
}

[[nodiscard]] std::vector<TopoDS_Face>
facesFromShapeList(
    const TopTools_ListOfShape& shapes) {
    std::vector<TopoDS_Face> result;
    for (TopTools_ListOfShape::Iterator it{shapes};
         it.More();
         it.Next()) {
        for (const auto& face :
             facesFromShape(it.Value())) {
            const bool duplicate =
                std::any_of(
                    result.begin(),
                    result.end(),
                    [&face](
                        const TopoDS_Face& existing) {
                        return existing.IsSame(face);
                    });
            if (!duplicate) {
                result.push_back(face);
            }
        }
    }
    return result;
}

[[nodiscard]] std::vector<TopoDS_Edge>
matchingFaceEdges(
    const TopoDS_Face& face,
    const TopoDS_Edge& source) {
    std::vector<TopoDS_Edge> matches;
    for (TopExp_Explorer explorer{
             face,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto candidate =
            TopoDS::Edge(
                explorer.Current());
        if (candidate.IsSame(source)) {
            matches.push_back(candidate);
        }
    }
    return matches;
}

struct NewSemanticSource final {
    kernel::ExtrudeFaceRole role;
    std::vector<TopoDS_Face> source_faces;
    kernel::SurfaceKind surface_kind{
        kernel::SurfaceKind::other};
    std::optional<kernel::Frame3>
        canonical_frame;
};

[[nodiscard]] kernel::ReferenceStatus
referenceStatus(
    std::size_t count) noexcept {
    if (count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (count == 1U) {
        return kernel::ReferenceStatus::resolved;
    }
    return kernel::ReferenceStatus::ambiguous;
}

class OcctRuntimeSolid final
    : public kernel::RuntimeSolid {
public:
    TopoDS_Solid solid;

    // Existing PM-01 semantic Face lineage subset. Only Faces with defended
    // semantic producer/provenance live here.
    std::map<std::uint64_t, TopoDS_Face>
        tracked_faces;

    // PM-02A complete current-stage provider inventory. These maps are
    // runtime-only realization tables and do not imply durable semantics.
    std::map<std::uint64_t, TopoDS_Face>
        inventory_faces;
    std::map<std::uint64_t, TopoDS_Edge>
        inventory_edges;
    std::map<std::uint64_t, TopoDS_Vertex>
        inventory_vertices;

    struct TrackedSurface final {
        kernel::SurfaceKind kind{
            kernel::SurfaceKind::other};
        std::optional<kernel::Frame3>
            canonical_frame;
        std::vector<TopoDS_Face> faces;
    };

    std::map<std::uint64_t, TrackedSurface>
        tracked_surfaces;

    std::uint64_t next_face_token{1U};
    std::uint64_t next_edge_token{1U};
    std::uint64_t next_vertex_token{1U};
    std::uint64_t next_surface_token{1U};
};

struct CandidateClaim final {
    enum class Kind {
        inherited,
        created,
    };

    Kind kind{Kind::created};
    std::size_t index{};
    std::vector<TopoDS_Face> candidates;
    bool aliased{false};
};

void markAliasedUniqueClaims(
    std::vector<CandidateClaim>& claims) {
    for (std::size_t first = 0U;
         first < claims.size();
         ++first) {
        if (claims[first].candidates.size() != 1U) {
            continue;
        }
        for (std::size_t second = first + 1U;
             second < claims.size();
             ++second) {
            if (claims[second].candidates.size() != 1U) {
                continue;
            }
            if (claims[first].candidates.front().IsSame(
                    claims[second].candidates.front())) {
                claims[first].aliased = true;
                claims[second].aliased = true;
            }
        }
    }
}

template <typename Mapper>
void publishLineage(
    kernel::SolidModelingResult& result,
    OcctRuntimeSolid& runtime,
    const OcctRuntimeSolid* upstream,
    const std::vector<NewSemanticSource>& created,
    Mapper&& mapper) {
    std::vector<CandidateClaim> claims;
    if (upstream != nullptr) {
        result.inherited_faces.reserve(
            upstream->tracked_faces.size());
        for (const auto& [token, face] :
             upstream->tracked_faces) {
            const auto index =
                result.inherited_faces.size();
            result.inherited_faces.push_back(
                {
                    kernel::RuntimeFaceToken{
                        token},
                    kernel::ReferenceStatus::
                        unsupported,
                    0U,
                });
            claims.push_back(
                {
                    CandidateClaim::Kind::
                        inherited,
                    index,
                    mapper(face),
                    false,
                });
        }
        runtime.next_face_token =
            upstream->next_face_token;
        runtime.next_edge_token =
            upstream->next_edge_token;
        runtime.next_vertex_token =
            upstream->next_vertex_token;
    }

    result.new_faces.reserve(created.size());
    for (const auto& source : created) {
        std::vector<TopoDS_Face> candidates;
        for (const auto& face :
             source.source_faces) {
            const auto mapped =
                mapper(face);
            for (const auto& candidate :
                 mapped) {
                const bool duplicate =
                    std::any_of(
                        candidates.begin(),
                        candidates.end(),
                        [&candidate](
                            const TopoDS_Face&
                                existing) {
                            return existing.IsSame(
                                candidate);
                        });
                if (!duplicate) {
                    candidates.push_back(
                        candidate);
                }
            }
        }

        const auto index =
            result.new_faces.size();
        result.new_faces.push_back(
            {
                source.role,
                kernel::ReferenceStatus::
                    unsupported,
                0U,
                std::nullopt,
            });
        claims.push_back(
            {
                CandidateClaim::Kind::created,
                index,
                std::move(candidates),
                false,
            });
    }

    markAliasedUniqueClaims(claims);

    for (const auto& claim : claims) {
        const auto count =
            claim.candidates.size();
        auto status =
            referenceStatus(count);
        if (claim.aliased &&
            status ==
                kernel::ReferenceStatus::
                    resolved) {
            status =
                kernel::ReferenceStatus::
                    ambiguous;
        }

        if (claim.kind ==
            CandidateClaim::Kind::inherited) {
            auto& published =
                result.inherited_faces[
                    claim.index];
            published.status = status;
            published.candidate_count =
                count;
            if (status ==
                kernel::ReferenceStatus::
                    resolved) {
                runtime.tracked_faces.emplace(
                    published.token.value,
                    claim.candidates.front());
            }
            continue;
        }

        auto& published =
            result.new_faces[claim.index];
        published.status = status;
        published.candidate_count =
            count;
        if (status !=
            kernel::ReferenceStatus::
                resolved) {
            continue;
        }

        if (runtime.next_face_token == 0U) {
            published.status =
                kernel::ReferenceStatus::
                    unsupported;
            continue;
        }
        const kernel::RuntimeFaceToken token{
            runtime.next_face_token};
        ++runtime.next_face_token;
        published.resolved_token = token;
        runtime.tracked_faces.emplace(
            token.value,
            claim.candidates.front());
    }
}

[[nodiscard]] kernel::PlanarProfileInput
shiftedProfile(
    const kernel::LinearExtrudeInput& input) {
    auto result = input.profile;
    result.frame.origin.x +=
        result.frame.normal.x *
        input.start_offset_mm;
    result.frame.origin.y +=
        result.frame.normal.y *
        input.start_offset_mm;
    result.frame.origin.z +=
        result.frame.normal.z *
        input.start_offset_mm;
    return result;
}

[[nodiscard]] std::optional<
    std::pair<
        TopoDS_Shape,
        std::vector<NewSemanticSource>>>
buildExtrudeTool(
    const kernel::LinearExtrudeInput& input) {
    const auto profile =
        shiftedProfile(input);
    const auto built =
        buildProfileFace(profile);
    if (!built) return std::nullopt;

    const double distance =
        input.end_offset_mm -
        input.start_offset_mm;
    BRepSweep_Prism sweep{
        built->face,
        gp_Vec{
            profile.frame.normal.x * distance,
            profile.frame.normal.y * distance,
            profile.frame.normal.z * distance},
        false,
        true};

    const auto shape = sweep.Shape();
    if (shape.IsNull()) return std::nullopt;

    const auto start_frame =
        shiftedCarrierFrame(
            input.profile.frame,
            input.start_offset_mm);
    const auto end_frame =
        shiftedCarrierFrame(
            input.profile.frame,
            input.end_offset_mm);
    if (!start_frame || !end_frame) {
        return std::nullopt;
    }

    std::vector<NewSemanticSource> sources;
    sources.push_back(
        {
            kernel::ExtrudeFaceRole{
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.start_cap_role,
                std::nullopt},
            facesFromShape(
                sweep.FirstShape()),
            kernel::SurfaceKind::plane,
            start_frame,
        });
    sources.push_back(
        {
            kernel::ExtrudeFaceRole{
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.end_cap_role,
                std::nullopt},
            facesFromShape(
                sweep.LastShape()),
            kernel::SurfaceKind::plane,
            end_frame,
        });

    for (const auto& source :
         built->source_edges) {
        std::vector<TopoDS_Face> faces;
        const auto basis_edges =
            matchingFaceEdges(
                built->face,
                source.edge);
        for (const auto& basis_edge :
             basis_edges) {
            const auto generated =
                sweep.Shape(basis_edge);
            const auto generated_faces =
                facesFromShape(generated);
            for (const auto& face :
                 generated_faces) {
                const bool duplicate =
                    std::any_of(
                        faces.begin(),
                        faces.end(),
                        [&face](
                            const TopoDS_Face&
                                existing) {
                            return existing.IsSame(
                                face);
                        });
                if (!duplicate) {
                    faces.push_back(face);
                }
            }
        }
        auto canonical_frame =
            source.canonical_frame;
        if (canonical_frame) {
            // buildProfileFace() consumed the profile shifted to the sweep
            // start. Canonical side O is authored on the source support plane
            // and therefore must not move with Reverse/Midplane start offset.
            canonical_frame->origin.x -=
                input.profile.frame.normal.x *
                input.start_offset_mm;
            canonical_frame->origin.y -=
                input.profile.frame.normal.y *
                input.start_offset_mm;
            canonical_frame->origin.z -=
                input.profile.frame.normal.z *
                input.start_offset_mm;
        }
        sources.push_back(
            {
                kernel::ExtrudeFaceRole{
                    kernel::ExtrudeGeneratedFaceRoleKind::side,
                    std::nullopt,
                    source.provenance},
                std::move(faces),
                source.surface_kind,
                std::move(canonical_frame),
            });
    }

    return std::make_pair(
        shape,
        std::move(sources));
}

[[nodiscard]] std::optional<
    std::pair<
        TopoDS_Shape,
        std::vector<NewSemanticSource>>>
buildRevolveTool(
    const kernel::AngularRevolveInput& input) {
    if (!input.valid()) {
        return std::nullopt;
    }

    const bool full = input.fullTurn();
    const double delta =
        input.end_angle_radians -
        input.start_angle_radians;

    kernel::PlanarProfileInput profile =
        input.profile;
    if (!full) {
        const auto rotated =
            rotatedProfile(
                input.profile,
                input.axis,
                input.start_angle_radians);
        if (!rotated) {
            return std::nullopt;
        }
        profile = *rotated;
    }

    const auto built =
        buildProfileFace(profile);
    if (!built) {
        return std::nullopt;
    }

    const auto direction =
        unit3(input.axis.direction);
    if (!direction) {
        return std::nullopt;
    }
    const gp_Ax1 axis{
        gp_Pnt{
            input.axis.origin.x,
            input.axis.origin.y,
            input.axis.origin.z},
        gp_Dir{
            direction->x,
            direction->y,
            direction->z}};

    std::unique_ptr<BRepSweep_Revol> sweep;
    if (full) {
        sweep =
            std::make_unique<BRepSweep_Revol>(
                built->face,
                axis,
                false);
    } else {
        sweep =
            std::make_unique<BRepSweep_Revol>(
                built->face,
                axis,
                delta,
                false);
    }

    const auto shape = sweep->Shape();
    if (shape.IsNull()) {
        return std::nullopt;
    }

    std::vector<NewSemanticSource> sources;
    if (!full) {
        const auto start_frame =
            rotatedFrame(
                input.profile.frame,
                input.axis,
                input.start_angle_radians);
        const auto end_frame =
            rotatedFrame(
                input.profile.frame,
                input.axis,
                input.end_angle_radians);
        if (!start_frame ||
            !end_frame) {
            return std::nullopt;
        }

        sources.push_back(
            {
                kernel::GeneratedFaceRole{
                    kernel::GeneratedFaceRoleKind::
                        revolve_start_cap,
                    std::nullopt,
                    std::nullopt},
                facesFromShape(
                    sweep->FirstShape()),
                kernel::SurfaceKind::plane,
                *start_frame,
            });
        sources.push_back(
            {
                kernel::GeneratedFaceRole{
                    kernel::GeneratedFaceRoleKind::
                        revolve_end_cap,
                    std::nullopt,
                    std::nullopt},
                facesFromShape(
                    sweep->LastShape()),
                kernel::SurfaceKind::plane,
                *end_frame,
            });
    }

    for (const auto& source :
         built->source_edges) {
        std::vector<TopoDS_Face> faces;
        const auto basis_edges =
            matchingFaceEdges(
                built->face,
                source.edge);
        for (const auto& basis_edge :
             basis_edges) {
            const auto generated =
                sweep->Shape(
                    basis_edge);
            for (const auto& face :
                 facesFromShape(generated)) {
                const bool duplicate =
                    std::any_of(
                        faces.begin(),
                        faces.end(),
                        [&face](
                            const TopoDS_Face&
                                existing) {
                            return existing.IsSame(
                                face);
                        });
                if (!duplicate) {
                    faces.push_back(face);
                }
            }
        }

        kernel::SurfaceKind kind{
            kernel::SurfaceKind::other};
        if (!faces.empty()) {
            kind =
                providerSurfaceKind(
                    faces.front());
            const bool uniform =
                std::all_of(
                    faces.begin(),
                    faces.end(),
                    [kind](
                        const TopoDS_Face& face) {
                        return providerSurfaceKind(
                                   face) == kind;
                    });
            if (!uniform) {
                return std::nullopt;
            }
        }

        std::optional<kernel::Frame3>
            canonical_frame;
        if (kind ==
            kernel::SurfaceKind::plane) {
            const auto* use =
                boundaryUseForProvenance(
                    input.profile,
                    source.provenance);
            if (use == nullptr) {
                return std::nullopt;
            }
            canonical_frame =
                revolvePlanarSideCarrierFrame(
                    input.profile,
                    input.axis,
                    *use);
            if (!canonical_frame) {
                return std::nullopt;
            }
        }

        sources.push_back(
            {
                kernel::GeneratedFaceRole{
                    kernel::GeneratedFaceRoleKind::
                        revolve_side,
                    std::nullopt,
                    source.provenance},
                std::move(faces),
                kind,
                std::move(canonical_frame),
            });
    }

    return std::make_pair(
        shape,
        std::move(sources));
}

[[nodiscard]] std::optional<std::uint64_t>
semanticFaceToken(
    const OcctRuntimeSolid& runtime,
    const TopoDS_Face& face) {
    for (const auto& [token, semantic_face] :
         runtime.tracked_faces) {
        if (semantic_face.IsSame(face)) {
            return token;
        }
    }
    return std::nullopt;
}

template <typename Token>
[[nodiscard]] std::optional<Token>
allocateRuntimeToken(
    std::uint64_t& next_value) noexcept {
    if (next_value == 0U) {
        return std::nullopt;
    }
    const Token token{next_value};
    ++next_value;
    return token.valid()
        ? std::optional<Token>{token}
        : std::nullopt;
}

[[nodiscard]] bool populateRuntimeTopologyInventory(
    kernel::SolidModelingResult& result,
    OcctRuntimeSolid& runtime,
    const TopoDS_Solid& solid) {
    runtime.inventory_faces.clear();
    runtime.inventory_edges.clear();
    runtime.inventory_vertices.clear();
    result.current_faces.clear();
    result.current_edges.clear();
    result.current_vertices.clear();

    TopTools_IndexedMapOfShape faces;
    TopTools_IndexedMapOfShape edges;
    TopTools_IndexedMapOfShape vertices;
    TopExp::MapShapes(
        solid,
        TopAbs_FACE,
        faces);
    TopExp::MapShapes(
        solid,
        TopAbs_EDGE,
        edges);
    TopExp::MapShapes(
        solid,
        TopAbs_VERTEX,
        vertices);

    result.face_count =
        static_cast<std::size_t>(faces.Extent());
    result.edge_count =
        static_cast<std::size_t>(edges.Extent());
    result.vertex_count =
        static_cast<std::size_t>(vertices.Extent());

    result.current_faces.reserve(result.face_count);
    result.current_edges.reserve(result.edge_count);
    result.current_vertices.reserve(result.vertex_count);

    for (Standard_Integer index = 1;
         index <= faces.Extent();
         ++index) {
        const auto face =
            TopoDS::Face(faces.FindKey(index));
        auto token_value =
            semanticFaceToken(runtime, face);
        if (!token_value) {
            const auto token =
                allocateRuntimeToken<
                    kernel::RuntimeFaceToken>(
                    runtime.next_face_token);
            if (!token) return false;
            token_value = token->value;
        }

        const kernel::RuntimeFaceToken token{
            *token_value};
        if (!token.valid() ||
            !runtime.inventory_faces.emplace(
                token.value,
                face).second) {
            return false;
        }
        result.current_faces.push_back(token);
    }

    for (Standard_Integer index = 1;
         index <= edges.Extent();
         ++index) {
        const auto token =
            allocateRuntimeToken<
                kernel::RuntimeEdgeToken>(
                runtime.next_edge_token);
        if (!token) return false;

        const auto edge =
            TopoDS::Edge(edges.FindKey(index));
        if (!runtime.inventory_edges.emplace(
                token->value,
                edge).second) {
            return false;
        }
        result.current_edges.push_back(*token);
    }

    for (Standard_Integer index = 1;
         index <= vertices.Extent();
         ++index) {
        const auto token =
            allocateRuntimeToken<
                kernel::RuntimeVertexToken>(
                runtime.next_vertex_token);
        if (!token) return false;

        const auto vertex =
            TopoDS::Vertex(
                vertices.FindKey(index));
        if (!runtime.inventory_vertices.emplace(
                token->value,
                vertex).second) {
            return false;
        }
        result.current_vertices.push_back(*token);
    }

    return result.current_faces.size() ==
               result.face_count &&
           result.current_edges.size() ==
               result.edge_count &&
           result.current_vertices.size() ==
               result.vertex_count;
}

[[nodiscard]] std::optional<kernel::RuntimeEdgeToken>
inventoryEdgeToken(
    const OcctRuntimeSolid& runtime,
    const TopoDS_Edge& edge) {
    for (const auto& [token, current] :
         runtime.inventory_edges) {
        if (current.IsSame(edge)) {
            return kernel::RuntimeEdgeToken{token};
        }
    }
    return std::nullopt;
}

struct SelectedRuntimeEdge final {
    kernel::RuntimeEdgeToken token;
    TopoDS_Edge edge;
};

[[nodiscard]] std::optional<
    std::vector<SelectedRuntimeEdge>>
selectedRuntimeEdges(
    const OcctRuntimeSolid& upstream,
    const std::vector<kernel::RuntimeEdgeToken>&
        requested) {
    std::vector<SelectedRuntimeEdge> result;
    result.reserve(requested.size());
    for (const auto token : requested) {
        const auto found =
            upstream.inventory_edges.find(
                token.value);
        if (!token.valid() ||
            found ==
                upstream.inventory_edges.end()) {
            return std::nullopt;
        }
        result.push_back(
            {token, found->second});
    }
    return result;
}

template <typename Operation>
[[nodiscard]] bool captureExactEdgeFeatureMembership(
    Operation& operation,
    const OcctRuntimeSolid& upstream,
    const std::vector<kernel::RuntimeEdgeToken>&
        requested,
    kernel::SolidModelingResult& result) {
    kernel::EdgeFeatureInputMembership membership;

    for (Standard_Integer contour = 1;
         contour <= operation.NbContours();
         ++contour) {
        const auto edge_count =
            operation.NbEdges(contour);
        for (Standard_Integer index = 1;
             index <= edge_count;
             ++index) {
            const auto edge =
                operation.Edge(
                    contour,
                    index);
            const auto token =
                inventoryEdgeToken(
                    upstream,
                    edge);
            if (!token) {
                return false;
            }
            if (std::find(
                    membership
                        .provider_contour_edges
                        .begin(),
                    membership
                        .provider_contour_edges
                        .end(),
                    *token) ==
                membership
                    .provider_contour_edges
                    .end()) {
                membership
                    .provider_contour_edges
                    .push_back(*token);
            }
        }
    }

    result.edge_feature_input_membership =
        std::move(membership);
    return result.edge_feature_input_membership
               ->exactFor(requested);
}

[[nodiscard]] std::optional<kernel::RuntimeVertexToken>
inventoryVertexToken(
    const OcctRuntimeSolid& runtime,
    const TopoDS_Vertex& vertex) {
    for (const auto& [token, current] :
         runtime.inventory_vertices) {
        if (current.IsSame(vertex)) {
            return kernel::RuntimeVertexToken{token};
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<kernel::RuntimeFaceToken>
inventoryFaceToken(
    const OcctRuntimeSolid& runtime,
    const TopoDS_Face& face) {
    for (const auto& [token, current] :
         runtime.inventory_faces) {
        if (current.IsSame(face)) {
            return kernel::RuntimeFaceToken{token};
        }
    }
    return std::nullopt;
}

void appendUniqueFaceCandidate(
    std::vector<TopoDS_Face>& faces,
    const TopoDS_Face& candidate) {
    const bool duplicate =
        std::any_of(
            faces.begin(),
            faces.end(),
            [&candidate](const TopoDS_Face& existing) {
                return existing.IsSame(candidate);
            });
    if (!duplicate) {
        faces.push_back(candidate);
    }
}

struct SurfaceCandidateClaim final {
    enum class Kind {
        inherited,
        created,
    };

    Kind kind{Kind::created};
    std::size_t index{};
    std::vector<TopoDS_Face> candidates;
    kernel::SurfaceKind surface_kind{
        kernel::SurfaceKind::other};
    std::optional<kernel::Frame3>
        canonical_frame;
    bool aliased{false};
    std::optional<kernel::RuntimeSurfaceToken>
        inherited_token;
    std::optional<kernel::RuntimeSurfaceToken>
        continued_into;
    std::vector<TopoDS_Face>
        contribution_candidates;
};

[[nodiscard]] bool surfaceClaimsShareFace(
    const SurfaceCandidateClaim& first,
    const SurfaceCandidateClaim& second) {
    for (const auto& first_face :
         first.candidates) {
        const bool shared =
            std::any_of(
                second.candidates.begin(),
                second.candidates.end(),
                [&first_face](
                    const TopoDS_Face& second_face) {
                    return first_face.IsSame(
                        second_face);
                });
        if (shared) return true;
    }
    return false;
}

[[nodiscard]] bool facesShareResultEdge(
    const TopoDS_Face& first,
    const TopoDS_Face& second) {
    for (TopExp_Explorer first_edges{
             first,
             TopAbs_EDGE};
         first_edges.More();
         first_edges.Next()) {
        const auto first_edge =
            TopoDS::Edge(
                first_edges.Current());
        for (TopExp_Explorer second_edges{
                 second,
                 TopAbs_EDGE};
             second_edges.More();
             second_edges.Next()) {
            if (first_edge.IsSame(
                    second_edges.Current())) {
                return true;
            }
        }
    }
    return false;
}

[[nodiscard]] bool planarFacesSameDomain(
    const TopoDS_Face& first,
    const TopoDS_Face& second) {
    if (providerSurfaceKind(first) !=
            kernel::SurfaceKind::plane ||
        providerSurfaceKind(second) !=
            kernel::SurfaceKind::plane) {
        return false;
    }

    const BRepAdaptor_Surface first_surface{
        first,
        true};
    const BRepAdaptor_Surface second_surface{
        second,
        true};
    if (first_surface.GetType() !=
            GeomAbs_Plane ||
        second_surface.GetType() !=
            GeomAbs_Plane) {
        return false;
    }

    const auto first_plane =
        first_surface.Plane();
    const auto second_plane =
        second_surface.Plane();
    const auto& first_normal =
        first_plane.Axis().Direction();
    const auto& second_normal =
        second_plane.Axis().Direction();

    if (!first_normal.IsParallel(
            second_normal,
            Precision::Angular())) {
        return false;
    }

    const double tolerance =
        std::max(
            {
                Precision::Confusion(),
                BRep_Tool::Tolerance(first),
                BRep_Tool::Tolerance(second),
            });
    return first_plane.Distance(
               second_plane.Location()) <=
           tolerance;
}


[[nodiscard]] bool surfaceClaimsHaveCertifiedContinuation(
    const SurfaceCandidateClaim& created,
    const SurfaceCandidateClaim& inherited) {
    if (surfaceClaimsShareFace(
            created,
            inherited)) {
        return true;
    }

    for (const auto& created_face :
         created.candidates) {
        for (const auto& inherited_face :
             inherited.candidates) {
            if (!facesShareResultEdge(
                    created_face,
                    inherited_face)) {
                continue;
            }
            if (planarFacesSameDomain(
                    created_face,
                    inherited_face)) {
                return true;
            }
        }
    }
    return false;
}

void applyAddSurfaceContinuations(
    std::vector<SurfaceCandidateClaim>& claims) {
    for (std::size_t created_index = 0U;
         created_index < claims.size();
         ++created_index) {
        auto& created = claims[created_index];
        if (created.kind !=
                SurfaceCandidateClaim::Kind::created ||
            created.surface_kind !=
                kernel::SurfaceKind::plane ||
            created.candidates.empty()) {
            continue;
        }

        std::vector<std::size_t>
            inherited_matches;
        for (std::size_t inherited_index = 0U;
             inherited_index < claims.size();
             ++inherited_index) {
            const auto& inherited =
                claims[inherited_index];
            if (inherited.kind !=
                    SurfaceCandidateClaim::Kind::
                        inherited ||
                inherited.surface_kind !=
                    kernel::SurfaceKind::plane ||
                !inherited.inherited_token ||
                !surfaceClaimsHaveCertifiedContinuation(
                    created,
                    inherited)) {
                continue;
            }
            inherited_matches.push_back(
                inherited_index);
        }

        // ADR-0017: only one inherited Boolean-lineage claim may own the
        // continuation. The provider may certify a shared descendant or one
        // same-domain planar partition adjacency between exact descendants;
        // no global proximity/coplanarity search participates in ownership.
        if (inherited_matches.size() != 1U) {
            continue;
        }

        auto& inherited =
            claims[inherited_matches.front()];
        created.continued_into =
            inherited.inherited_token;
        for (const auto& candidate :
             created.candidates) {
            appendUniqueFaceCandidate(
                inherited.candidates,
                candidate);
        }

        // The created Surface role remains contribution evidence but no longer
        // competes as an independent semantic carrier.
        created.candidates.clear();
    }
}

void markAliasedSurfaceClaims(
    std::vector<SurfaceCandidateClaim>& claims) {
    for (std::size_t first = 0U;
         first < claims.size();
         ++first) {
        for (std::size_t second = first + 1U;
             second < claims.size();
             ++second) {
            bool shared = false;
            for (const auto& first_face :
                 claims[first].candidates) {
                shared =
                    std::any_of(
                        claims[second].candidates.begin(),
                        claims[second].candidates.end(),
                        [&first_face](
                            const TopoDS_Face& second_face) {
                            return first_face.IsSame(
                                second_face);
                        });
                if (shared) break;
            }
            if (shared) {
                claims[first].aliased = true;
                claims[second].aliased = true;
            }
        }
    }
}

[[nodiscard]] kernel::ReferenceStatus
surfaceReferenceStatus(
    std::size_t count,
    bool aliased) noexcept {
    if (count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    return aliased
        ? kernel::ReferenceStatus::ambiguous
        : kernel::ReferenceStatus::resolved;
}

[[nodiscard]] kernel::ReferenceStatus
strictFaceReferenceStatus(
    std::size_t count,
    bool aliased) noexcept {
    if (count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (aliased) {
        return kernel::ReferenceStatus::ambiguous;
    }
    return referenceStatus(count);
}

template <typename Mapper>
[[nodiscard]] bool publishSurfaceLineage(
    kernel::SolidModelingResult& result,
    OcctRuntimeSolid& runtime,
    const OcctRuntimeSolid* upstream,
    const std::vector<NewSemanticSource>& created,
    bool allow_add_continuation,
    Mapper&& mapper) {
    std::vector<SurfaceCandidateClaim> claims;

    if (upstream != nullptr) {
        runtime.next_surface_token =
            upstream->next_surface_token;
        result.inherited_surfaces.reserve(
            upstream->tracked_surfaces.size());

        for (const auto& [token, tracked] :
             upstream->tracked_surfaces) {
            std::vector<TopoDS_Face> candidates;
            for (const auto& source_face :
                 tracked.faces) {
                const auto mapped =
                    mapper(source_face);
                for (const auto& candidate :
                     mapped) {
                    appendUniqueFaceCandidate(
                        candidates,
                        candidate);
                }
            }

            const auto index =
                result.inherited_surfaces.size();
            result.inherited_surfaces.push_back(
                {
                    kernel::RuntimeSurfaceToken{
                        token},
                    kernel::ReferenceStatus::
                        unsupported,
                    kernel::ReferenceStatus::
                        unsupported,
                    0U,
                    tracked.kind,
                    std::nullopt,
                    {},
                });
            claims.push_back(
                {
                    SurfaceCandidateClaim::Kind::
                        inherited,
                    index,
                    std::move(candidates),
                    tracked.kind,
                    tracked.canonical_frame,
                    false,
                });
            claims.back().inherited_token =
                kernel::RuntimeSurfaceToken{
                    token};
        }
    }

    result.new_surfaces.reserve(created.size());
    for (const auto& source : created) {
        std::vector<TopoDS_Face> candidates;
        for (const auto& source_face :
             source.source_faces) {
            const auto mapped =
                mapper(source_face);
            for (const auto& candidate :
                 mapped) {
                appendUniqueFaceCandidate(
                    candidates,
                    candidate);
            }
        }

        const auto index =
            result.new_surfaces.size();
        result.new_surfaces.push_back(
            {
                source.role,
                kernel::ReferenceStatus::
                    unsupported,
                kernel::ReferenceStatus::
                    unsupported,
                0U,
                source.surface_kind,
                std::nullopt,
                std::nullopt,
                {},
            });
        claims.push_back(
            {
                SurfaceCandidateClaim::Kind::
                    created,
                index,
                std::move(candidates),
                source.surface_kind,
                source.canonical_frame,
                false,
            });
        claims.back().contribution_candidates =
            claims.back().candidates;
    }

    if (allow_add_continuation) {
        applyAddSurfaceContinuations(claims);
    }
    markAliasedSurfaceClaims(claims);

    for (const auto& claim : claims) {
        if (claim.surface_kind ==
                kernel::SurfaceKind::plane) {
            if (!claim.canonical_frame ||
                !claim.canonical_frame->valid()) {
                return false;
            }
        } else if (claim.canonical_frame) {
            return false;
        }

        std::vector<kernel::RuntimeFaceToken>
            contribution_faces;
        contribution_faces.reserve(
            claim.contribution_candidates.size());
        for (const auto& candidate :
             claim.contribution_candidates) {
            const auto token =
                inventoryFaceToken(
                    runtime,
                    candidate);
            if (!token) {
                return false;
            }
            if (std::find(
                    contribution_faces.begin(),
                    contribution_faces.end(),
                    *token) ==
                contribution_faces.end()) {
                contribution_faces.push_back(
                    *token);
            }
        }

        std::vector<kernel::RuntimeFaceToken>
            current_faces;
        current_faces.reserve(
            claim.candidates.size());
        for (const auto& candidate :
             claim.candidates) {
            if (providerSurfaceKind(candidate) !=
                claim.surface_kind) {
                return false;
            }
            const auto token =
                inventoryFaceToken(
                    runtime,
                    candidate);
            if (!token) {
                return false;
            }
            current_faces.push_back(*token);
        }

        const auto surface_status =
            surfaceReferenceStatus(
                claim.candidates.size(),
                claim.aliased);
        const auto strict_face_status =
            strictFaceReferenceStatus(
                claim.candidates.size(),
                claim.aliased);

        if (claim.kind ==
            SurfaceCandidateClaim::Kind::
                inherited) {
            auto& published =
                result.inherited_surfaces[
                    claim.index];
            if (!published.token.valid()) {
                return false;
            }
            published.surface_status =
                surface_status;
            published.strict_face_status =
                strict_face_status;
            published.candidate_face_count =
                claim.candidates.size();
            published.current_faces =
                current_faces;
            published.canonical_frame.reset();

            if (surface_status ==
                kernel::ReferenceStatus::
                    resolved) {
                published.canonical_frame =
                    claim.canonical_frame;
                if (!runtime.tracked_surfaces.emplace(
                        published.token.value,
                        OcctRuntimeSolid::
                            TrackedSurface{
                                claim.surface_kind,
                                claim.canonical_frame,
                                claim.candidates})
                         .second) {
                    return false;
                }
            }
            continue;
        }

        auto& published =
            result.new_surfaces[
                claim.index];
        published.contribution_faces =
            std::move(contribution_faces);
        published.continued_into =
            claim.continued_into;
        published.surface_status =
            surface_status;
        published.strict_face_status =
            strict_face_status;
        published.candidate_face_count =
            claim.candidates.size();
        published.current_faces =
            std::move(current_faces);
        published.canonical_frame.reset();

        if (published.continued_into) {
            if (!published.continued_into->valid() ||
                published.contribution_faces.empty()) {
                return false;
            }
            published.surface_status =
                kernel::ReferenceStatus::unsupported;
            published.strict_face_status =
                kernel::ReferenceStatus::unsupported;
            published.candidate_face_count = 0U;
            published.current_faces.clear();
            continue;
        }

        if (surface_status !=
            kernel::ReferenceStatus::resolved) {
            continue;
        }

        published.canonical_frame =
            claim.canonical_frame;
        const auto token =
            allocateRuntimeToken<
                kernel::RuntimeSurfaceToken>(
                runtime.next_surface_token);
        if (!token) {
            published.surface_status =
                kernel::ReferenceStatus::
                    unsupported;
            published.strict_face_status =
                kernel::ReferenceStatus::
                    unsupported;
            published.current_faces.clear();
            return false;
        }

        published.resolved_token = *token;
        if (!runtime.tracked_surfaces.emplace(
                token->value,
                OcctRuntimeSolid::TrackedSurface{
                    claim.surface_kind,
                    claim.canonical_frame,
                    claim.candidates})
                 .second) {
            return false;
        }
    }

    return true;
}

[[nodiscard]] kernel::CurveKind
providerCurveKind(
    const TopoDS_Edge& edge) {
    BRepAdaptor_Curve curve{edge};
    switch (curve.GetType()) {
    case GeomAbs_Line:
        return kernel::CurveKind::line;
    case GeomAbs_Circle:
        return kernel::CurveKind::circle;
    default:
        return kernel::CurveKind::other;
    }
}

[[nodiscard]] bool faceContainsEdge(
    const TopoDS_Face& face,
    const TopoDS_Edge& edge) {
    for (TopExp_Explorer explorer{
             face,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(edge)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool faceContainsVertex(
    const TopoDS_Face& face,
    const TopoDS_Vertex& vertex) {
    for (TopExp_Explorer explorer{
             face,
             TopAbs_VERTEX};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(vertex)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool edgeContainsVertex(
    const TopoDS_Edge& edge,
    const TopoDS_Vertex& vertex) {
    for (TopExp_Explorer explorer{
             edge,
             TopAbs_VERTEX};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(vertex)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::optional<kernel::Frame3>
providerPlanarFrame(
    const TopoDS_Face& face) {
    BRepAdaptor_Surface surface{face, true};
    if (surface.GetType() !=
        GeomAbs_Plane) {
        return std::nullopt;
    }

    const gp_Pln plane =
        surface.Plane();
    const gp_Ax3 axes =
        plane.Position();
    kernel::Frame3 frame;
    frame.origin = {
        axes.Location().X(),
        axes.Location().Y(),
        axes.Location().Z()};
    const auto& provider_u =
        axes.XDirection();
    const auto& provider_n =
        axes.Direction();
    frame.u_axis = {
        provider_u.X(),
        provider_u.Y(),
        provider_u.Z()};
    frame.normal = {
        provider_n.X(),
        provider_n.Y(),
        provider_n.Z()};
    // gp_Ax3 may be indirect for an oriented Face. This is only transient
    // current-plane evidence. Part reconstructs semantic O/U/V/N from source
    // provenance, so provider Y/UV orientation is never semantic authority.
    frame.v_axis = {
        provider_n.Y() * provider_u.Z() -
            provider_n.Z() * provider_u.Y(),
        provider_n.Z() * provider_u.X() -
            provider_n.X() * provider_u.Z(),
        provider_n.X() * provider_u.Y() -
            provider_n.Y() * provider_u.X()};
    return frame.valid()
        ? std::optional<kernel::Frame3>{frame}
        : std::nullopt;
}

[[nodiscard]] bool faceTrackedBySurface(
    const OcctRuntimeSolid& runtime,
    const TopoDS_Face& face) {
    for (const auto& [token, tracked] :
         runtime.tracked_surfaces) {
        static_cast<void>(token);
        if (std::any_of(
                tracked.faces.begin(),
                tracked.faces.end(),
                [&face](const TopoDS_Face& item) {
                    return item.IsSame(face);
                })) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::optional<
    std::vector<kernel::RuntimeSurfaceToken>>
sourceSurfaceTokensForEdge(
    const OcctRuntimeSolid& upstream,
    const TopoDS_Edge& edge) {
    std::vector<kernel::RuntimeSurfaceToken>
        result;
    for (const auto& [token_value, surface] :
         upstream.tracked_surfaces) {
        const bool contains =
            std::any_of(
                surface.faces.begin(),
                surface.faces.end(),
                [&edge](const TopoDS_Face& face) {
                    return faceContainsEdge(
                        face,
                        edge);
                });
        if (contains) {
            result.push_back(
                kernel::RuntimeSurfaceToken{
                    token_value});
        }
    }

    // A PM-05 material Edge is a boundary between exactly two semantic
    // Surface carriers. Seams and same-Surface partitions were already
    // excluded at authoring/resolution.
    if (result.size() != 2U ||
        result[0] == result[1]) {
        return std::nullopt;
    }
    return result;
}

[[nodiscard]] bool generatedFaceTouchesSurface(
    const OcctRuntimeSolid& runtime,
    kernel::RuntimeSurfaceToken surface_token,
    const TopoDS_Face& generated) {
    const auto found =
        runtime.tracked_surfaces.find(
            surface_token.value);
    if (found ==
        runtime.tracked_surfaces.end()) {
        return false;
    }

    return std::any_of(
        found->second.faces.begin(),
        found->second.faces.end(),
        [&generated](
            const TopoDS_Face& inherited) {
            return facesShareResultEdge(
                generated,
                inherited);
        });
}

[[nodiscard]] bool generatedFaceMatchesSourceEdge(
    const OcctRuntimeSolid& runtime,
    const OcctRuntimeSolid& upstream,
    const SelectedRuntimeEdge& source,
    const TopoDS_Face& generated) {
    const auto surfaces =
        sourceSurfaceTokensForEdge(
            upstream,
            source.edge);
    if (!surfaces) {
        return false;
    }

    return std::all_of(
        surfaces->begin(),
        surfaces->end(),
        [&runtime, &generated](
            kernel::RuntimeSurfaceToken token) {
            return generatedFaceTouchesSurface(
                runtime,
                token,
                generated);
        });
}

struct SharedSelectedVertex final {
    kernel::RuntimeVertexToken token;
    TopoDS_Vertex vertex;
    std::vector<kernel::RuntimeEdgeToken>
        incident_edges;
};

[[nodiscard]] std::vector<SharedSelectedVertex>
sharedSelectedVertices(
    const OcctRuntimeSolid& upstream,
    const std::vector<SelectedRuntimeEdge>&
        selected) {
    std::vector<SharedSelectedVertex> result;
    for (const auto& [token_value, vertex] :
         upstream.inventory_vertices) {
        SharedSelectedVertex candidate;
        candidate.token =
            kernel::RuntimeVertexToken{
                token_value};
        candidate.vertex = vertex;

        for (const auto& edge : selected) {
            if (edgeContainsVertex(
                    edge.edge,
                    vertex)) {
                candidate.incident_edges
                    .push_back(edge.token);
            }
        }

        if (candidate.incident_edges.size() >= 2U) {
            result.push_back(
                std::move(candidate));
        }
    }
    return result;
}

template <typename Operation>
[[nodiscard]] bool publishEdgeFeatureGeneratedSurfaces(
    kernel::SolidModelingResult& result,
    OcctRuntimeSolid& runtime,
    const OcctRuntimeSolid& upstream,
    Operation& operation,
    const kernel::EdgeFeatureInput& input,
    const std::vector<SelectedRuntimeEdge>&
        selected) {
    result.edge_feature_surfaces.clear();

    const auto publish =
        [&result, &runtime, &input](
            kernel::EdgeFeatureGeneratedSurfaceKind
                kind,
            std::optional<kernel::RuntimeEdgeToken>
                source_edge,
            std::optional<kernel::RuntimeVertexToken>
                source_vertex,
            std::vector<kernel::RuntimeEdgeToken>
                incident_edges,
            std::vector<TopoDS_Face> faces)
            -> bool {
        if (faces.empty()) {
            return false;
        }

        const auto surface_kind =
            providerSurfaceKind(
                faces.front());
        if (!std::all_of(
                faces.begin(),
                faces.end(),
                [surface_kind](
                    const TopoDS_Face& face) {
                    return providerSurfaceKind(
                               face) ==
                           surface_kind;
                })) {
            return false;
        }

        if (surface_kind ==
            kernel::SurfaceKind::plane) {
            for (std::size_t index = 1U;
                 index < faces.size();
                 ++index) {
                if (!planarFacesSameDomain(
                        faces.front(),
                        faces[index])) {
                    return false;
                }
            }
        }

        for (const auto& face : faces) {
            if (faceTrackedBySurface(
                    runtime,
                    face)) {
                return false;
            }
        }

        std::optional<kernel::Frame3>
            canonical_frame;
        if (surface_kind ==
            kernel::SurfaceKind::plane) {
            canonical_frame =
                providerPlanarFrame(
                    faces.front());
            if (!canonical_frame) {
                return false;
            }
        }

        const auto token =
            allocateRuntimeToken<
                kernel::RuntimeSurfaceToken>(
                runtime.next_surface_token);
        if (!token) {
            return false;
        }

        std::vector<kernel::RuntimeFaceToken>
            current_faces;
        current_faces.reserve(
            faces.size());
        for (const auto& face : faces) {
            const auto face_token =
                inventoryFaceToken(
                    runtime,
                    face);
            if (!face_token) {
                return false;
            }
            current_faces.push_back(
                *face_token);
        }

        kernel::EdgeFeatureGeneratedSurfaceLineage
            lineage;
        lineage.operation =
            input.operation;
        lineage.kind = kind;
        lineage.runtime_token = *token;
        lineage.surface_kind =
            surface_kind;
        lineage.canonical_frame =
            canonical_frame;
        lineage.current_faces =
            current_faces;
        lineage.source_edge =
            source_edge;
        lineage.source_vertex =
            source_vertex;
        lineage.incident_source_edges =
            std::move(incident_edges);
        if (!lineage.valid()) {
            return false;
        }

        if (!runtime.tracked_surfaces.emplace(
                token->value,
                OcctRuntimeSolid::TrackedSurface{
                    surface_kind,
                    canonical_frame,
                    std::move(faces)})
                 .second) {
            return false;
        }
        result.edge_feature_surfaces.push_back(
            std::move(lineage));
        return true;
    };

    // OCCT's Generated(edge) history is evidence that a Face belongs to
    // the selected-Edge transition set, but a contour operation is not
    // required to return a one-Edge/one-list ownership partition. Build the
    // unique generated set first, then recover P2 source ownership solely from
    // exact topology: a transition Face must touch the Modified descendants
    // of both semantic Surface carriers adjacent to exactly one source Edge.
    std::vector<TopoDS_Face>
        edge_generated_faces;
    for (const auto& source : selected) {
        for (const auto& face :
             facesFromShapeList(
                 operation.Generated(
                     source.edge))) {
            appendUniqueFaceCandidate(
                edge_generated_faces,
                face);
        }
    }
    if (edge_generated_faces.empty()) {
        return false;
    }

    const auto shared_vertices =
        sharedSelectedVertices(
            upstream,
            selected);
    std::vector<std::vector<TopoDS_Face>>
        corner_faces;
    corner_faces.reserve(
        shared_vertices.size());
    for (const auto& source :
         shared_vertices) {
        auto faces =
            facesFromShapeList(
                operation.Generated(
                    source.vertex));
        for (const auto& face : faces) {
            if (std::any_of(
                    edge_generated_faces.begin(),
                    edge_generated_faces.end(),
                    [&face](
                        const TopoDS_Face& candidate) {
                        return candidate.IsSame(face);
                    })) {
                // P3 requires a distinct corner patch. Competing provider
                // histories do not justify choosing one durable owner.
                return false;
            }
        }
        corner_faces.push_back(
            std::move(faces));
    }

    std::vector<std::vector<TopoDS_Face>>
        faces_per_source(
            selected.size());
    for (const auto& face :
         edge_generated_faces) {
        std::optional<std::size_t>
            owner;
        for (std::size_t index = 0U;
             index < selected.size();
             ++index) {
            if (!generatedFaceMatchesSourceEdge(
                    runtime,
                    upstream,
                    selected[index],
                    face)) {
                continue;
            }
            if (owner) {
                // More than one semantic source Edge fits this Face: P2 is
                // ambiguous and must not use provider order as a tie-break.
                return false;
            }
            owner = index;
        }
        if (!owner) {
            return false;
        }
        appendUniqueFaceCandidate(
            faces_per_source[*owner],
            face);
    }

    for (std::size_t index = 0U;
         index < selected.size();
         ++index) {
        if (!publish(
                kernel::EdgeFeatureGeneratedSurfaceKind::
                    edge_transition,
                selected[index].token,
                std::nullopt,
                {},
                std::move(
                    faces_per_source[index]))) {
            return false;
        }
    }

    for (std::size_t index = 0U;
         index < shared_vertices.size();
         ++index) {
        if (corner_faces[index].empty()) {
            continue;
        }
        auto source =
            shared_vertices[index];
        if (!publish(
                kernel::EdgeFeatureGeneratedSurfaceKind::
                    corner_transition,
                std::nullopt,
                source.token,
                std::move(
                    source.incident_edges),
                std::move(
                    corner_faces[index]))) {
            return false;
        }
    }

    for (const auto& [face_token, face] :
         runtime.inventory_faces) {
        static_cast<void>(face_token);
        std::size_t owners = 0U;
        for (const auto& [surface_token, tracked] :
             runtime.tracked_surfaces) {
            static_cast<void>(surface_token);
            if (std::any_of(
                    tracked.faces.begin(),
                    tracked.faces.end(),
                    [&face](
                        const TopoDS_Face& item) {
                        return item.IsSame(face);
                    })) {
                ++owners;
            }
        }
        if (owners != 1U) {
            return false;
        }
    }

    return !result.edge_feature_surfaces.empty();
}

template <typename Token>
void appendUniqueToken(
    std::vector<Token>& values,
    Token token) {
    if (std::find(
            values.begin(),
            values.end(),
            token) ==
        values.end()) {
        values.push_back(token);
    }
}

template <typename Token>
struct RuntimeRealizationClaim final {
    Token source_token;
    std::vector<Token> current_tokens;
    bool aliased{false};
};

template <typename Token>
void markAliasedRealizationClaims(
    std::vector<RuntimeRealizationClaim<Token>>&
        claims) {
    for (std::size_t first = 0U;
         first < claims.size();
         ++first) {
        for (std::size_t second = first + 1U;
             second < claims.size();
             ++second) {
            bool shared = false;
            for (const auto first_token :
                 claims[first].current_tokens) {
                if (std::find(
                        claims[second]
                            .current_tokens.begin(),
                        claims[second]
                            .current_tokens.end(),
                        first_token) !=
                    claims[second]
                        .current_tokens.end()) {
                    shared = true;
                    break;
                }
            }
            if (shared) {
                claims[first].aliased = true;
                claims[second].aliased = true;
            }
        }
    }
}

[[nodiscard]] kernel::ReferenceStatus
realizationLineageStatus(
    std::size_t count,
    bool aliased) noexcept {
    if (count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (aliased || count > 1U) {
        return kernel::ReferenceStatus::ambiguous;
    }
    return kernel::ReferenceStatus::resolved;
}

template <typename Operation>
[[nodiscard]] bool publishCurrentSubshapeLineage(
    kernel::SolidModelingResult& result,
    const OcctRuntimeSolid& upstream,
    const OcctRuntimeSolid& runtime,
    Operation& operation,
    const TopoDS_Shape& shape,
    bool include_generated = true) {
    std::vector<
        RuntimeRealizationClaim<
            kernel::RuntimeEdgeToken>>
        edge_claims;
    edge_claims.reserve(
        upstream.inventory_edges.size());

    for (const auto& [source_value, edge] :
         upstream.inventory_edges) {
        RuntimeRealizationClaim<
            kernel::RuntimeEdgeToken> claim;
        claim.source_token =
            kernel::RuntimeEdgeToken{
                source_value};

        const auto descendants =
            descendantEdges(
                operation,
                edge,
                shape,
                include_generated);
        for (const auto& descendant :
             descendants) {
            const auto token =
                inventoryEdgeToken(
                    runtime,
                    descendant);
            if (!token) return false;
            appendUniqueToken(
                claim.current_tokens,
                *token);
        }
        edge_claims.push_back(
            std::move(claim));
    }

    markAliasedRealizationClaims(
        edge_claims);
    result.inherited_edge_realizations.reserve(
        edge_claims.size());
    for (const auto& claim : edge_claims) {
        result.inherited_edge_realizations
            .push_back(
                {
                    claim.source_token,
                    realizationLineageStatus(
                        claim.current_tokens.size(),
                        claim.aliased),
                    claim.current_tokens.size(),
                    claim.current_tokens,
                });
    }

    std::vector<
        RuntimeRealizationClaim<
            kernel::RuntimeVertexToken>>
        vertex_claims;
    vertex_claims.reserve(
        upstream.inventory_vertices.size());

    for (const auto& [source_value, vertex] :
         upstream.inventory_vertices) {
        RuntimeRealizationClaim<
            kernel::RuntimeVertexToken> claim;
        claim.source_token =
            kernel::RuntimeVertexToken{
                source_value};

        const auto descendants =
            descendantVertices(
                operation,
                vertex,
                shape,
                include_generated);
        for (const auto& descendant :
             descendants) {
            const auto token =
                inventoryVertexToken(
                    runtime,
                    descendant);
            if (!token) return false;
            appendUniqueToken(
                claim.current_tokens,
                *token);
        }
        vertex_claims.push_back(
            std::move(claim));
    }

    markAliasedRealizationClaims(
        vertex_claims);
    result.inherited_vertex_realizations.reserve(
        vertex_claims.size());
    for (const auto& claim : vertex_claims) {
        result.inherited_vertex_realizations
            .push_back(
                {
                    claim.source_token,
                    realizationLineageStatus(
                        claim.current_tokens.size(),
                        claim.aliased),
                    claim.current_tokens.size(),
                    claim.current_tokens,
                });
    }

    return true;
}

[[nodiscard]] bool populateCurrentTopologySemantics(
    kernel::SolidModelingResult& result,
    const OcctRuntimeSolid& runtime) {
    result.current_edge_semantics.clear();
    result.current_vertex_semantics.clear();

    result.current_edge_semantics.reserve(
        runtime.inventory_edges.size());
    for (const auto& [token_value, edge] :
         runtime.inventory_edges) {
        kernel::CurrentEdgeSemanticObservation
            observation;
        observation.runtime_token =
            kernel::RuntimeEdgeToken{
                token_value};
        observation.provider_curve_kind =
            providerCurveKind(edge);

        std::size_t partition_surface_count = 0U;
        for (const auto& [surface_value, surface] :
             runtime.tracked_surfaces) {
            std::size_t adjacent_face_count = 0U;
            bool seam = false;
            for (const auto& face :
                 surface.faces) {
                if (!faceContainsEdge(
                        face,
                        edge)) {
                    continue;
                }
                ++adjacent_face_count;
                if (BRepTools::IsReallyClosed(
                        edge,
                        face)) {
                    seam = true;
                }
            }
            if (adjacent_face_count > 0U) {
                appendUniqueToken(
                    observation.adjacent_surfaces,
                    kernel::RuntimeSurfaceToken{
                        surface_value});
            }
            if (adjacent_face_count >= 2U) {
                ++partition_surface_count;
            }
            observation.periodic_seam =
                observation.periodic_seam ||
                seam;
        }

        observation.same_surface_partition =
            !observation.periodic_seam &&
            observation.adjacent_surfaces.size() == 1U &&
            partition_surface_count == 1U;

        result.current_edge_semantics.push_back(
            std::move(observation));
    }

    result.current_vertex_semantics.reserve(
        runtime.inventory_vertices.size());
    for (const auto& [token_value, vertex] :
         runtime.inventory_vertices) {
        kernel::CurrentVertexSemanticObservation
            observation;
        observation.runtime_token =
            kernel::RuntimeVertexToken{
                token_value};

        for (const auto& [surface_value, surface] :
             runtime.tracked_surfaces) {
            const bool adjacent =
                std::any_of(
                    surface.faces.begin(),
                    surface.faces.end(),
                    [&vertex](
                        const TopoDS_Face& face) {
                        return faceContainsVertex(
                            face,
                            vertex);
                    });
            if (adjacent) {
                appendUniqueToken(
                    observation.adjacent_surfaces,
                    kernel::RuntimeSurfaceToken{
                        surface_value});
            }
        }

        for (const auto& edge_observation :
             result.current_edge_semantics) {
            if (edge_observation.periodic_seam ||
                edge_observation.same_surface_partition) {
                continue;
            }
            const auto found =
                runtime.inventory_edges.find(
                    edge_observation
                        .runtime_token.value);
            if (found ==
                runtime.inventory_edges.end()) {
                return false;
            }
            if (edgeContainsVertex(
                    found->second,
                    vertex)) {
                observation.incident_material_edges
                    .push_back(
                        edge_observation
                            .runtime_token);
            }
        }

        const auto point =
            BRep_Tool::Pnt(vertex);
        if (!std::isfinite(point.X()) ||
            !std::isfinite(point.Y()) ||
            !std::isfinite(point.Z())) {
            return false;
        }
        observation.provider_point =
            kernel::Point3{
                point.X(),
                point.Y(),
                point.Z()};

        result.current_vertex_semantics.push_back(
            std::move(observation));
    }

    if (result.current_edge_semantics.size() !=
            result.current_edges.size() ||
        result.current_vertex_semantics.size() !=
            result.current_vertices.size()) {
        return false;
    }

    for (const auto token :
         result.current_edges) {
        if (std::count_if(
                result.current_edge_semantics.begin(),
                result.current_edge_semantics.end(),
                [token](const auto& observation) {
                    return observation.runtime_token ==
                           token;
                }) != 1) {
            return false;
        }
    }
    for (const auto token :
         result.current_vertices) {
        if (std::count_if(
                result.current_vertex_semantics.begin(),
                result.current_vertex_semantics.end(),
                [token](const auto& observation) {
                    return observation.runtime_token ==
                           token;
                }) != 1) {
            return false;
        }
    }

    return true;
}

void populateDiagnostics(
    kernel::SolidModelingResult& result,
    const TopoDS_Shape& shape) {
    result.solid_count =
        countUniqueSubshapes(
            shape,
            TopAbs_SOLID);
    result.face_count =
        countUniqueSubshapes(
            shape,
            TopAbs_FACE);
    result.edge_count =
        countUniqueSubshapes(
            shape,
            TopAbs_EDGE);
    result.vertex_count =
        countUniqueSubshapes(
            shape,
            TopAbs_VERTEX);
    result.brep_valid =
        !shape.IsNull() &&
        BRepCheck_Analyzer{shape}.IsValid();
}

enum class VolumePresence {
    none,
    positive,
    invalid,
};

[[nodiscard]] VolumePresence volumePresence(
    const TopoDS_Shape& shape) {
    if (shape.IsNull()) {
        return VolumePresence::none;
    }

    bool found_solid = false;
    double total_volume = 0.0;
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_SOLID};
         explorer.More();
         explorer.Next()) {
        found_solid = true;
        const auto solid =
            TopoDS::Solid(
                explorer.Current());
        if (!BRepCheck_Analyzer{solid}.IsValid()) {
            return VolumePresence::invalid;
        }

        GProp_GProps properties;
        BRepGProp::VolumeProperties(
            solid,
            properties);
        const double volume =
            std::abs(properties.Mass());
        if (!std::isfinite(volume)) {
            return VolumePresence::invalid;
        }
        total_volume += volume;
        if (!std::isfinite(total_volume)) {
            return VolumePresence::invalid;
        }
    }

    if (!found_solid ||
        !(total_volume > 0.0)) {
        return VolumePresence::none;
    }
    return VolumePresence::positive;
}

template <typename Operation>
[[nodiscard]] bool upstreamExteriorUnchanged(
    Operation& operation,
    const TopoDS_Solid& upstream,
    const TopoDS_Shape& result) {
    if (countSubshapes(
            upstream,
            TopAbs_FACE) !=
        countSubshapes(
            result,
            TopAbs_FACE)) {
        return false;
    }

    for (TopExp_Explorer explorer{
             upstream,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        const auto source =
            TopoDS::Face(
                explorer.Current());
        const auto descendants =
            descendantFaces(
                operation,
                source,
                result);
        if (descendants.size() != 1U ||
            !descendants.front().IsSame(
                source)) {
            return false;
        }
    }
    return true;
}

template <typename Operation>
kernel::SolidModelingResult
finishBoolean(
    Operation& operation,
    const OcctRuntimeSolid& upstream,
    const std::vector<NewSemanticSource>& created,
    kernel::SolidBooleanOperation kind) {
    kernel::SolidModelingResult result;
    if (!operation.IsDone()) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    const auto shape = operation.Shape();
    if (shape.IsNull()) {
        result.status =
            kind ==
                    kernel::SolidBooleanOperation::
                        cut
                ? kernel::SolidModelingStatus::
                      empty_result
                : kernel::SolidModelingStatus::
                      provider_failure;
        return result;
    }

    populateDiagnostics(result, shape);

    if (kind ==
            kernel::SolidBooleanOperation::
                add &&
        result.solid_count > 1U) {
        result.status =
            kernel::SolidModelingStatus::
                detached_add;
        return result;
    }
    if (kind ==
            kernel::SolidBooleanOperation::
                cut &&
        result.solid_count == 0U) {
        result.status =
            kernel::SolidModelingStatus::
                empty_result;
        return result;
    }
    if (result.solid_count > 1U) {
        result.status =
            kernel::SolidModelingStatus::
                multi_solid;
        return result;
    }
    if (!result.brep_valid) {
        result.status =
            kernel::SolidModelingStatus::
                invalid_brep;
        return result;
    }

    const auto solid =
        singleSolid(shape);
    if (!solid) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (upstreamExteriorUnchanged(
            operation,
            upstream.solid,
            shape)) {
        result.status =
            kernel::SolidModelingStatus::
                no_effect;
        return result;
    }

    auto runtime =
        std::make_shared<OcctRuntimeSolid>();
    runtime->solid = *solid;

    publishLineage(
        result,
        *runtime,
        &upstream,
        created,
        [&operation, &shape](
            const TopoDS_Face& source) {
            return descendantFaces(
                operation,
                source,
                shape);
        });

    if (!populateRuntimeTopologyInventory(
            result,
            *runtime,
            runtime->solid)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!publishSurfaceLineage(
            result,
            *runtime,
            &upstream,
            created,
            kind ==
                kernel::SolidBooleanOperation::add,
            [&operation, &shape](
                const TopoDS_Face& source) {
                return descendantFaces(
                    operation,
                    source,
                    shape);
            })) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!publishCurrentSubshapeLineage(
            result,
            upstream,
            *runtime,
            operation,
            shape)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!populateCurrentTopologySemantics(
            result,
            *runtime)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    result.status =
        kernel::SolidModelingStatus::ok;
    result.solid = std::move(runtime);
    return result;
}

[[nodiscard]] bool appendFaceTriangles(
    const TopoDS_Face& face,
    kernel::SolidPresentationMesh& mesh,
    bool flat_delta_normals = false) {
    TopLoc_Location location;
    const Handle(Poly_Triangulation)
        triangulation =
            BRep_Tool::Triangulation(
                face,
                location);
    if (triangulation.IsNull()) {
        return false;
    }
    const auto transform =
        location.Transformation();

    // A finished Fillet Body can reach the same unsafe OCCT normal
    // mutation as the signed delta preview (Owner Part008 R2-P0).
    // Preserve provider-supplied smooth normals when they exist.
    // Otherwise, accumulate area-weighted facet normals per face-local
    // vertex *without modifying* shared Poly_Triangulation storage.
    // This keeps curved faces smoothly shaded and leaves B-Rep, Edge
    // identity and all modeling tolerances untouched.
    const bool missing_normals =
        !flat_delta_normals &&
        !triangulation->HasNormals();
    std::vector<gp_Vec> fallback_normals;
    if (missing_normals) {
        fallback_normals.resize(
            static_cast<std::size_t>(
                triangulation->NbNodes()) + 1U);
        for (Standard_Integer index = 1;
             index <= triangulation->NbTriangles();
             ++index) {
            Standard_Integer i0{};
            Standard_Integer i1{};
            Standard_Integer i2{};
            triangulation->Triangle(index).Get(i0, i1, i2);
            const gp_Pnt a =
                triangulation->Node(i0).Transformed(transform);
            const gp_Pnt b =
                triangulation->Node(i1).Transformed(transform);
            const gp_Pnt c =
                triangulation->Node(i2).Transformed(transform);
            const gp_Vec normal =
                gp_Vec{a, b}.Crossed(gp_Vec{a, c});
            const double length = normal.Magnitude();
            if (!std::isfinite(length) || !(length > 0.0)) {
                continue;
            }
            fallback_normals[static_cast<std::size_t>(i0)] += normal;
            fallback_normals[static_cast<std::size_t>(i1)] += normal;
            fallback_normals[static_cast<std::size_t>(i2)] += normal;
        }
    }

    const auto first_triangle =
        mesh.triangles.size();

    for (Standard_Integer index = 1;
         index <= triangulation->NbTriangles();
         ++index) {
        Standard_Integer first_index{};
        Standard_Integer second_index{};
        Standard_Integer third_index{};
        triangulation->Triangle(index).Get(
            first_index,
            second_index,
            third_index);

        gp_Pnt first =
            triangulation
                ->Node(first_index)
                .Transformed(transform);
        gp_Pnt second =
            triangulation
                ->Node(second_index)
                .Transformed(transform);
        gp_Pnt third =
            triangulation
                ->Node(third_index)
                .Transformed(transform);

        const gp_Vec first_edge{first, second};
        const gp_Vec second_edge{first, third};
        const gp_Vec cross =
            first_edge.Crossed(second_edge);
        const double magnitude = cross.Magnitude();
        if (!std::isfinite(magnitude) ||
            !(magnitude > 0.0)) {
            continue;
        }

        // Signed delta: flat normals. Committed Body: use untouched
        // provider normals, or the face-local smooth fallback above.
        const auto normal_at =
            [&](Standard_Integer node_index) {
                if (flat_delta_normals) {
                    return gp_Dir{cross};
                }
                if (!missing_normals) {
                    gp_Dir provider =
                        triangulation->Normal(node_index);
                    provider.Transform(transform);
                    return provider;
                }
                const gp_Vec& accumulated =
                    fallback_normals[
                        static_cast<std::size_t>(node_index)];
                const double length = accumulated.Magnitude();
                return std::isfinite(length) && length > 0.0
                    ? gp_Dir{accumulated}
                    : gp_Dir{cross};
            };
        gp_Dir first_normal = normal_at(first_index);
        gp_Dir second_normal = normal_at(second_index);
        gp_Dir third_normal = normal_at(third_index);

        if (face.Orientation() ==
            TopAbs_REVERSED) {
            std::swap(second, third);
            std::swap(
                second_normal,
                third_normal);
            first_normal.Reverse();
            second_normal.Reverse();
            third_normal.Reverse();
        }

        mesh.triangles.push_back(
            kernel::SolidMeshTriangle{
                {first.X(),
                 first.Y(),
                 first.Z()},
                {second.X(),
                 second.Y(),
                 second.Z()},
                {third.X(),
                 third.Y(),
                 third.Z()},
                {first_normal.X(),
                 first_normal.Y(),
                 first_normal.Z()},
                {second_normal.X(),
                 second_normal.Y(),
                 second_normal.Z()},
                {third_normal.X(),
                 third_normal.Y(),
                 third_normal.Z()}});
    }

    return mesh.triangles.size() >
           first_triangle;
}

[[nodiscard]] std::optional<std::vector<kernel::Point3>>
edgePresentationPoints(
    const TopoDS_Edge& edge) {
    BRepAdaptor_Curve curve{edge};
    // R2-E: display/pick sampling only. At 0.25 mm curve deflection
    // the production Revolve ring diverged 21.97 logical pixels from
    // its displayed material Edge at zoom scale 8, beyond the fixed
    // 8-pixel pick aperture (native Windows RED #1793).
    // Keep B-Rep/meshing and all CAD numerical tolerances unchanged.
    // A finer local presentation path preserves the accepted semantic
    // Edge identity and the Viewer's existing front-occlusion policy.
    constexpr double edge_display_deflection_mm = 0.01;
    GCPnts_QuasiUniformDeflection sampler{
        curve,
        edge_display_deflection_mm};
    if (!sampler.IsDone() ||
        sampler.NbPoints() < 2) {
        return std::nullopt;
    }

    std::vector<kernel::Point3> result;
    result.reserve(
        static_cast<std::size_t>(
            sampler.NbPoints()));
    for (Standard_Integer index = 1;
         index <= sampler.NbPoints();
         ++index) {
        const auto point =
            sampler.Value(index);
        result.push_back(
            {point.X(),
             point.Y(),
             point.Z()});
    }
    return result;
}

[[nodiscard]] kernel::SolidPresentationResult
presentationMeshForShape(
    const TopoDS_Shape& shape,
    bool flat_delta_normals = false) noexcept {
    kernel::SolidPresentationResult result;
    if (shape.IsNull()) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }

    try {
        constexpr double linear_deflection_mm = 0.25;
        constexpr double angular_deflection_rad = 0.35;

        BRepMesh_IncrementalMesh mesher{
            shape,
            linear_deflection_mm,
            false,
            angular_deflection_rad,
            true};
        mesher.Perform();
        if (!mesher.IsDone()) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }

        for (TopExp_Explorer solid_explorer{
                 shape,
                 TopAbs_SOLID};
             solid_explorer.More();
             solid_explorer.Next()) {
            const auto presentation_solid =
                TopoDS::Solid(
                    solid_explorer.Current());
            for (TopExp_Explorer explorer{
                     presentation_solid,
                     TopAbs_FACE};
                 explorer.More();
                 explorer.Next()) {
                if (!appendFaceTriangles(
                        TopoDS::Face(
                            explorer.Current()),
                        result.mesh,
                        flat_delta_normals)) {
                    result.status =
                        kernel::SolidPresentationStatus::
                            provider_failure;
                    result.mesh.triangles.clear();
                    return result;
                }
            }
        }

        if (!result.mesh.valid()) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            result.mesh.triangles.clear();
            return result;
        }

        result.status =
            kernel::SolidPresentationStatus::ok;
        return result;
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        result.mesh.triangles.clear();
        return result;
    } catch (...) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        result.mesh.triangles.clear();
        return result;
    }
}

} // namespace

kernel::SolidModelingResult
OcctSolidModelingKernel::extrude(
    const kernel::LinearExtrudeInput& input,
    kernel::RuntimeSolidHandle upstream) noexcept {
    kernel::SolidModelingResult result;
    if (!input.valid()) {
        result.status =
            kernel::SolidModelingStatus::
                invalid_input;
        return result;
    }

    if (input.operation ==
            kernel::SolidBooleanOperation::cut &&
        upstream == nullptr) {
        result.status =
            kernel::SolidModelingStatus::
                missing_upstream;
        return result;
    }

    const OcctRuntimeSolid* upstream_occt =
        nullptr;
    if (upstream != nullptr) {
        upstream_occt =
            dynamic_cast<
                const OcctRuntimeSolid*>(
                    upstream.get());
        if (upstream_occt == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_mismatch;
            return result;
        }
    }

    try {
        const auto tool =
            buildExtrudeTool(input);
        if (!tool) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        }

        const auto& tool_shape =
            tool->first;
        const auto& created =
            tool->second;

        if (upstream_occt == nullptr) {
            populateDiagnostics(
                result,
                tool_shape);
            if (result.solid_count != 1U) {
                result.status =
                    result.solid_count == 0U
                        ? kernel::SolidModelingStatus::
                              empty_result
                        : kernel::SolidModelingStatus::
                              multi_solid;
                return result;
            }
            if (!result.brep_valid) {
                result.status =
                    kernel::SolidModelingStatus::
                        invalid_brep;
                return result;
            }

            const auto solid =
                singleSolid(tool_shape);
            if (!solid) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            auto runtime =
                std::make_shared<
                    OcctRuntimeSolid>();
            runtime->solid = *solid;

            publishLineage(
                result,
                *runtime,
                nullptr,
                created,
                [&tool_shape](
                    const TopoDS_Face& source) {
                    std::vector<TopoDS_Face>
                        candidates;
                    if (containsSameFace(
                            tool_shape,
                            source)) {
                        candidates.push_back(
                            source);
                    }
                    return candidates;
                });

            if (!populateRuntimeTopologyInventory(
                    result,
                    *runtime,
                    runtime->solid)) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            if (!publishSurfaceLineage(
                    result,
                    *runtime,
                    nullptr,
                    created,
                    false,
                    [&tool_shape](
                        const TopoDS_Face& source) {
                        std::vector<TopoDS_Face>
                            candidates;
                        if (containsSameFace(
                                tool_shape,
                                source)) {
                            candidates.push_back(
                                source);
                        }
                        return candidates;
                    })) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            if (!populateCurrentTopologySemantics(
                    result,
                    *runtime)) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            result.status =
                kernel::SolidModelingStatus::ok;
            result.solid =
                std::move(runtime);
            return result;
        }

        if (input.operation ==
            kernel::SolidBooleanOperation::add) {
            BRepAlgoAPI_Fuse fuse{
                upstream_occt->solid,
                tool_shape};
            fuse.SetFuzzyValue(0.0);
            fuse.Build();
            return finishBoolean(
                fuse,
                *upstream_occt,
                created,
                input.operation);
        }

        // A Cut that only touches the Body by face/edge/point has no
        // volumetric modeling effect and must fail closed before we author it.
        BRepAlgoAPI_Common overlap{
            upstream_occt->solid,
            tool_shape};
        overlap.SetFuzzyValue(0.0);
        overlap.Build();
        if (!overlap.IsDone()) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        }
        switch (volumePresence(
                    overlap.Shape())) {
        case VolumePresence::none:
            result.status =
                kernel::SolidModelingStatus::
                    no_effect;
            return result;
        case VolumePresence::invalid:
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        case VolumePresence::positive:
            break;
        }

        BRepAlgoAPI_Cut cut{
            upstream_occt->solid,
            tool_shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        return finishBoolean(
            cut,
            *upstream_occt,
            created,
            input.operation);
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    } catch (...) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }
}


kernel::SolidModelingResult
OcctSolidModelingKernel::revolve(
    const kernel::AngularRevolveInput& input,
    kernel::RuntimeSolidHandle upstream) noexcept {
    kernel::SolidModelingResult result;
    if (!input.valid()) {
        result.status =
            kernel::SolidModelingStatus::
                invalid_input;
        return result;
    }

    if (input.operation ==
            kernel::SolidBooleanOperation::cut &&
        upstream == nullptr) {
        result.status =
            kernel::SolidModelingStatus::
                missing_upstream;
        return result;
    }

    const OcctRuntimeSolid* upstream_occt =
        nullptr;
    if (upstream != nullptr) {
        upstream_occt =
            dynamic_cast<
                const OcctRuntimeSolid*>(
                    upstream.get());
        if (upstream_occt == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_mismatch;
            return result;
        }
    }

    try {
        const auto tool =
            buildRevolveTool(input);
        if (!tool) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        }

        const auto& tool_shape =
            tool->first;
        const auto& created =
            tool->second;

        if (upstream_occt == nullptr) {
            populateDiagnostics(
                result,
                tool_shape);
            if (result.solid_count != 1U) {
                result.status =
                    result.solid_count == 0U
                        ? kernel::SolidModelingStatus::
                              empty_result
                        : kernel::SolidModelingStatus::
                              multi_solid;
                return result;
            }
            if (!result.brep_valid) {
                result.status =
                    kernel::SolidModelingStatus::
                        invalid_brep;
                return result;
            }

            const auto solid =
                singleSolid(tool_shape);
            if (!solid) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            auto runtime =
                std::make_shared<
                    OcctRuntimeSolid>();
            runtime->solid = *solid;

            publishLineage(
                result,
                *runtime,
                nullptr,
                created,
                [&tool_shape](
                    const TopoDS_Face& source) {
                    std::vector<TopoDS_Face>
                        candidates;
                    if (containsSameFace(
                            tool_shape,
                            source)) {
                        candidates.push_back(
                            source);
                    }
                    return candidates;
                });

            if (!populateRuntimeTopologyInventory(
                    result,
                    *runtime,
                    runtime->solid)) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            if (!publishSurfaceLineage(
                    result,
                    *runtime,
                    nullptr,
                    created,
                    false,
                    [&tool_shape](
                        const TopoDS_Face& source) {
                        std::vector<TopoDS_Face>
                            candidates;
                        if (containsSameFace(
                                tool_shape,
                                source)) {
                            candidates.push_back(
                                source);
                        }
                        return candidates;
                    })) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            if (!populateCurrentTopologySemantics(
                    result,
                    *runtime)) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }

            result.status =
                kernel::SolidModelingStatus::ok;
            result.solid =
                std::move(runtime);
            return result;
        }

        if (input.operation ==
            kernel::SolidBooleanOperation::add) {
            BRepAlgoAPI_Fuse fuse{
                upstream_occt->solid,
                tool_shape};
            fuse.SetFuzzyValue(0.0);
            fuse.Build();
            return finishBoolean(
                fuse,
                *upstream_occt,
                created,
                input.operation);
        }

        BRepAlgoAPI_Common overlap{
            upstream_occt->solid,
            tool_shape};
        overlap.SetFuzzyValue(0.0);
        overlap.Build();
        if (!overlap.IsDone()) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        }
        switch (volumePresence(
                    overlap.Shape())) {
        case VolumePresence::none:
            result.status =
                kernel::SolidModelingStatus::
                    no_effect;
            return result;
        case VolumePresence::invalid:
            result.status =
                kernel::SolidModelingStatus::
                    provider_failure;
            return result;
        case VolumePresence::positive:
            break;
        }

        BRepAlgoAPI_Cut cut{
            upstream_occt->solid,
            tool_shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        return finishBoolean(
            cut,
            *upstream_occt,
            created,
            input.operation);
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    } catch (...) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }
}

template <typename Operation>
[[nodiscard]] kernel::SolidModelingResult
finishEdgeFeature(
    Operation& operation,
    const OcctRuntimeSolid& upstream,
    const kernel::EdgeFeatureInput& input,
    const std::vector<SelectedRuntimeEdge>&
        selected) {
    kernel::SolidModelingResult result;
    if (!captureExactEdgeFeatureMembership(
            operation,
            upstream,
            input.edges,
            result)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_mismatch;
        return result;
    }

    operation.Build();
    if (!operation.IsDone()) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    const auto shape = operation.Shape();
    if (shape.IsNull()) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    populateDiagnostics(
        result,
        shape);
    if (result.solid_count == 0U) {
        result.status =
            kernel::SolidModelingStatus::
                empty_result;
        return result;
    }
    if (result.solid_count > 1U) {
        result.status =
            kernel::SolidModelingStatus::
                multi_solid;
        return result;
    }
    if (!result.brep_valid) {
#ifdef _DEBUG
        // Temporary Debug-only, opt-in forensic diagnostics.
        // Never use OCCT subshape index for authored CAD identity.
        if (input.operation ==
                kernel::EdgeFeatureOperation::chamfer &&
            input.edges.size() == 3U &&
            std::getenv("SS2_PM05F_R2_CHAMFER_TRIAGE")) {
            BRepCheck_Analyzer analyzer{shape};
            const auto scan =
                [&shape, &analyzer](TopAbs_ShapeEnum kind,
                                    const char* name) {
                    std::size_t total = 0U;
                    std::size_t invalid = 0U;
                    for (TopExp_Explorer it{shape, kind};
                         it.More(); it.Next()) {
                        ++total;
                        if (!analyzer.IsValid(it.Current())) {
                            ++invalid;
                            std::fprintf(
                                stderr,
                                "PM05F_R2_CHAMFER_INVALID %s index=%zu\n",
                                name, total);
                        }
                    }
                    std::fprintf(
                        stderr,
                        "PM05F_R2_CHAMFER_TOPOLOGY %s total=%zu invalid=%zu\n",
                        name, total, invalid);
                };
            scan(TopAbs_SOLID, "solid");
            scan(TopAbs_SHELL, "shell");
            scan(TopAbs_FACE, "face");
            scan(TopAbs_WIRE, "wire");
            scan(TopAbs_EDGE, "edge");
            scan(TopAbs_VERTEX, "vertex");
            // Diagnostic only: the symmetric Add(distance, edge)
            // failed. Test equal-distance Add(d,d,edge,sideFace) with
            // every adjacent upstream support face combination.
            // No result is accepted/published, no selection changes.
            std::vector<std::vector<TopoDS_Face>> supports;
            supports.reserve(selected.size());
            for (const auto& chosen : selected) {
                std::vector<TopoDS_Face> neighbors;
                for (const auto& [token, face] :
                     upstream.inventory_faces) {
                    static_cast<void>(token);
                    bool incident = false;
                    for (TopExp_Explorer e{face, TopAbs_EDGE};
                         e.More(); e.Next()) {
                        if (e.Current().IsSame(chosen.edge)) {
                            incident = true;
                            break;
                        }
                    }
                    if (incident) {
                        neighbors.push_back(face);
                    }
                }
                std::fprintf(
                    stderr,
                    "PM05F_R2_CHAMFER_SUPPORTS edge=%zu count=%zu\n",
                    supports.size(), neighbors.size());
                supports.push_back(std::move(neighbors));
            }
            if (supports.size() == 3U &&
                std::all_of(
                    supports.begin(), supports.end(),
                    [](const auto& faces) {
                        return faces.size() == 2U;
                    })) {
                for (unsigned mask = 0U; mask < 8U; ++mask) {
                    try {
                        BRepFilletAPI_MakeChamfer alternative{
                            upstream.solid};
                        for (std::size_t i = 0U; i < 3U; ++i) {
                            const auto selected_face =
                                supports[i][(mask >> i) & 1U];
                            alternative.Add(
                                input.parameter_mm,
                                input.parameter_mm,
                                selected[i].edge,
                                selected_face);
                        }
                        alternative.Build();
                        const bool done = alternative.IsDone();
                        const bool valid =
                            done &&
                            !alternative.Shape().IsNull() &&
                            BRepCheck_Analyzer{
                                alternative.Shape()}.IsValid();
                        std::fprintf(
                            stderr,
                            "PM05F_R2_CHAMFER_SIDE_COMBINATION mask=%u done=%d valid=%d\n",
                            mask, done ? 1 : 0, valid ? 1 : 0);
                        if (valid) {
                            const auto verified =
                                finishEdgeFeature(
                                    alternative,
                                    upstream,
                                    input,
                                    selected);
                            std::fprintf(
                                stderr,
                                "PM05F_R2_CHAMFER_SIDE_LINEAGE mask=%u status=%d surfaces=%zu membership=%d\n",
                                mask,
                                static_cast<int>(verified.status),
                                verified.edge_feature_surfaces.size(),
                                verified.edge_feature_input_membership &&
                                    verified.edge_feature_input_membership
                                        ->exactFor(input.edges)
                                    ? 1 : 0);
                        }
                    } catch (const Standard_Failure&) {
                        std::fprintf(
                            stderr,
                            "PM05F_R2_CHAMFER_SIDE_COMBINATION mask=%u OCC_EXCEPTION\n",
                            mask);
                    } catch (...) {
                        std::fprintf(
                            stderr,
                            "PM05F_R2_CHAMFER_SIDE_COMBINATION mask=%u EXCEPTION\n",
                            mask);
                    }
                }
            }
            // Diagnostic isolation of retained OCCT TShape sharing
            // after the authored Boolean Extrudes. Copying the upstream
            // without altering geometric coordinates gives an exact
            // TopoDS subshape map for each explicit selected Edge.
            // Never publish either trial or change durable semantics.
            for (const bool copy_geometry : {false, true}) {
                try {
                    BRepBuilderAPI_Copy copy{
                        upstream.solid,
                        copy_geometry,
                        false};
                    bool mapped = !copy.Shape().IsNull();
                    BRepFilletAPI_MakeChamfer clean{
                        copy.Shape()};
                    for (const auto& item : selected) {
                        const auto copied_edge =
                            copy.ModifiedShape(item.edge);
                        if (copied_edge.IsNull() ||
                            copied_edge.ShapeType() != TopAbs_EDGE) {
                            mapped = false;
                            break;
                        }
                        clean.Add(
                            input.parameter_mm,
                            TopoDS::Edge(copied_edge));
                    }
                    bool built = false;
                    bool valid = false;
                    if (mapped) {
                        clean.Build();
                        built = clean.IsDone();
                        valid = built &&
                            !clean.Shape().IsNull() &&
                            BRepCheck_Analyzer{
                                clean.Shape()}.IsValid();
                    }
                    std::fprintf(
                        stderr,
                        "PM05F_R2_CHAMFER_COPY copy_geometry=%d mapped=%d done=%d valid=%d\n",
                        copy_geometry ? 1 : 0,
                        mapped ? 1 : 0,
                        built ? 1 : 0,
                        valid ? 1 : 0);
                } catch (const Standard_Failure&) {
                    std::fprintf(
                        stderr,
                        "PM05F_R2_CHAMFER_COPY copy_geometry=%d OCCT_EXCEPTION\n",
                        copy_geometry ? 1 : 0);
                } catch (...) {
                    std::fprintf(
                        stderr,
                        "PM05F_R2_CHAMFER_COPY copy_geometry=%d EXCEPTION\n",
                        copy_geometry ? 1 : 0);
                }
            }
            std::fflush(stderr);
        }
#endif
        result.status =
            kernel::SolidModelingStatus::
                invalid_brep;
        return result;
    }

    const auto solid =
        singleSolid(shape);
    if (!solid) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (upstreamExteriorUnchanged(
            operation,
            upstream.solid,
            shape)) {
        result.status =
            kernel::SolidModelingStatus::
                no_effect;
        return result;
    }

    auto runtime =
        std::make_shared<OcctRuntimeSolid>();
    runtime->solid = *solid;

    // C2a preserves all inherited runtime semantic carriers. Generated
    // Fillet/Chamfer Surface claims are published separately in C2b.
    publishLineage(
        result,
        *runtime,
        &upstream,
        {},
        [&operation, &shape](
            const TopoDS_Face& source) {
            return descendantFaces(
                operation,
                source,
                shape,
                false);
        });

    if (!populateRuntimeTopologyInventory(
            result,
            *runtime,
            runtime->solid)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!publishSurfaceLineage(
            result,
            *runtime,
            &upstream,
            {},
            false,
            [&operation, &shape](
                const TopoDS_Face& source) {
                return descendantFaces(
                operation,
                source,
                shape,
                false);
            })) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!publishEdgeFeatureGeneratedSurfaces(
            result,
            *runtime,
            upstream,
            operation,
            input,
            selected)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!publishCurrentSubshapeLineage(
            result,
            upstream,
            *runtime,
            operation,
            shape,
            false)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    if (!populateCurrentTopologySemantics(
            result,
            *runtime)) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    result.status =
        kernel::SolidModelingStatus::ok;
    result.solid = std::move(runtime);
    return result;
}


kernel::SolidModelingResult
OcctSolidModelingKernel::edgeFeature(
    const kernel::EdgeFeatureInput& input,
    kernel::RuntimeSolidHandle upstream) noexcept {
    kernel::SolidModelingResult result;
    if (!input.valid()) {
        result.status =
            kernel::SolidModelingStatus::
                invalid_input;
        return result;
    }
    if (upstream == nullptr) {
        result.status =
            kernel::SolidModelingStatus::
                missing_upstream;
        return result;
    }

    const auto* upstream_occt =
        dynamic_cast<const OcctRuntimeSolid*>(
            upstream.get());
    if (upstream_occt == nullptr) {
        result.status =
            kernel::SolidModelingStatus::
                provider_mismatch;
        return result;
    }

    const auto selected =
        selectedRuntimeEdges(
            *upstream_occt,
            input.edges);
    if (!selected) {
        result.status =
            kernel::SolidModelingStatus::
                provider_mismatch;
        return result;
    }

    try {
        switch (input.operation) {
        case kernel::EdgeFeatureOperation::fillet: {
            BRepFilletAPI_MakeFillet operation{
                upstream_occt->solid};
            for (const auto& item : *selected) {
                operation.Add(
                    input.parameter_mm,
                    item.edge);
            }
            return finishEdgeFeature(
                operation,
                *upstream_occt,
                input,
                *selected);
        }
        case kernel::EdgeFeatureOperation::chamfer: {
            BRepFilletAPI_MakeChamfer operation{
                upstream_occt->solid};
            for (const auto& item : *selected) {
                operation.Add(
                    input.parameter_mm,
                    item.edge);
            }
            return finishEdgeFeature(
                operation,
                *upstream_occt,
                input,
                *selected);
        }
        }
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    } catch (...) {
        result.status =
            kernel::SolidModelingStatus::
                provider_failure;
        return result;
    }

    result.status =
        kernel::SolidModelingStatus::
            invalid_input;
    return result;
}


kernel::SolidPresentationResult
OcctSolidModelingKernel::extrudePreviewMesh(
    const kernel::LinearExtrudeInput& input,
    kernel::RuntimeSolidHandle upstream) noexcept {
    kernel::SolidPresentationResult result;
    if (!input.valid()) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }
    if (input.operation ==
            kernel::SolidBooleanOperation::cut &&
        upstream == nullptr) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }

    const OcctRuntimeSolid* upstream_occt =
        nullptr;
    if (upstream != nullptr) {
        upstream_occt =
            dynamic_cast<
                const OcctRuntimeSolid*>(
                    upstream.get());
        if (upstream_occt == nullptr) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_mismatch;
            return result;
        }
    }

    try {
        const auto tool =
            buildExtrudeTool(input);
        if (!tool) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }
        const auto& tool_shape =
            tool->first;

        TopoDS_Shape delta_shape;
        if (upstream_occt == nullptr) {
            delta_shape = tool_shape;
        } else if (
            input.operation ==
            kernel::SolidBooleanOperation::add) {
            BRepAlgoAPI_Cut delta{
                tool_shape,
                upstream_occt->solid};
            delta.SetFuzzyValue(0.0);
            delta.Build();
            if (!delta.IsDone()) {
                result.status =
                    kernel::SolidPresentationStatus::
                        provider_failure;
                return result;
            }
            delta_shape = delta.Shape();
        } else {
            BRepAlgoAPI_Common delta{
                tool_shape,
                upstream_occt->solid};
            delta.SetFuzzyValue(0.0);
            delta.Build();
            if (!delta.IsDone()) {
                result.status =
                    kernel::SolidPresentationStatus::
                        provider_failure;
                return result;
            }
            delta_shape = delta.Shape();
        }

        if (volumePresence(delta_shape) !=
            VolumePresence::positive) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }
        return presentationMeshForShape(
            delta_shape);
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        return result;
    } catch (...) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        return result;
    }
}

kernel::SolidPresentationResult
OcctSolidModelingKernel::revolvePreviewMesh(
    const kernel::AngularRevolveInput& input,
    kernel::RuntimeSolidHandle upstream) noexcept {
    kernel::SolidPresentationResult result;
    if (!input.valid()) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }
    if (input.operation ==
            kernel::SolidBooleanOperation::cut &&
        upstream == nullptr) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }

    const OcctRuntimeSolid* upstream_occt =
        nullptr;
    if (upstream != nullptr) {
        upstream_occt =
            dynamic_cast<
                const OcctRuntimeSolid*>(
                    upstream.get());
        if (upstream_occt == nullptr) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_mismatch;
            return result;
        }
    }

    try {
        const auto tool =
            buildRevolveTool(input);
        if (!tool) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }
        const auto& tool_shape =
            tool->first;

        TopoDS_Shape delta_shape;
        if (upstream_occt == nullptr) {
            delta_shape = tool_shape;
        } else if (
            input.operation ==
            kernel::SolidBooleanOperation::add) {
            BRepAlgoAPI_Cut delta{
                tool_shape,
                upstream_occt->solid};
            delta.SetFuzzyValue(0.0);
            delta.Build();
            if (!delta.IsDone()) {
                result.status =
                    kernel::SolidPresentationStatus::
                        provider_failure;
                return result;
            }
            delta_shape = delta.Shape();
        } else {
            BRepAlgoAPI_Common delta{
                tool_shape,
                upstream_occt->solid};
            delta.SetFuzzyValue(0.0);
            delta.Build();
            if (!delta.IsDone()) {
                result.status =
                    kernel::SolidPresentationStatus::
                        provider_failure;
                return result;
            }
            delta_shape = delta.Shape();
        }

        if (volumePresence(delta_shape) !=
            VolumePresence::positive) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }
        return presentationMeshForShape(
            delta_shape);
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        return result;
    } catch (...) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        return result;
    }
}

kernel::SolidMaterialDeltaPresentationResult
OcctSolidModelingKernel::materialDifferencePreview(
    kernel::RuntimeSolidHandle before,
    kernel::RuntimeSolidHandle after) noexcept {
    kernel::SolidMaterialDeltaPresentationResult result;
    if (!before || !after) {
        result.status =
            kernel::SolidPresentationStatus::invalid_input;
        return result;
    }

    const auto* first =
        dynamic_cast<const OcctRuntimeSolid*>(before.get());
    const auto* second =
        dynamic_cast<const OcctRuntimeSolid*>(after.get());
    if (!first || !second) {
        result.status =
            kernel::SolidPresentationStatus::provider_mismatch;
        return result;
    }

    // Exact transient B-Rep differences, never triangle subtraction.
    // Do not change modeling tolerance or use fuzzy/healing fallback.
    try {
        const auto cut_mesh =
            [](const TopoDS_Solid& a,
               const TopoDS_Solid& b)
                -> std::optional<
                    kernel::SolidPresentationResult> {
                BRepAlgoAPI_Cut difference{a, b};
                difference.SetFuzzyValue(0.0);
                difference.Build();
                if (!difference.IsDone()) {
                    return std::nullopt;
                }
                const auto& shape = difference.Shape();
                switch (volumePresence(shape)) {
                case VolumePresence::none:
                    return kernel::SolidPresentationResult{
                        kernel::SolidPresentationStatus::ok, {}};
                case VolumePresence::positive:
                    return presentationMeshForShape(
                        shape, true);
                case VolumePresence::invalid:
                    return std::nullopt;
                }
                return std::nullopt;
            };

        const auto removed =
            cut_mesh(first->solid, second->solid);
        const auto added =
            cut_mesh(second->solid, first->solid);
        if (!removed || !added ||
            removed->status != kernel::SolidPresentationStatus::ok ||
            added->status != kernel::SolidPresentationStatus::ok) {
            result.status =
                kernel::SolidPresentationStatus::provider_failure;
            return result;
        }

        if (removed->mesh.valid()) {
            result.removed = removed->mesh;
        }
        if (added->mesh.valid()) {
            result.added = added->mesh;
        }
        // A valid but exactly unchanged result is not visual evidence
        // of any Edge Feature effect. Do not fabricate a colored Body.
        result.status =
            (result.removed || result.added)
                ? kernel::SolidPresentationStatus::ok
                : kernel::SolidPresentationStatus::unsupported;
        return result;
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidPresentationStatus::provider_failure;
    } catch (...) {
        result.status =
            kernel::SolidPresentationStatus::provider_failure;
    }
    return result;
}

kernel::SolidPresentationResult
OcctSolidModelingKernel::presentationMesh(
    kernel::RuntimeSolidHandle solid) noexcept {
    kernel::SolidPresentationResult result;
    if (solid == nullptr) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }

    const auto* runtime =
        dynamic_cast<
            const OcctRuntimeSolid*>(
                solid.get());
    if (runtime == nullptr) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_mismatch;
        return result;
    }

    return presentationMeshForShape(
        runtime->solid);
}

kernel::BodyPresentationResult
OcctSolidModelingKernel::bodyPresentation(
    kernel::RuntimeSolidHandle solid) noexcept {
    kernel::BodyPresentationResult result;
    if (solid == nullptr) {
        result.status =
            kernel::SolidPresentationStatus::
                invalid_input;
        return result;
    }

    const auto* runtime =
        dynamic_cast<
            const OcctRuntimeSolid*>(
                solid.get());
    if (runtime == nullptr) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_mismatch;
        return result;
    }

    try {
        constexpr double linear_deflection_mm = 0.25;
        constexpr double angular_deflection_rad = 0.35;

        BRepMesh_IncrementalMesh mesher{
            runtime->solid,
            linear_deflection_mm,
            false,
            angular_deflection_rad,
            true};
        mesher.Perform();
        if (!mesher.IsDone()) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            return result;
        }

        result.body.faces.reserve(
            runtime->inventory_faces.size());
        for (const auto& [token_value, face] :
             runtime->inventory_faces) {
            const auto first_triangle =
                result.body.mesh.triangles.size();
            if (!appendFaceTriangles(
                    face,
                    result.body.mesh)) {
                result.status =
                    kernel::SolidPresentationStatus::
                        provider_failure;
                result.body = {};
                return result;
            }
            result.body.faces.push_back(
                kernel::BodyFacePresentationRange{
                    kernel::RuntimeFaceToken{
                        token_value},
                    first_triangle,
                    result.body.mesh.triangles.size() -
                        first_triangle});
        }

        result.body.edges.reserve(
            runtime->inventory_edges.size());
        for (const auto& [token_value, edge] :
             runtime->inventory_edges) {
            auto points =
                edgePresentationPoints(edge);
            if (!points) {
                result.status =
                    kernel::SolidPresentationStatus::
                        provider_failure;
                result.body = {};
                return result;
            }
            result.body.edges.push_back(
                kernel::BodyEdgePresentationPath{
                    kernel::RuntimeEdgeToken{
                        token_value},
                    std::move(*points)});
        }

        result.body.vertices.reserve(
            runtime->inventory_vertices.size());
        for (const auto& [token_value, vertex] :
             runtime->inventory_vertices) {
            const auto point =
                BRep_Tool::Pnt(vertex);
            result.body.vertices.push_back(
                kernel::BodyVertexPresentationPoint{
                    kernel::RuntimeVertexToken{
                        token_value},
                    {point.X(),
                     point.Y(),
                     point.Z()}});
        }

        if (!result.body.valid()) {
            result.status =
                kernel::SolidPresentationStatus::
                    provider_failure;
            result.body = {};
            return result;
        }

        result.status =
            kernel::SolidPresentationStatus::ok;
        return result;
    } catch (const Standard_Failure&) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        result.body = {};
        return result;
    } catch (...) {
        result.status =
            kernel::SolidPresentationStatus::
                provider_failure;
        result.body = {};
        return result;
    }
}

} // namespace simplesolid2::kernel_occt
