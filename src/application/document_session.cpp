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
                command.end);
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
                command.radius);
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
                command.sweep_angle);
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
            true,
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

    profile->name = command.name;
    profile->visible = command.visible;
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
    if (document_.state() != adjusted_expected) {
        return failure(
            DocumentSessionErrorCode::history_diverged,
            "Authored state no longer matches the Undo/Redo history cursor",
            path_);
    }

    auto adjusted_target = target;
    applySketchEntityIdCursors(adjusted_target);
    applyProfileIdCursor(adjusted_target);

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
