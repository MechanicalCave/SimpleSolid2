#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/core/document_reference.hpp>
#include <simplesolid2/core/units.hpp>
#include <simplesolid2/part/datum.hpp>
#include <simplesolid2/part/feature.hpp>
#include <simplesolid2/part/part_sketch.hpp>
#include <simplesolid2/part/profile.hpp>

#include <optional>
#include <utility>
#include <vector>

namespace simplesolid2::part {

struct PartPresentationState final {
    core::BuiltinReferenceVisibility builtin_references;

    friend bool operator==(
        const PartPresentationState&,
        const PartPresentationState&) = default;
};

struct PartAuthoredState final {
    PartAuthoredState() noexcept {
        const auto initial_body_id =
            next_body_id.allocate();
        if (initial_body_id) {
            body.id = *initial_body_id;
        }
    }

    core::DocumentProperties properties;
    PartPresentationState presentation;
    DatumIdCursor next_datum_id;
    std::vector<OffsetDatumPlane> datum_planes;
    std::vector<PartSketch> sketches;
    ProfileIdCursor next_profile_id;
    std::vector<PartProfile> profiles;
    core::LengthUnit length_unit{
        core::LengthUnit::millimetre};
    ModelingSemanticsVersion modeling_semantics_version{
        current_modeling_semantics_version};
    BodyIdCursor next_body_id;
    PartBody body;

    friend bool operator==(
        const PartAuthoredState&,
        const PartAuthoredState&) = default;
};

enum class PartCommitErrorCode {
    none,
    inactive_transaction,
    stale_transaction,
    invalid_state,
    revision_exhausted,
};

enum class PartReconstructErrorCode {
    none,
    invalid_state,
};

struct PartReconstructResult;

struct PartCommitResult final {
    PartCommitErrorCode code{PartCommitErrorCode::none};
    bool changed{false};

    [[nodiscard]] bool ok() const noexcept {
        return code == PartCommitErrorCode::none;
    }
};

class PartDocumentTransaction;

class PartDocument final {
public:
    PartDocument(const PartDocument&) = delete;
    PartDocument& operator=(const PartDocument&) = delete;
    PartDocument(PartDocument&&) noexcept = default;
    PartDocument& operator=(PartDocument&&) noexcept = default;
    ~PartDocument() = default;

    [[nodiscard]] static PartDocument create(core::DocumentId id);
    [[nodiscard]] static PartReconstructResult restore(
        core::DocumentId id,
        PartAuthoredState state,
        core::DocumentRevision revision = {});

    [[nodiscard]] const core::DocumentId& documentId() const noexcept { return id_; }
    [[nodiscard]] core::DocumentRevision revision() const noexcept { return revision_; }
    [[nodiscard]] const PartAuthoredState& state() const noexcept { return state_; }
    [[nodiscard]] const core::DocumentProperties& properties() const noexcept {
        return state_.properties;
    }
    [[nodiscard]] core::LengthUnit lengthUnit() const noexcept {
        return state_.length_unit;
    }
    [[nodiscard]] const PartPresentationState& presentation() const noexcept {
        return state_.presentation;
    }
    [[nodiscard]] bool builtinReferenceVisible(
        core::BuiltinReferenceRole role) const noexcept {
        return state_.presentation.builtin_references.visible(role);
    }

    [[nodiscard]] DatumIdCursor datumIdCursor() const noexcept {
        return state_.next_datum_id;
    }

    [[nodiscard]] const std::vector<OffsetDatumPlane>&
    datumPlanes() const noexcept {
        return state_.datum_planes;
    }

    [[nodiscard]] const OffsetDatumPlane* findDatumPlane(
        DatumId id) const noexcept;

    [[nodiscard]] const std::vector<PartSketch>& sketches() const noexcept {
        return state_.sketches;
    }

    [[nodiscard]] const PartSketch* findSketch(
        const sketch::SketchId& id) const noexcept;

    [[nodiscard]] ProfileIdCursor profileIdCursor()
        const noexcept {
        return state_.next_profile_id;
    }

    [[nodiscard]] const std::vector<PartProfile>&
    profiles() const noexcept {
        return state_.profiles;
    }

    [[nodiscard]] ModelingSemanticsVersion
    modelingSemanticsVersion() const noexcept {
        return state_.modeling_semantics_version;
    }

    [[nodiscard]] BodyIdCursor bodyIdCursor() const noexcept {
        return state_.next_body_id;
    }

    [[nodiscard]] const PartBody& body() const noexcept {
        return state_.body;
    }

    [[nodiscard]] const PartProfile* findProfile(
        ProfileId id) const noexcept;

    [[nodiscard]] const PartFeature* findFeature(
        FeatureId id) const noexcept;

    [[nodiscard]] bool profilePresentationVisible(
        ProfileId id) const noexcept;

    [[nodiscard]] std::optional<ResolvedProfileRegion>
    evaluateProfile(ProfileId id) const;

private:
    friend class PartDocumentTransaction;

    PartDocument(
        core::DocumentId id,
        PartAuthoredState state,
        core::DocumentRevision revision)
        : id_{std::move(id)},
          state_{std::move(state)},
          revision_{revision} {}

    [[nodiscard]] static bool validAuthoredState(
        const PartAuthoredState& state) noexcept;

    [[nodiscard]] PartCommitResult commitState(
        core::DocumentRevision expected_revision,
        PartAuthoredState state);

    core::DocumentId id_;
    PartAuthoredState state_;
    core::DocumentRevision revision_;
};

struct PartReconstructResult final {
    std::optional<PartDocument> document;
    PartReconstructErrorCode code{
        PartReconstructErrorCode::none};

    [[nodiscard]] bool ok() const noexcept {
        return document.has_value() &&
               code == PartReconstructErrorCode::none;
    }
};

class PartDocumentTransaction final {
public:
    explicit PartDocumentTransaction(PartDocument& document)
        : document_{&document},
          base_revision_{document.revision()},
          staged_{document.state()} {}

    PartDocumentTransaction(const PartDocumentTransaction&) = delete;
    PartDocumentTransaction& operator=(const PartDocumentTransaction&) = delete;
    PartDocumentTransaction(PartDocumentTransaction&&) = delete;
    PartDocumentTransaction& operator=(PartDocumentTransaction&&) = delete;
    ~PartDocumentTransaction() = default;

    void setProperties(core::DocumentProperties properties) {
        staged_.properties = std::move(properties);
    }

    [[nodiscard]] bool setLengthUnit(
        core::LengthUnit unit) noexcept {
        if (!core::isLengthUnit(unit)) {
            return false;
        }
        staged_.length_unit = unit;
        return true;
    }

    [[nodiscard]] bool setBuiltinReferenceVisible(
        core::BuiltinReferenceRole role,
        bool visible) noexcept {
        return staged_.presentation.builtin_references.setVisible(role, visible);
    }

    void replaceState(PartAuthoredState state) {
        staged_ = std::move(state);
    }

    [[nodiscard]] const PartAuthoredState& stagedState() const noexcept {
        return staged_;
    }

    [[nodiscard]] PartCommitResult commit();
    void rollback() noexcept { active_ = false; }

private:
    PartDocument* document_{};
    core::DocumentRevision base_revision_;
    PartAuthoredState staged_;
    bool active_{true};
};

} // namespace simplesolid2::part
