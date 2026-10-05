#include <simplesolid2/application/datum_plane_draft.hpp>

#include <simplesolid2/application/document_session.hpp>

#include <algorithm>
#include <cctype>
#include <limits>
#include <string>
#include <utility>

namespace simplesolid2::application {
namespace {

std::string trimAscii(std::string_view value) {
    std::size_t first = 0U;
    while (first < value.size() &&
           std::isspace(
               static_cast<unsigned char>(
                   value[first])) != 0) {
        ++first;
    }

    std::size_t last = value.size();
    while (last > first &&
           std::isspace(
               static_cast<unsigned char>(
                   value[last - 1U])) != 0) {
        --last;
    }
    return std::string{
        value.substr(first, last - first)};
}

std::string upperAscii(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const auto character : value) {
        result.push_back(
            static_cast<char>(
                std::toupper(
                    static_cast<unsigned char>(
                        character))));
    }
    return result;
}

} // namespace

DatumPlaneDraft::DatumPlaneDraft(
    core::DocumentId document_id,
    core::DocumentRevision source_revision,
    DatumPlaneDraftMode mode,
    std::optional<part::DatumId> datum_id,
    std::optional<part::PlaneReference> source,
    core::LengthValue offset)
    : document_id_{std::move(document_id)},
      source_revision_{source_revision},
      mode_{mode},
      datum_id_{datum_id},
      source_{std::move(source)},
      offset_{offset} {}

DatumPlaneDraft
DatumPlaneDraft::beginCreate(
    const DocumentSession& session) {
    return DatumPlaneDraft{
        session.documentId(),
        session.document().revision(),
        DatumPlaneDraftMode::create,
        std::nullopt,
        std::nullopt,
        core::LengthValue{10.0}};
}

std::optional<DatumPlaneDraft>
DatumPlaneDraft::beginCreate(
    const DocumentSession& session,
    part::PlaneReference source) {
    if (!source.valid()) {
        return std::nullopt;
    }

    return DatumPlaneDraft{
        session.documentId(),
        session.document().revision(),
        DatumPlaneDraftMode::create,
        std::nullopt,
        std::move(source),
        core::LengthValue{10.0}};
}

std::optional<DatumPlaneDraft>
DatumPlaneDraft::beginEdit(
    const DocumentSession& session,
    part::DatumId datum_id) {
    const auto* datum =
        session.document().findDatumPlane(
            datum_id);
    if (datum == nullptr) {
        return std::nullopt;
    }

    return DatumPlaneDraft{
        session.documentId(),
        session.document().revision(),
        DatumPlaneDraftMode::edit,
        datum_id,
        datum->source,
        datum->offset};
}

bool DatumPlaneDraft::valid() const noexcept {
    if (generation_ == 0U ||
        !source_ ||
        !source_->valid() ||
        !offset_.finite()) {
        return false;
    }

    if (mode_ ==
        DatumPlaneDraftMode::create) {
        return !datum_id_.has_value();
    }
    return datum_id_.has_value() &&
           datum_id_->valid();
}

bool DatumPlaneDraft::canAdvance() const noexcept {
    return generation_ !=
           std::numeric_limits<
               DatumPlaneDraftGeneration>::max();
}

void DatumPlaneDraft::advance() noexcept {
    ++generation_;
}

bool DatumPlaneDraft::setSource(
    part::PlaneReference source) {
    if (!source.valid()) {
        return false;
    }
    if (source_ &&
        *source_ == source) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    source_ = std::move(source);
    advance();
    return true;
}

bool DatumPlaneDraft::setOffset(
    core::LengthValue offset) noexcept {
    if (!offset.finite()) {
        return false;
    }
    if (offset_ == offset) {
        return true;
    }
    if (!canAdvance()) {
        return false;
    }
    offset_ = offset;
    advance();
    return true;
}

bool DatumPlaneDraft::reverse() noexcept {
    if (!offset_.finite()) {
        return false;
    }
    const core::LengthValue reversed{
        -offset_.millimetres};
    if (reversed == offset_) {
        return true;
    }
    return setOffset(reversed);
}

DatumPlaneDraftFinishResult
finishDatumPlaneDraft(
    DocumentSession& session,
    const DatumPlaneDraft& draft,
    const DatumPlaneDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel) {
    if (session.documentId() !=
            draft.documentId() ||
        session.document().revision() !=
            draft.sourceRevision()) {
        return {
            DatumPlaneDraftFinishStatus::
                stale_context,
            false,
            std::nullopt,
            "Datum Plane draft context is stale."};
    }

    if (!draft.valid() ||
        !draft.source()) {
        return {
            DatumPlaneDraftFinishStatus::
                invalid_draft,
            false,
            std::nullopt,
            "Datum Plane draft is incomplete or invalid."};
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
        evaluation.authored_datum_id ==
            draft.datumId() &&
        evaluation.source ==
            draft.source() &&
        evaluation.offset ==
            draft.offset();

    if (!exact_evaluation) {
        return {
            DatumPlaneDraftFinishStatus::
                stale_evaluation,
            false,
            std::nullopt,
            "Datum Plane Finish requires the current successful draft evaluation."};
    }

    if (draft.mode() ==
        DatumPlaneDraftMode::create) {
        const auto result =
            session.execute(
                CreateDatumPlaneCommand{
                    *draft.source(),
                    draft.sourceRevision(),
                    draft.offset(),
                    true},
                modeling_kernel);
        if (!result.ok() ||
            !result.datum_id) {
            return {
                DatumPlaneDraftFinishStatus::
                    rejected,
                false,
                std::nullopt,
                result.diagnostic.message};
        }
        if (evaluation.candidate_datum_id &&
            *evaluation.candidate_datum_id !=
                *result.datum_id) {
            return {
                DatumPlaneDraftFinishStatus::
                    rejected,
                false,
                std::nullopt,
                "DatumId changed between preview evaluation and Finish."};
        }
        return {
            DatumPlaneDraftFinishStatus::
                committed,
            result.changed,
            result.datum_id,
            {}};
    }

    const auto datum_id =
        draft.datumId();
    if (!datum_id) {
        return {
            DatumPlaneDraftFinishStatus::
                invalid_draft,
            false,
            std::nullopt,
            "Datum Plane edit draft has no DatumId."};
    }

    const auto result =
        session.execute(
            EditDatumPlaneCommand{
                *datum_id,
                *draft.source(),
                draft.sourceRevision(),
                draft.offset()},
            modeling_kernel);
    if (!result.ok()) {
        return {
            DatumPlaneDraftFinishStatus::
                rejected,
            false,
            std::nullopt,
            result.diagnostic.message};
    }

    return {
        DatumPlaneDraftFinishStatus::committed,
        result.changed,
        datum_id,
        {}};
}

DatumPlaneCadInputResult
submitDatumPlaneCadInput(
    DatumPlaneDraft& draft,
    std::string_view text,
    const CadInputNumberFormat& number_format) {
    const auto submitted =
        trimAscii(text);

    // Empty Enter is Finish in the active Datum Plane command context,
    // matching the accepted Extrude-style working grammar.
    if (submitted.empty()) {
        return {
            true,
            DatumPlaneCadInputAction::finish,
            {}};
    }

    const auto upper =
        upperAscii(submitted);
    if (upper == "FINISH") {
        return {
            true,
            DatumPlaneCadInputAction::finish,
            {}};
    }
    if (upper == "CANCEL" ||
        upper == "ESC") {
        return {
            true,
            DatumPlaneCadInputAction::cancel,
            {}};
    }
    if (upper == "REVERSE") {
        if (!draft.reverse()) {
            return {
                false,
                DatumPlaneCadInputAction::none,
                "Datum Plane offset could not be reversed."};
        }
        return {
            true,
            DatumPlaneCadInputAction::none,
            {}};
    }

    std::string_view expression{submitted};
    constexpr std::string_view offset_keyword{
        "OFFSET"};
    if (upper.size() >
            offset_keyword.size() &&
        upper.starts_with(offset_keyword)) {
        const auto separator =
            submitted[offset_keyword.size()];
        if (std::isspace(
                static_cast<unsigned char>(
                    separator)) == 0) {
            return {
                false,
                DatumPlaneCadInputAction::none,
                "Datum Plane expects OFFSET <Length>."};
        }
        expression =
            std::string_view{submitted}.substr(
                offset_keyword.size() + 1U);
    }

    const auto quantity =
        parseCadQuantity(
            expression,
            {
                CadQuantityDimension::length,
                number_format.length_unit});
    if (!quantity ||
        !draft.setOffset(
            core::LengthValue{
                quantity->canonical_value})) {
        return {
            false,
            DatumPlaneCadInputAction::none,
            "Datum Plane Offset expects a finite signed Length expression."};
    }

    return {
        true,
        DatumPlaneCadInputAction::none,
        {}};
}

} // namespace simplesolid2::application
