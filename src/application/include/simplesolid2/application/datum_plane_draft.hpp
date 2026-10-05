#pragma once

#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/core/document.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/kernel/profile_input.hpp>
#include <simplesolid2/part/datum.hpp>
#include <simplesolid2/part/datum_evaluation.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::kernel {
class ISolidModelingKernel;
}

namespace simplesolid2::application {

class DocumentSession;

using DatumPlaneDraftGeneration = std::uint64_t;

enum class DatumPlaneDraftMode : std::uint8_t {
    create,
    edit,
};

class DatumPlaneDraft final {
public:
    [[nodiscard]] static DatumPlaneDraft
    beginCreate(const DocumentSession& session);

    [[nodiscard]] static std::optional<DatumPlaneDraft>
    beginCreate(
        const DocumentSession& session,
        part::PlaneReference source);

    [[nodiscard]] static std::optional<DatumPlaneDraft>
    beginEdit(
        const DocumentSession& session,
        part::DatumId datum_id);

    [[nodiscard]] const core::DocumentId&
    documentId() const noexcept {
        return document_id_;
    }

    [[nodiscard]] core::DocumentRevision
    sourceRevision() const noexcept {
        return source_revision_;
    }

    [[nodiscard]] DatumPlaneDraftGeneration
    generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] DatumPlaneDraftMode
    mode() const noexcept {
        return mode_;
    }

    [[nodiscard]] std::optional<part::DatumId>
    datumId() const noexcept {
        return datum_id_;
    }

    [[nodiscard]] const std::optional<part::PlaneReference>&
    source() const noexcept {
        return source_;
    }

    [[nodiscard]] core::LengthValue
    offset() const noexcept {
        return offset_;
    }

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] bool setSource(
        part::PlaneReference source);

    [[nodiscard]] bool setOffset(
        core::LengthValue offset) noexcept;

    [[nodiscard]] bool reverse() noexcept;

private:
    DatumPlaneDraft(
        core::DocumentId document_id,
        core::DocumentRevision source_revision,
        DatumPlaneDraftMode mode,
        std::optional<part::DatumId> datum_id,
        std::optional<part::PlaneReference> source,
        core::LengthValue offset);

    [[nodiscard]] bool canAdvance() const noexcept;
    void advance() noexcept;

    core::DocumentId document_id_;
    core::DocumentRevision source_revision_;
    DatumPlaneDraftGeneration generation_{1U};
    DatumPlaneDraftMode mode_{
        DatumPlaneDraftMode::create};
    std::optional<part::DatumId> datum_id_;
    std::optional<part::PlaneReference> source_;
    // Authored semantic truth is one signed offset. Reverse only negates this
    // value; there is deliberately no second persisted direction flag.
    core::LengthValue offset_{10.0};
};

enum class DatumPlaneDraftEvaluationStatus : std::uint8_t {
    ok,
    stale_document,
    stale_revision,
    invalid_draft,
    missing_datum,
    id_exhausted,
    invalid_candidate,
    source_unresolved,
};

struct DatumPlaneDraftEvaluationResult final {
    DatumPlaneDraftEvaluationStatus status{
        DatumPlaneDraftEvaluationStatus::
            invalid_draft};
    std::optional<core::DocumentId> document_id;
    core::DocumentRevision source_revision;
    DatumPlaneDraftGeneration draft_generation{};
    DatumPlaneDraftMode mode{
        DatumPlaneDraftMode::create};
    std::optional<part::DatumId> authored_datum_id;
    std::optional<part::DatumId> candidate_datum_id;
    std::optional<part::PlaneReference> source;
    core::LengthValue offset{10.0};
    std::optional<kernel::Frame3> frame;
    std::optional<part::BodyStageRef>
        required_body_stage;
    std::optional<part::DatumPlaneEvaluationStatus>
        datum_status;
    std::optional<
        part::DatumPlaneEvaluationDiagnostic>
        datum_diagnostic;

    [[nodiscard]] bool committable() const noexcept {
        return status ==
                   DatumPlaneDraftEvaluationStatus::ok &&
               frame.has_value() &&
               frame->valid() &&
               candidate_datum_id.has_value();
    }
};

enum class DatumPlaneDraftFinishStatus : std::uint8_t {
    committed,
    stale_context,
    stale_evaluation,
    invalid_draft,
    rejected,
};

struct DatumPlaneDraftFinishResult final {
    DatumPlaneDraftFinishStatus status{
        DatumPlaneDraftFinishStatus::rejected};
    bool changed{};
    std::optional<part::DatumId> datum_id;
    std::string diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
               DatumPlaneDraftFinishStatus::committed;
    }
};

[[nodiscard]] DatumPlaneDraftFinishResult
finishDatumPlaneDraft(
    DocumentSession& session,
    const DatumPlaneDraft& draft,
    const DatumPlaneDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel);

enum class DatumPlaneCadInputAction : std::uint8_t {
    none,
    finish,
    cancel,
};

struct DatumPlaneCadInputResult final {
    bool accepted{};
    DatumPlaneCadInputAction action{
        DatumPlaneCadInputAction::none};
    std::string diagnostic;
};

// Command Line grammar for an already-active Datum Plane draft. Semantic
// source acquisition is shared with GUI/viewport/Tree selection and enters
// the same draft through setSource(); text input owns Offset/Reverse and
// Finish/Cancel only.
[[nodiscard]] DatumPlaneCadInputResult
submitDatumPlaneCadInput(
    DatumPlaneDraft& draft,
    std::string_view text,
    const CadInputNumberFormat& number_format);

} // namespace simplesolid2::application
