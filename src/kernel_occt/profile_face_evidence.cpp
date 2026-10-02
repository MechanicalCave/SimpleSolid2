#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepGProp.hxx>
#include <BRepSweep_Prism.hxx>
#include <GProp_GProps.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

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

[[nodiscard]] std::optional<TopoDS_Edge>
makeEvidenceLineEdge(
    double x0,
    double y0,
    double x1,
    double y1) {
    BRepBuilderAPI_MakeEdge make_edge{
        gp_Pnt{x0, y0, 0.0},
        gp_Pnt{x1, y1, 0.0}};
    if (!make_edge.IsDone()) {
        return std::nullopt;
    }
    return make_edge.Edge();
}

[[nodiscard]] bool validEvidenceEdge(
    const TopoDS_Edge& edge) {
    return BRepCheck_Analyzer{edge}.IsValid();
}

[[nodiscard]] double edgeLength(
    const TopoDS_Edge& edge) {
    GProp_GProps properties;
    BRepGProp::LinearProperties(
        edge,
        properties);
    return properties.Mass();
}

[[nodiscard]] bool nearMeasure(
    double first,
    double second) noexcept {
    const double scale =
        std::max({
            1.0,
            std::abs(first),
            std::abs(second)});
    return std::abs(first - second) <=
           1.0e-9 * scale;
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


kernel::CardinalityFixtureEvidence
buildSplitEdgeCardinalityFixture(
    const kernel::BoundaryUseProvenance& target,
    SplitCardinalityFixture fixture) noexcept {
    kernel::CardinalityFixtureEvidence result;
    result.source_edge_count = 1U;

    try {
        const auto source =
            makeEvidenceLineEdge(
                0.0,
                0.0,
                100.0,
                0.0);
        if (!source ||
            !validEvidenceEdge(*source)) {
            return result;
        }

        const double source_length =
            edgeLength(*source);

        switch (fixture) {
        case SplitCardinalityFixture::
            two_semantic_descendants: {
            const auto first =
                makeEvidenceLineEdge(
                    0.0,
                    0.0,
                    50.0,
                    0.0);
            const auto second =
                makeEvidenceLineEdge(
                    50.0,
                    0.0,
                    100.0,
                    0.0);
            if (!first || !second ||
                !validEvidenceEdge(*first) ||
                !validEvidenceEdge(*second)) {
                return result;
            }

            result.physical_candidate_count =
                2U;
            result.provider_geometry_valid =
                nearMeasure(
                    edgeLength(*first) +
                        edgeLength(*second),
                    source_length);
            result.candidates = {
                {
                    {target},
                    true,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
                {
                    {target},
                    true,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            result.source_history = {
                {
                    target,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            break;
        }

        case SplitCardinalityFixture::
            semantic_plus_technical: {
            const auto intended =
                makeEvidenceLineEdge(
                    0.0,
                    0.0,
                    100.0,
                    0.0);
            const auto technical =
                makeEvidenceLineEdge(
                    50.0,
                    0.0,
                    50.0,
                    10.0);
            if (!intended || !technical ||
                !validEvidenceEdge(*intended) ||
                !validEvidenceEdge(*technical)) {
                return result;
            }

            result.physical_candidate_count =
                2U;
            result.provider_geometry_valid =
                nearMeasure(
                    edgeLength(*intended),
                    source_length) &&
                edgeLength(*technical) > 0.0;
            result.candidates = {
                {
                    {target},
                    true,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
                {
                    {target},
                    false,
                    kernel::
                        ProviderLineageObservation::
                            generated,
                },
            };
            result.source_history = {
                {
                    target,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            break;
        }

        case SplitCardinalityFixture::
            deleted_target:
            result.physical_candidate_count =
                0U;
            result.provider_geometry_valid =
                true;
            result.source_history = {
                {
                    target,
                    kernel::
                        ProviderLineageObservation::
                            deleted,
                },
            };
            break;
        }

        return result;
    } catch (const Standard_Failure&) {
        return result;
    } catch (...) {
        return result;
    }
}

kernel::CardinalityFixtureEvidence
buildMergeEdgeCardinalityFixture(
    const kernel::BoundaryUseProvenance& first,
    const kernel::BoundaryUseProvenance& second,
    MergeCardinalityFixture fixture) noexcept {
    kernel::CardinalityFixtureEvidence result;
    result.source_edge_count = 2U;

    try {
        const auto first_source =
            makeEvidenceLineEdge(
                0.0,
                0.0,
                50.0,
                0.0);
        const auto second_source =
            makeEvidenceLineEdge(
                50.0,
                0.0,
                100.0,
                0.0);
        const auto merged =
            makeEvidenceLineEdge(
                0.0,
                0.0,
                100.0,
                0.0);

        if (!first_source ||
            !second_source ||
            !merged ||
            !validEvidenceEdge(*first_source) ||
            !validEvidenceEdge(*second_source) ||
            !validEvidenceEdge(*merged)) {
            return result;
        }

        result.physical_candidate_count = 1U;
        result.provider_geometry_valid =
            nearMeasure(
                edgeLength(*first_source) +
                    edgeLength(*second_source),
                edgeLength(*merged));

        switch (fixture) {
        case MergeCardinalityFixture::
            lost_distinction:
            result.candidates = {
                {
                    {first, second},
                    true,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            result.source_history = {
                {
                    first,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
                {
                    second,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            break;

        case MergeCardinalityFixture::
            modified_deleted_same_output:
            result.candidates = {
                {
                    {first, second},
                    true,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            result.source_history = {
                {
                    first,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
                {
                    second,
                    kernel::
                        ProviderLineageObservation::
                            deleted,
                },
            };
            break;

        case MergeCardinalityFixture::
            preserve_first_remove_second:
            result.candidates = {
                {
                    {first},
                    true,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
            };
            result.source_history = {
                {
                    first,
                    kernel::
                        ProviderLineageObservation::
                            modified,
                },
                {
                    second,
                    kernel::
                        ProviderLineageObservation::
                            deleted,
                },
            };
            break;
        }

        return result;
    } catch (const Standard_Failure&) {
        return result;
    } catch (...) {
        return result;
    }
}

} // namespace simplesolid2::kernel_occt
