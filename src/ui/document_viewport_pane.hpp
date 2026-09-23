#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QWidget>

#include <vector>

class QFrame;
class QResizeEvent;
class QToolButton;

namespace simplesolid2::ui {

class DocumentViewportPane final : public QWidget {
public:
    DocumentViewportPane(
        QWidget* viewport_widget,
        viewer::IDocumentViewport& viewport,
        QWidget* parent = nullptr);
    ~DocumentViewportPane() override;

    DocumentViewportPane(const DocumentViewportPane&) = delete;
    DocumentViewportPane& operator=(const DocumentViewportPane&) = delete;

protected:
    void resizeEvent(QResizeEvent* event) override;

private:
    struct StandardViewButton final {
        QToolButton* button{};
        viewer::StandardView view{};
    };

    void syncFromCamera(
        const viewer::CameraState& state);
    void toggleProjection();

    QWidget* viewport_widget_{};
    viewer::IDocumentViewport* viewport_{};
    QFrame* navigation_panel_{};
    QToolButton* projection_button_{};
    std::vector<StandardViewButton> standard_view_buttons_;
};

} // namespace simplesolid2::ui
