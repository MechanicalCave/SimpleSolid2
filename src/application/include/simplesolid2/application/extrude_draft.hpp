#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace simplesolid2::application {

class DocumentSession;

using ExtrudeDraftGeneration = std::uint64_t;

enum class ExtrudeDraftMode : std::uint8_t {
    create,
    edit,
};

enum class ExtrudeDraftExtentMode : std::uint8_t {
    one_side,
    midplane,
};

class ExtrudeDraft final {
public:
    [[nodiscard]] static std::optional<ExtrudeDraft>
    beginCreate(
        const DocumentSession& session,
        part::ProfileId profile_id);

    [[nodiscard]] static std::optional<ExtrudeDraft>
    beginEdit(
        const DocumentSession& session,
        part::FeatureId feature_id);

    [[nodiscard]] const core::DocumentId&
    documentId() const noexcept {
        return document_id_;
    }

    [[nodiscard]] core::DocumentRevision
    sourceRevision() const noexcept {
        return source_revision_;
    }

    [[nodiscard]] ExtrudeDraftGeneration
    generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] ExtrudeDraftMode
    mode() const noexcept {
        return mode_;
    }

    [[nodiscard]] part::ProfileId
    profileId() const noexcept {
        return profile_id_;
    }

    [[nodiscard]] std::optional<part::FeatureId>
    featureId() const noexcept {
        return feature_id_;
    }

    [[nodiscard]] part::ExtrudeOperation
    operation() const noexcept {
        return operation_;
    }

    [[nodiscard]] const part::ExtrudeExtent&
    extent() const noexcept {
        return extent_;
    }

    [[nodiscard]] const std::string&
    name() const noexcept {
        return name_;
    }

    [[nodiscard]] ExtrudeDraftExtentMode
    extentMode() const noexcept;

    [[nodiscard]] core::LengthValue
    distance() const noexcept;

    [[nodiscard]] bool reversed() const noexcept;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] bool setOperation(
        part::ExtrudeOperation operation) noexcept;

    [[nodiscard]] bool setExtentMode(
        ExtrudeDraftExtentMode mode) noexcept;

    [[nodiscard]] bool setDistance(
        core::LengthValue distance) noexcept;

    [[nodiscard]] bool setReversed(
        bool reversed) noexcept;

    [[nodiscard]] bool setName(
        std::string name);

private:
    ExtrudeDraft(
        core::DocumentId document_id,
        core::DocumentRevision source_revision,
        ExtrudeDraftMode mode,
        part::ProfileId profile_id,
        std::optional<part::FeatureId> feature_id,
        part::ExtrudeOperation operation,
        part::ExtrudeExtent extent,
        std::string name);

    [[nodiscard]] bool canAdvance() const noexcept;
    void advance() noexcept;

    core::DocumentId document_id_;
    core::DocumentRevision source_revision_;
    ExtrudeDraftGeneration generation_{1U};
    ExtrudeDraftMode mode_{ExtrudeDraftMode::create};
    part::ProfileId profile_id_;
    std::optional<part::FeatureId> feature_id_;
    part::ExtrudeOperation operation_{
        part::ExtrudeOperation::add};
    part::ExtrudeExtent extent_{
        part::OneSidedExtrudeExtent{}};
    std::string name_;
};

enum class ExtrudeDraftEvaluationStatus : std::uint8_t {
    ok,
    stale_document,
    stale_revision,
    invalid_draft,
    missing_profile,
    missing_feature,
    suppressed_feature,
    feature_id_exhausted,
    invalid_candidate,
    target_failed,
};

struct ExtrudeDraftEvaluationResult final {
    ExtrudeDraftEvaluationStatus status{
        ExtrudeDraftEvaluationStatus::invalid_draft};
    std::optional<core::DocumentId> document_id;
    core::DocumentRevision source_revision;
    ExtrudeDraftGeneration draft_generation{};
    ExtrudeDraftMode mode{
        ExtrudeDraftMode::create};
    part::ProfileId profile_id;
    std::optional<part::FeatureId> feature_id;
    part::ExtrudeOperation operation{
        part::ExtrudeOperation::add};
    part::ExtrudeExtent extent{
        part::OneSidedExtrudeExtent{}};
    std::string name;
    part::BodyEvaluationStatus body_status{
        part::BodyEvaluationStatus::unavailable};
    // Full candidate Body remains the authority for Finish validity.
    kernel::RuntimeSolidHandle body_solid;
    // Runtime-only exact operation delta used solely for presentation.
    // Add = tool - upstream Body; Cut = tool ∩ upstream Body.
    // It is never authored and never substitutes for candidate validation.
    std::optional<kernel::SolidPresentationMesh>
        preview_delta_mesh;
    std::optional<
        part::FeatureEvaluationDiagnosticCode>
        evaluation_diagnostic;

    [[nodiscard]] bool committable() const noexcept {
        return status ==
               ExtrudeDraftEvaluationStatus::ok;
    }

    [[nodiscard]] bool previewSolidAvailable()
        const noexcept {
        return committable() &&
               body_status ==
                   part::BodyEvaluationStatus::
                       up_to_date &&
               preview_delta_mesh.has_value() &&
               preview_delta_mesh->valid();
    }
};

enum class ExtrudeDraftFinishStatus : std::uint8_t {
    committed,
    stale_context,
    stale_evaluation,
    invalid_draft,
    rejected,
};

struct ExtrudeDraftFinishResult final {
    ExtrudeDraftFinishStatus status{
        ExtrudeDraftFinishStatus::rejected};
    bool changed{};
    std::optional<part::FeatureId> feature_id;
    std::string diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
               ExtrudeDraftFinishStatus::committed;
    }
};

[[nodiscard]] ExtrudeDraftFinishResult
finishExtrudeDraft(
    DocumentSession& session,
    const ExtrudeDraft& draft,
    const ExtrudeDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel);

} // namespace simplesolid2::application
