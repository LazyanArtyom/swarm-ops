#pragma once

#include <QObject>
#include <QString>

#include "app/app_services.h"

namespace app::ui {
class CentralPanel;
class NavigationPanel;
}  // namespace app::ui

namespace app::ui::actions {
class AppActions;
}  // namespace app::ui::actions

namespace app::controllers {

class NavigationController final : public QObject {
    Q_OBJECT

   public:
    NavigationController(const AppServices& services, ui::NavigationPanel* navigation_panel,
                         ui::CentralPanel* central_panel,
                         ui::actions::AppActions* app_actions,
                         QObject* parent = nullptr);
    ~NavigationController() override = default;

    void RefreshNavigationItems();
    void OpenHome();

   private:
    void WireNavigation();
    void WireMissionActions(ui::actions::AppActions* app_actions);
    void ShowAndSelectPage(const QString& page_key);
    void StartSimulation();
    void StartMission();

    app::AppServices services_;
    ui::NavigationPanel* navigation_panel_{nullptr};
    ui::CentralPanel* central_panel_{nullptr};
};

}  // namespace app::controllers
