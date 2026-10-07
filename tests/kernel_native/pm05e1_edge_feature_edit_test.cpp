#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/edge_feature_draft.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05E1 edge edit CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

application::DocumentSession makeBaseSession(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto document =
        part::PartDocument::create(core::DocumentId::generate());
    application::DocumentSession session{{}, std::move(document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(sketch_created.ok() && sketch_created.sketch_id);
    CHECK(
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false})
            .ok());

    const auto* sketch =
        session.document().findSketch(*sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto regions = sketch::analyzeRegions(sketch->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(regions.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    const auto base =
        session.execute(
            application::CreateExtrudeFeatureCommand{
                *profile.profile_id,
                session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false},
                "Base"},
            kernel);
    CHECK(base.ok() && base.feature_id);
    return session;
}

std::vector<part::MaterialEdgeReference> authorableEdges(
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto evaluation =
        part::evaluatePart(session.document(), kernel);
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.current_topology);
    CHECK(evaluation.current_topology->complete());

    std::vector<part::MaterialEdgeReference> result;
    for (const auto& edge : evaluation.current_topology->edges) {
        if (edge.accounting_class !=
                part::TopologyAccountingClass::referenceable ||
            edge.referenceability !=
                kernel::ReferenceStatus::resolved ||
            edge.periodic_seam ||
            edge.representation_partition) {
            continue;
        }
        const auto authored =
            part::authorMaterialEdgeReference(
                *evaluation.current_topology,
                edge.runtime_token);
        if (authored.ok()) {
            result.push_back(*authored.reference);
        }
    }
    CHECK(!result.empty());
    return result;
}

part::FeatureId createFillet(
    application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto edges = authorableEdges(session, kernel);
    auto draft =
        application::FilletDraft::beginCreate(
            session,
            {edges.front()});
    CHECK(draft);
    CHECK(draft->setRadius(core::LengthValue{1.0}));
    const auto evaluated =
        session.evaluateFilletDraft(*draft, kernel);
    CHECK(evaluated.committable());
    const auto finish =
        application::finishFilletDraft(
            session, *draft, evaluated, kernel);
    CHECK(finish.ok() && finish.feature_id);
    return *finish.feature_id;
}

part::FeatureId createChamfer(
    application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto edges = authorableEdges(session, kernel);
    auto draft =
        application::ChamferDraft::beginCreate(
            session,
            {edges.front()});
    CHECK(draft);
    CHECK(draft->setDistance(core::LengthValue{1.0}));
    const auto evaluated =
        session.evaluateChamferDraft(*draft, kernel);
    CHECK(evaluated.committable());
    const auto finish =
        application::finishChamferDraft(
            session, *draft, evaluated, kernel);
    CHECK(finish.ok() && finish.feature_id);
    return *finish.feature_id;
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;

    {
        auto session = makeBaseSession(kernel);
        const auto feature_id = createFillet(session, kernel);
        const auto feature_count =
            session.document().body().features.size();
        const auto undo_before = session.undoDepth();

        auto draft =
            application::FilletDraft::beginEdit(
                session, feature_id);
        CHECK(draft);
        CHECK(
            draft->mode() ==
            application::EdgeFeatureDraftMode::edit);
        CHECK(draft->featureId() == feature_id);
        CHECK(draft->radius());
        CHECK(draft->radius()->millimetres == 1.0);
        CHECK(
            draft->requiredStage() ==
            std::optional<part::BodyStageRef>{
                draft->edges().front().stage});

        const auto before_change =
            session.evaluateFilletDraft(*draft, kernel);
        CHECK(before_change.committable());
        CHECK(before_change.feature_id == feature_id);
        CHECK(
            before_change.mode ==
            application::EdgeFeatureDraftMode::edit);

        CHECK(draft->setRadius(core::LengthValue{1.5}));
        const auto stale =
            application::finishFilletDraft(
                session, *draft, before_change, kernel);
        CHECK(!stale.ok());
        CHECK(
            stale.status ==
            application::EdgeFeatureDraftFinishStatus::
                stale_evaluation);

        const auto current =
            session.evaluateFilletDraft(*draft, kernel);
        CHECK(current.committable());
        CHECK(current.previewSolidAvailable());
        const auto finish =
            application::finishFilletDraft(
                session, *draft, current, kernel);
        CHECK(finish.ok());
        CHECK(finish.feature_id == feature_id);
        CHECK(
            session.document().body().features.size() ==
            feature_count);
        CHECK(session.undoDepth() == undo_before + 1U);

        const auto* edited =
            session.document().findFeature(feature_id);
        CHECK(edited != nullptr);
        const auto* fillet =
            std::get_if<part::FilletFeature>(
                &edited->definition);
        CHECK(fillet != nullptr);
        CHECK(fillet->radius.millimetres == 1.5);

        CHECK(session.undo().ok());
        const auto* undone =
            session.document().findFeature(feature_id);
        CHECK(undone != nullptr);
        const auto* old_fillet =
            std::get_if<part::FilletFeature>(
                &undone->definition);
        CHECK(old_fillet != nullptr);
        CHECK(old_fillet->radius.millimetres == 1.0);

        CHECK(session.redo().ok());
        const auto* redone =
            session.document().findFeature(feature_id);
        CHECK(redone != nullptr);
        const auto* new_fillet =
            std::get_if<part::FilletFeature>(
                &redone->definition);
        CHECK(new_fillet != nullptr);
        CHECK(new_fillet->radius.millimetres == 1.5);
    }

    {
        auto session = makeBaseSession(kernel);
        const auto feature_id = createChamfer(session, kernel);
        const auto feature_count =
            session.document().body().features.size();

        auto draft =
            application::ChamferDraft::beginEdit(
                session, feature_id);
        CHECK(draft);
        CHECK(
            draft->mode() ==
            application::EdgeFeatureDraftMode::edit);
        CHECK(draft->featureId() == feature_id);
        CHECK(draft->distance());
        CHECK(draft->setDistance(core::LengthValue{1.25}));

        const auto evaluated =
            session.evaluateChamferDraft(*draft, kernel);
        CHECK(evaluated.committable());
        CHECK(evaluated.previewSolidAvailable());
        const auto finish =
            application::finishChamferDraft(
                session, *draft, evaluated, kernel);
        CHECK(finish.ok());
        CHECK(finish.feature_id == feature_id);
        CHECK(
            session.document().body().features.size() ==
            feature_count);

        const auto* edited =
            session.document().findFeature(feature_id);
        CHECK(edited != nullptr);
        const auto* chamfer =
            std::get_if<part::ChamferFeature>(
                &edited->definition);
        CHECK(chamfer != nullptr);
        CHECK(chamfer->distance.millimetres == 1.25);
    }

    std::cout
        << "PM-05E1 edge-feature edit tests passed\n";
    return EXIT_SUCCESS;
}
