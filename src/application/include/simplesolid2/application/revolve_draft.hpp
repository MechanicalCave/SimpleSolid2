#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/kernel/solid_modeling.hpp>
#include <simplesolid2/part/axis.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/feature_evaluation.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace simplesolid2::application {

class DocumentSession;

using RevolveDraftGeneration = std::uint64_t;

enum class RevolveDraftMode : std::uint8_t {
    create,
    edit,
};

enum class RevolveDraftExtentMode : std::uint8_t {
    one_side,
    midplane,
};

class RevolveDraft final {
public:
    [[nodiscard]] static RevolveDraft
    beginCreate(const DocumentSession& session);

    [[nodiscard]] static std::optional<RevolveDraft>
    beginCreate(
        const DocumentSession& session,
        part::ProfileId profile_id);

    [[nodiscard]] static std::optional<RevolveDraft>
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

    [[nodiscard]] RevolveDraftGeneration
    generation() const noexcept {
        return generation_;
    }

    [[nodiscard]] RevolveDraftMode
    mode() const noexcept {
        return mode_;
    }

    [[nodiscard]] const std::optional<part::ProfileId>&
    profileId() const noexcept {
        return profile_id_;
    }

    [[nodiscard]] const std::optional<part::AxisReference>&
    axis() const noexcept {
        return axis_;
    }

    [[nodiscard]] std::optional<part::FeatureId>
    featureId() const noexcept {
        return feature_id_;
    }

    [[nodiscard]] part::RevolveOperation
    operation() const noexcept {
        return operation_;
    }

    [[nodiscard]] const part::RevolveExtent&
    extent() const noexcept {
        return extent_;
    }

    [[nodiscard]] const std::string&
    name() const noexcept {
        return name_;
    }

    [[nodiscard]] RevolveDraftExtentMode
    extentMode() const noexcept;

    [[nodiscard]] core::AngleValue
    angle() const noexcept;

    [[nodiscard]] bool reversed() const noexcept;

    [[nodiscard]] bool valid() const noexcept;

    [[nodiscard]] bool setProfile(
        part::ProfileId profile_id) noexcept;

    [[nodiscard]] bool setAxis(
        part::AxisReference axis);

    [[nodiscard]] bool setOperation(
        part::RevolveOperation operation) noexcept;

    [[nodiscard]] bool setExtentMode(
        RevolveDraftExtentMode mode) noexcept;

    [[nodiscard]] bool setAngle(
        core::AngleValue angle) noexcept;

    [[nodiscard]] bool setReversed(
        bool reversed) noexcept;

    [[nodiscard]] bool setName(
        std::string name);

private:
    RevolveDraft(
        core::DocumentId document_id,
        core::DocumentRevision source_revision,
        RevolveDraftMode mode,
        std::optional<part::ProfileId> profile_id,
        std::optional<part::AxisReference> axis,
        std::optional<part::FeatureId> feature_id,
        part::RevolveOperation operation,
        part::RevolveExtent extent,
        std::string name);

    [[nodiscard]] bool canAdvance() const noexcept;
    void advance() noexcept;

    core::DocumentId document_id_;
    core::DocumentRevision source_revision_;
    RevolveDraftGeneration generation_{1U};
    RevolveDraftMode mode_{RevolveDraftMode::create};
    std::optional<part::ProfileId> profile_id_;
    std::optional<part::AxisReference> axis_;
    std::optional<part::FeatureId> feature_id_;
    part::RevolveOperation operation_{
        part::RevolveOperation::add};
    part::RevolveExtent extent_;
    std::string name_;
};

enum class RevolveDraftEvaluationStatus : std::uint8_t {
    ok,
    stale_document,
    stale_revision,
    incomplete_draft,
    missing_profile,
    missing_feature,
    suppressed_feature,
    feature_id_exhausted,
    invalid_candidate,
    target_failed,
};

struct RevolveDraftEvaluationResult final {
    RevolveDraftEvaluationStatus status{
        RevolveDraftEvaluationStatus::incomplete_draft};
    std::optional<core::DocumentId> document_id;
    core::DocumentRevision source_revision;
    RevolveDraftGeneration draft_generation{};
    RevolveDraftMode mode{
        RevolveDraftMode::create};
    std::optional<part::ProfileId> profile_id;
    std::optional<part::AxisReference> axis;
    std::optional<part::FeatureId> feature_id;
    part::RevolveOperation operation{
        part::RevolveOperation::add};
    part::RevolveExtent extent;
    std::string name;
    part::BodyEvaluationStatus body_status{
        part::BodyEvaluationStatus::unavailable};
    kernel::RuntimeSolidHandle body_solid;
    std::optional<kernel::SolidPresentationMesh>
        preview_delta_mesh;
    std::optional<
        part::FeatureEvaluationDiagnosticCode>
        evaluation_diagnostic;

    [[nodiscard]] bool committable() const noexcept {
        return status ==
               RevolveDraftEvaluationStatus::ok;
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

enum class RevolveDraftFinishStatus : std::uint8_t {
    committed,
    stale_context,
    stale_evaluation,
    invalid_draft,
    rejected,
};

struct RevolveDraftFinishResult final {
    RevolveDraftFinishStatus status{
        RevolveDraftFinishStatus::rejected};
    bool changed{};
    std::optional<part::FeatureId> feature_id;
    std::string diagnostic;

    [[nodiscard]] bool ok() const noexcept {
        return status ==
               RevolveDraftFinishStatus::committed;
    }
};

[[nodiscard]] RevolveDraftFinishResult
finishRevolveDraft(
    DocumentSession& session,
    const RevolveDraft& draft,
    const RevolveDraftEvaluationResult& evaluation,
    kernel::ISolidModelingKernel& modeling_kernel);

} // namespace simplesolid2::application
