#include <simplesolid2/application/document_session.hpp>

#include <algorithm>
#include <cmath>
#include <numbers>
#include <set>
#include <stdexcept>
#include <utility>

namespace simplesolid2::application {
namespace {

DocumentSessionResult failure(
    DocumentSessionErrorCode code,
    std::string message,
    std::filesystem::path path = {},
    part::PartCommitErrorCode commit_code = part::PartCommitErrorCode::none,
    part::PartStoreErrorCode store_code = part::PartStoreErrorCode::none) {
    return DocumentSessionResult{
        false,
        DocumentSessionDiagnostic{
            code,
            commit_code,
            store_code,
            std::move(message),
            std::move(path),
        },
    };
}

DocumentSessionResult success(bool changed = false) {
    return DocumentSessionResult{changed, DocumentSessionDiagnostic{}};
}

part::PartSketch* findSketch(
    part::PartAuthoredState& state,
    const sketch::SketchId& id) noexcept {
    const auto found = std::find_if(
        state.sketches.begin(),
        state.sketches.end(),
        [&id](const part::PartSketch& item) {
            return item.id == id;
        });

    return found == state.sketches.end()
        ? nullptr
        : &*found;
}

part::PartFeature* findFeature(
    part::PartAuthoredState& state,
    part::FeatureId id) noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found =
        std::find_if(
            state.body.features.begin(),
            state.body.features.end(),
            [id](const part::PartFeature& item) {
                return item.id == id;
            });
    return found == state.body.features.end()
        ? nullptr
        : &*found;
}

part::PartProfile* findProfile(
    part::PartAuthoredState& state,
    part::ProfileId id) noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found = std::find_if(
        state.profiles.begin(),
        state.profiles.end(),
        [id](const part::PartProfile& item) {
            return item.id == id;
        });
    return found == state.profiles.end()
        ? nullptr
        : &*found;
}

std::string defaultFeatureName(
    part::FeatureId id) {
    const auto serialized = id.serialized();
    std::string result{"Extrude"};
    if (serialized.size() < 3U) {
        result.append(
            3U - serialized.size(),
            '0');
    }
    result += serialized;
    return result;
}

std::string defaultProfileName(
    part::ProfileId id) {
    const auto serialized = id.serialized();
    std::string result{"Profile"};
    if (serialized.size() < 3U) {
        result.append(
            3U - serialized.size(),
            '0');
    }
    result += serialized;
    return result;
}

} // namespace

DocumentSession::DocumentSession(
    std::filesystem::path path,
    part::PartDocument document)
    : path_{std::move(path)},
      document_{std::move(document)},
      saved_state_{document_.state()},
      expected_revision_{document_.revision()} {
    absorbSketchEntityIdCursors(document_.state());
    profile_id_cursor_.preserve(
        document_.state().next_profile_id);
    body_id_cursor_.preserve(
        document_.state().next_body_id);
    feature_id_cursor_.preserve(
        document_.state().body.next_feature_id);
}

DocumentSession::DocumentSession(
    std::filesystem::path path,
    part::PartDocument document,
    part::PartFileCheckpoint file_checkpoint)
    : path_{std::move(path)},
      document_{std::move(document)},
      saved_state_{document_.state()},
      file_checkpoint_{std::move(file_checkpoint)},
      expected_revision_{document_.revision()} {
    if (file_checkpoint_->document_id !=
        document_.documentId()) {
        throw std::invalid_argument{
            "DocumentSession file checkpoint DocumentId mismatch"};
    }
    absorbSketchEntityIdCursors(document_.state());
    profile_id_cursor_.preserve(
        document_.state().next_profile_id);
    body_id_cursor_.preserve(
        document_.state().next_body_id);
    feature_id_cursor_.preserve(
        document_.state().body.next_feature_id);
}

DocumentSessionResult DocumentSession::verifyRevision() const {
    if (document_.revision() != expected_revision_) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Document revision diverged from the active command/history context",
            path_);
    }
    return success();
}

DocumentSessionResult DocumentSession::commitCommandState(
    part::PartAuthoredState after,
    const char* failure_message) {
    if (const auto verified = verifyRevision(); !verified.ok()) {
        return verified;
    }

    applySketchEntityIdCursors(after);
    applyProfileIdCursor(after);
    applyBodyFeatureIdCursors(after);

    if (after == document_.state()) {
        return success(false);
    }

    // C1 strong-consistency preparation: create only the one pending
    // history entry and all potentially allocating bookkeeping before
    // the Part transaction mutates the live document. Existing history
    // entries are never deep-copied.
    HistoryEntry pending{
        document_.state(),
        std::move(after)};

    history_.reserve(cursor_ + 1U);

    auto prepared_entity_id_cursors =
        sketch_entity_id_cursors_;
    absorbSketchEntityIdCursors(
        prepared_entity_id_cursors,
        pending.after);
    auto prepared_profile_id_cursor =
        profile_id_cursor_;
    prepared_profile_id_cursor.preserve(
        pending.after.next_profile_id);
    auto prepared_body_id_cursor =
        body_id_cursor_;
    prepared_body_id_cursor.preserve(
        pending.after.next_body_id);
    auto prepared_feature_id_cursor =
        feature_id_cursor_;
    prepared_feature_id_cursor.preserve(
        pending.after.body.next_feature_id);

    part::PartDocumentTransaction transaction{document_};
    transaction.replaceState(pending.after);
    const auto committed = transaction.commit();
    if (!committed.ok()) {
        return failure(
            DocumentSessionErrorCode::transaction_failure,
            failure_message,
            path_,
            committed.code);
    }
    if (!committed.changed) {
        return success(false);
    }

    // The suffix erase destroys Redo only after successful mutation.
    // reserve() above plus noexcept HistoryEntry move makes the append
    // non-allocating and non-throwing after the durable commit.
    history_.erase(
        history_.begin() +
            static_cast<std::ptrdiff_t>(cursor_),
        history_.end());
    history_.push_back(std::move(pending));
    cursor_ = history_.size();
    sketch_entity_id_cursors_.swap(
        prepared_entity_id_cursors);
    profile_id_cursor_ =
        prepared_profile_id_cursor;
    body_id_cursor_ =
        prepared_body_id_cursor;
    feature_id_cursor_ =
        prepared_feature_id_cursor;
    expected_revision_ = document_.revision();
    return success(true);
}

DocumentSessionResult DocumentSession::execute(
    const SetDocumentPropertiesCommand& command) {
    auto after = document_.state();
    after.properties = command.properties;
    return commitCommandState(
        std::move(after),
        "Part transaction failed while executing document properties command");
}

DocumentSessionResult DocumentSession::execute(
    const SetPartLengthUnitCommand& command) {
    if (!core::isLengthUnit(command.unit)) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Part length-unit command contains an invalid unit",
            path_);
    }

    auto after = document_.state();
    after.length_unit = command.unit;
    return commitCommandState(
        std::move(after),
        "Part transaction failed while setting Part length unit");
}

DocumentSessionResult DocumentSession::execute(
    const SetBuiltinReferenceVisibilityCommand& command) {
    for (const auto role : command.targets) {
        if (!core::isBuiltinReferenceRole(role)) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Visibility command contains an invalid built-in reference role",
                path_);
        }
    }

    auto after = document_.state();
    for (const auto role : command.targets) {
        static_cast<void>(
            after.presentation.builtin_references.setVisible(
                role,
                command.visible));
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while executing built-in reference visibility command");
}

CreatePartSketchResult DocumentSession::execute(
    const CreatePartSketchCommand& command) {
    const auto support =
        part::partSketchSupportForBuiltinPlane(
            command.support);
    if (!support) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Sketch creation requires XY, XZ or YZ built-in Origin plane support",
            path_);
        return CreatePartSketchResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto placement =
        part::sketchPlacementForSupport(*support);
    if (!placement) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Unable to derive a valid Sketch placement from the selected support",
            path_);
        return CreatePartSketchResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    std::optional<sketch::SketchId> id;
    for (unsigned attempt = 0U;
         attempt < 16U;
         ++attempt) {
        auto candidate =
            sketch::SketchId::generate();
        if (document_.findSketch(candidate) == nullptr) {
            id = std::move(candidate);
            break;
        }
    }

    if (!id) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Unable to allocate a unique SketchId",
            path_);
        return CreatePartSketchResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    after.sketches.push_back(
        part::PartSketch{
            *id,
            *support,
            *placement,
            true});

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating Sketch");
    if (!committed.ok() || !committed.changed) {
        return CreatePartSketchResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return CreatePartSketchResult{
        true,
        *id,
        DocumentSessionDiagnostic{}};
}

AddSketchLineResult DocumentSession::execute(
    const AddSketchLineCommand& command) {
    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Line target SketchId does not exist",
            path_);
        return AddSketchLineResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    std::optional<sketch::EntityId> entity_id;
    try {
        entity_id =
            target->model.addLine(
                command.start,
                command.end,
                command.role);
    } catch (const std::invalid_argument&) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Line contains invalid authored geometry",
            path_);
        return AddSketchLineResult{
            false,
            std::nullopt,
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Sketch EntityId allocation space is exhausted",
            path_);
        return AddSketchLineResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while adding Sketch Line");
    if (!committed.ok() || !committed.changed) {
        return AddSketchLineResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return AddSketchLineResult{
        true,
        *entity_id,
        DocumentSessionDiagnostic{}};
}

AddSketchCircleResult DocumentSession::execute(
    const AddSketchCircleCommand& command) {
    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Circle target SketchId does not exist",
            path_);
        return AddSketchCircleResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    std::optional<sketch::EntityId> entity_id;
    try {
        entity_id =
            target->model.addCircle(
                command.center,
                command.radius,
                command.role);
    } catch (const std::invalid_argument&) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Circle contains invalid authored geometry",
            path_);
        return AddSketchCircleResult{
            false,
            std::nullopt,
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Sketch EntityId allocation space is exhausted",
            path_);
        return AddSketchCircleResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while adding Sketch Circle");
    if (!committed.ok() || !committed.changed) {
        return AddSketchCircleResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return AddSketchCircleResult{
        true,
        *entity_id,
        DocumentSessionDiagnostic{}};
}

AddSketchArcResult DocumentSession::execute(
    const AddSketchArcCommand& command) {
    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Arc target SketchId does not exist",
            path_);
        return AddSketchArcResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    std::optional<sketch::EntityId> entity_id;
    try {
        entity_id =
            target->model.addArc(
                command.center,
                command.radius,
                command.start_angle,
                command.sweep_angle,
                command.role);
    } catch (const std::invalid_argument&) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Arc contains invalid authored geometry",
            path_);
        return AddSketchArcResult{
            false,
            std::nullopt,
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Sketch EntityId allocation space is exhausted",
            path_);
        return AddSketchArcResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while adding Sketch Arc");
    if (!committed.ok() || !committed.changed) {
        return AddSketchArcResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return AddSketchArcResult{
        true,
        *entity_id,
        DocumentSessionDiagnostic{}};
}


AddSketchRectangleResult DocumentSession::execute(
    const AddSketchRectangleCommand& command) {
    if (document_.revision() != command.expected_revision) {
        const auto failed = failure(
            DocumentSessionErrorCode::revision_diverged,
            "Add Sketch Rectangle was started from a stale DocumentRevision",
            path_);
        return {
            false,
            {},
            failed.diagnostic};
    }

    if (!command.first_corner.finite() ||
        !command.opposite_corner.finite() ||
        command.first_corner.u ==
            command.opposite_corner.u ||
        command.first_corner.v ==
            command.opposite_corner.v ||
        (command.perimeter_role !=
             sketch::EntityRole::regular &&
         command.perimeter_role !=
             sketch::EntityRole::construction)) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Rectangle contains invalid authored geometry or role",
            path_);
        return {
            false,
            {},
            failed.diagnostic};
    }

    auto after = document_.state();
    // Preserve the session-local non-reuse high-water before allocating the
    // Rectangle's fresh identities, matching semantic duplication/history.
    applySketchEntityIdCursors(after);

    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Rectangle target SketchId does not exist",
            path_);
        return {
            false,
            {},
            failed.diagnostic};
    }

    const auto a = command.first_corner;
    const auto c = command.opposite_corner;
    const sketch::Point2 b{c.u, a.v};
    const sketch::Point2 d{a.u, c.v};

    std::vector<sketch::EntityId> created;
    created.reserve(
        command.draw_diagonals ? 6U : 4U);

    try {
        created.push_back(
            target->model.addLine(
                a, b, command.perimeter_role));
        created.push_back(
            target->model.addLine(
                b, c, command.perimeter_role));
        created.push_back(
            target->model.addLine(
                c, d, command.perimeter_role));
        created.push_back(
            target->model.addLine(
                d, a, command.perimeter_role));

        if (command.draw_diagonals) {
            created.push_back(
                target->model.addLine(
                    a,
                    c,
                    sketch::EntityRole::construction));
            created.push_back(
                target->model.addLine(
                    b,
                    d,
                    sketch::EntityRole::construction));
        }
    } catch (const std::invalid_argument&) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Add Sketch Rectangle contains invalid authored geometry",
            path_);
        return {
            false,
            {},
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Sketch EntityId allocation space is exhausted",
            path_);
        return {
            false,
            {},
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while adding Sketch Rectangle");
    if (!committed.ok() || !committed.changed) {
        return {
            committed.changed,
            {},
            committed.diagnostic};
    }

    return {
        true,
        std::move(created),
        DocumentSessionDiagnostic{}};
}

DocumentSessionResult DocumentSession::execute(
    const EraseSketchEntityCommand& command) {
    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entity target SketchId does not exist",
            path_);
    }

    if (!target->model.erase(command.entity_id)) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entity target EntityId does not exist",
            path_);
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while erasing Sketch entity");
}

DocumentSessionResult DocumentSession::execute(
    const EraseSketchEntitiesCommand& command) {
    if (command.entity_ids.empty()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entities requires at least one EntityId",
            path_);
    }

    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Erase Sketch Entities target SketchId does not exist",
            path_);
    }

    std::set<sketch::EntityId> unique;
    for (const auto id : command.entity_ids) {
        if (!id.valid()) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Erase Sketch Entities contains an invalid EntityId",
                path_);
        }

        if (!unique.insert(id).second) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Erase Sketch Entities contains duplicate EntityIds",
                path_);
        }

        if (!target->model.contains(id)) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Erase Sketch Entities target EntityId does not exist",
                path_);
        }
    }

    for (const auto id : command.entity_ids) {
        if (!target->model.erase(id)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Erase Sketch Entities validation diverged before commit",
                path_);
        }
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while erasing Sketch entities");
}

DocumentSessionResult DocumentSession::execute(
    const SetSketchEntityRoleCommand& command) {
    if (command.entity_ids.empty()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Set Sketch Entity Role requires at least one EntityId",
            path_);
    }

    if (command.role != sketch::EntityRole::regular &&
        command.role != sketch::EntityRole::construction) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Set Sketch Entity Role contains an invalid role",
            path_);
    }

    if (document_.revision() != command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Set Sketch Entity Role was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    auto* target = findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Set Sketch Entity Role target SketchId does not exist",
            path_);
    }

    std::set<sketch::EntityId> unique;
    for (const auto id : command.entity_ids) {
        if (!id.valid() ||
            !unique.insert(id).second ||
            !target->model.contains(id)) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Set Sketch Entity Role contains an invalid, duplicate or missing EntityId",
                path_);
        }
    }

    for (const auto id : command.entity_ids) {
        if (!target->model.setEntityRole(id, command.role)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Set Sketch Entity Role validation diverged before commit",
                path_);
        }
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while setting Sketch entity role");
}

DocumentSessionResult DocumentSession::execute(
    const UpdateSketchLinesCommand& command) {
    return execute(
        UpdateSketchGeometryCommand{
            command.sketch_id,
            command.expected_revision,
            command.lines,
            {},
            {}});
}

DocumentSessionResult DocumentSession::execute(
    const UpdateSketchGeometryCommand& command) {
    if (command.lines.empty() &&
        command.circles.empty() &&
        command.arcs.empty()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Update Sketch Geometry requires at least one entity",
            path_);
    }

    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Update Sketch Geometry was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Update Sketch Geometry target SketchId does not exist",
            path_);
    }

    std::set<sketch::EntityId> unique;

    for (const auto& line : command.lines) {
        if (!line.entity_id.valid() ||
            !line.start.finite() ||
            !line.end.finite() ||
            line.start == line.end ||
            !unique.insert(line.entity_id).second ||
            target->model.findLine(line.entity_id) ==
                nullptr) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Update Sketch Geometry contains an invalid Line update",
                path_);
        }
    }

    for (const auto& circle : command.circles) {
        if (!circle.entity_id.valid() ||
            !circle.center.finite() ||
            !std::isfinite(circle.radius) ||
            circle.radius <= 0.0 ||
            !unique.insert(circle.entity_id).second ||
            target->model.findCircle(circle.entity_id) ==
                nullptr) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Update Sketch Geometry contains an invalid Circle update",
                path_);
        }
    }

    constexpr double full_turn =
        2.0 * std::numbers::pi_v<double>;
    for (const auto& arc : command.arcs) {
        if (!arc.entity_id.valid() ||
            !arc.center.finite() ||
            !std::isfinite(arc.radius) ||
            arc.radius <= 0.0 ||
            !std::isfinite(arc.start_angle) ||
            !std::isfinite(arc.sweep_angle) ||
            arc.sweep_angle == 0.0 ||
            std::abs(arc.sweep_angle) >=
                full_turn ||
            !unique.insert(arc.entity_id).second ||
            target->model.findArc(arc.entity_id) ==
                nullptr) {
            return failure(
                DocumentSessionErrorCode::invalid_command,
                "Update Sketch Geometry contains an invalid Arc update",
                path_);
        }
    }

    for (const auto& line : command.lines) {
        if (!target->model.updateLine(
                line.entity_id,
                line.start,
                line.end)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Update Sketch Geometry Line validation diverged before commit",
                path_);
        }
    }
    for (const auto& circle : command.circles) {
        if (!target->model.updateCircle(
                circle.entity_id,
                circle.center,
                circle.radius)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Update Sketch Geometry Circle validation diverged before commit",
                path_);
        }
    }
    for (const auto& arc : command.arcs) {
        if (!target->model.updateArc(
                arc.entity_id,
                arc.center,
                arc.radius,
                arc.start_angle,
                arc.sweep_angle)) {
            return failure(
                DocumentSessionErrorCode::transaction_failure,
                "Update Sketch Geometry Arc validation diverged before commit",
                path_);
        }
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while updating Sketch geometry");
}

namespace {

[[nodiscard]] DocumentSessionDiagnostic
structuralEditFailureDiagnostic(
    const std::filesystem::path& path,
    sketch::StructuralEditStatus status,
    const char* operation) {
    DocumentSessionDiagnostic diagnostic;
    diagnostic.code =
        status ==
                sketch::StructuralEditStatus::
                    identity_exhausted
            ? DocumentSessionErrorCode::
                  transaction_failure
            : DocumentSessionErrorCode::
                  invalid_command;
    diagnostic.path = path;
    diagnostic.message =
        std::string{operation} +
        " structural edit was rejected";
    return diagnostic;
}

} // namespace

SketchStructuralEditCommandResult
DocumentSession::execute(
    const TrimSketchCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed = failure(
            DocumentSessionErrorCode::revision_diverged,
            "Trim was started from a stale DocumentRevision",
            path_);
        return {
            false,
            sketch::StructuralEditStatus::invalid_request,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applySketchEntityIdCursors(after);
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Trim target SketchId does not exist",
            path_);
        return {
            false,
            sketch::StructuralEditStatus::missing_entity,
            std::nullopt,
            failed.diagnostic};
    }

    const auto evaluated =
        sketch::evaluateTrim(
            target->model,
            sketch::TrimSketchRequest{
                command.target,
                command.boundaries,
                command.pick});
    if (!evaluated.ready() ||
        !evaluated.state) {
        return {
            false,
            evaluated.status,
            std::nullopt,
            structuralEditFailureDiagnostic(
                path_,
                evaluated.status,
                "Trim")};
    }

    auto restored =
        sketch::SketchModel::restore(
            *evaluated.state);
    if (!restored) {
        return {
            false,
            sketch::StructuralEditStatus::
                invalid_request,
            std::nullopt,
            structuralEditFailureDiagnostic(
                path_,
                sketch::StructuralEditStatus::
                    invalid_request,
                "Trim")};
    }
    target->model = std::move(*restored);

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while executing Trim");
    return {
        committed.changed,
        evaluated.status,
        committed.ok() && committed.changed
            ? evaluated.result_entity
            : std::nullopt,
        committed.diagnostic};
}

SketchStructuralEditCommandResult
DocumentSession::execute(
    const ExtendSketchCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed = failure(
            DocumentSessionErrorCode::revision_diverged,
            "Extend was started from a stale DocumentRevision",
            path_);
        return {
            false,
            sketch::StructuralEditStatus::invalid_request,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applySketchEntityIdCursors(after);
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Extend target SketchId does not exist",
            path_);
        return {
            false,
            sketch::StructuralEditStatus::missing_entity,
            std::nullopt,
            failed.diagnostic};
    }

    const auto evaluated =
        sketch::evaluateExtend(
            target->model,
            sketch::ExtendSketchRequest{
                command.target,
                command.boundaries,
                command.endpoint});
    if (!evaluated.ready() ||
        !evaluated.state) {
        return {
            false,
            evaluated.status,
            std::nullopt,
            structuralEditFailureDiagnostic(
                path_,
                evaluated.status,
                "Extend")};
    }

    auto restored =
        sketch::SketchModel::restore(
            *evaluated.state);
    if (!restored) {
        return {
            false,
            sketch::StructuralEditStatus::
                invalid_request,
            std::nullopt,
            structuralEditFailureDiagnostic(
                path_,
                sketch::StructuralEditStatus::
                    invalid_request,
                "Extend")};
    }
    target->model = std::move(*restored);

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while executing Extend");
    return {
        committed.changed,
        evaluated.status,
        committed.ok() && committed.changed
            ? evaluated.result_entity
            : std::nullopt,
        committed.diagnostic};
}

SketchStructuralEditCommandResult
DocumentSession::execute(
    const ExtendBothSketchLinesCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed = failure(
            DocumentSessionErrorCode::revision_diverged,
            "Extend Both was started from a stale DocumentRevision",
            path_);
        return {
            false,
            sketch::StructuralEditStatus::invalid_request,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applySketchEntityIdCursors(after);
    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Extend Both target SketchId does not exist",
            path_);
        return {
            false,
            sketch::StructuralEditStatus::missing_entity,
            std::nullopt,
            failed.diagnostic};
    }

    const auto evaluated =
        sketch::evaluateExtendBoth(
            target->model,
            sketch::ExtendBothLinesRequest{
                command.first_line,
                command.second_line});
    if (!evaluated.ready() ||
        !evaluated.state) {
        return {
            false,
            evaluated.status,
            std::nullopt,
            structuralEditFailureDiagnostic(
                path_,
                evaluated.status,
                "Extend Both")};
    }

    auto restored =
        sketch::SketchModel::restore(
            *evaluated.state);
    if (!restored) {
        return {
            false,
            sketch::StructuralEditStatus::
                invalid_request,
            std::nullopt,
            structuralEditFailureDiagnostic(
                path_,
                sketch::StructuralEditStatus::
                    invalid_request,
                "Extend Both")};
    }
    target->model = std::move(*restored);

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while executing Extend Both");
    return {
        committed.changed,
        evaluated.status,
        std::nullopt,
        committed.diagnostic};
}

DuplicateSketchGeometryResult DocumentSession::execute(
    const DuplicateSketchGeometryCommand& command) {
    if (command.geometry.empty()) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Duplicate Sketch Geometry requires at least one entity",
            path_);
        return DuplicateSketchGeometryResult{
            false,
            {},
            failed.diagnostic};
    }

    if (document_.revision() !=
        command.expected_revision) {
        const auto failed = failure(
            DocumentSessionErrorCode::revision_diverged,
            "Duplicate Sketch Geometry was started from a stale DocumentRevision",
            path_);
        return DuplicateSketchGeometryResult{
            false,
            {},
            failed.diagnostic};
    }

    auto after = document_.state();
    applySketchEntityIdCursors(after);

    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Duplicate Sketch Geometry target SketchId does not exist",
            path_);
        return DuplicateSketchGeometryResult{
            false,
            {},
            failed.diagnostic};
    }

    std::set<sketch::EntityId> source_ids;
    const auto require_source =
        [&source_ids](
            sketch::EntityId id,
            bool present) {
            return id.valid() &&
                   present &&
                   source_ids.insert(id).second;
        };

    for (const auto& line : command.geometry.lines) {
        if (!require_source(
                line.id,
                target->model.findLine(line.id) != nullptr)) {
            const auto failed = failure(
                DocumentSessionErrorCode::invalid_command,
                "Duplicate Sketch Geometry contains an invalid, duplicate or mismatched Line source",
                path_);
            return DuplicateSketchGeometryResult{
                false,
                {},
                failed.diagnostic};
        }
    }

    for (const auto& circle : command.geometry.circles) {
        if (!require_source(
                circle.id,
                target->model.findCircle(circle.id) != nullptr)) {
            const auto failed = failure(
                DocumentSessionErrorCode::invalid_command,
                "Duplicate Sketch Geometry contains an invalid, duplicate or mismatched Circle source",
                path_);
            return DuplicateSketchGeometryResult{
                false,
                {},
                failed.diagnostic};
        }
    }

    for (const auto& arc : command.geometry.arcs) {
        if (!require_source(
                arc.id,
                target->model.findArc(arc.id) != nullptr)) {
            const auto failed = failure(
                DocumentSessionErrorCode::invalid_command,
                "Duplicate Sketch Geometry contains an invalid, duplicate or mismatched Arc source",
                path_);
            return DuplicateSketchGeometryResult{
                false,
                {},
                failed.diagnostic};
        }
    }

    std::vector<sketch::EntityId> created;
    created.reserve(
        command.geometry.lines.size() +
        command.geometry.circles.size() +
        command.geometry.arcs.size());

    try {
        for (const auto& line : command.geometry.lines) {
            created.push_back(
                target->model.addLine(
                    line.start,
                    line.end,
                    target->model.findLine(line.id)->role()));
        }
        for (const auto& circle : command.geometry.circles) {
            created.push_back(
                target->model.addCircle(
                    circle.center,
                    circle.radius,
                    target->model.findCircle(circle.id)->role()));
        }
        for (const auto& arc : command.geometry.arcs) {
            created.push_back(
                target->model.addArc(
                    arc.center,
                    arc.radius,
                    arc.start_angle,
                    arc.sweep_angle,
                    target->model.findArc(arc.id)->role()));
        }
    } catch (const std::invalid_argument&) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Duplicate Sketch Geometry contains invalid authored geometry",
            path_);
        return DuplicateSketchGeometryResult{
            false,
            {},
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "Sketch EntityId allocation space is exhausted",
            path_);
        return DuplicateSketchGeometryResult{
            false,
            {},
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while duplicating Sketch geometry");
    if (!committed.ok() || !committed.changed) {
        return DuplicateSketchGeometryResult{
            committed.changed,
            {},
            committed.diagnostic};
    }

    return DuplicateSketchGeometryResult{
        true,
        std::move(created),
        DocumentSessionDiagnostic{}};
}

CreateProfileResult DocumentSession::execute(
    const CreateProfileCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed = failure(
            DocumentSessionErrorCode::revision_diverged,
            "Create Profile was started from a stale DocumentRevision",
            path_);
        return CreateProfileResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }
    if (!part::profileRegionIntentStructurallyValid(
            command.region_intent)) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Create Profile contains malformed RegionIntent",
            path_);
        return CreateProfileResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applyProfileIdCursor(after);
    auto* source =
        findSketch(after, command.source_sketch_id);
    if (source == nullptr) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Create Profile source SketchId does not exist",
            path_);
        return CreateProfileResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto resolved =
        part::resolveProfileRegionIntent(
            source->model,
            command.region_intent);
    if (!resolved.valid()) {
        const auto failed = failure(
            DocumentSessionErrorCode::invalid_command,
            "Create Profile RegionIntent does not resolve to one current valid region",
            path_);
        return CreateProfileResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    const auto id =
        after.next_profile_id.allocate();
    if (!id) {
        const auto failed = failure(
            DocumentSessionErrorCode::transaction_failure,
            "ProfileId allocation space is exhausted",
            path_);
        return CreateProfileResult{
            false,
            std::nullopt,
            failed.diagnostic};
    }

    after.profiles.push_back(
        part::PartProfile{
            *id,
            command.source_sketch_id,
            defaultProfileName(*id),
            part::ProfileVisibilityPolicy::automatic,
            command.region_intent});

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating Profile");
    if (!committed.ok() ||
        !committed.changed) {
        return CreateProfileResult{
            committed.changed,
            std::nullopt,
            committed.diagnostic};
    }

    return CreateProfileResult{
        true,
        *id,
        DocumentSessionDiagnostic{}};
}

DocumentSessionResult DocumentSession::execute(
    const ReplaceProfileRegionIntentCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Edit Profile was started from a stale DocumentRevision",
            path_);
    }
    if (!part::profileRegionIntentStructurallyValid(
            command.region_intent)) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Edit Profile contains malformed RegionIntent",
            path_);
    }

    auto after = document_.state();
    auto* profile =
        findProfile(after, command.profile_id);
    if (profile == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Edit Profile target ProfileId does not exist",
            path_);
    }
    auto* source =
        findSketch(
            after,
            profile->source_sketch_id);
    if (source == nullptr) {
        return failure(
            DocumentSessionErrorCode::transaction_failure,
            "Edit Profile source SketchId is missing",
            path_);
    }

    if (!part::resolveProfileRegionIntent(
             source->model,
             command.region_intent)
             .valid()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Edit Profile RegionIntent does not resolve to one current valid region",
            path_);
    }

    profile->region_intent =
        command.region_intent;
    return commitCommandState(
        std::move(after),
        "Part transaction failed while editing Profile");
}

DocumentSessionResult DocumentSession::execute(
    const SetProfilePropertiesCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Set Profile Properties was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    auto* profile =
        findProfile(after, command.profile_id);
    if (profile == nullptr) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Set Profile Properties target ProfileId does not exist",
            path_);
    }

    if (!part::isProfileVisibilityPolicy(
            command.visibility)) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Set Profile Properties contains an invalid visibility policy",
            path_);
    }
    profile->name = command.name;
    profile->visibility = command.visibility;
    return commitCommandState(
        std::move(after),
        "Part transaction failed while setting Profile properties");
}

DocumentSessionResult DocumentSession::execute(
    const DeleteProfileCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::revision_diverged,
            "Delete Profile was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    const auto found =
        std::find_if(
            after.profiles.begin(),
            after.profiles.end(),
            [&command](
                const part::PartProfile& profile) {
                return profile.id ==
                       command.profile_id;
            });
    if (found == after.profiles.end()) {
        return failure(
            DocumentSessionErrorCode::invalid_command,
            "Delete Profile target ProfileId does not exist",
            path_);
    }

    after.profiles.erase(found);
    return commitCommandState(
        std::move(after),
        "Part transaction failed while deleting Profile");
}


ExtrudeDraftEvaluationResult
DocumentSession::evaluateExtrudeDraft(
    const ExtrudeDraft& draft,
    kernel::ISolidModelingKernel&
        modeling_kernel) const {
    ExtrudeDraftEvaluationResult result;
    result.document_id = draft.documentId();
    result.source_revision =
        draft.sourceRevision();
    result.draft_generation =
        draft.generation();
    result.mode =
        draft.mode();
    result.profile_id =
        draft.profileId();
    result.feature_id =
        draft.featureId();
    result.operation =
        draft.operation();
    result.extent =
        draft.extent();
    result.name =
        draft.name();

    if (documentId() !=
        draft.documentId()) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                stale_document;
        return result;
    }
    if (document_.revision() !=
        draft.sourceRevision()) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                stale_revision;
        return result;
    }
    if (!draft.valid()) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                invalid_draft;
        return result;
    }
    if (document_.findProfile(
            draft.profileId()) == nullptr) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                missing_profile;
        return result;
    }

    auto after =
        document_.state();
    applyBodyFeatureIdCursors(after);

    part::FeatureId target_id;
    const part::ExtrudeFeature definition{
        draft.profileId(),
        draft.operation(),
        draft.extent()};

    if (draft.mode() ==
        ExtrudeDraftMode::create) {
        const auto id =
            after.body.next_feature_id
                .allocate();
        if (!id) {
            result.status =
                ExtrudeDraftEvaluationStatus::
                    feature_id_exhausted;
            return result;
        }
        target_id = *id;
        after.body.features.push_back(
            part::PartFeature{
                target_id,
                draft.name().empty()
                    ? defaultFeatureName(
                          target_id)
                    : draft.name(),
                false,
                definition});
    } else {
        if (!draft.featureId()) {
            result.status =
                ExtrudeDraftEvaluationStatus::
                    invalid_draft;
            return result;
        }
        auto* feature =
            findFeature(
                after,
                *draft.featureId());
        if (feature == nullptr) {
            result.status =
                ExtrudeDraftEvaluationStatus::
                    missing_feature;
            return result;
        }
        if (feature->suppressed) {
            result.status =
                ExtrudeDraftEvaluationStatus::
                    suppressed_feature;
            return result;
        }
        target_id = feature->id;
        feature->name =
            draft.name();
        feature->definition =
            definition;
    }

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            std::move(after),
            document_.revision());
    if (!candidate.ok()) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                invalid_candidate;
        return result;
    }

    const auto evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    result.body_status =
        evaluation.body_status;
    result.body_solid =
        evaluation.body_solid;

    const auto* target =
        evaluation.findFeature(
            target_id);
    if (target == nullptr) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                invalid_candidate;
        result.body_solid.reset();
        return result;
    }

    result.evaluation_diagnostic =
        target->diagnostic;
    if (target->status !=
        part::FeatureEvaluationStatus::
            up_to_date) {
        result.status =
            ExtrudeDraftEvaluationStatus::
                target_failed;
        result.body_solid.reset();
        return result;
    }

    // Presentation uses the raw Extrude tool, not the complete candidate
    // Body. Full candidate evaluation above remains the sole authority for
    // whether Finish is legal. Force Add only for isolated tool generation:
    // Cut semantics are still validated against the upstream Body above.
    auto preview_input =
        part::makeKernelExtrudeInput(
            *candidate.document,
            definition);
    if (preview_input) {
        preview_input->operation =
            kernel::SolidBooleanOperation::add;
        const auto preview_tool =
            modeling_kernel.extrude(
                *preview_input);
        if (preview_tool.ok()) {
            result.preview_tool_solid =
                preview_tool.solid;
        }
    }

    result.status =
        ExtrudeDraftEvaluationStatus::ok;
    return result;
}

CreateExtrudeFeatureResult DocumentSession::execute(
    const CreateExtrudeFeatureCommand& command,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    revision_diverged,
                "Create Extrude was started from a stale DocumentRevision",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const part::ExtrudeFeature definition{
        command.profile_id,
        command.operation,
        command.extent};
    if (!part::extrudeFeatureStructurallyValid(
            definition) ||
        document_.findProfile(
            command.profile_id) == nullptr) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Extrude contains invalid inputs or missing ProfileId",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applyBodyFeatureIdCursors(after);
    const auto id =
        after.body.next_feature_id.allocate();
    if (!id) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "FeatureId allocation space is exhausted",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    after.body.features.push_back(
        part::PartFeature{
            *id,
            command.name.empty()
                ? defaultFeatureName(*id)
                : command.name,
            false,
            definition});

    auto candidate =
        part::PartDocument::restore(
            document_.documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "Create Extrude candidate violates Part authored-state invariants",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const auto evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    const auto* target =
        evaluation.findFeature(*id);
    if (target == nullptr ||
        target->status !=
            part::FeatureEvaluationStatus::
                up_to_date) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Extrude did not evaluate UpToDate; no authored mutation committed",
                path_);
        return {
            false,
            std::nullopt,
            target != nullptr
                ? std::optional<
                      part::FeatureEvaluationDiagnosticCode>{
                      target->diagnostic}
                : std::nullopt,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating Extrude Feature");
    if (!committed.ok() ||
        !committed.changed) {
        return {
            committed.changed,
            std::nullopt,
            std::nullopt,
            committed.diagnostic};
    }

    return {
        true,
        *id,
        part::FeatureEvaluationDiagnosticCode::
            none,
        DocumentSessionDiagnostic{}};
}

DocumentSessionResult DocumentSession::execute(
    const EditExtrudeFeatureCommand& command,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Edit Extrude was started from a stale DocumentRevision",
            path_);
    }

    const part::ExtrudeFeature definition{
        command.profile_id,
        command.operation,
        command.extent};
    if (!part::extrudeFeatureStructurallyValid(
            definition) ||
        document_.findProfile(
            command.profile_id) == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Extrude contains invalid inputs or missing ProfileId",
            path_);
    }

    auto after = document_.state();
    auto* feature =
        findFeature(
            after,
            command.feature_id);
    if (feature == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Extrude target FeatureId does not exist",
            path_);
    }
    if (feature->suppressed) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Extrude target is Suppressed; unsuppress before editing",
            path_);
    }

    feature->name = command.name;
    feature->definition = definition;

    auto candidate =
        part::PartDocument::restore(
            document_.documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        return failure(
            DocumentSessionErrorCode::
                transaction_failure,
            "Edit Extrude candidate violates Part authored-state invariants",
            path_);
    }

    const auto evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    const auto* target =
        evaluation.findFeature(
            command.feature_id);
    if (target == nullptr ||
        target->status !=
            part::FeatureEvaluationStatus::
                up_to_date) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Extrude target did not evaluate UpToDate; no authored mutation committed",
            path_);
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while editing Extrude Feature");
}

DocumentSessionResult DocumentSession::execute(
    const SetFeatureSuppressedCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Suppress Feature was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    auto* feature =
        findFeature(
            after,
            command.feature_id);
    if (feature == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Suppress Feature target FeatureId does not exist",
            path_);
    }

    feature->suppressed =
        command.suppressed;
    return commitCommandState(
        std::move(after),
        "Part transaction failed while changing Feature suppression");
}

DocumentSessionResult DocumentSession::execute(
    const DeleteFeatureCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Delete Feature was started from a stale DocumentRevision",
            path_);
    }

    auto after = document_.state();
    const auto found =
        std::find_if(
            after.body.features.begin(),
            after.body.features.end(),
            [&command](
                const part::PartFeature& feature) {
                return feature.id ==
                       command.feature_id;
            });
    if (found ==
        after.body.features.end()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Delete Feature target FeatureId does not exist",
            path_);
    }

    after.body.features.erase(found);
    return commitCommandState(
        std::move(after),
        "Part transaction failed while deleting Feature");
}

DocumentSessionResult DocumentSession::applyHistoricalState(
    const part::PartAuthoredState& expected_current,
    const part::PartAuthoredState& target) {
    if (const auto verified = verifyRevision(); !verified.ok()) {
        return verified;
    }
    auto adjusted_expected =
        expected_current;
    applyProfileIdCursor(
        adjusted_expected);
    applyBodyFeatureIdCursors(
        adjusted_expected);
    if (document_.state() != adjusted_expected) {
        return failure(
            DocumentSessionErrorCode::history_diverged,
            "Authored state no longer matches the Undo/Redo history cursor",
            path_);
    }

    auto adjusted_target = target;
    applySketchEntityIdCursors(adjusted_target);
    applyProfileIdCursor(adjusted_target);
    applyBodyFeatureIdCursors(adjusted_target);

    part::PartDocumentTransaction transaction{document_};
    transaction.replaceState(std::move(adjusted_target));
    const auto committed = transaction.commit();
    if (!committed.ok() || !committed.changed) {
        return failure(
            DocumentSessionErrorCode::transaction_failure,
            "Part transaction failed while applying Undo/Redo",
            path_,
            committed.code);
    }

    expected_revision_ = document_.revision();
    absorbSketchEntityIdCursors(document_.state());
    profile_id_cursor_.preserve(
        document_.state().next_profile_id);
    body_id_cursor_.preserve(
        document_.state().next_body_id);
    feature_id_cursor_.preserve(
        document_.state().body.next_feature_id);
    return success(true);
}

void DocumentSession::absorbSketchEntityIdCursors(
    SketchEntityIdCursorMap& cursors,
    const part::PartAuthoredState& state) {
    for (const auto& hosted : state.sketches) {
        const auto observed =
            hosted.model.entityIdCursor();

        const auto found =
            cursors.find(hosted.id);
        if (found == cursors.end()) {
            cursors.emplace(
                hosted.id,
                observed);
            continue;
        }

        if (observed > found->second) {
            found->second = observed;
        }
    }
}

void DocumentSession::absorbSketchEntityIdCursors(
    const part::PartAuthoredState& state) {
    absorbSketchEntityIdCursors(
        sketch_entity_id_cursors_,
        state);
}

void DocumentSession::applySketchEntityIdCursors(
    part::PartAuthoredState& state) const {
    for (auto& hosted : state.sketches) {
        const auto found =
            sketch_entity_id_cursors_.find(
                hosted.id);
        if (found ==
            sketch_entity_id_cursors_.end()) {
            continue;
        }

        hosted.model.preserveEntityIdCursor(
            found->second);
    }
}

void DocumentSession::applyProfileIdCursor(
    part::PartAuthoredState& state) const noexcept {
    state.next_profile_id.preserve(
        profile_id_cursor_);
}

void DocumentSession::applyBodyFeatureIdCursors(
    part::PartAuthoredState& state) const noexcept {
    state.next_body_id.preserve(
        body_id_cursor_);
    state.body.next_feature_id.preserve(
        feature_id_cursor_);
}

DocumentSessionResult DocumentSession::undo() {
    if (!canUndo()) return success(false);

    const auto& entry = history_[cursor_ - 1U];
    auto applied = applyHistoricalState(entry.after, entry.before);
    if (!applied.ok()) return applied;
    --cursor_;
    return applied;
}

DocumentSessionResult DocumentSession::redo() {
    if (!canRedo()) return success(false);

    const auto& entry = history_[cursor_];
    auto applied = applyHistoricalState(entry.before, entry.after);
    if (!applied.ok()) return applied;
    ++cursor_;
    return applied;
}

DocumentSessionResult DocumentSession::save() {
    if (const auto verified = verifyRevision(); !verified.ok()) {
        return verified;
    }

    if (!file_checkpoint_) {
        return failure(
            DocumentSessionErrorCode::
                persistence_failure,
            "Document session has no native file checkpoint",
            path_,
            part::PartCommitErrorCode::none,
            part::PartStoreErrorCode::
                io_failure);
    }

    const auto saved =
        store_.save(
            path_,
            document_,
            *file_checkpoint_);
    if (!saved.ok()) {
        return failure(
            part::isSaveConflict(
                saved.diagnostic.code)
                ? DocumentSessionErrorCode::
                      save_conflict
                : DocumentSessionErrorCode::
                      persistence_failure,
            saved.diagnostic.message,
            saved.diagnostic.path,
            part::PartCommitErrorCode::none,
            saved.diagnostic.code);
    }

    file_checkpoint_ =
        *saved.checkpoint;
    saved_state_ = document_.state();
    return success(false);
}

} // namespace simplesolid2::application
