#include <simplesolid2/kernel_occt/edge_feature_evidence.hpp>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepFilletAPI_MakeChamfer.hxx>
#include <BRepFilletAPI_MakeFillet.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Standard_Failure.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Vec.hxx>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

namespace simplesolid2::kernel_occt {
namespace {

constexpr double box_x = 40.0;
constexpr double box_y = 30.0;
constexpr double box_z = 20.0;
constexpr double normal_parameter = 2.0;
constexpr double excessive_parameter = 1000.0;
constexpr double point_tolerance = 1.0e-7;
constexpr double volume_tolerance = 1.0e-9;

struct EdgeEndpoints final {
    gp_Pnt first;
    gp_Pnt second;
};

struct OperationRun final {
    std::size_t contour_count{};
    std::size_t contour_edge_count{};
    bool exact_membership{false};
    bool build_attempted{false};
    bool build_succeeded{false};
    std::optional<kernel::ShapeEvidence> shape;
    std::optional<kernel::EdgeFeatureTopologySignature> signature;
    std::size_t new_face_count{};
    std::size_t edge_generated_face_count{};
    std::size_t shared_vertex_generated_face_count{};
    std::size_t unclaimed_new_face_count{};
    std::vector<std::size_t> generated_faces_per_edge;
};

[[nodiscard]] std::size_t uniqueCount(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    TopTools_IndexedMapOfShape map;
    TopExp::MapShapes(shape, kind, map);
    return static_cast<std::size_t>(map.Extent());
}

[[nodiscard]] kernel::ShapeEvidence
shapeEvidence(const TopoDS_Shape& shape) {
    kernel::ShapeEvidence result;
    if (shape.IsNull()) {
        return result;
    }

    result.solid_count =
        uniqueCount(shape, TopAbs_SOLID);
    result.face_count =
        uniqueCount(shape, TopAbs_FACE);
    result.wire_count =
        uniqueCount(shape, TopAbs_WIRE);
    result.edge_count =
        uniqueCount(shape, TopAbs_EDGE);

    const BRepCheck_Analyzer analyzer{shape};
    result.brep_valid = analyzer.IsValid();
    result.status =
        result.brep_valid
            ? kernel::EvidenceStatus::ok
            : kernel::EvidenceStatus::invalid_brep;
    return result;
}

[[nodiscard]] double shapeVolume(
    const TopoDS_Shape& shape) {
    GProp_GProps props;
    BRepGProp::VolumeProperties(shape, props);
    return props.Mass();
}

[[nodiscard]] kernel::EdgeFeatureTopologySignature
topologySignature(const TopoDS_Shape& shape) {
    return {
        uniqueCount(shape, TopAbs_FACE),
        uniqueCount(shape, TopAbs_EDGE),
        uniqueCount(shape, TopAbs_VERTEX),
        shapeVolume(shape),
    };
}

[[nodiscard]] bool nearPoint(
    const gp_Pnt& first,
    const gp_Pnt& second) noexcept {
    return first.Distance(second) <= point_tolerance;
}

[[nodiscard]] std::optional<TopoDS_Edge>
findEdgeByEndpoints(
    const TopoDS_Shape& shape,
    const EdgeEndpoints& endpoints) {
    for (TopExp_Explorer explorer{shape, TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto edge =
            TopoDS::Edge(explorer.Current());
        TopoDS_Vertex first_vertex;
        TopoDS_Vertex second_vertex;
        TopExp::Vertices(
            edge,
            first_vertex,
            second_vertex);
        if (first_vertex.IsNull() ||
            second_vertex.IsNull()) {
            continue;
        }

        const auto first =
            BRep_Tool::Pnt(first_vertex);
        const auto second =
            BRep_Tool::Pnt(second_vertex);

        if ((nearPoint(first, endpoints.first) &&
             nearPoint(second, endpoints.second)) ||
            (nearPoint(first, endpoints.second) &&
             nearPoint(second, endpoints.first))) {
            return edge;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool sameShape(
    const TopoDS_Shape& first,
    const TopoDS_Shape& second) {
    return first.IsSame(second);
}

template <typename Shape>
void appendUnique(
    std::vector<Shape>& values,
    const Shape& value) {
    const auto found =
        std::find_if(
            values.begin(),
            values.end(),
            [&value](const Shape& existing) {
                return sameShape(existing, value);
            });
    if (found == values.end()) {
        values.push_back(value);
    }
}

[[nodiscard]] std::vector<TopoDS_Face>
facesFromList(
    const TopTools_ListOfShape& shapes) {
    std::vector<TopoDS_Face> faces;
    for (TopTools_ListOfShape::Iterator it{shapes};
         it.More();
         it.Next()) {
        const auto& shape = it.Value();
        if (shape.ShapeType() == TopAbs_FACE) {
            appendUnique(
                faces,
                TopoDS::Face(shape));
            continue;
        }

        for (TopExp_Explorer explorer{
                 shape,
                 TopAbs_FACE};
             explorer.More();
             explorer.Next()) {
            appendUnique(
                faces,
                TopoDS::Face(
                    explorer.Current()));
        }
    }
    return faces;
}

[[nodiscard]] std::vector<TopoDS_Face>
newFaces(
    const TopoDS_Shape& source,
    const TopoDS_Shape& result) {
    std::vector<TopoDS_Face> source_faces;
    for (TopExp_Explorer explorer{source, TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        appendUnique(
            source_faces,
            TopoDS::Face(explorer.Current()));
    }

    std::vector<TopoDS_Face> result_faces;
    for (TopExp_Explorer explorer{result, TopAbs_FACE};
         explorer.More();
         explorer.Next()) {
        const auto face =
            TopoDS::Face(explorer.Current());
        const bool inherited =
            std::any_of(
                source_faces.begin(),
                source_faces.end(),
                [&face](const TopoDS_Face& source_face) {
                    return source_face.IsSame(face);
                });
        if (!inherited) {
            appendUnique(result_faces, face);
        }
    }
    return result_faces;
}

struct VertexUse final {
    TopoDS_Vertex vertex;
    std::size_t count{};
};

[[nodiscard]] std::vector<TopoDS_Vertex>
sharedVertices(
    const std::vector<TopoDS_Edge>& edges) {
    std::vector<VertexUse> uses;

    for (const auto& edge : edges) {
        TopoDS_Vertex first;
        TopoDS_Vertex second;
        TopExp::Vertices(edge, first, second);
        for (const auto& vertex : {first, second}) {
            if (vertex.IsNull()) {
                continue;
            }
            const auto found =
                std::find_if(
                    uses.begin(),
                    uses.end(),
                    [&vertex](const VertexUse& use) {
                        return use.vertex.IsSame(vertex);
                    });
            if (found == uses.end()) {
                uses.push_back(
                    VertexUse{vertex, 1U});
            } else {
                ++found->count;
            }
        }
    }

    std::vector<TopoDS_Vertex> result;
    for (const auto& use : uses) {
        if (use.count >= 2U) {
            result.push_back(use.vertex);
        }
    }
    return result;
}

[[nodiscard]] bool containsSameEdge(
    const std::vector<TopoDS_Edge>& edges,
    const TopoDS_Edge& candidate) {
    return std::any_of(
        edges.begin(),
        edges.end(),
        [&candidate](const TopoDS_Edge& edge) {
            return edge.IsSame(candidate);
        });
}

template <typename Operation>
void captureContourMembership(
    Operation& operation,
    const std::vector<TopoDS_Edge>& selected,
    OperationRun& run) {
    run.contour_count =
        static_cast<std::size_t>(
            operation.NbContours());

    std::vector<TopoDS_Edge> contour_edges;
    for (Standard_Integer contour = 1;
         contour <= operation.NbContours();
         ++contour) {
        const auto edge_count =
            operation.NbEdges(contour);
        run.contour_edge_count +=
            static_cast<std::size_t>(
                edge_count);
        for (Standard_Integer index = 1;
             index <= edge_count;
             ++index) {
            appendUnique(
                contour_edges,
                operation.Edge(
                    contour,
                    index));
        }
    }

    const bool no_extra =
        std::all_of(
            contour_edges.begin(),
            contour_edges.end(),
            [&selected](const TopoDS_Edge& edge) {
                return containsSameEdge(
                    selected,
                    edge);
            });
    const bool no_missing =
        std::all_of(
            selected.begin(),
            selected.end(),
            [&contour_edges](const TopoDS_Edge& edge) {
                return containsSameEdge(
                    contour_edges,
                    edge);
            });

    run.exact_membership =
        no_extra &&
        no_missing &&
        contour_edges.size() ==
            selected.size();
}

template <typename Operation>
void captureGeneratedTopology(
    Operation& operation,
    const TopoDS_Shape& source,
    const TopoDS_Shape& result,
    const std::vector<TopoDS_Edge>& selected,
    OperationRun& run) {
    const auto created_faces =
        newFaces(source, result);
    run.new_face_count =
        created_faces.size();

    std::vector<TopoDS_Face>
        generated_from_edges;
    run.generated_faces_per_edge.reserve(
        selected.size());

    for (const auto& edge : selected) {
        const auto faces =
            facesFromList(
                operation.Generated(edge));
        run.generated_faces_per_edge.push_back(
            faces.size());
        for (const auto& face : faces) {
            appendUnique(
                generated_from_edges,
                face);
        }
    }

    run.edge_generated_face_count =
        generated_from_edges.size();

    std::vector<TopoDS_Face>
        generated_from_vertices;
    for (const auto& vertex :
         sharedVertices(selected)) {
        const auto faces =
            facesFromList(
                operation.Generated(vertex));
        for (const auto& face : faces) {
            appendUnique(
                generated_from_vertices,
                face);
        }
    }
    run.shared_vertex_generated_face_count =
        generated_from_vertices.size();

    std::vector<TopoDS_Face> claimed =
        generated_from_edges;
    for (const auto& face :
         generated_from_vertices) {
        appendUnique(claimed, face);
    }

    for (const auto& face : created_faces) {
        const bool is_claimed =
            std::any_of(
                claimed.begin(),
                claimed.end(),
                [&face](const TopoDS_Face& candidate) {
                    return candidate.IsSame(face);
                });
        if (!is_claimed) {
            ++run.unclaimed_new_face_count;
        }
    }
}

template <typename Operation>
[[nodiscard]] OperationRun
runOperation(
    const TopoDS_Shape& source,
    const std::vector<TopoDS_Edge>& selected,
    double parameter) {
    OperationRun run;
    try {
        Operation operation{source};
        for (const auto& edge : selected) {
            operation.Add(parameter, edge);
        }

        captureContourMembership(
            operation,
            selected,
            run);

        run.build_attempted = true;
        operation.Build();
        if (!operation.IsDone()) {
            return run;
        }

        const auto result =
            operation.Shape();
        if (result.IsNull()) {
            return run;
        }

        const auto evidence =
            shapeEvidence(result);
        run.shape = evidence;
        if (!evidence.ok() ||
            evidence.solid_count != 1U) {
            return run;
        }

        run.build_succeeded = true;
        run.signature =
            topologySignature(result);
        captureGeneratedTopology(
            operation,
            source,
            result,
            selected,
            run);
        return run;
    } catch (const Standard_Failure&) {
        run.build_attempted = true;
        return run;
    } catch (...) {
        run.build_attempted = true;
        return run;
    }
}

[[nodiscard]] std::vector<EdgeEndpoints>
scenarioEndpoints(
    kernel::EdgeFeatureProbeScenario scenario) {
    const EdgeEndpoints top_front_x{
        {0.0, 0.0, box_z},
        {box_x, 0.0, box_z}};
    const EdgeEndpoints top_right_y{
        {box_x, 0.0, box_z},
        {box_x, box_y, box_z}};
    const EdgeEndpoints top_back_x{
        {0.0, box_y, box_z},
        {box_x, box_y, box_z}};
    const EdgeEndpoints top_left_y{
        {0.0, 0.0, box_z},
        {0.0, box_y, box_z}};
    const EdgeEndpoints front_right_z{
        {box_x, 0.0, 0.0},
        {box_x, 0.0, box_z}};
    const EdgeEndpoints back_left_z{
        {0.0, box_y, 0.0},
        {0.0, box_y, box_z}};
    const EdgeEndpoints bottom_back_x{
        {0.0, box_y, 0.0},
        {box_x, box_y, 0.0}};

    switch (scenario) {
    case kernel::EdgeFeatureProbeScenario::single_edge:
    case kernel::EdgeFeatureProbeScenario::excessive_parameter:
        return {top_front_x};
    case kernel::EdgeFeatureProbeScenario::disconnected_pair:
        return {
            top_front_x,
            bottom_back_x};
    case kernel::EdgeFeatureProbeScenario::adjacent_pair:
        return {
            top_front_x,
            top_right_y};
    case kernel::EdgeFeatureProbeScenario::trihedral_corner:
        return {
            top_front_x,
            top_right_y,
            front_right_z};
    case kernel::EdgeFeatureProbeScenario::closed_loop:
        return {
            top_front_x,
            top_right_y,
            top_back_x,
            top_left_y};
    case kernel::EdgeFeatureProbeScenario::
            mixed_connected_disconnected:
        return {
            top_front_x,
            top_right_y,
            back_left_z};
    }
    return {};
}

[[nodiscard]] OperationRun execute(
    kernel::EdgeFeatureEvidenceOperation operation,
    const TopoDS_Shape& source,
    const std::vector<TopoDS_Edge>& selected,
    double parameter);

[[nodiscard]] bool containsSameSubshape(
    const TopoDS_Shape& container,
    const TopoDS_Shape& candidate,
    TopAbs_ShapeEnum kind) {
    for (TopExp_Explorer explorer{container, kind};
         explorer.More();
         explorer.Next()) {
        if (explorer.Current().IsSame(candidate)) {
            return true;
        }
    }
    return false;
}

template <typename Operation>
[[nodiscard]] std::vector<TopoDS_Edge>
edgeHistoryDescendants(
    Operation& operation,
    const TopoDS_Edge& source,
    const TopoDS_Shape& result) {
    std::vector<TopoDS_Edge> descendants;

    const auto append_edges =
        [&descendants](const TopTools_ListOfShape& shapes) {
            for (TopTools_ListOfShape::Iterator it{shapes};
                 it.More();
                 it.Next()) {
                const auto& shape = it.Value();
                if (shape.ShapeType() == TopAbs_EDGE) {
                    appendUnique(
                        descendants,
                        TopoDS::Edge(shape));
                }
            }
        };

    append_edges(operation.Modified(source));
    append_edges(operation.Generated(source));

    if (!operation.IsDeleted(source) &&
        containsSameSubshape(
            result,
            source,
            TopAbs_EDGE)) {
        appendUnique(descendants, source);
    }

    descendants.erase(
        std::remove_if(
            descendants.begin(),
            descendants.end(),
            [&result](const TopoDS_Edge& edge) {
                return !containsSameSubshape(
                    result,
                    edge,
                    TopAbs_EDGE);
            }),
        descendants.end());
    return descendants;
}

[[nodiscard]] kernel::ReferenceStatus
referenceStatus(std::size_t candidate_count) noexcept {
    if (candidate_count == 0U) {
        return kernel::ReferenceStatus::missing;
    }
    if (candidate_count == 1U) {
        return kernel::ReferenceStatus::resolved;
    }
    return kernel::ReferenceStatus::ambiguous;
}

struct TangentFixture final {
    TopoDS_Shape shape;
    TopoDS_Edge selected_edge;
};

[[nodiscard]] std::optional<TangentFixture>
buildTangentFixture() {
    // Evidence-only fixture: the front top boundary is deliberately split into
    // two collinear segments. It is not proposed as a durable semantic Edge
    // model; it exists only to observe whether the native edge-feature
    // provider silently grows one registered Edge into a tangent contour.
    BRepBuilderAPI_MakePolygon polygon;
    polygon.Add(gp_Pnt{0.0, 0.0, 0.0});
    polygon.Add(gp_Pnt{20.0, 0.0, 0.0});
    polygon.Add(gp_Pnt{40.0, 0.0, 0.0});
    polygon.Add(gp_Pnt{40.0, 30.0, 0.0});
    polygon.Add(gp_Pnt{0.0, 30.0, 0.0});
    polygon.Close();
    if (!polygon.IsDone()) {
        return std::nullopt;
    }

    BRepBuilderAPI_MakeFace face{polygon.Wire()};
    if (!face.IsDone()) {
        return std::nullopt;
    }

    BRepPrimAPI_MakePrism prism{
        face.Face(),
        gp_Vec{0.0, 0.0, box_z}};
    const auto shape = prism.Shape();
    if (shape.IsNull()) {
        return std::nullopt;
    }

    const auto selected =
        findEdgeByEndpoints(
            shape,
            {
                {0.0, 0.0, box_z},
                {20.0, 0.0, box_z},
            });
    if (!selected) {
        return std::nullopt;
    }

    return TangentFixture{
        shape,
        *selected};
}

[[nodiscard]] kernel::EdgeFeatureTangentChainEvidence
buildTangentChainEvidence(
    kernel::EdgeFeatureEvidenceOperation operation) {
    kernel::EdgeFeatureTangentChainEvidence evidence;
    evidence.operation = operation;

    const auto fixture = buildTangentFixture();
    if (!fixture) {
        evidence.source_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }

    evidence.source_shape =
        shapeEvidence(fixture->shape);
    evidence.requested_edge_count = 1U;

    const auto run =
        execute(
            operation,
            fixture->shape,
            {fixture->selected_edge},
            normal_parameter);

    evidence.provider_contour_count =
        run.contour_count;
    evidence.provider_contour_edge_count =
        run.contour_edge_count;
    evidence.exact_provider_input_membership =
        run.exact_membership;
    evidence.build_succeeded =
        run.build_succeeded;
    return evidence;
}

[[nodiscard]] kernel::EdgeFeatureUpstreamEvidence
buildDimensionChangeEvidence(
    kernel::EdgeFeatureEvidenceOperation operation) {
    kernel::EdgeFeatureUpstreamEvidence evidence;
    evidence.operation = operation;
    evidence.scenario =
        kernel::EdgeFeatureUpstreamScenario::dimension_change;

    BRepPrimAPI_MakeBox before{
        40.0,
        20.0,
        10.0};
    BRepPrimAPI_MakeBox after{
        55.0,
        25.0,
        10.0};

    const auto before_shape = before.Shape();
    const auto after_shape = after.Shape();
    evidence.source_shape =
        shapeEvidence(before_shape);
    evidence.edited_shape =
        shapeEvidence(after_shape);

    const auto before_edge =
        findEdgeByEndpoints(
            before_shape,
            {
                {0.0, 0.0, 10.0},
                {40.0, 0.0, 10.0},
            });
    const auto after_edge =
        findEdgeByEndpoints(
            after_shape,
            {
                {0.0, 0.0, 10.0},
                {55.0, 0.0, 10.0},
            });

    evidence.current_edge_candidate_count =
        after_edge ? 1U : 0U;
    evidence.reference_status =
        referenceStatus(
            evidence.current_edge_candidate_count);

    if (!before_edge || !after_edge) {
        return evidence;
    }

    evidence.downstream_attempted = true;
    const auto run =
        execute(
            operation,
            after_shape,
            {*after_edge},
            1.0);
    evidence.downstream_succeeded =
        run.build_succeeded;
    evidence.exact_provider_input_membership =
        run.exact_membership;
    return evidence;
}

[[nodiscard]] kernel::EdgeFeatureUpstreamEvidence
buildBooleanUpstreamEvidence(
    kernel::EdgeFeatureEvidenceOperation operation,
    kernel::EdgeFeatureUpstreamScenario scenario) {
    kernel::EdgeFeatureUpstreamEvidence evidence;
    evidence.operation = operation;
    evidence.scenario = scenario;

    BRepPrimAPI_MakeBox base{
        40.0,
        20.0,
        10.0};
    const auto source = base.Shape();
    evidence.source_shape =
        shapeEvidence(source);

    const auto source_edge =
        findEdgeByEndpoints(
            source,
            {
                {0.0, 0.0, 10.0},
                {40.0, 0.0, 10.0},
            });
    if (!source_edge) {
        evidence.edited_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }

    gp_Pnt tool_origin;
    double tool_dx{};
    double tool_dy{};
    constexpr double tool_dz = 10.0;

    switch (scenario) {
    case kernel::EdgeFeatureUpstreamScenario::unchanged:
        tool_origin = gp_Pnt{30.0, 10.0, 5.0};
        tool_dx = 5.0;
        tool_dy = 5.0;
        break;
    case kernel::EdgeFeatureUpstreamScenario::trim:
        tool_origin = gp_Pnt{30.0, -5.0, 5.0};
        tool_dx = 15.0;
        tool_dy = 10.0;
        break;
    case kernel::EdgeFeatureUpstreamScenario::split:
        tool_origin = gp_Pnt{15.0, -5.0, 5.0};
        tool_dx = 10.0;
        tool_dy = 10.0;
        break;
    case kernel::EdgeFeatureUpstreamScenario::remove:
        tool_origin = gp_Pnt{-5.0, -5.0, 5.0};
        tool_dx = 50.0;
        tool_dy = 10.0;
        break;
    case kernel::EdgeFeatureUpstreamScenario::dimension_change:
        return buildDimensionChangeEvidence(operation);
    }

    BRepPrimAPI_MakeBox tool{
        tool_origin,
        tool_dx,
        tool_dy,
        tool_dz};

    BRepAlgoAPI_Cut cut{
        source,
        tool.Shape()};
    cut.SetFuzzyValue(0.0);
    cut.Build();
    if (!cut.IsDone() ||
        cut.Shape().IsNull()) {
        evidence.edited_shape.status =
            kernel::EvidenceStatus::provider_failure;
        return evidence;
    }

    const auto result =
        cut.Shape();
    evidence.edited_shape =
        shapeEvidence(result);

    const auto descendants =
        edgeHistoryDescendants(
            cut,
            *source_edge,
            result);
    evidence.current_edge_candidate_count =
        descendants.size();
    evidence.reference_status =
        referenceStatus(descendants.size());

    if (descendants.size() != 1U) {
        return evidence;
    }

    evidence.downstream_attempted = true;
    const auto run =
        execute(
            operation,
            result,
            {descendants.front()},
            1.0);
    evidence.downstream_succeeded =
        run.build_succeeded;
    evidence.exact_provider_input_membership =
        run.exact_membership;
    return evidence;
}

template <typename FirstOperation, typename SecondOperation>
[[nodiscard]] kernel::EdgeFeatureChainingEvidence
buildChainingEvidenceTyped(
    kernel::EdgeFeatureEvidenceOperation first_kind,
    kernel::EdgeFeatureEvidenceOperation second_kind) {
    kernel::EdgeFeatureChainingEvidence evidence;
    evidence.first_operation = first_kind;
    evidence.second_operation = second_kind;

    BRepPrimAPI_MakeBox box{
        box_x,
        box_y,
        box_z};
    const auto source = box.Shape();
    evidence.source_shape =
        shapeEvidence(source);

    const auto source_edge =
        findEdgeByEndpoints(
            source,
            {
                {0.0, 0.0, box_z},
                {box_x, 0.0, box_z},
            });
    if (!source_edge) {
        return evidence;
    }

    FirstOperation first{source};
    first.Add(normal_parameter, *source_edge);
    first.Build();
    if (!first.IsDone() ||
        first.Shape().IsNull()) {
        return evidence;
    }

    const auto first_result =
        first.Shape();
    evidence.first_result_shape =
        shapeEvidence(first_result);
    if (!evidence.first_result_shape.ok()) {
        return evidence;
    }

    const auto generated_faces =
        facesFromList(
            first.Generated(*source_edge));
    evidence.first_generated_face_count =
        generated_faces.size();

    TopTools_IndexedDataMapOfShapeListOfShape
        edge_faces;
    TopExp::MapShapesAndAncestors(
        first_result,
        TopAbs_EDGE,
        TopAbs_FACE,
        edge_faces);

    std::vector<TopoDS_Edge> candidates;
    for (const auto& face : generated_faces) {
        for (TopExp_Explorer explorer{
                 face,
                 TopAbs_EDGE};
             explorer.More();
             explorer.Next()) {
            const auto edge =
                TopoDS::Edge(explorer.Current());
            if (BRep_Tool::Degenerated(edge)) {
                continue;
            }

            const auto index =
                edge_faces.FindIndex(edge);
            if (index <= 0) {
                continue;
            }

            const auto& ancestors =
                edge_faces.FindFromIndex(index);
            if (ancestors.Extent() != 2) {
                continue;
            }
            appendUnique(candidates, edge);
        }
    }

    evidence.generated_boundary_edge_count =
        candidates.size();

    constexpr double second_parameter = 0.75;
    for (const auto& candidate : candidates) {
        ++evidence.second_operation_attempt_count;
        const auto run =
            runOperation<SecondOperation>(
                first_result,
                {candidate},
                second_parameter);
        if (run.build_succeeded) {
            ++evidence.second_operation_success_count;
        }
    }

    return evidence;
}

[[nodiscard]] kernel::EdgeFeatureChainingEvidence
buildChainingEvidence(
    kernel::EdgeFeatureEvidenceOperation first,
    kernel::EdgeFeatureEvidenceOperation second) {
    if (first ==
            kernel::EdgeFeatureEvidenceOperation::fillet &&
        second ==
            kernel::EdgeFeatureEvidenceOperation::chamfer) {
        return buildChainingEvidenceTyped<
            BRepFilletAPI_MakeFillet,
            BRepFilletAPI_MakeChamfer>(
                first,
                second);
    }
    return buildChainingEvidenceTyped<
        BRepFilletAPI_MakeChamfer,
        BRepFilletAPI_MakeFillet>(
            first,
            second);
}

[[nodiscard]] bool sameSignature(
    const std::optional<
        kernel::EdgeFeatureTopologySignature>& first,
    const std::optional<
        kernel::EdgeFeatureTopologySignature>& second) {
    if (first.has_value() != second.has_value()) {
        return false;
    }
    if (!first) {
        return true;
    }
    if (first->face_count != second->face_count ||
        first->edge_count != second->edge_count ||
        first->vertex_count != second->vertex_count) {
        return false;
    }
    const double scale =
        std::max({
            1.0,
            std::abs(first->volume),
            std::abs(second->volume)});
    return std::abs(
               first->volume -
               second->volume) <=
           volume_tolerance * scale;
}

[[nodiscard]] OperationRun execute(
    kernel::EdgeFeatureEvidenceOperation operation,
    const TopoDS_Shape& source,
    const std::vector<TopoDS_Edge>& selected,
    double parameter) {
    if (operation ==
        kernel::EdgeFeatureEvidenceOperation::fillet) {
        return runOperation<
            BRepFilletAPI_MakeFillet>(
                source,
                selected,
                parameter);
    }
    return runOperation<
        BRepFilletAPI_MakeChamfer>(
            source,
            selected,
            parameter);
}

} // namespace

kernel::EdgeFeatureProviderEvidence
buildEdgeFeatureProviderEvidence(
    kernel::EdgeFeatureEvidenceOperation operation,
    kernel::EdgeFeatureProbeScenario scenario) noexcept {
    kernel::EdgeFeatureProviderEvidence evidence;
    evidence.operation = operation;
    evidence.scenario = scenario;
    evidence.parameter =
        scenario ==
                kernel::EdgeFeatureProbeScenario::
                    excessive_parameter
            ? excessive_parameter
            : normal_parameter;

    try {
        BRepPrimAPI_MakeBox box{
            box_x,
            box_y,
            box_z};
        const auto source =
            box.Shape();
        evidence.source_shape =
            shapeEvidence(source);

        const auto endpoints =
            scenarioEndpoints(scenario);
        evidence.requested_edge_count =
            endpoints.size();

        std::vector<TopoDS_Edge> selected;
        selected.reserve(endpoints.size());
        for (const auto& pair : endpoints) {
            const auto edge =
                findEdgeByEndpoints(
                    source,
                    pair);
            if (!edge) {
                return evidence;
            }
            selected.push_back(*edge);
        }
        evidence.resolved_input_edge_count =
            selected.size();

        const auto forward =
            execute(
                operation,
                source,
                selected,
                evidence.parameter);

        evidence.provider_contour_count =
            forward.contour_count;
        evidence.provider_contour_edge_count =
            forward.contour_edge_count;
        evidence.exact_provider_input_membership =
            forward.exact_membership;
        evidence.build_attempted =
            forward.build_attempted;
        evidence.build_succeeded =
            forward.build_succeeded;
        evidence.result_shape =
            forward.shape;
        evidence.result_signature =
            forward.signature;
        evidence.new_face_count =
            forward.new_face_count;
        evidence.generated_from_selected_edges_face_count =
            forward.edge_generated_face_count;
        evidence.generated_from_shared_vertices_face_count =
            forward.shared_vertex_generated_face_count;
        evidence.unclaimed_new_face_count =
            forward.unclaimed_new_face_count;
        evidence.generated_faces_per_selected_edge =
            forward.generated_faces_per_edge;

        std::reverse(
            selected.begin(),
            selected.end());
        const auto reverse =
            execute(
                operation,
                source,
                selected,
                evidence.parameter);
        evidence.reverse_build_succeeded =
            reverse.build_succeeded;
        evidence.reverse_same_topology_and_volume =
            forward.build_succeeded ==
                reverse.build_succeeded &&
            sameSignature(
                forward.signature,
                reverse.signature);

        return evidence;
    } catch (...) {
        evidence.source_shape.status =
            kernel::EvidenceStatus::
                provider_failure;
        return evidence;
    }
}

kernel::EdgeFeatureProviderMatrixEvidence
buildEdgeFeatureProviderMatrixEvidence() noexcept {
    kernel::EdgeFeatureProviderMatrixEvidence matrix;

    constexpr kernel::EdgeFeatureProbeScenario
        scenarios[] = {
            kernel::EdgeFeatureProbeScenario::
                single_edge,
            kernel::EdgeFeatureProbeScenario::
                disconnected_pair,
            kernel::EdgeFeatureProbeScenario::
                adjacent_pair,
            kernel::EdgeFeatureProbeScenario::
                trihedral_corner,
            kernel::EdgeFeatureProbeScenario::
                closed_loop,
            kernel::EdgeFeatureProbeScenario::
                mixed_connected_disconnected,
            kernel::EdgeFeatureProbeScenario::
                excessive_parameter,
        };

    for (const auto operation : {
             kernel::EdgeFeatureEvidenceOperation::
                 fillet,
             kernel::EdgeFeatureEvidenceOperation::
                 chamfer}) {
        for (const auto scenario : scenarios) {
            matrix.probes.push_back(
                buildEdgeFeatureProviderEvidence(
                    operation,
                    scenario));
        }
    }

    return matrix;
}

kernel::EdgeFeatureLifecycleEvidence
buildEdgeFeatureLifecycleEvidence() noexcept {
    kernel::EdgeFeatureLifecycleEvidence evidence;

    try {
        for (const auto operation : {
                 kernel::EdgeFeatureEvidenceOperation::fillet,
                 kernel::EdgeFeatureEvidenceOperation::chamfer}) {
            evidence.tangent_chain.push_back(
                buildTangentChainEvidence(operation));

            for (const auto scenario : {
                     kernel::EdgeFeatureUpstreamScenario::dimension_change,
                     kernel::EdgeFeatureUpstreamScenario::unchanged,
                     kernel::EdgeFeatureUpstreamScenario::trim,
                     kernel::EdgeFeatureUpstreamScenario::split,
                     kernel::EdgeFeatureUpstreamScenario::remove}) {
                evidence.upstream.push_back(
                    buildBooleanUpstreamEvidence(
                        operation,
                        scenario));
            }
        }

        evidence.chaining.push_back(
            buildChainingEvidence(
                kernel::EdgeFeatureEvidenceOperation::fillet,
                kernel::EdgeFeatureEvidenceOperation::chamfer));
        evidence.chaining.push_back(
            buildChainingEvidence(
                kernel::EdgeFeatureEvidenceOperation::chamfer,
                kernel::EdgeFeatureEvidenceOperation::fillet));
        return evidence;
    } catch (...) {
        return evidence;
    }
}

} // namespace simplesolid2::kernel_occt
