#include "app/main_window.h"

#include <QAction>
#include <QCloseEvent>
#include <QStatusBar>

#include "app/app_context.h"
#include "app/document_session.h"
#include "app/controllers/app_command_controller.h"
#include "app/controllers/navigation_controller.h"
#include "app/controllers/page_info_panel_controller.h"
#include "app/controllers/workspace_controller.h"
#include "app/mission/mission_workspace_service.h"
#include "app/shell_layout_manager.h"
#include "ui/actions/app_actions.h"
#include "ui/chrome/main_menu_bar.h"
#include "ui/chrome/main_tool_bar.h"
#include "ui/dialogs/message_dialog.h"
#include "ui/panels/console_panel.h"
#include "ui/panels/info_panel.h"
#include "ui/panels/navigation_panel.h"
#include "ui/theme/theme_metrics.h"

namespace app {

MainWindow::MainWindow(AppContext& context, QWidget* parent)
    : QMainWindow(parent), context_(context) {
    SetupUi();
}

void MainWindow::SetupUi() {
    const bool dark = context_.Services().Settings().Theme().DarkTheme();
    context_.Services().Theme().Apply(dark ? ui::theme::ThemeId::kDark
                                           : ui::theme::ThemeId::kLight);

    CreateChrome();
    CreateStatusBar();
    CreateWorkspace();
    CreateControllers();

    shell_layout_manager_->Restore();
    connect(app_actions_->ResetLayoutAction(), &QAction::triggered, shell_layout_manager_,
            &ShellLayoutManager::Reset);
    RefreshToolBarMetrics();

    if (navigation_controller_ != nullptr) {
        navigation_controller_->OpenHome();
    }
    SyncDocumentSessionFromWorkspace();
    RefreshWindowTitle();
}

void MainWindow::CreateChrome() {
    app_actions_ = new ui::actions::AppActions(this);
    document_session_ = new DocumentSession(this);

    main_menu_bar_ = new ui::chrome::MainMenuBar(app_actions_, this);
    setMenuBar(main_menu_bar_);

    main_tool_bar_ = new ui::chrome::MainToolBar(app_actions_, this);
    addToolBar(main_tool_bar_);
}

void MainWindow::CreateStatusBar() {
    auto* status_bar = statusBar();
    status_bar->setSizeGripEnabled(false);
}

void MainWindow::CreateWorkspace() {
    workspace_controller_ = new controllers::WorkspaceController(context_.Services().Pages(), this);
    auto* central_panel = workspace_controller_->CreateCentralPanel(this);

    shell_layout_manager_ = new ShellLayoutManager(this, context_.Services().Settings(),
                                                   app_actions_, central_panel,
                                                   layout_profiles::Default(), this);
    shell_layout_manager_->BuildShell();
}

void MainWindow::CreateControllers() {
    if (workspace_controller_ == nullptr || shell_layout_manager_ == nullptr) {
        return;
    }

    const auto& widgets = shell_layout_manager_->Widgets();

    navigation_controller_ = new controllers::NavigationController(
        context_.Services(), widgets.navigation_panel, widgets.central_panel, app_actions_, this);
    navigation_controller_->RefreshNavigationItems();

    page_info_panel_controller_ = new controllers::PageInfoPanelController(
        widgets.central_panel, widgets.info_panel, this);
    page_info_panel_controller_->Wire();

    const controllers::AppCommandController::Targets command_targets{
        .services = &context_.Services(),
        .document_session = document_session_,
        .app_actions = app_actions_,
        .window = this,
        .navigation_panel = widgets.navigation_panel,
        .info_panel = widgets.info_panel,
        .console_panel = widgets.console_panel,
    };
    app_command_controller_ = new controllers::AppCommandController(command_targets, this);
    app_command_controller_->Wire();

    connect(document_session_, &DocumentSession::SigDirtyChanged, this,
            [this](bool) { RefreshWindowTitle(); });
    connect(document_session_, &DocumentSession::SigFilePathChanged, this,
            [this](const QString&) { RefreshWindowTitle(); });
    connect(&mission::MissionWorkspaceRuntime(),
            &mission::MissionWorkspaceService::SigDocumentStateChanged, this,
            [this] { SyncDocumentSessionFromWorkspace(); });

    connect(&ui::theme::ThemeMetrics::Instance(), &ui::theme::ThemeMetrics::SigMetricsChanged, this,
            [this] { RefreshToolBarMetrics(); });
}

void MainWindow::RefreshToolBarMetrics() {
    if (main_tool_bar_ == nullptr) {
        return;
    }
    main_tool_bar_->RefreshMetrics();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!ConfirmDiscardUnsavedWorkspace()) {
        event->ignore();
        return;
    }

    if (shell_layout_manager_ != nullptr) {
        shell_layout_manager_->Save();
    }
    QMainWindow::closeEvent(event);
}

QMenu* MainWindow::createPopupMenu() {
    return nullptr;
}

void MainWindow::showEvent(QShowEvent* event) {
    QMainWindow::showEvent(event);
    if (shell_layout_manager_ != nullptr) {
        shell_layout_manager_->AttachScreenTracking();
    }
    RefreshWindowTitle();
}

void MainWindow::RefreshWindowTitle() {
    const auto& workspace_service = mission::MissionWorkspaceRuntime();
    const QString dirty_marker = workspace_service.IsDirty() ? QStringLiteral("*") : QString();
    setWindowTitle(QStringLiteral("%1%2 - %3")
                       .arg(workspace_service.WorkspaceDisplayName(), dirty_marker,
                            context_.Services().Info().WindowTitle()));
}

void MainWindow::SyncDocumentSessionFromWorkspace() {
    if (document_session_ == nullptr) {
        return;
    }

    const auto& workspace_service = mission::MissionWorkspaceRuntime();
    document_session_->SetCurrentFilePath(workspace_service.WorkspaceFilePath());
    document_session_->SetDirty(workspace_service.IsDirty());
    RefreshWindowTitle();
}

bool MainWindow::ConfirmDiscardUnsavedWorkspace() {
    if (!mission::MissionWorkspaceRuntime().IsDirty()) {
        return true;
    }

    const auto reply = ui::MessageDialog::Confirm(
        this, tr("Unsaved Workspace"),
        tr("The current workspace has unsaved changes. Close without saving?"));
    return reply == QDialog::Accepted;
}

}  // namespace app
