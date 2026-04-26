#pragma once

#include "app/client_gateway/client_gateway.h"
#include "app/client_gateway/drone_gateway_client.h"
#include "app/client_gateway/mission_workspace_client.h"
#include "app/client_gateway/swarm_runtime_client.h"

namespace app::client_gateway {

class ApiGatewayMissionWorkspaceClient final : public IMissionWorkspaceClient {
    Q_OBJECT

   public:
    using IMissionWorkspaceClient::IMissionWorkspaceClient;

    void Start() override;
    void Stop() override;
    [[nodiscard]] GatewayConnectionState ConnectionState() const override;
    [[nodiscard]] QList<WorkspaceListItem> ListWorkspaces() const override;
    [[nodiscard]] mission::MissionWorkspace ActiveWorkspace() const override;
    [[nodiscard]] mission::MissionWorkspace GetWorkspaceSnapshot(
        const QString& workspace_id) const override;
    [[nodiscard]] QString CreateWorkspace(QString name,
                                          const mission::WorkspaceBackground& background) override;
    [[nodiscard]] GatewayResult SaveWorkspaceSnapshot(mission::MissionWorkspace workspace) override;
    [[nodiscard]] GatewayResult DeleteWorkspace(const QString& workspace_id) override;
    [[nodiscard]] GatewayResult JoinWorkspaceSession(const QString& workspace_id,
                                                     QString user_display_name) override;
    void LeaveWorkspaceSession() override;
    [[nodiscard]] GatewayResult ApplyWorkspaceSnapshot(
        mission::MissionWorkspace workspace, WorkspaceUpdateType update_type) override;
    [[nodiscard]] WorkspaceCheckpoint CreateCheckpoint(const QString& workspace_id,
                                                       QString name) override;
};

class ApiGatewayDroneGatewayClient final : public IDroneGatewayClient {
    Q_OBJECT

   public:
    using IDroneGatewayClient::IDroneGatewayClient;

    void Start() override;
    void Stop() override;
    [[nodiscard]] GatewayConnectionState ConnectionState() const override;
    [[nodiscard]] QList<DroneAgentEndpoint> DroneAgents() const override;
    [[nodiscard]] GatewayResult ValidateDroneAllocation(
        const mission::MissionWorkspace& workspace) const override;
};

class ApiGatewaySwarmRuntimeClient final : public ISwarmRuntimeClient {
    Q_OBJECT

   public:
    using ISwarmRuntimeClient::ISwarmRuntimeClient;

    void Start() override;
    void Stop() override;
    [[nodiscard]] GatewayConnectionState ConnectionState() const override;
    [[nodiscard]] SwarmStateSnapshot LatestSwarmState(const QString& swarm_id) const override;
    [[nodiscard]] CommandReceipt SendCommand(SwarmCommand command) override;
    [[nodiscard]] GatewayResult StartMissionSimulation(
        const mission::MissionWorkspace& workspace) override;
    void PauseMissionSimulation() override;
    void ResumeMissionSimulation() override;
    void StopMissionSimulation() override;
    void SetMissionSimulationSpeedMultiplier(double multiplier) override;
    [[nodiscard]] double MissionSimulationSpeedMultiplier() const override;
    [[nodiscard]] MissionSimulationFrame LatestMissionSimulationFrame() const override;
    [[nodiscard]] GatewayResult StartLiveMission(
        const mission::MissionWorkspace& workspace) override;
    void StopLiveMission() override;
    [[nodiscard]] MissionSimulationFrame LatestLiveMissionFrame() const override;
};

class ApiGatewayClient final : public IClientGateway {
    Q_OBJECT

   public:
    explicit ApiGatewayClient(QObject* parent = nullptr);

    [[nodiscard]] IMissionWorkspaceClient& MissionWorkspaces() override;
    [[nodiscard]] IDroneGatewayClient& DroneGateway() override;
    [[nodiscard]] ISwarmRuntimeClient& SwarmRuntime() override;

   private:
    ApiGatewayMissionWorkspaceClient mission_workspaces_;
    ApiGatewayDroneGatewayClient drone_gateway_;
    ApiGatewaySwarmRuntimeClient swarm_runtime_;
};

}  // namespace app::client_gateway
