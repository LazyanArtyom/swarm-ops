#pragma once

#include <QMainWindow>
#include <cstdint>

class QMenu;
class QWidget;

namespace app::controllers {
class AppCommandController;
class NavigationController;
class WorkspaceController;
}  // namespace app::controllers

namespace app::ui {
namespace actions {
class AppActions;
}  // namespace actions
namespace chrome {
class MainMenuBar;
class MainToolBar;
}  // namespace chrome
}  // namespace app::ui

namespace app::ui::theme {
enum class ThemeId : std::uint8_t;
}  // namespace app::ui::theme

namespace app {

class AppContext;
class DocumentSession;
class ShellLayoutManager;
struct MainWindowLayoutProfile;

class MainWindow final : public QMainWindow {
    Q_OBJECT

   public:
    explicit MainWindow(AppContext& context, QWidget* parent = nullptr);
    ~MainWindow() override = default;

   protected:
    void closeEvent(QCloseEvent* event) override;
    QMenu* createPopupMenu() override;
    void showEvent(QShowEvent* event) override;

   private:
    void SetupUi();

    void CreateChrome();
    void CreateStatusBar();
    void CreateWorkspace();
    void CreateControllers();

    void RefreshToolBarMetrics();
    void RefreshWindowTitle();
    void SyncDocumentSessionFromWorkspace();
    [[nodiscard]] bool ConfirmDiscardUnsavedWorkspace();

    AppContext& context_;
    ui::actions::AppActions* app_actions_{nullptr};
    DocumentSession* document_session_{nullptr};
    controllers::WorkspaceController* workspace_controller_{nullptr};
    controllers::NavigationController* navigation_controller_{nullptr};
    controllers::AppCommandController* app_command_controller_{nullptr};
    ShellLayoutManager* shell_layout_manager_{nullptr};
    ui::chrome::MainMenuBar* main_menu_bar_{nullptr};
    ui::chrome::MainToolBar* main_tool_bar_{nullptr};
};

}  // namespace app
