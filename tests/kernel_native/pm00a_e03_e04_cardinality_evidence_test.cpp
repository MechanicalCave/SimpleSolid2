#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <BRepAlgoAPI_Cut.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepTools_History.hxx>
#include <BRep_Tool.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopAbs_ShapeEnum.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopTools_ListOfShape.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <utility>
#include <variant>
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

struct TempDirectory final {
    std::filesystem::path path;

    TempDirectory() {
        path =
            std::filesystem::temp_directory_path() /
            ("simplesolid2_pm00a_e03_e04_" +
             std::to_string(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch()
                     .count()));
        std::filesystem::create_directories(path);
    }

    ~TempDirectory() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }
};

struct FixtureIds final {
    std::optional<sketch::SketchId> sketch_id;
    std::vector<sketch::EntityId> edges;
    std::optional<part::ProfileId> profile_id;
};

struct ProviderHistorySummary final {
    std::size_t modified_edge_count{};
    std::size_t generated_shape_count{};
    bool deleted{false};

    friend bool operator==(
        const ProviderHistorySummary&,
        const ProviderHistorySummary&) = default;
};

struct CardinalityCandidate final {
    std::vector<kernel::BoundaryUseProvenance>
        source_provenance;
    bool semantic_role_match{true};
};

struct CardinalityResult final {
    kernel::ReferenceStatus status{
        kernel::ReferenceStatus::unsupported};
    std::size_t physical_candidate_count{};
    std::size_t semantic_candidate_count{};
    std::size_t merged_source_count{};
    bool aggregate_requested{false};

    friend bool operator==(
        const CardinalityResult&,
        const CardinalityResult&) = default;
};

struct MatrixSnapshot final {
    ProviderHistorySummary e03_split_history;
    ProviderHistorySummary e03_partial_history;
    ProviderHistorySummary e03_deleted_history;

    CardinalityResult e03_split;
    CardinalityResult e03_semantic_plus_technical;
    CardinalityResult e03_deleted;
    CardinalityResult e03_aggregate;

    ProviderHistorySummary e04_first_history;
    ProviderHistorySummary e04_second_history;
    std::size_t e04_merged_bottom_edge_count{};
    bool e04_modified_deleted_pattern{false};
    std::string e04_preserved_source;

    CardinalityResult e04_lost_first;
    CardinalityResult e04_lost_second;
    CardinalityResult e04_bookkeeping_first;
    CardinalityResult e04_bookkeeping_second;
    CardinalityResult e04_preserved;
    CardinalityResult e04_removed;
    CardinalityResult e04_aggregate;

    friend bool operator==(
        const MatrixSnapshot&,
        const MatrixSnapshot&) = default;
};

[[nodiscard]] part::ProfileRegionIntent
currentRegionIntent(
    const part::PartDocument& document,
    sketch::SketchId sketch_id) {
    const auto* source_sketch =
        document.findSketch(sketch_id);
    CHECK(source_sketch != nullptr);

    const auto analysis =
        sketch::analyzeRegions(
            source_sketch->model);
    CHECK(analysis.complete());

    const auto pick =
        sketch::pickRegion(
            source_sketch->model,
            analysis,
            {10.0, 10.0});
    CHECK(pick.region_index.has_value());

    const auto region =
        std::find_if(
            analysis.regions.begin(),
            analysis.regions.end(),
            [&pick](
                const sketch::RegionCandidate2D&
                    candidate) {
                return candidate.region_index ==
                       *pick.region_index;
            });
    CHECK(region != analysis.regions.end());

    const auto intent =
        part::makeProfileRegionIntent(
            *region);
    CHECK(intent.has_value());
    return *intent;
}

FixtureIds createSplitBottomProfile(
    application::DocumentSession& session) {
    FixtureIds ids;

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id.has_value());
    ids.sketch_id =
        *sketch_created.sketch_id;

    const std::vector<
        std::pair<sketch::Point2, sketch::Point2>>
        edges{
            {{0.0, 0.0}, {20.0, 0.0}},
            {{20.0, 0.0}, {40.0, 0.0}},
            {{40.0, 0.0}, {40.0, 30.0}},
            {{40.0, 30.0}, {0.0, 30.0}},
            {{0.0, 30.0}, {0.0, 0.0}},
        };

    for (const auto& [start, end] : edges) {
        const auto added =
            session.execute(
                application::AddSketchLineCommand{
                    *ids.sketch_id,
                    start,
                    end,
                    sketch::EntityRole::regular});
        CHECK(
            added.ok() &&
            added.entity_id.has_value());
        ids.edges.push_back(
            *added.entity_id);
    }
    CHECK(ids.edges.size() == 5U);

    const auto intent =
        currentRegionIntent(
            session.document(),
            *ids.sketch_id);
    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *ids.sketch_id,
                session.document().revision(),
                intent});
    CHECK(
        profile.ok() &&
        profile.profile_id.has_value());
    ids.profile_id =
        *profile.profile_id;

    return ids;
}

[[nodiscard]] kernel::BoundaryUseProvenance
provenanceFor(
    const kernel::PlanarProfileInput& input,
    sketch::EntityId source) {
    const auto serialized =
        source.serialized();

    for (const auto& use : input.outer.boundary) {
        if (use.provenance.source_entity ==
            serialized) {
            return use.provenance;
        }
    }

    for (const auto& hole : input.holes) {
        for (const auto& use : hole.boundary) {
            if (use.provenance.source_entity ==
                serialized) {
                return use.provenance;
            }
        }
    }

    CHECK(false);
    return {};
}

[[nodiscard]] bool samePoint(
    const gp_Pnt& first,
    const gp_Pnt& second) noexcept {
    constexpr double tolerance = 1.0e-7;
    return first.Distance(second) <= tolerance;
}

[[nodiscard]] bool edgeHasEndpoints(
    const TopoDS_Edge& edge,
    const gp_Pnt& first,
    const gp_Pnt& second) {
    TopoDS_Vertex start;
    TopoDS_Vertex end;
    TopExp::Vertices(
        edge,
        start,
        end);
    if (start.IsNull() || end.IsNull()) {
        return false;
    }

    const auto p0 =
        BRep_Tool::Pnt(start);
    const auto p1 =
        BRep_Tool::Pnt(end);
    return (
        samePoint(p0, first) &&
        samePoint(p1, second)) ||
        (samePoint(p0, second) &&
         samePoint(p1, first));
}

[[nodiscard]] std::vector<TopoDS_Edge>
edgesByEndpoints(
    const TopoDS_Shape& shape,
    const gp_Pnt& first,
    const gp_Pnt& second) {
    std::vector<TopoDS_Edge> result;
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto edge =
            TopoDS::Edge(
                explorer.Current());
        if (edgeHasEndpoints(
                edge,
                first,
                second)) {
            result.push_back(edge);
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
        [&candidate](const auto& edge) {
            return edge.IsSame(candidate);
        });
}

[[nodiscard]] std::size_t countShapes(
    const TopTools_ListOfShape& shapes) {
    std::size_t count = 0U;
    for (TopTools_ListOfShape::Iterator it{
             shapes};
         it.More();
         it.Next()) {
        ++count;
    }
    return count;
}

[[nodiscard]] std::vector<TopoDS_Edge>
historyEdges(
    const TopTools_ListOfShape& shapes) {
    std::vector<TopoDS_Edge> result;
    for (TopTools_ListOfShape::Iterator it{
             shapes};
         it.More();
         it.Next()) {
        const auto& shape = it.Value();
        if (shape.ShapeType() !=
            TopAbs_EDGE) {
            continue;
        }

        const auto edge =
            TopoDS::Edge(shape);
        if (!containsSameEdge(result, edge)) {
            result.push_back(edge);
        }
    }
    return result;
}

[[nodiscard]] ProviderHistorySummary
cutHistory(
    const BRepAlgoAPI_Cut& cut,
    const TopoDS_Edge& source) {
    return {
        historyEdges(
            cut.Modified(source))
            .size(),
        countShapes(
            cut.Generated(source)),
        cut.IsDeleted(source),
    };
}

[[nodiscard]] std::optional<TopoDS_Edge>
firstTechnicalResultEdge(
    const TopoDS_Shape& shape,
    const std::vector<TopoDS_Edge>&
        semantic_descendants) {
    for (TopExp_Explorer explorer{
             shape,
             TopAbs_EDGE};
         explorer.More();
         explorer.Next()) {
        const auto candidate =
            TopoDS::Edge(
                explorer.Current());
        if (!containsSameEdge(
                semantic_descendants,
                candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

[[nodiscard]] bool containsProvenance(
    const CardinalityCandidate& candidate,
    const kernel::BoundaryUseProvenance&
        target) {
    return std::any_of(
        candidate.source_provenance.begin(),
        candidate.source_provenance.end(),
        [&target](const auto& provenance) {
            return provenance == target;
        });
}

[[nodiscard]] CardinalityResult classify(
    const kernel::BoundaryUseProvenance&
        target,
    const std::vector<CardinalityCandidate>&
        candidates,
    bool aggregate_requested = false) {
    CardinalityResult result;
    result.physical_candidate_count =
        candidates.size();
    result.aggregate_requested =
        aggregate_requested;

    for (const auto& candidate : candidates) {
        if (!candidate.semantic_role_match ||
            !containsProvenance(
                candidate,
                target)) {
            continue;
        }

        ++result.semantic_candidate_count;
        result.merged_source_count =
            std::max(
                result.merged_source_count,
                candidate.source_provenance
                    .size());
    }

    if (aggregate_requested) {
        result.status =
            kernel::ReferenceStatus::unsupported;
    } else if (
        result.semantic_candidate_count == 0U) {
        result.status =
            kernel::ReferenceStatus::missing;
    } else if (
        result.semantic_candidate_count != 1U ||
        result.merged_source_count != 1U) {
        result.status =
            kernel::ReferenceStatus::ambiguous;
    } else {
        result.status =
            kernel::ReferenceStatus::resolved;
    }

    return result;
}

struct ProfileFaceFixture final {
    TopoDS_Face face;
    std::vector<
        std::pair<
            kernel::BoundaryUseProvenance,
            TopoDS_Edge>>
        edges;
};

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

[[nodiscard]] ProfileFaceFixture
buildProfileFaceFixture(
    const kernel::PlanarProfileInput& input) {
    CHECK(input.holes.empty());

    BRepBuilderAPI_MakeWire make_wire;
    ProfileFaceFixture fixture;
    fixture.edges.reserve(
        input.outer.boundary.size());

    for (const auto& use :
         input.outer.boundary) {
        const auto* line =
            std::get_if<kernel::Line2>(
                &use.curve);
        CHECK(line != nullptr);
        CHECK(!use.whole_closed_curve);

        const double from =
            use.follows_source_direction
                ? use.start_parameter
                : use.end_parameter;
        const double to =
            use.follows_source_direction
                ? use.end_parameter
                : use.start_parameter;

        BRepBuilderAPI_MakeEdge make_edge{
            point3(
                input.frame,
                linePoint(*line, from)),
            point3(
                input.frame,
                linePoint(*line, to))};
        CHECK(make_edge.IsDone());

        make_wire.Add(
            make_edge.Edge());
        CHECK(make_wire.IsDone());

        fixture.edges.push_back({
            use.provenance,
            make_wire.Edge(),
        });
    }

    BRepBuilderAPI_MakeFace make_face{
        make_wire.Wire(),
        true};
    CHECK(make_face.IsDone());

    fixture.face =
        make_face.Face();
    CHECK(
        BRepCheck_Analyzer{
            fixture.face}
            .IsValid());
    return fixture;
}

[[nodiscard]] TopoDS_Edge fixtureEdge(
    const ProfileFaceFixture& fixture,
    const kernel::BoundaryUseProvenance&
        provenance) {
    const auto found =
        std::find_if(
            fixture.edges.begin(),
            fixture.edges.end(),
            [&provenance](const auto& item) {
                return item.first ==
                       provenance;
            });
    CHECK(found != fixture.edges.end());
    return found->second;
}

[[nodiscard]] ProviderHistorySummary
unifyHistory(
    const Handle(BRepTools_History)& history,
    const TopoDS_Edge& source) {
    CHECK(!history.IsNull());
    return {
        historyEdges(
            history->Modified(source))
            .size(),
        0U,
        history->IsRemoved(source),
    };
}

[[nodiscard]] MatrixSnapshot evaluateMatrix(
    const part::PartDocument& document,
    const FixtureIds& ids) {
    CHECK(ids.profile_id.has_value());
    CHECK(ids.edges.size() == 5U);

    const auto input =
        part::makeKernelProfileInput(
            document,
            *ids.profile_id);
    CHECK(input.has_value());
    CHECK(input->valid());
    CHECK(input->holes.empty());

    const auto first =
        provenanceFor(
            *input,
            ids.edges[0]);
    const auto second =
        provenanceFor(
            *input,
            ids.edges[1]);
    const auto split_target =
        provenanceFor(
            *input,
            ids.edges[3]);
    CHECK(first != second);
    CHECK(split_target != first);
    CHECK(split_target != second);

    MatrixSnapshot result;

    // E03: fixed box is aligned with the authored 40x30 profile. The semantic
    // target is the top Profile Line, represented here by the corresponding
    // top/back edge at z=10. Fixture selection happens before mutation and is
    // not used as post-operation identity.
    const TopoDS_Shape base =
        BRepPrimAPI_MakeBox(
            gp_Pnt{0.0, 0.0, 0.0},
            40.0,
            30.0,
            10.0)
            .Shape();
    CHECK(
        BRepCheck_Analyzer{base}.IsValid());

    const auto source_edges =
        edgesByEndpoints(
            base,
            gp_Pnt{0.0, 30.0, 10.0},
            gp_Pnt{40.0, 30.0, 10.0});
    CHECK(source_edges.size() == 1U);
    const auto source_edge =
        source_edges.front();

    // E03-01: a middle notch removes the centre of one source edge and OCCT
    // must report two Modified descendants. A singular semantic reference
    // therefore becomes Ambiguous; no first/longest/nearest fragment wins.
    BRepAlgoAPI_Cut split_cut{
        base,
        BRepPrimAPI_MakeBox(
            gp_Pnt{15.0, 25.0, 5.0},
            10.0,
            10.0,
            10.0)
            .Shape()};
    CHECK(split_cut.IsDone());
    CHECK(
        BRepCheck_Analyzer{
            split_cut.Shape()}
            .IsValid());

    const auto split_descendants =
        historyEdges(
            split_cut.Modified(
                source_edge));
    result.e03_split_history =
        cutHistory(
            split_cut,
            source_edge);

    std::cerr
        << "E03_SPLIT_HISTORY modified_edges="
        << result.e03_split_history
               .modified_edge_count
        << " generated_shapes="
        << result.e03_split_history
               .generated_shape_count
        << " deleted="
        << result.e03_split_history.deleted
        << '\n';

    CHECK(split_descendants.size() == 2U);
    CHECK(!result.e03_split_history.deleted);

    std::vector<CardinalityCandidate>
        split_candidates;
    for (const auto& edge :
         split_descendants) {
        (void)edge;
        split_candidates.push_back({
            {split_target},
            true,
        });
    }

    result.e03_split =
        classify(
            split_target,
            split_candidates);
    CHECK(
        result.e03_split.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e03_split
            .semantic_candidate_count ==
        2U);

    // E03-02: cut one end of the same source edge. The one Modified
    // descendant remains semantic. A second physical result edge is admitted
    // as a broad technical candidate but has no source-lineage role and is
    // excluded without using geometry ranking.
    BRepAlgoAPI_Cut partial_cut{
        base,
        BRepPrimAPI_MakeBox(
            gp_Pnt{20.0, 25.0, 5.0},
            25.0,
            10.0,
            10.0)
            .Shape()};
    CHECK(partial_cut.IsDone());
    CHECK(
        BRepCheck_Analyzer{
            partial_cut.Shape()}
            .IsValid());

    const auto partial_descendants =
        historyEdges(
            partial_cut.Modified(
                source_edge));
    result.e03_partial_history =
        cutHistory(
            partial_cut,
            source_edge);

    std::cerr
        << "E03_PARTIAL_HISTORY modified_edges="
        << result.e03_partial_history
               .modified_edge_count
        << " generated_shapes="
        << result.e03_partial_history
               .generated_shape_count
        << " deleted="
        << result.e03_partial_history.deleted
        << '\n';

    CHECK(partial_descendants.size() == 1U);
    CHECK(!result.e03_partial_history.deleted);

    const auto technical =
        firstTechnicalResultEdge(
            partial_cut.Shape(),
            partial_descendants);
    CHECK(technical.has_value());

    std::vector<CardinalityCandidate>
        partial_candidates{
            {
                {split_target},
                true,
            },
            {
                {},
                false,
            },
        };
    result.e03_semantic_plus_technical =
        classify(
            split_target,
            partial_candidates);
    CHECK(
        result.e03_semantic_plus_technical
            .status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        result.e03_semantic_plus_technical
            .physical_candidate_count ==
        2U);
    CHECK(
        result.e03_semantic_plus_technical
            .semantic_candidate_count ==
        1U);

    // E03-03: remove the whole corresponding strip. The source edge has no
    // Modified descendant and is reported deleted.
    BRepAlgoAPI_Cut deleted_cut{
        base,
        BRepPrimAPI_MakeBox(
            gp_Pnt{-5.0, 25.0, 5.0},
            50.0,
            10.0,
            10.0)
            .Shape()};
    CHECK(deleted_cut.IsDone());
    CHECK(
        BRepCheck_Analyzer{
            deleted_cut.Shape()}
            .IsValid());

    const auto deleted_descendants =
        historyEdges(
            deleted_cut.Modified(
                source_edge));
    result.e03_deleted_history =
        cutHistory(
            deleted_cut,
            source_edge);

    std::cerr
        << "E03_DELETED_HISTORY modified_edges="
        << result.e03_deleted_history
               .modified_edge_count
        << " generated_shapes="
        << result.e03_deleted_history
               .generated_shape_count
        << " deleted="
        << result.e03_deleted_history.deleted
        << '\n';

    CHECK(deleted_descendants.empty());
    CHECK(result.e03_deleted_history.deleted);

    result.e03_deleted =
        classify(
            split_target,
            {});
    CHECK(
        result.e03_deleted.status ==
        kernel::ReferenceStatus::missing);

    // E03-04: no undeclared aggregate/set reference type exists in PM-00A.
    result.e03_aggregate =
        classify(
            split_target,
            split_candidates,
            true);
    CHECK(
        result.e03_aggregate.status ==
        kernel::ReferenceStatus::unsupported);

    // E04: construct the authored split-bottom Profile as a real OCCT face,
    // preserving the exact transient edge accepted by MakeWire for each
    // Profile boundary use.
    const auto face_fixture =
        buildProfileFaceFixture(*input);
    const auto first_edge =
        fixtureEdge(
            face_fixture,
            first);
    const auto second_edge =
        fixtureEdge(
            face_fixture,
            second);

    ShapeUpgrade_UnifySameDomain unify{
        face_fixture.face,
        true,
        false,
        false};
    unify.Build();
    const auto unified =
        unify.Shape();
    CHECK(!unified.IsNull());
    CHECK(
        BRepCheck_Analyzer{unified}.IsValid());

    const auto merged_bottom =
        edgesByEndpoints(
            unified,
            gp_Pnt{0.0, 0.0, 0.0},
            gp_Pnt{40.0, 0.0, 0.0});
    result.e04_merged_bottom_edge_count =
        merged_bottom.size();
    CHECK(
        result.e04_merged_bottom_edge_count ==
        1U);

    const auto history =
        unify.History();
    CHECK(!history.IsNull());

    result.e04_first_history =
        unifyHistory(
            history,
            first_edge);
    result.e04_second_history =
        unifyHistory(
            history,
            second_edge);

    std::cerr
        << "E04_UNIFY_HISTORY first_modified="
        << result.e04_first_history
               .modified_edge_count
        << " first_removed="
        << result.e04_first_history.deleted
        << " second_modified="
        << result.e04_second_history
               .modified_edge_count
        << " second_removed="
        << result.e04_second_history.deleted
        << '\n';

    const std::vector<CardinalityCandidate>
        merged_candidate{
            {
                {first, second},
                true,
            },
        };

    // E04-01: one physical edge carries two old semantic meanings; the lost
    // distinction makes each singular reference Ambiguous.
    result.e04_lost_first =
        classify(
            first,
            merged_candidate);
    result.e04_lost_second =
        classify(
            second,
            merged_candidate);
    CHECK(
        result.e04_lost_first.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e04_lost_second.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e04_lost_first
            .merged_source_count ==
        2U);

    // E04-02: require the concrete provider history case where one old edge
    // is represented as Modified and the other as Removed. This bookkeeping
    // is recorded, but the semantic merged candidate still makes both
    // singular meanings Ambiguous.
    const bool first_modified_second_removed =
        result.e04_first_history
                .modified_edge_count >
            0U &&
        !result.e04_first_history.deleted &&
        result.e04_second_history.deleted;
    const bool second_modified_first_removed =
        result.e04_second_history
                .modified_edge_count >
            0U &&
        !result.e04_second_history.deleted &&
        result.e04_first_history.deleted;

    result.e04_modified_deleted_pattern =
        first_modified_second_removed ||
        second_modified_first_removed;
    CHECK(
        result.e04_modified_deleted_pattern);

    result.e04_bookkeeping_first =
        classify(
            first,
            merged_candidate);
    result.e04_bookkeeping_second =
        classify(
            second,
            merged_candidate);
    CHECK(
        result.e04_bookkeeping_first.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e04_bookkeeping_second.status ==
        kernel::ReferenceStatus::ambiguous);

    // E04-03: independent semantic evidence may preserve exactly one meaning.
    // Use the source that provider bookkeeping kept Modified only as fixture
    // selection; the classification itself is driven by the declared
    // semantic provenance set, not by Modified/Removed status.
    const auto& preserved =
        first_modified_second_removed
            ? first
            : second;
    const auto& removed =
        first_modified_second_removed
            ? second
            : first;
    result.e04_preserved_source =
        preserved.source_entity;

    const std::vector<CardinalityCandidate>
        preserved_candidate{
            {
                {preserved},
                true,
            },
        };
    result.e04_preserved =
        classify(
            preserved,
            preserved_candidate);
    result.e04_removed =
        classify(
            removed,
            preserved_candidate);
    CHECK(
        result.e04_preserved.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        result.e04_removed.status ==
        kernel::ReferenceStatus::missing);

    // E04-04: no undeclared merged aggregate meaning is invented.
    result.e04_aggregate =
        classify(
            first,
            merged_candidate,
            true);
    CHECK(
        result.e04_aggregate.status ==
        kernel::ReferenceStatus::unsupported);

    return result;
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path /
        "PM00AE03E04.ss2part";

    part::PartDocumentStore store;
    FixtureIds ids;
    MatrixSnapshot before;

    {
        auto document =
            part::PartDocument::create(
                core::DocumentId::generate());
        const auto published =
            store.createNew(path, document);
        CHECK(published.ok());

        application::DocumentSession session{
            path,
            std::move(document),
            *published.checkpoint};

        ids =
            createSplitBottomProfile(
                session);
        CHECK(ids.profile_id.has_value());

        before =
            evaluateMatrix(
                session.document(),
                ids);

        CHECK(session.save().ok());
    }

    // COLD replay: previous DocumentSession, neutral Profile input and all
    // OCCT Boolean/Unify objects are gone. Rebuild from native authored state
    // and require identical provider observations and semantic statuses.
    const auto loaded =
        store.load(path);
    CHECK(loaded.ok());

    const auto cold =
        evaluateMatrix(
            *loaded.document,
            ids);
    CHECK(cold == before);

    std::cout
        << "PM00A_E03_E04_PASS "
        << "rows=E03-01..E03-04,E04-01..E04-04 "
        << "false_resolved=0\n";
    return EXIT_SUCCESS;
}
