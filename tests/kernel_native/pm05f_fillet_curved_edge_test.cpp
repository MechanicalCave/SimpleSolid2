#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel_occt/solid_modeling_kernel.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05F curved Edge CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

std::optional<part::ProfileId> createProfile(
    application::DocumentSession& session,
    sketch::SketchId sketch_id) {
    const auto* sketch =
        session.document().findSketch(sketch_id);
    if (sketch == nullptr) {
        return std::nullopt;
    }
    const auto regions =
        sketch::analyzeRegions(sketch->model);
    if (!regions.complete() ||
        regions.regions.size() != 1U) {
        return std::nullopt;
    }
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
    if (!intent) {
        return std::nullopt;
    }
    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                sketch_id,
                session.document().revision(),
                *intent});
    return profile.ok()
        ? profile.profile_id
        : std::nullopt;
}

struct Fixture final {
    part::PartDocument document;
    part::FeatureId cut_id;
};

Fixture makeCircularCutPart() {
    auto source =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(source)};

    const auto base_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(base_sketch.ok() && base_sketch.sketch_id);
    CHECK(
        session.execute(
            application::AddSketchRectangleCommand{
                *base_sketch.sketch_id,
                session.document().revision(),
                {-30.0, -20.0},
                {30.0, 20.0},
                sketch::EntityRole::regular,
                false})
            .ok());
    const auto base_profile =
        createProfile(
            session,
            *base_sketch.sketch_id);
    CHECK(base_profile);

    const auto cut_sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(cut_sketch.ok() && cut_sketch.sketch_id);
    CHECK(
        session.execute(
            application::AddSketchCircleCommand{
                *cut_sketch.sketch_id,
                {0.0, 0.0},
                6.0,
                sketch::EntityRole::regular})
            .ok());
    const auto cut_profile =
        createProfile(
            session,
            *cut_sketch.sketch_id);
    CHECK(cut_profile);

    auto state = session.document().state();
    const auto base_id =
        state.body.next_feature_id.allocate();
    const auto cut_id =
        state.body.next_feature_id.allocate();
    CHECK(base_id && cut_id);

    state.body.features.push_back(
        part::PartFeature{
            *base_id,
            "Base",
            false,
            part::ExtrudeFeature{
                *base_profile,
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false}}});
    state.body.features.push_back(
        part::PartFeature{
            *cut_id,
            "CircularCut",
            false,
            part::ExtrudeFeature{
                *cut_profile,
                part::ExtrudeOperation::cut,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{20.0},
                    false}}});

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            std::move(state),
            session.document().revision());
    CHECK(restored.ok());
    return {
        std::move(*restored.document),
        *cut_id};
}

std::vector<part::MaterialEdgeReference>
circularBooleanEdges(
    const part::FeatureEvaluation& cut) {
    CHECK(cut.result_topology);
    std::vector<part::MaterialEdgeReference>
        result;

    for (const auto& curve :
         cut.produced_curves) {
        if (curve.address.producer_feature_id !=
                cut.feature_id ||
            curve.address.role !=
                part::FeatureCurveRoleKind::
                    boolean_intersection ||
            curve.curve_kind !=
                kernel::CurveKind::circle ||
            curve.status !=
                kernel::ReferenceStatus::resolved ||
            curve.strict_edge_status !=
                kernel::ReferenceStatus::resolved ||
            curve.current_edges.size() != 1U) {
            continue;
        }

        const auto authored =
            part::authorMaterialEdgeReference(
                *cut.result_topology,
                curve.current_edges.front());
        if (authored.ok()) {
            result.push_back(
                *authored.reference);
        }
    }

    std::sort(result.begin(), result.end());
    result.erase(
        std::unique(
            result.begin(),
            result.end()),
        result.end());
    return result;
}

part::PartDocument appendEdgeFeature(
    const part::PartDocument& source,
    part::MaterialEdgeReference edge,
    kernel::EdgeFeatureOperation operation,
    double parameter) {
    auto state = source.state();
    const auto id =
        state.body.next_feature_id.allocate();
    CHECK(id);

    if (operation ==
        kernel::EdgeFeatureOperation::fillet) {
        state.body.features.push_back(
            part::PartFeature{
                *id,
                "CircularFillet",
                false,
                part::FilletFeature{
                    {std::move(edge)},
                    core::LengthValue{parameter}}});
    } else {
        state.body.features.push_back(
            part::PartFeature{
                *id,
                "CircularChamfer",
                false,
                part::ChamferFeature{
                    {std::move(edge)},
                    core::LengthValue{parameter}}});
    }

    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

void verifyOperation(
    const Fixture& fixture,
    const part::FeatureEvaluation& cut,
    const part::MaterialEdgeReference& edge,
    kernel::EdgeFeatureOperation operation,
    double parameter,
    kernel_occt::OcctSolidModelingKernel& kernel) {
    CHECK(cut.result_topology);
    CHECK(cut.result_solid);

    std::optional<kernel::EdgeFeatureInput>
        input;
    if (operation ==
        kernel::EdgeFeatureOperation::fillet) {
        const part::FilletFeature definition{
            {edge},
            core::LengthValue{parameter}};
        const auto resolved =
            part::resolveKernelEdgeFeatureInput(
                definition,
                &*cut.result_topology);
        CHECK(resolved.ok());
        input = resolved.input;
    } else {
        const part::ChamferFeature definition{
            {edge},
            core::LengthValue{parameter}};
        const auto resolved =
            part::resolveKernelEdgeFeatureInput(
                definition,
                &*cut.result_topology);
        CHECK(resolved.ok());
        input = resolved.input;
    }
    CHECK(input);

    const auto provider =
        kernel.edgeFeature(
            *input,
            cut.result_solid);
    CHECK(provider.ok());
    CHECK(provider.edge_feature_input_membership);
    CHECK(
        provider.edge_feature_input_membership
            ->provider_contour_edges.size() == 1U);
    CHECK(
        provider.edge_feature_input_membership
            ->exactFor(input->edges));

    const auto document =
        appendEdgeFeature(
            fixture.document,
            edge,
            operation,
            parameter);
    const auto evaluation =
        part::evaluatePart(
            document,
            kernel);
    CHECK(evaluation.features.size() == 3U);
    const auto& target =
        evaluation.features[2];
    CHECK(
        target.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(target.result_topology);
    CHECK(target.result_topology->complete());
}

} // namespace

int main() {
    kernel_occt::OcctSolidModelingKernel kernel;
    const auto fixture =
        makeCircularCutPart();
    const auto evaluation =
        part::evaluatePart(
            fixture.document,
            kernel);

    CHECK(
        evaluation.body_status ==
        part::BodyEvaluationStatus::up_to_date);
    CHECK(evaluation.features.size() == 2U);
    const auto& cut =
        evaluation.features[1];
    CHECK(cut.feature_id == fixture.cut_id);
    CHECK(
        cut.status ==
        part::FeatureEvaluationStatus::up_to_date);
    CHECK(cut.result_topology);
    CHECK(cut.result_topology->complete());

    const auto circles =
        circularBooleanEdges(cut);
    CHECK(circles.size() == 2U);

    verifyOperation(
        fixture,
        cut,
        circles.front(),
        kernel::EdgeFeatureOperation::fillet,
        2.0,
        kernel);
    verifyOperation(
        fixture,
        cut,
        circles.front(),
        kernel::EdgeFeatureOperation::chamfer,
        1.5,
        kernel);

    std::cout
        << "PM05F_CURVED_EDGE_PASS"
        << " circular_boolean_edges=2"
        << " fillet_single_circle=1"
        << " chamfer_single_circle=1"
        << " exact_provider_membership=1\n";
    return EXIT_SUCCESS;
}
