#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <functional>

class QWidget;

namespace simplesolid2::ui {

struct ViewportSurface final {
    QWidget* widget{};
    viewer::IDocumentViewport* viewport{};

    [[nodiscard]] bool valid() const noexcept {
        return widget != nullptr && viewport != nullptr;
    }
};

using ViewportFactory =
    std::function<ViewportSurface(QWidget* parent)>;

} // namespace simplesolid2::ui
