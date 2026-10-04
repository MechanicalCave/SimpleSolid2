#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>

#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepAdaptor_Curve.hxx>
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
#include <GeomAbs_CurveType.hxx>
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
#include <TopTools_IndexedMapOfShape.hxx>
#include <gp_Ax1.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <numbers>
#include <optional>
#include <string>
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
                return circularEdge(
                    circle,
                    start_angle,
                    arcDelta(curve, use),
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

void populateTopologyInventory(
    kernel::EvidenceTopologyKindInventory& evidence,
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    evidence.provider_occurrence_count =
        countSubshapes(
            shape,
            kind);

    TopTools_IndexedMapOfShape unique;
    TopExp::MapShapes(
        shape,
        kind,
        unique);

    evidence.provider_unique_count =
        static_cast<std::size_t>(
            unique.Extent());

    // PM-02P.A proves complete accounting only. Semantic promotion to
    // Referenceable / RepresentationArtifact is intentionally owned by
    // later PM-02P checkpoints rather than guessed here.
    evidence.catalog.resize(
        evidence.provider_unique_count);
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

void appendUniqueFace(
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

[[nodiscard]] std::vector<TopoDS_Face>
uniqueFacesFromShape(
    const TopoDS_Shape& shape) {
    std::vector<TopoDS_Face> result;
    if (shape.IsNull()) {
        return result;
    }

    TopTools_IndexedMapOfShape unique;
    TopExp::MapShapes(
        shape,
        TopAbs_FACE,
        unique);
    result.reserve(
        static_cast<std::size_t>(
            unique.Extent()));
    for (Standard_Integer index = 1;
         index <= unique.Extent();
         ++index) {
        result.push_back(
            TopoDS::Face(
                unique.FindKey(index)));
    }
    return result;
}

[[nodiscard]] std::vector<TopoDS_Face>
uniqueFacesFromGeneratedShape(
    const TopoDS_Shape& shape) {
    std::vector<TopoDS_Face> result;
    for (const auto& face :
         facesFromGeneratedShape(shape)) {
        appendUniqueFace(
            result,
            face);
    }
    return result;
}

[[nodiscard]] kernel::FaceSurfaceKind
semanticSurfaceKind(
    const kernel::BoundaryUse2D& use) noexcept {
    return std::holds_alternative<kernel::Line2>(
               use.curve)
        ? kernel::FaceSurfaceKind::plane
        : kernel::FaceSurfaceKind::cylinder;
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

    const kernel::Point2 direction2{
        line->end.u - line->start.u,
        line->end.v - line->start.v};
    const gp_Vec u_vector =
        vector3(
            input.frame,
            direction2);
    if (!(u_vector.SquareMagnitude() > 0.0)) {
        return std::nullopt;
    }

    const gp_Dir u{u_vector};
    const gp_Dir v{
        input.frame.normal.x,
        input.frame.normal.y,
        input.frame.normal.z};
    const gp_Vec n_vector =
        gp_Vec{u}.Crossed(
            gp_Vec{v});
    if (!(n_vector.SquareMagnitude() > 0.0)) {
        return std::nullopt;
    }
    const gp_Dir n{n_vector};

    const auto origin =
        point3(
            input.frame,
            line->start);

    kernel::Frame3 result;
    result.origin = {
        origin.X(),
        origin.Y(),
        origin.Z()};
    result.u_axis = {
        u.X(),
        u.Y(),
        u.Z()};
    // The canonical carrier V direction is the source Sketch support normal,
    // not the signed Extrude direction. This keeps one planar carrier frame
    // stable when extent direction/length changes.
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

[[nodiscard]] kernel::ReferenceStatus
referenceStatus(
    std::size_t count) noexcept;

[[nodiscard]] kernel::FaceGeometryDiagnostics
faceGeometryDiagnostics(
    const TopoDS_Face& face);

[[nodiscard]] kernel::EvidenceSurfaceCarrierRecord
surfaceCarrierRecord(
    kernel::EvidenceSurfaceCarrierRoleKind role,
    std::optional<kernel::BoundaryUseProvenance> provenance,
    kernel::FaceSurfaceKind semantic_kind,
    const std::vector<TopoDS_Face>& candidates,
    std::optional<kernel::Frame3> canonical_frame) {
    kernel::EvidenceSurfaceCarrierRecord result;
    result.role = role;
    result.provenance =
        std::move(provenance);
    result.status =
        referenceStatus(
            candidates.size());
    result.candidate_face_count =
        candidates.size();
    result.semantic_surface_kind =
        semantic_kind;
    result.canonical_frame =
        std::move(canonical_frame);

    if (candidates.size() == 1U) {
        result.provider_surface_kind =
            faceGeometryDiagnostics(
                candidates.front())
                .surface_kind;
    }
    return result;
}

[[nodiscard]] bool claimFace(
    const std::vector<TopoDS_Face>& provider_faces,
    std::vector<std::size_t>& claim_counts,
    const TopoDS_Face& candidate) {
    for (std::size_t index = 0U;
         index < provider_faces.size();
         ++index) {
        if (provider_faces[index].IsSame(
                candidate)) {
            ++claim_counts[index];
            return true;
        }
    }
    return false;
}


[[nodiscard]] std::vector<TopoDS_Edge>
matchingFaceEdges(
    const TopoDS_Face& face,
    const TopoDS_Edge& source);

[[nodiscard]] const kernel::BoundaryUse2D*
findBoundaryUse(
    const kernel::PlanarProfileInput& input,
    const kernel::BoundaryUseProvenance& provenance) {
    for (const auto& use : input.outer.boundary) {
        if (use.provenance == provenance) {
            return &use;
        }
    }
    for (const auto& loop : input.holes) {
        for (const auto& use : loop.boundary) {
            if (use.provenance == provenance) {
                return &use;
            }
        }
    }
    return nullptr;
}

[[nodiscard]] kernel::BoundaryUse2D
evidenceLineUse(
    kernel::Point2 start,
    kernel::Point2 end,
    std::string source,
    std::uint32_t use_index) {
    return {
        kernel::Line2{start, end},
        0.0,
        1.0,
        true,
        false,
        false,
        kernel::BoundaryUseProvenance{
            std::move(source),
            0U,
            use_index,
            false},
    };
}

[[nodiscard]] kernel::PlanarProfileInput
evidenceRectangleProfile(
    double x0,
    double y0,
    double x1,
    double y1,
    double z,
    const std::string& prefix) {
    kernel::PlanarProfileInput input;
    input.frame.origin = {0.0, 0.0, z};
    input.outer.boundary = {
        evidenceLineUse(
            {x0, y0},
            {x1, y0},
            prefix + "-bottom",
            0U),
        evidenceLineUse(
            {x1, y0},
            {x1, y1},
            prefix + "-right",
            1U),
        evidenceLineUse(
            {x1, y1},
            {x0, y1},
            prefix + "-top",
            2U),
        evidenceLineUse(
            {x0, y1},
            {x0, y0},
            prefix + "-left",
            3U),
    };
    return input;
}

struct EvidencePrismSide final {
    kernel::BoundaryUseProvenance provenance;
    kernel::FaceSurfaceKind semantic_kind{
        kernel::FaceSurfaceKind::other};
    std::optional<kernel::Frame3> canonical_frame;
    TopoDS_Face face;
};

struct EvidencePrismBuild final {
    TopoDS_Shape shape;
    TopoDS_Face start_cap;
    TopoDS_Face end_cap;
    std::optional<kernel::Frame3> start_frame;
    std::optional<kernel::Frame3> end_frame;
    std::vector<EvidencePrismSide> sides;
};

[[nodiscard]] std::optional<EvidencePrismBuild>
buildEvidencePrism(
    const kernel::PlanarProfileInput& input,
    double distance) {
    if (!input.valid() ||
        !std::isfinite(distance) ||
        distance == 0.0) {
        return std::nullopt;
    }

    const auto built =
        buildProfileFace(input);
    if (!built) {
        return std::nullopt;
    }

    BRepSweep_Prism sweep{
        built->face,
        gp_Vec{
            input.frame.normal.x * distance,
            input.frame.normal.y * distance,
            input.frame.normal.z * distance},
        false,
        true};

    const TopoDS_Shape shape =
        sweep.Shape();
    if (shape.IsNull()) {
        return std::nullopt;
    }

    const auto start_faces =
        uniqueFacesFromGeneratedShape(
            sweep.FirstShape());
    const auto end_faces =
        uniqueFacesFromGeneratedShape(
            sweep.LastShape());
    if (start_faces.size() != 1U ||
        end_faces.size() != 1U) {
        return std::nullopt;
    }

    EvidencePrismBuild result;
    result.shape = shape;
    result.start_cap = start_faces.front();
    result.end_cap = end_faces.front();
    result.start_frame =
        shiftedCarrierFrame(
            input.frame,
            0.0);
    result.end_frame =
        shiftedCarrierFrame(
            input.frame,
            distance);

    result.sides.reserve(
        built->source_edges.size());
    for (const auto& source :
         built->source_edges) {
        std::vector<TopoDS_Face> candidates;
        const auto basis_edges =
            matchingFaceEdges(
                built->face,
                source.edge);
        for (const auto& basis_edge :
             basis_edges) {
            const auto generated =
                sweep.Shape(
                    basis_edge);
            for (const auto& face :
                 uniqueFacesFromGeneratedShape(
                     generated)) {
                appendUniqueFace(
                    candidates,
                    face);
            }
        }
        if (candidates.size() != 1U) {
            return std::nullopt;
        }

        const auto* use =
            findBoundaryUse(
                input,
                source.provenance);
        if (use == nullptr) {
            return std::nullopt;
        }

        const auto kind =
            semanticSurfaceKind(*use);
        result.sides.push_back({
            source.provenance,
            kind,
            kind == kernel::FaceSurfaceKind::plane
                ? lineSideCarrierFrame(input, *use)
                : std::nullopt,
            candidates.front(),
        });
    }

    return result;
}

[[nodiscard]] const EvidencePrismSide*
findPrismSide(
    const EvidencePrismBuild& prism,
    const std::string& source_entity) {
    const auto found =
        std::find_if(
            prism.sides.begin(),
            prism.sides.end(),
            [&source_entity](
                const EvidencePrismSide& side) {
                return side.provenance.source_entity ==
                       source_entity;
            });
    return found == prism.sides.end()
        ? nullptr
        : &*found;
}

void populateBodyTopologyEvidence(
    kernel::BodyTopologyInventoryEvidence& topology,
    const kernel::ShapeEvidence& shape_evidence,
    const TopoDS_Shape& shape) {
    topology.status =
        shape_evidence.status;
    topology.brep_valid =
        shape_evidence.brep_valid;
    topology.solid_count =
        shape_evidence.solid_count;
    populateTopologyInventory(
        topology.faces,
        shape,
        TopAbs_FACE);
    populateTopologyInventory(
        topology.edges,
        shape,
        TopAbs_EDGE);
    populateTopologyInventory(
        topology.vertices,
        shape,
        TopAbs_VERTEX);
}

struct EvidencePrismSurfaceClaim final {
    kernel::EvidenceSurfaceCarrierKey key;
    TopoDS_Face face;
};

[[nodiscard]] std::vector<EvidencePrismSurfaceClaim>
prismSurfaceClaims(
    const EvidencePrismBuild& prism) {
    std::vector<EvidencePrismSurfaceClaim> result;
    result.reserve(
        prism.sides.size() + 2U);
    result.push_back({
        {kernel::EvidenceSurfaceCarrierRoleKind::start_cap,
         std::nullopt},
        prism.start_cap});
    result.push_back({
        {kernel::EvidenceSurfaceCarrierRoleKind::end_cap,
         std::nullopt},
        prism.end_cap});
    for (const auto& side : prism.sides) {
        result.push_back({
            {kernel::EvidenceSurfaceCarrierRoleKind::side,
             side.provenance},
            side.face});
    }
    return result;
}

void appendUniqueEdge(
    std::vector<TopoDS_Edge>& edges,
    const TopoDS_Edge& candidate) {
    const bool duplicate =
        std::any_of(
            edges.begin(),
            edges.end(),
            [&candidate](const TopoDS_Edge& existing) {
                return existing.IsSame(candidate);
            });
    if (!duplicate) {
        edges.push_back(candidate);
    }
}

[[nodiscard]] std::vector<TopoDS_Edge>
uniqueEdgesFromShape(
    const TopoDS_Shape& shape) {
    std::vector<TopoDS_Edge> result;
    if (shape.IsNull()) {
        return result;
    }

    TopTools_IndexedMapOfShape unique;
    TopExp::MapShapes(
        shape,
        TopAbs_EDGE,
        unique);
    result.reserve(
        static_cast<std::size_t>(
            unique.Extent()));
    for (Standard_Integer index = 1;
         index <= unique.Extent();
         ++index) {
        result.push_back(
            TopoDS::Edge(
                unique.FindKey(index)));
    }
    return result;
}

[[nodiscard]] std::vector<TopoDS_Edge>
uniqueEdgesFromFace(
    const TopoDS_Face& face) {
    std::vector<TopoDS_Edge> result;
    for (TopExp_Explorer explorer{
             face,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        appendUniqueEdge(
            result,
            TopoDS::Edge(
                explorer.Current()));
    }
    return result;
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

void appendUniqueSurfaceKey(
    std::vector<kernel::EvidenceSurfaceCarrierKey>& keys,
    const kernel::EvidenceSurfaceCarrierKey& candidate) {
    if (std::find(
            keys.begin(),
            keys.end(),
            candidate) == keys.end()) {
        keys.push_back(candidate);
    }
}

[[nodiscard]] kernel::EvidenceCurveKind
providerCurveKind(
    const TopoDS_Edge& edge) {
    BRepAdaptor_Curve curve{
        edge};
    switch (curve.GetType()) {
    case GeomAbs_Line:
        return kernel::EvidenceCurveKind::line;
    case GeomAbs_Circle:
        return kernel::EvidenceCurveKind::circle;
    default:
        return kernel::EvidenceCurveKind::other;
    }
}

[[nodiscard]] kernel::EvidenceCurveKind
semanticCurveKind(
    const kernel::BoundaryUse2D& use) noexcept {
    return std::holds_alternative<kernel::Line2>(
               use.curve)
        ? kernel::EvidenceCurveKind::line
        : kernel::EvidenceCurveKind::circle;
}

[[nodiscard]] std::vector<TopoDS_Edge>
sharedEdges(
    const std::vector<TopoDS_Shape>& first_faces,
    const std::vector<TopoDS_Shape>& second_faces) {
    std::vector<TopoDS_Edge> first_edges;
    std::vector<TopoDS_Edge> second_edges;

    for (const auto& shape : first_faces) {
        if (shape.IsNull() ||
            shape.ShapeType() != TopAbs_FACE) {
            continue;
        }
        for (const auto& edge :
             uniqueEdgesFromFace(
                 TopoDS::Face(shape))) {
            appendUniqueEdge(
                first_edges,
                edge);
        }
    }
    for (const auto& shape : second_faces) {
        if (shape.IsNull() ||
            shape.ShapeType() != TopAbs_FACE) {
            continue;
        }
        for (const auto& edge :
             uniqueEdgesFromFace(
                 TopoDS::Face(shape))) {
            appendUniqueEdge(
                second_edges,
                edge);
        }
    }

    std::vector<TopoDS_Edge> result;
    for (const auto& first : first_edges) {
        const bool present =
            std::any_of(
                second_edges.begin(),
                second_edges.end(),
                [&first](const TopoDS_Edge& second) {
                    return first.IsSame(second);
                });
        if (present) {
            appendUniqueEdge(
                result,
                first);
        }
    }
    return result;
}

[[nodiscard]] bool allEdgesMatchCurveKind(
    const std::vector<TopoDS_Shape>& edges,
    kernel::EvidenceCurveKind expected) {
    if (edges.empty()) {
        return false;
    }
    return std::all_of(
        edges.begin(),
        edges.end(),
        [expected](const TopoDS_Shape& shape) {
            return !shape.IsNull() &&
                   shape.ShapeType() == TopAbs_EDGE &&
                   providerCurveKind(
                       TopoDS::Edge(shape)) ==
                       expected;
        });
}

[[nodiscard]] bool allEdgesMatchCurveKind(
    const std::vector<TopoDS_Edge>& edges,
    kernel::EvidenceCurveKind expected) {
    if (edges.empty()) {
        return false;
    }
    return std::all_of(
        edges.begin(),
        edges.end(),
        [expected](const TopoDS_Edge& edge) {
            return providerCurveKind(edge) ==
                   expected;
        });
}

void appendUniqueVertex(
    std::vector<TopoDS_Vertex>& vertices,
    const TopoDS_Vertex& candidate) {
    const bool duplicate =
        std::any_of(
            vertices.begin(),
            vertices.end(),
            [&candidate](const TopoDS_Vertex& existing) {
                return existing.IsSame(candidate);
            });
    if (!duplicate) {
        vertices.push_back(candidate);
    }
}

[[nodiscard]] std::vector<TopoDS_Vertex>
uniqueVerticesFromShape(
    const TopoDS_Shape& shape) {
    std::vector<TopoDS_Vertex> result;
    if (shape.IsNull()) {
        return result;
    }

    TopTools_IndexedMapOfShape unique;
    TopExp::MapShapes(
        shape,
        TopAbs_VERTEX,
        unique);
    result.reserve(
        static_cast<std::size_t>(
            unique.Extent()));
    for (Standard_Integer index = 1;
         index <= unique.Extent();
         ++index) {
        result.push_back(
            TopoDS::Vertex(
                unique.FindKey(index)));
    }
    return result;
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
    TopoDS_Vertex first;
    TopoDS_Vertex second;
    TopExp::Vertices(
        edge,
        first,
        second);
    return (!first.IsNull() &&
            first.IsSame(vertex)) ||
           (!second.IsNull() &&
            second.IsSame(vertex));
}

[[nodiscard]] kernel::Point3
providerPoint(
    const TopoDS_Vertex& vertex) {
    const auto point =
        BRep_Tool::Pnt(vertex);
    return {
        point.X(),
        point.Y(),
        point.Z()};
}

[[nodiscard]] bool sameSurfaceKeySet(
    const std::vector<kernel::EvidenceSurfaceCarrierKey>& first,
    const std::vector<kernel::EvidenceSurfaceCarrierKey>& second) {
    if (first.size() != second.size()) {
        return false;
    }
    return std::all_of(
        first.begin(),
        first.end(),
        [&second](const kernel::EvidenceSurfaceCarrierKey& key) {
            return std::find(
                       second.begin(),
                       second.end(),
                       key) != second.end();
        });
}

[[nodiscard]] kernel::EvidenceVertexSemanticKey
vertexSemanticKey(
    const EvidencePrismBuild& prism,
    const TopoDS_Vertex& vertex) {
    kernel::EvidenceVertexSemanticKey result;
    for (const auto& surface :
         prismSurfaceClaims(prism)) {
        if (faceContainsVertex(
                surface.face,
                vertex)) {
            appendUniqueSurfaceKey(
                result.adjacent_surfaces,
                surface.key);
        }
    }
    return result;
}

[[nodiscard]] std::vector<TopoDS_Vertex>
resolveVerticesBySemanticKey(
    const EvidencePrismBuild& prism,
    const kernel::EvidenceVertexSemanticKey& key) {
    std::vector<TopoDS_Vertex> result;
    for (const auto& vertex :
         uniqueVerticesFromShape(
             prism.shape)) {
        const auto candidate =
            vertexSemanticKey(
                prism,
                vertex);
        if (sameSurfaceKeySet(
                candidate.adjacent_surfaces,
                key.adjacent_surfaces)) {
            appendUniqueVertex(
                result,
                vertex);
        }
    }
    return result;
}

[[nodiscard]] std::size_t
incidentEdgeCount(
    const TopoDS_Shape& shape,
    const TopoDS_Vertex& vertex) {
    std::size_t count = 0U;
    for (const auto& edge :
         uniqueEdgesFromShape(shape)) {
        if (edgeContainsVertex(
                edge,
                vertex)) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] kernel::EvidenceSurfaceCarrierKey
capKey(
    kernel::EvidenceSurfaceCarrierRoleKind role) {
    return {
        role,
        std::nullopt};
}

[[nodiscard]] kernel::EvidenceSurfaceCarrierKey
sideKey(
    std::string source_entity,
    std::uint32_t use_index) {
    return {
        kernel::EvidenceSurfaceCarrierRoleKind::side,
        kernel::BoundaryUseProvenance{
            std::move(source_entity),
            0U,
            use_index,
            false}};
}

[[nodiscard]] kernel::EvidenceVertexSemanticKey
vertexKey(
    kernel::EvidenceSurfaceCarrierRoleKind cap_role,
    kernel::EvidenceSurfaceCarrierKey first_side,
    kernel::EvidenceSurfaceCarrierKey second_side) {
    kernel::EvidenceVertexSemanticKey result;
    result.adjacent_surfaces = {
        capKey(cap_role),
        std::move(first_side),
        std::move(second_side)};
    return result;
}

[[nodiscard]] kernel::PlanarProfileInput
evidenceChamferedRectangleProfile() {
    kernel::PlanarProfileInput input;
    input.outer.boundary = {
        evidenceLineUse(
            {5.0, 0.0},
            {40.0, 0.0},
            "base-bottom",
            0U),
        evidenceLineUse(
            {40.0, 0.0},
            {40.0, 20.0},
            "base-right",
            1U),
        evidenceLineUse(
            {40.0, 20.0},
            {0.0, 20.0},
            "base-top",
            2U),
        evidenceLineUse(
            {0.0, 20.0},
            {0.0, 5.0},
            "base-left",
            3U),
        evidenceLineUse(
            {0.0, 5.0},
            {5.0, 0.0},
            "base-chamfer",
            4U),
    };
    return input;
}

[[nodiscard]] kernel::ReferenceStatus
surfaceStatusFromSingleCarrierLineage(
    std::size_t descendant_face_count) noexcept {
    return descendant_face_count == 0U
        ? kernel::ReferenceStatus::missing
        : kernel::ReferenceStatus::resolved;
}

[[nodiscard]] std::size_t
countSurfaceKind(
    const std::vector<TopoDS_Shape>& faces,
    kernel::FaceSurfaceKind expected) {
    std::size_t count = 0U;
    for (const auto& shape : faces) {
        if (shape.ShapeType() != TopAbs_FACE) {
            continue;
        }
        if (faceGeometryDiagnostics(
                TopoDS::Face(shape))
                .surface_kind == expected) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] bool nearEvidenceValue(
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

[[nodiscard]] bool nearEvidencePoint(
    const kernel::Point3& first,
    const kernel::Point3& second) noexcept {
    return nearEvidenceValue(first.x, second.x) &&
           nearEvidenceValue(first.y, second.y) &&
           nearEvidenceValue(first.z, second.z);
}

[[nodiscard]] bool sameGeometryDiagnostics(
    const kernel::FaceGeometryDiagnostics& first,
    const kernel::FaceGeometryDiagnostics& second) noexcept {
    return first.surface_kind == second.surface_kind &&
           nearEvidenceValue(first.area, second.area) &&
           nearEvidencePoint(
               first.centroid,
               second.centroid) &&
           nearEvidencePoint(
               first.surface_axis,
               second.surface_axis);
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

kernel::BodyTopologyInventoryEvidence
buildExtrudeTopologyInventoryEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept {
    kernel::BodyTopologyInventoryEvidence evidence;

    if (!input.valid() ||
        !std::isfinite(distance) ||
        distance == 0.0) {
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
            evidence.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const BRepCheck_Analyzer analyzer{
            shape};
        evidence.brep_valid =
            analyzer.IsValid();
        evidence.solid_count =
            countSubshapes(
                shape,
                TopAbs_SOLID);

        populateTopologyInventory(
            evidence.faces,
            shape,
            TopAbs_FACE);
        populateTopologyInventory(
            evidence.edges,
            shape,
            TopAbs_EDGE);
        populateTopologyInventory(
            evidence.vertices,
            shape,
            TopAbs_VERTEX);

        evidence.status =
            evidence.brep_valid
                ? kernel::EvidenceStatus::ok
                : kernel::EvidenceStatus::invalid_brep;
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

kernel::ExtrudeSurfaceCarrierEvidence
buildExtrudeSurfaceCarrierEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept {
    kernel::ExtrudeSurfaceCarrierEvidence evidence;

    if (!input.valid() ||
        !std::isfinite(distance) ||
        distance == 0.0) {
        evidence.shape.status =
            kernel::EvidenceStatus::invalid_input;
        evidence.topology.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const auto built =
            buildProfileFace(input);
        if (!built) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            evidence.topology.status =
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
            evidence.topology.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            shape);
        if (!evidence.shape.ok()) {
            evidence.topology.status =
                evidence.shape.status;
            return evidence;
        }

        evidence.topology.status =
            evidence.shape.status;
        evidence.topology.brep_valid =
            evidence.shape.brep_valid;
        evidence.topology.solid_count =
            evidence.shape.solid_count;
        populateTopologyInventory(
            evidence.topology.faces,
            shape,
            TopAbs_FACE);
        populateTopologyInventory(
            evidence.topology.edges,
            shape,
            TopAbs_EDGE);
        populateTopologyInventory(
            evidence.topology.vertices,
            shape,
            TopAbs_VERTEX);

        const auto provider_faces =
            uniqueFacesFromShape(shape);
        std::vector<std::size_t> claim_counts(
            provider_faces.size(),
            0U);

        const auto start_faces =
            uniqueFacesFromGeneratedShape(
                sweep.FirstShape());
        evidence.start_cap =
            surfaceCarrierRecord(
                kernel::EvidenceSurfaceCarrierRoleKind::
                    start_cap,
                std::nullopt,
                kernel::FaceSurfaceKind::plane,
                start_faces,
                shiftedCarrierFrame(
                    input.frame,
                    0.0));
        for (const auto& face : start_faces) {
            if (!claimFace(
                    provider_faces,
                    claim_counts,
                    face)) {
                ++evidence.claim_outside_body_count;
            }
        }

        const auto end_faces =
            uniqueFacesFromGeneratedShape(
                sweep.LastShape());
        evidence.end_cap =
            surfaceCarrierRecord(
                kernel::EvidenceSurfaceCarrierRoleKind::
                    end_cap,
                std::nullopt,
                kernel::FaceSurfaceKind::plane,
                end_faces,
                shiftedCarrierFrame(
                    input.frame,
                    distance));
        for (const auto& face : end_faces) {
            if (!claimFace(
                    provider_faces,
                    claim_counts,
                    face)) {
                ++evidence.claim_outside_body_count;
            }
        }

        evidence.sides.reserve(
            built->source_edges.size());
        for (std::size_t source_index = 0U;
             source_index <
                 built->source_edges.size();
             ++source_index) {
            const auto& source =
                built->source_edges[source_index];

            std::vector<TopoDS_Face>
                candidate_faces;
            const auto basis_edges =
                matchingFaceEdges(
                    built->face,
                    source.edge);
            for (const auto& basis_edge :
                 basis_edges) {
                const auto generated =
                    sweep.Shape(
                        basis_edge);
                for (const auto& face :
                     uniqueFacesFromGeneratedShape(
                         generated)) {
                    appendUniqueFace(
                        candidate_faces,
                        face);
                }
            }

            // Source edges and resolved Profile boundary uses preserve the
            // same semantic order/provenance inside this evidence adapter.
            const auto* semantic_use =
                [&input, &source]()
                    -> const kernel::BoundaryUse2D* {
                    for (const auto& use :
                         input.outer.boundary) {
                        if (use.provenance ==
                            source.provenance) {
                            return &use;
                        }
                    }
                    for (const auto& loop :
                         input.holes) {
                        for (const auto& use :
                             loop.boundary) {
                            if (use.provenance ==
                                source.provenance) {
                                return &use;
                            }
                        }
                    }
                    return nullptr;
                }();

            const auto semantic_kind =
                semantic_use != nullptr
                    ? semanticSurfaceKind(
                          *semantic_use)
                    : kernel::FaceSurfaceKind::other;
            const auto frame =
                semantic_use != nullptr &&
                        semantic_kind ==
                            kernel::FaceSurfaceKind::plane
                    ? lineSideCarrierFrame(
                          input,
                          *semantic_use)
                    : std::nullopt;

            evidence.sides.push_back(
                surfaceCarrierRecord(
                    kernel::EvidenceSurfaceCarrierRoleKind::
                        side,
                    source.provenance,
                    semantic_kind,
                    candidate_faces,
                    frame));

            for (const auto& face :
                 candidate_faces) {
                if (!claimFace(
                        provider_faces,
                        claim_counts,
                        face)) {
                    ++evidence.claim_outside_body_count;
                }
            }
        }

        for (std::size_t index = 0U;
             index < claim_counts.size();
             ++index) {
            if (claim_counts[index] == 0U) {
                ++evidence.unclaimed_face_count;
                evidence.topology.faces
                    .catalog[index]
                    .accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            integrity_failure;
                continue;
            }
            if (claim_counts[index] > 1U) {
                ++evidence.multiply_claimed_face_count;
                evidence.topology.faces
                    .catalog[index]
                    .accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            integrity_failure;
                continue;
            }

            ++evidence.unique_claimed_face_count;
            evidence.topology.faces
                .catalog[index]
                .accounting_class =
                kernel::
                    EvidenceTopologyAccountingClass::
                        referenceable;
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        evidence.topology.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        evidence.topology.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::SurfaceBooleanLineageEvidence
buildSurfaceBooleanLineageEvidence(
    kernel::SurfaceBooleanProbeScenario scenario) noexcept {
    kernel::SurfaceBooleanLineageEvidence evidence;

    try {
        const auto base_input =
            evidenceRectangleProfile(
                0.0,
                0.0,
                40.0,
                20.0,
                0.0,
                "base");
        const auto base =
            buildEvidencePrism(
                base_input,
                10.0);
        if (!base ||
            !base->end_frame.has_value()) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.before_shape,
            base->shape);
        if (!evidence.before_shape.ok()) {
            return evidence;
        }

        auto publish =
            [&evidence, &base](
                auto& operation) {
                operation.Build();
                if (!operation.IsDone()) {
                    evidence.after_shape.status =
                        kernel::EvidenceStatus::provider_failure;
                    return;
                }

                const TopoDS_Shape result =
                    operation.Shape();
                if (result.IsNull()) {
                    evidence.after_shape.status =
                        kernel::EvidenceStatus::provider_failure;
                    return;
                }

                populateShapeEvidence(
                    evidence.after_shape,
                    result);
                if (!evidence.after_shape.ok()) {
                    return;
                }

                populateBodyTopologyEvidence(
                    evidence.after_topology,
                    evidence.after_shape,
                    result);

                evidence.source_history =
                    historyEvidence(
                        operation,
                        base->end_cap,
                        TopAbs_FACE,
                        result);
                const auto descendants =
                    historyDescendants(
                        operation,
                        base->end_cap,
                        TopAbs_FACE,
                        result);

                evidence.strict_face_status =
                    referenceStatus(
                        descendants.size());
                evidence.surface_status =
                    surfaceStatusFromSingleCarrierLineage(
                        descendants.size());
                evidence.current_face_realization_count =
                    descendants.size();
                evidence.planar_realization_count =
                    countSurfaceKind(
                        descendants,
                        kernel::FaceSurfaceKind::plane);
                if (!descendants.empty()) {
                    evidence.canonical_frame =
                        base->end_frame;
                }
            };

        if (scenario ==
            kernel::SurfaceBooleanProbeScenario::
                attached_add_trim) {
            const auto tool =
                buildEvidencePrism(
                    evidenceRectangleProfile(
                        10.0,
                        5.0,
                        30.0,
                        15.0,
                        10.0,
                        "add"),
                    10.0);
            if (!tool) {
                evidence.after_shape.status =
                    kernel::EvidenceStatus::provider_failure;
                return evidence;
            }

            BRepAlgoAPI_Fuse operation{
                base->shape,
                tool->shape};
            operation.SetFuzzyValue(0.0);
            publish(operation);
            return evidence;
        }

        if (scenario ==
            kernel::SurfaceBooleanProbeScenario::
                cut_trim) {
            const auto tool =
                buildEvidencePrism(
                    evidenceRectangleProfile(
                        30.0,
                        5.0,
                        45.0,
                        15.0,
                        5.0,
                        "cut-trim"),
                    10.0);
            if (!tool) {
                evidence.after_shape.status =
                    kernel::EvidenceStatus::provider_failure;
                return evidence;
            }

            BRepAlgoAPI_Cut operation{
                base->shape,
                tool->shape};
            operation.SetFuzzyValue(0.0);
            publish(operation);
            return evidence;
        }

        const auto tool =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    15.0,
                    -5.0,
                    25.0,
                    25.0,
                    8.0,
                    "cut-split"),
                7.0);
        if (!tool) {
            evidence.after_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Cut operation{
            base->shape,
            tool->shape};
        publish(operation);
        return evidence;
    } catch (const Standard_Failure&) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::SurfaceDeleteRecreateEvidence
buildSurfaceDeleteRecreateEvidence() noexcept {
    kernel::SurfaceDeleteRecreateEvidence evidence;

    try {
        const auto base =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        if (!base ||
            !base->end_frame.has_value()) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.before_shape,
            base->shape);
        if (!evidence.before_shape.ok()) {
            return evidence;
        }
        evidence.old_canonical_frame =
            base->end_frame;

        const auto delete_tool =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    -5.0,
                    -5.0,
                    45.0,
                    25.0,
                    8.0,
                    "delete"),
                7.0);
        if (!delete_tool) {
            evidence.after_delete_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Cut cut{
            base->shape,
            delete_tool->shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        if (!cut.IsDone()) {
            evidence.after_delete_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const TopoDS_Shape after_delete =
            cut.Shape();
        if (after_delete.IsNull()) {
            evidence.after_delete_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.after_delete_shape,
            after_delete);
        if (!evidence.after_delete_shape.ok()) {
            return evidence;
        }

        populateBodyTopologyEvidence(
            evidence.after_delete_topology,
            evidence.after_delete_shape,
            after_delete);

        evidence.delete_history =
            historyEvidence(
                cut,
                base->end_cap,
                TopAbs_FACE,
                after_delete);
        const auto old_descendants =
            historyDescendants(
                cut,
                base->end_cap,
                TopAbs_FACE,
                after_delete);
        evidence.old_surface_after_delete =
            surfaceStatusFromSingleCarrierLineage(
                old_descendants.size());

        const auto replacement =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    8.0,
                    "replacement"),
                2.0);
        if (!replacement ||
            !replacement->end_frame.has_value()) {
            evidence.after_recreate_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Fuse fuse{
            after_delete,
            replacement->shape};
        fuse.SetFuzzyValue(0.0);
        fuse.Build();
        if (!fuse.IsDone()) {
            evidence.after_recreate_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const TopoDS_Shape after_recreate =
            fuse.Shape();
        if (after_recreate.IsNull()) {
            evidence.after_recreate_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.after_recreate_shape,
            after_recreate);
        if (!evidence.after_recreate_shape.ok()) {
            return evidence;
        }

        populateBodyTopologyEvidence(
            evidence.after_recreate_topology,
            evidence.after_recreate_shape,
            after_recreate);

        // The old carrier was semantically deleted in the previous stage and
        // is not an input to this Fuse. Geometry equality cannot recreate a
        // lineage path for the old meaning.
        evidence.old_surface_after_recreate =
            evidence.old_surface_after_delete;

        const auto replacement_faces =
            historyDescendants(
                fuse,
                replacement->end_cap,
                TopAbs_FACE,
                after_recreate);
        evidence.replacement_face_count =
            replacement_faces.size();
        evidence.replacement_surface_status =
            surfaceStatusFromSingleCarrierLineage(
                replacement_faces.size());
        evidence.replacement_canonical_frame =
            replacement->end_frame;

        if (replacement_faces.size() == 1U) {
            evidence.replacement_geometry_matches_old =
                sameGeometryDiagnostics(
                    faceGeometryDiagnostics(
                        base->end_cap),
                    faceGeometryDiagnostics(
                        TopoDS::Face(
                            replacement_faces.front())));
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.after_recreate_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.after_recreate_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::CutExposedSurfaceEvidence
buildCutExposedSurfaceEvidence() noexcept {
    kernel::CutExposedSurfaceEvidence evidence;

    try {
        const auto base =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    30.0,
                    0.0,
                    "base"),
                10.0);
        const auto tool =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    10.0,
                    10.0,
                    30.0,
                    20.0,
                    5.0,
                    "cut-tool"),
                10.0);
        if (!base || !tool) {
            evidence.result_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto* source_side =
            findPrismSide(
                *tool,
                "cut-tool-bottom");
        if (source_side == nullptr ||
            source_side->semantic_kind !=
                kernel::FaceSurfaceKind::plane ||
            !source_side->canonical_frame.has_value()) {
            evidence.result_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Cut cut{
            base->shape,
            tool->shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        if (!cut.IsDone()) {
            evidence.result_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const TopoDS_Shape result =
            cut.Shape();
        if (result.IsNull()) {
            evidence.result_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.result_shape,
            result);
        if (!evidence.result_shape.ok()) {
            return evidence;
        }

        populateBodyTopologyEvidence(
            evidence.result_topology,
            evidence.result_shape,
            result);

        evidence.tool_surface_history =
            historyEvidence(
                cut,
                source_side->face,
                TopAbs_FACE,
                result);
        const auto descendants =
            historyDescendants(
                cut,
                source_side->face,
                TopAbs_FACE,
                result);

        evidence.strict_face_status =
            referenceStatus(
                descendants.size());
        evidence.surface_status =
            surfaceStatusFromSingleCarrierLineage(
                descendants.size());
        evidence.current_face_realization_count =
            descendants.size();
        evidence.semantic_surface_kind =
            source_side->semantic_kind;
        evidence.provenance =
            source_side->provenance;
        evidence.canonical_frame =
            source_side->canonical_frame;

        if (descendants.size() == 1U) {
            evidence.provider_surface_kind =
                faceGeometryDiagnostics(
                    TopoDS::Face(
                        descendants.front()))
                    .surface_kind;
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.result_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.result_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::ExtrudeEdgeOntologyEvidence
buildExtrudeEdgeOntologyEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept {
    kernel::ExtrudeEdgeOntologyEvidence evidence;

    if (!input.valid() ||
        !std::isfinite(distance) ||
        distance == 0.0) {
        evidence.shape.status =
            kernel::EvidenceStatus::invalid_input;
        evidence.topology.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const auto prism =
            buildEvidencePrism(
                input,
                distance);
        if (!prism) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            evidence.topology.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            prism->shape);
        if (!evidence.shape.ok()) {
            return evidence;
        }
        populateBodyTopologyEvidence(
            evidence.topology,
            evidence.shape,
            prism->shape);

        const auto provider_edges =
            uniqueEdgesFromShape(
                prism->shape);
        const auto surfaces =
            prismSurfaceClaims(
                *prism);

        evidence.edges.reserve(
            provider_edges.size());

        for (std::size_t index = 0U;
             index < provider_edges.size();
             ++index) {
            const auto& edge =
                provider_edges[index];

            kernel::EvidenceEdgeOntologyRecord record;
            record.provider_curve_kind =
                providerCurveKind(edge);

            for (const auto& surface : surfaces) {
                if (!faceContainsEdge(
                        surface.face,
                        edge)) {
                    continue;
                }

                appendUniqueSurfaceKey(
                    record.adjacent_surfaces,
                    surface.key);

                if (surface.key.role ==
                        kernel::
                            EvidenceSurfaceCarrierRoleKind::
                                side &&
                    BRepTools::IsReallyClosed(
                        edge,
                        surface.face)) {
                    record.periodic_seam = true;
                }
            }

            if (record.periodic_seam) {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            known_representation_artifact;
                record.status =
                    kernel::ReferenceStatus::unsupported;
                record.role =
                    kernel::
                        EvidenceEdgeSemanticRoleKind::
                            periodic_seam;
                record.semantic_curve_kind =
                    kernel::EvidenceCurveKind::other;
                ++evidence.representation_artifact_count;
                evidence.topology.edges.catalog[index]
                    .accounting_class =
                    record.accounting_class;
                evidence.edges.push_back(
                    std::move(record));
                continue;
            }

            if (record.adjacent_surfaces.size() != 2U) {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            integrity_failure;
                record.status =
                    kernel::ReferenceStatus::unsupported;
                record.role =
                    kernel::
                        EvidenceEdgeSemanticRoleKind::
                            unsupported;
                ++evidence.integrity_failure_count;
                evidence.topology.edges.catalog[index]
                    .accounting_class =
                    record.accounting_class;
                evidence.edges.push_back(
                    std::move(record));
                continue;
            }

            const auto is_cap =
                [](const kernel::EvidenceSurfaceCarrierKey& key) {
                    return key.role ==
                               kernel::
                                   EvidenceSurfaceCarrierRoleKind::
                                       start_cap ||
                           key.role ==
                               kernel::
                                   EvidenceSurfaceCarrierRoleKind::
                                       end_cap;
                };
            const auto is_side =
                [](const kernel::EvidenceSurfaceCarrierKey& key) {
                    return key.role ==
                           kernel::
                               EvidenceSurfaceCarrierRoleKind::
                                   side;
                };

            const bool cap_side =
                (is_cap(record.adjacent_surfaces[0]) &&
                 is_side(record.adjacent_surfaces[1])) ||
                (is_side(record.adjacent_surfaces[0]) &&
                 is_cap(record.adjacent_surfaces[1]));
            const bool side_side =
                is_side(record.adjacent_surfaces[0]) &&
                is_side(record.adjacent_surfaces[1]);

            if (cap_side) {
                record.role =
                    kernel::
                        EvidenceEdgeSemanticRoleKind::
                            cap_side;

                const auto& side_key =
                    is_side(record.adjacent_surfaces[0])
                        ? record.adjacent_surfaces[0]
                        : record.adjacent_surfaces[1];

                const auto* use =
                    side_key.provenance
                        ? findBoundaryUse(
                              input,
                              *side_key.provenance)
                        : nullptr;
                if (use != nullptr) {
                    record.semantic_curve_kind =
                        semanticCurveKind(
                            *use);
                } else {
                    record.semantic_curve_kind =
                        kernel::EvidenceCurveKind::other;
                }
            } else if (side_side) {
                record.role =
                    kernel::
                        EvidenceEdgeSemanticRoleKind::
                            side_side;
                // For a linear Extrude, adjacent side carriers meet along
                // the semantic sweep direction. This is a straight Curve
                // regardless of the individual side Surface classes.
                record.semantic_curve_kind =
                    kernel::EvidenceCurveKind::line;
            } else {
                record.role =
                    kernel::
                        EvidenceEdgeSemanticRoleKind::
                            unsupported;
            }

            const bool semantic_supported =
                record.role !=
                    kernel::
                        EvidenceEdgeSemanticRoleKind::
                            unsupported &&
                record.semantic_curve_kind !=
                    kernel::EvidenceCurveKind::other;

            const bool provider_matches =
                semantic_supported &&
                record.provider_curve_kind.has_value() &&
                *record.provider_curve_kind ==
                    record.semantic_curve_kind;

            if (semantic_supported &&
                provider_matches) {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            referenceable;
                record.status =
                    kernel::ReferenceStatus::resolved;
                ++evidence.referenceable_edge_count;
            } else if (semantic_supported) {
                // A semantic claim that contradicts provider geometry is not
                // downgraded to a guessable Unsupported result.
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            integrity_failure;
                record.status =
                    kernel::ReferenceStatus::unsupported;
                ++evidence.integrity_failure_count;
            } else {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            semantically_unsupported;
                record.status =
                    kernel::ReferenceStatus::unsupported;
                ++evidence.unsupported_edge_count;
            }

            evidence.topology.edges.catalog[index]
                .accounting_class =
                record.accounting_class;
            evidence.edges.push_back(
                std::move(record));
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        evidence.topology.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        evidence.topology.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::EdgeBooleanLineageEvidence
buildEdgeBooleanLineageEvidence(
    kernel::EdgeBooleanProbeScenario scenario) noexcept {
    kernel::EdgeBooleanLineageEvidence evidence;

    try {
        const auto base =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        if (!base) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto* bottom =
            findPrismSide(
                *base,
                "base-bottom");
        if (bottom == nullptr) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto source_edges =
            sharedEdges(
                {TopoDS_Shape{base->end_cap}},
                {TopoDS_Shape{bottom->face}});
        if (source_edges.size() != 1U) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.before_shape,
            base->shape);
        if (!evidence.before_shape.ok()) {
            return evidence;
        }

        double x0{};
        double y0{};
        double x1{};
        double y1{};
        double z{5.0};

        switch (scenario) {
        case kernel::EdgeBooleanProbeScenario::unchanged:
            x0 = 30.0;
            y0 = 10.0;
            x1 = 35.0;
            y1 = 15.0;
            break;
        case kernel::EdgeBooleanProbeScenario::trim:
            x0 = 30.0;
            y0 = -5.0;
            x1 = 45.0;
            y1 = 5.0;
            break;
        case kernel::EdgeBooleanProbeScenario::split:
            x0 = 15.0;
            y0 = -5.0;
            x1 = 25.0;
            y1 = 5.0;
            break;
        case kernel::EdgeBooleanProbeScenario::remove:
            x0 = -5.0;
            y0 = -5.0;
            x1 = 45.0;
            y1 = 5.0;
            break;
        }

        const auto tool =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    x0,
                    y0,
                    x1,
                    y1,
                    z,
                    "edge-cut"),
                10.0);
        if (!tool) {
            evidence.after_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Cut cut{
            base->shape,
            tool->shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        if (!cut.IsDone() ||
            cut.Shape().IsNull()) {
            evidence.after_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto result =
            cut.Shape();
        populateShapeEvidence(
            evidence.after_shape,
            result);
        if (!evidence.after_shape.ok()) {
            return evidence;
        }
        populateBodyTopologyEvidence(
            evidence.after_topology,
            evidence.after_shape,
            result);

        evidence.source_history =
            historyEvidence(
                cut,
                source_edges.front(),
                TopAbs_EDGE,
                result);
        const auto descendants =
            historyDescendants(
                cut,
                source_edges.front(),
                TopAbs_EDGE,
                result);

        evidence.edge_status =
            referenceStatus(
                descendants.size());
        evidence.current_edge_realization_count =
            descendants.size();
        evidence.semantic_curve_kind =
            kernel::EvidenceCurveKind::line;
        evidence.all_provider_curves_match_kind =
            descendants.empty()
                ? scenario ==
                      kernel::EdgeBooleanProbeScenario::remove
                : allEdgesMatchCurveKind(
                      descendants,
                      kernel::EvidenceCurveKind::line);

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::BooleanIntersectionEdgeEvidence
buildBooleanIntersectionEdgeEvidence() noexcept {
    kernel::BooleanIntersectionEdgeEvidence evidence;

    try {
        const auto base =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        const auto tool =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    10.0,
                    5.0,
                    30.0,
                    15.0,
                    5.0,
                    "tool"),
                10.0);
        if (!base || !tool) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto* tool_bottom =
            findPrismSide(
                *tool,
                "tool-bottom");
        if (tool_bottom == nullptr) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Cut cut{
            base->shape,
            tool->shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        if (!cut.IsDone() ||
            cut.Shape().IsNull()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto result =
            cut.Shape();
        populateShapeEvidence(
            evidence.shape,
            result);
        if (!evidence.shape.ok()) {
            return evidence;
        }
        populateBodyTopologyEvidence(
            evidence.topology,
            evidence.shape,
            result);

        const auto first_faces =
            historyDescendants(
                cut,
                base->end_cap,
                TopAbs_FACE,
                result);
        const auto second_faces =
            historyDescendants(
                cut,
                tool_bottom->face,
                TopAbs_FACE,
                result);
        const auto intersections =
            sharedEdges(
                first_faces,
                second_faces);

        evidence.status =
            referenceStatus(
                intersections.size());
        evidence.current_edge_realization_count =
            intersections.size();
        evidence.semantic_curve_kind =
            kernel::EvidenceCurveKind::line;
        evidence.all_provider_curves_match_kind =
            allEdgesMatchCurveKind(
                intersections,
                kernel::EvidenceCurveKind::line);
        evidence.first_surface = {
            kernel::EvidenceSurfaceCarrierRoleKind::end_cap,
            std::nullopt};
        evidence.second_surface = {
            kernel::EvidenceSurfaceCarrierRoleKind::side,
            tool_bottom->provenance};

        evidence.absent_from_both_source_shapes =
            !intersections.empty() &&
            std::all_of(
                intersections.begin(),
                intersections.end(),
                [&base, &tool](const TopoDS_Edge& edge) {
                    return !containsSameSubshape(
                               base->shape,
                               edge,
                               TopAbs_EDGE) &&
                           !containsSameSubshape(
                               tool->shape,
                               edge,
                               TopAbs_EDGE);
                });

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

kernel::SurfacePairBranchEvidence
buildSurfacePairBranchEvidence() noexcept {
    kernel::SurfacePairBranchEvidence evidence;

    try {
        const auto base =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        if (!base) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto* bottom =
            findPrismSide(
                *base,
                "base-bottom");
        if (bottom == nullptr) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto tool =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    15.0,
                    -5.0,
                    25.0,
                    25.0,
                    5.0,
                    "branch-cut"),
                10.0);
        if (!tool) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        BRepAlgoAPI_Cut cut{
            base->shape,
            tool->shape};
        cut.SetFuzzyValue(0.0);
        cut.Build();
        if (!cut.IsDone() ||
            cut.Shape().IsNull()) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        const auto result =
            cut.Shape();
        populateShapeEvidence(
            evidence.shape,
            result);
        if (!evidence.shape.ok()) {
            return evidence;
        }
        populateBodyTopologyEvidence(
            evidence.topology,
            evidence.shape,
            result);

        const auto top_faces =
            historyDescendants(
                cut,
                base->end_cap,
                TopAbs_FACE,
                result);
        const auto front_faces =
            historyDescendants(
                cut,
                bottom->face,
                TopAbs_FACE,
                result);
        const auto branches =
            sharedEdges(
                top_faces,
                front_faces);

        evidence.branch_count =
            branches.size();
        evidence.pair_only_status =
            referenceStatus(
                branches.size());
        evidence.semantic_curve_kind =
            kernel::EvidenceCurveKind::line;
        evidence.all_provider_curves_match_kind =
            allEdgesMatchCurveKind(
                branches,
                kernel::EvidenceCurveKind::line);
        evidence.first_surface = {
            kernel::EvidenceSurfaceCarrierRoleKind::end_cap,
            std::nullopt};
        evidence.second_surface = {
            kernel::EvidenceSurfaceCarrierRoleKind::side,
            bottom->provenance};

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

kernel::ExtrudeVertexOntologyEvidence
buildExtrudeVertexOntologyEvidence(
    const kernel::PlanarProfileInput& input,
    double distance) noexcept {
    kernel::ExtrudeVertexOntologyEvidence evidence;

    if (!input.valid() ||
        !std::isfinite(distance) ||
        distance == 0.0) {
        evidence.shape.status =
            kernel::EvidenceStatus::invalid_input;
        evidence.topology.status =
            kernel::EvidenceStatus::invalid_input;
        return evidence;
    }

    try {
        const auto prism =
            buildEvidencePrism(
                input,
                distance);
        if (!prism) {
            evidence.shape.status =
                kernel::EvidenceStatus::provider_failure;
            evidence.topology.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.shape,
            prism->shape);
        if (!evidence.shape.ok()) {
            return evidence;
        }
        populateBodyTopologyEvidence(
            evidence.topology,
            evidence.shape,
            prism->shape);

        const auto provider_vertices =
            uniqueVerticesFromShape(
                prism->shape);
        evidence.vertices.reserve(
            provider_vertices.size());

        std::vector<kernel::EvidenceVertexSemanticKey>
            keys;
        keys.reserve(
            provider_vertices.size());
        for (const auto& vertex :
             provider_vertices) {
            keys.push_back(
                vertexSemanticKey(
                    *prism,
                    vertex));
        }

        for (std::size_t index = 0U;
             index < provider_vertices.size();
             ++index) {
            kernel::EvidenceVertexOntologyRecord record;
            record.semantic_key =
                keys[index];
            record.incident_material_edge_count =
                incidentEdgeCount(
                    prism->shape,
                    provider_vertices[index]);
            record.provider_point =
                providerPoint(
                    provider_vertices[index]);

            const std::size_t semantic_matches =
                static_cast<std::size_t>(
                    std::count_if(
                        keys.begin(),
                        keys.end(),
                        [&record](
                            const kernel::
                                EvidenceVertexSemanticKey&
                                    candidate) {
                            return sameSurfaceKeySet(
                                candidate.adjacent_surfaces,
                                record.semantic_key
                                    .adjacent_surfaces);
                        }));

            if (record.semantic_key
                        .adjacent_surfaces.size() != 3U ||
                record.incident_material_edge_count != 3U) {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            semantically_unsupported;
                record.status =
                    kernel::ReferenceStatus::unsupported;
                ++evidence.unsupported_vertex_count;
            } else if (semantic_matches == 1U) {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            referenceable;
                record.status =
                    kernel::ReferenceStatus::resolved;
                ++evidence.referenceable_vertex_count;
            } else {
                record.accounting_class =
                    kernel::
                        EvidenceTopologyAccountingClass::
                            integrity_failure;
                record.status =
                    referenceStatus(
                        semantic_matches);
                ++evidence.integrity_failure_count;
            }

            evidence.topology.vertices.catalog[index]
                .accounting_class =
                record.accounting_class;
            evidence.vertices.push_back(
                std::move(record));
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        evidence.topology.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.shape.status =
            kernel::EvidenceStatus::provider_failure;
        evidence.topology.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::VertexDimensionEditEvidence
buildVertexDimensionEditEvidence() noexcept {
    kernel::VertexDimensionEditEvidence evidence;

    try {
        const auto before =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        const auto after =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    55.0,
                    25.0,
                    0.0,
                    "base"),
                10.0);
        if (!before || !after) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            evidence.after_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.before_shape,
            before->shape);
        populateShapeEvidence(
            evidence.after_shape,
            after->shape);
        if (!evidence.before_shape.ok() ||
            !evidence.after_shape.ok()) {
            return evidence;
        }

        populateBodyTopologyEvidence(
            evidence.before_topology,
            evidence.before_shape,
            before->shape);
        populateBodyTopologyEvidence(
            evidence.after_topology,
            evidence.after_shape,
            after->shape);

        evidence.semantic_key =
            vertexKey(
                kernel::
                    EvidenceSurfaceCarrierRoleKind::
                        start_cap,
                sideKey(
                    "base-bottom",
                    0U),
                sideKey(
                    "base-right",
                    1U));

        const auto before_candidates =
            resolveVerticesBySemanticKey(
                *before,
                evidence.semantic_key);
        const auto after_candidates =
            resolveVerticesBySemanticKey(
                *after,
                evidence.semantic_key);

        evidence.before_status =
            referenceStatus(
                before_candidates.size());
        evidence.after_status =
            referenceStatus(
                after_candidates.size());

        if (before_candidates.size() == 1U) {
            evidence.before_point =
                providerPoint(
                    before_candidates.front());
        }
        if (after_candidates.size() == 1U) {
            evidence.after_point =
                providerPoint(
                    after_candidates.front());
        }

        if (evidence.before_point &&
            evidence.after_point) {
            evidence.point_moved =
                !nearEvidencePoint(
                    *evidence.before_point,
                    *evidence.after_point);
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::VertexDeletionGenerationEvidence
buildVertexDeletionGenerationEvidence() noexcept {
    kernel::VertexDeletionGenerationEvidence evidence;

    try {
        const auto before =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        const auto after =
            buildEvidencePrism(
                evidenceChamferedRectangleProfile(),
                10.0);
        if (!before || !after) {
            evidence.before_shape.status =
                kernel::EvidenceStatus::provider_failure;
            evidence.after_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.before_shape,
            before->shape);
        populateShapeEvidence(
            evidence.after_shape,
            after->shape);
        if (!evidence.before_shape.ok() ||
            !evidence.after_shape.ok()) {
            return evidence;
        }

        populateBodyTopologyEvidence(
            evidence.before_topology,
            evidence.before_shape,
            before->shape);
        populateBodyTopologyEvidence(
            evidence.after_topology,
            evidence.after_shape,
            after->shape);

        evidence.deleted_key =
            vertexKey(
                kernel::
                    EvidenceSurfaceCarrierRoleKind::
                        start_cap,
                sideKey(
                    "base-bottom",
                    0U),
                sideKey(
                    "base-left",
                    3U));

        const auto before_deleted =
            resolveVerticesBySemanticKey(
                *before,
                evidence.deleted_key);
        const auto after_deleted =
            resolveVerticesBySemanticKey(
                *after,
                evidence.deleted_key);

        evidence.before_deleted_status =
            referenceStatus(
                before_deleted.size());
        evidence.after_deleted_status =
            referenceStatus(
                after_deleted.size());

        evidence.generated_keys = {
            vertexKey(
                kernel::
                    EvidenceSurfaceCarrierRoleKind::
                        start_cap,
                sideKey(
                    "base-left",
                    3U),
                sideKey(
                    "base-chamfer",
                    4U)),
            vertexKey(
                kernel::
                    EvidenceSurfaceCarrierRoleKind::
                        start_cap,
                sideKey(
                    "base-chamfer",
                    4U),
                sideKey(
                    "base-bottom",
                    0U)),
        };

        evidence.generated_statuses.reserve(
            evidence.generated_keys.size());
        for (const auto& key :
             evidence.generated_keys) {
            const auto candidates =
                resolveVerticesBySemanticKey(
                    *after,
                    key);
            evidence.generated_statuses.push_back(
                referenceStatus(
                    candidates.size()));
            if (candidates.size() == 1U) {
                ++evidence.generated_vertex_count;
            }
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.after_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }
}

kernel::VertexSamePointReplacementEvidence
buildVertexSamePointReplacementEvidence() noexcept {
    kernel::VertexSamePointReplacementEvidence evidence;

    try {
        const auto old_prism =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "base"),
                10.0);
        const auto replacement =
            buildEvidencePrism(
                evidenceRectangleProfile(
                    0.0,
                    0.0,
                    40.0,
                    20.0,
                    0.0,
                    "replacement"),
                10.0);
        if (!old_prism || !replacement) {
            evidence.old_shape.status =
                kernel::EvidenceStatus::provider_failure;
            evidence.replacement_shape.status =
                kernel::EvidenceStatus::provider_failure;
            return evidence;
        }

        populateShapeEvidence(
            evidence.old_shape,
            old_prism->shape);
        populateShapeEvidence(
            evidence.replacement_shape,
            replacement->shape);
        if (!evidence.old_shape.ok() ||
            !evidence.replacement_shape.ok()) {
            return evidence;
        }

        populateBodyTopologyEvidence(
            evidence.old_topology,
            evidence.old_shape,
            old_prism->shape);
        populateBodyTopologyEvidence(
            evidence.replacement_topology,
            evidence.replacement_shape,
            replacement->shape);

        evidence.old_key =
            vertexKey(
                kernel::
                    EvidenceSurfaceCarrierRoleKind::
                        start_cap,
                sideKey(
                    "base-bottom",
                    0U),
                sideKey(
                    "base-left",
                    3U));
        evidence.replacement_key =
            vertexKey(
                kernel::
                    EvidenceSurfaceCarrierRoleKind::
                        start_cap,
                sideKey(
                    "replacement-bottom",
                    0U),
                sideKey(
                    "replacement-left",
                    3U));

        const auto old_candidates =
            resolveVerticesBySemanticKey(
                *old_prism,
                evidence.old_key);
        const auto old_in_replacement =
            resolveVerticesBySemanticKey(
                *replacement,
                evidence.old_key);
        const auto replacement_candidates =
            resolveVerticesBySemanticKey(
                *replacement,
                evidence.replacement_key);

        evidence.old_key_in_old_shape =
            referenceStatus(
                old_candidates.size());
        evidence.old_key_in_replacement_shape =
            referenceStatus(
                old_in_replacement.size());
        evidence.replacement_key_status =
            referenceStatus(
                replacement_candidates.size());

        if (old_candidates.size() == 1U) {
            evidence.old_point =
                providerPoint(
                    old_candidates.front());
        }
        if (replacement_candidates.size() == 1U) {
            evidence.replacement_point =
                providerPoint(
                    replacement_candidates.front());
        }

        if (evidence.old_point &&
            evidence.replacement_point) {
            evidence.provider_points_equal =
                nearEvidencePoint(
                    *evidence.old_point,
                    *evidence.replacement_point);
        }

        return evidence;
    } catch (const Standard_Failure&) {
        evidence.replacement_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    } catch (...) {
        evidence.replacement_shape.status =
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
        populateBodyTopologyEvidence(
            evidence.topology,
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
