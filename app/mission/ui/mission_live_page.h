#pragma once

#include <QWidget>

#include "app/client_gateway/client_gateway_types.h"
#include "app/mission/mission_workspace.h"

class QAction;
class QLabel;
class QToolBar;

namespace app::mission {

class SimulationView;

class MissionLivePage final : public QWidget {
    Q_OBJECT

   public:
    explicit MissionLivePage(QWidget* parent = nullptr);

   private:
    void StopMission();
    void OnWorkspaceChanged(const MissionWorkspace& workspace);
    void OnLiveMissionFrame(const client_gateway::MissionSimulationFrame& frame);
    void RefreshToolbarState(client_gateway::MissionSimulationState state);
    void RefreshStatus(const client_gateway::MissionSimulationFrame& frame);

    QToolBar* tool_strip_{nullptr};
    SimulationView* view_{nullptr};
    QAction* stop_action_{nullptr};
    QAction* fit_action_{nullptr};
    QLabel* status_label_{nullptr};
    client_gateway::MissionSimulationState mission_state_{
        client_gateway::MissionSimulationState::kIdle};
};

}  // namespace app::mission
