#include "chrome/main_menu_bar.h"

#include <QAction>
#include <QMenu>

#include "actions/app_actions.h"
#include "object_names.h"

namespace app::ui::chrome {

MainMenuBar::MainMenuBar(actions::AppActions* app_actions, QWidget* parent) : QMenuBar(parent) {
    setObjectName(QString::fromLatin1(object_names::kMainMenuBar));
    setProperty("uiComponent", QStringLiteral("menu-chrome"));

    auto* file_menu = addMenu(tr("&File"));
    file_menu->addAction(app_actions->NewAction());
    file_menu->addSeparator();
    file_menu->addAction(app_actions->OpenAction());
    file_menu->addAction(app_actions->SaveAction());
    file_menu->addAction(app_actions->SaveAsAction());
    file_menu->addSeparator();
    file_menu->addAction(app_actions->SettingsAction());
    file_menu->addSeparator();
    file_menu->addAction(app_actions->ExitAction());

    auto* view_menu = addMenu(tr("&View"));
    view_menu->addAction(app_actions->ToggleNavigationAction());
    view_menu->addAction(app_actions->ToggleInfoAction());
    view_menu->addAction(app_actions->ToggleConsoleAction());
    view_menu->addSeparator();
    view_menu->addAction(app_actions->ResetLayoutAction());

    auto* help_menu = addMenu(tr("&Help"));
    help_menu->addAction(app_actions->AboutAction());
}

}  // namespace app::ui::chrome
