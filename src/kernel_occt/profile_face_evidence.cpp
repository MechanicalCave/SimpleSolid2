#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepSweep_Prism.hxx>
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
        result.source_edges.push_back({
            *edge,
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
            std::size_t face_count = 0U;
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
                    face_count +=
                        countSubshapes(
                            generated,
                            TopAbs_FACE);
                }
            }

            if (face_count != 1U) {
                std::cerr
                    << "E01_PROVIDER_DIAG source="
                    << source.provenance.source_entity
                    << " hole="
                    << source.provenance.hole
                    << " basis_matches="
                    << basis_edges.size()
                    << " generated_shapes="
                    << generated_shape_count;

                for (std::size_t index = 0U;
                     index < basis_edges.size();
                     ++index) {
                    const auto& basis_edge =
                        basis_edges[index];
                    const TopoDS_Shape generated =
                        sweep.Shape(basis_edge);
                    const TopoDS_Shape first =
                        sweep.FirstShape(basis_edge);
                    const TopoDS_Shape last =
                        sweep.LastShape(basis_edge);

                    std::cerr
                        << " edge[" << index << "]"
                        << " is_used="
                        << sweep.IsUsed(basis_edge)
                        << " gen_is_used="
                        << sweep.GenIsUsed(basis_edge)
                        << " generated_null="
                        << generated.IsNull()
                        << " generated_type="
                        << (generated.IsNull()
                                ? -1
                                : static_cast<int>(
                                      generated.ShapeType()))
                        << " generated_faces="
                        << (generated.IsNull()
                                ? 0U
                                : countSubshapes(
                                      generated,
                                      TopAbs_FACE))
                        << " first_null="
                        << first.IsNull()
                        << " first_type="
                        << (first.IsNull()
                                ? -1
                                : static_cast<int>(
                                      first.ShapeType()))
                        << " last_null="
                        << last.IsNull()
                        << " last_type="
                        << (last.IsNull()
                                ? -1
                                : static_cast<int>(
                                      last.ShapeType()));
                }

                std::cerr << '\n';
            }

            evidence.sides.push_back({
                kernel::ExtrudeFaceRoleKind::side,
                referenceStatus(face_count),
                face_count,
                source.provenance,
                basis_edges.size(),
                generated_shape_count});
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

} // namespace simplesolid2::kernel_occt
