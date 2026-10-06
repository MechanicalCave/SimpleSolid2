#pragma once

#include <simplesolid2/application/cad_input_semantics.hpp>
#include <simplesolid2/core/document.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/axis_evaluation.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace simplesolid2::kernel {
class ISolidModelingKernel;
}

namespace simplesolid2::application {

class DocumentSession;

using AxisDraftGeneration = std::uint64_t;

enum class AxisDraftMode : std::uint8_t {
    create,
    edit,
};

class AxisDraft final {
public:
    [[nodiscard]] static AxisDraft
    beginCreate(const DocumentSession& session);

    [[nodiscard]] static std::optional<AxisDraft>
    beginCreate(
        const DocumentSession& session,
        part::SketchLineAxisSource source);

    [[nodiscard]] static std::optional<AxisDraft>
    beginEdit(
        const DocumentSession& session,
        part::AxisId axis_id);

    [[nodiscard]] const core::DocumentId&
    documentId() const noexcept {
        return document_id_;
    }

    [[nodiscard]] core::DocumentRevision
    sourceRevision() const noexcept {
        return source_revision_;
    }

    [[nodiscard]] AxisDraftGeneration
    generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] AxisDraftMode
    mode() const noexcept {
        return mode_;
    }

    [[nodiscard]] std::optional<part::AxisId>
    axisId() const noexcept {
        return axis_id_;
    }

    [[nodiscard]] const std::optional<
        part::SketchLineAxisSource>&
    source() const noexcept {
        return source_;
    }

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] bool setSource(
        part::SketchLineAxisSource source);

private:
    AxisDraft(
        core::DocumentId document_id,
        core::DocumentRevision source_revision,
        AxisDraftMode mode,
        std::optional<part::AxisId> axis_id,
        std::optional<part::SketchLineAxisSource> source);

    [[nodiscard]] bool canAdvance() const noexcept;
    void advance() noexcept;

    core::DocumentId document_id_;
    core::DocumentRevision source_revision_;
    AxisDraftGeneration generation_{1U};
    AxisDraftMode mode_{AxisDraftMode::create};
    std::optional<part::AxisId> axis_id_;
    std::optional<part::SketchLineAxisSource> source_;
};

enum class AxisDraftEvaluationStatus : std::uint8_t {
    ok,
    stale_document,
    stale_revision,
    invalid_draft,
    missing_axis,
    id_exhausted,
    invalid_candidate,
    source_unresolved,
};

struct AxisDraftEvaluationResult final {
    AxisDraftEvaluationStatus status{
        AxisDraftEvaluationStatus::invalid_draft};
    std::optional<core::DocumentId> document_id;
    core::DocumentRevision source_revision;
    AxisDraftGeneration draft_generation{};
    AxisDraftMode mode{AxisDraftMode::create};
    std::optional<part::AxisId> authored_axis_id;
    std::optional<part::AxisId> candidate_axis_id;
    std::optional<part::SketchLineAxisSource> source;
    std::optional<part::ResolvedAxisLine> line;
    std::optional<part::BodyStageRef>
        required_body_stage;
    std::optional<part::AxisEvaluationStatus>
        axis_status;
    std::optional<part::AxisEvaluationDiagnostic>
        axis_diagnostic;

    [[nodiscard]] bool committable() const noexcept {
        return status == AxisDraftEvaluationStatus::ok &&
               candidate_axis_id.has_value() &&
               line.has_value() &&
               line->valid() &&
               axis_status ==
                   part::AxisEvaluationStatus::resolved &&
               axis_diagnostic ==
                   part::AxisEvaluationDiagnostic::none;
    }
};

enum class AxisDraftFinishStatus : std::uint8_t {
    committed,
    stale_context,
    stale_evaluation,
    invalid_draft,
    rejected,
};

struct AxisDraftFinishResult final {
    AxisDraftFinishStatus status{
        AxisDraftFinishStatus::rejected};
    bool changed{};
    std::optional<part::AxisId> axis_id;
    std::string diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
               AxisDraftFinishStatus::committed;
    }
};

[[nodiscard]] AxisDraftFinishResult finishAxisDraft(
    DocumentSession& session,
    const AxisDraft& draft,
    const AxisDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel);

enum class AxisCadInputAction : std::uint8_t {
    none,
    acquire_source,
    finish,
    cancel,
};

struct AxisCadInputResult final {
    bool accepted{};
    AxisCadInputAction action{
        AxisCadInputAction::none};
    std::string diagnostic;
};

// Command Line grammar for an active Axis draft. Source acquisition remains
// semantic selection and enters the same draft through setSource(); text input
// only switches to source-acquisition mode or requests Finish/Cancel.
[[nodiscard]] AxisCadInputResult submitAxisCadInput(
    AxisDraft& draft,
    std::string_view text,
    const CadInputNumberFormat& number_format);

} // namespace simplesolid2::application
