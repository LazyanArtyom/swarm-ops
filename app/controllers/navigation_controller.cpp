#include "app/controllers/navigation_controller.h"

#include <QAction>
#include <QList>
#include <QMessageBox>

#include "app/app_services.h"
#include "app/client_gateway/client_gateway.h"
#include "app/client_gateway/swarm_runtime_client.h"
#include "app/mission/mission_workspace_service.h"
#include "logging/logger.h"
#include "ui/actions/app_actions.h"
#include "ui/ids.h"
#include "ui/panels/central_panel.h"
#include "ui/panels/navigation_panel.h"
#include "ui/workspace/page_registry.h"

namespace app::controllers {
namespace {

constexpr auto kMissionLogCategory = "mission";

}  // namespace

NavigationController::NavigationController(const AppServices& services,
                                           ui::NavigationPanel* navigation_panel,
                                           ui::CentralPanel* central_panel,
                                           ui::actions::AppActions* app_actions,
                                           QObject* parent)
    : QObject(parent),
      services_(services),
      navigation_panel_(navigation_panel),
      central_panel_(central_panel) {
    WireNavigation();
    WireMissionActions(app_actions);
    connect(&mission::MissionWorkspaceRuntime(), &mission::MissionWorkspaceService::SigGraphEditorRequested,
            this, [this] {
                ShowAndSelectPage(ui::ids::kPageMissionGraphEditor);
            });
}

void NavigationController::RefreshNavigationItems() {
    if (navigation_panel_ == nullptr) {
        return;
    }

    QList<ui::NavigationPanel::Item> items;
    for (const ui::workspace::PageDescriptor& page : services_.Pages().NavigationPages()) {
        items.push_back({
            .page_key = page.page_key,
            .label = page.title,
            .icon = page.icon,
            .category = page.category,
            .shortcut = page.shortcut,
        });
    }
    navigation_panel_->SetItems(items);
}

void NavigationController::OpenHome() {
    const QString page_key = services_.Pages().DefaultPageKey();
    if (page_key.isEmpty()) {
        return;
    }

    if (central_panel_ != nullptr) {
        central_panel_->ShowPage(page_key);
    }

    if (navigation_panel_ != nullptr) {
        navigation_panel_->SelectPage(page_key);
    }
}

void NavigationController::WireNavigation() {
    if (central_panel_ == nullptr || navigation_panel_ == nullptr) {
        return;
    }

    connect(navigation_panel_, &ui::NavigationPanel::SigPageRequested, this,
            [this](const QString& page_key) {
                if (central_panel_ != nullptr) {
                    central_panel_->ShowPage(page_key);
                }
            });

    connect(central_panel_, &ui::CentralPanel::SigCurrentPageChanged, this,
            [this](const QString& page_key) {
                if (navigation_panel_ != nullptr) {
                    navigation_panel_->SelectPage(page_key);
                }
            });
}

void NavigationController::WireMissionActions(ui::actions::AppActions* app_actions) {
    if (app_actions == nullptr) {
        return;
    }

    connect(app_actions->StartSimulationAction(), &QAction::triggered, this,
            &NavigationController::StartSimulation);
    connect(app_actions->StartMissionAction(), &QAction::triggered, this,
            &NavigationController::StartMission);
}

void NavigationController::ShowAndSelectPage(const QString& page_key) {
    if (central_panel_ != nullptr) {
        central_panel_->ShowPage(page_key);
    }
    if (navigation_panel_ != nullptr) {
        navigation_panel_->SelectPage(page_key);
    }
}

void NavigationController::StartSimulation() {
    ShowAndSelectPage(ui::ids::kPageMissionSimulator);
    const auto result = client_gateway::ClientGatewayRuntime().SwarmRuntime().StartMissionSimulation(
        mission::MissionWorkspaceRuntime().ActiveWorkspace());
    if (!result.ok) {
        QMessageBox::information(central_panel_, tr("Mission Simulator"), result.message);
        return;
    }

    logging::Logger::InfoFor(kMissionLogCategory, "offline mission simulation started");
}

void NavigationController::StartMission() {
    ShowAndSelectPage(ui::ids::kPageMissionLive);
    const auto result = client_gateway::ClientGatewayRuntime().SwarmRuntime().StartLiveMission(
        mission::MissionWorkspaceRuntime().ActiveWorkspace());
    if (!result.ok) {
        QMessageBox::information(central_panel_, tr("Start Mission"), result.message);
        return;
    }

    logging::Logger::InfoFor(kMissionLogCategory, "live mission started");
}

}  // namespace app::controllers
