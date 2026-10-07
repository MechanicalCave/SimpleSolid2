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
            << "PM-05D1 Edge draft CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

application::DocumentSession makeBaseSession(
    kernel_occt::OcctSolidModelingKernel& kernel) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* sketch =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(sketch != nullptr);
    const auto regions =
        sketch::analyzeRegions(sketch->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
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

std::vector<part::MaterialEdgeReference>
authorableEdges(
    const application::DocumentSession& session,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    const auto evaluation =
        part::evaluatePart(
            session.document(),
            kernel);
    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.current_topology);
    CHECK(evaluation.current_topology->complete());

    std::vector<part::MaterialEdgeReference> result;
    for (const auto& edge :
         evaluation.current_topology->edges) {
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
            result.push_back(
                *authored.reference);
        }
    }
    CHECK(!result.empty());
    return result;
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;

    // Fillet draft: exact candidate preview does not author state, stale
    // evaluation is rejected, and one current Finish authors one Feature.
    {
        auto session = makeBaseSession(kernel);
        const auto edges =
            authorableEdges(session, kernel);
        const auto base_revision =
            session.document().revision();
        const auto base_undo =
            session.undoDepth();

        auto draft_opt =
            application::FilletDraft::beginCreate(
                session,
                {edges.front()});
        CHECK(draft_opt);
        auto draft = std::move(*draft_opt);
        CHECK(
            draft.setRadius(
                core::LengthValue{1.0}));
        CHECK(draft.valid());

        const auto preview =
            session.evaluateFilletDraft(
                draft,
                kernel);
        CHECK(preview.committable());
        CHECK(preview.previewSolidAvailable());
        CHECK(
            preview.target_status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(
            preview.operation ==
            kernel::EdgeFeatureOperation::fillet);
        CHECK(
            session.document().revision() ==
            base_revision);
        CHECK(session.undoDepth() == base_undo);
        CHECK(
            session.document().body()
                .features.size() == 1U);

        CHECK(
            draft.setRadius(
                core::LengthValue{1.25}));
        const auto stale =
            application::finishFilletDraft(
                session,
                draft,
                preview,
                kernel);
        CHECK(!stale.ok());
        CHECK(
            stale.status ==
            application::EdgeFeatureDraftFinishStatus::
                stale_evaluation);
        CHECK(
            session.document().body()
                .features.size() == 1U);

        const auto current =
            session.evaluateFilletDraft(
                draft,
                kernel);
        CHECK(current.committable());
        const auto finish =
            application::finishFilletDraft(
                session,
                draft,
                current,
                kernel);
        CHECK(finish.ok());
        CHECK(finish.changed);
        CHECK(finish.feature_id);
        CHECK(
            session.document().body()
                .features.size() == 2U);
        CHECK(
            session.undoDepth() ==
            base_undo + 1U);
        CHECK(
            std::get_if<part::FilletFeature>(
                &session.document().body()
                     .features.back()
                     .definition) != nullptr);
    }

    // Chamfer uses the same shared semantic draft/evaluation/Finish path.
    {
        auto session = makeBaseSession(kernel);
        const auto edges =
            authorableEdges(session, kernel);
        const auto base_undo =
            session.undoDepth();

        auto draft_opt =
            application::ChamferDraft::beginCreate(
                session,
                {edges.front()});
        CHECK(draft_opt);
        auto draft = std::move(*draft_opt);
        CHECK(
            draft.setDistance(
                core::LengthValue{1.0}));

        const auto preview =
            session.evaluateChamferDraft(
                draft,
                kernel);
        CHECK(preview.committable());
        CHECK(preview.previewSolidAvailable());
        CHECK(
            preview.target_status ==
            part::FeatureEvaluationStatus::up_to_date);
        CHECK(
            preview.operation ==
            kernel::EdgeFeatureOperation::chamfer);

        const auto finish =
            application::finishChamferDraft(
                session,
                draft,
                preview,
                kernel);
        CHECK(finish.ok());
        CHECK(finish.changed);
        CHECK(finish.feature_id);
        CHECK(
            session.undoDepth() ==
            base_undo + 1U);
        CHECK(
            std::get_if<part::ChamferFeature>(
                &session.document().body()
                     .features.back()
                     .definition) != nullptr);
    }

    std::cout
        << "PM-05D1 edge-feature draft preview tests passed\n";
    return EXIT_SUCCESS;
}
