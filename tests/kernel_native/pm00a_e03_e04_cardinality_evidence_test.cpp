#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/evidence.hpp>
#include <simplesolid2/kernel_occt/profile_face_evidence.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/part/profile_kernel_input.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
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

[[nodiscard]] bool hasSourceHistory(
    const kernel::CardinalityFixtureEvidence&
        fixture,
    const kernel::BoundaryUseProvenance&
        provenance,
    kernel::ProviderLineageObservation
        observation) {
    return std::any_of(
        fixture.source_history.begin(),
        fixture.source_history.end(),
        [&](const auto& item) {
            return item.provenance ==
                       provenance &&
                   item.observation ==
                       observation;
        });
}

struct MatrixSnapshot final {
    kernel::CardinalityFixtureEvidence
        e03_split;
    kernel::CardinalityFixtureEvidence
        e03_semantic_plus_technical;
    kernel::CardinalityFixtureEvidence
        e03_deleted;

    kernel::SingularReferenceCardinalityEvidence
        e03_split_status;
    kernel::SingularReferenceCardinalityEvidence
        e03_technical_status;
    kernel::SingularReferenceCardinalityEvidence
        e03_deleted_status;
    kernel::SingularReferenceCardinalityEvidence
        e03_aggregate_status;

    kernel::CardinalityFixtureEvidence
        e04_lost_distinction;
    kernel::CardinalityFixtureEvidence
        e04_modified_deleted;
    kernel::CardinalityFixtureEvidence
        e04_preserved_first;

    kernel::SingularReferenceCardinalityEvidence
        e04_lost_first_status;
    kernel::SingularReferenceCardinalityEvidence
        e04_lost_second_status;
    kernel::SingularReferenceCardinalityEvidence
        e04_bookkeeping_first_status;
    kernel::SingularReferenceCardinalityEvidence
        e04_bookkeeping_second_status;
    kernel::SingularReferenceCardinalityEvidence
        e04_preserved_first_status;
    kernel::SingularReferenceCardinalityEvidence
        e04_removed_second_status;
    kernel::SingularReferenceCardinalityEvidence
        e04_aggregate_status;

    friend bool operator==(
        const MatrixSnapshot&,
        const MatrixSnapshot&) = default;
};

[[nodiscard]] MatrixSnapshot evaluateMatrix(
    const part::PartDocument& document,
    const FixtureIds& ids) {
    CHECK(ids.profile_id.has_value());
    CHECK(ids.edges.size() >= 2U);

    const auto input =
        part::makeKernelProfileInput(
            document,
            *ids.profile_id);
    CHECK(input.has_value());
    CHECK(input->valid());

    const auto first =
        provenanceFor(
            *input,
            ids.edges[0]);
    const auto second =
        provenanceFor(
            *input,
            ids.edges[1]);
    CHECK(first != second);

    MatrixSnapshot result;

    // E03-01: one semantic target becomes two semantic descendants.
    result.e03_split =
        kernel_occt::
            buildSplitEdgeCardinalityFixture(
                first,
                kernel_occt::
                    SplitCardinalityFixture::
                        two_semantic_descendants);
    CHECK(
        result.e03_split
            .provider_geometry_valid);
    CHECK(
        result.e03_split.source_edge_count ==
        1U);
    CHECK(
        result.e03_split
            .physical_candidate_count ==
        2U);
    CHECK(
        result.e03_split.candidates.size() ==
        2U);
    CHECK(
        hasSourceHistory(
            result.e03_split,
            first,
            kernel::
                ProviderLineageObservation::
                    modified));

    result.e03_split_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e03_split.candidates);
    CHECK(
        result.e03_split_status.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e03_split_status
            .semantic_candidate_count ==
        2U);
    CHECK(
        result.e03_split_status
            .merged_source_count ==
        1U);

    // E03-02: a technical auxiliary edge may be present physically, but it
    // is excluded by semantic role rather than by length/order/proximity.
    result.e03_semantic_plus_technical =
        kernel_occt::
            buildSplitEdgeCardinalityFixture(
                first,
                kernel_occt::
                    SplitCardinalityFixture::
                        semantic_plus_technical);
    CHECK(
        result.e03_semantic_plus_technical
            .provider_geometry_valid);
    CHECK(
        result.e03_semantic_plus_technical
            .physical_candidate_count ==
        2U);
    CHECK(
        result.e03_semantic_plus_technical
            .candidates.size() ==
        2U);
    CHECK(
        std::count_if(
            result.e03_semantic_plus_technical
                .candidates.begin(),
            result.e03_semantic_plus_technical
                .candidates.end(),
            [](const auto& candidate) {
                return candidate
                    .semantic_role_match;
            }) ==
        1);

    result.e03_technical_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e03_semantic_plus_technical
                    .candidates);
    CHECK(
        result.e03_technical_status.status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        result.e03_technical_status
            .physical_candidate_count ==
        2U);
    CHECK(
        result.e03_technical_status
            .semantic_candidate_count ==
        1U);

    // E03-03: source deleted with no semantic descendant.
    result.e03_deleted =
        kernel_occt::
            buildSplitEdgeCardinalityFixture(
                first,
                kernel_occt::
                    SplitCardinalityFixture::
                        deleted_target);
    CHECK(
        result.e03_deleted
            .provider_geometry_valid);
    CHECK(
        result.e03_deleted
            .physical_candidate_count ==
        0U);
    CHECK(
        hasSourceHistory(
            result.e03_deleted,
            first,
            kernel::
                ProviderLineageObservation::
                    deleted));

    result.e03_deleted_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e03_deleted.candidates);
    CHECK(
        result.e03_deleted_status.status ==
        kernel::ReferenceStatus::missing);

    // E03-04: singular reference must not silently become a set.
    result.e03_aggregate_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e03_split.candidates,
                true);
    CHECK(
        result.e03_aggregate_status.status ==
        kernel::ReferenceStatus::unsupported);

    // E04-01: two semantic meanings collapse onto one physical candidate.
    result.e04_lost_distinction =
        kernel_occt::
            buildMergeEdgeCardinalityFixture(
                first,
                second,
                kernel_occt::
                    MergeCardinalityFixture::
                        lost_distinction);
    CHECK(
        result.e04_lost_distinction
            .provider_geometry_valid);
    CHECK(
        result.e04_lost_distinction
            .source_edge_count ==
        2U);
    CHECK(
        result.e04_lost_distinction
            .physical_candidate_count ==
        1U);
    CHECK(
        result.e04_lost_distinction
            .candidates.size() ==
        1U);
    CHECK(
        result.e04_lost_distinction
            .candidates.front()
            .source_provenance.size() ==
        2U);

    result.e04_lost_first_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e04_lost_distinction
                    .candidates);
    result.e04_lost_second_status =
        kernel::
            classifySingularReferenceCardinality(
                second,
                result.e04_lost_distinction
                    .candidates);
    CHECK(
        result.e04_lost_first_status.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e04_lost_second_status.status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e04_lost_first_status
            .merged_source_count ==
        2U);
    CHECK(
        result.e04_lost_second_status
            .merged_source_count ==
        2U);

    // E04-02: provider bookkeeping says first=Modified and second=Deleted,
    // but both semantic meanings feed the same physical candidate. Provider
    // bookkeeping alone is not allowed to choose the first as a winner.
    result.e04_modified_deleted =
        kernel_occt::
            buildMergeEdgeCardinalityFixture(
                first,
                second,
                kernel_occt::
                    MergeCardinalityFixture::
                        modified_deleted_same_output);
    CHECK(
        result.e04_modified_deleted
            .provider_geometry_valid);
    CHECK(
        hasSourceHistory(
            result.e04_modified_deleted,
            first,
            kernel::
                ProviderLineageObservation::
                    modified));
    CHECK(
        hasSourceHistory(
            result.e04_modified_deleted,
            second,
            kernel::
                ProviderLineageObservation::
                    deleted));

    result.e04_bookkeeping_first_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e04_modified_deleted
                    .candidates);
    result.e04_bookkeeping_second_status =
        kernel::
            classifySingularReferenceCardinality(
                second,
                result.e04_modified_deleted
                    .candidates);
    CHECK(
        result.e04_bookkeeping_first_status
            .status ==
        kernel::ReferenceStatus::ambiguous);
    CHECK(
        result.e04_bookkeeping_second_status
            .status ==
        kernel::ReferenceStatus::ambiguous);

    // E04-03: independent semantic evidence preserves only the first meaning;
    // second is genuinely removed.
    result.e04_preserved_first =
        kernel_occt::
            buildMergeEdgeCardinalityFixture(
                first,
                second,
                kernel_occt::
                    MergeCardinalityFixture::
                        preserve_first_remove_second);
    CHECK(
        result.e04_preserved_first
            .provider_geometry_valid);
    CHECK(
        result.e04_preserved_first
            .physical_candidate_count ==
        1U);
    CHECK(
        result.e04_preserved_first
            .candidates.front()
            .source_provenance.size() ==
        1U);
    CHECK(
        result.e04_preserved_first
            .candidates.front()
            .source_provenance.front() ==
        first);

    result.e04_preserved_first_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e04_preserved_first
                    .candidates);
    result.e04_removed_second_status =
        kernel::
            classifySingularReferenceCardinality(
                second,
                result.e04_preserved_first
                    .candidates);
    CHECK(
        result.e04_preserved_first_status
            .status ==
        kernel::ReferenceStatus::resolved);
    CHECK(
        result.e04_removed_second_status
            .status ==
        kernel::ReferenceStatus::missing);

    // E04-04: no undeclared merged aggregate reference is invented.
    result.e04_aggregate_status =
        kernel::
            classifySingularReferenceCardinality(
                first,
                result.e04_lost_distinction
                    .candidates,
                true);
    CHECK(
        result.e04_aggregate_status.status ==
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

    // COLD replay for all E03/E04 rows: no prior DocumentSession, transient
    // Kernel input, OCCT edge fixture or provider object survives this point.
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
