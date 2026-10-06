#include <simplesolid2/application/axis_draft.hpp>
#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/core/document.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/axis_evaluation.hpp>
#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/sketch/entity_role.hpp>

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>

using namespace simplesolid2;

namespace {

void check(bool value, const char* expression, int line) {
    if (!value) {
        std::cerr
            << "PM-04B1 Axis lifecycle/draft CHECK failed at line "
            << line << ": " << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}
#define CHECK(expr) check(static_cast<bool>(expr), #expr, __LINE__)

class NoopKernel final : public kernel::ISolidModelingKernel {
public:
    [[nodiscard]] kernel::SolidModelingResult
    extrude(
        const kernel::LinearExtrudeInput&,
        kernel::RuntimeSolidHandle) noexcept override {
        return {};
    }
};

part::AxisReference authoredAxis(
    part::AxisId id) {
    return part::AxisReference{
        part::AuthoredAxisReference{id}};
}

bool near(double lhs, double rhs) {
    return std::abs(lhs - rhs) < 1.0e-12;
}

struct Fixture final {
    application::DocumentSession session;
    sketch::SketchId sketch_id;
    sketch::EntityId regular_line;
    sketch::EntityId construction_line;
    sketch::EntityId circle;

    Fixture(
        application::DocumentSession session_value,
        sketch::SketchId sketch_id_value,
        sketch::EntityId regular_line_value,
        sketch::EntityId construction_line_value,
        sketch::EntityId circle_value)
        : session{std::move(session_value)},
          sketch_id{std::move(sketch_id_value)},
          regular_line{regular_line_value},
          construction_line{construction_line_value},
          circle{circle_value} {}
};

Fixture makeFixture() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());

    const auto support =
        part::partSketchSupportForBuiltinPlane(
            core::BuiltinReferenceRole::xy_plane);
    CHECK(support.has_value());

    const auto sketch_id =
        sketch::SketchId::generate();
    sketch::SketchModel model;
    const auto regular_line =
        model.addLine(
            {0.0, 0.0},
            {10.0, 0.0},
            sketch::EntityRole::regular);
    const auto construction_line =
        model.addLine(
            {0.0, 0.0},
            {0.0, 10.0},
            sketch::EntityRole::construction);
    const auto circle =
        model.addCircle(
            {5.0, 5.0},
            2.0,
            sketch::EntityRole::regular);

    auto state = document.state();
    state.sketches.push_back(
        part::PartSketch{
            sketch_id,
            *support,
            true,
            std::move(model)});

    part::PartDocumentTransaction tx{document};
    tx.replaceState(std::move(state));
    CHECK(tx.commit().ok());

    return Fixture{
        application::DocumentSession{
            std::filesystem::path{"AxisLifecycle.ss2part"},
            std::move(document)},
        sketch_id,
        regular_line,
        construction_line,
        circle};
}

part::AxisEvaluation evaluate(
    const application::DocumentSession& session,
    part::AxisId id) {
    return part::resolveAxisReference(
        session.document(),
        authoredAxis(id));
}

} // namespace

int main() {
    NoopKernel kernel;
    auto fixture = makeFixture();
    auto& session = fixture.session;

    // Create from Regular Line.
    const auto first =
        session.execute(
            application::CreateAxisCommand{
                {fixture.sketch_id, fixture.regular_line},
                session.document().revision(),
                {},
                true},
            kernel);
    CHECK(first.ok());
    CHECK(first.changed);
    CHECK(first.axis_id.has_value());
    CHECK(first.axis_id->serialized() == "1");
    CHECK(session.document().axes().size() == 1U);
    CHECK(
        session.document().findAxis(*first.axis_id)
            ->name == "Axis001");
    CHECK(evaluate(session, *first.axis_id).status ==
          part::AxisEvaluationStatus::resolved);

    // Undo removes the authored Axis but does not rewind the session's
    // high-water identity authority. Branching Create must allocate AxisId 2.
    CHECK(session.undo().ok());
    CHECK(session.document().axes().empty());
    const auto branched =
        session.execute(
            application::CreateAxisCommand{
                {fixture.sketch_id, fixture.regular_line},
                session.document().revision(),
                {},
                true},
            kernel);
    CHECK(branched.ok());
    CHECK(branched.axis_id.has_value());
    CHECK(branched.axis_id->serialized() == "2");
    CHECK(!session.canRedo());

    const auto axis_id = *branched.axis_id;

    // Construction Line is equally admissible. Hide Axis independently, then
    // re-source it while preserving AxisId and authored visibility.
    const auto construction =
        session.execute(
            application::CreateAxisCommand{
                {fixture.sketch_id, fixture.construction_line},
                session.document().revision(),
                "Construction Axis",
                true},
            kernel);
    CHECK(construction.ok());
    CHECK(construction.axis_id.has_value());
    CHECK(construction.axis_id->serialized() == "3");

    CHECK(
        session.execute(
            application::SetAxisVisibilityCommand{
                {axis_id},
                session.document().revision(),
                false})
            .ok());
    CHECK(
        !session.document().findAxis(axis_id)
             ->visible);
    CHECK(evaluate(session, axis_id).status ==
          part::AxisEvaluationStatus::resolved);

    const auto edit_revision =
        session.document().revision();
    CHECK(
        session.execute(
            application::EditAxisCommand{
                axis_id,
                {fixture.sketch_id,
                 fixture.construction_line},
                edit_revision},
            kernel)
            .ok());
    CHECK(session.document().findAxis(axis_id) != nullptr);
    CHECK(
        session.document().findAxis(axis_id)
            ->id == axis_id);
    CHECK(
        !session.document().findAxis(axis_id)
             ->visible);
    auto edited = evaluate(session, axis_id);
    CHECK(edited.status ==
          part::AxisEvaluationStatus::resolved);
    CHECK(edited.line.has_value());
    CHECK(near(edited.line->direction.x, 0.0));
    CHECK(near(edited.line->direction.y, 1.0));

    // Stale Edit cannot commit.
    const auto stale =
        session.execute(
            application::EditAxisCommand{
                axis_id,
                {fixture.sketch_id,
                 fixture.regular_line},
                edit_revision},
            kernel);
    CHECK(!stale.ok());
    CHECK(
        stale.diagnostic.code ==
        application::DocumentSessionErrorCode::
            revision_diverged);

    // Moving the source Line updates the derived Axis; no world-line state is
    // authored or cached as success truth.
    CHECK(
        session.execute(
            application::UpdateSketchLinesCommand{
                fixture.sketch_id,
                session.document().revision(),
                {{
                    fixture.construction_line,
                    {1.0, 2.0},
                    {4.0, 6.0}}}})
            .ok());
    auto moved = evaluate(session, axis_id);
    CHECK(moved.status ==
          part::AxisEvaluationStatus::resolved);
    CHECK(moved.line.has_value());
    CHECK(near(moved.line->origin.x, 1.0));
    CHECK(near(moved.line->origin.y, 2.0));
    CHECK(near(moved.line->direction.x, 0.6));
    CHECK(near(moved.line->direction.y, 0.8));

    // Existing non-Line identity cannot be committed as a Create/Edit source.
    const auto invalid_count =
        session.document().axes().size();
    const auto invalid_create =
        session.execute(
            application::CreateAxisCommand{
                {fixture.sketch_id, fixture.circle},
                session.document().revision(),
                {},
                true},
            kernel);
    CHECK(!invalid_create.ok());
    CHECK(
        invalid_create.evaluation_diagnostic ==
        part::AxisEvaluationDiagnostic::
            source_not_line);
    CHECK(
        session.document().axes().size() ==
        invalid_count);

    // Source Line deletion leaves durable Axis intent repairably Missing.
    CHECK(
        session.execute(
            application::EraseSketchEntityCommand{
                fixture.sketch_id,
                fixture.construction_line})
            .ok());
    const auto missing = evaluate(session, axis_id);
    CHECK(missing.valid());
    CHECK(missing.status ==
          part::AxisEvaluationStatus::missing);
    CHECK(missing.diagnostic ==
          part::AxisEvaluationDiagnostic::missing_line);
    CHECK(session.document().findAxis(axis_id) != nullptr);

    // Explicit re-source repairs the Axis with the same AxisId and visibility.
    const auto replacement =
        session.execute(
            application::AddSketchLineCommand{
                fixture.sketch_id,
                {-5.0, 0.0},
                {5.0, 0.0},
                sketch::EntityRole::regular});
    CHECK(replacement.ok());
    CHECK(replacement.entity_id.has_value());
    CHECK(
        session.execute(
            application::EditAxisCommand{
                axis_id,
                {fixture.sketch_id,
                 *replacement.entity_id},
                session.document().revision()},
            kernel)
            .ok());
    CHECK(
        session.document().findAxis(axis_id)
            ->id == axis_id);
    CHECK(
        !session.document().findAxis(axis_id)
             ->visible);
    CHECK(evaluate(session, axis_id).status ==
          part::AxisEvaluationStatus::resolved);

    // Delete + Undo restores the same AxisId.
    CHECK(
        session.execute(
            application::DeleteAxisCommand{
                axis_id,
                session.document().revision()})
            .ok());
    CHECK(session.document().findAxis(axis_id) == nullptr);
    CHECK(session.undo().ok());
    CHECK(session.document().findAxis(axis_id) != nullptr);
    CHECK(
        session.document().findAxis(axis_id)
            ->id == axis_id);

    // Shared selection-first draft evaluates and finishes through the same
    // semantic command path.
    auto draft =
        application::AxisDraft::beginCreate(
            session,
            part::SketchLineAxisSource{
                fixture.sketch_id,
                fixture.regular_line});
    CHECK(draft.has_value());
    const auto evaluated =
        session.evaluateAxisDraft(
            *draft,
            kernel);
    CHECK(evaluated.committable());
    CHECK(evaluated.candidate_axis_id.has_value());
    const auto finished =
        application::finishAxisDraft(
            session,
            *draft,
            evaluated,
            kernel);
    CHECK(finished.ok());
    CHECK(finished.axis_id.has_value());
    CHECK(
        session.document().findAxis(
            *finished.axis_id) != nullptr);

    // Command-first draft uses the same source acquisition surface. Text does
    // not encode Sketch/Entity identity and cannot bypass semantic selection.
    auto command_first =
        application::AxisDraft::beginCreate(
            session);
    const application::CadInputNumberFormat format;
    const auto source_action =
        application::submitAxisCadInput(
            command_first,
            "SOURCE",
            format);
    CHECK(source_action.accepted);
    CHECK(
        source_action.action ==
        application::AxisCadInputAction::
            acquire_source);
    CHECK(
        command_first.setSource(
            {fixture.sketch_id,
             fixture.regular_line}));
    CHECK(
        application::submitAxisCadInput(
            command_first,
            "",
            format)
            .action ==
        application::AxisCadInputAction::finish);
    CHECK(
        application::submitAxisCadInput(
            command_first,
            "CANCEL",
            format)
            .action ==
        application::AxisCadInputAction::cancel);
    CHECK(
        !application::submitAxisCadInput(
             command_first,
             "1,2,3",
             format)
             .accepted);

    // Any document mutation invalidates the old draft revision.
    auto stale_draft =
        application::AxisDraft::beginCreate(
            session,
            part::SketchLineAxisSource{
                fixture.sketch_id,
                fixture.regular_line});
    CHECK(stale_draft.has_value());
    CHECK(
        session.execute(
            application::SetAxisVisibilityCommand{
                {*finished.axis_id},
                session.document().revision(),
                false})
            .ok());
    CHECK(
        session.evaluateAxisDraft(
            *stale_draft,
            kernel)
            .status ==
        application::AxisDraftEvaluationStatus::
            stale_revision);

    std::cout
        << "PM-04B1 Axis lifecycle/draft tests passed\n";
    return EXIT_SUCCESS;
}
