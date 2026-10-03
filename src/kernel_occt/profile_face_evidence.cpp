#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepSweep_Prism.hxx>
#include <BRepSweep_Revol.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>
#include <iostream>
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

[[nodiscard]] gp_Vec vector3(
    const kernel::Frame3& frame,
    const kernel::Point2& direction) {
    return {
        frame.u_axis.x * direction.u +
            frame.v_axis.x * direction.v,
        frame.u_axis.y * direction.u +
            frame.v_axis.y * direction.v,
        frame.u_axis.z * direction.u +
            frame.v_axis.z * direction.v,
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
                double delta =
                    curve.sweep_angle *
                    (use.end_parameter -
                     use.start_parameter);
                if (!use.follows_source_direction) {
                    delta = -delta;
                }
                return circularEdge(
                    circle,
                    start_angle,
                    delta,
                    false);
            }
        },
        use.curve);
}

struct SourceEdgeEvidence final {
    TopoDS_Edge edge;
    kernel::BoundaryUseProvenance provenance;
};

struct WireEvidence final {
    TopoDS_Wire wire;
    std::vector<kernel::BoundaryLineageEvidence>
        lineage;
    std::vector<SourceEdgeEvidence> source_edges;
};

[[nodiscard]] std::optional<WireEvidence>
buildWire(
    const kernel::PlanarProfileInput& input,
    const kernel::ProfileLoopInput& loop) {
    BRepBuilderAPI_MakeWire make_wire;
    WireEvidence result;
    result.lineage.reserve(loop.boundary.size());
    result.source_edges.reserve(loop.boundary.size());

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
        // MakeWire may replace a geometrically coincident vertex
        // and therefore copy the supplied edge. Keep the exact edge that
        // the builder reports as added to the transient wire; semantic
        // identity remains use.provenance.
        result.source_edges.push_back({
            make_wire.Edge(),
            use.provenance,
        });
    }

    result.wire = make_wire.Wire();
    return result;
}

struct ProfileFaceBuild final {
    TopoDS_Face face;
    std::vector<kernel::BoundaryLineageEvidence>
        lineage;
    std::vector<SourceEdgeEvidence> source_edges;
};

[[nodiscard]] std::optional<ProfileFaceBuild>
buildProfileFace(
    const kernel::PlanarProfileInput& input) {
    const auto outer =
        buildWire(
            input,
            input.outer);
    if (!outer) {
        return std::nullopt;
    }

    BRepBuilderAPI_MakeFace make_face{
        outer->wire,
        true};
    if (!make_face.IsDone()) {
        return std::nullopt;
    }

    ProfileFaceBuild result;
    result.lineage = outer->lineage;
    result.source_edges = outer->source_edges;

    for (const auto& hole :
         input.holes) {
        auto built =
            buildWire(input, hole);
        if (!built) {
            return std::nullopt;
        }

        auto hole_wire =
            built->wire;
        hole_wire.Reverse();
        make_face.Add(hole_wire);

        result.lineage.insert(
            result.lineage.end(),
            built->lineage.begin(),
            built->lineage.end());
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
    std::size_t count = 0U;
    for (TopExp_Explorer explorer{shape, kind};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

[[nodiscard]] std::size_t countShapeOrSubshapes(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    if (shape.IsNull()) {
        return 0U;
    }
    if (shape.ShapeType() == kind) {
        return 1U;
    }
    return countSubshapes(shape, kind);
}

[[nodiscard]] std::vector<TopoDS_Face>
facesFromGeneratedShape(
    const TopoDS_Shape& shape) {
    std::vector<TopoDS_Face> faces;
    if (shape.IsNull()) {
        return faces;
    }

    if (shape.ShapeType() == TopAbs_FACE) {
        faces.push_back(TopoDS::Face(shape));
        return faces;
    }

    for (TopExp_Explorer explorer{
             shape,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        faces.push_back(
            TopoDS::Face(
                explorer.Current()));
    }
    return faces;
}

[[nodiscard]] kernel::FaceSurfaceKind
surfaceKind(
    GeomAbs_SurfaceType type) noexcept {
    switch (type) {
    case GeomAbs_Plane:
        return kernel::FaceSurfaceKind::plane;
    case GeomAbs_Cylinder:
        return kernel::FaceSurfaceKind::cylinder;
    case GeomAbs_Cone:
        return kernel::FaceSurfaceKind::cone;
    case GeomAbs_Sphere:
        return kernel::FaceSurfaceKind::sphere;
    case GeomAbs_Torus:
        return kernel::FaceSurfaceKind::torus;
    default:
        return kernel::FaceSurfaceKind::other;
    }
}

[[nodiscard]] kernel::Point3 canonicalAxis(
    const gp_Dir& direction) noexcept {
    double x = direction.X();
    double y = direction.Y();
    double z = direction.Z();

    const double ax = std::abs(x);
    const double ay = std::abs(y);
    const double az = std::abs(z);

    bool flip = false;
    if (ax >= ay && ax >= az) {
        flip = x < 0.0;
    } else if (ay >= az) {
        flip = y < 0.0;
    } else {
        flip = z < 0.0;
    }

    if (flip) {
        x = -x;
        y = -y;
        z = -z;
    }

    return {x, y, z};
}

[[nodiscard]] kernel::FaceGeometryDiagnostics
faceGeometryDiagnostics(
    const TopoDS_Face& face) {
    GProp_GProps properties;
    BRepGProp::SurfaceProperties(
        face,
        properties);

    const auto center =
        properties.CentreOfMass();

    BRepAdaptor_Surface surface{
        face,
        true};
    const auto type =
        surface.GetType();

    kernel::Point3 axis{};
    switch (type) {
    case GeomAbs_Plane:
        axis = canonicalAxis(
            surface.Plane()
                .Axis()
                .Direction());
        break;
    case GeomAbs_Cylinder:
        axis = canonicalAxis(
            surface.Cylinder()
                .Axis()
                .Direction());
        break;
    case GeomAbs_Cone:
        axis = canonicalAxis(
            surface.Cone()
                .Axis()
                .Direction());
        break;
    case GeomAbs_Torus:
        axis = canonicalAxis(
            surface.Torus()
                .Axis()
                .Direction());
        break;
    default:
        break;
    }

    return {
        surfaceKind(type),
        properties.Mass(),
        {center.X(), center.Y(), center.Z()},
        axis};
}

struct SeamDiagnostics final {
    std::size_t edge_count{};
    double total_length{};
};

[[nodiscard]] SeamDiagnostics
seamDiagnostics(
    const TopoDS_Face& face) {
    SeamDiagnostics result;
    std::vector<TopoDS_Edge> unique;

    for (TopExp_Explorer explorer{
             face,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto edge =
            TopoDS::Edge(
                explorer.Current());

        if (!BRepTools::IsReallyClosed(
                edge,
                face)) {
            continue;
        }

        const bool seen =
            std::any_of(
                unique.begin(),
                unique.end(),
                [&edge](
                    const TopoDS_Edge&
                        existing) {
                    return existing.IsSame(edge);
                });
        if (seen) {
            continue;
        }

        unique.push_back(edge);
        ++result.edge_count;

        GProp_GProps properties;
        BRepGProp::LinearProperties(
            edge,
            properties);
        result.total_length +=
            properties.Mass();
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

[[nodiscard]] bool nearPoint(
    const gp_Pnt& first,
    const gp_Pnt& second,
    double tolerance = 1.0e-7) noexcept {
    return first.Distance(second) <= tolerance;
}

[[nodiscard]] std::optional<TopoDS_Edge>
findEdgeByEndpoints(
    const TopoDS_Shape& shape,
    const gp_Pnt& first,
    const gp_Pnt& second) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto edge =
            TopoDS::Edge(
                explorer.Current());

        TopoDS_Vertex v1;
        TopoDS_Vertex v2;
        TopExp::Vertices(
            edge,
            v1,
            v2);
        if (v1.IsNull() || v2.IsNull()) {
            continue;
        }

        const auto p1 =
            BRep_Tool::Pnt(v1);
        const auto p2 =
            BRep_Tool::Pnt(v2);

        if ((nearPoint(p1, first) &&
             nearPoint(p2, second)) ||
            (nearPoint(p1, second) &&
             nearPoint(p2, first))) {
            return edge;
        }
    }
    return std::nullopt;
}

[[nodiscard]] std::vector<TopoDS_Edge>
findEdgesByEndpoints(
    const TopoDS_Shape& shape,
    const gp_Pnt& first,
    const gp_Pnt& second) {
    std::vector<TopoDS_Edge> matches;
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto edge =
            TopoDS::Edge(
                explorer.Current());

        TopoDS_Vertex v1;
        TopoDS_Vertex v2;
        TopExp::Vertices(
            edge,
            v1,
            v2);
        if (v1.IsNull() || v2.IsNull()) {
            continue;
        }

        const auto p1 =
            BRep_Tool::Pnt(v1);
        const auto p2 =
            BRep_Tool::Pnt(v2);

        if ((nearPoint(p1, first) &&
             nearPoint(p2, second)) ||
            (nearPoint(p1, second) &&
             nearPoint(p2, first))) {
            matches.push_back(edge);
        }
    }
    return matches;
}

[[nodiscard]] std::optional<TopoDS_Face>
findFaceByCentroid(
    const TopoDS_Shape& shape,
    const kernel::Point3& expected,
    double tolerance = 1.0e-7) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        const auto face =
            TopoDS::Face(
                explorer.Current());
        GProp_GProps properties;
        BRepGProp::SurfaceProperties(
            face,
            properties);
        const auto center =
            properties.CentreOfMass();

        if (std::abs(center.X() - expected.x) <=
                tolerance &&
            std::abs(center.Y() - expected.y) <=
                tolerance &&
            std::abs(center.Z() - expected.z) <=
                tolerance) {
            return face;
        }
    }
    return std::nullopt;
}

void appendUniqueShape(
    std::vector<TopoDS_Shape>& shapes,
    const TopoDS_Shape& candidate) {
    if (candidate.IsNull()) {
        return;
    }
    const auto found =
        std::find_if(
            shapes.begin(),
            shapes.end(),
            [&candidate](
                const TopoDS_Shape& existing) {
                return existing.IsSame(candidate);
            });
    if (found == shapes.end()) {
        shapes.push_back(candidate);
    }
}

[[nodiscard]] std::vector<TopoDS_Shape>
collectHistoryShapes(
    const TopTools_ListOfShape& history,
    TopAbs_ShapeEnum kind) {
    std::vector<TopoDS_Shape> result;
    for (TopTools_ListOfShape::Iterator it{
             history};
         it.More();
         it.Next()) {
        const auto& item = it.Value();
        if (item.ShapeType() == kind) {
            appendUniqueShape(
                result,
                item);
            continue;
        }

        for (TopExp_Explorer explorer{
                 item,
                 kind};
             explorer.More();
             explorer.Next()) {
            appendUniqueShape(
                result,
                explorer.Current());
        }
    }
    return result;
}

[[nodiscard]] bool containsSameSubshape(
    const TopoDS_Shape& result,
    const TopoDS_Shape& source,
    TopAbs_ShapeEnum kind) {
    for (TopExp_Explorer explorer{
             result,
             kind};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(source)) {
            return true;
        }
    }
    return false;
}

template <typename Operation>
[[nodiscard]] kernel::BooleanSubshapeHistoryEvidence
historyEvidence(
    Operation& operation,
    const TopoDS_Shape& source,
    TopAbs_ShapeEnum kind,
    const TopoDS_Shape& result) {
    const auto modified =
        collectHistoryShapes(
            operation.Modified(source),
            kind);
    const auto generated =
        collectHistoryShapes(
            operation.Generated(source),
            kind);
    const bool unchanged =
        containsSameSubshape(
            result,
            source,
            kind);

    std::vector<TopoDS_Shape> unique;
    unique.reserve(
        modified.size() +
        generated.size() +
        (unchanged ? 1U : 0U));

    for (const auto& shape : modified) {
        appendUniqueShape(unique, shape);
    }
    for (const auto& shape : generated) {
        appendUniqueShape(unique, shape);
    }
    if (unchanged) {
        appendUniqueShape(unique, source);
    }

    return {
        modified.size(),
        generated.size(),
        operation.IsDeleted(source),
        unchanged,
        unique.size()};
}

template <typename Operation>
[[nodiscard]] std::vector<TopoDS_Shape>
historyDescendants(
    Operation& operation,
    const TopoDS_Shape& source,
    TopAbs_ShapeEnum kind,
    const TopoDS_Shape& result) {
    auto unique =
        collectHistoryShapes(
            operation.Modified(source),
            kind);
    const auto generated =
        collectHistoryShapes(
            operation.Generated(source),
            kind);
    for (const auto& shape : generated) {
        appendUniqueShape(unique, shape);
    }
    if (containsSameSubshape(
            result,
            source,
            kind)) {
        appendUniqueShape(
            unique,
            source);
    }
    return unique;
}

[[nodiscard]] std::size_t sharedShapeCount(
    const std::vector<TopoDS_Shape>& first,
    const std::vector<TopoDS_Shape>& second) {
    std::size_t count = 0U;
    for (const auto& left : first) {
        const auto found =
            std::find_if(
                second.begin(),
                second.end(),
                [&left](
                    const TopoDS_Shape& right) {
                    return left.IsSame(right);
                });
        if (found != second.end()) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] kernel::ReferenceStatus
referenceStatus(
    std::size_t candidate_count) noexcept {
    if (candidate_count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (candidate_count == 1U) {
        return kernel::ReferenceStatus::resolved;
    }
    return kernel::ReferenceStatus::ambiguous;
}

void populateShapeEvidence(
    kernel::ShapeEvidence& evidence,
    const TopoDS_Shape& shape) {
    const BRepCheck_Analyzer analyzer{shape};
    evidence.brep_valid =
        analyzer.IsValid();
    evidence.solid_count =
        countSubshapes(
            shape,
            TopAbs_SOLID);
    evidence.face_count =
        countSubshapes(
            shape,
            TopAbs_FACE);
    evidence.wire_count =
        countSubshapes(
            shape,
            TopAbs_WIRE);
    evidence.edge_count =
        countSubshapes(
            shape,
            TopAbs_EDGE);
    evidence.status =
        evidence.brep_valid
            ? kernel::EvidenceStatus::ok
            : kernel::EvidenceStatus::invalid_brep;
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
        const auto built =
            buildProfileFace(input);
        if (!built) {
            evidence.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        evidence.boundary_lineage =
            built->lineage;
        populateShapeEvidence(
            evidence,
            built->face);
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

kernel::ExtrudeEvidence buildProfileExtrudeEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept {
    kernel::ExtrudeEvidence evidence;
    if (!input.valid() ||
        !std::isfinite(distance) ||
        distance == 0.0) {
        evidence.shape.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const auto built =
            buildProfileFace(input);
        if (!built) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const gp_Vec vector{
            input.frame.normal.x * distance,
            input.frame.normal.y * distance,
            input.frame.normal.z * distance};

        BRepSweep_Prism sweep{
            built->face,
            vector,
            false,
            true};

        const TopoDS_Shape shape =
            sweep.Shape();
        if (shape.IsNull()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            shape);
        if (!evidence.shape.ok()) {
            return evidence;
        }

        const auto start_count =
            countSubshapes(
                sweep.FirstShape(),
                TopAbs_FACE);
        const auto end_count =
            countSubshapes(
                sweep.LastShape(),
                TopAbs_FACE);

        evidence.start_cap = {
            kernel::ExtrudeFaceRoleKind::start_cap,
            referenceStatus(start_count),
            start_count,
            std::nullopt};
        evidence.end_cap = {
            kernel::ExtrudeFaceRoleKind::end_cap,
            referenceStatus(end_count),
            end_count,
            std::nullopt};

        evidence.sides.reserve(
            built->source_edges.size());
        for (const auto& source :
             built->source_edges) {
            std::vector<TopoDS_Face>
                candidate_faces;
            std::size_t generated_shape_count = 0U;
            const auto basis_edges =
                matchingFaceEdges(
                    built->face,
                    source.edge);
            for (const auto& basis_edge :
                 basis_edges) {
                // The high-level MakePrism Generated(edge) history can omit
                // an exact basis edge. Query the concrete transient sweep
                // directly for the shape generated from that exact edge.
                // Semantic identity remains source.provenance.
                const TopoDS_Shape generated =
                    sweep.Shape(
                        basis_edge);
                if (!generated.IsNull()) {
                    ++generated_shape_count;
                    auto generated_faces =
                        facesFromGeneratedShape(
                            generated);
                    candidate_faces.insert(
                        candidate_faces.end(),
                        generated_faces.begin(),
                        generated_faces.end());
                }
            }

            const auto face_count =
                candidate_faces.size();

            if (face_count != 1U) {
                std::cerr
                    << "E01_PROVIDER_DIAG source="
                    << source.provenance.source_entity
                    << " hole="
                    << source.provenance.hole
                    << " basis_matches="
                    << basis_edges.size()
                    << " generated_shapes="
                    << generated_shape_count
                    << " generated_faces="
                    << face_count
                    << '\n';
            }

            std::optional<
                kernel::FaceGeometryDiagnostics>
                diagnostics;
            if (candidate_faces.size() == 1U) {
                diagnostics =
                    faceGeometryDiagnostics(
                        candidate_faces.front());
            }

            evidence.sides.push_back({
                kernel::ExtrudeFaceRoleKind::side,
                referenceStatus(face_count),
                face_count,
                source.provenance,
                basis_edges.size(),
                generated_shape_count,
                diagnostics});
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::FullRevolveEvidence
buildProfileFullRevolveEvidence(
    const kernel::PlanarProfileInput& input,
    kernel::Point2 axis_origin,
    kernel::Point2 axis_direction) noexcept {
    kernel::FullRevolveEvidence evidence;

    if (!input.valid() ||
        !std::isfinite(axis_origin.u) ||
        !std::isfinite(axis_origin.v) ||
        !std::isfinite(axis_direction.u) ||
        !std::isfinite(axis_direction.v)) {
        evidence.shape.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const auto direction =
            vector3(
                input.frame,
                axis_direction);
        if (!(direction.SquareMagnitude() > 0.0)) {
            evidence.shape.status =
                kernel::EvidenceStatus::invalid_input;
            return evidence;
        }

        const auto built =
            buildProfileFace(input);
        if (!built) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const gp_Ax1 axis{
            point3(
                input.frame,
                axis_origin),
            gp_Dir{direction}};

        BRepSweep_Revol sweep{
            built->face,
            axis,
            false};

        const TopoDS_Shape shape =
            sweep.Shape();
        if (shape.IsNull()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            shape);
        if (!evidence.shape.ok()) {
            return evidence;
        }

        evidence.boundary_faces.reserve(
            built->source_edges.size());

        for (const auto& source :
             built->source_edges) {
            std::vector<TopoDS_Face>
                candidate_faces;

            const auto basis_edges =
                matchingFaceEdges(
                    built->face,
                    source.edge);
            for (const auto& basis_edge :
                 basis_edges) {
                const TopoDS_Shape generated =
                    sweep.Shape(
                        basis_edge);
                if (generated.IsNull()) {
                    continue;
                }

                auto generated_faces =
                    facesFromGeneratedShape(
                        generated);
                candidate_faces.insert(
                    candidate_faces.end(),
                    generated_faces.begin(),
                    generated_faces.end());
            }

            kernel::RevolveBoundaryFaceEvidence
                boundary;
            boundary.status =
                referenceStatus(
                    candidate_faces.size());
            boundary.candidate_face_count =
                candidate_faces.size();
            boundary.provenance =
                source.provenance;

            if (candidate_faces.size() == 1U) {
                const auto& face =
                    candidate_faces.front();
                boundary.geometry_diagnostics =
                    faceGeometryDiagnostics(face);

                const auto seams =
                    seamDiagnostics(face);
                boundary.periodic_surface =
                    seams.edge_count > 0U;
                boundary.seam_edge_count =
                    seams.edge_count;
                boundary.seam_total_length =
                    seams.total_length;

                evidence.provider_seam_edge_count +=
                    seams.edge_count;
                evidence.provider_seam_total_length +=
                    seams.total_length;
            } else {
                std::cerr
                    << "E06_PROVIDER_DIAG source="
                    << source.provenance.source_entity
                    << " basis_matches="
                    << basis_edges.size()
                    << " generated_faces="
                    << candidate_faces.size()
                    << '\n';
            }

            evidence.boundary_faces.push_back(
                std::move(boundary));
        }

        // The seam is provider-created periodic topology with no semantic
        // source record. It remains deliberately non-addressable.
        evidence.periodic_seam_reference_status =
            kernel::ReferenceStatus::unsupported;

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::EdgeSplitHistoryEvidence
buildEdgeSplitHistoryEvidence(
    kernel::EdgeSplitProbeScenario scenario) noexcept {
    kernel::EdgeSplitHistoryEvidence evidence;

    try {
        BRepPrimAPI_MakeBox base{
            gp_Pnt{0.0, 0.0, 0.0},
            40.0,
            20.0,
            10.0};
        const TopoDS_Shape base_shape =
            base.Shape();

        const auto target =
            findEdgeByEndpoints(
                base_shape,
                gp_Pnt{0.0, 0.0, 10.0},
                gp_Pnt{40.0, 0.0, 10.0});
        if (!target) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const bool middle_notch =
            scenario ==
            kernel::EdgeSplitProbeScenario::
                middle_notch;
        const gp_Pnt tool_origin =
            middle_notch
                ? gp_Pnt{15.0, -5.0, 5.0}
                : gp_Pnt{-5.0, -5.0, 5.0};
        const double tool_dx =
            middle_notch ? 10.0 : 50.0;
        const double tool_dy =
            middle_notch ? 10.0 : 30.0;

        BRepPrimAPI_MakeBox tool{
            tool_origin,
            tool_dx,
            tool_dy,
            10.0};

        BRepAlgoAPI_Cut cut{
            base_shape,
            tool.Shape()};
        cut.Build();
        if (!cut.IsDone()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const TopoDS_Shape result =
            cut.Shape();
        if (result.IsNull()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            result);
        evidence.target =
            historyEvidence(
                cut,
                *target,
                TopAbs_EDGE,
                result);
        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::FaceMergeHistoryEvidence
buildFaceMergeHistoryEvidence(
    kernel::FaceMergeProbeScenario scenario) noexcept {
    kernel::FaceMergeHistoryEvidence evidence;

    try {
        BRepPrimAPI_MakeBox first_box{
            gp_Pnt{0.0, 0.0, 0.0},
            40.0,
            20.0,
            10.0};

        const TopoDS_Shape first_shape =
            first_box.Shape();
        const auto first_face =
            findFaceByCentroid(
                first_shape,
                {20.0, 10.0, 10.0});
        if (!first_face) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        TopoDS_Shape second_shape;
        std::optional<TopoDS_Face>
            second_face;

        if (scenario ==
            kernel::FaceMergeProbeScenario::
                overlapping_coplanar) {
            // Fully redundant coplanar top region. OCCT reports both source
            // faces Modified to the same physical result face.
            BRepPrimAPI_MakeBox second_box{
                gp_Pnt{20.0, 0.0, 0.0},
                20.0,
                20.0,
                10.0};
            second_shape =
                second_box.Shape();
            second_face =
                findFaceByCentroid(
                    second_shape,
                    {30.0, 10.0, 10.0});
        } else if (
            scenario ==
            kernel::FaceMergeProbeScenario::
                asymmetric_history) {
            // One argument is a compound:
            //  - a fully internal lower box whose selected top face is
            //    removed from the final boundary;
            //  - an extension box that expands the first box.
            // The result is one 50x20x10 box. The first source top face is
            // therefore Modified while the internal selected face is Deleted.
            BRepPrimAPI_MakeBox internal_box{
                gp_Pnt{10.0, 5.0, 0.0},
                10.0,
                10.0,
                5.0};
            BRepPrimAPI_MakeBox extension_box{
                gp_Pnt{40.0, 0.0, 0.0},
                10.0,
                20.0,
                10.0};

            second_face =
                findFaceByCentroid(
                    internal_box.Shape(),
                    {15.0, 10.0, 5.0});

            BRep_Builder builder;
            TopoDS_Compound compound;
            builder.MakeCompound(compound);
            builder.Add(
                compound,
                internal_box.Shape());
            builder.Add(
                compound,
                extension_box.Shape());
            second_shape = compound;
        } else {
            // Fully internal lower box: outer top role survives unchanged,
            // inner top role disappears.
            BRepPrimAPI_MakeBox second_box{
                gp_Pnt{10.0, 5.0, 0.0},
                10.0,
                10.0,
                5.0};
            second_shape =
                second_box.Shape();
            second_face =
                findFaceByCentroid(
                    second_shape,
                    {15.0, 10.0, 5.0});
        }

        if (second_shape.IsNull() ||
            !second_face) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Fuse fuse{
            first_shape,
            second_shape};
        fuse.Build();
        if (!fuse.IsDone()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        fuse.SimplifyResult(
            true,
            true);

        const TopoDS_Shape result =
            fuse.Shape();
        if (result.IsNull()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            result);

        evidence.first =
            historyEvidence(
                fuse,
                *first_face,
                TopAbs_FACE,
                result);
        evidence.second =
            historyEvidence(
                fuse,
                *second_face,
                TopAbs_FACE,
                result);

        const auto first_descendants =
            historyDescendants(
                fuse,
                *first_face,
                TopAbs_FACE,
                result);
        const auto second_descendants =
            historyDescendants(
                fuse,
                *second_face,
                TopAbs_FACE,
                result);

        evidence.shared_descendant_count =
            sharedShapeCount(
                first_descendants,
                second_descendants);
        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::MultiStageLineageEvidence
buildMultiStageLineageEvidence(
    kernel::MultiStageProbeScenario scenario,
    double extrusion_height) noexcept {
    kernel::MultiStageLineageEvidence evidence;

    if (!std::isfinite(extrusion_height) ||
        !(extrusion_height > 0.0)) {
        evidence.extrude_shape.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const double depth =
            scenario ==
                    kernel::MultiStageProbeScenario::
                        upstream_thin_fillet_failure
                ? 2.0
                : 20.0;

        // Evidence-only "Extrude" stage: a deterministic one-solid prism.
        BRepPrimAPI_MakeBox base{
            gp_Pnt{0.0, 0.0, 0.0},
            40.0,
            depth,
            extrusion_height};

        const TopoDS_Shape extrude_shape =
            base.Shape();
        if (extrude_shape.IsNull()) {
            evidence.extrude_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.extrude_shape,
            extrude_shape);

        const auto selected_face =
            findFaceByCentroid(
                extrude_shape,
                {
                    0.0,
                    depth * 0.5,
                    extrusion_height * 0.5,
                });
        if (!selected_face) {
            evidence.extrude_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        evidence.at_extrude = {
            kernel::EvidenceProducerStage::
                extrude_output,
            kernel::ReferenceStatus::resolved,
            1U};

        // Cut either preserves the selected x=0 side or removes it entirely.
        const bool remove_selected =
            scenario ==
            kernel::MultiStageProbeScenario::
                remove_selected_face;

        const gp_Pnt tool_origin =
            remove_selected
                ? gp_Pnt{-5.0, -5.0, -5.0}
                : gp_Pnt{
                      30.0,
                      depth * 0.25,
                      0.0};

        const double tool_dx =
            remove_selected ? 15.0 : 15.0;
        const double tool_dy =
            remove_selected
                ? depth + 10.0
                : depth * 0.5;
        const double tool_dz =
            remove_selected
                ? extrusion_height + 10.0
                : extrusion_height;

        BRepPrimAPI_MakeBox tool{
            tool_origin,
            tool_dx,
            tool_dy,
            tool_dz};

        BRepAlgoAPI_Cut cut{
            extrude_shape,
            tool.Shape()};
        cut.Build();
        if (!cut.IsDone()) {
            evidence.cut_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const TopoDS_Shape cut_shape =
            cut.Shape();
        if (cut_shape.IsNull()) {
            evidence.cut_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.cut_shape,
            cut_shape);

        evidence.extrude_to_cut =
            historyEvidence(
                cut,
                *selected_face,
                TopAbs_FACE,
                cut_shape);

        const auto cut_descendants =
            historyDescendants(
                cut,
                *selected_face,
                TopAbs_FACE,
                cut_shape);

        evidence.at_cut = {
            kernel::EvidenceProducerStage::
                cut_output,
            referenceStatus(
                cut_descendants.size()),
            cut_descendants.size()};

        if (remove_selected) {
            evidence.downstream_outcome =
                kernel::EvidenceOperationOutcome::
                    not_run;
            return evidence;
        }

        if (cut_descendants.size() != 1U) {
            return evidence;
        }

        const TopoDS_Face cut_face =
            TopoDS::Face(
                cut_descendants.front());

        // The Fillet input is addressed on the immediate Cut output
        // inside the already-resolved semantic Cut-face context. Candidate
        // cardinality is checked there before the operation is allowed; the
        // whole Body is not searched for a geometry-similar edge.
        const auto input_edges =
            findEdgesByEndpoints(
                cut_face,
                gp_Pnt{0.0, 0.0, 0.0},
                gp_Pnt{
                    0.0,
                    0.0,
                    extrusion_height});

        evidence.downstream_input_edge_candidate_count =
            input_edges.size();

        if (input_edges.size() != 1U) {
            return evidence;
        }

        BRepFilletAPI_MakeFillet fillet{
            cut_shape};

        constexpr double fillet_radius = 3.0;
        fillet.Add(
            fillet_radius,
            input_edges.front());

        bool geometric_failure = false;
        try {
            fillet.Build();
            geometric_failure =
                !fillet.IsDone();
        } catch (const Standard_Failure&) {
            geometric_failure = true;
        }

        if (geometric_failure) {
            evidence.downstream_outcome =
                kernel::EvidenceOperationOutcome::
                    geometric_failure;
            return evidence;
        }

        const TopoDS_Shape downstream_shape =
            fillet.Shape();
        if (downstream_shape.IsNull()) {
            evidence.downstream_outcome =
                kernel::EvidenceOperationOutcome::
                    geometric_failure;
            return evidence;
        }

        kernel::ShapeEvidence downstream;
        populateShapeEvidence(
            downstream,
            downstream_shape);
        if (!downstream.ok()) {
            evidence.downstream_outcome =
                kernel::EvidenceOperationOutcome::
                    geometric_failure;
            return evidence;
        }
        evidence.downstream_shape =
            downstream;

        evidence.cut_to_downstream =
            historyEvidence(
                fillet,
                cut_face,
                TopAbs_FACE,
                downstream_shape);

        const auto downstream_descendants =
            historyDescendants(
                fillet,
                cut_face,
                TopAbs_FACE,
                downstream_shape);

        evidence.at_downstream = {
            kernel::EvidenceProducerStage::
                downstream_output,
            referenceStatus(
                downstream_descendants.size()),
            downstream_descendants.size()};

        evidence.downstream_outcome =
            kernel::EvidenceOperationOutcome::
                valid;
        return evidence;
    } catch (const Standard_Failure&) {
        evidence.extrude_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.extrude_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

} // namespace simplesolid2::kernel_occt
