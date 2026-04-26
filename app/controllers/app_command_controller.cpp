#include "app/controllers/app_command_controller.h"

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QKeySequence>
#include <QLatin1StringView>
#include <QMainWindow>
#include <QObject>
#include <QStatusBar>
#include <QWidget>
#include <memory>

#include "app/app_services.h"
#include "app/document_session.h"
#include "app/commands/command_ids.h"
#include "app/commands/command_registry.h"
#include "app/mission/mission_workspace_service.h"
#include "app/mission/ui/workspace_browser_dialog.h"
#include "logging/logger.h"
#include "ui/actions/app_actions.h"
#include "ui/dialogs/about_dialog.h"
#include "ui/dialogs/message_dialog.h"
#include "ui/dialogs/settings_dialog.h"
#include "ui/panels/console_panel.h"

namespace app::controllers {
namespace {

constexpr auto kCommandLogCategory = "commands";

[[nodiscard]] QString ToQString(QLatin1StringView text) {
    return {text};
}

}  // namespace

AppCommandController::AppCommandController(Targets targets, QObject* parent)
    : QObject(parent), targets_(targets) {}

void AppCommandController::Wire() {
    RegisterCommands();
    WireActions();
    UpdateCommandActionStates();
}

void AppCommandController::RegisterCommands() {
    if (commands_registered_ || targets_.services == nullptr) {
        return;
    }

    auto& registry = targets_.services->Commands();
    registered_commands_ = {
        std::make_shared<commands::LambdaCommand>(
            commands::CommandMetadata{
                .command_id = ToQString(commands::command_ids::kOpen),
                .title = tr("Open"),
                .description = tr("Open a workspace"),
                .group_id = QStringLiteral("file"),
                .shortcut = QKeySequence::Open,
                .placement =
                    {
                        .menu_id = QStringLiteral("file"),
                        .section_id = QStringLiteral("file.io"),
                        .show_in_toolbar = true,
                    },
            },
            [this](const commands::CommandContext& context) { return OpenFile(context); }),
        std::make_shared<commands::LambdaCommand>(
            commands::CommandMetadata{
                .command_id = ToQString(commands::command_ids::kSave),
                .title = tr("Save"),
                .description = tr("Save the current workspace"),
                .group_id = QStringLiteral("file"),
                .shortcut = QKeySequence::Save,
                .placement =
                    {
                        .menu_id = QStringLiteral("file"),
                        .section_id = QStringLiteral("file.io"),
                        .show_in_toolbar = true,
                    },
            },
            [this](const commands::CommandContext& context) { return SaveFile(context); },
            [](const commands::CommandContext& context) {
                return context.document_session != nullptr && context.document_session->IsDirty();
            }),
        std::make_shared<commands::LambdaCommand>(
            commands::CommandMetadata{
                .command_id = ToQString(commands::command_ids::kSaveAs),
                .title = tr("Save As"),
                .description = tr("Save the current workspace with a new name"),
                .group_id = QStringLiteral("file"),
                .shortcut = QKeySequence::SaveAs,
                .placement =
                    {
                        .menu_id = QStringLiteral("file"),
                        .section_id = QStringLiteral("file.io"),
                        .show_in_toolbar = false,
                    },
            },
            [this](const commands::CommandContext& context) { return SaveFileAs(context); }),
    };

    for (const auto& command : registered_commands_) {
        registry.RegisterCommand(command);
    }

    commands_registered_ = true;
}

void AppCommandController::WireActions() {
    auto* app_actions = targets_.app_actions;
    if (app_actions == nullptr) {
        return;
    }

    BindActionToCommand(app_actions->OpenAction(), ToQString(commands::command_ids::kOpen));
    BindActionToCommand(app_actions->SaveAction(), ToQString(commands::command_ids::kSave));
    BindActionToCommand(app_actions->SaveAsAction(), ToQString(commands::command_ids::kSaveAs));
    connect(app_actions->ExitAction(), &QAction::triggered, qApp, &QApplication::quit);
    connect(app_actions->SettingsAction(), &QAction::triggered, this, [this] {
        if (targets_.services == nullptr) {
            return;
        }

        ui::SettingsDialog dlg(targets_.services->Settings(), targets_.window);
        if (dlg.exec() != QDialog::Accepted) {
            return;
        }

        if (dlg.LanguageChanged()) {
            ui::MessageDialog::ShowInfo(
                targets_.window, tr("Restart Required"),
                tr("The language setting was saved. Restart the application to apply it."));
        }
    });
    connect(app_actions->AboutAction(), &QAction::triggered, this, [this] {
        ui::AboutDialog dlg(targets_.window);
        dlg.exec();
    });

    if (targets_.navigation_panel != nullptr) {
        connect(app_actions->ToggleNavigationAction(), &QAction::toggled, targets_.navigation_panel,
                &QWidget::setVisible);
    }
    if (targets_.info_panel != nullptr) {
        connect(app_actions->ToggleInfoAction(), &QAction::toggled, targets_.info_panel,
                &QWidget::setVisible);
    }
    if (targets_.console_panel != nullptr) {
        connect(app_actions->ToggleConsoleAction(), &QAction::toggled, targets_.console_panel,
                &QWidget::setVisible);
        auto* console_panel = qobject_cast<ui::ConsolePanel*>(targets_.console_panel);
        if (console_panel != nullptr) {
            connect(console_panel, &ui::ConsolePanel::SigCloseRequested, this, [app_actions] {
                app_actions->ToggleConsoleAction()->setChecked(false);
            });
        }
    }

    if (targets_.document_session != nullptr) {
        connect(targets_.document_session, &app::DocumentSession::SigDirtyChanged, this,
                [this](bool) { UpdateCommandActionStates(); });
        connect(targets_.document_session, &app::DocumentSession::SigFilePathChanged, this,
                [this](const QString&) { UpdateCommandActionStates(); });
    }

    connect(&mission::MissionWorkspaceRuntime(),
            &mission::MissionWorkspaceService::SigDocumentStateChanged, this, [this] {
                SyncDocumentSessionFromWorkspace();
                UpdateCommandActionStates();
            });
}

void AppCommandController::BindActionToCommand(QAction* action, const QString& command_id) {
    if (action == nullptr) {
        return;
    }

    connect(action, &QAction::triggered, this, [this, command_id] {
        TrackCommandExecution(command_id, ExecuteCommand(command_id));
    });
}

void AppCommandController::UpdateCommandActionStates() const {
    if (targets_.app_actions == nullptr || targets_.services == nullptr) {
        return;
    }

    const commands::CommandContext command_context = MakeCommandContext();
    const auto& registry = targets_.services->Commands();
    targets_.app_actions->OpenAction()->setEnabled(
        registry.IsCommandEnabled(ToQString(commands::command_ids::kOpen), command_context));
    targets_.app_actions->SaveAction()->setEnabled(
        registry.IsCommandEnabled(ToQString(commands::command_ids::kSave), command_context));
    targets_.app_actions->SaveAsAction()->setEnabled(
        registry.IsCommandEnabled(ToQString(commands::command_ids::kSaveAs), command_context));
}

commands::CommandContext AppCommandController::MakeCommandContext() const {
    return {
        .services = targets_.services,
        .document_session = targets_.document_session,
        .window = targets_.window,
    };
}

commands::CommandExecution AppCommandController::ExecuteCommand(const QString& command_id) const {
    if (targets_.services == nullptr) {
        return commands::CommandExecution::Completed(commands::CommandResult::Unavailable(
            tr("Command cannot run without application services.")));
    }

    return targets_.services->Commands().Execute(command_id, MakeCommandContext());
}

void AppCommandController::TrackCommandExecution(const QString& command_id,
                                                 commands::CommandExecution execution) const {
    if (execution.IsDeferred()) {
        auto* operation = execution.operation.data();
        QObject::connect(operation, &commands::CommandOperation::SigFinished, this,
                         [this, command_id](const commands::CommandResult& result) {
                             HandleCommandResult(command_id, result);
                             UpdateCommandActionStates();
                         });
        if (!execution.result.message.isEmpty()) {
            HandleCommandResult(command_id, execution.result);
        }
        return;
    }

    HandleCommandResult(command_id, execution.result);
    UpdateCommandActionStates();
}

void AppCommandController::HandleCommandResult(const QString& command_id,
                                               const commands::CommandResult& result) const {
    if (result.message.isEmpty()) {
        return;
    }

    auto* main_window = qobject_cast<QMainWindow*>(targets_.window);
    if (main_window != nullptr) {
        main_window->statusBar()->showMessage(result.message);
    }

    const std::string command_name = command_id.toStdString();
    const std::string message = result.message.toStdString();
    switch (result.status) {
        case commands::CommandStatus::kSuccess:
            logging::Logger::InfoFmtFor(kCommandLogCategory, "{}: {}", command_name, message);
            break;
        case commands::CommandStatus::kCancelled:
            logging::Logger::DebugFmtFor(kCommandLogCategory, "{}: {}", command_name, message);
            break;
        case commands::CommandStatus::kUnavailable:
            logging::Logger::WarnFmtFor(kCommandLogCategory, "{}: {}", command_name, message);
            break;
        case commands::CommandStatus::kFailed:
            logging::Logger::ErrorFmtFor(kCommandLogCategory, "{}: {}", command_name, message);
            break;
    }
}

commands::CommandExecution AppCommandController::OpenFile(const commands::CommandContext& context) {
    if (!ConfirmDiscardUnsaved(context)) {
        return commands::CommandExecution::Completed(
            commands::CommandResult::Cancelled(tr("Open cancelled.")));
    }

    mission::WorkspaceBrowserDialog dialog(mission::WorkspaceDialogMode::kOpen, context.window);
    if (dialog.exec() != QDialog::Accepted && !dialog.PerformedOperation()) {
        return commands::CommandExecution::Completed(
            commands::CommandResult::Cancelled(tr("Open cancelled.")));
    }

    SyncDocumentSessionFromWorkspace();
    const QString message = dialog.StatusMessage().isEmpty() ? tr("Workspace updated.")
                                                             : dialog.StatusMessage();
    return commands::CommandExecution::Completed(commands::CommandResult::Success(message));
}

commands::CommandExecution AppCommandController::SaveFile(const commands::CommandContext& context) {
    auto& workspace_service = mission::MissionWorkspaceRuntime();
    if (!workspace_service.IsDirty()) {
        return commands::CommandExecution::Completed(
            commands::CommandResult::Unavailable(tr("There are no changes to save.")));
    }

    if (workspace_service.NeedsSaveDialog()) {
        return SaveFileAs(context);
    }

    QString error_message;
    if (!workspace_service.SaveWorkspace(&error_message)) {
        return commands::CommandExecution::Completed(
            commands::CommandResult::Failed(error_message));
    }

    SyncDocumentSessionFromWorkspace();
    return commands::CommandExecution::Completed(
        commands::CommandResult::Success(tr("Saved %1.")
                                             .arg(workspace_service.WorkspaceDisplayName())));
}

commands::CommandExecution AppCommandController::SaveFileAs(
    const commands::CommandContext& context) {
    mission::WorkspaceBrowserDialog dialog(mission::WorkspaceDialogMode::kSave, context.window);
    if (dialog.exec() != QDialog::Accepted && !dialog.PerformedOperation()) {
        return commands::CommandExecution::Completed(
            commands::CommandResult::Cancelled(tr("Save cancelled.")));
    }

    SyncDocumentSessionFromWorkspace();
    const QString message = dialog.StatusMessage().isEmpty() ? tr("Workspace saved.")
                                                             : dialog.StatusMessage();
    return commands::CommandExecution::Completed(commands::CommandResult::Success(message));
}

bool AppCommandController::ConfirmDiscardUnsaved(
    const commands::CommandContext& context) const {
    if (!mission::MissionWorkspaceRuntime().IsDirty()) {
        return true;
    }

    const auto reply = ui::MessageDialog::Confirm(
        context.window, tr("Unsaved Workspace"),
        tr("The current workspace has unsaved changes. Continue without saving?"));
    return reply == QDialog::Accepted;
}

void AppCommandController::SyncDocumentSessionFromWorkspace() const {
    if (targets_.document_session == nullptr) {
        return;
    }

    auto& workspace_service = mission::MissionWorkspaceRuntime();
    targets_.document_session->SetCurrentFilePath(workspace_service.WorkspaceFilePath());
    targets_.document_session->SetDirty(workspace_service.IsDirty());
}

}  // namespace app::controllers
