#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <BRep_Tool.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <Poly_Triangle.hxx>
#include <Poly_Triangulation.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_Orientation.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Solid.hxx>
#include <TopoDS_Wire.hxx>
#include <TopLoc_Location.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>
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

struct SourceEdge final {
    TopoDS_Edge edge;
    kernel::BoundaryUseProvenance provenance;
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
        result.source_edges.push_back(
            {make_wire.Edge(),
             use.provenance});
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
    std::map<std::uint64_t, TopoDS_Face>
        tracked_faces;
    std::uint64_t next_token{1U};
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
        runtime.next_token =
            upstream->next_token;
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

        if (runtime.next_token == 0U) {
            published.status =
                kernel::ReferenceStatus::
                    unsupported;
            continue;
        }
        const kernel::RuntimeFaceToken token{
            runtime.next_token};
        ++runtime.next_token;
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

    std::vector<NewSemanticSource> sources;
    sources.push_back(
        {
            kernel::ExtrudeFaceRole{
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.start_cap_role,
                std::nullopt},
            facesFromShape(
                sweep.FirstShape()),
        });
    sources.push_back(
        {
            kernel::ExtrudeFaceRole{
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.end_cap_role,
                std::nullopt},
            facesFromShape(
                sweep.LastShape()),
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
        sources.push_back(
            {
                kernel::ExtrudeFaceRole{
                    kernel::ExtrudeGeneratedFaceRoleKind::side,
                    std::nullopt,
                    source.provenance},
                std::move(faces),
            });
    }

    return std::make_pair(
        shape,
        std::move(sources));
}

void populateDiagnostics(
    kernel::SolidModelingResult& result,
    const TopoDS_Shape& shape) {
    result.solid_count =
        countSubshapes(
            shape,
            TopAbs_SOLID);
    result.face_count =
        countSubshapes(
            shape,
            TopAbs_FACE);
    result.edge_count =
        countSubshapes(
            shape,
            TopAbs_EDGE);
    result.brep_valid =
        !shape.IsNull() &&
        BRepCheck_Analyzer{shape}.IsValid();
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

    result.status =
        kernel::SolidModelingStatus::ok;
    result.solid = std::move(runtime);
    return result;
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

    try {
        // Presentation-only policy. These values are intentionally private to
        // the provider and never participate in modeling semantics.
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

        for (TopExp_Explorer explorer{
                 runtime->solid,
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

                if (face.Orientation() ==
                    TopAbs_REVERSED) {
                    std::swap(
                        second,
                        third);
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

                const gp_Vec normal =
                    cross / magnitude;
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
                        {normal.X(),
                         normal.Y(),
                         normal.Z()}});
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

} // namespace simplesolid2::kernel_occt
