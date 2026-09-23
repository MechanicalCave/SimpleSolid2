#pragma once

#include "document_selection.hpp"

#include <simplesolid2/part/part_document.hpp>
#include <simplesolid2/viewer/reference_presentation.hpp>

#include <optional>

namespace simplesolid2::ui {

[[nodiscard]] viewer::PresentationToken
presentationTokenFor(
    core::BuiltinReferenceRole role) noexcept;

[[nodiscard]] std::optional<core::BuiltinReferenceRole>
builtinReferenceFor(
    viewer::PresentationToken token) noexcept;

[[nodiscard]] viewer::ReferenceScene
partReferenceScene(
    const part::PartDocument& document,
    const DocumentSelectionState& selection);

[[nodiscard]] viewer::ReferenceGridPresentation
defaultPartReferenceGrid() noexcept;

} // namespace simplesolid2::ui
