#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/application/extrude_draft.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/profile.hpp>
#include <simplesolid2/sketch/region_analysis.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-01E Extrude draft CHECK failed at line "
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
    std::size_t extrude_calls{};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput& input,
        kernel::RuntimeSolidHandle upstream = {}) noexcept override {
        ++extrude_calls;
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
        if (upstream != nullptr &&
            dynamic_cast<const FakeSolid*>(
                upstream.get()) == nullptr) {
            result.status =
                kernel::SolidModelingStatus::
                    provider_mismatch;
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
                {0.0, 0.0},
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

} // namespace

int main() {
    FakeKernel kernel;

    // Create preview is non-authoring and does not consume FeatureId.
    auto fixture = makeFixture();
    const auto base_revision =
        fixture.session.document().revision();
    const auto base_undo =
        fixture.session.undoDepth();

    auto draft =
        application::ExtrudeDraft::beginCreate(
            fixture.session,
            fixture.profile_id);
    CHECK(draft);
    CHECK(!draft->valid());
    CHECK(draft->generation() == 1U);

    auto incomplete =
        fixture.session.evaluateExtrudeDraft(
            *draft,
            kernel);
    CHECK(!incomplete.committable());
    CHECK(
        incomplete.status ==
        application::ExtrudeDraftEvaluationStatus::
            invalid_draft);
    CHECK(kernel.extrude_calls == 0U);
    CHECK(
        fixture.session.document().revision() ==
        base_revision);
    CHECK(
        fixture.session.undoDepth() ==
        base_undo);

    CHECK(
        draft->setDistance(
            core::LengthValue{10.0}));
    CHECK(draft->valid());
    CHECK(draft->generation() == 2U);

    const auto preview =
        fixture.session.evaluateExtrudeDraft(
            *draft,
            kernel);
    CHECK(preview.committable());
    CHECK(preview.previewSolidAvailable());
    CHECK(preview.body_solid != nullptr);
    CHECK(preview.preview_tool_solid != nullptr);
    CHECK(
        preview.body_solid !=
        preview.preview_tool_solid);
    CHECK(
        fixture.session.document().revision() ==
        base_revision);
    CHECK(
        fixture.session.undoDepth() ==
        base_undo);
    CHECK(
        fixture.session.document()
            .body().features.empty());

    // A draft change invalidates the older successful evaluation.
    CHECK(draft->setReversed(true));
    const auto stale_finish =
        application::finishExtrudeDraft(
            fixture.session,
            *draft,
            preview,
            kernel);
    CHECK(!stale_finish.ok());
    CHECK(
        stale_finish.status ==
        application::ExtrudeDraftFinishStatus::
            stale_evaluation);
    CHECK(
        fixture.session.document()
            .body().features.empty());
    CHECK(
        fixture.session.undoDepth() ==
        base_undo);

    const auto current_preview =
        fixture.session.evaluateExtrudeDraft(
            *draft,
            kernel);
    CHECK(current_preview.committable());
    const auto finish =
        application::finishExtrudeDraft(
            fixture.session,
            *draft,
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
        base_undo + 1U);

    // Multiple preview evaluations did not consume the durable first ID.
    CHECK(
        fixture.session.document()
            .body().features.front().id ==
        *finish.feature_id);

    // A metadata-only draft change also invalidates the older evaluation.
    auto renamed =
        application::ExtrudeDraft::beginEdit(
            fixture.session,
            *finish.feature_id);
    CHECK(renamed);
    const auto rename_preview =
        fixture.session.evaluateExtrudeDraft(
            *renamed,
            kernel);
    CHECK(rename_preview.committable());
    CHECK(renamed->setName("Renamed Extrude"));
    const auto stale_name_finish =
        application::finishExtrudeDraft(
            fixture.session,
            *renamed,
            rename_preview,
            kernel);
    CHECK(!stale_name_finish.ok());
    CHECK(
        stale_name_finish.status ==
        application::ExtrudeDraftFinishStatus::
            stale_evaluation);

    // Edit keeps FeatureId; Midplane has no authored Reverse.
    auto edit =
        application::ExtrudeDraft::beginEdit(
            fixture.session,
            *finish.feature_id);
    CHECK(edit);
    CHECK(edit->valid());
    CHECK(edit->featureId() ==
          finish.feature_id);
    CHECK(
        edit->setExtentMode(
            application::
                ExtrudeDraftExtentMode::midplane));
    CHECK(
        edit->setDistance(
            core::LengthValue{20.0}));
    CHECK(!edit->setReversed(true));
    CHECK(!edit->reversed());

    const auto edit_preview =
        fixture.session.evaluateExtrudeDraft(
            *edit,
            kernel);
    CHECK(edit_preview.committable());
    CHECK(edit_preview.previewSolidAvailable());

    const auto undo_before_edit =
        fixture.session.undoDepth();
    const auto edit_finish =
        application::finishExtrudeDraft(
            fixture.session,
            *edit,
            edit_preview,
            kernel);
    CHECK(edit_finish.ok());
    CHECK(edit_finish.changed);
    CHECK(
        edit_finish.feature_id ==
        finish.feature_id);
    CHECK(
        fixture.session.undoDepth() ==
        undo_before_edit + 1U);

    // Revision drift rejects Finish even with a previously successful preview.
    auto stale_fixture = makeFixture();
    auto stale_draft =
        application::ExtrudeDraft::beginCreate(
            stale_fixture.session,
            stale_fixture.profile_id);
    CHECK(stale_draft);
    CHECK(
        stale_draft->setDistance(
            core::LengthValue{5.0}));
    const auto stale_preview =
        stale_fixture.session.evaluateExtrudeDraft(
            *stale_draft,
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

    const auto stale_context_finish =
        application::finishExtrudeDraft(
            stale_fixture.session,
            *stale_draft,
            stale_preview,
            kernel);
    CHECK(!stale_context_finish.ok());
    CHECK(
        stale_context_finish.status ==
        application::ExtrudeDraftFinishStatus::
            stale_context);
    CHECK(
        stale_fixture.session.undoDepth() ==
        undo_after_change);
    CHECK(
        stale_fixture.session.document()
            .body().features.empty());

    // First Feature Cut is evaluable only as explicit failure and cannot Finish.
    auto cut_fixture = makeFixture();
    auto cut_draft =
        application::ExtrudeDraft::beginCreate(
            cut_fixture.session,
            cut_fixture.profile_id);
    CHECK(cut_draft);
    CHECK(
        cut_draft->setDistance(
            core::LengthValue{5.0}));
    CHECK(
        cut_draft->setOperation(
            part::ExtrudeOperation::cut));
    const auto cut_preview =
        cut_fixture.session.evaluateExtrudeDraft(
            *cut_draft,
            kernel);
    CHECK(!cut_preview.committable());
    CHECK(
        cut_preview.status ==
        application::ExtrudeDraftEvaluationStatus::
            target_failed);
    CHECK(
        cut_preview.evaluation_diagnostic ==
        part::FeatureEvaluationDiagnosticCode::
            missing_upstream_body);

    const auto cut_finish =
        application::finishExtrudeDraft(
            cut_fixture.session,
            *cut_draft,
            cut_preview,
            kernel);
    CHECK(!cut_finish.ok());
    CHECK(
        cut_fixture.session.document()
            .body().features.empty());

    return EXIT_SUCCESS;
}
