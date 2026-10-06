#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/revolve_draft.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04D1 Revolve draft CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class FakeSolid final
    : public kernel::RuntimeSolid {};

class FakeKernel final
    : public kernel::ISolidModelingKernel {
public:
    std::vector<kernel::AngularRevolveInput>
        revolve_inputs;
    std::vector<kernel::AngularRevolveInput>
        preview_inputs;
    std::vector<bool> revolve_had_upstream;
    std::vector<bool> preview_had_upstream;

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
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
        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }

    kernel::SolidModelingResult revolve(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
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

        revolve_inputs.push_back(input);
        revolve_had_upstream.push_back(
            upstream != nullptr);

        result.status =
            kernel::SolidModelingStatus::ok;
        result.solid =
            std::make_shared<FakeSolid>();
        result.brep_valid = true;
        result.solid_count = 1U;
        return result;
    }

    kernel::SolidPresentationResult
    revolvePreviewMesh(
        const kernel::AngularRevolveInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        if (!input.valid() ||
            (input.operation ==
                 kernel::SolidBooleanOperation::cut &&
             upstream == nullptr)) {
            return {
                kernel::SolidPresentationStatus::
                    invalid_input,
                {}};
        }

        preview_inputs.push_back(input);
        preview_had_upstream.push_back(
            upstream != nullptr);

        kernel::SolidPresentationMesh mesh;
        mesh.triangles.push_back(
            {
                {0.0, 0.0, 0.0},
                {10.0, 0.0, 0.0},
                {0.0, 10.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 1.0}});
        return {
            kernel::SolidPresentationStatus::ok,
            std::move(mesh)};
    }
};

struct Fixture final {
    application::DocumentSession session;
    part::ProfileId profile_id;
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
                {10.0, 0.0},
                {40.0, 30.0},
                sketch::EntityRole::regular,
                false});
    CHECK(rectangle.ok());

    const auto* source =
        session.document().findSketch(
            *sketch.sketch_id);
    CHECK(source != nullptr);
    const auto regions =
        sketch::analyzeRegions(source->model);
    CHECK(regions.complete());
    CHECK(regions.regions.size() == 1U);
    const auto intent =
        part::makeProfileRegionIntent(
            regions.regions.front());
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

part::AxisReference originY() {
    return part::AxisReference{
        part::BuiltinOriginAxisReference{
            core::BuiltinReferenceRole::
                y_axis}};
}

bool near(double first, double second) {
    return std::abs(first - second) <
           1.0e-12;
}

} // namespace

int main() {
    FakeKernel kernel;
    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;

    // Command-first starts incomplete but already carries the accepted
    // default authored parameter set: Add / OneSide / 360 / Reverse=false.
    auto fixture = makeFixture();
    const auto base_revision =
        fixture.session.document().revision();
    const auto base_undo =
        fixture.session.undoDepth();

    auto draft =
        application::RevolveDraft::beginCreate(
            fixture.session);
    CHECK(!draft.valid());
    CHECK(!draft.profileId());
    CHECK(!draft.axis());
    CHECK(
        draft.operation() ==
        part::RevolveOperation::add);
    CHECK(
        draft.extentMode() ==
        application::RevolveDraftExtentMode::
            one_side);
    CHECK(near(
        draft.angle().radians,
        full_turn));
    CHECK(!draft.reversed());
    CHECK(draft.generation() == 1U);

    const auto incomplete =
        fixture.session.evaluateRevolveDraft(
            draft,
            kernel);
    CHECK(!incomplete.committable());
    CHECK(
        incomplete.status ==
        application::RevolveDraftEvaluationStatus::
            incomplete_draft);
    CHECK(kernel.revolve_inputs.empty());
    CHECK(kernel.preview_inputs.empty());
    CHECK(
        fixture.session.document().revision() ==
        base_revision);
    CHECK(
        fixture.session.undoDepth() ==
        base_undo);

    CHECK(draft.setProfile(
        fixture.profile_id));
    CHECK(draft.setAxis(originY()));
    CHECK(draft.valid());
    const auto generation_ready =
        draft.generation();
    CHECK(draft.setAngle(
        core::AngleValue{full_turn}));
    CHECK(
        draft.generation() ==
        generation_ready);

    const auto preview =
        fixture.session.evaluateRevolveDraft(
            draft,
            kernel);
    CHECK(preview.committable());
    CHECK(preview.previewSolidAvailable());
    CHECK(preview.body_solid != nullptr);
    CHECK(preview.preview_delta_mesh);
    CHECK(preview.preview_delta_mesh->valid());
    CHECK(kernel.revolve_inputs.size() == 1U);
    CHECK(kernel.preview_inputs.size() == 1U);
    CHECK(
        kernel.revolve_inputs.back() ==
        kernel.preview_inputs.back());
    CHECK(!kernel.revolve_had_upstream.back());
    CHECK(!kernel.preview_had_upstream.back());
    CHECK(
        kernel.preview_inputs.back().fullTurn());
    CHECK(
        fixture.session.document().revision() ==
        base_revision);
    CHECK(
        fixture.session.undoDepth() ==
        base_undo);
    CHECK(
        fixture.session.document()
            .body().features.empty());

    // Any draft mutation invalidates the previous successful evaluation.
    CHECK(draft.setReversed(true));
    const auto stale_finish =
        application::finishRevolveDraft(
            fixture.session,
            draft,
            preview,
            kernel);
    CHECK(!stale_finish.ok());
    CHECK(
        stale_finish.status ==
        application::RevolveDraftFinishStatus::
            stale_evaluation);
    CHECK(
        fixture.session.document()
            .body().features.empty());

    const auto current_preview =
        fixture.session.evaluateRevolveDraft(
            draft,
            kernel);
    CHECK(current_preview.committable());
    const auto undo_before_finish =
        fixture.session.undoDepth();
    const auto finish =
        application::finishRevolveDraft(
            fixture.session,
            draft,
            current_preview,
            kernel);
    CHECK(finish.ok());
    CHECK(finish.changed);
    CHECK(finish.feature_id);
    CHECK(
        finish.feature_id->serialized() ==
        "1");
    CHECK(
        fixture.session.undoDepth() ==
        undo_before_finish + 1U);
    CHECK(
        fixture.session.document()
            .body().features.size() == 1U);

    const auto* authored =
        fixture.session.document().findFeature(
            *finish.feature_id);
    CHECK(authored != nullptr);
    const auto* authored_revolve =
        std::get_if<part::RevolveFeature>(
            &authored->definition);
    CHECK(authored_revolve != nullptr);
    CHECK(
        authored_revolve->profile_id ==
        fixture.profile_id);
    CHECK(
        authored_revolve->axis ==
        originY());
    CHECK(
        std::get<
            part::OneSidedRevolveExtent>(
                authored_revolve->extent)
            .reversed);

    // Edit re-enters the same draft and preserves FeatureId. Midplane has no
    // Reverse and uses total angle semantics.
    auto edit =
        application::RevolveDraft::beginEdit(
            fixture.session,
            *finish.feature_id);
    CHECK(edit);
    CHECK(edit->valid());
    CHECK(
        edit->featureId() ==
        finish.feature_id);
    CHECK(
        edit->setExtentMode(
            application::RevolveDraftExtentMode::
                midplane));
    CHECK(
        edit->setAngle(
            core::AngleValue{
                std::numbers::pi_v<double>}));
    CHECK(!edit->setReversed(true));
    CHECK(!edit->reversed());

    const auto edit_preview =
        fixture.session.evaluateRevolveDraft(
            *edit,
            kernel);
    CHECK(edit_preview.committable());
    CHECK(edit_preview.previewSolidAvailable());
    const auto edit_undo =
        fixture.session.undoDepth();
    const auto edit_finish =
        application::finishRevolveDraft(
            fixture.session,
            *edit,
            edit_preview,
            kernel);
    CHECK(edit_finish.ok());
    CHECK(
        edit_finish.feature_id ==
        finish.feature_id);
    CHECK(
        fixture.session.undoDepth() ==
        edit_undo + 1U);

    const auto* edited =
        fixture.session.document().findFeature(
            *finish.feature_id);
    CHECK(edited != nullptr);
    const auto* edited_revolve =
        std::get_if<part::RevolveFeature>(
            &edited->definition);
    CHECK(edited_revolve != nullptr);
    const auto* midplane =
        std::get_if<
            part::MidplaneRevolveExtent>(
                &edited_revolve->extent);
    CHECK(midplane != nullptr);
    CHECK(near(
        midplane->total_angle.radians,
        std::numbers::pi_v<double>));

    // A revision change after preview rejects Finish with zero extra mutation.
    auto stale_fixture = makeFixture();
    auto stale =
        application::RevolveDraft::beginCreate(
            stale_fixture.session);
    CHECK(stale.setProfile(
        stale_fixture.profile_id));
    CHECK(stale.setAxis(originY()));
    const auto stale_preview =
        stale_fixture.session.evaluateRevolveDraft(
            stale,
            kernel);
    CHECK(stale_preview.committable());

    auto properties =
        stale_fixture.session.document()
            .properties();
    properties.title = "revision changed";
    const auto changed =
        stale_fixture.session.execute(
            application::SetDocumentPropertiesCommand{
                std::move(properties)});
    CHECK(changed.ok() && changed.changed);
    const auto undo_after_change =
        stale_fixture.session.undoDepth();

    const auto stale_context =
        application::finishRevolveDraft(
            stale_fixture.session,
            stale,
            stale_preview,
            kernel);
    CHECK(!stale_context.ok());
    CHECK(
        stale_context.status ==
        application::RevolveDraftFinishStatus::
            stale_context);
    CHECK(
        stale_fixture.session.undoDepth() ==
        undo_after_change);
    CHECK(
        stale_fixture.session.document()
            .body().features.empty());

    // First Feature Cut is blocked before provider invocation.
    auto cut_fixture = makeFixture();
    auto first_cut =
        application::RevolveDraft::beginCreate(
            cut_fixture.session);
    CHECK(first_cut.setProfile(
        cut_fixture.profile_id));
    CHECK(first_cut.setAxis(originY()));
    CHECK(
        first_cut.setOperation(
            part::RevolveOperation::cut));
    const auto cut_preview =
        cut_fixture.session.evaluateRevolveDraft(
            first_cut,
            kernel);
    CHECK(!cut_preview.committable());
    CHECK(
        cut_preview.status ==
        application::RevolveDraftEvaluationStatus::
            target_failed);
    CHECK(
        cut_preview.evaluation_diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_upstream_body);
    CHECK(
        cut_fixture.session.document()
            .body().features.empty());

    // With an upstream Body, Cut modeling and preview both receive that exact
    // prefix runtime solid.
    auto cut_with_body = makeFixture();
    const auto base =
        cut_with_body.session.execute(
            application::CreateExtrudeFeatureCommand{
                cut_with_body.profile_id,
                cut_with_body.session.document().revision(),
                part::ExtrudeOperation::add,
                part::OneSidedExtrudeExtent{
                    core::LengthValue{10.0},
                    false},
                "Base"},
            kernel);
    CHECK(base.ok() && base.feature_id);

    auto cut =
        application::RevolveDraft::beginCreate(
            cut_with_body.session);
    CHECK(cut.setProfile(
        cut_with_body.profile_id));
    CHECK(cut.setAxis(originY()));
    CHECK(
        cut.setOperation(
            part::RevolveOperation::cut));
    CHECK(
        cut.setAngle(
            core::AngleValue{
                std::numbers::pi_v<double> /
                2.0}));

    const auto cut_with_body_preview =
        cut_with_body.session.evaluateRevolveDraft(
            cut,
            kernel);
    CHECK(cut_with_body_preview.committable());
    CHECK(
        cut_with_body_preview.previewSolidAvailable());
    CHECK(!kernel.revolve_had_upstream.empty());
    CHECK(!kernel.preview_had_upstream.empty());
    CHECK(kernel.revolve_had_upstream.back());
    CHECK(kernel.preview_had_upstream.back());
    CHECK(
        kernel.revolve_inputs.back() ==
        kernel.preview_inputs.back());
    CHECK(
        kernel.preview_inputs.back().operation ==
        kernel::SolidBooleanOperation::cut);

    const auto cut_finish =
        application::finishRevolveDraft(
            cut_with_body.session,
            cut,
            cut_with_body_preview,
            kernel);
    CHECK(cut_finish.ok());
    CHECK(cut_finish.feature_id);
    CHECK(
        cut_finish.feature_id->serialized() ==
        "2");

    return EXIT_SUCCESS;
}
