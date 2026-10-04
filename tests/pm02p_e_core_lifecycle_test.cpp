#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/project_session.hpp>
#include <simplesolid2/application/project_workspace_metadata.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-02P.E core lifecycle CHECK failed at line "
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
            ("simplesolid2_pm02p_e_" +
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

enum class EvidenceTopologyKind {
    face,
    edge,
    vertex,
};

struct RuntimeTopologyTicket final {
    core::DocumentId document_id;
    core::DocumentRevision revision;
    std::uint64_t session_generation{};
    std::uint64_t evaluation_generation{};
    std::uint64_t provider_generation{};
    EvidenceTopologyKind kind{
        EvidenceTopologyKind::face};
    std::uint64_t runtime_token{};
};

enum class FreshnessOutcome {
    accepted,
    stale_revision,
    stale_session,
    stale_evaluation_generation,
    stale_provider_generation,
};

class TopologyFreshnessGate final {
public:
    [[nodiscard]] bool bindCanonical(
        const application::ProjectSession& project,
        const core::DocumentId& document_id) {
        const auto* session =
            project.documentSession(document_id);
        if (session == nullptr ||
            session->documentId() != document_id) {
            return false;
        }

        document_id_ = document_id;
        current_session_ = session;
        ++session_generation_;
        evaluation_generation_ = 1U;
        provider_generation_ = 1U;
        return true;
    }

    [[nodiscard]] std::optional<RuntimeTopologyTicket>
    begin(
        const application::ProjectSession& project,
        const core::DocumentId& document_id,
        EvidenceTopologyKind kind,
        std::uint64_t runtime_token) const {
        const auto* session =
            project.documentSession(document_id);
        if (!document_id_.has_value() ||
            *document_id_ != document_id ||
            session == nullptr ||
            session != current_session_ ||
            runtime_token == 0U) {
            return std::nullopt;
        }

        return RuntimeTopologyTicket{
            document_id,
            session->document().revision(),
            session_generation_,
            evaluation_generation_,
            provider_generation_,
            kind,
            runtime_token,
        };
    }

    void advanceEvaluationGeneration() noexcept {
        ++evaluation_generation_;
    }

    void replaceProviderRuntime() noexcept {
        ++provider_generation_;
    }

    [[nodiscard]] FreshnessOutcome validate(
        const application::ProjectSession& project,
        const RuntimeTopologyTicket& ticket) const {
        const auto* session =
            project.documentSession(
                ticket.document_id);

        if (!document_id_.has_value() ||
            *document_id_ != ticket.document_id ||
            session == nullptr ||
            session != current_session_ ||
            ticket.session_generation !=
                session_generation_) {
            return FreshnessOutcome::stale_session;
        }

        if (ticket.revision !=
            session->document().revision()) {
            return FreshnessOutcome::stale_revision;
        }

        if (ticket.evaluation_generation !=
            evaluation_generation_) {
            return FreshnessOutcome::
                stale_evaluation_generation;
        }

        if (ticket.provider_generation !=
            provider_generation_) {
            return FreshnessOutcome::
                stale_provider_generation;
        }

        return FreshnessOutcome::accepted;
    }

private:
    std::optional<core::DocumentId> document_id_;
    const application::DocumentSession*
        current_session_{};
    std::uint64_t session_generation_{};
    std::uint64_t evaluation_generation_{};
    std::uint64_t provider_generation_{};
};

void verifyTopologyFreshness(
    const std::filesystem::path& project_path) {
    std::filesystem::create_directories(project_path);

    application::ProjectWorkspaceMetadataService
        metadata_service;
    const auto initialized =
        metadata_service.initialize(
            project_path,
            "PM-02P E22");
    CHECK(initialized.ok());

    auto opened =
        application::ProjectSession::open(
            project_path);
    CHECK(opened.ok());

    auto& project =
        *opened.session;
    const auto created =
        project.createPart(
            "TopologyFreshness.ss2part");
    CHECK(created.ok());
    CHECK(created.session != nullptr);

    auto* session_a =
        created.session;
    const auto document_id =
        session_a->documentId();

    TopologyFreshnessGate gate;
    CHECK(
        gate.bindCanonical(
            project,
            document_id));

    // Current Face/Edge/Vertex evidence is admissible in the exact current
    // revision/session/evaluation/provider generation.
    for (const auto kind : {
             EvidenceTopologyKind::face,
             EvidenceTopologyKind::edge,
             EvidenceTopologyKind::vertex}) {
        const auto ticket =
            gate.begin(
                project,
                document_id,
                kind,
                7U);
        CHECK(ticket.has_value());
        CHECK(
            gate.validate(
                project,
                *ticket) ==
            FreshnessOutcome::accepted);
    }

    // DocumentRevision invalidates all runtime topology evidence.
    const auto stale_revision =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::face,
            11U);
    CHECK(stale_revision.has_value());

    auto properties =
        session_a->document().properties();
    properties.title =
        "Advance revision for PM-02P E22";
    const auto changed =
        session_a->execute(
            application::SetDocumentPropertiesCommand{
                properties});
    CHECK(changed.ok());
    CHECK(changed.changed);
    CHECK(
        gate.validate(
            project,
            *stale_revision) ==
        FreshnessOutcome::stale_revision);

    // Evaluation-generation replacement invalidates a token even when its
    // numeric token value is reused.
    const auto stale_evaluation =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::edge,
            23U);
    CHECK(stale_evaluation.has_value());

    gate.advanceEvaluationGeneration();
    CHECK(
        gate.validate(
            project,
            *stale_evaluation) ==
        FreshnessOutcome::
            stale_evaluation_generation);

    const auto current_same_edge_token =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::edge,
            23U);
    CHECK(current_same_edge_token.has_value());
    CHECK(
        gate.validate(
            project,
            *current_same_edge_token) ==
        FreshnessOutcome::accepted);

    // Provider teardown/replacement is a separate authority boundary. Token
    // number reuse does not revive the old Vertex identity.
    const auto stale_provider =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::vertex,
            31U);
    CHECK(stale_provider.has_value());

    gate.replaceProviderRuntime();
    CHECK(
        gate.validate(
            project,
            *stale_provider) ==
        FreshnessOutcome::
            stale_provider_generation);

    const auto current_same_vertex_token =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::vertex,
            31U);
    CHECK(current_same_vertex_token.has_value());
    CHECK(
        gate.validate(
            project,
            *current_same_vertex_token) ==
        FreshnessOutcome::accepted);

    CHECK(session_a->save().ok());

    // Canonical Session A is replaced by Session B for the same durable
    // DocumentId. Runtime evidence from A cannot authorize B.
    const auto stale_session =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::face,
            41U);
    CHECK(stale_session.has_value());

    CHECK(
        project.closeDocument(
            document_id,
            false));
    const auto reopened =
        project.openDocument(
            document_id);
    CHECK(reopened.ok());
    CHECK(reopened.session != nullptr);
    CHECK(!reopened.reused_session);

    CHECK(
        gate.bindCanonical(
            project,
            document_id));
    CHECK(
        gate.validate(
            project,
            *stale_session) ==
        FreshnessOutcome::stale_session);

    const auto session_b_current =
        gate.begin(
            project,
            document_id,
            EvidenceTopologyKind::face,
            41U);
    CHECK(session_b_current.has_value());
    CHECK(
        gate.validate(
            project,
            *session_b_current) ==
        FreshnessOutcome::accepted);
}

class CycleSolid final
    : public kernel::RuntimeSolid {};

class CycleKernel final
    : public kernel::ISolidModelingKernel {
public:
    [[nodiscard]] kernel::SolidModelingResult
    extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        kernel::SolidModelingResult result;

        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::missing_upstream;
            return result;
        }
        if (upstream != nullptr &&
            dynamic_cast<const CycleSolid*>(
                upstream.get()) == nullptr) {
            result.status =
                kernel::SolidModelingStatus::provider_mismatch;
            return result;
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<CycleSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }
};

struct ProfileFixture final {
    sketch::SketchId sketch_id;
    part::ProfileId profile_id;
};

ProfileFixture createRectangleProfile(
    application::DocumentSession& session,
    double x0,
    double y0,
    double x1,
    double y1) {
    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id.has_value());

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {x0, y0},
                {x1, y1},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.changed);

    const auto* sketch =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(sketch != nullptr);

    const auto analysis =
        sketch::analyzeRegions(
            sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);

    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent.has_value());

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id.has_value());

    return {
        *sketch_created.sketch_id,
        *profile.profile_id};
}

enum class ProspectiveSupportDecision {
    admissible,
    cycle_dependency,
    missing_sketch,
    missing_support_stage,
};

ProspectiveSupportDecision
prospectiveSurfaceSupportDecision(
    const part::PartDocument& document,
    sketch::SketchId sketch_id,
    part::FeatureId support_producer) {
    if (document.findSketch(sketch_id) ==
        nullptr) {
        return ProspectiveSupportDecision::
            missing_sketch;
    }

    const auto& features =
        document.body().features;
    const auto support =
        std::find_if(
            features.begin(),
            features.end(),
            [support_producer](
                const part::PartFeature& feature) {
                return feature.id ==
                       support_producer;
            });
    if (support == features.end()) {
        return ProspectiveSupportDecision::
            missing_support_stage;
    }

    const std::size_t support_index =
        static_cast<std::size_t>(
            std::distance(
                features.begin(),
                support));

    std::set<part::ProfileId>
        profiles_from_sketch;
    for (const auto& profile :
         document.profiles()) {
        if (profile.source_sketch_id ==
            sketch_id) {
            profiles_from_sketch.insert(
                profile.id);
        }
    }

    for (std::size_t index = 0U;
         index < features.size();
         ++index) {
        const auto consumed =
            part::sourceProfileId(
                features[index]);
        if (!consumed ||
            !profiles_from_sketch.contains(
                *consumed)) {
            continue;
        }

        // Support produced by Feature N becomes available only after N.
        // A consuming Feature must therefore be strictly downstream.
        if (support_index >= index) {
            return ProspectiveSupportDecision::
                cycle_dependency;
        }
    }

    return ProspectiveSupportDecision::
        admissible;
}

void verifyBoundedCycleRejection(
    const std::filesystem::path& part_path) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        part_path,
        std::move(document)};

    const auto first =
        createRectangleProfile(
            session,
            0.0,
            0.0,
            40.0,
            30.0);
    const auto second =
        createRectangleProfile(
            session,
            5.0,
            5.0,
            20.0,
            15.0);

    CycleKernel kernel;

    const auto first_feature =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                first.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Feature 1"},
            kernel);
    CHECK(
        first_feature.ok() &&
        first_feature.feature_id.has_value());

    const auto second_feature =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                second.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{5.0},
                    false},
                "Feature 2"},
            kernel);
    CHECK(
        second_feature.ok() &&
        second_feature.feature_id.has_value());

    const auto revision_before =
        session.document().revision();
    const auto state_before =
        session.document().state();

    // Sketch 1 is consumed by Feature 1. Supporting it on Feature 1 itself
    // or on Feature 2 would create a self/forward dependency.
    CHECK(
        prospectiveSurfaceSupportDecision(
            session.document(),
            first.sketch_id,
            *first_feature.feature_id) ==
        ProspectiveSupportDecision::
            cycle_dependency);
    CHECK(
        prospectiveSurfaceSupportDecision(
            session.document(),
            first.sketch_id,
            *second_feature.feature_id) ==
        ProspectiveSupportDecision::
            cycle_dependency);

    // Sketch 2 is consumed only by Feature 2. A Surface produced by Feature 1
    // is upstream and therefore admissible; Feature 2 itself is not.
    CHECK(
        prospectiveSurfaceSupportDecision(
            session.document(),
            second.sketch_id,
            *first_feature.feature_id) ==
        ProspectiveSupportDecision::
            admissible);
    CHECK(
        prospectiveSurfaceSupportDecision(
            session.document(),
            second.sketch_id,
            *second_feature.feature_id) ==
        ProspectiveSupportDecision::
            cycle_dependency);

    // Preflight is pure evidence. Rejected support attempts perform no Part
    // mutation, and no general dependency graph is introduced.
    CHECK(
        session.document().revision() ==
        revision_before);
    CHECK(
        session.document().state() ==
        state_before);
}

} // namespace

int main() {
    TempDirectory temp;

    verifyTopologyFreshness(
        temp.path / "Project");
    verifyBoundedCycleRejection(
        temp.path / "CycleEvidence.ss2part");

    std::cout
        << "PM02P_E_CORE_LIFECYCLE_PASS"
        << " stale_topology_acceptance=0"
        << " token_reuse_authority=0"
        << " cycle_mutations=0\n";

    return EXIT_SUCCESS;
}
