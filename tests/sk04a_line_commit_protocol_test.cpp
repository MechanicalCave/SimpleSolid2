#include <simplesolid2/application/document_session.hpp>
#include <simplesolid2/sketch/interaction_state.hpp>

#include <cstdlib>
#include <iostream>

using namespace simplesolid2;

namespace {

void check(
    bool value,
    const char* expression,
    int line) {
    if (!value) {
        std::cerr
            << "SK-04A Line protocol CHECK failed at line "
            << line << ": "
            << expression << '\n';
        std::exit(EXIT_FAILURE);
    }
}

#define CHECK(expr) \
    check(static_cast<bool>(expr), #expr, __LINE__)

application::AddSketchLineResult commitPending(
    application::DocumentSession& session,
    sketch::SketchInteractionState& interaction,
    const sketch::SketchId& sketch_id) {
    const auto request =
        interaction.pendingLineRequest();
    CHECK(request.has_value());

    const auto result =
        session.execute(
            application::AddSketchLineCommand{
                sketch_id,
                request->start,
                request->end});

    CHECK(
        interaction.resolveLineRequest(
            result.ok() && result.changed));
    return result;
}

} // namespace

int main() {
    auto document =
        part::PartDocument::create(
            core::DocumentId::generate());
    application::DocumentSession session{
        "sk04a-line-protocol.ss2part",
        std::move(document)};

    const auto created =
        session.execute(
            application::CreatePartSketchCommand{
                core::BuiltinReferenceRole::xy_plane});
    CHECK(created.ok());
    CHECK(created.sketch_id.has_value());
    const auto sketch_id =
        *created.sketch_id;

    const auto baseline_depth =
        session.undoDepth();

    sketch::SketchInteractionState interaction;
    interaction.activateLine();

    const sketch::Point2 a{0.0, 0.0};
    const sketch::Point2 b{10.0, 0.0};
    const sketch::Point2 c{10.0, 8.0};
    const sketch::Point2 d{3.0, 8.0};

    CHECK(
        interaction.acceptLinePoint(a).outcome ==
        sketch::LinePointOutcome::
            first_point_accepted);

    const auto ab_request =
        interaction.acceptLinePoint(b);
    CHECK(
        ab_request.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    const auto ab =
        commitPending(
            session,
            interaction,
            sketch_id);
    CHECK(ab.ok() && ab.changed);
    CHECK(ab.entity_id.has_value());
    CHECK(interaction.lineAnchor() == b);

    const auto bc_request =
        interaction.acceptLinePoint(c);
    CHECK(
        bc_request.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    const auto bc =
        commitPending(
            session,
            interaction,
            sketch_id);
    CHECK(bc.ok() && bc.changed);
    CHECK(bc.entity_id.has_value());
    CHECK(interaction.lineAnchor() == c);

    const auto cd_request =
        interaction.acceptLinePoint(d);
    CHECK(
        cd_request.outcome ==
        sketch::LinePointOutcome::
            segment_requested);
    const auto cd =
        commitPending(
            session,
            interaction,
            sketch_id);
    CHECK(cd.ok() && cd.changed);
    CHECK(cd.entity_id.has_value());
    CHECK(interaction.lineAnchor() == d);

    const auto* hosted =
        session.document().findSketch(sketch_id);
    CHECK(hosted != nullptr);
    CHECK(hosted->model.entityCount() == 3U);
    CHECK(
        session.undoDepth() ==
        baseline_depth + 3U);

    // Tool cancellation discards only runtime state. Already committed
    // segments remain ordinary authored geometry and history.
    interaction.cancelTool();
    CHECK(
        interaction.tool() ==
        sketch::SketchTool::select);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.entityCount() == 3U);
    CHECK(
        session.undoDepth() ==
        baseline_depth + 3U);

    CHECK(session.undo().changed);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.entityCount() == 2U);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*cd.entity_id) ==
        nullptr);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*bc.entity_id) !=
        nullptr);

    CHECK(session.undo().changed);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.entityCount() == 1U);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*bc.entity_id) ==
        nullptr);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*ab.entity_id) !=
        nullptr);

    CHECK(session.undo().changed);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.entityCount() == 0U);
    CHECK(
        session.undoDepth() ==
        baseline_depth);

    CHECK(session.redo().changed);
    CHECK(session.redo().changed);
    CHECK(session.redo().changed);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.entityCount() == 3U);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*ab.entity_id) !=
        nullptr);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*bc.entity_id) !=
        nullptr);
    CHECK(
        session.document()
            .findSketch(sketch_id)
            ->model.findLine(*cd.entity_id) !=
        nullptr);

    return EXIT_SUCCESS;
}
