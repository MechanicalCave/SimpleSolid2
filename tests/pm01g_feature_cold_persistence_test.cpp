#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <variant>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01G cold persistence CHECK failed at line "
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
            ("simplesolid2_pm01g_" +
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

class ColdSolid final
    : public kernel::RuntimeSolid {};

class ColdKernel final
    : public kernel::ISolidModelingKernel {
public:
    std::size_t calls{};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        ++calls;
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
            dynamic_cast<const ColdSolid*>(
                upstream.get()) == nullptr) {
            result.status =
                kernel::SolidModelingStatus::provider_mismatch;
            return result;
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<ColdSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }
};

part::ProfileId createRectangleProfile(
    application::DocumentSession& session,
    double x0,
    double y0,
    double x1,
    double y1) {
    const auto sketch_result =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_result.ok() &&
        sketch_result.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_result.sketch_id,
                session.document().revision(),
                {x0, y0},
                {x1, y1},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* sketch =
        session.document().findSketch(
            *sketch_result.sketch_id);
    CHECK(sketch != nullptr);
    const auto analysis =
        sketch::analyzeRegions(sketch->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_result.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);
    return *profile.profile_id;
}


void runProjection00AColdRepairEvidence() {
    TempDirectory temp;
    const auto path = temp.path / "Projection00ARepair.ss2part";
    part::PartDocumentStore store;
    const auto document_id = core::DocumentId::generate();
    const auto created_document = part::PartDocument::create(document_id);
    const auto created = store.createNew(path, created_document);
    CHECK(created.ok());

    const part::ProfileId original_profile_id = [&] {
        application::DocumentSession session{
            path, created_document, *created.checkpoint};
        const auto profile_id =
            createRectangleProfile(session, 0.0, 0.0, 40.0, 30.0);
        const auto cut_profile_id =
            createRectangleProfile(session, 5.0, 5.0, 16.0, 14.0);
        const auto* original_profile =
            session.document().findProfile(profile_id);
        CHECK(original_profile != nullptr);
        const auto sketch_id = original_profile->source_sketch_id;
        const auto* original_sketch =
            session.document().findSketch(sketch_id);
        CHECK(original_sketch != nullptr);
        CHECK(original_sketch->model.state().lines.size() == 4U);

        ColdKernel authoring_kernel;
        const auto base = session.execute(
            application::CreateExtrudeFeatureCommand{
                profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0}, false},
                "Projection 00A Base"},
            authoring_kernel);
        CHECK(base.ok() && base.feature_id);
        const auto base_id = *base.feature_id;
        const auto pocket = session.execute(
            application::CreateExtrudeFeatureCommand{
                cut_profile_id,
                session.document().revision(),
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{4.0}, false},
                "Projection 00A Pocket"},
            authoring_kernel);
        CHECK(pocket.ok() && pocket.feature_id);
        const auto pocket_id = *pocket.feature_id;
        CHECK(part::evaluatePart(session.document(), authoring_kernel)
                  .body_status == part::BodyEvaluationStatus::up_to_date);

        // A7: removing a referenced Line does not replace it with a similar
        // one, nor may the downstream Cut present stale current Body truth.
        const auto lines =
            session.document().findSketch(sketch_id)->model.state().lines;
        const auto right = std::find_if(
            lines.begin(), lines.end(),
            [](const sketch::SketchLineState& line) {
                return line.start.u == 40.0 && line.end.u == 40.0;
            });
        CHECK(right != lines.end());
        const auto removed_id = right->id;
        const auto start = right->start;
        const auto end = right->end;
        const auto erased = session.execute(
            application::EraseSketchEntityCommand{
                sketch_id, removed_id});
        CHECK(erased.ok() && erased.changed);
        const auto* deleted_sketch =
            session.document().findSketch(sketch_id);
        CHECK(part::resolveProfileRegionIntent(
                  deleted_sketch->model,
                  session.document().findProfile(profile_id)->region_intent)
                  .status ==
              part::ProfileIntentResolutionStatus::missing_source_entity);
        ColdKernel blocked_kernel;
        const auto blocked =
            part::evaluatePart(session.document(), blocked_kernel);
        CHECK(blocked.body_status == part::BodyEvaluationStatus::unavailable);
        CHECK(blocked.features.size() == 2U);
        CHECK(blocked.features[0].status !=
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(blocked.features[1].status !=
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(session.document().findFeature(base_id) != nullptr);
        CHECK(session.document().findFeature(pocket_id) != nullptr);

        const auto half1 = session.execute(
            application::AddSketchLineCommand{
                sketch_id, start, {40.0, 15.0},
                sketch::EntityRole::regular});
        const auto half2 = session.execute(
            application::AddSketchLineCommand{
                sketch_id, {40.0, 15.0}, end,
                sketch::EntityRole::regular});
        CHECK(half1.ok() && half1.entity_id);
        CHECK(half2.ok() && half2.entity_id);
        CHECK(*half1.entity_id != removed_id);
        CHECK(*half2.entity_id != removed_id);
        const auto* five_line_sketch =
            session.document().findSketch(sketch_id);
        CHECK(five_line_sketch != nullptr);
        CHECK(five_line_sketch->model.state().lines.size() == 5U);
        const auto candidates =
            sketch::analyzeRegions(five_line_sketch->model);
        CHECK(candidates.complete());
        CHECK(candidates.regions.size() == 1U);
        const auto new_intent =
            part::makeProfileRegionIntent(candidates.regions.front());
        CHECK(new_intent);

        const auto repair = session.execute(
            application::ReplaceProfileRegionIntentCommand{
                profile_id,
                session.document().revision(),
                *new_intent});
        CHECK(repair.ok() && repair.changed);
        CHECK(session.document().findProfile(profile_id)->id == profile_id);
        CHECK(session.document().findFeature(base_id)->id == base_id);
        CHECK(session.document().findFeature(pocket_id)->id == pocket_id);
        ColdKernel repaired_kernel;
        const auto repaired =
            part::evaluatePart(session.document(), repaired_kernel);
        CHECK(repaired.body_status == part::BodyEvaluationStatus::up_to_date);
        CHECK(repaired.features.size() == 2U);
        CHECK(repaired.features[0].status ==
              part::FeatureEvaluationStatus::up_to_date);
        CHECK(repaired.features[1].status ==
              part::FeatureEvaluationStatus::up_to_date);

        // The complete authored repair is Undo/Redo-safe.
        CHECK(session.undo().changed);
        const auto* before_redo =
            session.document().findSketch(sketch_id);
        CHECK(part::resolveProfileRegionIntent(
                  before_redo->model,
                  session.document().findProfile(profile_id)->region_intent)
                  .status ==
              part::ProfileIntentResolutionStatus::missing_source_entity);
        CHECK(session.redo().changed);
        CHECK(part::evaluatePart(session.document(), repaired_kernel)
                  .body_status == part::BodyEvaluationStatus::up_to_date);
        CHECK(session.save().ok());
        return profile_id;
    }();

    // Fresh store/provider/session; no previously evaluated BRep, tokens
    // or cached Profile region can be reused as persistent model truth.
    const auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(loaded.document->documentId() == document_id);
    const auto* profile =
        loaded.document->findProfile(original_profile_id);
    CHECK(profile != nullptr);
    const auto* source =
        loaded.document->findSketch(profile->source_sketch_id);
    CHECK(source != nullptr);
    CHECK(source->model.state().lines.size() == 5U);
    CHECK(part::resolveProfileRegionIntent(
              source->model, profile->region_intent).valid());
    CHECK(loaded.document->body().features.size() == 2U);
    const auto base_id = loaded.document->body().features[0].id;
    const auto cut_id = loaded.document->body().features[1].id;
    CHECK(base_id.serialized() == "1");
    CHECK(cut_id.serialized() == "2");
    const auto* base_def = std::get_if<part::ExtrudeFeature>(
        &loaded.document->body().features[0].definition);
    CHECK(base_def != nullptr);
    CHECK(base_def->profile_id == original_profile_id);
    ColdKernel cold;
    const auto current =
        part::evaluatePart(*loaded.document, cold);
    CHECK(current.body_status == part::BodyEvaluationStatus::up_to_date);
    CHECK(current.features.size() == 2U);
    CHECK(current.features[0].status ==
          part::FeatureEvaluationStatus::up_to_date);
    CHECK(current.features[1].status ==
          part::FeatureEvaluationStatus::up_to_date);
    CHECK(cold.calls == 2U);
    std::cout << "PROJECTION00A_A5_A7_COLD_REPAIR_PASS"
              << " profiles_preserved=1 features_preserved=2"
              << " fresh_kernel_calls=" << cold.calls << '\n';
}

} // namespace

int main() {
    TempDirectory temp;
    const auto path =
        temp.path / "ColdFeatures.ss2part";
    part::PartDocumentStore store;
    const auto document_id =
        core::DocumentId::generate();

    part::ProfileId add_profile;
    part::ProfileId cut_profile;
    part::FeatureId add_feature;
    part::FeatureId cut_feature;

    {
        auto document =
            part::PartDocument::create(
                document_id);
        const auto created =
            store.createNew(path, document);
        CHECK(created.ok());

        application::DocumentSession session{
            path,
            std::move(document),
            *created.checkpoint};

        add_profile =
            createRectangleProfile(
                session,
                0.0, 0.0,
                40.0, 30.0);
        cut_profile =
            createRectangleProfile(
                session,
                10.0, 8.0,
                30.0, 22.0);

        ColdKernel authoring_kernel;
        const auto first =
            session.execute(
                application::
                    CreateExtrudeFeatureCommand{
                        add_profile,
                        session.document().revision(),
                        part::ExtrudeOperation::add,
                        part::OneSidedExtrudeExtent{
                            core::LengthValue{10.0},
                            false},
                        "Base Add"},
                authoring_kernel);
        CHECK(first.ok() && first.feature_id);
        add_feature = *first.feature_id;

        const auto second =
            session.execute(
                application::
                    CreateExtrudeFeatureCommand{
                        cut_profile,
                        session.document().revision(),
                        part::ExtrudeOperation::cut,
                        part::MidplaneExtrudeExtent{
                            core::LengthValue{6.0}},
                        "Pocket Cut"},
                authoring_kernel);
        CHECK(second.ok() && second.feature_id);
        cut_feature = *second.feature_id;
        CHECK(add_feature.serialized() == "1");
        CHECK(cut_feature.serialized() == "2");

        const auto suppressed =
            session.execute(
                application::
                    SetFeatureSuppressedCommand{
                        cut_feature,
                        session.document().revision(),
                        true});
        CHECK(
            suppressed.ok() &&
            suppressed.changed);
        CHECK(
            !session.document()
                 .profilePresentationVisible(
                     add_profile));
        CHECK(
            session.document()
                .profilePresentationVisible(
                    cut_profile));

        const auto saved = session.save();
        CHECK(saved.ok());
        CHECK(!session.needsSave());
    }

    // True cold load: no previous DocumentSession, evaluator result, provider
    // shape or presentation cache survives.
    auto loaded = store.load(path);
    CHECK(loaded.ok());
    CHECK(
        loaded.document->documentId() ==
        document_id);
    CHECK(
        loaded.document->body()
            .features.size() == 2U);
    CHECK(
        loaded.document->body()
            .features[0].id ==
        add_feature);
    CHECK(
        loaded.document->body()
            .features[1].id ==
        cut_feature);
    CHECK(
        !loaded.document->body()
             .features[0].suppressed);
    CHECK(
        loaded.document->body()
            .features[1].suppressed);
    CHECK(
        !loaded.document
             ->profilePresentationVisible(
                 add_profile));
    CHECK(
        loaded.document
            ->profilePresentationVisible(
                cut_profile));

    const auto* persisted_cut =
        std::get_if<part::ExtrudeFeature>(
            &loaded.document->body()
                 .features[1].definition);
    CHECK(persisted_cut != nullptr);
    CHECK(
        persisted_cut->operation ==
        part::ExtrudeOperation::cut);
    const auto* persisted_midplane =
        std::get_if<
            part::MidplaneExtrudeExtent>(
            &persisted_cut->extent);
    CHECK(persisted_midplane != nullptr);
    CHECK(
        persisted_midplane
            ->total_distance.millimetres ==
        6.0);

    ColdKernel cold_kernel;
    const auto cold_evaluation =
        part::evaluatePart(
            *loaded.document,
            cold_kernel);
    CHECK(cold_kernel.calls == 1U);
    CHECK(
        cold_evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(
        cold_evaluation.features.size() ==
        2U);
    CHECK(
        cold_evaluation.features[0].status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        cold_evaluation.features[1].status ==
        part::FeatureEvaluationStatus::suppressed);

    application::DocumentSession reopened{
        path,
        std::move(*loaded.document),
        *loaded.checkpoint};

    const auto unsuppressed =
        reopened.execute(
            application::
                SetFeatureSuppressedCommand{
                    cut_feature,
                    reopened.document().revision(),
                    false});
    CHECK(
        unsuppressed.ok() &&
        unsuppressed.changed);
    CHECK(
        !reopened.document()
             .profilePresentationVisible(
                 cut_profile));

    ColdKernel recompute_kernel;
    const auto recomputed =
        part::evaluatePart(
            reopened.document(),
            recompute_kernel);
    CHECK(recompute_kernel.calls == 2U);
    CHECK(
        recomputed.features[1].status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(
        recomputed.body_status ==
        part::BodyEvaluationStatus::up_to_date);

    CHECK(reopened.undo().changed);
    CHECK(
        reopened.document()
            .findFeature(cut_feature)
            ->suppressed);
    CHECK(
        reopened.document()
            .profilePresentationVisible(
                cut_profile));
    CHECK(reopened.redo().changed);
    CHECK(
        !reopened.document()
             .findFeature(cut_feature)
             ->suppressed);

    CHECK(reopened.save().ok());

    auto active_reload =
        store.load(path);
    CHECK(active_reload.ok());
    CHECK(
        !active_reload.document->findFeature(
             cut_feature)
             ->suppressed);
    CHECK(
        active_reload.document->body()
            .features[0].id ==
        add_feature);
    CHECK(
        active_reload.document->body()
            .features[1].id ==
        cut_feature);

    application::DocumentSession delete_session{
        path,
        std::move(*active_reload.document),
        *active_reload.checkpoint};
    const auto deleted =
        delete_session.execute(
            application::DeleteFeatureCommand{
                cut_feature,
                delete_session.document()
                    .revision()});
    CHECK(deleted.ok() && deleted.changed);
    CHECK(
        delete_session.document()
            .findFeature(cut_feature) == nullptr);
    CHECK(
        delete_session.document()
            .findProfile(cut_profile) != nullptr);
    CHECK(
        delete_session.document()
            .profilePresentationVisible(
                cut_profile));
    CHECK(delete_session.save().ok());

    const auto delete_reload =
        store.load(path);
    CHECK(delete_reload.ok());
    CHECK(
        delete_reload.document->body()
            .features.size() == 1U);
    CHECK(
        delete_reload.document->body()
            .features.front().id ==
        add_feature);
    CHECK(
        delete_reload.document->findFeature(
            cut_feature) == nullptr);
    CHECK(
        delete_reload.document->findProfile(
            cut_profile) != nullptr);

    ColdKernel delete_cold_kernel;
    const auto delete_cold =
        part::evaluatePart(
            *delete_reload.document,
            delete_cold_kernel);
    CHECK(delete_cold_kernel.calls == 1U);
    CHECK(
        delete_cold.body_status ==
        part::BodyEvaluationStatus::up_to_date);

    runProjection00AColdRepairEvidence();

    std::cout
        << "PM01G_FEATURE_COLD_PERSISTENCE_PASS\n";
    return EXIT_SUCCESS;
}
