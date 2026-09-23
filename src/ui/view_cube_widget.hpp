#pragma once

#include <simplesolid2/viewer/document_viewport.hpp>

#include <QFrame>

class QPushButton;
class QString;

namespace simplesolid2::ui {

class ViewCubeWidget final : public QFrame {
public:
    explicit ViewCubeWidget(
        viewer::IDocumentViewport* viewport,
        QWidget* parent = nullptr);

    void setViewport(
        viewer::IDocumentViewport* viewport);

private:
    QPushButton* addViewButton(
        const QString& text,
        const QString& tooltip,
        viewer::StandardView view,
        int row,
        int column);

    void applyStandardView(
        viewer::StandardView view);
    void toggleProjection();
    void refreshProjectionLabel();
    void syncEnabledState();

    viewer::IDocumentViewport* viewport_{};
    class QGridLayout* grid_{};
    QPushButton* fit_button_{};
    QPushButton* projection_button_{};
};

} // namespace simplesolid2::ui
