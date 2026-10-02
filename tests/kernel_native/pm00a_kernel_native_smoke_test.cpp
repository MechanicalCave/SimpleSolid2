#include <BRepCheck_Analyzer.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Shape.hxx>

#include <cstdlib>
#include <iostream>

namespace {

std::size_t countSubshapes(const TopoDS_Shape& shape, TopAbs_ShapeEnum kind)
{
    std::size_t count = 0;
    for (TopExp_Explorer explorer(shape, kind); explorer.More(); explorer.Next()) {
        ++count;
    }
    return count;
}

} // namespace

int main()
{
    const TopoDS_Shape shape = BRepPrimAPI_MakeBox(10.0, 20.0, 30.0).Shape();

    if (shape.IsNull()) {
        std::cerr << "kernel-native smoke produced a null shape\n";
        return EXIT_FAILURE;
    }

    if (shape.ShapeType() != TopAbs_SOLID) {
        std::cerr << "kernel-native smoke did not produce a solid\n";
        return EXIT_FAILURE;
    }

    const BRepCheck_Analyzer analyzer(shape);
    if (!analyzer.IsValid()) {
        std::cerr << "kernel-native smoke produced an invalid B-Rep\n";
        return EXIT_FAILURE;
    }

    const auto faceCount = countSubshapes(shape, TopAbs_FACE);
    const auto edgeCount = countSubshapes(shape, TopAbs_EDGE);
    if (faceCount != 6 || edgeCount < 12) {
        std::cerr << "unexpected topology counts: faces=" << faceCount
                  << " edge-occurrences=" << edgeCount << "\n";
        return EXIT_FAILURE;
    }

    std::cout << "PM00A_KERNEL_NATIVE_SMOKE_PASS"
              << " faces=" << faceCount
              << " edge_occurrences=" << edgeCount << "\n";
    return EXIT_SUCCESS;
}
