#include <simplesolid2/application/extrude_draft.hpp>

#include <simplesolid2/application/document_session.hpp>

#include <limits>
#include <utility>
#include <variant>

namespace simplesolid2::application {
namespace {

[[nodiscard]] bool validOperation(
    part::ExtrudeOperation operation) noexcept {
    switch (operation) {
    case part::ExtrudeOperation::add:
    case part::ExtrudeOperation::cut:
        return true;
    }
    return false;
}

[[nodiscard]] bool validExtentMode(
    ExtrudeDraftExtentMode mode) noexcept {
    switch (mode) {
    case ExtrudeDraftExtentMode::one_side:
    case ExtrudeDraftExtentMode::midplane:
        return true;
    }
    return false;
}

} // namespace

ExtrudeDraft::ExtrudeDraft(
    core::DocumentId document_id,
    core::DocumentRevision source_revision,
    ExtrudeDraftMode mode,
    part::ProfileId profile_id,
    std::optional<part::FeatureId> feature_id,
    part::ExtrudeOperation operation,
    part::ExtrudeExtent extent,
    std::string name)
    : document_id_{std::move(document_id)},
      source_revision_{source_revision},
      mode_{mode},
      profile_id_{profile_id},
      feature_id_{feature_id},
      operation_{operation},
      extent_{std::move(extent)},
      name_{std::move(name)} {}

std::optional<ExtrudeDraft>
ExtrudeDraft::beginCreate(
    const DocumentSession& session,
    part::ProfileId profile_id) {
    if (!profile_id.valid() ||
        session.document().findProfile(
            profile_id) == nullptr) {
        return std::nullopt;
    }

    return ExtrudeDraft{
        session.documentId(),
        session.document().revision(),
        ExtrudeDraftMode::create,
        profile_id,
        std::nullopt,
        part::ExtrudeOperation::add,
        part::OneSidedExtrudeExtent{
            core::LengthValue{0.0},
            false},
        {}};
}

std::optional<ExtrudeDraft>
ExtrudeDraft::beginEdit(
    const DocumentSession& session,
    part::FeatureId feature_id) {
    const auto* feature =
        session.document().findFeature(
            feature_id);
    if (feature == nullptr ||
        feature->suppressed) {
        return std::nullopt;
    }

    const auto* extrude =
        std::get_if<part::ExtrudeFeature>(
            &feature->definition);
    if (extrude == nullptr) {
        return std::nullopt;
    }

    return ExtrudeDraft{
        session.documentId(),
        session.document().revision(),
        ExtrudeDraftMode::edit,
        extrude->profile_id,
        feature_id,
        extrude->operation,
        extrude->extent,
        feature->name};
}

ExtrudeDraftExtentMode
ExtrudeDraft::extentMode() const noexcept {
    return std::holds_alternative<
               part::MidplaneExtrudeExtent>(
               extent_)
        ? ExtrudeDraftExtentMode::midplane
        : ExtrudeDraftExtentMode::one_side;
}

core::LengthValue
ExtrudeDraft::distance() const noexcept {
    if (const auto* one =
            std::get_if<
                part::OneSidedExtrudeExtent>(
                &extent_)) {
        return one->distance;
    }
    return std::get<
               part::MidplaneExtrudeExtent>(
               extent_)
        .total_distance;
}

bool ExtrudeDraft::reversed() const noexcept {
    if (const auto* one =
            std::get_if<
                part::OneSidedExtrudeExtent>(
                &extent_)) {
        return one->reversed;
    }
    return false;
}

bool ExtrudeDraft::valid() const noexcept {
    if (generation_ == 0U ||
        !profile_id_.valid() ||
        !validOperation(operation_)) {
        return false;
    }

    if (mode_ == ExtrudeDraftMode::edit) {
        if (!feature_id_ ||
            !feature_id_->valid()) {
            return false;
        }
    } else if (feature_id_) {
        return false;
    }

    return part::extrudeFeatureStructurallyValid(
        part::ExtrudeFeature{
            profile_id_,
            operation_,
            extent_});
}

bool ExtrudeDraft::canAdvance() const noexcept {
    return generation_ !=
           std::numeric_limits<
               ExtrudeDraftGeneration>::max();
}

void ExtrudeDraft::advance() noexcept {
    ++generation_;
}

bool ExtrudeDraft::setOperation(
    part::ExtrudeOperation operation) noexcept {
    if (!validOperation(operation)) {
        return false;
    }
    if (operation_ == operation) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    operation_ = operation;
    advance();
    return true;
}

bool ExtrudeDraft::setExtentMode(
    ExtrudeDraftExtentMode mode) noexcept {
    if (!validExtentMode(mode)) {
        return false;
    }
    if (extentMode() == mode) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }

    const auto current_distance =
        distance();
    if (mode ==
        ExtrudeDraftExtentMode::midplane) {
        extent_ =
            part::MidplaneExtrudeExtent{
                current_distance};
    } else {
        extent_ =
            part::OneSidedExtrudeExtent{
                current_distance,
                false};
    }
    advance();
    return true;
}

bool ExtrudeDraft::setDistance(
    core::LengthValue distance) noexcept {
    if (!distance.finite() ||
        !(distance.millimetres > 0.0)) {
        return false;
    }
    if (this->distance() == distance) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }

    if (auto* one =
            std::get_if<
                part::OneSidedExtrudeExtent>(
                &extent_)) {
        one->distance = distance;
    } else {
        std::get<
            part::MidplaneExtrudeExtent>(
            extent_)
            .total_distance = distance;
    }
    advance();
    return true;
}

bool ExtrudeDraft::setReversed(
    bool reversed) noexcept {
    auto* one =
        std::get_if<
            part::OneSidedExtrudeExtent>(
            &extent_);
    if (one == nullptr) {
        return !reversed;
    }
    if (one->reversed == reversed) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    one->reversed = reversed;
    advance();
    return true;
}

bool ExtrudeDraft::setName(
    std::string name) {
    if (name_ == name) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    name_ = std::move(name);
    advance();
    return true;
}

ExtrudeDraftFinishResult
finishExtrudeDraft(
    DocumentSession& session,
    const ExtrudeDraft& draft,
    const ExtrudeDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (session.documentId() !=
            draft.documentId() ||
        session.document().revision() !=
            draft.sourceRevision()) {
        return {
            ExtrudeDraftFinishStatus::
                stale_context,
            false,
            std::nullopt,
            "Extrude draft context is stale."};
    }

    if (!draft.valid()) {
        return {
            ExtrudeDraftFinishStatus::
                invalid_draft,
            false,
            std::nullopt,
            "Extrude draft is incomplete or invalid."};
    }

    const bool exact_evaluation =
        evaluation.committable() &&
        evaluation.document_id &&
        *evaluation.document_id ==
            draft.documentId() &&
        evaluation.source_revision ==
            draft.sourceRevision() &&
        evaluation.draft_generation ==
            draft.generation() &&
        evaluation.mode ==
            draft.mode() &&
        evaluation.profile_id ==
            draft.profileId() &&
        evaluation.feature_id ==
            draft.featureId() &&
        evaluation.operation ==
            draft.operation() &&
        evaluation.extent ==
            draft.extent() &&
        evaluation.name ==
            draft.name();

    if (!exact_evaluation) {
        return {
            ExtrudeDraftFinishStatus::
                stale_evaluation,
            false,
            std::nullopt,
            "Extrude Finish requires the current successful draft evaluation."};
    }

    if (draft.mode() ==
        ExtrudeDraftMode::create) {
        const auto result =
            session.execute(
                CreateExtrudeFeatureCommand{
                    draft.profileId(),
                    draft.sourceRevision(),
                    draft.operation(),
                    draft.extent(),
                    draft.name()},
                modeling_kernel);
        if (!result.ok()) {
            return {
                ExtrudeDraftFinishStatus::rejected,
                false,
                std::nullopt,
                result.diagnostic.message};
        }
        return {
            ExtrudeDraftFinishStatus::committed,
            result.changed,
            result.feature_id,
            {}};
    }

    const auto feature_id =
        draft.featureId();
    if (!feature_id) {
        return {
            ExtrudeDraftFinishStatus::
                invalid_draft,
            false,
            std::nullopt,
            "Extrude edit draft has no FeatureId."};
    }

    const auto result =
        session.execute(
            EditExtrudeFeatureCommand{
                *feature_id,
                draft.sourceRevision(),
                draft.profileId(),
                draft.operation(),
                draft.extent(),
                draft.name()},
            modeling_kernel);
    if (!result.ok()) {
        return {
            ExtrudeDraftFinishStatus::rejected,
            false,
            std::nullopt,
            result.diagnostic.message};
    }
    return {
        ExtrudeDraftFinishStatus::committed,
        result.changed,
        feature_id,
        {}};
}

} // namespace simplesolid2::application
