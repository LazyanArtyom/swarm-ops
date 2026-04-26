#include "actions/app_actions.h"

#include <QAction>
#include <QKeySequence>
#include <QSignalBlocker>

#include "theme/theme_icons.h"

namespace app::ui::actions {
namespace {

QString WithShortcut(QAction* action, const QString& text) {
    if (action == nullptr || action->shortcut().isEmpty()) {
        return text;
    }

    return QStringLiteral("%1 (%2)").arg(text,
                                         action->shortcut().toString(QKeySequence::NativeText));
}

struct ActionHelp final {
    QString tool_tip;
    QString status_tip;
};

void SetActionHelp(QAction* action, const ActionHelp& help) {
    if (action == nullptr) {
        return;
    }

    action->setToolTip(WithShortcut(action, help.tool_tip));
    action->setStatusTip(help.status_tip);
}

}  // namespace

AppActions::AppActions(QObject* parent) : QObject(parent) {
    CreateActions();
    ConfigureToolTips();
    BindIcons();
}

QAction* AppActions::OpenAction() const {
    return open_action_;
}

QAction* AppActions::SaveAction() const {
    return save_action_;
}

QAction* AppActions::SaveAsAction() const {
    return save_as_action_;
}

QAction* AppActions::StartSimulationAction() const {
    return start_simulation_action_;
}

QAction* AppActions::StartMissionAction() const {
    return start_mission_action_;
}

QAction* AppActions::SettingsAction() const {
    return settings_action_;
}

QAction* AppActions::ExitAction() const {
    return exit_action_;
}

QAction* AppActions::ToggleNavigationAction() const {
    return toggle_navigation_action_;
}

QAction* AppActions::ToggleInfoAction() const {
    return toggle_info_action_;
}

QAction* AppActions::ToggleConsoleAction() const {
    return toggle_console_action_;
}

QAction* AppActions::ResetLayoutAction() const {
    return reset_layout_action_;
}

QAction* AppActions::AboutAction() const {
    return about_action_;
}

void AppActions::SetPanelVisibilityChecked(bool navigation_visible, bool info_visible,
                                           bool console_visible) {
    const QSignalBlocker navigation_blocker(toggle_navigation_action_);
    const QSignalBlocker info_blocker(toggle_info_action_);
    const QSignalBlocker console_blocker(toggle_console_action_);

    toggle_navigation_action_->setChecked(navigation_visible);
    toggle_info_action_->setChecked(info_visible);
    toggle_console_action_->setChecked(console_visible);
}

void AppActions::CreateActions() {
    open_action_ = new QAction(tr("Open"), this);
    open_action_->setShortcut(QKeySequence::Open);
    open_action_->setIconVisibleInMenu(false);

    save_action_ = new QAction(tr("Save"), this);
    save_action_->setShortcut(QKeySequence::Save);
    save_action_->setIconVisibleInMenu(false);

    save_as_action_ = new QAction(tr("Save As..."), this);
    save_as_action_->setShortcut(QKeySequence::SaveAs);

    start_simulation_action_ = new QAction(tr("Start Simulation"), this);

    start_mission_action_ = new QAction(tr("Start Mission"), this);

    settings_action_ = new QAction(tr("Settings..."), this);
    settings_action_->setShortcut(QKeySequence::Preferences);

    exit_action_ = new QAction(tr("Exit"), this);
    exit_action_->setShortcut(QKeySequence::Quit);

    toggle_navigation_action_ = new QAction(tr("Navigation"), this);
    toggle_navigation_action_->setCheckable(true);
    toggle_navigation_action_->setChecked(true);

    toggle_info_action_ = new QAction(tr("Info"), this);
    toggle_info_action_->setCheckable(true);
    toggle_info_action_->setChecked(true);

    toggle_console_action_ = new QAction(tr("Console"), this);
    toggle_console_action_->setCheckable(true);
    toggle_console_action_->setChecked(true);

    reset_layout_action_ = new QAction(tr("Reset Layout"), this);

    about_action_ = new QAction(tr("About"), this);
}

void AppActions::ConfigureToolTips() {
    SetActionHelp(open_action_,
                  {.tool_tip = tr("Open workspace"), .status_tip = tr("Open a workspace")});
    SetActionHelp(save_action_,
                  {.tool_tip = tr("Save workspace"),
                   .status_tip = tr("Save the current workspace")});
    SetActionHelp(save_as_action_, {.tool_tip = tr("Save as"),
                                    .status_tip =
                                        tr("Save the current workspace with a new name")});
    SetActionHelp(start_simulation_action_,
                  {.tool_tip = tr("Start simulation"),
                   .status_tip = tr("Run the expected offline mission simulation")});
    SetActionHelp(start_mission_action_,
                  {.tool_tip = tr("Start mission"),
                   .status_tip = tr("Start the mission and open live telemetry")});
    SetActionHelp(settings_action_, {.tool_tip = tr("Open settings"),
                                     .status_tip = tr("Configure application settings")});
    SetActionHelp(exit_action_,
                  {.tool_tip = tr("Exit"), .status_tip = tr("Close the application")});
    SetActionHelp(toggle_navigation_action_,
                  {.tool_tip = tr("Show or hide navigation"),
                   .status_tip = tr("Show or hide the navigation panel")});
    SetActionHelp(toggle_info_action_, {.tool_tip = tr("Show or hide info"),
                                        .status_tip = tr("Show or hide the info panel")});
    SetActionHelp(toggle_console_action_, {.tool_tip = tr("Show or hide console"),
                                           .status_tip = tr("Show or hide the console panel")});
    SetActionHelp(reset_layout_action_, {.tool_tip = tr("Reset layout"),
                                         .status_tip = tr("Restore the active layout profile")});
    SetActionHelp(about_action_,
                  {.tool_tip = tr("About"), .status_tip = tr("Show application information")});
}

void AppActions::BindIcons() {
    theme::ThemeIcons::Instance().BindAction(open_action_, QStringLiteral("open"));
    theme::ThemeIcons::Instance().BindAction(save_action_, QStringLiteral("save"));
    theme::ThemeIcons::Instance().BindAction(start_simulation_action_,
                                             QStringLiteral("mission_sim_start"));
    theme::ThemeIcons::Instance().BindAction(start_mission_action_,
                                             QStringLiteral("mission_sim_start"));
}

}  // namespace app::ui::actions
