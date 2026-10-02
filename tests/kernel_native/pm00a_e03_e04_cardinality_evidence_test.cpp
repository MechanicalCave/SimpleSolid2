#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepSweep_Prism.hxx>
#include <BRepTools_History.hxx>
#include <BRep_Tool.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp_Pnt.hxx>
#include <gp_Vec.hxx>

#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E03/E04 diagnostic CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

[[nodiscard]] bool near(
    const gp_Pnt& first,
    const gp_Pnt& second) {
    return first.Distance(second) <= 1.0e-8;
}

[[nodiscard]] bool sameUnorderedEndpoints(
    const TopoDS_Edge& edge,
    const gp_Pnt& first,
    const gp_Pnt& second) {
    TopoDS_Vertex start;
    TopoDS_Vertex end;
    TopExp::Vertices(
        edge,
        start,
        end,
        true);
    if (start.IsNull() || end.IsNull()) {
        return false;
    }

    const gp_Pnt a = BRep_Tool::Pnt(start);
    const gp_Pnt b = BRep_Tool::Pnt(end);
    return (near(a, first) && near(b, second)) ||
           (near(a, second) && near(b, first));
}

[[nodiscard]] std::optional<TopoDS_Edge>
findEdge(
    const TopoDS_Shape& shape,
    const gp_Pnt& first,
    const gp_Pnt& second) {
    std::optional<TopoDS_Edge> result;
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto edge =
            TopoDS::Edge(
                explorer.Current());
        if (!sameUnorderedEndpoints(
                edge,
                first,
                second)) {
            continue;
        }

        CHECK(!result.has_value());
        result = edge;
    }
    return result;
}

[[nodiscard]] std::size_t countKind(
    const TopoDS_Shape& shape,
    TopAbs_ShapeEnum kind) {
    std::size_t count = 0U;
    for (TopExp_Explorer explorer{
             shape,
             kind};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

[[nodiscard]] std::size_t countKind(
    const TopTools_ListOfShape& shapes,
    TopAbs_ShapeEnum kind) {
    std::size_t count = 0U;
    for (TopTools_ListOfShape::Iterator it{shapes};
         it.More();
         it.Next()) {
        const auto& shape = it.Value();
        if (shape.ShapeType() == kind) {
            ++count;
        } else {
            count += countKind(shape, kind);
        }
    }
    return count;
}

[[nodiscard]] bool listsShareSameFace(
    const TopTools_ListOfShape& first,
    const TopTools_ListOfShape& second) {
    for (TopTools_ListOfShape::Iterator a{first};
         a.More();
         a.Next()) {
        for (TopTools_ListOfShape::Iterator b{second};
             b.More();
             b.Next()) {
            if (a.Value().ShapeType() ==
                    TopAbs_FACE &&
                b.Value().ShapeType() ==
                    TopAbs_FACE &&
                a.Value().IsSame(
                    b.Value())) {
                return true;
            }
        }
    }
    return false;
}

void diagnoseCutHistory() {
    const TopoDS_Shape base =
        BRepPrimAPI_MakeBox(
            gp_Pnt{0.0, 0.0, 0.0},
            40.0,
            30.0,
            10.0)
            .Shape();

    const auto source =
        findEdge(
            base,
            {0.0, 0.0, 10.0},
            {40.0, 0.0, 10.0});
    CHECK(source.has_value());

    // Middle notch: expected to split the top-front source edge.
    {
        const TopoDS_Shape tool =
            BRepPrimAPI_MakeBox(
                gp_Pnt{15.0, -5.0, 5.0},
                10.0,
                10.0,
                10.0)
                .Shape();

        BRepAlgoAPI_Cut cut{
            base,
            tool};
        CHECK(cut.IsDone());
        CHECK(!cut.Shape().IsNull());

        const auto& modified =
            cut.Modified(*source);
        const auto& generated =
            cut.Generated(*source);

        std::cout
            << "E03_SPLIT modified_edges="
            << countKind(modified, TopAbs_EDGE)
            << " generated_edges="
            << countKind(generated, TopAbs_EDGE)
            << " is_deleted="
            << cut.IsDeleted(*source)
            << " result_edges="
            << countKind(
                   cut.Shape(),
                   TopAbs_EDGE)
            << '\n';
    }

    // Full-width upper-front removal: expected to delete the source edge.
    {
        const TopoDS_Shape tool =
            BRepPrimAPI_MakeBox(
                gp_Pnt{-1.0, -5.0, 5.0},
                42.0,
                10.0,
                10.0)
                .Shape();

        BRepAlgoAPI_Cut cut{
            base,
            tool};
        CHECK(cut.IsDone());
        CHECK(!cut.Shape().IsNull());

        const auto& modified =
            cut.Modified(*source);
        const auto& generated =
            cut.Generated(*source);

        std::cout
            << "E03_DELETE modified_edges="
            << countKind(modified, TopAbs_EDGE)
            << " generated_edges="
            << countKind(generated, TopAbs_EDGE)
            << " is_deleted="
            << cut.IsDeleted(*source)
            << " result_edges="
            << countKind(
                   cut.Shape(),
                   TopAbs_EDGE)
            << '\n';
    }

    // End notch: expected to retain one direct lineage descendant while
    // the boolean introduces other technical/result edges.
    {
        const TopoDS_Shape tool =
            BRepPrimAPI_MakeBox(
                gp_Pnt{30.0, -5.0, 5.0},
                15.0,
                10.0,
                10.0)
                .Shape();

        BRepAlgoAPI_Cut cut{
            base,
            tool};
        CHECK(cut.IsDone());
        CHECK(!cut.Shape().IsNull());

        const auto& modified =
            cut.Modified(*source);
        const auto& generated =
            cut.Generated(*source);

        std::cout
            << "E03_TRIM modified_edges="
            << countKind(modified, TopAbs_EDGE)
            << " generated_edges="
            << countKind(generated, TopAbs_EDGE)
            << " is_deleted="
            << cut.IsDeleted(*source)
            << " result_edges="
            << countKind(
                   cut.Shape(),
                   TopAbs_EDGE)
            << '\n';
    }
}

struct SplitPrism final {
    TopoDS_Shape shape;
    TopoDS_Face side_a;
    TopoDS_Face side_b;
};

[[nodiscard]] SplitPrism
buildSplitBottomPrism() {
    const std::array<gp_Pnt, 6U> points{
        gp_Pnt{0.0, 0.0, 0.0},
        gp_Pnt{20.0, 0.0, 0.0},
        gp_Pnt{40.0, 0.0, 0.0},
        gp_Pnt{40.0, 30.0, 0.0},
        gp_Pnt{0.0, 30.0, 0.0},
        gp_Pnt{0.0, 0.0, 0.0},
    };

    BRepBuilderAPI_MakeWire make_wire;
    std::vector<TopoDS_Edge> accepted_edges;
    accepted_edges.reserve(5U);

    for (std::size_t index = 0U;
         index < 5U;
         ++index) {
        BRepBuilderAPI_MakeEdge make_edge{
            points[index],
            points[index + 1U]};
        CHECK(make_edge.IsDone());

        make_wire.Add(
            make_edge.Edge());
        CHECK(make_wire.IsDone());
        accepted_edges.push_back(
            make_wire.Edge());
    }

    BRepBuilderAPI_MakeFace make_face{
        make_wire.Wire(),
        true};
    CHECK(make_face.IsDone());

    BRepSweep_Prism sweep{
        make_face.Face(),
        gp_Vec{0.0, 0.0, 10.0},
        false,
        true};

    const TopoDS_Shape shape =
        sweep.Shape();
    CHECK(!shape.IsNull());

    const TopoDS_Shape side_a =
        sweep.Shape(
            accepted_edges[0]);
    const TopoDS_Shape side_b =
        sweep.Shape(
            accepted_edges[1]);
    CHECK(!side_a.IsNull());
    CHECK(!side_b.IsNull());
    CHECK(side_a.ShapeType() == TopAbs_FACE);
    CHECK(side_b.ShapeType() == TopAbs_FACE);

    return {
        shape,
        TopoDS::Face(side_a),
        TopoDS::Face(side_b)};
}

void diagnoseUnifyHistory() {
    const auto prism =
        buildSplitBottomPrism();

    const auto pre_faces =
        countKind(
            prism.shape,
            TopAbs_FACE);

    ShapeUpgrade_UnifySameDomain unify{
        prism.shape,
        true,
        true,
        false};
    unify.Build();
    const TopoDS_Shape result =
        unify.Shape();
    CHECK(!result.IsNull());

    const auto history =
        unify.History();
    CHECK(!history.IsNull());

    const auto& modified_a =
        history->Modified(
            prism.side_a);
    const auto& modified_b =
        history->Modified(
            prism.side_b);

    std::cout
        << "E04_UNIFY pre_faces="
        << pre_faces
        << " post_faces="
        << countKind(
               result,
               TopAbs_FACE)
        << " a_modified_faces="
        << countKind(
               modified_a,
               TopAbs_FACE)
        << " a_removed="
        << history->IsRemoved(
               prism.side_a)
        << " b_modified_faces="
        << countKind(
               modified_b,
               TopAbs_FACE)
        << " b_removed="
        << history->IsRemoved(
               prism.side_b)
        << " share_same_face="
        << listsShareSameFace(
               modified_a,
               modified_b)
        << '\n';
}

} // namespace

int main() {
    diagnoseCutHistory();
    diagnoseUnifyHistory();

    std::cout
        << "PM00A_E03_E04_DIAGNOSTIC_PASS\n";
    return EXIT_SUCCESS;
}
