#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRepGProp.hxx>
#include <BRepLib_ToolTriangulatedShape.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
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
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
#include <GProp_GProps.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepSweep_Prism.hxx>

#include <algorithm>
#include <cmath>
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
    const TopoDS_Shape& result) {
    std::vector<TopoDS_Face> descendants;

    const auto& modified =
        operation.Modified(source);
    for (const auto& item : modified) {
        appendUniqueFace(
            descendants,
            item,
            result);
    }

    const auto& generated =
        operation.Generated(source);
    for (const auto& item : generated) {
        appendUniqueFace(
            descendants,
            item,
            result);
    }

    if (descendants.empty() &&
        !operation.IsDeleted(source) &&
        containsSameFace(result, source)) {
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
        kernel::ExtrudeFaceRole role;
        kernel::SurfaceKind kind{
            kernel::SurfaceKind::other};
        std::optional<kernel::Frame3>
            canonical_frame;
        std::vector<TopoDS_Face> faces;
        // Stage-relative only: true when this Surface was introduced by the
        // operation producing this RuntimeSolid. It is reset to false when
        // the Surface is inherited into the next stage.
        bool produced_by_current_operation{false};
    };

    struct TrackedEdge final {
        TopoDS_Edge edge;
        kernel::EdgeSemanticRoleKind role{
            kernel::EdgeSemanticRoleKind::unsupported};
        kernel::CurveKind curve_kind{
            kernel::CurveKind::other};
        std::vector<kernel::RuntimeSurfaceToken>
            adjacent_surfaces;
    };

    std::map<std::uint64_t, TrackedSurface>
        tracked_surfaces;
    std::vector<TrackedEdge> tracked_edges;

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
    kernel::ExtrudeFaceRole role;
    kernel::SurfaceKind surface_kind{
        kernel::SurfaceKind::other};
    std::optional<kernel::Frame3>
        canonical_frame;
    bool aliased{false};
};

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
                    tracked.role,
                    tracked.kind,
                    tracked.canonical_frame,
                    false,
                });
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
                source.role,
                source.surface_kind,
                source.canonical_frame,
                false,
            });
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
                                claim.role,
                                claim.surface_kind,
                                claim.canonical_frame,
                                claim.candidates,
                                false})
                         .second) {
                    return false;
                }
            }
            continue;
        }

        auto& published =
            result.new_surfaces[
                claim.index];
        published.surface_status =
            surface_status;
        published.strict_face_status =
            strict_face_status;
        published.candidate_face_count =
            claim.candidates.size();
        published.current_faces =
            std::move(current_faces);
        published.canonical_frame.reset();

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
                    claim.role,
                    claim.surface_kind,
                    claim.canonical_frame,
                    claim.candidates,
                    true})
                 .second) {
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

    result.status =
        kernel::SolidModelingStatus::ok;
    result.solid = std::move(runtime);
    return result;
}

[[nodiscard]] kernel::SolidPresentationResult
presentationMeshForShape(
    const TopoDS_Shape& shape) noexcept {
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
                const auto face =
                    TopoDS::Face(
                        explorer.Current());
                TopLoc_Location location;
                const Handle(Poly_Triangulation)
                    triangulation =
                        BRep_Tool::Triangulation(
                            face,
                            location);
                if (triangulation.IsNull()) {
                    continue;
                }
                if (!triangulation->HasNormals()) {
                    BRepLib_ToolTriangulatedShape::
                        ComputeNormals(
                            face,
                            triangulation);
                }
                if (!triangulation->HasNormals()) {
                    result.status =
                        kernel::SolidPresentationStatus::
                            provider_failure;
                    result.mesh.triangles.clear();
                    return result;
                }

                const auto transform =
                    location.Transformation();
                for (Standard_Integer index = 1;
                     index <=
                         triangulation->NbTriangles();
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

                gp_Dir first_normal =
                    triangulation->Normal(
                        first_index);
                gp_Dir second_normal =
                    triangulation->Normal(
                        second_index);
                gp_Dir third_normal =
                    triangulation->Normal(
                        third_index);
                first_normal.Transform(transform);
                second_normal.Transform(transform);
                third_normal.Transform(transform);

                if (face.Orientation() ==
                    TopAbs_REVERSED) {
                    std::swap(
                        second,
                        third);
                    std::swap(
                        second_normal,
                        third_normal);
                    first_normal.Reverse();
                    second_normal.Reverse();
                    third_normal.Reverse();
                }

                const gp_Vec first_edge{
                    first,
                    second};
                const gp_Vec second_edge{
                    first,
                    third};
                const gp_Vec cross =
                    first_edge.Crossed(
                        second_edge);
                const double magnitude =
                    cross.Magnitude();
                if (!std::isfinite(magnitude) ||
                    !(magnitude > 0.0)) {
                    continue;
                }

                    result.mesh.triangles.push_back(
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

} // namespace simplesolid2::kernel_occt
