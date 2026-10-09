#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01C feature commands CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class FakeSolid final
    : public kernel::RuntimeSolid {
public:
    double last_span{};
};

class FakeKernel final
    : public kernel::ISolidModelingKernel {
public:
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

        const double span =
            input.end_offset_mm -
            input.start_offset_mm;
        if (span == 99.0) {
            result.status =
                kernel::SolidModelingStatus::
                    no_effect;
            return result;
        }

        if (input.operation ==
                kernel::SolidBooleanOperation::cut &&
            upstream != nullptr) {
            const auto* source =
                dynamic_cast<
                    const FakeSolid*>(
                    upstream.get());
            if (source == nullptr) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_mismatch;
                return result;
            }
            if (source->last_span == 20.0 &&
                span == 5.0) {
                result.status =
                    kernel::SolidModelingStatus::
                        provider_failure;
                return result;
            }
        }

        auto solid =
            std::make_shared<FakeSolid>();
        solid->last_span = span;
        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid = std::move(solid);
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }
};

struct Fixture final {
    application::DocumentSession session;
    part::ProfileId profile_id;

    Fixture(
        application::DocumentSession&& source,
        part::ProfileId profile)
        : session{std::move(source)},
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

    const auto* source =
        session.document().findSketch(
            *sketch_created.sketch_id);
    CHECK(source != nullptr);
    const auto analysis =
        sketch::analyzeRegions(
            source->model);
    CHECK(analysis.complete());
    CHECK(analysis.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            analysis.regions.front());
    CHECK(intent);

    const auto profile =
        session.execute(
            application::CreateProfileCommand{
                *sketch_created.sketch_id,
                session.document().revision(),
                *intent});
    CHECK(profile.ok() && profile.profile_id);

    return {
        std::move(session),
        *profile.profile_id};
}

part::ExtrudeExtent oneSide(double distance) {
    return part::OneSidedExtrudeExtent{
        core::LengthValue{distance},
        false};
}


void runProjection00AProfileRepairEvidence() {
    // A0-A6 expected existing CAD semantics; report unsupported repair paths.\n    // Characterize existing command semantics only: no projected entities,
    // no private document mutation and no new Profile/Extrude implementation.
    FakeKernel kernel;
    auto fixture = makeFixture();
    auto& session = fixture.session;
    const auto profile_id = fixture.profile_id;
    const auto* initial_profile = session.document().findProfile(profile_id);
    CHECK(initial_profile != nullptr);
    const auto sketch_id = initial_profile->source_sketch_id;
    const auto* source = session.document().findSketch(sketch_id);
    CHECK(source != nullptr);
    const auto rectangle_lines = source->model.state().lines;
    CHECK(rectangle_lines.size() == 4U);

    // A0: ProfileId/FeatureId are stable authored identities.
    const auto add = session.execute(
        application::CreateExtrudeFeatureCommand{
            profile_id,
            session.document().revision(),
            part::ExtrudeOperation::add,
            oneSide(9.0),
            "Projection 00A Base"},
        kernel);
    CHECK(add.ok() && add.feature_id);
    const auto feature_id = *add.feature_id;
    const auto baseline = part::evaluatePart(session.document(), kernel);
    CHECK(baseline.body_status == part::BodyEvaluationStatus::up_to_date);
    CHECK(session.document().findProfile(profile_id) != nullptr);
    CHECK(session.document().findFeature(feature_id) != nullptr);

    // A1: alter rectangle width without changing any Sketch EntityId.
    std::vector<application::SketchLineGeometryUpdate> updates;
    for (const auto& line : rectangle_lines) {
        auto start = line.start;
        auto end = line.end;
        if (start.u == 40.0) { start.u = 55.0; }
        if (end.u == 40.0) { end.u = 55.0; }
        updates.push_back({line.id, start, end});
    }
    const auto change = session.execute(
        application::UpdateSketchLinesCommand{
            sketch_id, session.document().revision(), updates});
    CHECK(change.ok() && change.changed);
    CHECK(session.document().findProfile(profile_id)->id == profile_id);
    CHECK(session.document().findFeature(feature_id)->id == feature_id);
    const auto moved = part::evaluatePart(session.document(), kernel);
    CHECK(moved.body_status == part::BodyEvaluationStatus::up_to_date);
    CHECK(session.undo().changed);
    CHECK(session.redo().changed);
    CHECK(part::evaluatePart(session.document(), kernel).body_status ==
          part::BodyEvaluationStatus::up_to_date);

    // A2: replace one semantic EntityId by two geometrically matching
    // halves. A rejected erase must leave the entire document untouched.
    const auto* changed_sketch = session.document().findSketch(sketch_id);
    CHECK(changed_sketch != nullptr);
    const auto lines = changed_sketch->model.state().lines;
    const auto target = std::find_if(
        lines.begin(), lines.end(),
        [](const sketch::SketchLineState& line) {
            return line.start.u == 55.0 && line.end.u == 55.0;
        });
    CHECK(target != lines.end());
    const auto deleted_id = target->id;
    const auto first = target->start;
    const auto last = target->end;
    const sketch::Point2 mid{55.0, 15.0};
    const auto before_erase = session.document().state();
    const auto erased = session.execute(
        application::EraseSketchEntityCommand{sketch_id, deleted_id});
    if (!erased.ok()) {
        CHECK(!erased.changed);
        CHECK(session.document().state() == before_erase);
        std::cerr << "PROJECTION00A_A2_ERASE_REJECTED_NO_MUTATION: "
                  << erased.diagnostic.message << '\n';
    }
    // The proposed repair path requires authorable, dangling intent. If
    // current SS2 rejects the erase, leave this strict RED as architecture
    // evidence rather than silently skipping all later repair assertions.
    CHECK(erased.ok() && erased.changed);
    CHECK(session.document().findProfile(profile_id) != nullptr);
    CHECK(session.document().findFeature(feature_id) != nullptr);

    const auto* missing_sketch = session.document().findSketch(sketch_id);
    const auto missing_profile = part::resolveProfileRegionIntent(
        missing_sketch->model,
        session.document().findProfile(profile_id)->region_intent);
    CHECK(missing_profile.status ==
          part::ProfileIntentResolutionStatus::missing_source_entity);
    CHECK(part::evaluatePart(session.document(), kernel).body_status !=
          part::BodyEvaluationStatus::up_to_date);

    const auto half1 = session.execute(
        application::AddSketchLineCommand{
            sketch_id, first, mid, sketch::EntityRole::regular});
    const auto half2 = session.execute(
        application::AddSketchLineCommand{
            sketch_id, mid, last, sketch::EntityRole::regular});
    CHECK(half1.ok() && half1.entity_id);
    CHECK(half2.ok() && half2.entity_id);
    CHECK(*half1.entity_id != deleted_id);
    CHECK(*half2.entity_id != deleted_id);

    const auto* repaired_sketch = session.document().findSketch(sketch_id);
    CHECK(repaired_sketch != nullptr);
    CHECK(repaired_sketch->model.entityCount() == 5U);
    // A6: even geometrically equivalent replacement cannot rebind an
    // authored Profile without a matching semantic EntityId.
    CHECK(part::resolveProfileRegionIntent(
              repaired_sketch->model,
              session.document().findProfile(profile_id)->region_intent)
              .status ==
          part::ProfileIntentResolutionStatus::missing_source_entity);

    const auto regions = sketch::analyzeRegions(repaired_sketch->model);
    if (!regions.complete() || regions.regions.size() != 1U) {
        std::cout
            << "PROJECTION00A_A3_NEW_REGION_UNAVAILABLE"
            << " regions=" << regions.regions.size()
            << " complete=" << regions.complete() << '\n';
        // A missing valid region is a real evidence RED, not a skipped PASS.
        // No production code changes are allowed by the 00A Work Contract.
        CHECK(regions.complete() && regions.regions.size() == 1U);
    }
    const auto replacement_intent = part::makeProfileRegionIntent(
        regions.regions.front());
    CHECK(replacement_intent);

    // A3: repair the SAME ProfileId, without generating a replacement.
    const auto replace = session.execute(
        application::ReplaceProfileRegionIntentCommand{
            profile_id, session.document().revision(), *replacement_intent});
    CHECK(replace.ok() && replace.changed);
    CHECK(session.document().findProfile(profile_id)->id == profile_id);
    CHECK(session.document().findFeature(feature_id)->id == feature_id);
    CHECK(part::evaluatePart(session.document(), kernel).body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(session.undo().changed);
    const auto* undone_sketch = session.document().findSketch(sketch_id);
    CHECK(part::resolveProfileRegionIntent(
              undone_sketch->model,
              session.document().findProfile(profile_id)->region_intent)
              .status ==
          part::ProfileIntentResolutionStatus::missing_source_entity);
    CHECK(session.redo().changed);
    CHECK(part::evaluatePart(session.document(), kernel).body_status ==
          part::BodyEvaluationStatus::up_to_date);

    // A4: separately create Profile002, then repoint the same Extrude001.
    const auto second_profile = session.execute(
        application::CreateProfileCommand{
            sketch_id, session.document().revision(), *replacement_intent});
    CHECK(second_profile.ok() && second_profile.profile_id);
    CHECK(*second_profile.profile_id != profile_id);
    const auto reassign = session.execute(
        application::EditExtrudeFeatureCommand{
            feature_id,
            session.document().revision(),
            *second_profile.profile_id,
            part::ExtrudeOperation::add,
            oneSide(9.0),
            "Projection 00A Reassigned"},
        kernel);
    CHECK(reassign.ok() && reassign.changed);
    CHECK(session.document().findFeature(feature_id)->id == feature_id);
    CHECK(part::evaluatePart(session.document(), kernel).body_status ==
          part::BodyEvaluationStatus::up_to_date);
    CHECK(session.undo().changed);
    CHECK(session.redo().changed);
    CHECK(session.document().findFeature(feature_id)->id == feature_id);
    CHECK(part::evaluatePart(session.document(), kernel).body_status ==
          part::BodyEvaluationStatus::up_to_date);

    // A4 negative: an invalid Profile reassignment must not mutate the
    // authored Part document or spend a new durable FeatureId.
    const auto valid_state = session.document().state();
    const auto invalid_reassignment = session.execute(
        application::EditExtrudeFeatureCommand{
            feature_id,
            session.document().revision(),
            part::ProfileId{},
            part::ExtrudeOperation::add,
            oneSide(9.0),
            "Invalid Profile reassignment"},
        kernel);
    CHECK(!invalid_reassignment.ok() && !invalid_reassignment.changed);
    CHECK(session.document().state() == valid_state);
    CHECK(session.document().findFeature(feature_id)->id == feature_id);
    std::cout
        << "PROJECTION00A_A0_A4_A6_PASS"
        << " linked_source_rebinding=0"
        << " retained_profile_id=1 retained_feature_id=1\n";
}

} // namespace

int main() {
    FakeKernel kernel;

    // First Add and following Cut commit through the semantic session.
    auto fixture = makeFixture();
    const auto undo_before =
        fixture.session.undoDepth();
    const auto add =
        fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                fixture.profile_id,
                fixture.session.document().revision(),
                part::ExtrudeOperation::add,
                oneSide(10.0),
                {}},
            kernel);
    CHECK(add.ok());
    CHECK(add.changed);
    CHECK(add.feature_id);
    CHECK(add.feature_id->serialized() == "1");
    CHECK(
        fixture.session.undoDepth() ==
        undo_before + 1U);
    CHECK(
        !fixture.session.document()
             .profilePresentationVisible(
                 fixture.profile_id));

    const auto cut =
        fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                fixture.profile_id,
                fixture.session.document().revision(),
                part::ExtrudeOperation::cut,
                oneSide(5.0),
                "Cut"},
            kernel);
    CHECK(cut.ok());
    CHECK(cut.feature_id);
    CHECK(cut.feature_id->serialized() == "2");

    const auto before_edit_revision =
        fixture.session.document().revision();
    const auto edit =
        fixture.session.execute(
            application::EditExtrudeFeatureCommand{
                *add.feature_id,
                before_edit_revision,
                fixture.profile_id,
                part::ExtrudeOperation::add,
                oneSide(20.0),
                "Base"},
            kernel);
    CHECK(edit.ok());
    CHECK(edit.changed);

    // The edited target is valid, so the edit commits even though its
    // downstream Cut now fails under this deterministic fake provider.
    const auto after_edit =
        part::evaluatePart(
            fixture.session.document(),
            kernel);
    CHECK(
        after_edit.features[0].status ==
        part::FeatureEvaluationStatus::
            up_to_date);
    CHECK(
        after_edit.features[1].status ==
        part::FeatureEvaluationStatus::
            failed);
    CHECK(
        after_edit.body_status ==
        part::BodyEvaluationStatus::
            unavailable);

    // Failed Finish is non-authoring: no revision, no Undo, no consumed ID.
    auto rejected_fixture = makeFixture();
    const auto rejected_revision =
        rejected_fixture.session
            .document().revision();
    const auto rejected_undo =
        rejected_fixture.session
            .undoDepth();

    const auto first_cut =
        rejected_fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                rejected_fixture.profile_id,
                rejected_revision,
                part::ExtrudeOperation::cut,
                oneSide(5.0),
                {}},
            kernel);
    CHECK(!first_cut.ok());
    CHECK(!first_cut.changed);
    CHECK(!first_cut.feature_id);
    CHECK(
        first_cut.evaluation_diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_upstream_body);
    CHECK(
        rejected_fixture.session
            .document().revision() ==
        rejected_revision);
    CHECK(
        rejected_fixture.session.undoDepth() ==
        rejected_undo);

    const auto no_effect =
        rejected_fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                rejected_fixture.profile_id,
                rejected_revision,
                part::ExtrudeOperation::add,
                oneSide(99.0),
                {}},
            kernel);
    CHECK(!no_effect.ok());
    CHECK(
        no_effect.evaluation_diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            no_effect);
    CHECK(
        rejected_fixture.session
            .document().revision() ==
        rejected_revision);

    const auto accepted =
        rejected_fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                rejected_fixture.profile_id,
                rejected_revision,
                part::ExtrudeOperation::add,
                oneSide(10.0),
                {}},
            kernel);
    CHECK(accepted.ok());
    CHECK(accepted.feature_id);
    CHECK(
        accepted.feature_id->serialized() ==
        "1");

    // Undo + branch never reuses a committed FeatureId.
    CHECK(rejected_fixture.session.undo().changed);
    const auto branched =
        rejected_fixture.session.execute(
            application::CreateExtrudeFeatureCommand{
                rejected_fixture.profile_id,
                rejected_fixture.session
                    .document().revision(),
                part::ExtrudeOperation::add,
                oneSide(12.0),
                {}},
            kernel);
    CHECK(branched.ok());
    CHECK(branched.feature_id);
    CHECK(
        branched.feature_id->serialized() ==
        "2");
    CHECK(!rejected_fixture.session.canRedo());

    // Suppress is authored intent, distinct from visibility and Delete.
    const auto suppressed =
        rejected_fixture.session.execute(
            application::SetFeatureSuppressedCommand{
                *branched.feature_id,
                rejected_fixture.session
                    .document().revision(),
                true});
    CHECK(suppressed.ok());
    CHECK(suppressed.changed);
    CHECK(
        rejected_fixture.session.document()
            .findFeature(
                *branched.feature_id)
            ->suppressed);
    CHECK(
        rejected_fixture.session.document()
            .profilePresentationVisible(
                rejected_fixture.profile_id));

    const auto unsuppressed =
        rejected_fixture.session.execute(
            application::SetFeatureSuppressedCommand{
                *branched.feature_id,
                rejected_fixture.session
                    .document().revision(),
                false});
    CHECK(unsuppressed.ok());
    CHECK(
        !rejected_fixture.session.document()
             .profilePresentationVisible(
                 rejected_fixture.profile_id));

    const auto deleted =
        rejected_fixture.session.execute(
            application::DeleteFeatureCommand{
                *branched.feature_id,
                rejected_fixture.session
                    .document().revision()});
    CHECK(deleted.ok());
    CHECK(deleted.changed);
    CHECK(
        rejected_fixture.session.document()
            .findFeature(
                *branched.feature_id) ==
        nullptr);
    CHECK(
        rejected_fixture.session.document()
            .profilePresentationVisible(
                rejected_fixture.profile_id));

    runProjection00AProfileRepairEvidence();

    std::cout
        << "PM01C_FEATURE_COMMANDS_PASS"
        << " rejected_finish_mutations=0"
        << " feature_id_reuse=0\n";
    return EXIT_SUCCESS;
}
