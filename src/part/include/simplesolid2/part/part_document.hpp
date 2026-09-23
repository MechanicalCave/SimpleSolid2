#pragma once

#include <simplesolid2/core/document.hpp>
#include <simplesolid2/core/document_reference.hpp>

#include <utility>

namespace simplesolid2::part {

struct PartPresentationState final {
    core::BuiltinReferenceVisibility builtin_references;

    friend bool operator==(
        const PartPresentationState&,
        const PartPresentationState&) = default;
};

struct PartAuthoredState final {
    core::DocumentProperties properties;
    PartPresentationState presentation;

    friend bool operator==(const PartAuthoredState&, const PartAuthoredState&) = default;
};

enum class PartCommitErrorCode {
    none,
    inactive_transaction,
    revision_exhausted,
};

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
    [[nodiscard]] static PartDocument restore(
        core::DocumentId id,
        PartAuthoredState state,
        core::DocumentRevision revision = {});

    [[nodiscard]] const core::DocumentId& documentId() const noexcept { return id_; }
    [[nodiscard]] core::DocumentRevision revision() const noexcept { return revision_; }
    [[nodiscard]] const PartAuthoredState& state() const noexcept { return state_; }
    [[nodiscard]] const core::DocumentProperties& properties() const noexcept {
        return state_.properties;
    }
    [[nodiscard]] const PartPresentationState& presentation() const noexcept {
        return state_.presentation;
    }
    [[nodiscard]] bool builtinReferenceVisible(
        core::BuiltinReferenceRole role) const noexcept {
        return state_.presentation.builtin_references.visible(role);
    }

private:
    friend class PartDocumentTransaction;

    PartDocument(
        core::DocumentId id,
        PartAuthoredState state,
        core::DocumentRevision revision)
        : id_{std::move(id)},
          state_{std::move(state)},
          revision_{revision} {}

    [[nodiscard]] PartCommitResult commitState(PartAuthoredState state);

    core::DocumentId id_;
    PartAuthoredState state_;
    core::DocumentRevision revision_;
};

class PartDocumentTransaction final {
public:
    explicit PartDocumentTransaction(PartDocument& document)
        : document_{&document}, staged_{document.state()} {}

    PartDocumentTransaction(const PartDocumentTransaction&) = delete;
    PartDocumentTransaction& operator=(const PartDocumentTransaction&) = delete;
    PartDocumentTransaction(PartDocumentTransaction&&) = delete;
    PartDocumentTransaction& operator=(PartDocumentTransaction&&) = delete;
    ~PartDocumentTransaction() = default;

    void setProperties(core::DocumentProperties properties) {
        staged_.properties = std::move(properties);
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
    PartAuthoredState staged_;
    bool active_{true};
};

} // namespace simplesolid2::part
