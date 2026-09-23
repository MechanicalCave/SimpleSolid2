#pragma once

#include "part_document_tree_controller.hpp"

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QObject>

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace simplesolid2::ui {

class PartViewportController final : public QObject {
public:
    using SelectionChangedHandler = std::function<void(
        const std::vector<core::BuiltinReferenceRole>&,
        std::optional<core::BuiltinReferenceRole>)>;

    PartViewportController(
        PartDocumentTreeController& tree,
        viewer::IDocumentViewport* viewport,
        QObject* parent = nullptr);

    void setDocumentSession(
        application::DocumentSession* session);

    void clear();
    void refreshPresentation();

    void setSelectionChangedHandler(
        SelectionChangedHandler handler) {
        selection_changed_handler_ = std::move(handler);
    }

    [[nodiscard]] std::optional<core::BuiltinReferenceRole>
    primarySelection() const;

private:
    struct SemanticSelection final {
        std::vector<core::BuiltinReferenceRole> selected;
        std::optional<core::BuiltinReferenceRole> primary;
    };

    [[nodiscard]] static viewer::PresentationToken tokenFor(
        core::BuiltinReferenceRole role) noexcept;

    [[nodiscard]] static std::optional<core::BuiltinReferenceRole>
    roleFor(viewer::PresentationToken token) noexcept;

    [[nodiscard]] viewer::ReferenceScene buildScene() const;

    [[nodiscard]] SemanticSelection& activeSelection();
    [[nodiscard]] const SemanticSelection* activeSelection() const;

    void onTreeSelection(
        const std::vector<core::BuiltinReferenceRole>& selected,
        std::optional<core::BuiltinReferenceRole> primary);

    void onViewportIntent(
        const viewer::SelectionIntent& intent);

    void applySelectionToSurfaces();
    void notifySelectionChanged();

    PartDocumentTreeController* tree_{};
    viewer::IDocumentViewport* viewport_{};
    application::DocumentSession* session_{};

    std::unordered_map<std::string, SemanticSelection>
        selections_;

    SelectionChangedHandler selection_changed_handler_;
};

} // namespace simplesolid2::ui
