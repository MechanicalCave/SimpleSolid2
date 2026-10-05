#include <simplesolid2/application/datum_plane_draft.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-03C Datum commands/draft CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class UnexpectedSolid final : public kernel::RuntimeSolid {};

class OriginOnlyKernel final
    : public kernel::ISolidModelingKernel {
public:
    std::size_t extrude_calls{};

    kernel::SolidModelingResult extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle = {}) noexcept override {
        ++extrude_calls;
        kernel::SolidModelingResult result;
        result.status =
            kernel::SolidModelingStatus::provider_failure;
        return result;
    }
};

part::PlaneReference originPlane(
    core::BuiltinReferenceRole role) {
    return part::PlaneReference{
        part::BuiltinOriginPlaneReference{role}};
}

part::PlaneReference datumPlane(
    part::DatumId id) {
    return part::PlaneReference{
        part::DatumPlaneReference{id}};
}

application::DocumentSession makeSession() {
    return application::DocumentSession{
        {},
        part::PartDocument::create(
            core::DocumentId::generate())};
}

part::DatumId finishCreate(
    application::DocumentSession& session,
    OriginOnlyKernel& kernel,
    part::PlaneReference source,
    double offset_mm) {
    auto draft =
        application::DatumPlaneDraft::beginCreate(
            session,
            std::move(source));
    CHECK(draft.has_value());
    CHECK(
        draft->setOffset(
            core::LengthValue{offset_mm}));

    const auto evaluation =
        session.evaluateDatumPlaneDraft(
            *draft,
            kernel);
    CHECK(evaluation.committable());

    const auto finished =
        application::finishDatumPlaneDraft(
            session,
            *draft,
            evaluation,
            kernel);
    CHECK(finished.ok());
    CHECK(finished.changed);
    CHECK(finished.datum_id.has_value());
    return *finished.datum_id;
}

} // namespace

int main() {
    OriginOnlyKernel kernel;
    auto session = makeSession();

    const auto base_state =
        session.document().state();
    const auto base_revision =
        session.document().revision();
    const auto base_undo =
        session.undoDepth();

    // New draft is non-authoring, has the accepted 10 mm default and may
    // exist before a source is acquired.
    auto draft =
        application::DatumPlaneDraft::beginCreate(
            session);
    CHECK(!draft.valid());
    CHECK(draft.offset().millimetres == 10.0);
    CHECK(draft.generation() == 1U);
    CHECK(
        session.document().state() ==
        base_state);
    CHECK(
        session.document().revision() ==
        base_revision);
    CHECK(session.undoDepth() == base_undo);

    CHECK(
        draft.setSource(
            originPlane(
                core::BuiltinReferenceRole::
                    xy_plane)));
    CHECK(draft.valid());
    CHECK(draft.generation() == 2U);

    const auto first_preview =
        session.evaluateDatumPlaneDraft(
            draft,
            kernel);
    CHECK(first_preview.committable());
    CHECK(first_preview.candidate_datum_id.has_value());
    CHECK(
        first_preview.candidate_datum_id->serialized() ==
        "1");
    CHECK(first_preview.frame.has_value());
    CHECK(
        first_preview.frame->origin.z ==
        10.0);
    CHECK(kernel.extrude_calls == 0U);
    CHECK(
        session.document().state() ==
        base_state);
    CHECK(session.undoDepth() == base_undo);

    // Command Line and GUI mutate the same runtime draft. Reverse is only
    // signed-offset negation and makes the prior preview stale.
    const application::CadInputNumberFormat number_format{
        ".",
        core::LengthUnit::millimetre};
    const auto reverse =
        application::submitDatumPlaneCadInput(
            draft,
            "REVERSE",
            number_format);
    CHECK(reverse.accepted);
    CHECK(
        reverse.action ==
        application::DatumPlaneCadInputAction::none);
    CHECK(draft.offset().millimetres == -10.0);
    CHECK(draft.generation() == 3U);

    const auto stale_finish =
        application::finishDatumPlaneDraft(
            session,
            draft,
            first_preview,
            kernel);
    CHECK(!stale_finish.ok());
    CHECK(
        stale_finish.status ==
        application::DatumPlaneDraftFinishStatus::
            stale_evaluation);
    CHECK(
        session.document().datumPlanes().empty());
    CHECK(session.undoDepth() == base_undo);

    const auto offset_input =
        application::submitDatumPlaneCadInput(
            draft,
            "OFFSET -5 mm",
            number_format);
    CHECK(offset_input.accepted);
    CHECK(draft.offset().millimetres == -5.0);

    const auto current_preview =
        session.evaluateDatumPlaneDraft(
            draft,
            kernel);
    CHECK(current_preview.committable());
    CHECK(current_preview.frame.has_value());
    CHECK(
        current_preview.frame->origin.z ==
        -5.0);

    const auto finish_token =
        application::submitDatumPlaneCadInput(
            draft,
            "",
            number_format);
    CHECK(finish_token.accepted);
    CHECK(
        finish_token.action ==
        application::DatumPlaneCadInputAction::finish);

    const auto create_finish =
        application::finishDatumPlaneDraft(
            session,
            draft,
            current_preview,
            kernel);
    CHECK(create_finish.ok());
    CHECK(create_finish.changed);
    CHECK(create_finish.datum_id.has_value());
    const auto first_id =
        *create_finish.datum_id;
    CHECK(first_id.serialized() == "1");
    CHECK(session.undoDepth() == base_undo + 1U);
    CHECK(
        session.document().datumPlanes().size() ==
        1U);
    CHECK(
        session.document()
            .findDatumPlane(first_id)
            ->offset.millimetres ==
        -5.0);

    // Cancel is a draft action only: no authored state/history mutation.
    const auto state_before_cancel =
        session.document().state();
    const auto undo_before_cancel =
        session.undoDepth();
    auto cancelled =
        application::DatumPlaneDraft::beginCreate(
            session,
            originPlane(
                core::BuiltinReferenceRole::
                    xz_plane));
    CHECK(cancelled.has_value());
    const auto cancel =
        application::submitDatumPlaneCadInput(
            *cancelled,
            "CANCEL",
            number_format);
    CHECK(cancel.accepted);
    CHECK(
        cancel.action ==
        application::DatumPlaneCadInputAction::cancel);
    CHECK(
        session.document().state() ==
        state_before_cancel);
    CHECK(
        session.undoDepth() ==
        undo_before_cancel);

    // Edit preserves DatumId and one successful Finish is one history entry.
    auto edit =
        application::DatumPlaneDraft::beginEdit(
            session,
            first_id);
    CHECK(edit.has_value());
    CHECK(edit->datumId() == first_id);
    CHECK(
        edit->setOffset(
            core::LengthValue{7.0}));
    const auto edit_preview =
        session.evaluateDatumPlaneDraft(
            *edit,
            kernel);
    CHECK(edit_preview.committable());
    CHECK(
        edit_preview.candidate_datum_id ==
        first_id);

    const auto undo_before_edit =
        session.undoDepth();
    const auto edit_finish =
        application::finishDatumPlaneDraft(
            session,
            *edit,
            edit_preview,
            kernel);
    CHECK(edit_finish.ok());
    CHECK(edit_finish.changed);
    CHECK(edit_finish.datum_id == first_id);
    CHECK(
        session.undoDepth() ==
        undo_before_edit + 1U);
    CHECK(
        session.document()
            .findDatumPlane(first_id)
            ->offset.millimetres ==
        7.0);

    // A no-op edit Finish commits no new CAD history.
    auto noop_edit =
        application::DatumPlaneDraft::beginEdit(
            session,
            first_id);
    CHECK(noop_edit.has_value());
    const auto noop_preview =
        session.evaluateDatumPlaneDraft(
            *noop_edit,
            kernel);
    CHECK(noop_preview.committable());
    const auto undo_before_noop =
        session.undoDepth();
    const auto noop_finish =
        application::finishDatumPlaneDraft(
            session,
            *noop_edit,
            noop_preview,
            kernel);
    CHECK(noop_finish.ok());
    CHECK(!noop_finish.changed);
    CHECK(
        session.undoDepth() ==
        undo_before_noop);

    // Visibility is authored. Repeating the same value is a no-op.
    const auto hide =
        session.execute(
            application::
                SetDatumPlaneVisibilityCommand{
                    {first_id},
                    session.document().revision(),
                    false});
    CHECK(hide.ok());
    CHECK(hide.changed);
    CHECK(
        !session.document()
             .findDatumPlane(first_id)
             ->visible);
    const auto undo_after_hide =
        session.undoDepth();

    const auto hide_again =
        session.execute(
            application::
                SetDatumPlaneVisibilityCommand{
                    {first_id},
                    session.document().revision(),
                    false});
    CHECK(hide_again.ok());
    CHECK(!hide_again.changed);
    CHECK(
        session.undoDepth() ==
        undo_after_hide);

    // Datum dependency protects producer deletion atomically.
    const auto second_id =
        finishCreate(
            session,
            kernel,
            datumPlane(first_id),
            3.0);
    CHECK(second_id.serialized() == "2");
    const auto state_before_rejected_delete =
        session.document().state();
    const auto undo_before_rejected_delete =
        session.undoDepth();

    const auto rejected_delete =
        session.execute(
            application::DeleteDatumPlaneCommand{
                first_id,
                session.document().revision()});
    CHECK(!rejected_delete.ok());
    CHECK(!rejected_delete.changed);
    CHECK(
        session.document().state() ==
        state_before_rejected_delete);
    CHECK(
        session.undoDepth() ==
        undo_before_rejected_delete);

    // Deleting an unreferenced Datum is one normal transaction and is
    // Undo/Redo correct.
    const auto delete_second =
        session.execute(
            application::DeleteDatumPlaneCommand{
                second_id,
                session.document().revision()});
    CHECK(delete_second.ok());
    CHECK(delete_second.changed);
    CHECK(
        session.document()
            .findDatumPlane(second_id) ==
        nullptr);

    const auto undo_delete =
        session.undo();
    CHECK(undo_delete.ok());
    CHECK(undo_delete.changed);
    CHECK(
        session.document()
            .findDatumPlane(second_id) !=
        nullptr);

    const auto redo_delete =
        session.redo();
    CHECK(redo_delete.ok());
    CHECK(redo_delete.changed);
    CHECK(
        session.document()
            .findDatumPlane(second_id) ==
        nullptr);

    // A structurally valid but missing Datum source cannot preview/Finish and
    // does not mutate or consume durable history.
    const auto missing_id =
        part::DatumId::parse("999");
    CHECK(missing_id.has_value());
    auto invalid_source =
        application::DatumPlaneDraft::beginCreate(
            session,
            datumPlane(*missing_id));
    CHECK(invalid_source.has_value());
    const auto invalid_preview =
        session.evaluateDatumPlaneDraft(
            *invalid_source,
            kernel);
    CHECK(!invalid_preview.committable());
    CHECK(
        invalid_preview.status ==
        application::DatumPlaneDraftEvaluationStatus::
            invalid_candidate);
    const auto state_before_invalid_finish =
        session.document().state();
    const auto undo_before_invalid_finish =
        session.undoDepth();
    const auto invalid_finish =
        application::finishDatumPlaneDraft(
            session,
            *invalid_source,
            invalid_preview,
            kernel);
    CHECK(!invalid_finish.ok());
    CHECK(
        session.document().state() ==
        state_before_invalid_finish);
    CHECK(
        session.undoDepth() ==
        undo_before_invalid_finish);

    // Revision drift rejects a once-successful evaluation with zero Datum
    // mutation from the stale draft.
    auto stale_session = makeSession();
    auto stale_draft =
        application::DatumPlaneDraft::beginCreate(
            stale_session,
            originPlane(
                core::BuiltinReferenceRole::
                    yz_plane));
    CHECK(stale_draft.has_value());
    const auto stale_preview =
        stale_session.evaluateDatumPlaneDraft(
            *stale_draft,
            kernel);
    CHECK(stale_preview.committable());

    auto properties =
        stale_session.document().properties();
    properties.title = "revision drift";
    const auto changed =
        stale_session.execute(
            application::SetDocumentPropertiesCommand{
                std::move(properties)});
    CHECK(changed.ok());
    CHECK(changed.changed);
    const auto stale_state_before_finish =
        stale_session.document().state();
    const auto stale_undo_before_finish =
        stale_session.undoDepth();

    const auto stale_context =
        application::finishDatumPlaneDraft(
            stale_session,
            *stale_draft,
            stale_preview,
            kernel);
    CHECK(!stale_context.ok());
    CHECK(
        stale_context.status ==
        application::DatumPlaneDraftFinishStatus::
            stale_context);
    CHECK(
        stale_session.document().state() ==
        stale_state_before_finish);
    CHECK(
        stale_session.undoDepth() ==
        stale_undo_before_finish);

    // Session-local Datum high-water survives Undo branching: ID 1 is never
    // reused after its creation is undone.
    auto id_session = makeSession();
    const auto allocated_one =
        finishCreate(
            id_session,
            kernel,
            originPlane(
                core::BuiltinReferenceRole::
                    xy_plane),
            1.0);
    CHECK(allocated_one.serialized() == "1");
    CHECK(id_session.undo().ok());
    CHECK(
        id_session.document().datumPlanes().empty());

    const auto allocated_two =
        finishCreate(
            id_session,
            kernel,
            originPlane(
                core::BuiltinReferenceRole::
                    xz_plane),
            2.0);
    CHECK(allocated_two.serialized() == "2");
    CHECK(allocated_two != allocated_one);

    std::cout
        << "PM-03C Datum command/draft tests passed\n";
    return EXIT_SUCCESS;
}
