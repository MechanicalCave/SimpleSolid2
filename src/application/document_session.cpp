#include <simplesolid2/application/document_session.hpp>

#include <algorithm>
#include <cstddef>
#include <iterator>
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

part::PartAxis* findAxis(
    part::PartAuthoredState& state,
    part::AxisId id) noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found =
        std::find_if(
            state.axes.begin(),
            state.axes.end(),
            [id](const part::PartAxis& item) {
                return item.id == id;
            });
    return found == state.axes.end()
        ? nullptr
        : &*found;
}

bool axisSourceInUse(
    const part::PartAuthoredState& state,
    const part::SketchLineAxisSource& source,
    std::optional<part::AxisId> excluded_axis_id =
        std::nullopt) noexcept {
    return std::any_of(
        state.axes.begin(),
        state.axes.end(),
        [&source, excluded_axis_id](
            const part::PartAxis& axis) {
            return axis.source == source &&
                   (!excluded_axis_id ||
                    axis.id != *excluded_axis_id);
        });
}

part::OffsetDatumPlane* findDatumPlane(
    part::PartAuthoredState& state,
    part::DatumId id) noexcept {
    if (!id.valid()) {
        return nullptr;
    }
    const auto found =
        std::find_if(
            state.datum_planes.begin(),
            state.datum_planes.end(),
            [id](const part::OffsetDatumPlane& item) {
                return item.id == id;
            });
    return found == state.datum_planes.end()
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

std::string defaultRevolveFeatureName(
    part::FeatureId id) {
    const auto serialized = id.serialized();
    std::string result{"Revolve"};
    if (serialized.size() < 3U) {
        result.append(
            3U - serialized.size(),
            '0');
    }
    result += serialized;
    return result;
}


std::string defaultFilletFeatureName(
    part::FeatureId id) {
    const auto serialized = id.serialized();
    std::string result{"Fillet"};
    if (serialized.size() < 3U) {
        result.append(
            3U - serialized.size(),
            '0');
    }
    result += serialized;
    return result;
}

std::string defaultChamferFeatureName(
    part::FeatureId id) {
    const auto serialized = id.serialized();
    std::string result{"Chamfer"};
    if (serialized.size() < 3U) {
        result.append(
            3U - serialized.size(),
            '0');
    }
    result += serialized;
    return result;
}

std::string defaultAxisName(
    part::AxisId id) {
    const auto serialized = id.serialized();
    std::string result{"Axis"};
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

std::string axisEvaluationMessage(
    const part::AxisEvaluation& evaluated) {
    using Diagnostic =
        part::AxisEvaluationDiagnostic;
    switch (evaluated.diagnostic) {
    case Diagnostic::none:
        return {};
    case Diagnostic::invalid_reference:
        return "Axis reference is invalid";
    case Diagnostic::missing_axis:
        return "Axis does not exist";
    case Diagnostic::missing_sketch:
        return "Axis source Sketch is missing";
    case Diagnostic::missing_line:
        return "Axis source Line is missing";
    case Diagnostic::source_not_line:
        return "Axis source identity does not identify a Sketch Line";
    case Diagnostic::sketch_support_missing:
        return "Axis source Sketch support is missing";
    case Diagnostic::sketch_support_ambiguous:
        return "Axis source Sketch support is ambiguous";
    case Diagnostic::sketch_support_unsupported:
        return "Axis source Sketch support is unsupported";
    case Diagnostic::sketch_support_blocked:
        return "Axis source Sketch support is blocked";
    case Diagnostic::stale_part_evaluation:
        return "Axis evaluation requires the current Part evaluation";
    case Diagnostic::stale_datum_evaluation:
        return "Axis evaluation requires the current Datum evaluation";
    case Diagnostic::support_stage_unavailable:
        return "Axis source Sketch Body stage is unavailable";
    case Diagnostic::invalid_frame:
        return "Axis source resolves to an invalid world frame";
    }
    return "Axis source could not be resolved";
}

part::AxisEvaluation evaluateAxisInDocument(
    const part::PartDocument& document,
    part::AxisId axis_id,
    kernel::ISolidModelingKernel& modeling_kernel) {
    const part::AxisReference reference{
        part::AuthoredAxisReference{axis_id}};

    const auto* axis =
        document.findAxis(axis_id);
    if (axis == nullptr) {
        return part::resolveAxisReference(
            document,
            reference);
    }

    const auto* source =
        document.findSketch(
            axis->source.sketch_id);
    if (source == nullptr ||
        part::builtinOriginPlaneForSketchSupport(
            source->support)) {
        return part::resolveAxisReference(
            document,
            reference);
    }

    const auto evaluation =
        part::evaluatePart(
            document,
            modeling_kernel);

    if (part::datumPlaneIdForSketchSupport(
            source->support)) {
        const auto datums =
            part::evaluateDatums(
                document,
                evaluation);
        return part::resolveAxisReference(
            document,
            reference,
            &evaluation,
            &datums);
    }

    return part::resolveAxisReference(
        document,
        reference,
        &evaluation);
}

SketchSupportMutationResult supportMutationFailure(
    SketchSupportMutationStatus status,
    DocumentSessionErrorCode code,
    std::string message,
    const std::filesystem::path& path) {
    return {
        false,
        std::nullopt,
        status,
        DocumentSessionDiagnostic{
            code,
            part::PartCommitErrorCode::none,
            part::PartStoreErrorCode::none,
            std::move(message),
            path}};
}

const part::BodyStageTopologyCatalog*
topologyAtStage(
    const part::PartEvaluation& evaluation,
    const part::BodyStageRef& stage) noexcept {
    if (!stage.valid()) {
        return nullptr;
    }
    for (const auto& feature : evaluation.features) {
        if (feature.status !=
                part::FeatureEvaluationStatus::
                    up_to_date ||
            !feature.result_topology ||
            feature.result_topology->stage != stage) {
            continue;
        }
        return &*feature.result_topology;
    }
    return nullptr;
}

SketchSupportMutationStatus supportMutationStatus(
    part::SketchSupportResolutionStatus status) noexcept {
    switch (status) {
    case part::SketchSupportResolutionStatus::resolved:
        return SketchSupportMutationStatus::applied;
    case part::SketchSupportResolutionStatus::missing:
        return SketchSupportMutationStatus::missing;
    case part::SketchSupportResolutionStatus::ambiguous:
        return SketchSupportMutationStatus::ambiguous;
    case part::SketchSupportResolutionStatus::unsupported:
        return SketchSupportMutationStatus::unsupported;
    }
    return SketchSupportMutationStatus::invalid_support;
}

std::string supportResolutionMessage(
    const part::ResolvedSketchSupport& resolved) {
    using Diagnostic =
        part::SketchSupportResolutionDiagnostic;
    switch (resolved.diagnostic) {
    case Diagnostic::none:
        return {};
    case Diagnostic::invalid_support:
        return "Sketch support is structurally invalid";
    case Diagnostic::missing_stage:
        return "Sketch support Body stage is unavailable in the current evaluation";
    case Diagnostic::missing_surface:
        return "Sketch support Surface is missing at its declared Body stage";
    case Diagnostic::ambiguous_surface:
        return "Sketch support Surface is ambiguous at its declared Body stage";
    case Diagnostic::unsupported_non_planar:
        return "Selected Surface is non-planar and cannot host a standard Sketch";
    case Diagnostic::unsupported_surface:
        return "Selected Surface is unsupported for standard Sketch support";
    case Diagnostic::missing_datum:
        return "Sketch support Datum Plane is missing";
    case Diagnostic::ambiguous_datum:
        return "Sketch support Datum Plane is ambiguous";
    case Diagnostic::unsupported_datum:
        return "Sketch support Datum Plane is unsupported";
    case Diagnostic::blocked_datum:
        return "Sketch support Datum Plane is blocked by unavailable upstream geometry";
    }
    return "Sketch support could not be resolved";
}

std::optional<part::ResolvedSketchSupport>
resolveSupportForMutation(
    const part::PartDocument& document,
    const part::PartSketchSupport& support,
    kernel::ISolidModelingKernel* modeling_kernel) {
    if (!support.valid()) {
        return std::nullopt;
    }
    if (part::builtinOriginPlaneForSketchSupport(
            support)) {
        return part::resolveSketchSupport(
            support);
    }

    if (modeling_kernel == nullptr) {
        return std::nullopt;
    }

    const auto evaluation =
        part::evaluatePart(
            document,
            *modeling_kernel);

    if (part::datumPlaneIdForSketchSupport(
            support)) {
        const auto datums =
            part::evaluateDatums(
                document,
                evaluation);
        return part::resolveSketchSupport(
            support,
            nullptr,
            &datums);
    }

    const auto* reference =
        part::bodyPlanarSurfaceReference(
            support);
    if (reference == nullptr) {
        return std::nullopt;
    }

    const auto* topology =
        topologyAtStage(
            evaluation,
            reference->stage);
    return part::resolveSketchSupport(
        support,
        topology);
}

std::optional<part::BodyStageRef>
requiredBodyStageForSketchSupport(
    const part::PartAuthoredState& state,
    const part::PartSketchSupport& support) noexcept {
    if (const auto* reference =
            part::bodyPlanarSurfaceReference(
                support)) {
        return reference->stage;
    }

    auto datum_id =
        part::datumPlaneIdForSketchSupport(
            support);
    std::set<part::DatumId> visited;
    while (datum_id) {
        if (!visited.insert(*datum_id).second) {
            return std::nullopt;
        }

        const auto found =
            std::find_if(
                state.datum_planes.begin(),
                state.datum_planes.end(),
                [datum_id](
                    const part::OffsetDatumPlane& datum) {
                    return datum.id == *datum_id;
                });
        if (found ==
            state.datum_planes.end()) {
            return std::nullopt;
        }

        if (const auto* surface =
                part::bodyPlanarSurfaceForPlaneReference(
                    found->source)) {
            return surface->stage;
        }

        datum_id =
            part::datumPlaneIdForPlaneReference(
                found->source);
    }

    return std::nullopt;
}

bool supportWouldCreateCycle(
    const part::PartAuthoredState& state,
    sketch::SketchId sketch_id,
    const part::PartSketchSupport& support) noexcept {
    const auto required_stage =
        requiredBodyStageForSketchSupport(
            state,
            support);
    if (!required_stage) {
        return false;
    }

    const auto stage_it =
        std::find_if(
            state.body.features.begin(),
            state.body.features.end(),
            [required_stage](
                const part::PartFeature& feature) {
                return required_stage->feature_id &&
                       feature.id ==
                           *required_stage->feature_id;
            });
    if (stage_it == state.body.features.end()) {
        return true;
    }
    const auto stage_index =
        static_cast<std::size_t>(
            std::distance(
                state.body.features.begin(),
                stage_it));

    std::set<part::ProfileId> consumed_profiles;
    for (const auto& profile : state.profiles) {
        if (profile.source_sketch_id == sketch_id) {
            consumed_profiles.insert(profile.id);
        }
    }

    for (std::size_t index = 0U;
         index < state.body.features.size();
         ++index) {
        const auto source =
            part::sourceProfileId(
                state.body.features[index]);
        if (!source ||
            consumed_profiles.find(*source) ==
                consumed_profiles.end()) {
            continue;
        }
        if (stage_index >= index) {
            return true;
        }
    }
    return false;
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
    axis_id_cursor_.preserve(
        document_.state().next_axis_id);
    datum_id_cursor_.preserve(
        document_.state().next_datum_id);
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
    axis_id_cursor_.preserve(
        document_.state().next_axis_id);
    datum_id_cursor_.preserve(
        document_.state().next_datum_id);
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
    applyAxisIdCursor(after);
    applyDatumIdCursor(after);
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
    auto prepared_axis_id_cursor =
        axis_id_cursor_;
    prepared_axis_id_cursor.preserve(
        pending.after.next_axis_id);
    auto prepared_datum_id_cursor =
        datum_id_cursor_;
    prepared_datum_id_cursor.preserve(
        pending.after.next_datum_id);
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
    axis_id_cursor_ =
        prepared_axis_id_cursor;
    datum_id_cursor_ =
        prepared_datum_id_cursor;
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

SketchSupportMutationResult DocumentSession::execute(
    const CreatePartSketchOnSupportCommand& command,
    kernel::ISolidModelingKernel* modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                stale_revision,
            DocumentSessionErrorCode::
                revision_diverged,
            "Create Sketch support selection is stale",
            path_);
    }

    const auto resolved =
        resolveSupportForMutation(
            document_,
            command.support,
            modeling_kernel);
    if (!resolved) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                invalid_support,
            DocumentSessionErrorCode::
                invalid_command,
            "Create Sketch contains an invalid support reference",
            path_);
    }
    if (resolved->status !=
        part::SketchSupportResolutionStatus::
            resolved) {
        return supportMutationFailure(
            supportMutationStatus(
                resolved->status),
            DocumentSessionErrorCode::
                invalid_command,
            supportResolutionMessage(*resolved),
            path_);
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
        return supportMutationFailure(
            SketchSupportMutationStatus::
                evaluation_failure,
            DocumentSessionErrorCode::
                transaction_failure,
            "Unable to allocate a unique SketchId",
            path_);
    }

    auto after = document_.state();
    after.sketches.push_back(
        part::PartSketch{
            *id,
            command.support,
            true});

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating supported Sketch");
    if (!committed.ok() ||
        !committed.changed) {
        return {
            committed.changed,
            std::nullopt,
            committed.ok()
                ? SketchSupportMutationStatus::
                      no_change
                : SketchSupportMutationStatus::
                      evaluation_failure,
            committed.diagnostic};
    }

    return {
        true,
        *id,
        SketchSupportMutationStatus::applied,
        DocumentSessionDiagnostic{}};
}

SketchSupportMutationResult DocumentSession::execute(
    const SetPartSketchSupportCommand& command,
    kernel::ISolidModelingKernel* modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                stale_revision,
            DocumentSessionErrorCode::
                revision_diverged,
            "Re-support selection is stale",
            path_);
    }
    const auto* existing =
        document_.findSketch(
            command.sketch_id);
    if (existing == nullptr) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                missing_sketch,
            DocumentSessionErrorCode::
                invalid_command,
            "Re-support target SketchId does not exist",
            path_);
    }
    if (!command.support.valid()) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                invalid_support,
            DocumentSessionErrorCode::
                invalid_command,
            "Re-support contains an invalid support reference",
            path_);
    }
    const auto resolved =
        resolveSupportForMutation(
            document_,
            command.support,
            modeling_kernel);
    if (!resolved) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                invalid_support,
            DocumentSessionErrorCode::
                invalid_command,
            "Re-support contains an invalid support reference",
            path_);
    }
    if (resolved->status !=
        part::SketchSupportResolutionStatus::
            resolved) {
        return supportMutationFailure(
            supportMutationStatus(
                resolved->status),
            DocumentSessionErrorCode::
                invalid_command,
            supportResolutionMessage(*resolved),
            path_);
    }

    if (existing->support == command.support) {
        return {
            false,
            command.sketch_id,
            SketchSupportMutationStatus::
                no_change,
            DocumentSessionDiagnostic{}};
    }

    if (supportWouldCreateCycle(
            document_.state(),
            command.sketch_id,
            command.support)) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                cycle_dependency,
            DocumentSessionErrorCode::
                invalid_command,
            "Re-support would create a Sketch -> Profile -> Feature dependency cycle",
            path_);
    }

    auto after = document_.state();
    auto* target =
        findSketch(
            after,
            command.sketch_id);
    if (target == nullptr) {
        return supportMutationFailure(
            SketchSupportMutationStatus::
                missing_sketch,
            DocumentSessionErrorCode::
                invalid_command,
            "Re-support target disappeared before commit",
            path_);
    }

    target->support = command.support;
    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while changing Sketch support");
    if (!committed.ok()) {
        return {
            false,
            std::nullopt,
            SketchSupportMutationStatus::
                evaluation_failure,
            committed.diagnostic};
    }
    return {
        committed.changed,
        command.sketch_id,
        committed.changed
            ? SketchSupportMutationStatus::applied
            : SketchSupportMutationStatus::no_change,
        committed.diagnostic};
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

AddSketchLineWithAxisResult DocumentSession::execute(
    const AddSketchLineWithAxisCommand& command,
    kernel::ISolidModelingKernel&
        modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    revision_diverged,
                "Add Sketch Line with Axis was started from a stale DocumentRevision",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applySketchEntityIdCursors(after);
    applyAxisIdCursor(after);

    auto* target =
        findSketch(after, command.sketch_id);
    if (target == nullptr) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Add Sketch Line with Axis target SketchId does not exist",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
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
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Add Sketch Line with Axis contains invalid authored geometry",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    } catch (const std::overflow_error&) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "Sketch EntityId allocation space is exhausted",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const part::SketchLineAxisSource source{
        command.sketch_id,
        *entity_id};
    if (axisSourceInUse(after, source)) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "New Sketch Line source is already designated by an authored Axis",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const auto axis_id =
        after.next_axis_id.allocate();
    if (!axis_id) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "AxisId allocation space is exhausted",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    after.axes.push_back(
        part::PartAxis{
            *axis_id,
            command.axis_name.empty()
                ? defaultAxisName(*axis_id)
                : command.axis_name,
            source,
            command.axis_visible});

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "Add Sketch Line with Axis produced invalid authored Part state",
                path_,
                part::PartCommitErrorCode::
                    invalid_state);
        return {
            false,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const auto evaluated =
        evaluateAxisInDocument(
            *candidate.document,
            *axis_id,
            modeling_kernel);
    if (evaluated.status !=
            part::AxisEvaluationStatus::resolved ||
        !evaluated.line ||
        !evaluated.line->valid()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                axisEvaluationMessage(evaluated),
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            evaluated.diagnostic,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while adding Sketch Line with Axis");
    if (!committed.ok() ||
        !committed.changed) {
        return {
            committed.changed,
            std::nullopt,
            std::nullopt,
            std::nullopt,
            committed.diagnostic};
    }

    return {
        true,
        *entity_id,
        *axis_id,
        part::AxisEvaluationDiagnostic::none,
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


AxisDraftEvaluationResult
DocumentSession::evaluateAxisDraft(
    const AxisDraft& draft,
    kernel::ISolidModelingKernel&
        modeling_kernel) const {
    AxisDraftEvaluationResult result;
    result.document_id = draft.documentId();
    result.source_revision = draft.sourceRevision();
    result.draft_generation = draft.generation();
    result.mode = draft.mode();
    result.authored_axis_id = draft.axisId();
    result.source = draft.source();

    if (documentId() != draft.documentId()) {
        result.status =
            AxisDraftEvaluationStatus::
                stale_document;
        return result;
    }
    if (document_.revision() !=
        draft.sourceRevision()) {
        result.status =
            AxisDraftEvaluationStatus::
                stale_revision;
        return result;
    }
    if (!draft.valid() || !draft.source()) {
        result.status =
            AxisDraftEvaluationStatus::
                invalid_draft;
        return result;
    }

    if (axisSourceInUse(
            document_.state(),
            *draft.source(),
            draft.mode() == AxisDraftMode::edit
                ? draft.axisId()
                : std::nullopt)) {
        result.status =
            AxisDraftEvaluationStatus::
                source_already_designated;
        return result;
    }

    auto after = document_.state();
    applyAxisIdCursor(after);

    part::AxisId target_id;
    if (draft.mode() == AxisDraftMode::create) {
        const auto allocated =
            after.next_axis_id.allocate();
        if (!allocated) {
            result.status =
                AxisDraftEvaluationStatus::
                    id_exhausted;
            return result;
        }
        target_id = *allocated;
        after.axes.push_back(
            part::PartAxis{
                target_id,
                defaultAxisName(target_id),
                *draft.source(),
                true});
    } else {
        const auto authored_id =
            draft.axisId();
        if (!authored_id) {
            result.status =
                AxisDraftEvaluationStatus::
                    invalid_draft;
            return result;
        }
        auto* axis =
            findAxis(after, *authored_id);
        if (axis == nullptr) {
            result.status =
                AxisDraftEvaluationStatus::
                    missing_axis;
            return result;
        }
        target_id = axis->id;
        axis->source = *draft.source();
    }

    result.candidate_axis_id = target_id;

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            std::move(after),
            document_.revision());
    if (!candidate.ok()) {
        result.status =
            AxisDraftEvaluationStatus::
                invalid_candidate;
        return result;
    }

    const auto evaluated =
        evaluateAxisInDocument(
            *candidate.document,
            target_id,
            modeling_kernel);
    result.axis_status = evaluated.status;
    result.axis_diagnostic = evaluated.diagnostic;
    result.line = evaluated.line;
    result.required_body_stage =
        evaluated.required_body_stage;

    if (evaluated.status !=
            part::AxisEvaluationStatus::resolved ||
        !evaluated.line ||
        !evaluated.line->valid()) {
        result.status =
            AxisDraftEvaluationStatus::
                source_unresolved;
        result.line.reset();
        return result;
    }

    result.status =
        AxisDraftEvaluationStatus::ok;
    return result;
}

CreateAxisResult DocumentSession::execute(
    const CreateAxisCommand& command,
    kernel::ISolidModelingKernel&
        modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    revision_diverged,
                "Create Axis was started from a stale DocumentRevision",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }
    if (!command.source.valid()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Axis contains an invalid Sketch-Line source",
                path_);
        return {
            false,
            std::nullopt,
            part::AxisEvaluationDiagnostic::
                invalid_reference,
            failed.diagnostic};
    }
    if (axisSourceInUse(
            document_.state(),
            command.source)) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Axis source Line is already designated by another authored Axis",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applyAxisIdCursor(after);
    const auto id =
        after.next_axis_id.allocate();
    if (!id) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "AxisId allocation space is exhausted",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    after.axes.push_back(
        part::PartAxis{
            *id,
            command.name.empty()
                ? defaultAxisName(*id)
                : command.name,
            command.source,
            command.visible});

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "Create Axis produced invalid authored Part state",
                path_,
                part::PartCommitErrorCode::
                    invalid_state);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const auto evaluated =
        evaluateAxisInDocument(
            *candidate.document,
            *id,
            modeling_kernel);
    if (evaluated.status !=
            part::AxisEvaluationStatus::resolved ||
        !evaluated.line ||
        !evaluated.line->valid()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                axisEvaluationMessage(evaluated),
                path_);
        return {
            false,
            std::nullopt,
            evaluated.diagnostic,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating Axis");
    if (!committed.ok()) {
        return {
            false,
            std::nullopt,
            std::nullopt,
            committed.diagnostic};
    }

    return {
        committed.changed,
        *id,
        part::AxisEvaluationDiagnostic::none,
        DocumentSessionDiagnostic{}};
}

DocumentSessionResult DocumentSession::execute(
    const EditAxisCommand& command,
    kernel::ISolidModelingKernel&
        modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Edit Axis was started from a stale DocumentRevision",
            path_);
    }
    if (!command.axis_id.valid() ||
        !command.source.valid()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Axis contains an invalid target or source",
            path_);
    }

    auto after = document_.state();
    auto* axis =
        findAxis(
            after,
            command.axis_id);
    if (axis == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Axis target AxisId does not exist",
            path_);
    }
    if (axisSourceInUse(
            after,
            command.source,
            command.axis_id)) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Axis target source Line is already designated by another authored Axis",
            path_);
    }

    axis->source = command.source;

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        return failure(
            DocumentSessionErrorCode::
                transaction_failure,
            "Edit Axis produced invalid authored Part state",
            path_,
            part::PartCommitErrorCode::
                invalid_state);
    }

    const auto evaluated =
        evaluateAxisInDocument(
            *candidate.document,
            command.axis_id,
            modeling_kernel);
    if (evaluated.status !=
            part::AxisEvaluationStatus::resolved ||
        !evaluated.line ||
        !evaluated.line->valid()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            axisEvaluationMessage(evaluated),
            path_);
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while editing Axis");
}

DocumentSessionResult DocumentSession::execute(
    const SetAxisVisibilityCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Axis visibility change was started from a stale DocumentRevision",
            path_);
    }

    std::set<part::AxisId> unique_targets;
    for (const auto id : command.targets) {
        if (!id.valid() ||
            document_.findAxis(id) == nullptr) {
            return failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Axis visibility command contains a missing or invalid AxisId",
                path_);
        }
        unique_targets.insert(id);
    }
    if (unique_targets.empty()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Axis visibility command requires at least one AxisId",
            path_);
    }

    auto after = document_.state();
    for (auto& axis : after.axes) {
        if (unique_targets.contains(axis.id)) {
            axis.visible = command.visible;
        }
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while changing Axis visibility");
}

DocumentSessionResult DocumentSession::execute(
    const DeleteAxisCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Delete Axis was started from a stale DocumentRevision",
            path_);
    }
    if (!command.axis_id.valid()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Delete Axis contains an invalid AxisId",
            path_);
    }

    auto after = document_.state();
    const auto found =
        std::find_if(
            after.axes.begin(),
            after.axes.end(),
            [&command](const part::PartAxis& axis) {
                return axis.id ==
                       command.axis_id;
            });
    if (found == after.axes.end()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Delete Axis target AxisId does not exist",
            path_);
    }

    after.axes.erase(found);
    return commitCommandState(
        std::move(after),
        "Part transaction failed while deleting Axis");
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

    // Full candidate evaluation above remains the sole authority for
    // whether Finish is legal. Preview is a separate exact operation delta:
    // Add = tool - upstream Body, Cut = tool ∩ upstream Body.
    kernel::RuntimeSolidHandle preview_upstream;
    bool preview_upstream_ready = false;
    const auto& candidate_features =
        candidate.document->body().features;
    const auto target_authored =
        std::find_if(
            candidate_features.begin(),
            candidate_features.end(),
            [target_id](
                const part::PartFeature& feature) {
                return feature.id == target_id;
            });

    if (target_authored !=
        candidate_features.end()) {
        const auto target_index =
            static_cast<std::size_t>(
                std::distance(
                    candidate_features.begin(),
                    target_authored));
        if (target_index == 0U) {
            preview_upstream_ready = true;
        } else {
            auto upstream_state =
                candidate.document->state();
            upstream_state.body.features.erase(
                upstream_state.body.features.begin() +
                    static_cast<std::ptrdiff_t>(
                        target_index),
                upstream_state.body.features.end());

            auto upstream_document =
                part::PartDocument::restore(
                    documentId(),
                    std::move(upstream_state),
                    document_.revision());
            if (upstream_document.ok()) {
                const auto upstream_evaluation =
                    part::evaluatePart(
                        *upstream_document.document,
                        modeling_kernel);
                if (upstream_evaluation.body_status ==
                    part::BodyEvaluationStatus::
                        up_to_date) {
                    preview_upstream =
                        upstream_evaluation.body_solid;
                    preview_upstream_ready =
                        preview_upstream != nullptr;
                } else if (
                    upstream_evaluation.body_status ==
                    part::BodyEvaluationStatus::empty) {
                    preview_upstream_ready = true;
                }
            }
        }
    }

    const part::BodyStageTopologyCatalog*
        preview_support_topology = nullptr;
    std::optional<part::DatumEvaluation>
        preview_support_datums;
    if (const auto* profile =
            candidate.document->findProfile(
                definition.profile_id)) {
        if (const auto* source =
                candidate.document->findSketch(
                    profile->source_sketch_id)) {
            if (const auto* support =
                    part::bodyPlanarSurfaceReference(
                        source->support)) {
                preview_support_topology =
                    topologyAtStage(
                        evaluation,
                        support->stage);
            } else if (
                part::datumPlaneIdForSketchSupport(
                    source->support)) {
                preview_support_datums =
                    part::evaluateDatums(
                        *candidate.document,
                        evaluation);
            }
        }
    }

    auto preview_input =
        part::makeKernelExtrudeInput(
            *candidate.document,
            definition,
            preview_support_topology,
            preview_support_datums
                ? &*preview_support_datums
                : nullptr);
    if (preview_input &&
        preview_upstream_ready) {
        auto preview =
            modeling_kernel.extrudePreviewMesh(
                *preview_input,
                preview_upstream);
        if (preview.ok()) {
            result.preview_delta_mesh =
                std::move(preview.mesh);
        }
    }

    result.status =
        ExtrudeDraftEvaluationStatus::ok;
    return result;
}

RevolveDraftEvaluationResult
DocumentSession::evaluateRevolveDraft(
    const RevolveDraft& draft,
    kernel::ISolidModelingKernel&
        modeling_kernel) const {
    RevolveDraftEvaluationResult result;
    result.document_id = draft.documentId();
    result.source_revision =
        draft.sourceRevision();
    result.draft_generation =
        draft.generation();
    result.mode =
        draft.mode();
    result.profile_id =
        draft.profileId();
    result.axis =
        draft.axis();
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
            RevolveDraftEvaluationStatus::
                stale_document;
        return result;
    }
    if (document_.revision() !=
        draft.sourceRevision()) {
        result.status =
            RevolveDraftEvaluationStatus::
                stale_revision;
        return result;
    }
    if (!draft.profileId() ||
        !draft.axis() ||
        !draft.valid()) {
        result.status =
            RevolveDraftEvaluationStatus::
                incomplete_draft;
        return result;
    }
    if (document_.findProfile(
            *draft.profileId()) == nullptr) {
        result.status =
            RevolveDraftEvaluationStatus::
                missing_profile;
        return result;
    }

    auto after =
        document_.state();
    applyBodyFeatureIdCursors(after);

    part::FeatureId target_id;
    const part::RevolveFeature definition{
        *draft.profileId(),
        *draft.axis(),
        draft.operation(),
        draft.extent()};

    if (draft.mode() ==
        RevolveDraftMode::create) {
        const auto id =
            after.body.next_feature_id
                .allocate();
        if (!id) {
            result.status =
                RevolveDraftEvaluationStatus::
                    feature_id_exhausted;
            return result;
        }
        target_id = *id;
        after.body.features.push_back(
            part::PartFeature{
                target_id,
                draft.name().empty()
                    ? defaultRevolveFeatureName(
                          target_id)
                    : draft.name(),
                false,
                definition});
    } else {
        if (!draft.featureId()) {
            result.status =
                RevolveDraftEvaluationStatus::
                    incomplete_draft;
            return result;
        }
        auto* feature =
            findFeature(
                after,
                *draft.featureId());
        if (feature == nullptr) {
            result.status =
                RevolveDraftEvaluationStatus::
                    missing_feature;
            return result;
        }
        if (feature->suppressed) {
            result.status =
                RevolveDraftEvaluationStatus::
                    suppressed_feature;
            return result;
        }
        if (std::get_if<part::RevolveFeature>(
                &feature->definition) == nullptr) {
            result.status =
                RevolveDraftEvaluationStatus::
                    invalid_candidate;
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
            RevolveDraftEvaluationStatus::
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
            RevolveDraftEvaluationStatus::
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
            RevolveDraftEvaluationStatus::
                target_failed;
        result.body_solid.reset();
        return result;
    }

    // Finish legality is established solely by the complete candidate
    // evaluation above. Preview is a separate presentation-only exact delta
    // computed from the exact Body prefix immediately before this Feature.
    const auto& candidate_features =
        candidate.document->body().features;
    const auto target_authored =
        std::find_if(
            candidate_features.begin(),
            candidate_features.end(),
            [target_id](
                const part::PartFeature& feature) {
                return feature.id ==
                       target_id;
            });

    if (target_authored !=
        candidate_features.end()) {
        const auto target_index =
            static_cast<std::size_t>(
                std::distance(
                    candidate_features.begin(),
                    target_authored));

        auto upstream_state =
            candidate.document->state();
        upstream_state.body.features.erase(
            upstream_state.body.features.begin() +
                static_cast<std::ptrdiff_t>(
                    target_index),
            upstream_state.body.features.end());

        auto upstream_document =
            part::PartDocument::restore(
                documentId(),
                std::move(upstream_state),
                document_.revision());
        if (upstream_document.ok()) {
            const auto prefix_evaluation =
                part::evaluatePart(
                    *upstream_document.document,
                    modeling_kernel);

            kernel::RuntimeSolidHandle
                preview_upstream;
            bool preview_upstream_ready = false;
            if (prefix_evaluation.body_status ==
                part::BodyEvaluationStatus::
                    up_to_date) {
                preview_upstream =
                    prefix_evaluation.body_solid;
                preview_upstream_ready =
                    preview_upstream != nullptr;
            } else if (
                prefix_evaluation.body_status ==
                part::BodyEvaluationStatus::empty) {
                preview_upstream_ready = true;
            }

            const auto datums =
                part::evaluateDatums(
                    *candidate.document,
                    prefix_evaluation);
            const auto resolved =
                part::resolveKernelRevolveInput(
                    *candidate.document,
                    definition,
                    &prefix_evaluation,
                    &datums);
            if (resolved.ok() &&
                preview_upstream_ready) {
                auto preview =
                    modeling_kernel.revolvePreviewMesh(
                        *resolved.input,
                        preview_upstream);
                if (preview.ok()) {
                    result.preview_delta_mesh =
                        std::move(preview.mesh);
                }
            }
        }
    }

    result.status =
        RevolveDraftEvaluationStatus::ok;
    return result;
}


EdgeFeatureDraftEvaluationResult
DocumentSession::evaluateFilletDraft(
    const FilletDraft& draft,
    kernel::ISolidModelingKernel&
        modeling_kernel) const {
    EdgeFeatureDraftEvaluationResult result;
    result.document_id = draft.documentId();
    result.source_revision = draft.sourceRevision();
    result.draft_generation = draft.generation();
    result.operation =
        kernel::EdgeFeatureOperation::fillet;
    result.edges = draft.edges();
    result.parameter = draft.radius();

    if (documentId() != draft.documentId()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                stale_document;
        return result;
    }
    if (document_.revision() !=
        draft.sourceRevision()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                stale_revision;
        return result;
    }
    if (!draft.valid() ||
        !draft.radius() ||
        !draft.requiredStage()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                incomplete_draft;
        return result;
    }

    auto after = document_.state();
    applyBodyFeatureIdCursors(after);
    const auto id =
        after.body.next_feature_id.allocate();
    if (!id) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                feature_id_exhausted;
        return result;
    }

    const part::FilletFeature definition{
        draft.edges(),
        *draft.radius()};
    after.body.features.push_back(
        part::PartFeature{
            *id,
            draft.name().empty()
                ? defaultFilletFeatureName(*id)
                : draft.name(),
            false,
            definition});

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            std::move(after),
            document_.revision());
    if (!candidate.ok()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                invalid_candidate;
        return result;
    }

    const auto evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    result.body_status = evaluation.body_status;
    result.body_solid = evaluation.body_solid;

    const auto* target =
        evaluation.findFeature(*id);
    if (target == nullptr) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                invalid_candidate;
        result.body_solid.reset();
        return result;
    }

    result.evaluation_diagnostic =
        target->diagnostic;
    result.failing_edge_input_index =
        target->failing_edge_input_index;
    result.edge_reference_status =
        target->edge_reference_status;
    if (target->status !=
        part::FeatureEvaluationStatus::
            up_to_date) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                target_failed;
        result.body_solid.reset();
        return result;
    }

    if (result.body_solid != nullptr) {
        auto preview =
            modeling_kernel.presentationMesh(
                result.body_solid);
        if (preview.ok()) {
            result.preview_mesh =
                std::move(preview.mesh);
        }
    }

    result.status =
        EdgeFeatureDraftEvaluationStatus::ok;
    return result;
}

EdgeFeatureDraftEvaluationResult
DocumentSession::evaluateChamferDraft(
    const ChamferDraft& draft,
    kernel::ISolidModelingKernel&
        modeling_kernel) const {
    EdgeFeatureDraftEvaluationResult result;
    result.document_id = draft.documentId();
    result.source_revision = draft.sourceRevision();
    result.draft_generation = draft.generation();
    result.operation =
        kernel::EdgeFeatureOperation::chamfer;
    result.edges = draft.edges();
    result.parameter = draft.distance();

    if (documentId() != draft.documentId()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                stale_document;
        return result;
    }
    if (document_.revision() !=
        draft.sourceRevision()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                stale_revision;
        return result;
    }
    if (!draft.valid() ||
        !draft.distance() ||
        !draft.requiredStage()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                incomplete_draft;
        return result;
    }

    auto after = document_.state();
    applyBodyFeatureIdCursors(after);
    const auto id =
        after.body.next_feature_id.allocate();
    if (!id) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                feature_id_exhausted;
        return result;
    }

    const part::ChamferFeature definition{
        draft.edges(),
        *draft.distance()};
    after.body.features.push_back(
        part::PartFeature{
            *id,
            draft.name().empty()
                ? defaultChamferFeatureName(*id)
                : draft.name(),
            false,
            definition});

    auto candidate =
        part::PartDocument::restore(
            documentId(),
            std::move(after),
            document_.revision());
    if (!candidate.ok()) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                invalid_candidate;
        return result;
    }

    const auto evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    result.body_status = evaluation.body_status;
    result.body_solid = evaluation.body_solid;

    const auto* target =
        evaluation.findFeature(*id);
    if (target == nullptr) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                invalid_candidate;
        result.body_solid.reset();
        return result;
    }

    result.evaluation_diagnostic =
        target->diagnostic;
    result.failing_edge_input_index =
        target->failing_edge_input_index;
    result.edge_reference_status =
        target->edge_reference_status;
    if (target->status !=
        part::FeatureEvaluationStatus::
            up_to_date) {
        result.status =
            EdgeFeatureDraftEvaluationStatus::
                target_failed;
        result.body_solid.reset();
        return result;
    }

    if (result.body_solid != nullptr) {
        auto preview =
            modeling_kernel.presentationMesh(
                result.body_solid);
        if (preview.ok()) {
            result.preview_mesh =
                std::move(preview.mesh);
        }
    }

    result.status =
        EdgeFeatureDraftEvaluationStatus::ok;
    return result;
}

DatumPlaneDraftEvaluationResult
DocumentSession::evaluateDatumPlaneDraft(
    const DatumPlaneDraft& draft,
    kernel::ISolidModelingKernel& modeling_kernel) const {
    DatumPlaneDraftEvaluationResult result;
    result.document_id = draft.documentId();
    result.source_revision = draft.sourceRevision();
    result.draft_generation = draft.generation();
    result.mode = draft.mode();
    result.authored_datum_id = draft.datumId();
    result.source = draft.source();
    result.offset = draft.offset();

    if (draft.documentId() != document_.documentId()) {
        result.status =
            DatumPlaneDraftEvaluationStatus::
                stale_document;
        return result;
    }
    if (draft.sourceRevision() !=
        document_.revision()) {
        result.status =
            DatumPlaneDraftEvaluationStatus::
                stale_revision;
        return result;
    }
    if (!draft.valid() || !draft.source()) {
        result.status =
            DatumPlaneDraftEvaluationStatus::
                invalid_draft;
        return result;
    }

    auto after = document_.state();
    applyDatumIdCursor(after);

    part::DatumId target_id;
    if (draft.mode() ==
        DatumPlaneDraftMode::create) {
        const auto allocated =
            after.next_datum_id.allocate();
        if (!allocated) {
            result.status =
                DatumPlaneDraftEvaluationStatus::
                    id_exhausted;
            return result;
        }
        target_id = *allocated;
        after.datum_planes.push_back(
            part::OffsetDatumPlane{
                target_id,
                *draft.source(),
                draft.offset(),
                true});
    } else {
        const auto authored_id =
            draft.datumId();
        if (!authored_id) {
            result.status =
                DatumPlaneDraftEvaluationStatus::
                    invalid_draft;
            return result;
        }
        auto* target =
            findDatumPlane(after, *authored_id);
        if (target == nullptr) {
            result.status =
                DatumPlaneDraftEvaluationStatus::
                    missing_datum;
            return result;
        }
        target_id = *authored_id;
        target->source = *draft.source();
        target->offset = draft.offset();
    }
    result.candidate_datum_id = target_id;

    auto candidate =
        part::PartDocument::restore(
            document_.documentId(),
            std::move(after),
            document_.revision());
    if (!candidate.ok()) {
        result.status =
            DatumPlaneDraftEvaluationStatus::
                invalid_candidate;
        return result;
    }

    const auto part_evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    const auto datum_evaluation =
        part::evaluateDatums(
            *candidate.document,
            part_evaluation);
    const auto* evaluated =
        datum_evaluation.find(target_id);
    if (evaluated == nullptr ||
        !evaluated->valid()) {
        result.status =
            DatumPlaneDraftEvaluationStatus::
                invalid_candidate;
        return result;
    }

    result.datum_status = evaluated->status;
    result.datum_diagnostic =
        evaluated->diagnostic;
    result.frame = evaluated->frame;
    result.required_body_stage =
        evaluated->required_body_stage;

    if (evaluated->status !=
            part::DatumPlaneEvaluationStatus::
                resolved ||
        !evaluated->frame) {
        result.status =
            DatumPlaneDraftEvaluationStatus::
                source_unresolved;
        return result;
    }

    result.status =
        DatumPlaneDraftEvaluationStatus::ok;
    return result;
}

CreateDatumPlaneResult DocumentSession::execute(
    const CreateDatumPlaneCommand& command,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    revision_diverged,
                "Create Datum Plane was started from a stale DocumentRevision",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }
    if (!command.source.valid() ||
        !command.offset.finite()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Datum Plane contains an invalid source or offset",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    auto after = document_.state();
    applyDatumIdCursor(after);
    const auto id =
        after.next_datum_id.allocate();
    if (!id) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    transaction_failure,
                "DatumId allocation space is exhausted",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    after.datum_planes.push_back(
        part::OffsetDatumPlane{
            *id,
            command.source,
            command.offset,
            command.visible});

    auto candidate =
        part::PartDocument::restore(
            document_.documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Datum Plane candidate violates Part authored-state invariants",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const auto part_evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    const auto datum_evaluation =
        part::evaluateDatums(
            *candidate.document,
            part_evaluation);
    const auto* target =
        datum_evaluation.find(*id);
    if (target == nullptr ||
        target->status !=
            part::DatumPlaneEvaluationStatus::
                resolved ||
        !target->frame) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Datum Plane source is not currently Resolved; no authored mutation committed",
                path_);
        return {
            false,
            std::nullopt,
            target != nullptr
                ? std::optional<
                      part::DatumPlaneEvaluationDiagnostic>{
                      target->diagnostic}
                : std::nullopt,
            failed.diagnostic};
    }

    const auto committed =
        commitCommandState(
            std::move(after),
            "Part transaction failed while creating Datum Plane");
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
        part::DatumPlaneEvaluationDiagnostic::none,
        DocumentSessionDiagnostic{}};
}

DocumentSessionResult DocumentSession::execute(
    const EditDatumPlaneCommand& command,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Edit Datum Plane was started from a stale DocumentRevision",
            path_);
    }
    if (!command.datum_id.valid() ||
        !command.source.valid() ||
        !command.offset.finite()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Datum Plane contains an invalid target, source or offset",
            path_);
    }

    auto after = document_.state();
    auto* target =
        findDatumPlane(
            after,
            command.datum_id);
    if (target == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Datum Plane target DatumId does not exist",
            path_);
    }
    if (target->source == command.source &&
        target->offset == command.offset) {
        return success(false);
    }

    target->source = command.source;
    target->offset = command.offset;

    auto candidate =
        part::PartDocument::restore(
            document_.documentId(),
            after,
            document_.revision());
    if (!candidate.ok()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Datum Plane candidate violates Part authored-state or dependency invariants",
            path_);
    }

    const auto part_evaluation =
        part::evaluatePart(
            *candidate.document,
            modeling_kernel);
    const auto datum_evaluation =
        part::evaluateDatums(
            *candidate.document,
            part_evaluation);
    const auto* evaluated =
        datum_evaluation.find(
            command.datum_id);
    if (evaluated == nullptr ||
        evaluated->status !=
            part::DatumPlaneEvaluationStatus::
                resolved ||
        !evaluated->frame) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Datum Plane source is not currently Resolved; no authored mutation committed",
            path_);
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while editing Datum Plane");
}

DocumentSessionResult DocumentSession::execute(
    const SetDatumPlaneVisibilityCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Datum visibility change was started from a stale DocumentRevision",
            path_);
    }

    std::set<part::DatumId> unique_targets;
    for (const auto id : command.targets) {
        if (!id.valid() ||
            document_.findDatumPlane(id) ==
                nullptr) {
            return failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Datum visibility command contains a missing or invalid DatumId",
                path_);
        }
        unique_targets.insert(id);
    }
    if (unique_targets.empty()) {
        return success(false);
    }

    auto after = document_.state();
    for (const auto id : unique_targets) {
        auto* target =
            findDatumPlane(after, id);
        if (target == nullptr) {
            return failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Datum visibility target disappeared before commit",
                path_);
        }
        target->visible = command.visible;
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while changing Datum Plane visibility");
}

DocumentSessionResult DocumentSession::execute(
    const DeleteDatumPlaneCommand& command) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Delete Datum Plane was started from a stale DocumentRevision",
            path_);
    }
    if (!command.datum_id.valid()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Delete Datum Plane contains an invalid DatumId",
            path_);
    }

    const auto& datums =
        document_.datumPlanes();
    const auto found =
        std::find_if(
            datums.begin(),
            datums.end(),
            [&command](
                const part::OffsetDatumPlane& datum) {
                return datum.id ==
                       command.datum_id;
            });
    if (found == datums.end()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Delete Datum Plane target DatumId does not exist",
            path_);
    }

    for (const auto& datum : datums) {
        if (datum.id == command.datum_id) {
            continue;
        }
        const auto source =
            part::datumPlaneIdForPlaneReference(
                datum.source);
        if (source &&
            *source == command.datum_id) {
            return failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Delete Datum Plane rejected because another Datum Plane depends on it",
                path_);
        }
    }

    for (const auto& sketch :
         document_.sketches()) {
        const auto support =
            part::datumPlaneIdForSketchSupport(
                sketch.support);
        if (support &&
            *support == command.datum_id) {
            return failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Delete Datum Plane rejected because a Sketch depends on it",
                path_);
        }
    }

    auto after = document_.state();
    const auto erase =
        std::find_if(
            after.datum_planes.begin(),
            after.datum_planes.end(),
            [&command](
                const part::OffsetDatumPlane& datum) {
                return datum.id ==
                       command.datum_id;
            });
    if (erase ==
        after.datum_planes.end()) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Delete Datum Plane target disappeared before commit",
            path_);
    }
    after.datum_planes.erase(erase);

    return commitCommandState(
        std::move(after),
        "Part transaction failed while deleting Datum Plane");
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

CreateRevolveFeatureResult DocumentSession::execute(
    const CreateRevolveFeatureCommand& command,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    revision_diverged,
                "Create Revolve was started from a stale DocumentRevision",
                path_);
        return {
            false,
            std::nullopt,
            std::nullopt,
            failed.diagnostic};
    }

    const part::RevolveFeature definition{
        command.profile_id,
        command.axis,
        command.operation,
        command.extent};
    if (!part::revolveFeatureStructurallyValid(
            definition) ||
        document_.findProfile(
            command.profile_id) == nullptr) {
        const auto failed =
            failure(
                DocumentSessionErrorCode::
                    invalid_command,
                "Create Revolve contains invalid inputs or missing ProfileId",
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
                ? defaultRevolveFeatureName(*id)
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
                "Create Revolve candidate violates Part authored-state invariants",
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
                "Create Revolve did not evaluate UpToDate; no authored mutation committed",
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
            "Part transaction failed while creating Revolve Feature");
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
    const EditRevolveFeatureCommand& command,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (document_.revision() !=
        command.expected_revision) {
        return failure(
            DocumentSessionErrorCode::
                revision_diverged,
            "Edit Revolve was started from a stale DocumentRevision",
            path_);
    }

    const part::RevolveFeature definition{
        command.profile_id,
        command.axis,
        command.operation,
        command.extent};
    if (!part::revolveFeatureStructurallyValid(
            definition) ||
        document_.findProfile(
            command.profile_id) == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Revolve contains invalid inputs or missing ProfileId",
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
            "Edit Revolve target FeatureId does not exist",
            path_);
    }
    if (feature->suppressed) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Revolve target is Suppressed; unsuppress before editing",
            path_);
    }
    if (std::get_if<part::RevolveFeature>(
            &feature->definition) == nullptr) {
        return failure(
            DocumentSessionErrorCode::
                invalid_command,
            "Edit Revolve target FeatureId is not a Revolve Feature",
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
            "Edit Revolve candidate violates Part authored-state invariants",
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
            "Edit Revolve target did not evaluate UpToDate; no authored mutation committed",
            path_);
    }

    return commitCommandState(
        std::move(after),
        "Part transaction failed while editing Revolve Feature");
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
    applyAxisIdCursor(
        adjusted_expected);
    applyDatumIdCursor(
        adjusted_expected);
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
    applyAxisIdCursor(adjusted_target);
    applyDatumIdCursor(adjusted_target);
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
    axis_id_cursor_.preserve(
        document_.state().next_axis_id);
    datum_id_cursor_.preserve(
        document_.state().next_datum_id);
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

void DocumentSession::applyAxisIdCursor(
    part::PartAuthoredState& state) const noexcept {
    state.next_axis_id.preserve(
        axis_id_cursor_);
}

void DocumentSession::applyDatumIdCursor(
    part::PartAuthoredState& state) const noexcept {
    state.next_datum_id.preserve(
        datum_id_cursor_);
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
