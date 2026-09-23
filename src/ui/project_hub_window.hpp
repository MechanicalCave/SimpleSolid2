#pragma once

#include "project_hub_controller.hpp"

#include <QMainWindow>

#include <filesystem>
#include <string>

class QLabel;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QWidget;

namespace simplesolid2::ui {

class ProjectHubWindow final : public QMainWindow {
public:
    explicit ProjectHubWindow(
        std::filesystem::path recent_catalog_path,
        QWidget* parent = nullptr);

private:
    void buildHubPage();
    void buildWorkspacePage();
    void refreshRecent();
    void createProject();
    void openProject();
    void openSelectedRecent();
    void locateSelectedRecent();
    void removeSelectedRecent();
    void closeProject();
    void enterWorkspace();

    [[nodiscard]] std::string selectedProjectId() const;
    void showFailure(
        const application::internal::ProjectHubDiagnostic& diagnostic);

    application::internal::ProjectHubController controller_;

    QStackedWidget* pages_{};
    QWidget* hub_page_{};
    QWidget* workspace_page_{};

    QListWidget* recent_list_{};
    QPushButton* open_recent_button_{};
    QPushButton* locate_recent_button_{};
    QPushButton* remove_recent_button_{};
    QLabel* hub_status_{};

    QLabel* workspace_name_{};
    QLabel* workspace_id_{};
    QLabel* workspace_path_{};
};

} // namespace simplesolid2::ui
