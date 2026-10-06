#include <simplesolid2/application/revolve_draft.hpp>

#include <simplesolid2/application/document_session.hpp>

#include <limits>
#include <numbers>
#include <utility>
#include <variant>

namespace simplesolid2::application {
namespace {

[[nodiscard]] bool validOperation(
    part::RevolveOperation operation) noexcept {
    switch (operation) {
    case part::RevolveOperation::add:
    case part::RevolveOperation::cut:
        return true;
    }
    return false;
}

[[nodiscard]] bool validExtentMode(
    RevolveDraftExtentMode mode) noexcept {
    switch (mode) {
    case RevolveDraftExtentMode::one_side:
    case RevolveDraftExtentMode::midplane:
        return true;
    }
    return false;
}

[[nodiscard]] part::RevolveExtent defaultExtent() {
    return part::OneSidedRevolveExtent{
        core::AngleValue{
            2.0 * std::numbers::pi_v<double>},
        false};
}

} // namespace

RevolveDraft::RevolveDraft(
    core::DocumentId document_id,
    core::DocumentRevision source_revision,
    RevolveDraftMode mode,
    std::optional<part::ProfileId> profile_id,
    std::optional<part::AxisReference> axis,
    std::optional<part::FeatureId> feature_id,
    part::RevolveOperation operation,
    part::RevolveExtent extent,
    std::string name)
    : document_id_{std::move(document_id)},
      source_revision_{source_revision},
      mode_{mode},
      profile_id_{std::move(profile_id)},
      axis_{std::move(axis)},
      feature_id_{feature_id},
      operation_{operation},
      extent_{std::move(extent)},
      name_{std::move(name)} {}

RevolveDraft
RevolveDraft::beginCreate(
    const DocumentSession& session) {
    return RevolveDraft{
        session.documentId(),
        session.document().revision(),
        RevolveDraftMode::create,
        std::nullopt,
        std::nullopt,
        std::nullopt,
        part::RevolveOperation::add,
        defaultExtent(),
        {}};
}

std::optional<RevolveDraft>
RevolveDraft::beginCreate(
    const DocumentSession& session,
    part::ProfileId profile_id) {
    if (!profile_id.valid() ||
        session.document().findProfile(
            profile_id) == nullptr) {
        return std::nullopt;
    }

    auto draft = beginCreate(session);
    draft.profile_id_ = profile_id;
    return draft;
}

std::optional<RevolveDraft>
RevolveDraft::beginEdit(
    const DocumentSession& session,
    part::FeatureId feature_id) {
    const auto* feature =
        session.document().findFeature(
            feature_id);
    if (feature == nullptr ||
        feature->suppressed) {
        return std::nullopt;
    }

    const auto* revolve =
        std::get_if<part::RevolveFeature>(
            &feature->definition);
    if (revolve == nullptr) {
        return std::nullopt;
    }

    return RevolveDraft{
        session.documentId(),
        session.document().revision(),
        RevolveDraftMode::edit,
        revolve->profile_id,
        revolve->axis,
        feature_id,
        revolve->operation,
        revolve->extent,
        feature->name};
}

RevolveDraftExtentMode
RevolveDraft::extentMode() const noexcept {
    return std::holds_alternative<
               part::MidplaneRevolveExtent>(
               extent_)
        ? RevolveDraftExtentMode::midplane
        : RevolveDraftExtentMode::one_side;
}

core::AngleValue
RevolveDraft::angle() const noexcept {
    if (const auto* one =
            std::get_if<
                part::OneSidedRevolveExtent>(
                &extent_)) {
        return one->angle;
    }
    return std::get<
               part::MidplaneRevolveExtent>(
               extent_)
        .total_angle;
}

bool RevolveDraft::reversed() const noexcept {
    if (const auto* one =
            std::get_if<
                part::OneSidedRevolveExtent>(
                &extent_)) {
        return one->reversed;
    }
    return false;
}

bool RevolveDraft::valid() const noexcept {
    if (generation_ == 0U ||
        !profile_id_ ||
        !profile_id_->valid() ||
        !axis_ ||
        !axis_->valid() ||
        !validOperation(operation_)) {
        return false;
    }

    if (mode_ == RevolveDraftMode::edit) {
        if (!feature_id_ ||
            !feature_id_->valid()) {
            return false;
        }
    } else if (feature_id_) {
        return false;
    }

    return part::revolveFeatureStructurallyValid(
        part::RevolveFeature{
            *profile_id_,
            *axis_,
            operation_,
            extent_});
}

bool RevolveDraft::canAdvance() const noexcept {
    return generation_ !=
           std::numeric_limits<
               RevolveDraftGeneration>::max();
}

void RevolveDraft::advance() noexcept {
    ++generation_;
}

bool RevolveDraft::setProfile(
    part::ProfileId profile_id) noexcept {
    if (!profile_id.valid()) {
        return false;
    }
    if (profile_id_ &&
        *profile_id_ == profile_id) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    profile_id_ = profile_id;
    advance();
    return true;
}

bool RevolveDraft::setAxis(
    part::AxisReference axis) {
    if (!axis.valid()) {
        return false;
    }
    if (axis_ &&
        *axis_ == axis) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    axis_ = std::move(axis);
    advance();
    return true;
}

bool RevolveDraft::setOperation(
    part::RevolveOperation operation) noexcept {
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

bool RevolveDraft::setExtentMode(
    RevolveDraftExtentMode mode) noexcept {
    if (!validExtentMode(mode)) {
        return false;
    }
    if (extentMode() == mode) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }

    const auto current_angle = angle();
    if (mode ==
        RevolveDraftExtentMode::midplane) {
        extent_ =
            part::MidplaneRevolveExtent{
                current_angle};
    } else {
        extent_ =
            part::OneSidedRevolveExtent{
                current_angle,
                false};
    }
    advance();
    return true;
}

bool RevolveDraft::setAngle(
    core::AngleValue angle) noexcept {
    if (!angle.finite() ||
        !(angle.radians > 0.0) ||
        angle.radians >
            2.0 * std::numbers::pi_v<double>) {
        return false;
    }
    if (this->angle() == angle) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }

    if (auto* one =
            std::get_if<
                part::OneSidedRevolveExtent>(
                &extent_)) {
        one->angle = angle;
    } else {
        std::get<
            part::MidplaneRevolveExtent>(
            extent_)
            .total_angle = angle;
    }
    advance();
    return true;
}

bool RevolveDraft::setReversed(
    bool reversed) noexcept {
    auto* one =
        std::get_if<
            part::OneSidedRevolveExtent>(
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

bool RevolveDraft::setName(
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

RevolveDraftFinishResult
finishRevolveDraft(
    DocumentSession& session,
    const RevolveDraft& draft,
    const RevolveDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (session.documentId() !=
            draft.documentId() ||
        session.document().revision() !=
            draft.sourceRevision()) {
        return {
            RevolveDraftFinishStatus::
                stale_context,
            false,
            std::nullopt,
            "Revolve draft context is stale."};
    }

    if (!draft.valid() ||
        !draft.profileId() ||
        !draft.axis()) {
        return {
            RevolveDraftFinishStatus::
                invalid_draft,
            false,
            std::nullopt,
            "Revolve draft is incomplete or invalid."};
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
        evaluation.axis ==
            draft.axis() &&
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
            RevolveDraftFinishStatus::
                stale_evaluation,
            false,
            std::nullopt,
            "Revolve Finish requires the current successful draft evaluation."};
    }

    if (draft.mode() ==
        RevolveDraftMode::create) {
        const auto result =
            session.execute(
                CreateRevolveFeatureCommand{
                    *draft.profileId(),
                    *draft.axis(),
                    draft.sourceRevision(),
                    draft.operation(),
                    draft.extent(),
                    draft.name()},
                modeling_kernel);
        if (!result.ok()) {
            return {
                RevolveDraftFinishStatus::rejected,
                false,
                std::nullopt,
                result.diagnostic.message};
        }
        return {
            RevolveDraftFinishStatus::committed,
            result.changed,
            result.feature_id,
            {}};
    }

    const auto feature_id =
        draft.featureId();
    if (!feature_id) {
        return {
            RevolveDraftFinishStatus::
                invalid_draft,
            false,
            std::nullopt,
            "Revolve edit draft has no FeatureId."};
    }

    const auto result =
        session.execute(
            EditRevolveFeatureCommand{
                *feature_id,
                draft.sourceRevision(),
                *draft.profileId(),
                *draft.axis(),
                draft.operation(),
                draft.extent(),
                draft.name()},
            modeling_kernel);
    if (!result.ok()) {
        return {
            RevolveDraftFinishStatus::rejected,
            false,
            std::nullopt,
            result.diagnostic.message};
    }
    return {
        RevolveDraftFinishStatus::committed,
        result.changed,
        feature_id,
        {}};
}

} // namespace simplesolid2::application
