#include <BRepAlgoAPI_Cut.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools_History.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS.hxx>
#include <gp_Pnt.hxx>

#include <cstdlib>
#include <iostream>

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E03/E04 registration CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

} // namespace

int main() {
    const TopoDS_Shape base =
        BRepPrimAPI_MakeBox(
            gp_Pnt{0.0, 0.0, 0.0},
            40.0,
            30.0,
            10.0)
            .Shape();
    const TopoDS_Shape tool =
        BRepPrimAPI_MakeBox(
            gp_Pnt{15.0, -5.0, 5.0},
            10.0,
            10.0,
            10.0)
            .Shape();

    TopExp_Explorer edges{
        base,
        TopAbs_EDGE};
    CHECK(edges.More());
    const TopoDS_Edge source_edge =
        TopoDS::Edge(edges.Current());

    BRepAlgoAPI_Cut cut{
        base,
        tool};
    CHECK(cut.IsDone());
    CHECK(!cut.Shape().IsNull());

    (void)cut.Modified(source_edge);
    (void)cut.Generated(source_edge);
    (void)cut.IsDeleted(source_edge);

    ShapeUpgrade_UnifySameDomain unify{
        base,
        true,
        true,
        false};
    unify.Build();
    CHECK(!unify.Shape().IsNull());

    const auto history =
        unify.History();
    CHECK(!history.IsNull());
    (void)history->Modified(source_edge);
    (void)history->IsRemoved(source_edge);

    std::cout
        << "PM00A_E03_E04_REGISTRATION_PASS\n";
    return EXIT_SUCCESS;
}
