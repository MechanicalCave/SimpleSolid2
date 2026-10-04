#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01C evaluator CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class FakeSolid final
    : public kernel::RuntimeSolid {
public:
    std::vector<kernel::RuntimeFaceToken>
        tokens;
    std::uint64_t next_token{1U};
};

class FakeKernel final
    : public kernel::ISolidModelingKernel {
public:
    bool saw_reverse{false};
    bool saw_midplane{false};
    bool corrupt_topology_inventory{false};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream) noexcept override {
        kernel::SolidModelingResult result;
        if (!input.valid()) {
            result.status =
                kernel::SolidModelingStatus::
                    invalid_input;
            return result;
        }
        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    missing_upstream;
            return result;
        }

        if (input.start_offset_mm < 0.0 &&
            input.end_offset_mm == 0.0 &&
            input.start_cap_role ==
                kernel::ExtrudeCapRole::
                    extent_cap &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    profile_cap) {
            saw_reverse = true;
        }
        if (input.start_offset_mm < 0.0 &&
            input.end_offset_mm > 0.0 &&
            input.start_cap_role ==
                kernel::ExtrudeCapRole::
                    negative_cap &&
            input.end_cap_role ==
                kernel::ExtrudeCapRole::
                    positive_cap) {
            saw_midplane = true;
        }

        const double span =
            input.end_offset_mm -
            input.start_offset_mm;
        if (span == 99.0) {
            result.status =
                kernel::SolidModelingStatus::
                    no_effect;
            return result;
        }

        auto runtime =
            std::make_shared<FakeSolid>();
        if (upstream) {
            const auto* existing =
                dynamic_cast<
                    const FakeSolid*>(
                    upstream.get());
            if (!existing) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }
            runtime->tokens =
                existing->tokens;
            runtime->next_token =
                existing->next_token;
            for (const auto token :
                 existing->tokens) {
                result.inherited_faces.push_back(
                    {
                        token,
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                    });
            }
        }

        auto publish =
            [&result, &runtime](
                kernel::ExtrudeFaceRole role) {
                const kernel::RuntimeFaceToken token{
                    runtime->next_token++};
                runtime->tokens.push_back(token);
                result.new_faces.push_back(
                    {
                        std::move(role),
                        kernel::ReferenceStatus::
                            resolved,
                        1U,
                        token,
                    });
            };

        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.start_cap_role,
                std::nullopt,
            });
        publish(
            {
                kernel::ExtrudeGeneratedFaceRoleKind::cap,
                input.end_cap_role,
                std::nullopt,
            });
        for (const auto& use :
             input.profile.outer.boundary) {
            publish(
                {
                    kernel::ExtrudeGeneratedFaceRoleKind::
                        side,
                    std::nullopt,
                    use.provenance,
                });
        }

        result.status =
            kernel::SolidModelingStatus::ok;
        result.brep_valid = true;
        result.solid_count = 1U;
        result.current_faces =
            runtime->tokens;
        result.face_count =
            result.current_faces.size();
        result.edge_count = 0U;
        result.vertex_count = 0U;
        if (corrupt_topology_inventory &&
            !result.current_faces.empty()) {
            result.current_faces.pop_back();
        }
        result.solid =
            std::move(runtime);
        return result;
    }
};

struct Fixture final {
    part::PartDocument document;
    part::ProfileId profile_id;

    Fixture(
        part::PartDocument&& source,
        part::ProfileId profile)
        : document{std::move(source)},
          profile_id{profile} {}
};

Fixture makeFixture() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        {},
        std::move(document)};

    const auto sketch_created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(
        sketch_created.ok() &&
        sketch_created.sketch_id);
    const auto sketch_id =
        *sketch_created.sketch_id;

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());
    CHECK(rectangle.entity_ids.size() == 4U);

    const auto* sketch =
        session.document().findSketch(
            sketch_id);
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
                sketch_id,
                session.document().revision(),
                *intent});
    CHECK(
        profile.ok() &&
        profile.profile_id);

    auto restored =
        part::PartDocument::restore(
            session.document().documentId(),
            session.document().state(),
            session.document().revision());
    CHECK(restored.ok());
    return Fixture{
        std::move(*restored.document),
        *profile.profile_id};
}

part::PartDocument withFeatures(
    const part::PartDocument& source,
    std::vector<part::PartFeature> features,
    std::optional<part::FeatureIdCursor>
        cursor = std::nullopt) {
    auto state = source.state();
    if (cursor) {
        state.body.next_feature_id =
            *cursor;
    }
    state.body.features =
        std::move(features);
    auto restored =
        part::PartDocument::restore(
            source.documentId(),
            std::move(state),
            source.revision());
    CHECK(restored.ok());
    return std::move(*restored.document);
}

part::PartFeature featureWithExtent(
    part::FeatureId id,
    part::ProfileId profile,
    part::ExtrudeOperation operation,
    part::ExtrudeExtent extent,
    bool suppressed = false) {
    return {
        id,
        "Extrude" + id.serialized(),
        suppressed,
        part::ExtrudeFeature{
            profile,
            operation,
            std::move(extent)},
    };
}

part::PartFeature feature(
    part::FeatureId id,
    part::ProfileId profile,
    part::ExtrudeOperation operation,
    double distance,
    bool suppressed = false) {
    return featureWithExtent(
        id,
        profile,
        operation,
        part::OneSidedExtrudeExtent{
            core::LengthValue{distance},
            false},
        suppressed);
}

} // namespace

int main() {
    FakeKernel kernel;
    auto fixture = makeFixture();

    const auto cursor =
        *part::FeatureIdCursor::parse("4");
    const auto id1 =
        *part::FeatureId::parse("1");
    const auto id2 =
        *part::FeatureId::parse("2");
    const auto id3 =
        *part::FeatureId::parse("3");

    // Add -> Cut is a valid ordered one-Body chain.
    auto valid =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    5.0),
            },
            cursor);
    const auto valid_eval =
        part::evaluatePart(
            valid,
            kernel);
    CHECK(
        valid_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(valid_eval.body_solid != nullptr);
    CHECK(valid_eval.features.size() == 2U);
    CHECK(
        valid_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        valid_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        valid_eval.current_face_references.size() ==
        12U);
    CHECK(valid_eval.current_topology.has_value());
    CHECK(
        valid_eval.current_topology->stage.kind ==
        part::BodyStageKind::after_feature);
    CHECK(
        valid_eval.current_topology->stage.feature_id ==
        std::optional<part::FeatureId>{id2});
    CHECK(valid_eval.current_topology->faces.size() == 12U);
    CHECK(valid_eval.current_topology->edges.empty());
    CHECK(valid_eval.current_topology->vertices.empty());
    CHECK(valid_eval.features[0].result_solid != nullptr);
    CHECK(valid_eval.features[0].result_topology.has_value());
    CHECK(
        valid_eval.features[0].result_topology
            ->stage.feature_id ==
        std::optional<part::FeatureId>{id1});
    CHECK(valid_eval.features[0].result_topology->faces.size() == 6U);
    CHECK(valid_eval.features[1].result_solid != nullptr);
    CHECK(valid_eval.features[1].result_topology.has_value());
    for (const auto& face :
         valid_eval.current_topology->faces) {
        CHECK(face.valid());
        CHECK(
            face.accounting_class ==
            part::TopologyAccountingClass::
                referenceable);
        CHECK(face.semantic_address.has_value());
    }
    for (const auto& reference :
         valid_eval.current_face_references) {
        CHECK(reference.address.valid());
        CHECK(
            reference.status ==
            kernel::ReferenceStatus::resolved);
        CHECK(reference.runtime_token);
    }

    // Part -> Kernel translation preserves Reverse OneSide semantics.
    kernel.saw_reverse = false;
    auto reversed =
        withFeatures(
            fixture.document,
            {
                featureWithExtent(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    part::OneSidedExtrudeExtent{
                        core::LengthValue{8.0},
                        true}),
            },
            cursor);
    const auto reversed_evaluation =
        part::evaluatePart(
            reversed,
            kernel);
    CHECK(
        reversed_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(kernel.saw_reverse);

    // Midplane uses total distance split equally and negative/positive caps.
    kernel.saw_midplane = false;
    auto centered =
        withFeatures(
            fixture.document,
            {
                featureWithExtent(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    part::MidplaneExtrudeExtent{
                        core::LengthValue{10.0}}),
            },
            cursor);
    const auto centered_evaluation =
        part::evaluatePart(
            centered,
            kernel);
    CHECK(
        centered_evaluation.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);
    CHECK(kernel.saw_midplane);

    bool negative_cap = false;
    bool positive_cap = false;
    for (const auto& face :
         centered_evaluation.features[0]
             .produced_faces) {
        negative_cap =
            negative_cap ||
            face.address.role ==
                part::FeatureFaceRoleKind::
                    negative_cap;
        positive_cap =
            positive_cap ||
            face.address.role ==
                part::FeatureFaceRoleKind::
                    positive_cap;
    }
    CHECK(negative_cap);
    CHECK(positive_cap);

    // A first Cut is Blocked, and a later Add cannot silently restart history.
    auto blocked =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    5.0),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto blocked_eval =
        part::evaluatePart(
            blocked,
            kernel);
    CHECK(
        blocked_eval.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(blocked_eval.body_solid == nullptr);
    CHECK(blocked_eval.resolved_prefix_solid == nullptr);
    CHECK(
        blocked_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        blocked_eval.features[0].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_upstream_body);
    CHECK(
        blocked_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        blocked_eval.features[1].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            upstream_unavailable);

    // Suppressed history does not contribute and does not poison a later
    // first successful Add.
    auto suppressed =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    5.0,
                    true),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto suppressed_eval =
        part::evaluatePart(
            suppressed,
            kernel);
    CHECK(
        suppressed_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            suppressed);
    CHECK(
        suppressed_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        suppressed_eval.body_status ==
        part::BodyEvaluationStatus::
            up_to_date);

    // A geometric/kernel failure invalidates final Body truth and blocks all
    // later active Features. H7 may retain only the same-revision upstream
    // prefix for presentation; it is not final Body truth and is never
    // consumed downstream.
    auto failed =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
                feature(
                    id2,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    99.0),
                feature(
                    id3,
                    fixture.profile_id,
                    part::ExtrudeOperation::cut,
                    2.0),
            },
            cursor);
    const auto failed_eval =
        part::evaluatePart(
            failed,
            kernel);
    CHECK(
        failed_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        failed_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            failed);
    CHECK(
        failed_eval.features[1].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            no_effect);
    CHECK(
        failed_eval.features[2].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        failed_eval.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(failed_eval.body_solid == nullptr);
    CHECK(failed_eval.resolved_prefix_solid != nullptr);
    CHECK(failed_eval.resolved_prefix_topology.has_value());
    CHECK(
        failed_eval.resolved_prefix_topology->stage.feature_id ==
        std::optional<part::FeatureId>{id1});
    CHECK(!failed_eval.current_topology.has_value());
    CHECK(
        failed_eval.current_face_references.empty());

    // PM-02A fail-closed admission: a provider result whose declared
    // unique Face count does not match its current Face inventory cannot
    // become Body truth even when the modeling operation itself returned ok.
    kernel.corrupt_topology_inventory = true;
    auto corrupt_topology =
        withFeatures(
            fixture.document,
            {
                feature(
                    id1,
                    fixture.profile_id,
                    part::ExtrudeOperation::add,
                    10.0),
            },
            cursor);
    const auto corrupt_eval =
        part::evaluatePart(
            corrupt_topology,
            kernel);
    CHECK(
        corrupt_eval.body_status ==
        part::BodyEvaluationStatus::
            unavailable);
    CHECK(corrupt_eval.body_solid == nullptr);
    CHECK(!corrupt_eval.current_topology.has_value());
    CHECK(corrupt_eval.resolved_prefix_solid == nullptr);
    CHECK(!corrupt_eval.resolved_prefix_topology.has_value());
    CHECK(
        corrupt_eval.features.size() == 1U);
    CHECK(
        corrupt_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            failed);
    CHECK(
        corrupt_eval.features[0].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            topology_integrity_failure);
    CHECK(corrupt_eval.features[0].result_solid == nullptr);
    CHECK(!corrupt_eval.features[0].result_topology.has_value());
    kernel.corrupt_topology_inventory = false;

    // A deleted Profile leaves repairable authored Feature intent but blocks
    // evaluation rather than corrupting the Part.
    auto missing_state =
        valid.state();
    missing_state.profiles.clear();
    auto missing_restore =
        part::PartDocument::restore(
            valid.documentId(),
            std::move(missing_state),
            valid.revision());
    CHECK(missing_restore.ok());
    const auto missing_eval =
        part::evaluatePart(
            *missing_restore.document,
            kernel);
    CHECK(
        missing_eval.features[0].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(
        missing_eval.features[0].diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_profile);
    CHECK(
        missing_eval.features[1].status ==
        part::FeatureEvaluationStatus::
            blocked);
    CHECK(missing_eval.body_solid == nullptr);
    CHECK(missing_eval.resolved_prefix_solid == nullptr);

    std::cout
        << "PM01C_EVALUATOR_PASS"
        << " stale_last_good=0"
        << " resolved_prefix_presentation=1"
        << " topology_catalog=1"
        << " topology_integrity_fail_closed=1"
        << " restart_after_failure=0\n";
    return EXIT_SUCCESS;
}
