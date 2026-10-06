#include <simplesolid2/application/axis_draft.hpp>

#include <simplesolid2/application/document_session.hpp>

#include <algorithm>
#include <cctype>
#include <limits>
#include <utility>

namespace simplesolid2::application {
namespace {

std::string trimAscii(std::string_view text) {
    auto first = text.begin();
    auto last = text.end();
    while (first != last &&
           std::isspace(
               static_cast<unsigned char>(*first)) != 0) {
        ++first;
    }
    while (last != first &&
           std::isspace(
               static_cast<unsigned char>(*(last - 1))) != 0) {
        --last;
    }
    return std::string{first, last};
}

std::string upperAscii(std::string text) {
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char ch) {
            return static_cast<char>(
                std::toupper(ch));
        });
    return text;
}

} // namespace

AxisDraft::AxisDraft(
    core::DocumentId document_id,
    core::DocumentRevision source_revision,
    AxisDraftMode mode,
    std::optional<part::AxisId> axis_id,
    std::optional<part::SketchLineAxisSource> source)
    : document_id_{std::move(document_id)},
      source_revision_{source_revision},
      mode_{mode},
      axis_id_{axis_id},
      source_{std::move(source)} {}

AxisDraft AxisDraft::beginCreate(
    const DocumentSession& session) {
    return AxisDraft{
        session.documentId(),
        session.document().revision(),
        AxisDraftMode::create,
        std::nullopt,
        std::nullopt};
}

std::optional<AxisDraft> AxisDraft::beginCreate(
    const DocumentSession& session,
    part::SketchLineAxisSource source) {
    if (!source.valid()) {
        return std::nullopt;
    }

    return AxisDraft{
        session.documentId(),
        session.document().revision(),
        AxisDraftMode::create,
        std::nullopt,
        std::move(source)};
}

std::optional<AxisDraft> AxisDraft::beginEdit(
    const DocumentSession& session,
    part::AxisId axis_id) {
    const auto* axis =
        session.document().findAxis(axis_id);
    if (axis == nullptr) {
        return std::nullopt;
    }

    return AxisDraft{
        session.documentId(),
        session.document().revision(),
        AxisDraftMode::edit,
        axis_id,
        axis->source};
}

bool AxisDraft::valid() const noexcept {
    if (generation_ == 0U ||
        !source_ ||
        !source_->valid()) {
        return false;
    }

    if (mode_ == AxisDraftMode::create) {
        return !axis_id_.has_value();
    }
    return axis_id_.has_value() &&
           axis_id_->valid();
}

bool AxisDraft::canAdvance() const noexcept {
    return generation_ !=
           std::numeric_limits<
               AxisDraftGeneration>::max();
}

void AxisDraft::advance() noexcept {
    ++generation_;
}

bool AxisDraft::setSource(
    part::SketchLineAxisSource source) {
    if (!source.valid()) {
        return false;
    }
    if (source_ && *source_ == source) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    source_ = std::move(source);
    advance();
    return true;
}

AxisDraftFinishResult finishAxisDraft(
    DocumentSession& session,
    const AxisDraft& draft,
    const AxisDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (session.documentId() !=
            draft.documentId() ||
        session.document().revision() !=
            draft.sourceRevision()) {
        return {
            AxisDraftFinishStatus::stale_context,
            false,
            std::nullopt,
            "Axis draft context is stale."};
    }

    if (!draft.valid() || !draft.source()) {
        return {
            AxisDraftFinishStatus::invalid_draft,
            false,
            std::nullopt,
            "Axis draft is incomplete or invalid."};
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
        evaluation.mode == draft.mode() &&
        evaluation.authored_axis_id ==
            draft.axisId() &&
        evaluation.source == draft.source();

    if (!exact_evaluation) {
        return {
            AxisDraftFinishStatus::stale_evaluation,
            false,
            std::nullopt,
            "Axis Finish requires the current successful draft evaluation."};
    }

    if (draft.mode() == AxisDraftMode::create) {
        const auto result =
            session.execute(
                CreateAxisCommand{
                    *draft.source(),
                    draft.sourceRevision(),
                    {},
                    true},
                modeling_kernel);
        if (!result.ok() || !result.axis_id) {
            return {
                AxisDraftFinishStatus::rejected,
                false,
                std::nullopt,
                result.diagnostic.message};
        }
        if (evaluation.candidate_axis_id &&
            *evaluation.candidate_axis_id !=
                *result.axis_id) {
            return {
                AxisDraftFinishStatus::rejected,
                false,
                std::nullopt,
                "AxisId changed between preview evaluation and Finish."};
        }
        return {
            AxisDraftFinishStatus::committed,
            result.changed,
            result.axis_id,
            {}};
    }

    const auto axis_id = draft.axisId();
    if (!axis_id) {
        return {
            AxisDraftFinishStatus::invalid_draft,
            false,
            std::nullopt,
            "Axis edit draft has no AxisId."};
    }

    const auto result =
        session.execute(
            EditAxisCommand{
                *axis_id,
                *draft.source(),
                draft.sourceRevision()},
            modeling_kernel);
    if (!result.ok()) {
        return {
            AxisDraftFinishStatus::rejected,
            false,
            std::nullopt,
            result.diagnostic.message};
    }

    return {
        AxisDraftFinishStatus::committed,
        result.changed,
        axis_id,
        {}};
}

AxisCadInputResult submitAxisCadInput(
    AxisDraft&,
    std::string_view text,
    const CadInputNumberFormat&) {
    const auto submitted =
        trimAscii(text);

    if (submitted.empty()) {
        return {
            true,
            AxisCadInputAction::finish,
            {}};
    }

    const auto upper =
        upperAscii(submitted);
    if (upper == "SOURCE") {
        return {
            true,
            AxisCadInputAction::acquire_source,
            {}};
    }
    if (upper == "FINISH") {
        return {
            true,
            AxisCadInputAction::finish,
            {}};
    }
    if (upper == "CANCEL" ||
        upper == "ESC") {
        return {
            true,
            AxisCadInputAction::cancel,
            {}};
    }

    return {
        false,
        AxisCadInputAction::none,
        "Axis input accepts SOURCE, FINISH, CANCEL or empty Enter."};
}

} // namespace simplesolid2::application
