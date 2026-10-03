#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

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

    const auto sketch =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::
                    xy_plane});
    CHECK(sketch.ok() && sketch.sketch_id);

    const auto rectangle =
        session.execute(
            application::AddSketchRectangleCommand{
                *sketch.sketch_id,
                session.document().revision(),
                {0.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* source =
        session.document().findSketch(
            *sketch.sketch_id);
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
                *sketch.sketch_id,
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

    std::cout
        << "PM01C_FEATURE_COMMANDS_PASS"
        << " rejected_finish_mutations=0"
        << " feature_id_reuse=0\n";
    return EXIT_SUCCESS;
}
