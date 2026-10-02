#include <simplesolid2/kernel/evidence.hpp>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepTools_History.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Wire.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-00A E03/E04 CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

using Token = kernel::TransientLineageToken;

[[nodiscard]] std::size_t edgeCount(
    const TopoDS_Shape& shape) {
    std::size_t count = 0U;
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        ++count;
    }
    return count;
}

[[nodiscard]] std::vector<TopoDS_Edge>
edgeList(
    const TopTools_ListOfShape& shapes) {
    std::vector<TopoDS_Edge> result;
    for (TopTools_ListOfShape::Iterator it{shapes};
         it.More();
         it.Next()) {
        const auto& shape = it.Value();
        if (shape.ShapeType() == TopAbs_EDGE) {
            result.push_back(
                TopoDS::Edge(shape));
            continue;
        }

        for (TopExp_Explorer explorer{
                 shape,
                 TopAbs_EDGE};
             explorer.More();
             explorer.Next()) {
            result.push_back(
                TopoDS::Edge(
                    explorer.Current()));
        }
    }
    return result;
}

[[nodiscard]] std::vector<Token>
tokensForUniqueEdges(
    const std::vector<TopoDS_Edge>& edges,
    std::vector<TopoDS_Edge>& registry) {
    std::vector<Token> tokens;
    tokens.reserve(edges.size());

    for (const auto& edge : edges) {
        std::optional<Token> token;
        for (std::size_t index = 0U;
             index < registry.size();
             ++index) {
            if (registry[index].IsSame(edge)) {
                token = static_cast<Token>(
                    index + 1U);
                break;
            }
        }

        if (!token) {
            registry.push_back(edge);
            token = static_cast<Token>(
                registry.size());
        }

        if (std::find(
                tokens.begin(),
                tokens.end(),
                *token) == tokens.end()) {
            tokens.push_back(*token);
        }
    }

    return tokens;
}

struct SplitObservation final {
    bool provider_deleted{false};
    std::vector<Token> candidates;

    friend bool operator==(
        const SplitObservation&,
        const SplitObservation&) = default;
};

[[nodiscard]] SplitObservation
probeSplitEdge() {
    const TopoDS_Edge source =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{0.0, 0.0, 0.0},
            gp_Pnt{40.0, 0.0, 0.0}}
            .Edge();
    const TopoDS_Edge tool =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{20.0, -10.0, 0.0},
            gp_Pnt{20.0, 10.0, 0.0}}
            .Edge();

    TopTools_ListOfShape arguments;
    arguments.Append(source);
    TopTools_ListOfShape tools;
    tools.Append(tool);

    BRepAlgoAPI_Splitter splitter;
    splitter.SetArguments(arguments);
    splitter.SetTools(tools);
    splitter.Build();
    CHECK(splitter.IsDone());

    const auto modified =
        edgeList(
            splitter.Modified(source));

    std::vector<TopoDS_Edge> registry;
    const auto tokens =
        tokensForUniqueEdges(
            modified,
            registry);

    std::cerr
        << "E03_SPLIT_DIAG deleted="
        << splitter.IsDeleted(source)
        << " candidates="
        << tokens.size()
        << " result_edges="
        << edgeCount(splitter.Shape())
        << '\n';

    return {
        splitter.IsDeleted(source),
        tokens};
}

struct DeleteObservation final {
    bool provider_deleted{false};
    std::size_t modified_count{};

    friend bool operator==(
        const DeleteObservation&,
        const DeleteObservation&) = default;
};

[[nodiscard]] DeleteObservation
probeDeletedEdge() {
    const TopoDS_Edge source =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{0.0, 0.0, 0.0},
            gp_Pnt{40.0, 0.0, 0.0}}
            .Edge();
    const TopoDS_Edge covering_tool =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{-10.0, 0.0, 0.0},
            gp_Pnt{50.0, 0.0, 0.0}}
            .Edge();

    BRepAlgoAPI_Cut cut{
        source,
        covering_tool};
    cut.Build();
    CHECK(cut.IsDone());

    const auto modified =
        edgeList(
            cut.Modified(source));

    std::cerr
        << "E03_DELETE_DIAG deleted="
        << cut.IsDeleted(source)
        << " modified="
        << modified.size()
        << " result_edges="
        << edgeCount(cut.Shape())
        << '\n';

    return {
        cut.IsDeleted(source),
        modified.size()};
}

struct MergeObservation final {
    bool first_removed{false};
    bool second_removed{false};
    std::vector<Token> first_candidates;
    std::vector<Token> second_candidates;
    std::size_t input_edge_count{};
    std::size_t output_edge_count{};

    friend bool operator==(
        const MergeObservation&,
        const MergeObservation&) = default;
};

[[nodiscard]] MergeObservation
probeMergedEdges() {
    const TopoDS_Edge first =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{0.0, 0.0, 0.0},
            gp_Pnt{20.0, 0.0, 0.0}}
            .Edge();
    const TopoDS_Edge second =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{20.0, 0.0, 0.0},
            gp_Pnt{40.0, 0.0, 0.0}}
            .Edge();
    const TopoDS_Edge right =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{40.0, 0.0, 0.0},
            gp_Pnt{40.0, 30.0, 0.0}}
            .Edge();
    const TopoDS_Edge top =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{40.0, 30.0, 0.0},
            gp_Pnt{0.0, 30.0, 0.0}}
            .Edge();
    const TopoDS_Edge left =
        BRepBuilderAPI_MakeEdge{
            gp_Pnt{0.0, 30.0, 0.0},
            gp_Pnt{0.0, 0.0, 0.0}}
            .Edge();

    BRepBuilderAPI_MakeWire make_wire;
    make_wire.Add(first);
    make_wire.Add(second);
    make_wire.Add(right);
    make_wire.Add(top);
    make_wire.Add(left);
    CHECK(make_wire.IsDone());

    const TopoDS_Face face =
        BRepBuilderAPI_MakeFace{
            make_wire.Wire(),
            true}
            .Face();

    ShapeUpgrade_UnifySameDomain unify;
    unify.Initialize(
        face,
        false,
        true,
        false);
    unify.Build();

    const TopoDS_Shape result =
        unify.Shape();
    CHECK(!result.IsNull());

    const auto history =
        unify.History();
    CHECK(!history.IsNull());

    const auto first_modified =
        edgeList(
            history->Modified(first));
    const auto second_modified =
        edgeList(
            history->Modified(second));

    std::vector<TopoDS_Edge> registry;
    const auto first_tokens =
        tokensForUniqueEdges(
            first_modified,
            registry);
    const auto second_tokens =
        tokensForUniqueEdges(
            second_modified,
            registry);

    const bool first_removed =
        history->IsRemoved(first);
    const bool second_removed =
        history->IsRemoved(second);

    std::cerr
        << "E04_MERGE_DIAG"
        << " first_removed="
        << first_removed
        << " second_removed="
        << second_removed
        << " first_modified="
        << first_tokens.size()
        << " second_modified="
        << second_tokens.size()
        << " input_edges="
        << edgeCount(face)
        << " output_edges="
        << edgeCount(result)
        << '\n';

    return {
        first_removed,
        second_removed,
        first_tokens,
        second_tokens,
        edgeCount(face),
        edgeCount(result)};
}

[[nodiscard]] std::vector<
    kernel::SingularLineageSourceEvidence>
splitSources(
    const SplitObservation& split) {
    return {{
        "source-edge",
        false,
        split.provider_deleted,
        split.candidates,
        split.candidates}};
}

void verifyE03(
    const SplitObservation& split,
    const DeleteObservation& deletion) {
    CHECK(split.candidates.size() == 2U);

    // E03-01: a singular source with two semantic descendants is Ambiguous.
    auto sources =
        splitSources(split);
    CHECK(
        kernel::classifySingularLineage(
            sources,
            "source-edge") ==
        kernel::ReferenceStatus::ambiguous);

    // E03-02: provider cardinality may remain two, but independent semantic
    // role evidence can exclude one technical descendant without ranking by
    // length, distance or provider order.
    sources.front().semantic_candidates = {
        split.candidates.front()};
    CHECK(
        kernel::classifySingularLineage(
            sources,
            "source-edge") ==
        kernel::ReferenceStatus::resolved);

    // E03-03: explicit provider deletion with no semantic descendant is
    // Missing; no unrelated candidate is fabricated.
    CHECK(deletion.provider_deleted);
    CHECK(deletion.modified_count == 0U);
    const std::vector<
        kernel::SingularLineageSourceEvidence>
        deleted_sources{{
            "deleted-edge",
            false,
            true,
            {},
            {}}};
    CHECK(
        kernel::classifySingularLineage(
            deleted_sources,
            "deleted-edge") ==
        kernel::ReferenceStatus::missing);

    // E03-04: asking a singular reference to turn into an undeclared
    // collection remains Unsupported.
    sources =
        splitSources(split);
    sources.front().aggregate_requested = true;
    CHECK(
        kernel::classifySingularLineage(
            sources,
            "source-edge") ==
        kernel::ReferenceStatus::unsupported);
}

void verifyE04(
    const MergeObservation& merge) {
    CHECK(merge.input_edge_count == 5U);
    CHECK(merge.output_edge_count == 4U);
    CHECK(!merge.first_candidates.empty());
    CHECK(!merge.second_candidates.empty());

    // The real provider history must expose one common result edge for the
    // two formerly distinct source meanings.
    std::optional<Token> common;
    for (const auto first :
         merge.first_candidates) {
        if (std::find(
                merge.second_candidates.begin(),
                merge.second_candidates.end(),
                first) !=
            merge.second_candidates.end()) {
            common = first;
            break;
        }
    }
    CHECK(common.has_value());

    // E04-01: two live semantic meanings collide on one provider subshape.
    // Both singular references are Ambiguous; both must never report
    // Resolved to the same physical edge.
    std::vector<
        kernel::SingularLineageSourceEvidence>
        collided{
            {
                "first-edge",
                false,
                merge.first_removed,
                merge.first_candidates,
                {*common},
            },
            {
                "second-edge",
                false,
                merge.second_removed,
                merge.second_candidates,
                {*common},
            },
        };
    CHECK(
        kernel::classifySingularLineage(
            collided,
            "first-edge") ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        kernel::classifySingularLineage(
            collided,
            "second-edge") ==
        kernel::ReferenceStatus::ambiguous);

    // E04-02: provider Modified/Removed bookkeeping alone cannot choose a
    // semantic winner. Even if one source is reported removed, retaining
    // both meanings against the common provider candidate is Ambiguous.
    CHECK(
        kernel::classifySingularLineage(
            collided,
            "first-edge") ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        kernel::classifySingularLineage(
            collided,
            "second-edge") ==
        kernel::ReferenceStatus::ambiguous);

    // E04-03: independent producer-role evidence may explicitly preserve
    // one meaning and remove the other.
    auto preserved = collided;
    preserved[0].semantic_candidates = {
        *common};
    preserved[1].semantic_candidates.clear();
    CHECK(
        kernel::classifySingularLineage(
            preserved,
            "first-edge") ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        kernel::classifySingularLineage(
            preserved,
            "second-edge") ==
        kernel::ReferenceStatus::missing);

    // E04-04: undeclared merged aggregate semantics remain Unsupported.
    auto aggregate = collided;
    aggregate[0].aggregate_requested = true;
    CHECK(
        kernel::classifySingularLineage(
            aggregate,
            "first-edge") ==
        kernel::ReferenceStatus::unsupported);
}

} // namespace

int main() {
    const auto split =
        probeSplitEdge();
    const auto deletion =
        probeDeletedEdge();
    const auto merge =
        probeMergedEdges();

    verifyE03(split, deletion);
    verifyE04(merge);

    // COLD replay: rebuild every provider shape/history from neutral constants
    // with no prior TopoDS handles or history objects reused.
    const auto cold_split =
        probeSplitEdge();
    const auto cold_deletion =
        probeDeletedEdge();
    const auto cold_merge =
        probeMergedEdges();

    CHECK(cold_split == split);
    CHECK(cold_deletion == deletion);
    CHECK(cold_merge == merge);
    verifyE03(cold_split, cold_deletion);
    verifyE04(cold_merge);

    std::cout
        << "PM00A_E03_E04_PASS"
        << " rows=E03-01..E03-04,E04-01..E04-04"
        << " false_resolved=0\n";
    return EXIT_SUCCESS;
}
