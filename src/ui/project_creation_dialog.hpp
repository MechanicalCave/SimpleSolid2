#pragma once

#include <QDialog>
#include <QString>

class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QWidget;

namespace simplesolid2::ui {

class ProjectCreationDialog final : public QDialog {
public:
    explicit ProjectCreationDialog(QWidget* parent = nullptr);

    [[nodiscard]] QString projectName() const;
    [[nodiscard]] QString location() const;
    [[nodiscard]] QString projectFolder() const;

private:
    void browseLocation();
    void updateDerivedState();

    QLineEdit* project_name_{};
    QLineEdit* location_{};
    QLineEdit* project_folder_{};
    QLabel* final_path_{};
    QPushButton* browse_button_{};
    QDialogButtonBox* buttons_{};

    bool folder_follows_name_{true};
};

} // namespace simplesolid2::ui
