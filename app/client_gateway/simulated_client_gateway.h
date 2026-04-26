#pragma once

#include "app/client_gateway/client_gateway.h"
#include "app/client_gateway/simulated_drone_gateway_client.h"
#include "app/client_gateway/simulated_mission_workspace_client.h"
#include "app/client_gateway/simulated_swarm_runtime_client.h"

namespace app::client_gateway {

class SimulatedClientGateway final : public IClientGateway {
    Q_OBJECT

   public:
    explicit SimulatedClientGateway(QObject* parent = nullptr);

    [[nodiscard]] IMissionWorkspaceClient& MissionWorkspaces() override;
    [[nodiscard]] IDroneGatewayClient& DroneGateway() override;
    [[nodiscard]] ISwarmRuntimeClient& SwarmRuntime() override;

   private:
    SimulatedMissionWorkspaceClient mission_workspaces_;
    SimulatedDroneGatewayClient drone_gateway_;
    SimulatedSwarmRuntimeClient swarm_runtime_;
};

}  // namespace app::client_gateway
