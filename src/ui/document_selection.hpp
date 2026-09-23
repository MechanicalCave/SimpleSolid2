#pragma once

#include <simplesolid2/core/document_reference.hpp>

#include <algorithm>
#include <optional>
#include <vector>

namespace simplesolid2::ui {

enum class DocumentSelectionTargetKind {
    document_root,
    builtin_reference,
};

struct DocumentSelectionTarget final {
    DocumentSelectionTargetKind kind{
        DocumentSelectionTargetKind::document_root};
    core::BuiltinReferenceRole builtin_reference{
        core::BuiltinReferenceRole::origin_point};

    [[nodiscard]] static constexpr
    DocumentSelectionTarget documentRoot() noexcept {
        return {};
    }

    [[nodiscard]] static constexpr
    DocumentSelectionTarget builtinReference(
        core::BuiltinReferenceRole role) noexcept {
        return {
            DocumentSelectionTargetKind::builtin_reference,
            role};
    }

    [[nodiscard]] constexpr bool valid() const noexcept {
        return kind == DocumentSelectionTargetKind::document_root ||
               (kind == DocumentSelectionTargetKind::builtin_reference &&
                core::isBuiltinReferenceRole(builtin_reference));
    }

    friend constexpr bool operator==(
        const DocumentSelectionTarget&,
        const DocumentSelectionTarget&) = default;
};

struct DocumentSelectionState final {
    std::vector<DocumentSelectionTarget> selected;
    std::optional<DocumentSelectionTarget> primary;

    [[nodiscard]] bool contains(
        const DocumentSelectionTarget& target) const {
        return std::find(
                   selected.begin(),
                   selected.end(),
                   target) != selected.end();
    }

    [[nodiscard]] bool valid() const {
        for (const auto& target : selected) {
            if (!target.valid()) return false;
        }
        return !primary ||
               (primary->valid() && contains(*primary));
    }

    [[nodiscard]] static DocumentSelectionState
    documentRootOnly() {
        const auto root =
            DocumentSelectionTarget::documentRoot();
        return {{root}, root};
    }

    friend bool operator==(
        const DocumentSelectionState&,
        const DocumentSelectionState&) = default;
};

} // namespace simplesolid2::ui
