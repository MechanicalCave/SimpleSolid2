#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/edge_feature_draft.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-05B2 edge-draft CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

part::FeatureId featureId(const char* text) {
    const auto parsed =
        part::FeatureId::parse(text);
    CHECK(parsed.has_value());
    return *parsed;
}

part::FeatureSurfaceAddress cap(
    part::FeatureId producer,
    part::FeatureSurfaceRoleKind role) {
    part::FeatureSurfaceAddress result{
        producer,
        role,
        std::nullopt,
        0U,
        0U,
        false};
    CHECK(result.valid());
    return result;
}

part::FeatureCurveAddress curve(
    part::FeatureId producer,
    part::FeatureSurfaceAddress first,
    part::FeatureSurfaceAddress second) {
    std::vector<part::FeatureSurfaceAddress>
        surfaces{
            std::move(first),
            std::move(second)};
    std::sort(
        surfaces.begin(),
        surfaces.end());
    part::FeatureCurveAddress result{
        producer,
        part::FeatureCurveRoleKind::cap_side,
        std::move(surfaces)};
    CHECK(result.valid());
    return result;
}

part::MaterialEdgeReference edgeReference(
    part::FeatureId stage_id,
    part::FeatureCurveAddress address) {
    part::MaterialEdgeReference result{
        {
            part::BodyStageKind::after_feature,
            stage_id},
        std::move(address),
        part::SingularAtAuthoredStage{}};
    CHECK(result.valid());
    return result;
}

application::DocumentSession makeSession(
    part::FeatureId& final_feature_id) {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    auto state =
        document.state();

    const auto historical =
        state.body.next_feature_id.allocate();
    const auto current =
        state.body.next_feature_id.allocate();
    CHECK(historical && current);

    const auto first =
        cap(
            *historical,
            part::FeatureSurfaceRoleKind::
                profile_cap);
    const auto second =
        cap(
            *historical,
            part::FeatureSurfaceRoleKind::
                extent_cap);

    state.body.features.push_back(
        part::PartFeature{
            *current,
            "ExistingFillet",
            false,
            part::FilletFeature{
                {
                    edgeReference(
                        *historical,
                        curve(
                            *historical,
                            first,
                            second)),
                },
                core::LengthValue{1.0}}});

    auto restored =
        part::PartDocument::restore(
            document.documentId(),
            std::move(state),
            document.revision());
    CHECK(restored.ok());

    final_feature_id = *current;
    return application::DocumentSession{
        {},
        std::move(*restored.document)};
}

} // namespace

int main() {
    part::FeatureId final_feature;
    auto session =
        makeSession(final_feature);

    const auto first =
        cap(
            final_feature,
            part::FeatureSurfaceRoleKind::
                profile_cap);
    const auto second =
        cap(
            final_feature,
            part::FeatureSurfaceRoleKind::
                extent_cap);
    const auto third =
        cap(
            final_feature,
            part::FeatureSurfaceRoleKind::
                negative_cap);

    auto edge_a =
        edgeReference(
            final_feature,
            curve(
                final_feature,
                first,
                second));
    auto edge_b =
        edgeReference(
            final_feature,
            curve(
                final_feature,
                first,
                third));

    // Create drafts begin incomplete, capture Document/Revision and require
    // the exact stage immediately preceding the future Feature.
    auto fillet =
        application::FilletDraft::beginCreate(
            session);
    CHECK(!fillet.valid());
    CHECK(fillet.currentFor(session));
    CHECK(fillet.requiredStage().has_value());
    CHECK(
        fillet.requiredStage()->feature_id ==
        final_feature);
    CHECK(!fillet.command().has_value());

    const auto initial_generation =
        fillet.generation();
    CHECK(
        fillet.setEdges(
            {edge_b, edge_a}));
    CHECK(
        fillet.generation() ==
        initial_generation + 1U);
    CHECK(
        std::is_sorted(
            fillet.edges().begin(),
            fillet.edges().end()));
    CHECK(!fillet.valid());

    CHECK(
        !fillet.setRadius(
            core::LengthValue{0.0}));
    CHECK(
        fillet.setRadius(
            core::LengthValue{2.5}));
    CHECK(fillet.valid());
    CHECK(fillet.setName("Edge blend"));

    const auto fillet_command =
        fillet.command();
    CHECK(fillet_command.has_value());
    CHECK(fillet_command->valid());
    CHECK(
        fillet_command->expected_revision ==
        session.document().revision());
    CHECK(fillet_command->edges.size() == 2U);
    CHECK(
        std::is_sorted(
            fillet_command->edges.begin(),
            fillet_command->edges.end()));
    CHECK(
        fillet_command->radius.millimetres ==
        2.5);

    // Duplicate semantic Edge intent is rejected instead of being silently
    // deduplicated. A reference from any stage other than the exact consumed
    // stage is also rejected.
    const auto before_reject =
        fillet.generation();
    CHECK(
        !fillet.setEdges(
            {edge_a, edge_a}));
    CHECK(
        fillet.generation() ==
        before_reject);

    const auto wrong_stage =
        featureId("99");
    auto wrong =
        edge_a;
    wrong.stage = {
        part::BodyStageKind::after_feature,
        wrong_stage};
    CHECK(
        !fillet.setEdges(
            {wrong}));
    CHECK(
        fillet.generation() ==
        before_reject);

    // Chamfer uses the same semantic set/canonicalization rules but owns an
    // independent equal-distance parameter.
    auto chamfer =
        application::ChamferDraft::beginCreate(
            session,
            {edge_b, edge_a});
    CHECK(chamfer.has_value());
    CHECK(!chamfer->valid());
    CHECK(
        chamfer->setDistance(
            core::LengthValue{1.25}));
    CHECK(chamfer->valid());

    const auto chamfer_command =
        chamfer->command();
    CHECK(chamfer_command.has_value());
    CHECK(chamfer_command->valid());
    CHECK(
        chamfer_command->distance.millimetres ==
        1.25);
    CHECK(
        chamfer_command->edges ==
        fillet_command->edges);

    // Draft freshness is DocumentRevision-bound. This slice deliberately
    // creates no execute()/Finish path; PM-05C will add provider execution.
    auto props =
        session.document().properties();
    props.title = "Revision change";
    const auto changed =
        session.execute(
            application::SetDocumentPropertiesCommand{
                props});
    CHECK(changed.ok());
    CHECK(changed.changed);
    CHECK(!fillet.currentFor(session));
    CHECK(!chamfer->currentFor(session));

    std::cout
        << "PM05B2_EDGE_FEATURE_DRAFT_PASS"
        << " provider_execution=0"
        << " canonical_set=1"
        << " duplicate_reject=1"
        << " stage_bound=1"
        << " revision_bound=1\n";
    return EXIT_SUCCESS;
}
