#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/part_document_store.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

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

    std::cout
        << "PM01G_FEATURE_COLD_PERSISTENCE_PASS\n";
    return EXIT_SUCCESS;
}
